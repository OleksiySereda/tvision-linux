/*------------------------------------------------------------*/
/* filename -       tprogbar.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TProgressBar and TProgressDialog member   */
/*                  functions                                 */
/*------------------------------------------------------------*/

#define Uses_TProgressBar
#define Uses_TProgressDialog
#define Uses_TDialog
#define Uses_TButton
#define Uses_TProgram
#define Uses_TDeskTop
#define Uses_TDrawBuffer
#define Uses_TPalette
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TRect
#define Uses_TScreen
#define Uses_TEventQueue
#include <tvaddons.h>

#include <stdio.h>
#include <string.h>
#include <chrono>

// the terminal driver of the port: waitInput( 0 ) reads what has come
// in without waiting (TEvent::getKeyEvent() only takes it from the queue)
#include "../../source/platform/platform.h"

// ---- TProgressBar

#define cpProgressBar "\x1B\x18"

TProgressBar::TProgressBar( const TRect& bounds, long aTotal ) :
    TView( bounds ),
    showText( True ),
    current( 0 ),
    maximum( aTotal > 0 ? aTotal : 1 )
{
}

TPalette& TProgressBar::getPalette() const
{
    static TPalette palette( cpProgressBar, sizeof( cpProgressBar ) - 1 );
    return palette;
}

int TProgressBar::percent() const
{
    return int( current * 100 / maximum );
}

void TProgressBar::setValue( long aValue )
{
    if( aValue < 0 )
        aValue = 0;
    if( aValue > maximum )
        aValue = maximum;
    if( aValue != current )
        {
        current = aValue;
        drawView();
        }
}

void TProgressBar::setTotal( long aTotal )
{
    maximum = aTotal > 0 ? aTotal : 1;
    if( current > maximum )
        current = maximum;
    drawView();
}

void TProgressBar::setText( const char *aText )
{
    text = aText != 0 ? aText : "";
    drawView();
}

void TProgressBar::draw()
{
    ushort done = getColor( 1 ), rest = getColor( 2 );
    int w = size.x;
    int filled = int( (long long) w * current / maximum );

    std::string t;
    if( showText == True )
        {
        if( !text.empty() )
            t = text;
        else
            {
            char s[8];
            snprintf( s, sizeof( s ), "%d%%", percent() );
            t = s;
            }
        if( int( t.size() ) > w )
            t.resize( size_t( w ) );
        }
    int start = ( w - int( t.size() ) ) / 2;

    TDrawBuffer b;
    for( int x = 0; x < w; x++ )
        {
        char c = x >= start && x < start + int( t.size() ) ? t[size_t( x - start )] : ' ';
        b.moveChar( ushort( x ), c, x < filled ? done : rest, 1 );
        }
    for( int y = 0; y < size.y; y++ )
        writeLine( 0, short( y ), size.x, 1, b );
}

// ---- TProgressDialog

// the line of text above the bar
class TProgressLine : public TView
{

public:

    TProgressLine( const TRect& r ) : TView( r ) {}

    virtual void draw()
    {
        TDrawBuffer b;
        ushort color = getColor( 6 );           // the dialog's static text
        b.moveChar( 0, ' ', color, size.x );
        b.moveStr( 0, text.substr( 0, size_t( size.x ) ).c_str(), color );
        writeLine( 0, 0, size.x, 1, b );
    }

    std::string text;
};

static long nowMs()
{
    using namespace std::chrono;
    return long( duration_cast<milliseconds>( steady_clock::now().time_since_epoch() ).count() );
}

TProgressDialog::TProgressDialog( const char *title, long total ) :
    TWindowInit( &TProgressDialog::initFrame ),
    TDialog( TRect( 0, 0, 50, 9 ), title ),
    pollMs( 50 ),
    cancelFlag( False ),
    lastPoll( 0 )
{
    options |= ofCentered;
    flags &= ~wfClose;
    message = new TProgressLine( TRect( 3, 2, 47, 3 ) );
    insert( message );
    bar = new TProgressBar( TRect( 3, 4, 47, 5 ), total );
    insert( bar );
    insert( new TButton( TRect( 19, 6, 31, 8 ), "Cancel", cmCancel, bfDefault ) );
    selectNext( False );
}

void TProgressDialog::setText( const char *text )
{
    message->text = text != 0 ? text : "";
    message->drawView();
}

void TProgressDialog::setValue( long value )
{
    bar->setValue( value );
}

void TProgressDialog::setTotal( long total )
{
    bar->setTotal( total );
}

void TProgressDialog::finish()
{
    if( ( state & sfModal ) != 0 )
        endModal( cmOK );
}

void TProgressDialog::handleEvent( TEvent& event )
{
    if( event.what == evCommand && event.message.command == cmCancel )
        {
        cancelFlag = True;
        if( ( state & sfModal ) == 0 )
            {
            clearEvent( event );
            return;
            }
        }
    TDialog::handleEvent( event );
}

// not modal, cmCancel (from the button or Esc) would wait in the
// program's queue until the loop ends
void TProgressDialog::putEvent( TEvent& event )
{
    if( ( state & sfModal ) == 0 && event.what == evCommand &&
        event.message.command == cmCancel )
        {
        cancelFlag = True;
        return;
        }
    TDialog::putEvent( event );
}

Boolean TProgressDialog::cancelled()
{
    long now = nowMs();
    if( cancelFlag == True || now - lastPoll < pollMs )
        return cancelFlag;
    lastPoll = now;

    // TProgram::getEvent() waits for input when there is none; here the
    // work must go on, so the events are taken without waiting.  A
    // resized terminal is followed as getEvent() does.  Only the dialog
    // gets the events; the Cancel button and Esc come back as cmCancel
    // through putEvent(), which takes it.
    TProgram *app = TProgram::application;
    if( TScreen::checkResize() )
        {
        app->buffer = TScreen::screenBuffer;
        app->changeBounds( TRect( 0, 0, TScreen::screenWidth, TScreen::screenHeight ) );
        app->redraw();
        }
    tvplatform::waitInput( 0 );
    for( int n = 0; n < 64; n++ )
        {
        TEvent e;
        e.getMouseEvent();
        if( e.what == evNothing )
            e.getKeyEvent();
        if( e.what == evNothing )
            break;
        if( ( e.what & positionalEvents ) == 0 || mouseInView( e.mouse.where ) )
            handleEvent( e );
        }
    TScreen::flushScreen();
    return cancelFlag;
}
