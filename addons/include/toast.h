/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   TOAST.H                                                               */
/*                                                                         */
/*   defines the classes TToast and TStatusMessageLine                     */
/*                                                                         */
/*   Short messages that go away by themselves, without a message box to   */
/*   close:                                                                */
/*                                                                         */
/*       TToast::show( "Saved", TToast::tkSuccess );   // 3 seconds        */
/*       TToast::show( "2 servers failed", TToast::tkError, 6 );           */
/*                                                                         */
/*   - a toast shows at the bottom right of the desktop, over the          */
/*     windows; several stack upwards; a click closes one;                 */
/*   - TStatusMessageLine is a status line that shows a message in place  */
/*     of its keys for a while: statusLine->message( "Copied 3 lines" );  */
/*                                                                         */
/*   Both go away when their time is up, which the program checks in its   */
/*   idle():                                                               */
/*                                                                         */
/*       void TMyApp::idle() { TApplication::idle(); TToast::idle(); }     */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TToast or                        */
/*   Uses_TStatusMessageLine.                                              */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TToast ) && !defined( __TToast )
#define __TToast

#include <string>

class TRect;
struct TEvent;

class TToast : public TView
{

public:

    enum { tkInfo, tkSuccess, tkWarning, tkError };

    // shows text for seconds on the desktop
    static TToast *show( const char *text, int kind = tkInfo, int seconds = 3 );
    // removes the toasts (and status messages) whose time is up
    static void idle();
    // removes all toasts at once
    static void clearAll();
    static int count();

    virtual void draw();
    virtual void handleEvent( TEvent& event );

    // white on cyan, green, brown, red
    static uchar colors[4];

    std::string text;
    int kind;
    long until;                         // milliseconds of the clock

    TToast( const TRect& bounds, const char *aText, int aKind, long anUntil );
    ~TToast();
};

// the clock of the toasts, in milliseconds
long toastClock();

#endif  // Uses_TToast


#if defined( Uses_TStatusMessageLine ) && !defined( __TStatusMessageLine )
#define __TStatusMessageLine

#include <string>

class TRect;
class TStatusDef;

class TStatusMessageLine : public TStatusLine
{

public:

    TStatusMessageLine( const TRect& bounds, TStatusDef& aDefs );
    ~TStatusMessageLine();

    // shows text in place of the keys for seconds (0: until the next
    // message or clearMessage())
    void message( const char *text, int seconds = 3 );
    void clearMessage();
    Boolean hasMessage() const
        { return Boolean( !text.empty() ); }

    virtual void draw();

    // called by TToast::idle()
    void tick( long now );

    std::string text;
    long until;
};

#endif  // Uses_TStatusMessageLine
