/*------------------------------------------------------------*/
/* filename -       tfillist.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TFileList member functions                */
/*------------------------------------------------------------*/

/*------------------------------------------------------------*/
/*                                                            */
/*    Turbo Vision -  Version 1.0                             */
/*                                                            */
/*                                                            */
/*    Copyright (c) 1991 by Borland International             */
/*    All Rights Reserved.                                    */
/*                                                            */
/*------------------------------------------------------------*/

#define Uses_TVMemMgr
#define Uses_MsgBox
#define Uses_TFileList
#define Uses_TRect
#define Uses_TSearchRec
#define Uses_TEvent
#define Uses_TGroup
#include <tv.h>

#if !defined( __DIR_H )
#include <dir.h>
#endif  // __DIR_H

#if !defined( __ERRNO_H )
#include <errno.h>
#endif  // __ERRNO_H

#if !defined( __IO_H )
#include <io.h>
#endif  // __IO_H

#if !defined( __STDIO_H )
#include <stdio.h>
#endif  // __STDIO_H

#if !defined( __CTYPE_H )
#include <ctype.h>
#endif  // __CTYPE_H

#if !defined( __ASSERT_H )
#include <assert.h>
#endif  // __ASSERT_H

#if !defined( __DOS_H )
#include <dos.h>
#endif  // __DOS_H

#include <unistd.h>

#if !defined( __STRING_H )
#include <string.h>
#endif  // __STRING_H

void fexpand( char * );

TFileList::TFileList( const TRect& bounds,
                      TScrollBar *aScrollBar) :
    TSortedListBox( bounds, 2, aScrollBar )
{
}

TFileList::~TFileList()
{
   if ( list() )
      destroy ( list() );
}

void TFileList::focusItem( short item )
{
    TSortedListBox::focusItem( item );
    message( owner, evBroadcast, cmFileFocused, list()->at(item) );
}

void TFileList::getData( void * )
{
}

void TFileList::setData( void * )
{
}

ushort TFileList::dataSize()
{
    return 0;
}

void* TFileList::getKey( const char *s )
{
static TSearchRec sR;

    if( (shiftState & 0x03) != 0 || *s == '.' )
        sR.attr = FA_DIREC;
    else
        sR.attr = 0;
    strcpy( sR.name, s );
    return &sR;
}

void TFileList::getText( char *dest, short item, short maxChars )
{
	TSearchRec *f = (TSearchRec *)(list()->at(item));

	strncpy( dest, f->name, maxChars );
	dest[maxChars] = '\0';
    if( f->attr & FA_DIREC )
        strcat( dest, "/" );
}

void TFileList::handleEvent( TEvent & event )
{
    if( event.what == evMouseDown && event.mouse.doubleClick )
        {
        event.what = evCommand;
        event.message.command = cmOK;
        putEvent( event );
        clearEvent( event );
        }
    else
        TSortedListBox::handleEvent( event );
}

void TFileList::readDirectory( const char *dir, const char *wildCard )
{
    char path[MAXPATH];
    strcpy( path, dir );
    strcat( path, wildCard );
    readDirectory( path );
}

struct DirSearchRec : public TSearchRec
{

    void *operator new( size_t ) throw();

};

void *DirSearchRec::operator new( size_t sz ) throw()
{
    void *temp = ::operator new( sz );
    if( TVMemMgr::safetyPoolExhausted() )
        {
        ::operator delete( temp );
        temp = 0;
        }
    return temp;
}

static void copySearchRec( TSearchRec *p, const ffblk &s )
{
    p->attr = s.ff_attrib;
    p->time = ( long( s.ff_fdate ) << 16 ) | s.ff_ftime;
    p->size = s.ff_fsize;
    strncpy( p->name, s.ff_name, sizeof( p->name ) - 1 );
    p->name[sizeof( p->name ) - 1] = EOS;
}

void TFileList::readDirectory( const char *aWildCard )
{
ffblk s;

char path[MAXPATH];
char dir[MAXDIR];
char file[MAXFILE];
char ext[MAXEXT];

const unsigned findAttr = FA_RDONLY | FA_ARCH;

    strcpy( path, aWildCard );
    fexpand( path );
    fnsplit( path, 0, dir, file, ext );

    TFileCollection *fileList = new TFileCollection( 5, 5 );

    int res = findfirst( path, &s, findAttr );
    DirSearchRec *p;
    Boolean outOfMemory = False;
    while( !outOfMemory && res == 0 )
        {
        if( (s.ff_attrib & FA_DIREC) == 0 )
            {
            p = new DirSearchRec;
            if( p != 0 )
                {
                copySearchRec( p, s );
                fileList->insert( p );
                }
            else
                outOfMemory = True;
            }
        res = findnext( &s );
        }
    while( res == 0 )
        res = findnext( &s );

    fnmerge( path, 0, dir, "*", "" );

    res = findfirst( path, &s, FA_DIREC );
    while( !outOfMemory && res == 0 )
        {
        if( (s.ff_attrib & FA_DIREC) != 0 && s.ff_name[0] != '.' )
            {
            p = new DirSearchRec;
            if( p != 0 )
                {
                copySearchRec( p, s );
                fileList->insert( p );
                }
            else
                outOfMemory = True;
            }
        res = findnext( &s );
        }
    while( res == 0 )                   // let the search close its handle
        res = findnext( &s );

    if( !outOfMemory && strcmp( dir, "/" ) != 0 )
        {
        p = new DirSearchRec;
        if( p == 0 )
            outOfMemory = True;
        else
            {
            strcpy( p->name, ".." );
            p->size = 0;
            p->time = 0x210000L;
            p->attr = FA_DIREC;
            fileList->insert( p );
            }
        }

    if( outOfMemory )
        messageBox( tooManyFiles, mfOKButton | mfWarning );
    newList(fileList);
    if( list()->getCount() > 0 )
        message( owner, evBroadcast, cmFileFocused, list()->at(0) );
    else
        {
        static DirSearchRec noFile;
        message( owner, evBroadcast, cmFileFocused, &noFile );
        }
}

/*
    fexpand:    reimplementation of pascal's FExpand routine.  Takes a
                relative path and makes an absolute path of the form

                    /[subdir/ ...]filename.ext

                Linux port: "~/" is expanded to $HOME, "." and ".."
                components are resolved, a trailing '/' is kept.
*/

void squeeze( char *path )
{
    // path is absolute; resolve "//", "/./" and "/../"
    char out[MAXPATH];
    size_t len = 0;
    const char *src = path;
    while( *src != EOS )
        {
        while( *src == '/' )
            src++;
        const char *end = strchr( src, '/' );
        size_t n = end ? size_t( end - src ) : strlen( src );
        if( n == 0 )
            break;
        if( n == 1 && src[0] == '.' )
            ;
        else if( n == 2 && src[0] == '.' && src[1] == '.' )
            {
            while( len > 0 && out[len-1] != '/' )
                len--;
            if( len > 0 )
                len--;
            }
        else if( len + n + 2 < sizeof( out ) )
            {
            out[len++] = '/';
            memcpy( out + len, src, n );
            len += n;
            }
        src += n;
        }
    if( len == 0 || ( strlen( path ) > 0 && path[strlen( path ) - 1] == '/' ) )
        out[len++] = '/';
    out[len] = EOS;
    strcpy( path, out );
}

void fexpand( char *rpath )
{
char path[MAXPATH * 2];

    if( rpath[0] == '~' && ( rpath[1] == '/' || rpath[1] == EOS ) &&
        getenv( "HOME" ) != 0 )
        {
        snprintf( path, sizeof( path ), "%s/%s", getenv( "HOME" ),
                  rpath + ( rpath[1] ? 2 : 1 ) );
        }
    else if( rpath[0] != '/' )
        {
        char cwd[MAXPATH];
        if( getcwd( cwd, sizeof( cwd ) ) == 0 )
            strcpy( cwd, "/" );
        snprintf( path, sizeof( path ), "%s/%s", cwd, rpath );
        }
    else
        strcpy( path, rpath );
    path[MAXPATH - 1] = EOS;
    squeeze( path );
    strcpy( rpath, path );
}

TStreamable *TFileList::build()
{
    return new TFileList( streamableInit );
}


