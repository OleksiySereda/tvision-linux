/*------------------------------------------------------------*/
/* filename -       input.cpp                                 */
/*                                                            */
/* function(s)                                                */
/*          Windows console input: key and mouse events ->    */
/*          BIOS style key codes, mouse states                */
/*                                                            */
/*   The console gives the state of Shift, Ctrl and Alt with  */
/*   every key, so Shift+arrows and the like always work.     */
/*------------------------------------------------------------*/

#include "../driver.h"

#include <windows.h>

namespace tvplatform
{

// terminal.cpp
int windowTop();
HANDLE inputHandle();

static int modsOf( DWORD state )
{
    int m = 0;
    if( state & SHIFT_PRESSED )
        m |= mShift;
    if( state & ( LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED ) )
        m |= mCtrl;
    if( state & ( LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED ) )
        m |= mAlt;
    return m;
}

static SpecialKey specialOf( WORD vk )
{
    switch( vk )
        {
        case VK_UP:     return skUp;
        case VK_DOWN:   return skDown;
        case VK_RIGHT:  return skRight;
        case VK_LEFT:   return skLeft;
        case VK_HOME:   return skHome;
        case VK_END:    return skEnd;
        case VK_INSERT: return skIns;
        case VK_DELETE: return skDel;
        case VK_PRIOR:  return skPgUp;
        case VK_NEXT:   return skPgDn;
        }
    if( vk >= VK_F1 && vk <= VK_F12 )
        return SpecialKey( skF1 + ( vk - VK_F1 ) );
    return skNone;
}

static void keyEvent( const KEY_EVENT_RECORD &k )
{
    int mods = modsOf( k.dwControlKeyState );
    setShift( modsToShift( mods ) );
    if( !k.bKeyDown )
        return;

    wchar_t ch = k.uChar.UnicodeChar;
    // AltGr (right Alt) is reported as Ctrl+Alt; with a character it types
    // the character
    bool altGr = ( k.dwControlKeyState & RIGHT_ALT_PRESSED ) &&
                 ( k.dwControlKeyState & LEFT_CTRL_PRESSED ) && ch >= 0x20;

    for( int n = k.wRepeatCount > 0 ? k.wRepeatCount : 1; n > 0; n-- )
        {
        SpecialKey sk = specialOf( k.wVirtualKeyCode );
        if( sk != skNone )
            pushSpecial( sk, mods );
        else if( k.wVirtualKeyCode == VK_TAB && ( mods & mShift ) )
            pushKey( 0x0F00, kLeftShift );              // kbShiftTab
        else if( ch == 0 )
            return;                                     // Shift, Ctrl, ... alone
        else if( altGr )
            pushChar( ch, false );
        else if( mods & mAlt )
            {
            // Alt+letter: the letter of the key, whatever the layout
            WORD vk = k.wVirtualKeyCode;
            if( vk >= 'A' && vk <= 'Z' )
                pushChar( vk + 32, true );
            else if( vk >= '0' && vk <= '9' )
                pushChar( vk, true );
            else
                pushChar( ch, true );
            }
        else if( ( mods & mCtrl ) && k.wVirtualKeyCode == VK_RETURN )
            pushChar( 0x0A, false );                    // kbCtrlEnter
        else
            {
            pushChar( ch, false );
            if( mods != 0 )
                addShift( keyCount() - 1, modsToShift( mods ) );
            }
        }
}

static void mouseEvent( const MOUSE_EVENT_RECORD &m )
{
    setShift( modsToShift( modsOf( m.dwControlKeyState ) ) );
    MouseState ms = currentMouse();
    ms.x = m.dwMousePosition.X;
    ms.y = short( m.dwMousePosition.Y - windowTop() );
    ms.wheel = 0;
    if( m.dwEventFlags & ( MOUSE_WHEELED | MOUSE_HWHEELED ) )
        {
        // the high word: the distance, > 0 away from the user / right
        short delta = short( HIWORD( m.dwButtonState ) );
        if( m.dwEventFlags & MOUSE_WHEELED )
            ms.wheel = delta > 0 ? 0x01 : 0x02;         // mwUp, mwDown
        else
            ms.wheel = delta > 0 ? 0x08 : 0x04;         // mwRight, mwLeft
        pushMouse( ms );
        return;
        }
    // the same bits as mbLeftButton, mbRightButton and the middle button
    ms.buttons = (unsigned char)( m.dwButtonState & 0x07 );
    pushMouse( ms );
}

void waitInput( int ms )
{
    if( inputQueued() )
        ms = 0;
    HANDLE h = inputHandle();
    if( WaitForSingleObject( h, DWORD( ms < 0 ? 0 : ms ) ) != WAIT_OBJECT_0 )
        return;
    DWORD count = 0;
    while( GetNumberOfConsoleInputEvents( h, &count ) && count > 0 )
        {
        INPUT_RECORD rec[64];
        DWORD got = 0;
        if( !ReadConsoleInputW( h, rec, 64, &got ) || got == 0 )
            return;
        for( DWORD i = 0; i < got; i++ )
            switch( rec[i].EventType )
                {
                case KEY_EVENT:
                    keyEvent( rec[i].Event.KeyEvent );
                    break;
                case MOUSE_EVENT:
                    mouseEvent( rec[i].Event.MouseEvent );
                    break;
                }               // the size is checked by consumeResize()
        }
}

}
