/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   EDITORS.H                                                             */
/*                                                                         */
/*   Copyright (c) Borland International 1991                              */
/*   All Rights Reserved.                                                  */
/*                                                                         */
/*   defines the classes TIndicator, TEditor, TMemo, TFileEditor,          */
/*   and TEditWindow                                                       */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if !defined( __DIR_H )
#include <dir.h>
#endif  // __DIR_H

#if !defined( __STRING_H )
#include <string.h>
#endif  // __STRING_H

#if !defined( __LIMITS_H )
#include <limits.h>
#endif  // __LIMITS_H

#if defined( __BCOPT__ )

#endif

#if !defined( __EDIT_COMMAND_CODES )
#define __EDIT_COMMAND_CODES

const int
  ufUpdate = 0x01,
  ufLine   = 0x02,
  ufView   = 0x04;

const int
  smExtend = 0x01,
  smDouble = 0x02;

const unsigned
  sfSearchFailed = 0xFFFF;

const int
  cmSave        = 80,
  cmSaveAs      = 81,
  cmFind        = 82,
  cmReplace     = 83,
  cmSearchAgain = 84;

const int
  cmCharLeft    = 500,
  cmCharRight   = 501,
  cmWordLeft    = 502,
  cmWordRight   = 503,
  cmLineStart   = 504,
  cmLineEnd     = 505,
  cmLineUp      = 506,
  cmLineDown    = 507,
  cmPageUp      = 508,
  cmPageDown    = 509,
  cmTextStart   = 510,
  cmTextEnd     = 511,
  cmNewLine     = 512,
  cmBackSpace   = 513,
  cmDelChar     = 514,
  cmDelWord     = 515,
  cmDelStart    = 516,
  cmDelEnd      = 517,
  cmDelLine     = 518,
  cmInsMode     = 519,
  cmStartSelect = 520,
  cmHideSelect  = 521,
  cmIndentMode  = 522,
  cmUpdateTitle = 523;

const int
  edOutOfMemory   = 0,
  edReadError     = 1,
  edWriteError    = 2,
  edCreateError   = 3,
  edSaveModify    = 4,
  edSaveUntitled  = 5,
  edSaveAs        = 6,
  edFind          = 7,
  edSearchFailed  = 8,
  edReplace       = 9,
  edReplacePrompt = 10;

const int
  efCaseSensitive   = 0x0001,
  efWholeWordsOnly  = 0x0002,
  efPromptOnReplace = 0x0004,
  efReplaceAll      = 0x0008,
  efDoReplace       = 0x0010,
  efBackupFiles     = 0x0100;

const int
  maxLineLength = 256;

#endif  // __EDIT_COMMAND_CODES

typedef ushort (*TEditorDialog)( int, ... );
ushort defEditorDialog( int dialog, ... );

#if defined( Uses_TIndicator ) && !defined( __TIndicator )
#define __TIndicator

class TRect;

class TIndicator : public TView
{

public:

    TIndicator( const TRect& );

    virtual void draw();
    virtual TPalette& getPalette() const;
    virtual void setState( ushort, Boolean );
    void setValue( const TPoint&, Boolean );

protected:

    TPoint location;
    Boolean modified;

private:

    static const char dragFrame;
    static const char normalFrame;

    virtual const char *streamableName() const
        { return name; }

protected:

    TIndicator( StreamableInit );

public:

    static const char * const name;
    static TStreamable *build();

};

inline ipstream& operator >> ( ipstream& is, TIndicator& cl )
    { return is >> (TStreamable&)cl; }
inline ipstream& operator >> ( ipstream& is, TIndicator*& cl )
    { return is >> (void *&)cl; }

inline opstream& operator << ( opstream& os, TIndicator& cl )
    { return os << (TStreamable&)cl; }
inline opstream& operator << ( opstream& os, TIndicator* cl )
    { return os << (TStreamable *)cl; }

#endif  // Uses_TIndicator


#if defined( Uses_TEditor ) && !defined( __TEditor )
#define __TEditor

class TRect;
class TScrollBar;
class TIndicator;
struct TEvent;

class TEditor : public TView
{

public:

    friend void genRefs();

    TEditor( const TRect&, TScrollBar *, TScrollBar *, TIndicator *, ushort );
    virtual ~TEditor();

    virtual void shutDown();

    char bufChar( ushort );
    ushort bufPtr( ushort );
    virtual void changeBounds( const TRect& );
    virtual void convertEvent( TEvent& );
    Boolean cursorVisible();
    void deleteSelect();
    virtual void doneBuffer();
    virtual void draw();
    virtual TPalette& getPalette() const;
    virtual void handleEvent( TEvent& );
    virtual void initBuffer();
    Boolean insertBuffer( char *, ushort, ushort, Boolean, Boolean );
    virtual Boolean insertFrom( TEditor * );
    Boolean insertText( const void *, ushort, Boolean );
    void scrollTo( int, int );
    Boolean search( const char *, ushort );
    virtual Boolean setBufSize( ushort );
    void setCmdState( ushort, Boolean );
    void setSelect( ushort, ushort, Boolean);
    virtual void setState( ushort, Boolean );
    void trackCursor( Boolean );
    void undo();
    virtual void updateCommands();
    virtual Boolean valid( ushort );

    int charPos( ushort, ushort );
    ushort charPtr( ushort, int );
    Boolean clipCopy();
    void clipCut();
    void clipPaste();
    void deleteRange( ushort, ushort, Boolean );
    void doUpdate();
    void doSearchReplace();
    // Linux port: virtual, so that derived editors can colour the text
    // (see addons/include/yamledit.h)
    virtual void drawLines( int, int, ushort );
    void formatLine(void *, ushort, int, ushort );
    void find();
    ushort getMousePtr( TPoint );
    Boolean hasSelection();
    void hideSelect();
    Boolean isClipboard();
    ushort lineEnd( ushort );
    ushort lineMove( ushort, int );
    ushort lineStart( ushort );
    void lock();
    void newLine();
    ushort nextChar( ushort );
    ushort nextLine( ushort );
    ushort nextWord( ushort );
    ushort prevChar( ushort );
    ushort prevLine( ushort );
    ushort prevWord( ushort );
    void replace();
    void setBufLen( ushort );
    void setCurPtr( ushort, uchar );
    void startSelect();
    void toggleInsMode();
    void unlock();
    void update( uchar );
    void checkScrollBar( const TEvent&, TScrollBar *, int& );

    TScrollBar *hScrollBar;
    TScrollBar *vScrollBar;
    TIndicator *indicator;
    char *buffer;
    ushort bufSize;
    ushort bufLen;
    ushort gapLen;
    ushort selStart;
    ushort selEnd;
    ushort curPtr;
    TPoint curPos;
    TPoint delta;
    TPoint limit;
    int drawLine;
    ushort drawPtr;
    ushort delCount;
    ushort insCount;
    Boolean isValid;
    Boolean canUndo;
    Boolean modified;
    Boolean selecting;
    Boolean overwrite;
    Boolean autoIndent;

    static TEditorDialog editorDialog;
    static ushort editorFlags;
    static char findStr[maxFindStrLen];
    static char replaceStr[maxReplaceStrLen];
    static TEditor * clipboard;
    uchar lockCount;
    uchar updateFlags;
    int keyState;

private:

    virtual const char *streamableName() const
        { return name; }

protected:

    TEditor( StreamableInit );
    virtual void write( opstream& );
    virtual void *read( ipstream& );

public:

    static const char * const name;
    static TStreamable *build();

};

inline ipstream& operator >> ( ipstream& is, TEditor& cl )
    { return is >> (TStreamable&)cl; }
inline ipstream& operator >> ( ipstream& is, TEditor*& cl )
    { return is >> (void *&)cl; }

inline opstream& operator << ( opstream& os, TEditor& cl )
    { return os << (TStreamable&)cl; }
inline opstream& operator << ( opstream& os, TEditor* cl )
    { return os << (TStreamable *)cl; }

#endif  // Uses_TEditor

#if defined( Uses_TMemo ) && !defined( __TMemo )
#define __TMemo

struct TEvent;

struct TMemoData
{
    ushort length;
    char buffer[INT_MAX];
};

class TMemo : public TEditor
{

public:

    TMemo( const TRect&, TScrollBar *, TScrollBar *, TIndicator *, ushort );
    virtual void getData( void *rec );
    virtual void setData( void *rec );
    virtual ushort dataSize();
    virtual TPalette& getPalette() const;
    virtual void handleEvent( TEvent& );

private:

    virtual const char *streamableName() const
        { return name; }

protected:

    TMemo( StreamableInit );
    virtual void write( opstream& );
    virtual void *read( ipstream& );

public:

    static const char * const name;
    static TStreamable *build();

};

inline ipstream& operator >> ( ipstream& is, TMemo& cl )
    { return is >> (TStreamable&)cl; }
inline ipstream& operator >> ( ipstream& is, TMemo*& cl )
    { return is >> (void *&)cl; }

inline opstream& operator << ( opstream& os, TMemo& cl )
    { return os << (TStreamable&)cl; }
inline opstream& operator << ( opstream& os, TMemo* cl )
    { return os << (TStreamable *)cl; }

#endif  // Uses_TMemo


#if defined( Uses_TFileEditor ) && !defined( __TFileEditor )
#define __TFileEditor

#if !defined( __DIR_H )
#include <dir.h>
#endif  // __DIR_H

class TRect;
class TScrollBar;
class TIndicator;
struct TEvent;

class TFileEditor : public TEditor
{

public:

    char fileName[MAXPATH];
    Boolean dosLineEnds;    // Linux port: file had CR LF line ends
    Boolean utf8Text;       // Linux port: file is UTF-8, edited in the
                            // 8-bit code page ($TVCODEPAGE)
    TFileEditor( const TRect&,
                 TScrollBar *,
                 TScrollBar *,
                 TIndicator *,
                 const char *
               );
    virtual void doneBuffer();
    virtual void handleEvent( TEvent& );
    virtual void initBuffer();
    Boolean loadFile();
    Boolean save();
    Boolean saveAs();
    Boolean saveFile();
    virtual Boolean setBufSize( ushort );
    virtual void shutDown();
    virtual void updateCommands();
    virtual Boolean valid( ushort );

private:

    static const char * backupExt;
    
    virtual const char *streamableName() const
        { return name; }

protected:

    TFileEditor( StreamableInit );
    virtual void write( opstream& );
    virtual void *read( ipstream& );

public:

    static const char * const name;
    static TStreamable *build();

};

inline ipstream& operator >> ( ipstream& is, TFileEditor& cl )
    { return is >> (TStreamable&)cl; }
inline ipstream& operator >> ( ipstream& is, TFileEditor*& cl )
    { return is >> (void *&)cl; }

inline opstream& operator << ( opstream& os, TFileEditor& cl )
    { return os << (TStreamable&)cl; }
inline opstream& operator << ( opstream& os, TFileEditor* cl )
    { return os << (TStreamable *)cl; }

#endif  // Uses_TFileEditor


#if defined( Uses_TEditWindow ) && !defined( __TEditWindow )
#define __TEditWindow

class TFileEditor;

class TEditWindow : public TWindow
{

public:

    TEditWindow( const TRect&, const char *, int );
    virtual void close();
    virtual const char *getTitle( short );
    virtual void handleEvent( TEvent& );
    virtual void sizeLimits( TPoint& min, TPoint& max );

    TFileEditor *editor;

private:

    static const char * clipboardTitle;
    static const char * untitled;
    
    virtual const char *streamableName() const
        { return name; }

protected:

    TEditWindow( StreamableInit );
    virtual void write( opstream& );
    virtual void *read( ipstream& );

public:

    static const char * const name;
    static TStreamable *build();

};

inline ipstream& operator >> ( ipstream& is, TEditWindow& cl )
    { return is >> (TStreamable&)cl; }
inline ipstream& operator >> ( ipstream& is, TEditWindow*& cl )
    { return is >> (void *&)cl; }

inline opstream& operator << ( opstream& os, TEditWindow& cl )
    { return os << (TStreamable&)cl; }
inline opstream& operator << ( opstream& os, TEditWindow* cl )
    { return os << (TStreamable *)cl; }

#endif  // Uses_TEditWindow


#if defined( Uses_TFindDialogRec ) && !defined( __TFindDialogRec )
#define __TFindDialogRec

#if !defined( __STRING_H )
#include <string.h>
#endif  // __STRING_H

struct TFindDialogRec
{
    TFindDialogRec( const char *str, ushort flgs )
        {
        strcpy( find, str );
        options = flgs;
        }
    char find[maxFindStrLen];
    ushort options;
};

#endif  // Uses_TFindDialogRec

#if defined( Uses_TReplaceDialogRec ) && !defined( __TReplaceDialogRec )
#define __TReplaceDialogRec

#if !defined( __STRING_H )
#include <string.h>
#endif  // __STRING_H

struct TReplaceDialogRec
{
    TReplaceDialogRec( const char *str, const char *rep, ushort flgs )
        {
        strcpy( find, str );
        strcpy( replace, rep );
        options = flgs;
        }
    char find[maxFindStrLen];
    char replace[maxReplaceStrLen];
    ushort options;
};

#endif  // Uses_TReplaceDialogRec

#if defined( __BCOPT__ )

#endif

