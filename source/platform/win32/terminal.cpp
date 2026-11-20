/*------------------------------------------------------------*/
/* filename -       terminal.cpp                              */
/*                                                            */
/* function(s)                                                */
/*          Windows console: setup/restore (console modes,    */
/*          UTF-8, escape sequences), window size, writing    */
/*------------------------------------------------------------*/

#include "../driver.h"

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

namespace tvplatform
{

static bool started = false;
static HANDLE hIn = INVALID_HANDLE_VALUE, hOut = INVALID_HANDLE_VALUE;
static DWORD savedInMode = 0, savedOutMode = 0;
static UINT savedOutCP = 0, savedInCP = 0;
static int lastCols = 0, lastRows = 0;

// as on Linux, without the mouse: the console reports it as input events
static const char enterSeq[] =
    "\x1b[?1049h"       // alternate screen
    "\x1b[?7l"          // no auto-wrap
    "\x1b[?25l"         // hide cursor
    "\x1b[0m\x1b[2J";

static const char leaveSeq[] =
    "\x1b[0m\x1b[0 q\x1b[?25h\x1b[?7h"
    "\x1b[?1049l";

long long monotonicMs()
{
    return (long long)GetTickCount64();
}

void writeOut( const char *s, size_t n )
{
    while( n > 0 )
        {
        DWORD w = 0;
        if( !WriteFile( hOut, s, DWORD( n ), &w, 0 ) || w == 0 )
            return;
        s += w;
        n -= w;
        }
}

// Ctrl+C is a key (ENABLE_PROCESSED_INPUT is off); Ctrl+Break and closing
// the window end the program, with the console restored
static BOOL WINAPI ctrlHandler( DWORD type )
{
    if( type == CTRL_C_EVENT )
        return TRUE;
    stop();
    return FALSE;
}

static void atExitHandler()
{
    stop();
}

void start()
{
    if( started )
        return;
    // the console itself when stdin or stdout is redirected
    hIn = GetStdHandle( STD_INPUT_HANDLE );
    if( !GetConsoleMode( hIn, &savedInMode ) )
        hIn = CreateFileA( "CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           0, OPEN_EXISTING, 0, 0 );
    hOut = GetStdHandle( STD_OUTPUT_HANDLE );
    if( !GetConsoleMode( hOut, &savedOutMode ) )
        hOut = CreateFileA( "CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            0, OPEN_EXISTING, 0, 0 );
    if( !GetConsoleMode( hIn, &savedInMode ) || !GetConsoleMode( hOut, &savedOutMode ) )
        {
        fprintf( stderr, "Turbo Vision: stdin/stdout must be a console\n" );
        exit( 1 );
        }

    static bool handlersInstalled = false;
    if( !handlersInstalled )
        {
        handlersInstalled = true;
        atexit( atExitHandler );
        SetConsoleCtrlHandler( ctrlHandler, TRUE );
        }

    if( !SetConsoleMode( hOut, ENABLE_PROCESSED_OUTPUT |
                               ENABLE_VIRTUAL_TERMINAL_PROCESSING |
                               DISABLE_NEWLINE_AUTO_RETURN ) )
        {
        fprintf( stderr, "Turbo Vision: the console does not take escape "
                         "sequences (Windows 10 or later is needed)\n" );
        exit( 1 );
        }
    // the mouse and the window size as events; no quick edit selection,
    // no line editing, no echo
    SetConsoleMode( hIn, ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS );

    savedOutCP = GetConsoleOutputCP();
    savedInCP = GetConsoleCP();
    SetConsoleOutputCP( CP_UTF8 );
    SetConsoleCP( CP_UTF8 );

    writeOut( enterSeq, sizeof( enterSeq ) - 1 );
    invalidate();
    getSize( lastCols, lastRows );
    started = true;
}

void stop()
{
    if( !started )
        return;
    started = false;
    writeOut( leaveSeq, sizeof( leaveSeq ) - 1 );
    SetConsoleOutputCP( savedOutCP );
    SetConsoleCP( savedInCP );
    SetConsoleMode( hOut, savedOutMode );
    SetConsoleMode( hIn, savedInMode );
}

bool isStarted()
{
    return started;
}

// the visible window, not the whole scroll back buffer
void getSize( int &cols, int &rows )
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    HANDLE h = hOut != INVALID_HANDLE_VALUE ? hOut : GetStdHandle( STD_OUTPUT_HANDLE );
    cols = rows = 0;
    if( GetConsoleScreenBufferInfo( h, &info ) )
        {
        cols = info.srWindow.Right - info.srWindow.Left + 1;
        rows = info.srWindow.Bottom - info.srWindow.Top + 1;
        }
    if( cols <= 0 || rows <= 0 )
        {
        cols = 80;
        rows = 25;
        }
}

// the console reports a new size as WINDOW_BUFFER_SIZE_EVENT, but not
// always when only the window changes: the size is compared instead
bool consumeResize()
{
    if( !started )
        return false;
    int cols, rows;
    getSize( cols, rows );
    if( cols == lastCols && rows == lastRows )
        return false;
    lastCols = cols;
    lastRows = rows;
    return true;
}

// the top line of the window in the buffer (mouse positions are in the
// buffer)
int windowTop()
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    if( GetConsoleScreenBufferInfo( hOut, &info ) )
        return info.srWindow.Top;
    return 0;
}

HANDLE inputHandle()
{
    return hIn;
}

}
