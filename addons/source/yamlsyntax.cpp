/*------------------------------------------------------------*/
/* filename -       yamlsyntax.cpp                            */
/*                                                            */
/* function(s)                                                */
/*                  yamlScanLine, yamlOpensBlock              */
/*                                                            */
/*   A line-by-line classifier for YAML.  It does not parse:  */
/*   it decides what each character looks like, which is all  */
/*   that colouring needs.  Text of block scalars is detected */
/*   by the editor (TYamlEditor::inBlockScalar), because that */
/*   needs the lines above.                                   */
/*------------------------------------------------------------*/

#include <tvaddons.h>

#include <string.h>

static Boolean isBlank( char c )
{
    return Boolean( c == ' ' || c == '\t' );
}

static void mark( uchar *cls, int from, int to, uchar t )
{
    for( int i = from; i < to; i++ )
        cls[i] = t;
}

// end of a quoted scalar that starts at i (one past the closing quote,
// or n if it is not closed on this line)
static int quotedEnd( const char *s, int n, int i )
{
    char q = s[i];
    for( i++; i < n; i++ )
        {
        if( q == '"' && s[i] == '\\' )
            i++;
        else if( s[i] == q )
            {
            if( q == '\'' && i + 1 < n && s[i+1] == '\'' )
                i++;
            else
                return i + 1;
            }
        }
    return n;
}

static Boolean equalsAny( const char *s, int n, const char *const *words )
{
    for( ; *words != 0; words++ )
        if( int( strlen( *words ) ) == n && strncmp( s, *words, n ) == 0 )
            return True;
    return False;
}

static Boolean isDigit( char c )
{
    return Boolean( c >= '0' && c <= '9' );
}

// numbers in the forms YAML knows: 12, -3, +4, 1_000, 3.14, 1e-5,
// 0x1F, 0o17, .5, .inf, -.Inf, .nan
static Boolean isNumber( const char *s, int n )
{
    static const char *const special[] =
        { ".inf", ".Inf", ".INF", "-.inf", "-.Inf", "-.INF", "+.inf", "+.Inf",
          "+.INF", ".nan", ".NaN", ".NAN", 0 };
    if( equalsAny( s, n, special ) )
        return True;
    int i = 0;
    if( i < n && ( s[i] == '-' || s[i] == '+' ) )
        i++;
    if( i + 1 < n && s[i] == '0' && ( s[i+1] == 'x' || s[i+1] == 'o' ) )
        {
        Boolean hex = Boolean( s[i+1] == 'x' );
        i += 2;
        if( i >= n )
            return False;
        for( ; i < n; i++ )
            if( !( isDigit( s[i] ) || s[i] == '_' ||
                   ( hex && ( ( s[i] >= 'a' && s[i] <= 'f' ) || ( s[i] >= 'A' && s[i] <= 'F' ) ) ) ) ||
                ( !hex && s[i] > '7' ) )
                return False;
        return True;
        }
    int digits = 0;
    for( ; i < n && ( isDigit( s[i] ) || s[i] == '_' ); i++ )
        digits += isDigit( s[i] );
    if( i < n && s[i] == '.' )
        for( i++; i < n && ( isDigit( s[i] ) || s[i] == '_' ); i++ )
            digits += isDigit( s[i] );
    if( digits == 0 )
        return False;
    if( i < n && ( s[i] == 'e' || s[i] == 'E' ) )
        {
        i++;
        if( i < n && ( s[i] == '-' || s[i] == '+' ) )
            i++;
        if( i >= n || !isDigit( s[i] ) )
            return False;
        while( i < n && isDigit( s[i] ) )
            i++;
        }
    return Boolean( i == n );
}

static Boolean isConstant( const char *s, int n )
{
    static const char *const words[] =
        { "true", "True", "TRUE", "false", "False", "FALSE",
          "yes", "Yes", "YES", "no", "No", "NO", "on", "On", "ON", "off", "Off", "OFF",
          "null", "Null", "NULL", "~", 0 };
    return Boolean( equalsAny( s, n, words ) || isNumber( s, n ) );
}

// Position of the ':' that ends a mapping key starting at i, or -1.
static int keyColon( const char *s, int n, int i )
{
    if( i >= n || strchr( "[{#|>&*!%@`,", s[i] ) )
        return -1;
    if( s[i] == '"' || s[i] == '\'' )
        {
        int j = quotedEnd( s, n, i );
        while( j < n && isBlank( s[j] ) )
            j++;
        return j < n && s[j] == ':' && ( j + 1 == n || isBlank( s[j+1] ) ) ? j : -1;
        }
    for( int j = i; j < n; j++ )
        {
        if( s[j] == '#' && j > i && isBlank( s[j-1] ) )
            return -1;
        if( s[j] == ':' && ( j + 1 == n || isBlank( s[j+1] ) ) )
            return j;
        }
    return -1;
}

// a plain scalar from i: to the end of the line, a comment, or in a flow
// collection one of , [ ] { } or ": "
static int plainEnd( const char *s, int n, int i, int depth )
{
    int j = i;
    for( ; j < n; j++ )
        {
        if( s[j] == '#' && j > i && isBlank( s[j-1] ) )
            break;
        if( depth > 0 && strchr( ",[]{}", s[j] ) )
            break;
        if( depth > 0 && s[j] == ':' && ( j + 1 == n || isBlank( s[j+1] ) ||
                                          strchr( ",[]{}", s[j+1] ) ) )
            break;
        }
    while( j > i && isBlank( s[j-1] ) )
        j--;
    return j;
}

static void scanValue( const char *s, int n, int i, uchar *cls )
{
    int depth = 0;
    Boolean start = True;           // nothing of the value seen yet
    while( i < n )
        {
        char c = s[i];
        if( isBlank( c ) )
            {
            i++;
            continue;
            }
        if( c == '#' && ( i == 0 || isBlank( s[i-1] ) ) )
            {
            mark( cls, i, n, ytComment );
            return;
            }
        if( c == '"' || c == '\'' )
            {
            int j = quotedEnd( s, n, i );
            int k = j;
            while( k < n && isBlank( s[k] ) )
                k++;
            // "key": value inside a flow mapping
            Boolean key = Boolean( depth > 0 && k < n && s[k] == ':' );
            mark( cls, i, j, key ? ytKey : ytString );
            i = j;
            start = False;
            continue;
            }
        if( c == '&' || c == '*' || c == '!' )
            {
            int j = i + 1;
            while( j < n && !isBlank( s[j] ) && !( depth > 0 && strchr( ",[]{}", s[j] ) ) )
                j++;
            mark( cls, i, j, ytAnchor );
            i = j;
            continue;
            }
        if( depth == 0 && start && ( c == '|' || c == '>' ) )
            {
            int j = i + 1;
            while( j < n && ( s[j] == '-' || s[j] == '+' || isDigit( s[j] ) ) )
                j++;
            mark( cls, i, j, ytPunct );
            i = j;
            start = False;
            continue;
            }
        if( c == '[' || c == '{' )
            {
            cls[i++] = ytPunct;
            depth++;
            start = False;
            continue;
            }
        if( depth > 0 && ( c == ']' || c == '}' || c == ',' ) )
            {
            cls[i++] = ytPunct;
            if( c != ',' )
                depth--;
            continue;
            }
        if( depth > 0 && c == ':' )
            {
            cls[i++] = ytPunct;
            continue;
            }
        int j = plainEnd( s, n, i, depth );
        if( j == i )
            j = i + 1;              // a stray ] or } outside a collection
        int k = j;
        while( k < n && isBlank( s[k] ) )
            k++;
        if( depth > 0 && k < n && s[k] == ':' )
            mark( cls, i, j, ytKey );
        else if( isConstant( s + i, j - i ) )
            mark( cls, i, j, ytConstant );
        i = j;
        start = False;
        }
}

void yamlScanLine( const char *s, int n, Boolean inBlock, uchar *cls )
{
    mark( cls, 0, n, inBlock ? ytBlock : ytPlain );
    if( inBlock || n == 0 )
        return;

    int i = 0;
    if( n >= 3 && ( strncmp( s, "---", 3 ) == 0 || strncmp( s, "...", 3 ) == 0 ) &&
        ( n == 3 || isBlank( s[3] ) ) )
        {
        mark( cls, 0, 3, ytDocument );
        i = 3;
        }

    // "- " items, possibly nested ("- - x"), and the "? " of complex keys
    for( ;; )
        {
        while( i < n && isBlank( s[i] ) )
            i++;
        if( i < n && ( s[i] == '-' || s[i] == '?' ) && ( i + 1 == n || isBlank( s[i+1] ) ) )
            cls[i++] = ytPunct;
        else
            break;
        }

    // anchors and tags may come before a key: "&base key: value"
    while( i < n && ( s[i] == '&' || s[i] == '!' ) )
        {
        int j = i;
        while( j < n && !isBlank( s[j] ) )
            j++;
        mark( cls, i, j, ytAnchor );
        while( j < n && isBlank( s[j] ) )
            j++;
        i = j;
        }

    int colon = keyColon( s, n, i );
    if( colon >= 0 )
        {
        int e = colon;
        while( e > i && isBlank( s[e-1] ) )
            e--;
        mark( cls, i, e, ytKey );
        cls[colon] = ytPunct;
        i = colon + 1;
        }
    scanValue( s, n, i, cls );
}

Boolean yamlOpensBlock( const char *s, int n )
{
    if( n <= 0 )
        return False;
    uchar stackBuf[512];
    uchar *cls = n <= int( sizeof( stackBuf ) ) ? stackBuf : new uchar[n];
    yamlScanLine( s, n, False, cls );
    int j = n - 1;
    while( j >= 0 && ( cls[j] == ytComment || isBlank( s[j] ) ) )
        j--;
    while( j >= 0 && ( s[j] == '-' || s[j] == '+' || isDigit( s[j] ) ) && cls[j] == ytPunct )
        j--;
    Boolean result = Boolean( j >= 0 && ( s[j] == '|' || s[j] == '>' ) && cls[j] == ytPunct );
    if( cls != stackBuf )
        delete[] cls;
    return result;
}
