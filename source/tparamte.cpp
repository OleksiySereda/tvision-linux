/*------------------------------------------------------------*/
/* filename -       tparamte.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TParamText member functions               */
/*------------------------------------------------------------*/

/*------------------------------------------------------------*/
/*                                                            */
/*    Turbo Vision -  Version 1.0                             */
/*                                                            */
/*                                                            */
/*    Copyright (c) 1991 by Borland International             */
/*    All Rights Reserved.                                    */
/*                                                            */
/*------------------------------------------------------------*/

#define Uses_TParamText
#include <tv.h>

#include <string.h>

#if !defined( __STDIO_H )
#include <stdio.h>
#endif  // __STDIO_H

TParamText::TParamText( const TRect& bounds,
                        const char *aText,
                        int aParamCount ) :
    TStaticText(bounds, aText),
    paramList( 0 ),
    paramCount( aParamCount )
{
}

ushort TParamText::dataSize()
{
    return paramCount * sizeof(long);
}

/*
    Linux port: the DOS version handed the data record to vsprintf() as
    its va_list.  That is not possible with the x86-64 calling convention,
    so the format is interpreted here: every parameter occupies one
    long-sized slot of the record, holding either a long (%d %i %u %x %X
    %o %c, with or without 'l') or a pointer (%s).
*/

void TParamText::getText( char *s )
{
    *s = EOS;
    if( text == 0 )
        return;
    const long *arg = (const long *)paramList;
    int used = 0;
    char *out = s;
    for( const char *f = text; *f != EOS; )
        {
        if( *f != '%' )
            {
            *out++ = *f++;
            continue;
            }
        if( f[1] == '%' )
            {
            *out++ = '%';
            f += 2;
            continue;
            }
        // copy the conversion spec, dropping length modifiers
        char spec[32];
        int n = 0;
        spec[n++] = *f++;
        while( *f != EOS && strchr( "-+ #0123456789.", *f ) && n < 24 )
            spec[n++] = *f++;
        while( *f == 'l' || *f == 'h' )
            f++;
        char conv = *f;
        if( conv == EOS )
            break;
        f++;
        if( arg == 0 || used >= paramCount )
            continue;
        if( conv == 's' )
            {
            spec[n++] = 's';
            spec[n] = EOS;
            const char *str = (const char *)(intptr_t)arg[used++];
            out += sprintf( out, spec, str ? str : "" );
            }
        else
            {
            spec[n++] = 'l';
            spec[n++] = conv;
            spec[n] = EOS;
            out += sprintf( out, spec, arg[used++] );
            }
        }
    *out = EOS;
}

void TParamText::setData( void *rec )
{
    paramList = rec;
}

void TParamText::write( opstream& os )
{
    TStaticText::write( os );
    os << paramCount;
}

void *TParamText::read( ipstream& is )
{
    TStaticText::read( is );
    is >> paramCount;
    paramList = 0;
    return this;
}

TStreamable *TParamText::build()
{
    return new TParamText( streamableInit );
}

TParamText::TParamText( StreamableInit ) : TStaticText( streamableInit )
{
}


