/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   TREEVIEW.H                                                            */
/*                                                                         */
/*   defines the classes TTreeNode and TTreeView                           */
/*                                                                         */
/*   A tree of lines that open and close (Turbo Vision 2.0 has TOutline;   */
/*   1.0 has nothing of the kind):                                         */
/*                                                                         */
/*       tree = new TTreeView( TRect( 1, 1, 39, 15 ), 0, vScrollBar );     */
/*       TTreeNode *web = tree->addNode( 0, "web" );                       */
/*       tree->addNode( web, "web1" );                                     */
/*       tree->addNode( web, "web2" );                                     */
/*       tree->addNode( 0, "db" );                                         */
/*                                                                         */
/*       - web                  (new nodes are open; a node without      */
/*         ├── web1              children has no + or -)                   */
/*         └── web2                                                        */
/*         db                                                              */
/*                                                                         */
/*   Keys: the arrows, PgUp/PgDn, Home/End move; Right or '+' opens a      */
/*   node (and goes into it when it is open), Left or '-' closes it (or    */
/*   goes to its parent), '*' opens all of it; a click on + / - opens or   */
/*   closes.  Enter and a double click broadcast cmListItemSelected (and   */
/*   Enter goes on to the default button of the dialog).                   */
/*                                                                         */
/*   With markable, each line has a check box: Space (or a click on it)    */
/*   marks a node with all its children; a node whose children are only    */
/*   partly marked shows [-].                                              */
/*                                                                         */
/*   The view owns the nodes.  Put it into a dialog, as a TListBox (the    */
/*   colours are the list viewer's).                                       */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TTreeView.                       */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TTreeView ) && !defined( __TTreeView )
#define __TTreeView

#include <string>
#include <vector>

class TRect;
class TScrollBar;
struct TEvent;

struct TTreeNode
{
    std::string text;
    void *data;                         // the program's
    TTreeNode *parent;
    std::vector<TTreeNode *> children;
    Boolean expanded;
    Boolean marked;                     // of a leaf; see TTreeView::markOf()

    int depth() const;
};

class TTreeView : public TListViewer
{

public:

    enum { mkNone, mkAll, mkSome };

    TTreeView( const TRect& bounds, TScrollBar *aHScrollBar, TScrollBar *aVScrollBar );
    ~TTreeView();

    // parent 0: a node at the top
    TTreeNode *addNode( TTreeNode *parent, const char *text, void *data = 0 );
    void removeNode( TTreeNode *node );
    void clear();
    const std::vector<TTreeNode *>& roots() const
        { return top; }

    void expand( TTreeNode *node, Boolean open );
    void expandAll( TTreeNode *node, Boolean open );    // node 0: the whole tree

    // the lines on the screen: the nodes whose parents are all open
    int visibleCount() const
        { return int( lines.size() ); }
    TTreeNode *nodeAt( int item ) const;
    int itemOf( TTreeNode *node ) const;        // -1 if it is hidden
    TTreeNode *focusedNode() const
        { return nodeAt( focused ); }
    // opens its parents and moves the focus to it
    void focusNode( TTreeNode *node );

    // marks
    Boolean markable;
    void setMark( TTreeNode *node, Boolean on );        // with all its children
    int markOf( TTreeNode *node ) const;                // mkNone, mkAll or mkSome
    // the marked leaves (nodes without children), in tree order
    std::vector<TTreeNode *> markedLeaves() const;

    // the line of an item as drawn: lines, + or -, the check box, text
    std::string formatLine( int item ) const;

    virtual void draw();
    virtual void handleEvent( TEvent& event );
    virtual void getText( char *dest, short item, short maxLen );
    virtual void changeBounds( const TRect& bounds );

private:

    void rebuild( TTreeNode *keep );
    void addLines( TTreeNode *n );
    void destroyNode( TTreeNode *n );
    void markTree( TTreeNode *n, Boolean on );
    void collect( TTreeNode *n, std::vector<TTreeNode *>& v ) const;
    int prefixWidth( int item ) const;          // up to the text

    std::vector<TTreeNode *> top;
    std::vector<TTreeNode *> lines;
};

#endif  // Uses_TTreeView
