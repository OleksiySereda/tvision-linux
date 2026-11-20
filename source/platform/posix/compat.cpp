/*------------------------------------------------------------*/
/* filename -       compat.cpp                                */
/*                                                            */
/* function(s)                                                */
/*          POSIX implementations of the Borland C++ runtime  */
/*          functions declared in tvcompat.h                  */
/*------------------------------------------------------------*/

#include <tvcompat.h>

#include <ctype.h>
#include <dirent.h>
#include <fnmatch.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static void copyStr( char *dst, const char *src, size_t len, size_t size )
{
    if( dst == 0 )
        return;
    if( len >= size )
        len = size - 1;
    memcpy( dst, src, len );
    dst[len] = '\0';
}

/* ---- findfirst / findnext --------------------------------------------- */

static void packTime( time_t t, unsigned short &dosTime,
                      unsigned short &dosDate )
{
    struct tm tmv;
    localtime_r( &t, &tmv );
    int year = tmv.tm_year + 1900 - 1980;
    if( year < 0 ) year = 0;
    if( year > 127 ) year = 127;
    dosTime = (unsigned short)( (tmv.tm_hour << 11) | (tmv.tm_min << 5) |
                                (tmv.tm_sec / 2) );
    dosDate = (unsigned short)( (year << 9) | ((tmv.tm_mon + 1) << 5) |
                                tmv.tm_mday );
}

static int scanNext( ffblk *ff )
{
    DIR *dir = (DIR *)ff->ff_dir;
    if( dir == 0 )
        return -1;
    struct dirent *de;
    while( (de = readdir( dir )) != 0 )
        {
        const char *name = de->d_name;
        if( strcmp( name, "." ) == 0 )
            continue;
        if( strcmp( name, ".." ) == 0 && strcmp( ff->ff_path, "/" ) == 0 )
            continue;
        if( fnmatch( ff->ff_pattern, name, 0 ) != 0 )
            continue;

        char full[MAXPATH * 2];
        snprintf( full, sizeof( full ), "%s%s", ff->ff_path, name );
        struct stat st;
        if( stat( full, &st ) != 0 )
            continue;

        unsigned char attr = FA_ARCH;
        if( S_ISDIR( st.st_mode ) )
            attr = FA_DIREC;
        if( name[0] == '.' && strcmp( name, ".." ) != 0 )
            attr |= FA_HIDDEN;
        if( access( full, W_OK ) != 0 )
            attr |= FA_RDONLY;

        // DOS semantics: directories / hidden files only when asked for
        if( (attr & FA_DIREC) && !(ff->ff_reqattr & FA_DIREC) )
            continue;
        if( (attr & FA_HIDDEN) && !(ff->ff_reqattr & FA_HIDDEN) )
            continue;

        ff->ff_attrib = attr;
        ff->ff_fsize = long( st.st_size );
        packTime( st.st_mtime, ff->ff_ftime, ff->ff_fdate );
        copyStr( ff->ff_name, name, strlen( name ), sizeof( ff->ff_name ) );
        return 0;
        }
    closedir( dir );
    ff->ff_dir = 0;
    return -1;
}

int findfirst( const char *path, ffblk *ff, int attrib )
{
    memset( ff, 0, sizeof( *ff ) );
    const char *slash = strrchr( path, '/' );
    if( slash != 0 )
        {
        copyStr( ff->ff_path, path, size_t( slash - path ) + 1,
                 sizeof( ff->ff_path ) );
        copyStr( ff->ff_pattern, slash + 1, strlen( slash + 1 ),
                 sizeof( ff->ff_pattern ) );
        }
    else
        {
        strcpy( ff->ff_path, "./" );
        copyStr( ff->ff_pattern, path, strlen( path ),
                 sizeof( ff->ff_pattern ) );
        }
    // DOS "*.*" means "every file", not "files containing a dot"
    if( strcmp( ff->ff_pattern, "*.*" ) == 0 || ff->ff_pattern[0] == '\0' )
        strcpy( ff->ff_pattern, "*" );
    ff->ff_reqattr = attrib;
    ff->ff_dir = opendir( ff->ff_path );
    if( ff->ff_dir == 0 )
        return -1;
    return scanNext( ff );
}

int findnext( ffblk *ff )
{
    return scanNext( ff );
}

/* ---- fnsplit / fnmerge ------------------------------------------------ */

int fnsplit( const char *path, char *drive, char *dir, char *name, char *ext )
{
    int flags = 0;
    if( drive )
        drive[0] = '\0';

    const char *slash = strrchr( path, '/' );
    const char *base = slash ? slash + 1 : path;
    copyStr( dir, path, size_t( base - path ), MAXDIR );
    if( base != path )
        flags |= DIRECTORY;

    const char *dot = strrchr( base, '.' );
    if( dot == base || ( dot && strcmp( base, ".." ) == 0 ) )
        dot = 0;                        // ".profile" and ".." have no extension
    size_t nameLen = dot ? size_t( dot - base ) : strlen( base );
    copyStr( name, base, nameLen, MAXFILE );
    if( nameLen )
        flags |= FILENAME;
    if( dot )
        {
        copyStr( ext, dot, strlen( dot ), MAXEXT );
        flags |= EXTENSION;
        }
    else if( ext )
        ext[0] = '\0';
    if( strpbrk( base, "*?" ) )
        flags |= WILDCARDS;
    return flags;
}

void fnmerge( char *path, const char *, const char *dir,
              const char *name, const char *ext )
{
    path[0] = '\0';
    if( dir && *dir )
        {
        strcat( path, dir );
        if( path[strlen( path ) - 1] != '/' )
            strcat( path, "/" );
        }
    if( name )
        strcat( path, name );
    if( ext && *ext )
        {
        if( *ext != '.' )
            strcat( path, "." );
        strcat( path, ext );
        }
}

/* ---- drives / current directory -------------------------------------- */

int rootLength( const char *path )
{
    return path[0] == '/' ? 1 : 0;
}

int getdisk()
{
    return 0;
}

int setdisk( int )
{
    return 1;
}

// DOS semantics: the current directory without the leading separator.
int getcurdir( int, char *dir )
{
    char buf[MAXPATH];
    if( getcwd( buf, sizeof( buf ) ) == 0 )
        {
        dir[0] = '\0';
        return -1;
        }
    strcpy( dir, buf[0] == '/' ? buf + 1 : buf );
    return 0;
}

/* ---- string helpers --------------------------------------------------- */

char *strupr( char *s )
{
    for( char *p = s; *p; p++ )
        *p = char( toupper( (unsigned char)*p ) );
    return s;
}

char *strlwr( char *s )
{
    for( char *p = s; *p; p++ )
        *p = char( tolower( (unsigned char)*p ) );
    return s;
}

int stricmp( const char *s1, const char *s2 )
{
    return strcasecmp( s1, s2 );
}

int strnicmp( const char *s1, const char *s2, size_t n )
{
    return strncasecmp( s1, s2, n );
}

char *ultoa( unsigned long value, char *buf, int radix )
{
    char tmp[sizeof( long ) * 8 + 1];
    int i = 0;
    do  {
        int d = int( value % radix );
        tmp[i++] = char( d < 10 ? '0' + d : 'a' + d - 10 );
        value /= radix;
        } while( value != 0 );
    for( int j = 0; j < i; j++ )
        buf[j] = tmp[i - 1 - j];
    buf[i] = '\0';
    return buf;
}

char *ltoa( long value, char *buf, int radix )
{
    if( value < 0 && radix == 10 )
        {
        buf[0] = '-';
        ultoa( (unsigned long)( -value ), buf + 1, radix );
        return buf;
        }
    return ultoa( (unsigned long)value, buf, radix );
}

char *itoa( int value, char *buf, int radix )
{
    if( radix != 10 )
        return ultoa( (unsigned)value, buf, radix );
    return ltoa( value, buf, radix );
}

/* ---- io.h ------------------------------------------------------------- */

long filelength( int fd )
{
    struct stat st;
    if( fstat( fd, &st ) != 0 )
        return -1;
    return long( st.st_size );
}
