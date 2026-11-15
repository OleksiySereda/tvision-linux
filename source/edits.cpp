/*------------------------------------------------------------*/
/* filename -       edits.cpp                                 */
/*                                                            */
/* function(s)                                                */
/*                  TEditor buffer access member functions    */
/*------------------------------------------------------------*/

/*------------------------------------------------------------*/
/*                                                            */
/*    Turbo Vision -  Version 1.0                             */
/*                                                            */
/*                                                            */
/*    Copyright (c) 1991 by Borland International             */
/*    All Rights Reserved.                                    */
/*                                                            */
/*    Linux port: C++ version of EDITS.ASM.  The editor       */
/*    buffer has a gap at curPtr; line ends are CR LF         */
/*    (TFileEditor converts LF files on load and save).       */
/*                                                            */
/*------------------------------------------------------------*/

#define Uses_TEditor
#include <tv.h>

char TEditor::bufChar( ushort p )
{
    return buffer[p < curPtr ? p : p + gapLen];
}

ushort TEditor::bufPtr( ushort p )
{
    return p < curPtr ? p : ushort( p + gapLen );
}

void TEditor::formatLine( void *drawBuf, ushort linePtr, int width,
                          ushort colors )
{
    ushort *d = (ushort *)drawBuf;
    int x = 0;
    ushort p = linePtr;                 // physical buffer index
    ushort attr = 0;
    Boolean done = False;

    // copies buffer[p .. limit) with the given attribute; stops at CR or
    // when the line is full
    auto fill = [&]( uchar a, ushort limit )
        {
        attr = ushort( a << 8 );
        while( !done && p < limit )
            {
            uchar c = buffer[p++];
            if( c == '\r' )
                done = True;
            else if( c == '\t' )
                {
                do  {
                    d[x++] = ushort( ' ' | attr );
                    } while( (x & 7) != 0 && x < width );
                }
            else
                d[x++] = ushort( c | attr );
            if( x >= width )
                done = True;
            }
        };

    fill( uchar( colors ), selStart );
    fill( uchar( colors >> 8 ), curPtr );
    if( p < curPtr )
        p = curPtr;
    p += gapLen;
    fill( uchar( colors >> 8 ), ushort( selEnd + gapLen ) );
    fill( uchar( colors ), bufSize );

    while( x < width )
        d[x++] = ushort( ' ' | attr );
}

ushort TEditor::lineEnd( ushort p )
{
    for( ; p < curPtr; p++ )
        if( buffer[p] == '\r' )
            return p;
    for( ; p < bufLen; p++ )
        if( buffer[p + gapLen] == '\r' )
            return p;
    return bufLen;
}

ushort TEditor::lineStart( ushort p )
{
    long i = long( p ) - 1;
    for( ; i >= 0; i-- )
        if( bufChar( ushort( i ) ) == '\r' )
            break;
    if( i < 0 )
        return 0;
    ushort r = ushort( i + 1 );
    if( r != bufLen && bufChar( r ) == '\n' )
        r++;
    return r;
}

ushort TEditor::nextChar( ushort p )
{
    if( p == bufLen )
        return p;
    p++;
    if( p == bufLen )
        return p;
    if( bufChar( p - 1 ) == '\r' && bufChar( p ) == '\n' )
        p++;
    return p;
}

ushort TEditor::prevChar( ushort p )
{
    if( p == 0 )
        return p;
    p--;
    if( p == 0 )
        return p;
    if( bufChar( p - 1 ) == '\r' && bufChar( p ) == '\n' )
        p--;
    return p;
}
