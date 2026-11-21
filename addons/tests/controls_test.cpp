/*------------------------------------------------------------*/
/*   Tests of TProgressBar, TTabView, TTreeView, TComboBox    */
/*   and TAppSettings.  make check-addons                     */
/*------------------------------------------------------------*/

#define Uses_TProgressBar
#define Uses_TTabView
#define Uses_TTreeView
#define Uses_TComboBox
#define Uses_TAppSettings
#define Uses_TDialog
#define Uses_TInputLine
#define Uses_TButton
#define Uses_TMenuItem
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TRect
#include <tvaddons.h>

#include <stdio.h>
#include <string.h>
#include <string>

static int failures = 0, checks = 0;

static void check( bool ok, const char *what )
{
    checks++;
    if( !ok )
        {
        failures++;
        printf( "failed: %s\n", what );
        }
}

static void key( TView *v, ushort keyCode )
{
    TEvent e;
    memset( &e, 0, sizeof( e ) );
    e.what = evKeyDown;
    e.keyDown.keyCode = keyCode;
    v->handleEvent( e );
}

static void testProgress()
{
    TProgressBar *b = new TProgressBar( TRect( 0, 0, 20, 1 ), 200 );
    check( b->value() == 0 && b->percent() == 0, "progress: starts at 0" );
    b->setValue( 50 );
    check( b->percent() == 25, "progress: 50 of 200 is 25%" );
    b->setValue( 500 );
    check( b->value() == 200 && b->percent() == 100, "progress: not past the total" );
    b->setValue( -3 );
    check( b->value() == 0, "progress: not below 0" );
    b->setValue( 30 );
    b->setTotal( 20 );
    check( b->value() == 20 && b->total() == 20, "progress: a smaller total cuts the value" );
    b->setTotal( 0 );
    check( b->total() == 1, "progress: the total is at least 1" );
    TObject::destroy( b );
}

static void testTabs()
{
    TDialog *d = new TDialog( TRect( 0, 0, 60, 20 ), "Tabs" );
    TTabView *t = new TTabView( TRect( 1, 1, 59, 16 ) );
    TGroup *p1 = t->addPage( "~G~eneral" );
    TInputLine *name = new TInputLine( TRect( 2, 1, 30, 2 ), 21 );
    TInputLine *host = new TInputLine( TRect( 2, 3, 30, 4 ), 21 );
    p1->insert( name );
    p1->insert( host );
    TGroup *p2 = t->addPage( "~N~etwork" );
    TInputLine *port = new TInputLine( TRect( 2, 1, 10, 2 ), 6 );
    p2->insert( port );
    d->insert( t );
    d->insert( new TButton( TRect( 20, 17, 30, 19 ), "O~K~", cmOK, bfDefault ) );
    d->selectNext( False );

    check( t->pageCount() == 2 && t->currentPage() == 0, "tabs: two pages, the first shown" );
    check( ( p1->state & sfVisible ) != 0 && ( p2->state & sfVisible ) == 0, "tabs: only one page is visible" );
    check( p1->size.x == 58 && p1->size.y == 14 && p1->origin.y == 1, "tabs: a page is under the titles" );
    check( t->tabX( 0 ) == 1 && t->tabWidth( 0 ) == 9 && t->tabX( 1 ) == 11, "tabs: the places of the titles" );

    key( t, kbCtrlPgDn );
    check( t->currentPage() == 1 && ( p2->state & sfVisible ) != 0 && ( p1->state & sfVisible ) == 0,
           "tabs: Ctrl+PgDn shows the next page" );
    key( t, kbCtrlPgDn );
    check( t->currentPage() == 0, "tabs: and wraps round" );
    key( t, kbAltN );
    check( t->currentPage() == 1, "tabs: Alt and the letter of the title" );
    t->selectPage( 0 );

    // Tab goes through the page, then out of it (only with the focus,
    // which a dialog has on the desktop)
    d->setState( sfFocused, True );
    check( d->current == t && p1->current == name, "tabs: the focus starts in the first field" );
    key( t, kbTab );
    check( p1->current == host, "tabs: Tab goes to the next field of the page" );
    key( t, kbTab );
    check( p1->current == name, "tabs: past the last field the page starts over (the dialog moves on)" );

    // one record for the dialog: the pages in the order they were added
    struct { char name[21], host[21], port[6]; } rec, back;
    check( d->dataSize() == sizeof( rec ), "tabs: the data of every page" );
    memset( &rec, 0, sizeof( rec ) );
    strcpy( rec.name, "shop" );
    strcpy( rec.host, "web1" );
    strcpy( rec.port, "8080" );
    d->setData( &rec );
    check( strcmp( port->data, "8080" ) == 0, "tabs: setData reaches a hidden page" );
    d->getData( &back );
    check( memcmp( &rec, &back, sizeof( rec ) ) == 0, "tabs: getData gives it back" );

    TObject::destroy( d );
}

static void testTree()
{
    TTreeView *t = new TTreeView( TRect( 0, 0, 30, 10 ), 0, 0 );
    TTreeNode *web = t->addNode( 0, "web" );
    TTreeNode *web1 = t->addNode( web, "web1" );
    TTreeNode *web2 = t->addNode( web, "web2" );
    TTreeNode *db = t->addNode( 0, "db" );
    TTreeNode *db1 = t->addNode( db, "db1" );
    TTreeNode *deep = t->addNode( db1, "replica" );
    check( t->visibleCount() == 6 && t->range == 6, "tree: new nodes are open" );
    check( t->formatLine( 0 ) == "- web", "tree: an open node" );
    check( t->formatLine( 1 ) == "  \xC3\xC4\xC4 web1", "tree: a child with a sibling after it" );
    check( t->formatLine( 2 ) == "  \xC0\xC4\xC4 web2", "tree: the last child" );
    check( t->formatLine( 4 ) == "  \xC0\xC4- db1", "tree: a child with children" );
    check( t->formatLine( 5 ) == "    \xC0\xC4\xC4 replica", "tree: two levels down" );
    check( deep->depth() == 2 && web->depth() == 0, "tree: depth" );

    t->expand( web, False );
    check( t->visibleCount() == 4 && t->formatLine( 0 ) == "+ web" && t->nodeAt( 1 ) == db,
           "tree: closing hides the children" );
    t->focusNode( web2 );
    check( web->expanded == True && t->focusedNode() == web2, "tree: focusNode opens the parents" );
    key( t, kbLeft );
    check( t->focusedNode() == web, "tree: Left on a leaf goes to the parent" );
    key( t, kbLeft );
    check( web->expanded == False && t->focusedNode() == web, "tree: Left closes" );
    key( t, kbRight );
    check( web->expanded == True, "tree: Right opens" );
    key( t, kbRight );
    check( t->focusedNode() == web1, "tree: Right on an open node goes into it" );
    t->expandAll( 0, False );
    check( t->visibleCount() == 2, "tree: expandAll closes everything" );
    t->focusNode( db );
    key( t, '*' );
    check( t->visibleCount() == 4 && db1->expanded == True && web->expanded == False,
           "tree: '*' opens all of a node, only of it" );

    t->markable = True;
    t->focusNode( web );
    key( t, ' ' );
    check( t->markOf( web ) == TTreeView::mkAll && web1->marked && web2->marked,
           "tree: Space marks a node with its children" );
    t->setMark( web2, False );
    check( t->markOf( web ) == TTreeView::mkSome, "tree: partly marked" );
    check( t->formatLine( t->itemOf( web ) ) == "+ [-] web", "tree: [-] for a partly marked node" );
    t->setMark( db, True );
    std::vector<TTreeNode *> m = t->markedLeaves();
    check( m.size() == 2 && m[0] == web1 && m[1] == deep, "tree: the marked leaves in tree order" );

    t->focusNode( deep );
    t->removeNode( db1 );
    check( t->focusedNode() == db && t->visibleCount() == 2, "tree: removing a node keeps the focus near" );
    t->clear();
    check( t->visibleCount() == 0 && t->roots().empty(), "tree: clear" );
    TObject::destroy( t );
}

static void testCombo()
{
    TComboBox *c = new TComboBox( TRect( 0, 0, 20, 1 ), 11, "dev|test|prod" );
    check( c->itemCount() == 3 && strcmp( c->item( 2 ), "prod" ) == 0, "combo: the items" );
    check( c->findItem( "TEST" ) == 1 && c->findItem( "tes" ) == -1, "combo: findItem without case" );
    c->selectItem( 2 );
    check( strcmp( c->data, "prod" ) == 0, "combo: selectItem sets the text" );
    c->addItem( "staging" );
    check( c->itemCount() == 4, "combo: addItem" );
    c->setItems( "a,b", ',' );
    check( c->itemCount() == 2 && strcmp( c->item( 1 ), "b" ) == 0, "combo: another separator" );
    c->clearItems();
    check( c->itemCount() == 0 && c->openList() == -1, "combo: no list without items" );

    c->setItems( "dev|test|prod" );
    c->setListOnly( True );
    strcpy( c->data, "PROD" );
    check( c->valid( cmOK ) && strcmp( c->data, "prod" ) == 0,
           "combo: listOnly takes an item in any case and makes it the item's" );
    check( c->valid( cmCancel ), "combo: Cancel does not check" );
    TObject::destroy( c );
}

static void testSettings()
{
    const char *name = "controls_test.ini";
    remove( name );
    {
    TAppSettings s( "controls-test", name );
    check( s.fileName() == name && !s.load(), "settings: no file yet" );
    s.set( "general", "theme", "dark" );
    s.setInt( "general", "width", 120 );
    s.setBool( "general", "sound", True );
    s.set( "", "version", "3" );
    for( int i = 1; i <= 10; i++ )
        s.addRecent( ( "file" + std::to_string( i ) + ".txt" ).c_str() );
    s.addRecent( "file5.txt" );
    check( s.save(), "settings: save" );
    }

    // a comment of the user stays where it is
    FILE *f = fopen( name, "a" );
    fprintf( f, "; my note\n[other]\nkey = spaced value\n" );
    fclose( f );

    TAppSettings s( "controls-test", name );
    check( s.get( "general", "theme" ) == "dark" && s.getInt( "general", "width" ) == 120 &&
           s.getBool( "general", "sound" ) == True, "settings: the values come back" );
    check( s.get( "", "version" ) == "3", "settings: a key without a section" );
    check( s.get( "other", "key" ) == "spaced value", "settings: blanks round = are not part of it" );
    check( s.get( "general", "missing", "def" ) == "def" && s.getInt( "general", "theme", 7 ) == 7 &&
           s.getBool( "general", "theme", True ) == True, "settings: defaults" );
    std::vector<std::string> r = s.recent();
    check( r.size() == 8 && r[0] == "file5.txt" && r[1] == "file10.txt" && r[7] == "file3.txt",
           "settings: the recent files, newest first, at most 8" );
    TMenuItem *items = s.recentMenu( 1000 );
    check( items != 0 && items->command == 1000 && strcmp( items->name, "~1~ file5.txt" ) == 0 &&
           items->next != 0 && items->next->command == 1001, "settings: recentMenu" );
    while( items != 0 )
        {
        TMenuItem *n = items->next;
        delete items;
        items = n;
        }
    s.remove( "general", "width" );
    check( !s.has( "general", "width" ), "settings: remove" );
    s.save();

    FILE *g = fopen( name, "r" );
    std::string text;
    char line[200];
    while( fgets( line, sizeof( line ), g ) != 0 )
        text += line;
    fclose( g );
    check( text.find( "; my note\n" ) != std::string::npos && text.find( "version=3\n" ) == 0,
           "settings: comments stay; keys without a section come first" );

    // windows: kept inside the owner
    TGroup *desk = new TGroup( TRect( 0, 0, 80, 25 ) );
    TView *w = new TView( TRect( 10, 5, 50, 20 ) );
    desk->insert( w );
    s.saveWindow( "main", w );
    TRect small( 0, 0, 10, 5 );
    w->locate( small );
    check( s.restoreWindow( "main", w ) && w->origin.x == 10 && w->origin.y == 5 && w->size.x == 40,
           "settings: restoreWindow" );
    s.setInt( "window.main", "x", 70 );
    s.setInt( "window.main", "width", 100 );
    s.restoreWindow( "main", w );
    check( w->origin.x == 0 && w->size.x == 80, "settings: a window larger than the screen is cut" );
    check( !s.restoreWindow( "nothing", w ), "settings: nothing saved" );
    TObject::destroy( desk );

    // THistory lists
    historyAdd( 42, "alpha" );
    historyAdd( 42, "beta" );
    s.saveHistory( 42 );
    check( s.get( "history.42", "item1" ) == historyStr( 42, 0 ) &&
           s.get( "history.42", "item2" ) == historyStr( 42, 1 ), "settings: saveHistory" );
    s.set( "history.43", "item1", "gamma" );
    s.loadHistory( 43 );
    check( historyCount( 43 ) == 1 && strcmp( historyStr( 43, 0 ), "gamma" ) == 0, "settings: loadHistory" );

    remove( name );
}

// the history lists are made by TApplication; there is none here
void initHistory();

int main()
{
    initHistory();
    testProgress();
    testTabs();
    testTree();
    testCombo();
    testSettings();
    printf( "%d checks, %d failed\n", checks, failures );
    return failures == 0 ? 0 : 1;
}
