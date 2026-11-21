#!/usr/bin/env python3
"""One-shot mechanical conversion of the Borland Turbo Vision sources to a
GCC-friendly tree under tvision/.

  * file names are lowercased, CRLF -> LF, trailing ^Z removed
  * #include names are lowercased, '\\' -> '/'
  * Borland pragmas are dropped
  * near / far / huge / _FAR / _Cdecl memory-model keywords are dropped
  * implicit-int constants (`const foo = 1;`) get an explicit `int`
  * `friend Foo;` becomes `friend class Foo;`
  * high-bit (CP437) bytes inside string/char literals become octal escapes,
    inside comments they are re-encoded as UTF-8

Usage:  python tools/convert.py <original-root> <tvision-root>
        (here: python tools/convert.py ../tvision-original .)
Already existing destination files are NOT overwritten unless --force.
"""

import os
import re
import sys

LIB_SKIP = {"GENINC.CPP"}

DEMOS = ["ASCII", "CALC", "CALENDAR", "FILEVIEW", "GADGETS", "MOUSEDLG",
         "PUZZLE", "TVDEMO1", "TVDEMO2", "TVDEMO3", "TVEDIT1", "TVEDIT2",
         "TVEDIT3", "HELLO",
         "TVFORMS", "DATACOLL", "FIELDS", "LISTDLG", "FORMS", "GENFORM"]
DEMO_H = ["ASCII", "CALC", "CALENDAR", "FILEVIEW", "GADGETS", "MOUSEDLG",
          "PUZZLE", "TVDEMO", "TVEDIT", "TVCMDS", "DEMOHELP",
          "DATACOLL", "FIELDS", "LISTDLG", "FORMS", "GENPARTS", "GENPHONE",
          "FORMCMDS"]
DOCDEMOS = ["TVGUID%02d" % i for i in range(1, 20)] + ["TVG17B"]

KEYWORDS = re.compile(r"\b(near|far|huge|_FAR|_Cdecl|_export)\b\s*")
IMPLICIT_INT = re.compile(r"\bconst(\s+)([A-Za-z_]\w*)(\s*=)")
FRIEND = re.compile(r"\bfriend\s+([A-Za-z_]\w*)\s*;")
INCLUDE = re.compile(r'^(\s*#\s*include\s*)([<"])([^>"]+)([>"])', re.M)
PRAGMA = re.compile(r"^\s*#\s*pragma\s+(option|warn|saveregs|hdrstop|inline|argsused|startup|exit)\b.*$", re.M)
TYPE_WORDS = {"int", "char", "short", "long", "unsigned", "signed", "ushort",
              "uchar", "Boolean", "float", "double", "void", "TPoint",
              "TRect", "size_t", "ccIndex"}


def split_tokens(text):
    """Yield (kind, chunk) with kind in code/comment/literal."""
    i, n, start = 0, len(text), 0
    while i < n:
        c = text[i]
        if c == "/" and i + 1 < n and text[i + 1] in "/*":
            if start < i:
                yield "code", text[start:i]
            if text[i + 1] == "/":
                j = text.find("\n", i)
                j = n if j < 0 else j
            else:
                j = text.find("*/", i + 2)
                j = n if j < 0 else j + 2
            yield "comment", text[i:j]
            i = start = j
        elif c in "\"'":
            if start < i:
                yield "code", text[start:i]
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            j = min(j + 1, n)
            yield "literal", text[i:j]
            i = start = j
        else:
            i += 1
    if start < n:
        yield "code", text[start:]


def fix_code(chunk):
    chunk = KEYWORDS.sub("", chunk)

    def implicit(m):
        before = chunk[:m.start()].rstrip()
        if m.group(2) in TYPE_WORDS or before.endswith(("*", "&")):
            return m.group(0)
        return "const int" + m.group(1) + m.group(2) + m.group(3)
    chunk = IMPLICIT_INT.sub(implicit, chunk)
    chunk = FRIEND.sub(lambda m: m.group(0) if m.group(1) == "class"
                       else "friend class %s;" % m.group(1), chunk)
    return chunk


def fix_literal(chunk):
    return "".join("\\%03o" % ord(ch) if ord(ch) >= 0x80 else ch for ch in chunk)


def fix_comment(chunk):
    if all(ord(ch) < 0x80 for ch in chunk):
        return chunk
    return chunk.encode("latin-1").decode("cp437")


def convert_text(raw):
    text = raw.replace(b"\r\n", b"\n").replace(b"\r", b"\n").rstrip(b"\x1a")
    text = text.decode("latin-1")
    text = PRAGMA.sub("", text)
    text = INCLUDE.sub(lambda m: m.group(1) + m.group(2) +
                       m.group(3).lower().replace("\\", "/") + m.group(4), text)
    out = []
    for kind, chunk in split_tokens(text):
        if kind == "code":
            out.append(fix_code(chunk))
        elif kind == "literal":
            out.append(fix_literal(chunk))
        else:
            out.append(fix_comment(chunk))
    return "".join(out)


def copy(src, dst, force, binary=False):
    if os.path.exists(dst) and not force:
        return
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with open(src, "rb") as f:
        raw = f.read()
    if binary:
        data = raw
    else:
        data = convert_text(raw).encode("utf-8")
    with open(dst, "wb") as f:
        f.write(data)


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    force = "--force" in sys.argv
    src, dst = args
    for name in os.listdir(os.path.join(src, "INCLUDE")):
        if name.upper().endswith(".H"):
            copy(os.path.join(src, "INCLUDE", name),
                 os.path.join(dst, "include", name.lower()), force)
    for name in os.listdir(os.path.join(src, "SOURCE")):
        if name.upper().endswith(".CPP") and name.upper() not in LIB_SKIP:
            copy(os.path.join(src, "SOURCE", name),
                 os.path.join(dst, "source", name.lower()), force)
    for base in DEMOS:
        copy(os.path.join(src, "DEMOS", base + ".CPP"),
             os.path.join(dst, "demos", base.lower() + ".cpp"), force)
    for base in DEMO_H:
        copy(os.path.join(src, "DEMOS", base + ".H"),
             os.path.join(dst, "demos", base.lower() + ".h"), force)
    copy(os.path.join(src, "DEMOS", "DEMOHELP.HLP"),
         os.path.join(dst, "demos", "demohelp.hlp"), force, binary=True)
    for name in ("TVHC.CPP", "TVHC.H"):
        copy(os.path.join(src, "HELP", name),
             os.path.join(dst, "help", name.lower()), force)
    # help source text: CP437 bytes and DOS line ends are kept as they are
    copy(os.path.join(src, "HELP", "DEMOHELP.TXT"),
         os.path.join(dst, "help", "demohelp.txt"), force, binary=True)
    for base in DOCDEMOS:
        copy(os.path.join(src, "DOCDEMOS", base + ".CPP"),
             os.path.join(dst, "docdemos", base.lower() + ".cpp"), force)


if __name__ == "__main__":
    main()
