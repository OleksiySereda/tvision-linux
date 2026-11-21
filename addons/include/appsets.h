/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   APPSETS.H                                                             */
/*                                                                         */
/*   defines the class TAppSettings                                        */
/*                                                                         */
/*   What a program keeps from one run to the next, in an INI file:       */
/*                                                                         */
/*       Linux    $XDG_CONFIG_HOME/<name>/settings.ini                     */
/*                (~/.config/<name>/settings.ini)                          */
/*       Windows  %APPDATA%/<name>/settings.ini                            */
/*                                                                         */
/*       TAppSettings settings( "vnm" );    // reads the file if it is      */
/*       settings.restoreWindow( "main", window );                         */
/*       settings.addRecent( "examples/sum.vnm" );                         */
/*       ...                                                               */
/*       settings.saveWindow( "main", window );                            */
/*       settings.save();                                                  */
/*                                                                         */
/*   - get() / set() for text, getInt() / getBool() for numbers and yes    */
/*     or no, by section and key;                                          */
/*   - saveWindow() / restoreWindow(): the place and size of a view (a     */
/*     window or a dialog), kept inside the desktop when it is restored;   */
/*   - the recent files (MRU): addRecent() puts a file first, at most      */
/*     maxRecent; recentMenu() makes menu items for them;                  */
/*   - saveHistory() / loadHistory(): the lists of THistory (by id);       */
/*   - savePalette() / loadPalette(): the colours of the application       */
/*     (TProgram's palette), so a colour theme chosen with TColorDialog    */
/*     stays.                                                              */
/*                                                                         */
/*   save() writes a new file and renames it over the old one, so a       */
/*   crash leaves either the old or the new settings.  Lines that are      */
/*   not "key=value" or "[section]" are kept as they are.                  */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TAppSettings.                    */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TAppSettings ) && !defined( __TAppSettings )
#define __TAppSettings

#include <string>
#include <vector>

class TView;
class TMenuItem;

class TAppSettings
{

public:

    // name: the program's; fileName: another file than the usual one
    TAppSettings( const char *name, const char *fileName = 0 );

    Boolean load();                     // False if there is no file
    Boolean save();                     // False if it could not be written
    const std::string& fileName() const
        { return path; }

    std::string get( const char *section, const char *key, const char *def = "" ) const;
    long getInt( const char *section, const char *key, long def = 0 ) const;
    Boolean getBool( const char *section, const char *key, Boolean def = False ) const;
    void set( const char *section, const char *key, const char *value );
    void setInt( const char *section, const char *key, long value );
    void setBool( const char *section, const char *key, Boolean value );
    void remove( const char *section, const char *key );
    void removeSection( const char *section );
    Boolean has( const char *section, const char *key ) const;

    // [window.<name>]: x, y, width, height
    void saveWindow( const char *name, TView *view );
    // False if nothing was saved; the view is moved inside its owner
    Boolean restoreWindow( const char *name, TView *view );

    // [recent]: file1 ... fileN
    void addRecent( const char *file );
    void removeRecent( const char *file );
    std::vector<std::string> recent() const;
    int maxRecent;                      // 8
    // "~1~ name" ... for the recent files, commands firstCommand,
    // firstCommand + 1, ...; then next.  0 if there are none.
    TMenuItem *recentMenu( ushort firstCommand, TMenuItem *next = 0 ) const;

    // [history.<id>]: the strings of THistory list id, oldest first
    void saveHistory( uchar id );
    void loadHistory( uchar id );

    // [colors]: the palette of TProgram as hex
    void savePalette();
    Boolean loadPalette();

    // the directory of the settings of name ("" if there is none)
    static std::string configDir( const char *name );

private:

    enum { lnSection, lnPair, lnOther };
    struct Line
    {
        int kind;
        std::string section, key, value;
        std::string raw;                // lnOther: the line as it was
    };
    std::vector<Line> lines;
    std::string path;

    int find( const std::string& section, const std::string& key ) const;
};

#endif  // Uses_TAppSettings
