/*------------------------------------------------------------*/
/* filename -       tsplitter.cpp                             */
/*                                                            */
/* function(s)                                                */
/*                  TSplitter member functions                */
/*------------------------------------------------------------*/

#define Uses_TSplitter
#define Uses_TAppSettings
#define Uses_TGroup
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TRect
#include <tvaddons.h>

#include <algorithm>
#include <string>

TSplitter::TSplitter( const TRect& bounds, Boolean aVertical ) :
    TView( bounds ),
    vertical( aVertical ),
    minFirst( 3 ),
    minSecond( 3 ),
    dragging( False )
{
    growMode = vertical == True ? gfGrowHiY : gfGrowHiX;
}

void TSplitter::shutDown()
{
    parts.clear();
    TView::shutDown();
}

void TSplitter::addFirst( TView *view, int mode )
{
    if( view != 0 )
        parts.push_back( Part{ view, mode, True } );
}

void TSplitter::addSecond( TView *view, int mode )
{
    if( view != 0 )
        parts.push_back( Part{ view, mode, False } );
}

void TSplitter::setPosition( int pos )
{
    // between the far edges of the views that are resized, less the
    // minimums
    int lo = 1, hi = owner != 0 ? ( vertical == True ? owner->size.x : owner->size.y ) - 2 : pos;
    for( const Part& p : parts )
        {
        if( p.mode != smResize )
            continue;
        TRect r = p.view->getBounds();
        if( p.first == True )
            lo = std::max( lo, ( vertical == True ? r.a.x : r.a.y ) + minFirst );
        else
            hi = std::min( hi, ( vertical == True ? r.b.x : r.b.y ) - minSecond - 1 );
        }
    if( pos < lo )
        pos = lo;
    if( pos > hi )
        pos = hi;
    int d = pos - position();
    if( d == 0 )
        return;
    for( const Part& p : parts )
        {
        TRect r = p.view->getBounds();
        if( p.mode == smMove )
            r.move( vertical == True ? d : 0, vertical == True ? 0 : d );
        else if( vertical == True )
            ( p.first == True ? r.b.x : r.a.x ) += d;
        else
            ( p.first == True ? r.b.y : r.a.y ) += d;
        p.view->locate( r );
        }
    TRect me = getBounds();
    me.move( vertical == True ? d : 0, vertical == True ? 0 : d );
    locate( me );
}

void TSplitter::save( TAppSettings& settings, const char *name )
{
    settings.setInt( ( std::string( "splitter." ) + name ).c_str(), "position", position() );
}

void TSplitter::restore( TAppSettings& settings, const char *name )
{
    std::string s = std::string( "splitter." ) + name;
    if( settings.has( s.c_str(), "position" ) )
        setPosition( int( settings.getInt( s.c_str(), "position" ) ) );
}

void TSplitter::draw()
{
    // the frame's colours: passive, active while it is dragged
    ushort color = getColor( dragging == True ? 2 : 1 );
    TDrawBuffer b;
    if( vertical == True )
        {
        b.moveChar( 0, dragging == True ? '\xBA' : '\xB3', color, size.x );
        writeLine( 0, 0, size.x, size.y, b );
        }
    else
        {
        b.moveChar( 0, dragging == True ? '\xCD' : '\xC4', color, size.x );
        writeLine( 0, 0, size.x, size.y, b );
        }
}

void TSplitter::handleEvent( TEvent& event )
{
    TView::handleEvent( event );
    if( event.what == evMouseDown && owner != 0 )
        {
        dragging = True;
        drawView();
        do  {
            TPoint p = owner->makeLocal( event.mouse.where );
            setPosition( vertical == True ? p.x : p.y );
            } while( mouseEvent( event, evMouseMove ) );
        dragging = False;
        drawView();
        clearEvent( event );
        }
}
