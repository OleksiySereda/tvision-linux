#!/usr/bin/env python3
"""Generates the HTML documentation in tvision/doc/.

    python tools/mkdoc/mkdoc.py

classes_data.py holds the class reference, porting.py the porting guide.
"""

import html
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from classes_data import SECTIONS          # noqa: E402
from porting import PORTING                 # noqa: E402

OUT = os.path.join(HERE, "..", "..", "doc")

CSS = r"""
:root {
  --bg: #fbfaf7; --fg: #1d1d1f; --muted: #5c5c66; --rule: #e2dfd6;
  --panel: #ffffff; --code-bg: #f3f1ea; --code-fg: #1d1d1f;
  --accent: #1f5fae; --accent-soft: #e6eef9; --note: #fff6dc; --note-rule: #e8cf86;
  --tag: #eceae3;
}
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    --bg: #15161a; --fg: #e8e6e1; --muted: #a09fa6; --rule: #2c2d33;
    --panel: #1c1d22; --code-bg: #22232a; --code-fg: #e8e6e1;
    --accent: #79a8e8; --accent-soft: #1f2a3a; --note: #2a2618; --note-rule: #6b5a25;
    --tag: #2a2b31; color-scheme: dark;
  }
}
:root[data-theme="dark"] {
  --bg: #15161a; --fg: #e8e6e1; --muted: #a09fa6; --rule: #2c2d33;
  --panel: #1c1d22; --code-bg: #22232a; --code-fg: #e8e6e1;
  --accent: #79a8e8; --accent-soft: #1f2a3a; --note: #2a2618; --note-rule: #6b5a25;
  --tag: #2a2b31; color-scheme: dark;
}
* { box-sizing: border-box; }
html { scroll-padding-top: 1rem; }
body {
  margin: 0; background: var(--bg); color: var(--fg);
  font: 16px/1.6 system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
}
a { color: var(--accent); text-decoration: none; }
a:hover { text-decoration: underline; }
.layout { display: grid; grid-template-columns: 17rem minmax(0, 1fr); min-height: 100vh; }
nav.side {
  position: sticky; top: 0; height: 100vh; overflow-y: auto;
  border-right: 1px solid var(--rule); padding: 1.25rem 1rem; font-size: 0.9rem;
  background: var(--panel);
}
nav.side .brand { font-weight: 650; font-size: 1rem; margin-bottom: 0.25rem; }
nav.side .sub { color: var(--muted); font-size: 0.8rem; margin-bottom: 1rem; }
nav.side h4 { margin: 1.1rem 0 0.3rem; font-size: 0.72rem; text-transform: uppercase;
  letter-spacing: 0.06em; color: var(--muted); }
nav.side ul { list-style: none; margin: 0; padding: 0; }
nav.side li a { display: block; padding: 0.08rem 0.4rem; border-radius: 4px; color: var(--fg); }
nav.side li a:hover { background: var(--accent-soft); text-decoration: none; }
nav.side input {
  width: 100%; padding: 0.4rem 0.55rem; border: 1px solid var(--rule); border-radius: 6px;
  background: var(--bg); color: var(--fg); font: inherit;
}
main { padding: 2rem 2.5rem 4rem; max-width: 62rem; }
h1 { font-size: 2rem; line-height: 1.2; margin: 0 0 0.5rem; }
h2 { font-size: 1.45rem; margin: 2.6rem 0 0.6rem; padding-top: 0.6rem;
  border-top: 1px solid var(--rule); }
h3 { font-size: 1.12rem; margin: 1.8rem 0 0.3rem; }
p.lead { color: var(--muted); font-size: 1.05rem; margin-top: 0; }
code { font: 0.88em/1.45 ui-monospace, "SF Mono", Menlo, Consolas, monospace;
  background: var(--code-bg); padding: 0.08em 0.3em; border-radius: 4px; }
pre { background: var(--code-bg); color: var(--code-fg); padding: 0.9rem 1rem;
  border-radius: 8px; overflow-x: auto; border: 1px solid var(--rule); }
pre code { background: none; padding: 0; font-size: 0.85rem; }
.cls { border: 1px solid var(--rule); border-radius: 10px; padding: 0.2rem 1.2rem 0.4rem;
  margin: 1rem 0; background: var(--panel); }
.cls h3 { margin-top: 0.9rem; font-family: ui-monospace, Menlo, Consolas, monospace; }
.meta { display: flex; flex-wrap: wrap; gap: 0.4rem; margin: 0.2rem 0 0.6rem; font-size: 0.82rem; }
.meta span { background: var(--tag); border-radius: 999px; padding: 0.05rem 0.6rem; color: var(--muted); }
.meta span b { color: var(--fg); font-weight: 600; }
ul.tree, ul.tree ul { list-style: none; margin: 0; padding-left: 1.3rem; }
ul.tree { padding-left: 0; font-family: ui-monospace, Menlo, Consolas, monospace; font-size: 0.88rem; }
ul.tree li { position: relative; padding-left: 0.9rem; }
ul.tree ul li::before { content: ""; position: absolute; left: -0.4rem; top: 0;
  border-left: 1px solid var(--rule); height: 100%; }
ul.tree ul li::after { content: ""; position: absolute; left: -0.4rem; top: 0.8rem;
  width: 0.9rem; border-top: 1px solid var(--rule); }
ul.tree ul li:last-child::before { height: 0.8rem; }
ul.tree .also { color: var(--muted); font-size: 0.8rem; }
table { border-collapse: collapse; width: 100%; margin: 0.8rem 0 1.2rem; font-size: 0.92rem; }
th, td { border: 1px solid var(--rule); padding: 0.45rem 0.6rem; text-align: left; vertical-align: top; }
th { background: var(--code-bg); }
.note { background: var(--note); border-left: 4px solid var(--note-rule);
  padding: 0.6rem 1rem; border-radius: 6px; margin: 1rem 0; }
.cards { display: grid; grid-template-columns: repeat(auto-fit, minmax(16rem, 1fr)); gap: 1rem; }
.card { border: 1px solid var(--rule); border-radius: 10px; padding: 1rem 1.2rem; background: var(--panel); }
.card h3 { margin-top: 0; }
.ba { display: grid; grid-template-columns: 1fr 1fr; gap: 0.8rem; }
.ba > div > .lbl { font-size: 0.75rem; text-transform: uppercase; letter-spacing: 0.06em;
  color: var(--muted); margin-bottom: -0.6rem; }
@media (max-width: 52rem) {
  .layout { grid-template-columns: 1fr; }
  nav.side { position: static; height: auto; border-right: 0; border-bottom: 1px solid var(--rule); }
  main { padding: 1.2rem 1rem 3rem; }
  .ba { grid-template-columns: 1fr; }
  table { display: block; overflow-x: auto; }
}
"""


def esc(s):
    return html.escape(s, quote=False)


def code(s):
    return "<pre><code>" + esc(s.strip("\n")) + "</code></pre>"


def page(title, nav, body):
    return f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{esc(title)}</title>
<link rel="stylesheet" href="style.css">
</head>
<body>
<div class="layout">
<nav class="side">
<div class="brand"><a href="index.html">Turbo Vision for Linux</a></div>
<div class="sub">Port of Turbo Vision 1.0 (Borland C++ 3.1) to GCC / C++11</div>
{nav}
</nav>
<main>
{body}
</main>
</div>
</body>
</html>
"""


# ---- class reference -------------------------------------------------------

def all_classes():
    for title, intro, classes in SECTIONS:
        for c in classes:
            yield c


def build_tree():
    known = {c[0] for c in all_classes()}
    children = {}
    roots = []
    for name, header, bases, uses, summary, example in all_classes():
        parent = next((b for b in bases if b in known), None)
        if parent is None:
            roots.append(name)
        else:
            children.setdefault(parent, []).append(name)
    bases_of = {c[0]: c[2] for c in all_classes()}

    def render(name):
        others = [b for b in bases_of[name][1:]]
        also = (' <span class="also">+ ' + ", ".join(esc(o) for o in others)
                + "</span>") if others else ""
        out = f'<li><a href="#{name}">{esc(name)}</a>{also}'
        kids = sorted(children.get(name, []))
        if kids:
            out += "<ul>" + "".join(render(k) for k in kids) + "</ul>"
        return out + "</li>"

    # roots that have children first, then standalone classes
    with_kids = sorted(r for r in roots if r in children)
    alone = sorted(r for r in roots if r not in children)
    tree = '<ul class="tree">' + "".join(render(r) for r in with_kids) + "</ul>"
    standalone = ", ".join(f'<a href="#{n}"><code>{esc(n)}</code></a>' for n in alone)
    return tree, standalone


def section_id(title):
    return "s-" + "".join(ch.lower() if ch.isalnum() else "-" for ch in title)


def classes_page():
    nav = ['<input id="filter" type="search" placeholder="Filter classes…" '
           'aria-label="Filter classes">']
    nav.append('<h4>Contents</h4><ul><li><a href="#hierarchy">Class hierarchy</a></li>'
               '<li><a href="#using">Using the library</a></li></ul>')
    for title, intro, classes in SECTIONS:
        nav.append(f'<h4><a href="#{section_id(title)}">{esc(title)}</a></h4><ul>')
        for c in classes:
            nav.append(f'<li data-cls="{c[0].lower()}"><a href="#{c[0]}">{esc(c[0])}</a></li>')
        nav.append("</ul>")
    nav.append('<h4>See also</h4><ul><li><a href="porting.html">Porting guide</a></li></ul>')

    tree, standalone = build_tree()
    count = sum(len(c) for _, _, c in SECTIONS)
    body = [f"""<h1>Class reference</h1>
<p class="lead">All {count} classes of the library, grouped by area, with a short
example for each. Every class keeps the interface of the original Turbo Vision 1.0;
notes marked <b>Port note</b> describe where the Linux version behaves differently.</p>

<h2 id="using">Using the library</h2>
<p>A source file states which classes it needs with <code>Uses_</code> macros and then
includes <code>tv.h</code>, which pulls in exactly the required headers (and their
dependencies):</p>
{code('''#define Uses_TApplication
#define Uses_TWindow
#define Uses_TDialog
#define Uses_TButton
#define Uses_MsgBox
#include <tv.h>''')}
<p>Build with the library's include directories and link the whole archive, so that
the self-registering streamable classes are kept by the linker:</p>
{code('''g++ -std=c++11 -Itv/include -Itv/include/compat -c app.cpp
g++ -o app app.o -Wl,--whole-archive tv/lib/libtv.a -Wl,--no-whole-archive''')}
<p>Views are allocated with <code>new</code> and handed to their owner with
<code>insert()</code>; the owner destroys them. Release objects you own with
<code>TObject::destroy()</code>.</p>

<h2 id="hierarchy">Class hierarchy</h2>
<p>Each class is shown under its first base class; other bases follow the
<b>+</b> sign.</p>
{tree}
<p><b>Classes without a base class</b> (records, helpers and the system layer):
{standalone}.</p>
"""]

    for title, intro, classes in SECTIONS:
        body.append(f'<h2 id="{section_id(title)}">{esc(title)}</h2><p>{intro}</p>')
        for name, header, bases, uses, summary, example in classes:
            meta = [f"<span>header <b>{esc(header)}</b></span>"]
            if bases:
                meta.append("<span>base <b>" + ", ".join(esc(b) for b in bases)
                            + "</b></span>")
            if uses:
                meta.append(f"<span><b>{esc(uses)}</b></span>")
            body.append(f'<div class="cls" id="{name}" data-cls="{name.lower()}">'
                        f'<h3>{esc(name)}</h3><div class="meta">{"".join(meta)}</div>'
                        f'<p>{summary}</p>{code(example)}</div>')

    body.append("""<script>
(function () {
  var box = document.getElementById('filter');
  if (!box) return;
  box.addEventListener('input', function () {
    var q = box.value.trim().toLowerCase();
    document.querySelectorAll('[data-cls]').forEach(function (el) {
      el.style.display = !q || el.getAttribute('data-cls').indexOf(q) >= 0 ? '' : 'none';
    });
  });
})();
</script>""")
    return page("Turbo Vision Class Reference", "\n".join(nav), "\n".join(body))


# ---- porting guide ---------------------------------------------------------

def porting_page():
    nav = ['<h4>Contents</h4><ul>']
    body = ['<h1>Porting guide</h1>',
            '<p class="lead">How the Linux port differs from the original Borland '
            'Turbo Vision 1.0, how to use it in a terminal, and what to change in '
            'existing programs.</p>']
    for anchor, title, content in PORTING:
        nav.append(f'<li><a href="#{anchor}">{esc(title)}</a></li>')
        body.append(f'<h2 id="{anchor}">{esc(title)}</h2>')
        for kind, value in content:
            if kind == "html":
                body.append(value)
            elif kind == "code":
                body.append(code(value))
            elif kind == "ba":
                before, after = value
                body.append('<div class="ba"><div><p class="lbl">Borland C++ / DOS</p>'
                            + code(before) + '</div><div><p class="lbl">Linux port</p>'
                            + code(after) + '</div></div>')
    nav.append('</ul><h4>See also</h4><ul><li><a href="classes.html">Class reference</a></li></ul>')
    return page("Turbo Vision Porting Guide", "\n".join(nav), "\n".join(body))


# ---- index -------------------------------------------------------------------

def index_page():
    nav = ('<h4>Documents</h4><ul><li><a href="classes.html">Class reference</a></li>'
           '<li><a href="porting.html">Porting guide</a></li></ul>')
    count = sum(len(c) for _, _, c in SECTIONS)
    body = f"""<h1>Turbo Vision for Linux</h1>
<p class="lead">Borland Turbo Vision 1.0 for C++, ported from Borland C++ 3.1 / DOS
to GCC and C++11. Programs run full-screen in any xterm-compatible terminal,
including over ssh, and follow the size of the terminal window. The same sources
also build on Windows with Visual Studio.</p>
<div class="cards">
<div class="card"><h3><a href="classes.html">Class reference</a></h3>
<p>The class hierarchy and all {count} classes by area — views, menus, dialogs,
editors, help, collections, streams, resources, system layer — each with an
example.</p></div>
<div class="card"><h3><a href="porting.html">Porting guide</a></h3>
<p>What differs from the original library, terminal behaviour and settings, and
step-by-step instructions for moving DOS code to the port.</p></div>
<div class="card"><h3><a href="classes.html#s-add-ons">Add-ons</a></h3>
<p>New classes in their own library: <code>TAdvancedEditor</code> (Ctrl+C/X/V/A,
Ctrl+B selection, go to line, incremental search, bracket pairs, mouse wheel),
line numbers with marks, <code>TColoredEditor</code> for syntax colouring,
<code>TYamlEditor</code>, the editor dialogs, <code>TLogView</code>,
<code>TTableView</code> with sorting and marks, input lines with
validators, tabs, a tree view, combo boxes, a progress dialog, settings
kept between runs, a wizard, commands with their output, a command
palette, a Markdown view, a date input with a calendar, a splitter, colour
themes, toasts and syntax colouring for C/C++, shell, INI, JSON and
Markdown.</p></div>
</div>
<h2>Quick start</h2>
<p>Linux, with g++ and GNU make:</p>
{code('''cd tvision
make                    # lib/libtv.a, lib/libtvaddons.a and the demos in bin/
cd bin && ./tvdemo      # Alt-X quits
./yamledit ../addons/demos/sample.yaml   # the YAML editor of the add-ons''')}
<p>Windows 10 or later, with Visual Studio 2022 or later (see
<a href="porting.html#windows">Building on Windows</a>):</p>
{code('''cd tvision
cmake -B build
cmake --build build --config Release    # lib/tv.lib, lib/tvaddons.lib, bin/*.exe
bin/tvdemo.exe''')}
<h2>What is included</h2>
<table>
<tr><th>Path</th><th>Contents</th></tr>
<tr><td><code>include/</code></td><td>Library headers (<code>tv.h</code> and the class headers); <code>include/compat/</code> holds stand-ins for Borland headers such as <code>dos.h</code>, <code>dir.h</code>, <code>iostream.h</code>.</td></tr>
<tr><td><code>source/</code></td><td>Library sources; <code>source/platform/</code> is the new terminal driver: the shared part (screen output, key codes, code pages), <code>posix/</code> for Linux and <code>win32/</code> for the Windows console.</td></tr>
<tr><td><code>demos/</code></td><td>TVDEMO, TVEDIT, TVFORMS (with the form generators), HELLO.</td></tr>
<tr><td><code>addons/</code></td><td>New classes built on the library, in <code>lib/libtvaddons.a</code> with the header <code>tvaddons.h</code>: <code>TAdvancedEditor</code>, <code>TBlockIndicator</code>, <code>TLineNumbers</code>, <code>TColoredEditor</code>, <code>TYamlEditor</code>, <code>TYamlEditWindow</code>, <code>editorDialogs()</code>, <code>TLogView</code>, <code>TTableView</code>, <code>TTableHeader</code>, <code>TValidatedInputLine</code> and its validators (also IP address, network, mask, host, e-mail, password), <code>TProgressBar</code>, <code>TProgressDialog</code>, <code>TTabView</code>, <code>TTreeView</code>, <code>TComboBox</code>, <code>TComboButton</code>, <code>TAppSettings</code>, <code>TPage</code>, <code>TWizard</code>, <code>TProcess</code>, <code>TProcessView</code>, <code>TProcessDialog</code>, <code>TCommandPalette</code>, <code>TMarkdownView</code>, <code>TMarkdownWindow</code>, <code>TDateInput</code>, <code>TDateButton</code>, <code>TSplitter</code>, the themes, <code>TToast</code>, <code>TStatusMessageLine</code>, <code>TSyntaxEditor</code>, <code>TSyntaxEditWindow</code>; the <code>yamledit</code>, <code>validate</code> and <code>controls</code> demos and tests (<code>make check-addons</code>).</td></tr>
<tr><td><code>docdemos/</code></td><td>The examples from the Turbo Vision guide (TVGUID01–19, TVG17B).</td></tr>
<tr><td><code>help/</code></td><td>TVHC, the help compiler, and the demo help source.</td></tr>
<tr><td><code>tools/</code></td><td><code>keytest</code>, tmux test scripts, the converter used for the port, this documentation's generator.</td></tr>
<tr><td><code>Makefile</code>, <code>CMakeLists.txt</code></td><td>The build: make on Linux, CMake with Visual Studio on Windows.</td></tr>
</table>
"""
    return page("Turbo Vision for Linux", nav, body)


def main():
    os.makedirs(OUT, exist_ok=True)
    files = {
        "style.css": CSS.lstrip(),
        "index.html": index_page(),
        "classes.html": classes_page(),
        "porting.html": porting_page(),
    }
    for name, text in files.items():
        with open(os.path.join(OUT, name), "w", encoding="utf-8", newline="\n") as f:
            f.write(text)
        print("wrote", os.path.normpath(os.path.join(OUT, name)))


if __name__ == "__main__":
    main()
