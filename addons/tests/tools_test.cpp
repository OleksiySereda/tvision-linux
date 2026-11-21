/*------------------------------------------------------------*/
/*   Tests of TWizard, TProcess, TCommandPalette,              */
/*   TMarkdownView, TDate and TDateInput, TSplitter, the      */
/*   themes, TStatusMessageLine and TSyntaxEditor.            */
/*   make check-addons                                        */
/*------------------------------------------------------------*/

#define Uses_TWizard
#define Uses_TProcess
#define Uses_TProcessView
#define Uses_TCommandPalette
#define Uses_TMarkdownView
#define Uses_TDateInput
#define Uses_TSplitter
#define Uses_TThemes
#define Uses_TToast
#define Uses_TStatusMessageLine
#define Uses_TSyntaxEditor
#define Uses_TAppSettings
#define Uses_TInputLine
#define Uses_TButton
#define Uses_TMenu
#define Uses_TMenuItem
#define Uses_TSubMenu
#define Uses_TStatusDef
#define Uses_TStatusItem
#define Uses_TProgram
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TRect
#include <tvaddons.h>

#include <stdio.h>
#include <string.h>
#include <chrono>
#include <string>
#include <thread>

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

static void command( TView *v, ushort c )
{
    TEvent e;
    memset( &e, 0, sizeof( e ) );
    e.what = evCommand;
    e.message.command = c;
    v->handleEvent( e );
}

// ---- TWizard

class TTestWizard : public TWizard
{
public:
    // TWindowInit is a virtual base: the class made last starts it
    TTestWizard() :
        TWindowInit( &TTestWizard::initFrame ),
        TWizard( TRect( 0, 0, 60, 18 ), "Install" ), shownLast( -1 ), allow( True ) {}
    virtual void pageShown( int i ) { shownLast = i; }
    virtual Boolean canLeave( int i, Boolean forwards )
        { return forwards == True && allow == False ? False : TWizard::canLeave( i, forwards ); }
    int shownLast;
    Boolean allow;
};

static void testWizard()
{
    TTestWizard *w = new TTestWizard;
    TPage *p1 = w->addPage( "~W~here" );
    TValidatedInputLine *dir = new TValidatedInputLine( TRect( 2, 1, 40, 2 ), 31,
                                                        new TRegexValidator( "/.*" ) );
    p1->insert( dir );
    TPage *p2 = w->addPage( "Options" );
    TInputLine *opt = new TInputLine( TRect( 2, 1, 20, 2 ), 11 );
    p2->insert( opt );
    TPage *p3 = w->addPage( "Done" );
    (void) p3;

    check( w->pageCount() == 3 && w->currentPage() == 0, "wizard: three pages, the first shown" );
    check( ( w->backButton->state & sfVisible ) == 0 && ( w->nextButton->state & sfVisible ) != 0 &&
           ( w->finishButton->state & sfVisible ) == 0, "wizard: no Back on the first page" );
    check( p1->origin.y == 3 && p1->size.y == 12, "wizard: a page between the header and the buttons" );

    w->allow = False;
    check( !w->next() && w->currentPage() == 0, "wizard: canLeave can keep the page" );
    w->allow = True;
    strcpy( dir->data, "/opt/shop" );
    command( w, cmWizardNext );
    check( w->currentPage() == 1 && w->shownLast == 1 && ( p2->state & sfVisible ) != 0 &&
           ( p1->state & sfVisible ) == 0, "wizard: Next shows the next page" );
    check( ( w->backButton->state & sfVisible ) != 0, "wizard: Back is there now" );
    command( w, cmWizardNext );
    check( w->currentPage() == 2 && ( w->finishButton->state & sfVisible ) != 0 &&
           ( w->nextButton->state & sfVisible ) == 0, "wizard: Finish on the last page" );
    command( w, cmWizardBack );
    command( w, cmWizardBack );
    check( w->currentPage() == 0, "wizard: Back twice" );

    struct { char dir[31], opt[11]; } rec, back;
    check( w->dataSize() == sizeof( rec ), "wizard: the data of every page" );
    memset( &rec, 0, sizeof( rec ) );
    strcpy( rec.dir, "/srv" );
    strcpy( rec.opt, "fast" );
    w->setData( &rec );
    w->getData( &back );
    check( strcmp( back.dir, "/srv" ) == 0 && strcmp( back.opt, "fast" ) == 0,
           "wizard: setData and getData" );
    TObject::destroy( w );
}

// ---- TProcess, TProcessView

static std::string runToEnd( TProcess& p, int maxMs = 5000 )
{
    std::string out;
    auto start = std::chrono::steady_clock::now();
    while( p.running() )
        {
        out += p.read();
        std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
        if( std::chrono::steady_clock::now() - start > std::chrono::milliseconds( maxMs ) )
            break;
        }
    return out + p.read();
}

static void testProcess()
{
#if defined( _WIN32 )
    const char *both = "echo hello& echo oops 1>&2& exit 3";
    const char *slow = "ping -n 20 127.0.0.1 >NUL";
    const char *where = "cd";
    const char *var = "echo %TOOLS_TEST%";
#else
    const char *both = "echo hello; echo oops >&2; exit 3";
    const char *slow = "sleep 20";
    const char *where = "pwd";
    const char *var = "echo $TOOLS_TEST";
#endif
    {
    TProcess p;
    check( p.start( both ), "process: start" );
    std::string out = runToEnd( p );
    check( out.find( "hello" ) != std::string::npos && out.find( "oops" ) != std::string::npos,
           "process: stdout and stderr" );
    check( p.exitCode() == 3 && !p.stopped(), "process: the exit code" );
    }
    {
    TProcess p;
    p.start( slow );
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    check( p.running(), "process: it runs" );
    p.stop();
    runToEnd( p, 3000 );
    check( !p.running() && p.stopped() && p.exitCode() == 143, "process: stop" );
    }
    {
    TProcess p;
#if defined( _WIN32 )
    p.start( where, "C:\\Windows" );
    check( runToEnd( p ).find( "Windows" ) != std::string::npos, "process: the directory" );
#else
    p.start( where, "/tmp" );
    check( runToEnd( p ).find( "/tmp" ) != std::string::npos, "process: the directory" );
#endif
    }
    {
    TProcess p;
    p.start( var, "", std::vector<std::string>{ "TOOLS_TEST=forty-two" } );
    check( runToEnd( p ).find( "forty-two" ) != std::string::npos, "process: the environment" );
    }

    TProcessView *v = new TProcessView( TRect( 0, 0, 60, 10 ), 0, 0 );
    v->start( both );
    for( int i = 0; i < 1000 && v->poll(); i++ )
        std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
    bool hello = false;
    for( int i = 0; i < v->lineCount(); i++ )
        if( strstr( v->lineText( i ), "hello" ) != 0 )
            hello = true;
    check( strncmp( v->lineText( 0 ), "$ ", 2 ) == 0 && hello &&
           strcmp( v->lineText( v->lineCount() - 1 ), "exit code 3" ) == 0 &&
           v->lineColor( v->lineCount() - 1 ) == TLogView::errorColor,
           "process view: the command, the output, the exit line" );
    TObject::destroy( v );
}

// ---- TCommandPalette

static void testPalette()
{
    check( TCommandPalette::score( "File > Save as...", "sav" ) > TCommandPalette::score( "File > Open", "sav" ),
           "palette: a match beats none" );
    check( TCommandPalette::score( "File > Open", "xyz" ) == 0, "palette: no match" );
    check( TCommandPalette::score( "Edit > Select all", "all" ) > TCommandPalette::score( "Edit > Smallest", "all" ),
           "palette: the start of a word beats the middle" );
    check( TCommandPalette::score( "Search > Find", "sfd" ) > 0, "palette: letters in order" );

    TMenu *m = new TMenu(
        *new TMenuItem( "~F~ile", kbAltF, new TMenu(
            *new TMenuItem( "~O~pen...", 1000, kbF3, hcNoContext, "F3",
                new TMenuItem( "~S~ave", 1001, kbF2, hcNoContext, "F2" ) ) ) ) );
    std::vector<TPaletteEntry> v;
    TCommandPalette::collect( m, "", v );
    check( v.size() == 2 && v[0].name == "File > Open..." && v[0].key == "F3" && v[1].command == 1001,
           "palette: the commands of a menu with their path" );
    delete m;
}

// ---- TMarkdownView

static int findLine( TMarkdownView *v, const char *text )
{
    for( int i = 0; i < v->lineCount(); i++ )
        if( v->lineText( i ).find( text ) != std::string::npos )
            return i;
    return -1;
}

static void testMarkdown()
{
    TMarkdownView *v = new TMarkdownView( TRect( 0, 0, 40, 10 ), 0, 0 );
    v->setText(
        "# Title\n"
        "\n"
        "Some **bold** and `code` text that is long enough to be wrapped at forty columns.\n"
        "\n"
        "- one\n"
        "- two\n"
        "  1. nested\n"
        "\n"
        "| Name | Port |\n"
        "|------|-----:|\n"
        "| web  | 80   |\n"
        "\n"
        "```\n"
        "int  x;\n"
        "```\n"
        "\n"
        "See [the end](#the-end) or [site](https://example.com).\n"
        "\n"
        "## The End\n" );
    check( v->lineText( 0 ) == " Title" && v->lineText( 1 ) == " \xCD\xCD\xCD\xCD\xCD",
           "markdown: a heading, underlined" );
    int p = findLine( v, "Some bold" );
    check( p >= 0 && findLine( v, "columns." ) > p, "markdown: marks taken out, the paragraph wrapped" );
    for( int i = 0; i < v->lineCount(); i++ )
        if( v->lineText( i ).size() > 39 && v->lineText( i ).find( "int" ) == std::string::npos )
            check( false, "markdown: no line wider than the view" );
    check( findLine( v, "\x07 one" ) >= 0 && findLine( v, "\x07 two" ) >= 0, "markdown: bullets" );
    check( findLine( v, "   1. nested" ) >= 0, "markdown: a nested numbered item" );
    int t = findLine( v, "Name" );
    check( t >= 0 && v->lineText( t ) == " Name \xB3 Port" && v->lineText( t + 2 ) == " web  \xB3 80  ",
           "markdown: a table" );
    check( findLine( v, "   int  x;" ) >= 0, "markdown: a code block as it is" );
    check( v->linkCount() == 2 && v->linkTarget( 0 ) == "#the-end" &&
           v->linkTarget( 1 ) == "https://example.com", "markdown: the links" );
    check( TMarkdownView::anchorOf( "Building on **Windows**!" ) == "building-on-windows",
           "markdown: anchors as GitHub makes them" );
    check( v->gotoAnchor( "#the-end" ) && v->position().y > 0, "markdown: gotoAnchor scrolls" );
    key( v, kbHome );
    v->focusedLink = -1;
    key( v, kbTab );
    check( v->focusedLink == 0, "markdown: Tab goes to the first link" );
    key( v, kbEnter );
    check( v->position().y == findLine( v, "The End" ) || v->position().y > 0,
           "markdown: Enter follows a #link" );
    TObject::destroy( v );
}

// ---- TDate, TDateInput

static void testDates()
{
    check( TDate( 2026, 11, 22 ).weekday() == 6 && TDate( 2024, 2, 29 ).weekday() == 3,
           "date: weekdays (Monday = 0)" );
    check( TDate( 2024, 2, 29 ).isValid() && !TDate( 2023, 2, 29 ).isValid() &&
           !TDate( 2026, 13, 1 ).isValid() && !TDate( 2026, 4, 31 ).isValid(), "date: isValid" );
    check( TDate( 2026, 12, 31 ).addDays( 1 ) == TDate( 2027, 1, 1 ) &&
           TDate( 2024, 3, 1 ).addDays( -1 ) == TDate( 2024, 2, 29 ), "date: addDays" );
    check( TDate( 2026, 1, 31 ).addMonths( 1 ) == TDate( 2026, 2, 28 ) &&
           TDate( 2026, 11, 15 ).addMonths( -12 ) == TDate( 2025, 11, 15 ), "date: addMonths" );
    check( formatDate( TDate( 2026, 3, 7 ), dfISO ) == "2026-03-07" &&
           formatDate( TDate( 2026, 3, 7 ), dfDMY ) == "07.03.2026" &&
           formatDate( TDate( 2026, 3, 7 ), dfMDY ) == "03/07/2026", "date: the formats" );
    TDate d;
    check( parseDate( "07.03.2026", dfDMY, d ) && d == TDate( 2026, 3, 7 ) &&
           !parseDate( "2026-02-30", dfISO, d ) && !parseDate( "2026-3-7", dfISO, d ), "date: parse" );
    TDateValidator iso;
    check( iso.isValidInput( "2026-0" ) && !iso.isValidInput( "2026/" ) && !iso.isValidInput( "20a" ),
           "date: typing" );

    TDateInput *in = new TDateInput( TRect( 0, 0, 12, 1 ), dfDMY );
    in->setState( sfSelected, True );
    const char *typed = "22112026";
    for( const char *p = typed; *p != 0; p++ )
        key( in, ushort( *p ) );
    check( strcmp( in->data, "22.11.2026" ) == 0 && in->date() == TDate( 2026, 11, 22 ),
           "date input: the dots come by themselves" );
    in->setDate( TDate( 2027, 1, 2 ) );
    check( strcmp( in->data, "02.01.2027" ) == 0, "date input: setDate" );
    TObject::destroy( in );
}

// ---- TSplitter

static void testSplitter()
{
    TGroup *g = new TGroup( TRect( 0, 0, 80, 20 ) );
    TView *left = new TView( TRect( 0, 0, 30, 20 ) );
    TView *bar = new TView( TRect( 29, 0, 30, 20 ) );
    TView *right = new TView( TRect( 31, 0, 80, 20 ) );
    TSplitter *s = new TSplitter( TRect( 30, 0, 31, 20 ), True );
    g->insert( left );
    g->insert( bar );
    g->insert( right );
    g->insert( s );
    left->growMode = bar->growMode = right->growMode = 0;
    s->addFirst( left );
    s->addFirst( bar, TSplitter::smMove );
    s->addSecond( right );
    s->setPosition( 40 );
    check( s->position() == 40 && left->size.x == 40 && bar->origin.x == 39 && right->origin.x == 41 &&
           right->size.x == 39, "splitter: the views follow" );
    s->setPosition( 1 );
    check( s->position() == 3 && left->size.x == 3, "splitter: not past minFirst" );
    s->setPosition( 79 );
    check( s->position() == 76 && right->size.x == 3, "splitter: not past minSecond" );

    TAppSettings set( "tools-test", "tools_test.ini" );
    s->setPosition( 25 );
    s->save( set, "main" );
    s->setPosition( 50 );
    s->restore( set, "main" );
    check( s->position() == 25, "splitter: save and restore" );
    TObject::destroy( g );
}

// ---- themes, status messages

static void testThemes()
{
    check( findTheme( "dark" ) == thDark && findTheme( "HIGH CONTRAST" ) == thHighContrast &&
           findTheme( "pink" ) == -1, "themes: by name" );
    for( int i = 0; i < themeCount; i++ )
        check( themePalette( i ) != 0 && strlen( themeName( i ) ) > 0, "themes: a palette for each" );
    check( memcmp( themePalette( thClassic ), cpColor, 63 ) == 0, "themes: Classic is cpColor" );
    check( !applyTheme( thDark ), "themes: nothing to apply without a program" );
    TMenuItem *m = themeMenu( 500 );
    check( m != 0 && m->command == 500 && m->next->next->next->command == 503 &&
           m->next->next->next->next == 0, "themes: the menu" );
    while( m != 0 )
        {
        TMenuItem *n = m->next;
        delete m;
        m = n;
        }

    TStatusMessageLine *sl = new TStatusMessageLine( TRect( 0, 24, 80, 25 ),
        *new TStatusDef( 0, 0xFFFF ) + *new TStatusItem( "~F1~ Help", kbF1, cmHelp ) );
    sl->message( "Saved", 1 );
    check( sl->hasMessage(), "status message: shown" );
    TToast::idle();
    check( sl->hasMessage(), "status message: still there" );
    sl->tick( toastClock() + 1500 );
    check( !sl->hasMessage(), "status message: gone when its time is up" );
    check( TToast::show( "x" ) == 0 && TToast::count() == 0, "toast: none without a desktop" );
    TObject::destroy( sl );
}

// ---- TSyntaxEditor

static std::string classes( int lang, const char *s, int state = 0, int *out = 0 )
{
    uchar cls[256];
    int n = int( strlen( s ) );
    int st = TSyntaxEditor::scanText( lang, s, n, state, cls );
    if( out != 0 )
        *out = st;
    std::string r;
    for( int i = 0; i < n; i++ )
        r += char( cls[i] < 10 ? '0' + cls[i] : 'a' + cls[i] - 10 );
    return r;
}

static void testSyntax()
{
    check( TSyntaxEditor::languageFor( "src/main.cpp" ) == slCpp &&
           TSyntaxEditor::languageFor( "deploy.SH" ) == slShell &&
           TSyntaxEditor::languageFor( "Makefile" ) == slShell &&
           TSyntaxEditor::languageFor( ".env" ) == slIni && TSyntaxEditor::languageFor( ".env.local" ) == slIni &&
           TSyntaxEditor::languageFor( "a.json" ) == slJson && TSyntaxEditor::languageFor( "README.md" ) == slMarkdown &&
           TSyntaxEditor::languageFor( "x.yml" ) == slYaml && TSyntaxEditor::languageFor( "notes" ) == slPlain,
           "syntax: the language from the file name" );

    // 1 keyword, 2 type, 3 string, 4 number, 5 comment, 6 preprocessor,
    // 7 punctuation, 8 variable, 9 heading, a key, b section, c constant,
    // d emphasis, e code
    check( classes( slCpp, "int x = 42; // hi" ) == "22200070447055555", "syntax: C++ type, number, comment" );
    check( classes( slCpp, "#include <a.h>" ) == "66666666033333", "syntax: C++ preprocessor" );
    check( classes( slCpp, "return \"a\\\"b\";" ) == "11111103333337", "syntax: C++ keyword and string" );
    int st = 0;
    check( classes( slCpp, "x /* open", 0, &st ) == "005555555" && st == 1, "syntax: a block comment opens" );
    check( classes( slCpp, "still */ y", 1, &st ) == "5555555500" && st == 0, "syntax: and closes" );
    check( classes( slShell, "if [ \"$HOME\" ]; then # c" ) == "110703888883077011110555",
           "syntax: shell keyword, string with a variable, comment" );
    check( classes( slShell, "NAME=1 ${X}" ) == "88887408888", "syntax: shell assignment and ${X}" );
    check( classes( slIni, "[main]" ) == "bbbbbb" && classes( slIni, "; c" ) == "555",
           "syntax: INI section and comment" );
    check( classes( slIni, "port = 8080" ) == "aaaaa704444" &&
           classes( slIni, "export DB=\"x\"" ) == "1111110aa7333", "syntax: INI and .env" );
    check( classes( slJson, "{\"a\": [1, true, \"s\"]}" ) == "7aaa707470cccc7033377",
           "syntax: JSON key, number, constant, string" );
    check( classes( slMarkdown, "# Title" ) == "9999999" && classes( slMarkdown, "- **b** `c`" ) == "7077d770eee",
           "syntax: Markdown heading, list, bold, code" );
    check( classes( slMarkdown, "```", 0, &st ) == "eee" && st == 1 &&
           classes( slMarkdown, "# not", 1 ) == "eeeee", "syntax: a code fence" );
}

void initHistory();

int main()
{
    initHistory();
    testWizard();
    testProcess();
    testPalette();
    testMarkdown();
    testDates();
    testSplitter();
    testThemes();
    testSyntax();
    remove( "tools_test.ini" );
    printf( "%d checks, %d failed\n", checks, failures );
    return failures == 0 ? 0 : 1;
}
