/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   ADVEDIT.H                                                             */
/*                                                                         */
/*   defines the class TAdvancedEditor                                     */
/*                                                                         */
/*   TAdvancedEditor is a TFileEditor with the clipboard keys of today's   */
/*   editors:                                                              */
/*                                                                         */
/*       Ctrl+C  copy           Ctrl+X  cut                                */
/*       Ctrl+V  paste          Ctrl+A  select all                         */
/*                                                                         */
/*   In TEditor these keys are WordStar commands (page down, line down,    */
/*   insert mode, word left); those stay on PgDn, Down, Ins and            */
/*   Ctrl+Left.  After the Ctrl+K and Ctrl+Q prefixes the WordStar keys    */
/*   work as before.  Selecting with Shift and the cursor keys (arrows,    */
/*   Home/End, PgUp/PgDn, Ctrl+arrows) is TEditor's own.                   */
/*                                                                         */
/*   The mouse wheel scrolls the text (evMouseWheel, Linux port).          */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TAdvancedEditor.                 */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if !defined( __ADVEDIT_COMMAND_CODES )
#define __ADVEDIT_COMMAND_CODES

// selects the whole text; like the other editor commands (500-523) it
// cannot be disabled, so it may be put into menus as it is
const int
  cmSelectAll   = 530;

#endif  // __ADVEDIT_COMMAND_CODES


#if defined( Uses_TAdvancedEditor ) && !defined( __TAdvancedEditor )
#define __TAdvancedEditor

class TRect;
class TScrollBar;
class TIndicator;
struct TEvent;

class TAdvancedEditor : public TFileEditor
{

public:

    TAdvancedEditor( const TRect&, TScrollBar *, TScrollBar *, TIndicator *,
                     const char *aFileName );

    virtual void handleEvent( TEvent& );

    void selectAll();

    // False gives Ctrl+C, X, V and A back to the WordStar commands
    Boolean clipboardKeys;

    // Scrolls the text by wheelLines for one wheel step (mwUp, mwDown,
    // mwLeft, mwRight; with Shift, up and down scroll sideways).  The
    // cursor and the selection do not move.
    virtual void wheel( uchar direction );
    int wheelLines;

private:

    virtual const char *streamableName() const
        { return name; }

protected:

    TAdvancedEditor( StreamableInit );

public:

    static const char * const name;
    static TStreamable *build();

};

inline ipstream& operator >> ( ipstream& is, TAdvancedEditor& cl )
    { return is >> (TStreamable&)cl; }
inline ipstream& operator >> ( ipstream& is, TAdvancedEditor*& cl )
    { return is >> (void *&)cl; }

inline opstream& operator << ( opstream& os, TAdvancedEditor& cl )
    { return os << (TStreamable&)cl; }
inline opstream& operator << ( opstream& os, TAdvancedEditor* cl )
    { return os << (TStreamable *)cl; }

#endif  // Uses_TAdvancedEditor
