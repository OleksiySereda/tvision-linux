/*------------------------------------------------------------*/
/* filename -       terminal.cpp                              */
/*                                                            */
/* function(s)                                                */
/*          POSIX terminal: setup/restore (termios, signals), */
/*          window size, writing                              */
/*------------------------------------------------------------*/

#include "../driver.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace tvplatform
{

static bool started = false;
static bool haveSavedTermios = false;
static struct termios savedTermios;
static volatile sig_atomic_t resizePending = 0;

static const char enterSeq[] =
    "\x1b[?1049h"       // alternate screen
    "\x1b[?7l"          // no auto-wrap
    "\x1b[?25l"         // hide cursor
    "\x1b[?1002h"       // mouse: button + drag events
    "\x1b[?1006h"       // mouse: SGR coordinates
    "\x1b[0m\x1b[2J";

static const char leaveSeq[] =
    "\x1b[?1006l\x1b[?1002l"
    "\x1b[0m\x1b[0 q\x1b[?25h\x1b[?7h"
    "\x1b[?1049l";

void writeOut( const char *s, size_t n )
{
    while( n > 0 )
        {
        ssize_t w = ::write( STDOUT_FILENO, s, n );
        if( w < 0 )
            {
            if( errno == EINTR || errno == EAGAIN )
                continue;
            return;
            }
        s += w;
        n -= size_t( w );
        }
}

static void setRawMode()
{
    struct termios t = savedTermios;
    t.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
                   ICRNL | IXON | IXOFF);
    t.c_oflag &= ~OPOST;
    t.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    t.c_cflag &= ~(CSIZE | PARENB);
    t.c_cflag |= CS8;
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 0;
    tcsetattr( STDIN_FILENO, TCSAFLUSH, &t );
}

static void fatalSignal( int sig )
{
    stop();
    signal( sig, SIG_DFL );
    raise( sig );
}

static void winchHandler( int )
{
    resizePending = 1;
}

static void contHandler( int )
{
    if( started )
        {
        setRawMode();
        writeOut( enterSeq, sizeof( enterSeq ) - 1 );
        invalidate();
        resizePending = 1;          // forces a full repaint
        }
}

static void atExitHandler()
{
    stop();
}

void start()
{
    if( started )
        return;
    if( !isatty( STDIN_FILENO ) || !isatty( STDOUT_FILENO ) )
        {
        fprintf( stderr, "Turbo Vision: stdin/stdout must be a terminal\n" );
        exit( 1 );
        }

    static bool handlersInstalled = false;
    if( !handlersInstalled )
        {
        handlersInstalled = true;
        atexit( atExitHandler );
        struct sigaction sa;
        memset( &sa, 0, sizeof( sa ) );
        sigemptyset( &sa.sa_mask );
        sa.sa_handler = winchHandler;
        sigaction( SIGWINCH, &sa, 0 );
        sa.sa_handler = contHandler;
        sigaction( SIGCONT, &sa, 0 );
        sa.sa_handler = fatalSignal;
        sa.sa_flags = SA_RESETHAND;
        const int fatal[] = { SIGTERM, SIGHUP, SIGINT, SIGQUIT,
                              SIGSEGV, SIGBUS, SIGFPE, SIGABRT };
        for( int sig : fatal )
            sigaction( sig, &sa, 0 );
        signal( SIGPIPE, SIG_IGN );
        }

    if( tcgetattr( STDIN_FILENO, &savedTermios ) == 0 )
        haveSavedTermios = true;
    setRawMode();
    writeOut( enterSeq, sizeof( enterSeq ) - 1 );
    invalidate();
    started = true;
}

void stop()
{
    if( !started )
        return;
    started = false;
    writeOut( leaveSeq, sizeof( leaveSeq ) - 1 );
    if( haveSavedTermios )
        tcsetattr( STDIN_FILENO, TCSAFLUSH, &savedTermios );
}

bool isStarted()
{
    return started;
}

void getSize( int &cols, int &rows )
{
    struct winsize ws;
    cols = rows = 0;
    if( ioctl( STDOUT_FILENO, TIOCGWINSZ, &ws ) == 0 )
        {
        cols = ws.ws_col;
        rows = ws.ws_row;
        }
    if( cols <= 0 || rows <= 0 )
        {
        const char *c = getenv( "COLUMNS" ), *l = getenv( "LINES" );
        cols = c ? atoi( c ) : 0;
        rows = l ? atoi( l ) : 0;
        }
    if( cols <= 0 || rows <= 0 )
        {
        cols = 80;
        rows = 25;
        }
}

bool consumeResize()
{
    if( resizePending )
        {
        resizePending = 0;
        return true;
        }
    return false;
}

}
