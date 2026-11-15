/*------------------------------------------------------------*/
/* filename -       tscreen.cpp                               */
/*                                                            */
/* function(s)                                                */
/*                  TScreen member functions                  */
/*------------------------------------------------------------*/

/*------------------------------------------------------------*/
/*                                                            */
/*    Turbo Vision -  Version 1.0                             */
/*                                                            */
/*                                                            */
/*    Copyright (c) 1991 by Borland International             */
/*    All Rights Reserved.                                    */
/*                                                            */
/*    Linux port: the screen is an in-memory cell buffer      */
/*    that flushScreen() sends to the terminal.               */
/*                                                            */
/*------------------------------------------------------------*/

#define Uses_TScreen
#define Uses_TEvent
#include <tv.h>

#include "platform/platform.h"

#include <stdlib.h>
#include <string.h>

ushort TScreen::startupMode = 0xFFFF;
ushort TScreen::startupCursor = 0;
ushort TScreen::screenMode = 0;
ushort TScreen::screenWidth = 0;
ushort TScreen::screenHeight = 0;
Boolean TScreen::hiResScreen = False;
Boolean TScreen::checkSnow = False;
ushort * TScreen::screenBuffer = 0;
ushort TScreen::cursorLines = 0;

// cursor shape: 0x2000 = hidden, otherwise start/end scan lines;
// a start line of 0 means a block cursor (insert mode in TV)
static ushort cursorType = 0x2000;
static ushort cursorX = 0, cursorY = 0;
static ushort videoMode = TDisplay::smCO80;

static void updateCursor()
{
    tvplatform::setCursor( cursorX, cursorY,
                           (cursorType & 0x2000) == 0,
                           (cursorType >> 8) == 0 );
}

ushort TDisplay::getCursorType()
{
    return cursorType;
}

void TDisplay::setCursorType( ushort ct )
{
    cursorType = ct;
    updateCursor();
}

void TDisplay::clearScreen( ushort w, ushort h )
{
    if( TScreen::screenBuffer != 0 )
        for( size_t i = 0; i < size_t( w ) * h; i++ )
            TScreen::screenBuffer[i] = 0x0720;
}

ushort TDisplay::getRows()
{
    int cols, rows;
    tvplatform::getSize( cols, rows );
    return ushort( rows );
}

ushort TDisplay::getCols()
{
    int cols, rows;
    tvplatform::getSize( cols, rows );
    if( cols > maxViewWidth )
        cols = maxViewWidth;
    return ushort( cols );
}

ushort TDisplay::getCrtMode()
{
    return videoMode;
}

void TDisplay::setCrtMode( ushort mode )
{
    // there are no video modes on a terminal; only remember whether the
    // application asked for colour, black & white or monochrome output
    videoMode = mode & 0x00FF;
}

static ushort defaultMode()
{
    const char *mono = getenv( "TVMONO" );
    if( mono != 0 && *mono != '\0' && *mono != '0' )
        return TDisplay::smMono;
    const char *term = getenv( "TERM" );
    if( term != 0 && ( strcmp( term, "dumb" ) == 0 ||
                       strcmp( term, "vt100" ) == 0 ) )
        return TDisplay::smMono;
    return TDisplay::smCO80;
}

// Linux port: the terminal is switched to full-screen mode only when a
// TProgram is created, so programs that merely link the library (or use
// only its collections and streams) keep a normal console.
TScreen::TScreen()
{
    videoMode = defaultMode();
    startupMode = getCrtMode();
    startupCursor = getCursorType();
    setCrtData();
}

void TScreen::resume()
{
    tvplatform::start();
    tvplatform::invalidate();
    setCrtData();
}

TScreen::~TScreen()
{
    suspend();
    free( screenBuffer );
    screenBuffer = 0;
}

void TScreen::suspend()
{
    setCursorType( startupCursor );
    tvplatform::stop();
}

ushort TScreen::fixCrtMode( ushort mode )
{
    ushort m = mode & 0x00FF;
    if( m != smMono && m != smCO80 && m != smBW80 )
        m = smCO80;
    return m;
}

void TScreen::setCrtData()
{
    screenMode = getCrtMode();
    ushort w = getCols(), h = getRows();
    if( screenBuffer == 0 || w != screenWidth || h != screenHeight )
        {
        free( screenBuffer );
        screenWidth = w;
        screenHeight = h;
        screenBuffer = (ushort *)malloc( sizeof( ushort ) * w * h );
        clearScreen();
        }
    hiResScreen = Boolean( screenHeight > 25 );
    checkSnow = False;
    cursorLines = 0x0607;
    setCursorType( 0x2000 );
}

void TScreen::clearScreen()
{
    TDisplay::clearScreen( screenWidth, screenHeight );
}

void TScreen::setVideoMode( ushort mode )
{
    setCrtMode( fixCrtMode( mode ) );
    setCrtData();
}

void TScreen::flushScreen()
{
    if( screenBuffer != 0 )
        tvplatform::flush( screenBuffer, screenWidth, screenHeight );
}

void TScreen::setCursorPos( ushort x, ushort y )
{
    cursorX = x;
    cursorY = y;
    updateCursor();
}

Boolean TScreen::checkResize()
{
    if( !tvplatform::consumeResize() )
        return False;
    tvplatform::invalidate();
    ushort w = getCols(), h = getRows();
    if( w == screenWidth && h == screenHeight )
        return True;                    // e.g. SIGCONT: repaint only
    ushort *old = screenBuffer;
    ushort oldW = screenWidth, oldH = screenHeight;
    screenBuffer = 0;
    setCrtData();
    // keep what fits until the views redraw themselves
    for( ushort y = 0; y < h && y < oldH; y++ )
        memcpy( screenBuffer + size_t( y ) * w, old + size_t( y ) * oldW,
                sizeof( ushort ) * ( w < oldW ? w : oldW ) );
    free( old );
    return True;
}
