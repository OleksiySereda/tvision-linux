/*------------------------------------------------------------*/
/* filename -       ttableview.cpp                            */
/*                                                            */
/* function(s)                                                */
/*                  TTableView and TTableHeader member        */
/*                  functions                                 */
/*                                                            */
/*   The rows keep the order they were added in; order maps   */
/*   the items on the screen to them, so sorting moves no     */
/*   text and marks stay with their rows.                     */
/*------------------------------------------------------------*/

#define Uses_TTableView
#define Uses_TTableHeader
#define Uses_TScrollBar
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TKeys
#include <tvaddons.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

static const char markChar = '\xFB';            // CP437 check mark
static const char dividerChar = '\xB3';         // CP437 vertical line
static const char upArrow = '\x1E', downArrow = '\x1F';

TTableView::TTableView( const TRect& bounds, TScrollBar *aHScrollBar,
                        TScrollBar *aVScrollBar ) :
    TListViewer( bounds, 1, aHScrollBar, aVScrollBar ),
    sortColumn( -1 ),
    sortAscending( True ),
    markable( False ),
    header( 0 )
{
    eventMask |= evMouseWheel;
}

void TTableView::shutDown()
{
    if( header != 0 )
        header->table = 0;
    header = 0;
    TListViewer::shutDown();
}

// ---- columns

void TTableView::addColumn( const char *title, int width, int align )
{
    Column c;
    c.title = title != 0 ? title : "";
    c.width = std::max( 1, width );
    c.align = align;
    columns.push_back( c );
    for( std::vector<std::string>& r : rows )
        r.resize( columns.size() );
    changed();
}

const char *TTableView::columnTitle( int col ) const
{
    return col >= 0 && col < columnCount() ? columns[col].title.c_str() : "";
}

int TTableView::columnWidth( int col ) const
{
    return col >= 0 && col < columnCount() ? columns[col].width : 0;
}

int TTableView::columnX( int col ) const
{
    int x = markWidth();
    for( int c = 0; c < col && c < columnCount(); c++ )
        x += columns[c].width + 1;
    return x;
}

int TTableView::totalWidth() const
{
    return columns.empty() ? markWidth() : columnX( columnCount() ) - 1;
}

int TTableView::columnAt( int x ) const
{
    if( hScrollBar != 0 )
        x += hScrollBar->value;
    for( int c = 0; c < columnCount(); c++ )
        {
        int start = columnX( c );
        if( x >= start && x < start + columns[c].width )
            return c;
        }
    return -1;
}

// ---- rows

int TTableView::addRow( const std::vector<std::string>& cells )
{
    int keep = focusedRow();
    int row = rowCount();
    rows.push_back( cells );
    rows.back().resize( std::max( cells.size(), columns.size() ) );
    marks.push_back( false );

    // a sorted table stays sorted: the new row goes after its equals
    std::vector<int>::iterator at = order.end();
    if( sortColumn >= 0 )
        at = std::upper_bound( order.begin(), order.end(), row,
            [this]( int a, int b )
                {
                int r = compare( a, b, sortColumn );
                return sortAscending == True ? r < 0 : r > 0;
                } );
    order.insert( at, row );
    changed();
    if( keep >= 0 )
        focusRow( keep );
    return row;
}

void TTableView::setCell( int row, int col, const char *text )
{
    if( row < 0 || row >= rowCount() || col < 0 )
        return;
    if( col >= int( rows[row].size() ) )
        rows[row].resize( col + 1 );
    rows[row][col] = text != 0 ? text : "";
    drawView();
}

const char *TTableView::cell( int row, int col ) const
{
    if( row < 0 || row >= rowCount() || col < 0 || col >= int( rows[row].size() ) )
        return "";
    return rows[row][col].c_str();
}

void TTableView::removeRow( int row )
{
    if( row < 0 || row >= rowCount() )
        return;
    rows.erase( rows.begin() + row );
    marks.erase( marks.begin() + row );
    order.erase( std::find( order.begin(), order.end(), row ) );
    for( int& r : order )
        if( r > row )
            r--;
    changed();
}

void TTableView::clear()
{
    rows.clear();
    marks.clear();
    order.clear();
    focused = topItem = 0;
    changed();
}

int TTableView::rowAt( int item ) const
{
    return item >= 0 && item < int( order.size() ) ? order[item] : -1;
}

int TTableView::itemOf( int row ) const
{
    for( size_t i = 0; i < order.size(); i++ )
        if( order[i] == row )
            return int( i );
    return -1;
}

int TTableView::focusedRow() const
{
    return rowAt( focused );
}

void TTableView::focusRow( int row )
{
    int item = itemOf( row );
    if( item >= 0 )
        {
        focusItemNum( short( item ) );
        drawView();
        }
}

void TTableView::changed()
{
    int n = int( order.size() );
    if( focused >= n )
        focused = short( n > 0 ? n - 1 : 0 );
    if( topItem > focused )
        topItem = focused;
    setRange( short( n ) );
    if( n > 0 )
        focusItemNum( focused );
    if( hScrollBar != 0 )
        hScrollBar->setParams( hScrollBar->value, 0,
                               std::max( 0, totalWidth() - size.x ),
                               std::max( 1, size.x - 1 ), 1 );
    drawView();
}

void TTableView::changeBounds( const TRect& bounds )
{
    TListViewer::changeBounds( bounds );
    changed();
}

// ---- sorting

static Boolean asNumber( const char *s, double& v )
{
    while( *s == ' ' )
        s++;
    if( *s == 0 )
        return False;
    char *end;
    v = strtod( s, &end );
    while( *end == ' ' )
        end++;
    return Boolean( end != s && *end == 0 );
}

int TTableView::compare( int rowA, int rowB, int col ) const
{
    const char *a = cell( rowA, col ), *b = cell( rowB, col );
    double x, y;
    Boolean na = asNumber( a, x ), nb = asNumber( b, y );
    if( na == True && nb == True )
        return x < y ? -1 : x > y ? 1 : 0;
    if( na != nb )
        return na == True ? -1 : 1;     // numbers before text
    for( ; *a != 0 || *b != 0; a++, b++ )
        {
        int d = tolower( uchar( *a ) ) - tolower( uchar( *b ) );
        if( d != 0 )
            return d;
        }
    return 0;
}

void TTableView::sortBy( int col, Boolean ascending )
{
    sortColumn = col >= 0 && col < columnCount() ? col : -1;
    sortAscending = ascending;
    resort();
}

void TTableView::resort()
{
    int keep = focusedRow();
    for( size_t i = 0; i < order.size(); i++ )
        order[i] = int( i );
    // equal rows keep the order they were added in
    if( sortColumn >= 0 )
        std::stable_sort( order.begin(), order.end(),
            [this]( int a, int b )
                {
                int r = compare( a, b, sortColumn );
                return sortAscending == True ? r < 0 : r > 0;
                } );
    if( keep >= 0 )
        focused = short( itemOf( keep ) );
    changed();
    if( header != 0 )
        header->drawView();
}

// ---- marks

void TTableView::setMark( int row, Boolean on )
{
    if( row >= 0 && row < rowCount() && marks[row] != ( on == True ) )
        {
        marks[row] = on == True;
        drawView();
        }
}

Boolean TTableView::isMarked( int row ) const
{
    return Boolean( row >= 0 && row < rowCount() && marks[row] );
}

int TTableView::markedCount() const
{
    return int( std::count( marks.begin(), marks.end(), true ) );
}

void TTableView::markAll( Boolean on )
{
    std::fill( marks.begin(), marks.end(), on == True );
    drawView();
}

Boolean TTableView::isSelected( short item )
{
    return isMarked( rowAt( item ) );
}

// ---- drawing

std::string TTableView::pad( const std::string& s, int width, int align ) const
{
    std::string t = s.substr( 0, size_t( width ) );
    int room = width - int( t.size() );
    switch( align )
        {
        case alRight:
            return std::string( size_t( room ), ' ' ) + t;
        case alCenter:
            return std::string( size_t( room / 2 ), ' ' ) + t +
                   std::string( size_t( room - room / 2 ), ' ' );
        default:
            return t + std::string( size_t( room ), ' ' );
        }
}

std::string TTableView::formatRow( int item ) const
{
    int row = rowAt( item );
    if( row < 0 )
        return std::string();
    std::string s;
    if( markable == True )
        {
        s += marks[row] ? markChar : ' ';
        s += ' ';
        }
    for( int c = 0; c < columnCount(); c++ )
        {
        if( c > 0 )
            s += dividerChar;
        s += pad( cell( row, c ), columns[c].width, columns[c].align );
        }
    return s;
}

std::string TTableView::formatHeader() const
{
    std::string s( size_t( markWidth() ), ' ' );
    for( int c = 0; c < columnCount(); c++ )
        {
        if( c > 0 )
            s += dividerChar;
        int w = columns[c].width;
        if( c == sortColumn && w > 1 )
            {
            s += pad( columns[c].title, w - 1, alLeft );
            s += sortAscending == True ? upArrow : downArrow;
            }
        else
            s += pad( columns[c].title, w, alLeft );
        }
    return s;
}

void TTableView::draw()
{
    Boolean active = Boolean( ( state & ( sfSelected | sfActive ) ) == ( sfSelected | sfActive ) );
    ushort normal = getColor( active == True ? 1 : 2 );
    ushort focusColor = getColor( 3 ), markColor = getColor( 4 ), divider = getColor( 5 );
    int indent = hScrollBar != 0 ? hScrollBar->value : 0;

    for( int y = 0; y < size.y; y++ )
        {
        int item = topItem + y;
        TDrawBuffer b;
        ushort color = normal;
        Boolean isFocused = Boolean( active == True && item == focused && item < range );
        if( isFocused == True )
            color = focusColor;
        else if( item < range && isMarked( rowAt( item ) ) )
            color = markColor;
        b.moveChar( 0, ' ', color, size.x );
        if( item < range )
            {
            std::string s = formatRow( item );
            if( int( s.size() ) > indent )
                b.moveStr( 0, s.substr( size_t( indent ), size_t( size.x ) ).c_str(), color );
            }
        // the dividers go on below the last row
        if( isFocused == False )
            for( int c = 1; c < columnCount(); c++ )
                {
                int x = columnX( c ) - 1 - indent;
                if( x >= 0 && x < size.x )
                    b.moveChar( ushort( x ), dividerChar, divider, 1 );
                }
        writeLine( 0, short( y ), size.x, 1, b );
        }
    if( active == True && focused < range )
        setCursor( 0, focused - topItem );
    if( header != 0 )
        header->drawView();
}

void TTableView::getText( char *dest, short item, short maxLen )
{
    std::string s = formatRow( item );
    strncpy( dest, s.c_str(), size_t( maxLen ) );
    dest[maxLen > 0 ? maxLen - 1 : 0] = 0;
}

// ---- events

void TTableView::handleEvent( TEvent& event )
{
    if( event.what == evMouseWheel )
        {
        if( event.mouse.buttons == mwUp )
            focusItemNum( short( focused - 3 ) );
        else if( event.mouse.buttons == mwDown )
            focusItemNum( short( focused + 3 ) );
        drawView();
        clearEvent( event );
        return;
        }
    if( event.what == evMouseDown && markable == True )
        {
        TPoint p = makeLocal( event.mouse.where );
        int item = topItem + p.y;
        int indent = hScrollBar != 0 ? hScrollBar->value : 0;
        if( p.x + indent < markWidth() && item < range )
            {
            if( ( state & sfSelected ) == 0 )
                select();
            focusItemNum( short( item ) );
            setMark( rowAt( item ), Boolean( !isMarked( rowAt( item ) ) ) );
            drawView();
            clearEvent( event );
            return;
            }
        }
    if( event.what == evKeyDown && focused < range )
        {
        if( markable == True &&
            ( event.keyDown.charScan.charCode == ' ' || event.keyDown.keyCode == kbIns ) )
            {
            int row = focusedRow();
            setMark( row, Boolean( !isMarked( row ) ) );
            if( event.keyDown.keyCode == kbIns )
                focusItemNum( short( focused + 1 ) );
            drawView();
            clearEvent( event );
            return;
            }
        if( event.keyDown.keyCode == kbEnter )
            {
            selectItem( focused );
            clearEvent( event );
            return;
            }
        }
    TListViewer::handleEvent( event );
}

// ---- TTableHeader

TTableHeader::TTableHeader( const TRect& bounds, TTableView *aTable ) :
    TView( bounds ),
    table( aTable )
{
    growMode = gfGrowHiX;
    if( table != 0 )
        table->header = this;
}

void TTableHeader::shutDown()
{
    if( table != 0 && table->header == this )
        table->header = 0;
    table = 0;
    TView::shutDown();
}

void TTableHeader::draw()
{
    TDrawBuffer b;
    if( table == 0 )
        {
        b.moveChar( 0, ' ', getColor( 1 ), size.x );
        writeLine( 0, 0, size.x, size.y, b );
        return;
        }
    ushort color = table->getColor( 5 );
    int indent = table->hScrollBar != 0 ? table->hScrollBar->value : 0;
    std::string s = table->formatHeader();
    b.moveChar( 0, ' ', color, size.x );
    if( int( s.size() ) > indent )
        b.moveStr( 0, s.substr( size_t( indent ), size_t( size.x ) ).c_str(), color );
    writeLine( 0, 0, size.x, size.y, b );
}

void TTableHeader::handleEvent( TEvent& event )
{
    TView::handleEvent( event );
    if( event.what == evMouseDown && table != 0 )
        {
        int col = table->columnAt( makeLocal( event.mouse.where ).x );
        if( col >= 0 )
            {
            if( col == table->sortColumn )
                table->sortBy( col, Boolean( table->sortAscending == False ) );
            else
                table->sortBy( col, True );
            }
        clearEvent( event );
        }
}
