/*------------------------------------------------------------*/
/* filename -       keys.cpp                                  */
/*                                                            */
/* function(s)                                                */
/*          the key and mouse queues of the terminal driver:  */
/*          characters and special keys -> BIOS style key     */
/*          codes, for both systems                           */
/*------------------------------------------------------------*/

#include "driver.h"

#include <string.h>

#include <deque>

namespace tvplatform
{

static std::deque<KeyEvent> keyQueue;
static std::deque<MouseState> mouseQueue;
static MouseState mouseNow = { 0, 0, 0, 0 };
static unsigned short lastShift = 0;

unsigned short ticks()
{
    return (unsigned short)( monotonicMs() * 182 / 10000 );
}

// ---- BIOS scan codes ------------------------------------------------------

// US keyboard: scan code of each printable ASCII character
unsigned char asciiScan( unsigned char c )
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

void pushKey( unsigned short code, unsigned short shift )
{
    KeyEvent ev = { code, shift };
    keyQueue.push_back( ev );
}

unsigned short modsToShift( int mods )
{
    unsigned short s = 0;
    if( mods & mShift ) s |= kLeftShift;
    if( mods & mAlt )   s |= kAlt;
    if( mods & mCtrl )  s |= kCtrl;
    return s;
}

// A plain (possibly Alt-prefixed) character arriving from the terminal.
void pushChar( uint32_t u, bool alt )
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

void pushSpecial( SpecialKey k, int mods )
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

size_t keyCount()
{
    return keyQueue.size();
}

void addShift( size_t from, unsigned short shift )
{
    for( size_t k = from; k < keyQueue.size(); k++ )
        keyQueue[k].shift |= shift;
}

void pushMouse( const MouseState &ms )
{
    if( ms.wheel != 0 )
        {
        // one event per wheel step; the buttons and mouseNow stay
        if( mouseQueue.size() < 64 )
            mouseQueue.push_back( ms );
        return;
        }
    mouseNow = ms;
    if( mouseQueue.size() >= 64 )
        mouseQueue.pop_back();
    mouseQueue.push_back( ms );
}

void setShift( unsigned short shift )
{
    lastShift = shift;
}

bool inputQueued()
{
    return !keyQueue.empty() || !mouseQueue.empty();
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
