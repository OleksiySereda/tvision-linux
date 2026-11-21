/*------------------------------------------------------------*/
/* filename -       tcombobox.cpp                             */
/*                                                            */
/* function(s)                                                */
/*                  TComboBox and TComboButton member         */
/*                  functions                                 */
/*                                                            */
/*   The list is a window like THistoryWindow (its palette    */
/*   too), run in the dialog over the line.                   */
/*------------------------------------------------------------*/

#define Uses_TComboBox
#define Uses_TValidatedInputLine
#define Uses_TWindow
#define Uses_TListViewer
#define Uses_TScrollBar
#define Uses_TDrawBuffer
#define Uses_TPalette
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TRect
#define Uses_MsgBox
#include <tvaddons.h>

#include <ctype.h>
#include <string.h>
#include <algorithm>

#define cpComboButton "\x16\x17"                        // as THistory
#define cpComboWindow "\x13\x13\x15\x18\x17\x13\x14"     // as THistoryWindow
#define cpComboViewer "\x06\x06\x07\x06\x06"             // as THistoryViewer

namespace
{

class TComboViewer : public TListViewer
{

public:

    TComboViewer( const TRect& r, TScrollBar *h, TScrollBar *v,
                  const std::vector<std::string>& anItems ) :
        TListViewer( r, 1, h, v ),
        items( anItems )
    {
        setRange( short( items.size() ) );
        if( h != 0 )
            {
            size_t w = 0;
            for( const std::string& s : items )
                w = std::max( w, s.size() );
            h->setRange( 0, short( std::max( 0, int( w ) - size.x + 3 ) ) );
            }
    }

    virtual TPalette& getPalette() const
    {
        static TPalette palette( cpComboViewer, sizeof( cpComboViewer ) - 1 );
        return palette;
    }

    virtual void getText( char *dest, short item, short maxLen )
    {
        const std::string& s = items[size_t( item )];
        strncpy( dest, s.c_str(), size_t( maxLen ) );
        if( maxLen > 0 )
            dest[maxLen - 1] = 0;
    }

    virtual void handleEvent( TEvent& event )
    {
        if( ( event.what == evMouseDown && event.mouse.doubleClick ) ||
            ( event.what == evKeyDown && event.keyDown.keyCode == kbEnter ) )
            {
            endModal( cmOK );
            clearEvent( event );
            }
        else if( ( event.what == evKeyDown && event.keyDown.keyCode == kbEsc ) ||
                 ( event.what == evCommand && event.message.command == cmCancel ) )
            {
            endModal( cmCancel );
            clearEvent( event );
            }
        else
            TListViewer::handleEvent( event );
    }

    const std::vector<std::string>& items;
};

class TComboWindow : public TWindow
{

public:

    TComboWindow( const TRect& r, const std::vector<std::string>& items ) :
        TWindowInit( &TComboWindow::initFrame ),
        TWindow( r, 0, wnNoNumber )
    {
        flags = wfClose;
        TRect v = getExtent();
        v.grow( -1, -1 );
        viewer = new TComboViewer( v, standardScrollBar( sbHorizontal | sbHandleKeyboard ),
                                   standardScrollBar( sbVertical | sbHandleKeyboard ), items );
        insert( viewer );
    }

    virtual TPalette& getPalette() const
    {
        static TPalette palette( cpComboWindow, sizeof( cpComboWindow ) - 1 );
        return palette;
    }

    TComboViewer *viewer;
};

}   // namespace

// ---- TComboBox

TComboBox::TComboBox( const TRect& bounds, int aMaxLen, const char *aItems, char separator ) :
    TValidatedInputLine( bounds, aMaxLen, 0 ),
    listHeight( 8 ),
    onlyList( False )
{
    if( aItems != 0 )
        setItems( aItems, separator );
}

void TComboBox::setItems( const char *aItems, char separator )
{
    items.clear();
    if( aItems == 0 || *aItems == 0 )
        return;
    const char *p = aItems;
    for( ;; )
        {
        const char *e = strchr( p, separator );
        items.push_back( e != 0 ? std::string( p, size_t( e - p ) ) : std::string( p ) );
        if( e == 0 )
            break;
        p = e + 1;
        }
}

void TComboBox::addItem( const char *anItem )
{
    items.push_back( anItem != 0 ? anItem : "" );
}

void TComboBox::clearItems()
{
    items.clear();
}

const char *TComboBox::item( int i ) const
{
    return i >= 0 && i < itemCount() ? items[size_t( i )].c_str() : "";
}

int TComboBox::findItem( const char *text ) const
{
    for( int i = 0; i < itemCount(); i++ )
        {
        const std::string& s = items[size_t( i )];
        if( s.size() == strlen( text ) )
            {
            size_t k = 0;
            while( k < s.size() && tolower( uchar( s[k] ) ) == tolower( uchar( text[k] ) ) )
                k++;
            if( k == s.size() )
                return i;
            }
        }
    return -1;
}

void TComboBox::selectItem( int i )
{
    if( i < 0 || i >= itemCount() )
        return;
    strncpy( data, item( i ), size_t( maxLen ) );
    data[maxLen] = 0;
    selectAll( True );
    drawView();
}

void TComboBox::setListOnly( Boolean on )
{
    onlyList = on;
}

int TComboBox::openList()
{
    if( owner == 0 || items.empty() )
        return -1;
    select();
    int rows = std::min( int( items.size() ), std::max( 1, listHeight ) );
    TRect r = getBounds();
    r.a.x--;
    r.b.x += 3;                         // over the button too
    r.a.y--;
    r.b.y = r.a.y + rows + 2;
    TRect p = owner->getExtent();
    if( r.b.y > p.b.y )
        r.move( 0, p.b.y - r.b.y );
    r.intersect( p );
    if( r.b.y - r.a.y < 3 )
        return -1;

    TComboWindow *w = new TComboWindow( r, items );
    int current = findItem( data );
    if( current >= 0 )
        w->viewer->focusItem( short( current ) );
    ushort c = owner->execView( w );
    int taken = c == cmOK ? w->viewer->focused : -1;
    destroy( w );
    if( taken >= 0 )
        selectItem( taken );
    return taken;
}

void TComboBox::handleEvent( TEvent& event )
{
    if( event.what == evKeyDown && ( state & sfSelected ) != 0 &&
        ctrlToArrow( event.keyDown.keyCode ) == kbDown )
        {
        openList();
        clearEvent( event );
        return;
        }
    TValidatedInputLine::handleEvent( event );
}

Boolean TComboBox::valid( ushort command )
{
    if( onlyList == True && command != cmValid && command != cmCancel &&
        command != cmReleasedFocus && ( state & sfDisabled ) == 0 )
        {
        int i = findItem( data );
        if( i < 0 )
            {
            std::string t = "Choose one of: ";
            for( int k = 0; k < itemCount(); k++ )
                t += ( k > 0 ? ", " : "" ) + items[size_t( k )];
            select();
            messageBox( t.c_str(), mfError | mfOKButton );
            return False;
            }
        strncpy( data, item( i ), size_t( maxLen ) );   // the case of the item
        data[maxLen] = 0;
        }
    return TValidatedInputLine::valid( command );
}

// ---- TComboButton

TComboButton::TComboButton( const TRect& bounds, TComboBox *aLink ) :
    TView( bounds ),
    link( aLink )
{
}

void TComboButton::shutDown()
{
    link = 0;
    TView::shutDown();
}

TPalette& TComboButton::getPalette() const
{
    static TPalette palette( cpComboButton, sizeof( cpComboButton ) - 1 );
    return palette;
}

void TComboButton::draw()
{
    TDrawBuffer b;
    b.moveCStr( 0, "\xDE~\x19~\xDD", getColor( 0x0102 ) );
    writeLine( 0, 0, size.x, size.y, b );
}

void TComboButton::handleEvent( TEvent& event )
{
    TView::handleEvent( event );
    if( event.what == evMouseDown && link != 0 )
        {
        link->openList();
        clearEvent( event );
        }
}
