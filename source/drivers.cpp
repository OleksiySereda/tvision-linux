/*------------------------------------------------------------------------*/
/* filename - drivers.cpp                                                 */
/*                                                                        */
/* function(s)                                                            */
/*        moveBuf  --   moves a buffer of char/attribute pairs            */
/*        moveChar --   sets a buffer with a char/attribute pair          */
/*        moveCStr --   moves a char array into a buffer & adds an        */
/*                      attribute to each char                            */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/*                                                                        */
/*    Turbo Vision -  Version 1.0                                         */
/*                                                                        */
/*                                                                        */
/*    Copyright (c) 1991 by Borland International                         */
/*    All Rights Reserved.                                                */
/*                                                                        */
/*    Linux port: C++ versions of the original inline assembly.           */
/*                                                                        */
/*------------------------------------------------------------------------*/

#define Uses_TDrawBuffer
#include <tv.h>

/*------------------------------------------------------------------------*/
/*                                                                        */
/*  TDrawBuffer::moveBuf                                                  */
/*                                                                        */
/*  arguments:                                                            */
/*                                                                        */
/*      indent - character position within the buffer where the data      */
/*               is to go                                                 */
/*                                                                        */
/*      source - pointer to an array of characters                        */
/*                                                                        */
/*      attr   - attribute to be used for all characters (0 to retain     */
/*               the attribute already in the buffer)                     */
/*                                                                        */
/*      count   - number of characters to move                            */
/*                                                                        */
/*------------------------------------------------------------------------*/

void TDrawBuffer::moveBuf( ushort indent, const void *source,
                           ushort attr, ushort count )

{
    const uchar *s = (const uchar *)source;
    ushort *d = &data[indent];
    uchar a = uchar( attr );
    if( a != 0 )
        while( count-- )
            *d++ = ushort( *s++ | (a << 8) );
    else
        while( count-- )
            {
            loByte( *d ) = *s++;
            d++;
            }
}

/*------------------------------------------------------------------------*/
/*                                                                        */
/*  TDrawBuffer::moveChar                                                 */
/*                                                                        */
/*  arguments:                                                            */
/*                                                                        */
/*      indent  - character position within the buffer where the data     */
/*                is to go                                                */
/*                                                                        */
/*      c       - character to be put into the buffer (0 to retain the    */
/*                character already there)                                */
/*                                                                        */
/*      attr    - attribute to be put into the buffer (0 to retain the    */
/*                attribute already there)                                */
/*                                                                        */
/*      count   - number of character/attribute pairs to put into the     */
/*                buffer                                                  */
/*                                                                        */
/*------------------------------------------------------------------------*/

void TDrawBuffer::moveChar( ushort indent, char c, ushort attr, ushort count )
{
    ushort *d = &data[indent];
    uchar ch = uchar( c ), a = uchar( attr );
    if( ch != 0 && a != 0 )
        while( count-- )
            *d++ = ushort( ch | (a << 8) );
    else if( ch != 0 )
        while( count-- )
            {
            loByte( *d ) = ch;
            d++;
            }
    else
        while( count-- )
            {
            hiByte( *d ) = a;
            d++;
            }
}

/*------------------------------------------------------------------------*/
/*                                                                        */
/*  TDrawBuffer::moveCStr                                                 */
/*                                                                        */
/*  arguments:                                                            */
/*                                                                        */
/*      indent  - character position within the buffer where the data     */
/*                is to go                                                */
/*                                                                        */
/*      str     - pointer to a 0-terminated string of characters to       */
/*                be moved into the buffer                                */
/*                                                                        */
/*      attrs   - pair of text attributes to be put into the buffer       */
/*                with each character in the string.  Initially the       */
/*                low byte is used, and a '~' in the string toggles       */
/*                between the low byte and the high byte.                 */
/*                                                                        */
/*------------------------------------------------------------------------*/

void TDrawBuffer::moveCStr( ushort indent, const char *str, ushort attrs )
{
    ushort *d = &data[indent];
    uchar a = uchar( attrs ), other = uchar( attrs >> 8 );
    for( const uchar *s = (const uchar *)str; *s != 0; s++ )
        {
        if( *s == '~' )
            {
            uchar t = a;
            a = other;
            other = t;
            }
        else if( d < data + maxViewWidth )
            *d++ = ushort( *s | (a << 8) );
        }
}

/*------------------------------------------------------------------------*/
/*                                                                        */
/*  TDrawBuffer::moveStr                                                  */
/*                                                                        */
/*  arguments:                                                            */
/*                                                                        */
/*      indent  - character position within the buffer where the data     */
/*                is to go                                                */
/*                                                                        */
/*      str     - pointer to a 0-terminated string of characters to       */
/*                be moved into the buffer                                */
/*                                                                        */
/*      attr    - text attribute to be put into the buffer with each      */
/*                character in the string.                                */
/*                                                                        */
/*------------------------------------------------------------------------*/

void TDrawBuffer::moveStr( ushort indent, const char *str, ushort attr )
{
    ushort *d = &data[indent];
    uchar a = uchar( attr );
    for( const uchar *s = (const uchar *)str;
         *s != 0 && d < data + maxViewWidth; s++ )
        *d++ = ushort( *s | (a << 8) );
}
