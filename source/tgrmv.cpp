/*------------------------------------------------------------*/
/* filename -       tgrmv.cpp                                 */
/*                                                            */
/* function(s)                                                */
/*                  TGroup::removeView member function        */
/*------------------------------------------------------------*/

/*------------------------------------------------------------*/
/*                                                            */
/*    Turbo Vision -  Version 1.0                             */
/*                                                            */
/*                                                            */
/*    Copyright (c) 1991 by Borland International             */
/*    All Rights Reserved.                                    */
/*                                                            */
/*    Linux port: C++ version of TGRMV.ASM.                   */
/*                                                            */
/*------------------------------------------------------------*/

#define Uses_TGroup
#include <tv.h>

void TGroup::removeView( TView *p )
{
    if( last == 0 )
        return;
    TView *s = last;
    for( ;; )
        {
        TView *n = s->next;
        if( n == p )
            {
            s->next = p->next;
            if( p == last )
                last = ( p->next == p ) ? 0 : s;
            return;
            }
        if( n == last )
            return;                     // p is not in this group
        s = n;
        }
}
