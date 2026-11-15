/*------------------------------------------------------------*/
/* filename -       tmouse.cpp                                */
/*                                                            */
/* function(s)                                                */
/*                  TMouse and THWMouse member functions      */
/*------------------------------------------------------------*/

/*------------------------------------------------------------*/
/*                                                            */
/*    Turbo Vision -  Version 1.0                             */
/*                                                            */
/*                                                            */
/*    Copyright (c) 1991 by Borland International             */
/*    All Rights Reserved.                                    */
/*                                                            */
/*    Linux port: the terminal reports the mouse (xterm SGR   */
/*    protocol); the terminal itself draws the pointer.       */
/*                                                            */
/*------------------------------------------------------------*/

#define Uses_TEvent
#include <tv.h>

#include "platform/platform.h"

uchar THWMouse::buttonCount = 0;
Boolean THWMouse::noMouse = False;

THWMouse::THWMouse()
{
    resume();
}

void THWMouse::resume()
{
    if( noMouse == False )
        buttonCount = 2;
}

THWMouse::~THWMouse()
{
    suspend();
}

void THWMouse::suspend()
{
    buttonCount = 0;
}

void THWMouse::show()
{
}

void THWMouse::hide()
{
}

void THWMouse::setRange( ushort, ushort )
{
}

void THWMouse::getEvent( MouseEventType& me )
{
    tvplatform::MouseState ms = tvplatform::currentMouse();
    me.buttons = ms.buttons;
    me.doubleClick = False;
    me.where.x = ms.x;
    me.where.y = ms.y;
}

TMouse::TMouse()
{
    show();
}

TMouse::~TMouse()
{
    hide();
}
