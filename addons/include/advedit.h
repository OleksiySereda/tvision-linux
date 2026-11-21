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
/*       Ctrl+G  go to line     (a dialog: editorDialog( edGotoLine ))     */
/*       Ctrl+F  incremental search: the text typed next is searched      */
/*               as it is typed; Ctrl+F again finds the next one,          */
/*               Backspace takes a character back, Enter ends it at the    */
/*               text found, Esc goes back to where it started             */
/*                                                                         */
/*   Ctrl+G and Ctrl+F (WordStar delete char and word right, also on Del   */
/*   and Ctrl+Right) belong to clipboardKeys too.  The bracket next to     */
/*   the cursor and its pair are shown in bracketColor.                    */
/*                                                                         */
/*   TBlockIndicator is a TIndicator that shows "B" while Ctrl+B           */
/*   selecting is on, and the text of an incremental search.               */
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
  cmToggleSelect = 531,     // Ctrl+B: start / end selecting
  cmGotoLine     = 532,     // Ctrl+G: asks for a line number
  cmIncSearch    = 533;     // Ctrl+F: incremental search

// the dialog of cmGotoLine, after TEditor's edOutOfMemory .. edReplacePrompt:
// editorDialog( edGotoLine, char *number ) gets the line number as text
// (at most gotoLineLen characters with the 0) and returns cmOK or cmCancel
const int
  edGotoLine  = 20,
  gotoLineLen = 8;

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

    // shows "/text" (or "!text" when it is not found) instead of the
    // position while an incremental search runs; 0 ends it
    void setSearch( const char *text, Boolean found );
    char search[40];
    Boolean searching, searchFound;

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
    virtual void setState( ushort, Boolean );
    TLineNumbers *lineNumbers;          // set by TLineNumbers, else 0

    // moves the cursor to the start of line (1 = the first; past the end:
    // the last line) and shows it in the middle of the editor
    void gotoLine( int line );
    // the line of the cursor, 1 = the first
    int cursorLine();
    // Ctrl+G: asks for the line through editorDialog( edGotoLine )
    void askGotoLine();

    // the incremental search (Ctrl+F); typed text is searched without
    // regard to case.  Ending it leaves the text in findStr, so Ctrl+L
    // (cmSearchAgain) goes on with it.
    void startIncSearch();
    void endIncSearch( Boolean keep );
    Boolean incSearching() const
        { return searching; }
    const char *incSearchText() const
        { return searchText; }

    // the pair of the bracket ( ) [ ] { } at the cursor, or else just
    // before it: False if there is none; ignores quotes and comments
    Boolean bracketPair( ushort& at, ushort& pair );
    // the bracket that pairs with the one at pos, or False
    Boolean matchBracket( ushort pos, ushort& pair );
    Boolean matchBrackets;              // True: show the pair
    static uchar bracketColor;          // black on cyan

protected:

    virtual void drawLines( int, int, ushort );
    // paints the bracket pair over the lines drawLines() has drawn;
    // an editor with its own drawLines() calls it at the end
    void paintBrackets( int y, int count, ushort linePtr );

private:

    // tells a TBlockIndicator whether selecting is on
    void showSelecting();
    void showSearch();
    Boolean searchFrom( ushort pos );
    Boolean incSearchKey( TEvent& );

    Boolean searching, searchFound;
    char searchText[maxFindStrLen];
    ushort searchOrigin;
    ushort shownAt, shownPair;          // the brackets drawn last, or 0xFFFF

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
//
// A line may be marked: its number is shown in a colour of its own, for
// a breakpoint, an error or the line being run.  The marks belong to the
// line numbers, not to the text: they do not move when lines are inserted
// or deleted above them.
class TLineNumbers : public TView
{

public:

    TLineNumbers( const TRect&, TAdvancedEditor *anEditor );

    virtual void draw();
    virtual void shutDown();

    // the colour of the digits; the 2 blanks take the editor's
    static uchar numberColor;

    // marks line (1 = the first) with the attribute color, or changes the
    // colour of its mark; color 0 removes the mark
    void setMark( int line, uchar color );
    void clearMark( int line )
        { setMark( line, 0 ); }
    void clearMarks();
    // the colour of the mark of line, 0 if it has none
    uchar markOf( int line ) const;

private:

    friend class TAdvancedEditor;       // forgets editor when it shuts down
    TAdvancedEditor *editor;

    enum { maxMarks = 64 };
    int markLine[maxMarks];
    uchar markColor[maxMarks];
    int markCount;
};

#endif  // Uses_TLineNumbers
