/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   EDITDLG.H                                                             */
/*                                                                         */
/*   the dialogs of the editors, ready to use                              */
/*                                                                         */
/*   TEditor asks its questions through TEditor::editorDialog, which by    */
/*   default answers cmCancel to everything: no Find, no Replace, no       */
/*   "Save?" and no "Save as".  editorDialogs() is a complete one, with    */
/*   the dialogs of TVEDIT:                                                */
/*                                                                         */
/*       TEditor::editorDialog = editorDialogs;                            */
/*       editorFileMask = "*.yaml";     // the Save as file list           */
/*                                                                         */
/*   The editor commands cmFind, cmReplace and cmSearchAgain (Ctrl+Q F,    */
/*   Ctrl+Q A, Ctrl+L) then work and may be put into a menu.               */
/*                                                                         */
/*   Include through tvaddons.h with Uses_TEditorDialogs.                  */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TEditorDialogs ) && !defined( __TEditorDialogs )
#define __TEditorDialogs

class TDialog;

// a TEditor::editorDialog: messages, Save as, Find, Replace and the
// replace prompt
ushort editorDialogs( int dialog, ... );

// the file mask of the Save as dialog, "*.*" by default
extern const char *editorFileMask;

// the Find and Replace dialogs; their data are a TFindDialogRec and a
// TReplaceDialogRec (editors.h)
TDialog *newFindDialog();
TDialog *newReplaceDialog();

// runs d on the desktop with data (may be 0) and destroys it; returns
// the command that ended it
ushort execEditorDialog( TDialog *d, void *data );

#endif  // Uses_TEditorDialogs
