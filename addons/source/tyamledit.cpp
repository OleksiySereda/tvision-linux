/*------------------------------------------------------------*/
/* filename -       tyamledit.cpp                             */
/*                                                            */
/* function(s)                                                */
/*                  TYamlEditor and TYamlEditWindow member    */
/*                  functions, their stream registration      */
/*                                                            */
/*   TColoredEditor draws the lines; scanLine() gives it the  */
/*   YAML token of each character, classColor() its colour.  */
/*------------------------------------------------------------*/

#define Uses_TYamlEditor
#define Uses_TYamlEditWindow
#define Uses_TBlockIndicator
#define Uses_TProgram
#define Uses_TApplication
#define Uses_TStreamableClass
#define Uses_opstream
#define Uses_ipstream
#include <tvaddons.h>

// how many lines up inBlockScalar() looks for the "key: |" line
const int maxBlockSearch = 2000;

//                                   plain key   string const comm  punct anchor doc   block
uchar TYamlEditor::darkColors[ytCount]  = { 0xFF, 0x0F, 0x0A, 0x0D, 0x07, 0x0B, 0x0C, 0x0F, 0x0A };
uchar TYamlEditor::lightColors[ytCount] = { 0xFF, 0x01, 0x02, 0x05, 0x08, 0x00, 0x04, 0x01, 0x02 };

TYamlEditor::TYamlEditor( const TRect& bounds,
                          TScrollBar *aHScrollBar,
                          TScrollBar *aVScrollBar,
                          TIndicator *aIndicator,
                          const char *aFileName ) :
    TColoredEditor( bounds, aHScrollBar, aVScrollBar, aIndicator, aFileName )
{
}

TYamlEditor::TYamlEditor( StreamableInit ) :
    TColoredEditor( streamableInit )
{
}

static int indentOf( const char *s, int n )
{
    int i = 0;
    while( i < n && s[i] == ' ' )
        i++;
    return i;
}

static Boolean blankLine( const char *s, int n )
{
    for( int i = 0; i < n; i++ )
        if( s[i] != ' ' && s[i] != '\t' )
            return False;
    return True;
}

// the column where the content of a line starts, after "- " items: the
// keys of "  - run: x" are at column 4
static int contentIndent( const char *s, int n )
{
    int i = indentOf( s, n );
    while( i + 1 < n && s[i] == '-' && s[i+1] == ' ' )
        {
        i++;
        while( i < n && s[i] == ' ' )
            i++;
        }
    return i;
}

// A line is block scalar text if, going up, the first line that is less
// indented than everything in between ends with | or >.  The lines in
// between may be indented in any way, as block text can be.
Boolean TYamlEditor::inBlockScalar( ushort linePtr )
{
    char buf[maxScanLength];
    int n = lineText( linePtr, buf, maxScanLength );
    if( blankLine( buf, n ) )
        return False;
    int minIndent = indentOf( buf, n );
    if( minIndent == 0 )
        return False;

    ushort p = linePtr;
    for( int guard = 0; p > 0 && guard < maxBlockSearch; guard++ )
        {
        p = prevLine( p );
        n = lineText( p, buf, maxScanLength );
        if( blankLine( buf, n ) )
            continue;
        int ind = contentIndent( buf, n );
        if( ind >= minIndent )
            continue;
        if( yamlOpensBlock( buf, n ) )
            return True;
        if( indentOf( buf, n ) == 0 )
            return False;
        minIndent = ind;
        }
    return False;
}

void TYamlEditor::scanLine( ushort linePtr, const char *s, int n, uchar *cls )
{
    yamlScanLine( s, n, inBlockScalar( linePtr ), cls );
}

uchar TYamlEditor::classColor( uchar cls, uchar normal )
{
    uchar bg = uchar( ( normal >> 4 ) & 0x07 );
    const uchar *fg = ( bg == 2 || bg == 3 || bg == 6 || bg == 7 ) ? lightColors : darkColors;
    uchar f = fg[cls];
    if( f == 0xFF || f == bg )
        return normal;
    return uchar( ( normal & 0xF0 ) | f );
}

const char * const TYamlEditor::name = "TYamlEditor";

TStreamable *TYamlEditor::build()
{
    return new TYamlEditor( streamableInit );
}

TStreamableClass RYamlEditor( TYamlEditor::name,
                              TYamlEditor::build,
                              __DELTA(TYamlEditor)
                            );


// The window is built by TEditWindow with an empty TFileEditor, which
// is then replaced by a TYamlEditor on the same scroll bars; the
// TIndicator is replaced by a TBlockIndicator in the same place.
TYamlEditWindow::TYamlEditWindow( const TRect& bounds,
                                  const char *fileName,
                                  int aNumber ) :
    TWindowInit( &TYamlEditWindow::initFrame ),
    TEditWindow( bounds, 0, aNumber )
{
    TFileEditor *plain = editor;
    TRect r = plain->getBounds();
    TScrollBar *h = plain->hScrollBar;
    TScrollBar *v = plain->vScrollBar;
    TIndicator *plainInd = plain->indicator;
    remove( plain );
    destroy( plain );

    // the indicator that shows Ctrl+B selecting
    TIndicator *ind = new TBlockIndicator( plainInd->getBounds() );
    remove( plainInd );
    destroy( plainInd );
    insert( ind );

    editor = new TYamlEditor( r, h, v, ind, fileName );
    insert( editor );
}

TYamlEditWindow::TYamlEditWindow( StreamableInit ) :
    TWindowInit( streamableInit ),
    TEditWindow( streamableInit )
{
}

const char * const TYamlEditWindow::name = "TYamlEditWindow";

TStreamable *TYamlEditWindow::build()
{
    return new TYamlEditWindow( streamableInit );
}

TStreamableClass RYamlEditWindow( TYamlEditWindow::name,
                                  TYamlEditWindow::build,
                                  __DELTA(TYamlEditWindow)
                                );
