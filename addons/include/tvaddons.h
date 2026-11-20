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
#endif

#if defined( Uses_TYamlEditor )
#define Uses_TAdvancedEditor
#endif

#if defined( Uses_TAdvancedEditor )
#define Uses_TFileEditor
#define Uses_TScrollBar
#define Uses_TIndicator
#endif

#include <tv.h>

// the class declarations are selected inside by the Uses_ macros;
// a base class comes before the classes derived from it
#include <advedit.h>
#include <yamledit.h>
