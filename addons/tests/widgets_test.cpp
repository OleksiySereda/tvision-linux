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

    t->setFilter( "WE" );
    check( t->range == 1 && t->rowAt( 0 ) == web && t->rowShown( web ) && !t->rowShown( db ),
           "the filter shows the rows with the text in a cell, without case" );
    check( t->isMarked( mail ) && t->markedCount() == 4, "hidden rows keep their marks" );
    int www = t->addRow( { "www", "80" } );
    int gw = t->addRow( { "Wedge", "1" } );
    check( !t->rowShown( www ) && t->rowShown( gw ) && t->range == 2,
           "a new row is shown only if it matches" );
    t->setFilter( "" );
    check( t->range == 6 && t->rowShown( db ), "an empty filter shows them all" );
    t->removeRow( gw );
    t->removeRow( www );

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

    // IPv4 addresses
    TIPAddressValidator ipv4;
    unsigned long addr = 0;
    check( TIPAddressValidator::parse( "192.168.0.1", addr ) && addr == 0xC0A80001UL,
           "ip: parse gives the address" );
    check( ipv4.isValid( "0.0.0.0" ) && ipv4.isValid( "255.255.255.255" ), "ip: the ends" );
    check( !ipv4.isValid( "256.1.1.1" ) && !ipv4.isValid( "1.2.3" ) && !ipv4.isValid( "1.2.3.4.5" ) &&
           !ipv4.isValid( "01.2.3.4" ) && !ipv4.isValid( "1..3.4" ) && !ipv4.isValid( "" ) &&
           !ipv4.isValid( "1.2.3.4 " ), "ip: not addresses" );
    check( ipv4.isValidInput( "" ) && ipv4.isValidInput( "192.168." ) && ipv4.isValidInput( "10" ),
           "ip: typing the start" );
    check( !ipv4.isValidInput( "1921" ) && !ipv4.isValidInput( "300" ) && !ipv4.isValidInput( "1..") &&
           !ipv4.isValidInput( "1.2.3.4." ) && !ipv4.isValidInput( "1a" ), "ip: keys not taken" );

    // networks
    TNetworkValidator net, strictNet( True );
    int prefix = -1;
    check( TNetworkValidator::parse( "10.0.0.0/8", addr, prefix ) && addr == 0x0A000000UL &&
           prefix == 8, "network: parse" );
    check( net.isValid( "10.1.2.3/8" ) && !strictNet.isValid( "10.1.2.3/8" ) &&
           strictNet.isValid( "192.168.1.0/24" ) && strictNet.isValid( "0.0.0.0/0" ),
           "network: strict wants the host bits 0" );
    check( !net.isValid( "10.0.0.0" ) && !net.isValid( "10.0.0.0/33" ) && !net.isValid( "10.0.0.0/" ) &&
           !net.isValid( "10.0.0.0/08" ), "network: not networks" );
    check( net.isValidInput( "10.0.0.0/" ) && net.isValidInput( "10.0.0.0/2" ) &&
           !net.isValidInput( "10.0.0.0/40" ) && !net.isValidInput( "10.0.0.0/8/" ), "network: typing" );

    // masks
    TNetMaskValidator mask;
    check( TNetMaskValidator::prefix( "255.255.255.0" ) == 24 && TNetMaskValidator::prefix( "/24" ) == 24 &&
           TNetMaskValidator::prefix( "24" ) == 24 && TNetMaskValidator::prefix( "0.0.0.0" ) == 0 &&
           TNetMaskValidator::prefix( "255.255.255.255" ) == 32 &&
           TNetMaskValidator::prefix( "255.255.240.0" ) == 20, "mask: the prefix of each form" );
    check( !mask.isValid( "255.0.255.0" ) && !mask.isValid( "255.255.255.1" ) && !mask.isValid( "/33" ) &&
           !mask.isValid( "" ), "mask: the ones must come first" );
    check( mask.isValidInput( "/2" ) && mask.isValidInput( "255.255." ) && !mask.isValidInput( "/333" ),
           "mask: typing" );

    // host names
    THostValidator host, nameOnly( False );
    check( host.isValid( "db1.internal" ) && host.isValid( "localhost" ) && host.isValid( "10.0.0.5" ) &&
           host.isValid( "a-b.example.com" ), "host: names and addresses" );
    check( !host.isValid( "-a.com" ) && !host.isValid( "a-.com" ) && !host.isValid( "a..com" ) &&
           !host.isValid( "a.com." ) && !host.isValid( "999.1.1.1" ) && !host.isValid( "a_b.com" ) &&
           !host.isValid( "" ), "host: not names" );
    check( !nameOnly.isValid( "10.0.0.5" ) && nameOnly.isValid( "web1" ), "host: without addresses" );
    std::string label63( 63, 'a' ), label64( 64, 'a' );
    check( host.isValid( label63.c_str() ) && !host.isValid( label64.c_str() ), "host: labels up to 63" );
    check( host.isValidInput( "db1." ) && !host.isValidInput( "db 1" ), "host: typing" );

    // e-mail
    TEmailValidator mail;
    check( mail.isValid( "name@example.com" ) && mail.isValid( "first.last+tag@mail.example.org" ),
           "email: addresses" );
    check( !mail.isValid( "name@localhost" ) && !mail.isValid( "@example.com" ) &&
           !mail.isValid( "name@" ) && !mail.isValid( "a@b@c.com" ) && !mail.isValid( ".a@b.com" ) &&
           !mail.isValid( "a..b@c.com" ) && !mail.isValid( "a.@b.com" ) && !mail.isValid( "a@-b.com" ),
           "email: not addresses" );
    check( mail.isValidInput( "name@" ) && !mail.isValidInput( "na me" ) && !mail.isValidInput( "a@b@" ),
           "email: typing" );

    // passwords
    TPasswordValidator pw;
    check( pw.isValid( "Secret42" ) && !pw.isValid( "secret42" ) && !pw.isValid( "Sec42" ) &&
           !pw.isValid( "SecretXY" ), "password: length, digit, upper, lower" );
    check( pw.errorText( "abc" ) ==
           "The password needs at least 8 characters, a digit and an upper case letter.",
           "password: the message says what is missing" );
    TPasswordValidator symbol( 4, pwSymbol );
    check( symbol.isValid( "ab!c" ) && !symbol.isValid( "abcd" ) &&
           symbol.errorText( "abcd" ) == "The password needs a symbol such as ! or #.",
           "password: a symbol" );
    check( TPasswordValidator::kinds( "aB3 " ) == ( pwLower | pwUpper | pwDigit | pwSymbol ),
           "password: kinds" );

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
    line->password = True;
    line->draw();
    check( strcmp( line->data, "420x" ) == 0, "a password line keeps the real text" );
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
