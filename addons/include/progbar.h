/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   PROGBAR.H                                                             */
/*                                                                         */
/*   defines the classes TProgressBar and TProgressDialog                  */
/*                                                                         */
/*   TProgressBar is a bar that fills from the left as value goes from     */
/*   0 to total, with the percentage (or a text of its own) in the         */
/*   middle:                                                               */
/*                                                                         */
/*       bar = new TProgressBar( TRect( 2, 3, 50, 4 ), 120 );              */
/*       bar->setValue( 30 );                  // 25%                      */
/*                                                                         */
/*   TProgressDialog is a dialog with a message, a bar and a Cancel       */
/*   button, for work that takes a while.  Either a loop of the program    */
/*   runs it, giving the screen a chance between its steps:               */
/*                                                                         */
/*       TProgressDialog *d = new TProgressDialog( "Copying", n );         */
/*       TProgram::deskTop->insert( d );                                   */
/*       for( int i = 0; i < n && !d->cancelled(); i++ )                   */
/*           {                                                             */
/*           d->setText( names[i] );                                       */
/*           copy( names[i] );                                             */
/*           d->setValue( i + 1 );                                         */
/*           }                                                             */
/*       TObject::destroy( d );                                            */
/*                                                                         */
/*   cancelled() handles the events that came in meanwhile (the Cancel     */
/*   button, Esc, a resized window); the rest of the program does not get  */
/*   them while the loop runs.  Or the dialog is run with execView() and   */
/*   the work goes on in idle() or in events: setValue(), setText(), and   */
/*   finish() ends it with cmOK; Cancel ends it with cmCancel.             */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TProgressBar or                  */
/*   Uses_TProgressDialog.                                                 */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TProgressBar ) && !defined( __TProgressBar )
#define __TProgressBar

#include <string>

class TRect;

// Palette layout (into the dialog's)
//   1 = the part done (list viewer focused)
//   2 = the rest (scroll bar page)
class TProgressBar : public TView
{

public:

    TProgressBar( const TRect& bounds, long aTotal = 100 );

    virtual void draw();
    virtual TPalette& getPalette() const;

    void setValue( long aValue );
    void setTotal( long aTotal );
    long value() const
        { return current; }
    long total() const
        { return maximum; }
    // 0 to 100
    int percent() const;

    // shown in the middle instead of "45%"; "" goes back to the percentage
    void setText( const char *aText );
    // False: no text at all
    Boolean showText;

private:

    long current, maximum;
    std::string text;
};

#endif  // Uses_TProgressBar


#if defined( Uses_TProgressDialog ) && !defined( __TProgressDialog )
#define __TProgressDialog

class TProgressBar;
class TProgressLine;
struct TEvent;

class TProgressDialog : public TDialog
{

public:

    // 50 wide, centered on the desktop
    TProgressDialog( const char *title, long total = 100 );

    virtual void handleEvent( TEvent& event );
    virtual void putEvent( TEvent& event );

    // the line above the bar
    void setText( const char *text );
    void setValue( long value );
    void setTotal( long total );

    // handles the events waiting (see above) and says whether Cancel was
    // pressed; at most every pollMs milliseconds, so that a loop of
    // short steps is not slowed down
    Boolean cancelled();
    // a modal dialog ends with cmOK
    void finish();

    TProgressBar *bar;
    int pollMs;

private:

    TProgressLine *message;
    Boolean cancelFlag;
    long lastPoll;
};

#endif  // Uses_TProgressDialog
