/*------------------------------------------------------------*/
/* filename -       tprocview.cpp                             */
/*                                                            */
/* function(s)                                                */
/*                  TProcessView and TProcessDialog member    */
/*                  functions                                 */
/*                                                            */
/*   The dialog reads the output in getEvent(): the loop of   */
/*   a modal view calls it between its events, at least       */
/*   every 30 ms (TProgram::getEvent waits that long).        */
/*------------------------------------------------------------*/

#define Uses_TProcess
#define Uses_TProcessView
#define Uses_TProcessDialog
#define Uses_TLogView
#define Uses_TextConv
#define Uses_TDialog
#define Uses_TButton
#define Uses_TScrollBar
#define Uses_TProgram
#define Uses_TDeskTop
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TRect
#include <tvaddons.h>

#include <stdio.h>
#include <string>

// ---- TProcessView

TProcessView::TProcessView( const TRect& bounds, TScrollBar *aHScrollBar,
                            TScrollBar *aVScrollBar ) :
    TLogView( bounds, aHScrollBar, aVScrollBar, 5000 ),
    showExit( True ),
    process( 0 ),
    wasRunning( False )
{
}

TProcessView::~TProcessView()
{
    delete process;
}

bool TProcessView::start( const char *command, const char *dir )
{
    delete process;
    process = new TProcess;
    clear();
    partial.clear();
    add( std::string( "$ " ) + toScreenText( command ), warningColor );
    if( !process->start( command, dir != 0 ? dir : "" ) )
        {
        add( "cannot start: " + toScreenText( process->error() ), errorColor );
        wasRunning = False;
        return false;
        }
    wasRunning = True;
    return true;
}

Boolean TProcessView::running()
{
    return Boolean( process != 0 && process->running() );
}

void TProcessView::stop()
{
    if( process != 0 )
        process->stop();
}

// whole lines go to the log; '\r' alone starts the line over (progress
// counters), a line without its end waits
void TProcessView::addOutput( const std::string& text, Boolean flush )
{
    for( size_t i = 0; i < text.size(); i++ )
        {
        char c = text[i];
        if( c == '\n' )
            {
            add( partial );
            partial.clear();
            }
        else if( c == '\r' )
            {
            if( i + 1 < text.size() && text[i + 1] == '\n' )
                continue;
            partial.clear();
            }
        else
            partial += c;
        }
    if( flush == True && !partial.empty() )
        {
        add( partial );
        partial.clear();
        }
}

Boolean TProcessView::poll()
{
    if( process == 0 || wasRunning == False )
        return False;
    std::string raw = process->read();
    if( !raw.empty() )
        addOutput( toScreenText( stripAnsi( TProcess::toUtf8( raw ) ) ), False );
    if( process->running() )
        return True;
    addOutput( toScreenText( stripAnsi( TProcess::toUtf8( process->read() ) ) ), True );
    wasRunning = False;
    if( showExit == True )
        {
        int c = process->exitCode();
        char s[64];
        if( process->stopped() )
            snprintf( s, sizeof( s ), "stopped (exit code %d)", c );
        else
            snprintf( s, sizeof( s ), "exit code %d", c );
        add( s, c == 0 ? goodColor : errorColor );
        }
    return False;
}

// ---- TProcessDialog

// the line under the output: running, or how it ended
class TProcessStatus : public TView
{

public:

    TProcessStatus( const TRect& r ) : TView( r )
    {
        growMode = gfGrowLoY | gfGrowHiY | gfGrowHiX;
    }

    virtual void draw()
    {
        TDrawBuffer b;
        ushort color = getColor( 6 );                   // the dialog's static text
        b.moveChar( 0, ' ', color, size.x );
        b.moveStr( 0, text.substr( 0, size_t( size.x ) ).c_str(), color );
        writeLine( 0, 0, size.x, 1, b );
    }

    std::string text;
};

static TRect processBounds()
{
    TRect r = TProgram::deskTop->getExtent();
    r.grow( -2, -1 );
    if( r.b.x - r.a.x > 120 )
        {
        int w = 120;
        r.a.x = ( r.a.x + r.b.x - w ) / 2;
        r.b.x = r.a.x + w;
        }
    return r;
}

TProcessDialog::TProcessDialog( const char *title, const char *aCommand, const char *aDir ) :
    TWindowInit( &TProcessDialog::initFrame ),
    TDialog( processBounds(), title ),
    command( aCommand != 0 ? aCommand : "" ),
    dir( aDir != 0 ? aDir : "" ),
    ended( False )
{
    flags |= wfGrow;
    int w = size.x, h = size.y;
    TScrollBar *vs = new TScrollBar( TRect( w - 3, 1, w - 2, h - 5 ) );
    TScrollBar *hs = new TScrollBar( TRect( 2, h - 5, w - 3, h - 4 ) );
    vs->growMode = gfGrowLoX | gfGrowHiX | gfGrowHiY;
    hs->growMode = gfGrowLoY | gfGrowHiY | gfGrowHiX;
    vs->options |= ofPostProcess;
    hs->options |= ofPostProcess;
    insert( vs );
    insert( hs );
    view = new TProcessView( TRect( 2, 1, w - 3, h - 5 ), hs, vs );
    view->growMode = gfGrowHiX | gfGrowHiY;
    insert( view );
    status = new TProcessStatus( TRect( 2, h - 4, w - 30, h - 3 ) );
    insert( status );
    stopButton = new TButton( TRect( w - 27, h - 3, w - 15, h - 1 ), "~S~top", cmProcessStop,
                              bfNormal );
    stopButton->growMode = gfGrowAll;
    insert( stopButton );
    TButton *close = new TButton( TRect( w - 14, h - 3, w - 2, h - 1 ), "~C~lose", cmCancel,
                                  bfDefault );
    close->growMode = gfGrowAll;
    insert( close );
    view->select();
}

void TProcessDialog::showStatus()
{
    TProcess *p = view->process;
    if( p == 0 )
        status->text = "";
    else if( ended == False )
        status->text = p->stopped() ? "Stopping..." : "Running...";
    else if( p->exitCode() < 0 )
        status->text = "Not started.";
    else if( p->stopped() )
        status->text = "Stopped.";
    else
        {
        char s[64];
        snprintf( s, sizeof( s ), p->exitCode() == 0 ? "Done." : "Failed: exit code %d.",
                  p->exitCode() );
        status->text = s;
        }
    status->drawView();
}

ushort TProcessDialog::execute()
{
    ended = Boolean( !view->start( command.c_str(), dir.c_str() ) );
    showStatus();
    return TDialog::execute();
}

void TProcessDialog::getEvent( TEvent& event )
{
    TDialog::getEvent( event );
    if( ended == False && !view->poll() )
        {
        ended = True;
        stopButton->hide();
        showStatus();
        }
}

void TProcessDialog::handleEvent( TEvent& event )
{
    if( event.what == evCommand && event.message.command == cmProcessStop )
        {
        view->stop();
        showStatus();
        clearEvent( event );
        return;
        }
    // Close (or Esc) while it runs ends it first
    if( event.what == evCommand && event.message.command == cmCancel && ended == False )
        view->stop();
    TDialog::handleEvent( event );
}

int TProcessDialog::run( const char *title, const char *command, const char *dir )
{
    TProcessDialog *d = new TProcessDialog( title, command, dir );
    TView *v = TProgram::application->validView( d );
    if( v == 0 )
        return -1;
    TProgram::deskTop->execView( v );
    int code = d->view->exitCode();
    TObject::destroy( v );
    return code;
}
