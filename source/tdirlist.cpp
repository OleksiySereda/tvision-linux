/*------------------------------------------------------------*/
/* filename -       tdirlist.cpp                              */
/*                                                            */
/* function(s)                                                */
/*                  TDirListBox member functions              */
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

#define Uses_TDirListBox
#define Uses_TEvent
#define Uses_TDirCollection
#define Uses_TChDirDialog
#define Uses_TDirEntry
#define Uses_TButton
#include <tv.h>

#include <stdlib.h>

#if !defined( __STRING_H )
#include <string.h>
#endif  // __STRING_H

#if !defined( __DIR_H )
#include <dir.h>
#endif  // __DIR_H

#if !defined( __DOS_H )
#include <dos.h>
#endif  // __DOS_H

TDirListBox::TDirListBox( const TRect& bounds, TScrollBar *aScrollBar ) :
    TListBox( bounds, 1, aScrollBar ),
    cur( 0 )
{
    *dir = EOS;
}

TDirListBox::~TDirListBox()
{ 
   if ( list() )
      destroy( list() );
}

void TDirListBox::getText( char *text, short item, short maxChars )
{
	strncpy( text, list()->at(item)->text(), maxChars );
	text[maxChars] = '\0';
}

void TDirListBox::handleEvent( TEvent& event )
{
    if( event.what == evMouseDown && event.mouse.doubleClick )
        {
        event.what = evCommand;
        event.message.command = cmChangeDir;
        putEvent( event );
        clearEvent( event );
        }
    else
        TListBox::handleEvent( event );
}

Boolean TDirListBox::isSelected( short item )
{
    return Boolean( item == cur );
}

// Linux port: there are no drives; the tree starts at "/".
void TDirListBox::showDrives( TDirCollection * )
{
}

static int compareNames( const void *a, const void *b )
{
    return strcmp( *(const char * const *)a, *(const char * const *)b );
}

void TDirListBox::showDirs( TDirCollection *dirs )
{
    const int indentSize = 2;
    int indent = indentSize;

    char buf[MAXPATH+MAXFILE+MAXEXT+16];
    memset( buf, ' ', sizeof( buf ) );
    char *name = buf + sizeof(buf) - (MAXFILE+MAXEXT);

    char *org = name - strlen(pathDir);
    strcpy( org, pathDir );

    // the root: "/", or "C:/" on Windows
    int root = rootLength( dir ) > 0 ? rootLength( dir ) : 1;
    strncpy( name, dir, size_t( root ) );
    name[root] = EOS;
    dirs->insert( new TDirEntry( org, name ) );

    // each component of the current directory
    char *curDir = dir + root;
    char *end;
    while( (end = strchr( curDir, '/' )) != 0 )
        {
        *end = EOS;
        strncpy( name, curDir, size_t(end-curDir) );
        name[size_t(end-curDir)] = EOS;
        dirs->insert( new TDirEntry( org - indent, dir ) );
        *end = '/';
        curDir = end+1;
        indent += indentSize;
        }

    cur = dirs->getCount() - 1;

    // the subdirectories, sorted
    end = strrchr( dir, '/' );
    char path[MAXPATH+MAXFILE+MAXEXT];
    strncpy( path, dir, size_t(end-dir+1) );
    end = path + unsigned(end-dir)+1;
    strcpy( end, "*" );

    char **names = 0;
    int count = 0, capacity = 0;
    ffblk ff;
    int res = findfirst( path, &ff, FA_DIREC );
    while( res == 0 )
        {
        if( (ff.ff_attrib & FA_DIREC) != 0 && ff.ff_name[0] != '.' )
            {
            if( count == capacity )
                {
                capacity = capacity ? capacity * 2 : 32;
                names = (char **)realloc( names, capacity * sizeof( char * ) );
                }
            names[count++] = newStr( ff.ff_name );
            }
        res = findnext( &ff );
        }
    if( count > 0 )
        qsort( names, count, sizeof( char * ), compareNames );

    for( int k = 0; k < count; k++ )
        {
        if( k == 0 )
            memcpy( org, firstDir, strlen(firstDir)+1 );
        else
            memcpy( org, middleDir, strlen(middleDir)+1 );
        strcpy( name, names[k] );
        strcpy( end, names[k] );
        dirs->insert( new TDirEntry( org - indent, path ) );
        delete[] names[k];
        }
    free( names );

    char *p = dirs->at(dirs->getCount()-1)->text();
    char *i = strchr( p, graphics[0] );
    if( i == 0 )
        {
        i = strchr( p, graphics[1] );
        if( i != 0 )
            *i = graphics[0];
        }
    else
        {
        *(i+1) = graphics[2];
        *(i+2) = graphics[2];
        }
}

void TDirListBox::newDirectory( const char *str )
{
    strcpy( dir, str );
    if( dir[strlen( dir ) - 1] != '/' )
        strcat( dir, "/" );
    TDirCollection *dirs = new TDirCollection( 5, 5 );
    showDirs( dirs );
    newList( dirs );
    focusItem( cur );
}

void TDirListBox::setState( ushort nState, Boolean enable )
{
    TListBox::setState( nState, enable );
    if( (nState & sfFocused) != 0 )
        ((TChDirDialog *)owner)->chDirButton->makeDefault( enable );
}

TStreamable *TDirListBox::build()
{
    return new TDirListBox( streamableInit );
}


