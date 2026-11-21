# Turbo Vision add-ons

New classes for the Linux port of Turbo Vision (also built on Windows). They
are built into a library of their own, `lib/libtvaddons.a` (`lib/tvaddons.lib`
on Windows), and are used the way the library classes are: a `Uses_` macro and
one header.

```cpp
#define Uses_TYamlEditWindow
#include <tvaddons.h>
```

```sh
g++ -std=c++11 -Ipath/include -Ipath/include/compat -Ipath/addons/include app.cpp \
    -Wl,--whole-archive path/lib/libtvaddons.a path/lib/libtv.a -Wl,--no-whole-archive -o app
```

`make` (or `make lib`) in `tvision/` builds both libraries; `make check-addons`
runs the tests of the add-ons. On Windows, `CMakeLists.txt` builds them; a
program links them with `/WHOLEARCHIVE`, which the CMake target `tvaddons`
brings along:

```cmake
add_subdirectory( path/to/tvision tvision EXCLUDE_FROM_ALL )
tv_addons_program( app app.cpp )
```

| Directory | Contents |
|---|---|
| `include/tvaddons.h` | The header to include; each `Uses_` macro also selects the library classes it needs |
| `include/*.h` | Declarations of the add-on classes |
| `source/` | Their code, built into `lib/libtvaddons.a` |
| `demos/` | Programs that show them (`bin/...`) |
| `tests/` | Tests (`make check-addons`) |

To add a class: put its declaration into a header in `include/` under
`#if defined( Uses_TName ) && !defined( __TName )`, the dependencies of
`Uses_TName` into `tvaddons.h`, and the code into `source/`. The Makefile picks
up every `source/*.cpp` by itself.

## TAdvancedEditor

`TAdvancedEditor` is a `TFileEditor` with the clipboard keys of today's
editors. `TEditor` itself is not changed, so `TEditWindow`, `TMemo` and the
DOS programs keep the WordStar keys.

| Key | TAdvancedEditor | TEditor (WordStar), still on |
|---|---|---|
| Ctrl+C | copy | page down: PgDn |
| Ctrl+X | cut | line down: Down |
| Ctrl+V | paste | insert mode: Ins |
| Ctrl+A | select all (`cmSelectAll`) | word left: Ctrl+Left |
| Ctrl+B | start / end selecting without Shift (`cmToggleSelect`) | (free in `TEditor`) |

```cpp
#define Uses_TAdvancedEditor
#include <tvaddons.h>

TAdvancedEditor *ed = new TAdvancedEditor( r, hScrollBar, vScrollBar, indicator, "notes.txt" );
ed->clipboardKeys = False;  // the WordStar keys again
```

- Selecting with Shift is `TEditor`'s own and works with every cursor key:
  Shift+arrows, Shift+Home/End, Shift+PgUp/PgDn, Shift+Ctrl+Left/Right.
  It needs a terminal that sends Shift with these keys; `bin/keytest` shows
  whether yours does (`shift 0x00` for Shift+Left means it does not, as
  with PuTTY's defaults).

### Selecting without Shift: Ctrl+B

For terminals that do not send Shift with the cursor keys, Ctrl+B selects
the way F3 does in Midnight Commander:

1. **Ctrl+B** starts selecting at the cursor.
2. Every cursor key (arrows, Home/End, PgUp/PgDn, Ctrl+arrows, Ctrl+PgUp/PgDn)
   extends the selection, as if Shift were held.
3. **Ctrl+C** copies or **Ctrl+X** cuts, and that ends selecting. Typing,
   pasting and Del end it too; a second **Ctrl+B** ends it and keeps the
   selection until the cursor moves.

While selecting is on, a `TBlockIndicator` shows **B** in the window frame,
next to the line:column position (`TYamlEditWindow` has one; in a window of
your own, pass a `TBlockIndicator` to the editor instead of a `TIndicator`).
Ctrl+B is a free key in `TEditor`, so it works also with
`clipboardKeys = False`. The same mode exists in every TV editor as the
WordStar keys **Ctrl+K B** (start) and **Ctrl+K H** (end and hide).
- Ctrl+Ins, Shift+Ins and Shift+Del still copy, paste and cut, and after
  Ctrl+K or Ctrl+Q the second key is WordStar's (Ctrl+K Ctrl+C pastes).
- The clipboard is `TEditor::clipboard`, as for every editor: the program
  has to create it (as TVEDIT does), or Ctrl+C and Ctrl+V do nothing.
- `cmSelectAll` (530) and `cmToggleSelect` (531) are editor commands like
  `cmCharLeft` (500–523): they cannot be disabled and can be put into a
  menu as they are.
- The mouse wheel scrolls the text under the mouse by `wheelLines` lines
  (3); Shift+wheel and the sideways wheel scroll horizontally. The cursor
  and the selection stay where they are. `wheel()` is virtual and can be
  overridden. The wheel comes as `evMouseWheel`, an event of this port
  (see `include/system.h`).

### Line numbers: TLineNumbers

A `TLineNumbers` is a column of its own left of a `TAdvancedEditor`: the
number of each line on 4 digits (white on gray, `TLineNumbers::numberColor`)
and 2 blanks. It follows the editor's scrolling and the number of lines.

```cpp
#define Uses_TLineNumbers
TRect nr = r;  nr.b.x = r.a.x + 6;  r.a.x = nr.b.x;  // 6 columns for the numbers
TAdvancedEditor *ed = new TAdvancedEditor( r, 0, vScrollBar, indicator, "prog.txt" );
insert( new TLineNumbers( nr, ed ) );
insert( ed );
```

A line can be marked: its number is then shown in a colour of its own, for a
breakpoint, an error or the line being run. The marks belong to the numbers,
not to the text, so they stay on their line numbers when lines are inserted
or deleted; the program sets them again when it needs to. Up to 64 lines are
marked; past that the oldest mark goes.

```cpp
numbers->setMark( 12, 0x4F );       // line 12: white on red
numbers->markOf( 12 );              // 0x4F; 0 for an unmarked line
numbers->clearMark( 12 );           // or setMark( 12, 0 )
numbers->clearMarks();
```

## TColoredEditor

A `TAdvancedEditor` that colours its text by classes that a derived class
gives the characters of each line. A syntax colouring editor needs only two
functions:

```cpp
#define Uses_TColoredEditor
#include <tvaddons.h>

// numbers in green
class TNumberEditor : public TColoredEditor
{
public:
    TNumberEditor( const TRect& r, TScrollBar *h, TScrollBar *v, TIndicator *i, const char *f ) :
        TColoredEditor( r, h, v, i, f ) {}
    // the class of each of the n characters s of the line at linePtr
    virtual void scanLine( ushort linePtr, const char *s, int n, uchar *cls )
        { for( int k = 0; k < n; k++ ) cls[k] = s[k] >= '0' && s[k] <= '9'; }
    // the attribute of a class, where the normal attribute is normal
    virtual uchar classColor( uchar cls, uchar normal )
        { return cls == 1 ? uchar( ( normal & 0xF0 ) | 0x0A ) : normal; }
};
```

`drawLines()` lets `TEditor::formatLine()` lay out the line (tabs, selection,
horizontal scrolling) and then gives the cells in the normal text colour the
colour of their class; selected text keeps the selection colour. Lines are
classified up to `TColoredEditor::maxScanLength` (1024) characters, and
`lineText()` gives the text of a line. `highlight = False` turns the colours
off; with the monochrome or black-and-white palette nothing is coloured.
`linePtr` lets `scanLine()` look at the lines around (as `TYamlEditor` does for
block scalars). `TColoredEditor` is not registered for streams itself: a
derived class registers its own `name` and `build()`.

## Editor dialogs: editorDialogs()

`TEditor` asks its questions through `TEditor::editorDialog`, which by default
answers *Cancel* to everything: no Find, no Replace, no "Save?" and no
"Save as". `editorDialogs()` answers all of them with the dialogs of TVEDIT:

```cpp
#define Uses_TEditorDialogs
#include <tvaddons.h>

TEditor::editorDialog = editorDialogs;
editorFileMask = "*.yaml";          // the file list of Save as; "*.*" by default
```

Then `cmFind`, `cmReplace` and `cmSearchAgain` work (Ctrl+Q F, Ctrl+Q A,
Ctrl+L in the editor) and can be put into a menu. `newFindDialog()`,
`newReplaceDialog()` and `execEditorDialog()` are there for programs that
build their own `editorDialog`.

## TYamlEditor, TYamlEditWindow

`TYamlEditor` is a `TColoredEditor` that colours YAML. Editing, files, the
clipboard, find and replace, undo and streaming all come from `TFileEditor`,
the Ctrl+C/X/V/A keys from `TAdvancedEditor`; only the drawing is new. `TYamlEditWindow` is a `TEditWindow` whose editor is a
`TYamlEditor`, so it can be used wherever a `TEditWindow` is used.

```cpp
#define Uses_TYamlEditWindow
#define Uses_TDeskTop
#define Uses_TProgram
#include <tvaddons.h>

void openYaml( const char *fileName )
{
    TRect r = TProgram::deskTop->getExtent();
    TYamlEditWindow *w = new TYamlEditWindow( r, fileName, wnNoNumber );
    TProgram::deskTop->insert( TProgram::application->validView( w ) );
}
```

An editor inside your own window or dialog:

```cpp
TYamlEditor *ed = new TYamlEditor( r, hScrollBar, vScrollBar, indicator, "app.yaml" );
ed->highlight = False;      // plain text again
```

What gets a colour of its own:

| Token | Example | On a blue background |
|---|---|---|
| key | `name:` `"quoted key":` `{key: v}` | bright white |
| quoted string | `"text"` `'it''s'` | bright green |
| constant | `42` `-1.5e3` `0x1F` `.inf` `true` `no` `null` `~` | bright magenta |
| comment | `# ...` (not inside quotes or block text) | light gray |
| punctuation | `-` `:` `?` `[ ] { } ,` and the `\|` `>` block indicators | bright cyan |
| anchor, alias, tag | `&base` `*base` `!!str` | bright red |
| document marker | `---` `...` | bright white |
| block scalar text | the lines under `script: \|` | bright green |
| other values | `shop:latest` `10.0.0.11` | the normal editor colour |

The colours depend on the editor's background: `TYamlEditor::darkColors` is
used on black, blue, red and magenta backgrounds, and `lightColors` on the
others (gray, cyan, ...). Both are arrays of foreground colours (0–15) indexed
by `YamlToken`, and can be changed by the program; `0xFF` keeps the normal
colour. Selected text keeps the selection colour. With the monochrome or
black-and-white palette (`TVMONO=1`, `TERM=vt100`), nothing is coloured.

How it works:

- `TColoredEditor` overrides `TEditor::drawLines()`, which is virtual in this
  port for that purpose (`include/editors.h`); `TYamlEditor::scanLine()` gives
  it the token of each character and `classColor()` its colour.
- `yamlScanLine()` classifies the characters of one line. It does not parse
  the file, so invalid YAML is coloured as well as it can be, never rejected.
- Block scalar text is recognised by `inBlockScalar()`. It looks up from the
  line for the first line that is less indented than everything in between,
  and checks whether that line ends with `|` or `>` (up to 2000 lines up).
- Quoted strings that continue on the next line, and plain scalars over
  several lines, are coloured only on their first line.

`bin/yamledit` is TVEDIT with `TYamlEditWindow`s (the menus and dialogs are
those of `demos/tvedit*.cpp`); files given on the command line are opened at
start, and **File | Open** lists `*.yaml` and `*.yml`. `addons/demos/sample.yaml`
contains every kind of token:

```sh
bin/yamledit addons/demos/sample.yaml
```
