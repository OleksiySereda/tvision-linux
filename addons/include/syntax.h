/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   SYNTAX.H                                                              */
/*                                                                         */
/*   defines the classes TSyntaxEditor and TSyntaxEditWindow               */
/*                                                                         */
/*   A TColoredEditor for C/C++, shell scripts, INI and .env files, JSON   */
/*   and Markdown:                                                         */
/*                                                                         */
/*       ed = new TSyntaxEditor( r, h, v, ind, "deploy.sh" );   // shell  */
/*       ed->setLanguage( slJson );                 // or chosen by hand   */
/*                                                                         */
/*       // a TEditWindow with the editor for the file's kind (YAML too)  */
/*       w = new TSyntaxEditWindow( r, "config.ini", wnNoNumber );         */
/*                                                                         */
/*   The language comes from the name of the file: .c .h .cpp .hpp ...    */
/*   C/C++; .sh .bash, Makefile: shell; .ini .cfg .conf .env, .env.*:     */
/*   INI; .json; .md: Markdown; .yaml .yml: TYamlEditor in the window.     */
/*   Colours are by class (keyword, string, comment, ...); darkColors and  */
/*   lightColors hold them for dark and light backgrounds, as for          */
/*   TYamlEditor.  Block comments and ``` blocks over several lines are    */
/*   followed from the start of the text.                                  */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TSyntaxEditor or                 */
/*   Uses_TSyntaxEditWindow.                                               */
/*                                                                         */
/* ------------------------------------------------------------------------*/

// only with Uses_TSyntaxEditor, so that the names meet no others
#if defined( Uses_TSyntaxEditor ) && !defined( __SYNTAX_CLASSES )
#define __SYNTAX_CLASSES

// the languages
enum { slPlain, slCpp, slShell, slIni, slJson, slMarkdown, slYaml, slCount };

// the classes of characters
enum
{
    scNone, scKeyword, scType, scString, scNumber, scComment, scPreprocessor,
    scPunctuation, scVariable, scHeading, scKey, scSection, scConstant,
    scEmphasis, scCode, scCount
};

#endif  // __SYNTAX_CLASSES


#if defined( Uses_TSyntaxEditor ) && !defined( __TSyntaxEditor )
#define __TSyntaxEditor

class TRect;
class TScrollBar;
class TIndicator;
struct TEvent;

class TSyntaxEditor : public TColoredEditor
{

public:

    // language -1: from the file name
    TSyntaxEditor( const TRect&, TScrollBar *, TScrollBar *, TIndicator *,
                   const char *aFileName, int aLanguage = -1 );

    void setLanguage( int aLanguage );
    int language;

    static int languageFor( const char *fileName );
    static const char *languageName( int language );

    // the classes of s[0..n) for a language; state: what is open at the
    // start of the line (a /* comment, a ``` block), returned for the end
    static int scanText( int language, const char *s, int n, int state, uchar *cls );

    virtual void scanLine( ushort linePtr, const char *s, int n, uchar *cls );
    virtual uchar classColor( uchar cls, uchar normal );
    virtual void handleEvent( TEvent& event );

    // foreground per class; 0xFF: the normal colour
    static uchar darkColors[scCount];
    static uchar lightColors[scCount];

private:

    int stateBefore( ushort linePtr );
    int stateAfterLine( ushort linePtr );

    ushort cachePtr;
    int cacheState;
    Boolean cacheValid;
};

#endif  // Uses_TSyntaxEditor


#if defined( Uses_TSyntaxEditWindow ) && !defined( __TSyntaxEditWindow )
#define __TSyntaxEditWindow

class TRect;

class TSyntaxEditWindow : public TEditWindow
{

public:

    // the editor: a TYamlEditor for YAML, else a TSyntaxEditor, with a
    // TBlockIndicator
    TSyntaxEditWindow( const TRect& bounds, const char *fileName, int aNumber );
};

#endif  // Uses_TSyntaxEditWindow
