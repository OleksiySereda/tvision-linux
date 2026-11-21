/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   TABLEVIEW.H                                                           */
/*                                                                         */
/*   defines the classes TTableView and TTableHeader                       */
/*                                                                         */
/*   TTableView is a TListViewer of rows with columns of text:            */
/*                                                                         */
/*       table = new TTableView( TRect( 1, 2, 59, 15 ), 0, vScrollBar );   */
/*       table->addColumn( "Name", 16 );                                   */
/*       table->addColumn( "Port", 6, TTableView::alRight );               */
/*       table->addRow( { "web1", "22" } );                                */
/*       insert( table );                                                  */
/*       insert( new TTableHeader( TRect( 1, 1, 59, 2 ), table ) );       */
/*                                                                         */
/*   - TTableHeader shows the titles over the table and follows its        */
/*     sideways scrolling; a click on a title sorts by that column, a      */
/*     second click the other way round (an arrow shows which);            */
/*   - sortBy() sorts from the program; numbers are compared as numbers,   */
/*     other text without regard to case (compare() may be overridden);    */
/*   - with markable, Space and Ins mark or unmark a row (Ins moves on),   */
/*     a click in the first column too; the marked rows show a check       */
/*     mark and the "selected" colour;                                     */
/*   - Enter and a double click broadcast cmListItemSelected, as           */
/*     TListViewer does.                                                   */
/*                                                                         */
/*   Rows are counted in the order they were added (row); the place on     */
/*   the screen, after sorting, is an item (TListViewer's focused,         */
/*   topItem).  rowAt() and itemOf() turn one into the other.             */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TTableView and                   */
/*   Uses_TTableHeader.                                                    */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TTableView ) && !defined( __TTableView )
#define __TTableView

#include <string>
#include <vector>

class TRect;
class TScrollBar;
class TTableHeader;
struct TEvent;

class TTableView : public TListViewer
{

public:

    enum { alLeft, alRight, alCenter };

    TTableView( const TRect& bounds, TScrollBar *aHScrollBar, TScrollBar *aVScrollBar );

    // columns
    void addColumn( const char *title, int width, int align = alLeft );
    int columnCount() const
        { return int( columns.size() ); }
    const char *columnTitle( int col ) const;
    int columnWidth( int col ) const;
    // where the column starts and how wide all of them are, without the
    // sideways scrolling
    int columnX( int col ) const;
    int totalWidth() const;
    // the column at x (in the view), -1 for the mark or a divider
    int columnAt( int x ) const;

    // rows; addRow() returns the new row, cells missing are empty
    int addRow( const std::vector<std::string>& cells );
    void setCell( int row, int col, const char *text );
    const char *cell( int row, int col ) const;
    void removeRow( int row );
    void clear();
    int rowCount() const
        { return int( rows.size() ); }

    int rowAt( int item ) const;        // -1 if none
    int itemOf( int row ) const;        // -1 if none
    int focusedRow() const;             // -1 if the table is empty
    void focusRow( int row );

    // sorting: col -1 is the order the rows were added in; sorting again
    // after a change keeps the order chosen.  The focused row stays.
    void sortBy( int col, Boolean ascending = True );
    void resort();
    int sortColumn;
    Boolean sortAscending;
    // < 0, 0, > 0 like strcmp
    virtual int compare( int rowA, int rowB, int col ) const;

    // marks
    Boolean markable;
    void setMark( int row, Boolean on );
    Boolean isMarked( int row ) const;
    int markedCount() const;
    void markAll( Boolean on );

    virtual void draw();
    virtual void handleEvent( TEvent& event );
    virtual void getText( char *dest, short item, short maxLen );
    virtual Boolean isSelected( short item );
    virtual void changeBounds( const TRect& bounds );
    virtual void shutDown();

    TTableHeader *header;               // set by TTableHeader, else 0

    // the line of an item as it is drawn (before the sideways scrolling):
    // the mark, the cells padded to their widths and the dividers
    std::string formatRow( int item ) const;
    // the titles the same way, with the arrow of the sort column
    std::string formatHeader() const;

private:

    struct Column
    {
        std::string title;
        int width, align;
    };
    std::vector<Column> columns;
    std::vector< std::vector<std::string> > rows;
    std::vector<int> order;             // item -> row
    std::vector<bool> marks;            // by row

    std::string pad( const std::string& s, int width, int align ) const;
    int markWidth() const
        { return markable == True ? 2 : 0; }
    void changed();                     // range, scroll bar, redraw

};

class TTableHeader : public TView
{

public:

    TTableHeader( const TRect& bounds, TTableView *aTable );

    virtual void draw();
    virtual void handleEvent( TEvent& event );
    virtual void shutDown();

private:

    friend class TTableView;            // forgets table when it shuts down
    TTableView *table;
};

#endif  // Uses_TTableView
