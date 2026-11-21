# Class reference data for the Turbo Vision (Linux port) documentation.
#
# Each entry: (name, header, bases, uses, summary, example)
#   bases   - list of base classes; the first one places the class in the tree
#   uses    - the Uses_ macro to define before #include <tv.h> ('' if none)
#   example - C++ snippet (plain text, escaped by the generator)

SECTIONS = []


def section(title, intro, classes):
    SECTIONS.append((title, intro, classes))


# --------------------------------------------------------------------------
section("Basic types", "Geometry, commands, palettes, draw buffers and events "
        "used by every view.", [

("TObject", "tvobjs.h", [], "Uses_TObject",
 "Root of the class hierarchy. Provides a virtual destructor, "
 "<code>shutDown()</code> and the static <code>destroy()</code> that every "
 "view and collection should be released with.",
r'''#define Uses_TObject
#define Uses_TWindow
#include <tv.h>

TWindow *w = new TWindow( TRect( 0, 0, 40, 10 ), "Temp", wnNoNumber );
// ... use it ...
TObject::destroy( w );      // calls shutDown() and then deletes the object'''),

("TPoint", "objects.h", [], "Uses_TPoint",
 "A pair of <code>short</code> coordinates (<code>x</code>, <code>y</code>) "
 "with arithmetic and comparison operators. It is an aggregate: there is no "
 "constructor.",
r'''TPoint p = { 10, 5 };
TPoint d = { 1, 1 };
p += d;                         // p == {11, 6}
if( p == view->size ) { /* ... */ }
TPoint local = view->makeLocal( event.mouse.where );'''),

("TRect", "objects.h", [], "Uses_TRect",
 "A rectangle given by its top-left corner <code>a</code> and the "
 "bottom-right corner <code>b</code> (exclusive). Used for view bounds and "
 "clipping.",
r'''TRect r( 0, 0, 40, 12 );        // 40 columns x 12 rows
r.move( 5, 2 );                 // shift by (5,2)
r.grow( -1, -1 );               // shrink by one cell on every side
if( r.contains( mouse ) ) { /* ... */ }
TRect inside = getExtent();     // (0,0)-(size.x,size.y) of a view'''),

("TCommandSet", "views.h", [], "Uses_TCommandSet",
 "A set of command codes 0..255. Used to enable and disable commands for the "
 "whole application (menus and buttons gray out automatically).",
r'''TCommandSet cs;
cs += cmSave;
cs += cmSaveAs;
TView::disableCommands( cs );       // gray out Save and Save As
// later:
TView::enableCommands( cs );
if( TView::commandEnabled( cmSave ) ) { /* ... */ }'''),

("TPalette", "views.h", [], "Uses_TPalette",
 "Maps a view's color indexes into its owner's palette. A view returns its "
 "palette from <code>getPalette()</code>; the indexes are then resolved up the "
 "owner chain until the application palette yields a text attribute.",
r'''#define cpMyView "\x06\x07"      // entries 6 and 7 of the owner palette

class TMyView : public TView
{
public:
    TMyView( const TRect& r ) : TView( r ) {}
    virtual TPalette& getPalette() const
    {
        static TPalette palette( cpMyView, sizeof( cpMyView ) - 1 );
        return palette;
    }
};'''),

("TDrawBuffer", "drawbuf.h", [], "Uses_TDrawBuffer",
 "A row of character/attribute cells that a view fills and then writes with "
 "<code>writeLine()</code> or <code>writeBuf()</code>. Up to "
 "<code>maxViewWidth</code> (1024 in this port) cells. Characters are bytes of "
 "the active code page.",
r'''void TMyView::draw()
{
    TDrawBuffer b;
    ushort color = getColor( 1 );
    b.moveChar( 0, ' ', color, size.x );        // clear the row
    b.moveStr( 1, "Hello", color );             // plain text
    b.moveCStr( 8, "~F~ile", getColor( 0x0201 ) ); // '~' toggles highlight
    b.putChar( 0, '\x10' );                     // a CP437 glyph
    writeLine( 0, 0, size.x, size.y, b );
}'''),

("TEvent", "system.h", [], "Uses_TEvent",
 "The event record (<code>struct</code>). <code>what</code> selects which union "
 "member is valid: <code>mouse</code>, <code>keyDown</code> or "
 "<code>message</code>. Related records: <code>MouseEventType</code>, "
 "<code>KeyDownEvent</code>, <code>CharScanType</code>, "
 "<code>MessageEvent</code>.",
r'''void TMyView::handleEvent( TEvent& event )
{
    TView::handleEvent( event );
    switch( event.what )
        {
        case evKeyDown:
            if( event.keyDown.keyCode == kbF5 )
                { refresh(); clearEvent( event ); }
            break;
        case evMouseDown:
            if( event.mouse.doubleClick )
                { open( makeLocal( event.mouse.where ) ); clearEvent( event ); }
            break;
        case evCommand:
            if( event.message.command == cmMyCommand )
                { doIt(); clearEvent( event ); }
            break;
        }
}'''),
])

# --------------------------------------------------------------------------
section("Views and groups", "The visible building blocks. Every visible object "
        "is a <code>TView</code>; containers are <code>TGroup</code>s.", [

("TView", "views.h", ["TObject", "TStreamable"], "Uses_TView",
 "Base of everything visible: a rectangle that can draw itself, handle events "
 "and be streamed. Derive from it and override <code>draw()</code> and "
 "<code>handleEvent()</code>.",
r'''class THello : public TView
{
public:
    THello( const TRect& r ) : TView( r )
    {
        options |= ofSelectable;
        growMode = gfGrowHiX;
    }
    virtual void draw()
    {
        TDrawBuffer b;
        b.moveChar( 0, ' ', getColor( 1 ), size.x );
        b.moveStr( 1, "Hello, world", getColor( 1 ) );
        writeLine( 0, 0, size.x, size.y, b );
    }
};'''),

("TGroup", "views.h", ["TView"], "Uses_TGroup",
 "A view that owns other views (subviews), draws them in Z-order, routes "
 "events to them and can run one of them modally with "
 "<code>execView()</code>.",
r'''class TPanel : public TGroup
{
public:
    TPanel( const TRect& r ) : TGroup( r )
    {
        insert( new TStaticText( TRect( 1, 1, 20, 2 ), "Inside a group" ) );
        insert( new TButton( TRect( 1, 3, 11, 5 ), "~O~K", cmOK, bfDefault ) );
        selectNext( False );
    }
};

// iterate over the subviews
static void hideIt( TView *p, void * ) { p->hide(); }
group->forEach( hideIt, 0 );'''),

("TFrame", "views.h", ["TView"], "Uses_TFrame",
 "The double/single line border of a window with the close and zoom icons "
 "and the title. A window creates it through its <code>initFrame</code> "
 "function, which you can replace.",
r'''class TMyWindow : public TWindow
{
public:
    TMyWindow( const TRect& r ) :
        TWindow( r, "Framed", wnNoNumber ),
        TWindowInit( &TMyWindow::initFrame ) {}
    static TFrame *initFrame( TRect r ) { return new TFrame( r ); }
};'''),

("TScrollBar", "views.h", ["TView"], "Uses_TScrollBar",
 "A horizontal or vertical scroll bar. Broadcasts "
 "<code>cmScrollBarChanged</code> when its value changes.",
r'''TScrollBar *vs = window->standardScrollBar( sbVertical | sbHandleKeyboard );
vs->setParams( 0,       // value
               0, 99,   // min, max
               10,      // page step
               1 );     // arrow step
int pos = vs->value;'''),

("TScroller", "views.h", ["TView"], "Uses_TScroller",
 "A view with a virtual area larger than itself, scrolled by two scroll bars. "
 "<code>delta</code> holds the scroll position, <code>setLimit()</code> the "
 "virtual size.",
r'''class TLinesView : public TScroller
{
public:
    TLinesView( const TRect& r, TScrollBar *h, TScrollBar *v ) :
        TScroller( r, h, v ) { setLimit( 200, 1000 ); }
    virtual void draw()
    {
        TDrawBuffer b;
        for( short y = 0; y < size.y; y++ )
            {
            char s[32];
            sprintf( s, "line %d", delta.y + y );
            b.moveChar( 0, ' ', getColor( 1 ), size.x );
            b.moveStr( 0, s, getColor( 1 ) );
            writeLine( 0, y, size.x, 1, b );
            }
    }
};'''),

("TListViewer", "views.h", ["TView"], "Uses_TListViewer",
 "Abstract scrolling list with focus and selection. Override "
 "<code>getText()</code> and set the item count with "
 "<code>setRange()</code>.",
r'''class TDaysList : public TListViewer
{
public:
    TDaysList( const TRect& r, TScrollBar *vs ) :
        TListViewer( r, 1, 0, vs ) { setRange( 7 ); }
    virtual void getText( char *dest, short item, short maxLen )
    {
        static const char *days[] =
            { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
        strncpy( dest, days[item], maxLen );
        dest[maxLen] = EOS;
    }
};'''),

("TWindow", "views.h", ["TGroup", "TWindowInit"], "Uses_TWindow",
 "A movable, resizable group with a frame, title and optional number "
 "(Alt-1..Alt-9). Windows created on the desktop grow with it "
 "(<code>gfGrowAll | gfGrowRel</code>) when the terminal is resized.",
r'''TWindow *w = new TWindow( TRect( 5, 2, 45, 14 ), "My window", 1 );
w->insert( new THello( TRect( 1, 1, 30, 2 ) ) );
TProgram::deskTop->insert( w );'''),

("TBackground", "app.h", ["TView"], "Uses_TBackground",
 "The pattern that fills the desktop. The default pattern is the CP437 "
 "character 0xB0 (&#x2591;).",
r'''class TMyDeskTop : public TDeskTop
{
public:
    TMyDeskTop( const TRect& r ) : TDeskTop( r ), TDeskInit( &initBackground ) {}
    static TBackground *initBackground( TRect r )
        { return new TBackground( r, '\xB2' ); }     // a denser pattern
};'''),

("TDeskTop", "app.h", ["TGroup", "TDeskInit"], "Uses_TDeskTop",
 "The group between the menu bar and the status line that holds the windows. "
 "Can tile and cascade them.",
r'''TRect r = TProgram::deskTop->getExtent();
TProgram::deskTop->tile( r );       // cmTile
TProgram::deskTop->cascade( r );    // cmCascade
TProgram::deskTop->insert( new TWindow( TRect( 0, 0, 30, 8 ), "New", 0 ) );'''),

("TProgram", "app.h", ["TGroup", "TProgInit"], "Uses_TProgram",
 "The application group: owns the desktop, menu bar and status line, runs the "
 "event loop. In this port its constructor switches the terminal to full "
 "screen mode, and <code>getEvent()</code> also handles terminal resizing and "
 "flushes the screen.",
r'''// static members give access to the main parts
TProgram::deskTop->insert( window );
TProgram::application->putEvent( event );

// idle() runs whenever there is no input
void TMyApp::idle()
{
    TProgram::idle();
    clock->update();
}'''),

("TApplication", "app.h", ["TProgram"], "Uses_TApplication",
 "<code>TProgram</code> plus the start-up and shutdown of the screen, event "
 "queue and history list. <code>suspend()</code>/<code>resume()</code> give the "
 "terminal back to the shell and take it again.",
r'''#define Uses_TApplication
#define Uses_TMenuBar
#define Uses_TSubMenu
#define Uses_TMenuItem
#define Uses_TStatusLine
#define Uses_TStatusDef
#define Uses_TStatusItem
#define Uses_TKeys
#include <tv.h>

class TMyApp : public TApplication
{
public:
    TMyApp() : TProgInit( &TMyApp::initStatusLine,
                          &TMyApp::initMenuBar,
                          &TMyApp::initDeskTop ) {}
    static TMenuBar *initMenuBar( TRect r )
    {
        r.b.y = r.a.y + 1;
        return new TMenuBar( r,
            *new TSubMenu( "~F~ile", kbAltF ) +
                *new TMenuItem( "E~x~it", cmQuit, kbAltX, hcNoContext, "Alt-X" ) );
    }
    static TStatusLine *initStatusLine( TRect r )
    {
        r.a.y = r.b.y - 1;
        return new TStatusLine( r,
            *new TStatusDef( 0, 0xFFFF ) +
                *new TStatusItem( "~Alt-X~ Exit", kbAltX, cmQuit ) );
    }
};

int main()
{
    TMyApp app;
    app.run();
    return 0;
}'''),
])

# --------------------------------------------------------------------------
section("Menus and status line", "Menus are built from linked "
        "<code>TMenuItem</code> lists, usually with the <code>+</code> "
        "operators.", [

("TMenuItem", "menus.h", [], "Uses_TMenuItem",
 "One menu entry: name with <code>~</code> hot letter, command, hot key, help "
 "context and the key text shown on the right. <code>newLine()</code> returns "
 "a separator.",
r'''// items are chained onto a TSubMenu with +
*new TSubMenu( "~F~ile", kbAltF ) +
    *new TMenuItem( "~O~pen...", cmOpen, kbF3, hcNoContext, "F3" ) +
     newLine() +
    *new TMenuItem( "E~x~it", cmQuit, kbAltX, hcNoContext, "Alt-X" )

// without a submenu, link the items through the last argument
TMenuItem *items =
    new TMenuItem( "~C~opy", cmCopy, kbNoKey, hcNoContext, 0,
    new TMenuItem( "~P~aste", cmPaste, kbNoKey ) );'''),

("TSubMenu", "menus.h", ["TMenuItem"], "Uses_TSubMenu",
 "A menu item that opens a submenu; the items added to it with "
 "<code>+</code> form that submenu.",
r'''TSubMenu& file =
    *new TSubMenu( "~F~ile", kbAltF ) +
        *new TMenuItem( "~N~ew", cmNew, kbNoKey ) +
        *new TMenuItem( "~O~pen...", cmOpen, kbF3, hcNoContext, "F3" );
TSubMenu& all = file + *new TSubMenu( "~H~elp", kbAltH ) +
        *new TMenuItem( "~A~bout", cmAbout, kbNoKey );'''),

("TMenu", "menus.h", [], "Uses_TMenu",
 "A list of menu items with a default item. Normally created for you by "
 "<code>TMenuBar</code>; build one yourself for a pop-up menu.",
r'''TMenu *menu = new TMenu(
    *new TMenuItem( "~C~opy", cmCopy, kbNoKey, hcNoContext, 0,
     new TMenuItem( "~P~aste", cmPaste, kbNoKey ) ) );'''),

("TMenuView", "menus.h", ["TView"], "Uses_TMenuView",
 "Common base of the menu bar and menu boxes: keyboard and mouse navigation, "
 "hot keys. You rarely use it directly; <code>execute()</code> runs a menu and "
 "returns the chosen command.",
r'''// find the command bound to a hot key in the application menu
TMenuItem *item = TProgram::menuBar->hotKey( kbF3 );
if( item != 0 )
    message( TProgram::application, evCommand, item->command, 0 );'''),

("TMenuBar", "menus.h", ["TMenuView"], "Uses_TMenuBar",
 "The one-line horizontal menu at the top. F10 (or a click) activates it, "
 "Alt+letter opens a submenu. With a Cyrillic keyboard layout Alt+letter still "
 "works by key position.",
r'''static TMenuBar *initMenuBar( TRect r )
{
    r.b.y = r.a.y + 1;
    return new TMenuBar( r,
        *new TSubMenu( "~F~ile", kbAltF ) +
            *new TMenuItem( "~O~pen", cmOpen, kbF3, hcNoContext, "F3" ) +
        *new TSubMenu( "~W~indow", kbAltW ) +
            *new TMenuItem( "~N~ext", cmNext, kbF6, hcNoContext, "F6" ) );
}'''),

("TMenuBox", "menus.h", ["TMenuView"], "Uses_TMenuBox",
 "A framed vertical menu. The menu bar opens them for submenus; you can also "
 "run one as a pop-up.",
r'''TMenu *menu = new TMenu(
    *new TMenuItem( "~C~opy", cmCopy, kbNoKey, hcNoContext, 0,
     new TMenuItem( "~P~aste", cmPaste, kbNoKey ) ) );
TMenuBox *box = new TMenuBox( TRect( 10, 5, 24, 9 ), menu, 0 );
ushort cmd = TProgram::deskTop->execView( box );
TObject::destroy( box );
if( cmd != 0 )
    message( TProgram::application, evCommand, cmd, 0 );'''),

("TStatusItem", "menus.h", [], "Uses_TStatusItem",
 "A status line entry: text (may be 0 for an invisible hot key), key and "
 "command.",
r'''*new TStatusItem( "~F2~ Save", kbF2, cmSave ) +
*new TStatusItem( 0, kbF10, cmMenu )        // hot key only, not shown'''),

("TStatusDef", "menus.h", [], "Uses_TStatusDef",
 "A set of status items valid for a range of help contexts; the status line "
 "shows the first definition whose range contains the current context.",
r'''const ushort hcEditor = 100;                 // your help context

*new TStatusDef( hcEditor, hcEditor ) +      // while editing
    *new TStatusItem( "~F2~ Save", kbF2, cmSave ) +
*new TStatusDef( 0, 0xFFFF ) +               // everywhere else
    *new TStatusItem( "~Alt-X~ Exit", kbAltX, cmQuit )'''),

("TStatusLine", "menus.h", ["TView"], "Uses_TStatusLine",
 "The bottom line with key hints. Override <code>hint()</code> to show help "
 "text for the current help context.",
r'''class TMyStatusLine : public TStatusLine
{
public:
    TMyStatusLine( const TRect& r, TStatusDef& d ) : TStatusLine( r, d ) {}
    virtual const char *hint( ushort ctx )      // hcEditor: your help context
        { return ctx == hcEditor ? "Type text; F2 saves" : ""; }
};'''),
])

# --------------------------------------------------------------------------
section("Dialogs and controls", "Dialog boxes and the controls inside them. "
        "Controls exchange their values with a data record through "
        "<code>getData()</code>/<code>setData()</code>.", [

("TDialog", "dialogs.h", ["TWindow"], "Uses_TDialog",
 "A window for controls: gray palette, Esc cancels, Enter presses the default "
 "button. Usually run modally with <code>execView()</code>.",
r'''TDialog *d = new TDialog( TRect( 0, 0, 40, 9 ), "Question" );
d->options |= ofCentered;
d->insert( new TStaticText( TRect( 2, 2, 38, 3 ), "Continue?" ) );
d->insert( new TButton( TRect( 6, 5, 18, 7 ), "~Y~es", cmYes, bfDefault ) );
d->insert( new TButton( TRect( 22, 5, 34, 7 ), "~N~o", cmNo, bfNormal ) );
d->selectNext( False );
ushort result = TProgram::deskTop->execView( d );
TObject::destroy( d );'''),

("TButton", "dialogs.h", ["TView"], "Uses_TButton",
 "A push button that generates its command. <code>bfDefault</code> marks the "
 "button pressed by Enter; <code>bfBroadcast</code> broadcasts the command "
 "instead of sending it.",
r'''d->insert( new TButton( TRect( 10, 6, 20, 8 ), "~O~K", cmOK, bfDefault ) );
d->insert( new TButton( TRect( 22, 6, 32, 8 ), "Cancel", cmCancel, bfNormal ) );'''),

("TInputLine", "dialogs.h", ["TView"], "Uses_TInputLine",
 "A one-line text editor. Its data is a <code>char</code> array of "
 "<code>aMaxLen</code> bytes (including the terminating 0). Typing UTF-8 "
 "characters stores the matching byte of the active code page.",
r'''struct { char name[31]; } rec = { "John" };

TInputLine *in = new TInputLine( TRect( 10, 2, 38, 3 ), 31 );
d->insert( in );
d->insert( new TLabel( TRect( 2, 2, 9, 3 ), "~N~ame", in ) );
d->setData( &rec );
if( TProgram::deskTop->execView( d ) == cmOK )
    d->getData( &rec );'''),

("TStaticText", "dialogs.h", ["TView"], "Uses_TStaticText",
 "Fixed, word-wrapped text. A line starting with <code>\\x03</code> is "
 "centered.",
r'''d->insert( new TStaticText( TRect( 2, 2, 38, 5 ),
           "\x03Turbo Vision\n\n\x03" "for Linux" ) );'''),

("TParamText", "dialogs.h", ["TStaticText"], "Uses_TParamText",
 "Static text with <code>printf</code>-style parameters taken from the data "
 "record. <b>Port note:</b> each parameter occupies one <code>long</code>-sized "
 "slot of the record (a <code>long</code> for <code>%d %u %x %c</code>, a "
 "pointer for <code>%s</code>).",
r'''struct { const char *name; long count; } rec = { "inbox", 42 };
TParamText *t = new TParamText( TRect( 2, 2, 38, 3 ),
                                "%s: %ld messages", 2 );
d->insert( t );
t->setData( &rec );         // shows "inbox: 42 messages"'''),

("TLabel", "dialogs.h", ["TStaticText"], "Uses_TLabel",
 "A label linked to a control: clicking it or pressing its Alt+letter selects "
 "the control.",
r'''TInputLine *phone = new TInputLine( TRect( 12, 4, 32, 5 ), 20 );
d->insert( phone );
d->insert( new TLabel( TRect( 2, 4, 11, 5 ), "~P~hone", phone ) );'''),

("TCluster", "dialogs.h", ["TView"], "Uses_TCluster",
 "Abstract base of check boxes and radio buttons: a column of items built from "
 "a <code>TSItem</code> list. Its data is a <code>ushort</code>.",
r'''// TCluster itself is abstract; use TCheckBoxes or TRadioButtons.
TCluster *c = new TRadioButtons( r, new TSItem( "A", new TSItem( "B", 0 ) ) );
ushort value = c->value;'''),

("TCheckBoxes", "dialogs.h", ["TCluster"], "Uses_TCheckBoxes",
 "Independent on/off items; the data is a bit mask (bit 0 = first item).",
r'''TCheckBoxes *cb = new TCheckBoxes( TRect( 2, 3, 20, 5 ),
    new TSItem( "~B~usiness",
    new TSItem( "~P~ersonal", 0 ) ) );
d->insert( cb );
ushort mask = 0x0003;           // both checked
cb->setData( &mask );'''),

("TRadioButtons", "dialogs.h", ["TCluster"], "Uses_TRadioButtons",
 "Mutually exclusive items; the data is the index of the selected item.",
r'''TRadioButtons *rb = new TRadioButtons( TRect( 22, 3, 38, 5 ),
    new TSItem( "~M~ale",
    new TSItem( "~F~emale", 0 ) ) );
d->insert( rb );
ushort choice = 1;              // "Female"
rb->setData( &choice );'''),

("TSItem", "dialogs.h", [], "Uses_TSItem",
 "A node of the string list given to clusters; the text is copied.",
r'''TSItem *items = new TSItem( "Red", new TSItem( "Green", new TSItem( "Blue", 0 ) ) );'''),

("TListBox", "dialogs.h", ["TListViewer"], "Uses_TListBox",
 "A list viewer that shows a <code>TCollection</code> of strings. Its data "
 "record is <code>{ TCollection *items; ushort selection; }</code>.",
r'''TStringCollection *names = new TStringCollection( 10, 5 );
names->insert( newStr( "Kyiv" ) );
names->insert( newStr( "Lviv" ) );

TScrollBar *sb = new TScrollBar( TRect( 31, 2, 32, 8 ) );
d->insert( sb );
TListBox *lb = new TListBox( TRect( 2, 2, 31, 8 ), 1, sb );
d->insert( lb );
lb->newList( names );
short selected = lb->focused;'''),

("THistory", "dialogs.h", ["TView"], "Uses_THistory",
 "The small arrow next to an input line that opens the list of previously "
 "entered values with the same history id.",
r'''TInputLine *in = new TInputLine( TRect( 2, 2, 30, 3 ), 80 );
d->insert( in );
d->insert( new THistory( TRect( 30, 2, 33, 3 ), in, 10 ) );   // id 10
historyAdd( 10, "previous value" );'''),

("THistoryViewer", "dialogs.h", ["TListViewer"], "Uses_THistoryViewer",
 "The list inside the history window. Created by "
 "<code>THistoryWindow</code>; derive from it only to change how entries are "
 "shown.",
r'''// what THistoryWindow::initViewer does
TListViewer *v = new THistoryViewer( r,
    standardScrollBar( sbHorizontal | sbHandleKeyboard ),
    standardScrollBar( sbVertical | sbHandleKeyboard ),
    historyId );'''),

("THistoryWindow", "dialogs.h", ["TWindow", "THistInit"], "Uses_THistoryWindow",
 "The pop-up window that <code>THistory</code> opens. Override "
 "<code>THistory::initHistoryWindow()</code> to supply your own.",
r'''THistoryWindow *TMyHistory::initHistoryWindow( const TRect& r )
{
    THistoryWindow *w = new THistoryWindow( r, historyId );
    w->helpCtx = link->helpCtx;
    return w;
}'''),

("MsgBoxText", "msgbox.h", [], "Uses_MsgBox",
 "Holds the button and title strings of <code>messageBox()</code> and "
 "<code>inputBox()</code> (for translation). The functions themselves are the "
 "usual way to ask simple questions.",
r'''#define Uses_MsgBox
#include <tv.h>

if( messageBox( "Save changes?", mfConfirmation | mfYesNoCancel ) == cmYes )
    save();
messageBox( mfError | mfOKButton, "Cannot open %s", fileName );

char name[40] = "";
if( inputBox( "Login", "~N~ame", name, sizeof( name ) - 1 ) == cmOK ) { /* ... */ }

MsgBoxText::yesText = "~\x92~\xA0\xAA";  // "Так" in CP866/CP1125 bytes'''),
])

# --------------------------------------------------------------------------
section("Standard dialogs", "Ready-made file and directory dialogs. In this "
        "port they use Unix paths: <code>/</code>, no drive letters, "
        "<code>*.*</code> means all files.", [

("TFileDialog", "stddlg.h", ["TDialog"], "Uses_TFileDialog",
 "The Open / Save as dialog: file name input with history, file list, "
 "directory and file information. <code>getFileName()</code> returns an "
 "absolute path.",
r'''TFileDialog *d = new TFileDialog( "*.txt", "Open a file", "~N~ame",
                                  fdOpenButton, 100 );
if( TProgram::deskTop->execView( d ) != cmCancel )
    {
    char fileName[MAXPATH];
    d->getFileName( fileName );         // e.g. "/home/user/notes.txt"
    openEditor( fileName );
    }
TObject::destroy( d );'''),

("TFileInputLine", "stddlg.h", ["TInputLine"], "Uses_TFileInputLine",
 "The name field of the file dialog; it follows the focused file in the list. "
 "Part of <code>TFileDialog</code>.",
r'''// inside a custom file dialog
fileName = new TFileInputLine( TRect( 3, 3, 31, 4 ), MAXPATH );
insert( fileName );'''),

("TSortedListBox", "stddlg.h", ["TListBox"], "Uses_TSortedListBox",
 "A list box over a sorted collection with incremental search: typing jumps "
 "to the first matching item.",
r'''TStringCollection *c = new TStringCollection( 20, 10 );   // sorted
c->insert( newStr( "banana" ) );
c->insert( newStr( "apple" ) );
TSortedListBox *lb = new TSortedListBox( TRect( 2, 2, 20, 10 ), 1, sb );
lb->newList( c );'''),

("TFileList", "stddlg.h", ["TSortedListBox"], "Uses_TFileList",
 "The file list of the file dialog. <code>readDirectory()</code> fills it "
 "from a wildcard; directories are shown with a trailing <code>/</code> and "
 "<code>..</code> (except at <code>/</code>).",
r'''fileList->readDirectory( "/home/user/", "*.cpp" );'''),

("TFileInfoPane", "stddlg.h", ["TView"], "Uses_TFileInfoPane",
 "Shows the path, size and modification time of the focused file. Part of "
 "<code>TFileDialog</code>.",
r'''insert( new TFileInfoPane( TRect( 1, 16, 48, 18 ) ) );'''),

("TFileCollection", "stddlg.h", ["TSortedCollection"], "Uses_TFileCollection",
 "A sorted collection of <code>TSearchRec</code> (directories after files, "
 "<code>..</code> last). Filled by <code>TFileList</code>.",
r'''TFileCollection *files = fileList->list();
for( ccIndex i = 0; i < files->getCount(); i++ )
    {
    TSearchRec *r = (TSearchRec *)files->at( i );
    printf( "%s %ld\n", r->name, r->size );
    }'''),

("TSearchRec", "stddlg.h", [], "Uses_TSearchRec",
 "<code>struct</code> describing one directory entry: <code>attr</code> "
 "(<code>FA_DIREC</code>...), <code>time</code> (DOS packed date/time), "
 "<code>size</code>, <code>name</code>. Sent with "
 "<code>cmFileFocused</code>.",
r'''if( event.what == evBroadcast && event.message.command == cmFileFocused )
    {
    TSearchRec *r = (TSearchRec *)event.message.infoPtr;
    if( r->attr & FA_DIREC ) { /* a directory */ }
    }'''),

("TChDirDialog", "stddlg.h", ["TDialog"], "Uses_TChDirDialog",
 "Change the current directory with a directory tree and a path input. "
 "Changes the process's working directory (<code>chdir</code>).",
r'''TChDirDialog *d = new TChDirDialog( cdNormal, 101 );
TProgram::deskTop->execView( d );
TObject::destroy( d );'''),

("TDirListBox", "stddlg.h", ["TListBox"], "Uses_TDirListBox",
 "The directory tree of <code>TChDirDialog</code>: the path from "
 "<code>/</code> down to the current directory and its sorted "
 "subdirectories.",
r'''TDirListBox *tree = new TDirListBox( TRect( 3, 6, 31, 15 ), sb );
tree->newDirectory( "/usr/local/" );'''),

("TDirCollection", "stddlg.h", ["TCollection"], "Uses_TDirCollection",
 "The collection of <code>TDirEntry</code> items shown by "
 "<code>TDirListBox</code>.",
r'''TDirCollection *dirs = tree->list();
TDirEntry *e = dirs->at( tree->focused );'''),

("TDirEntry", "stddlg.h", [], "Uses_TDirEntry",
 "One line of the directory tree: the displayed text (with tree graphics) and "
 "the full directory path.",
r'''TDirEntry *e = new TDirEntry( "\xC0\xC4\xC2src", "/home/user/src" );
const char *path = e->dir();'''),
])

# --------------------------------------------------------------------------
section("Editors", "A gap-buffer text editor with selection, clipboard, undo, "
        "search and replace. Files up to 64 KB.", [

("TEditor", "editors.h", ["TView"], "Uses_TEditor",
 "The editor view. <code>editorDialog</code> is a hook for its dialogs "
 "(find, replace, errors); <code>clipboard</code> points to the editor used as "
 "clipboard. <b>Port note:</b> <code>drawLines()</code> is virtual, so a "
 "derived editor can colour its text (see <a href=\"#TYamlEditor\">TYamlEditor</a>).",
r'''TEditor *e = new TEditor( r, hScroll, vScroll, indicator, 4096 );
e->insertText( "Hello\r\n", 7, False );     // the buffer uses CR LF
TEditor::editorDialog = myEditorDialog;     // ushort f( int dialog, ... )'''),

("TMemo", "editors.h", ["TEditor"], "Uses_TMemo",
 "An editor used as a dialog control. Its data is a "
 "<code>TMemoData</code>: a <code>ushort</code> length followed by the text.",
r'''struct { ushort length; char text[512]; } memo = { 5, "Hello" };
TMemo *m = new TMemo( TRect( 2, 2, 30, 8 ), 0, vs, 0, sizeof( memo.text ) );
d->insert( m );
m->setData( &memo );'''),

("TFileEditor", "editors.h", ["TEditor"], "Uses_TFileEditor",
 "An editor bound to a file. <b>Port note:</b> LF files are converted to CR LF "
 "on load and written back with LF. A UTF-8 file whose characters exist in "
 "the code page is edited in that code page and saved as UTF-8. The backup "
 "is <code>name.ext.bak</code>.",
r'''TFileEditor *fe = new TFileEditor( r, hs, vs, ind, "/etc/motd" );
if( fe->modified )
    fe->save();                     // or saveAs()'''),

("TEditWindow", "editors.h", ["TWindow"], "Uses_TEditWindow",
 "A window with a <code>TFileEditor</code>, scroll bars and an indicator. Pass "
 "0 as the file name for an untitled window.",
r'''TEditWindow *w = new TEditWindow( TProgram::deskTop->getExtent(),
                                  "/home/user/todo.txt", wnNoNumber );
TProgram::deskTop->insert( w );'''),

("TIndicator", "editors.h", ["TView"], "Uses_TIndicator",
 "The line:column indicator in the editor window frame; shows a modified "
 "mark.",
r'''TIndicator *ind = new TIndicator( TRect( 2, size.y - 1, 16, size.y ) );
insert( ind );'''),
])

# --------------------------------------------------------------------------
section("Add-ons", "New classes of the port, in <code>addons/</code>: a library of "
        "their own, <code>lib/libtvaddons.a</code>. Include "
        "<code>&lt;tvaddons.h&gt;</code> (with <code>-Iaddons/include</code>) after "
        "the <code>Uses_</code> macros, and link "
        "<code>-Wl,--whole-archive lib/libtvaddons.a lib/libtv.a "
        "-Wl,--no-whole-archive</code> (on Windows the CMake target "
        "<code>tvaddons</code>, with <code>/WHOLEARCHIVE</code>). "
        "<code>editdlg.h</code> (<code>Uses_TEditorDialogs</code>) adds "
        "<code>editorDialogs()</code>, a complete <code>TEditor::editorDialog</code>: "
        "Find, Replace, the replace prompt, Go to line, Save?, Save as and the error messages "
        "(<code>TEditor::editorDialog = editorDialogs; editorFileMask = \"*.yaml\";</code>).", [

("TAdvancedEditor", "advedit.h", ["TFileEditor"], "Uses_TAdvancedEditor",
 "A <code>TFileEditor</code> with the keys of today's editors: Ctrl+C copy, "
 "Ctrl+X cut, Ctrl+V paste, Ctrl+A select all (<code>cmSelectAll</code>). "
 "Ctrl+B starts and ends selecting with the cursor keys alone, for terminals "
 "that do not send Shift (<code>cmToggleSelect</code>). The mouse wheel scrolls "
 "the text (<code>evMouseWheel</code>, <code>wheelLines</code>). "
 "Ctrl+G goes to a line (<code>cmGotoLine</code>, <code>gotoLine()</code>), "
 "Ctrl+F searches as you type (<code>cmIncSearch</code>), and the bracket at "
 "the cursor and its pair are shown in <code>bracketColor</code>. "
 "<code>clipboardKeys = False</code> gives Ctrl+C/X/V/A/G/F back to the WordStar "
 "commands; <code>TEditor</code> itself is unchanged.",
r'''#define Uses_TAdvancedEditor
#include <tvaddons.h>

TAdvancedEditor *ed = new TAdvancedEditor( r, hScroll, vScroll, indicator, "notes.txt" );
ed->wheelLines = 5;             // lines per wheel step
ed->clipboardKeys = False;      // the WordStar keys again
ed->gotoLine( 120 );            // as Ctrl+G does'''),

("TBlockIndicator", "advedit.h", ["TIndicator"], "Uses_TBlockIndicator",
 "A <code>TIndicator</code> that shows <b>B</b> while Ctrl+B selecting is on, "
 "and <code>/text</code> while an incremental search runs. "
 "Pass it to a <code>TAdvancedEditor</code> instead of a plain indicator.",
r'''TBlockIndicator *ind = new TBlockIndicator( TRect( 2, size.y - 1, 16, size.y ) );
insert( ind );
TAdvancedEditor *ed = new TAdvancedEditor( r, 0, vScroll, ind, 0 );'''),

("TLineNumbers", "advedit.h", ["TView"], "Uses_TLineNumbers",
 "A column of line numbers left of a <code>TAdvancedEditor</code>: 4 digits "
 "(white on gray, <code>numberColor</code>) and 2 blanks, so 6 wide. It follows "
 "the editor's scrolling. <code>setMark( line, color )</code> shows a line "
 "number in a colour of its own (a breakpoint, an error, the line being run); "
 "<code>clearMark()</code>, <code>clearMarks()</code>, <code>markOf()</code>. "
 "Marks stay on their line numbers when the text changes; up to 64.",
r'''TRect nr = r;  nr.b.x = r.a.x + 6;  r.a.x = nr.b.x;
TAdvancedEditor *ed = new TAdvancedEditor( r, 0, vScroll, indicator, "prog.txt" );
TLineNumbers *numbers = new TLineNumbers( nr, ed );
insert( numbers );
insert( ed );
numbers->setMark( 12, 0x4F );   // line 12: white on red'''),

("TColoredEditor", "colredit.h", ["TAdvancedEditor"], "Uses_TColoredEditor",
 "The base of syntax colouring editors. A derived class gives "
 "<code>scanLine()</code>, the class of each character of a line, and "
 "<code>classColor()</code>, the colour of a class; <code>drawLines()</code> "
 "colours the cells in the normal text colour and leaves the selection. "
 "<code>highlight = False</code> turns the colours off. Not registered for "
 "streams itself.",
r'''class TNumberEditor : public TColoredEditor
{
public:
    TNumberEditor( const TRect& r, TScrollBar *v ) : TColoredEditor( r, 0, v, 0, 0 ) {}
    virtual void scanLine( ushort, const char *s, int n, uchar *cls )
        { for( int k = 0; k < n; k++ ) cls[k] = s[k] >= '0' && s[k] <= '9'; }
    virtual uchar classColor( uchar cls, uchar normal )
        { return cls == 1 ? uchar( ( normal & 0xF0 ) | 0x0A ) : normal; }
};'''),

("TYamlEditor", "yamledit.h", ["TColoredEditor"], "Uses_TYamlEditor",
 "A <code>TColoredEditor</code> that colours YAML: keys, quoted strings, "
 "numbers and true/false/null, comments, punctuation, anchors and tags, "
 "document markers and block scalar text, through "
 "<code>scanLine()</code> and <code>classColor()</code>; <code>highlight = False</code> turns the colours "
 "off, <code>darkColors</code> / <code>lightColors</code> set them.",
r'''TYamlEditor *ed = new TYamlEditor( r, hScroll, vScroll, indicator, "app.yaml" );
ed->highlight = False;          // plain text again'''),

("TYamlEditWindow", "yamledit.h", ["TEditWindow"], "Uses_TYamlEditWindow",
 "A <code>TEditWindow</code> whose editor is a <code>TYamlEditor</code> and "
 "whose indicator is a <code>TBlockIndicator</code>. "
 "<code>bin/yamledit</code> is TVEDIT with these windows.",
r'''TYamlEditWindow *w = new TYamlEditWindow( TProgram::deskTop->getExtent(),
                                          "docker-compose.yaml", wnNoNumber );
TProgram::deskTop->insert( TProgram::application->validView( w ) );'''),

("TLogView", "logview.h", ["TScroller"], "Uses_TLogView",
 "A log: lines added at the end with <code>add()</code> or <code>addf()</code>, "
 "each in a colour of its own if need be (<code>errorColor</code>, "
 "<code>warningColor</code>, <code>goodColor</code>). Keeps the last "
 "<code>maxLines</code> (2000); follows the end while it is shown "
 "(<code>autoScroll</code>); scrolls with the keys and the wheel, with or "
 "without scroll bars; <code>saveToFile()</code>. A view of lines kept "
 "elsewhere overrides <code>lineCount()</code>, <code>lineText()</code>, "
 "<code>lineColor()</code> and calls <code>update()</code>.",
r'''TLogView *log = new TLogView( r, 0, vScroll );
insert( log );
log->add( "Build: 19 cells" );
log->add( "line 3: unknown label", TLogView::errorColor );
log->addf( "STOP after %d steps", steps );'''),

("TTableView", "tableview.h", ["TListViewer"], "Uses_TTableView",
 "A list of rows with columns of text (<code>addColumn()</code>, "
 "<code>addRow()</code>, <code>setCell()</code>), sorted by any column "
 "(<code>sortBy()</code>; numbers as numbers), with marks on rows when "
 "<code>markable</code> (Space, Ins, a click in the first column). "
 "<code>rowAt()</code> and <code>itemOf()</code> convert between the rows and "
 "their places on the screen.",
r'''TTableView *t = new TTableView( TRect( 2, 3, 60, 15 ), 0, vScroll );
t->addColumn( "Server", 12 );
t->addColumn( "Port", 6, TTableView::alRight );
t->addRow( { "web1", "22" } );
t->markable = True;
t->sortBy( 1 );                 // by port'''),

("TTableHeader", "tableview.h", ["TView"], "Uses_TTableHeader",
 "The titles over a <code>TTableView</code>, with an arrow on the sort "
 "column; a click on a title sorts by it, a second click the other way round.",
r'''insert( t );
insert( new TTableHeader( TRect( 2, 2, 60, 3 ), t ) );'''),

("TValidatedInputLine", "validate.h", ["TInputLine"], "Uses_TValidatedInputLine",
 "A <code>TInputLine</code> with a validator (Turbo Vision 1.0 has none): keys "
 "that make the text impossible are not taken, and closing the dialog with "
 "anything but Cancel shows why a text is wrong and keeps the dialog open. "
 "Validators: <code>TFilterValidator</code>, <code>TRangeValidator</code>, "
 "<code>TLookupValidator</code>, <code>TRegexValidator</code> "
 "(<code>std::regex</code>), <code>TFuncValidator</code>, and for networks and "
 "accounts <code>TIPAddressValidator</code>, <code>TNetworkValidator</code> "
 "(10.0.0.0/8), <code>TNetMaskValidator</code> (255.255.255.0 or /24), "
 "<code>THostValidator</code>, <code>TEmailValidator</code> and "
 "<code>TPasswordValidator</code> (length, digits, upper and lower case, symbols); "
 "their base <code>TValidator</code> has <code>isValidInput()</code>, <code>isValid()</code>, "
 "<code>errorText()</code> and <code>allowEmpty</code>. With <code>password = True</code> "
 "the line shows stars. <code>bin/validate</code> is a form with all of them.",
r'''d->insert( new TValidatedInputLine( TRect( 3, 2, 12, 3 ), 6,   // 5 characters + 0
               new TRangeValidator( 1, 65535 ) ) );
d->insert( new TValidatedInputLine( TRect( 3, 4, 22, 5 ), 16,
               new TIPAddressValidator ) );
TValidatedInputLine *pw = new TValidatedInputLine( TRect( 3, 6, 34, 7 ), 31,
                                                   new TPasswordValidator );
pw->password = True;
d->insert( pw );'''),

("TProgressBar", "progbar.h", ["TView"], "Uses_TProgressBar",
 "A bar that fills as <code>setValue()</code> goes from 0 to the total "
 "(<code>setTotal()</code>), with the percentage or <code>setText()</code> in the "
 "middle; <code>percent()</code>.",
r'''TProgressBar *bar = new TProgressBar( TRect( 3, 4, 47, 5 ), files );
d->insert( bar );
bar->setValue( done );'''),

("TProgressDialog", "progbar.h", ["TDialog"], "Uses_TProgressDialog",
 "A dialog with a line of text, a <code>TProgressBar</code> and Cancel. A loop "
 "calls <code>cancelled()</code> between its steps: it takes the waiting events "
 "(Cancel, Esc, a resized terminal) without waiting for more. Run with "
 "<code>execView()</code>, <code>finish()</code> ends it with cmOK.",
r'''TProgressDialog *d = new TProgressDialog( "Copying", n );
TProgram::deskTop->insert( d );
for( int i = 0; i < n && !d->cancelled(); i++ )
    {
    d->setText( names[i] );
    copy( names[i] );
    d->setValue( i + 1 );
    }
TObject::destroy( d );'''),

("TTabView", "tabview.h", ["TGroup"], "Uses_TTabView",
 "Pages with tabs over them, in a dialog: <code>addPage( \"~N~etwork\" )</code> "
 "returns a group for the controls of the page. A click, Alt+letter or "
 "Ctrl+PgDn/PgUp shows a page (<code>cmTabChanged</code>); Tab goes through the "
 "page, then on; the dialog's data take every page; on OK the page of a wrong "
 "field is shown first.",
r'''TTabView *tabs = new TTabView( TRect( 2, 1, 62, 15 ) );
TGroup *p = tabs->addPage( "~G~eneral" );
p->insert( new TInputLine( TRect( 15, 1, 47, 2 ), 31 ) );
p = tabs->addPage( "~N~etwork" );
d->insert( tabs );'''),

("TTreeView", "treeview.h", ["TListViewer"], "Uses_TTreeView",
 "A tree of <code>TTreeNode</code>s that open and close (Right/Left, +/-, *, "
 "a click), drawn with lines; with <code>markable</code>, check boxes that mark "
 "a node with its children (<code>[-]</code> when partly marked, "
 "<code>markedLeaves()</code>).",
r'''TTreeView *tree = new TTreeView( TRect( 2, 2, 41, 15 ), 0, vScroll );
tree->markable = True;
TTreeNode *web = tree->addNode( 0, "web" );
tree->addNode( web, "web1" );
tree->addNode( web, "web2" );'''),

("TComboBox", "combobox.h", ["TValidatedInputLine"], "Uses_TComboBox",
 "An input line with a list (<code>setItems( \"a|b|c\" )</code>, "
 "<code>addItem()</code>) that Down or a <code>TComboButton</code> opens; with "
 "<code>setListOnly( True )</code> only an item of the list is valid.",
r'''TComboBox *env = new TComboBox( TRect( 15, 3, 28, 4 ), 11, "dev|test|prod" );
env->setListOnly( True );
d->insert( env );
d->insert( new TComboButton( TRect( 28, 3, 31, 4 ), env ) );'''),

("TComboButton", "combobox.h", ["TView"], "Uses_TComboBox",
 "The arrow right of a <code>TComboBox</code> (as <code>THistory</code>'s): a click "
 "opens the list.",
r'''d->insert( new TComboButton( TRect( 28, 3, 31, 4 ), env ) );'''),

("TAppSettings", "appsets.h", [], "Uses_TAppSettings",
 "Settings kept in an INI file (<code>~/.config/&lt;name&gt;/settings.ini</code>, "
 "<code>%APPDATA%</code> on Windows): <code>get()</code>/<code>set()</code> and "
 "numbers and yes/no, the places of windows (<code>saveWindow()</code>, "
 "<code>restoreWindow()</code>), the recent files with a menu "
 "(<code>addRecent()</code>, <code>recentMenu()</code>), the <code>THistory</code> "
 "lists and the colours. <code>save()</code> replaces the file whole; comments stay.",
r'''TAppSettings settings( "myapp" );
settings.restoreWindow( "editor", window );
settings.addRecent( fileName );
settings.saveWindow( "editor", window );
settings.save();'''),

("TPage", "page.h", ["TGroup"], "Uses_TPage",
 "A group for the controls of one page of <code>TTabView</code> or <code>TWizard</code>: "
 "the dialog's background under them, <code>tabOrder()</code>, <code>firstInvalid()</code>, "
 "<code>validPage()</code> (fields checked from the top), <code>focusFirst()</code>, "
 "<code>focusNext()</code>.",
r"""TPage *p = tabs->addPage( "~G~eneral" );
p->insert( new TInputLine( TRect( 15, 1, 47, 2 ), 31 ) );"""),

("TWizard", "wizard.h", ["TDialog"], "Uses_TWizard",
 "A dialog of steps: <code>addPage()</code>, Back, Next (which checks the page and "
 "<code>canLeave()</code>), Finish on the last page, \"Step 2 of 3\" on top; "
 "<code>pageShown()</code> fills a page from the ones before. The data take every page.",
r"""TWizard *w = new TWizard( TRect( 0, 0, 60, 18 ), "Install" );
TPage *p = w->addPage( "Where to" );
p->insert( new TInputLine( TRect( 15, 1, 50, 2 ), 61 ) );
w->addPage( "Options" );
deskTop->execView( w );"""),

("TProcess", "tvproc.h", [], "Uses_TProcess",
 "A command run through the shell (<code>/bin/sh -c</code>, <code>cmd.exe /c</code>) with "
 "its output in a pipe read without waiting: <code>start( command, dir, env )</code>, "
 "<code>read()</code>, <code>running()</code>, <code>exitCode()</code>, <code>stop()</code> "
 "(the process group, a job object on Windows).",
r"""TProcess p;
p.start( "make -j4", "/home/me/shop" );
while( p.running() )
    out += p.read();"""),

("TProcessView", "tvproc.h", ["TLogView"], "Uses_TProcessView",
 "A <code>TLogView</code> with the output of a command as it comes, in the code page of "
 "the screen, and its exit code; <code>poll()</code> reads it (from <code>idle()</code>).",
r"""view->start( "ping -c 3 127.0.0.1" );
...
void TMyApp::idle() { TApplication::idle(); view->poll(); }"""),

("TProcessDialog", "tvproc.h", ["TDialog"], "Uses_TProcessDialog",
 "Runs a command in a dialog with its output, a status line, Stop and Close; "
 "<code>run()</code> returns the exit code.",
r"""int code = TProcessDialog::run( "Build", "make -j4" );"""),

("TCommandPalette", "cmdpal.h", ["TDialog"], "Uses_TCommandPalette",
 "The commands of the menu bar with their path and key, found by typing a part of the "
 "name (word starts first, then letters in order); <code>run()</code> puts the chosen one "
 "into the queue.",
r"""case cmPalette:
    TCommandPalette::run();
    break;"""),

("TMarkdownView", "markdown.h", ["TScroller"], "Uses_TMarkdownView",
 "Markdown laid out for the width of the view: headings, wrapped paragraphs, bold, code, "
 "lists, quotes, code blocks, tables; Tab goes between links, Enter follows "
 "<code>#anchors</code> (others: <code>cmMarkdownLink</code>).",
r"""TMarkdownView *v = new TMarkdownView( r, hScroll, vScroll );
v->loadFile( "README.md" );
v->gotoAnchor( "#building" );"""),

("TMarkdownWindow", "markdown.h", ["TWindow"], "Uses_TMarkdownWindow",
 "A window with a <code>TMarkdownView</code> of a file and its scroll bars.",
r"""deskTop->insert( validView( new TMarkdownWindow( deskTop->getExtent(), "README.md" ) ) );"""),

("TDateInput", "dateedit.h", ["TValidatedInputLine"], "Uses_TDateInput",
 "An input line for a date (YYYY-MM-DD, DD.MM.YYYY or MM/DD/YYYY) that puts in the "
 "separators itself and wants a real date; Down or F4 opens a calendar (Monday first). "
 "<code>date()</code>, <code>setDate()</code>; <code>TDate</code> counts days and months.",
r"""TDateInput *d = new TDateInput( TRect( 15, 7, 28, 8 ), dfDMY );
dlg->insert( d );
dlg->insert( new TDateButton( TRect( 28, 7, 31, 8 ), d ) );"""),

("TDateButton", "dateedit.h", ["TView"], "Uses_TDateInput",
 "The arrow right of a <code>TDateInput</code>: a click opens the calendar.",
r"""dlg->insert( new TDateButton( TRect( 28, 7, 31, 8 ), d ) );"""),

("TSplitter", "splitter.h", ["TView"], "Uses_TSplitter",
 "A bar between two parts of a window that the mouse drags; the views of each side are "
 "resized or moved with it, within <code>minFirst</code> / <code>minSecond</code>; "
 "<code>save()</code> and <code>restore()</code> keep it in <code>TAppSettings</code>.",
r"""TSplitter *s = new TSplitter( TRect( 30, 1, 31, h - 1 ), True );
s->addFirst( tree );
s->addSecond( text );
insert( s );"""),

("TToast", "toast.h", ["TView"], "Uses_TToast",
 "A short message at the bottom right of the desktop that goes away by itself "
 "(<code>show( text, kind, seconds )</code>; info, success, warning, error); the program "
 "calls <code>TToast::idle()</code> from its <code>idle()</code>.",
r"""TToast::show( "Saved", TToast::tkSuccess );"""),

("TStatusMessageLine", "toast.h", ["TStatusLine"], "Uses_TStatusMessageLine",
 "A status line that shows <code>message( text, seconds )</code> in place of its keys for "
 "a while.",
r"""( (TStatusMessageLine *) statusLine )->message( "Copied 3 lines" );"""),

("TSyntaxEditor", "syntax.h", ["TColoredEditor"], "Uses_TSyntaxEditor",
 "Colours for C/C++, shell, INI/.env, JSON and Markdown, chosen by the file name "
 "(<code>languageFor()</code>) or <code>setLanguage()</code>; block comments and code "
 "fences over several lines are followed. Colour themes are in <code>themes.h</code>: "
 "<code>applyTheme( thDark )</code>, <code>themeMenu()</code>.",
r"""TSyntaxEditor *ed = new TSyntaxEditor( r, h, v, ind, "deploy.sh" );"""),

("TSyntaxEditWindow", "syntax.h", ["TEditWindow"], "Uses_TSyntaxEditWindow",
 "A <code>TEditWindow</code> whose editor colours the file: a <code>TSyntaxEditor</code>, "
 "or a <code>TYamlEditor</code> for YAML.",
r"""deskTop->insert( validView( new TSyntaxEditWindow( r, "config.ini", wnNoNumber ) ) );"""),
])

# --------------------------------------------------------------------------
section("Text devices", "Views that behave like output streams.", [

("TTextDevice", "textview.h", ["TScroller", "streambuf"], "Uses_TTextDevice",
 "A scroller that is also a <code>std::streambuf</code>. Derive from it and "
 "implement <code>do_sputn()</code>.",
r'''class TLogView : public TTextDevice
{
public:
    TLogView( const TRect& r, TScrollBar *h, TScrollBar *v ) :
        TTextDevice( r, h, v ) {}
    virtual int do_sputn( const char *s, int count )
        { store( s, count ); drawView(); return count; }
};'''),

("TTerminal", "textview.h", ["TTextDevice"], "Uses_TTerminal",
 "A text device with a circular buffer: lines scroll up like on a terminal.",
r'''TTerminal *term = new TTerminal( r, hs, vs, 8192 );
window->insert( term );'''),

("otstream", "textview.h", ["ostream"], "Uses_otstream",
 "A <code>std::ostream</code> writing into a <code>TTerminal</code>.",
r'''otstream out( term );
out << "Connected to " << host << endl;
out << "Received " << bytes << " bytes" << endl;'''),
])

# --------------------------------------------------------------------------
section("Help system", "Context-sensitive help from <code>.hlp</code> files "
        "compiled with <code>tvhc</code>. The file format is the DOS one, so old "
        "help files work unchanged.", [

("THelpFile", "helpbase.h", ["TObject"], "Uses_THelpFile",
 "Reads topics from a help file (an <code>fpstream</code>).",
r'''fpstream *s = new fpstream( "demohelp.hlp", ios::in | ios::binary );
THelpFile *hf = new THelpFile( *s );        // takes ownership of the stream'''),

("THelpWindow", "help.h", ["TWindow"], "Uses_THelpWindow",
 "A window that shows a help topic and follows cross references (Tab, Enter, "
 "mouse).",
r'''void TMyApp::showHelp()
{
    fpstream *s = new fpstream( "demohelp.hlp", ios::in | ios::binary );
    if( !s->good() )
        { messageBox( "No help file", mfError | mfOKButton ); delete s; return; }
    THelpFile *hf = new THelpFile( *s );
    TWindow *w = new THelpWindow( hf, getHelpCtx() );
    if( validView( w ) != 0 )
        { execView( w ); destroy( w ); }
}'''),

("THelpViewer", "help.h", ["TScroller"], "Uses_THelpViewer",
 "The scrolling view inside <code>THelpWindow</code>.",
r'''insert( new THelpViewer( r, hs, vs, helpFile, context ) );'''),

("THelpTopic", "helpbase.h", ["TObject", "TStreamable"], "Uses_THelpTopic",
 "One topic: paragraphs and cross references. Returned by "
 "<code>THelpFile::getTopic()</code>.",
r'''THelpTopic *t = helpFile->getTopic( hcFile );
for( int i = 1; i <= t->numLines(); i++ )
    puts( t->getLine( i ) );
delete t;'''),

("THelpIndex", "helpbase.h", ["TObject", "TStreamable"], "Uses_THelpIndex",
 "Maps help contexts to file positions; stored at the end of the help file.",
r'''long pos = helpFile->index->position( hcFile );   // -1 if unknown'''),

("TParagraph", "helpbase.h", [], "Uses_TParagraph",
 "A paragraph of topic text (<code>size</code>, <code>wrap</code>, "
 "<code>text</code>). Used by the help compiler and viewer.",
r'''TParagraph *p = new TParagraph;
p->text = newStr( "Some help text." );
p->size = strlen( p->text );
p->wrap = True;
topic->addParagraph( p );'''),

("TCrossRef", "helpbase.h", [], "Uses_TCrossRef",
 "A link inside a topic: target context, text offset and length.",
r'''TCrossRef ref;
ref.ref = hcEditMenu;
ref.offset = 12;
ref.length = 9;
topic->addCrossRef( ref );'''),
])

# --------------------------------------------------------------------------
section("Color selection", "The interactive palette editor "
        "(<code>TColorDialog</code>) and its parts.", [

("TColorDialog", "colorsel.h", ["TDialog"], "Uses_TColorDialog",
 "Lets the user change the application palette. Groups and items say which "
 "palette entries mean what.",
r'''TColorGroup &groups =
    *new TColorGroup( "Desktop" ) + *new TColorItem( "Color", 1 ) +
    *new TColorGroup( "Menus" ) +
        *new TColorItem( "Normal", 2 ) +
        *new TColorItem( "Selected", 5 );
TColorDialog *c = new TColorDialog( (TPalette *)0, &groups );
c->setData( &getPalette() );
if( deskTop->execView( c ) != cmCancel )
    {
    getPalette() = *( c->pal );
    redraw();
    }
destroy( c );'''),

("TColorGroup", "colorsel.h", [], "Uses_TColorGroup",
 "A named group of color items; groups are chained with <code>+</code>.",
r'''TColorGroup &g = *new TColorGroup( "Dialogs" ) +
                 *new TColorItem( "Frame", 33 ) +
                 *new TColorItem( "Text", 37 );'''),

("TColorItem", "colorsel.h", [], "Uses_TColorItem",
 "A named palette index.",
r'''TColorItem &i = *new TColorItem( "Button normal", 41 ) +
                *new TColorItem( "Button default", 42 );'''),

("TColorSelector", "colorsel.h", ["TView"], "Uses_TColorSelector",
 "The 16-color (foreground) or 8-color (background) picker grid.",
r'''insert( new TColorSelector( TRect( 3, 3, 15, 7 ),
                            TColorSelector::csForeground ) );'''),

("TMonoSelector", "colorsel.h", ["TCluster"], "Uses_TMonoSelector",
 "Radio buttons for monochrome attributes (normal, highlight, underline, "
 "inverse); used when the palette is monochrome (<code>TVMONO=1</code>).",
r'''insert( new TMonoSelector( TRect( 3, 3, 20, 7 ) ) );'''),

("TColorDisplay", "colorsel.h", ["TView"], "Uses_TColorDisplay",
 "Shows sample text in the selected color.",
r'''insert( new TColorDisplay( TRect( 3, 14, 30, 18 ), "Text " ) );'''),

("TColorGroupList", "colorsel.h", ["TListViewer"], "Uses_TColorGroupList",
 "The list of color groups in the color dialog.",
r'''insert( new TColorGroupList( TRect( 3, 3, 18, 14 ), sb, &groups ) );'''),

("TColorItemList", "colorsel.h", ["TListViewer"], "Uses_TColorItemList",
 "The list of items of the focused group.",
r'''insert( new TColorItemList( TRect( 21, 3, 42, 14 ), sb, groups.items ) );'''),
])

# --------------------------------------------------------------------------
section("Collections", "Dynamic arrays of <code>void*</code> items. The "
        "<code>TNS...</code> classes are not streamable; the others can be "
        "written to object streams.", [

("TNSCollection", "tvobjs.h", ["TObject"], "Uses_TNSCollection",
 "An unsorted, non-streamable collection with iterators.",
r'''TNSCollection list( 10, 5 );        // initial limit, growth step
list.insert( new TClient( "Smith" ) );
TClient *c = (TClient *)list.at( 0 );
static void printOne( void *item, void * ) { ((TClient *)item)->print(); }
list.forEach( printOne, 0 );
list.free( c );                     // remove and freeItem()'''),

("TNSSortedCollection", "tvobjs.h", ["TNSCollection"], "Uses_TNSSortedCollection",
 "A sorted collection. Override <code>compare()</code> (and "
 "<code>keyOf()</code> if the key is not the item itself).",
r'''class TIntSet : public TNSSortedCollection
{
public:
    TIntSet() : TNSSortedCollection( 10, 10 ) {}
    virtual int compare( void *a, void *b )
        { return *(int *)a - *(int *)b; }
};
ccIndex i;
if( set.search( &key, i ) ) { /* found at i */ }'''),

("TCollection", "objects.h", ["TNSCollection", "TStreamable"], "Uses_TCollection",
 "A streamable collection. Implement <code>readItem()</code> and "
 "<code>writeItem()</code> and register the class.",
r'''class TPointList : public TCollection
{
public:
    TPointList() : TCollection( 10, 10 ) {}
private:
    virtual void *readItem( ipstream& is )
        { TPoint *p = new TPoint; is >> p->x >> p->y; return p; }
    virtual void writeItem( void *obj, opstream& os )
        { os << ((TPoint *)obj)->x << ((TPoint *)obj)->y; }
    // plus name, build(), streamableName() and a TStreamableClass object
};'''),

("TSortedCollection", "objects.h", ["TNSSortedCollection", "TCollection"],
 "Uses_TSortedCollection",
 "A streamable sorted collection.",
r'''class TWordList : public TSortedCollection
{
public:
    TWordList() : TSortedCollection( 50, 50 ) {}
    virtual int compare( void *a, void *b )
        { return strcmp( (char *)a, (char *)b ); }
    // readItem()/writeItem() as for TCollection
};'''),

("TStringCollection", "resource.h", ["TSortedCollection"], "Uses_TStringCollection",
 "A sorted, streamable collection of strings allocated with "
 "<code>newStr()</code>.",
r'''TStringCollection *c = new TStringCollection( 10, 10 );
c->insert( newStr( "beta" ) );
c->insert( newStr( "alpha" ) );          // kept sorted
const char *first = (const char *)c->at( 0 );   // "alpha"'''),

("TResourceCollection", "resource.h", ["TStringCollection"], "Uses_TResourceCollection",
 "The index of a resource file (key, position, size). Used internally by "
 "<code>TResourceFile</code>.",
r'''// number of objects stored in a resource file
short n = resFile->count();             // = index->getCount()'''),
])

# --------------------------------------------------------------------------
section("Object streams", "Persistent storage of objects and their links. "
        "Every streamable class registers itself; pointers to the same object "
        "are written once. The format on disk is the DOS one: <code>int</code> "
        "is 16 bits, <code>long</code> 32 bits, little-endian.", [

("TStreamable", "tobjstrm.h", [], "Uses_TStreamable",
 "Interface of streamable objects: <code>streamableName()</code>, "
 "<code>read()</code>, <code>write()</code>.",
r'''class TNote : public TObject, public TStreamable
{
public:
    TNote( const char *t ) : text( newStr( t ) ) {}
    static const char * const name;
    static TStreamable *build() { return new TNote( streamableInit ); }
protected:
    TNote( StreamableInit ) : text( 0 ) {}
    virtual void write( opstream& os ) { os.writeString( text ); }
    virtual void *read( ipstream& is ) { text = is.readString(); return this; }
private:
    virtual const char *streamableName() const { return name; }
    char *text;
};
const char * const TNote::name = "TNote";'''),

("TStreamableClass", "tobjstrm.h", [], "Uses_TStreamableClass",
 "Registers a streamable class (name, builder, base offset). Define one "
 "static object per class.",
r'''TStreamableClass RNote( TNote::name, TNote::build, __DELTA( TNote ) );

// in a module that must pull a class in from a static library:
__link( RNote )'''),

("pstream", "tobjstrm.h", [], "Uses_pstream",
 "Common base of the object streams: state (<code>good()</code>, "
 "<code>fail()</code>...), the type registry and errors.",
r'''if( !stream.good() )
    messageBox( "Stream error", mfError | mfOKButton );'''),

("ipstream", "tobjstrm.h", ["pstream"], "Uses_ipstream",
 "Reads objects and basic values. <code>&gt;&gt; ptr</code> creates the object "
 "of the stored class.",
r'''TNote *note;
is >> note;                 // builds a TNote and calls its read()
ushort w = is.readWord();
char *s = is.readString();  // allocated with new[]'''),

("opstream", "tobjstrm.h", ["pstream"], "Uses_opstream",
 "Writes objects and basic values.",
r'''os << note;                 // pointer: written once, then referenced
os.writeWord( 1 );
os.writeString( "text" );
os << 123456L;              // 4 bytes on disk, as in DOS'''),

("iopstream", "tobjstrm.h", ["ipstream", "opstream"], "Uses_iopstream",
 "Both directions on one stream buffer.",
r'''iopstream ios( &myStreamBuf );
ios << obj;
ios.seekg( 0 );
ios >> obj2;'''),

("fpbase", "tobjstrm.h", ["pstream"], "Uses_fpbase",
 "File stream base built on <code>std::filebuf</code>. <b>Port note:</b> the "
 "constructors taking a DOS file handle and <code>attach()</code> are gone.",
r'''// fpbase is not used directly; see ifpstream, ofpstream, fpstream
filebuf *fb = stream.rdbuf();'''),

("ifpstream", "tobjstrm.h", ["fpbase", "ipstream"], "Uses_ifpstream",
 "Reads objects from a file (opened in binary mode).",
r'''ifpstream in( "notes.dat" );
TNote *n;
in >> n;'''),

("ofpstream", "tobjstrm.h", ["fpbase", "opstream"], "Uses_ofpstream",
 "Writes objects to a file (truncates it).",
r'''ofpstream out( "notes.dat" );
out << new TNote( "remember the milk" );'''),

("fpstream", "tobjstrm.h", ["fpbase", "iopstream"], "Uses_fpstream",
 "Reads and writes one file; used for resource and help files. Opening an "
 "absent file for <code>ios::in | ios::out</code> creates it.",
r'''fpstream *s = new fpstream( "app.res", ios::in | ios::out | ios::binary );
TResourceFile *res = new TResourceFile( s );'''),
])

# --------------------------------------------------------------------------
section("Resources", "Named objects in a file and string lists for "
        "localization.", [

("TResourceFile", "resource.h", ["TObject"], "Uses_TResourceFile",
 "A file of objects addressed by string keys.",
r'''TResourceFile *res = new TResourceFile(
    new fpstream( "app.res", ios::out | ios::binary ) );
res->put( makeMainDialog(), "MainDialog" );
TObject::destroy( res );                // flushes and closes

TResourceFile *in = new TResourceFile(
    new fpstream( "app.res", ios::in | ios::binary ) );
TDialog *d = (TDialog *)in->get( "MainDialog" );'''),

("TStrListMaker", "resource.h", ["TObject", "TStreamable"], "Uses_TStrListMaker",
 "Builds a string list (write side). Stored in a resource file and read back "
 "as a <code>TStringList</code>.",
r'''TStrListMaker *m = new TStrListMaker( 4096, 64 );   // text size, index size
m->put( 1, "File not found" );
m->put( 2, "Disk full" );
res->put( m, "Strings" );'''),

("TStringList", "resource.h", ["TObject", "TStreamable"], "Uses_TStringList",
 "Read side of a string list: <code>get()</code> a string by its number.",
r'''TStringList *msgs = (TStringList *)res->get( "Strings" );
char buf[256];
msgs->get( buf, 2 );                    // "Disk full"'''),

("TStrIndexRec", "resource.h", [], "Uses_TStrIndexRec",
 "Index entry of a string list (key, count, offset). Internal.",
r'''// written by TStrListMaker, read by TStringList; not used directly'''),
])

# --------------------------------------------------------------------------
section("System layer", "Screen, keyboard, mouse and memory. In the Linux port "
        "these are implemented on top of the terminal driver in "
        "<code>source/platform/</code>.", [

("TDisplay", "system.h", [], "Uses_TScreen",
 "Low-level screen information: rows, columns, cursor shape. Sizes come from "
 "the terminal (<code>TIOCGWINSZ</code>).",
r'''ushort cols = TDisplay::getCols();      // terminal width (max 1024)
ushort rows = TDisplay::getRows();'''),

("TScreen", "system.h", ["TDisplay"], "Uses_TScreen",
 "The screen buffer and its size. <b>Port additions:</b> "
 "<code>screenWidth</code>/<code>screenHeight</code> are <code>ushort</code>, "
 "<code>screenBuffer</code> is an in-memory <code>ushort*</code>, "
 "<code>flushScreen()</code> sends the changed cells to the terminal, "
 "<code>checkResize()</code> picks up a window size change.",
r'''printf( "%dx%d\n", TScreen::screenWidth, TScreen::screenHeight );
// a long computation outside the event loop: show progress now
progress->drawView();
TScreen::flushScreen();'''),

("TEventQueue", "system.h", [], "Uses_TEventQueue",
 "Turns mouse reports into TV mouse events (double click, auto repeat). "
 "<b>Port addition:</b> <code>getShiftState()</code> returns the Shift/Ctrl/Alt "
 "state of the last key or mouse event.",
r'''TEventQueue::doubleDelay = 6;          // double-click time in ticks (55 ms)
TEventQueue::mouseReverse = True;      // swap the mouse buttons
if( TEventQueue::getShiftState() & (kbLeftShift | kbRightShift) )
    extendSelection();'''),

("THWMouse", "system.h", [], "Uses_TEvent",
 "Mouse hardware interface. In this port the terminal reports the mouse "
 "(xterm SGR protocol) and draws the pointer itself, so "
 "<code>show()</code>/<code>hide()</code> do nothing.",
r'''// nothing to call in normal programs; mouse events arrive as TEvent'''),

("TMouse", "system.h", ["THWMouse"], "Uses_TEvent",
 "Public face of <code>THWMouse</code>.",
r'''if( TMouse::present() ) { /* the terminal sends mouse reports */ }
MouseEventType m;
TMouse::getEvent( m );                 // current position and buttons'''),

("TSystemError", "system.h", [], "Uses_TSystemError",
 "In DOS: the critical error and Ctrl-Break handler. In this port a stub; "
 "Ctrl-C arrives as an ordinary key (<code>kbCtrlC</code>).",
r'''// kept for source compatibility
TSystemError::suspend();
TSystemError::resume();'''),

("TVMemMgr", "buffers.h", [], "Uses_TVMemMgr",
 "The safety pool and discardable buffers behind <code>lowMemory()</code> and "
 "<code>validView()</code>.",
r'''TVMemMgr::resizeSafetyPool( 8192 );
if( lowMemory() )
    messageBox( "Memory is low", mfWarning | mfOKButton );'''),
])

# --------------------------------------------------------------------------
section("Internal classes", "Helpers used by the library itself. You normally "
        "never create them, but they appear in constructors and debuggers.", [

("TProgInit", "app.h", [], "",
 "Virtual base of <code>TProgram</code> that stores the functions creating "
 "the status line, menu bar and desktop.",
r'''TMyApp::TMyApp() :
    TProgInit( &TMyApp::initStatusLine, &TMyApp::initMenuBar,
               &TMyApp::initDeskTop ) {}'''),

("TWindowInit", "views.h", [], "",
 "Virtual base of <code>TWindow</code> that stores the frame creation "
 "function. <b>Port addition:</b> a <code>TWindowInit( StreamableInit )</code> "
 "constructor.",
r'''TMyWindow::TMyWindow( const TRect& r ) :
    TWindow( r, "Title", wnNoNumber ),
    TWindowInit( &TMyWindow::initFrame ) {}'''),

("TDeskInit", "app.h", [], "",
 "Virtual base of <code>TDeskTop</code> that stores the background creation "
 "function.",
r'''TMyDeskTop::TMyDeskTop( const TRect& r ) :
    TDeskTop( r ), TDeskInit( &TMyDeskTop::initBackground ) {}'''),

("THistInit", "dialogs.h", [], "",
 "Virtual base of <code>THistoryWindow</code> that stores the viewer creation "
 "function.",
r'''THistoryWindow( r, id ) /* : THistInit( &THistoryWindow::initViewer ) */'''),

("TStreamableTypes", "tobjstrm.h", ["TNSSortedCollection"], "",
 "The registry of streamable classes, filled by <code>TStreamableClass</code> "
 "objects.",
r'''// pstream::initTypes() creates it; registration is automatic'''),

("TPWrittenObjects", "tobjstrm.h", ["TNSSortedCollection"], "",
 "Objects already written to an <code>opstream</code> (so that a second "
 "pointer to the same object is written as a reference).",
r'''// maintained by opstream'''),

("TPWObj", "tobjstrm.h", [], "",
 "An entry of <code>TPWrittenObjects</code>: address and id.",
r'''// maintained by opstream'''),

("TPReadObjects", "tobjstrm.h", ["TNSCollection"], "",
 "Objects already read from an <code>ipstream</code>, by id.",
r'''// maintained by ipstream'''),

("TBufListEntry", "buffers.h", [], "",
 "A discardable buffer (group screen caches) that is freed when memory runs "
 "low.",
r'''// used by TGroup::getBuffer() through TVMemMgr::allocateDiscardable()'''),
])
