/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   THEMES.H                                                              */
/*                                                                         */
/*   colour themes for the whole program                                   */
/*                                                                         */
/*   A theme is a palette of the application (TProgram's colour palette,  */
/*   63 colours: desktop, menus, windows, dialogs):                        */
/*                                                                         */
/*       applyTheme( thDark );                                             */
/*                                                                         */
/*       // a submenu "Theme" with the themes, commands cmTheme..+3       */
/*       *new TSubMenu( "~T~heme", kbNoKey ) + *themeMenu( cmTheme )       */
/*       ...                                                               */
/*       case cmTheme ... cmTheme + 3: applyTheme( command - cmTheme );    */
/*                                                                         */
/*       loadTheme( settings );   // the one chosen last time ([colors])   */
/*       saveTheme( settings );                                            */
/*                                                                         */
/*   Classic is the palette of Turbo Vision; Dark has black windows and    */
/*   dialogs; Light has gray windows; High contrast is white and yellow    */
/*   on black.  Themes apply to the colour palette only (not when the      */
/*   program runs in black and white).                                     */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TThemes.                         */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TThemes ) && !defined( __TThemes )
#define __TThemes

class TMenuItem;
class TAppSettings;

enum { thClassic, thDark, thLight, thHighContrast, themeCount };

const char *themeName( int theme );         // "Classic" ...
// the theme of that name (without regard to case), or -1
int findTheme( const char *name );
// sets the palette and redraws the program; False if the program does
// not use the colour palette or theme is not one
Boolean applyTheme( int theme );
int currentTheme();
// the palette of a theme: 63 bytes, as cpColor
const char *themePalette( int theme );

// "~C~lassic", "~D~ark", ... with commands firstCommand ... + 3; the
// current one is marked
TMenuItem *themeMenu( ushort firstCommand, TMenuItem *next = 0 );

// [colors] theme=Dark
void saveTheme( TAppSettings& settings );
Boolean loadTheme( TAppSettings& settings );

#endif  // Uses_TThemes
