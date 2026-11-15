#!/usr/bin/env python3
"""Compiles the larger documentation examples against the library.

Writes obj/docexamples/*.cpp; the Makefile target 'check-doc' compiles them.
Fragments are wrapped into small programs here so that they are type-checked.
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from classes_data import SECTIONS   # noqa: E402

OUT = os.path.join(HERE, "..", "..", "obj", "docexamples")

USES = """#define Uses_TKeys
#define Uses_TApplication
#define Uses_TWindow
#define Uses_TDialog
#define Uses_TButton
#define Uses_TInputLine
#define Uses_TLabel
#define Uses_TStaticText
#define Uses_TParamText
#define Uses_TCheckBoxes
#define Uses_TRadioButtons
#define Uses_TSItem
#define Uses_TListBox
#define Uses_TScrollBar
#define Uses_TScroller
#define Uses_TListViewer
#define Uses_TMenuBar
#define Uses_TMenuBox
#define Uses_TSubMenu
#define Uses_TMenuItem
#define Uses_TMenu
#define Uses_TStatusLine
#define Uses_TStatusDef
#define Uses_TStatusItem
#define Uses_TDeskTop
#define Uses_TBackground
#define Uses_TFrame
#define Uses_TFileDialog
#define Uses_TChDirDialog
#define Uses_TStringCollection
#define Uses_TSortedListBox
#define Uses_TEditWindow
#define Uses_TFileEditor
#define Uses_TMemo
#define Uses_TTerminal
#define Uses_otstream
#define Uses_TColorDialog
#define Uses_TColorGroup
#define Uses_TColorItem
#define Uses_TResourceFile
#define Uses_TStrListMaker
#define Uses_TStringList
#define Uses_fpstream
#define Uses_MsgBox
#define Uses_TEventQueue
#define Uses_TScreen
#define Uses_TVMemMgr
#define Uses_THistory
#define Uses_TStreamableClass
#include <tv.h>
#include <help.h>
#include <stdio.h>
#include <assert.h>
"""

# complete examples (class definitions / whole programs) compiled as they are
WHOLE = ["TPalette", "TView", "TFrame", "TScroller", "TListViewer",
         "TBackground", "TApplication", "TStatusLine", "TTextDevice",
         "TStreamable", "TSortedCollection"]

# fragments compiled inside a function with some variables in scope
FRAGMENT_ENV = """
static TRect r( 0, 0, 40, 10 );
static TScrollBar *sb, *vs, *hs, *hScroll, *vScroll;
static TIndicator *indicator, *ind;
static TDialog *d;
static TWindow *window;
static TTerminal *term;
static long bytes;
static const char *host = "example.org";
static ushort myEditorDialog( int, ... ) { return cmCancel; }
static void openEditor( const char * ) {}
static void save() {}
static void extendSelection() {}
const ushort hcEditor = 100;
static TView *progress;
static const char *fileName = "x";
static TDialog *makeMainDialog() { return 0; }
class THello : public TView { public: THello( const TRect& b ) : TView( b ) {} };
"""
# examples written as the body of a view method (TRect, TGroup's forEach,
# TNSSortedCollection's search) are reviewed by hand
FRAGMENTS = ["TObject", "TCommandSet", "TScrollBar", "TWindow",
             "TDialog", "TButton", "TInputLine", "TStaticText", "TParamText",
             "TCheckBoxes", "TRadioButtons", "TListBox", "THistory", "MsgBoxText",
             "TFileDialog", "TChDirDialog", "TEditor", "TMemo", "TEditWindow",
             "TTerminal", "otstream", "TStringCollection", "TResourceFile",
             "TScreen", "TEventQueue", "TVMemMgr", "TMenuBox", "TMenu"]


def main():
    os.makedirs(OUT, exist_ok=True)
    for old in os.listdir(OUT):
        os.remove(os.path.join(OUT, old))
    examples = {c[0]: c[5] for _, _, cs in SECTIONS for c in cs}
    n = 0
    for name in WHOLE:
        src = USES + "\nconst ushort hcEditor = 100;\n" + examples[name]
        if name == "TStreamable":
            src += "\nTStreamableClass RNote( TNote::name, TNote::build, __DELTA( TNote ) );\n"
        if "int main" not in src:
            src += "\nint main() { return 0; }\n"
        if name in ("TTextDevice",):
            src = src.replace("store( s, count );", "(void)s;")
        if name == "TSortedCollection":
            src = src.replace("    // readItem()/writeItem() as for TCollection",
                              "private:\n    virtual void *readItem( ipstream& ) { return 0; }\n"
                              "    virtual void writeItem( void *, opstream& ) {}")
        open(os.path.join(OUT, name + ".cpp"), "w", encoding="utf-8", newline="\n").write(src)
        n += 1
    for name in FRAGMENTS:
        body = "\n".join( l for l in examples[name].split("\n")
                          if not l.startswith("#") )
        src = (USES + FRAGMENT_ENV + "\nstatic void example()\n{\n" + body
               + "\n}\nint main() { if( 0 ) example(); return 0; }\n")
        open(os.path.join(OUT, name + ".cpp"), "w", encoding="utf-8", newline="\n").write(src)
        n += 1
    print(n, "examples written to", os.path.normpath(OUT))


if __name__ == "__main__":
    main()
