/*------------------------------------------------------------*/
/* filename -       ttoast.cpp                                */
/*                                                            */
/* function(s)                                                */
/*                  TToast and TStatusMessageLine member      */
/*                  functions                                 */
/*                                                            */
/*   Views have no timers in Turbo Vision; the toasts and     */
/*   the status lines with a message are kept in lists that   */
/*   TToast::idle() goes through.                             */
/*------------------------------------------------------------*/

#define Uses_TToast
#define Uses_TStatusMessageLine
#define Uses_TStatusLine
#define Uses_TStatusDef
#define Uses_TProgram
#define Uses_TDeskTop
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TRect
#include <tvaddons.h>

#include <algorithm>
#include <chrono>
#include <vector>

namespace
{

std::vector<TToast *> toasts;
std::vector<TStatusMessageLine *> statusLines;

// the newest at the bottom, the older ones above it
void restack()
{
    if( TProgram::deskTop == 0 )
        return;
    TPoint room = TProgram::deskTop->size;
    int y = room.y - 1;
    for( int i = int( toasts.size() ) - 1; i >= 0; i--, y-- )
        {
        TToast *t = toasts[size_t( i )];
        int w = t->size.x;
        TRect r( room.x - w - 1, y, room.x - 1, y + 1 );
        if( r.a.x < 0 )
            r.a.x = 0;
        if( t->getBounds() != r )
            t->locate( r );
        }
}

}   // namespace

long toastClock()
{
    using namespace std::chrono;
    return long( duration_cast<milliseconds>( steady_clock::now().time_since_epoch() ).count() );
}

uchar TToast::colors[4] = { 0x3F, 0x2F, 0x6F, 0x4F };

TToast::TToast( const TRect& bounds, const char *aText, int aKind, long anUntil ) :
    TView( bounds ),
    text( aText != 0 ? aText : "" ),
    kind( aKind >= tkInfo && aKind <= tkError ? aKind : tkInfo ),
    until( anUntil )
{
    growMode = gfGrowLoX | gfGrowHiX | gfGrowLoY | gfGrowHiY;
}

TToast::~TToast()
{
    toasts.erase( std::remove( toasts.begin(), toasts.end(), this ), toasts.end() );
}

TToast *TToast::show( const char *text, int kind, int seconds )
{
    if( TProgram::deskTop == 0 )
        return 0;
    std::string t = text != 0 ? text : "";
    int w = std::min( int( t.size() ) + 4, int( TProgram::deskTop->size.x ) - 2 );
    TToast *toast = new TToast( TRect( 0, 0, std::max( 4, w ), 1 ), t.c_str(), kind,
                                toastClock() + 1000L * std::max( 1, seconds ) );
    toasts.push_back( toast );
    restack();
    TProgram::deskTop->insert( toast );
    return toast;
}

void TToast::idle()
{
    long now = toastClock();
    bool gone = false;
    for( size_t i = 0; i < toasts.size(); )
        if( toasts[i]->until <= now )
            {
            TObject::destroy( toasts[i] );      // takes itself out of the list
            gone = true;
            }
        else
            i++;
    if( gone )
        restack();
    for( TStatusMessageLine *s : statusLines )
        s->tick( now );
}

void TToast::clearAll()
{
    while( !toasts.empty() )
        TObject::destroy( toasts.back() );
}

int TToast::count()
{
    return int( toasts.size() );
}

void TToast::draw()
{
    TDrawBuffer b;
    ushort color = colors[kind];
    b.moveChar( 0, ' ', color, size.x );
    std::string t = text.substr( 0, size_t( std::max( 0, size.x - 2 ) ) );
    b.moveStr( ushort( ( size.x - int( t.size() ) ) / 2 ), t.c_str(), color );
    writeLine( 0, 0, size.x, 1, b );
}

void TToast::handleEvent( TEvent& event )
{
    TView::handleEvent( event );
    if( event.what == evMouseDown )
        {
        until = 0;                      // goes at the next idle()
        hide();
        clearEvent( event );
        }
}

// ---- TStatusMessageLine

TStatusMessageLine::TStatusMessageLine( const TRect& bounds, TStatusDef& aDefs ) :
    TStatusLine( bounds, aDefs ),
    until( 0 )
{
    statusLines.push_back( this );
}

TStatusMessageLine::~TStatusMessageLine()
{
    statusLines.erase( std::remove( statusLines.begin(), statusLines.end(), this ),
                       statusLines.end() );
}

void TStatusMessageLine::message( const char *aText, int seconds )
{
    text = aText != 0 ? aText : "";
    until = seconds > 0 ? toastClock() + 1000L * seconds : 0;
    drawView();
}

void TStatusMessageLine::clearMessage()
{
    if( !text.empty() )
        {
        text.clear();
        until = 0;
        drawView();
        }
}

void TStatusMessageLine::tick( long now )
{
    if( !text.empty() && until != 0 && now >= until )
        clearMessage();
}

void TStatusMessageLine::draw()
{
    if( text.empty() )
        {
        TStatusLine::draw();
        return;
        }
    TDrawBuffer b;
    ushort color = getColor( 1 );
    b.moveChar( 0, ' ', color, size.x );
    b.moveStr( 1, text.substr( 0, size_t( std::max( 0, size.x - 2 ) ) ).c_str(), color );
    writeLine( 0, 0, size.x, 1, b );
}
