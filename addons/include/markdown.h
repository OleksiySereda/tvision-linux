/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   MARKDOWN.H                                                            */
/*                                                                         */
/*   defines the classes TMarkdownView and TMarkdownWindow                 */
/*                                                                         */
/*   Shows a Markdown text (a README, a help page) laid out for the        */
/*   width of the view:                                                    */
/*                                                                         */
/*       TMarkdownWindow *w = new TMarkdownWindow( r, "README.md" );       */
/*       deskTop->insert( validView( w ) );                                */
/*                                                                         */
/*   - headings (#, ##, ...) in colours of their own, the first two        */
/*     underlined; paragraphs wrapped at the width of the view;           */
/*   - **bold**, *emphasis*, `code`, [links](#a-heading);                  */
/*   - lists (-, *, +, 1.), also nested; > quotes; --- lines;              */
/*   - ``` code blocks as they are (they scroll sideways);                 */
/*   - | tables | with a line under the header;                            */
/*   - Tab and Shift+Tab go from link to link, Enter or a click follows    */
/*     one: #name goes to the heading of that name (as GitHub makes        */
/*     them: "Building on Windows" is #building-on-windows); another       */
/*     target is broadcast as cmMarkdownLink (infoPtr: the target, a       */
/*     const char *) for the program;                                      */
/*   - the text is UTF-8 and is shown in the code page of the screen.      */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TMarkdownView or                 */
/*   Uses_TMarkdownWindow.                                                 */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TMarkdownView ) && !defined( __TMarkdownView )
#define __TMarkdownView

#include <string>
#include <vector>

class TRect;
class TScrollBar;
struct TEvent;

const ushort cmMarkdownLink = 545;

class TMarkdownView : public TScroller
{

public:

    enum Style
    {
        msNormal, msHeading1, msHeading2, msHeading3, msBold, msEmphasis,
        msCode, msCodeBlock, msQuote, msLink, msRule, msTable
    };

    TMarkdownView( const TRect& bounds, TScrollBar *aHScrollBar, TScrollBar *aVScrollBar );

    void setText( const std::string& markdown );
    // False if the file cannot be read
    Boolean loadFile( const char *fileName );

    // the first column and line shown
    TPoint position() const
        { return delta; }

    // scrolls to the heading with that anchor ("#building" or "building")
    Boolean gotoAnchor( const char *anchor );
    // the anchor GitHub gives a heading
    static std::string anchorOf( const std::string& heading );

    // the colour of a style, from the view's normal colour
    virtual uchar styleColor( int style, uchar normal );

    virtual void draw();
    virtual void handleEvent( TEvent& event );
    virtual void changeBounds( const TRect& bounds );
    virtual void scrollDraw();

    // what was laid out: lines of text in the screen's code page and a
    // style for each character
    int lineCount() const
        { return int( lines.size() ); }
    const std::string& lineText( int i ) const
        { return lines[size_t( i )].text; }
    int linkCount() const
        { return int( links.size() ); }
    const std::string& linkTarget( int i ) const
        { return links[size_t( i )].target; }
    int linkLine( int i ) const
        { return links[size_t( i )].line; }
    int focusedLink;                    // -1: none

    // moves to the next (previous) link and shows it
    void nextLink( Boolean forwards );
    void followLink( int i );

    int wheelLines;

private:

    struct Line
    {
        std::string text;
        std::vector<uchar> style;
    };
    struct Link
    {
        int line, x, length;
        std::string target;
    };
    struct Block
    {
        int kind, level, number;        // number: of an ordered list item
        std::vector<std::string> lines;
    };

    void parse();
    void layout();
    void addBlock( const Block& b, int width );
    void addWrapped( const std::string& text, int width, int indent, const std::string& first,
                     uchar baseStyle, int quote );
    void addTable( const Block& b, int width );
    void scrollToPos( int x, int y );
    int linkAt( int line, int x ) const;

    std::string source;
    std::vector<Block> blocks;
    std::vector<Line> lines;
    std::vector<Link> links;
    std::vector<std::pair<std::string, int> > anchors;
    int laidOutWidth;
};

#endif  // Uses_TMarkdownView


#if defined( Uses_TMarkdownWindow ) && !defined( __TMarkdownWindow )
#define __TMarkdownWindow

class TRect;
class TMarkdownView;

class TMarkdownWindow : public TWindow
{

public:

    // the file's name is the title
    TMarkdownWindow( const TRect& bounds, const char *fileName );

    TMarkdownView *view;
};

#endif  // Uses_TMarkdownWindow
