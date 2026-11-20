/*------------------------------------------------------------*/
/*   Tests of TAdvancedEditor, TLineNumbers and             */
/*   TColoredEditor.  make check-addons                       */
/*------------------------------------------------------------*/

#define Uses_TAdvancedEditor
#define Uses_TBlockIndicator
#define Uses_TLineNumbers
#define Uses_TColoredEditor
#define Uses_TEditor
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

// what the keyboard sends: the scan code of the letter and the control
// character (kbCtrlC etc. have only the character)
const ushort keyCtrlA = 0x1E01, keyCtrlB = 0x3002, keyCtrlC = 0x2E03, keyCtrlK = 0x250B,
             keyCtrlV = 0x2F16, keyCtrlX = 0x2D18;

static void key( TEditor *ed, ushort keyCode )
{
    TEvent e;
    memset( &e, 0, sizeof( e ) );
    e.what = evKeyDown;
    e.keyDown.keyCode = keyCode;
    ed->handleEvent( e );
}

static std::string text( TEditor *ed )
{
    std::string s;
    for( ushort p = 0; p < ed->bufLen; p++ )
        s += ed->bufChar( p );
    return s;
}

// colours the digits 1 to 9 by their value
class TDigitEditor : public TColoredEditor
{
public:
    TDigitEditor( const TRect &r ) : TColoredEditor( r, 0, 0, 0, 0 ) {}
    virtual void scanLine( ushort, const char *s, int n, uchar *cls )
        {
        for( int i = 0; i < n; i++ )
            cls[i] = s[i] >= '1' && s[i] <= '9' ? uchar( s[i] - '0' ) : 0;
        }
    virtual uchar classColor( uchar cls, uchar normal )
        { return cls != 0 ? uchar( 0x40 | cls ) : normal; }
};

static std::string selection( TEditor *ed )
{
    std::string s;
    for( ushort p = ed->selStart; p < ed->selEnd; p++ )
        s += ed->bufChar( p );
    return s;
}

int main()
{
    TRect r( 0, 0, 80, 25 );
    TEditor *clip = new TEditor( r, 0, 0, 0, 1000 );
    TEditor::clipboard = clip;

    TAdvancedEditor *ed = new TAdvancedEditor( r, 0, 0, 0, 0 );
    const char *t = "alpha\r\nbeta\r\n";
    ed->insertText( t, ushort( strlen( t ) ), False );

    key( ed, keyCtrlA );
    check( ed->selStart == 0 && ed->selEnd == ed->bufLen, "Ctrl+A selects all" );

    key( ed, keyCtrlC );
    check( selection( clip ) == t, "Ctrl+C copies the selection" );
    check( text( ed ) == t, "Ctrl+C keeps the text" );

    ed->setSelect( 0, 5, False );               // "alpha"
    key( ed, keyCtrlX );
    check( text( ed ) == "\r\nbeta\r\n", "Ctrl+X removes the selection" );
    check( selection( clip ) == "alpha", "Ctrl+X puts it on the clipboard" );

    ed->setSelect( ed->bufLen, ed->bufLen, False );
    key( ed, keyCtrlV );
    check( text( ed ) == "\r\nbeta\r\nalpha", "Ctrl+V pastes at the cursor" );

    // after the Ctrl+K prefix the WordStar block keys are used: ^K ^C = paste
    ed->setSelect( 0, 0, False );
    key( ed, keyCtrlK );
    key( ed, keyCtrlC );
    check( text( ed ) == "alpha\r\nbeta\r\nalpha", "Ctrl+K Ctrl+C pastes (WordStar)" );

    // clipboardKeys = False: Ctrl+A is word left again, nothing is selected
    ed->clipboardKeys = False;
    ed->setSelect( 3, 3, False );
    key( ed, keyCtrlA );
    check( ed->curPtr == 0 && ed->selStart == ed->selEnd, "Ctrl+A is word left when off" );
    ushort len = ed->bufLen;
    ed->setSelect( 0, 5, False );
    key( ed, keyCtrlC );
    check( ed->bufLen == len, "Ctrl+C does not copy or cut when off" );

    // the wheel scrolls the text, the cursor stays
    std::string many;
    for( int i = 0; i < 100; i++ )
        many += "line\r\n";
    ed->setSelect( 0, ed->bufLen, False );
    ed->insertText( many.c_str(), ushort( many.size() ), False );
    ed->setSelect( 0, 0, False );
    ed->scrollTo( 0, 0 );
    check( ( ed->eventMask & evMouseWheel ) != 0, "the editor takes evMouseWheel" );
    TEvent e;
    memset( &e, 0, sizeof( e ) );
    e.what = evMouseWheel;
    e.mouse.buttons = mwDown;
    ed->handleEvent( e );
    check( ed->delta.y == 3 && ed->curPtr == 0, "wheel down scrolls 3 lines" );
    check( e.what == evNothing, "the wheel event is cleared" );
    e.what = evMouseWheel;
    e.mouse.buttons = mwUp;
    ed->handleEvent( e );
    e.what = evMouseWheel;
    ed->handleEvent( e );
    check( ed->delta.y == 0, "wheel up stops at the top" );

    // Ctrl+B: the cursor keys select without Shift, a second Ctrl+B ends it
    TBlockIndicator *ind = new TBlockIndicator( TRect( 0, 0, 14, 1 ) );
    TAdvancedEditor *bed = new TAdvancedEditor( r, 0, 0, ind, 0 );
    bed->insertText( many.c_str(), ushort( many.size() ), False );
    bed->setSelect( 0, 0, False );
    key( bed, keyCtrlB );
    check( ind->block == True, "Ctrl+B shows B" );
    key( bed, kbDown );
    key( bed, kbDown );
    check( selection( bed ) == "line\r\nline\r\n", "Ctrl+B, Down, Down selects two lines" );
    key( bed, keyCtrlB );
    check( ind->block == False, "the second Ctrl+B hides B" );
    key( bed, kbDown );
    check( bed->selStart == bed->selEnd, "after it Down does not select" );
    key( bed, keyCtrlB );
    key( bed, kbRight );
    key( bed, keyCtrlC );
    check( ind->block == False && selection( clip ) == "l", "Ctrl+C copies and ends selecting" );
    TObject::destroy( bed );
    TObject::destroy( ind );

    // line marks
    TLineNumbers *nums = new TLineNumbers( TRect( 0, 0, 6, 25 ), ed );
    check( ed->lineNumbers == nums, "TLineNumbers attaches to the editor" );
    nums->setMark( 2, 0x2F );
    nums->setMark( 5, 0x4F );
    check( nums->markOf( 2 ) == 0x2F && nums->markOf( 5 ) == 0x4F && nums->markOf( 3 ) == 0,
           "setMark marks lines" );
    nums->setMark( 2, 0x1F );
    check( nums->markOf( 2 ) == 0x1F, "setMark changes a mark" );
    nums->clearMark( 2 );
    check( nums->markOf( 2 ) == 0 && nums->markOf( 5 ) == 0x4F, "clearMark removes one mark" );
    for( int i = 1; i <= 70; i++ )
        nums->setMark( 100 + i, 0x2F );
    check( nums->markOf( 5 ) == 0 && nums->markOf( 170 ) == 0x2F, "past 64 marks the oldest goes" );
    nums->clearMarks();
    check( nums->markOf( 170 ) == 0, "clearMarks removes all" );
    TObject::destroy( nums );
    check( ed->lineNumbers == 0, "destroyed line numbers detach" );

    // TColoredEditor: scanLine and classColor, through a derived class
    TDigitEditor *ded = new TDigitEditor( r );
    const char *dt = "a1b2";
    ded->insertText( dt, ushort( strlen( dt ) ), False );
    char buf[16];
    int n = ded->lineText( 0, buf, sizeof( buf ) );
    check( n == 4 && memcmp( buf, dt, 4 ) == 0, "lineText gives the line" );
    uchar cls[4];
    ded->scanLine( 0, buf, n, cls );
    check( cls[0] == 0 && cls[1] == 1 && cls[3] == 2, "scanLine classifies" );
    check( ded->classColor( 2, 0x1E ) == 0x42 && ded->classColor( 0, 0x1E ) == 0x1E,
           "classColor colours the classes" );
    check( ded->highlight == True, "highlight is on" );
    TObject::destroy( ded );

    TEditor::clipboard = 0;
    TObject::destroy( ed );
    TObject::destroy( clip );

    printf( "%d checks, %d failed\n", checks, failures );
    return failures == 0 ? 0 : 1;
}
