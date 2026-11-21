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
#define Uses_TEditorDialogs
#define Uses_TStreamableClass
#define Uses_opstream
#define Uses_ipstream
#include <tvaddons.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

TAdvancedEditor::TAdvancedEditor( const TRect& bounds,
                                  TScrollBar *aHScrollBar,
                                  TScrollBar *aVScrollBar,
                                  TIndicator *aIndicator,
                                  const char *aFileName ) :
    TFileEditor( bounds, aHScrollBar, aVScrollBar, aIndicator, aFileName ),
    clipboardKeys( True ),
    wheelLines( 3 ),
    lineNumbers( 0 ),
    matchBrackets( True ),
    searching( False ),
    searchFound( True ),
    searchOrigin( 0 ),
    shownAt( 0xFFFF ),
    shownPair( 0xFFFF )
{
    eventMask |= evMouseWheel;
}

// eventMask, with evMouseWheel, is read from the stream by TView::read
TAdvancedEditor::TAdvancedEditor( StreamableInit ) :
    TFileEditor( streamableInit ),
    clipboardKeys( True ),
    wheelLines( 3 ),
    lineNumbers( 0 ),
    matchBrackets( True ),
    searching( False ),
    searchFound( True ),
    searchOrigin( 0 ),
    shownAt( 0xFFFF ),
    shownPair( 0xFFFF )
{
}

// every scroll and every change of the number of lines redraws the whole
// editor, so the numbers are redrawn with it
void TAdvancedEditor::draw()
{
    // the brackets drawLines() paints now; handleEvent() redraws when
    // they change
    if( !bracketPair( shownAt, shownPair ) )
        shownAt = shownPair = 0xFFFF;
    TFileEditor::draw();
    if( lineNumbers != 0 )
        lineNumbers->drawView();
}

void TAdvancedEditor::setState( ushort aState, Boolean enable )
{
    TFileEditor::setState( aState, enable );
    if( ( aState & sfFocused ) != 0 )
        {
        if( enable == False && searching == True )
            endIncSearch( True );
        if( matchBrackets == True )
            drawView();
        }
}

void TAdvancedEditor::drawLines( int y, int count, ushort linePtr )
{
    TFileEditor::drawLines( y, count, linePtr );
    paintBrackets( y, count, linePtr );
}

// ---- brackets

uchar TAdvancedEditor::bracketColor = 0x30;     // black on cyan

Boolean TAdvancedEditor::matchBracket( ushort pos, ushort& pair )
{
    static const char brackets[] = "()[]{}";
    if( pos >= bufLen )
        return False;
    char c = bufChar( pos );
    const char *p = c != 0 ? strchr( brackets, c ) : 0;
    if( p == 0 )
        return False;
    int k = int( p - brackets );
    char open = brackets[k & ~1], close = brackets[k | 1];
    int depth = 0;
    if( ( k & 1 ) == 0 )
        {
        for( ushort i = pos; i < bufLen; i++ )
            {
            char d = bufChar( i );
            if( d == open )
                depth++;
            else if( d == close && --depth == 0 )
                {
                pair = i;
                return True;
                }
            }
        }
    else
        {
        for( int i = pos; i >= 0; i-- )
            {
            char d = bufChar( ushort( i ) );
            if( d == close )
                depth++;
            else if( d == open && --depth == 0 )
                {
                pair = ushort( i );
                return True;
                }
            }
        }
    return False;
}

Boolean TAdvancedEditor::bracketPair( ushort& at, ushort& pair )
{
    if( matchBrackets == False || ( state & sfFocused ) == 0 )
        return False;
    if( matchBracket( curPtr, pair ) )
        {
        at = curPtr;
        return True;
        }
    if( curPtr > 0 && matchBracket( ushort( curPtr - 1 ), pair ) )
        {
        at = ushort( curPtr - 1 );
        return True;
        }
    return False;
}

void TAdvancedEditor::paintBrackets( int y, int count, ushort linePtr )
{
    ushort at, pair;
    if( !bracketPair( at, pair ) )
        return;
    for( ; count > 0 && linePtr < bufLen; count--, y++ )
        {
        ushort next = nextLine( linePtr );
        ushort both[2] = { at, pair };
        for( int k = 0; k < 2; k++ )
            if( both[k] >= linePtr && both[k] < next )
                {
                int x = charPos( linePtr, both[k] ) - delta.x;
                if( x >= 0 && x < size.x )
                    {
                    TDrawBuffer b;
                    b.moveChar( 0, bufChar( both[k] ), bracketColor, 1 );
                    writeLine( short( x ), short( y ), 1, 1, b );
                    }
                }
        if( next == linePtr )
            break;
        linePtr = next;
        }
}

// ---- go to line

int TAdvancedEditor::cursorLine()
{
    return curPos.y + 1;
}

void TAdvancedEditor::gotoLine( int line )
{
    ushort p = 0;
    for( int i = 1; i < line; i++ )
        {
        ushort next = nextLine( p );
        if( next >= bufLen && ( bufLen == 0 || bufChar( ushort( bufLen - 1 ) ) != '\n' ) )
            break;                      // p starts the last line
        if( next == p )
            break;
        p = next;
        }
    lock();
    setCurPtr( p, 0 );
    trackCursor( True );
    unlock();
}

void TAdvancedEditor::askGotoLine()
{
    char number[gotoLineLen];
    snprintf( number, sizeof( number ), "%u", unsigned( ushort( cursorLine() ) ) );
    if( editorDialog( edGotoLine, number ) != cmCancel )
        {
        int line = atoi( number );
        if( line > 0 )
            gotoLine( line );
        }
}

// ---- incremental search

void TAdvancedEditor::showSearch()
{
    TBlockIndicator *b = dynamic_cast<TBlockIndicator *>( indicator );
    if( b != 0 )
        b->setSearch( searching == True ? searchText : 0, searchFound );
}

void TAdvancedEditor::startIncSearch()
{
    searching = True;
    searchFound = True;
    searchText[0] = 0;
    searchOrigin = curPtr;
    showSearch();
}

void TAdvancedEditor::endIncSearch( Boolean keep )
{
    if( searching == False )
        return;
    searching = False;
    if( searchText[0] != 0 )
        {
        strcpy( findStr, searchText );
        editorFlags &= ~( efCaseSensitive | efWholeWordsOnly | efReplaceAll | efDoReplace );
        }
    if( keep == False )
        {
        lock();
        setCurPtr( searchOrigin, 0 );
        trackCursor( Boolean( !cursorVisible() ) );
        unlock();
        }
    showSearch();
}

// searches searchText from pos; when it is not there, the selection and
// the cursor stay as they were
Boolean TAdvancedEditor::searchFrom( ushort pos )
{
    ushort oldStart = selStart, oldEnd = selEnd, oldCur = curPtr;
    lock();
    setCurPtr( pos, 0 );
    Boolean found = search( searchText, 0 );
    if( found == False )
        {
        if( oldStart == oldEnd )
            setSelect( oldCur, oldCur, False );
        else
            setSelect( oldStart, oldEnd, Boolean( oldCur == oldStart ) );
        }
    unlock();
    return found;
}

// a key while searching: True if it belonged to the search
Boolean TAdvancedEditor::incSearchKey( TEvent& event )
{
    uchar c = event.keyDown.charScan.charCode;
    ushort code = event.keyDown.keyCode;
    if( c == kbCtrlF || event.what == evCommand )
        {
        if( searchText[0] == 0 )
            {
            // Ctrl+F twice: the text searched last
            strncpy( searchText, findStr, maxFindStrLen - 1 );
            searchText[maxFindStrLen - 1] = 0;
            if( searchText[0] != 0 )
                searchFound = searchFrom( searchOrigin );
            }
        else
            {
            searchFound = searchFrom( curPtr );
            if( searchFound == False )
                searchFound = searchFrom( 0 );      // wraps round
            }
        }
    else if( code == kbEnter || c == 0x0D )
        endIncSearch( True );
    else if( code == kbEsc || c == 0x1B )
        endIncSearch( False );
    else if( code == kbBack || c == 0x08 )
        {
        size_t n = strlen( searchText );
        if( n > 0 )
            searchText[n - 1] = 0;
        if( searchText[0] == 0 )
            {
            lock();
            setCurPtr( searchOrigin, 0 );
            unlock();
            searchFound = True;
            }
        else
            searchFound = searchFrom( searchOrigin );
        }
    else if( c >= ' ' && c != 0x7F )
        {
        size_t n = strlen( searchText );
        if( n + 1 < sizeof( searchText ) )
            {
            searchText[n] = char( c );
            searchText[n + 1] = 0;
            }
        searchFound = searchFrom( searchOrigin );
        }
    else
        {
        endIncSearch( True );
        return False;                   // the key does what it always does
        }
    showSearch();
    return True;
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

    if( searching == True )
        {
        if( event.what == evKeyDown ||
            ( event.what == evCommand && event.message.command == cmIncSearch ) )
            {
            if( incSearchKey( event ) )
                {
                clearEvent( event );
                return;
                }
            }
        else if( event.what == evMouseDown )
            endIncSearch( True );
        }

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
                case kbCtrlG: command = cmGotoLine;  break;
                case kbCtrlF: command = cmIncSearch; break;
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
            case cmGotoLine:
                clearEvent( event );
                askGotoLine();
                return;
            case cmIncSearch:
                clearEvent( event );
                startIncSearch();
                return;
            case cmCopy:
                selecting = False;      // cut and paste end it themselves
                break;
            }

    TFileEditor::handleEvent( event );
    if( selecting != wasSelecting )
        showSelecting();

    // TEditor redraws only the line of the cursor when the cursor moves;
    // the bracket pair shown before may be on another one
    ushort at = 0xFFFF, pair = 0xFFFF;
    if( !bracketPair( at, pair ) )
        at = pair = 0xFFFF;
    if( at != shownAt || pair != shownPair )
        drawView();
}

// TBlockIndicator

TBlockIndicator::TBlockIndicator( const TRect& bounds ) :
    TIndicator( bounds ),
    block( False ),
    searching( False ),
    searchFound( True )
{
    search[0] = 0;
}

TBlockIndicator::TBlockIndicator( StreamableInit ) :
    TIndicator( streamableInit ),
    block( False ),
    searching( False ),
    searchFound( True )
{
    search[0] = 0;
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
void TBlockIndicator::setSearch( const char *text, Boolean found )
{
    searching = Boolean( text != 0 );
    searchFound = found;
    if( text != 0 )
        {
        strncpy( search, text, sizeof( search ) - 1 );
        search[sizeof( search ) - 1] = 0;
        }
    drawView();
}

void TBlockIndicator::draw()
{
    if( searching == True )
        {
        // the end of the text when it is longer than the indicator
        ushort color = getColor( 1 );
        TDrawBuffer b;
        b.moveChar( 0, ' ', color, size.x );
        int n = int( strlen( search ) ), room = size.x - 2;
        const char *t = n > room && room > 0 ? search + n - room : search;
        char s[sizeof( search ) + 2];
        snprintf( s, sizeof( s ), "%c%s", searchFound == True ? '/' : '!', t );
        b.moveStr( 1, s, color );
        writeLine( 0, 0, size.x, 1, b );
        return;
        }
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
