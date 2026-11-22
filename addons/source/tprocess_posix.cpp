/*------------------------------------------------------------*/
/* filename -       tprocess_posix.cpp                        */
/*                                                            */
/* function(s)                                                */
/*                  TProcess on Linux: fork, /bin/sh -c or    */
/*                  execvp, pipes read with O_NONBLOCK        */
/*                                                            */
/*   The child gets a process group of its own (or a          */
/*   session, newSession), so stop() reaches what the shell   */
/*   started too.  Only tvproc.h is included, not tv.h.       */
/*------------------------------------------------------------*/

#if !defined( _WIN32 )

#define Uses_TProcess
#include <tvproc.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

namespace
{

void closeFd( int& fd )
{
    if( fd >= 0 )
        close( fd );
    fd = -1;
}

int statusCode( int st )
{
    if( WIFEXITED( st ) )
        return WEXITSTATUS( st );
    if( WIFSIGNALED( st ) )
        return 128 + WTERMSIG( st );
    return 255;
}

[[noreturn]] void childFail( const std::string& what )
{
    std::string msg = what + ": " + strerror( errno ) + "\n";
    if( write( 2, msg.data(), msg.size() ) < 0 )
        {}
    _exit( 127 );
}

// The terminal driver installs signal handlers and blocks some; the
// command starts with the defaults, its variables and its directory.
void prepareChild( const TProcessOptions& o )
{
    const int sigs[] = { SIGINT, SIGQUIT, SIGTERM, SIGHUP, SIGPIPE, SIGTSTP,
                         SIGTTIN, SIGTTOU, SIGWINCH, SIGCHLD, SIGSEGV,
                         SIGBUS, SIGFPE, SIGABRT };
    for( int s : sigs )
        signal( s, SIG_DFL );
    sigset_t none;
    sigemptyset( &none );
    sigprocmask( SIG_SETMASK, &none, 0 );
    for( const auto& kv : o.env )
        setenv( kv.first.c_str(), kv.second.c_str(), 1 );
    if( !o.dir.empty() && chdir( o.dir.c_str() ) != 0 )
        childFail( "cannot change to " + o.dir );
}

[[noreturn]] void execCommand( const std::vector<std::string>& argv, bool shell )
{
    if( shell )
        {
        execl( "/bin/sh", "sh", "-c", argv[0].c_str(), (char *) 0 );
        childFail( "cannot run /bin/sh" );
        }
    std::vector<char *> args;
    for( const std::string& a : argv )
        args.push_back( const_cast<char *>( a.c_str() ) );
    args.push_back( 0 );
    execvp( args[0], args.data() );
    childFail( "cannot run " + argv[0] );
}

// what the pipe has now; closed at its end
void drain( int& fd, std::string& into )
{
    char buf[4096];
    size_t total = 0;
    while( fd >= 0 && total < 65536 )
        {
        ssize_t n = ::read( fd, buf, sizeof( buf ) );
        if( n > 0 )
            {
            into.append( buf, size_t( n ) );
            total += size_t( n );
            }
        else if( n == 0 )
            closeFd( fd );
        else if( errno == EINTR )
            continue;
        else
            {
            if( errno != EAGAIN && errno != EWOULDBLOCK )
                closeFd( fd );
            break;
            }
        }
}

void ourEnd( int fd, bool nonBlocking )
{
    if( nonBlocking )
        fcntl( fd, F_SETFL, fcntl( fd, F_GETFL ) | O_NONBLOCK );
    fcntl( fd, F_SETFD, FD_CLOEXEC );
}

}   // namespace

long TProcess::clockMs()
{
    timespec t;
    clock_gettime( CLOCK_MONOTONIC, &t );
    return long( t.tv_sec ) * 1000L + t.tv_nsec / 1000000L;
}

TProcess::TProcess() :
    code( -1 ), started( false ), stopRequested( false ), done( false ), stops( 0 ),
    exitTime( 0 ), termTime( 0 ), pid( -1 ), outFd( -1 ), capFd( -1 ), inFd( -1 )
{
}

TProcess::~TProcess()
{
    if( started && !done )
        {
        kill( -pid, SIGKILL );
        int st;
        while( waitpid( pid, &st, 0 ) < 0 && errno == EINTR )
            ;
        }
    closeFd( outFd );
    closeFd( capFd );
    closeFd( inFd );
}

bool TProcess::start( const std::string& command, const TProcessOptions& options )
{
    return launch( std::vector<std::string>{ command }, true, options );
}

bool TProcess::start( const std::string& command, const std::string& dir,
                      const std::vector<std::string>& env )
{
    TProcessOptions o;
    o.dir = dir;
    for( const std::string& e : env )
        {
        size_t eq = e.find( '=' );
        if( eq != std::string::npos )
            o.env.push_back( std::make_pair( e.substr( 0, eq ), e.substr( eq + 1 ) ) );
        }
    return start( command, o );
}

bool TProcess::startArgv( const std::vector<std::string>& argv, const TProcessOptions& options )
{
    if( argv.empty() )
        {
        failure = "no program";
        return false;
        }
    return launch( argv, false, options );
}

bool TProcess::launch( const std::vector<std::string>& argv, bool shell, const TProcessOptions& o )
{
    if( started )
        {
        failure = "already started";
        return false;
        }
    // a write to a stdin the command has closed gives an error, not SIGPIPE
    signal( SIGPIPE, SIG_IGN );
    int out[2], cap[2] = { -1, -1 }, in[2];
    if( pipe( out ) != 0 )
        {
        failure = strerror( errno );
        return false;
        }
    if( pipe( in ) != 0 || ( o.captureStdout && pipe( cap ) != 0 ) )
        {
        failure = strerror( errno );
        close( out[0] );
        close( out[1] );
        return false;
        }
    pid_t p = fork();
    if( p < 0 )
        {
        failure = strerror( errno );
        close( out[0] ); close( out[1] );
        close( in[0] ); close( in[1] );
        if( o.captureStdout )
            {
            close( cap[0] );
            close( cap[1] );
            }
        return false;
        }
    if( p == 0 )
        {
        // a session of its own has no controlling terminal
        if( o.newSession )
            setsid();
        else
            setpgid( 0, 0 );
        dup2( in[0], 0 );
        dup2( o.captureStdout ? cap[1] : out[1], 1 );
        dup2( out[1], 2 );
        close( in[0] ); close( in[1] );
        close( out[0] ); close( out[1] );
        if( o.captureStdout )
            {
            close( cap[0] );
            close( cap[1] );
            }
        prepareChild( o );
        execCommand( argv, shell );
        }
    if( !o.newSession )
        setpgid( p, p );
    close( out[1] );
    close( in[0] );
    outFd = out[0];
    ourEnd( outFd, true );
    if( o.captureStdout )
        {
        close( cap[1] );
        capFd = cap[0];
        ourEnd( capFd, true );
        }
    inFd = in[1];
    ourEnd( inFd, false );
    pid = p;
    started = true;
    send( o.input );
    if( !o.keepInput )
        closeInput();
    return true;
}

std::string TProcess::read()
{
    std::string s;
    drain( outFd, s );
    return s;
}

std::string TProcess::readCaptured()
{
    std::string s;
    drain( capFd, s );
    return s;
}

bool TProcess::poll( std::string& out )
{
    drain( outFd, out );
    drain( capFd, out );
    return running();
}

bool TProcess::poll( std::string& out, std::string& captured )
{
    drain( outFd, out );
    drain( capFd, captured );
    return running();
}

// what is sent is small (a command, a password): the pipe takes it
bool TProcess::send( const std::string& data )
{
    size_t sent = 0;
    while( inFd >= 0 && sent < data.size() )
        {
        ssize_t n = write( inFd, data.data() + sent, data.size() - sent );
        if( n < 0 && errno == EINTR )
            continue;
        if( n <= 0 )
            {
            closeFd( inFd );
            return false;
            }
        sent += size_t( n );
        }
    return inFd >= 0;
}

void TProcess::closeInput()
{
    closeFd( inFd );
}

void TProcess::check()
{
    if( !started )
        return;
    if( !done )
        {
        int st;
        if( waitpid( pid, &st, WNOHANG ) == pid )
            {
            done = true;
            code = statusCode( st );
            exitTime = clockMs();
            }
        else if( termTime != 0 && clockMs() - termTime > killMs )
            {
            kill( -pid, SIGKILL );
            termTime = 0;
            }
        }
    if( done && ( outFd >= 0 || capFd >= 0 ) && clockMs() - exitTime > drainMs )
        {
        closeFd( outFd );
        closeFd( capFd );
        }
}

bool TProcess::running()
{
    if( !started )
        return false;
    check();
    return !( done && outFd < 0 && capFd < 0 );
}

bool TProcess::exited()
{
    check();
    return done;
}

void TProcess::stop()
{
    if( !started || done )
        return;
    stopRequested = true;
    if( stops++ == 0 )
        {
        kill( -pid, SIGTERM );
        termTime = clockMs();
        }
    else
        kill( -pid, SIGKILL );
}

int TProcess::runAttached( const std::string& command, const TProcessOptions& options )
{
    pid_t p = fork();
    if( p < 0 )
        return 127;
    if( p == 0 )
        {
        prepareChild( options );
        execCommand( std::vector<std::string>{ command }, true );
        }
    // Ctrl+C on the terminal is meant for the command, not for us
    struct sigaction ignore, oldInt, oldQuit;
    memset( &ignore, 0, sizeof( ignore ) );
    ignore.sa_handler = SIG_IGN;
    sigemptyset( &ignore.sa_mask );
    sigaction( SIGINT, &ignore, &oldInt );
    sigaction( SIGQUIT, &ignore, &oldQuit );
    int st = 0;
    while( waitpid( p, &st, 0 ) < 0 && errno == EINTR )
        ;
    sigaction( SIGINT, &oldInt, 0 );
    sigaction( SIGQUIT, &oldQuit, 0 );
    return statusCode( st );
}

std::string TProcess::toUtf8( const std::string& raw )
{
    return raw;
}

#endif  // !_WIN32
