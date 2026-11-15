/*------------------------------------------------------------*/
/* filename - tfiledtr.cpp                                    */
/*                                                            */
/* function(s)                                                */
/*            TFileEditor member functions                    */
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

#define Uses_TGroup
#define Uses_TEditor
#define Uses_TFileEditor
#define Uses_TEvent
#define Uses_opstream
#define Uses_ipstream
#include <tv.h>

#include "platform/platform.h"

#include <iterator>
#include <string>

#if !defined( __LIMITS_H )
#include <limits.h>
#endif  // __LIMITS_H

#if !defined( __STRING_H )
#include <string.h>
#endif  // __STRING_H

#if !defined( __FSTREAM_H )
#include <fstream.h>
#endif  // __FSTREAM_H

#if !defined( __IO_H )
#include <io.h>
#endif  // __IO_H

#if !defined( __STDIO_H )
#include <stdio.h>
#endif  // __STDIO_H

inline ushort min( ushort u1, ushort u2 )
{
    return u1 < u2 ? u1 : u2;
}

TFileEditor::TFileEditor( const TRect& bounds,
                          TScrollBar *aHScrollBar,
                          TScrollBar *aVScrollBar,
                          TIndicator *aIndicator,
                          const char *aFileName
                        ) :
    TEditor( bounds, aHScrollBar, aVScrollBar, aIndicator, 0 )
{
    dosLineEnds = False;
    utf8Text = True;                    // new text is saved as UTF-8
    if( aFileName == 0 )
        fileName[0] = EOS;
    else
        {
        strcpy( fileName, aFileName );
        fexpand( fileName );
        if( isValid )
            isValid = loadFile();
        }
}

void TFileEditor::doneBuffer()
{
    delete buffer;
}

void TFileEditor::handleEvent( TEvent& event )
{
    TEditor::handleEvent(event);
    switch( event.what )
        {
        case evCommand:
            switch( event.message.command )
                {
                case cmSave:
                    save();
                    break;
                case cmSaveAs:
                    saveAs();
                    break;
                default:
                    return;
                }
            break;
        default:
            return;
        }
    clearEvent(event);
}

void TFileEditor::initBuffer()
{
    buffer = new char[bufSize];
}

// Linux port: converts a UTF-8 text to the active 8-bit code page.
// Fails (and the file is edited byte for byte) when the text is not
// valid UTF-8 or has characters outside the code page.
static Boolean utf8ToCodePage( const std::string &in, std::string &out )
{
    out.clear();
    out.reserve( in.size() );
    for( size_t i = 0; i < in.size(); )
        {
        uchar c = in[i];
        if( c < 0x80 )
            {
            out += char( c );
            i++;
            continue;
            }
        size_t len = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC2 ? 2 : 0;
        if( len == 0 || i + len > in.size() )
            return False;
        uint32_t u = c & ( 0x7F >> len );
        for( size_t k = 1; k < len; k++ )
            {
            uchar cc = in[i + k];
            if( (cc & 0xC0) != 0x80 )
                return False;
            u = (u << 6) | (cc & 0x3F);
            }
        int b = tvplatform::fromUnicode( u );
        if( b < 0x80 )
            return False;
        out += char( b );
        i += len;
        }
    return True;
}

static void putUtf8( ofstream &f, uchar c )
{
    uint32_t u = tvplatform::toUnicode( c );
    if( u < 0x800 )
        {
        f.put( char( 0xC0 | (u >> 6) ) );
        f.put( char( 0x80 | (u & 0x3F) ) );
        }
    else
        {
        f.put( char( 0xE0 | (u >> 12) ) );
        f.put( char( 0x80 | ((u >> 6) & 0x3F) ) );
        f.put( char( 0x80 | (u & 0x3F) ) );
        }
}

// Linux port: the editor works with CR LF line ends internally.  Files
// with LF line ends are converted on load and written back with LF.
Boolean TFileEditor::loadFile()
{
    dosLineEnds = False;
    utf8Text = True;
    ifstream f( fileName, ios::in | ios::binary );
    if( !f )
        {
        setBufLen( 0 );
        return True;
        }

    std::string raw( (std::istreambuf_iterator<char>( f )),
                     std::istreambuf_iterator<char>() );
    if( f.bad() )
        {
        editorDialog( edReadError, fileName );
        return False;
        }
    std::string converted;
    utf8Text = utf8ToCodePage( raw, converted );
    if( utf8Text )
        raw.swap( converted );
    std::string text;
    text.reserve( raw.size() + raw.size() / 16 );
    for( size_t i = 0; i < raw.size(); i++ )
        {
        if( raw[i] == '\n' )
            {
            if( i > 0 && raw[i-1] == '\r' )
                dosLineEnds = True;
            else
                text += '\r';
            }
        text += raw[i];
        }

    long fSize = long( text.size() );
    if( fSize > 0xFFE0L || setBufSize(ushort(fSize)) == False )
        {
        editorDialog( edOutOfMemory );
        return False;
        }
    memcpy( &buffer[bufSize - ushort(fSize)], text.data(), size_t( fSize ) );
    setBufLen(ushort(fSize));
    return True;
}

Boolean TFileEditor::save()
{
    if( *fileName == EOS )
        return saveAs();
    else
        return saveFile();
}

Boolean TFileEditor::saveAs()
{
    Boolean res = False;
    if( editorDialog( edSaveAs, fileName ) != cmCancel )
        {
        fexpand( fileName );
        message( owner, evBroadcast, cmUpdateTitle, 0 );
        res = saveFile();
        if( isClipboard() == True )
            *fileName = EOS;
        }
    return res;
}

static void writeBlock( ofstream& f, char *buf, unsigned len,
                        Boolean dosLineEnds, Boolean utf8Text,
                        Boolean &pendingCR )
{
    for( unsigned i = 0; i < len; i++ )
        {
        char c = buf[i];
        if( pendingCR )
            {
            pendingCR = False;
            if( c != '\n' || dosLineEnds )
                f.put( '\r' );
            }
        if( c == '\r' )
            pendingCR = True;
        else if( utf8Text && uchar( c ) >= 0x80 )
            putUtf8( f, uchar( c ) );
        else
            f.put( c );
        }
}

Boolean TFileEditor::saveFile()
{
    if( (editorFlags & efBackupFiles) != 0 )
        {
        // Linux port: "name.ext" is backed up as "name.ext.bak"
        char backupName[MAXPATH + 8];
        strcpy( backupName, fileName );
        strcat( backupName, backupExt );
        unlink( backupName );
        rename( fileName, backupName );
        }

    ofstream f( fileName, ios::out | ios::binary );

    if( !f )
        {
        editorDialog( edCreateError, fileName );
        return False;
        }
    else
        {
        Boolean pendingCR = False;
        writeBlock( f, buffer, curPtr, dosLineEnds, utf8Text, pendingCR );
        writeBlock( f, buffer+curPtr+gapLen, bufLen-curPtr, dosLineEnds,
                    utf8Text, pendingCR );
        if( pendingCR )
            f.put( '\r' );

        if( !f )
            {
            editorDialog( edWriteError, fileName );
            return False;
            }
        else
            {
            modified = False;
            update(ufUpdate);
            }
        }
    return True;
}

Boolean TFileEditor::setBufSize( ushort newSize )
{
    if( newSize > 0xF000 )
        newSize = 0xFFE0;
    else
        newSize = (newSize + 0x0FFF) & 0xF000;
    if( newSize != bufSize )
        {
        char *temp = buffer;
        if( (buffer = new char[newSize]) == 0 )
            {
            delete temp;
            return False;
            }
        ushort n = bufLen - curPtr + delCount;
        memcpy( buffer, temp, min( newSize, bufSize ) );
        memmove( &buffer[newSize - n], &temp[bufSize - n], n );
        delete temp;
        bufSize = newSize;
        gapLen = bufSize - bufLen;
        }
    return True;
}

void TFileEditor::shutDown()
{
    setCmdState(cmSave, False);
    setCmdState(cmSaveAs, False);
    TEditor::shutDown();
}

void TFileEditor::updateCommands()
{
    TEditor::updateCommands();
    setCmdState(cmSave, True);
    setCmdState(cmSaveAs, True);
}

Boolean TFileEditor::valid( ushort command )
{
    if( command == cmValid )
        return isValid;
    else
        {
        if( modified == True )
            {
            int d;
            if( *fileName == EOS )
                d = edSaveUntitled;
            else
                d = edSaveModify;

            switch( editorDialog( d, fileName ) )
                {
                case cmYes:
                    return save();
                case cmNo:
                    modified = False;
                    return True;
                case cmCancel:
                    return False;
                }
            }
        }
    return True;
}

void TFileEditor::write( opstream& os )
{
    TEditor::write( os );
    os.writeString( fileName );
    os << selStart << selEnd << curPtr;
}

void *TFileEditor::read( ipstream& is )
{
    TEditor::read( is );
    is.readString( fileName, sizeof( fileName ) );
    if( isValid )
        {
        isValid = loadFile();
        ushort sStart, sEnd, curs;
        is >> sStart >> sEnd >> curs;
        if( isValid && sEnd <= bufLen )
            {
            setSelect( sStart, sEnd, Boolean(curs == sStart) );
            trackCursor( True );
            }
        }
    return this;
}

TStreamable *TFileEditor::build()
{
    return new TFileEditor( streamableInit );
}

TFileEditor::TFileEditor( StreamableInit ) : TEditor( streamableInit )
{
}

