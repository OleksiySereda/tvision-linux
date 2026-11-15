/*------------------------------------------------------------*/
/* filename -       tvwrite.cpp                               */
/*                                                            */
/* function(s)                                                */
/*                  TView write member functions              */
/*------------------------------------------------------------*/

/*------------------------------------------------------------*/
/*                                                            */
/*    Turbo Vision -  Version 1.0                             */
/*                                                            */
/*                                                            */
/*    Copyright (c) 1991 by Borland International             */
/*    All Rights Reserved.                                    */
/*                                                            */
/*    Linux port: C++ version of TVWRITE.ASM.                 */
/*                                                            */
/*------------------------------------------------------------*/

#define Uses_TView
#define Uses_TGroup
#include <tv.h>

extern TPoint shadowSize;
extern uchar shadowAttr;

/*
    The span [x, cx) of row y, in the coordinates of `target's owner,
    is checked against the siblings in front of `target'.  A sibling that
    covers part of the span splits it; the parts in a sibling's shadow are
    written with shadowAttr.  What survives is copied into the owner's
    buffer (if it has one) and, unless the owner is locked, passed on to
    the owner's owner.

    `offset' is the x coordinate (in the current coordinate system) of
    the first cell of `buf'; `shadow' counts the shadows covering the span.
*/

static void writeUp( TView *v, short y, short x, short cx, short offset,
                     int shadow, const ushort *buf );

static void copyToBuffer( TGroup *g, short y, short x, short cx,
                          short offset, int shadow, const ushort *buf )
{
    ushort *dst = g->buffer + y * g->size.x + x;
    const ushort *src = buf + ( x - offset );
    short n = cx - x;
    if( shadow == 0 )
        memcpy( dst, src, n * sizeof( ushort ) );
    else
        for( short i = 0; i < n; i++ )
            dst[i] = ushort( ( src[i] & 0x00FF ) | ( shadowAttr << 8 ) );
}

static void scanSiblings( TView *p, TView *target, TGroup *g,
                          short y, short x, short cx, short offset,
                          int shadow, const ushort *buf )
{
    for( ;; )
        {
        p = p->next;
        if( p == target )
            {
            // nothing in front covers [x, cx) any more
            if( g->buffer != 0 )
                copyToBuffer( g, y, x, cx, offset, shadow, buf );
            if( g->lockFlag == 0 )
                writeUp( g, y, x, cx, offset, shadow, buf );
            return;
            }

        if( (p->state & sfVisible) == 0 || y < p->origin.y )
            continue;

        short si;
        if( y < p->origin.y + p->size.y )
            {
            // row passes through p
            si = p->origin.x;
            if( x < si )
                {
                if( cx <= si )
                    continue;
                scanSiblings( p, target, g, y, x, si, offset, shadow, buf );
                x = si;
                }
            si += p->size.x;
            if( x < si )
                {
                if( cx <= si )
                    return;             // the rest is hidden by p
                x = si;
                }
            if( (p->state & sfShadow) == 0 ||
                y < p->origin.y + shadowSize.y )
                continue;
            si += shadowSize.x;         // right-hand shadow
            }
        else
            {
            // row is below p: maybe in its bottom shadow
            if( (p->state & sfShadow) == 0 ||
                y >= p->origin.y + p->size.y + shadowSize.y )
                continue;
            si = p->origin.x + shadowSize.x;
            if( x < si )
                {
                if( cx <= si )
                    continue;
                scanSiblings( p, target, g, y, x, si, offset, shadow, buf );
                x = si;
                }
            si += p->size.x;
            }

        // [x, si) lies in p's shadow
        if( x >= si )
            continue;
        if( cx <= si )
            {
            shadow++;                   // the whole rest is shadowed
            continue;
            }
        scanSiblings( p, target, g, y, x, si, offset, shadow + 1, buf );
        x = si;
        }
}

// Translates the span from v's coordinates to its owner's and clips it.
static void writeUp( TView *v, short y, short x, short cx, short offset,
                     int shadow, const ushort *buf )
{
    if( (v->state & sfVisible) == 0 || v->owner == 0 )
        return;
    y += v->origin.y;
    x += v->origin.x;
    cx += v->origin.x;
    offset += v->origin.x;

    TGroup *g = v->owner;
    if( y < g->clip.a.y || y >= g->clip.b.y )
        return;
    if( x < g->clip.a.x )
        x = g->clip.a.x;
    if( cx > g->clip.b.x )
        cx = g->clip.b.x;
    if( x >= cx )
        return;

    scanSiblings( g->last, v, g, y, x, cx, offset, shadow, buf );
}

void TView::writeView( short x, short y, short count, const void *b )
{
    if( y < 0 || y >= size.y )
        return;
    short offset = x;
    short cx = x + count;
    if( x < 0 )
        x = 0;
    if( cx > size.x )
        cx = size.x;
    if( x >= cx )
        return;
    writeUp( this, y, x, cx, offset, 0, (const ushort *)b );
}

void TView::writeBuf( short x, short y, short w, short h, const void *buf )
{
    const ushort *b = (const ushort *)buf;
    for( ; h > 0; h--, y++, b += w )
        writeView( x, y, w, b );
}

void TView::writeChar( short x, short y, char c, uchar color, short count )
{
    if( count <= 0 )
        return;
    if( count > maxViewWidth )
        count = maxViewWidth;
    ushort cell = ushort( uchar( c ) | ( mapColor( color ) << 8 ) );
    ushort b[maxViewWidth];
    for( short i = 0; i < count; i++ )
        b[i] = cell;
    writeView( x, y, count, b );
}

void TView::writeLine( short x, short y, short w, short h, const void *buf )
{
    for( ; h > 0; h--, y++ )
        writeView( x, y, w, buf );
}

void TView::writeStr( short x, short y, const char *str, uchar color )
{
    if( str == 0 )
        return;
    size_t len = strlen( str );
    if( len == 0 )
        return;
    if( len > size_t( maxViewWidth ) )
        len = maxViewWidth;
    ushort attr = ushort( mapColor( color ) << 8 );
    ushort b[maxViewWidth];
    for( size_t i = 0; i < len; i++ )
        b[i] = ushort( uchar( str[i] ) | attr );
    writeView( x, y, short( len ), b );
}
