/*------------------------------------------------------------*/
/* filename -       tdircoll.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TDirCollection member functions           */
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

#define Uses_TDirCollection
#define Uses_TDirEntry
#define Uses_opstream
#define Uses_ipstream
#include <tv.h>

#if !defined( __DIR_H )
#include <dir.h>
#endif  // __DIR_H

#if !defined( __STRING_H )
#include <string.h>
#endif  // __STRING_H

#include <sys/stat.h>
#include <unistd.h>

#if !defined( __DOS_H )
#include <dos.h>
#endif  // __DOS_H


// Linux port: there are no drive letters; paths use '/'.

Boolean driveValid( char )
{
    return True;
}


Boolean isDir( const char *str )
{
    struct stat st;
    return Boolean( stat( str, &st ) == 0 && S_ISDIR( st.st_mode ) );
}

Boolean pathValid( const char *path )
{
    char expPath[MAXPATH];
    strcpy( expPath, path );
    fexpand( expPath );
    int len = strlen(expPath);
    if( len <= rootLength( expPath ) )
        return True;                    // "/", "C:/"

    if( expPath[len-1] == '/' )
        expPath[len-1] = EOS;

    return isDir( expPath );
}

Boolean validFileName( const char *fileName )
{
    char dir[MAXDIR];
    char name[MAXFILE];
    char ext[MAXEXT];

    fnsplit( fileName, 0, dir, name, ext );
    if( *dir != EOS && !pathValid( dir ) )
        return False;
    if( *name == EOS && *ext == EOS )
        return False;
    return True;
}

void getCurDir( char *dir )
{
    if( getcwd( dir, MAXPATH - 1 ) == 0 )
        strcpy( dir, "/" );
    if( dir[strlen( dir ) - 1] != '/' )
        strcat( dir, "/" );
}

Boolean isWild( const char *f )
{
    return Boolean( strpbrk( f, "?*" ) != 0 );
}

TStreamable *TDirCollection::build()
{
    return new TDirCollection( streamableInit );
}

void TDirCollection::writeItem( void *obj, opstream& os )
{
    TDirEntry *item = (TDirEntry *)obj;
    os.writeString( item->text() );
    os.writeString( item->dir() );
}

void *TDirCollection::readItem( ipstream& is )
{
    const char *txt = is.readString();
    const char *dir = is.readString();
    return new TDirEntry( txt, dir );
}
