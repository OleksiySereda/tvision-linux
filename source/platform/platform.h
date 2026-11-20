/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   PLATFORM.H                                                            */
/*                                                                         */
/*   Terminal driver for the Linux port of Turbo Vision.  Replaces the     */
/*   BIOS video, keyboard and mouse services used by the DOS version.      */
/*                                                                         */
/*   Screen cells use the original DOS layout: low byte = character in    */
/*   the active 8-bit code page, high byte = CGA text attribute.           */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if !defined( __TV_PLATFORM_H )
#define __TV_PLATFORM_H

#include <stdint.h>

namespace tvplatform
{

struct KeyEvent
{
    unsigned short keyCode;     // BIOS style: scan << 8 | char
    unsigned short shift;       // kbLeftShift | kbCtrlShift | kbAltShift
};

struct MouseState
{
    unsigned char buttons;      // mbLeftButton | mbRightButton
    short x, y;
    unsigned char wheel;        // a wheel step: mwUp ... mwRight, or 0
};

// Puts the terminal into full-screen raw mode.  Safe to call repeatedly.
void start();
// Restores the terminal to the state it had before start().
void stop();
bool isStarted();

void getSize( int &cols, int &rows );
// Returns true once after each SIGWINCH.
bool consumeResize();

// Writes the cells that differ from what is already on the terminal.
void flush( const unsigned short *cells, int cols, int rows );
// Forgets what is on the terminal so the next flush repaints everything.
void invalidate();

void setCursor( int x, int y, bool visible, bool block );

// Waits up to `ms` milliseconds for input and decodes what arrives.
void waitInput( int ms );
bool getKey( KeyEvent &ev );
bool getMouse( MouseState &ms );
MouseState currentMouse();
unsigned short shiftState();

// BIOS-compatible timer: 18.2 ticks per second.
unsigned short ticks();

// Active code page: 437 (default), 866 or 1125, from $TVCODEPAGE.
int codePage();
uint32_t toUnicode( unsigned char c );
// Byte in the active code page, or -1 if the character is not in it.
int fromUnicode( uint32_t u );

}

#endif  // __TV_PLATFORM_H
