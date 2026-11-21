/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   TEXTCONV.H                                                            */
/*                                                                         */
/*   UTF-8 text for the screen                                             */
/*                                                                         */
/*   The screen of Turbo Vision shows bytes of an 8-bit code page (437 by  */
/*   default, 866 or 1125 with $TVCODEPAGE).  toScreenText() turns UTF-8   */
/*   into it: a character the code page has becomes its byte, dashes and   */
/*   quotes become - and ", anything else '?'.  Invalid UTF-8 bytes are    */
/*   taken as they are.                                                    */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TextConv.                        */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TextConv ) && !defined( __TextConv )
#define __TextConv

#include <string>

std::string toScreenText( const std::string& utf8 );
// the same for one character; -1 if it is not shown at all (U+FEFF)
int screenChar( unsigned long codePoint );
// ESC [ ... sequences (colours of terminal programs) taken out
std::string stripAnsi( const std::string& s );

#endif  // Uses_TextConv
