/*------------------------------------------------------------*/
/* filename -       tprocess_posix.cpp                        */
/*                                                            */
/* function(s)                                                */
/*                  TProcess on Linux: fork, /bin/sh -c, a    */
/*                  pipe read with O_NONBLOCK                 */
/*                                                            */
/*   The child gets a process group of its own, so stop()     */
/*   reaches what the shell started too.                      */
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
#include <unistd.h>

TProcess::TProcess() :
    code( -1 ), started( false ), stopRequested( false ), eof( false ), stops( 0 ),
    pid( -1 ), fd( -1 )
{
}

TProcess::~TProcess()
{
    if( started && code < 0 )
        {
        kill( -pid, SIGKILL );
        waitpid( pid, 0, 0 );
        }
    if( fd >= 0 )
        close( fd );
}

bool TProcess::start( const std::string& command, const std::string& dir,
                      const std::vector<std::string>& env )
{
    if( started )
        {
        failure = "already started";
        return false;
        }
    int p[2];
    if( pipe( p ) != 0 )
        {
        failure = strerror( errno );
        return false;
        }
    pid = fork();
    if( pid < 0 )
        {
        failure = strerror( errno );
        close( p[0] );
        close( p[1] );
        return false;
        }
    if( pid == 0 )
        {
        setpgid( 0, 0 );
        int in = open( "/dev/null", O_RDONLY );
        if( in >= 0 )
            dup2( in, 0 );
        dup2( p[1], 1 );
        dup2( p[1], 2 );
        close( p[0] );
        close( p[1] );
        signal( SIGINT, SIG_DFL );
        signal( SIGTERM, SIG_DFL );
        signal( SIGPIPE, SIG_DFL );
        for( const std::string& e : env )
            putenv( strdup( e.c_str() ) );
        if( !dir.empty() && chdir( dir.c_str() ) != 0 )
            {
            const char *m = "cannot change to the directory\n";
            ssize_t r = write( 2, m, strlen( m ) );
            (void) r;
            _exit( 127 );
            }
        execl( "/bin/sh", "sh", "-c", command.c_str(), (char *) 0 );
        _exit( 127 );
        }
    setpgid( pid, pid );
    close( p[1] );
    fd = p[0];
    fcntl( fd, F_SETFL, fcntl( fd, F_GETFL ) | O_NONBLOCK );
    fcntl( fd, F_SETFD, FD_CLOEXEC );
    started = true;
    return true;
}

std::string TProcess::read()
{
    std::string out;
    if( fd < 0 || eof )
        return out;
    char buf[4096];
    for( ;; )
        {
        ssize_t n = ::read( fd, buf, sizeof( buf ) );
        if( n > 0 )
            {
            out.append( buf, size_t( n ) );
            if( out.size() > 65536 )
                break;                  // the rest next time
            continue;
            }
        if( n == 0 )
            eof = true;
        else if( errno == EINTR )
            continue;
        break;
        }
    return out;
}

bool TProcess::finished()
{
    if( !started )
        return true;
    if( code >= 0 )
        return true;
    int status;
    pid_t r = waitpid( pid, &status, WNOHANG );
    if( r == pid )
        {
        if( WIFEXITED( status ) )
            code = WEXITSTATUS( status );
        else if( WIFSIGNALED( status ) )
            code = 128 + WTERMSIG( status );
        else
            code = 1;
        return true;
        }
    return false;
}

bool TProcess::running()
{
    if( !started )
        return false;
    // the output may still come after the shell itself has ended (from
    // what it started in the background): the pipe decides
    bool done = finished();
    return !( done && eof );
}

void TProcess::stop()
{
    if( !started || code >= 0 )
        return;
    stopRequested = true;
    kill( -pid, stops++ == 0 ? SIGTERM : SIGKILL );
}

std::string TProcess::toUtf8( const std::string& raw )
{
    return raw;
}

#endif  // !_WIN32
