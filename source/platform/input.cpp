/*------------------------------------------------------------*/
/* filename -       input.cpp                                 */
/*                                                            */
/* function(s)                                                */
/*          terminal input decoding: escape sequences ->      */
/*          BIOS style key codes, xterm SGR mouse reports     */
/*------------------------------------------------------------*/

#include "platform.h"

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <deque>
#include <string>

namespace tvplatform
{

// Shift state bits, as in the BIOS keyboard flags byte (tkeys.h)
const unsigned short kRightShift = 0x0001, kLeftShift = 0x0002,
                     kCtrl = 0x0004, kAlt = 0x0008;

// Modifier bits of the xterm "1;<m>" parameter (m - 1)
const int mShift = 1, mAlt = 2, mCtrl = 4;

static std::string inbuf;
static long long inbufTime = 0;         // when inbuf became non-empty
static std::deque<KeyEvent> keyQueue;
static std::deque<MouseState> mouseQueue;
static MouseState mouseNow = { 0, 0, 0 };
static unsigned short lastShift = 0;

static const int escTimeoutMs = 40;

static long long nowMs()
{
    struct timespec ts;
    clock_gettime( CLOCK_MONOTONIC, &ts );
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

unsigned short ticks()
{
    return (unsigned short)( nowMs() * 182 / 10000 );
}

// ---- BIOS scan codes ------------------------------------------------------

// US keyboard: scan code of each printable ASCII character
static unsigned char asciiScan( unsigned char c )
{
    static const char *const rows[] =
        {
        "1!", "2@", "3#", "4$", "5%", "6^", "7&", "8*", "9(", "0)", "-_", "=+"
        };
    static const char letters1[] = "qwertyuiop[]";   // 0x10..
    static const char letters2[] = "asdfghjkl;'`";   // 0x1E..
    static const char letters3[] = "\\zxcvbnm,./";   // 0x2B..
    if( c == ' ' )
        return 0x39;
    for( int i = 0; i < 12; i++ )
        if( c == rows[i][0] || c == rows[i][1] )
            return 0x02 + i;
    unsigned char l = ( c >= 'A' && c <= 'Z' ) ? c + 32 : c;
    const char *p;
    if( (p = strchr( letters1, l )) != 0 && l )
        return 0x10 + (p - letters1);
    if( (p = strchr( letters2, l )) != 0 && l )
        return 0x1E + (p - letters2);
    if( (p = strchr( letters3, l )) != 0 && l )
        return 0x2B + (p - letters3);
    switch( c )
        {
        case '{': return 0x1A;
        case '}': return 0x1B;
        case ':': return 0x27;
        case '"': return 0x28;
        case '~': return 0x29;
        case '|': return 0x2B;
        case '<': return 0x33;
        case '>': return 0x34;
        case '?': return 0x35;
        }
    return 0;
}

// Cyrillic letters (Ukrainian/Russian JCUKEN layout) -> the Latin key at
// the same position, so that Alt+hotkeys work with a Cyrillic layout.
static int cyrillicToLatin( uint32_t u )
{
    static const struct { uint16_t lo, up; char key; } map[] =
        {
        { 0x0439, 0x0419, 'q' }, { 0x0446, 0x0426, 'w' }, { 0x0443, 0x0423, 'e' },
        { 0x043A, 0x041A, 'r' }, { 0x0435, 0x0415, 't' }, { 0x043D, 0x041D, 'y' },
        { 0x0433, 0x0413, 'u' }, { 0x0448, 0x0428, 'i' }, { 0x0449, 0x0429, 'o' },
        { 0x0437, 0x0417, 'p' }, { 0x0444, 0x0424, 'a' }, { 0x0456, 0x0406, 's' },
        { 0x044B, 0x042B, 's' }, { 0x0432, 0x0412, 'd' }, { 0x0430, 0x0410, 'f' },
        { 0x043F, 0x041F, 'g' }, { 0x0440, 0x0420, 'h' }, { 0x043E, 0x041E, 'j' },
        { 0x043B, 0x041B, 'k' }, { 0x0434, 0x0414, 'l' }, { 0x044F, 0x042F, 'z' },
        { 0x0447, 0x0427, 'x' }, { 0x0441, 0x0421, 'c' }, { 0x043C, 0x041C, 'v' },
        { 0x0438, 0x0418, 'b' }, { 0x0442, 0x0422, 'n' }, { 0x044C, 0x042C, 'm' },
        };
    for( const auto &m : map )
        if( u == m.lo || u == m.up )
            return m.key;
    return -1;
}

static void pushKey( unsigned short code, unsigned short shift )
{
    KeyEvent ev = { code, shift };
    keyQueue.push_back( ev );
}

static unsigned short modsToShift( int mods )
{
    unsigned short s = 0;
    if( mods & mShift ) s |= kLeftShift;
    if( mods & mAlt )   s |= kAlt;
    if( mods & mCtrl )  s |= kCtrl;
    return s;
}

// A plain (possibly Alt-prefixed) character arriving from the terminal.
static void pushChar( uint32_t u, bool alt )
{
    if( alt )
        {
        int c = int( u );
        if( u >= 0x400 )
            c = cyrillicToLatin( u );
        if( c >= 'A' && c <= 'Z' )
            c += 32;
        unsigned short code = 0;
        if( c >= 'a' && c <= 'z' )
            code = asciiScan( (unsigned char)c ) << 8;
        else if( c >= '1' && c <= '9' )
            code = (0x78 + c - '1') << 8;
        else if( c == '0' )
            code = 0x8100;
        else if( c == '-' )
            code = 0x8200;
        else if( c == '=' )
            code = 0x8300;
        else if( c == ' ' )
            code = 0x0200;              // kbAltSpace
        else if( c == 0x7F || c == 0x08 )
            code = 0x0E00;              // Alt-Backspace
        else if( c == '\r' )
            code = 0x1C00;
        if( code )
            pushKey( code, kAlt );
        return;
        }

    switch( u )
        {
        case 0x0D: pushKey( 0x1C0D, 0 ); return;        // kbEnter
        case 0x0A: pushKey( 0x1C0A, kCtrl ); return;    // kbCtrlEnter
        case 0x09: pushKey( 0x0F09, 0 ); return;        // kbTab
        case 0x7F:
        case 0x08: pushKey( 0x0E08, 0 ); return;        // kbBack
        case 0x1B: pushKey( 0x011B, 0 ); return;        // kbEsc
        case 0x00: return;                              // Ctrl-Space
        }
    if( u < 0x20 )
        {
        // Ctrl+letter: scan code of the letter, ASCII control char
        pushKey( (asciiScan( (unsigned char)( u + 'a' - 1 ) ) << 8) | u,
                 kCtrl );
        return;
        }
    if( u < 0x80 )
        {
        pushKey( (asciiScan( (unsigned char)u ) << 8) | u, 0 );
        return;
        }
    int b = fromUnicode( u );
    if( b >= 0 )
        pushKey( (unsigned short)b, 0 );
}

enum SpecialKey
{
    skUp, skDown, skRight, skLeft, skHome, skEnd, skIns, skDel,
    skPgUp, skPgDn, skF1, skF2, skF3, skF4, skF5, skF6, skF7, skF8,
    skF9, skF10, skF11, skF12, skNone
};

static void pushSpecial( SpecialKey k, int mods )
{
    unsigned short shift = modsToShift( mods );
    bool ctrl = mods & mCtrl, alt = mods & mAlt, sh = mods & mShift;
    unsigned short code = 0;

    if( k >= skF1 && k <= skF10 )
        {
        int n = k - skF1;
        code = ( alt ? 0x68 : ctrl ? 0x5E : sh ? 0x54 : 0x3B ) + n;
        code <<= 8;
        }
    else if( k == skF11 || k == skF12 )
        {
        int n = k - skF11;
        code = ( alt ? 0x8B : ctrl ? 0x89 : sh ? 0x87 : 0x85 ) + n;
        code <<= 8;
        }
    else switch( k )
        {
        case skUp:    code = ctrl ? 0x8D00 : alt ? 0x9800 : 0x4800; break;
        case skDown:  code = ctrl ? 0x9100 : alt ? 0xA000 : 0x5000; break;
        case skLeft:  code = ctrl ? 0x7300 : alt ? 0x9B00 : 0x4B00; break;
        case skRight: code = ctrl ? 0x7400 : alt ? 0x9D00 : 0x4D00; break;
        case skHome:  code = ctrl ? 0x7700 : alt ? 0x9700 : 0x4700; break;
        case skEnd:   code = ctrl ? 0x7500 : alt ? 0x9F00 : 0x4F00; break;
        case skPgUp:  code = ctrl ? 0x8400 : alt ? 0x9900 : 0x4900; break;
        case skPgDn:  code = ctrl ? 0x7600 : alt ? 0xA100 : 0x5100; break;
        case skIns:   code = sh ? 0x0500 : ctrl ? 0x0400 : 0x5200; break;
        case skDel:   code = sh ? 0x0700 : ctrl ? 0x0600 : 0x5300; break;
        default:      return;
        }
    pushKey( code, shift );
}

static void pushMouse( int b, int x, int y, bool release )
{
    if( b & 64 )
        return;                         // wheel: not supported by TV 1.0
    unsigned char mask = 0;
    switch( b & 3 )
        {
        case 0: mask = 0x01; break;     // mbLeftButton
        case 2: mask = 0x02; break;     // mbRightButton
        case 1: mask = 0x04; break;     // middle
        }
    MouseState ms = mouseNow;
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
    lastShift = modsToShift( ((b & 4) ? mShift : 0) | ((b & 8) ? mAlt : 0) |
                             ((b & 16) ? mCtrl : 0) );
    mouseNow = ms;
    if( mouseQueue.size() >= 64 )
        mouseQueue.pop_back();
    mouseQueue.push_back( ms );
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
                pushMouse( b, x, y, final == 'm' );
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
            size_t saveSize = keyQueue.size();
            inbuf.erase( 0, 1 );
            size_t used = decodeOne( timedOut );
            if( used == 0 )
                {
                inbuf.insert( 0, 1, '\x1b' );
                return 0;
                }
            for( size_t k = saveSize; k < keyQueue.size(); k++ )
                {
                keyQueue[k].shift |= kAlt;
                }
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
        inbufTime = nowMs();
        }
}

void waitInput( int ms )
{
    if( !keyQueue.empty() || !mouseQueue.empty() )
        ms = 0;
    if( !inbuf.empty() )
        {
        long long left = inbufTime + escTimeoutMs - nowMs();
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
                    inbufTime = nowMs();
                inbuf.append( buf, size_t( got ) );
                }
            } while( got == ssize_t( sizeof( buf ) ) );
        }
    decode( false );
    if( !inbuf.empty() && nowMs() - inbufTime >= escTimeoutMs )
        decode( true );
}

bool getKey( KeyEvent &ev )
{
    if( keyQueue.empty() )
        return false;
    ev = keyQueue.front();
    keyQueue.pop_front();
    lastShift = ev.shift;
    return true;
}

bool getMouse( MouseState &ms )
{
    if( mouseQueue.empty() )
        return false;
    ms = mouseQueue.front();
    mouseQueue.pop_front();
    return true;
}

MouseState currentMouse()
{
    return mouseNow;
}

unsigned short shiftState()
{
    return lastShift;
}

}
