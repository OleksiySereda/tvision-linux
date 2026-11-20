/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   SYSTEM.H                                                              */
/*                                                                         */
/*   Copyright (c) Borland International 1991                              */
/*   All Rights Reserved.                                                  */
/*                                                                         */
/*   defines the classes THWMouse, TMouse, TEventQueue, TDisplay,          */
/*   TScreen, and TSystemError                                             */
/*                                                                         */
/*   Linux port: the hardware layer (BIOS video, INT 16h keyboard,         */
/*   INT 33h mouse) is replaced by a terminal driver, see                  */
/*   source/platform/.                                                     */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if !defined( __EVENT_CODES )
#define __EVENT_CODES

/* Event codes */

const int evMouseDown = 0x0001;
const int evMouseUp   = 0x0002;
const int evMouseMove = 0x0004;
const int evMouseAuto = 0x0008;
const int evKeyDown   = 0x0010;
const int evCommand   = 0x0100;
const int evBroadcast = 0x0200;

// Linux port: the mouse wheel.  Not part of evMouse, so views written for
// TV 1.0 never see it; it goes to the view under the mouse (see
// positionalEvents) if that view has evMouseWheel in its eventMask.
// event.mouse.buttons is one of mwUp, mwDown, mwLeft, mwRight.
const int evMouseWheel = 0x0020;

/* Event masks */

const int evNothing   = 0x0000;
const int evMouse     = 0x000f;
const int evKeyboard  = 0x0010;
const int evMessage   = 0xFF00;

/* Mouse button state masks */

const int mbLeftButton  = 0x01;
const int mbRightButton = 0x02;

/* Mouse wheel directions (evMouseWheel), Linux port */

const int mwUp    = 0x01;
const int mwDown  = 0x02;
const int mwLeft  = 0x04;
const int mwRight = 0x08;

#endif  // __EVENT_CODES


#if defined( Uses_TEvent ) && !defined( __TEvent )
#define __TEvent

struct MouseEventType
{
    uchar buttons;
    Boolean doubleClick;
    TPoint where;
};

class THWMouse
{

protected:

    THWMouse();
    THWMouse( const THWMouse& ) {};
    ~THWMouse();

    static void show();
    static void hide();

    static void setRange( ushort, ushort );
    static void getEvent( MouseEventType& );
    static Boolean present();

    static void suspend();
    static void resume();
    static void inhibit();

protected:

    static uchar buttonCount;

private:

    static Boolean noMouse;

};

inline Boolean THWMouse::present()
{
    return Boolean( buttonCount != 0 );
}

inline void THWMouse::inhibit()
{
    noMouse = True;
}

class TMouse : public THWMouse
{

public:

    TMouse();
    ~TMouse();

    static void show();
    static void hide();

    static void setRange( ushort, ushort );

    static void getEvent( MouseEventType& );
    static Boolean present();

    static void suspend() { THWMouse::suspend(); }
    static void resume() { THWMouse::resume(); show(); }

};

inline void TMouse::show()
{
    THWMouse::show();
}

inline void TMouse::hide()
{
    THWMouse::hide();
}

inline void TMouse::setRange( ushort rx, ushort ry )
{
    THWMouse::setRange( rx, ry );
}

inline void TMouse::getEvent( MouseEventType& me )
{
    THWMouse::getEvent( me );
}

inline Boolean TMouse::present()
{
    return THWMouse::present();
}

struct CharScanType
{
    uchar charCode;
    uchar scanCode;
};

struct KeyDownEvent
{
    union
        {
        ushort keyCode;
        CharScanType charScan;
        };
};

struct MessageEvent
{
    ushort command;
    union
        {
        void *infoPtr;
        long infoLong;
        ushort infoWord;
        short infoInt;
        uchar infoByte;
        char infoChar;
        };
};

struct TEvent
{
    ushort what;
    union
    {
        MouseEventType mouse;
        KeyDownEvent keyDown;
        MessageEvent message;
    };
    void getMouseEvent();
    void getKeyEvent();
};

#endif  // Uses_TEvent

#if defined( Uses_TEventQueue ) && !defined( __TEventQueue )
#define __TEventQueue

class TEventQueue
{
public:
    TEventQueue();
    ~TEventQueue();

    static void getMouseEvent( TEvent& );
    static void suspend();
    static void resume();

    // Keyboard shift state (kbRightShift, kbLeftShift, kbCtrlShift,
    // kbAltShift) of the most recent key or mouse event; replaces the
    // BIOS data area byte at 0040:0017.
    static ushort getShiftState();

    friend class TView;
    friend void genRefs();
    friend class TProgram;
    static ushort doubleDelay;
    static Boolean mouseReverse;

private:

    static TMouse mouse;
    static void getMouseState( TEvent& );

    static MouseEventType lastMouse;
    static MouseEventType curMouse;

    static MouseEventType downMouse;
    static ushort downTicks;

    static Boolean mouseEvents;

    static ushort repeatDelay;
    static ushort autoTicks;
    static ushort autoDelay;

};

inline void TEvent::getMouseEvent()
{
    TEventQueue::getMouseEvent( *this );
}

#endif  // Uses_TEventQueue

#if defined( Uses_TScreen ) && !defined( __TScreen )
#define __TScreen

class TDisplay
{

public:

    friend class TView;

    enum videoModes
        {
        smBW80      = 0x0002,
        smCO80      = 0x0003,
        smMono      = 0x0007,
        smFont8x8   = 0x0100
        };

    static void clearScreen( ushort, ushort );

    static void setCursorType( ushort );
    static ushort getCursorType();

    static ushort getRows();
    static ushort getCols();

    static void setCrtMode( ushort );
    static ushort getCrtMode();

protected:

    TDisplay() { updateIntlChars(); };
    TDisplay( const TDisplay& ) { updateIntlChars(); };
    ~TDisplay() {};

private:

    static void updateIntlChars();

};

class TScreen : public TDisplay
{

public:

    TScreen();
    ~TScreen();

    static void setVideoMode( ushort mode );
    static void clearScreen();

    static ushort startupMode;
    static ushort startupCursor;
    static ushort screenMode;
    static ushort screenWidth;
    static ushort screenHeight;
    static Boolean hiResScreen;
    static Boolean checkSnow;
    static ushort * screenBuffer;
    static ushort cursorLines;

    static void setCrtData();
    static ushort fixCrtMode( ushort );

    static void suspend();
    static void resume();

    // Linux port additions

    // Sends the changed part of screenBuffer to the terminal.
    static void flushScreen();
    // Hardware cursor position (set by TView::resetCursor).
    static void setCursorPos( ushort x, ushort y );
    // True when the terminal has been resized since the last call;
    // screenWidth/screenHeight/screenBuffer are updated on return.
    static Boolean checkResize();

};

#endif  // Uses_TScreen

#if defined( Uses_TSystemError ) && !defined( __TSystemError )
#define __TSystemError

class TDrawBuffer;

class TSystemError
{

public:

    TSystemError();
    ~TSystemError();

    static Boolean ctrlBreakHit;

    static void suspend();
    static void resume();
    static short ( *sysErrorFunc )( short, uchar );

private:

    static ushort sysColorAttr;
    static ushort sysMonoAttr;
    static Boolean saveCtrlBreak;
    static Boolean sysErrActive;

    static void swapStatusLine( TDrawBuffer & );
    static ushort selectKey();
    static short sysErr( short, uchar );

    static Boolean inIDE;

    static const char *const errorString[14];
    static const char * sRetryOrCancel;

};

#endif  // Uses_TSystemError
