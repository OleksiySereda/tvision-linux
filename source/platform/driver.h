/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   DRIVER.H                                                              */
/*                                                                         */
/*   Inside the terminal driver: what the shared parts (keys.cpp,          */
/*   screen.cpp) and the system parts (posix/, win32/) give each other.    */
/*   Not for the library.                                                  */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if !defined( __TV_DRIVER_H )
#define __TV_DRIVER_H

#include "platform.h"

#include <stddef.h>

namespace tvplatform
{

// ---- given by the system part --------------------------------------------

// milliseconds of a clock that only goes forward
long long monotonicMs();

// writes all n bytes to the terminal
void writeOut( const char *s, size_t n );

// ---- keys.cpp: the key and mouse queues ----------------------------------

// Shift state bits, as in the BIOS keyboard flags byte (tkeys.h)
const unsigned short kRightShift = 0x0001, kLeftShift = 0x0002,
                     kCtrl = 0x0004, kAlt = 0x0008;

// modifier bits of pushSpecial(), as in the xterm "1;<m>" parameter (m - 1)
const int mShift = 1, mAlt = 2, mCtrl = 4;

enum SpecialKey
{
    skUp, skDown, skRight, skLeft, skHome, skEnd, skIns, skDel,
    skPgUp, skPgDn, skF1, skF2, skF3, skF4, skF5, skF6, skF7, skF8,
    skF9, skF10, skF11, skF12, skNone
};

// US keyboard: the BIOS scan code of a printable ASCII character
unsigned char asciiScan( unsigned char c );

unsigned short modsToShift( int mods );
void pushKey( unsigned short code, unsigned short shift );
// a character typed, Alt+character if alt
void pushChar( uint32_t u, bool alt );
// a cursor or function key with mShift | mAlt | mCtrl
void pushSpecial( SpecialKey k, int mods );

// the keys queued so far; ESC ESC [ ... adds Alt to the keys it gave
size_t keyCount();
void addShift( size_t from, unsigned short shift );

// a mouse state (buttons, position, or a wheel step) for getMouse();
// a state without a wheel step also becomes currentMouse()
void pushMouse( const MouseState &ms );
// the shift state shiftState() gives until the next key
void setShift( unsigned short shift );

bool inputQueued();

}

#endif  // __TV_DRIVER_H
