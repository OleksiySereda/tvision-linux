/*------------------------------------------------------------*/
/* filename -       tadvedit.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TAdvancedEditor member functions, its     */
/*                  stream registration                       */
/*                                                            */
/*   handleEvent() turns Ctrl+C, X, V and A into cmCopy,      */
/*   cmCut, cmPaste and cmSelectAll before TEditor sees the   */
/*   key, so TEditor's own key map is left unchanged.         */
/*------------------------------------------------------------*/

#define Uses_TAdvancedEditor
#define Uses_TEvent
#define Uses_TKeys
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
    clipboardKeys( True )
{
}

TAdvancedEditor::TAdvancedEditor( StreamableInit ) :
    TFileEditor( streamableInit ),
    clipboardKeys( True )
{
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
