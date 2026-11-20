/*-------------------------------------------------------------------*/
/*                                                                   */
/*   Turbo Vision 1.0                                                */
/*   Turbo Vision Demo                                               */
/*   Copyright (c) 1991 by Borland International                     */
/*                                                                   */
/*   Gadgets.cpp:  Gadgets for the Turbo Vision Demo.  Includes a    */
/*        heap view and a clock view which display the clock at the  */
/*        right end of the menu bar and the current heap space at    */
/*        the right end of the status line.                          */
/*-------------------------------------------------------------------*/

#define Uses_TRect
#define Uses_TView
#define Uses_TDrawBuffer
#include <tv.h>

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <strstrea.h>
#include <iomanip.h>
#include <malloc.h>
#include <time.h>

#include "gadgets.h"


//
// ------------- Heap Viewer functions
//

THeapView::THeapView(TRect& r) : TView( r )
{
    oldMem = 0;
    newMem = heapSize();
}


void THeapView::draw()
{
    TDrawBuffer buf;
    char c = getColor(2);

    buf.moveChar(0, ' ', c, size.x);
    buf.moveStr(0, heapStr, c);
    writeLine(0, 0, size.x, 1, buf);
}


void THeapView::update()
{
    if( (newMem = heapSize()) != oldMem )
        {
        oldMem = newMem;
        drawView();
        }
}


long THeapView::heapSize()
{
    // Linux port: bytes allocated from the heap (DOS showed free memory)
#if defined( _WIN32 )
    long total = 0;
    _HEAPINFO h;
    h._pentry = 0;
    while( _heapwalk( &h ) == _HEAPOK )
        if( h._useflag == _USEDENTRY )
            total += long( h._size );
#elif defined( __GLIBC__ ) && ( __GLIBC__ > 2 || __GLIBC_MINOR__ >= 33 )
    long total = long( mallinfo2().uordblks );
#else
    long total = long( mallinfo().uordblks );
#endif
    ostrstream totalStr( heapStr, sizeof heapStr);
    totalStr << setw(12) << total << ends;
    return(total);
}


//
// -------------- Clock Viewer functions
//

TClockView::TClockView( TRect& r ) : TView( r )
{
    strcpy(lastTime, "        ");
    strcpy(curTime, "        ");
}


void TClockView::draw()
{
    TDrawBuffer buf;
    char c = getColor(2);

    buf.moveChar(0, ' ', c, size.x);
    buf.moveStr(0, curTime, c);
    writeLine(0, 0, size.x, 1, buf);
}


void TClockView::update()
{
    time_t t = time(0);
    char *date = ctime(&t);

    date[19] = '\0';
    strcpy(curTime, &date[11]);        /* Extract time. */

    if( strcmp(lastTime, curTime) )
        {
        drawView();
        strcpy(lastTime, curTime);
        }
}

