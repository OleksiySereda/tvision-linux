/*------------------------------------------------------------*/
/* filename -       tprocess_win32.cpp                        */
/*                                                            */
/* function(s)                                                */
/*                  TProcess on Windows: cmd.exe /d /s /c or  */
/*                  a program and its arguments, in a job     */
/*                  object; anonymous pipes read with         */
/*                  PeekNamedPipe                             */
/*                                                            */
/*   The job ends with the process and everything it          */
/*   started; no console window is opened (CREATE_NO_WINDOW:  */
/*   ssh and the askpass it runs never write over the         */
/*   screen).  Strings are UTF-8.  Only tvproc.h is           */
/*   included, not tv.h, so that windows.h meets no Turbo     */
/*   Vision names.                                            */
/*------------------------------------------------------------*/

#if defined( _WIN32 )

#if !defined( WIN32_LEAN_AND_MEAN )
#define WIN32_LEAN_AND_MEAN
#endif
#if !defined( NOMINMAX )
#define NOMINMAX
#endif
#include <windows.h>
#include <string.h>

#define Uses_TProcess
#include <tvproc.h>

namespace
{

const int terminatedCode = 143;         // 128 + SIGTERM, as on Linux

std::wstring widen( const std::string& s )
{
    if( s.empty() )
        return std::wstring();
    int n = MultiByteToWideChar( CP_UTF8, 0, s.data(), int( s.size() ), 0, 0 );
    std::wstring w( size_t( n ), L'\0' );
    MultiByteToWideChar( CP_UTF8, 0, s.data(), int( s.size() ), &w[0], n );
    return w;
}

std::string lastError()
{
    char buf[256];
    DWORD e = GetLastError();
    DWORD n = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, e,
                              0, buf, sizeof( buf ), 0 );
    if( n == 0 )
        return "error " + std::to_string( e );
    while( n > 0 && ( buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == '.' ) )
        n--;
    return std::string( buf, n );
}

// one argument as the C runtime of the child splits it again
std::string quoteArg( const std::string& a )
{
    if( !a.empty() && a.find_first_of( " \t\"" ) == std::string::npos )
        return a;
    std::string q = "\"";
    size_t slashes = 0;
    for( char c : a )
        {
        if( c == '\\' )
            slashes++;
        else
            {
            if( c == '"' )
                q.append( slashes + 1, '\\' );
            slashes = 0;
            }
        q += c;
        }
    q.append( slashes, '\\' );
    return q + "\"";
}

// cmd.exe does not split its command line the C way: after /c the rest
// is passed as it is, in quotes that /s removes
std::string argvLine( const std::vector<std::string>& argv )
{
    bool cmd = _stricmp( argv[0].c_str(), "cmd.exe" ) == 0 || _stricmp( argv[0].c_str(), "cmd" ) == 0;
    std::string line;
    for( size_t i = 0; i < argv.size(); i++ )
        {
        if( i > 0 )
            line += ' ';
        if( cmd && i > 0 && _stricmp( argv[i - 1].c_str(), "/c" ) == 0 )
            {
            line += "\"" + argv[i] + "\"";
            for( size_t k = i + 1; k < argv.size(); k++ )
                line += " " + argv[k];
            break;
            }
        line += quoteArg( argv[i] );
        }
    return line;
}

std::wstring commandLine( const std::vector<std::string>& argv, bool shell )
{
    if( shell )
        return L"cmd.exe /d /s /c \"" + widen( argv[0] ) + L"\"";
    return widen( argvLine( argv ) );
}

// The variables are set in our own environment while the child is
// created (it inherits them), and put back after.
class EnvScope
{

public:

    EnvScope( const TProcessEnv& env )
    {
        for( const auto& kv : env )
            {
            std::wstring name = widen( kv.first );
            DWORD n = GetEnvironmentVariableW( name.c_str(), 0, 0 );
            Saved s = { name, std::wstring(), n != 0 };
            if( n != 0 )
                {
                s.value.resize( n );
                GetEnvironmentVariableW( name.c_str(), &s.value[0], n );
                s.value.resize( n - 1 );
                }
            saved.push_back( s );
            SetEnvironmentVariableW( name.c_str(), widen( kv.second ).c_str() );
            }
    }

    ~EnvScope()
    {
        for( const Saved& s : saved )
            SetEnvironmentVariableW( s.name.c_str(), s.had ? s.value.c_str() : 0 );
    }

private:

    struct Saved { std::wstring name, value; bool had; };
    std::vector<Saved> saved;
};

void closeHandle( void *&h )
{
    if( h != 0 )
        CloseHandle( (HANDLE) h );
    h = 0;
}

bool makePipe( HANDLE& rd, HANDLE& wr, bool readIsOurs )
{
    SECURITY_ATTRIBUTES sa = { sizeof( sa ), 0, TRUE };
    if( !CreatePipe( &rd, &wr, &sa, 0 ) )
        return false;
    // our end is not inherited
    SetHandleInformation( readIsOurs ? rd : wr, HANDLE_FLAG_INHERIT, 0 );
    return true;
}

void drain( void *&pipe, std::string& into )
{
    char buf[4096];
    size_t total = 0;
    while( pipe != 0 && total < 65536 )
        {
        DWORD avail = 0;
        if( !PeekNamedPipe( (HANDLE) pipe, 0, 0, 0, &avail, 0 ) )
            {
            closeHandle( pipe );            // the writers are gone
            break;
            }
        if( avail == 0 )
            break;
        DWORD n = 0;
        if( !ReadFile( (HANDLE) pipe, buf, avail < sizeof( buf ) ? avail : DWORD( sizeof( buf ) ),
                       &n, 0 ) || n == 0 )
            {
            closeHandle( pipe );
            break;
            }
        into.append( buf, n );
        total += n;
        }
}

}   // namespace

long TProcess::clockMs()
{
    return long( GetTickCount64() );
}

TProcess::TProcess() :
    code( -1 ), started( false ), stopRequested( false ), done( false ), stops( 0 ),
    exitTime( 0 ), termTime( 0 ), process( 0 ), job( 0 ), outPipe( 0 ), capPipe( 0 ), inPipe( 0 )
{
}

TProcess::~TProcess()
{
    if( started && !done && job != 0 )
        TerminateJobObject( (HANDLE) job, terminatedCode );
    closeHandle( process );
    closeHandle( job );
    closeHandle( outPipe );
    closeHandle( capPipe );
    closeHandle( inPipe );
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
    HANDLE outRead, outWrite, inRead, inWrite, capRead = 0, capWrite = 0;
    if( !makePipe( outRead, outWrite, true ) )
        {
        failure = lastError();
        return false;
        }
    if( !makePipe( inRead, inWrite, false ) )
        {
        failure = lastError();
        CloseHandle( outRead );
        CloseHandle( outWrite );
        return false;
        }
    if( o.captureStdout && !makePipe( capRead, capWrite, true ) )
        {
        failure = lastError();
        CloseHandle( outRead ); CloseHandle( outWrite );
        CloseHandle( inRead ); CloseHandle( inWrite );
        return false;
        }

    STARTUPINFOW si;
    memset( &si, 0, sizeof( si ) );
    si.cb = sizeof( si );
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = inRead;
    si.hStdOutput = o.captureStdout ? capWrite : outWrite;
    si.hStdError = outWrite;

    HANDLE j = CreateJobObjectW( 0, 0 );
    if( j != 0 )
        {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION li;
        memset( &li, 0, sizeof( li ) );
        li.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject( j, JobObjectExtendedLimitInformation, &li, sizeof( li ) );
        }

    std::wstring line = commandLine( argv, shell );
    std::wstring wdir = widen( o.dir );
    PROCESS_INFORMATION pi;
    BOOL ok;
        {
        EnvScope scope( o.env );
        ok = CreateProcessW( 0, &line[0], 0, 0, TRUE,
                             CREATE_NO_WINDOW | CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT,
                             0, wdir.empty() ? 0 : wdir.c_str(), &si, &pi );
        }
    if( !ok )
        failure = lastError();
    CloseHandle( outWrite );
    CloseHandle( inRead );
    if( capWrite != 0 )
        CloseHandle( capWrite );
    if( !ok )
        {
        CloseHandle( outRead );
        CloseHandle( inWrite );
        if( capRead != 0 )
            CloseHandle( capRead );
        if( j != 0 )
            CloseHandle( j );
        return false;
        }
    if( j != 0 )
        AssignProcessToJobObject( j, pi.hProcess );
    ResumeThread( pi.hThread );
    CloseHandle( pi.hThread );

    process = pi.hProcess;
    job = j;
    outPipe = outRead;
    capPipe = capRead;
    inPipe = inWrite;
    started = true;
    send( o.input );
    if( !o.keepInput )
        closeInput();
    return true;
}

std::string TProcess::read()
{
    std::string s;
    drain( outPipe, s );
    return s;
}

std::string TProcess::readCaptured()
{
    std::string s;
    drain( capPipe, s );
    return s;
}

bool TProcess::poll( std::string& out )
{
    drain( outPipe, out );
    drain( capPipe, out );
    return running();
}

bool TProcess::poll( std::string& out, std::string& captured )
{
    drain( outPipe, out );
    drain( capPipe, captured );
    return running();
}

// small writes (a command, a password): the pipe buffer takes them
bool TProcess::send( const std::string& data )
{
    size_t sent = 0;
    while( inPipe != 0 && sent < data.size() )
        {
        DWORD n = 0;
        if( !WriteFile( (HANDLE) inPipe, data.data() + sent, DWORD( data.size() - sent ), &n, 0 ) ||
            n == 0 )
            {
            closeHandle( inPipe );
            return false;
            }
        sent += n;
        }
    return inPipe != 0;
}

void TProcess::closeInput()
{
    closeHandle( inPipe );
}

void TProcess::check()
{
    if( !started )
        return;
    if( !done && WaitForSingleObject( (HANDLE) process, 0 ) == WAIT_OBJECT_0 )
        {
        DWORD c = 255;
        GetExitCodeProcess( (HANDLE) process, &c );
        done = true;
        code = termTime != 0 ? terminatedCode : int( c );
        exitTime = clockMs();
        }
    if( done && ( outPipe != 0 || capPipe != 0 ) && clockMs() - exitTime > drainMs )
        {
        closeHandle( outPipe );
        closeHandle( capPipe );
        }
}

bool TProcess::running()
{
    if( !started )
        return false;
    check();
    return !( done && outPipe == 0 && capPipe == 0 );
}

bool TProcess::exited()
{
    check();
    return done;
}

// the job and everything in it end at once
void TProcess::stop()
{
    if( !started || done )
        return;
    stopRequested = true;
    stops++;
    if( termTime == 0 )
        termTime = clockMs();
    if( job != 0 )
        TerminateJobObject( (HANDLE) job, terminatedCode );
    else
        TerminateProcess( (HANDLE) process, terminatedCode );
}

int TProcess::runAttached( const std::string& command, const TProcessOptions& options )
{
    std::wstring line = commandLine( std::vector<std::string>{ command }, true );
    std::wstring wdir = widen( options.dir );
    STARTUPINFOW si;
    memset( &si, 0, sizeof( si ) );
    si.cb = sizeof( si );
    PROCESS_INFORMATION pi;
    BOOL ok;
        {
        EnvScope scope( options.env );
        ok = CreateProcessW( 0, &line[0], 0, 0, TRUE, CREATE_UNICODE_ENVIRONMENT, 0,
                             wdir.empty() ? 0 : wdir.c_str(), &si, &pi );
        }
    if( !ok )
        return 127;
    CloseHandle( pi.hThread );
    // Ctrl+C in the console is meant for the command, not for us
    SetConsoleCtrlHandler( 0, TRUE );
    WaitForSingleObject( pi.hProcess, INFINITE );
    SetConsoleCtrlHandler( 0, FALSE );
    DWORD c = 255;
    GetExitCodeProcess( pi.hProcess, &c );
    CloseHandle( pi.hProcess );
    return int( c );
}

static bool isUtf8( const std::string& s )
{
    return s.empty() ||
           MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), int( s.size() ), 0, 0 ) > 0;
}

std::string TProcess::toUtf8( const std::string& raw )
{
    if( isUtf8( raw ) )
        return raw;
    int n = MultiByteToWideChar( CP_OEMCP, 0, raw.data(), int( raw.size() ), 0, 0 );
    std::wstring w( size_t( n ), L'\0' );
    MultiByteToWideChar( CP_OEMCP, 0, raw.data(), int( raw.size() ), &w[0], n );
    int m = WideCharToMultiByte( CP_UTF8, 0, w.data(), n, 0, 0, 0, 0 );
    std::string out( size_t( m ), '\0' );
    WideCharToMultiByte( CP_UTF8, 0, w.data(), n, &out[0], m, 0, 0 );
    return out;
}

#endif  // _WIN32
