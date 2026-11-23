/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   LOGVIEW.H                                                             */
/*                                                                         */
/*   defines the class TLogView                                            */
/*                                                                         */
/*   A TScroller that shows lines of text added at the end, as a log:     */
/*                                                                         */
/*       log = new TLogView( r, hScrollBar, vScrollBar );                  */
/*       log->add( "Build: 19 cells" );                                    */
/*       log->add( "line 3: unknown label", TLogView::errorColor );        */
/*       log->addf( "STOP after %d steps", n );                            */
/*                                                                         */
/*   - only the last maxLines lines are kept (2000 by default);            */
/*   - while the end is shown, new lines scroll it along (autoScroll);     */
/*     scrolled up, the view stays where it is;                            */
/*   - a line may have a colour of its own (an attribute, 0 = the          */
/*     view's colour);                                                     */
/*   - tabs become blanks, '\n' starts a new line;                         */
/*   - the arrow keys, PgUp/PgDn, Home/End (Ctrl+PgUp/PgDn too) and the    */
/*     mouse wheel scroll it, with or without scroll bars;                 */
/*   - saveToFile() writes the lines to a file;                            */
/*   - find() looks for a text (without regard to case), setHighlight()    */
/*     shows every place of it in matchColor, showLine() scrolls a line    */
/*     into view and shows its places in foundColor:                       */
/*                                                                         */
/*       log->setHighlight( "error" );                                     */
/*       int i = log->find( "error", 0, True );   // -1: none              */
/*       if( i >= 0 )                                                      */
/*           log->showLine( i );                                           */
/*                                                                         */
/*   A view of lines kept elsewhere overrides lineCount(), lineText()      */
/*   and lineColor() and calls update() when there are new ones.          */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TLogView.                        */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TLogView ) && !defined( __TLogView )
#define __TLogView

#include <deque>
#include <string>

class TRect;
class TScrollBar;
struct TEvent;

class TLogView : public TScroller
{

public:

    TLogView( const TRect& bounds, TScrollBar *aHScrollBar, TScrollBar *aVScrollBar,
              int aMaxLines = 2000 );

    // adds text (several lines if it has '\n') in color, 0 = normal
    void add( const char *text, uchar color = 0 );
    void add( const std::string& text, uchar color = 0 )
        { add( text.c_str(), color ); }
    // printf; the line in the normal colour
    void addf( const char *format, ... )
#if defined( __GNUC__ )
        __attribute__(( format( printf, 2, 3 ) ))
#endif
        ;
    void clear();

    // the lines shown; the text is valid until the next change
    virtual int lineCount();
    virtual const char *lineText( int i );
    virtual uchar lineColor( int i );
    // after lines have been added (or the lines of a derived view have
    // changed): sets the limits and follows the end
    void update();

    // writes every line, '\n' after each; False if it could not
    Boolean saveToFile( const char *fileName );

    // the first line from `from` on (forward) or before it (backward)
    // with `text` in it, without regard to case, round the end; -1 if
    // there is none or the text is empty
    int find( const std::string& text, int from, Boolean forward );
    // the places of the text are shown in matchColor ("": none)
    void setHighlight( const std::string& text );
    const std::string& highlightText() const
        { return highlight; }
    // the lines with the highlight text in them (before `line`, if >= 0)
    int countMatches( int line = -1 );
    // scrolls the line into view, with the highlight text if it is out of
    // sight to the side, and shows its places in foundColor; -1: none
    void showLine( int line );
    int shownLine() const
        { return current; }

    virtual void draw();
    virtual void handleEvent( TEvent& event );
    virtual void changeBounds( const TRect& bounds );
    // keeps delta where there is no scroll bar
    virtual void scrollDraw();

    // moves the text, with or without scroll bars
    void scrollToPos( int x, int y );
    void scrollToEnd();
    Boolean atEnd();
    // the first column and line shown (delta)
    TPoint position() const
        { return delta; }

    int maxLines;               // the lines kept, at least 1
    Boolean autoScroll;         // True: new lines are scrolled into view
    int wheelLines;             // lines per wheel step, 3

    static uchar errorColor;    // white on red
    static uchar warningColor;  // yellow on brown
    static uchar goodColor;     // white on green
    static uchar matchColor;    // black on cyan: the highlight text
    static uchar foundColor;    // yellow on green: it in the line of showLine()

private:

    struct Line
    {
        std::string text;
        uchar color;
    };
    std::deque<Line> lines;
    int shown;                  // the lines update() has counted
    int width;                  // the longest of them, + 1
    std::string highlight;      // lower case
    int current;                // the line of showLine(), -1

    // the column of the text in line i at or after `from`, -1
    int matchAt( int i, int from );

};

#endif  // Uses_TLogView
