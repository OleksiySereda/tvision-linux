/*------------------------------------------------------------*/
/* filename -       ttextconv.cpp                             */
/*                                                            */
/* function(s)                                                */
/*                  toScreenText, screenChar, stripAnsi       */
/*------------------------------------------------------------*/

#define Uses_TextConv
#include <tvaddons.h>

// the code page of the screen, from the terminal driver of the port
#include "../../source/platform/platform.h"

int screenChar( unsigned long u )
{
    if( u < 0x80 )
        return int( u );
    if( u == 0xFEFF )
        return -1;                      // a byte order mark
    int b = tvplatform::fromUnicode( uint32_t( u ) );
    if( b >= 0 )
        return b;
    switch( u )
        {
        case 0x2010: case 0x2011: case 0x2012: case 0x2013: case 0x2014: case 0x2212:
            return '-';
        case 0x2018: case 0x2019: case 0x201A: case 0x2032:
            return '\'';
        case 0x201C: case 0x201D: case 0x201E: case 0x00AB: case 0x00BB:
            return '"';
        case 0x2026:
            return '.';
        case 0x00A0: case 0x2002: case 0x2003: case 0x2009: case 0x202F:
            return ' ';
        case 0x2022: case 0x25CF:
            return '*';
        case 0x2192:
            return '>';
        case 0x2190:
            return '<';
        }
    return '?';
}

std::string toScreenText( const std::string& s )
{
    std::string out;
    out.reserve( s.size() );
    size_t i = 0, n = s.size();
    while( i < n )
        {
        unsigned char c = uchar( s[i] );
        if( c < 0x80 )
            {
            out += char( c );
            i++;
            continue;
            }
        int len = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 0;
        unsigned long u = len == 4 ? c & 0x07 : len == 3 ? c & 0x0F : c & 0x1F;
        bool ok = len > 0 && i + size_t( len ) <= n;
        for( int k = 1; ok && k < len; k++ )
            {
            unsigned char d = uchar( s[i + size_t( k )] );
            if( ( d & 0xC0 ) != 0x80 )
                ok = false;
            else
                u = ( u << 6 ) | ( d & 0x3F );
            }
        if( !ok )
            {
            out += char( c );           // not UTF-8: the byte itself
            i++;
            continue;
            }
        int b = screenChar( u );
        if( b >= 0 )
            out += char( b );
        i += size_t( len );
        }
    return out;
}

std::string fromScreenText( const std::string& s )
{
    std::string out;
    out.reserve( s.size() );
    for( unsigned char c : s )
        {
        uint32_t u = c < 0x80 ? c : tvplatform::toUnicode( c );
        if( u < 0x80 )
            out += char( u );
        else if( u < 0x800 )
            {
            out += char( 0xC0 | ( u >> 6 ) );
            out += char( 0x80 | ( u & 0x3F ) );
            }
        else if( u < 0x10000 )
            {
            out += char( 0xE0 | ( u >> 12 ) );
            out += char( 0x80 | ( ( u >> 6 ) & 0x3F ) );
            out += char( 0x80 | ( u & 0x3F ) );
            }
        else
            {
            out += char( 0xF0 | ( u >> 18 ) );
            out += char( 0x80 | ( ( u >> 12 ) & 0x3F ) );
            out += char( 0x80 | ( ( u >> 6 ) & 0x3F ) );
            out += char( 0x80 | ( u & 0x3F ) );
            }
        }
    return out;
}

std::string stripAnsi( const std::string& s )
{
    std::string out;
    out.reserve( s.size() );
    for( size_t i = 0; i < s.size(); i++ )
        {
        if( s[i] == '\x1B' && i + 1 < s.size() && s[i + 1] == '[' )
            {
            size_t k = i + 2;
            while( k < s.size() && !( s[k] >= 0x40 && s[k] <= 0x7E ) )
                k++;
            i = k;                      // the final byte too
            continue;
            }
        if( s[i] == '\x1B' && i + 1 < s.size() && s[i + 1] == ']' )
            {
            // OSC ... BEL or ESC backslash (window titles)
            size_t k = i + 2;
            while( k < s.size() && s[k] != '\x07' && !( s[k] == '\x1B' && k + 1 < s.size() ) )
                k++;
            if( k < s.size() && s[k] == '\x1B' )
                k++;
            i = k;
            continue;
            }
        out += s[i];
    }
    return out;
}
