/*------------------------------------------------------------*/
/* filename -       tyamledit.cpp                             */
/*                                                            */
/* function(s)                                                */
/*                  TYamlEditor and TYamlEditWindow member    */
/*                  functions, their stream registration      */
/*                                                            */
/*   TYamlEditor::drawLines() lets TEditor::formatLine() lay  */
/*   out the line (tabs, selection, horizontal scrolling) and */
/*   then gives the cells in the normal text colour the       */
/*   colour of their YAML token.  Selected text keeps the     */
/*   selection colour.                                        */
/*------------------------------------------------------------*/

#define Uses_TYamlEditor
#define Uses_TYamlEditWindow
#define Uses_TProgram
#define Uses_TApplication
#define Uses_TStreamableClass
#define Uses_opstream
#define Uses_ipstream
#include <tvaddons.h>

// longest part of a line that is classified; the rest keeps its colour
const int maxScanLength = 1024;
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
    TAdvancedEditor( bounds, aHScrollBar, aVScrollBar, aIndicator, aFileName ),
    highlight( True )
{
}

TYamlEditor::TYamlEditor( StreamableInit ) :
    TAdvancedEditor( streamableInit ),
    highlight( True )
{
}

// the text of the line at p, without the line end
int TYamlEditor::lineText( ushort p, char *buf, int size )
{
    int n = 0;
    for( ; p < bufLen && n < size; p++ )
        {
        char c = bufChar( p );
        if( c == '\r' || c == '\n' )
            break;
        buf[n++] = c;
        }
    return n;
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

void TYamlEditor::drawLines( int y, int count, ushort linePtr )
{
    ushort color = getColor( 0x0201 );
    uchar normal = uchar( color );
    Boolean colours = Boolean( highlight && TProgram::appPalette == apColor );
    uchar bg = uchar( ( normal >> 4 ) & 0x07 );
    const uchar *fg = ( bg == 2 || bg == 3 || bg == 6 || bg == 7 ) ? lightColors : darkColors;

    while( count-- > 0 )
        {
        ushort b[maxLineLength + maxViewWidth];     // as in TEditor::drawLines
        int width = delta.x + size.x;
        formatLine( b, linePtr, width, color );

        if( colours )
            {
            char text[maxScanLength];
            uchar cls[maxScanLength];
            int n = lineText( linePtr, text, maxScanLength );
            yamlScanLine( text, n, inBlockScalar( linePtr ), cls );

            // the same columns as formatLine(): tabs to multiples of 8
            int x = 0;
            for( int i = 0; i < n && x < width; i++ )
                {
                int cells = text[i] == '\t' ? 8 - ( x & 7 ) : 1;
                uchar f = fg[cls[i]];
                for( ; cells > 0 && x < width; cells--, x++ )
                    {
                    uchar attr = uchar( b[x] >> 8 );
                    if( attr == normal && f != 0xFF && f != bg )
                        b[x] = ushort( ( b[x] & 0x00FF ) | ( ( ( normal & 0xF0 ) | f ) << 8 ) );
                    }
                }
            }

        writeBuf( 0, y, size.x, 1, &b[delta.x] );
        linePtr = nextLine( linePtr );
        y++;
        }
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
// is then replaced by a TYamlEditor on the same scroll bars and indicator.
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
    TIndicator *ind = plain->indicator;
    remove( plain );
    destroy( plain );

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
