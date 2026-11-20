/*------------------------------------------------------------*/
/* filename -       input.cpp                                 */
/*                                                            */
/* function(s)                                                */
/*          terminal input decoding: escape sequences ->      */
/*          BIOS style key codes, xterm SGR mouse reports     */
/*------------------------------------------------------------*/

#include "../driver.h"

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <string>

namespace tvplatform
{

static std::string inbuf;
static long long inbufTime = 0;         // when inbuf became non-empty

static const int escTimeoutMs = 40;

long long monotonicMs()
{
    struct timespec ts;
    clock_gettime( CLOCK_MONOTONIC, &ts );
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void xtermMouse( int b, int x, int y, bool release )
{
    setShift( modsToShift( ((b & 4) ? mShift : 0) | ((b & 8) ? mAlt : 0) |
                           ((b & 16) ? mCtrl : 0) ) );
    if( b & 64 )
        {
        // wheel: 64 up, 65 down, 66 left, 67 right; one event per step,
        // the buttons stay as they are
        static const unsigned char dir[4] = { 0x01, 0x02, 0x04, 0x08 };
        MouseState ms = currentMouse();
        ms.x = short( x - 1 );
        ms.y = short( y - 1 );
        ms.wheel = dir[b & 3];
        pushMouse( ms );
        return;
        }
    unsigned char mask = 0;
    switch( b & 3 )
        {
        case 0: mask = 0x01; break;     // mbLeftButton
        case 2: mask = 0x02; break;     // mbRightButton
        case 1: mask = 0x04; break;     // middle
        }
    MouseState ms = currentMouse();
    ms.x = short( x - 1 );
    ms.y = short( y - 1 );
    if( !(b & 32) )
        {
        if( release )
            ms.buttons &= ~mask;
        else
            ms.buttons |= mask;
        }
    if( (b & 3) == 3 && !(b & 32) )
        ms.buttons = 0;                 // legacy "release" report
    ms.wheel = 0;
    pushMouse( ms );
}

static SpecialKey tildeKey( int n )
{
    switch( n )
        {
        case 1: case 7: return skHome;
        case 2: return skIns;
        case 3: return skDel;
        case 4: case 8: return skEnd;
        case 5: return skPgUp;
        case 6: return skPgDn;
        case 11: return skF1;
        case 12: return skF2;
        case 13: return skF3;
        case 14: return skF4;
        case 15: return skF5;
        case 17: return skF6;
        case 18: return skF7;
        case 19: return skF8;
        case 20: return skF9;
        case 21: return skF10;
        case 23: return skF11;
        case 24: return skF12;
        }
    return skNone;
}

static SpecialKey letterKey( char c )
{
    switch( c )
        {
        case 'A': return skUp;
        case 'B': return skDown;
        case 'C': return skRight;
        case 'D': return skLeft;
        case 'H': return skHome;
        case 'F': return skEnd;
        case 'P': return skF1;
        case 'Q': return skF2;
        case 'R': return skF3;
        case 'S': return skF4;
        }
    return skNone;
}

// Length of a UTF-8 sequence starting with byte c (1 for invalid bytes).
static size_t utf8Len( unsigned char c )
{
    if( c < 0xC0 ) return 1;
    if( c < 0xE0 ) return 2;
    if( c < 0xF0 ) return 3;
    return 4;
}

static uint32_t utf8Decode( const std::string &s, size_t pos, size_t len )
{
    unsigned char c = s[pos];
    if( len == 1 )
        return c;
    uint32_t u = c & ( 0x7F >> len );
    for( size_t i = 1; i < len; i++ )
        u = (u << 6) | ( (unsigned char)s[pos + i] & 0x3F );
    return u;
}

// Tries to decode one input item at the start of inbuf.  Returns the
// number of bytes consumed, or 0 if more bytes are needed.
static size_t decodeOne( bool timedOut )
{
    const std::string &s = inbuf;
    size_t n = s.size();
    unsigned char c0 = s[0];

    if( c0 != 0x1B )
        {
        size_t len = utf8Len( c0 );
        if( n < len )
            return timedOut ? 1 : 0;
        pushChar( utf8Decode( s, 0, len ), false );
        return len;
        }

    if( n == 1 )
        {
        if( !timedOut )
            return 0;
        pushChar( 0x1B, false );
        return 1;
        }

    char c1 = s[1];
    if( c1 == '[' || c1 == 'O' )
        {
        // CSI / SS3: ESC [ params final   (also ESC [ < mouse, ESC [ [ x)
        if( c1 == '[' && n >= 3 && s[2] == '[' )
            {
            // Linux console F1..F5: ESC [ [ A..E
            if( n < 4 )
                return timedOut ? 3 : 0;
            if( s[3] >= 'A' && s[3] <= 'E' )
                pushSpecial( SpecialKey( skF1 + s[3] - 'A' ), 0 );
            return 4;
            }
        size_t i = 2;
        while( i < n && (unsigned char)s[i] >= 0x20 &&
               (unsigned char)s[i] <= 0x3F )
            i++;
        if( i >= n )
            {
            if( !timedOut )
                return 0;
            pushChar( (unsigned char)c1, true );    // Alt-[ / Alt-O
            return 2;
            }
        char final = s[i];
        std::string params = s.substr( 2, i - 2 );
        size_t used = i + 1;

        if( !params.empty() && params[0] == '<' )
            {
            int b = 0, x = 0, y = 0;
            if( sscanf( params.c_str() + 1, "%d;%d;%d", &b, &x, &y ) == 3 &&
                ( final == 'M' || final == 'm' ) )
                xtermMouse( b, x, y, final == 'm' );
            return used;
            }

        int p1 = 1, p2 = 1;
        if( !params.empty() )
            sscanf( params.c_str(), "%d;%d", &p1, &p2 );
        int mods = p2 > 1 ? p2 - 1 : 0;

        if( final == '~' )
            {
            SpecialKey k = tildeKey( p1 );
            if( k != skNone )
                pushSpecial( k, mods );
            }
        else if( final == 'Z' )
            pushKey( 0x0F00, kLeftShift );              // kbShiftTab
        else if( c1 == 'O' && final >= 'p' && final <= 'y' )
            pushChar( (unsigned char)( '0' + final - 'p' ), false ); // keypad
        else if( c1 == 'O' && final == 'M' )
            pushChar( 0x0D, false );                    // keypad Enter
        else
            {
            SpecialKey k = letterKey( final );
            if( k != skNone )
                pushSpecial( k, mods );
            else if( final == 'E' )
                ;                                       // keypad 5
            }
        return used;
        }

    if( c1 == 0x1B )
        {
        // ESC ESC: Alt+special or a doubled Escape
        if( n >= 3 && ( s[2] == '[' || s[2] == 'O' ) )
            {
            size_t saveSize = keyCount();
            inbuf.erase( 0, 1 );
            size_t used = decodeOne( timedOut );
            if( used == 0 )
                {
                inbuf.insert( 0, 1, '\x1b' );
                return 0;
                }
            addShift( saveSize, kAlt );
            return used;                // ESC already removed from inbuf
            }
        pushChar( 0x1B, false );
        return 1;
        }

    // ESC + character = Alt+character
    size_t len = utf8Len( (unsigned char)c1 );
    if( n < 1 + len )
        return timedOut ? 1 : 0;
    pushChar( utf8Decode( s, 1, len ), true );
    return 1 + len;
}

static void decode( bool timedOut )
{
    while( !inbuf.empty() )
        {
        size_t used = decodeOne( timedOut );
        if( used == 0 )
            break;
        // decodeOne may already have shortened inbuf (ESC ESC case)
        inbuf.erase( 0, used < inbuf.size() ? used : inbuf.size() );
        inbufTime = monotonicMs();
        }
}

void waitInput( int ms )
{
    if( inputQueued() )
        ms = 0;
    if( !inbuf.empty() )
        {
        long long left = inbufTime + escTimeoutMs - monotonicMs();
        if( left < ms )
            ms = left < 0 ? 0 : int( left );
        }

    struct pollfd pfd = { STDIN_FILENO, POLLIN, 0 };
    int r = poll( &pfd, 1, ms );
    if( r > 0 && (pfd.revents & POLLIN) )
        {
        char buf[4096];
        ssize_t got;
        do  {
            got = ::read( STDIN_FILENO, buf, sizeof( buf ) );
            if( got > 0 )
                {
                if( inbuf.empty() )
                    inbufTime = monotonicMs();
                inbuf.append( buf, size_t( got ) );
                }
            } while( got == ssize_t( sizeof( buf ) ) );
        }
    decode( false );
    if( !inbuf.empty() && monotonicMs() - inbufTime >= escTimeoutMs )
        decode( true );
}

}
