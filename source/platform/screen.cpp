/*------------------------------------------------------------*/
/* filename -       screen.cpp                                */
/*                                                            */
/* function(s)                                                */
/*          screen output with ANSI escape sequences and      */
/*          UTF-8, for both systems (Windows 10 and later     */
/*          take them in the console as well)                 */
/*------------------------------------------------------------*/

#include "driver.h"

#include <stdio.h>

#include <string>
#include <vector>

namespace tvplatform
{

static std::vector<unsigned short> shadow;      // what the terminal shows
static int shadowCols = 0, shadowRows = 0;

static int cursorX = 0, cursorY = 0;
static bool cursorVisible = false, cursorBlock = false;

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
    if( !isStarted() )
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

    writeOut( out.data(), out.size() );
}

}
