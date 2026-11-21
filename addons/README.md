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
| `tests/` | Tests (`make check-addons`; `ctest` on Windows) |
| `demos/validate.cpp` | `bin/validate`: a form with every validator |
| `demos/controls.cpp` | `bin/controls`: tabs, combo boxes, a tree, a progress dialog, settings |

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
| Ctrl+G | go to line (`cmGotoLine`) | delete char: Del |
| Ctrl+F | incremental search (`cmIncSearch`) | word right: Ctrl+Right |

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

### Go to line, incremental search, brackets

- **Ctrl+G** asks for a line number through `TEditor::editorDialog`
  (`edGotoLine`, the data are the number as text, `char[gotoLineLen]`);
  `editorDialogs()` has the dialog. `gotoLine( n )` does it from the
  program and shows the line in the middle; `cursorLine()` is the line of
  the cursor.
- **Ctrl+F** starts an incremental search: what is typed next is searched
  at once, without regard to case, from where the search started; Ctrl+F
  again finds the next one (at the end it starts over at the top),
  Backspace takes a character back, Enter ends the search at the text found,
  Esc goes back to where it started, any other key ends it and does what
  it always does. Ctrl+F twice searches the text of the last search. The
  text is left in `TEditor::findStr`, so Ctrl+L (Search again) goes on with
  it. A `TBlockIndicator` shows `/text` while it runs (`!text` when it is
  not found).
- The bracket `( ) [ ] { }` at the cursor, or else the one before it, and
  its pair are shown in `TAdvancedEditor::bracketColor` (black on cyan)
  while the editor has the focus. Quotes and comments are not taken into
  account. `matchBrackets = False` turns it off; `matchBracket( pos, pair )`
  and `bracketPair( at, pair )` give the positions. An editor with its own
  `drawLines()` calls `paintBrackets()` at the end of it, as
  `TColoredEditor` does.

Ctrl+G and Ctrl+F belong to `clipboardKeys` like Ctrl+C/X/V/A.

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
Ctrl+L in the editor) and can be put into a menu, and so does `cmGotoLine`
of `TAdvancedEditor` (Ctrl+G). `newFindDialog()`, `newReplaceDialog()`,
`newGotoLineDialog()` and `execEditorDialog()` are there for programs that
build their own `editorDialog`.

## TLogView

A `TScroller` that shows lines added at the end, as a log:

```cpp
#define Uses_TLogView
#include <tvaddons.h>

TLogView *log = new TLogView( r, hScrollBar, vScrollBar );   // 2000 lines kept
log->add( "Build: 19 cells" );
log->add( "line 3: unknown label", TLogView::errorColor );
log->addf( "STOP after %d steps, AC = %d", steps, ac );
log->saveToFile( "run.log" );
```

- Only the last `maxLines` lines are kept (the fourth argument, 2000 by
  default).
- While the end is shown, new lines scroll it along (`autoScroll`); after
  the user has scrolled up the view stays where it is until they go back
  to the end.
- A line may have a colour of its own, an attribute (0 = the view's
  colour): `errorColor` (white on red), `warningColor` (yellow on brown),
  `goodColor` (white on green), or any other.
- Tabs become blanks and `\n` starts a new line.
- The arrow keys, PgUp/PgDn, Home/End (and Ctrl+PgUp/PgDn) and the mouse
  wheel scroll it; it scrolls without scroll bars too, and while they are
  hidden. `scrollToPos()`, `scrollToEnd()`, `atEnd()` and `position()` do
  it from the program.
- A view of lines kept elsewhere overrides `lineCount()`, `lineText()` and
  `lineColor()` and calls `update()` when there are new ones (the logs of
  deployer do so).

## TTableView, TTableHeader

A `TListViewer` of rows with columns of text, and a header over it:

```cpp
#define Uses_TTableView
#define Uses_TTableHeader
#include <tvaddons.h>

TTableView *table = new TTableView( TRect( 2, 3, 60, 15 ), 0, vScrollBar );
table->addColumn( "Server", 12 );
table->addColumn( "Port", 6, TTableView::alRight );
table->addColumn( "Description", 30 );
table->addRow( { "web1", "22", "Frontend" } );
table->markable = True;                 // Space / Ins mark rows
insert( table );
insert( new TTableHeader( TRect( 2, 2, 60, 3 ), table ) );
```

- A click on a title sorts by that column, a second click the other way
  round; an arrow in the title shows it. `sortBy( col, ascending )` does it
  from the program, `sortBy( -1 )` goes back to the order of adding.
  Numbers are compared as numbers, other text without regard to case;
  `compare()` is virtual. A row added to a sorted table goes into its
  place; after `setCell()` the program calls `resort()` if it wants the
  rows sorted again.
- With `markable`, Space and Ins (which moves on) mark and unmark the
  focused row, as does a click in the first column; marked rows show a
  check mark in the "selected" colour. `setMark()`, `isMarked()`,
  `markedCount()`, `markAll()`.
- Enter and a double click broadcast `cmListItemSelected` as `TListViewer`
  does; `focusedRow()` is the row.
- Rows are numbered in the order they were added (`row`); their place on
  the screen after sorting is an item (`focused`, `topItem`): `rowAt()` and
  `itemOf()` convert. `removeRow()` renumbers the rows after it.
- The header follows the sideways scrolling of the table (with a
  horizontal scroll bar). Colours are the list viewer's, so the table
  belongs in a dialog, as `TListBox` does.

## TValidatedInputLine and validators

Turbo Vision 1.0 has no validators (they came with 2.0). A
`TValidatedInputLine` is a `TInputLine` with a `TValidator`:

```cpp
#define Uses_TValidatedInputLine
#include <tvaddons.h>

d->insert( new TValidatedInputLine( TRect( 3, 2, 12, 3 ), 6,   // 5 characters and the 0
               new TRangeValidator( 1, 65535 ) ) );
d->insert( new TValidatedInputLine( TRect( 3, 4, 20, 5 ), 16,
               new TRegexValidator( "(\\d{1,3}\\.){3}\\d{1,3}", "Not an IPv4 address." ) ) );
```

- While typing, a key that would make the text impossible (a letter in a
  number, a number past the range) is not taken.
- When the dialog is closed with any command but Cancel (OK, Yes, ...), a
  wrong text gives a message box saying why, the line is selected and the
  dialog stays open. Moving to another control with Tab does not check.

| Validator | Accepts |
|---|---|
| `TFilterValidator( "0123456789abcdef" )` | only these characters |
| `TRangeValidator( min, max )` | a whole number from min to max; `value()` gives it |
| `TLookupValidator( "yes\|no", '\|', caseSensitive )` | one of a list |
| `TRegexValidator( expression, message, caseSensitive )` | the whole text matches (`std::regex`, ECMAScript) |
| `TFuncValidator( check, message, data )` | what `check( text, data )` says |
| `TIPAddressValidator` | an IPv4 address, `192.168.0.1`: 4 numbers 0–255 without leading zeros; `parse()` gives it as a number |
| `TNetworkValidator( strict )` | address/prefix, `10.0.0.0/8`; with `strict` the bits after the prefix must be 0 |
| `TNetMaskValidator` | a mask: `255.255.255.0`, `/24` or `24`; the ones must come first; `prefix()` gives the length |
| `THostValidator( allowAddress )` | a host name (RFC 1123: labels of letters, digits and `-`, up to 63, 253 in all) or, by default, an IPv4 address |
| `TEmailValidator` | `name@example.com`: a practical check (one `@`, the characters allowed before it, a host name with a dot after it), not all of RFC 5322 |
| `TPasswordValidator( minLength, require )` | at least `minLength` (8) characters with the kinds in `require`: `pwDigit`, `pwUpper`, `pwLower` (the default), `pwSymbol`; the message says what is missing |

While typing, the network validators take only what can still become
valid: no fifth number, no `256`, no second `/`.

A line with `password = True` shows `*` for each character; its data are the
real text:

```cpp
TValidatedInputLine *pw = new TValidatedInputLine( TRect( 15, 6, 46, 7 ), 31,
                                                   new TPasswordValidator );
pw->password = True;
```

The second argument is the size of the data with the 0, as for `TInputLine`:
31 for 30 characters. `bin/validate` (`addons/demos/validate.cpp`) is a form
with a line for each of them.

`allowEmpty = True` lets an empty text through whatever the validator says.
A validator of its own derives from `TValidator` and overrides
`isValidInput()` (while typing), `isValid()` and `errorText()`. The line owns
its validator; `setValidator()` changes it. The data stay text, as for a
`TInputLine`; the line is written to a stream as a plain `TInputLine`.

## TProgressBar, TProgressDialog

`TProgressBar` fills from the left as its value goes from 0 to the total,
with the percentage (or a text given with `setText()`) in the middle.
`TProgressDialog` is a dialog with a line of text, a bar and Cancel, for
work that takes a while. A loop of the program runs it:

```cpp
#define Uses_TProgressDialog
#include <tvaddons.h>

TProgressDialog *d = new TProgressDialog( "Copying", n );
TProgram::deskTop->insert( d );
for( int i = 0; i < n && !d->cancelled(); i++ )
    {
    d->setText( names[i] );
    copy( names[i] );
    d->setValue( i + 1 );
    }
TObject::destroy( d );
```

`cancelled()` takes the events that came in meanwhile (Cancel, Esc, a
resized terminal), without waiting for more, at most every `pollMs` (50)
milliseconds; only the dialog gets them. A dialog run with `execView()`
instead, while the work goes on in `idle()`, ends with `finish()` (cmOK)
or Cancel (cmCancel).

## TTabView

Pages with tabs over them, in a dialog:

```cpp
#define Uses_TTabView
#include <tvaddons.h>

TTabView *tabs = new TTabView( TRect( 2, 1, 62, 15 ) );
TGroup *p = tabs->addPage( "~G~eneral" );
p->insert( new TInputLine( TRect( 15, 1, 47, 2 ), 31 ) );   // in the page's coordinates
p = tabs->addPage( "~N~etwork" );
...
d->insert( tabs );
```

- A click on a title, Alt and its highlighted letter, or Ctrl+PgDn /
  Ctrl+PgUp show a page; it broadcasts `cmTabChanged`.
- Tab and Shift+Tab go through the controls of the page, then on to the
  dialog's buttons; coming back, the page starts at its first control.
- `getData()` / `setData()` of the dialog take every page, in the order
  they were added, so one (packed) record holds the whole dialog.
- When the dialog is closed with OK, the page of the first wrong field is
  shown before its message, and within a page the fields are checked from
  the top.

The titles and the background of a page are views of their own, so a
closed list or message box leaves nothing behind (a `TGroup` only draws
its subviews).

## TTreeView

A tree of lines that open and close (`TOutline` came only with Turbo
Vision 2.0):

```cpp
#define Uses_TTreeView
#include <tvaddons.h>

TTreeView *tree = new TTreeView( TRect( 2, 2, 41, 15 ), 0, vScrollBar );
tree->markable = True;
TTreeNode *web = tree->addNode( 0, "web" );
tree->addNode( web, "web1" );
tree->addNode( web, "web2" );
```

```
- [-] production
  ├─- [-] web
  │ ├── [x] web1
  │ └── [ ] web2
  └─+ [ ] db
```

- Right or `+` opens a node (or goes into it), Left or `-` closes it (or
  goes to the parent), `*` opens all of it, a click on `+`/`-` too.
- With `markable`, Space or a click on the box marks a node with all its
  children; a partly marked node shows `[-]`; `markedLeaves()` gives the
  marked nodes without children.
- Enter and a double click broadcast `cmListItemSelected`; Enter goes on
  to the default button.
- `TTreeNode` holds `text`, `data` (the program's), `parent`, `children`,
  `expanded` and `marked`; the view owns the nodes.

## TComboBox, TComboButton

An input line with a list to choose from:

```cpp
#define Uses_TComboBox
#include <tvaddons.h>

TComboBox *env = new TComboBox( TRect( 15, 3, 28, 4 ), 11, "dev|test|staging|prod" );
env->setListOnly( True );               // only an item of the list
d->insert( env );
d->insert( new TComboButton( TRect( 28, 3, 31, 4 ), env ) );
```

Down in the line or a click on the button opens the list (like the list of
`THistory`); Enter or a double click takes an item. With `setListOnly()`
the text must be one of the items when the dialog is closed (in any case;
it then gets the case of the item). It is a `TValidatedInputLine`, so a
validator can be used instead.

## TAppSettings

What a program keeps from one run to the next, in an INI file
(`$XDG_CONFIG_HOME/<name>/settings.ini`, that is `~/.config/...`, on Linux;
`%APPDATA%/<name>/settings.ini` on Windows):

```cpp
#define Uses_TAppSettings
#include <tvaddons.h>

TAppSettings settings( "myapp" );       // reads the file
settings.restoreWindow( "editor", window );
settings.set( "general", "theme", "dark" );
settings.addRecent( fileName );
...
settings.saveWindow( "editor", window );
settings.save();
```

- `get()` / `set()`, `getInt()` / `setInt()`, `getBool()` / `setBool()`
  (yes/no, true/false, 1/0), `has()`, `remove()`, `removeSection()`.
- `saveWindow()` / `restoreWindow()`: the place and size of a view; a
  window larger than the screen is made to fit.
- The recent files: `addRecent()` puts a file first, at most `maxRecent`
  (8); `recentMenu( firstCommand, next )` makes the menu items "~1~ name"
  ... for them.
- `saveHistory( id )` / `loadHistory( id )`: the lists of `THistory`.
- `savePalette()` / `loadPalette()`: the colours of the application, e.g.
  after `TColorDialog`.
- `save()` writes a new file and renames it over the old one; comments and
  unknown lines stay where they were.

`bin/controls` (`addons/demos/controls.cpp`) uses all of these: a settings
dialog with three pages, combo boxes and validated lines, a tree of
servers, a progress dialog, the recent files in the File menu, and the
settings and dialog positions kept between runs.

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
