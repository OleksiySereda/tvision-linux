/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   PAGE.H                                                                */
/*                                                                         */
/*   defines the class TPage                                               */
/*                                                                         */
/*   A group for the controls of one page of a dialog, as TTabView and     */
/*   TWizard use: it has the dialog's background under its controls (a     */
/*   view of its own, so that what a closed list or message box leaves     */
/*   is drawn again), and it knows its controls in Tab order.              */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TPage.                           */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TPage ) && !defined( __TPage )
#define __TPage

#include <vector>

class TRect;
class TValidatedInputLine;

class TPage : public TGroup
{

public:

    TPage( const TRect& bounds );

    // the controls Tab goes through, in Tab order (the order they were
    // inserted in)
    std::vector<TView *> tabOrder();
    // the first validated line, in Tab order, whose text is wrong; no
    // message is shown
    TValidatedInputLine *firstInvalid();
    // valid() of the page with the fields checked from the top: the
    // first wrong one shows its message and gets the focus
    Boolean validPage( ushort command );
    // the focus to the first control (a group would take the one
    // inserted last)
    void focusFirst();
    // Tab (forwards) or Shift+Tab inside the page: True if the focus
    // moved; False at the end, where the dialog goes on to its next
    // control, and the page then starts over from its first (last)
    // control the next time
    Boolean focusNext( Boolean forwards );
};

#endif  // Uses_TPage
