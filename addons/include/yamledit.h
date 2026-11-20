/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   YAMLEDIT.H                                                            */
/*                                                                         */
/*   defines the classes TYamlEditor and TYamlEditWindow                   */
/*                                                                         */
/*   TYamlEditor is a TAdvancedEditor that colours YAML: keys, quoted      */
/*   strings, numbers and true/false/null, comments, punctuation (- : [ ]  */
/*   { } , | >), anchors, aliases and tags, document markers, and the     */
/*   text of block scalars (key: | ...).  The text, editing, files and     */
/*   streaming are those of TFileEditor, the Ctrl+C/X/V/A keys those of    */
/*   TAdvancedEditor (advedit.h).                                          */
/*                                                                         */
/*   TYamlEditWindow is a TEditWindow whose editor is a TYamlEditor.       */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TYamlEditor or                   */
/*   Uses_TYamlEditWindow.                                                 */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if !defined( __YAMLSYNTAX )
#define __YAMLSYNTAX

// what a character of a YAML line is
enum YamlToken
{
    ytPlain,        // plain value text, blanks
    ytKey,          // mapping key
    ytString,       // "double" or 'single' quoted scalar
    ytConstant,     // number, true / false / yes / no / on / off, null, ~
    ytComment,      // # to the end of the line
    ytPunct,        // - : ? [ ] { } , and the | > block indicators
    ytAnchor,       // &anchor, *alias, !tag
    ytDocument,     // --- and ...
    ytBlock,        // a line of a block scalar
    ytCount
};

// Classifies the n characters of one line (no line end); cls gets n
// entries.  inBlock: the line belongs to a block scalar.
void yamlScanLine( const char *s, int n, Boolean inBlock, uchar *cls );

// True if the line ends with a block scalar indicator: "key: |",
// "- >-", "run: |2  # comment".
Boolean yamlOpensBlock( const char *s, int n );

#endif  // __YAMLSYNTAX


#if defined( Uses_TYamlEditor ) && !defined( __TYamlEditor )
#define __TYamlEditor

class TRect;
class TScrollBar;
class TIndicator;

class TYamlEditor : public TAdvancedEditor
{

public:

    TYamlEditor( const TRect&, TScrollBar *, TScrollBar *, TIndicator *,
                 const char *aFileName );

    virtual void drawLines( int y, int count, ushort linePtr );

    // False shows the text without colours.  The colours are used only
    // with the colour palette (not with TVMONO or a black and white
    // terminal).
    Boolean highlight;

    // Foreground colour of each YamlToken, on the editor's own background;
    // 0xFF leaves the normal text colour.  darkColors is used on a black,
    // blue, red or magenta background, lightColors on the others.
    static uchar darkColors[ytCount];
    static uchar lightColors[ytCount];

    // Whether the line starting at linePtr is the text of a block scalar.
    Boolean inBlockScalar( ushort linePtr );

private:

    int lineText( ushort p, char *buf, int size );

    virtual const char *streamableName() const
        { return name; }

protected:

    TYamlEditor( StreamableInit );

public:

    static const char * const name;
    static TStreamable *build();

};

inline ipstream& operator >> ( ipstream& is, TYamlEditor& cl )
    { return is >> (TStreamable&)cl; }
inline ipstream& operator >> ( ipstream& is, TYamlEditor*& cl )
    { return is >> (void *&)cl; }

inline opstream& operator << ( opstream& os, TYamlEditor& cl )
    { return os << (TStreamable&)cl; }
inline opstream& operator << ( opstream& os, TYamlEditor* cl )
    { return os << (TStreamable *)cl; }

#endif  // Uses_TYamlEditor


#if defined( Uses_TYamlEditWindow ) && !defined( __TYamlEditWindow )
#define __TYamlEditWindow

class TYamlEditWindow : public TEditWindow
{

public:

    TYamlEditWindow( const TRect& bounds, const char *fileName, int aNumber );

    // editor, as what it is
    TYamlEditor *yamlEditor() const
        { return (TYamlEditor *) editor; }

private:

    virtual const char *streamableName() const
        { return name; }

protected:

    TYamlEditWindow( StreamableInit );

public:

    static const char * const name;
    static TStreamable *build();

};

inline ipstream& operator >> ( ipstream& is, TYamlEditWindow& cl )
    { return is >> (TStreamable&)cl; }
inline ipstream& operator >> ( ipstream& is, TYamlEditWindow*& cl )
    { return is >> (void *&)cl; }

inline opstream& operator << ( opstream& os, TYamlEditWindow& cl )
    { return os << (TStreamable&)cl; }
inline opstream& operator << ( opstream& os, TYamlEditWindow* cl )
    { return os << (TStreamable *)cl; }

#endif  // Uses_TYamlEditWindow
