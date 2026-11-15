/*------------------------------------------------------------*/
/* filename -       tvcursor.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TView::resetCursor member function        */
/*------------------------------------------------------------*/

/*------------------------------------------------------------*/
/*                                                            */
/*    Turbo Vision -  Version 1.0                             */
/*                                                            */
/*                                                            */
/*    Copyright (c) 1991 by Borland International             */
/*    All Rights Reserved.                                    */
/*                                                            */
/*    Linux port: C++ version of TVCURSOR.ASM.                */
/*                                                            */
/*------------------------------------------------------------*/

#define Uses_TView
#define Uses_TGroup
#define Uses_TScreen
#include <tv.h>

void TView::resetCursor()
{
    const ushort need = sfVisible | sfCursorVis | sfFocused;
    if( (state & need) == need )
        {
        TView *v = this;
        short x = cursor.x, y = cursor.y;
        for( ;; )
            {
            if( x < 0 || x >= v->size.x || y < 0 || y >= v->size.y )
                break;
            x += v->origin.x;
            y += v->origin.y;
            TGroup *g = v->owner;
            if( g == 0 )
                {
                // reached the top: the cursor is visible here
                TScreen::setCursorPos( x, y );
                ushort shape = TScreen::cursorLines;
                if( (state & sfCursorIns) != 0 )
                    {
                    shape &= 0x00FF;
                    if( shape == 0 )
                        shape = 7;
                    }
                TScreen::setCursorType( shape );
                return;
                }
            if( (g->state & sfVisible) == 0 )
                break;
            // is the cursor hidden by a view in front of v?
            TView *p = g->last->next;
            Boolean covered = False;
            for( ; p != v; p = p->next )
                if( (p->state & sfVisible) != 0 &&
                    y >= p->origin.y && y < p->origin.y + p->size.y &&
                    x >= p->origin.x && x < p->origin.x + p->size.x )
                    {
                    covered = True;
                    break;
                    }
            if( covered )
                break;
            v = g;
            }
        }
    TScreen::setCursorType( 0x2000 );
}
