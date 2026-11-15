/*------------------------------------------------------------*/
/* filename -       tvexposd.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TView::exposed member function            */
/*------------------------------------------------------------*/

/*------------------------------------------------------------*/
/*                                                            */
/*    Turbo Vision -  Version 1.0                             */
/*                                                            */
/*                                                            */
/*    Copyright (c) 1991 by Borland International             */
/*    All Rights Reserved.                                    */
/*                                                            */
/*    Linux port: C++ version of TVEXPOSD.ASM.                */
/*                                                            */
/*------------------------------------------------------------*/

#define Uses_TView
#define Uses_TGroup
#include <tv.h>

static Boolean spanExposed( TView *v, short y, short x, short cx );

// Checks [x, cx) of row y (owner coordinates of target) against the
// siblings in front of target.  Returns True if some part is visible.
static Boolean scanSiblings( TView *p, TView *target, TGroup *g,
                             short y, short x, short cx )
{
    for( ;; )
        {
        p = p->next;
        if( p == target )
            {
            // nothing in front hides it here; go on up unless the
            // owner has its own buffer (then it is "exposed")
            if( g->buffer != 0 )
                return True;
            return spanExposed( g, y, x, cx );
            }

        if( (p->state & sfVisible) == 0 ||
            y < p->origin.y || y >= p->origin.y + p->size.y )
            continue;

        short si = p->origin.x;
        if( x >= si )
            {
            si += p->size.x;
            if( x >= si )
                continue;
            x = si;
            if( x < cx )
                continue;
            return False;           // completely covered
            }

        if( cx <= si )
            continue;
        si += p->size.x;
        if( cx <= si )
            {
            cx = p->origin.x;       // right part covered
            continue;
            }

        // p lies strictly inside the span: check the left part first
        if( scanSiblings( p, target, g, y, x, p->origin.x ) )
            return True;
        x = si;
        }
}

static Boolean spanExposed( TView *v, short y, short x, short cx )
{
    y += v->origin.y;
    x += v->origin.x;
    cx += v->origin.x;
    TGroup *g = v->owner;
    if( g == 0 )
        return True;
    if( y < g->clip.a.y || y >= g->clip.b.y )
        return False;
    if( x < g->clip.a.x )
        x = g->clip.a.x;
    if( cx > g->clip.b.x )
        cx = g->clip.b.x;
    if( x >= cx )
        return False;
    return scanSiblings( g->last, v, g, y, x, cx );
}

Boolean TView::exposed()
{
    if( (state & sfExposed) == 0 || size.x <= 0 || size.y <= 0 )
        return False;
    for( short y = 0; y < size.y; y++ )
        if( spanExposed( this, y, 0, size.x ) )
            return True;
    return False;
}
