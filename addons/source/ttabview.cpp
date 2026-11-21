/*------------------------------------------------------------*/
/* filename -       ttabview.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TTabView member functions                 */
/*                                                            */
/*   The pages are groups inside the TTabView; the hidden     */
/*   ones keep their controls, so getData() and setData()     */
/*   of the dialog reach them.  A group draws only its        */
/*   subviews (and keeps them in a buffer), so the titles     */
/*   and the background of a page are views of their own,    */
/*   as TFrame and TBackground are: what a closed list or     */
/*   message box leaves is drawn again from them.             */
/*------------------------------------------------------------*/

#define Uses_TTabView
#define Uses_TValidatedInputLine
#define Uses_TGroup
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TRect
#include <tvaddons.h>

#include <ctype.h>
#include <string.h>

// the dialog's interior under the controls of a page
class TPageBackground : public TView
{

public:

    TPageBackground( const TRect& r ) : TView( r )
    {
        growMode = gfGrowHiX | gfGrowHiY;
    }

    virtual void draw()
    {
        TDrawBuffer b;
        b.moveChar( 0, ' ', getColor( 1 ), size.x );    // as TFrame fills it
        writeLine( 0, 0, size.x, size.y, b );
    }
};

class TTabPage : public TGroup
{

public:

    TTabPage( const TRect& r ) : TGroup( r )
    {
        options |= ofSelectable;
        growMode = gfGrowHiX | gfGrowHiY;
        insert( new TPageBackground( getExtent() ) );   // under everything
    }
};

// the first line: the titles, the shown one in the button colours, the
// others in the label colours, on a line
class TTabTitles : public TView
{

public:

    TTabTitles( const TRect& r, TTabView *aTabs ) : TView( r ), tabs( aTabs )
    {
        growMode = gfGrowHiX;
    }

    virtual void draw()
    {
        TDrawBuffer b;
        b.moveChar( 0, '\xC4', getColor( 0x0707 ), size.x );
        Boolean focused = Boolean( ( tabs->state & sfFocused ) != 0 );
        for( int i = 0; i < tabs->pageCount(); i++ )
            {
            ushort color;
            if( i != tabs->currentPage() )
                color = getColor( 0x0907 );
            else if( focused == True )
                color = getColor( 0x0E0C );
            else
                color = getColor( 0x0E0A );
            int x = tabs->tabX( i );
            if( x >= size.x )
                break;
            b.moveChar( ushort( x ), ' ', color, ushort( tabs->tabWidth( i ) ) );
            b.moveCStr( ushort( x + 1 ), tabs->pageTitle( i ), color );
            }
        writeLine( 0, 0, size.x, 1, b );
    }

    TTabView *tabs;
};

// the controls of a group that Tab goes through, in Tab order (the order
// they were inserted in, as TGroup::selectNext( False ) goes)
static std::vector<TView *> tabOrder( TGroup *g )
{
    std::vector<TView *> v;
    TView *last = g->last;
    if( last == 0 )
        return v;
    TView *p = last;
    do  {
        if( ( p->state & ( sfVisible | sfDisabled ) ) == sfVisible &&
            ( p->options & ofSelectable ) != 0 )
            v.push_back( p );
        p = p->prev();
        } while( p != last );
    return v;
}

static char hotKey( const std::string& title )
{
    size_t i = title.find( '~' );
    return i != std::string::npos && i + 1 < title.size() ?
           char( toupper( uchar( title[i + 1] ) ) ) : 0;
}

TTabView::TTabView( const TRect& bounds ) :
    TGroup( bounds ),
    shown( -1 )
{
    options |= ofSelectable | ofPostProcess;
    titles_ = new TTabTitles( TRect( 0, 0, size.x, 1 ), this );
    insert( titles_ );
}

void TTabView::shutDown()
{
    pages.clear();
    titles_ = 0;
    TGroup::shutDown();
}

TGroup *TTabView::addPage( const char *title )
{
    TTabPage *p = new TTabPage( TRect( 0, 1, size.x, size.y ) );
    if( !pages.empty() )
        p->hide();
    insert( p );
    pages.push_back( p );
    titles.push_back( title != 0 ? title : "" );
    started.push_back( false );
    if( shown < 0 )
        shown = 0;
    titles_->drawView();
    return p;
}

const char *TTabView::pageTitle( int i ) const
{
    return i >= 0 && i < pageCount() ? titles[i].c_str() : "";
}

int TTabView::tabWidth( int i ) const
{
    return cstrlen( pageTitle( i ) ) + 2;
}

int TTabView::tabX( int i ) const
{
    int x = 1;
    for( int k = 0; k < i; k++ )
        x += tabWidth( k ) + 1;
    return x;
}

void TTabView::selectPage( int i )
{
    if( i < 0 || i >= pageCount() )
        return;
    Boolean changed = Boolean( i != shown );
    if( changed == True )
        {
        if( shown >= 0 )
            pages[shown]->hide();
        shown = i;
        pages[i]->show();
        }
    select();
    pages[i]->select();
    startPage( i );
    titles_->drawView();
    if( changed == True )
        message( owner, evBroadcast, cmTabChanged, this );
}

// the first time, the focus goes to the first control of the page (a
// group would take the one inserted last)
void TTabView::startPage( int i )
{
    if( i < 0 || i >= pageCount() || started[size_t( i )] )
        return;
    started[size_t( i )] = true;
    std::vector<TView *> v = tabOrder( pages[i] );
    if( !v.empty() )
        v.front()->select();
}

void TTabView::setState( ushort aState, Boolean enable )
{
    if( ( aState & sfFocused ) != 0 && enable == True && shown >= 0 && titles_ != 0 )
        {
        // focused from outside (Tab, a click): into the page shown
        if( current != pages[size_t( shown )] )
            pages[size_t( shown )]->select();
        startPage( shown );
        }
    TGroup::setState( aState, enable );
    if( ( aState & ( sfFocused | sfActive ) ) != 0 && titles_ != 0 )
        titles_->drawView();
}

// Tab inside the page; False at its end, where the dialog goes on to its
// next control.  The page then starts over from its first control the
// next time Tab comes into it (and from its last for Shift+Tab).
Boolean TTabView::moveFocus( Boolean forwards )
{
    if( shown < 0 )
        return False;
    std::vector<TView *> v = tabOrder( pages[shown] );
    if( v.empty() )
        return False;
    int at = -1;
    for( size_t k = 0; k < v.size(); k++ )
        if( v[k] == pages[shown]->current )
            at = int( k );
    int next = forwards == True ? at + 1 : ( at < 0 ? int( v.size() ) - 1 : at - 1 );
    if( next >= 0 && next < int( v.size() ) )
        {
        v[next]->select();
        return True;
        }
    v[forwards == True ? 0 : v.size() - 1]->select();
    return False;
}

void TTabView::handleEvent( TEvent& event )
{
    if( event.what == evMouseDown )
        {
        TPoint p = makeLocal( event.mouse.where );
        if( p.y == 0 )
            {
            for( int i = 0; i < pageCount(); i++ )
                if( p.x >= tabX( i ) && p.x < tabX( i ) + tabWidth( i ) )
                    selectPage( i );
            clearEvent( event );
            return;
            }
        }
    else if( event.what == evKeyDown && pageCount() > 0 )
        {
        ushort code = event.keyDown.keyCode;
        if( code == kbCtrlPgDn || code == kbCtrlPgUp )
            {
            int n = pageCount();
            selectPage( ( shown + ( code == kbCtrlPgDn ? 1 : n - 1 ) ) % n );
            clearEvent( event );
            return;
            }
        char c = getAltChar( code );
        if( c != 0 )
            for( int i = 0; i < pageCount(); i++ )
                if( hotKey( titles[i] ) == toupper( uchar( c ) ) )
                    {
                    selectPage( i );
                    clearEvent( event );
                    return;
                    }
        if( ( code == kbTab || code == kbShiftTab ) && ( state & sfFocused ) != 0 &&
            owner != 0 && owner->current == this )
            {
            if( moveFocus( Boolean( code == kbTab ) ) )
                {
                clearEvent( event );
                return;
                }
            return;                     // the dialog moves on
            }
        }
    TGroup::handleEvent( event );
}

// the first wrong validated line of page i, in Tab order, or 0
static TValidatedInputLine *firstInvalid( TGroup *page )
{
    TView *last = page->last;
    if( last == 0 )
        return 0;
    TView *p = last;
    do  {
        TValidatedInputLine *l = dynamic_cast<TValidatedInputLine *>( p );
        if( l != 0 && l->validator != 0 && ( l->state & sfDisabled ) == 0 &&
            !( l->validator->allowEmpty == True && l->data[0] == 0 ) &&
            !l->validator->isValid( l->data ) )
            return l;
        p = p->prev();
        } while( p != last );
    return 0;
}

int TTabView::pageWithInvalidLine( ushort )
{
    for( int i = 0; i < pageCount(); i++ )
        if( firstInvalid( pages[i] ) != 0 )
            return i;
    return -1;
}

// the page of a wrong field is shown before the message, and within a
// page the fields are checked from the top
Boolean TTabView::valid( ushort command )
{
    if( command == cmValid || command == cmCancel || command == cmReleasedFocus )
        return TGroup::valid( command );
    int i = pageWithInvalidLine( command );
    if( i >= 0 )
        {
        selectPage( i );
        return firstInvalid( pages[i] )->valid( command );
        }
    for( i = 0; i < pageCount(); i++ )
        if( !pages[i]->valid( command ) )
            {
            selectPage( i );
            return False;
            }
    return True;
}
