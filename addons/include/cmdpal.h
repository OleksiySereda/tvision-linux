/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   CMDPAL.H                                                              */
/*                                                                         */
/*   defines the class TCommandPalette                                     */
/*                                                                         */
/*   A list of every command of the menu bar, found by typing a part of    */
/*   its name:                                                             */
/*                                                                         */
/*       case cmPalette:                                                   */
/*           TCommandPalette::run();       // the chosen command follows   */
/*                                                                         */
/*   - the commands come from TProgram::menuBar, with their menu path      */
/*     ("File > Save as...") and their key ("F2"); the disabled ones are   */
/*     left out; addCommand() adds others;                                 */
/*   - the text typed is looked for in the names without regard to case:  */
/*     the start of a word first, then anywhere, then its letters in       */
/*     order ("sva" finds "Save as");                                      */
/*   - Up, Down, PgUp, PgDn move in the list while typing; Enter or a      */
/*     double click runs the command (putEvent), Esc closes.               */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TCommandPalette.                 */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TCommandPalette ) && !defined( __TCommandPalette )
#define __TCommandPalette

#include <string>
#include <vector>

class TMenu;
class TInputLine;
class TListViewer;
struct TEvent;

struct TPaletteEntry
{
    std::string name;                   // "File > Save as..."
    std::string key;                    // "F2", or ""
    ushort command;
};

class TCommandPalette : public TDialog
{

public:

    // the commands of menuBar (and of menu, if not 0)
    TCommandPalette( TMenu *menu = 0 );

    void addCommand( const char *name, ushort command, const char *key = "" );
    // the entries of a menu and its submenus, path before each name
    static void collect( TMenu *menu, const std::string& path,
                         std::vector<TPaletteEntry>& to );
    // how well text matches name: 0 not at all, larger is better
    static int score( const std::string& name, const char *text );

    // the entries that match the text typed, best first
    void filter();
    const std::vector<TPaletteEntry>& matches() const
        { return shown; }
    // the command chosen (after execView returned cmOK), or 0
    ushort chosen();

    virtual void handleEvent( TEvent& event );

    // shows the palette over the desktop and puts the chosen command into
    // the event queue; returns it, or 0
    static ushort run( TMenu *menu = 0 );

    TInputLine *input;
    TListViewer *list;

private:

    std::vector<TPaletteEntry> entries, shown;
    std::string lastText;
};

#endif  // Uses_TCommandPalette
