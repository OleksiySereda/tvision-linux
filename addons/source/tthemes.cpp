/*------------------------------------------------------------*/
/* filename -       tthemes.cpp                               */
/*                                                            */
/* function(s)                                                */
/*                  the colour themes                         */
/*                                                            */
/*   The palettes follow cpColor (app.h), entry by entry:     */
/*     1      desktop                                         */
/*     2- 7   menus: normal, disabled, shortcut, selected,    */
/*            selected disabled, selected shortcut            */
/*     8-15   blue windows, 16-23 cyan, 24-31 gray: frame     */
/*            passive, active, icon, scroll bar page and      */
/*            controls, scroller normal and selected, -       */
/*    32-63   dialogs: frame (3), scroll bar (2), static      */
/*            text, labels (3), buttons (6: normal, default,  */
/*            selected, disabled, shortcut, shadow), clusters */
/*            (3), input lines (3), history (4), list         */
/*            viewers (4), info pane, -, -                    */
/*------------------------------------------------------------*/

#define Uses_TThemes
#define Uses_TAppSettings
#define Uses_TProgram
#define Uses_TApplication
#define Uses_TGroup
#define Uses_TPalette
#define Uses_TMenuItem
#define Uses_TKeys
#include <tvaddons.h>

#include <ctype.h>
#include <string.h>
#include <string>

namespace
{

const char classic[] =
    "\x71\x70\x78\x74\x20\x28\x24\x17\x1F\x1A\x31\x31\x1E\x71\x00"
    "\x37\x3F\x3A\x13\x13\x3E\x21\x00\x70\x7F\x7A\x13\x13\x70\x7F\x00"
    "\x70\x7F\x7A\x13\x13\x70\x70\x7F\x7E\x20\x2B\x2F\x78\x2E\x70\x30"
    "\x3F\x3E\x1F\x2F\x1A\x20\x72\x31\x31\x30\x2F\x3E\x31\x13\x00\x00";

const char dark[] =
    // desktop, menus
    "\x08\x07\x08\x0E\x1F\x18\x1E"
    // blue, cyan, gray windows
    "\x08\x0F\x0A\x08\x07\x07\x70\x00"
    "\x08\x0F\x0A\x08\x07\x0B\x30\x00"
    "\x08\x0F\x0A\x08\x07\x07\x0F\x00"
    // dialogs: frame, scroll bar, static text, labels
    "\x07\x0F\x0A\x08\x07\x07\x07\x0F\x0E"
    // buttons
    "\x17\x1B\x1F\x08\x1E\x00"
    // clusters, input lines, history, lists, info pane
    "\x07\x0F\x0E\x1F\x2F\x0A\x17\x08\x08\x07\x07\x1F\x0E\x08\x07\x00\x00";

const char light[] =
    "\x78\x70\x78\x74\x1F\x18\x1E"
    "\x70\x71\x72\x78\x70\x70\x1F\x00"
    "\x30\x3F\x3A\x78\x70\x30\x1F\x00"
    "\x70\x71\x72\x78\x70\x70\x1F\x00"
    "\x70\x71\x72\x78\x70\x70\x70\x71\x74"
    "\x1F\x1E\x2F\x78\x1E\x70"
    "\x70\x71\x74\x1F\x2F\x72\x1F\x78\x78\x70\x70\x1F\x71\x78\x70\x00\x00";

const char highContrast[] =
    "\x07\x0F\x08\x0E\x70\x78\x74"
    "\x0F\x0E\x0E\x07\x0F\x0F\x70\x00"
    "\x0F\x0E\x0E\x07\x0F\x0F\x70\x00"
    "\x0F\x0E\x0E\x07\x0F\x0F\x70\x00"
    "\x0F\x0E\x0E\x07\x0F\x0F\x0F\x0E\x0E"
    "\x70\x74\x0E\x08\x74\x00"
    "\x0F\x0E\x0E\x70\x0E\x0E\x70\x07\x07\x0F\x0F\x70\x0E\x07\x0F\x00\x00";

static_assert( sizeof( classic ) - 1 == 63, "63 colours" );
static_assert( sizeof( dark ) - 1 == 63, "63 colours" );
static_assert( sizeof( light ) - 1 == 63, "63 colours" );
static_assert( sizeof( highContrast ) - 1 == 63, "63 colours" );

const char *const names[] = { "Classic", "Dark", "Light", "High contrast" };
const char *const menuNames[] = { "~C~lassic", "~D~ark", "~L~ight", "~H~igh contrast" };
const char *const palettes[] = { classic, dark, light, highContrast };

int current = thClassic;

// the windows and the desktop keep what they show in buffers, in the old
// colours; without them redraw() draws everything again.  Not the
// program itself: its buffer is the screen.
void freeBuffers( TGroup *g )
{
    TView *last = g->last;
    if( last == 0 )
        return;
    TView *p = last;
    do  {
        TGroup *sub = dynamic_cast<TGroup *>( p );
        if( sub != 0 )
            {
            freeBuffers( sub );
            sub->freeBuffer();
            }
        p = p->next;
        } while( p != last );
}

}   // namespace

const char *themeName( int theme )
{
    return theme >= 0 && theme < themeCount ? names[theme] : "";
}

int findTheme( const char *name )
{
    if( name == 0 )
        return -1;
    for( int i = 0; i < themeCount; i++ )
        {
        const char *a = names[i], *b = name;
        while( *a != 0 && tolower( uchar( *a ) ) == tolower( uchar( *b ) ) )
            a++, b++;
        if( *a == 0 && *b == 0 )
            return i;
        }
    return -1;
}

const char *themePalette( int theme )
{
    return theme >= 0 && theme < themeCount ? palettes[theme] : 0;
}

int currentTheme()
{
    return current;
}

Boolean applyTheme( int theme )
{
    if( theme < 0 || theme >= themeCount || TProgram::application == 0 ||
        TProgram::appPalette != apColor )
        return False;
    TPalette& p = TProgram::application->getPalette();
    if( uchar( p[0] ) != 63 )
        return False;
    for( int i = 1; i <= 63; i++ )
        p[i] = palettes[theme][i - 1];
    current = theme;
    freeBuffers( TProgram::application );
    TProgram::application->redraw();
    return True;
}

TMenuItem *themeMenu( ushort firstCommand, TMenuItem *next )
{
    for( int i = themeCount - 1; i >= 0; i-- )
        next = new TMenuItem( menuNames[i], ushort( firstCommand + i ), kbNoKey, hcNoContext,
                              (char *) ( i == current ? "*" : "" ), next );
    return next;
}

void saveTheme( TAppSettings& settings )
{
    settings.set( "colors", "theme", themeName( current ) );
}

Boolean loadTheme( TAppSettings& settings )
{
    int t = findTheme( settings.get( "colors", "theme" ).c_str() );
    return t >= 0 ? applyTheme( t ) : False;
}
