/*------------------------------------------------------------*/
/* filename -       tsyntax.cpp                               */
/*                                                            */
/* function(s)                                                */
/*                  TSyntaxEditor and TSyntaxEditWindow       */
/*                  member functions, the scanners            */
/*                                                            */
/*   A scanner gives each character of a line its class; the  */
/*   state carries what goes on to the next line (a block     */
/*   comment, a code fence).  The state at the start of a     */
/*   line is found by scanning from the start of the text,    */
/*   and kept for the next line while a screen is drawn.      */
/*------------------------------------------------------------*/

#define Uses_TSyntaxEditor
#define Uses_TSyntaxEditWindow
#define Uses_TYamlEditor
#define Uses_TBlockIndicator
#define Uses_TEditWindow
#define Uses_TScrollBar
#define Uses_TEvent
#define Uses_TRect
#include <tvaddons.h>

#include <ctype.h>
#include <string.h>

uchar TSyntaxEditor::darkColors[scCount] =
    { 0xFF, 0x0F, 0x0B, 0x0A, 0x0D, 0x07, 0x0C, 0x0B, 0x0E, 0x0F, 0x0F, 0x0E, 0x0D, 0x0F, 0x0A };
uchar TSyntaxEditor::lightColors[scCount] =
    { 0xFF, 0x01, 0x03, 0x02, 0x05, 0x08, 0x04, 0x03, 0x06, 0x01, 0x01, 0x04, 0x05, 0x00, 0x02 };

namespace
{

bool isIdentStart( char c )
{
    return isalpha( uchar( c ) ) || c == '_';
}

bool isIdent( char c )
{
    return isalnum( uchar( c ) ) || c == '_';
}

bool inList( const char *const *list, const char *s, int n )
{
    for( ; *list != 0; list++ )
        if( int( strlen( *list ) ) == n && strncmp( *list, s, size_t( n ) ) == 0 )
            return true;
    return false;
}

void fill( uchar *cls, int from, int to, uchar c )
{
    for( int i = from; i < to; i++ )
        cls[i] = c;
}

// a quoted string from s[i] (the quote); escapes with backslash if esc
int quoted( const char *s, int n, int i, bool esc )
{
    char q = s[i++];
    while( i < n && s[i] != q )
        i += ( esc && s[i] == '\\' && i + 1 < n ) ? 2 : 1;
    return i < n ? i + 1 : n;
}

int number( const char *s, int n, int i )
{
    while( i < n && ( isalnum( uchar( s[i] ) ) || s[i] == '.' || s[i] == '\'' ||
                      ( ( s[i] == '+' || s[i] == '-' ) && i > 0 &&
                        ( s[i - 1] == 'e' || s[i - 1] == 'E' ) ) ) )
        i++;
    return i;
}

const char *const cppKeywords[] =
    { "if", "else", "for", "while", "do", "switch", "case", "default", "break", "continue",
      "return", "goto", "sizeof", "typedef", "struct", "class", "union", "enum", "namespace",
      "using", "template", "typename", "public", "private", "protected", "virtual", "override",
      "final", "static", "extern", "inline", "const", "constexpr", "volatile", "mutable",
      "friend", "operator", "new", "delete", "this", "try", "catch", "throw", "noexcept",
      "nullptr", "true", "false", "auto", "register", "explicit", "static_assert", "decltype",
      "static_cast", "dynamic_cast", "const_cast", "reinterpret_cast", "alignas", "alignof",
      0 };
const char *const cppTypes[] =
    { "void", "bool", "char", "short", "int", "long", "float", "double", "signed",
      "unsigned", "size_t", "ssize_t", "wchar_t", "char16_t", "char32_t", "int8_t", "int16_t",
      "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t", "uintptr_t",
      "ptrdiff_t", "std", "string", "vector", "uchar", "ushort", "Boolean", 0 };

int scanCpp( const char *s, int n, int state, uchar *cls )
{
    int i = 0;
    int first = 0;
    while( first < n && isspace( uchar( s[first] ) ) )
        first++;
    bool directive = state == 0 && first < n && s[first] == '#';
    while( i < n )
        {
        if( state == 1 )
            {
            int start = i;
            while( i < n && !( s[i] == '*' && i + 1 < n && s[i + 1] == '/' ) )
                i++;
            if( i < n )
                {
                i += 2;
                state = 0;
                }
            fill( cls, start, i, scComment );
            continue;
            }
        char c = s[i];
        if( c == '/' && i + 1 < n && s[i + 1] == '/' )
            {
            fill( cls, i, n, scComment );
            break;
            }
        if( c == '/' && i + 1 < n && s[i + 1] == '*' )
            {
            fill( cls, i, i + 2, scComment );
            i += 2;
            state = 1;
            continue;
            }
        if( c == '"' || c == '\'' )
            {
            int e = quoted( s, n, i, true );
            fill( cls, i, e, scString );
            i = e;
            continue;
            }
        if( directive && c == '#' )
            {
            int e = i + 1;
            while( e < n && s[e] == ' ' )
                e++;
            while( e < n && isIdent( s[e] ) )
                e++;
            fill( cls, i, e, scPreprocessor );
            i = e;
            // #include <file>
            while( i < n && s[i] == ' ' )
                i++;
            if( i < n && s[i] == '<' )
                {
                int k = i;
                while( k < n && s[k] != '>' )
                    k++;
                fill( cls, i, k < n ? k + 1 : n, scString );
                i = k < n ? k + 1 : n;
                }
            continue;
            }
        if( isdigit( uchar( c ) ) || ( c == '.' && i + 1 < n && isdigit( uchar( s[i + 1] ) ) ) )
            {
            int e = number( s, n, i );
            fill( cls, i, e, scNumber );
            i = e;
            continue;
            }
        if( isIdentStart( c ) )
            {
            int e = i;
            while( e < n && isIdent( s[e] ) )
                e++;
            uchar k = scNone;
            if( inList( cppKeywords, s + i, e - i ) )
                k = scKeyword;
            else if( inList( cppTypes, s + i, e - i ) )
                k = scType;
            else
                {
                // MACRO_NAMES and constants written in capitals
                bool caps = e - i >= 2;
                for( int j = i; j < e && caps; j++ )
                    if( islower( uchar( s[j] ) ) )
                        caps = false;
                if( caps && isupper( uchar( s[i] ) ) )
                    k = scConstant;
                }
            fill( cls, i, e, k );
            i = e;
            continue;
            }
        cls[i] = strchr( "{}()[];,.<>+-*/%=&|^!~?:", c ) != 0 && c != 0 ? scPunctuation : scNone;
        i++;
        }
    return state;
}

const char *const shellKeywords[] =
    { "if", "then", "else", "elif", "fi", "for", "while", "until", "do", "done", "case",
      "esac", "in", "function", "select", "time", "return", "export", "local", "readonly",
      "declare", "unset", "shift", "exit", "break", "continue", "source", "alias", "set",
      "trap", "eval", "exec", "test", 0 };

// $VAR, ${...}, $1, $?, $@ ...: the end
int variable( const char *s, int n, int i )
{
    int e = i + 1;
    if( e < n && s[e] == '{' )
        {
        while( e < n && s[e] != '}' )
            e++;
        return e < n ? e + 1 : n;
        }
    if( e < n && ( isdigit( uchar( s[e] ) ) || strchr( "?@#*$!-", s[e] ) != 0 ) && s[e] != 0 )
        return e + 1;
    while( e < n && isIdent( s[e] ) )
        e++;
    return e;
}

int scanShell( const char *s, int n, int state, uchar *cls )
{
    int i = 0;
    while( i < n )
        {
        char c = s[i];
        if( c == '#' && ( i == 0 || isspace( uchar( s[i - 1] ) ) || s[i - 1] == ';' ) )
            {
            fill( cls, i, n, scComment );
            break;
            }
        if( c == '\'' )
            {
            int e = quoted( s, n, i, false );
            fill( cls, i, e, scString );
            i = e;
            continue;
            }
        if( c == '"' )
            {
            int e = quoted( s, n, i, true );
            fill( cls, i, e, scString );
            // the variables inside stand out
            for( int k = i + 1; k < e - 1; k++ )
                if( s[k] == '$' && s[k - 1] != '\\' )
                    {
                    int v = variable( s, e - 1, k );
                    if( v > k + 1 )
                        fill( cls, k, v, scVariable );
                    k = v - 1;
                    }
            i = e;
            continue;
            }
        if( c == '$' )
            {
            int e = variable( s, n, i );
            if( e > i + 1 )
                {
                fill( cls, i, e, scVariable );
                i = e;
                continue;
                }
            }
        if( isIdentStart( c ) && ( i == 0 || !isIdent( s[i - 1] ) ) )
            {
            int e = i;
            while( e < n && ( isIdent( s[e] ) || s[e] == '-' ) )
                e++;
            // NAME=value
            if( e < n && s[e] == '=' )
                fill( cls, i, e, scVariable );
            else
                fill( cls, i, e, inList( shellKeywords, s + i, e - i ) ? scKeyword : scNone );
            i = e;
            continue;
            }
        if( isdigit( uchar( c ) ) && ( i == 0 || !isIdent( s[i - 1] ) ) )
            {
            int e = i;
            while( e < n && isdigit( uchar( s[e] ) ) )
                e++;
            if( e == n || !isIdent( s[e] ) )
                fill( cls, i, e, scNumber );
            i = e;
            continue;
            }
        cls[i] = c != 0 && strchr( "|&;<>(){}[]=", c ) != 0 ? scPunctuation : scNone;
        i++;
        }
    return state;
}

int scanIni( const char *s, int n, int state, uchar *cls )
{
    int i = 0;
    while( i < n && isspace( uchar( s[i] ) ) )
        i++;
    if( i >= n )
        return state;
    if( s[i] == ';' || s[i] == '#' )
        {
        fill( cls, i, n, scComment );
        return state;
        }
    if( s[i] == '[' )
        {
        fill( cls, i, n, scSection );
        return state;
        }
    // export NAME=value (.env)
    if( n - i > 7 && strncmp( s + i, "export ", 7 ) == 0 )
        {
        fill( cls, i, i + 6, scKeyword );
        i += 7;
        }
    int k = i;
    while( k < n && s[k] != '=' && s[k] != ':' )
        k++;
    if( k >= n )
        return state;
    fill( cls, i, k, scKey );
    cls[k] = scPunctuation;
    i = k + 1;
    while( i < n && isspace( uchar( s[i] ) ) )
        i++;
    if( i >= n )
        return state;
    if( s[i] == '"' || s[i] == '\'' )
        {
        int e = quoted( s, n, i, s[i] == '"' );
        fill( cls, i, e, scString );
        i = e;
        }
    else
        {
        int e = n;
        while( e > i && isspace( uchar( s[e - 1] ) ) )
            e--;
        static const char *const constants[] =
            { "true", "false", "yes", "no", "on", "off", "null", "none", 0 };
        bool num = e > i;
        for( int j = i; j < e && num; j++ )
            if( !isdigit( uchar( s[j] ) ) && s[j] != '.' && !( j == i && s[j] == '-' ) )
                num = false;
        if( num )
            fill( cls, i, e, scNumber );
        else if( inList( constants, s + i, e - i ) )
            fill( cls, i, e, scConstant );
        }
    // ${VAR} and $VAR in the value; a comment after a blank
    for( ; i < n; i++ )
        if( s[i] == '$' )
            {
            int e = variable( s, n, i );
            if( e > i + 1 )
                fill( cls, i, e, scVariable );
            i = e - 1;
            }
        else if( ( s[i] == '#' || s[i] == ';' ) && i > 0 && s[i - 1] == ' ' && cls[i] != scString )
            {
            fill( cls, i, n, scComment );
            break;
            }
    return state;
}

int scanJson( const char *s, int n, int state, uchar *cls )
{
    int i = 0;
    while( i < n )
        {
        char c = s[i];
        if( c == '"' )
            {
            int e = quoted( s, n, i, true );
            int k = e;
            while( k < n && isspace( uchar( s[k] ) ) )
                k++;
            fill( cls, i, e, k < n && s[k] == ':' ? scKey : scString );
            i = e;
            continue;
            }
        if( isdigit( uchar( c ) ) || ( c == '-' && i + 1 < n && isdigit( uchar( s[i + 1] ) ) ) )
            {
            int e = number( s, n, i + 1 );
            fill( cls, i, e, scNumber );
            i = e;
            continue;
            }
        if( isalpha( uchar( c ) ) )
            {
            int e = i;
            while( e < n && isalpha( uchar( s[e] ) ) )
                e++;
            static const char *const words[] = { "true", "false", "null", 0 };
            fill( cls, i, e, inList( words, s + i, e - i ) ? scConstant : scNone );
            i = e;
            continue;
            }
        cls[i] = c != 0 && strchr( "{}[]:,", c ) != 0 ? scPunctuation : scNone;
        i++;
        }
    return state;
}

int scanMarkdown( const char *s, int n, int state, uchar *cls )
{
    int i = 0;
    while( i < n && s[i] == ' ' )
        i++;
    bool fence = n - i >= 3 && ( strncmp( s + i, "```", 3 ) == 0 || strncmp( s + i, "~~~", 3 ) == 0 );
    if( fence )
        {
        fill( cls, 0, n, scCode );
        return state == 1 ? 0 : 1;
        }
    if( state == 1 )
        {
        fill( cls, 0, n, scCode );
        return state;
        }
    if( i < n && s[i] == '#' )
        {
        fill( cls, i, n, scHeading );
        return state;
        }
    if( i < n && s[i] == '>' )
        {
        fill( cls, i, n, scComment );
        return state;
        }
    if( i + 1 < n && ( s[i] == '-' || s[i] == '*' || s[i] == '+' ) && s[i + 1] == ' ' )
        {
        cls[i] = scPunctuation;
        i += 2;
        }
    else
        {
        int k = i;
        while( k < n && isdigit( uchar( s[k] ) ) )
            k++;
        if( k > i && k + 1 < n && s[k] == '.' && s[k + 1] == ' ' )
            {
            fill( cls, i, k + 1, scPunctuation );
            i = k + 2;
            }
        }
    bool strong = false, em = false;
    while( i < n )
        {
        char c = s[i];
        if( c == '`' )
            {
            int e = i + 1;
            while( e < n && s[e] != '`' )
                e++;
            e = e < n ? e + 1 : n;
            fill( cls, i, e, scCode );
            i = e;
            continue;
            }
        if( ( c == '*' || c == '_' ) && i + 1 < n && s[i + 1] == c )
            {
            fill( cls, i, i + 2, scPunctuation );
            strong = !strong;
            i += 2;
            continue;
            }
        if( c == '*' && ( em || ( i + 1 < n && s[i + 1] != ' ' ) ) )
            {
            cls[i] = scPunctuation;
            em = !em;
            i++;
            continue;
            }
        if( c == '[' )
            {
            int close = i + 1;
            while( close < n && s[close] != ']' )
                close++;
            if( close + 1 < n && s[close + 1] == '(' )
                {
                int end = close + 2;
                while( end < n && s[end] != ')' )
                    end++;
                fill( cls, i, close + 1, scConstant );
                fill( cls, close + 1, end < n ? end + 1 : n, scString );
                i = end < n ? end + 1 : n;
                continue;
                }
            }
        cls[i] = strong || em ? scEmphasis : scNone;
        i++;
        }
    return state;
}

}   // namespace

int TSyntaxEditor::scanText( int language, const char *s, int n, int state, uchar *cls )
{
    fill( cls, 0, n, scNone );
    switch( language )
        {
        case slCpp:      return scanCpp( s, n, state, cls );
        case slShell:    return scanShell( s, n, state, cls );
        case slIni:      return scanIni( s, n, state, cls );
        case slJson:     return scanJson( s, n, state, cls );
        case slMarkdown: return scanMarkdown( s, n, state, cls );
        }
    return 0;
}

int TSyntaxEditor::languageFor( const char *fileName )
{
    if( fileName == 0 )
        return slPlain;
    const char *base = fileName;
    for( const char *p = fileName; *p != 0; p++ )
        if( *p == '/' || *p == '\\' )
            base = p + 1;
    std::string b;
    for( const char *p = base; *p != 0; p++ )
        b += char( tolower( uchar( *p ) ) );
    if( b == "makefile" || b == "gnumakefile" || b == ".bashrc" || b == ".profile" )
        return slShell;
    if( b == ".env" || b.compare( 0, 5, ".env." ) == 0 )
        return slIni;
    size_t dot = b.rfind( '.' );
    if( dot == std::string::npos )
        return slPlain;
    std::string ext = b.substr( dot + 1 );
    static const char *const cpp[] =
        { "c", "h", "cpp", "cc", "cxx", "hpp", "hh", "hxx", "ino", "inl", 0 };
    static const char *const sh[] = { "sh", "bash", "zsh", "ksh", "mk", 0 };
    static const char *const ini[] = { "ini", "cfg", "conf", "env", "properties", "desktop", 0 };
    static const char *const md[] = { "md", "markdown", 0 };
    static const char *const yaml[] = { "yaml", "yml", 0 };
    int n = int( ext.size() );
    if( inList( cpp, ext.c_str(), n ) )
        return slCpp;
    if( inList( sh, ext.c_str(), n ) )
        return slShell;
    if( inList( ini, ext.c_str(), n ) )
        return slIni;
    if( ext == "json" )
        return slJson;
    if( inList( md, ext.c_str(), n ) )
        return slMarkdown;
    if( inList( yaml, ext.c_str(), n ) )
        return slYaml;
    return slPlain;
}

const char *TSyntaxEditor::languageName( int language )
{
    static const char *const names[] =
        { "Text", "C/C++", "Shell", "INI", "JSON", "Markdown", "YAML" };
    return language >= 0 && language < slCount ? names[language] : "";
}

TSyntaxEditor::TSyntaxEditor( const TRect& bounds, TScrollBar *aHScrollBar,
                              TScrollBar *aVScrollBar, TIndicator *aIndicator,
                              const char *aFileName, int aLanguage ) :
    TColoredEditor( bounds, aHScrollBar, aVScrollBar, aIndicator, aFileName ),
    language( aLanguage >= 0 ? aLanguage : languageFor( aFileName ) ),
    cachePtr( 0 ),
    cacheState( 0 ),
    cacheValid( False )
{
}

void TSyntaxEditor::setLanguage( int aLanguage )
{
    language = aLanguage;
    cacheValid = False;
    drawView();
}

// the state at the end of the line that starts at linePtr
int TSyntaxEditor::stateAfterLine( ushort linePtr )
{
    char text[maxScanLength];
    uchar cls[maxScanLength];
    int n = lineText( linePtr, text, maxScanLength );
    return scanText( language, text, n, stateBefore( linePtr ), cls );
}

int TSyntaxEditor::stateBefore( ushort linePtr )
{
    if( language != slCpp && language != slMarkdown )
        return 0;
    if( cacheValid == True && cachePtr == linePtr )
        return cacheState;
    // from the start of the text (or of the last line known before it)
    ushort p = 0;
    int state = 0;
    if( cacheValid == True && cachePtr < linePtr )
        {
        p = cachePtr;
        state = cacheState;
        }
    char text[maxScanLength];
    uchar cls[maxScanLength];
    while( p < linePtr )
        {
        int n = lineText( p, text, maxScanLength );
        state = scanText( language, text, n, state, cls );
        ushort next = nextLine( p );
        if( next == p )
            break;
        p = next;
        }
    cachePtr = linePtr;
    cacheState = state;
    cacheValid = True;
    return state;
}

void TSyntaxEditor::scanLine( ushort linePtr, const char *s, int n, uchar *cls )
{
    int state = scanText( language, s, n, stateBefore( linePtr ), cls );
    // the next line of the screen goes on from here
    cachePtr = nextLine( linePtr );
    cacheState = state;
    cacheValid = True;
}

uchar TSyntaxEditor::classColor( uchar cls, uchar normal )
{
    uchar bg = uchar( ( normal >> 4 ) & 0x07 );
    const uchar *fg = ( bg == 2 || bg == 3 || bg == 6 || bg == 7 ) ? lightColors : darkColors;
    uchar f = cls < scCount ? fg[cls] : 0xFF;
    if( f == 0xFF || f == bg )
        return normal;
    return uchar( ( normal & 0xF0 ) | f );
}

// a change may open or close a block comment: then the lines below the
// cursor are drawn again, not only the line of the cursor
void TSyntaxEditor::handleEvent( TEvent& event )
{
    Boolean edit = Boolean( event.what == evKeyDown || event.what == evCommand );
    Boolean multi = Boolean( language == slCpp || language == slMarkdown );
    int before = 0;
    ushort line = 0;
    if( edit == True && multi == True )
        {
        cacheValid = False;
        line = lineStart( curPtr );
        before = stateAfterLine( line );
        }
    TColoredEditor::handleEvent( event );
    if( edit == True )
        {
        cacheValid = False;
        if( multi == True && lineStart( curPtr ) == line && stateAfterLine( line ) != before )
            drawView();
        cacheValid = False;
        }
}

// ---- TSyntaxEditWindow

TSyntaxEditWindow::TSyntaxEditWindow( const TRect& bounds, const char *fileName, int aNumber ) :
    TWindowInit( &TSyntaxEditWindow::initFrame ),
    TEditWindow( bounds, 0, aNumber )
{
    TFileEditor *plain = editor;
    TRect r = plain->getBounds();
    TScrollBar *h = plain->hScrollBar;
    TScrollBar *v = plain->vScrollBar;
    TIndicator *plainInd = plain->indicator;
    remove( plain );
    destroy( plain );

    TIndicator *ind = new TBlockIndicator( plainInd->getBounds() );
    remove( plainInd );
    destroy( plainInd );
    insert( ind );

    if( TSyntaxEditor::languageFor( fileName ) == slYaml )
        editor = new TYamlEditor( r, h, v, ind, fileName );
    else
        editor = new TSyntaxEditor( r, h, v, ind, fileName );
    insert( editor );
}
