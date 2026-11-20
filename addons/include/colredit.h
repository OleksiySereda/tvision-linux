/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   COLREDIT.H                                                            */
/*                                                                         */
/*   defines the class TColoredEditor                                      */
/*                                                                         */
/*   TColoredEditor is a TAdvancedEditor that colours its text by the      */
/*   classes a derived class gives the characters of each line.  A        */
/*   syntax colouring editor needs only two functions:                     */
/*                                                                         */
/*       scanLine()     the class of each character of a line              */
/*       classColor()   the colour of a class                              */
/*                                                                         */
/*   drawLines() lets TEditor::formatLine() lay out the line (tabs,        */
/*   selection, horizontal scrolling) and then gives the cells in the      */
/*   normal text colour the colour of their class.  Selected text keeps    */
/*   the selection colour.  TYamlEditor (yamledit.h) is one.               */
/*                                                                         */
/*   TColoredEditor itself is not registered for streams: a derived        */
/*   class registers its own name and build().                             */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TColoredEditor.                  */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TColoredEditor ) && !defined( __TColoredEditor )
#define __TColoredEditor

class TRect;
class TScrollBar;
class TIndicator;

class TColoredEditor : public TAdvancedEditor
{

public:

    TColoredEditor( const TRect&, TScrollBar *, TScrollBar *, TIndicator *,
                    const char *aFileName );

    virtual void drawLines( int y, int count, ushort linePtr );

    // Classifies the n characters s of the line starting at linePtr (no
    // line end); cls gets n entries.  Lines longer than maxScanLength
    // are classified up to there.  The default makes all of them 0.
    virtual void scanLine( ushort linePtr, const char *s, int n, uchar *cls );

    // The attribute of a character of class cls, where the normal text
    // attribute is normal; returning normal leaves the character as it is.
    // The default does that for every class.
    virtual uchar classColor( uchar cls, uchar normal );

    // False shows the text without colours.  The colours are used only
    // with the colour palette (not with TVMONO or a black and white
    // terminal).
    Boolean highlight;

    // the text of the line at p without the line end, at most size
    // characters; returns their number
    int lineText( ushort p, char *buf, int size );

    static const int maxScanLength = 1024;

protected:

    TColoredEditor( StreamableInit );

};

#endif  // Uses_TColoredEditor
