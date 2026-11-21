/* <unistd.h> for the Windows build: the POSIX functions the library (and
   the programs built on it) use, with their MSVC names */
#if !defined( __TV_WIN32_UNISTD_H )
#define __TV_WIN32_UNISTD_H

#include <corecrt_io.h>
#include <direct.h>
#include <fcntl.h>
#include <limits.h>
#include <process.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#define F_OK 0
#define W_OK 2
#define R_OK 4
#define X_OK 0                  /* Windows has no execute permission bit */

#if !defined( S_ISDIR )
#define S_ISDIR( m ) ( ( (m) & _S_IFMT ) == _S_IFDIR )
#define S_ISREG( m ) ( ( (m) & _S_IFMT ) == _S_IFREG )
#endif

#if !defined( PATH_MAX )
#define PATH_MAX 1024
#endif

typedef intptr_t ssize_t;
typedef int mode_t;

/* the current directory with '/', as the library uses inside */
char *tv_getcwd( char *buf, int size );
#define getcwd tv_getcwd

/* files: no POSIX permissions; fsync is _commit */
inline int fsync( int fd ) { return _commit( fd ); }
inline int fchmod( int, mode_t ) { return 0; }

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#endif
