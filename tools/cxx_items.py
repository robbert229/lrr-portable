#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Split a C/C++ source file into top-level items (stdlib only).

``tools/relocate_unit.py`` moves whole top-level items between files: function
definitions, declarations, type definitions and preprocessor directives. This
module finds them without a compiler. It is a scanner, not a parser: it masks
comments and literals, follows braces, semicolons and preprocessor
conditionals, and records for each item the names it defines and the
identifiers it uses. Anything it cannot place with certainty is recorded in
``Item.problems``; callers refuse to move such an item.

    from cxx_items import scan
    unit = scan(text)
    for item in unit.items:
        print(item.kind, item.names, item.line, item.problems)

Supported shapes:

* function definitions, including ``__declspec(naked)`` bodies with
  ``__asm`` blocks;
* declarations of variables, prototypes, typedefs, struct/union/enum/class
  definitions;
* ``#define`` macros (with continuation lines), ``#include``, other
  directives;
* ``extern "C" { }`` and ``namespace [name] { }`` blocks, whose members are
  items with a ``container``;
* a conditional group that wraps alternative definitions of one function
  (``#if A`` def ``#else`` def ``#endif``), merged into one ``function`` item;
* a top-level, non-nested conditional group whose items form one cluster
  (macro alternatives, a single conditional declaration or datum, a group
  with ``#include`` lines), merged into one item with ``cgroup`` set (#501).
  Macros, types and prototypes count as one cluster; data must connect to
  them, or to each other, by name. It moves whole and verbatim; ``parts``
  keeps its members, each with the index of its branch, for guarded
  declarations.

Anything else inside ``#if`` blocks keeps a ``cond`` depth above zero and
names its group (``in_group``, ``group_why``), and an item whose head or
body is split by directives in a way that changes brace depth is marked with
a problem.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field

IDENT = re.compile(r"[A-Za-z_]\w*")
ANNOTATION = re.compile(
    r"^\s*//\s*(FUNCTION|STUB|LIBRARY|GLOBAL|STRING|VTABLE|TEMPLATE|SYNTHETIC):"
    r"\s+LEGORR\s+0x([0-9a-fA-F]+)", re.M)
DIRECTIVE = re.compile(r"[ \t]*#[ \t]*(\w*)")
CONTROL = {"if", "for", "while", "switch", "catch", "sizeof", "return", "do", "else"}
DECL_STRIP = re.compile(r"__declspec\s*\((?:[^()]|\([^()]*\))*\)|__attribute__\s*\(\((?:[^()]|\([^()]*\))*\)\)")
TYPE_KEYWORDS = ("struct", "union", "enum", "class")
NESTED_NAMESPACES = re.compile(r"(?:ns:[A-Za-z_]\w*/)+(?:ns:[A-Za-z_]\w*|anon)")


# --------------------------------------------------------------------------
# Lexical masking


def mask(text: str) -> str:
    """Blank comments and the contents of string and character literals.

    Offsets and newlines are kept, so positions in the result are positions in
    ``text``. Quote characters stay, so ``extern "C"`` reads ``extern " "``.
    A quote with no closing quote on its line is left alone: ``;`` comments
    in ``__asm`` blocks may contain apostrophes.
    """
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            j = text.find("\n", i)
            j = n if j < 0 else j
            for k in range(i, j):
                out[k] = " "
            i = j
        elif c == "/" and i + 1 < n and text[i + 1] == "*":
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                if text[k] != "\n":
                    out[k] = " "
            i = j
        elif c in "\"'":
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            if j < n and text[j] == c:
                for k in range(i + 1, j):
                    if text[k] != "\n":
                        out[k] = " "
                i = j + 1
            else:
                i += 1
        else:
            i += 1
    return "".join(out)


def strip_comments(text: str) -> str:
    """``text`` without comments (literals kept), whitespace collapsed."""
    def blank(m):
        s = m.group(0)
        return s if s[0] in "\"'" else " "
    text = re.sub(r'"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'|/\*.*?\*/|//[^\n]*', blank, text, flags=re.S)
    return re.sub(r"\s+", " ", text).strip()


def normalise(text: str) -> str:
    """Line endings and whitespace normalised, for verbatim comparisons."""
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    return re.sub(r"\s+", " ", text).strip()


# --------------------------------------------------------------------------
# Items


@dataclass
class Item:
    kind: str                 # function, declaration, macro, include, directive
    start: int                # first character of the item's code
    end: int                  # one past the item, through its line end
    lead: int = -1            # first character of the attached leading comments
    category: str = ""        # function, var, proto, type, macro, include, other
    names: list[str] = field(default_factory=list)
    uses: set[str] = field(default_factory=set)
    static: bool = False      # internal linkage keyword (static)
    inline: bool = False
    cond: int = 0             # depth of enclosing top-level #if groups
    container: str | None = None   # None, "extern-c", "anon", "ns:<name>"
    group: bool = False       # a merged #if group of one function's alternatives
    problems: list[str] = field(default_factory=list)
    annotations: list[tuple[str, int]] = field(default_factory=list)
    line: int = 0             # 1-based line of ``start``
    body: int = -1            # offset of a function's opening brace
    multi: bool = False       # declaration with several declarators
    include: str | None = None     # spelled include target, with <> or ""
    decl_start: int = -1      # start of the declaration (a group's first alternative)
    qualifier: str | None = None   # X in an out-of-line definition ``T X::name(...)``
    using: tuple[str, str] | None = None   # ("namespace", X) or ("name", "X::n")
    defines_type: bool = False     # a declaration that also defines a struct/union/enum
    cgroup: bool = False      # a merged top-level #if group of one cluster (#501)
    parts: list["Item"] = field(default_factory=list)   # a cgroup's members, in order
    branch: int = 0           # a cgroup member's branch (0: the #if branch)
    condition: str = ""       # a cgroup's #if/#ifdef/#ifndef directive, verbatim
    endif: str = ""           # a cgroup's #endif line, verbatim
    branches: int = 0         # a cgroup's number of branches
    in_group: int = 0         # line of the unmerged top-level #if group holding the item
    group_why: str = ""       # why that group was not merged

    def head(self, source: str) -> str:
        """A function's text before its body, or a declaration without ``;``."""
        start = self.decl_start if self.decl_start >= 0 else self.start
        if self.category == "function":
            return source[start:self.body]
        return source[start:source.rindex(";", start, self.end)]

    def text(self, source: str) -> str:
        """The item with its leading comments, verbatim."""
        return source[self.lead:self.end]

    def code(self, source: str) -> str:
        """The item without its leading comments, verbatim."""
        return source[self.start:self.end]


@dataclass
class Group:
    """A top-level preprocessor conditional group."""

    start: int                # start of the #if line
    end: int = -1             # one past the #endif line
    if_end: int = -1          # one past the #if directive
    endif_start: int = -1     # start of the #endif line
    branches: list[list[int]] = field(default_factory=list)   # item indices
    nested: bool = False
    in_item: bool = False


@dataclass
class Scan:
    text: str
    masked: str
    items: list[Item]
    groups: list[Group]
    problems: list[str]       # file-level problems (unbalanced braces...)

    def annotated(self) -> dict[int, Item]:
        """Address -> item for every annotation attached to an item."""
        out = {}
        for item in self.items:
            for _, addr in item.annotations:
                out[addr] = item
        return out


def _directive_end(text: str, start: int) -> int:
    """One past the last line of the preprocessor directive at ``start``.

    A directive continues past a line that ends in a backslash, and past a
    line that ends inside a block comment: a ``/* */`` comment that begins in
    a directive belongs to it until it closes, and the directive continues
    if a backslash follows the comment's end on its last line.
    """
    n = len(text)
    i = start
    in_comment = False
    while i < n:
        e = text.find("\n", i)
        e = n if e < 0 else e
        k = i
        line_end_code = i          # where the code part of the line ends
        while k < e:
            if in_comment:
                close = text.find("*/", k, e)
                if close < 0:
                    k = e
                    break
                in_comment = False
                k = close + 2
                line_end_code = k
                continue
            c = text[k]
            if text.startswith("/*", k):
                in_comment = True
                k += 2
            elif text.startswith("//", k):
                break
            elif c in "\"'":
                q = k + 1
                while q < e and text[q] != c:
                    q += 2 if text[q] == "\\" else 1
                k = min(q + 1, e)
                line_end_code = k
            else:
                k += 1
                if not c.isspace():
                    line_end_code = k
        if e >= n:
            return n
        if in_comment or text[i:line_end_code].rstrip().endswith("\\"):
            i = e + 1
            continue
        return e + 1
    return n


def _line_of(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def _line_start(text: str, offset: int) -> int:
    return text.rfind("\n", 0, offset) + 1


def _line_end(text: str, offset: int) -> int:
    """One past the newline of the line holding ``offset`` (or the end)."""
    j = text.find("\n", offset)
    return len(text) if j < 0 else j + 1


def _is_function_head(head: str) -> bool:
    h = DECL_STRIP.sub(" ", head).strip()
    h = re.sub(r"\bthrow\s*\([^()]*\)\s*$", "", h).strip()
    h = re.sub(r"\bconst\s*$", "", h).strip()
    if not h.endswith(")") or h.startswith("typedef"):
        return False
    depth = 0
    for ch in h:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        elif ch == "=" and depth == 0:
            return False
    return True


def scan(text: str) -> Scan:
    masked = mask(text)
    n = len(text)
    items: list[Item] = []
    groups: list[Group] = []
    problems: list[str] = []
    containers: list[str] = []
    frames: list[dict] = []           # open #if frames
    depth = 0
    cur: int | None = None            # start of the item being read
    cur_problems: list[str] = []
    brace_kind = None                 # body or agg for the open top-level brace
    body_at = -1
    i = 0
    at_line_start = True

    def top_frames():
        return [f for f in frames if f["top"]]

    def finish(end: int, kind: str):
        nonlocal cur, cur_problems, brace_kind, body_at
        stop = end
        rest = masked[end:_line_end(text, end)]
        if rest.strip():
            cur_problems.append("shares its last line with other code")
        else:
            stop = _line_end(text, end)
        item = Item(kind=kind, start=cur, end=stop, problems=list(cur_problems),
                    cond=len(top_frames()), container="/".join(containers) or None,
                    body=body_at if kind == "function" else -1)
        if masked[_line_start(text, cur):cur].strip():
            item.problems.append("starts on a line with other code")
        items.append(item)
        for f in top_frames():
            f["group"].branches[-1].append(len(items) - 1)
        cur, cur_problems, brace_kind, body_at = None, [], None, -1

    while i < n:
        if at_line_start:
            m = DIRECTIVE.match(masked, i)
            if m:
                stop = _directive_end(text, i)
                word = m.group(1)
                if word in ("if", "ifdef", "ifndef", "elif", "else", "endif") and depth == 0 and cur is not None:
                    cur_problems.append(f"a #{word} directive splits the declaration")
                if word in ("if", "ifdef", "ifndef"):
                    top = depth == 0 and cur is None
                    g = Group(start=i, in_item=not top, if_end=stop)
                    g.branches.append([])
                    if top:
                        for f in top_frames():
                            f["group"].nested = True
                    frames.append({"depth": depth, "ends": [], "top": top, "group": g})
                elif word in ("elif", "else"):
                    if not frames:
                        problems.append(f"line {_line_of(text, i)}: #{word} without #if")
                    else:
                        f = frames[-1]
                        f["ends"].append(depth)
                        depth = f["depth"]
                        if f["top"]:
                            f["group"].branches.append([])
                elif word == "endif":
                    if not frames:
                        problems.append(f"line {_line_of(text, i)}: #endif without #if")
                    else:
                        f = frames.pop()
                        f["ends"].append(depth)
                        if len(set(f["ends"])) > 1:
                            msg = (f"#if at line {_line_of(text, f['group'].start)} changes brace depth "
                                   "differently in its branches")
                            if cur is not None:
                                cur_problems.append(msg)
                            else:
                                problems.append(msg)
                        depth = f["ends"][0]
                        if f["top"]:
                            f["group"].end = stop
                            f["group"].endif_start = i
                            groups.append(f["group"])
                elif depth == 0 and cur is None:
                    item = Item(kind="directive", start=i, end=stop, cond=len(top_frames()),
                                container="/".join(containers) or None, category="other")
                    if word == "define":
                        dm = re.match(r"\s*#\s*define\s+([A-Za-z_]\w*)", masked[i:stop])
                        item.kind = item.category = "macro"
                        if dm:
                            item.names = [dm.group(1)]
                            body = masked[i + dm.end():stop]
                            item.uses = set(IDENT.findall(body)) - {dm.group(1)}
                    elif word == "include":
                        im = re.match(r'\s*#\s*include\s*([<"][^>"]+[>"])', text[i:stop])
                        item.kind = item.category = "include"
                        item.include = im.group(1) if im else None
                    elif word == "undef":
                        um = re.match(r"\s*#\s*undef\s+([A-Za-z_]\w*)", masked[i:stop])
                        if um:
                            item.uses = {um.group(1)}
                        item.category = "undef"
                    items.append(item)
                    for f in top_frames():
                        f["group"].branches[-1].append(len(items) - 1)
                elif depth == 0 and cur is not None and brace_kind is None:
                    cur_problems.append(f"a #{word} directive interrupts the declaration")
                i = stop
                at_line_start = True
                continue
        c = masked[i]
        if c == "\n":
            at_line_start = True
            i += 1
            continue
        at_line_start = False
        if c in " \t\r\f\v":
            i += 1
            continue
        if cur is None:
            if c == "}" and depth == 0 and containers:
                containers.pop()
                i += 1
                continue
            if c == ";" and depth == 0:
                i += 1       # stray semicolon (after a container or function)
                continue
            cur = i
        if c == "{":
            if depth == 0:
                head = text[cur:i]
                hm = masked[cur:i]
                if re.fullmatch(r'\s*extern\s*"C"\s*', head):
                    containers.append("extern-c")
                    cur = None
                    i += 1
                    continue
                nm = re.fullmatch(r"\s*namespace\s*([A-Za-z_]\w*)?\s*", hm)
                if nm:
                    containers.append("ns:" + nm.group(1) if nm.group(1) else "anon")
                    cur = None
                    i += 1
                    continue
                brace_kind = "body" if _is_function_head(hm) else "agg"
                if brace_kind == "body":
                    body_at = i
            depth += 1
        elif c == "}":
            if depth == 0:
                problems.append(f"line {_line_of(text, i)}: unbalanced closing brace")
            else:
                depth -= 1
                if depth == 0 and brace_kind == "body":
                    finish(i + 1, "function")
        elif c == ";" and depth == 0:
            finish(i + 1, "declaration")
        i += 1
    if cur is not None:
        problems.append(f"line {_line_of(text, cur)}: unterminated item at end of file")
    if frames:
        problems.append("unterminated #if at end of file")
    if depth:
        problems.append("unbalanced braces at end of file")
    for item in items:
        _describe(item, text, masked)
        if item.container and "/" in item.container and not NESTED_NAMESPACES.fullmatch(item.container):
            # Named namespaces nest (scope A::B, #514), optionally with an
            # anonymous namespace innermost (internal linkage); a linkage
            # block or an anonymous namespace inside another block does not.
            item.problems.append(f"nested namespace or linkage block ({item.container})")
        elif item.container and "/" in item.container:
            names = [p[3:] for p in item.container.split("/") if p.startswith("ns:")]
            if len(set(names)) < len(names):
                # namespace X { namespace X { } }: inside the outer X, a
                # qualified X:: names the inner one once it is declared, so
                # lookups depend on position and a relocation, which
                # reorders items, can change them (#514).
                item.problems.append(f"nested namespace or linkage block ({item.container}): namespace "
                                     f"{names[-1]} is nested in itself, so qualified {names[-1]}:: lookups "
                                     "depend on position; close the outer namespace first (a source fix)")
    items = _merge_groups(items, groups, text)
    for item in items:
        item.lead = _lead(item, text, masked)
        item.line = _line_of(text, item.start)
        for m in ANNOTATION.finditer(text, item.lead, item.end if item.group or item.cgroup else item.start):
            item.annotations.append((m.group(1), int(m.group(2), 16)))
        if item.cgroup and ANNOTATION.search(text, item.start, item.end):
            item.problems.append(f"the #if group at line {item.line} holds an annotated item; "
                                 "only unannotated groups move whole")
    return Scan(text, masked, items, groups, problems)


def _lead(item: Item, text: str, masked: str) -> int:
    """Start of the comment lines directly above ``item`` (no blank line)."""
    lead = _line_start(text, item.start)
    while lead > 0:
        prev = _line_start(text, lead - 1)
        line, mline = text[prev:lead], masked[prev:lead]
        if not line.strip() or mline.strip():
            break
        lead = prev
    return lead


# --------------------------------------------------------------------------
# Names and uses


QUALIFIED = re.compile(r"(?:\b[A-Za-z_]\w*\s*::\s*)+[A-Za-z_]\w*|(?<![\w:])::\s*[A-Za-z_]\w*|[A-Za-z_]\w*")


def _uses(masked: str) -> set[str]:
    """Names used in ``masked``.

    A bare identifier is reported as itself and a qualified one as
    ``X::n`` (``::n`` reads as ``n``). Member names after ``.`` and ``->``
    are skipped.
    """
    out = set()
    for m in QUALIFIED.finditer(masked):
        k = m.start() - 1
        while k >= 0 and masked[k] in " \t\r\n":
            k -= 1
        if k >= 0 and (masked[k] == "." or (masked[k] == ">" and k > 0 and masked[k - 1] == "-")):
            continue
        name = re.sub(r"\s+", "", m.group(0)).lstrip(":")
        out.add(name)
    return out


def _split_top(text: str, sep: str = ",") -> list[str]:
    parts, depth, cur = [], 0, []
    for ch in text:
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        if ch == sep and depth == 0:
            parts.append("".join(cur))
            cur = []
        else:
            cur.append(ch)
    parts.append("".join(cur))
    return parts


def _strip_braced(text: str) -> tuple[str, list[str]]:
    """Remove top-level ``{...}`` bodies; return the rest and the bodies."""
    out, bodies, depth, start = [], [], 0, 0
    for k, ch in enumerate(text):
        if ch == "{":
            if depth == 0:
                start = k
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                bodies.append(text[start + 1:k])
                out.append(" ")
                continue
        if depth == 0 and ch != "}":
            out.append(ch)
    return "".join(out), bodies


def _function_name(head: str) -> tuple[str | None, list[str], str | None]:
    """Name declared by a function head (text before the body or ``;``).

    The parameter list is the last top-level parenthesised group; the name is
    the identifier just before it. Any other top-level group before the name
    is a macro invocation or a complex declarator.
    """
    problems = []
    h = DECL_STRIP.sub(lambda m: " " * len(m.group(0)), head).rstrip()
    h = re.sub(r"\bthrow\s*\([^()]*\)\s*$", "", h).rstrip()
    h = re.sub(r"\bconst\s*$", "", h).rstrip()
    groups, depth, start = [], 0, 0
    for k, ch in enumerate(h):
        if ch == "(":
            if depth == 0:
                start = k
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                groups.append(start)
    if not groups:
        return None, ["no parameter list"], None
    p = groups[-1]
    m = re.search(r"([A-Za-z_]\w*)\s*$", h[:p])
    if not m or m.group(1) in CONTROL:
        return None, ["complex declarator (function pointer or macro before the name)"], None
    before = h[:m.start()]
    qualifier = None
    q = re.search(r"((?:[A-Za-z_]\w*\s*::\s*)+)$", before)
    if q:
        parts = [x for x in re.split(r"\s*::\s*", q.group(1)) if x]
        if len(parts) > 1:
            problems.append("qualified definition with several levels (" + "::".join(parts) + ")")
        qualifier = parts[-1]
        before = before[:q.start()]
    if "(" in before or ")" in before:
        problems.append("macro or complex declarator before the function name")
    return m.group(1), problems, qualifier


def _describe(item: Item, text: str, masked: str) -> None:
    if item.kind in ("macro", "include", "directive"):
        return
    code = masked[item.start:item.end]
    item.uses = _uses(code)
    head = code if item.kind == "declaration" else masked[item.start:item.body]
    first = DECL_STRIP.sub(" ", head).split()
    lead_words = []
    for w in first:
        if not re.fullmatch(r"[A-Za-z_]\w*", w):
            break
        lead_words.append(w)
    item.static = "static" in lead_words[:4]
    item.inline = any(w in ("inline", "__inline", "__forceinline") for w in lead_words[:4])
    if item.kind == "function":
        item.category = "function"
        name, probs, item.qualifier = _function_name(head)
        item.names = [name] if name else []
        item.problems.extend(probs)
        item.uses -= set(item.names)
        if item.qualifier:
            item.uses.discard(f"{item.qualifier}::{name}")
            item.uses.add(item.qualifier)
        return
    decl = code.rstrip().rstrip(";")
    stripped = decl.strip()
    words = stripped.split()
    if words and words[0] == "typedef":
        item.category = "type"
        rest, bodies = _strip_braced(stripped[len("typedef"):])
        names = []
        tm = re.match(r"\s*(?:struct|union|enum|class)\s+([A-Za-z_]\w*)", rest)
        if tm:
            names.append(tm.group(1))
        for part in _split_top(rest):
            # (*Name), (__stdcall *Name) or (CONV_MACRO *Name) (#501)
            fp = re.search(r"\(\s*(?:[A-Za-z_]\w*\s+)*\*\s*([A-Za-z_]\w*)\s*(?:\[[^\]]*\]\s*)*\)",
                           DECL_STRIP.sub(" ", part))
            if fp:
                names.append(fp.group(1))
                continue
            part = re.sub(r"\[[^\]]*\]", " ", part)
            m = re.search(r"([A-Za-z_]\w*)\s*$", part)
            if m:
                names.append(m.group(1))
        if re.match(r"\s*enum\b", rest):
            for body in bodies:
                names.extend(_enumerators(body))
        item.names = list(dict.fromkeys(names))
        if not item.names:
            item.problems.append("a typedef whose name the scanner cannot find")
    elif words and words[0] in TYPE_KEYWORDS and "{" in stripped:
        rest, bodies = _strip_braced(stripped)
        tm = re.match(r"\s*(?:struct|union|enum|class)\s+([A-Za-z_]\w*)", rest)
        names = [tm.group(1)] if tm else []
        if words[0] == "enum":
            for body in bodies:
                names.extend(_enumerators(body))
        tail = re.sub(r"^\s*(?:struct|union|enum|class)\s*(?:[A-Za-z_]\w*)?\s*", "", rest)
        declarators = [p for p in _split_top(tail) if p.strip()]
        if declarators:
            item.category = "var"
            item.defines_type = True
            for p in declarators:
                m = re.search(r"([A-Za-z_]\w*)\s*(?:\[[^\]]*\]\s*)*(?:=.*)?$", p.strip(), re.S)
                if m:
                    names.append(m.group(1))
        else:
            item.category = "type"
        item.names = list(dict.fromkeys(names))
    elif words and words[0] in TYPE_KEYWORDS and len(words) == 2:
        item.category = "type"          # forward declaration: struct X;
        item.names = [words[1]]
    elif words and words[0] == "using":
        um = re.fullmatch(r"using\s+namespace\s+([A-Za-z_]\w*)", stripped)
        un = re.fullmatch(r"using\s+((?:[A-Za-z_]\w*\s*::\s*)+[A-Za-z_]\w*)", stripped)
        ua = re.fullmatch(r"using\s+([A-Za-z_]\w*)\s*=.*", stripped, re.S)
        item.category = "using"
        if um:
            item.using = ("namespace", um.group(1))
            item.uses = {um.group(1)}
        elif un:
            item.using = ("name", re.sub(r"\s+", "", un.group(1)))
            item.uses = {item.using[1]}
        elif ua:
            item.category = "type"         # alias declaration: using T = ...;
            item.names = [ua.group(1)]
        else:
            item.category = "other"
            item.problems.append("unrecognised 'using' declaration")
    elif words and words[0] in ("template", "static_assert"):
        item.category = "other"
        item.problems.append(f"'{words[0]}' declaration")
    else:
        parts = _split_top(DECL_STRIP.sub(" ", stripped))
        first_part = parts[0]
        no_init = _split_top(first_part, "=")[0]
        fp = re.search(r"\(\s*(?:__\w+\s+)?\*\s*([A-Za-z_]\w*)\s*\)", no_init)
        if "(" in no_init and not fp:
            item.category = "proto"
            name, probs, item.qualifier = _function_name(no_init)
            item.problems.extend(probs)
            names = [name] if name else []
            if len(parts) > 1:
                item.multi = True
                item.problems.append("declares several functions in one declaration")
        else:
            item.category = "var"
            names = []
            tagged = re.search(r"\b(?:struct|union|enum|class)\s*([A-Za-z_]\w*)?\s*\{", no_init)
            if tagged:
                # static struct T { ... } v[N]; defines T as well as v.
                item.defines_type = True
                if tagged.group(1):
                    names.append(tagged.group(1))
                parts = [_strip_braced(part)[0] for part in parts]
            for part in parts:
                part = _split_top(part, "=")[0]
                f2 = re.search(r"\(\s*(?:__\w+\s+)?\*\s*([A-Za-z_]\w*)\s*\)", part)
                if f2:
                    names.append(f2.group(1))
                    continue
                part = re.sub(r"\[[^\]]*\]", " ", part)
                m = re.search(r"([A-Za-z_]\w*)\s*$", part)
                if m:
                    names.append(m.group(1))
            item.multi = len(parts) > 1
        item.names = names
    item.uses -= set(item.names)


def _enumerators(body: str) -> list[str]:
    out = []
    for part in _split_top(body):
        m = re.match(r"\s*([A-Za-z_]\w*)", part)
        if m:
            out.append(m.group(1))
    return out


def _merge_groups(items: list[Item], groups: list[Group], text: str) -> list[Item]:
    """Merge #if groups that hold alternative definitions of one function."""
    drop: set[int] = set()
    merged: dict[int, Item] = {}
    for g in groups:
        if g.in_item or g.end < 0:
            continue
        names = set()
        ok = not g.nested
        members = []
        for branch in g.branches if ok else []:
            if len(branch) != 1:
                ok = False
                break
            it = items[branch[0]]
            if it.kind != "function" or len(it.names) != 1 or it.cond < 1:
                ok = False
                break
            names.add(it.names[0])
            members.append(branch[0])
        if not ok or len(names) != 1 or not members or len({items[k].cond for k in members}) != 1:
            cluster, why = _cluster_group(g, items, text)
            if cluster is not None:
                merged[cluster[0]] = cluster[1]
                drop.update(k for b in g.branches for k in b)
            else:
                line = _line_of(text, g.start)
                # Groups are listed innermost first: the outermost one names
                # the item's group.
                for b in g.branches:
                    for k in b:
                        items[k].in_group, items[k].group_why = line, why
            continue
        parts = [items[k] for k in members]
        first = parts[0]
        item = Item(kind="function", start=g.start, end=g.end, category="function",
                    names=list(names), cond=first.cond - 1, container=first.container,
                    group=True, body=first.body, decl_start=first.start)
        for p in parts:
            item.uses |= p.uses
            item.static = item.static or p.static
            item.inline = item.inline or p.inline
            item.problems.extend(x for x in p.problems if x not in item.problems)
        if any(p.static for p in parts) and not all(p.static for p in parts):
            item.problems.append("the #if alternatives disagree on static")
        merged[members[0]] = item
        drop.update(members)
    out = []
    for k, it in enumerate(items):
        if k in merged:
            out.append(merged[k])
        elif k not in drop:
            out.append(it)
    return out



CLUSTER_CATEGORIES = ("macro", "include", "type", "proto", "var", "function")


def _cluster_group(g: Group, items: list[Item], text: str) -> tuple[tuple[int, Item] | None, str]:
    """Merge a top-level group whose items form one cluster (#501), or say why not.

    The group moves whole, so its members must belong together: every item
    other than an ``#include`` shares a name with, or uses a name of, another
    one, and the ``#include`` lines come along with them. Function
    definitions (other than whole alternatives, merged above), ``#undef``
    and other directives, nested groups and include guards are not merged.
    """
    if g.in_item or g.end < 0:
        return None, "inside an item"
    if g.nested:
        return None, "it holds a nested #if group"
    keys = [k for b in g.branches for k in b]
    if not keys:
        return None, "it holds no items"
    members = [items[k] for k in keys]
    masked = mask(text)
    if any(it.cond != 1 for it in members):
        return None, "it is nested in another #if group"
    for it in members:
        if it.category == "function" and ANNOTATION.search(
                text, _lead(Item(kind="", start=it.start, end=it.start), text, masked), it.start):
            return None, f"it holds the annotated function definition {', '.join(it.names) or '?'} besides other items"
        if it.category not in CLUSTER_CATEGORIES:
            word = DIRECTIVE.match(text, it.start)
            return None, f"it holds a #{word.group(1) if word else it.category} directive"
    if all(it.category == "include" for it in members):
        return None, "it holds only #include lines"
    guard = re.match(r"\s*#\s*ifndef\s+(\w+)\s*$", text[g.start:g.if_end])
    if guard and members[0].category == "macro" and members[0].names == [guard.group(1)]:
        return None, "it is an include guard"
    if len({it.container for it in members}) != 1 or len({it.cond for it in members}) != 1:
        return None, "its items are in different scopes"
    # One cluster: union by shared names and by uses of each other's names.
    # Header-safe members (macros, types, prototypes) always belong together:
    # they can all go to the private header (the plan's P2 allows two macros
    # per branch). So do unannotated helper functions (#514: G98-009's COM
    # helpers and the GNU branch's typedefs). Data must connect by names.
    named = [k for k, it in enumerate(members) if it.category != "include"]
    parent = {k: k for k in named}

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x
    safe = [k for k in named if members[k].category in ("macro", "type", "proto", "function")]
    for k in safe[1:]:
        parent[find(k)] = find(safe[0])
    owner: dict[str, int] = {}
    for k in named:
        for n in members[k].names:
            if n in owner:
                parent[find(k)] = find(owner[n])
            else:
                owner[n] = k
    for k in named:
        for u in members[k].uses:
            o = owner.get(u)
            if o is not None:
                parent[find(k)] = find(o)
    roots = {find(k) for k in named}
    if len(roots) > 1:
        return None, f"its items form {len(roots)} unrelated clusters"
    branch_of = {k: b for b, branch in enumerate(g.branches) for k in branch}
    parts = []
    for k in keys:
        it = items[k]
        it.branch = branch_of[k]
        parts.append(it)
    cats = {it.category for it in parts}
    category = next(c for c in ("function", "var", "proto", "type", "macro") if c in cats)
    names = list(dict.fromkeys(n for it in parts for n in it.names))
    cond_uses = set()
    for m in DIRECTIVE_LINE.finditer(masked, g.start, g.end):
        if m.group(1) in ("if", "ifdef", "ifndef", "elif"):
            cond_uses |= set(IDENT.findall(m.group(2)))
    cond_uses -= {"defined"}
    item = Item(kind="macro" if category == "macro" else "declaration", start=g.start, end=g.end,
                category=category, names=names, cond=parts[0].cond - 1, container=parts[0].container,
                cgroup=True, parts=parts, condition=text[g.start:g.if_end],
                endif=text[g.endif_start:g.end], branches=len(g.branches))
    item.uses = (set().union(*(it.uses for it in parts)) | cond_uses) - set(names)
    item.static = any(it.static for it in parts)
    item.inline = any(it.inline for it in parts)
    item.defines_type = False
    for it in parts:
        item.problems.extend(x for x in it.problems if x not in item.problems)
    return (keys[0], item), ""


DIRECTIVE_LINE = re.compile(r"^[ \t]*#[ \t]*(\w+)([^\n]*(?:\\\n[^\n]*)*)", re.M)
