/*------------------------------------------------------------*/
/* filename -       tlogview.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TLogView member functions                 */
/*                                                            */
/*   TScroller takes delta from its scroll bars; TLogView     */
/*   moves delta itself too, so that it scrolls without       */
/*   them (and while they are hidden).                        */
/*------------------------------------------------------------*/

#define Uses_TLogView
#define Uses_TScrollBar
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TKeys
#include <tvaddons.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>

uchar TLogView::errorColor   = 0x4F;
uchar TLogView::warningColor = 0x6E;
uchar TLogView::goodColor    = 0x2F;

static const int maxLimit = 32000;      // limits are shorts

TLogView::TLogView( const TRect& bounds, TScrollBar *aHScrollBar,
                    TScrollBar *aVScrollBar, int aMaxLines ) :
    TScroller( bounds, aHScrollBar, aVScrollBar ),
    maxLines( aMaxLines > 0 ? aMaxLines : 1 ),
    autoScroll( True ),
    wheelLines( 3 ),
    shown( 0 ),
    width( 1 )
{
    options |= ofSelectable;
    eventMask |= evMouseWheel;
}

void TLogView::add( const char *text, uchar color )
{
    const char *p = text;
    for( ;; )
        {
        const char *e = strchr( p, '\n' );
        size_t n = e != 0 ? size_t( e - p ) : strlen( p );
        Line l;
        l.color = color;
        for( size_t i = 0; i < n; i++ )
            if( p[i] == '\t' )
                l.text.append( 8 - l.text.size() % 8, ' ' );
            else if( p[i] != '\r' )
                l.text += p[i];
        lines.push_back( l );
        if( e == 0 )
            break;
        p = e + 1;
        }

    int k = int( lines.size() ) - maxLines;
    if( k > 0 )
        {
        lines.erase( lines.begin(), lines.begin() + k );
        shown = std::max( 0, shown - k );
        delta.y = short( std::max( 0, delta.y - k ) );
        if( vScrollBar != 0 )
            {
            drawLock++;
            vScrollBar->setValue( delta.y );
            drawLock--;
            }
        }
    update();
}

void TLogView::addf( const char *format, ... )
{
    char buf[1024];
    va_list arg;
    va_start( arg, format );
    vsnprintf( buf, sizeof( buf ), format, arg );
    va_end( arg );
    add( buf );
}

void TLogView::clear()
{
    lines.clear();
    shown = 0;
    width = 1;
    delta.x = delta.y = 0;
    update();
}

int TLogView::lineCount()
{
    return int( lines.size() );
}

const char *TLogView::lineText( int i )
{
    return lines[size_t( i )].text.c_str();
}

uchar TLogView::lineColor( int i )
{
    return lines[size_t( i )].color;
}

Boolean TLogView::atEnd()
{
    return Boolean( delta.y + size.y >= shown );
}

void TLogView::update()
{
    Boolean follow = Boolean( autoScroll == True && atEnd() );
    int n = lineCount();
    int from = shown;
    if( n < shown )
        {
        width = 1;
        from = 0;
        }
    for( int i = from; i < n; i++ )
        width = std::max( width, int( strlen( lineText( i ) ) ) + 1 );
    shown = n;
    setLimit( short( std::min( width, maxLimit ) ), short( std::min( n, maxLimit ) ) );
    if( follow == True )
        scrollToEnd();
    else
        scrollToPos( delta.x, delta.y );
}

void TLogView::scrollToEnd()
{
    scrollToPos( delta.x, limit.y - size.y );
}

void TLogView::scrollToPos( int x, int y )
{
    x = std::max( 0, std::min( x, limit.x - size.x ) );
    y = std::max( 0, std::min( y, limit.y - size.y ) );
    drawLock++;
    if( hScrollBar != 0 )
        hScrollBar->setValue( short( x ) );
    if( vScrollBar != 0 )
        vScrollBar->setValue( short( y ) );
    drawLock--;
    delta.x = short( x );
    delta.y = short( y );
    drawFlag = False;
    drawView();
}

void TLogView::scrollDraw()
{
    TPoint d = delta;
    if( hScrollBar != 0 )
        d.x = hScrollBar->value;
    if( vScrollBar != 0 )
        d.y = vScrollBar->value;
    if( d.x != delta.x || d.y != delta.y )
        {
        delta = d;
        if( drawLock != 0 )
            drawFlag = True;
        else
            drawView();
        }
}

void TLogView::changeBounds( const TRect& bounds )
{
    Boolean follow = Boolean( autoScroll == True && atEnd() );
    TScroller::changeBounds( bounds );
    if( follow == True )
        scrollToEnd();
    else
        scrollToPos( delta.x, delta.y );
}

void TLogView::draw()
{
    ushort normal = getColor( 1 );
    char s[maxViewWidth + 1];
    for( int y = 0; y < size.y; y++ )
        {
        TDrawBuffer b;
        int i = delta.y + y;
        ushort color = normal;
        s[0] = 0;
        if( i < shown && i < lineCount() )
            {
            if( lineColor( i ) != 0 )
                color = lineColor( i );
            const char *t = lineText( i );
            int n = int( strlen( t ) );
            if( n > delta.x )
                {
                int m = std::min( n - delta.x, int( size.x ) );
                m = std::min( m, int( maxViewWidth ) );
                memcpy( s, t + delta.x, size_t( m ) );
                s[m] = 0;
                }
            }
        b.moveChar( 0, ' ', color, size.x );
        b.moveStr( 0, s, color );
        writeLine( 0, short( y ), size.x, 1, b );
        }
}

void TLogView::handleEvent( TEvent& event )
{
    TScroller::handleEvent( event );
    if( event.what == evMouseWheel )
        {
        switch( event.mouse.buttons )
            {
            case mwUp:    scrollToPos( delta.x, delta.y - wheelLines ); break;
            case mwDown:  scrollToPos( delta.x, delta.y + wheelLines ); break;
            case mwLeft:  scrollToPos( delta.x - wheelLines, delta.y ); break;
            case mwRight: scrollToPos( delta.x + wheelLines, delta.y ); break;
            }
        clearEvent( event );
        }
    else if( event.what == evKeyDown )
        {
        int page = std::max( 1, size.y - 1 );
        switch( ctrlToArrow( event.keyDown.keyCode ) )
            {
            case kbUp:       scrollToPos( delta.x, delta.y - 1 ); break;
            case kbDown:     scrollToPos( delta.x, delta.y + 1 ); break;
            case kbLeft:     scrollToPos( delta.x - 1, delta.y ); break;
            case kbRight:    scrollToPos( delta.x + 1, delta.y ); break;
            case kbPgUp:     scrollToPos( delta.x, delta.y - page ); break;
            case kbPgDn:     scrollToPos( delta.x, delta.y + page ); break;
            case kbHome:
            case kbCtrlPgUp: scrollToPos( 0, 0 ); break;
            case kbEnd:
            case kbCtrlPgDn: scrollToPos( 0, limit.y ); break;
            default:
                return;
            }
        clearEvent( event );
        }
}

Boolean TLogView::saveToFile( const char *fileName )
{
    FILE *f = fopen( fileName, "w" );
    if( f == 0 )
        return False;
    int n = lineCount();
    for( int i = 0; i < n; i++ )
        fprintf( f, "%s\n", lineText( i ) );
    Boolean ok = Boolean( ferror( f ) == 0 );
    if( fclose( f ) != 0 )
        ok = False;
    return ok;
}
