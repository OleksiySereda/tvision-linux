/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   TABVIEW.H                                                             */
/*                                                                         */
/*   defines the class TTabView                                            */
/*                                                                         */
/*   Pages with tabs over them, in a dialog:                               */
/*                                                                         */
/*       TTabView *tabs = new TTabView( TRect( 1, 1, 59, 16 ) );           */
/*       TGroup *p = tabs->addPage( "~G~eneral" );                         */
/*       p->insert( new TInputLine( TRect( 14, 1, 40, 2 ), 30 ) );         */
/*       ...                                                               */
/*       p = tabs->addPage( "~N~etwork" );                                 */
/*       ...                                                               */
/*       d->insert( tabs );                                                */
/*                                                                         */
/*   - the first line shows the titles; a page fills the rest, and its     */
/*     controls are placed in its own coordinates (0, 0 is just under      */
/*     the titles);                                                        */
/*   - a click on a title, Alt and the highlighted letter of a title, or   */
/*     Ctrl+PgDn / Ctrl+PgUp (the next / previous page) show a page;       */
/*   - Tab and Shift+Tab go through the controls of the page, then on to   */
/*     the controls of the dialog after or before the tabs;                */
/*   - getData() and setData() of the dialog take every page, in the       */
/*     order they were added, so one record holds the whole dialog;        */
/*   - when the dialog is closed (OK), a page with a wrong field is shown  */
/*     before its message;                                                 */
/*   - a new page broadcasts cmTabChanged to the owner (infoPtr: the       */
/*     TTabView).                                                          */
/*                                                                         */
/*   The colours are the dialog's (label and button colours): put it      */
/*   into a TDialog.                                                       */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TTabView.                        */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TTabView ) && !defined( __TTabView )
#define __TTabView

#include <string>
#include <vector>

class TRect;
struct TEvent;

const ushort cmTabChanged = 540;

class TTabView : public TGroup
{

public:

    TTabView( const TRect& bounds );

    // a new page with title ("~N~etwork": Alt+N shows it); the first one
    // added is shown
    TGroup *addPage( const char *title );

    int pageCount() const
        { return int( pages.size() ); }
    TGroup *page( int i ) const
        { return i >= 0 && i < pageCount() ? pages[i] : 0; }
    const char *pageTitle( int i ) const;
    int currentPage() const
        { return shown; }
    // shows page i and moves the focus into it when the tabs have it
    void selectPage( int i );

    virtual void handleEvent( TEvent& event );
    virtual Boolean valid( ushort command );
    virtual void setState( ushort aState, Boolean enable );
    virtual void shutDown();

    // the x of each title on the first line, and its width
    int tabX( int i ) const;
    int tabWidth( int i ) const;

private:

    Boolean moveFocus( Boolean forwards );
    void startPage( int i );
    int pageWithInvalidLine( ushort command );

    std::vector<TGroup *> pages;
    std::vector<std::string> titles;
    int shown;
    TView *titles_;                     // the first line
    std::vector<bool> started;          // the focus has been in the page
};

#endif  // Uses_TTabView
