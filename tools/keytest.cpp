/*------------------------------------------------------------*/
/* filename -       keytest.cpp                               */
/*                                                            */
/*  Prints the BIOS-style key codes and mouse reports the     */
/*  terminal driver decodes, to check a terminal/ssh client.  */
/*  Press Ctrl-C (or q twice) to quit.                        */
/*------------------------------------------------------------*/

#include "../source/platform/platform.h"

#include <stdio.h>
#include <string>

int main()
{
    tvplatform::start();
    std::string log = "keytest: press keys, click the mouse; Ctrl-C quits\r\n";
    int quits = 0;
    for( ;; )
        {
        fputs( log.c_str(), stdout );
        fflush( stdout );
        log.clear();
        tvplatform::waitInput( 100 );
        tvplatform::KeyEvent k;
        while( tvplatform::getKey( k ) )
            {
            char buf[80];
            unsigned ch = k.keyCode & 0xFF;
            snprintf( buf, sizeof( buf ),
                      "key 0x%04X  scan 0x%02X  char 0x%02X '%c'  shift 0x%02X\r\n",
                      k.keyCode, k.keyCode >> 8, ch,
                      ch >= 0x20 && ch < 0x7F ? ch : '.', k.shift );
            log += buf;
            if( k.keyCode == 0x2E03 )
                quits = 2;
            else if( ch == 'q' )
                quits++;
            else
                quits = 0;
            }
        tvplatform::MouseState m;
        while( tvplatform::getMouse( m ) )
            {
            char buf[80];
            snprintf( buf, sizeof( buf ), "mouse x=%d y=%d buttons=%d\r\n",
                      m.x, m.y, m.buttons );
            log += buf;
            }
        if( quits >= 2 )
            break;
        }
    fputs( log.c_str(), stdout );
    tvplatform::stop();
    return 0;
}
