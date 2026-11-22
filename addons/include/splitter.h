/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   SPLITTER.H                                                            */
/*                                                                         */
/*   defines the class TSplitter                                           */
/*                                                                         */
/*   A bar between two parts of a window that the mouse drags:            */
/*                                                                         */
/*       TSplitter *s = new TSplitter( TRect( 30, 1, 31, h - 1 ), True );  */
/*       s->addFirst( tree );                  // left: grows and shrinks  */
/*       s->addFirst( treeScrollBar, TSplitter::smMove );   // moves along */
/*       s->addSecond( text );                 // right                    */
/*       insert( s );                                                      */
/*       ...                                                               */
/*       s->restore( settings, "main" );       // where it was last time   */
/*                                                                         */
/*   - vertical: the bar is one column, the first views are left of it;   */
/*     horizontal: one line, the first views are above it;                 */
/*   - a view of a side is resized (its edge at the bar follows it) or,    */
/*     with smMove, moved (a scroll bar next to the bar);                  */
/*   - minFirst and minSecond (3) keep the parts from vanishing;           */
/*   - setPosition() moves it from the program; save() and restore()       */
/*     keep the position in TAppSettings ([splitter.<name>]);              */
/*   - a horizontal bar may have a title on it (" Log "); doubleLine draws */
/*     it as a frame does; normalColor and dragColor are the indexes of    */
/*     its colours in the owner's palette (1 and 2: a window's passive and */
/*     active frame);                                                      */
/*   - replace() puts a new view in the place of one that goes (an editor  */
/*     opened again);                                                      */
/*   - after it has moved it broadcasts cmSplitterMoved to its owner       */
/*     (infoPtr: the splitter).                                            */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TSplitter.                       */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TSplitter ) && !defined( __TSplitter )
#define __TSplitter

#include <string>
#include <vector>

class TRect;
class TAppSettings;
struct TEvent;

const ushort cmSplitterMoved = 546;

class TSplitter : public TView
{

public:

    enum { smResize, smMove };

    TSplitter( const TRect& bounds, Boolean aVertical );

    void addFirst( TView *view, int mode = smResize );
    void addSecond( TView *view, int mode = smResize );
    // the new view moves with the bar as the old one did
    void replace( TView *oldView, TView *newView );
    void remove( TView *view );

    // the column (vertical) or line of the bar in its owner
    int position() const
        { return vertical == True ? origin.x : origin.y; }
    // moves the bar there (kept between the minimums) and the views
    void setPosition( int pos );

    void save( TAppSettings& settings, const char *name );
    void restore( TAppSettings& settings, const char *name );

    virtual void draw();
    virtual void handleEvent( TEvent& event );
    virtual void shutDown();

    Boolean vertical;
    int minFirst, minSecond;

    std::string title;                  // on a horizontal bar, at titleX
    int titleX;
    Boolean doubleLine;
    uchar normalColor, dragColor;

private:

    struct Part
    {
        TView *view;
        int mode;
        Boolean first;
    };
    std::vector<Part> parts;
    Boolean dragging;
};

#endif  // Uses_TSplitter
