#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Exhaustive, time-capped and resumable order searches for one function.

Four modes, each a family of semantics-preserving rewrites of the function at
ADDRESS, scored with the single-function match loop (match_function.py):

  stores    the order of independent simple statements within a run of
            one-line assignments (finding 635's store-order search)
  cases     the order of a switch's case blocks (finding 649)
  decls     the order of a block's leading local declarations, and of the
            declarators within one declaration
  operands  the operand order of commutative and mirrored binary operators

Each mode splits the function into groups (one run, one switch, one
declaration list, one operator site). A round visits every group once: it
scores every order of the group (all of them up to --exhaustive items,
single insertion moves above that) with the other groups at their best, and
keeps the best. Rounds repeat until one finds nothing better. The operand
mode then tries pairs of sites once. Candidates are written to the real
source path, compiled and restored, so __FILE__ and the COFF line records are
the original's; never run a search while a build compiles the same tree.

The output directory holds state.json (resume with --resume), the round's
base source, the best source, best.patch and result.json. The state file is
rewritten after every improvement and every few attempts, so an interrupted
search loses little; the budget (--seconds) ends the run with exit status 3
and a resumable state.
"""
from __future__ import annotations

import argparse
import difflib
import hashlib
import itertools
import json
import os
from pathlib import Path
import re
import signal
import sys
import time
from typing import Callable, Iterator

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(Path(__file__).resolve().parent))

import msvc_permuter  # noqa: E402  (mask, function_span)

MODES = ("stores", "cases", "decls", "operands")
STATE_VERSION = 1


# ---------------------------------------------------------------------------
# Shared text helpers


def sha(text: str) -> str:
    return hashlib.sha256(text.encode()).hexdigest()[:16]


def line_starts(text: str) -> list[int]:
    starts = [0]
    for m in re.finditer(r"\n", text):
        starts.append(m.end())
    return starts


def function_span(text: str, address: int) -> tuple[int, int]:
    """The body's [start, end) offsets inside its braces.

    Unlike msvc_permuter's, it allows preprocessor lines between the
    annotation and the signature (`#if defined(SOUND3D_DIRECTSOUND)`)."""
    found = [m for m in re.finditer(r"//\s*(?:FUNCTION|STUB|LIBRARY)\s*:\s*LEGORR\s+(0x[0-9a-fA-F]+)\b", text)
             if int(m.group(1), 16) == address]
    if len(found) != 1:
        raise ValueError("expected exactly one FUNCTION annotation")
    code = msvc_permuter.mask(text)
    start = found[0].end()
    opening = code.find("{", start)
    header = re.sub(r"(?m)^[ \t]*#.*$", "", code[start:opening])
    if opening < 0 or ";" in header or "#" in header:
        raise ValueError("cannot safely locate function body")
    depth = 1
    for i in range(opening + 1, len(code)):
        if code[i] == "{":
            depth += 1
        elif code[i] == "}":
            depth -= 1
            if not depth:
                return opening + 1, i
    raise ValueError("unterminated function")


def body_lines(text: str, address: int) -> tuple[int, int, list[int]]:
    """First and last+1 line index of the function body's whole lines.

    The lines strictly between the line of the opening brace and the line of
    the closing brace."""
    start, end = function_span(text, address)
    starts = line_starts(text)
    first = _line_of(starts, start) + 1
    last = _line_of(starts, end)
    return first, last, starts


def _line_of(starts: list[int], offset: int) -> int:
    lo, hi = 0, len(starts) - 1
    while lo < hi:
        mid = (lo + hi + 1) // 2
        if starts[mid] <= offset:
            lo = mid
        else:
            hi = mid - 1
    return lo


def line_text(text: str, starts: list[int], index: int) -> str:
    end = starts[index + 1] if index + 1 < len(starts) else len(text)
    return text[starts[index]:end]


def line_depths(code: str, starts: list[int], first: int, last: int) -> list[int]:
    """Brace depth at the start of each line in [first, last), relative to first."""
    depths = []
    depth = 0
    for i in range(first, last):
        depths.append(depth)
        seg = line_text(code, starts, i)
        depth += seg.count("{") - seg.count("}")
    return depths


IDENT = r"[A-Za-z_]\w*"
KEYWORDS = {
    "if", "else", "for", "while", "do", "switch", "case", "default", "return",
    "break", "continue", "goto", "sizeof", "typedef", "struct", "union", "enum",
    "static", "extern", "register", "const", "volatile", "unsigned", "signed",
    "int", "long", "short", "char", "float", "double", "void", "__asm",
    "_asm", "asm", "new", "delete", "this", "operator", "throw", "try", "catch",
}
BUILTIN_TYPES = {"int", "long", "short", "char", "float", "double", "unsigned", "signed", "void"}
CALL_RE = re.compile(r"\b(" + IDENT + r")\s*\(")


def has_call(code: str) -> bool:
    return any(m[1] not in ("sizeof",) and m[1] not in KEYWORDS for m in CALL_RE.finditer(code))


# ---------------------------------------------------------------------------
# Groups


class Group:
    """One independently searched part of the function."""

    label = ""

    def candidates(self, text: str) -> Iterator[tuple[str, str]]:
        raise NotImplementedError

    def size(self) -> int:
        raise NotImplementedError


class OrderGroup(Group):
    """Items occupying [start, end) of the text, searched by order.

    `before` holds (i, j) pairs: item i must stay before item j. A pinned
    last item stays last."""

    def __init__(self, kind, label, start, end, items, before=(), pinned_last=False,
                 joiner="", exhaustive=6, names=None):
        self.kind = kind
        self.label = label
        self.start = start
        self.end = end
        self.items = list(items)
        self.before = set(before)
        self.pinned_last = pinned_last
        self.joiner = joiner
        self.exhaustive = exhaustive
        self.names = names or [str(i) for i in range(len(items))]

    def allowed(self, order) -> bool:
        pos = {item: index for index, item in enumerate(order)}
        if self.pinned_last and order[-1] != len(self.items) - 1:
            return False
        return all(pos[i] < pos[j] for i, j in self.before)

    def orders(self) -> Iterator[tuple[int, ...]]:
        n = len(self.items)
        identity = tuple(range(n))
        if n <= self.exhaustive:
            for order in itertools.permutations(range(n)):
                if order != identity and self.allowed(order):
                    yield order
            return
        seen = {identity}
        for i in range(n):
            for j in range(n):
                if i == j:
                    continue
                order = list(identity)
                order.insert(j, order.pop(i))
                order = tuple(order)
                if order not in seen and self.allowed(order):
                    seen.add(order)
                    yield order

    def size(self) -> int:
        return sum(1 for _ in self.orders())

    def apply(self, text: str, order) -> str:
        return text[:self.start] + self.joiner.join(self.items[i] for i in order) + text[self.end:]

    def candidates(self, text):
        for order in self.orders():
            yield " ".join(self.names[i] for i in order), self.apply(text, order)


class EditGroup(Group):
    """A fixed set of non-overlapping replacements, applied together."""

    def __init__(self, label, edits):
        self.label = label
        self.edits = sorted(edits, reverse=True)

    def size(self):
        return 1

    def candidates(self, text):
        out = text
        for a, b, replacement in self.edits:
            out = out[:a] + replacement + out[b:]
        yield self.label, out


# ---------------------------------------------------------------------------
# stores: independent simple statements


ASSIGN_OPS = r"(?:<<=|>>=|[-+*/%&|^]=|=(?!=))"


def split_top(code: str, pattern: str) -> list[str] | None:
    """Split at top-level matches of pattern (outside parentheses/brackets)."""
    parts, depth, last = [], 0, 0
    i = 0
    regex = re.compile(pattern)
    while i < len(code):
        c = code[i]
        if c in "([":
            depth += 1
        elif c in ")]":
            depth -= 1
            if depth < 0:
                return None
        elif depth == 0:
            m = regex.match(code, i)
            if m and m.end() > m.start():
                # `a <= b`, `a >= b`, `a != b` and `a == b` are not assignments.
                if not (code[m.start()] == "=" and i > 0 and code[i - 1] in "<>!="):
                    parts.append(code[last:i])
                    last = i = m.end()
                    continue
        i += 1
    if depth:
        return None
    parts.append(code[last:])
    return parts


class Access:
    """What one statement reads and writes, as conservative alias keys."""

    def __init__(self):
        self.write_vars: set[str] = set()
        self.write_fields: set[str] = set()
        self.write_mem = False
        self.write_roots: set[str] = set()
        self.idents: set[str] = set()
        self.bare_idents: set[str] = set()
        self.fields: set[str] = set()
        self.mem = False
        self.write_global = False


def _scan_reads(code: str, acc: Access) -> None:
    for m in re.finditer(r"(\.|->)\s*(" + IDENT + r")|\b(" + IDENT + r")\b", code):
        if m[2]:
            acc.fields.add(m[2])
            continue
        name = m[3]
        if name in KEYWORDS or name[0].isdigit():
            continue
        acc.idents.add(name)
        after = code[m.end():].lstrip()
        if not (after.startswith(".") or after.startswith("->")):
            acc.bare_idents.add(name)
    # A dereference or index reads memory that any pointer may alias.
    if re.search(r"\[|(?:^|[^\w\]\s])\s*\*\s*[A-Za-z_(]", code):
        acc.mem = True


def statement_access(code: str, locals_: frozenset = frozenset()) -> Access | None:
    """Parse `lv = lv = expr;`, `lv op= expr;` or `lv++;`; None if not simple."""
    code = code.strip()
    if not code.endswith(";") or code.count(";") != 1:
        return None
    code = code[:-1].strip()
    if not code or has_call(code) or re.search(r"\?|,", code):
        return None
    first = re.match(IDENT, code)
    if first and first[0] in KEYWORDS:
        return None
    acc = Access()
    inc = re.fullmatch(r"(?:\+\+|--)\s*(.+)|(.+?)\s*(?:\+\+|--)", code)
    if inc:
        lvalues, rhs = [inc[1] or inc[2]], ""
    else:
        parts = split_top(code, ASSIGN_OPS)
        if not parts or len(parts) < 2:
            return None
        lvalues, rhs = parts[:-1], parts[-1]
    if "++" in rhs or "--" in rhs:
        return None
    for lv in lvalues:
        lv = lv.strip()
        if not lv or "++" in lv or "--" in lv or re.search(ASSIGN_OPS, lv):
            return None
        # A declaration has two names in a row (`int x = 0`, `T *p = q`).
        if re.match(IDENT + r"[\s*]+" + IDENT, lv):
            return None
        root = re.match(r"[(\s*]*(" + IDENT + r")", lv)
        if re.fullmatch(IDENT, lv):
            acc.write_vars.add(lv)
            if lv not in locals_:
                acc.write_global = True
        elif re.search(r"(?:\.|->)\s*" + IDENT + r"\s*$", lv):
            acc.write_fields.add(re.search(r"(" + IDENT + r")\s*$", lv)[1])
            if root:
                acc.write_roots.add(root[1])
        else:
            acc.write_mem = True
        # Index expressions and pointer bases are reads.
        _scan_reads(lv, acc)
    _scan_reads(rhs, acc)
    return acc


def conflicts(a: Access, b: Access) -> bool:
    return _writes_hit(a, b) or _writes_hit(b, a)


def _writes_hit(w: Access, r: Access) -> bool:
    if w.write_vars & (r.idents | r.write_vars):
        return True
    if w.write_fields & (r.fields | r.write_fields):
        return True
    if (w.write_fields or w.write_mem) and (r.mem or r.write_mem):
        return True
    if w.write_mem and (r.fields or r.write_fields):
        return True
    if w.write_roots & r.bare_idents:
        return True
    # A global (or a local whose address escapes) may be any pointee.
    if w.write_global and (r.mem or r.write_mem or r.fields or r.write_fields):
        return True
    return False


def function_locals(text: str, address: int) -> frozenset:
    """Parameters and body declarations whose address is never taken."""
    code = msvc_permuter.mask(text)
    start, end = function_span(text, address)
    names = set()
    head = code[:start - 1]
    close = head.rfind(")")
    opening = head.rfind("(", 0, close)
    if 0 <= opening < close:
        for param in head[opening + 1:close].split(","):
            m = re.search(r"(" + IDENT + r")\s*(?:\[[^\]]*\]\s*)*$", param.strip())
            if m and m[1] not in KEYWORDS:
                names.add(m[1])
    for line in code[start:end].split("\n"):
        decl = parse_declaration(line)
        if decl:
            names.update(d[0] for d in decl[1])
    body = code[start:end]
    return frozenset(n for n in names if not re.search(r"&\s*\(*\s*" + re.escape(n) + r"\b", body))


def store_groups(text: str, address: int, c_language: bool = True, exhaustive: int = 6) -> list[Group]:
    code = msvc_permuter.mask(text)
    first, last, starts = body_lines(text, address)
    groups: list[Group] = []
    run: list[tuple[int, Access]] = []

    def flush():
        if len(run) >= 2:
            indices = [i for i, _ in run]
            items = [line_text(text, starts, i) for i in indices]
            before = {(x, y) for x in range(len(run)) for y in range(x + 1, len(run)) if conflicts(run[x][1], run[y][1])}
            group = OrderGroup("stores", f"stores@{indices[0] + 1}-{indices[-1] + 1}",
                               starts[indices[0]], starts[indices[-1] + 1], items, before,
                               exhaustive=exhaustive, names=[f"L{i + 1}" for i in indices])
            if group.size():
                groups.append(group)
        run.clear()

    locals_ = function_locals(text, address)
    previous = "{"
    for index in range(first, last):
        masked = line_text(code, starts, index)
        stripped = masked.strip()
        acc = None
        if stripped and "{" not in stripped and "}" not in stripped and "#" not in stripped:
            # A line that continues a statement or is the body of an
            # unbraced if/else/for/while is not a free-standing statement.
            if previous.endswith((";", "{", "}")) or re.search(r"(?:^|[^:]):$", previous) and not previous.endswith("?:"):
                acc = statement_access(stripped, locals_)
        if acc is None:
            flush()
        else:
            run.append((index, acc))
        if stripped:
            previous = stripped
    flush()
    return groups


# ---------------------------------------------------------------------------
# cases: switch case blocks

LABEL_RE = re.compile(r"^\s*(?:case\b(?:[^:]|::)*?[^:]:(?!:)|default\s*:(?!:))")
TERMINATOR_RE = re.compile(r"^(?:break|continue|return\b[\s\S]*|goto\s+" + IDENT + r")$")


def _last_statement(code: str) -> str | None:
    """The block's last top-level statement, or None when it ends in a brace."""
    body = code.strip()
    if not body:
        return ""
    if body.endswith("}"):
        return None
    if not body.endswith(";"):
        return None
    inner = body[:-1]
    depth = 0
    cut = 0
    for i, c in enumerate(inner):
        if c in "({[":
            depth += 1
        elif c in ")}]":
            depth -= 1
            if c == "}" and depth == 0:
                cut = i + 1
        elif c == ";" and depth == 0:
            cut = i + 1
    tail = inner[cut:]
    # Strip leading labels (`case 3: break`).
    while True:
        m = LABEL_RE.match(tail)
        if not m:
            break
        tail = tail[m.end():]
    return tail.strip()


def block_terminates(code: str) -> bool:
    body = code
    while True:
        m = LABEL_RE.match(body)
        if not m:
            break
        body = body[m.end():]
    stmt = _last_statement(body)
    if stmt is None:
        # `case X: { ...; break; }`: one compound statement.
        stripped = body.strip()
        if stripped.startswith("{") and stripped.endswith("}"):
            depth = 0
            for i, c in enumerate(stripped):
                depth += (c == "{") - (c == "}")
                if depth == 0 and i != len(stripped) - 1:
                    return False
            stmt = _last_statement(stripped[1:-1])
        if stmt is None:
            return False
    return bool(TERMINATOR_RE.match(stmt))


def case_groups(text: str, address: int, c_language: bool = True, exhaustive: int = 6) -> list[Group]:
    code = msvc_permuter.mask(text)
    fstart, fend = function_span(text, address)
    starts = line_starts(text)
    groups: list[Group] = []
    for sw in re.finditer(r"\bswitch\s*\(", code[fstart:fend]):
        pos = fstart + sw.end()
        depth = 1
        while pos < fend and depth:
            depth += (code[pos] == "(") - (code[pos] == ")")
            pos += 1
        m = re.compile(r"\s*\{[ \t]*\n").match(code, pos)
        if not m:
            continue
        open_line = _line_of(starts, m.end() - 1)
        depth, close = 1, m.end()
        while close < fend and depth:
            depth += (code[close] == "{") - (code[close] == "}")
            close += 1
        close -= 1
        close_line = _line_of(starts, close)
        if code[starts[close_line]:close].strip():
            continue
        first, last = open_line + 1, close_line
        depths = line_depths(code, starts, first, last)
        body = code[starts[first]:starts[last]]
        nested_switch = re.search(r"\bswitch\b", body) is not None
        label_lines = []
        refuse = False
        for offset, index in enumerate(range(first, last)):
            masked = line_text(code, starts, index)
            if LABEL_RE.match(masked):
                rest = masked
                while LABEL_RE.match(rest):
                    rest = rest[LABEL_RE.match(rest).end():]
                if re.search(r"\b(?:case\b|default\s*:)", rest):
                    refuse = True  # a label after a statement on one line
                if depths[offset] == 0:
                    label_lines.append(index)
                elif not nested_switch:
                    refuse = True
            elif re.search(r"\b(?:case\b|default\s*:)", masked) and depths[offset] == 0:
                refuse = True  # a label after other text on a line
        if refuse or len(label_lines) < 2:
            continue
        if code[starts[first]:starts[label_lines[0]]].strip():
            continue
        # Blocks start at a label line whose predecessor block has statements.
        blocks: list[int] = []
        for index in label_lines:
            if blocks:
                prev_start = blocks[-1]
                prev_code = code[starts[prev_start]:starts[index]]
                rest = prev_code
                while True:
                    lm = LABEL_RE.match(rest)
                    if not lm:
                        break
                    rest = rest[lm.end():]
                if not rest.strip():
                    continue  # adjacent labels stay together
            blocks.append(index)
        bounds = blocks + [last]
        spans = [(bounds[i], bounds[i + 1]) for i in range(len(blocks))]
        # Fallthrough blocks travel with the block they fall into.
        units: list[tuple[int, int]] = []
        current = None
        for a, b in spans:
            if current is None:
                current = a
            if block_terminates(code[starts[a]:starts[b]]):
                units.append((current, b))
                current = None
        pinned = current is not None
        if pinned:
            units.append((current, last))
        if len(units) < 2:
            continue
        items = [text[starts[a]:starts[b]] for a, b in units]
        names = [re.sub(r"\s+", " ", LABEL_RE.match(code[starts[a]:starts[b]])[0].strip()) for a, _ in units]
        group = OrderGroup("cases", f"switch@{_line_of(starts, fstart + sw.start()) + 1} ({len(units)} blocks)",
                           starts[units[0][0]], starts[last], items, pinned_last=pinned,
                           exhaustive=exhaustive, names=names)
        if group.size():
            groups.append(group)
    return groups


# ---------------------------------------------------------------------------
# decls: leading local declarations and declarators

DECL_PREFIX = r"(?:(?:static|register|const|volatile|unsigned|signed|struct|union|enum|long|short)\s+)*"
DECLARATOR = r"\**\s*(" + IDENT + r")\s*(?:\[[^\]]*\]\s*)*(?:=([^;,]*))?"
DECLARATOR_NC = r"\**\s*" + IDENT + r"\s*(?:\[[^\]]*\]\s*)*(?:=[^;,]*)?"
DECL_RE = re.compile(r"^" + DECL_PREFIX + r"(" + IDENT + r")(?:\s+|\s*(?=\*))(" + DECLARATOR_NC + r"(?:\s*,\s*" + DECLARATOR_NC + r")*)\s*;$")


def parse_declaration(code: str):
    """(type, [(name, initializer)], declarator-list span) for a declaration line."""
    stripped = code.strip()
    m = DECL_RE.match(stripped)
    if not m:
        return None
    type_name = m[1]
    if type_name in ("return", "goto", "case", "else", "do", "typedef", "delete", "throw"):
        return None
    declarators = []
    for part in split_top(m[2], r",") or []:
        dm = re.fullmatch(r"\s*" + DECLARATOR + r"\s*", part)
        if not dm:
            return None
        declarators.append((dm[1], (dm[2] or "").strip(), part.strip()))
    lead = len(code) - len(code.lstrip())
    return type_name, declarators, (lead + m.start(2), lead + m.end(2)), stripped


def decl_groups(text: str, address: int, c_language: bool = True, exhaustive: int = 6) -> list[Group]:
    code = msvc_permuter.mask(text)
    first, last, starts = body_lines(text, address)
    groups: list[Group] = []
    run: list[tuple[int, tuple]] = []

    def flush():
        if len(run) >= 2:
            indices = [i for i, _ in run]
            names = [[d[0] for d in decl[1]] for _, decl in run]
            before = set()
            for x in range(len(run)):
                for y in range(x + 1, len(run)):
                    inits_y = " ".join(d[1] for d in run[y][1][1])
                    inits_x = " ".join(d[1] for d in run[x][1][1])
                    uses = any(re.search(r"\b" + re.escape(n) + r"\b", inits_y) for n in names[x])
                    calls = (has_call(inits_x) and inits_y) or (has_call(inits_y) and inits_x)
                    if uses or calls:
                        before.add((x, y))
            items = [line_text(text, starts, i) for i in indices]
            group = OrderGroup("decls", f"decls@{indices[0] + 1}-{indices[-1] + 1}", starts[indices[0]],
                               starts[indices[-1] + 1], items, before, exhaustive=exhaustive,
                               names=[",".join(n) for n in names])
            if group.size():
                groups.append(group)
        run.clear()

    previous = ""
    for index in range(first - 1, last):
        masked = line_text(code, starts, index)
        stripped = masked.strip()
        decl = None
        if index >= first and stripped:
            decl = parse_declaration(masked.rstrip("\n"))
            if decl and not c_language:
                builtin = decl[0] in BUILTIN_TYPES
                pointers = all(d[2].startswith("*") for d in decl[1])
                if not (builtin or pointers) or any(d[1] for d in decl[1]):
                    decl = None
        if decl and (run or previous.endswith("{")):
            if run and index != run[-1][0] + 1:
                flush()
            run.append((index, decl))
            # Declarators within one line, without initializers.
            if len(decl[1]) >= 2 and not any(d[1] for d in decl[1]):
                a, b = decl[2]
                base = starts[index]
                original = text[base + a:base + b]
                parts = [p.strip() for p in split_top(original, r",")]
                groups.append(OrderGroup("declarators", f"declarators@{index + 1}", base + a, base + b, parts,
                                         joiner=", ", exhaustive=exhaustive, names=[d[0] for d in decl[1]]))
        elif stripped:
            flush()
        if stripped:
            previous = stripped
    flush()
    return groups


# ---------------------------------------------------------------------------
# operands: commutative and mirrored binary operators

TOKEN_RE = re.compile(
    r"(?P<space>\s+)|(?P<comment>//[^\n]*|/\*[\s\S]*?\*/)|(?P<pp>^[ \t]*#[^\n]*)"
    r"|(?P<lit>\"(?:\\.|[^\"\\\n])*\"|'(?:\\.|[^'\\\n])*')"
    r"|(?P<num>(?:0[xX][0-9a-fA-F]+|\d+\.?\d*(?:[eE][-+]?\d+)?|\.\d+(?:[eE][-+]?\d+)?)[uUlLfF]*)"
    r"|(?P<ident>[A-Za-z_]\w*)"
    r"|(?P<punct>->|\+\+|--|<<=|>>=|<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%&|^]=|::|[-+*/%&|^~!<>=?:;,.(){}\[\]])",
    re.M,
)
PRECEDENCE = {
    "*": 13, "/": 13, "%": 13, "+": 12, "-": 12, "<<": 11, ">>": 11,
    "<": 10, ">": 10, "<=": 10, ">=": 10, "==": 9, "!=": 9, "&": 8, "^": 7,
    "|": 6, "&&": 5, "||": 4,
}
SWAPPED = {"+": "+", "*": "*", "&": "&", "|": "|", "^": "^", "==": "==", "!=": "!=",
           "<": ">", ">": "<", "<=": ">=", ">=": "<="}
ASSIGNMENT_TOKENS = {"=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>="}
STOP_KEYWORDS = {"return", "case", "else", "do", "goto", "throw", "if", "while", "for", "switch"}
MACRO_RE = re.compile(r"^[A-Z][A-Z0-9_]+$")
SAFE_MACROS = {"NULL", "TRUE", "FALSE"}


def _macro_like(tokens) -> bool:
    if len(tokens) == 3 and tokens[0][1] == "(" and tokens[-1][1] == ")":
        return False
    return any(t[0] == "ident" and MACRO_RE.match(t[1]) and t[1] not in SAFE_MACROS for t in tokens)


def tokenize(text: str, start: int, end: int):
    tokens = []
    pos = start
    while pos < end:
        m = TOKEN_RE.match(text, pos)
        if not m or m.end() == pos:
            return None
        kind = m.lastgroup
        if kind not in ("space", "comment"):
            tokens.append((kind, m.group(), m.start(), m.end()))
        pos = m.end()
    return tokens


def _ends_operand(tok) -> bool:
    kind, value = tok[0], tok[1]
    return kind in ("num", "lit") or (kind == "ident" and value not in KEYWORDS) or value in (")", "]", "++", "--")


def _is_cast_close(tokens, k) -> bool:
    """tokens[k] is `)`; does its group look like a cast `(type *)`?"""
    depth = 0
    for i in range(k, -1, -1):
        value = tokens[i][1]
        if value == ")":
            depth += 1
        elif value == "(":
            depth -= 1
            if depth == 0:
                inner = tokens[i + 1:k]
                if not inner:
                    return False
                before = tokens[i - 1] if i else None
                if before and before[0] == "ident" and before[1] not in KEYWORDS:
                    return False  # a call's argument list
                return all(t[0] == "ident" or t[1] == "*" for t in inner)
    return False


def _control_close(tokens, k) -> bool:
    """tokens[k] is `)`; does it close an if/while/for/switch condition?"""
    depth = 0
    for i in range(k, -1, -1):
        value = tokens[i][1]
        if value == ")":
            depth += 1
        elif value == "(":
            depth -= 1
            if depth == 0:
                return i > 0 and tokens[i - 1][1] in ("if", "while", "for", "switch")
    return False


def binary_kind(tokens, k) -> str:
    """'binary', 'unary' or 'unknown' for an operator token."""
    if k == 0:
        return "unary"
    prev = tokens[k - 1]
    if not _ends_operand(prev):
        return "unary"
    if prev[1] == ")" and _control_close(tokens, k - 1):
        return "unary"  # `if (p) *p = 0;`
    if prev[1] == ")" and tokens[k][1] in ("*", "&", "+", "-") and _is_cast_close(tokens, k - 1):
        return "unknown"
    return "binary"


def operand_extent(tokens, k, direction):
    """Token index range of the operand on one side of binary operator k."""
    p = PRECEDENCE[tokens[k][1]]
    i = k + direction
    depth = 0
    edge = None
    while 0 <= i < len(tokens):
        kind, value = tokens[i][0], tokens[i][1]
        opens, closes = ("(", "["), (")", "]")
        if direction < 0:
            opens, closes = closes, opens
        if value in opens:
            depth += 1
        elif value in closes:
            if depth == 0:
                break
            depth -= 1
        elif depth == 0:
            if kind == "pp" or value in ("{", "}", ";", ",", "?", ":") or value in ASSIGNMENT_TOKENS:
                break
            if kind == "ident" and value in STOP_KEYWORDS:
                break
            if value in PRECEDENCE and value not in ("++", "--"):
                bk = binary_kind(tokens, i)
                if bk == "unknown":
                    return None
                if bk == "binary":
                    q = PRECEDENCE[value]
                    # Left-associative: the left operand keeps equal precedence.
                    if q < p or (direction > 0 and q == p):
                        break
        edge = i
        i += direction
    if edge is None or depth:
        return None
    return (edge, k - 1) if direction < 0 else (k + 1, edge)


def _side_effects(tokens) -> tuple[bool, bool]:
    """(has a call, has another side effect) in a token run."""
    call = any(t[0] == "ident" and t[1] not in KEYWORDS and i + 1 < len(tokens) and tokens[i + 1][1] == "("
               for i, t in enumerate(tokens))
    other = any(t[1] in ("++", "--") or t[1] in ASSIGNMENT_TOKENS for t in tokens)
    return call, other


def operand_sites(text: str, address: int, c_language: bool = True):
    start, end = function_span(text, address)
    tokens = tokenize(text, start, end)
    if tokens is None:
        return []
    starts = line_starts(text)
    sites = []
    for k, tok in enumerate(tokens):
        op = tok[1]
        if tok[0] != "punct" or op not in SWAPPED:
            continue
        if not c_language and op in ("<", ">", "<=", ">="):
            continue  # template brackets
        if binary_kind(tokens, k) != "binary":
            continue
        left = operand_extent(tokens, k, -1)
        right = operand_extent(tokens, k, +1)
        if not left or not right:
            continue
        ltoks, rtoks = tokens[left[0]:left[1] + 1], tokens[right[0]:right[1] + 1]
        if not (_ends_operand(ltoks[-1]) and _ends_operand(rtoks[-1])):
            continue  # a type (`(T **)`), not an expression
        if op in ("*", "&") and all(t[0] == "ident" for t in ltoks) and \
                (left[0] == 0 or tokens[left[0] - 1][1] in ("{", "}", ";")):
            continue  # a declaration (`T *p`, `const T &r`), not a product
        lcall, lother = _side_effects(ltoks)
        rcall, rother = _side_effects(rtoks)
        if lother or rother:
            continue
        if lcall or rcall:
            # One call is safe only against an operand of literals.
            other = rtoks if lcall else ltoks
            if (lcall and rcall) or not all(t[0] in ("num", "lit") for t in other):
                continue
        a, b = ltoks[0][2], rtoks[-1][3]
        ltext = text[ltoks[0][2]:ltoks[-1][3]]
        rtext = text[rtoks[0][2]:rtoks[-1][3]]
        middle = text[ltoks[-1][3]:rtoks[0][2]]
        middle = middle.replace(op, SWAPPED[op], 1)
        p = PRECEDENCE[op]
        # The old left operand becomes a right operand: a top-level operator of
        # the same precedence needs parentheses; so does a possible macro.
        depth = 0
        needs = False
        for i in range(left[0], left[1] + 1):
            value = tokens[i][1]
            depth += value in ("(", "[")
            depth -= value in (")", "]")
            if depth == 0 and value in PRECEDENCE and PRECEDENCE[value] == p and binary_kind(tokens, i) == "binary":
                needs = True
        new_left, new_right = rtext, "(" + ltext + ")" if needs else ltext
        # A macro operand may expand to a lower-precedence expression.
        if _macro_like(rtoks):
            new_left = "(" + rtext + ")"
        if _macro_like(ltoks) and not needs:
            new_right = "(" + ltext + ")"
        line = _line_of(starts, tok[2]) + 1
        shown = " ".join(ltext.split()) + " " + op + " " + " ".join(rtext.split())
        label = f"{op}@{line} {shown}"
        sites.append((a, b, new_left + middle + new_right, label))
    return sites


def operand_groups(text: str, address: int, c_language: bool = True, exhaustive: int = 6, pairs: bool = False) -> list[Group]:
    sites = operand_sites(text, address, c_language)
    if not pairs:
        return [EditGroup(label, [(a, b, r)]) for a, b, r, label in sites]
    groups = []
    for x, y in itertools.combinations(sites, 2):
        if x[1] <= y[0] or y[1] <= x[0]:
            groups.append(EditGroup(x[3] + " + " + y[3], [x[:3], y[:3]]))
    return groups


GROUPERS = {
    "stores": store_groups,
    "cases": case_groups,
    "decls": decl_groups,
    "operands": operand_groups,
}


def groups_for(mode: str, text: str, address: int, c_language: bool, exhaustive: int,
               phase: str = "single", lines: tuple[int, int] | None = None) -> list[Group]:
    if mode == "operands":
        groups = operand_groups(text, address, c_language, exhaustive, pairs=phase == "pairs")
    else:
        groups = GROUPERS[mode](text, address, c_language, exhaustive)
    if lines:
        groups = [g for g in groups if _group_lines(g, text) and _overlaps(_group_lines(g, text), lines)]
    return groups


def _group_lines(group: Group, text: str):
    starts = line_starts(text)
    if isinstance(group, OrderGroup):
        return _line_of(starts, group.start) + 1, _line_of(starts, max(group.start, group.end - 1)) + 1
    a = min(e[0] for e in group.edits)
    b = max(e[1] for e in group.edits)
    return _line_of(starts, a) + 1, _line_of(starts, b) + 1


def _overlaps(x, y) -> bool:
    return x[0] <= y[1] and y[0] <= x[1]


# ---------------------------------------------------------------------------
# Search engine


class Budget(Exception):
    pass


class Search:
    """The round/group/candidate loop, with its state on disk."""

    def __init__(self, address: int, mode: str, source: Path, out: Path,
                 score: Callable[[str], tuple[float, bool]], original: str, *,
                 seconds: float, exhaustive: int = 6, max_rounds: int = 8,
                 lines=None, pairs: bool = True, log=print, rel: str | None = None):
        self.address = address
        self.mode = mode
        self.source = source
        self.rel = rel or str(source)
        self.out = out
        self.score_fn = score
        self.original = original
        self.deadline = time.monotonic() + seconds
        self.exhaustive = exhaustive
        self.max_rounds = max_rounds
        self.lines = lines
        self.pairs = pairs and mode == "operands"
        self.log = log
        self.c_language = source.suffix == ".c"
        self.state: dict = {}

    # -- state files

    def _paths(self):
        suffix = self.source.suffix
        return (self.out / "state.json", self.out / ("base" + suffix), self.out / ("best" + suffix))

    def init_state(self, initial: float, effective: bool):
        self.state = {
            "version": STATE_VERSION, "address": hex(self.address), "mode": self.mode,
            "source": self.rel, "original_sha": sha(self.original),
            "exhaustive": self.exhaustive, "lines": self.lines, "phase": "single",
            "round": 1, "group": 0, "index": 0, "round_improved": False,
            "initial": initial, "initial_effective": effective, "best": initial,
            "best_effective": effective, "attempts": 0, "failures": 0, "seconds": 0.0,
            "finished": False, "seen": [], "improvements": [], "top": [], "group_log": [],
        }
        self.base = self.original
        self.best = self.original

    def load_state(self):
        state_path, base_path, best_path = self._paths()
        self.state = json.loads(state_path.read_text())
        if self.state.get("version") != STATE_VERSION or self.state["mode"] != self.mode or int(self.state["address"], 16) != self.address:
            raise ValueError(f"{state_path} is for another search ({self.state.get('mode')} {self.state.get('address')})")
        if self.state["original_sha"] != sha(self.original):
            raise ValueError(f"{self.rel} changed since the search started; start a fresh search")
        self.exhaustive = self.state["exhaustive"]
        self.lines = tuple(self.state["lines"]) if self.state.get("lines") else None
        self.base = base_path.read_text()
        self.best = best_path.read_text()

    def save(self):
        state_path, base_path, best_path = self._paths()
        self.out.mkdir(parents=True, exist_ok=True)
        for path, content in ((base_path, self.base), (best_path, self.best)):
            tmp = path.with_suffix(path.suffix + ".tmp")
            tmp.write_text(content)
            os.replace(tmp, path)
        tmp = state_path.with_suffix(".tmp")
        tmp.write_text(json.dumps(self.state, indent=1) + "\n")
        os.replace(tmp, state_path)
        self.write_report()

    def write_report(self):
        patch = "".join(difflib.unified_diff(self.original.splitlines(True), self.best.splitlines(True),
                                             fromfile="a/" + self.rel, tofile="b/" + self.rel))
        (self.out / "best.patch").write_text(patch)
        s = self.state
        report = {
            "address": s["address"], "mode": s["mode"], "source": s["source"],
            "before": s["initial"], "best": s["best"], "best_effective": s["best_effective"],
            "attempts": s["attempts"], "failures": s["failures"], "seconds": round(s["seconds"], 1),
            "finished": s["finished"], "round": s["round"], "phase": s["phase"],
            "next": None if s["finished"] else {"group": s["group"], "index": s["index"]},
            "improvements": s["improvements"], "top": s["top"], "groups": s["group_log"],
            "failure_samples": s.get("failure_samples", []),
        }
        (self.out / "result.json").write_text(json.dumps(report, indent=2) + "\n")

    # -- the loop

    def _note_top(self, score, label, desc):
        top = self.state["top"]
        top.append({"score": score, "group": label, "order": desc})
        top.sort(key=lambda e: -e["score"])
        del top[10:]

    def run(self) -> int:
        started = time.monotonic()
        seen = set(self.state["seen"])
        try:
            while not self.state["finished"]:
                if self.state["best"] >= 1.0:
                    self.state["finished"] = True
                    break
                groups = groups_for(self.mode, self.base, self.address, self.c_language,
                                    self.exhaustive, self.state["phase"], self.lines)
                g = self.state["group"]
                if g >= len(groups):
                    self._end_round()
                    continue
                group = groups[g]
                for index, (desc, candidate) in enumerate(group.candidates(self.base)):
                    if index < self.state["index"]:
                        continue
                    if time.monotonic() >= self.deadline:
                        self.state["index"] = index
                        raise Budget()
                    digest = sha(candidate)
                    self.state["index"] = index + 1
                    if digest in seen:
                        continue
                    seen.add(digest)
                    self.state["seen"].append(digest)
                    self.state["attempts"] += 1
                    try:
                        value, effective = self.score_fn(candidate)
                    except Exception as ex:  # pylint: disable=broad-except
                        self.state["failures"] += 1
                        samples = self.state.setdefault("failure_samples", [])
                        if len(samples) < 5:
                            samples.append({"group": group.label, "order": desc, "error": str(ex)[-400:]})
                        self.log(f"  {group.label} [{desc}]: {str(ex).splitlines()[0] if str(ex) else ex}")
                        continue
                    self._note_top(value, group.label, desc)
                    if value > self.state["best"]:
                        self.state["best"], self.state["best_effective"] = value, effective
                        self.best = candidate
                        self.state["round_improved"] = True
                        self.state["improvements"].append({"round": self.state["round"], "phase": self.state["phase"],
                                                           "group": group.label, "order": desc, "score": value})
                        self.log(f"{group.label} [{desc}]: {value * 100:.2f}%")
                        self.save()
                        if value >= 1.0:
                            break
                    elif self.state["attempts"] % 10 == 0:
                        self.save()
                self.state["group_log"].append({"round": self.state["round"], "phase": self.state["phase"],
                                                "group": group.label, "candidates": group.size()})
                self.state["group"] += 1
                self.state["index"] = 0
                self.base = self.best
        except Budget:
            self.state["seconds"] += time.monotonic() - started
            self.save()
            return 3
        self.state["seconds"] += time.monotonic() - started
        self.save()
        return 0

    def _end_round(self):
        s = self.state
        more = s["round_improved"] and s["round"] < self.max_rounds
        if s["phase"] == "single":
            if more:
                s["round"] += 1
            elif self.pairs:
                s["phase"] = "pairs"
            else:
                s["finished"] = True
        elif more:
            s["phase"] = "single"
            s["round"] += 1
        else:
            s["finished"] = True
        s["group"] = 0
        s["index"] = 0
        s["round_improved"] = False
        self.base = self.best


# ---------------------------------------------------------------------------
# The in-place scorer


class InPlaceScorer:
    """Score one function's translation unit, compiling candidates in place.

    The project context (original image, annotations) is built once. Each
    candidate is written over the real source, compiled, and the original is
    written back before the object is analysed."""

    def __init__(self, address: int, out: Path, build_dir: str | None = None, orig: Path | None = None):
        import logging
        import tempfile
        import match_function as mf
        from reccmp.dir import source_code_search
        from reccmp.formats import TextFile, detect_image

        logging.getLogger("reccmp").setLevel(logging.CRITICAL)
        logging.getLogger().setLevel(logging.CRITICAL)
        self.mf = mf
        args = argparse.Namespace(build_dir=build_dir, orig=orig)
        self.env = mf.Environment.detect(args)
        commands = mf.load_compile_commands(self.env, {})
        orig_path = mf.find_orig(args)
        if not orig_path.exists():
            raise ValueError(f"original executable not found: {orig_path} (symlink reference/ into the worktree)")
        orig_bin = detect_image(filepath=orig_path)
        code_files = list(TextFile.from_files(source_code_search([self.env.root / "src"]), allow_error=True, encoding="utf-8"))
        markers = mf.scan_markers(code_files)
        marker = markers.get(address)
        if marker is None:
            raise ValueError(f"no annotation names {address:#x}")
        self.address = address
        self.source = marker.path.resolve()
        self.rel = os.path.relpath(self.source, self.env.root)
        self.command = {c.source: c for c in commands}.get(self.source)
        if self.command is None:
            raise ValueError(f"{self.rel} is not compiled by the matching build")
        libc_symbols, libc_data, libc_index = mf.load_libc_index(self.env)
        self.ctx = mf.ProjectContext.build(orig_bin, code_files, markers, libc_symbols)
        self.ctx.libc_data, self.ctx.libc_index = libc_data, libc_index
        self.ctx.link_end = mf.link_end(orig_bin, self.env.root)
        runs = self.env.build_dir / "runs"
        runs.mkdir(parents=True, exist_ok=True)
        self.workdir = Path(tempfile.mkdtemp(prefix="search-", dir=runs))
        # A search killed while a candidate was in place left this lock and
        # the original beside it; put the original back first.
        out.mkdir(parents=True, exist_ok=True)
        self.lock = out / "in-place.lock"
        self.backup = out / ("original" + self.source.suffix)
        if self.lock.exists():
            self.source.write_text(self.backup.read_text())
            self.lock.unlink()
            print(f"restored {self.rel} from {self.backup} (an earlier search was killed)", file=sys.stderr)
        self.original = self.source.read_text()
        self.backup.write_text(self.original)
        self.count = 0

    def restore(self):
        if self.source.read_text() != self.original:
            self.source.write_text(self.original)
        self.lock.unlink(missing_ok=True)

    def __call__(self, text: str) -> tuple[float, bool]:
        if text.count("\n") != self.original.count("\n"):
            raise ValueError("candidate changes the line count")
        self.count += 1
        index = self.count % 50
        try:
            if text != self.original:
                self.lock.write_text(str(self.source) + "\n")
                self.source.write_text(text)
            obj_path, _ = self.mf.compile_source(self.env, self.command, self.workdir, index)
        finally:
            self.restore()
        try:
            from synthetic_image import SyntheticImage
            obj = self.mf.msvc_coff.read_coff(obj_path)
            img = SyntheticImage(obj, self.ctx, self.source)
            analysis = self.mf.analyse(img, self.ctx)
            res = self.mf.compare_function(analysis, self.ctx, self.address)
        finally:
            obj_path.unlink(missing_ok=True)
        if res.error or res.matching is None:
            raise ValueError(res.error or "no score")
        return res.matching, bool(res.effective)

    def close(self):
        import shutil
        self.restore()
        shutil.rmtree(self.workdir, ignore_errors=True)


def parse_lines(text: str):
    m = re.fullmatch(r"(\d+)(?:-(\d+))?", text)
    if not m:
        raise argparse.ArgumentTypeError("LINES is N or N-M")
    return int(m[1]), int(m[2] or m[1])


def build_parser():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("address", type=lambda x: int(x, 16))
    parser.add_argument("--mode", choices=MODES, required=True)
    parser.add_argument("--seconds", type=float, required=True, help="time budget for this run (resumable)")
    parser.add_argument("--output", type=Path, required=True, help="directory for the state, best source, patch and report")
    parser.add_argument("--resume", action="store_true", help="continue the search recorded in OUTPUT/state.json")
    parser.add_argument("--exhaustive", type=int, default=6, help="score every order of groups up to this many items (default 6); larger groups get single insertion moves")
    parser.add_argument("--lines", type=parse_lines, help="only groups that overlap source lines N or N-M")
    parser.add_argument("--max-rounds", type=int, default=8)
    parser.add_argument("--no-pairs", action="store_true", help="operands: skip the pairs phase")
    parser.add_argument("--list", action="store_true", help="print the groups and their candidate counts, then exit (no compile)")
    parser.add_argument("--source", type=Path, help="with --list: the source file (default: found by annotation)")
    parser.add_argument("--build-dir", help="match_function.py's flags cache (default build-match/)")
    return parser


def find_source(address: int) -> Path:
    pattern = re.compile(r"//\s*(?:FUNCTION|STUB|LIBRARY)\s*:\s*LEGORR\s+0x0*" + format(address, "x") + r"\b", re.I)
    for path in sorted((ROOT / "src").rglob("*")):
        if path.suffix in (".c", ".cpp") and pattern.search(path.read_text(errors="replace")):
            return path
    raise ValueError(f"no annotation names {address:#x}")


def main(argv=None) -> int:
    args = build_parser().parse_args(argv)
    if args.seconds <= 0:
        print("permuter_search: --seconds must be positive", file=sys.stderr)
        return 2
    if args.list:
        try:
            source = args.source or find_source(args.address)
            text = source.read_text()
            phases = ["single", "pairs"] if args.mode == "operands" and not args.no_pairs else ["single"]
            for phase in phases:
                groups = groups_for(args.mode, text, args.address, source.suffix == ".c", args.exhaustive, phase, args.lines)
                print(f"{phase}: {len(groups)} groups, {sum(g.size() for g in groups)} candidates")
                for g in groups[:200]:
                    print(f"  {g.label}: {g.size()}")
        except ValueError as ex:
            print(f"permuter_search: {ex}", file=sys.stderr)
            return 2
        return 0

    def interrupt(signum, _frame):
        raise KeyboardInterrupt(f"signal {signum}")

    for sig in (signal.SIGTERM, signal.SIGHUP):
        signal.signal(sig, interrupt)
    scorer = None
    search = None
    try:
        scorer = InPlaceScorer(args.address, args.output, args.build_dir)
        search = Search(args.address, args.mode, scorer.source, args.output, scorer, scorer.original,
                        seconds=args.seconds, exhaustive=args.exhaustive, max_rounds=args.max_rounds,
                        lines=args.lines, pairs=not args.no_pairs, rel=scorer.rel)
        state_path = args.output / "state.json"
        if args.resume:
            search.load_state()
            print(f"resuming: round {search.state['round']} {search.state['phase']}, group {search.state['group']}, "
                  f"index {search.state['index']}, best {search.state['best'] * 100:.2f}%", flush=True)
        else:
            if state_path.exists():
                raise ValueError(f"{state_path} exists; pass --resume or choose another --output")
            initial, effective = scorer(scorer.original)
            search.init_state(initial, effective)
            print(f"{scorer.rel} {args.address:#x}: {initial * 100:.2f}% before", flush=True)
            search.save()
        status = search.run()
    except KeyboardInterrupt:
        if search is not None and search.state:
            search.save()
        print("permuter_search: interrupted; state saved", file=sys.stderr)
        return 3
    except (ValueError, OSError) as ex:
        print(f"permuter_search: {ex}", file=sys.stderr)
        return 2
    finally:
        if scorer is not None:
            scorer.close()
    s = search.state
    word = "finished" if s["finished"] else "budget spent, resumable"
    print(f"{word}: {s['initial'] * 100:.2f}% -> {s['best'] * 100:.2f}% after {s['attempts']} attempts "
          f"({s['failures']} failed); report in {args.output / 'result.json'}", flush=True)
    return status


if __name__ == "__main__":
    sys.exit(main())
