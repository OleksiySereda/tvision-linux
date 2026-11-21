/*------------------------------------------------------------*/
/*   Tests of TLogView, TTableView and the validators.        */
/*   make check-addons                                        */
/*------------------------------------------------------------*/

#define Uses_TLogView
#define Uses_TTableView
#define Uses_TTableHeader
#define Uses_TValidatedInputLine
#define Uses_TScrollBar
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

static Boolean evenLength( const char *s, void * )
{
    return Boolean( strlen( s ) % 2 == 0 );
}

static void testLog()
{
    TLogView *log = new TLogView( TRect( 0, 0, 20, 5 ), 0, 0, 10 );
    check( log->lineCount() == 0, "a new log is empty" );
    log->add( "one" );
    log->add( "two\nthree", TLogView::errorColor );
    check( log->lineCount() == 3, "'\\n' makes two lines" );
    check( strcmp( log->lineText( 2 ), "three" ) == 0 && log->lineColor( 2 ) == TLogView::errorColor &&
           log->lineColor( 0 ) == 0, "text and colour of the lines" );
    log->add( "a\tb" );
    check( strcmp( log->lineText( 3 ), "a       b" ) == 0, "tabs become blanks" );
    log->addf( "%d steps", 51 );
    check( strcmp( log->lineText( 4 ), "51 steps" ) == 0, "addf formats" );
    check( log->position().y == 0 && log->atEnd(), "5 lines fit in 5 rows" );

    for( int i = 0; i < 3; i++ )
        log->addf( "line %d", i );
    check( log->position().y == 3 && log->atEnd(), "at the end new lines scroll it" );
    log->scrollToPos( 0, 0 );
    log->add( "more" );
    check( log->position().y == 0, "scrolled up it stays" );

    for( int i = 0; i < 10; i++ )
        log->addf( "x%d", i );
    check( log->lineCount() == 10 && strcmp( log->lineText( 0 ), "x0" ) == 0,
           "only maxLines lines are kept" );

    log->scrollToEnd();
    check( log->position().y == 5, "scrollToEnd" );
    key( log, kbUp );
    check( log->position().y == 4, "Up scrolls one line" );
    key( log, kbHome );
    check( log->position().y == 0, "Home goes to the top" );
    key( log, kbPgDn );
    check( log->position().y == 4, "PgDn scrolls a page less one line" );
    key( log, kbEnd );
    check( log->position().y == 5, "End goes to the end" );

    log->add( "a long line, longer than the view is wide" );
    key( log, kbRight );
    check( log->position().x == 1, "Right scrolls sideways without a scroll bar" );

    const char *name = "widgets_test.log";
    check( log->saveToFile( name ), "saveToFile writes" );
    FILE *f = fopen( name, "r" );
    char line[80] = "";
    int n = 0;
    while( f != 0 && fgets( line, sizeof( line ), f ) != 0 )
        n++;
    if( f != 0 )
        fclose( f );
    remove( name );
    check( n == 10 && strncmp( line, "a long line", 11 ) == 0, "the file has the lines" );

    log->clear();
    check( log->lineCount() == 0 && log->position().y == 0 && log->position().x == 0, "clear" );
    TObject::destroy( log );

    // with a scroll bar the position goes to it as well
    TScrollBar *vs = new TScrollBar( TRect( 20, 0, 21, 5 ) );
    log = new TLogView( TRect( 0, 0, 20, 5 ), 0, vs );
    for( int i = 0; i < 20; i++ )
        log->addf( "%d", i );
    check( log->position().y == 15 && vs->value == 15, "the scroll bar follows" );
    TObject::destroy( log );
    TObject::destroy( vs );
}

static void testTable()
{
    TTableView *t = new TTableView( TRect( 0, 0, 30, 5 ), 0, 0 );
    TTableHeader *h = new TTableHeader( TRect( 0, 0, 30, 1 ), t );
    check( t->header == h, "the header attaches to the table" );
    t->addColumn( "Name", 6 );
    t->addColumn( "Port", 5, TTableView::alRight );
    int web = t->addRow( { "web", "8080" } );
    int db = t->addRow( { "db", "5432" } );
    int mail = t->addRow( { "Mail", "25" } );
    check( t->rowCount() == 3 && t->range == 3, "three rows" );
    check( strcmp( t->cell( mail, 1 ), "25" ) == 0, "cell" );
    check( t->formatRow( 0 ) == "web   \xB3 8080", "a row: padded cells and the divider" );
    check( t->columnAt( 2 ) == 0 && t->columnAt( 6 ) == -1 && t->columnAt( 8 ) == 1,
           "columnAt" );

    t->sortBy( 1 );
    check( t->rowAt( 0 ) == mail && t->rowAt( 1 ) == db && t->rowAt( 2 ) == web,
           "numbers sort as numbers" );
    check( t->formatHeader() == "Name  \xB3Port\x1E", "the sort column has an arrow" );
    t->sortBy( 0, False );
    check( t->rowAt( 0 ) == web && t->rowAt( 1 ) == mail && t->rowAt( 2 ) == db,
           "text sorts without case, here descending" );
    t->focusRow( db );
    check( t->focused == 2 && t->focusedRow() == db, "focusRow" );
    t->sortBy( -1 );
    check( t->rowAt( 0 ) == web && t->focusedRow() == db, "unsorted again; the focus stays on its row" );

    t->sortBy( 1 );
    int ftp = t->addRow( { "ftp", "21" } );
    check( t->rowAt( 0 ) == ftp, "a new row goes into its place" );

    t->markable = True;
    t->focusRow( web );
    key( t, ' ' );
    check( t->isMarked( web ) && t->markedCount() == 1, "Space marks" );
    check( t->formatRow( t->itemOf( web ) ) == "\xFB web   \xB3 8080", "a marked row shows the mark" );
    t->focusRow( ftp );
    key( t, kbIns );
    check( t->isMarked( ftp ) && t->focusedRow() == mail, "Ins marks and moves on" );
    key( t, kbUp );
    key( t, ' ' );
    check( !t->isMarked( ftp ) && t->markedCount() == 1, "Space again unmarks" );
    t->markAll( True );
    check( t->markedCount() == 4, "markAll" );
    t->sortBy( 0 );
    check( t->isMarked( mail ), "the marks stay with their rows" );

    t->removeRow( db );
    check( t->rowCount() == 3 && strcmp( t->cell( 2, 0 ), "ftp" ) == 0 && t->itemOf( 3 ) == -1,
           "removeRow renumbers the rows after it" );
    t->clear();
    check( t->rowCount() == 0 && t->range == 0 && t->focusedRow() == -1, "clear" );

    TObject::destroy( h );
    check( t->header == 0, "a destroyed header detaches" );
    TObject::destroy( t );
}

static void testValidators()
{
    TRangeValidator range( 1, 65535 );
    check( range.isValid( "1" ) && range.isValid( "65535" ) && range.isValid( " 80 " ),
           "range: numbers in it" );
    check( !range.isValid( "0" ) && !range.isValid( "65536" ) && !range.isValid( "" ) &&
           !range.isValid( "8a" ), "range: anything else" );
    check( range.isValidInput( "6553" ) && !range.isValidInput( "70000" ) &&
           !range.isValidInput( "-1" ) && !range.isValidInput( "1x" ),
           "range: typing" );
    check( TRangeValidator::value( "443" ) == 443, "range: value" );
    TRangeValidator neg( -10, 10 );
    check( neg.isValidInput( "-" ) && neg.isValid( "-10" ) && !neg.isValidInput( "-11" ),
           "range: negative numbers" );

    TFilterValidator hex( "0123456789abcdef" );
    check( hex.isValidInput( "c0ffee" ) && !hex.isValidInput( "coffee" ), "filter" );

    TLookupValidator yesno( "yes|no" );
    check( yesno.isValid( "YES" ) && yesno.isValid( "no" ) && !yesno.isValid( "maybe" ) &&
           !yesno.isValid( "" ), "lookup without case" );
    TLookupValidator exact( "Yes,No", ',', True );
    check( exact.isValid( "No" ) && !exact.isValid( "no" ), "lookup with case" );
    check( yesno.errorText( "" ) == "The value must be one of: yes, no", "lookup: the message" );

    TRegexValidator ip( "(\\d{1,3}\\.){3}\\d{1,3}", "Not an IPv4 address." );
    check( ip.isValid( "192.168.0.1" ) && !ip.isValid( "192.168.0" ) && !ip.isValid( "x192.168.0.1" ),
           "regex matches the whole text" );
    check( ip.errorText( "" ) == "Not an IPv4 address.", "regex: the message" );
    TRegexValidator bad( "(" );
    check( !bad.isValid( "(" ), "a bad expression accepts nothing" );

    TFuncValidator even( evenLength, "Even, please." );
    check( even.isValid( "ab" ) && !even.isValid( "abc" ), "function" );

    TValidator *v = new TRangeValidator( 1, 10 );
    v->allowEmpty = True;
    check( v->validate( "" ), "allowEmpty" );
    delete v;

    // typing into the line
    TValidatedInputLine *line = new TValidatedInputLine( TRect( 0, 0, 10, 1 ), 6,
                                                         new TRangeValidator( 1, 999 ) );
    line->setState( sfSelected, True );
    key( line, '4' );
    key( line, 'x' );
    key( line, '2' );
    check( strcmp( line->data, "42" ) == 0, "a letter is not taken" );
    key( line, '0' );
    key( line, '0' );
    check( strcmp( line->data, "420" ) == 0, "a digit that leaves the range is not taken" );
    check( line->valid( cmCancel ) && line->valid( cmReleasedFocus ), "Cancel and Tab do not check" );
    line->setValidator( 0 );
    key( line, 'x' );
    check( strcmp( line->data, "420x" ) == 0, "without a validator anything goes" );
    TObject::destroy( line );
}

int main()
{
    testLog();
    testTable();
    testValidators();
    printf( "%d checks, %d failed\n", checks, failures );
    return failures == 0 ? 0 : 1;
}
