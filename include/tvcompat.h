/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   TVCOMPAT.H                                                            */
/*                                                                         */
/*   Replacements for the Borland C++ / DOS runtime pieces Turbo Vision    */
/*   relies on (dir.h, dos.h, mem.h, string extensions), implemented on    */
/*   top of POSIX.  Included by ttypes.h, so available everywhere.         */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if !defined( __TVCOMPAT_H )
#define __TVCOMPAT_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* ---- dir.h ------------------------------------------------------------ */

#define MAXPATH     512
#define MAXDRIVE    3
#define MAXDIR      512
#define MAXFILE     256
#define MAXEXT      256

#define WILDCARDS   0x01
#define EXTENSION   0x02
#define FILENAME    0x04
#define DIRECTORY   0x08
#define DRIVE       0x10

#define FA_NORMAL   0x00
#define FA_RDONLY   0x01
#define FA_HIDDEN   0x02
#define FA_SYSTEM   0x04
#define FA_LABEL    0x08
#define FA_DIREC    0x10
#define FA_ARCH     0x20

struct ffblk
{
    unsigned char   ff_attrib;
    unsigned short  ff_ftime;       // DOS packed time
    unsigned short  ff_fdate;       // DOS packed date
    long            ff_fsize;
    char            ff_name[MAXFILE+MAXEXT];

    // private search state
    void           *ff_dir;
    int             ff_reqattr;
    char            ff_path[MAXPATH];
    char            ff_pattern[MAXFILE+MAXEXT];
};

int findfirst( const char *path, ffblk *ff, int attrib );
int findnext( ffblk *ff );

int fnsplit( const char *path, char *drive, char *dir, char *name, char *ext );
void fnmerge( char *path, const char *drive, const char *dir,
              const char *name, const char *ext );

int getdisk();
int setdisk( int drive );
int getcurdir( int drive, char *dir );

/* ---- dos.h ------------------------------------------------------------ */

struct ftime
{
    unsigned ft_tsec  : 5;
    unsigned ft_min   : 6;
    unsigned ft_hour  : 5;
    unsigned ft_day   : 5;
    unsigned ft_month : 4;
    unsigned ft_year  : 7;
};

/* ---- mem.h / string.h / stdlib.h extensions --------------------------- */

inline void movmem( const void *src, void *dest, unsigned len )
{
    memmove( dest, src, len );
}

inline void setmem( void *dest, unsigned len, char value )
{
    memset( dest, value, len );
}

char *strupr( char *s );
char *strlwr( char *s );
int stricmp( const char *s1, const char *s2 );
int strnicmp( const char *s1, const char *s2, size_t n );
char *itoa( int value, char *buf, int radix );
char *ltoa( long value, char *buf, int radix );
char *ultoa( unsigned long value, char *buf, int radix );

/* ---- stdlib.h: Borland random(n) / randomize() ----------------------- */

#include <time.h>

inline int random( int num )
{
    return num > 0 ? int( ::random() % num ) : 0;
}

inline void randomize()
{
    srandom( (unsigned)time( 0 ) );
}

/* ---- io.h ------------------------------------------------------------- */

long filelength( int fd );

#endif  // __TVCOMPAT_H
