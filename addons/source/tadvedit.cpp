/*------------------------------------------------------------*/
/* filename -       tadvedit.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TAdvancedEditor and TBlockIndicator       */
/*                  member functions, their stream            */
/*                  registration                              */
/*                                                            */
/*   handleEvent() turns Ctrl+C, X, V and A into cmCopy,      */
/*   cmCut, cmPaste and cmSelectAll before TEditor sees the   */
/*   key, so TEditor's own key map is left unchanged.  The    */
/*   mouse wheel (evMouseWheel) scrolls the text.             */
/*------------------------------------------------------------*/

#define Uses_TAdvancedEditor
#define Uses_TBlockIndicator
#define Uses_TLineNumbers
#define Uses_TDrawBuffer
#define Uses_TProgram
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TEventQueue
#define Uses_TStreamableClass
#define Uses_opstream
#define Uses_ipstream
#include <tvaddons.h>

#include <stdio.h>

TAdvancedEditor::TAdvancedEditor( const TRect& bounds,
                                  TScrollBar *aHScrollBar,
                                  TScrollBar *aVScrollBar,
                                  TIndicator *aIndicator,
                                  const char *aFileName ) :
    TFileEditor( bounds, aHScrollBar, aVScrollBar, aIndicator, aFileName ),
    clipboardKeys( True ),
    wheelLines( 3 ),
    lineNumbers( 0 )
{
    eventMask |= evMouseWheel;
}

// eventMask, with evMouseWheel, is read from the stream by TView::read
TAdvancedEditor::TAdvancedEditor( StreamableInit ) :
    TFileEditor( streamableInit ),
    clipboardKeys( True ),
    wheelLines( 3 ),
    lineNumbers( 0 )
{
}

// every scroll and every change of the number of lines redraws the whole
// editor, so the numbers are redrawn with it
void TAdvancedEditor::draw()
{
    TFileEditor::draw();
    if( lineNumbers != 0 )
        lineNumbers->drawView();
}

void TAdvancedEditor::shutDown()
{
    if( lineNumbers != 0 )
        lineNumbers->editor = 0;
    lineNumbers = 0;
    TFileEditor::shutDown();
}

// TLineNumbers

uchar TLineNumbers::numberColor = 0x7F;     // bright white on light gray

TLineNumbers::TLineNumbers( const TRect& bounds, TAdvancedEditor *anEditor ) :
    TView( bounds ),
    editor( anEditor ),
    markCount( 0 )
{
    growMode = gfGrowHiY;
    if( editor != 0 )
        editor->lineNumbers = this;
}

void TLineNumbers::shutDown()
{
    if( editor != 0 && editor->lineNumbers == this )
        editor->lineNumbers = 0;
    editor = 0;
    TView::shutDown();
}

// the marks are kept unsorted; with more than maxMarks the oldest goes
void TLineNumbers::setMark( int line, uchar color )
{
    int i = 0;
    while( i < markCount && markLine[i] != line )
        i++;
    if( i < markCount )
        {
        if( markColor[i] == color )
            return;
        if( color != 0 )
            markColor[i] = color;
        else
            {
            markCount--;
            for( ; i < markCount; i++ )
                {
                markLine[i] = markLine[i+1];
                markColor[i] = markColor[i+1];
                }
            }
        }
    else
        {
        if( color == 0 )
            return;
        if( markCount == maxMarks )
            {
            markCount--;
            for( int k = 0; k < markCount; k++ )
                {
                markLine[k] = markLine[k+1];
                markColor[k] = markColor[k+1];
                }
            }
        markLine[markCount] = line;
        markColor[markCount] = color;
        markCount++;
        }
    drawView();
}

void TLineNumbers::clearMarks()
{
    if( markCount != 0 )
        {
        markCount = 0;
        drawView();
        }
}

uchar TLineNumbers::markOf( int line ) const
{
    for( int i = 0; i < markCount; i++ )
        if( markLine[i] == line )
            return markColor[i];
    return 0;
}

void TLineNumbers::draw()
{
    ushort blank = editor != 0 ? uchar( editor->getColor( 1 ) ) : uchar( getColor( 1 ) );
    ushort number = numberColor;
    if( TProgram::appPalette != apColor && editor != 0 )
        number = uchar( editor->getColor( 2 ) );        // the selection colour
    for( int y = 0; y < size.y; y++ )
        {
        TDrawBuffer b;
        b.moveChar( 0, ' ', blank, size.x );
        int digits = size.x > 2 ? size.x - 2 : size.x;
        int line = editor != 0 ? editor->delta.y + y + 1 : 0;
        Boolean shown = Boolean( editor != 0 && line <= editor->limit.y );
        ushort c = number;
        if( shown && markOf( line ) != 0 )
            c = markOf( line );
        b.moveChar( 0, ' ', c, digits );
        if( shown )
            {
            char s[16];
            snprintf( s, sizeof( s ), "%*d", digits, line % 10000 );
            b.moveStr( 0, s, c );
            }
        writeLine( 0, y, size.x, 1, b );
        }
}

// the text moves, the cursor and the selection stay where they are
void TAdvancedEditor::wheel( uchar direction )
{
    int dx = 0, dy = 0;
    switch( direction )
        {
        case mwUp:    dy = -wheelLines; break;
        case mwDown:  dy =  wheelLines; break;
        case mwLeft:  dx = -wheelLines; break;
        case mwRight: dx =  wheelLines; break;
        }
    if( dy != 0 && (TEventQueue::getShiftState() & 0x03) != 0 )
        {
        dx = dy;                        // Shift+wheel: sideways
        dy = 0;
        }
    lock();
    scrollTo( delta.x + dx, delta.y + dy );
    unlock();
}

void TAdvancedEditor::selectAll()
{
    lock();
    setSelect( 0, bufLen, False );
    trackCursor( Boolean( !cursorVisible() ) );
    unlock();
}

void TAdvancedEditor::toggleSelect()
{
    if( selecting == True )
        selecting = False;
    else
        startSelect();
    showSelecting();
}

void TAdvancedEditor::showSelecting()
{
    TBlockIndicator *b = dynamic_cast<TBlockIndicator *>( indicator );
    if( b != 0 )
        b->setBlock( selecting );
}

void TAdvancedEditor::handleEvent( TEvent& event )
{
    // the indicator is told only when selecting changes: while the window
    // shuts down, the editor still gets broadcasts after its indicator has
    // been destroyed
    Boolean wasSelecting = selecting;

    // keyState != 0: the second key after Ctrl+K or Ctrl+Q
    if( event.what == evKeyDown && keyState == 0 )
        {
        // by the character only, as TEditor's key map does: the keyboard
        // gives Ctrl+C as 0x2E03, kbCtrlC is 0x0003.  Ctrl+B is free in
        // TEditor, so it does not depend on clipboardKeys.
        ushort command = 0;
        uchar c = event.keyDown.charScan.charCode;
        if( c == kbCtrlB )
            command = cmToggleSelect;
        else if( clipboardKeys == True )
            switch( c )
                {
                case kbCtrlC: command = cmCopy;      break;
                case kbCtrlX: command = cmCut;       break;
                case kbCtrlV: command = cmPaste;     break;
                case kbCtrlA: command = cmSelectAll; break;
                }
        if( command != 0 )
            {
            event.what = evCommand;
            event.message.command = command;
            event.message.infoPtr = 0;
            }
        }

    if( event.what == evMouseWheel )
        {
        wheel( event.mouse.buttons );
        clearEvent( event );
        return;
        }

    if( event.what == evCommand )
        switch( event.message.command )
            {
            case cmSelectAll:
                selectAll();
                clearEvent( event );
                return;
            case cmToggleSelect:
                toggleSelect();
                clearEvent( event );
                return;
            case cmCopy:
                selecting = False;      // cut and paste end it themselves
                break;
            }

    TFileEditor::handleEvent( event );
    if( selecting != wasSelecting )
        showSelecting();
}

// TBlockIndicator

TBlockIndicator::TBlockIndicator( const TRect& bounds ) :
    TIndicator( bounds ),
    block( False )
{
}

TBlockIndicator::TBlockIndicator( StreamableInit ) :
    TIndicator( streamableInit ),
    block( False )
{
}

void TBlockIndicator::setBlock( Boolean on )
{
    if( block != on )
        {
        block = on;
        drawView();
        }
}

// "B" in the column after the modified mark; the position never reaches
// it (" 65535:" starts in column 2)
void TBlockIndicator::draw()
{
    TIndicator::draw();
    if( block == True && size.x > 1 )
        writeChar( 1, 0, 'B', 1, 1 );
}

const char * const TBlockIndicator::name = "TBlockIndicator";

TStreamable *TBlockIndicator::build()
{
    return new TBlockIndicator( streamableInit );
}

TStreamableClass RBlockIndicator( TBlockIndicator::name,
                                  TBlockIndicator::build,
                                  __DELTA(TBlockIndicator)
                                );

const char * const TAdvancedEditor::name = "TAdvancedEditor";

TStreamable *TAdvancedEditor::build()
{
    return new TAdvancedEditor( streamableInit );
}

TStreamableClass RAdvancedEditor( TAdvancedEditor::name,
                                  TAdvancedEditor::build,
                                  __DELTA(TAdvancedEditor)
                                );
