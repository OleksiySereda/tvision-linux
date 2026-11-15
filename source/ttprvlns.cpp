/*------------------------------------------------------------*/
/* filename -       ttprvlns.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TTerminal::prevLines member function      */
/*------------------------------------------------------------*/

/*------------------------------------------------------------*/
/*                                                            */
/*    Turbo Vision -  Version 1.0                             */
/*                                                            */
/*                                                            */
/*    Copyright (c) 1991 by Borland International             */
/*    All Rights Reserved.                                    */
/*                                                            */
/*    Linux port: C++ version of TTPRVLNS.ASM.                */
/*                                                            */
/*------------------------------------------------------------*/

#define Uses_TTerminal
#include <tv.h>

// Returns the start of the line `lines' lines before the one containing
// `pos' in the circular buffer, never going back past queBack.

ushort TTerminal::prevLines( ushort pos, ushort lines )
{
    const char lineSeparator = '\n';

    if( lines == 0 )
        {
        // as in the original: step over the separator pair
        pos = ushort( (pos + 1) % bufSize );
        return ushort( (pos + 1) % bufSize );
        }
    if( pos == queBack )
        return queBack;

    long di = pos == 0 ? bufSize - 1 : pos - 1;
    for( ;; )
        {
        long limit = di >= queBack ? queBack : 0;
        long i = di;
        while( i >= limit && buffer[i] != lineSeparator )
            i--;
        if( i < limit )
            {
            // no separator down to limit
            if( limit == queBack || queBack == 0 )
                return queBack;
            di = bufSize - 1;
            continue;
            }
        if( --lines == 0 )
            return ushort( (i + 1) % bufSize );
        if( i == queBack )
            return queBack;
        di = i - 1;
        if( di < 0 )
            {
            if( queBack == 0 )
                return 0;
            di = bufSize - 1;
            }
        }
}
