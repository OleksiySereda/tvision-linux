/*------------------------------------------------------------*/
/* filename -       tprocess_win32.cpp                        */
/*                                                            */
/* function(s)                                                */
/*                  TProcess on Windows: cmd.exe /d /s /c in  */
/*                  a job object, an anonymous pipe read      */
/*                  with PeekNamedPipe                        */
/*                                                            */
/*   The job ends with the process and everything it          */
/*   started; no console window is opened.  Only tvproc.h     */
/*   is included, not tv.h, so that windows.h meets no        */
/*   Turbo Vision names.                                      */
/*------------------------------------------------------------*/

#if defined( _WIN32 )

#if !defined( WIN32_LEAN_AND_MEAN )
#define WIN32_LEAN_AND_MEAN
#endif
#if !defined( NOMINMAX )
#define NOMINMAX
#endif
#include <windows.h>

#define Uses_TProcess
#include <tvproc.h>

static std::wstring widen( const std::string& s )
{
    if( s.empty() )
        return std::wstring();
    int n = MultiByteToWideChar( CP_UTF8, 0, s.data(), int( s.size() ), 0, 0 );
    std::wstring w( size_t( n ), L'\0' );
    MultiByteToWideChar( CP_UTF8, 0, s.data(), int( s.size() ), &w[0], n );
    return w;
}

static std::string lastError()
{
    char buf[256];
    DWORD n = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0,
                              GetLastError(), 0, buf, sizeof( buf ), 0 );
    while( n > 0 && ( buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == '.' ) )
        n--;
    return std::string( buf, n );
}

TProcess::TProcess() :
    code( -1 ), started( false ), stopRequested( false ), eof( false ), stops( 0 ),
    process( 0 ), job( 0 ), pipe( 0 )
{
}

TProcess::~TProcess()
{
    if( job != 0 )
        {
        if( started && code < 0 )
            TerminateJobObject( (HANDLE) job, 143 );
        CloseHandle( (HANDLE) job );
        }
    if( process != 0 )
        CloseHandle( (HANDLE) process );
    if( pipe != 0 )
        CloseHandle( (HANDLE) pipe );
}

bool TProcess::start( const std::string& command, const std::string& dir,
                      const std::vector<std::string>& env )
{
    if( started )
        {
        failure = "already started";
        return false;
        }
    SECURITY_ATTRIBUTES sa = { sizeof( sa ), 0, TRUE };
    HANDLE rd, wr;
    if( !CreatePipe( &rd, &wr, &sa, 0 ) )
        {
        failure = lastError();
        return false;
        }
    SetHandleInformation( rd, HANDLE_FLAG_INHERIT, 0 );
    HANDLE nul = CreateFileW( L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
                              OPEN_EXISTING, 0, 0 );

    STARTUPINFOW si;
    ZeroMemory( &si, sizeof( si ) );
    si.cb = sizeof( si );
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = nul;
    si.hStdOutput = wr;
    si.hStdError = wr;

    // cmd.exe /s takes what is between the first and the last quote as
    // it is
    std::wstring line = L"cmd.exe /d /s /c \"" + widen( command ) + L"\"";
    std::wstring wdir = widen( dir );

    // the variables for the child only: set here for the moment, then
    // put back
    std::vector<std::pair<std::wstring, std::wstring> > saved;
    std::vector<bool> had;
    for( const std::string& e : env )
        {
        size_t eq = e.find( '=' );
        if( eq == std::string::npos )
            continue;
        std::wstring name = widen( e.substr( 0, eq ) );
        wchar_t old[32768];
        DWORD n = GetEnvironmentVariableW( name.c_str(), old, 32768 );
        saved.push_back( std::make_pair( name, n > 0 && n < 32768 ? std::wstring( old, n )
                                                                   : std::wstring() ) );
        had.push_back( n > 0 && n < 32768 );
        SetEnvironmentVariableW( name.c_str(), widen( e.substr( eq + 1 ) ).c_str() );
        }

    PROCESS_INFORMATION pi;
    ZeroMemory( &pi, sizeof( pi ) );
    BOOL ok = CreateProcessW( 0, &line[0], 0, 0, TRUE,
                              CREATE_NO_WINDOW | CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT,
                              0, wdir.empty() ? 0 : wdir.c_str(), &si, &pi );
    DWORD err = GetLastError();

    for( size_t i = 0; i < saved.size(); i++ )
        SetEnvironmentVariableW( saved[i].first.c_str(), had[i] ? saved[i].second.c_str() : 0 );

    CloseHandle( wr );
    if( nul != INVALID_HANDLE_VALUE )
        CloseHandle( nul );
    if( !ok )
        {
        SetLastError( err );
        failure = lastError();
        CloseHandle( rd );
        return false;
        }

    HANDLE j = CreateJobObjectW( 0, 0 );
    if( j != 0 )
        {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION li;
        ZeroMemory( &li, sizeof( li ) );
        li.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject( j, JobObjectExtendedLimitInformation, &li, sizeof( li ) );
        AssignProcessToJobObject( j, pi.hProcess );
        }
    ResumeThread( pi.hThread );
    CloseHandle( pi.hThread );

    process = pi.hProcess;
    job = j;
    pipe = rd;
    started = true;
    return true;
}

std::string TProcess::read()
{
    std::string out;
    if( pipe == 0 || eof )
        return out;
    char buf[4096];
    for( ;; )
        {
        DWORD avail = 0;
        if( !PeekNamedPipe( (HANDLE) pipe, 0, 0, 0, &avail, 0 ) )
            {
            eof = true;                 // broken: everyone has closed it
            break;
            }
        if( avail == 0 || out.size() > 65536 )
            break;
        DWORD got = 0;
        if( !ReadFile( (HANDLE) pipe, buf, avail < sizeof( buf ) ? avail : DWORD( sizeof( buf ) ),
                       &got, 0 ) || got == 0 )
            {
            eof = true;
            break;
            }
        out.append( buf, got );
        }
    return out;
}

bool TProcess::finished()
{
    if( !started || code >= 0 )
        return true;
    if( WaitForSingleObject( (HANDLE) process, 0 ) != WAIT_OBJECT_0 )
        return false;
    DWORD c = 1;
    GetExitCodeProcess( (HANDLE) process, &c );
    code = int( c );
    return true;
}

bool TProcess::running()
{
    if( !started )
        return false;
    bool done = finished();
    return !( done && eof );
}

void TProcess::stop()
{
    if( !started || code >= 0 )
        return;
    stopRequested = true;
    stops++;
    if( job != 0 )
        TerminateJobObject( (HANDLE) job, 143 );
    else
        TerminateProcess( (HANDLE) process, 143 );
}

static bool isUtf8( const std::string& s )
{
    return MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), int( s.size() ),
                                0, 0 ) > 0 || s.empty();
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
