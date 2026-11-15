/*------------------------------------------------------------*/
/* filename -       syserr.cpp                                */
/*                                                            */
/* function(s)                                                */
/*          TSystemError member functions                     */
/*------------------------------------------------------------*/

/*------------------------------------------------------------*/
/*                                                            */
/*    Turbo Vision -  Version 1.0                             */
/*                                                            */
/*                                                            */
/*    Copyright (c) 1991 by Borland International             */
/*    All Rights Reserved.                                    */
/*                                                            */
/*------------------------------------------------------------*/


#define Uses_TDrawBuffer
#define Uses_TSystemError
#define Uses_TScreen
#include <tv.h>

#include "platform/platform.h"

#include <stdio.h>
#include <string.h>

Boolean TSystemError::ctrlBreakHit = False;
short ( * TSystemError::sysErrorFunc )( short, uchar ) = &TSystemError::sysErr;
ushort TSystemError::sysColorAttr = 0x4E4F;
ushort TSystemError::sysMonoAttr = 0x7070;
Boolean TSystemError::saveCtrlBreak = False;
Boolean TSystemError::sysErrActive = False;
Boolean TSystemError::inIDE = False;

TSystemError::TSystemError()
{
    inIDE = False;
    resume();
}

TSystemError::~TSystemError()
{
    suspend();
}


static int getakey()
{
    tvplatform::KeyEvent k;
    tvplatform::waitInput( 50 );
    return tvplatform::getKey( k ) ? k.keyCode : 0;
}


ushort TSystemError::selectKey()
{
    ushort crsrType = TScreen::getCursorType();

    TScreen::setCursorType( 0x2000 );

    while( getakey() )
        ;

    int ch = getakey() & 0xFF;
    while( ch != 13 && ch != 27 )
        ch = getakey() & 0xFF;

    TScreen::setCursorType( crsrType );
    return ch == 27;
}

short TSystemError::sysErr( short errorCode, uchar drive )
{
    ushort c = ( (TScreen::screenMode & 0x00fF) != TDisplay::smMono  ) ?
                                        sysColorAttr : sysMonoAttr;
    char s[ 128 ];
    TDrawBuffer b;

    sprintf( s, errorString[ errorCode ], drive + 'a' );

    b.moveChar( 0, ' ', c, 80);
    b.moveCStr( 1, s, c);
    b.moveCStr( 79-cstrlen(sRetryOrCancel), sRetryOrCancel, c);
    swapStatusLine(b);
    int res = selectKey();
    swapStatusLine(b);
    return res;
}

// Linux port: the DOS critical-error (INT 24h), Ctrl-Break (INT 1Bh/23h)
// and keyboard (INT 09h) hooks have no equivalent; Ctrl-C reaches the
// application as an ordinary key because the terminal is in raw mode.

void TSystemError::resume()
{
}

void TSystemError::suspend()
{
}

// Swaps the bottom screen row with the buffer (was SWAPST.ASM).
void TSystemError::swapStatusLine( TDrawBuffer &b )
{
    if( TScreen::screenBuffer == 0 )
        return;
    ushort w = TScreen::screenWidth < maxViewWidth ?
               TScreen::screenWidth : maxViewWidth;
    ushort *line = TScreen::screenBuffer +
                   size_t( TScreen::screenHeight - 1 ) * TScreen::screenWidth;
    for( ushort i = 0; i < w; i++ )
        {
        ushort t = line[i];
        line[i] = b.data[i];
        b.data[i] = t;
        }
    TScreen::flushScreen();
}
