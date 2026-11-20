/*------------------------------------------------------------*/
/* filename -       tcolredit.cpp                             */
/*                                                            */
/* function(s)                                                */
/*                  TColoredEditor member functions           */
/*                                                            */
/*   drawLines() lets TEditor::formatLine() lay out the line  */
/*   and then gives the cells in the normal text colour the   */
/*   colour of their class, from scanLine() and classColor(). */
/*------------------------------------------------------------*/

#define Uses_TColoredEditor
#define Uses_TProgram
#define Uses_TApplication
#include <tvaddons.h>

TColoredEditor::TColoredEditor( const TRect& bounds,
                                TScrollBar *aHScrollBar,
                                TScrollBar *aVScrollBar,
                                TIndicator *aIndicator,
                                const char *aFileName ) :
    TAdvancedEditor( bounds, aHScrollBar, aVScrollBar, aIndicator, aFileName ),
    highlight( True )
{
}

TColoredEditor::TColoredEditor( StreamableInit ) :
    TAdvancedEditor( streamableInit ),
    highlight( True )
{
}

int TColoredEditor::lineText( ushort p, char *buf, int size )
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

void TColoredEditor::scanLine( ushort, const char *, int n, uchar *cls )
{
    for( int i = 0; i < n; i++ )
        cls[i] = 0;
}

uchar TColoredEditor::classColor( uchar, uchar normal )
{
    return normal;
}

void TColoredEditor::drawLines( int y, int count, ushort linePtr )
{
    ushort color = getColor( 0x0201 );
    uchar normal = uchar( color );
    Boolean colours = Boolean( highlight && TProgram::appPalette == apColor );

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
            scanLine( linePtr, text, n, cls );

            // the same columns as formatLine(): tabs to multiples of 8
            int x = 0;
            for( int i = 0; i < n && x < width; i++ )
                {
                int cells = text[i] == '\t' ? 8 - ( x & 7 ) : 1;
                uchar a = classColor( cls[i], normal );
                for( ; cells > 0 && x < width; cells--, x++ )
                    if( uchar( b[x] >> 8 ) == normal )      // not selected text
                        b[x] = ushort( ( b[x] & 0x00FF ) | ( a << 8 ) );
                }
            }

        writeBuf( 0, y, size.x, 1, &b[delta.x] );
        linePtr = nextLine( linePtr );
        y++;
        }
}
