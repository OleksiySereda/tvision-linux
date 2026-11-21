/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   WIZARD.H                                                              */
/*                                                                         */
/*   defines the class TWizard                                             */
/*                                                                         */
/*   A dialog of steps, one page at a time, with Back, Next (Finish on     */
/*   the last page) and Cancel:                                            */
/*                                                                         */
/*       TWizard *w = new TWizard( TRect( 0, 0, 60, 18 ), "Install" );     */
/*       TPage *p = w->addPage( "Where to" );                              */
/*       p->insert( new TInputLine( TRect( 15, 1, 50, 2 ), 61 ) );         */
/*       p = w->addPage( "Options" );                                      */
/*       ...                                                               */
/*       if( deskTop->execView( w ) == cmOK ) w->getData( &rec );          */
/*                                                                         */
/*   - the first line says "Step 2 of 4: Options";                         */
/*   - Next checks the page (its validated lines from the top, and         */
/*     canLeave()) before it goes on; Back does not check;                 */
/*   - Finish checks the last page and ends the dialog with cmOK;          */
/*   - Tab goes through the controls of the page, then to the buttons;     */
/*   - getData() / setData() take every page, in the order they were       */
/*     added;                                                              */
/*   - a derived wizard overrides canLeave() (e.g. to check a path) and    */
/*     pageShown() (to fill a page from what came before); as for any      */
/*     window, its constructor starts TWindowInit itself:                  */
/*         TMyWizard() : TWindowInit( &TMyWizard::initFrame ),             */
/*                       TWizard( r, "Install" ) { ... }                   */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TWizard.                         */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TWizard ) && !defined( __TWizard )
#define __TWizard

#include <string>
#include <vector>

class TRect;
class TPage;
class TButton;
struct TEvent;

const ushort
    cmWizardBack   = 541,
    cmWizardNext   = 542,
    cmWizardFinish = 543;

class TWizard : public TDialog
{

public:

    TWizard( const TRect& bounds, const char *aTitle );

    // a new page; the first one added is shown first
    TPage *addPage( const char *title );

    int pageCount() const
        { return int( pages.size() ); }
    TPage *page( int i ) const
        { return i >= 0 && i < pageCount() ? pages[size_t( i )] : 0; }
    const char *pageTitle( int i ) const;
    int currentPage() const
        { return shown; }

    // Next and Back from the program; False if the page may not be left
    Boolean next();
    void back();
    // shows page i without checks
    void showPage( int i );

    // may page i be left (forwards: by Next or Finish)?  By default,
    // forwards needs the fields of the page to be valid
    virtual Boolean canLeave( int i, Boolean forwards );
    // page i has just been shown
    virtual void pageShown( int i );

    virtual void handleEvent( TEvent& event );
    // shows the first page with the focus on its first control (the
    // controls are inserted after addPage())
    virtual ushort execute();

    TButton *backButton, *nextButton, *finishButton;

private:

    void updateButtons();

    std::vector<TPage *> pages;
    std::vector<std::string> titles;
    std::vector<bool> started;
    int shown;
    TView *header;
};

#endif  // Uses_TWizard
