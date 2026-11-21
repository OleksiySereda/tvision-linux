/*------------------------------------------------------------*/
/* filename -       editdlg.cpp                               */
/*                                                            */
/* function(s)                                                */
/*                  editorDialogs, newFindDialog,             */
/*                  newReplaceDialog, execEditorDialog        */
/*                                                            */
/*   The dialogs are those of TVEDIT (demos/tvedit2.cpp and   */
/*   tvedit3.cpp).                                            */
/*------------------------------------------------------------*/

#define Uses_TEditorDialogs
#define Uses_TDialog
#define Uses_TDeskTop
#define Uses_TProgram
#define Uses_TApplication
#define Uses_TObject
#define Uses_TInputLine
#define Uses_TLabel
#define Uses_THistory
#define Uses_TRect
#define Uses_TPoint
#define Uses_TCheckBoxes
#define Uses_TButton
#define Uses_TSItem
#define Uses_TFileDialog
#define Uses_MsgBox
#define Uses_TValidatedInputLine
#include <tvaddons.h>

#include <stdarg.h>
#include <stdio.h>

const char *editorFileMask = "*.*";

ushort execEditorDialog( TDialog *d, void *data )
{
    TView *p = TProgram::application->validView( d );
    if( p == 0 )
        return cmCancel;
    if( data != 0 )
        p->setData( data );
    ushort result = TProgram::deskTop->execView( p );
    if( result != cmCancel && data != 0 )
        p->getData( data );
    TObject::destroy( p );
    return result;
}

TDialog *newFindDialog()
{
    TDialog *d = new TDialog( TRect( 0, 0, 38, 12 ), "Find" );
    d->options |= ofCentered;

    TInputLine *control = new TInputLine( TRect( 3, 3, 32, 4 ), maxFindStrLen );
    d->insert( control );
    d->insert( new TLabel( TRect( 2, 2, 15, 3 ), "~T~ext to find", control ) );
    d->insert( new THistory( TRect( 32, 3, 35, 4 ), control, 10 ) );

    d->insert( new TCheckBoxes( TRect( 3, 5, 35, 7 ),
        new TSItem( "~C~ase sensitive",
        new TSItem( "~W~hole words only", 0 ) ) ) );

    d->insert( new TButton( TRect( 14, 9, 24, 11 ), "O~K~", cmOK, bfDefault ) );
    d->insert( new TButton( TRect( 26, 9, 36, 11 ), "Cancel", cmCancel, bfNormal ) );

    d->selectNext( False );
    return d;
}

TDialog *newReplaceDialog()
{
    TDialog *d = new TDialog( TRect( 0, 0, 40, 16 ), "Replace" );
    d->options |= ofCentered;

    TInputLine *control = new TInputLine( TRect( 3, 3, 34, 4 ), maxFindStrLen );
    d->insert( control );
    d->insert( new TLabel( TRect( 2, 2, 15, 3 ), "~T~ext to find", control ) );
    d->insert( new THistory( TRect( 34, 3, 37, 4 ), control, 10 ) );

    control = new TInputLine( TRect( 3, 6, 34, 7 ), maxReplaceStrLen );
    d->insert( control );
    d->insert( new TLabel( TRect( 2, 5, 12, 6 ), "~N~ew text", control ) );
    d->insert( new THistory( TRect( 34, 6, 37, 7 ), control, 11 ) );

    d->insert( new TCheckBoxes( TRect( 3, 8, 37, 12 ),
        new TSItem( "~C~ase sensitive",
        new TSItem( "~W~hole words only",
        new TSItem( "~P~rompt on replace",
        new TSItem( "~R~eplace all", 0 ) ) ) ) ) );

    d->insert( new TButton( TRect( 17, 13, 27, 15 ), "O~K~", cmOK, bfDefault ) );
    d->insert( new TButton( TRect( 28, 13, 38, 15 ), "Cancel", cmCancel, bfNormal ) );

    d->selectNext( False );
    return d;
}

TDialog *newGotoLineDialog()
{
    TDialog *d = new TDialog( TRect( 0, 0, 30, 8 ), "Go to line" );
    d->options |= ofCentered;

    TInputLine *control = new TValidatedInputLine( TRect( 15, 2, 24, 3 ), gotoLineLen,
                                                   new TRangeValidator( 1, 65535 ) );
    d->insert( control );
    d->insert( new TLabel( TRect( 2, 2, 14, 3 ), "~L~ine number", control ) );

    d->insert( new TButton( TRect( 4, 5, 14, 7 ), "O~K~", cmOK, bfDefault ) );
    d->insert( new TButton( TRect( 16, 5, 26, 7 ), "Cancel", cmCancel, bfNormal ) );

    d->selectNext( False );
    return d;
}

// a message with the file name the editor passes
static ushort fileMessage( const char *format, va_list arg, ushort options )
{
    char buf[300];
    snprintf( buf, sizeof( buf ), format, va_arg( arg, char * ) );
    return messageBox( buf, options );
}

ushort editorDialogs( int dialog, ... )
{
    va_list arg;
    va_start( arg, dialog );
    ushort result = cmCancel;
    switch( dialog )
        {
        case edOutOfMemory:
            result = messageBox( "Not enough memory for this operation.",
                                 mfError | mfOKButton );
            break;
        case edReadError:
            result = fileMessage( "Error reading file %s.", arg, mfError | mfOKButton );
            break;
        case edWriteError:
            result = fileMessage( "Error writing file %s.", arg, mfError | mfOKButton );
            break;
        case edCreateError:
            result = fileMessage( "Error creating file %s.", arg, mfError | mfOKButton );
            break;
        case edSaveModify:
            result = fileMessage( "%s has been modified. Save?", arg,
                                  mfInformation | mfYesNoCancel );
            break;
        case edSaveUntitled:
            result = messageBox( "Save untitled file?", mfInformation | mfYesNoCancel );
            break;
        case edSaveAs:
            result = execEditorDialog( new TFileDialog( editorFileMask, "Save file as",
                                           "~N~ame", fdOKButton, 101 ),
                                       va_arg( arg, char * ) );
            break;
        case edFind:
            result = execEditorDialog( newFindDialog(), va_arg( arg, void * ) );
            break;
        case edSearchFailed:
            result = messageBox( "Search string not found.", mfError | mfOKButton );
            break;
        case edReplace:
            result = execEditorDialog( newReplaceDialog(), va_arg( arg, void * ) );
            break;
        case edGotoLine:
            result = execEditorDialog( newGotoLineDialog(), va_arg( arg, char * ) );
            break;
        case edReplacePrompt:
            {
            // not on the line of the cursor
            TRect r( 0, 1, 40, 8 );
            r.move( ( TProgram::deskTop->size.x - r.b.x ) / 2, 0 );
            TPoint t = TProgram::deskTop->makeGlobal( r.b );
            t.y++;
            TPoint *cursor = va_arg( arg, TPoint * );
            if( cursor->y <= t.y )
                r.move( 0, TProgram::deskTop->size.y - r.b.y - 2 );
            result = messageBoxRect( r, "Replace this occurence?",
                                     mfYesNoCancel | mfInformation );
            break;
            }
        }
    va_end( arg );
    return result;
}
