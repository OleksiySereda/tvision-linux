/*------------------------------------------------------------*/
/* filename -       tcmdpal.cpp                               */
/*                                                            */
/* function(s)                                                */
/*                  TCommandPalette member functions          */
/*------------------------------------------------------------*/

#define Uses_TCommandPalette
#define Uses_TDialog
#define Uses_TInputLine
#define Uses_TListViewer
#define Uses_TScrollBar
#define Uses_TLabel
#define Uses_TMenu
#define Uses_TMenuItem
#define Uses_TMenuBar
#define Uses_TProgram
#define Uses_TDeskTop
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TRect
#include <tvaddons.h>

#include <ctype.h>
#include <string.h>
#include <algorithm>

namespace
{

class TPaletteList : public TListViewer
{

public:

    TPaletteList( const TRect& r, TScrollBar *vs, const std::vector<TPaletteEntry>& anItems ) :
        TListViewer( r, 1, 0, vs ),
        items( anItems )
    {
    }

    virtual void getText( char *dest, short item, short maxLen )
    {
        const TPaletteEntry& e = items[size_t( item )];
        std::string s = " " + e.name;
        int room = size.x - int( e.key.size() ) - 1;
        if( int( s.size() ) < room )
            s.append( size_t( room - int( s.size() ) ), ' ' );
        s += e.key;
        strncpy( dest, s.c_str(), size_t( maxLen ) );
        if( maxLen > 0 )
            dest[maxLen - 1] = 0;
    }

    const std::vector<TPaletteEntry>& items;
};

// TMenuView::menu is protected; a class derived from TMenuView may read
// it through a pointer of its own type
struct TMenuAccess : public TMenuView
{
    static TMenu *menuOf( TMenuView *v )
        { return static_cast<TMenuAccess *>( v )->menu; }
};

std::string lower( const std::string& s )
{
    std::string t( s );
    for( char& c : t )
        c = char( tolower( uchar( c ) ) );
    return t;
}

}   // namespace

TCommandPalette::TCommandPalette( TMenu *menu ) :
    TWindowInit( &TCommandPalette::initFrame ),
    TDialog( TRect( 0, 0, 64, 18 ), "Commands" ),
    lastText( "\x01" )
{
    options |= ofCenterX;
    origin.y = 1;
    if( TProgram::menuBar != 0 )
        collect( TMenuAccess::menuOf( TProgram::menuBar ), "", entries );
    if( menu != 0 )
        collect( menu, "", entries );

    input = new TInputLine( TRect( 3, 2, 61, 3 ), 61 );
    insert( input );
    insert( new TLabel( TRect( 2, 1, 30, 2 ), "~C~ommand (type a part of it)", input ) );
    TScrollBar *vs = new TScrollBar( TRect( 61, 4, 62, 17 ) );
    insert( vs );
    list = new TPaletteList( TRect( 2, 4, 61, 17 ), vs, shown );
    insert( list );
    input->select();
    filter();
}

void TCommandPalette::collect( TMenu *menu, const std::string& path,
                               std::vector<TPaletteEntry>& to )
{
    if( menu == 0 )
        return;
    for( TMenuItem *i = menu->items; i != 0; i = i->next )
        {
        if( i->name == 0 )
            continue;                   // a line
        std::string name;
        for( const char *p = i->name; *p != 0; p++ )
            if( *p != '~' )
                name += *p;
        std::string full = path.empty() ? name : path + " > " + name;
        if( i->command == 0 )
            collect( i->subMenu, full, to );
        else if( i->disabled == False && TView::commandEnabled( i->command ) )
            {
            TPaletteEntry e;
            e.name = full;
            e.key = i->param != 0 ? i->param : "";
            e.command = i->command;
            to.push_back( e );
            }
        }
}

void TCommandPalette::addCommand( const char *name, ushort command, const char *key )
{
    TPaletteEntry e;
    e.name = name != 0 ? name : "";
    e.key = key != 0 ? key : "";
    e.command = command;
    entries.push_back( e );
    lastText = "\x01";
    filter();
}

int TCommandPalette::score( const std::string& name, const char *text )
{
    std::string n = lower( name ), t = lower( text != 0 ? text : "" );
    if( t.empty() )
        return 1;
    int base = 0;
    size_t at = n.find( t );
    if( at == 0 )
        base = 400;
    else if( at != std::string::npos )
        {
        // the start of a word anywhere wins over the middle of one
        base = 200;
        for( size_t k = at; k != std::string::npos; k = n.find( t, k + 1 ) )
            if( k > 0 && !isalnum( uchar( n[k - 1] ) ) )
                {
                base = 300;
                break;
                }
        }
    else
        {
        size_t k = 0;
        for( char c : t )
            {
            k = n.find( c, k );
            if( k == std::string::npos )
                return 0;
            k++;
            }
        base = 100;
        }
    // the last part of the path (the command itself) counts more
    size_t gt = n.rfind( "> " );
    if( gt != std::string::npos && n.find( t, gt ) != std::string::npos )
        base += 50;
    return base;
}

void TCommandPalette::filter()
{
    std::string text = input->data;
    if( text == lastText )
        return;
    lastText = text;
    std::vector<std::pair<int, size_t> > v;
    for( size_t i = 0; i < entries.size(); i++ )
        {
        int s = score( entries[i].name, text.c_str() );
        if( s > 0 )
            v.push_back( std::make_pair( -s, i ) );
        }
    std::stable_sort( v.begin(), v.end() );
    shown.clear();
    for( auto& p : v )
        shown.push_back( entries[p.second] );
    list->setRange( short( shown.size() ) );
    list->focusItemNum( 0 );
    list->drawView();
}

ushort TCommandPalette::chosen()
{
    return list->focused >= 0 && list->focused < short( shown.size() ) ?
           shown[size_t( list->focused )].command : 0;
}

void TCommandPalette::handleEvent( TEvent& event )
{
    if( event.what == evKeyDown )
        {
        ushort code = ctrlToArrow( event.keyDown.keyCode );
        if( ( code == kbUp || code == kbDown || code == kbPgUp || code == kbPgDn ) &&
            current == input )
            {
            list->handleEvent( event );
            return;
            }
        if( event.keyDown.keyCode == kbEnter )
            {
            if( chosen() != 0 )
                endModal( cmOK );
            clearEvent( event );
            return;
            }
        }
    if( event.what == evBroadcast && event.message.command == cmListItemSelected &&
        event.message.infoPtr == list )
        {
        if( chosen() != 0 )
            endModal( cmOK );
        clearEvent( event );
        return;
        }
    TDialog::handleEvent( event );
    filter();
}

ushort TCommandPalette::run( TMenu *menu )
{
    TCommandPalette *d = new TCommandPalette( menu );
    TView *v = TProgram::application->validView( d );
    if( v == 0 )
        return 0;
    ushort c = TProgram::deskTop->execView( v ) == cmOK ? d->chosen() : 0;
    TObject::destroy( v );
    if( c != 0 )
        {
        TEvent e;
        e.what = evCommand;
        e.message.command = c;
        e.message.infoPtr = 0;
        TProgram::application->putEvent( e );
        }
    return c;
}
