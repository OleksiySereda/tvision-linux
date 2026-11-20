/*----------------------------------------------------------*/
/*                                                          */
/*   YAMLEDIT - TVEDIT with TYamlEditWindow                 */
/*                                                          */
/*   The menus, the status line and the find / replace      */
/*   dialogs are those of TVEDIT (demos/tvedit2.cpp and     */
/*   tvedit3.cpp); files are opened in TYamlEditWindows.    */
/*                                                          */
/*       yamledit [file.yaml ...]                           */
/*                                                          */
/*----------------------------------------------------------*/

#define Uses_TApplication
#define Uses_TEditWindow
#define Uses_TYamlEditWindow
#define Uses_TEditorDialogs
#define Uses_TDeskTop
#define Uses_TRect
#define Uses_TEditor
#define Uses_TFileEditor
#define Uses_TFileDialog
#define Uses_TChDirDialog

#include <tvaddons.h>

#include "tvedit.h"

#include <stdlib.h>
#include <string.h>
#include <iostream.h>

TEditWindow *clipWindow;
static char **startFiles = 0;

// files are YAML editors; the clipboard (no file name, hidden) is plain
TEditWindow *TEditorApp::openEditor( const char *fileName, Boolean visible )
{
    TRect r = deskTop->getExtent();
    TEditWindow *w;
    if( visible )
        w = new TYamlEditWindow( r, fileName, wnNoNumber );
    else
        w = new TEditWindow( r, fileName, wnNoNumber );
    TView *p = validView( w );
    if( p == 0 )
        return 0;
    if( !visible )
        p->hide();
    deskTop->insert( p );
    return (TEditWindow *)p;
}

TEditorApp::TEditorApp() :
    TProgInit( TEditorApp::initStatusLine,
               TEditorApp::initMenuBar,
               TEditorApp::initDeskTop
             ),
    TApplication()
{
    TCommandSet ts;
    ts.enableCmd( cmSave );
    ts.enableCmd( cmSaveAs );
    ts.enableCmd( cmCut );
    ts.enableCmd( cmCopy );
    ts.enableCmd( cmPaste );
    ts.enableCmd( cmClear );
    ts.enableCmd( cmUndo );
    ts.enableCmd( cmFind );
    ts.enableCmd( cmReplace );
    ts.enableCmd( cmSearchAgain );
    disableCommands( ts );

    TEditor::editorDialog = editorDialogs;     // the add-ons dialogs
    editorFileMask = "*.y*ml";
    clipWindow = openEditor( 0, False );
    if( clipWindow != 0 )
        {
        TEditor::clipboard = clipWindow->editor;
        TEditor::clipboard->canUndo = False;
        }

    // the files named on the command line (openEditor() is private, so
    // main() leaves them here)
    int count = 0;
    for( char **f = startFiles; f != 0 && *f != 0; f++, count++ )
        openEditor( *f, True );
    if( count > 1 )
        tile();
}

void TEditorApp::fileOpen()
{
    char fileName[MAXPATH];
    strcpy( fileName, "*.y*ml" );

    if( execDialog( new TFileDialog( "*.y*ml", "Open file",
            "~N~ame", fdOpenButton, 100 ), fileName) != cmCancel )
        openEditor( fileName, True );
}

void TEditorApp::fileNew()
{
    openEditor( 0, True );
}

void TEditorApp::changeDir()
{
    execDialog( new TChDirDialog( cdNormal, 0 ), 0 );
}

void TEditorApp::dosShell()
{
    suspend();
    const char *sh = getenv( "SHELL" );
    cout << "Type EXIT to return..." << endl;
    if( system( sh != 0 && *sh != EOS ? sh : "/bin/sh" ) == -1 )
        cout << "Cannot start the shell" << endl;
    resume();
    redraw();
}

void TEditorApp::showClip()
{
    clipWindow->select();
    clipWindow->show();
}

void TEditorApp::tile()
{
    deskTop->tile( deskTop->getExtent() );
}

void TEditorApp::cascade()
{
    deskTop->cascade( deskTop->getExtent() );
}

void TEditorApp::handleEvent( TEvent& event )
{
    TApplication::handleEvent( event );
    if( event.what != evCommand )
        return;
    else
        switch( event.message.command )
            {
            case cmOpen:
                fileOpen();
                break;

            case cmNew:
                fileNew();
                break;

            case cmChangeDrct:
                changeDir();
                break;

            case cmDosShell:
                dosShell();
                break;

            case cmShowClip:
                showClip();
                break;

            case cmTile:
                tile();
                break;

            case cmCascade:
                cascade();
                break;

            default:
                return ;
            }
    clearEvent( event );
}

int main( int argc, char **argv )
{
    startFiles = argv + 1;
    TEditorApp editorApp;
    editorApp.run();
    return 0;
}
