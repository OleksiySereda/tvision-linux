/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   ADVEDIT.H                                                             */
/*                                                                         */
/*   defines the classes TAdvancedEditor and TBlockIndicator               */
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
/*   Ctrl+B starts and ends selecting with the cursor keys alone, as F3    */
/*   does in Midnight Commander; for terminals that do not send Shift      */
/*   with the cursor keys.                                                 */
/*                                                                         */
/*   The mouse wheel scrolls the text (evMouseWheel, Linux port).          */
/*                                                                         */
/*   TBlockIndicator is a TIndicator that shows "B" while Ctrl+B           */
/*   selecting is on.                                                      */
/*                                                                         */
/*   TLineNumbers shows the line numbers left of a TAdvancedEditor.        */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TAdvancedEditor,                 */
/*   Uses_TBlockIndicator or Uses_TLineNumbers.                            */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if !defined( __ADVEDIT_COMMAND_CODES )
#define __ADVEDIT_COMMAND_CODES

// like the other editor commands (500-523) these cannot be disabled, so
// they may be put into menus as they are
const int
  cmSelectAll    = 530,     // select the whole text
  cmToggleSelect = 531;     // Ctrl+B: start / end selecting

#endif  // __ADVEDIT_COMMAND_CODES


#if defined( Uses_TBlockIndicator ) && !defined( __TBlockIndicator )
#define __TBlockIndicator

class TRect;

class TBlockIndicator : public TIndicator
{

public:

    TBlockIndicator( const TRect& );

    virtual void draw();

    // shows or hides the "B"
    void setBlock( Boolean on );
    Boolean block;

private:

    virtual const char *streamableName() const
        { return name; }

protected:

    TBlockIndicator( StreamableInit );

public:

    static const char * const name;
    static TStreamable *build();

};

inline ipstream& operator >> ( ipstream& is, TBlockIndicator& cl )
    { return is >> (TStreamable&)cl; }
inline ipstream& operator >> ( ipstream& is, TBlockIndicator*& cl )
    { return is >> (void *&)cl; }

inline opstream& operator << ( opstream& os, TBlockIndicator& cl )
    { return os << (TStreamable&)cl; }
inline opstream& operator << ( opstream& os, TBlockIndicator* cl )
    { return os << (TStreamable *)cl; }

#endif  // Uses_TBlockIndicator


#if defined( Uses_TAdvancedEditor ) && !defined( __TAdvancedEditor )
#define __TAdvancedEditor

class TRect;
class TScrollBar;
class TIndicator;
class TLineNumbers;
struct TEvent;

class TAdvancedEditor : public TFileEditor
{

public:

    TAdvancedEditor( const TRect&, TScrollBar *, TScrollBar *, TIndicator *,
                     const char *aFileName );

    virtual void handleEvent( TEvent& );

    void selectAll();

    // Ctrl+B: starts selecting (the cursor keys then extend the
    // selection, as with Shift) or ends it, keeping the selection.
    // Copying, cutting, typing and pasting end it too.
    void toggleSelect();

    // False gives Ctrl+C, X, V and A back to the WordStar commands
    Boolean clipboardKeys;

    // Scrolls the text by wheelLines for one wheel step (mwUp, mwDown,
    // mwLeft, mwRight; with Shift, up and down scroll sideways).  The
    // cursor and the selection do not move.
    virtual void wheel( uchar direction );
    int wheelLines;

    // redraws the line numbers too (see TLineNumbers)
    virtual void draw();
    virtual void shutDown();
    TLineNumbers *lineNumbers;          // set by TLineNumbers, else 0

private:

    // tells a TBlockIndicator whether selecting is on
    void showSelecting();

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


#if defined( Uses_TLineNumbers ) && !defined( __TLineNumbers )
#define __TLineNumbers

class TRect;
class TAdvancedEditor;

// The numbers of the lines an editor shows, in a column of their own left
// of it: 4 digits (white on gray) and 2 blanks, so 6 wide.  Put it next to
// the editor, at the same height; it follows the editor's scrolling.
class TLineNumbers : public TView
{

public:

    TLineNumbers( const TRect&, TAdvancedEditor *anEditor );

    virtual void draw();
    virtual void shutDown();

    // the colour of the digits; the 2 blanks take the editor's
    static uchar numberColor;

private:

    friend class TAdvancedEditor;       // forgets editor when it shuts down
    TAdvancedEditor *editor;
};

#endif  // Uses_TLineNumbers
