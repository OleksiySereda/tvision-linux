# Turbo Vision add-ons

New classes for the Linux port of Turbo Vision. They are built into a library
of their own, `lib/libtvaddons.a`, and are used the way the library classes
are: a `Uses_` macro and one header.

```cpp
#define Uses_TYamlEditWindow
#include <tvaddons.h>
```

```sh
g++ -std=c++11 -Ipath/include -Ipath/include/compat -Ipath/addons/include app.cpp \
    -Wl,--whole-archive path/lib/libtvaddons.a path/lib/libtv.a -Wl,--no-whole-archive -o app
```

`make` (or `make lib`) in `linux/` builds both libraries; `make check-addons`
runs the tests of the add-ons.

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

```cpp
#define Uses_TAdvancedEditor
#include <tvaddons.h>

TAdvancedEditor *ed = new TAdvancedEditor( r, hScrollBar, vScrollBar, indicator, "notes.txt" );
ed->clipboardKeys = False;  // the WordStar keys again
```

- Selecting with Shift is `TEditor`'s own and works with every cursor key:
  Shift+arrows, Shift+Home/End, Shift+PgUp/PgDn, Shift+Ctrl+Left/Right.
- Ctrl+Ins, Shift+Ins and Shift+Del still copy, paste and cut, and after
  Ctrl+K or Ctrl+Q the second key is WordStar's (Ctrl+K Ctrl+C pastes).
- The clipboard is `TEditor::clipboard`, as for every editor: the program
  has to create it (as TVEDIT does), or Ctrl+C and Ctrl+V do nothing.
- `cmSelectAll` (530) is an editor command like `cmCharLeft` (500–523): it
  cannot be disabled and can be put into a menu as it is.

## TYamlEditor, TYamlEditWindow

`TYamlEditor` is a `TAdvancedEditor` that colours YAML. Editing, files, the
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

- `TYamlEditor` overrides `TEditor::drawLines()`, which is virtual in this port
  for that purpose (`include/editors.h`). `TEditor::formatLine()` still lays out
  the line (tabs, selection, horizontal scrolling); then the cells in the
  normal text colour get the colour of their token.
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
