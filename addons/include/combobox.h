/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   COMBOBOX.H                                                            */
/*                                                                         */
/*   defines the classes TComboBox and TComboButton                        */
/*                                                                         */
/*   An input line with a list to choose from, like THistory with a list  */
/*   the program gives:                                                    */
/*                                                                         */
/*       TComboBox *env = new TComboBox( TRect( 14, 2, 30, 3 ), 21,       */
/*                                       "dev|test|prod" );                */
/*       env->setListOnly( True );        // nothing outside the list       */
/*       d->insert( env );                                                 */
/*       d->insert( new TComboButton( TRect( 30, 2, 33, 3 ), env ) );      */
/*                                                                         */
/*   - Down in the line, or a click on the button, opens                  */
/*     the list under the line; Enter or a double click takes an item,     */
/*     Esc closes it;                                                      */
/*   - typing works as in any input line; with listOnly, the text must     */
/*     be one of the items (without regard to case) when the dialog is     */
/*     closed, and the case is then made that of the item;                 */
/*   - it is a TValidatedInputLine, so any validator can be used instead   */
/*     (setValidator()); the data are the text, as for a TInputLine.       */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TComboBox.                       */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TComboBox ) && !defined( __TComboBox )
#define __TComboBox

#include <string>
#include <vector>

class TRect;
struct TEvent;

class TComboBox : public TValidatedInputLine
{

public:

    // aMaxLen as for TInputLine (the size with the 0); items separated
    // by separator, or 0
    TComboBox( const TRect& bounds, int aMaxLen, const char *aItems = 0,
               char separator = '|' );

    void setItems( const char *aItems, char separator = '|' );
    void addItem( const char *item );
    void clearItems();
    int itemCount() const
        { return int( items.size() ); }
    const char *item( int i ) const;
    // the item equal to the text (without regard to case), or -1
    int findItem( const char *text ) const;

    // opens the list; returns the item taken, or -1
    int openList();
    // the text becomes item i
    void selectItem( int i );

    // True: only an item of the list is valid
    void setListOnly( Boolean on );
    Boolean listOnly() const
        { return onlyList; }

    // lines of the list at most
    int listHeight;

    virtual void handleEvent( TEvent& event );
    virtual Boolean valid( ushort command );

private:

    std::vector<std::string> items;
    Boolean onlyList;
};

// the down arrow right of a TComboBox; Palette as THistory's
class TComboButton : public TView
{

public:

    TComboButton( const TRect& bounds, TComboBox *aLink );

    virtual void draw();
    virtual TPalette& getPalette() const;
    virtual void handleEvent( TEvent& event );
    virtual void shutDown();

    TComboBox *link;
};

#endif  // Uses_TComboBox
