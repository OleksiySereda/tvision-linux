/*------------------------------------------------------------*/
/* filename -       ttreeview.cpp                             */
/*                                                            */
/* function(s)                                                */
/*                  TTreeView member functions                */
/*                                                            */
/*   lines holds the nodes on the screen (those whose         */
/*   parents are all open), in tree order; rebuild() makes   */
/*   it again after a node opens, closes, comes or goes.      */
/*------------------------------------------------------------*/

#define Uses_TTreeView
#define Uses_TScrollBar
#define Uses_TDrawBuffer
#define Uses_TEvent
#define Uses_TKeys
#include <tvaddons.h>

#include <string.h>
#include <algorithm>

static const char vLine = '\xB3', tee = '\xC3', corner = '\xC0', hLine = '\xC4';

int TTreeNode::depth() const
{
    int d = 0;
    for( const TTreeNode *p = parent; p != 0; p = p->parent )
        d++;
    return d;
}

static Boolean hasNextSibling( const TTreeNode *n, const std::vector<TTreeNode *>& top )
{
    const std::vector<TTreeNode *>& v = n->parent != 0 ? n->parent->children : top;
    return Boolean( !v.empty() && v.back() != n );
}

TTreeView::TTreeView( const TRect& bounds, TScrollBar *aHScrollBar,
                      TScrollBar *aVScrollBar ) :
    TListViewer( bounds, 1, aHScrollBar, aVScrollBar ),
    markable( False )
{
    eventMask |= evMouseWheel;
}

TTreeView::~TTreeView()
{
    for( TTreeNode *n : top )
        destroyNode( n );
}

void TTreeView::destroyNode( TTreeNode *n )
{
    for( TTreeNode *c : n->children )
        destroyNode( c );
    delete n;
}

TTreeNode *TTreeView::addNode( TTreeNode *parent, const char *text, void *data )
{
    TTreeNode *n = new TTreeNode;
    n->text = text != 0 ? text : "";
    n->data = data;
    n->parent = parent;
    n->expanded = True;
    n->marked = False;
    ( parent != 0 ? parent->children : top ).push_back( n );
    rebuild( focusedNode() );
    return n;
}

void TTreeView::removeNode( TTreeNode *node )
{
    if( node == 0 )
        return;
    // the focus goes to the parent when it was in what goes; lines still
    // hold the nodes, so they are made again before the nodes are freed
    TTreeNode *keep = focusedNode();
    for( TTreeNode *p = keep; p != 0; p = p->parent )
        if( p == node )
            {
            keep = node->parent;
            break;
            }
    if( keep == 0 )
        focused = short( std::max( 0, itemOf( node ) ) );
    std::vector<TTreeNode *>& v = node->parent != 0 ? node->parent->children : top;
    v.erase( std::remove( v.begin(), v.end(), node ), v.end() );
    rebuild( keep );
    destroyNode( node );
}

void TTreeView::clear()
{
    for( TTreeNode *n : top )
        destroyNode( n );
    top.clear();
    lines.clear();
    focused = topItem = 0;
    rebuild( 0 );
}

void TTreeView::addLines( TTreeNode *n )
{
    lines.push_back( n );
    if( n->expanded == True )
        for( TTreeNode *c : n->children )
            addLines( c );
}

// the focus stays on keep, or on the nearest parent of it still shown;
// with keep 0, on the same line
void TTreeView::rebuild( TTreeNode *keep )
{
    lines.clear();
    for( TTreeNode *n : top )
        addLines( n );
    int item = -1;
    for( TTreeNode *k = keep; k != 0 && item < 0; k = k->parent )
        item = itemOf( k );
    int n = visibleCount();
    focused = short( item >= 0 ? item : ( focused < n ? focused : ( n > 0 ? n - 1 : 0 ) ) );
    if( topItem > focused )
        topItem = focused;
    setRange( short( n ) );
    if( n > 0 )
        focusItemNum( focused );
    if( hScrollBar != 0 )
        {
        int w = 0;
        for( int i = 0; i < n; i++ )
            w = std::max( w, int( formatLine( i ).size() ) );
        hScrollBar->setParams( hScrollBar->value, 0, std::max( 0, w - size.x ),
                               std::max( 1, size.x - 1 ), 1 );
        }
    drawView();
}

void TTreeView::changeBounds( const TRect& bounds )
{
    TListViewer::changeBounds( bounds );
    rebuild( focusedNode() );
}

TTreeNode *TTreeView::nodeAt( int item ) const
{
    return item >= 0 && item < visibleCount() ? lines[item] : 0;
}

int TTreeView::itemOf( TTreeNode *node ) const
{
    for( size_t i = 0; i < lines.size(); i++ )
        if( lines[i] == node )
            return int( i );
    return -1;
}

void TTreeView::expand( TTreeNode *node, Boolean open )
{
    if( node != 0 && node->expanded != open )
        {
        node->expanded = open;
        rebuild( focusedNode() );
        }
}

static void setOpen( TTreeNode *n, Boolean open )
{
    n->expanded = open;
    for( TTreeNode *c : n->children )
        setOpen( c, open );
}

void TTreeView::expandAll( TTreeNode *node, Boolean open )
{
    if( node != 0 )
        setOpen( node, open );
    else
        for( TTreeNode *n : top )
            setOpen( n, open );
    rebuild( focusedNode() );
}

void TTreeView::focusNode( TTreeNode *node )
{
    if( node == 0 )
        return;
    Boolean changed = False;
    for( TTreeNode *p = node->parent; p != 0; p = p->parent )
        if( p->expanded == False )
            {
            p->expanded = True;
            changed = True;
            }
    if( changed == True )
        rebuild( focusedNode() );
    int item = itemOf( node );
    if( item >= 0 )
        {
        focusItemNum( short( item ) );
        drawView();
        }
}

// ---- marks

void TTreeView::markTree( TTreeNode *n, Boolean on )
{
    n->marked = on;
    for( TTreeNode *c : n->children )
        markTree( c, on );
}

void TTreeView::setMark( TTreeNode *node, Boolean on )
{
    if( node != 0 )
        {
        markTree( node, on );
        drawView();
        }
}

int TTreeView::markOf( TTreeNode *node ) const
{
    if( node == 0 )
        return mkNone;
    if( node->children.empty() )
        return node->marked == True ? mkAll : mkNone;
    Boolean all = True, none = True;
    for( TTreeNode *c : node->children )
        {
        int m = markOf( c );
        if( m != mkAll )
            all = False;
        if( m != mkNone )
            none = False;
        }
    return all == True ? mkAll : none == True ? mkNone : mkSome;
}

void TTreeView::collect( TTreeNode *n, std::vector<TTreeNode *>& v ) const
{
    if( n->children.empty() )
        {
        if( n->marked == True )
            v.push_back( n );
        }
    else
        for( TTreeNode *c : n->children )
            collect( c, v );
}

std::vector<TTreeNode *> TTreeView::markedLeaves() const
{
    std::vector<TTreeNode *> v;
    for( TTreeNode *n : top )
        collect( n, v );
    return v;
}

// ---- drawing

// where + or - stands: 0 at the top, then 2 more for each level, after
// the lines of the parents and the connector
static int expanderX( const TTreeNode *n )
{
    int d = n->depth();
    return d == 0 ? 0 : 2 * d + 2;
}

int TTreeView::prefixWidth( int item ) const
{
    TTreeNode *n = nodeAt( item );
    return n == 0 ? 0 : expanderX( n ) + 2 + ( markable == True ? 4 : 0 );
}

std::string TTreeView::formatLine( int item ) const
{
    TTreeNode *n = nodeAt( item );
    if( n == 0 )
        return std::string();
    std::string s;
    int d = n->depth();
    if( d > 0 )
        {
        // the parents from the top one down: a line goes on where a
        // parent has a sibling after it
        std::vector<const TTreeNode *> up;
        for( const TTreeNode *p = n->parent; p != 0; p = p->parent )
            up.push_back( p );
        s += "  ";
        for( int k = int( up.size() ) - 2; k >= 0; k-- )
            {
            s += hasNextSibling( up[size_t( k )], top ) ? vLine : ' ';
            s += ' ';
            }
        s += hasNextSibling( n, top ) ? tee : corner;
        s += hLine;
        }
    if( !n->children.empty() )
        s += n->expanded == True ? '-' : '+';
    else
        s += d > 0 ? hLine : ' ';
    s += ' ';
    if( markable == True )
        {
        int m = markOf( n );
        s += m == mkAll ? "[x] " : m == mkSome ? "[-] " : "[ ] ";
        }
    s += n->text;
    return s;
}

void TTreeView::draw()
{
    Boolean active = Boolean( ( state & ( sfSelected | sfActive ) ) == ( sfSelected | sfActive ) );
    ushort normal = getColor( active == True ? 1 : 2 );
    ushort focusColor = getColor( 3 );
    int indent = hScrollBar != 0 ? hScrollBar->value : 0;
    for( int y = 0; y < size.y; y++ )
        {
        int item = topItem + y;
        TDrawBuffer b;
        ushort color = active == True && item == focused && item < range ? focusColor : normal;
        b.moveChar( 0, ' ', normal, size.x );
        if( item < range )
            {
            std::string s = formatLine( item );
            if( int( s.size() ) > indent )
                b.moveStr( 0, s.substr( size_t( indent ), size_t( size.x ) ).c_str(), normal );
            // the focus covers the text only, not the lines before it
            if( color != normal )
                {
                int from = std::max( 0, prefixWidth( item ) - indent );
                int to = std::min( int( size.x ), int( s.size() ) - indent );
                for( int x = from; x < to; x++ )
                    b.putAttribute( ushort( x ), color );
                }
            }
        writeLine( 0, short( y ), size.x, 1, b );
        }
}

void TTreeView::getText( char *dest, short item, short maxLen )
{
    TTreeNode *n = nodeAt( item );
    strncpy( dest, n != 0 ? n->text.c_str() : "", size_t( maxLen ) );
    if( maxLen > 0 )
        dest[maxLen - 1] = 0;
}

// ---- events

void TTreeView::handleEvent( TEvent& event )
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
    if( event.what == evMouseDown && !event.mouse.doubleClick )
        {
        TPoint p = makeLocal( event.mouse.where );
        int item = topItem + p.y;
        TTreeNode *n = nodeAt( item );
        int x = p.x + ( hScrollBar != 0 ? hScrollBar->value : 0 );
        if( n != 0 )
            {
            int ex = expanderX( n );
            Boolean onExpander = Boolean( x == ex && !n->children.empty() );
            Boolean onBox = Boolean( markable == True && x >= ex + 2 && x < ex + 5 );
            if( onExpander == True || onBox == True )
                {
                if( ( state & sfSelected ) == 0 )
                    select();
                focusItemNum( short( item ) );
                if( onExpander == True )
                    expand( n, Boolean( n->expanded == False ) );
                else
                    setMark( n, Boolean( markOf( n ) != mkAll ) );
                drawView();
                clearEvent( event );
                return;
                }
            }
        }
    if( event.what == evKeyDown )
        {
        TTreeNode *n = focusedNode();
        uchar c = event.keyDown.charScan.charCode;
        ushort code = ctrlToArrow( event.keyDown.keyCode );
        Boolean done = True;
        if( n == 0 )
            done = False;
        else if( code == kbRight || c == '+' )
            {
            if( !n->children.empty() )
                {
                if( n->expanded == False )
                    expand( n, True );
                else
                    focusNode( n->children.front() );
                }
            }
        else if( code == kbLeft || c == '-' )
            {
            if( !n->children.empty() && n->expanded == True )
                expand( n, False );
            else if( n->parent != 0 )
                focusNode( n->parent );
            }
        else if( c == '*' )
            expandAll( n, True );
        else if( c == ' ' && markable == True )
            setMark( n, Boolean( markOf( n ) != mkAll ) );
        else if( event.keyDown.keyCode == kbEnter )
            {
            // and the dialog gets Enter for its default button
            selectItem( focused );
            done = False;
            }
        else
            done = False;
        if( done == True )
            {
            clearEvent( event );
            return;
            }
        }
    TListViewer::handleEvent( event );
}
