/*------------------------------------------------------------*/
/* filename -       tappsets.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TAppSettings member functions             */
/*                                                            */
/*   The file is kept as its lines, so that save() writes     */
/*   back comments and unknown lines where they were.         */
/*------------------------------------------------------------*/

#define Uses_TAppSettings
#define Uses_TView
#define Uses_TGroup
#define Uses_TRect
#define Uses_TProgram
#define Uses_TPalette
#define Uses_TMenuItem
#define Uses_TKeys
#include <tvaddons.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <algorithm>
#if defined( _WIN32 )
#include <direct.h>
#endif

static std::string trim( const std::string& s )
{
    size_t a = 0, b = s.size();
    while( a < b && isspace( uchar( s[a] ) ) )
        a++;
    while( b > a && isspace( uchar( s[b - 1] ) ) )
        b--;
    return s.substr( a, b - a );
}

static std::string slashes( std::string s )
{
    std::replace( s.begin(), s.end(), '\\', '/' );
    return s;
}

std::string TAppSettings::configDir( const char *name )
{
    std::string base;
#if defined( _WIN32 )
    const char *a = getenv( "APPDATA" );
    if( a != 0 && *a != 0 )
        base = slashes( a );
#else
    const char *x = getenv( "XDG_CONFIG_HOME" );
    const char *h = getenv( "HOME" );
    if( x != 0 && *x == '/' )
        base = x;
    else if( h != 0 && *h != 0 )
        base = std::string( h ) + "/.config";
#endif
    if( base.empty() )
        return std::string();
    return base + "/" + name;
}

// every directory of path that is missing
static void makeDirs( const std::string& dir )
{
    for( size_t i = 1; i <= dir.size(); i++ )
        if( i == dir.size() || dir[i] == '/' )
            {
            std::string d = dir.substr( 0, i );
            if( d.size() == 2 && d[1] == ':' )
                continue;               // C:
#if defined( _WIN32 )
            _mkdir( d.c_str() );
#else
            mkdir( d.c_str(), 0755 );
#endif
            }
}

TAppSettings::TAppSettings( const char *name, const char *aFileName ) :
    maxRecent( 8 )
{
    if( aFileName != 0 )
        path = slashes( aFileName );
    else
        {
        std::string d = configDir( name );
        path = d.empty() ? std::string( name ) + ".ini" : d + "/settings.ini";
        }
    load();
}

Boolean TAppSettings::load()
{
    lines.clear();
    FILE *f = fopen( path.c_str(), "r" );
    if( f == 0 )
        return False;
    std::string section;
    char buf[4096];
    while( fgets( buf, sizeof( buf ), f ) != 0 )
        {
        std::string raw( buf );
        while( !raw.empty() && ( raw.back() == '\n' || raw.back() == '\r' ) )
            raw.pop_back();
        std::string t = trim( raw );
        Line l;
        l.kind = lnOther;
        l.section = section;
        l.raw = raw;
        size_t eq = t.find( '=' );
        if( t.size() >= 2 && t[0] == '[' && t.back() == ']' )
            {
            section = trim( t.substr( 1, t.size() - 2 ) );
            l.kind = lnSection;
            l.section = section;
            }
        else if( !t.empty() && t[0] != ';' && t[0] != '#' && eq != std::string::npos )
            {
            l.kind = lnPair;
            l.key = trim( t.substr( 0, eq ) );
            l.value = trim( t.substr( eq + 1 ) );
            }
        lines.push_back( l );
        }
    fclose( f );
    return True;
}

Boolean TAppSettings::save()
{
    size_t slash = path.rfind( '/' );
    if( slash != std::string::npos && slash > 0 )
        makeDirs( path.substr( 0, slash ) );
    std::string tmp = path + ".new";
    FILE *f = fopen( tmp.c_str(), "w" );
    if( f == 0 )
        return False;
    for( const Line& l : lines )
        if( l.kind == lnSection )
            fprintf( f, "[%s]\n", l.section.c_str() );
        else if( l.kind == lnPair )
            fprintf( f, "%s=%s\n", l.key.c_str(), l.value.c_str() );
        else
            fprintf( f, "%s\n", l.raw.c_str() );
    Boolean ok = Boolean( ferror( f ) == 0 );
    if( fclose( f ) != 0 )
        ok = False;
    if( ok == False )
        {
        ::remove( tmp.c_str() );
        return False;
        }
    if( rename( tmp.c_str(), path.c_str() ) != 0 )
        {
        // Windows: rename() does not replace a file
        ::remove( path.c_str() );
        if( rename( tmp.c_str(), path.c_str() ) != 0 )
            return False;
        }
    return True;
}

int TAppSettings::find( const std::string& section, const std::string& key ) const
{
    for( size_t i = 0; i < lines.size(); i++ )
        if( lines[i].kind == lnPair && lines[i].section == section && lines[i].key == key )
            return int( i );
    return -1;
}

Boolean TAppSettings::has( const char *section, const char *key ) const
{
    return Boolean( find( section, key ) >= 0 );
}

std::string TAppSettings::get( const char *section, const char *key, const char *def ) const
{
    int i = find( section, key );
    return i >= 0 ? lines[size_t( i )].value : std::string( def != 0 ? def : "" );
}

long TAppSettings::getInt( const char *section, const char *key, long def ) const
{
    std::string v = get( section, key );
    char *end;
    long n = strtol( v.c_str(), &end, 10 );
    return v.empty() || *end != 0 ? def : n;
}

Boolean TAppSettings::getBool( const char *section, const char *key, Boolean def ) const
{
    std::string v = get( section, key );
    for( char& c : v )
        c = char( tolower( uchar( c ) ) );
    if( v == "1" || v == "yes" || v == "true" || v == "on" )
        return True;
    if( v == "0" || v == "no" || v == "false" || v == "off" )
        return False;
    return def;
}

void TAppSettings::set( const char *section, const char *key, const char *value )
{
    int i = find( section, key );
    if( i >= 0 )
        {
        lines[size_t( i )].value = value != 0 ? value : "";
        return;
        }
    Line l;
    l.kind = lnPair;
    l.section = section;
    l.key = key;
    l.value = value != 0 ? value : "";
    // after the last line of the section, or a new section at the end
    int last = -1;
    for( size_t k = 0; k < lines.size(); k++ )
        if( lines[k].section == section && lines[k].kind != lnOther )
            last = int( k );
    if( last < 0 )
        {
        if( *section != 0 )
            {
            Line h;
            h.kind = lnSection;
            h.section = section;
            lines.push_back( h );
            }
        else
            {
            // keys without a section go before the first section
            size_t k = 0;
            while( k < lines.size() && lines[k].kind != lnSection )
                k++;
            lines.insert( lines.begin() + long( k ), l );
            return;
            }
        lines.push_back( l );
        }
    else
        lines.insert( lines.begin() + last + 1, l );
}

void TAppSettings::setInt( const char *section, const char *key, long value )
{
    set( section, key, std::to_string( value ).c_str() );
}

void TAppSettings::setBool( const char *section, const char *key, Boolean value )
{
    set( section, key, value == True ? "yes" : "no" );
}

void TAppSettings::remove( const char *section, const char *key )
{
    int i = find( section, key );
    if( i >= 0 )
        lines.erase( lines.begin() + i );
}

void TAppSettings::removeSection( const char *section )
{
    lines.erase( std::remove_if( lines.begin(), lines.end(),
                     [section]( const Line& l ) { return l.section == section; } ),
                 lines.end() );
}

// ---- windows

void TAppSettings::saveWindow( const char *name, TView *view )
{
    if( view == 0 )
        return;
    std::string s = std::string( "window." ) + name;
    TRect r = view->getBounds();
    setInt( s.c_str(), "x", r.a.x );
    setInt( s.c_str(), "y", r.a.y );
    setInt( s.c_str(), "width", r.b.x - r.a.x );
    setInt( s.c_str(), "height", r.b.y - r.a.y );
}

Boolean TAppSettings::restoreWindow( const char *name, TView *view )
{
    std::string s = std::string( "window." ) + name;
    if( view == 0 || !has( s.c_str(), "width" ) || !has( s.c_str(), "height" ) )
        return False;
    int x = int( getInt( s.c_str(), "x" ) ), y = int( getInt( s.c_str(), "y" ) );
    int w = int( getInt( s.c_str(), "width" ) ), h = int( getInt( s.c_str(), "height" ) );
    if( view->owner != 0 )
        {
        // a smaller screen than last time: the window still fits
        TPoint room = view->owner->size;
        w = std::min( w, int( room.x ) );
        h = std::min( h, int( room.y ) );
        x = std::max( 0, std::min( x, room.x - w ) );
        y = std::max( 0, std::min( y, room.y - h ) );
        }
    TRect r( x, y, x + w, y + h );
    view->locate( r );
    return True;
}

// ---- recent files

std::vector<std::string> TAppSettings::recent() const
{
    std::vector<std::string> v;
    for( int i = 1; i <= maxRecent; i++ )
        {
        std::string f = get( "recent", ( "file" + std::to_string( i ) ).c_str() );
        if( !f.empty() )
            v.push_back( f );
        }
    return v;
}

static void writeRecent( TAppSettings *s, const std::vector<std::string>& v )
{
    s->removeSection( "recent" );
    for( size_t i = 0; i < v.size(); i++ )
        s->set( "recent", ( "file" + std::to_string( i + 1 ) ).c_str(), v[i].c_str() );
}

void TAppSettings::addRecent( const char *file )
{
    if( file == 0 || *file == 0 )
        return;
    std::vector<std::string> v = recent();
    v.erase( std::remove( v.begin(), v.end(), std::string( file ) ), v.end() );
    v.insert( v.begin(), file );
    if( int( v.size() ) > maxRecent )
        v.resize( size_t( std::max( 0, maxRecent ) ) );
    writeRecent( this, v );
}

void TAppSettings::removeRecent( const char *file )
{
    std::vector<std::string> v = recent();
    v.erase( std::remove( v.begin(), v.end(), std::string( file ) ), v.end() );
    writeRecent( this, v );
}

TMenuItem *TAppSettings::recentMenu( ushort firstCommand, TMenuItem *next ) const
{
    std::vector<std::string> v = recent();
    for( int i = int( v.size() ) - 1; i >= 0; i-- )
        {
        std::string f = v[size_t( i )];
        if( f.size() > 40 )
            f = "..." + f.substr( f.size() - 37 );
        // a '~' in the name would start a highlighted letter
        std::replace( f.begin(), f.end(), '~', '-' );
        std::string label = i < 9 ? "~" + std::to_string( i + 1 ) + "~ " + f : "   " + f;
        next = new TMenuItem( label.c_str(), ushort( firstCommand + i ), kbNoKey,
                              hcNoContext, 0, next );
        }
    return next;
}

// ---- THistory lists

void TAppSettings::saveHistory( uchar id )
{
    std::string s = "history." + std::to_string( id );
    removeSection( s.c_str() );
    int n = historyCount( id );
    for( int i = 0; i < n; i++ )
        set( s.c_str(), ( "item" + std::to_string( i + 1 ) ).c_str(), historyStr( id, i ) );
}

void TAppSettings::loadHistory( uchar id )
{
    std::string s = "history." + std::to_string( id );
    for( int i = 1; ; i++ )
        {
        std::string key = "item" + std::to_string( i );
        if( !has( s.c_str(), key.c_str() ) )
            break;
        historyAdd( id, get( s.c_str(), key.c_str() ).c_str() );
        }
}

// ---- colours

void TAppSettings::savePalette()
{
    if( TProgram::application == 0 )
        return;
    TPalette& p = TProgram::application->getPalette();
    int n = uchar( p[0] );
    std::string hex;
    char b[4];
    for( int i = 1; i <= n; i++ )
        {
        snprintf( b, sizeof( b ), "%02X", uchar( p[i] ) );
        hex += b;
        }
    set( "colors", "palette", hex.c_str() );
}

// the program redraws itself after it (TProgram::application->redraw())
Boolean TAppSettings::loadPalette()
{
    if( TProgram::application == 0 )
        return False;
    std::string hex = get( "colors", "palette" );
    TPalette& p = TProgram::application->getPalette();
    int n = uchar( p[0] );
    if( int( hex.size() ) != 2 * n )
        return False;
    for( size_t i = 0; i < hex.size(); i++ )
        if( !isxdigit( uchar( hex[i] ) ) )
            return False;
    for( int i = 1; i <= n; i++ )
        p[i] = char( strtol( hex.substr( size_t( 2 * ( i - 1 ) ), 2 ).c_str(), 0, 16 ) );
    return True;
}
