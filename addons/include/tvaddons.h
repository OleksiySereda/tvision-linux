/* ------------------------------------------------------------------------*/
/*                                                                         */
/*   TVADDONS.H                                                            */
/*                                                                         */
/*   New classes for the Linux port of Turbo Vision.  Used like tv.h:      */
/*                                                                         */
/*       #define Uses_TYamlEditWindow                                      */
/*       #include <tvaddons.h>                                             */
/*                                                                         */
/*   Each Uses_ macro also selects the library classes it needs, then      */
/*   tv.h and the add-on headers are included.  Compile with               */
/*   -Iaddons/include and link lib/libtvaddons.a before lib/libtv.a.       */
/*                                                                         */
/* ------------------------------------------------------------------------*/

#if defined( Uses_TYamlEditWindow )
#define Uses_TYamlEditor
#define Uses_TEditWindow
#define Uses_TBlockIndicator
#endif

#if defined( Uses_TYamlEditor )
#define Uses_TColoredEditor
#endif

#if defined( Uses_TColoredEditor )
#define Uses_TAdvancedEditor
#endif

#if defined( Uses_TEditorDialogs )
#define Uses_TEditor
#define Uses_TDialog
#endif

#if defined( Uses_TProgressDialog )
#define Uses_TProgressBar
#define Uses_TDialog
#endif

#if defined( Uses_TProgressBar )
#define Uses_TView
#endif

#if defined( Uses_TTabView )
#define Uses_TGroup
#endif

#if defined( Uses_TTreeView )
#define Uses_TListViewer
#endif

#if defined( Uses_TComboBox )
#define Uses_TValidatedInputLine
#endif

#if defined( Uses_TAppSettings )
#define Uses_TMenuItem
#endif

#if defined( Uses_TValidatedInputLine )
#define Uses_TValidator
#define Uses_TInputLine
#endif

#if defined( Uses_TLogView )
#define Uses_TScroller
#endif

#if defined( Uses_TTableHeader )
#define Uses_TTableView
#endif

#if defined( Uses_TTableView )
#define Uses_TListViewer
#endif

#if defined( Uses_TAdvancedEditor )
#define Uses_TFileEditor
#define Uses_TScrollBar
#define Uses_TIndicator
#endif

#if defined( Uses_TBlockIndicator )
#define Uses_TIndicator
#endif

#if defined( Uses_TLineNumbers )
#define Uses_TView
#endif

#include <tv.h>

// the class declarations are selected inside by the Uses_ macros;
// a base class comes before the classes derived from it
#include <advedit.h>
#include <colredit.h>
#include <yamledit.h>
#include <editdlg.h>
#include <validate.h>
#include <logview.h>
#include <tableview.h>
#include <progbar.h>
#include <tabview.h>
#include <treeview.h>
#include <combobox.h>
#include <appsets.h>
