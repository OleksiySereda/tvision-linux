/*------------------------------------------------------------*/
/*   CONTROLS: the add-on controls in one program             */
/*                                                            */
/*     Demo | Settings   a dialog with tabs, combo boxes and  */
/*                       validated lines                      */
/*     Demo | Servers    a tree with marks                    */
/*     Demo | Progress   a long job with a progress dialog    */
/*     Demo | Wizard     a wizard of three steps              */
/*     Demo | Run        a command with its output            */
/*     Demo | README     README.md in a Markdown view         */
/*     Demo | Split      two views with a splitter            */
/*     Options | Theme   the colour themes                    */
/*     Ctrl+P            the command palette                  */
/*     File | Open       a file in an editor that colours it  */
/*                       (and the recent files list)          */
/*                                                            */
/*   What was entered, the recent files and where the         */
/*   dialogs were moved to are kept with TAppSettings         */
/*   (~/.config/tvcontrols/settings.ini, or                   */
/*   %APPDATA%/tvcontrols/settings.ini).                      */
/*------------------------------------------------------------*/

#define Uses_TApplication
#define Uses_TDeskTop
#define Uses_TDialog
#define Uses_TButton
#define Uses_TLabel
#define Uses_TStaticText
#define Uses_TCheckBoxes
#define Uses_TSItem
#define Uses_TScrollBar
#define Uses_TRect
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TMenuBar
#define Uses_TSubMenu
#define Uses_TMenuItem
#define Uses_TMenu
#define Uses_TStatusLine
#define Uses_TStatusDef
#define Uses_TStatusItem
#define Uses_TFileDialog
#define Uses_MsgBox
#define Uses_TTabView
#define Uses_TComboBox
#define Uses_TValidatedInputLine
#define Uses_TTreeView
#define Uses_TProgressDialog
#define Uses_TAppSettings
#define Uses_TWizard
#define Uses_TProcessDialog
#define Uses_TCommandPalette
#define Uses_TMarkdownWindow
#define Uses_TDateInput
#define Uses_TSplitter
#define Uses_TThemes
#define Uses_TToast
#define Uses_TStatusMessageLine
#define Uses_TSyntaxEditWindow
#define Uses_TEditorDialogs
#define Uses_TWindow
#include <tvaddons.h>

#include <dir.h>
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <string>
#include <thread>

const ushort
    cmSettings = 100,
    cmServers  = 101,
    cmProgress = 102,
    cmOpenFile = 103,
    cmWizard   = 104,
    cmRun      = 105,
    cmPalette  = 106,
    cmReadme   = 107,
    cmSplit    = 108,
    cmRecent   = 200,               // 200..207: the recent files
    cmTheme    = 300;               // 300..303: the themes

// Ctrl+P as the keyboard sends it: the scan code of P and the control
// character (kbCtrlP is the character alone, which a status line or a
// menu would not match)
const ushort keyCtrlP = 0x1910;

static TAppSettings *settings = 0;

// the data of the settings dialog, page by page, in the order of the
// controls (TTabView hands it to every page); packed, as the controls
// put their data one after the other
#pragma pack( push, 1 )
struct SettingsData
{
    char name[31];                  // General
    char environment[11];
    char theme[11];
    char start[11];
    ushort options;
    char address[16];               // Network
    char mask[19];
    char host[41];
    char port[6];
    char email[41];                 // Account
    char password[31];
};
#pragma pack( pop )

static const char *const fieldKeys[] =
    { "name", "environment", "theme", "start", 0, "address", "mask", "host", "port", "email", 0 };
const int fieldCount = 11;

static void loadData( SettingsData& d )
{
    memset( &d, 0, sizeof( d ) );
    char *fields[] = { d.name, d.environment, d.theme, d.start, 0, d.address, d.mask, d.host,
                       d.port, d.email, 0 };
    size_t sizes[] = { sizeof( d.name ), sizeof( d.environment ), sizeof( d.theme ),
                       sizeof( d.start ), 0, sizeof( d.address ), sizeof( d.mask ),
                       sizeof( d.host ), sizeof( d.port ), sizeof( d.email ), 0 };
    for( int i = 0; i < fieldCount; i++ )
        if( fieldKeys[i] != 0 )
            {
            std::string v = settings->get( "settings", fieldKeys[i] );
            strncpy( fields[i], v.c_str(), sizes[i] - 1 );
            }
    d.options = ushort( settings->getInt( "settings", "options", 1 ) );
    // the password is not kept
}

static void saveData( const SettingsData& d )
{
    const char *fields[] = { d.name, d.environment, d.theme, d.start, 0, d.address, d.mask,
                             d.host, d.port, d.email, 0 };
    for( int i = 0; i < fieldCount; i++ )
        if( fieldKeys[i] != 0 )
            settings->set( "settings", fieldKeys[i], fields[i] );
    settings->setInt( "settings", "options", d.options );
}

// a label, a line and, for a combo box, its button
static void field( TGroup *g, int y, const char *label, TInputLine *line, Boolean combo = False )
{
    g->insert( line );
    g->insert( new TLabel( TRect( 2, y, 14, y + 1 ), label, line ) );
    if( combo == True )
        {
        TRect r = line->getBounds();
        g->insert( new TComboButton( TRect( r.b.x, y, r.b.x + 3, y + 1 ),
                                     (TComboBox *) line ) );
        }
}

static TDialog *settingsDialog()
{
    TDialog *d = new TDialog( TRect( 0, 0, 64, 19 ), "Settings" );
    d->options |= ofCentered;
    TTabView *tabs = new TTabView( TRect( 2, 1, 62, 15 ) );

    TGroup *p = tabs->addPage( "~G~eneral" );
    field( p, 1, "~N~ame", new TValidatedInputLine( TRect( 15, 1, 47, 2 ), 31,
                                                     new TRegexValidator( ".+", "A name, please." ) ) );
    TComboBox *env = new TComboBox( TRect( 15, 3, 28, 4 ), 11, "dev|test|staging|prod" );
    env->setListOnly( True );
    field( p, 3, "~E~nvironment", env, True );
    field( p, 5, "~T~heme", new TComboBox( TRect( 15, 5, 28, 6 ), 11, "blue|gray|dark|light" ), True );
    TDateInput *start = new TDateInput( TRect( 15, 7, 28, 8 ) );
    start->validator->allowEmpty = True;
    p->insert( start );
    p->insert( new TLabel( TRect( 2, 7, 14, 8 ), "~S~tart on", start ) );
    p->insert( new TDateButton( TRect( 28, 7, 31, 8 ), start ) );
    TCheckBoxes *opts = new TCheckBoxes( TRect( 15, 9, 47, 12 ),
        new TSItem( "~S~ave on exit",
        new TSItem( "Show ~h~ints",
        new TSItem( "~V~erbose log", 0 ) ) ) );
    p->insert( opts );
    p->insert( new TLabel( TRect( 2, 9, 14, 10 ), "~O~ptions", opts ) );

    p = tabs->addPage( "~N~etwork" );
    field( p, 1, "~A~ddress", new TValidatedInputLine( TRect( 15, 1, 33, 2 ), 16,
                                                        new TIPAddressValidator ) );
    field( p, 3, "~M~ask", new TValidatedInputLine( TRect( 15, 3, 36, 4 ), 19,
                                                     new TNetMaskValidator ) );
    field( p, 5, "~H~ost", new TValidatedInputLine( TRect( 15, 5, 57, 6 ), 41,
                                                     new THostValidator ) );
    field( p, 7, "~P~ort", new TValidatedInputLine( TRect( 15, 7, 23, 8 ), 6,
                                                     new TRangeValidator( 1, 65535 ) ) );

    p = tabs->addPage( "~A~ccount" );
    field( p, 1, "~E~-mail", new TValidatedInputLine( TRect( 15, 1, 57, 2 ), 41,
                                                       new TEmailValidator ) );
    TValidatedInputLine *pw = new TValidatedInputLine( TRect( 15, 3, 47, 4 ), 31,
                                                       new TPasswordValidator );
    pw->password = True;
    pw->validator->allowEmpty = True;
    field( p, 3, "~P~assword", pw );
    p->insert( new TStaticText( TRect( 15, 5, 57, 7 ),
        "8 characters or more, with a digit, an upper and a lower case letter." ) );

    d->insert( tabs );
    d->insert( new TButton( TRect( 38, 16, 49, 18 ), "O~K~", cmOK, bfDefault ) );
    d->insert( new TButton( TRect( 50, 16, 61, 18 ), "Cancel", cmCancel, bfNormal ) );
    d->selectNext( False );
    return d;
}

static TDialog *serversDialog( TTreeView *&tree )
{
    TDialog *d = new TDialog( TRect( 0, 0, 44, 20 ), "Servers" );
    d->options |= ofCentered;
    TScrollBar *vs = new TScrollBar( TRect( 41, 2, 42, 15 ) );
    d->insert( vs );
    tree = new TTreeView( TRect( 2, 2, 41, 15 ), 0, vs );
    tree->markable = True;
    const char *groups[][4] =
        { { "web", "web1", "web2", "web3" }, { "db", "db1", "db2", 0 },
          { "cache", "redis1", 0, 0 } };
    TTreeNode *all = tree->addNode( 0, "production" );
    for( auto& g : groups )
        {
        TTreeNode *n = tree->addNode( all, g[0] );
        for( int i = 1; i < 4 && g[i] != 0; i++ )
            tree->addNode( n, g[i] );
        }
    TTreeNode *test = tree->addNode( 0, "test" );
    tree->addNode( test, "test1" );
    tree->expand( test, False );
    d->insert( tree );
    d->insert( new TLabel( TRect( 1, 1, 20, 2 ), "~S~ervers", tree ) );
    d->insert( new TStaticText( TRect( 2, 15, 42, 17 ),
        "Space marks, Right/Left open and close, '*' opens all." ) );
    d->insert( new TButton( TRect( 18, 17, 29, 19 ), "O~K~", cmOK, bfDefault ) );
    d->insert( new TButton( TRect( 30, 17, 41, 19 ), "Cancel", cmCancel, bfNormal ) );
    d->selectNext( False );
    return d;
}

class TControlsApp : public TApplication
{

public:

    TControlsApp() :
        TProgInit( &TControlsApp::initStatusLine, &TControlsApp::initMenuBar,
                   &TControlsApp::initDeskTop )
    {
    }

    static TMenuBar *initMenuBar( TRect r )
    {
        r.b.y = r.a.y + 1;
        // Open, a line, the recent files, Exit
        TMenuItem *exit = new TMenuItem( "E~x~it", cmQuit, kbAltX, hcNoContext, "Alt-X" );
        TMenuItem *recent = settings->recentMenu( cmRecent, exit );
        TMenuItem& line = newLine();
        line.next = recent;
        TMenuItem *fileItems = new TMenuItem( "~O~pen...", cmOpenFile, kbF3, hcNoContext, "F3",
                                              &line );
        return new TMenuBar( r,
            *new TSubMenu( "~F~ile", kbAltF ) + *fileItems +
            *new TSubMenu( "~D~emo", kbAltD ) +
                *new TMenuItem( "~S~ettings...", cmSettings, kbF2, hcNoContext, "F2" ) +
                *new TMenuItem( "Se~r~vers...", cmServers, kbF4, hcNoContext, "F4" ) +
                *new TMenuItem( "~P~rogress", cmProgress, kbF5, hcNoContext, "F5" ) +
                *new TMenuItem( "~W~izard...", cmWizard, kbF6, hcNoContext, "F6" ) +
                *new TMenuItem( "R~u~n a command...", cmRun, kbF7, hcNoContext, "F7" ) +
                *new TMenuItem( "R~E~ADME", cmReadme, kbF8, hcNoContext, "F8" ) +
                *new TMenuItem( "Sp~l~it view", cmSplit, kbNoKey ) +
                newLine() +
                *new TMenuItem( "~C~ommand palette...", cmPalette, keyCtrlP, hcNoContext, "Ctrl-P" ) +
            *new TSubMenu( "~O~ptions", kbAltO ) +
                *new TMenuItem( "~T~heme", kbNoKey, new TMenu( *themeMenu( cmTheme ) ) ) );
    }

    static TStatusLine *initStatusLine( TRect r )
    {
        r.a.y = r.b.y - 1;
        return new TStatusMessageLine( r,
            *new TStatusDef( 0, 0xFFFF ) +
                *new TStatusItem( "~F2~ Settings", kbF2, cmSettings ) +
                *new TStatusItem( "~F3~ Open", kbF3, cmOpenFile ) +
                *new TStatusItem( "~F4~ Servers", kbF4, cmServers ) +
                *new TStatusItem( "~F5~ Progress", kbF5, cmProgress ) +
                *new TStatusItem( "~F6~ Wizard", kbF6, cmWizard ) +
                *new TStatusItem( "~F7~ Run", kbF7, cmRun ) +
                *new TStatusItem( "~Ctrl-P~ Commands", keyCtrlP, cmPalette ) +
                *new TStatusItem( 0, kbF8, cmReadme ) +
                *new TStatusItem( "~Alt-X~ Exit", kbAltX, cmQuit ) );
    }

    // the toasts and the status message go when their time is up
    virtual void idle()
    {
        TApplication::idle();
        TToast::idle();
    }

    void say( const std::string& s )
    {
        ( (TStatusMessageLine *) statusLine )->message( s.c_str(), 4 );
    }

    virtual void handleEvent( TEvent& event )
    {
        TApplication::handleEvent( event );
        if( event.what != evCommand )
            return;
        ushort c = event.message.command;
        if( c == cmSettings )
            showSettings();
        else if( c == cmServers )
            showServers();
        else if( c == cmProgress )
            runProgress();
        else if( c == cmOpenFile )
            openFile();
        else if( c >= cmRecent && c < cmRecent + 8 )
            openRecent( c - cmRecent );
        else if( c == cmWizard )
            runWizard();
        else if( c == cmRun )
            runCommand();
        else if( c == cmPalette )
            TCommandPalette::run();
        else if( c == cmReadme )
            showReadme();
        else if( c == cmSplit )
            showSplit();
        else if( c >= cmTheme && c < cmTheme + themeCount )
            {
            applyTheme( c - cmTheme );
            saveTheme( *settings );
            settings->save();
            rebuildMenu();              // the * at the theme chosen
            TToast::show( ( std::string( "Theme: " ) + themeName( c - cmTheme ) ).c_str() );
            }
        else
            return;
        clearEvent( event );
    }

    // runs d where it was last time
    ushort execDialog( TDialog *d, const char *name, void *data )
    {
        d = (TDialog *) validView( d );
        if( d == 0 )
            return cmCancel;
        settings->restoreWindow( name, d );
        if( data != 0 )
            d->setData( data );
        ushort result = deskTop->execView( d );
        if( result != cmCancel && data != 0 )
            d->getData( data );
        settings->saveWindow( name, d );
        destroy( d );
        return result;
    }

    void showSettings()
    {
        SettingsData data;
        loadData( data );
        if( execDialog( settingsDialog(), "settings", &data ) == cmOK )
            {
            saveData( data );
            if( settings->save() )
                {
                TToast::show( "Settings saved", TToast::tkSuccess );
                say( "Saved to " + settings->fileName() );
                }
            else
                TToast::show( "The settings could not be saved", TToast::tkError, 5 );
            }
    }

    void showServers()
    {
        TTreeView *tree;
        TDialog *d = (TDialog *) validView( serversDialog( tree ) );
        if( d == 0 )
            return;
        settings->restoreWindow( "servers", d );
        ushort result = deskTop->execView( d );
        std::string marked;
        for( TTreeNode *n : tree->markedLeaves() )
            marked += ( marked.empty() ? "" : ", " ) + n->text;
        settings->saveWindow( "servers", d );
        destroy( d );
        if( result == cmOK )
            messageBox( ( "Marked: " + ( marked.empty() ? std::string( "none" ) : marked ) ).c_str(),
                        mfInformation | mfOKButton );
    }

    void runProgress()
    {
        const int steps = 120;
        TProgressDialog *d = new TProgressDialog( "Deploying", steps );
        deskTop->insert( d );
        int i = 0;
        for( ; i < steps && !d->cancelled(); i++ )
            {
            char s[60];
            snprintf( s, sizeof( s ), "Server %d of %d", i / 10 + 1, steps / 10 );
            d->setText( s );
            std::this_thread::sleep_for( std::chrono::milliseconds( 25 ) );
            d->setValue( i + 1 );
            }
        destroy( d );
        if( i == steps )
            TToast::show( "Deployed", TToast::tkSuccess );
        else
            TToast::show( "Cancelled", TToast::tkWarning );
    }

    void rebuildMenu()
    {
        TRect r = menuBar->getBounds();
        destroy( menuBar );
        menuBar = initMenuBar( r );
        insert( menuBar );
    }

    void openFile()
    {
        char name[MAXPATH] = "*.*";
        TFileDialog *d = new TFileDialog( "*.*", "Open a file", "~N~ame", fdOpenButton, 100 );
        if( execDialog( d, "open", name ) != cmCancel )
            {
            settings->addRecent( name );
            settings->save();
            rebuildMenu();
            edit( name );
            }
    }

    void edit( const std::string& name )
    {
        TSyntaxEditWindow *w = new TSyntaxEditWindow( deskTop->getExtent(), name.c_str(),
                                                      wnNoNumber );
        if( validView( w ) != 0 )
            {
            deskTop->insert( w );
            TFileEditor *ed = w->editor;
            TSyntaxEditor *se = dynamic_cast<TSyntaxEditor *>( ed );
            say( name + ": " + ( se != 0 ? TSyntaxEditor::languageName( se->language ) : "YAML" ) );
            }
    }

    void openRecent( int i )
    {
        std::vector<std::string> r = settings->recent();
        if( i >= int( r.size() ) )
            return;
        std::string name = r[size_t( i )];
        settings->addRecent( name.c_str() );    // first again
        settings->save();
        rebuildMenu();
        edit( name );
    }

    void runWizard();
    void runCommand();
    void showReadme();
    void showSplit();
};

// ---- the wizard: the last page sums up the others

class TDeployWizard : public TWizard
{

public:

    TDeployWizard() :
        TWindowInit( &TDeployWizard::initFrame ),
        TWizard( TRect( 0, 0, 60, 18 ), "New deployment" )
    {
        TPage *p = addPage( "Project" );
        name = new TValidatedInputLine( TRect( 15, 1, 45, 2 ), 31,
                                        new TRegexValidator( "[A-Za-z][A-Za-z0-9_-]*",
                                                             "A name: letters, digits, - and _." ) );
        field( p, 1, "~N~ame", name );
        env = new TComboBox( TRect( 15, 3, 28, 4 ), 11, "dev|test|staging|prod" );
        env->setListOnly( True );
        field( p, 3, "~E~nvironment", env, True );

        p = addPage( "Server" );
        host = new TValidatedInputLine( TRect( 15, 1, 50, 2 ), 41, new THostValidator );
        field( p, 1, "~H~ost", host );
        port = new TValidatedInputLine( TRect( 15, 3, 23, 4 ), 6, new TRangeValidator( 1, 65535 ) );
        field( p, 3, "~P~ort", port );
        when = new TDateInput( TRect( 15, 5, 28, 6 ) );
        p->insert( when );
        p->insert( new TLabel( TRect( 2, 5, 14, 6 ), "~S~tart on", when ) );
        p->insert( new TDateButton( TRect( 28, 5, 31, 6 ), when ) );

        p = addPage( "Summary" );
        summary = new TLogView( TRect( 2, 1, 56, 10 ), 0, 0 );
        p->insert( summary );
    }

    virtual void pageShown( int i )
    {
        if( i != 2 )
            return;
        summary->clear();
        summary->addf( "Project      %s (%s)", name->data, env->data );
        summary->addf( "Server       %s:%s", host->data, port->data );
        summary->addf( "Start on     %s", when->data );
        summary->add( "" );
        summary->add( "Finish creates it, Back changes it." );
    }

    TValidatedInputLine *name, *host, *port;
    TComboBox *env;
    TDateInput *when;
    TLogView *summary;
};

void TControlsApp::runWizard()
{
    TDeployWizard *w = new TDeployWizard;
    strcpy( w->env->data, "dev" );
    w->when->setDate( TDate::today() );
    strcpy( w->port->data, "22" );
    if( execDialog( w, "wizard", 0 ) == cmOK )
        TToast::show( "Deployment created", TToast::tkSuccess );
}

// ---- a command and its output

void TControlsApp::runCommand()
{
#if defined( _WIN32 )
    const char *examples = "dir|ver|ping -n 3 127.0.0.1|where cmd";
#else
    const char *examples = "ls -la|uname -a|ping -c 3 127.0.0.1|make -n";
#endif
    TDialog *d = new TDialog( TRect( 0, 0, 60, 8 ), "Run a command" );
    d->options |= ofCentered;
    TComboBox *c = new TComboBox( TRect( 3, 2, 53, 3 ), 201, examples );
    c->setValidator( new TRegexValidator( ".*\\S.*", "A command, please." ) );
    d->insert( c );
    d->insert( new TLabel( TRect( 2, 1, 20, 2 ), "~C~ommand", c ) );
    d->insert( new TComboButton( TRect( 53, 2, 56, 3 ), c ) );
    d->insert( new TButton( TRect( 34, 5, 45, 7 ), "~R~un", cmOK, bfDefault ) );
    d->insert( new TButton( TRect( 46, 5, 57, 7 ), "Cancel", cmCancel, bfNormal ) );
    d->selectNext( False );
    char cmd[201];
    strncpy( cmd, settings->get( "run", "command", "" ).c_str(), sizeof( cmd ) - 1 );
    cmd[sizeof( cmd ) - 1] = 0;
    if( execDialog( d, "run", cmd ) != cmOK )
        return;
    settings->set( "run", "command", cmd );
    settings->save();
    int code = TProcessDialog::run( cmd, cmd );
    if( code == 0 )
        TToast::show( "Done", TToast::tkSuccess );
    else
        TToast::show( ( "Exit code " + std::to_string( code ) ).c_str(), TToast::tkError );
}

// ---- README

void TControlsApp::showReadme()
{
    const char *names[] = { "README.md", "../README.md", "../../README.md" };
    for( const char *n : names )
        {
        FILE *f = fopen( n, "r" );
        if( f != 0 )
            {
            fclose( f );
            TMarkdownWindow *w = new TMarkdownWindow( deskTop->getExtent(), n );
            if( validView( w ) != 0 )
                deskTop->insert( w );
            say( "Tab: the next link, Enter: follow it" );
            return;
            }
        }
    TToast::show( "No README.md here", TToast::tkWarning );
}

// ---- two views and a splitter; the place of the splitter is kept

class TSplitWindow : public TWindow
{

public:

    TSplitWindow( const TRect& r ) :
        TWindowInit( &TSplitWindow::initFrame ),
        TWindow( r, "Split view", wnNoNumber )
    {
        int half = size.x / 2;
        left = new TMarkdownView( TRect( 1, 1, half, size.y - 1 ), 0, 0 );
        left->growMode = gfGrowHiY;
        right = new TMarkdownView( TRect( half + 1, 1, size.x - 1, size.y - 1 ), 0, 0 );
        right->growMode = gfGrowHiX | gfGrowHiY;
        splitter = new TSplitter( TRect( half, 1, half + 1, size.y - 1 ), True );
        splitter->addFirst( left );
        splitter->addSecond( right );
        insert( left );
        insert( right );
        insert( splitter );
        left->setText(
            "# Left\n\nDrag the bar in the middle with the mouse. Where it is "
            "is kept in the settings when the window closes.\n\n"
            "- the text is laid out again for the new width\n"
            "- Tab goes between links: [to the right](#right)\n" );
        right->setText(
            "# Right\n\n"
            "| Key | Does |\n|---|---|\n| Tab | next link |\n| Enter | follow it |\n"
            "| PgUp, PgDn | scroll |\n\n> A quote, and `some code`.\n" );
        splitter->restore( *settings, "split" );
    }

    virtual void close()
    {
        splitter->save( *settings, "split" );
        settings->save();
        TWindow::close();
    }

    TMarkdownView *left, *right;
    TSplitter *splitter;
};

void TControlsApp::showSplit()
{
    TRect r = deskTop->getExtent();
    TSplitWindow *w = new TSplitWindow( r );
    if( validView( w ) != 0 )
        deskTop->insert( w );
}

int main()
{
    TAppSettings s( "tvcontrols" );
    settings = &s;
    TEditor::editorDialog = editorDialogs;
    TControlsApp app;
    loadTheme( s );
    s.loadHistory( 100 );               // the file names of the Open dialog
    app.run();
    s.saveHistory( 100 );
    s.save();
    return 0;
}
