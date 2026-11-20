/*------------------------------------------------------------*/
/* filename -       tadvedit.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TAdvancedEditor member functions, its     */
/*                  stream registration                       */
/*                                                            */
/*   handleEvent() turns Ctrl+C, X, V and A into cmCopy,      */
/*   cmCut, cmPaste and cmSelectAll before TEditor sees the   */
/*   key, so TEditor's own key map is left unchanged.  The    */
/*   mouse wheel (evMouseWheel) scrolls the text.             */
/*------------------------------------------------------------*/

#define Uses_TAdvancedEditor
#define Uses_TEvent
#define Uses_TKeys
#define Uses_TEventQueue
#define Uses_TStreamableClass
#define Uses_opstream
#define Uses_ipstream
#include <tvaddons.h>

TAdvancedEditor::TAdvancedEditor( const TRect& bounds,
                                  TScrollBar *aHScrollBar,
                                  TScrollBar *aVScrollBar,
                                  TIndicator *aIndicator,
                                  const char *aFileName ) :
    TFileEditor( bounds, aHScrollBar, aVScrollBar, aIndicator, aFileName ),
    clipboardKeys( True ),
    wheelLines( 3 )
{
    eventMask |= evMouseWheel;
}

// eventMask, with evMouseWheel, is read from the stream by TView::read
TAdvancedEditor::TAdvancedEditor( StreamableInit ) :
    TFileEditor( streamableInit ),
    clipboardKeys( True ),
    wheelLines( 3 )
{
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

void TAdvancedEditor::handleEvent( TEvent& event )
{
    // keyState != 0: the second key after Ctrl+K or Ctrl+Q
    if( event.what == evKeyDown && clipboardKeys == True && keyState == 0 )
        {
        // by the character only, as TEditor's key map does: the keyboard
        // gives Ctrl+C as 0x2E03, kbCtrlC is 0x0003
        ushort command = 0;
        switch( event.keyDown.charScan.charCode )
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

    if( event.what == evCommand && event.message.command == cmSelectAll )
        {
        selectAll();
        clearEvent( event );
        return;
        }

    TFileEditor::handleEvent( event );
}

const char * const TAdvancedEditor::name = "TAdvancedEditor";

TStreamable *TAdvancedEditor::build()
{
    return new TAdvancedEditor( streamableInit );
}

TStreamableClass RAdvancedEditor( TAdvancedEditor::name,
                                  TAdvancedEditor::build,
                                  __DELTA(TAdvancedEditor)
                                );
