/*------------------------------------------------------------*/
/* filename -       framelin.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TFrame::frameLine member function         */
/*------------------------------------------------------------*/

/*------------------------------------------------------------*/
/*                                                            */
/*    Turbo Vision -  Version 1.0                             */
/*                                                            */
/*                                                            */
/*    Copyright (c) 1991 by Borland International             */
/*    All Rights Reserved.                                    */
/*                                                            */
/*    Linux port: C++ version of FRAMELIN.ASM.                */
/*                                                            */
/*------------------------------------------------------------*/

#define Uses_TFrame
#define Uses_TGroup
#define Uses_TDrawBuffer
#include <tv.h>

/*
    Builds one row of the frame.  Each cell gets a bit mask (left/right/
    up/down line segments) from initFrame, then the frames of the views
    in front of this one (ofFramed) add their junctions; frameChars maps
    the mask to the box-drawing character.
*/

void TFrame::frameLine( TDrawBuffer& frameBuf, short y, short n, uchar color )
{
    short w = size.x;
    if( w <= 0 )
        return;
    if( w > maxViewWidth )
        w = maxViewWidth;

    uchar mask[maxViewWidth];
    mask[0] = initFrame[n];
    for( short i = 1; i < w - 1; i++ )
        mask[i] = initFrame[n+1];
    if( w > 1 )
        mask[w-1] = initFrame[n+2];

    short right = w - 1;
    for( TView *p = owner->last->next; p != this; p = p->next )
        {
        if( (p->options & ofFramed) == 0 || (p->state & sfVisible) == 0 )
            continue;

        ushort bits;
        short dy = y - p->origin.y;
        if( dy < 0 )
            {
            if( dy != -1 )
                continue;
            bits = 0x0A06;          // row just above p
            }
        else if( dy > p->size.y )
            continue;
        else if( dy < p->size.y )
            bits = 0x0005;          // row crossing p
        else
            bits = 0x0A03;          // row just below p

        short si = p->origin.x;
        short di = si + p->size.x;
        if( si < 1 )
            si = 1;
        if( di > right )
            di = right;
        if( si >= di )
            continue;

        uchar lo = uchar( bits ), hi = uchar( bits >> 8 );
        mask[si-1] |= lo;
        mask[di] |= uchar( lo ^ hi );
        if( hi != 0 )
            for( short i = si; i < di; i++ )
                mask[i] |= hi;
        }

    for( short i = 0; i < w; i++ )
        frameBuf.data[i] = ushort( uchar( frameChars[mask[i]] ) | ( color << 8 ) );
}
