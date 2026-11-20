/*------------------------------------------------------------*/
/* filename -       tevent.cpp                                */
/*                                                            */
/* function(s)                                                */
/*                  TEvent member functions                   */
/*------------------------------------------------------------*/

/*------------------------------------------------------------*/
/*                                                            */
/*    Turbo Vision -  Version 1.0                             */
/*                                                            */
/*                                                            */
/*    Copyright (c) 1991 by Borland International             */
/*    All Rights Reserved.                                    */
/*                                                            */
/*    Linux port: mouse and keyboard events come from the     */
/*    terminal decoder instead of INT 33h / INT 16h; the      */
/*    wheel gives evMouseWheel events.                        */
/*                                                            */
/*------------------------------------------------------------*/

#define Uses_TEventQueue
#define Uses_TEvent
#define Uses_TScreen
#include <tv.h>

#include "platform/platform.h"

Boolean TEventQueue::mouseEvents = False;
Boolean TEventQueue::mouseReverse = False;
ushort TEventQueue::doubleDelay = 8;
ushort TEventQueue::repeatDelay = 8;
ushort TEventQueue::autoTicks = 0;
ushort TEventQueue::autoDelay = 0;

MouseEventType TEventQueue::lastMouse;
MouseEventType TEventQueue::curMouse;
MouseEventType TEventQueue::downMouse;
ushort TEventQueue::downTicks = 0;

// the wheel step of the last getMouseState() (mwUp ... mwRight), or 0
static uchar wheelStep = 0;

TEventQueue::TEventQueue()
{
    resume();
}

void TEventQueue::resume()
{
    if( mouse.present() == False )
        mouse.resume();
    if( mouse.present() == False )
        return;
    mouse.getEvent( curMouse );
    lastMouse = curMouse;
    mouseEvents = True;
    mouse.show();
    TMouse::setRange( TScreen::screenWidth-1, TScreen::screenHeight-1 );
}

void TEventQueue::suspend()
{
    mouse.suspend();
}

TEventQueue::~TEventQueue()
{
    suspend();
}

void TEventQueue::getMouseEvent( TEvent& ev )
{
    if( mouseEvents == True )
        {

        getMouseState( ev );

        if( wheelStep != 0 )
            {
            ev.what = evMouseWheel;
            ev.mouse.buttons = wheelStep;
            lastMouse.where = ev.mouse.where;
            return;
            }

        if( ev.mouse.buttons == 0 && lastMouse.buttons != 0 )
            {
            ev.what = evMouseUp;
            lastMouse = ev.mouse;
            return;
            }

        if( ev.mouse.buttons != 0 && lastMouse.buttons == 0 )
            {
            if( ev.mouse.buttons == downMouse.buttons &&
                ev.mouse.where == downMouse.where &&
                ushort( ev.what - downTicks ) <= doubleDelay )
                    ev.mouse.doubleClick = True;

            downMouse = ev.mouse;
            autoTicks = downTicks = ev.what;
            autoDelay = repeatDelay;
            ev.what = evMouseDown;
            lastMouse = ev.mouse;
            return;
            }

        ev.mouse.buttons = lastMouse.buttons;

        if( ev.mouse.where != lastMouse.where )
            {
            ev.what = evMouseMove;
            lastMouse = ev.mouse;
            return;
            }

        if( ev.mouse.buttons != 0 && ushort( ev.what - autoTicks ) > autoDelay )
            {
            autoTicks = ev.what;
            autoDelay = 1;
            ev.what = evMouseAuto;
            lastMouse = ev.mouse;
            return;
            }
        }

    ev.what = evNothing;
}

void TEventQueue::getMouseState( TEvent & ev )
{
    tvplatform::MouseState ms;
    if( !tvplatform::getMouse( ms ) )
        ms = tvplatform::currentMouse();
    wheelStep = ms.wheel;
    curMouse.buttons = ms.buttons;
    curMouse.doubleClick = False;
    curMouse.where.x = ms.x;
    curMouse.where.y = ms.y;

    ev.what = tvplatform::ticks();
    ev.mouse = curMouse;
    if( mouseReverse != False && ev.mouse.buttons != 0 && ev.mouse.buttons != 3 )
        ev.mouse.buttons ^= 3;
}

ushort TEventQueue::getShiftState()
{
    return tvplatform::shiftState();
}

void TEvent::getKeyEvent()
{
    tvplatform::KeyEvent k;
    if( !tvplatform::getKey( k ) )
        {
        what = evNothing;
        return;
        }
    what = evKeyDown;
    keyDown.keyCode = k.keyCode;
}
