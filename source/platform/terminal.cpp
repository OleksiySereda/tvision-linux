/*------------------------------------------------------------*/
/* filename -       terminal.cpp                              */
/*                                                            */
/* function(s)                                                */
/*          terminal setup/restore, window size, screen       */
/*          output (ANSI escape sequences, UTF-8)             */
/*------------------------------------------------------------*/

#include "platform.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <string>
#include <vector>

namespace tvplatform
{

static bool started = false;
static bool haveSavedTermios = false;
static struct termios savedTermios;
static volatile sig_atomic_t resizePending = 0;

static std::vector<unsigned short> shadow;      // what the terminal shows
static int shadowCols = 0, shadowRows = 0;

static int cursorX = 0, cursorY = 0;
static bool cursorVisible = false, cursorBlock = false;

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

static void writeAll( const char *s, size_t n )
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
        writeAll( enterSeq, sizeof( enterSeq ) - 1 );
        shadowCols = shadowRows = 0;
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
    writeAll( enterSeq, sizeof( enterSeq ) - 1 );
    shadowCols = shadowRows = 0;
    started = true;
}

void stop()
{
    if( !started )
        return;
    started = false;
    writeAll( leaveSeq, sizeof( leaveSeq ) - 1 );
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

void invalidate()
{
    shadowCols = shadowRows = 0;
}

void setCursor( int x, int y, bool visible, bool block )
{
    cursorX = x;
    cursorY = y;
    cursorVisible = visible;
    cursorBlock = block;
}

static void putUtf8( std::string &out, uint32_t u )
{
    if( u < 0x80 )
        out += char( u );
    else if( u < 0x800 )
        {
        out += char( 0xC0 | (u >> 6) );
        out += char( 0x80 | (u & 0x3F) );
        }
    else
        {
        out += char( 0xE0 | (u >> 12) );
        out += char( 0x80 | ((u >> 6) & 0x3F) );
        out += char( 0x80 | (u & 0x3F) );
        }
}

static void putNum( std::string &out, int n )
{
    char buf[16];
    snprintf( buf, sizeof( buf ), "%d", n );
    out += buf;
}

// CGA colour index -> ANSI colour index
static const int cgaToAnsi[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

static void putAttr( std::string &out, unsigned char attr )
{
    int fg = attr & 0x0F, bg = (attr >> 4) & 0x0F;
    out += "\x1b[0;";
    putNum( out, (fg & 8 ? 90 : 30) + cgaToAnsi[fg & 7] );
    out += ';';
    putNum( out, (bg & 8 ? 100 : 40) + cgaToAnsi[bg & 7] );
    out += 'm';
}

void flush( const unsigned short *cells, int cols, int rows )
{
    if( !started )
        return;

    std::string out;
    bool full = ( cols != shadowCols || rows != shadowRows );
    if( full )
        {
        shadow.assign( size_t( cols ) * rows, 0 );
        shadowCols = cols;
        shadowRows = rows;
        out += "\x1b[0m\x1b[2J";
        }

    int lastAttr = -1;
    int px = -1, py = -1;               // terminal cursor after output
    for( int y = 0; y < rows; y++ )
        {
        const unsigned short *src = cells + size_t( y ) * cols;
        unsigned short *dst = &shadow[size_t( y ) * cols];
        for( int x = 0; x < cols; x++ )
            {
            unsigned short cell = src[x];
            if( !full && dst[x] == cell )
                continue;
            dst[x] = cell;
            if( px != x || py != y )
                {
                out += "\x1b[";
                putNum( out, y + 1 );
                out += ';';
                putNum( out, x + 1 );
                out += 'H';
                }
            unsigned char attr = cell >> 8;
            if( attr != lastAttr )
                {
                putAttr( out, attr );
                lastAttr = attr;
                }
            putUtf8( out, toUnicode( cell & 0xFF ) );
            px = x + 1;
            py = y;
            if( px >= cols )
                px = -1;                // pending wrap: position unknown
            }
        }

    static int shownX = -1, shownY = -1;
    static bool shownVisible = false, shownBlock = false;
    if( out.empty() && shownX == cursorX && shownY == cursorY &&
        shownVisible == cursorVisible && shownBlock == cursorBlock )
        return;                         // nothing changed: stay quiet on ssh
    shownX = cursorX;
    shownY = cursorY;
    shownVisible = cursorVisible;
    shownBlock = cursorBlock;

    if( cursorVisible && cursorX >= 0 && cursorX < cols &&
        cursorY >= 0 && cursorY < rows )
        {
        out += "\x1b[";
        putNum( out, cursorY + 1 );
        out += ';';
        putNum( out, cursorX + 1 );
        out += 'H';
        out += cursorBlock ? "\x1b[2 q" : "\x1b[4 q";
        out += "\x1b[?25h";
        }
    else
        out += "\x1b[?25l";

    writeAll( out.data(), out.size() );
}

}
