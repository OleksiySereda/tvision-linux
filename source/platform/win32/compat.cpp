/*------------------------------------------------------------*/
/* filename -       compat.cpp                                */
/*                                                            */
/* function(s)                                                */
/*          Windows implementations of the Borland C++        */
/*          runtime functions declared in tvcompat.h          */
/*                                                            */
/*   Paths take both / and \; drives work as in DOS.          */
/*------------------------------------------------------------*/

#include <tvcompat.h>

#include <windows.h>

#include <ctype.h>
#include <direct.h>
#include <io.h>
#include <stdio.h>
#include <sys/stat.h>

static void copyStr( char *dst, const char *src, size_t len, size_t size )
{
    if( dst == 0 )
        return;
    if( len >= size )
        len = size - 1;
    memcpy( dst, src, len );
    dst[len] = '\0';
}

static bool isSep( char c )
{
    return c == '/' || c == '\\';
}

// the last / or \ of path, or 0
static const char *lastSep( const char *path )
{
    const char *s = 0;
    for( const char *p = path; *p; p++ )
        if( isSep( *p ) )
            s = p;
    return s;
}

/* ---- findfirst / findnext --------------------------------------------- */

static void packTime( const FILETIME &ft, unsigned short &dosTime,
                      unsigned short &dosDate )
{
    FILETIME local;
    FileTimeToLocalFileTime( &ft, &local );
    WORD d = 0, t = 0;
    FileTimeToDosDateTime( &local, &d, &t );
    dosDate = d;
    dosTime = t;
}

// takes the found entry if it is one that was asked for, else -1
static int take( ffblk *ff, const WIN32_FIND_DATAA &fd )
{
    const char *name = fd.cFileName;
    if( strcmp( name, "." ) == 0 )
        return -1;
    unsigned char attr = FA_ARCH;
    if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        attr = FA_DIREC;
    if( fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN )
        attr |= FA_HIDDEN;
    if( fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM )
        attr |= FA_SYSTEM;
    if( fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY )
        attr |= FA_RDONLY;

    // DOS semantics: directories / hidden files only when asked for
    if( (attr & FA_DIREC) && !(ff->ff_reqattr & FA_DIREC) )
        return -1;
    if( (attr & (FA_HIDDEN | FA_SYSTEM)) && !(ff->ff_reqattr & FA_HIDDEN) )
        return -1;

    ff->ff_attrib = attr;
    ff->ff_fsize = long( fd.nFileSizeLow );
    packTime( fd.ftLastWriteTime, ff->ff_ftime, ff->ff_fdate );
    copyStr( ff->ff_name, name, strlen( name ), sizeof( ff->ff_name ) );
    return 0;
}

static int scanNext( ffblk *ff, bool first, const WIN32_FIND_DATAA *fd0 )
{
    HANDLE h = (HANDLE)ff->ff_dir;
    if( h == 0 )
        return -1;
    WIN32_FIND_DATAA fd;
    if( first && take( ff, *fd0 ) == 0 )
        return 0;
    while( FindNextFileA( h, &fd ) )
        if( take( ff, fd ) == 0 )
            return 0;
    FindClose( h );
    ff->ff_dir = 0;
    return -1;
}

int findfirst( const char *path, ffblk *ff, int attrib )
{
    memset( ff, 0, sizeof( *ff ) );
    const char *sep = lastSep( path );
    const char *base = sep ? sep + 1 : path;
    if( sep == 0 && path[0] != 0 && path[1] == ':' )
        base = path + 2;                // "C:*.*"
    copyStr( ff->ff_path, path, size_t( base - path ), sizeof( ff->ff_path ) );
    copyStr( ff->ff_pattern, base, strlen( base ), sizeof( ff->ff_pattern ) );
    if( ff->ff_pattern[0] == '\0' )
        strcpy( ff->ff_pattern, "*" );
    ff->ff_reqattr = attrib;

    char full[MAXPATH * 2];
    snprintf( full, sizeof( full ), "%s%s", ff->ff_path, ff->ff_pattern );
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA( full, &fd );
    if( h == INVALID_HANDLE_VALUE )
        return -1;
    ff->ff_dir = h;
    return scanNext( ff, true, &fd );
}

int findnext( ffblk *ff )
{
    return scanNext( ff, false, 0 );
}

/* ---- fnsplit / fnmerge ------------------------------------------------ */

int fnsplit( const char *path, char *drive, char *dir, char *name, char *ext )
{
    int flags = 0;
    if( drive )
        drive[0] = '\0';
    if( path[0] != 0 && path[1] == ':' )
        {
        copyStr( drive, path, 2, MAXDRIVE );
        flags |= DRIVE;
        path += 2;
        }

    const char *sep = lastSep( path );
    const char *base = sep ? sep + 1 : path;
    copyStr( dir, path, size_t( base - path ), MAXDIR );
    if( base != path )
        flags |= DIRECTORY;

    const char *dot = strrchr( base, '.' );
    if( dot == base || ( dot && strcmp( base, ".." ) == 0 ) )
        dot = 0;
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

void fnmerge( char *path, const char *drive, const char *dir,
              const char *name, const char *ext )
{
    path[0] = '\0';
    if( drive && *drive )
        {
        strcat( path, drive );
        if( path[strlen( path ) - 1] != ':' )
            strcat( path, ":" );
        }
    if( dir && *dir )
        {
        strcat( path, dir );
        if( !isSep( path[strlen( path ) - 1] ) )
            strcat( path, "\\" );
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
    if( path[0] != 0 && path[1] == ':' && isSep( path[2] ) )
        return 3;
    return isSep( path[0] ) ? 1 : 0;
}

// getcwd() with '/' (unistd.h)
char *tv_getcwd( char *buf, int size )
{
    if( _getcwd( buf, size ) == 0 )
        return 0;
    for( char *p = buf; *p; p++ )
        if( *p == '\\' )
            *p = '/';
    return buf;
}

// 0 = A:, as in DOS
int getdisk()
{
    return _getdrive() - 1;
}

// returns the number of drives, as DOS does
int setdisk( int drive )
{
    _chdrive( drive + 1 );
    return 26;
}

// DOS semantics: the current directory of drive (0 = the current drive,
// 1 = A:) without the drive and the leading separator
int getcurdir( int drive, char *dir )
{
    char buf[MAXPATH];
    if( _getdcwd( drive, buf, sizeof( buf ) ) == 0 )
        {
        dir[0] = '\0';
        return -1;
        }
    const char *p = buf;
    if( p[0] != 0 && p[1] == ':' )
        p += 2;
    if( isSep( *p ) )
        p++;
    strcpy( dir, p );
    return 0;
}

/* strupr, stricmp, itoa, filelength and the like are in the Windows C
   runtime under the same names */
