/*------------------------------------------------------------*/
/*   CONTROLS: the add-on controls in one program             */
/*                                                            */
/*     Demo | Settings   a dialog with tabs, combo boxes and  */
/*                       validated lines                      */
/*     Demo | Servers    a tree with marks                    */
/*     Demo | Progress   a long job with a progress dialog    */
/*     File | Open       a file for the recent files list     */
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
    cmRecent   = 200;               // 200..207: the recent files

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
    { "name", "environment", "theme", 0, "address", "mask", "host", "port", "email", 0 };

static void loadData( SettingsData& d )
{
    memset( &d, 0, sizeof( d ) );
    char *fields[] = { d.name, d.environment, d.theme, 0, d.address, d.mask, d.host, d.port,
                       d.email, 0 };
    size_t sizes[] = { sizeof( d.name ), sizeof( d.environment ), sizeof( d.theme ), 0,
                       sizeof( d.address ), sizeof( d.mask ), sizeof( d.host ),
                       sizeof( d.port ), sizeof( d.email ), 0 };
    for( int i = 0; i < 10; i++ )
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
    const char *fields[] = { d.name, d.environment, d.theme, 0, d.address, d.mask, d.host,
                             d.port, d.email, 0 };
    for( int i = 0; i < 10; i++ )
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
    TCheckBoxes *opts = new TCheckBoxes( TRect( 15, 7, 47, 10 ),
        new TSItem( "~S~ave on exit",
        new TSItem( "Show ~h~ints",
        new TSItem( "~V~erbose log", 0 ) ) ) );
    p->insert( opts );
    p->insert( new TLabel( TRect( 2, 7, 14, 8 ), "~O~ptions", opts ) );

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
                *new TMenuItem( "~P~rogress", cmProgress, kbF5, hcNoContext, "F5" ) );
    }

    static TStatusLine *initStatusLine( TRect r )
    {
        r.a.y = r.b.y - 1;
        return new TStatusLine( r,
            *new TStatusDef( 0, 0xFFFF ) +
                *new TStatusItem( "~F2~ Settings", kbF2, cmSettings ) +
                *new TStatusItem( "~F3~ Open", kbF3, cmOpenFile ) +
                *new TStatusItem( "~F4~ Servers", kbF4, cmServers ) +
                *new TStatusItem( "~F5~ Progress", kbF5, cmProgress ) +
                *new TStatusItem( "~Alt-X~ Exit", kbAltX, cmQuit ) );
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
            settings->save();
            char s[200];
            snprintf( s, sizeof( s ), "Saved: %s, %s, %s:%s", data.name, data.environment,
                      data.host, data.port );
            messageBox( s, mfInformation | mfOKButton );
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
        messageBox( i == steps ? "Done." : "Cancelled.", mfInformation | mfOKButton );
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
            messageBox( ( std::string( "Opened " ) + name ).c_str(), mfInformation | mfOKButton );
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
        messageBox( ( "Opened " + name ).c_str(), mfInformation | mfOKButton );
    }
};

int main()
{
    TAppSettings s( "tvcontrols" );
    settings = &s;
    TControlsApp app;
    s.loadHistory( 100 );               // the file names of the Open dialog
    app.run();
    s.saveHistory( 100 );
    s.save();
    return 0;
}
