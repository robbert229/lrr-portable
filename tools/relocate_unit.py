#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Relocate one original translation unit verbatim into its own file (ADR 0006).

*Relocation* (GLOSSARY.md) moves a unit's ``// FUNCTION``-annotated functions
and the ``// GLOBAL`` state it owns, verbatim and in original address order,
into ``src/<game or gods98>/<stem>.cpp``. The result stays C++; it is not a
conversion (ADR 0005). Stage S2a of the #388 plan.

    python3 tools/relocate_unit.py GAME-001 --dry-run   # plan and report, write nothing
    python3 tools/relocate_unit.py GAME-001             # relocate Advisor.c into src/game/advisor.cpp
    python3 tools/relocate_unit.py GAME-004 --stem nullnames
    python3 tools/relocate_unit.py --survey             # what blocks each remaining unit

What moves, besides the marked functions and owned globals:

* the file-static closure only the moved code uses: static helpers and
  variables, file-local types and macros, forward declarations;
* nothing else. Unmarked code with external linkage stays where it is.

What is shared: a static that moved code and remaining code both use gets
external linkage through a private ``<file>_shared.hpp`` next to the source
file, which both include. File-local types and macros both sides need move
into that header. Shared headers (``*.hpp`` that other files include) are never
edited, because declaration-count drift (finding 506) would spread to every
includer.

It also updates the link-order list (a ``unit`` entry at the unit's original
position), the Meson source list, the ``current_source`` column of
``translation-units.csv``, and writes ``docs/coverage/relocations/<unit>.csv``:
a SHA-256 of every moved item before and after, which must agree after
whitespace normalisation (only a documented ``static`` removal may change a
head, never a body). A second run finds the unit relocated and does nothing.

A top-level ``#if`` group whose items form one cluster moves whole and
verbatim (#501); a static it shares gets a declaration wrapped in the group's
condition. A static that a private-header item needs is exported (T2).

Constructs it refuses rather than guesses at: definitions inside ``#if`` blocks
(except a group of alternative definitions of one function, or a whole
group as above), directives that
split a declaration, functions defined in headers, multi-level qualified or
macro-built definitions, linkage blocks inside namespaces, shared annotated
``inline`` functions, shared statics that
need a declaration it cannot write, and name clashes between moved files.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import io
import json
import re
import shutil
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import conversion_footprint as cf  # noqa: E402
import cxx_items as cx  # noqa: E402
import link_order  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
RECORDS = "docs/coverage/relocations"
UNIT_FUNCTIONS = cf.UNIT_FUNCTIONS
FUNCTION_KINDS = ("FUNCTION", "STUB")
INCLUDE_PATHS = {
    "game": ("src", "src/game", "src/gods98"),
    "program": ("src", "src/game", "src/gods98"),
    "gods98": ("src", "src/gods98", "src/game"),
    "platform_glue": ("src",),
}
NAKED = re.compile(r"__declspec\s*\(\s*naked\s*\)\s*")


class Refused(Exception):
    """The unit cannot be relocated safely; ``reasons`` says why."""

    def __init__(self, unit: str, reasons: list[str]):
        super().__init__(f"{unit}: " + "; ".join(reasons))
        self.unit = unit
        self.reasons = reasons


def sha(text: str) -> str:
    return hashlib.sha256(cx.normalise(text).encode("utf-8")).hexdigest()


# --------------------------------------------------------------------------
# Include resolution and header visibility


class Headers:
    """Resolves includes and caches the names each header declares."""

    def __init__(self, root: Path):
        self.root = root
        self.names: dict[str, set[str]] = {}
        self.includes: dict[str, list[str]] = {}

    def resolve(self, spelled: str, including: str, library: str) -> str | None:
        """Repository-relative path of an include, or None for system headers."""
        name = spelled[1:-1]
        dirs = ([str(Path(including).parent)] if spelled.startswith('"') else []) + \
            list(INCLUDE_PATHS.get(library, ("src",)))
        for d in dirs:
            p = (self.root / d / name).resolve()
            try:
                rel = p.relative_to(self.root.resolve()).as_posix()
            except ValueError:
                continue
            if p.is_file():
                return rel
        return None

    def _includes(self, rel: str) -> list[str]:
        if rel not in self.includes:
            text = (self.root / rel).read_text(encoding="utf-8", errors="replace")
            self.includes[rel] = re.findall(r'^[ \t]*#[ \t]*include[ \t]*([<"][^>"]+[>"])', text, re.M)
        return self.includes[rel]

    def declared(self, rel: str) -> set[str]:
        """Functions and variables a header declares, as ``n`` or ``X::n``."""
        if rel not in self.names:
            scan = cx.scan((self.root / rel).read_text(encoding="utf-8", errors="replace"))
            names = set()
            for it in (p for x in scan.items for p in (x.parts or [x])):
                if it.category not in ("function", "proto", "var"):
                    continue
                scope = _container_namespace(it.container) or it.qualifier or ""
                names.update(_qualify(scope, n) for n in it.names)
            self.names[rel] = names
        return self.names[rel]

    def visible(self, spelled: list[str], including: str, library: str) -> set[str]:
        """Function and variable names declared by the headers reachable from
        ``spelled`` includes of ``including`` (conditional includes too)."""
        seen: set[str] = set()
        todo = [(s, including) for s in spelled]
        while todo:
            s, frm = todo.pop()
            rel = self.resolve(s, frm, library)
            if rel is None or rel in seen:
                continue
            seen.add(rel)
            todo.extend((x, rel) for x in self._includes(rel))
        names: set[str] = set()
        for rel in seen:
            names |= self.declared(rel)
        return names


def _qualify(scope: str, name: str) -> str:
    return f"{scope}::{name}" if scope else name


def _container_namespace(container: str | None) -> str | None:
    """X for a member of ``namespace X {}``, ``A::B`` for nested named
    namespaces (#514), each optionally with an anonymous namespace innermost
    (``namespace X { namespace { } }``); else None."""
    if container and container.startswith("ns:"):
        parts = container.split("/")
        if parts[-1] == "anon":
            parts = parts[:-1]
        if parts and all(re.fullmatch(r"ns:[A-Za-z_]\w*", p) for p in parts):
            return "::".join(p[3:] for p in parts)
    return None


def _ns_names(wrap: str) -> list[str]:
    """The named namespaces of a wrap, outermost first: ``ns:A/ns:B`` (a
    container) or ``ns:A::B`` (a declaration's scope)."""
    ns = _container_namespace(wrap) if "/" in wrap else wrap[3:]
    return ns.split("::") if ns else []


def _ns_block(names: list[str], body: str, anon: bool = False, inline: bool = False) -> str:
    """``body`` inside nested ``namespace A { namespace B { } }`` blocks (MSVC
    5.0 has no ``namespace A::B``), with an anonymous namespace innermost."""
    opens = [f"namespace {n} {{" for n in names] + (["namespace {"] if anon else [])
    closes = (["} // namespace"] if anon else []) + [f"}} // namespace {n}" for n in reversed(names)]
    if inline:
        return " ".join(opens) + " " + body + " " + " ".join("}" for _ in closes)
    return "\n".join(opens) + "\n" + body + "\n" + "\n".join(closes) + "\n"


def _outer_wrap(container: str | None) -> str | None:
    """The block an item lands in when it leaves its anonymous namespace."""
    if container == "anon":
        return None
    if container and container.endswith("/anon"):
        return container[:-len("/anon")]
    return container


def _anonymous(container: str | None) -> bool:
    """Internal linkage through an anonymous namespace (alone or inside X)."""
    return container == "anon" or bool(container and container.endswith("/anon"))


def _scope(item: cx.Item, namespaces: set[str]) -> str:
    """The namespace an item's names belong to ("" for the global scope).

    Members of ``namespace X { }`` and out-of-line ``T X::f()`` definitions
    belong to X when X is a namespace. ``extern "C"`` blocks and anonymous
    namespaces put names in the global scope of the file.
    """
    ns = _container_namespace(item.container)
    if ns:
        return ns
    if item.qualifier and item.qualifier in namespaces:
        return item.qualifier
    return ""


def known_namespaces(root: Path) -> set[str]:
    """Names of every ``namespace X {`` in src/ (not classes or structs)."""
    found = set()
    for path in (Path(root) / "src").rglob("*"):
        if path.suffix.lower() in cf.SOURCE_SUFFIXES and path.is_file():
            found.update(re.findall(r"\bnamespace\s+([A-Za-z_]\w*)\s*\{",
                                    cx.mask(path.read_text(encoding="utf-8", errors="replace"))))
    return found


@dataclass(frozen=True)
class Decl:
    """A declaration for a private header."""

    name: str        # qualified: n or X::n
    text: str        # the declaration, unqualified, ending in ;
    why: str
    ns: str = ""     # namespace to declare it in ("" for the global scope)
    direction: str = "for"   # for: the legacy file's item, which the unit uses;
                             # from: the unit's (moved) item, which the legacy file uses
    const_object: bool = False   # a const object: needs a prior extern for external linkage
    guard: str = ""          # a conditional item's #if directive, verbatim (#501)
    endif: str = ""          # the line that closes the guard
    origin: str = ""         # file:line of the definition (for the records)

    def rendered(self) -> str:
        """The declaration, wrapped in its group's condition if it has one."""
        if not self.guard:
            return self.text + "\n"
        return f"{self.guard}\n{self.text}\n{self.endif}\n"


# --------------------------------------------------------------------------
# Plan


@dataclass
class Move:
    """One item moving into the unit file (or, ``to_header``, the private header)."""

    source: str
    item: cx.Item
    role: str                 # function, global, closure-<category>, shared-<category>
    address: int | None
    name: str
    static_removed: bool = False
    wrap: str | None = None   # extern-c, anon, ns:X or ns:X/anon
    anon_left: bool = False   # leaves its anonymous namespace (shared, so external)

    def before(self, text: str) -> str:
        return self.item.code(text)


@dataclass
class Split:
    """A declaration the relocation needs split (``--split-declarations``)."""

    source: str
    item: cx.Item
    text: str                 # the replacement, one declaration per line
    kind: str                 # declarators or type-and-variables
    start: int = -1           # where the replacement starts, when before the item (#525)


@dataclass
class Source:
    rel: str
    entry: link_order.Entry
    text: str
    scan: cx.Scan
    moves: list[Move] = field(default_factory=list)
    header_items: list[cx.Item] = field(default_factory=list)   # types/macros/decls to the private header
    strip_static: list[cx.Item] = field(default_factory=list)   # staying items that get external linkage
    anon_exits: list[cx.Item] = field(default_factory=list)     # staying items that leave their anonymous namespace
    declarations: list[Decl] = field(default_factory=list)
    guarded_groups: list = field(default_factory=list)   # (#if group, direction) split across the files

    unit_stem: str = ""

    @property
    def shared_header(self) -> str:
        """The pre-#496 per-source header (now frozen; types and macros only)."""
        p = Path(self.rel)
        return (p.parent / f"{p.stem}_shared.hpp").as_posix()

    @property
    def for_header(self) -> str:
        """What the unit needs from this legacy file (#496): the unit and the
        legacy file include it. Written once, by this relocation."""
        p = Path(self.rel)
        return (p.parent / f"{p.stem}_for_{self.unit_stem}.hpp").as_posix()

    @property
    def from_header(self) -> str:
        """What this legacy file needs from the unit (#496): only the legacy
        file includes it, so the unit never sees declarations of its own
        definitions (finding 538 rule A)."""
        p = Path(self.rel)
        return (p.parent / f"{p.stem}_from_{self.unit_stem}.hpp").as_posix()

    @property
    def for_decls(self) -> list[Decl]:
        return [d for d in self.declarations if d.direction == "for"]

    @property
    def from_decls(self) -> list[Decl]:
        return [d for d in self.declarations if d.direction == "from"]

    @property
    def shares_for(self) -> bool:
        return bool(self.header_items or self.for_decls)

    @property
    def shares_from(self) -> bool:
        return bool(self.from_decls)

    @property
    def shares(self) -> bool:
        return self.shares_for or self.shares_from


@dataclass
class Plan:
    unit: cf.UnitRanges
    stem: str
    library: str
    target: str
    sources: list[Source]
    missing: list[tuple[int, str]]            # unit functions with no annotation in src/
    notes: list[str]
    files: dict[str, str] = field(default_factory=dict)
    records: list[dict] = field(default_factory=list)
    order: list[Move] = field(default_factory=list)
    forward: list[str] = field(default_factory=list)
    done: bool = False                        # already relocated: nothing to do
    namespaces: set[str] = field(default_factory=set)
    usings: list[str] = field(default_factory=list)
    units: list[cf.UnitRanges] = field(default_factory=list)
    split_mode: bool = False                  # --split-declarations: rewrite declarations only
    splits: list["Split"] = field(default_factory=list)

    @property
    def moves(self) -> list[Move]:
        return [m for s in self.sources for m in s.moves]


def _annotations(root: Path, u: cf.UnitRanges,
                 units: list[cf.UnitRanges]) -> tuple[list[cf.Annotation], list[cf.Annotation]]:
    """The unit's function and owned-global annotations anywhere in src/."""
    marker = re.compile(r"LEGORR\s+0x([0-9a-fA-F]+)")
    funcs, globs = [], []
    for path in sorted((root / "src").rglob("*")):
        if path.suffix.lower() not in cf.SOURCE_SUFFIXES or not path.is_file() or path.name.startswith("test_"):
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        if not any(_wanted(int(a, 16), u, units) for a in marker.findall(text)):
            continue
        rel = path.relative_to(root).as_posix()
        for number, line in enumerate(text.split("\n")):
            m = cf.ANNOTATION.match(line)
            if not m:
                continue
            kind, addr = m.group(1), int(m.group(2), 16)
            if kind in FUNCTION_KINDS + ("LIBRARY",) and u.text_start <= addr < u.text_end:
                funcs.append(cf.Annotation(kind, addr, rel, number + 1, None))
            elif kind == "GLOBAL" and owns_data(addr, u, units):
                globs.append(cf.Annotation(kind, addr, rel, number + 1, None))
    return funcs, globs


def owns_data(addr: int, u: cf.UnitRanges, units: list[cf.UnitRanges]) -> bool:
    """True when ``addr`` is in one of ``u``'s data ranges.

    A range ends at the start of its last item, and
    ``conversion_footprint._in_ranges`` adds a few bytes of slack past it for
    that item's size. An address in the slack belongs to ``u`` only if no
    other unit's range holds it: the slack never reaches into the next unit
    (finding 527: GAME-014's .bss slack claimed 004c7eb0, the first item of
    GAME-015's).
    """
    if any(lo <= addr <= hi for lo, hi in u.ranges.values()):
        return True
    if not cf._in_ranges(addr, u):
        return False
    return not any(lo <= addr <= hi for v in units if v.unit_id != u.unit_id for lo, hi in v.ranges.values())


def _wanted(addr: int, u: cf.UnitRanges, units: list[cf.UnitRanges]) -> bool:
    return u.text_start <= addr < u.text_end or owns_data(addr, u, units)


def plan_relocation(key: str, root: Path = ROOT, stem: str | None = None,
                    headers: Headers | None = None, split: bool = False) -> Plan:
    root = Path(root)
    units = cf.load_units(root)
    u = cf.find_unit(units, key)
    reasons: list[str] = []
    if u.unit_kind not in cf.CONVERTIBLE_KINDS:
        raise Refused(u.unit_id, [f"{u.unit_kind} units are not relocated (only game and gods98)"])
    if not stem and not u.unit_name:
        raise Refused(u.unit_id, ["unnamed unit: pass --stem with a descriptive lower-case stem (ADR 0006 decision 4)"])
    given = bool(stem)
    stem = stem or u.stem
    if not given:
        # A default stem that names an existing file gets the variant
        # <stem>_unit (ADR 0006 decision 4); --stem overrides it.
        directory = root / cf.SOURCE_DIRS.get(u.unit_kind, "src/game")
        taken = [p for p in directory.glob(f"{stem}.*") if p.name != f"{stem}_shared.hpp"]
        listed_unit = any(e.kind == "unit" and e.key == u.unit_id and e.path == f"{cf.SOURCE_DIRS.get(u.unit_kind)}/{stem}.cpp"
                          for e in link_order.load(root))
        if taken and not listed_unit:
            stem += "_unit"
    if not re.fullmatch(r"[a-z][a-z0-9_]*", stem):
        raise Refused(u.unit_id, [f"stem {stem!r} must be lower-case letters, digits and underscores"])
    library = u.unit_kind
    target = f"{cf.SOURCE_DIRS[library]}/{stem}.cpp"
    entries = link_order.load(root)
    by_source = {e.path: e for e in entries}
    listed = [e for e in entries if e.kind == "unit" and e.key == u.unit_id]
    headers = headers or Headers(root)

    func_ann, glob_ann = _annotations(root, u, units)
    # Globals in another unit's file (pooled literals a conversion annotated
    # in place) stay there: a relocation never edits another unit.
    kept = [a for a in glob_ann if a.file in by_source and by_source[a.file].kind == "unit"
            and a.file != target and not any(f.file == a.file for f in func_ann)]
    glob_ann = [a for a in glob_ann if a not in kept]
    elsewhere = [a for a in func_ann + glob_ann if a.file != target]
    if listed:
        e = listed[0]
        if e.path != target:
            raise Refused(u.unit_id, [f"already listed as {e.source} ({'converted' if e.source.endswith('.c') else 'relocated'})"])
        if not elsewhere:
            return Plan(u, stem, library, target, [], [], [f"{u.unit_id} is already relocated to {target}"], done=True)
        reasons.append(f"{target} is listed, but {len(elsewhere)} annotation(s) of the unit are still elsewhere "
                       f"(first: {elsewhere[0].file}:{elsewhere[0].line}); finish or revert the earlier run")
    elif target in by_source:
        reasons.append(f"{target} is the {by_source[target].kind} file of the list; pass --stem with a variant "
                       f"(for example {stem}_unit)")
    elif (root / target).exists():
        reasons.append(f"{target} exists but is not in the link-order list")
    clash = [p.relative_to(root).as_posix() for p in (root / cf.SOURCE_DIRS[library]).glob(f"{stem}.*")
             if p.relative_to(root).as_posix() != target and p.name != f"{stem}_shared.hpp"]
    if clash:
        reasons.append(f"stem {stem!r} clashes with {', '.join(sorted(clash))}; pass --stem with a variant")

    rows = [r for r in cf.load_function_rows(root) if r.unit_id == u.unit_id]
    found = {a.address for a in func_ann}
    missing = [(r.address, r.ghidra_name) for r in rows if r.address not in found]
    for a in func_ann:
        if a.kind == "LIBRARY":
            reasons.append(f"{a.file}:{a.line}: LIBRARY annotation at 0x{a.address:08x} inside a game/gods98 unit")
    files = sorted({a.file for a in func_ann + glob_ann})
    for rel in files:
        e = by_source.get(rel)
        if rel.endswith((".h", ".hpp")):
            reasons.append(f"{rel}: the unit has annotated definitions in a header; moving them would edit a shared header")
        elif e is None:
            reasons.append(f"{rel} holds annotations of the unit but is not in the link-order list")
        elif e.kind == "unit":
            reasons.append(f"{rel} is the unit file of {e.key}")
        elif e.platform:
            reasons.append(f"{rel} builds for the {e.platform} backend only")
    if reasons:
        raise Refused(u.unit_id, reasons)
    if not func_ann:
        raise Refused(u.unit_id, ["no // FUNCTION annotation of the unit in src/"])

    sources = []
    for rel in files:
        text = (root / rel).read_text(encoding="utf-8")
        scan = cx.scan(text)
        sources.append(Source(rel, by_source[rel], text, scan))
    plan = Plan(u, stem, library, target, sources, missing,
                [f"GLOBAL 0x{a.address:08x} stays in {a.file}, the unit file of {by_source[a.file].key}"
                 for a in kept])
    if not given and stem != u.stem:
        plan.notes.append(f"The default stem {u.stem!r} names an existing file in {cf.SOURCE_DIRS[library]}; "
                          f"using {stem!r} (pass --stem to choose another)")
    plan.namespaces = known_namespaces(root)
    plan.units = units
    plan.split_mode = split
    for src in sources:
        src.plan = plan
        src.unit_stem = stem
    for s in sources:
        _plan_source(plan, s, [a for a in func_ann if a.file == s.rel],
                     [a for a in glob_ann if a.file == s.rel], headers, reasons)
    if reasons:
        raise Refused(u.unit_id, reasons)
    if split:
        _render_splits(plan, reasons)
        if reasons:
            raise Refused(u.unit_id, reasons)
        return plan
    _check_clashes(plan, root, reasons)
    if reasons:
        raise Refused(u.unit_id, reasons)
    _render(plan, root, headers, reasons)
    if reasons:
        raise Refused(u.unit_id, reasons)
    return plan


# --------------------------------------------------------------------------
# Closure of one source file


class _Clusters:
    """Items that define a common (qualified) name form one symbol.

    Names are scoped: ``n`` in the global scope, ``X::n`` in namespace X. A use
    resolves as C++ looks it up: ``X::n`` exactly; a bare ``n`` inside X to
    ``X::n`` when this file defines it, else to the global ``n``; a bare
    ``n`` in the global scope through the file's ``using`` items.
    """

    SKIP = ("include", "undef", "other", "using")

    def __init__(self, items: list[cx.Item], namespaces: set[str]):
        parent = list(range(len(items)))

        def find(x):
            while parent[x] != x:
                parent[x] = parent[parent[x]]
                x = parent[x]
            return x
        self.usings = [it.using for it in items if it.category == "using" and it.using
                       and not it.cond and not it.container]
        owner: dict[str, int] = {}
        for it in items:
            it.scope = _scope(it, namespaces)
        # Overloads: a name with several function definitions in the file is
        # keyed by its parameter types, n(int,float*), and a use of the name
        # resolves to the whole overload set.
        definitions: dict[str, int] = {}
        for it in items:
            if it.category == "function" and it.names:
                q = _qualify(it.scope, it.names[0])
                definitions[q] = definitions.get(q, 0) + 1
        self.overloads: dict[str, set[str]] = {}
        for it in items:
            if it.category in self.SKIP:
                it.qnames = []
                continue
            if it.qualifier and not it.scope:
                # An out-of-line class member (#514): X::f, never the free f.
                it.qnames = [f"{it.qualifier}::{n}" for n in it.names]
            else:
                it.qnames = [_qualify(it.scope, n) for n in it.names]
            if it.category in ("function", "proto") and not it.cgroup and it.qnames \
                    and definitions.get(it.qnames[0], 0) > 1:
                key = f"{it.qnames[0]}({_param_key(it.head(_text_of(it)))})"
                self.overloads.setdefault(it.qnames[0], set()).add(key)
                it.qnames = [key]
        for k, it in enumerate(items):
            for name in it.qnames:
                if name in owner:
                    parent[find(k)] = find(owner[name])
                else:
                    owner[name] = k
        self.owner = owner
        self.known = set(owner) | set(self.overloads)
        for it in items:
            it.ruses = set().union(*(self.resolve_all(u, it.scope) for u in it.uses)) if it.uses else set()
        self.of = [find(k) for k in range(len(items))]
        self.members: dict[int, list[int]] = {}
        for k, c in enumerate(self.of):
            self.members.setdefault(c, []).append(k)
        self.names: dict[int, set[str]] = {c: set() for c in self.members}
        self.uses: dict[int, set[str]] = {c: set() for c in self.members}
        for k, it in enumerate(items):
            c = self.of[k]
            self.names[c].update(it.qnames)
            self.uses[c].update(it.ruses)
        self.definer: dict[str, int] = {}
        for c, names in self.names.items():
            for n in names:
                self.definer[n] = c
        for c in self.uses:
            self.uses[c] -= self.names[c]
        self.users: dict[int, set[int]] = {c: set() for c in self.members}
        for c, uses in self.uses.items():
            for n in uses:
                d = self.definer.get(n)
                if d is not None and d != c:
                    self.users[d].add(c)

    def resolve(self, use: str, scope: str) -> str:
        if "::" in use:
            return use
        enclosing = scope.split("::") if scope else []
        while enclosing:       # A::B, then A (C++ lookup through enclosing namespaces)
            if f"{'::'.join(enclosing)}::{use}" in self.known:
                return f"{'::'.join(enclosing)}::{use}"
            enclosing.pop()
        if use in self.known:
            return use
        for kind, target in self.usings:
            if kind == "namespace" and f"{target}::{use}" in self.known:
                return f"{target}::{use}"
            if kind == "name" and target.rsplit("::", 1)[-1] == use and target in self.known:
                return target
        return use

    def resolve_all(self, use: str, scope: str) -> set[str]:
        """``resolve``, with an overloaded name expanded to its overload set."""
        r = self.resolve(use, scope)
        return set(self.overloads.get(r, ())) or {r}


def _text_of(item: cx.Item) -> str:
    """The source text an item was scanned from (set by _plan_source)."""
    return getattr(item, "source_text", "")


def _base(name: str) -> str:
    """A qualified name without an overload's parameter key."""
    return name.split("(", 1)[0]


def _bases(names) -> set[str]:
    return {_base(n) for n in names}


def _param_key(head: str) -> str:
    """Parameter types of a function head, for telling overloads apart.

    Parameter names are dropped (the last identifier of a parameter with two
    or more), defaults and array bounds too: ``(const float first[3], int n
    = 2)`` reads ``const float*,int``.
    """
    masked = cx.mask(head)
    depth, start, end = 0, -1, -1
    for k, ch in enumerate(masked):
        if ch == "(":
            if depth == 0:
                start = k
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                end = k
    inner = masked[start + 1:end] if start >= 0 and end > start else ""
    out = []
    for part in cx._split_top(inner):
        part = cx._split_top(part, "=")[0]
        arrays = part.count("[")
        part = re.sub(r"\[[^\]]*\]", " ", part)
        tokens = re.findall(r"[A-Za-z_]\w*|[*&]", part)
        idents = [t for t in tokens if t not in ("*", "&")]
        if len(idents) >= 2 and tokens and tokens[-1] == idents[-1]:
            tokens = tokens[:-1]
        out.append(" ".join(tokens) + "*" * arrays)
    key = ",".join(out)
    return "" if key == "void" else key


def _kind(items: list[cx.Item], members: list[int]) -> str:
    """local (type/macro), internal (static), declaration (no definition here), external."""
    its = [items[k] for k in members]
    cats = {i.category for i in its}
    if cats <= {"type", "macro"}:
        return "local"
    funcs = [i for i in its if i.category == "function" and not i.cgroup]
    if funcs and all(f.inline for f in funcs) and cats <= {"function", "proto"} \
            and not any(i.annotations for i in its):
        # An unannotated inline function (#514) is header-safe: shared, it
        # goes verbatim to the private header, as a type or macro does.
        return "local"
    if any(i.static or _anonymous(i.container) for i in its):
        return "internal"
    if not any(i.category == "function" or (i.category == "var" and not _is_extern_decl(i)) for i in its):
        return "declaration"
    return "external"


def _is_extern_decl(item: cx.Item) -> bool:
    if item.cgroup:
        vars_ = [p for p in item.parts if p.category == "var"]
        return bool(vars_) and all(_is_extern_decl(p) for p in vars_)
    code = getattr(item, "text_code", "")
    return item.category == "var" and bool(re.match(r"\s*extern\b", code)) and "=" not in code


def _plan_source(plan: Plan, s: Source, funcs: list[cf.Annotation], globs: list[cf.Annotation],
                 headers: Headers, reasons: list[str]) -> None:
    items = s.scan.items
    for it in items:
        for x in [it] + it.parts:
            x.text_code = s.text[x.start:x.end]
            x.source_text = s.text
    where = {}
    for k, it in enumerate(items):
        for _, addr in it.annotations:
            where.setdefault(addr, k)
    seeds: dict[int, tuple[str, int]] = {}
    for a in funcs + globs:
        k = where.get(a.address)
        if k is None:
            after = s.text.split("\n")[a.line:a.line + 12]
            nxt = next((x.strip() for x in after if x.strip() and not x.strip().startswith(("//", "/*", "*"))), "")
            if nxt.startswith("#"):
                reasons.append(f"{s.rel}:{a.line}: the definition annotated 0x{a.address:08x} is inside an #if block "
                               "that is not a group of whole alternative definitions of one function")
            else:
                reasons.append(f"{s.rel}:{a.line}: the {a.kind} annotation 0x{a.address:08x} is not attached to an item "
                               "(a blank line or code between the annotation and the definition?)")
            continue
        it = items[k]
        role = "function" if a.kind in FUNCTION_KINDS else "global"
        if role == "function" and it.category != "function":
            reasons.append(f"{s.rel}:{it.line}: 0x{a.address:08x} annotates a {it.category or it.kind}, not a function definition")
        if role == "global" and it.category != "var":
            reasons.append(f"{s.rel}:{it.line}: GLOBAL 0x{a.address:08x} annotates a {it.category}, not a variable")
        if role == "global" and (it.multi or it.defines_type):
            _need_split(plan, s, it, reasons,
                        f"GLOBAL 0x{a.address:08x} annotates a declaration of several names ({', '.join(it.names)})")
        if role == "global" and _is_extern_decl(it):
            reasons.append(f"{s.rel}:{it.line}: GLOBAL 0x{a.address:08x} annotates an extern declaration, not a definition")
        others = [x for _, x in it.annotations if not _wanted(x, plan.unit, plan.units)]
        if others:
            reasons.append(f"{s.rel}:{it.line}: the item also carries annotations of other units "
                           + ", ".join(f"0x{x:08x}" for x in others))
        seeds[k] = (role, a.address) if k not in seeds or role == "function" else seeds[k]
    if reasons:
        return
    cl = _Clusters(items, plan.namespaces)
    core = {cl.of[k] for k in seeds}
    for c in core:
        defs = [k for k in cl.members[c] if items[k].category in ("function", "var") and not _is_extern_decl(items[k])]
        extra = [k for k in defs if k not in seeds]
        if extra:
            reasons.append(f"{s.rel}:{items[extra[0]].line}: {', '.join(sorted(cl.names[c]))} is also defined by an "
                           "unannotated item (overload or several declarators)")
    if reasons:
        return

    # The greatest set of file-local candidates that only moving code uses.
    # Reachability expands only through items that move: what a staying
    # function uses stays with it.
    kinds = {c: _kind(items, cl.members[c]) for c in cl.members}
    # An item annotated for another unit is that unit's: it never moves with
    # this one. It stays, and gets external linkage if moved code uses it.
    pinned = {c for c in cl.members if c not in core and any(
        kind in FUNCTION_KINDS + ("LIBRARY", "GLOBAL") for k in cl.members[c] for kind, _ in items[k].annotations)}
    local = {c for c in cl.members if kinds[c] in ("local", "internal", "declaration")} - set(core) - pinned

    def touched(roots):
        seen = set()
        for c in roots:
            for n in cl.uses[c]:
                d = cl.definer.get(n)
                if d is not None and d not in core:
                    seen.add(d)
        return seen

    moving = set(core)
    frontier = set(core)
    while frontier:
        new = (touched(frontier) & local) - moving
        moving |= new
        frontier = new
    while True:
        drop = {c for c in moving - set(core) if cl.users[c] - moving}
        moving -= drop
        keep = set(core)
        frontier = set(core)
        while frontier:
            new = (touched(frontier) & moving) - keep
            keep |= new
            frontier = new
        if not drop and keep == moving:
            break
        moving = keep
    reach = touched(moving) - moving
    candidates = reach & (local | pinned)
    shared = candidates
    to_header = {c for c in shared if kinds[c] in ("local", "declaration") and c not in pinned}
    exported = {c for c in shared if kinds[c] == "internal"}

    stay_includes = [it.include for it in items if it.category == "include" and it.include]
    visible_here = headers.visible(stay_includes, s.rel, s.entry.library)
    def first(c):
        return cl.members[c][0]
    for c in sorted(core, key=first):
        if not (cl.users[c] - moving):
            continue
        if any(items[k].inline for k in cl.members[c]) and kinds[c] != "internal":
            # An annotated inline function must be defined in every file
            # that uses it: a declaration is not enough (#514).
            reasons.append(f"{s.rel}:{items[cl.members[c][0]].line}: inline {', '.join(sorted(cl.names[c]))} "
                           "is used by moved and remaining code")
            continue
        if kinds[c] == "internal":
            exported.add(c)
        elif kinds[c] == "external" and not (_bases(cl.names[c]) & visible_here):
            s.declarations.extend(_declarations(s, items, cl, c, "moved; remaining code uses it", reasons, "from"))
    # An external item pinned to another unit stays too, and needs the same
    # declaration when no header declares it (#501: G98-025's ledger stub).
    pinned_external = {c for c in candidates & pinned if kinds[c] == "external"}
    for c in sorted((reach - candidates) | pinned_external, key=first):
        if kinds[c] == "external" and not (_bases(cl.names[c]) & visible_here):
            s.declarations.extend(_declarations(s, items, cl, c, "stays; moved code uses it", reasons, "for"))
    anon_exit = set()
    for c in sorted(exported, key=first):
        if any(_anonymous(items[k].container) for k in cl.members[c]):
            # Mirrors the static rule: the item leaves its anonymous namespace
            # (gaining external linkage) and is declared in the private header.
            anon_exit.add(c)
        if any(items[k].inline for k in cl.members[c]):
            reasons.append(f"{s.rel}:{items[cl.members[c][0]].line}: inline {', '.join(sorted(cl.names[c]))} is used by "
                           "moved and remaining code")
            continue
        why = ("anonymous-namespace item shared by moved and remaining code" if c in anon_exit
               else "static shared by moved and remaining code")
        s.declarations.extend(_declarations(s, items, cl, c, why, reasons, "from" if c in moving else "for"))

    # Types, macros and declarations the private header needs.
    need = set()
    for d in s.declarations:
        for u in cx._uses(cx.mask(d.text)):
            need |= cl.resolve_all(u, d.ns)
    for c in to_header:
        need |= cl.uses[c]
    while True:
        add = set()
        need_extra: set[str] = set()
        for n in list(need):
            d = cl.definer.get(n)
            if d is None or d in to_header:
                continue
            if kinds[d] in ("local", "declaration"):
                add.add(d)
            elif d in moving or d in exported or kinds[d] in ("internal", "external"):
                if kinds[d] == "internal" and d not in exported:
                    # T2 (#501): export it, as the static rule does for a
                    # static that moved and remaining code share.
                    members = [items[k] for k in cl.members[d]]
                    if any(_anonymous(x.container) for x in members) or any(x.inline for x in members):
                        reasons.append(f"{s.rel}: the private header needs {n}, a static that is not shared "
                                       "(inline or in an anonymous namespace: not exported)")
                        continue
                    exported.add(d)
                    new = _declarations(s, items, cl, d, "static used by a private-header item", reasons,
                                        "from" if d in moving else "for")
                    s.declarations.extend(new)
                    for x in new:
                        for u in cx._uses(cx.mask(x.text)):
                            add_need = cl.resolve_all(u, x.ns) - need
                            need_extra.update(add_need)
        need |= need_extra
        if not add and not need_extra:
            break
        to_header |= add
        moving -= add
        for d in add:
            need |= cl.uses[d]

    # A group split across the files gives the other file a declaration of
    # its data only: that file must not use the group's macros or types.
    for group, direction in s.guarded_groups:
        local = {n for p in group.parts if p.category in ("macro", "type") for n in p.names}
        k_group = items.index(group)
        group_moves = cl.of[k_group] in moving
        other = [it for k, it in enumerate(items) if k != k_group and (cl.of[k] in moving) != group_moves
                 and cl.of[k] not in to_header]
        used = sorted({u for it in other for u in it.uses} & local)
        if used:
            reasons.append(f"{s.rel}:{group.line}: the #if group at line {group.line} is split across the files "
                           f"(its data is declared in the private header) but "
                           f"{'the unit' if not group_moves else 'the remaining code'} also uses its "
                           f"{', '.join(used)}; hoist the data into its own group first")

    # Problems of every item that changes place or linkage.
    involved = set(moving) | to_header | exported
    for c in involved:
        for k in cl.members[c]:
            it = items[k]
            where_ = f"{s.rel}:{it.line} ({', '.join(it.names) or it.category})"
            if it.cond:
                if it.in_group:
                    reasons.append(f"{where_}: inside an #if block: the group at line {it.in_group} "
                                   f"cannot move whole ({it.group_why})")
                else:
                    reasons.append(f"{where_}: inside an #if block")
            for p in it.problems:
                reasons.append(f"{where_}: {p}")
            if c in to_header and any(kind == "GLOBAL" or kind in FUNCTION_KINDS for kind, _ in it.annotations):
                reasons.append(f"{where_}: an annotated item would move into the private header")
            if it.category == "macro" or it.cgroup:
                for name in [n for p in (it.parts or [it]) if p.category == "macro" for n in p.names[:1]]:
                    verdict = _redefined(s, items, name)
                    if verdict and verdict.startswith("note:"):
                        if verdict[len("note: "):] not in plan.notes:
                            plan.notes.append(verdict[len("note: "):])
                    elif verdict:
                        reasons.append(f"{where_}: macro {name} is #undef'd or redefined in the file ({verdict})")
    if reasons:
        return

    for c in sorted(moving, key=lambda c: cl.members[c][0]):
        for k in cl.members[c]:
            it = items[k]
            if k in seeds:
                role, addr = seeds[k]
            else:
                role, addr = f"closure-{it.category}", None
            leaves = c in anon_exit and _anonymous(it.container)
            wrap = it.container if it.container in ("extern-c", "anon") or _container_namespace(it.container) else None
            s.moves.append(Move(s.rel, it, role, addr, ",".join(it.names) or it.category,
                                static_removed=c in exported and it.static,
                                wrap=_outer_wrap(wrap) if leaves else wrap, anon_left=leaves))
    for c in sorted(to_header, key=lambda c: cl.members[c][0]):
        s.header_items.extend(items[k] for k in cl.members[c])
    for c in exported - moving:
        s.strip_static.extend(items[k] for k in cl.members[c] if items[k].static)
        if c in anon_exit:
            s.anon_exits.extend(items[k] for k in cl.members[c] if _anonymous(items[k].container))


def plan_of(s: Source) -> Plan:
    return s.plan


SPLIT_HINT = ("split it first with `relocate_unit.py <unit> --split-declarations`: a source change, "
              "recorded and committed on its own before the relocation")


def _need_split(plan: Plan, s: Source, it: cx.Item, reasons: list[str], why: str) -> None:
    """Record a declaration that must be split, or refuse outside split mode."""
    where = f"{s.rel}:{it.line}"
    if not plan.split_mode:
        reasons.append(f"{where}: {why}; {SPLIT_HINT}")
        return
    if any(x.item is it for x in plan.splits):
        return
    try:
        text, kind = _split_declaration(s.text, it)
    except ValueError as e:
        reasons.append(f"{where}: {why}, and it cannot be split: {e}")
        return
    start = -1
    if kind == "type-and-variables":
        text, start = _annotations_onto_variable(s.text, it, text)
    plan.splits.append(Split(s.rel, it, text, kind, start))


def _annotations_onto_variable(text: str, it: cx.Item, split: str) -> tuple[str, int]:
    """Move a type-and-variables split's annotations onto its first variable.

    The annotations in the leading comments named the variable, not the
    type (#525: ``resource.cpp``'s ``s_handle_set_band``). They leave the
    leading comments and go directly above the first variable declaration,
    so the replacement then starts at the leading comments."""
    if it.lead < 0 or it.lead >= it.start:
        return split, -1
    lead = text[it.lead:it.start]
    lines = lead.splitlines(keepends=True)
    notes = [ln for ln in lines if cx.ANNOTATION.match(ln)]
    if not notes:
        return split, -1
    kept = "".join(ln for ln in lines if not cx.ANNOTATION.match(ln))
    masked, depth, type_end = cx.mask(split), 0, -1
    for k in range(masked.index("{"), len(masked)):
        depth += {"{": 1, "}": -1}.get(masked[k], 0)
        if depth == 0:
            type_end = masked.index("\n", k) + 1     # past "};" and its line end
            break
    return kept + split[:type_end] + "".join(notes) + split[type_end:], it.lead


def _split_declaration(text: str, it: cx.Item) -> tuple[str, str]:
    """One declaration per declarator, in the original order.

    ``static int a = 0, b = 0;`` becomes ``static int a = 0;`` and ``static int
    b = 0;``. ``static struct T { ... } v[3], w;`` becomes ``struct T { ... };``
    then ``static struct T v[3];`` and ``static struct T w;``. Leading
    comments stay above the first line and a trailing comment after the last.
    """
    code = it.code(text)
    masked = cx.mask(code)
    depth, semi = 0, -1
    for k, ch in enumerate(masked):
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        elif ch == ";" and depth == 0:
            semi = k
    if semi < 0:
        raise ValueError("no terminating semicolon")
    decl, trailer = code[:semi], code[semi + 1:]
    mdecl = masked[:semi]
    if "(" in cx.DECL_STRIP.sub(" ", mdecl.split("=")[0]) and not it.defines_type:
        raise ValueError("a function or function-pointer declarator")
    lines: list[str] = []
    if it.defines_type:
        open_ = mdecl.find("{")
        kw = None
        for m in re.finditer(r"\b(struct|union|enum|class)\b", mdecl[:open_]):
            kw = m
        if kw is None:
            raise ValueError("no struct, union, enum or class keyword before the body")
        depth, close = 0, -1
        for k in range(open_, len(mdecl)):
            if mdecl[k] == "{":
                depth += 1
            elif mdecl[k] == "}":
                depth -= 1
                if depth == 0:
                    close = k
                    break
        tag = re.match(r"\s*([A-Za-z_]\w*)?\s*$", mdecl[kw.end():open_])
        if not tag or not tag.group(1):
            raise ValueError("the type has no tag, so a separate declaration could not name it")
        specifiers = decl[:kw.start()]
        type_text = decl[kw.start():close + 1]
        elaborated = f"{kw.group(1)} {tag.group(1)}"
        lines.append(type_text + ";")
        rest_masked = mdecl[close + 1:]
        offsets = _top_commas(rest_masked)
        rest = decl[close + 1:]
        for a, b in zip([0] + [o + 1 for o in offsets], offsets + [len(rest)]):
            declarator = rest[a:b].strip()
            if declarator:
                lines.append(f"{specifiers}{elaborated} {declarator};")
        kind = "type-and-variables"
    else:
        offsets = _top_commas(mdecl)
        bounds = list(zip([0] + [o + 1 for o in offsets], offsets + [len(decl)]))
        first_masked = mdecl[bounds[0][0]:bounds[0][1]]
        name_end = len(cx._split_top(first_masked, "=")[0])
        head = re.sub(r"\[[^\]]*\]", lambda m: " " * len(m.group(0)), first_masked[:name_end]).rstrip()
        name = re.search(r"([A-Za-z_]\w*)$", head)
        if not name:
            raise ValueError("cannot find the first declarator")
        start = name.start()
        while start > 0 and first_masked[start - 1] in " \t*&":
            start -= 1
        specifiers = decl[:start].rstrip()
        if not specifiers or "{" in specifiers:
            raise ValueError("cannot find the shared specifiers")
        for n, (a, b) in enumerate(bounds):
            declarator = decl[start if n == 0 else a:b].strip()
            lines.append(f"{specifiers} {declarator};")
        kind = "declarators"
    new = "\n".join(lines)
    return new + trailer if trailer.strip() else new + "\n", kind


def _top_commas(masked: str) -> list[int]:
    out, depth = [], 0
    for k, ch in enumerate(masked):
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        elif ch == "," and depth == 0:
            out.append(k)
    return out


def _render_splits(plan: Plan, reasons: list[str]) -> None:
    """``--split-declarations``: rewrite the sources' declarations only."""
    if not plan.splits:
        plan.notes.append("No declaration needs splitting for this relocation.")
        return
    records = []
    for s in plan.sources:
        mine = sorted((x for x in plan.splits if x.source == s.rel), key=lambda x: x.item.start)
        if not mine:
            continue
        out, last = [], 0
        for x in mine:
            out.append(s.text[last:x.start if x.start >= 0 else x.item.start])
            out.append(x.text)
            last = x.item.end
        out.append(s.text[last:])
        new_text = "".join(out)
        # Verify: the same names and annotations, one declarator per item.
        after = cx.scan(new_text)
        if sorted(cx.ANNOTATION.findall(new_text)) != sorted(cx.ANNOTATION.findall(s.text)):
            reasons.append(f"{s.rel}: annotations changed while splitting")
        for x in mine:
            start = len("".join(out[:2 * mine.index(x) + 1]))
            new_items = [it for it in after.items if start <= it.start < start + len(x.text)]
            names = [n for it in new_items for n in it.names]
            if sorted(names) != sorted(x.item.names) or any(it.multi for it in new_items):
                reasons.append(f"{s.rel}:{x.item.line}: splitting {', '.join(x.item.names)} did not give one "
                               "declaration per name")
            records.append({"role": f"split-{x.kind}", "address": "", "name": ",".join(x.item.names),
                            "from": f"{s.rel}:{x.item.line}", "to": f"{s.rel}:{x.item.line}",
                            "before_sha256": sha(x.item.code(s.text)), "after_sha256": sha(x.text),
                            "edit": f"split into {len(new_items)} declarations"})
        plan.files[s.rel] = new_text
    plan.records = records
    plan.files[f"{RECORDS}/{plan.unit.unit_id}-splits.csv"] = _records_csv(records)


def data_layout(obj) -> list[tuple]:
    """Data placement of an MSVC object (``msvc_coff.CoffObject``).

    Every named data symbol as (section, offset, name): external COFF
    symbols in data sections, commons (section ``common``, size as offset)
    and CodeView data records (statics are only named there), then each data
    section's size. Two compiles of a unit with equal layouts place their data
    identically (finding 525: common order follows the COFF symbol table, so
    that order is kept as well).
    """
    out = []
    sections = {sec.index: sec for sec in obj.sections}

    def data_section(index):
        sec = sections.get(index)
        return sec if sec is not None and not sec.is_code and not sec.is_debug else None
    for sym in sorted(obj.symbols.values(), key=lambda x: x.index):
        if sym.is_function or sym.name.startswith("."):
            continue
        if sym.section == 0 and sym.value > 0 and sym.is_external:
            out.append(("common", sym.value, sym.name))
        elif sym.section > 0 and data_section(sym.section) is not None:
            out.append((sections[sym.section].name, sym.value, sym.name))
    for cv in obj.data:
        sym = obj.symbols.get(cv.symbol)
        if sym is not None and sym.section > 0 and data_section(sym.section) is not None:
            out.append((sections[sym.section].name, sym.value + cv.offset, cv.name))
    sizes = [("size", sec.size, sec.name) for sec in obj.sections if data_section(sec.index) is not None]
    return out + sizes


def check_split_layout(plan: Plan, root: Path) -> list[tuple[str, bool, str]]:
    """Compile each split source before and after with the matching flags
    (MSVC 5.0, as ``match_function.py`` does) and compare ``data_layout``.

    Returns (source, unchanged, detail) per source.
    """
    import tempfile
    import match_function as mf
    import msvc_coff
    env = mf.Environment.detect(argparse.Namespace(build_dir=None))
    commands = {c.source: c for c in mf.load_compile_commands(env, {})}
    results = []
    env.build_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="split-layout-", dir=env.build_dir) as tmp:
        work = Path(tmp)
        for n, rel in enumerate(sorted({x.source for x in plan.splits})):
            command = commands.get((root / rel).resolve())
            if command is None:
                results.append((rel, False, "not compiled by the matching build"))
                continue
            candidate = work / f"candidate-{n}-{Path(rel).name}"
            candidate.write_text(plan.files[rel], encoding="utf-8")
            before, _ = mf.compile_source(env, command, work, 2 * n)
            after, _ = mf.compile_source(env, command, work, 2 * n + 1, candidate=candidate)
            a, b = data_layout(msvc_coff.read_coff(before)), data_layout(msvc_coff.read_coff(after))
            if a == b:
                results.append((rel, True, f"{len(a)} data symbols and section sizes identical"))
            else:
                diff = [f"- {x}" for x in a if x not in b][:6] + [f"+ {x}" for x in b if x not in a][:6]
                if not diff:
                    diff = ["same symbols, different order"]
                results.append((rel, False, "; ".join(map(str, diff))))
    return results


def render_split_report(plan: Plan, layout: list[tuple[str, bool, str]] | None) -> str:
    out = [f"## Declaration splits for {plan.unit.unit_id} {plan.unit.unit_name or '(unnamed)'}", ""]
    if not plan.splits:
        out.append("No declaration needs splitting; run the relocation directly.")
        return "\n".join(out) + "\n"
    out.append("A source change, separate from the relocation (ADR 0006 rule 3): commit it on its "
               f"own, then run `relocate_unit.py {plan.unit.unit_id}` without `--split-declarations`. "
               f"Record: `{RECORDS}/{plan.unit.unit_id}-splits.csv`.")
    out.append("")
    for x in plan.splits:
        out.append(f"* `{x.source}:{x.item.line}` ({x.kind}): `{cx.normalise(x.item.code(plan_of_source(plan, x.source).text))}`")
        out.append("  becomes:")
        out.extend("      " + line for line in x.text.rstrip("\n").split("\n"))
    out.append("")
    if layout is None:
        out.append("Data layout not checked; `--check-layout` compiles each split source before and after "
                   "with MSVC 5.0 and compares its data symbols (finding 525).")
    else:
        for rel, same, detail in layout:
            out.append(f"* layout of `{rel}`: {'unchanged' if same else 'CHANGED'} ({detail})")
    return "\n".join(out) + "\n"


def plan_of_source(plan: Plan, rel: str) -> Source:
    return next(s for s in plan.sources if s.rel == rel)


def _redefined(s: Source, items: list[cx.Item], name: str) -> str:
    """Why macro ``name`` cannot move: "" when it is defined once, or once per
    branch of one #if group (alternatives, #501). An ``#undef`` after the
    macro's last use stays where it is and is reported ("note: ..."); any
    other ``#undef`` or a second definition refuses."""
    definitions, undefs = 0, []
    for it in items:
        if it.cgroup:
            per_branch: dict[int, int] = {}
            for p in it.parts:
                if p.category == "macro" and p.names == [name]:
                    per_branch[p.branch] = per_branch.get(p.branch, 0) + 1
            if per_branch:
                if max(per_branch.values()) > 1:
                    return "defined twice in one branch of its #if group"
                definitions += 1
            undefs += [p for p in it.parts if p.category == "undef" and name in p.uses]
        elif it.category == "macro" and it.names == [name]:
            definitions += 1
        elif it.category == "undef" and name in it.uses:
            undefs.append(it)
    if definitions > 1:
        return "defined more than once"
    if not undefs:
        return ""
    masked = s.scan.masked
    spans = [(u.start, u.end) for u in undefs]
    uses = [m.start() for m in re.finditer(r"\b%s\b" % re.escape(name), masked)
            if not any(a <= m.start() < b for a, b in spans)
            and not re.match(r"[ \t]*#[ \t]*define[ \t]+%s\b" % re.escape(name),
                             masked[masked.rfind("\n", 0, m.start()) + 1:])]
    last = max(uses, default=-1)
    early = [u for u in undefs if u.start < last]
    if early:
        return f"#undef at line {cx._line_of(s.text, early[0].start)} comes before a later use"
    lines = ", ".join(str(cx._line_of(s.text, u.start)) for u in undefs)
    return (f"note: {s.rel}:{lines}: #undef {name} after its last use stays in place "
            "(#501; harmless once the macro moves)")


def _head_decl(s: Source, it: cx.Item) -> str:
    """An item's head as a declaration: comments, ``__declspec(naked)``, the
    namespace qualifier of an out-of-line definition removed."""
    head = cx.strip_comments(it.head(s.text))
    head = NAKED.sub("", head)
    if it.qualifier and it.scope and it.names:
        head = re.sub(r"\b%s\s*::\s*(?=%s\b)" % (re.escape(it.qualifier), re.escape(it.names[0])), "", head)
    return head


def _const_object(head: str, name: str) -> bool:
    """True when ``head`` declares ``name`` as a const object (not a pointer
    to const): it then has internal linkage unless declared extern first."""
    masked = cx.mask(head)
    m = re.search(r"\b%s\b" % re.escape(name), masked)
    before = masked[:m.start()] if m else masked
    consts = [x.start() for x in re.finditer(r"\bconst\b", before)]
    stars = [x.start() for x in re.finditer(r"\*", before)]
    return bool(consts) and (not stars or consts[-1] > stars[-1])


def _declarations(s: Source, items: list[cx.Item], cl: _Clusters, c: int, why: str,
                  reasons: list[str], direction: str = "for") -> list[Decl]:
    """External declarations for the private header, from a cluster's definition."""
    groups = [items[k] for k in cl.members[c] if items[k].cgroup]
    if groups:
        # Every group that defines something is declared, not only the
        # first: a macro-only group can share a name with a data group, as
        # frontend.cpp's winapi `#define s_fe_flow` and its non-winapi
        # `int s_fe_flow[5]` do (#525).
        defining = [g for g in groups if any(p.category in ("function", "var", "proto") for p in g.parts)]
        return [d for g in (defining or groups[:1])
                for d in _group_declarations(s, g, why, reasons, direction)]
    defs = [items[k] for k in cl.members[c] if items[k].category in ("function", "var")
            and not _is_extern_decl(items[k])]
    protos = [items[k] for k in cl.members[c] if items[k].category == "proto"]
    it = (defs or protos or [None])[0]
    if it is None:
        return []
    return _declaration(s, it, it.qnames[0], it.scope, why, reasons, direction)


def _group_declarations(s: Source, group: cx.Item, why: str, reasons: list[str],
                        direction: str) -> list[Decl]:
    """Guarded declarations for a conditional group's definitions (#501).

    Each is wrapped in the group's own #if directive, copied verbatim. Only
    definitions in a single-branch group or in the first branch qualify; an
    #elif or #else branch's condition would have to be synthesised.
    """
    where = f"{s.rel}:{group.line}"
    defs = [p for p in group.parts if (p.category in ("function", "var") and not _is_extern_decl(p))]
    if not defs:
        defs = [p for p in group.parts if p.category == "proto"]
    late = [p for p in defs if p.branch > 0]
    if late:
        reasons.append(f"{where}: {', '.join(late[0].names)} is defined in branch {late[0].branch + 1} of the "
                       f"#if group at line {group.line}; a guarded declaration needs the first branch "
                       "(an #elif/#else condition would have to be synthesised)")
        return []
    guard = group.condition.rstrip("\n")
    endif = group.endif.rstrip("\n") if group.branches == 1 else "#endif"
    s.guarded_groups.append((group, direction))
    out = []
    for p in defs:
        if p.multi or p.defines_type:
            reasons.append(f"{s.rel}:{p.line}: {', '.join(p.names)}: several declarators inside the #if group "
                           f"at line {group.line}; split them first")
            continue
        for d in _declaration(s, p, _qualify(group.scope, p.names[0]), group.scope, why, reasons, direction):
            out.append(Decl(d.name, d.text, d.why, d.ns, d.direction, d.const_object, guard, endif,
                            f"{s.rel}:{p.line}"))
    return out


def _declaration(s: Source, it: cx.Item, name: str, scope: str, why: str, reasons: list[str],
                 direction: str) -> list[Decl]:
    """The external declaration of one definition or prototype."""
    where = f"{s.rel}:{it.line}"
    if it.qualifier and not it.scope:
        return []          # an out-of-line class member: its class declares it
    if it.multi or it.defines_type:
        what = ("a type and a variable defined in one declaration" if it.defines_type
                else "several variables in one declaration")
        _need_split(plan_of(s), s, it, reasons, f"{', '.join(it.names)}: {what}, and one of them is shared")
        return []
    head = re.sub(r"\bstatic\s+", "", _head_decl(s, it), count=1)
    prefix = 'extern "C" ' if it.container == "extern-c" else ""
    if it.category in ("function", "proto"):
        params = head[head.find("("):]
        if re.search(r"=", params):
            reasons.append(f"{where}: {it.names[0]} has default arguments; a declaration cannot repeat them")
            return []
        return [Decl(name, f"{prefix}{head};", why, scope, direction)]
    parts = cx._split_top(head, "=")
    decl = parts[0].strip()
    if len(parts) > 1 and re.search(r"\[\s*\]$", decl):
        decl = _bounded(decl, "=".join(parts[1:]))
    if not decl.startswith("extern"):
        decl = "extern " + decl
    if prefix:
        decl = prefix + decl[len("extern "):]
    return [Decl(name, decl + ";", why, scope, direction, _const_object(head, it.names[0]))]


def _bounded(decl: str, init: str) -> str:
    """``T a[]`` with the bound its brace initializer implies (#525).

    ``extern T a[];`` is an incomplete type, so a user's ``sizeof a`` would
    stop compiling in the file that gets only the declaration. A
    one-dimensional brace list gives the count; anything else is left as
    written."""
    body = init.strip()
    if not (body.startswith("{") and body.endswith("}")):
        return decl
    masked = cx.mask(body[1:-1])
    if not masked.strip():
        return decl
    elements = [e for e in cx._split_top(masked) if e.strip()]
    return re.sub(r"\[\s*\]$", f"[{len(elements)}]", decl)


def _check_clashes(plan: Plan, root: Path, reasons: list[str]) -> None:
    # Definitions that would meet in the unit file. Prototypes and extern
    # declarations may repeat a definition from another source: they do not
    # clash.
    seen: dict[str, str] = {}
    for s in plan.sources:
        names = {n for m in s.moves if m.item.category != "proto" and not _is_extern_decl(m.item)
                 for n in m.item.qnames} | {n for it in s.header_items for n in it.qnames}
        for n in sorted(names):
            if n in seen and seen[n] != s.rel:
                reasons.append(f"{n} is defined by both {seen[n]} and {s.rel}; they would meet in {plan.target}")
            seen.setdefault(n, s.rel)
    exported = {d.name: s.rel for s in plan.sources for d in s.declarations
                if d.why.startswith(("static", "anonymous"))}
    if not exported:
        return
    bare = sorted({n.rsplit("::", 1)[-1] for n in exported})
    pattern = re.compile(r"\b(" + "|".join(map(re.escape, bare)) + r")\b")
    for path in sorted((root / "src").rglob("*.c*")):
        rel = path.relative_to(root).as_posix()
        if path.suffix not in (".c", ".cpp") or rel in exported.values() or path.name.startswith("test_"):
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        if not pattern.search(text):
            continue
        for it in cx.scan(text).items:
            if it.category in ("function", "var") and not it.static and not _anonymous(it.container):
                scope = _scope(it, plan.namespaces)
                for n in {_qualify(scope, x) for x in it.names} & set(exported):
                    reasons.append(f"{n} would get external linkage but {rel}:{it.line} defines it too"
                                   + _clash_advice(plan, exported[n], n, rel, it))


def _clash_advice(plan: Plan, source: str, name: str, rel: str, other: cx.Item) -> str:
    """What the original's addresses say about two definitions of one name
    (#514, as in #482): one original global (unify them) or two (rename)."""
    s = next(x for x in plan.sources if x.rel == source)
    mine = next((it for it in s.scan.items if it.static and name.rsplit("::", 1)[-1] in it.names), None)
    here = {a for k, a in (mine.annotations if mine else []) if k == "GLOBAL" or k in FUNCTION_KINDS}
    there = {a for k, a in other.annotations if k == "GLOBAL" or k in FUNCTION_KINDS}
    where = f"{source}:{mine.line}" if mine else source
    if here and there and here & there:
        return (f": one original global (0x{min(here & there):08x}) defined twice; unify them first "
                "(a source fix, not a relocation)")
    if here and there:
        return (f": two original globals (0x{min(here):08x} at {where}, 0x{min(there):08x} at {rel}:{other.line}); "
                f"rename the static at {where} first (a source fix, not a relocation)")
    if here:
        return f": the definition at {rel}:{other.line} is port-only (no annotation); rename it first"
    if there:
        return f": the static at {where} is port-only (no annotation); rename it first"
    return ": neither is annotated; rename one first"


# --------------------------------------------------------------------------
# Rendering


def _strip_static(code: str) -> str:
    """Remove ``static`` from the declaration heads in ``code`` (brace depth 0)."""
    masked = cx.mask(code)
    out, depth, paren, last = [], 0, 0, 0
    for m in re.finditer(r"[{}()]|\bstatic\b[ \t]*", masked):
        tok = m.group(0)
        if tok == "{":
            depth += 1
        elif tok == "}":
            depth -= 1
        elif tok == "(":
            paren += 1
        elif tok == ")":
            paren -= 1
        elif depth == 0 and paren == 0:
            out.append(code[last:m.start()])
            last = m.end()
    out.append(code[last:])
    return "".join(out)


def _strip_static_item(text: str, it: cx.Item) -> str:
    """``it``'s code with ``static`` removed; in a conditional group (#501),
    only from its declarations and definitions, never from a macro body."""
    if not it.cgroup:
        return _strip_static(it.code(text))
    out, last = [], it.start
    for p in it.parts:
        if p.static and p.category in ("var", "function", "proto"):
            out.append(text[last:p.start])
            out.append(_strip_static(p.code(text)))
            last = p.end
    out.append(text[last:it.end])
    return "".join(out)


def _wrap(text: str, wrap: str | None) -> str:
    """``text`` inside the block a wrap names (extern-c, anon, ns:X)."""
    body = text.rstrip("\n")
    if wrap == "extern-c":
        return 'extern "C" {\n' + body + '\n}\n'
    if wrap == "anon":
        return "namespace {\n" + body + "\n} // namespace\n"
    if wrap and wrap.startswith("ns:"):
        names = _ns_names(wrap)
        if names:
            return _ns_block(names, body, anon=wrap.endswith("/anon"))
    return text


def _move_text(s: Source, m: Move) -> str:
    """A moved item with its leading comments, unwrapped."""
    lead = s.text[m.item.lead:m.item.start]
    code = _strip_static_item(s.text, m.item) if m.static_removed else m.item.code(s.text)
    text = lead + code
    return text if text.endswith("\n") else text + "\n"


def _blocks(chunks: list[tuple[str | None, str]], sep: str = "\n") -> list[str]:
    """Join chunks; consecutive chunks with the same wrap share one block."""
    out: list[str] = []
    i = 0
    while i < len(chunks):
        wrap, text = chunks[i]
        if wrap is None:
            out.append(text)
            i += 1
            continue
        group = [text]
        i += 1
        while i < len(chunks) and chunks[i][0] == wrap:
            group.append(chunks[i][1])
            i += 1
        out.append(_wrap(sep.join(t.rstrip("\n") + "\n" for t in group), wrap))
    return out


def _include_only_group(scan, it) -> "cx.Group | None":
    """The top-level, non-nested #if group around include ``it`` when the
    group holds only ``#include`` lines (#525), else None."""
    if it.cond != 1 or it.container or not it.in_group or it.group_why != "it holds only #include lines":
        return None
    for g in scan.groups:
        if g.end >= 0 and not g.in_item and not g.nested and g.start <= it.start < g.end:
            return g
    return None


def _spell(spelled: str, including: str, target: str, library: str, headers: Headers) -> str:
    """The include spelling that resolves from ``target`` to the same file."""
    if spelled.startswith("<"):
        return spelled
    want = headers.resolve(spelled, including, library)
    if want is None or headers.resolve(spelled, target, library) == want:
        return spelled
    if Path(want).parent == Path(target).parent:
        return f'"{Path(want).name}"'
    return f'"{Path(want).relative_to("src").as_posix()}"'


def _private_include(header: str, frm: str) -> str:
    if Path(header).parent == Path(frm).parent:
        return f'#include "{Path(header).name}"'
    return f'#include "{Path(header).relative_to("src").as_posix()}"'


def _remove(text: str, spans: list[tuple[int, int]]) -> str:
    """Remove whole-line spans; collapse the blank lines left at each seam."""
    out, last = [], 0
    for a, b in sorted(spans):
        out.append(text[last:a])
        last = b
    out.append(text[last:])
    result = out[0]
    for part in out[1:]:
        if result.endswith("\n\n"):
            part = part.lstrip("\n")       # one blank line at the seam, not two
        result += part
    if result.count("\n\n\n") > text.count("\n\n\n"):
        result = re.sub(r"\n{3,}", "\n\n", result)
    return result


def _render(plan: Plan, root: Path, headers: Headers, reasons: list[str]) -> None:
    u = plan.unit
    primary = max(plan.sources, key=lambda s: (sum(1 for m in s.moves if m.role == "function"), -plan.sources.index(s)))
    ordered_sources = [primary] + [s for s in plan.sources if s is not primary]

    # This relocation's private headers (#496): one pair per source, written
    # once and never edited again.
    header_texts: dict[str, str] = {}
    for s in plan.sources:
        if s.shares_for:
            header_texts[s.for_header] = _render_private(root, s, "for", reasons)
        if s.shares_from:
            header_texts[s.from_header] = _render_private(root, s, "from", reasons)
    needed = {u.rsplit("::", 1)[-1] for s in plan.sources for m in s.moves for u in m.item.uses}
    for s in plan.sources:
        if s.shares_for:
            needed |= _tail_uses(header_texts[s.for_header])

    # Includes: the primary source's, in order, then the others' new ones.
    # A legacy file's private headers come along only when the unit uses a
    # name they declare: they are frozen, so including them is safe, but a
    # unit that does not need them should not count their declarations.
    # A top-level #if group that holds only #include lines (#525) is copied
    # verbatim, in its place, once: the unit needs the platform headers it
    # selects. Other conditional includes are not copied.
    includes, seen, skipped, copied_groups = [], set(), [], set()
    for s in ordered_sources:
        for it in s.scan.items:
            if it.category != "include" or not it.include:
                continue
            if it.cond or it.container:
                g = _include_only_group(s.scan, it)
                members = [x for x in s.scan.items if g is not None and g.start <= x.start < g.end]
                if g is None or any(_spell(x.include, s.rel, plan.target, s.entry.library, headers)
                                    != x.include for x in members):
                    skipped.append(f"{s.rel}:{it.line} {it.include}")
                    continue
                block = s.scan.text[g.start:g.end].rstrip("\n")
                if block not in copied_groups:
                    copied_groups.add(block)
                    includes.append(block)
                continue
            resolved = headers.resolve(it.include, s.rel, s.entry.library) or it.include
            if resolved in seen:
                continue
            if is_private_header(root, resolved) and not (_names_in_file(root / resolved) & needed):
                continue
            seen.add(resolved)
            includes.append("#include " + _spell(it.include, s.rel, plan.target, s.entry.library, headers))
    if skipped:
        plan.notes.append("Conditional includes not copied (the build checks the result): " + ", ".join(skipped))
    private = [s.for_header for s in ordered_sources if s.shares_for]
    # A const object defined in the unit needs its extern first, or it would
    # have internal linkage: only then does the unit include the from-header.
    private += [s.from_header for s in ordered_sources if any(d.const_object for d in s.from_decls)]
    private = [h for h in dict.fromkeys(private) if h not in seen]

    # The unit file: types and macros, globals, variables, forward
    # declarations, the unit's functions in address order, static helpers.
    def section(pred):
        return [m for s in ordered_sources for m in s.moves if pred(m)]
    types = section(lambda m: m.item.category in ("type", "macro"))
    # A whole #if group with helper functions (#514) goes before the unit's
    # functions, as it was before its users in the source: no forward
    # declarations for a group's alternatives.
    groups = section(lambda m: m.item.cgroup and m.item.category == "function")
    globals_ = sorted(section(lambda m: m.role == "global"), key=lambda m: m.address)
    variables = section(lambda m: m.role.startswith("closure") and m.item.category == "var")
    protos = section(lambda m: m.item.category == "proto")
    functions = sorted(section(lambda m: m.role == "function"), key=lambda m: m.address)
    helpers = section(lambda m: m.role.startswith("closure") and m.item.category == "function"
                      and not m.item.cgroup)
    data = _data_order(globals_ + variables)
    globals_, variables = data, []
    plan.order = types + globals_ + variables + protos + groups + functions + helpers
    if len(plan.order) != len(plan.moves):
        reasons.append("internal error: moved items not all placed")
        return

    # Forward declarations for functions used before their definition.
    src_of = {id(m): next(s for s in plan.sources if m in s.moves) for m in plan.order}
    visible = set()
    for s in ordered_sources:
        visible |= headers.visible([it.include for it in s.scan.items if it.category == "include" and it.include],
                                   s.rel, s.entry.library)
        visible |= {d.name for d in s.for_decls}       # the unit does not include its from-headers
    defined_at = {}
    for pos, m in enumerate(plan.order):
        for n in m.item.qnames:
            defined_at.setdefault(n, pos)
    forward: list[tuple[str, str]] = []           # (namespace, declaration)
    declared = set()
    for pos, m in enumerate(plan.order):
        if m.item.category == "proto":
            declared.update(m.item.qnames)
        for n in sorted(m.item.ruses):
            at = defined_at.get(n)
            if at is None or at <= pos or n in visible or n in declared:
                continue
            target_move = plan.order[at]
            if target_move.item.category != "function":
                reasons.append(f"{n} ({target_move.item.category}) is used before its definition in {plan.target}")
                continue
            # An anonymous-namespace function is forward-declared in an
            # anonymous namespace too: the same one, per translation unit (#514).
            anon = _anonymous(target_move.wrap)
            s = src_of[id(target_move)]
            head = _head_decl(s, target_move.item)
            if target_move.static_removed:
                head = re.sub(r"\bstatic\s+", "", head, count=1)
            decl = head + ";"
            if target_move.wrap == "extern-c":
                decl = 'extern "C" ' + decl
            forward.append((target_move.item.scope, decl, anon))
            declared.add(n)
    plan.forward = [_ns_block(ns.split("::") if ns else [], d, anon=anon, inline=True) if ns or anon else d
                    for ns, d, anon in forward]

    # using directives and declarations the moved global-scope code needs.
    usings = _needed_usings(plan, ordered_sources, visible, reasons)
    plan.usings = usings

    title = u.unit_name or u.unit_id
    out = ["// SPDX-License-Identifier: LGPL-3.0-or-later",
           f"/* {u.unit_id} {title}: the unit's functions in original address order and the",
           " * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).",
           f" * Body hashes: {RECORDS}/{u.unit_id}.csv. */"]
    out += includes
    out += [_private_include(h, plan.target) for h in private]
    body = ["\n".join(out) + "\n"]
    if usings:
        body.append("\n".join(usings) + "\n")
    chunks: list[tuple[str | None, str]] = []
    prev: Move | None = None

    def add(m: Move):
        nonlocal prev
        s = src_of[id(m)]
        text = _move_text(s, m)
        adjacent = (prev is not None and src_of[id(prev)] is s and prev.wrap == m.wrap
                    and s.text[prev.item.end:m.item.lead] == "")
        if adjacent:
            chunks[-1] = (m.wrap, chunks[-1][1].rstrip("\n") + "\n" + text.rstrip("\n") + "\n")
        else:
            chunks.append((m.wrap, text))
        prev = m
    for m in types + globals_ + variables + protos + groups:
        add(m)
    body.extend(_blocks(chunks))
    chunks = []
    prev = None
    if plan.forward:
        body.append("/* Defined below (forward declarations added by tools/relocate_unit.py). */\n"
                    + "\n".join(plan.forward) + "\n")
    for m in functions + helpers:
        add(m)
    body.extend(_blocks(chunks))
    plan.files[plan.target] = "\n".join(b.rstrip("\n") + "\n" for b in body)

    # The sources: remove what moved, strip static from shared statics,
    # include the private header.
    for s in plan.sources:
        spans = [(m.item.lead, m.item.end) for m in s.moves] + [(it.lead, it.end) for it in s.header_items]
        text = s.text
        edits = []
        for it in s.strip_static:
            edits.append((it.start, it.end, _strip_static_item(text, it)))
        for first, last in _runs(s.anon_exits, text):
            # Close the anonymous namespace before the shared items and
            # reopen it after them, so their order in the file is kept.
            edits.append((first.lead, first.lead, "} // namespace\n"))
            edits.append((last.end, last.end, "namespace {\n"))
        pieces, last = [], 0
        for a, b, new in sorted(edits, key=lambda e: (e[0], e[1])):
            pieces.append(text[last:a])
            pieces.append(new)
            last = b
        pieces.append(text[last:])
        new_text = "".join(pieces)
        # Recompute spans after the edits (edits never overlap spans). An
        # insertion at a span's start goes before it; at its end, after.
        delta = [(a, b, len(new) - (b - a)) for a, b, new in edits]

        def moved(p, end=False):
            return p + sum(d for a, b, d in delta if a < p or (a == b == p and not end))
        spans = [(moved(a), moved(b, end=True)) for a, b in spans]
        new_text = _remove(new_text, spans)
        new_text = _drop_empty_anonymous(text, new_text)
        seen_ns = {n.split("::")[0] for n in headers.visible(
            [it.include for it in s.scan.items if it.category == "include" and it.include],
            s.rel, s.entry.library) if "::" in n}
        new_text = _drop_emptied_namespaces(s.text, new_text, seen_ns)
        for path in (s.for_header, s.from_header):
            if path in header_texts:
                plan.files[path] = header_texts[path]
                names = {n for it in cx.scan(header_texts[path]).items for n in it.names}
                new_text = _place_private_include(new_text, _private_include(path, s.rel), names)
        plan.files[s.rel] = new_text

    plan.files.update(_wiring(plan, root))
    _verify(plan, reasons)


EMPTY_ANON = re.compile(r"^namespace[ \t]*\{[ \t]*\n(?:[ \t]*\n)*\}[^\n]*\n", re.M)


def _runs(items: list[cx.Item], text: str) -> list[tuple[cx.Item, cx.Item]]:
    """Consecutive items (only blank lines between them) as (first, last)."""
    runs: list[list[cx.Item]] = []
    for it in sorted(items, key=lambda i: i.start):
        if runs and runs[-1][-1].container == it.container and not text[runs[-1][-1].end:it.lead].strip():
            runs[-1].append(it)
        else:
            runs.append([it])
    return [(r[0], r[-1]) for r in runs]


def _drop_empty_anonymous(before: str, after: str) -> str:
    """Remove anonymous ``namespace { }`` blocks the edits left empty."""
    if len(EMPTY_ANON.findall(after)) <= len(EMPTY_ANON.findall(before)):
        return after
    result = EMPTY_ANON.sub("", after)
    if result.count("\n\n\n") > before.count("\n\n\n"):
        result = re.sub(r"\n{3,}", "\n\n", result)
    return result


EMPTY_NS = re.compile(r"^namespace[ \t]+([A-Za-z_]\w*)[ \t]*\{\s*\}[ \t]*(?://[^\n]*)?\n", re.M)


def _drop_emptied_namespaces(before: str, after: str, declared: set[str] = frozenset()) -> str:
    """Remove ``namespace X { }`` blocks the relocation emptied.

    An emptied block is dropped only when X stays declared for later uses:
    by another ``namespace X {`` in the file or by an included header
    (``declared``).
    """
    had = {m.group(0) for m in EMPTY_NS.finditer(before)}

    def drop(m):
        if m.group(0) in had:
            return m.group(0)
        rest = after[:m.start()] + after[m.end():]
        if m.group(1) in declared:
            return ""
        if re.search(r"\bnamespace\s+%s\s*\{" % re.escape(m.group(1)), cx.mask(rest)):
            return ""
        return m.group(0)
    result = EMPTY_NS.sub(drop, after)
    if result.count("\n\n\n") > before.count("\n\n\n"):
        result = re.sub(r"\n{3,}", "\n\n", result)
    return result


def _needed_usings(plan: Plan, sources: list[Source], visible: set[str], reasons: list[str]) -> list[str]:
    """The sources' file-scope ``using`` items that moved global-scope code needs.

    ``using namespace X;`` is needed when moved code in the global scope uses a
    bare name that resolves only to ``X::name`` (in the source or a visible
    header); ``using X::n;`` when it uses a bare ``n``. They are copied
    verbatim, after the includes, and stay in the source as well.
    """
    out: list[str] = []
    for s in sources:
        usings = [it for it in s.scan.items if it.category == "using" and it.using
                  and not it.cond and not it.container]
        if not usings:
            continue
        file_names = {n for it in s.scan.items for n in getattr(it, "qnames", [])}
        known = file_names | visible
        moved_bare = {u for m in s.moves if not m.item.scope for u in m.item.uses if "::" not in u}
        for it in usings:
            kind, target = it.using
            if kind == "namespace":
                needed = any(f"{target}::{u}" in known and u not in known for u in moved_bare)
            else:
                needed = target.rsplit("::", 1)[-1] in moved_bare
            if not needed:
                continue
            ns = target if kind == "namespace" else target.split("::")[0]
            if not any(n.startswith(ns + "::") for n in visible):
                reasons.append(f"{s.rel}:{it.line}: moved code needs '{it.code(s.text).strip()}', but no header "
                               f"the unit file includes declares namespace {ns}")
                continue
            line = it.code(s.text).strip()
            if line not in out:
                out.append(line)
    return out


def _data_order(moves: list[Move]) -> list[Move]:
    """Owned globals in address order, closure variables after them, except
    that a variable moves up to just before the first item that uses it."""
    order = list(moves)
    for _ in range(len(order) * len(order) + 1):
        changed = False
        for i, m in enumerate(order):
            for j in range(i + 1, len(order)):
                if set(order[j].item.qnames) & m.item.ruses:
                    order.insert(i, order.pop(j))
                    changed = True
                    break
            if changed:
                break
        if not changed:
            return order
    return order


def _place_private_include(text: str, line: str, names: set[str]) -> str:
    """Insert the private include, or move an existing one up when the
    header's names (now including this relocation's) are used before it."""
    at = text.find(line + "\n")
    if at < 0:
        return _insert_include(text, line, names)
    without = text[:at] + text[at + len(line) + 1:]
    wanted = _insert_include(without, line, names)
    return wanted if wanted.find(line) < at else text


def _insert_include(text: str, line: str, names: set[str] = frozenset()) -> str:
    """Add the private header's include ``line`` before its first use.

    It goes after the last unconditional ``#include`` that precedes the first
    code item using one of ``names`` (the header's names), so includes the
    header's declarations depend on stay in front of it. With no such use it
    goes after the top include block (the includes before the first code
    item). A late include after code that already uses the names would come
    too late (finding 527).
    """
    scan = cx.scan(text)

    def uses_names(it):
        return any(u.rsplit("::", 1)[-1] in names for u in it.uses)
    code = [it for it in scan.items if it.kind in ("function", "declaration")]
    first_use = next((it.start for it in code if uses_names(it)), None)
    limit = first_use if first_use is not None else (code[0].start if code else len(text))
    incs = [it for it in scan.items if it.category == "include" and not it.cond and not it.container
            and it.start < limit]
    at = incs[-1].end if incs else _line_after_spdx(text)
    return text[:at] + line + "\n" + text[at:]


def _line_after_spdx(text: str) -> int:
    m = re.match(r"(//[^\n]*SPDX[^\n]*\n)", text)
    return m.end() if m else 0


PRIVATE_NAME = re.compile(r"^[A-Za-z0-9_]+?_(?:shared|for_[A-Za-z0-9_]+|from_[A-Za-z0-9_]+)\.hpp$")


def is_private_header(root: Path, rel: str) -> bool:
    """A relocation's private header: ``<l>_shared.hpp`` (before #496),
    ``<l>_for_<unit>.hpp`` or ``<l>_from_<unit>.hpp`` next to legacy ``<l>``."""
    p = Path(rel)
    name = p.name
    if not PRIVATE_NAME.match(name):
        return False
    for m in re.finditer(r"_(?:shared\.hpp$|for_|from_)", name):
        stem = name[:m.start()]
        if any((Path(root) / p.parent / f"{stem}{ext}").exists() for ext in (".cpp", ".c")):
            return True
    return False


def legacy_private_headers(root: Path, legacy: str) -> list[str]:
    """The private headers of a legacy file that exist on disk."""
    p = Path(legacy)
    found = []
    for path in sorted((Path(root) / p.parent).glob(f"{p.stem}_*.hpp")):
        name = path.name
        if name == f"{p.stem}_shared.hpp" or re.fullmatch(rf"{re.escape(p.stem)}_(?:for|from)_[A-Za-z0-9_]+\.hpp", name):
            found.append(path.relative_to(root).as_posix())
    return found


_NAMES_CACHE: dict[Path, set[str]] = {}


def _names_in_file(path: Path) -> set[str]:
    """Names a header declares or defines (bare), from a cxx_items scan."""
    if path not in _NAMES_CACHE:
        try:
            text = path.read_text(encoding="utf-8")
        except OSError:
            return set()
        _NAMES_CACHE[path] = {n for it in cx.scan(text).items for n in it.names}
    return _NAMES_CACHE[path]


def _tail_uses(text: str) -> set[str]:
    return {u.rsplit("::", 1)[-1] for u in cx._uses(cx.mask(text))}


def _render_private(root: Path, s: Source, kind: str, reasons: list[str]) -> str:
    """One of this relocation's private headers (#496), self-contained.

    ``for``: types and macros moved out of the legacy file, and declarations of
    its items the unit uses. ``from``: declarations of the unit's items the
    legacy file uses. Each includes the legacy file's earlier private headers
    whose names it uses, so it never has to be edited later.
    """
    rel = s.for_header if kind == "for" else s.from_header
    guard = "LRR_" + re.sub(r"\W", "_", Path(rel).relative_to("src").as_posix()).upper()
    chunks: list[tuple[str | None, str]] = []
    functions: list[tuple[str | None, str]] = []
    if kind == "for":
        for it in s.header_items:
            text = it.text(s.text).rstrip("\n") + "\n"
            # A type or macro that leaves an anonymous namespace lands in
            # its outer namespace (recorded as such in the relocation CSV).
            wrap = _outer_wrap(it.container) if _anonymous(it.container) else it.container
            # Inline functions (#514) come after the declarations they may use.
            (functions if it.category in ("function", "proto") else chunks).append(
                (wrap if _container_namespace(wrap) else None, text))
    parts = _blocks(chunks)
    decls = [(f"ns:{d.ns}" if d.ns else None, d.rendered())
             for d in (s.for_decls if kind == "for" else s.from_decls)]
    decl_text = "".join(_blocks(decls, sep=""))
    body = "\n".join(p.rstrip("\n") + "\n" for p in parts)
    if decl_text:
        body += ("\n" if body else "") + decl_text
    if functions:
        body += ("\n" if body else "") + "\n".join(p.rstrip("\n") + "\n" for p in _blocks(functions))
    uses = _tail_uses(body)
    deps = [h for h in legacy_private_headers(root, s.rel)
            if h != rel and _names_in_file(Path(root) / h) & uses]
    if kind == "from" and s.shares_for:
        for_names = {n for it in s.header_items for n in it.names} | {d.name.rsplit("::", 1)[-1] for d in s.for_decls}
        if for_names & uses:
            deps.append(s.for_header)
    unit = s.unit_stem
    who = (f"what {unit} uses from {Path(s.rel).name}" if kind == "for"
           else f"what {Path(s.rel).name} uses from {unit}")
    lines = ["// SPDX-License-Identifier: LGPL-3.0-or-later",
             f"/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): {who}.",
             " * Written once by that relocation and never edited again, so later",
             " * relocations do not change what earlier unit files see. "
             + ("Included by both files." if kind == "for" else f"Included by {Path(s.rel).name} only.")
             + " */",
             f"#ifndef {guard}",
             f"#define {guard}", ""]
    lines += [_private_include(h, rel) for h in deps]
    text = "\n".join(lines) + ("\n\n" if deps else "\n") + body + f"\n#endif /* {guard} */\n"
    existing = Path(root) / rel
    if existing.exists() and existing.read_text(encoding="utf-8") != text:
        reasons.append(f"{rel} exists and private headers are frozen; a relocation writes its own pair")
    return text


def _wiring(plan: Plan, root: Path) -> dict[str, str]:
    files = {}
    rel_source = Path(plan.target).relative_to("src").as_posix()
    text = (root / link_order.LIST).read_text(encoding="utf-8")
    text = link_order.insert_unit(text, plan.unit.unit_id, plan.library, rel_source, link_order.unit_starts(root))
    crt = link_order.crt_start(root)
    starts = link_order.unit_starts(root)
    for s in plan.sources:
        left = plan.files.get(s.rel, s.text)
        source = Path(s.rel).relative_to("src").as_posix()
        median = link_order.lower_median(left)
        if median is None:
            text = link_order.retire_legacy(text, source)
            plan.notes.append(f"{s.rel} holds no original function any more: it moves to the port tail")
        elif s.entry.kind == "legacy" and f"{median:08x}" != s.entry.key:
            text = link_order.place_legacy(text, source, median, starts, crt)
            plan.notes.append(f"{s.rel}: legacy key {s.entry.key} -> {median:08x} (the lower median of its "
                              "remaining markers), placed in address order")
    files[link_order.LIST] = text

    meson = root / cf.SOURCE_DIRS[plan.library] / "meson.build"
    if meson.exists():
        mt = meson.read_text(encoding="utf-8")
        name = Path(plan.target).name
        if f"'{name}'" not in mt:
            # The native parity check reads these literal arrays, which are
            # passed to the target; adding a positional target source bypasses it.
            m = re.search(r"^%s_sources\s*=\s*\[\s*\n" % plan.library, mt, re.M)
            if not m:
                plan.notes.append(f"{meson.relative_to(root)}: no {plan.library}_sources list; "
                                  "add the source by hand")
            else:
                mt = mt[:m.end()] + f"  '{name}',\n" + mt[m.end():]
                files[meson.relative_to(root).as_posix()] = mt

    path = root / UNIT_FUNCTIONS
    rows = list(csv.reader(io.StringIO(path.read_text(encoding="utf-8"))))
    moved = {m.address for m in plan.moves if m.role == "function"}
    col = rows[0].index("current_source")
    for r in rows[1:]:
        if int(r[0], 16) in moved:
            r[col] = plan.target
    buf = io.StringIO()
    csv.writer(buf, lineterminator="\n").writerows(rows)
    files[UNIT_FUNCTIONS] = buf.getvalue()
    return files


# --------------------------------------------------------------------------
# Verification and the record


def _verify(plan: Plan, reasons: list[str]) -> None:
    """Re-scan the written text and require every moved item to be verbatim."""
    new_scan = cx.scan(plan.files[plan.target])
    if new_scan.problems:
        reasons.extend(f"{plan.target}: {p}" for p in new_scan.problems)
    by_addr = new_scan.annotated()
    by_name: dict[str, list[cx.Item]] = {}
    for it in new_scan.items:
        for n in it.names:
            by_name.setdefault(n, []).append(it)
    records = []
    for m in plan.order:
        s = next(x for x in plan.sources if m in x.moves)
        before = m.item.code(s.text)
        expected = _strip_static_item(s.text, m.item) if m.static_removed else before
        if m.address is not None:
            after_item = by_addr.get(m.address)
        else:
            cands = [it for it in by_name.get(m.item.names[0] if m.item.names else "", [])
                     if it.category == m.item.category and cx.normalise(it.code(plan.files[plan.target]))
                     == cx.normalise(expected)]
            after_item = cands[0] if cands else None
        after = after_item.code(plan.files[plan.target]) if after_item else ""
        ok = cx.normalise(after) == cx.normalise(expected)
        expected_container = _outer_wrap(m.item.container) if m.anon_left else m.item.container
        if after_item is not None and after_item.container != expected_container:
            ok = False
            reasons.append(f"{m.source}:{m.item.line} {m.name}: moved from scope {m.item.container or 'global'} "
                           f"into {after_item.container or 'global'}")
        if m.item.category == "function" and after_item is not None:
            # A group's "body" includes its later alternatives' heads, so it
            # is compared after the recorded static removal.
            reference = expected if m.item.group else before
            ok = ok and cx.normalise(_body(reference, m.item, s.text)) == cx.normalise(_body(after, after_item, plan.files[plan.target]))
        if not ok:
            reasons.append(f"{m.source}:{m.item.line} {m.name}: the moved text differs from the original")
        records.append({
            "role": m.role, "address": f"0x{m.address:08x}" if m.address is not None else "",
            "name": m.name, "from": f"{m.source}:{m.item.line}",
            "to": f"{plan.target}:{after_item.line if after_item else '?'}",
            "before_sha256": sha(before), "after_sha256": sha(after),
            "edit": "; ".join(x for x in (
                "static removed (shared with remaining code)" if m.static_removed else "",
                "left the anonymous namespace (shared with remaining code)" if m.anon_left else "") if x),
        })
    for s in plan.sources:
        new_text = plan.files[s.rel]
        for it in s.header_items:
            header = plan.files[s.for_header]
            ok = cx.normalise(it.text(s.text)) in cx.normalise(header)
            if not ok:
                reasons.append(f"{s.rel}:{it.line}: the header text differs from the original")
            records.append({"role": f"shared-{it.category}", "address": "", "name": ",".join(it.names),
                            "from": f"{s.rel}:{it.line}", "to": s.for_header,
                            "before_sha256": sha(it.code(s.text)), "after_sha256": sha(it.code(s.text)) if ok else "",
                            "edit": ""})
        for it in s.anon_exits:
            records.append({"role": f"stays-{it.category}", "address": "", "name": ",".join(it.names),
                            "from": f"{s.rel}:{it.line}", "to": s.rel,
                            "before_sha256": sha(it.code(s.text)),
                            "after_sha256": sha(_strip_static_item(s.text, it) if it in s.strip_static else it.code(s.text)),
                            "edit": "left the anonymous namespace (shared with the unit file)"})
            if it.code(s.text) and cx.normalise(it.code(s.text)) not in cx.normalise(new_text) \
                    and it not in s.strip_static:
                reasons.append(f"{s.rel}:{it.line}: {', '.join(it.names)} changed while leaving its anonymous namespace")
        for it in s.header_items:
            if _anonymous(it.container):
                records.append({"role": f"shared-{it.category}", "address": "", "name": ",".join(it.names),
                                "from": f"{s.rel}:{it.line}", "to": s.for_header,
                                "before_sha256": sha(it.code(s.text)), "after_sha256": sha(it.code(s.text)),
                                "edit": "left the anonymous namespace (moved to the private header)"})
        for d in s.declarations:
            if not d.guard:
                continue
            path = s.for_header if d.direction == "for" else s.from_header
            ok = cx.normalise(d.rendered()) in cx.normalise(plan.files.get(path, ""))
            if not ok:
                reasons.append(f"{d.origin}: the guarded declaration of {d.name} is missing from {path}")
            records.append({"role": "guarded-declaration", "address": "", "name": d.name,
                            "from": d.origin, "to": path,
                            "before_sha256": sha(d.guard), "after_sha256": sha(d.rendered()),
                            "edit": "declaration wrapped in its #if group's condition, verbatim: "
                                    + d.guard.splitlines()[0].strip()})
        for it in s.strip_static:
            records.append({"role": f"stays-{it.category}", "address": "", "name": ",".join(it.names),
                            "from": f"{s.rel}:{it.line}", "to": s.rel,
                            "before_sha256": sha(it.code(s.text)), "after_sha256": sha(_strip_static_item(s.text, it)),
                            "edit": "static removed (shared with the unit file)"})
        # Each private header must come before any code that uses its names.
        for path, names in ((s.for_header, {d.name.rsplit("::", 1)[-1] for d in s.for_decls}
                             | {n for it in s.header_items for n in it.names}),
                            (s.from_header, {d.name.rsplit("::", 1)[-1] for d in s.from_decls})):
            if path not in plan.files:
                continue
            include = _private_include(path, s.rel)
            at = new_text.find(include)
            early = [it for it in cx.scan(new_text[:max(at, 0)]).items
                     if it.kind in ("function", "declaration") and set(it.uses) & names]
            if at < 0 or early:
                reasons.append(f"{s.rel}: {Path(path).name} is included after code that uses "
                               + ", ".join(sorted(set().union(*(set(it.uses) & names for it in early))) or "it"))
        # Annotation conservation: nothing lost or doubled.
        before = sorted(cx.ANNOTATION.findall(s.text))
        after = sorted(cx.ANNOTATION.findall(new_text))
        moved = sorted(a for m in s.moves for a in cx.ANNOTATION.findall(m.item.text(s.text)))
        if sorted(after + moved) != before:
            reasons.append(f"{s.rel}: annotations were lost or duplicated")
    for kind, addr in cx.ANNOTATION.findall(plan.files[plan.target]):
        a = int(addr, 16)
        if kind in FUNCTION_KINDS + ("LIBRARY",) and not plan.unit.text_start <= a < plan.unit.text_end:
            reasons.append(f"{plan.target}: holds {kind} 0x{a:08x}, outside {plan.unit.unit_id}")
    target_ann = sorted(cx.ANNOTATION.findall(plan.files[plan.target]))
    moved_all = sorted(a for s in plan.sources for m in s.moves
                       for a in cx.ANNOTATION.findall(m.item.text(s.text)))
    if target_ann != moved_all:
        reasons.append(f"{plan.target}: annotations differ from the moved ones")
    plan.records = records
    plan.files[f"{RECORDS}/{plan.unit.unit_id}.csv"] = _records_csv(records)


def _body(code: str, item: cx.Item, text: str) -> str:
    """A function's body from its first brace (for #if groups, the whole group)."""
    if item.group:
        return code[code.find("\n#"):] if "\n#" in code else code
    off = item.body - item.start
    return code[off:] if 0 <= off < len(code) else code


FIELDS = ("role", "address", "name", "from", "to", "before_sha256", "after_sha256", "edit")


def _records_csv(records: list[dict]) -> str:
    buf = io.StringIO()
    w = csv.DictWriter(buf, FIELDS, lineterminator="\n")
    w.writeheader()
    w.writerows(records)
    return buf.getvalue()


# --------------------------------------------------------------------------
# Migration of the per-source private headers (#496)


@dataclass
class Migration:
    files: dict[str, str | None] = field(default_factory=dict)   # None: delete the file
    records: list[dict] = field(default_factory=list)
    notes: list[str] = field(default_factory=list)
    reasons: list[str] = field(default_factory=list)


def _defined_qnames(text: str, namespaces: set[str]) -> set[str]:
    """Qualified names a source file defines (functions and variables)."""
    out = set()
    for it in cx.scan(text).items:
        if it.category == "function" or (it.category == "var" and not (
                re.match(r"\s*extern\b", text[it.start:it.end]) and "=" not in text[it.start:it.end])):
            scope = _scope(it, namespaces)
            out.update(_qualify(scope, n) for n in it.names)
    return out


def _include_lines(text: str, rel: str, target: str, root: Path, library: str, headers: Headers) -> list[re.Match]:
    """``#include`` lines of ``text`` (file ``rel``) that resolve to ``target``."""
    out = []
    for m in re.finditer(r'^[ \t]*#[ \t]*include[ \t]*("[^"\n]+")[^\n]*\n', text, re.M):
        if headers.resolve(m.group(1), rel, library) == target:
            out.append(m)
    return out


def plan_header_migration(root: Path = ROOT) -> Migration:
    """Give every unit file its own frozen snapshot of the pre-#496 headers.

    Before #496 each legacy file had one ``<l>_shared.hpp`` that every
    relocation appended to, and every unit relocated out of ``<l>`` included it.
    For each such unit U, the migration writes ``<l>_for_<U>.hpp``: a verbatim
    copy of the header's body (only the guard and the comment differ) and points
    each of U's includes of it there. U's declaration sequence, and therefore every symbol
    counter MSVC derives from it (finding 538), is unchanged, so the migration
    itself changes no listing. ``<l>_shared.hpp`` stays as it is, included only
    by ``<l>``, and the tool never writes it again: new relocations write their
    own pair (``_render_private``). A duplicate include of the old header is
    kept (pointing at the snapshot): it still costs a symbol-counter step.
    """
    root = Path(root)
    mig = Migration()
    entries = link_order.load(root)
    by_path = {e.path: e for e in entries}
    unit_files = {e.path: e for e in entries if e.kind == "unit"}
    headers = Headers(root)
    texts: dict[str, str] = {}

    def text_of(rel):
        if rel not in texts:
            texts[rel] = (root / rel).read_text(encoding="utf-8")
        return texts[rel]

    def library_of(rel):
        e = by_path.get(rel)
        return e.library if e else ("gods98" if rel.startswith("src/gods98/") else "game")

    sources = sorted(p.relative_to(root).as_posix() for p in (root / "src").rglob("*")
                     if p.suffix.lower() in cf.SOURCE_SUFFIXES and p.is_file() and not p.name.startswith("test_"))
    edits: dict[str, list[tuple[int, int, str]]] = {}
    for p in sorted((root / "src").rglob("*_shared.hpp")):
        hdr = p.relative_to(root).as_posix()
        stem = p.name[:-len("_shared.hpp")]
        legacy = next((f"{p.parent.relative_to(root).as_posix()}/{stem}{ext}" for ext in (".cpp", ".c")
                       if (p.parent / f"{stem}{ext}").exists()), None)
        if legacy is None:
            continue
        htext = text_of(hdr)
        guard = re.search(r"^#ifndef[ \t]+(\w+)[ \t]*\n#define[ \t]+\1[ \t]*\n", htext, re.M)
        end = htext.rstrip().rfind("#endif")
        if not guard or end < guard.end():
            mig.reasons.append(f"{hdr}: no include guard to replace")
            continue
        body = htext[guard.end():end]
        includers = [rel for rel in sources if rel != hdr and p.name in text_of(rel)
                     and _include_lines(text_of(rel), rel, hdr, root, library_of(rel), headers)]
        others = [rel for rel in includers if rel != legacy and rel not in unit_files]
        if others:
            mig.reasons.append(f"{hdr} is included by {', '.join(others)}, neither its legacy file nor a unit file")
            continue
        for rel in includers:
            if rel == legacy:
                continue
            snapshot = _pair_header(legacy, rel, "for")
            if (root / snapshot).exists():
                mig.reasons.append(f"{snapshot} exists already")
                continue
            mig.files[snapshot] = _snapshot_text(snapshot, hdr, legacy, rel, body)
            matches = _include_lines(text_of(rel), rel, hdr, root, library_of(rel), headers)
            # Every occurrence points at the snapshot, duplicates included: a
            # repeated include of a guarded header still costs MSVC a step of
            # its symbol counter (finding 538), so dropping one would drift
            # the unit (it re-rolled 0x004089d0 in a trial).
            # Each include keeps its spelling, with only the file name changed:
            # MSVC's codegen depends on the spelled include path, not only on
            # the declarations (a "gods98/" prefix re-rolled 0x004089d0).
            for m in matches:
                old_name, new_name = Path(hdr).name, Path(snapshot).name
                line = m.group(0)
                quoted = m.group(1)
                spelled = quoted[:-1 - len(old_name)] + new_name + '"' if quoted.endswith(old_name + '"') else None
                if spelled is None:
                    mig.reasons.append(f"{rel}: cannot respell {quoted.strip()}")
                    continue
                edits.setdefault(rel, []).append((m.start(), m.end(), line.replace(quoted, spelled, 1)))
            mig.records.append({"header": hdr, "unit": rel, "snapshot": snapshot,
                                "body_sha256": sha(body), "includes_replaced": len(matches)})
    for rel, es in edits.items():
        text = text_of(rel)
        out, last = [], 0
        for a, b, new in sorted(es):
            out.append(text[last:a])
            out.append(new)
            last = b
        out.append(text[last:])
        mig.files[rel] = "".join(out)
    # Verbatim: no annotated item changed; each snapshot's declarations equal
    # the old header's, in order.
    for rel, new in mig.files.items():
        if rel.endswith((".hpp", ".h")):
            continue
        before = {a: it.code(text_of(rel)) for it in cx.scan(text_of(rel)).items for _, a in it.annotations}
        after = {a: it.code(new) for it in cx.scan(new).items for _, a in it.annotations}
        if before != after:
            mig.reasons.append(f"{rel}: an annotated item changed during the migration")
    for r in mig.records:
        old = [cx.normalise(it.code(text_of(r["header"]))) for it in cx.scan(text_of(r["header"])).items
               if not (it.category == "macro" and it.names and it.names[0].startswith("LRR_"))]
        new_text = mig.files[r["snapshot"]]
        new = [cx.normalise(it.code(new_text)) for it in cx.scan(new_text).items
               if not (it.category == "macro" and it.names and it.names[0].startswith("LRR_"))]
        if old != new:
            mig.reasons.append(f"{r['snapshot']}: declarations differ from {r['header']}")
    buf = io.StringIO()
    w = csv.DictWriter(buf, ("header", "unit", "snapshot", "body_sha256", "includes_replaced"), lineterminator="\n")
    w.writeheader()
    w.writerows(mig.records)
    mig.files[f"{RECORDS}/private-header-migration.csv"] = buf.getvalue()
    return mig


def _snapshot_text(rel: str, old: str, legacy: str, unit_file: str, body: str) -> str:
    guard = "LRR_" + re.sub(r"\W", "_", Path(rel).relative_to("src").as_posix()).upper()
    lines = ["// SPDX-License-Identifier: LGPL-3.0-or-later",
             f"/* Private (ADR 0006 decision 4; #496): what {Path(unit_file).name} sees from",
             f" * {Path(legacy).name}. A verbatim snapshot of {Path(old).name} as it was when the",
             " * #496 migration froze it, so the unit's declarations no longer change",
             " * with later relocations. Never edited by tools/relocate_unit.py. */",
             f"#ifndef {guard}",
             f"#define {guard}"]
    return "\n".join(lines) + "\n" + body + f"#endif /* {guard} */\n"


def _pair_header(legacy: str, unit_file: str, kind: str) -> str:
    p = Path(legacy)
    return (p.parent / f"{p.stem}_{kind}_{Path(unit_file).stem}.hpp").as_posix()


def _pair_header_text(rel: str, legacy: str, unit_file: str, kind: str, deps: list[str], body: str) -> str:
    guard = "LRR_" + re.sub(r"\W", "_", Path(rel).relative_to("src").as_posix()).upper()
    unit = Path(unit_file).stem
    who = (f"what {unit} uses from {Path(legacy).name}" if kind == "for"
           else f"what {Path(legacy).name} uses from {unit}")
    lines = ["// SPDX-License-Identifier: LGPL-3.0-or-later",
             f"/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): {who}.",
             " * Written once (here by the #496 migration) and never edited again, so",
             " * later relocations do not change what earlier unit files see. "
             + ("Included by both files." if kind == "for" else f"Included by {Path(legacy).name} only.") + " */",
             f"#ifndef {guard}",
             f"#define {guard}", ""]
    lines += [_private_include(h, rel) for h in deps]
    return "\n".join(lines) + ("\n\n" if deps else "\n") + body + f"\n#endif /* {guard} */\n"


# --------------------------------------------------------------------------
# Report


CHECKLIST = (
    "Relocated with `tools/relocate_unit.py {id}`; body hashes in `{records}` (all verbatim)",
    "Claim rule: no open conversion PR for {id}",
    "One commit for the unit, holding only the move and the linkage it needs (ADR 0006 rule 3)",
    "No shared header edited; private headers only: {private}",
    "Native: `cmake --build build` and `ctest --test-dir build --output-on-failure`",
    "Meson source list updated ({meson}) until #452 decides Meson's future",
    "`make reccmp-check`; every drop labelled `drift` or `drift-reordered` (ADR 0006 rule 2); "
    "review `drift-review` drops and report them",
    "`git add` the new files and run `python3 tools/binary_progress_hash.py --write-manifest`, "
    "then `make match-progress`; commit `match-baseline.txt`, `match-listing-digests.txt`, "
    "`binary-progress.json` and `tools/binary_progress_inputs.txt`",
    "Report the change in pair inversions, unit-order inversions, unit contiguity and the net score",
)


def render_report(plan: Plan, footprint: cf.Footprint | None) -> str:
    u = plan.unit
    out = []
    w = out.append
    w(f"## Relocation: {u.unit_id} {u.unit_name or '(unnamed)'} -> `{plan.target}`")
    w("")
    if plan.done:
        w(plan.notes[0] + "; nothing to do.")
        return "\n".join(out) + "\n"
    fn = [m for m in plan.order if m.role == "function"]
    gl = [m for m in plan.order if m.role == "global"]
    cl = [m for m in plan.order if m.role.startswith("closure")]
    w(f"Moves {len(fn)} functions, {len(gl)} owned globals and {len(cl)} closure items from "
      + ", ".join(f"`{s.rel}`" for s in plan.sources) + ".")
    w("")
    w("| Address | Function | From |")
    w("| --- | --- | --- |")
    for m in fn:
        w(f"| `{m.address:08x}` | `{m.name}` | `{m.source}:{m.item.line}` |")
    for addr, name in plan.missing:
        w(f"| `{addr:08x}` | {name} | not in `src/` (nothing to move) |")
    if gl:
        w("")
        w("Owned globals: " + ", ".join(f"`{m.name}` (`{m.address:08x}`)" for m in gl))
    if cl:
        w("")
        w("Closure (used only by the moved code): " + ", ".join(f"`{m.name}` ({m.item.category})" for m in cl))
    for s in plan.sources:
        if not s.shares and not s.strip_static:
            continue
        w("")
        w(f"Shared with the rest of `{s.rel}` through `{s.for_header}` (both files) and "
          f"`{s.from_header}` (`{Path(s.rel).name}` only):")
        for it in s.header_items:
            w(f"* moved into the header: `{', '.join(it.names)}` ({it.category})")
        for d in s.declarations:
            w(f"* `{d.text}`" + (f" in namespace `{d.ns}`" if d.ns else "") + f" ({d.why})")
    if plan.forward:
        w("")
        w("Forward declarations added: " + ", ".join(f"`{d}`" for d in plan.forward))
    if plan.usings:
        w("")
        w("`using` items carried over: " + ", ".join(f"`{d}`" for d in plan.usings))
    for note in plan.notes:
        w("")
        w(f"> {note}")
    if footprint is not None:
        w("")
        w("### Conversion footprint (for the later ADR 0005 conversion)")
        w("")
        w("```")
        w(cf.render_text(footprint).rstrip())
        w("```")
    w("")
    w("### Relocation checklist (ADR 0006, plan S3)")
    w("")
    private = ", ".join(f"`{h}`" for s in plan.sources for h, on in ((s.for_header, s.shares_for),
                                                                      (s.from_header, s.shares_from)) if on) or "none"
    meson = ", ".join(f"`{p}`" for p in plan.files if p.endswith("meson.build")) or "already listed"
    for n, text in enumerate(CHECKLIST, 1):
        text = text.format(id=u.unit_id, records=f"{RECORDS}/{u.unit_id}.csv", private=private, meson=meson)
        w(f"- [{'x' if n in (1, 4, 6) else ' '}] {n}. {text}")
    return "\n".join(out) + "\n"


# --------------------------------------------------------------------------
# Survey


def survey(root: Path) -> list[dict]:
    """Try every unrelocated game and gods98 unit; collect what blocks each."""
    units = cf.load_units(root)
    listed = {e.key for e in link_order.load(root) if e.kind == "unit"}
    headers = Headers(root)
    results = []
    for u in units:
        if u.unit_kind not in cf.CONVERTIBLE_KINDS or u.unit_id in listed:
            continue
        stem = u.stem if u.unit_name else u.unit_id.lower().replace("-", "_")
        try:
            try:
                plan = plan_relocation(u.unit_id, root, stem=stem, headers=headers)
            except Refused as e:
                # A stem clash only needs --stem: look past it with a variant.
                if not any(classify_reason(r).startswith("stem") for r in e.reasons):
                    raise
                stem += "_unit"
                plan = plan_relocation(u.unit_id, root, stem=stem, headers=headers)
            results.append({"unit": u.unit_id, "name": u.unit_name,
                            "status": "ok" if not stem.endswith("_unit") else f"ok with --stem {stem}",
                            "functions": sum(1 for m in plan.moves if m.role == "function"),
                            "sources": [s.rel for s in plan.sources],
                            "shared": sum(len(s.declarations) + len(s.header_items) for s in plan.sources),
                            "reasons": []})
        except Refused as e:
            status = "refused"
            reasons = e.reasons
            if all(classify_reason(r).startswith("needs --split") for r in e.reasons):
                # Would it relocate once its declarations are split? Plan the
                # relocation itself on the split text, in a scratch copy: a
                # blocker behind the split must show (#514: G98-009, GAME-021).
                try:
                    split = plan_relocation(u.unit_id, root, stem=stem, headers=headers, split=True)
                    if split.splits:
                        behind = _plan_after_split(root, u.unit_id, stem, split.files)
                        if behind:
                            status = "refused after --split-declarations"
                            reasons = e.reasons + behind
                        else:
                            status = "ok after --split-declarations"
                except Refused:
                    pass
            results.append({"unit": u.unit_id, "name": u.unit_name, "status": status,
                            "functions": 0, "sources": [], "shared": 0, "reasons": reasons})
    return results


def _plan_after_split(root: Path, unit: str, stem: str, files: dict[str, str]) -> list[str]:
    """The refusals of ``unit``'s relocation once ``files`` (its declaration
    splits) are applied, planned in a scratch copy of src/ and docs/coverage/."""
    with tempfile.TemporaryDirectory(prefix="relocate-survey-") as tmp:
        scratch = Path(tmp)
        for d in ("src", "docs/coverage"):
            shutil.copytree(Path(root) / d, scratch / d, symlinks=True)
        for rel, text in files.items():
            (scratch / rel).parent.mkdir(parents=True, exist_ok=True)
            (scratch / rel).write_text(text, encoding="utf-8")
        try:
            plan_relocation(unit, scratch, stem=stem)
        except Refused as e:
            return [r.replace(tmp + "/", "") for r in e.reasons]
    return []


def classify_reason(reason: str) -> str:
    table = (
        ("--split-declarations", "needs --split-declarations (source change)"),
        ("cannot be split", "declaration that cannot be split"),
        ("synthesised", "#if group: definition outside the first branch"),
        ("holds an annotated item", "#if group with an annotated item"),
        ("inside an #if block", "#if block"),
        ("is inside an #if block", "#if block"),
        ("directive splits", "directive inside a declaration"),
        ("interrupts the declaration", "directive inside a declaration"),
        ("annotated definitions in a header", "function defined in a header"),
        ("nested in itself", "namespace nested in itself (source fix)"),
        ("two original globals", "name clash: two original globals (rename, source fix)"),
        ("one original global", "name clash: one global defined twice (unify, source fix)"),
        ("port-only", "name clash with a port-only definition (rename, source fix)"),
        ("qualified", "qualified (namespace/member) definition"),
        ("inside namespace", "named namespace"),
        ("nested namespace", "nested namespace or linkage block"),
        ("out-of-line member of", "out-of-line class member"),
        ("several levels", "qualified definition with several levels"),
        ("declares namespace", "using without a declared namespace"),
        ("moved from scope", "verbatim check failed"),
        ("macro or complex declarator", "macro or complex declarator"),
        ("complex declarator", "macro or complex declarator"),
        ("not attached", "annotation not attached to a definition"),
        ("anonymous namespace", "anonymous namespace shared"),
        ("inline", "shared inline function"),
        ("several declarators", "several declarators"),
        ("share one declaration", "several declarators"),
        ("several variables", "several declarators"),
        ("would meet in", "name clash between source files"),
        ("would get external linkage", "name clash with another file"),
        ("is #undef'd or redefined", "macro redefined or #undef'd"),
        ("builds for the", "backend-only source"),
        ("not in the link-order list", "file not in the list"),
        ("is the unit file of", "in another unit's file"),
        ("LIBRARY annotation", "LIBRARY annotation"),
        ("shares its last line", "two items on one line"),
        ("starts on a line", "two items on one line"),
        ("type and a variable", "type and variable in one declaration"),
        ("defined in one declaration", "type and variable in one declaration"),
        ("default arguments", "default arguments"),
        ("used before its definition", "use before definition"),
        ("annotates a", "annotation on a non-definition"),
        ("private header needs", "header needs a static"),
        ("no // FUNCTION", "no annotated function"),
        ("clashes with", "stem clashes with an existing file (needs --stem)"),
        ("file of the list", "stem is a legacy file's name (needs --stem)"),
        ("defined in namespace", "named namespace"),
        ("inside an #if block that is not", "#if block"),
        ("unnamed unit", "unnamed unit (needs --stem)"),
        ("also defined by", "overload or duplicate definition"),
        ("differs from the original", "verbatim check failed"),
    )
    for needle, label in table:
        if needle in reason:
            return label
    return "other"


def render_survey(results: list[dict]) -> str:
    from collections import Counter
    out = ["| Unit | Name | Status | Functions | Shared decls | Blocking constructs |",
           "| --- | --- | --- | ---: | ---: | --- |"]
    total = Counter()
    for r in results:
        labels = Counter(classify_reason(x) for x in r["reasons"])
        total.update(set(labels))
        blocks = ", ".join(f"{k} ×{v}" for k, v in sorted(labels.items())) or "-"
        out.append(f"| {r['unit']} | {r['name'] or '-'} | {r['status']} | {r['functions'] or '-'} | "
                   f"{r['shared'] or '-'} | {blocks} |")
    out.append("")
    out.append("Units blocked by each construct:")
    out.append("")
    for k, v in total.most_common():
        out.append(f"* {k}: {v}")
    ok = sum(1 for r in results if r["status"] == "ok")
    out.append("")
    out.append(f"{ok} of {len(results)} units relocate as they are.")
    return "\n".join(out) + "\n"


# --------------------------------------------------------------------------
# CLI


def write(plan: Plan, root: Path) -> list[str]:
    written = []
    for rel, text in plan.files.items():
        path = root / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
        written.append(rel)
    return written


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description=__doc__.split("\n\n")[0],
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("unit", nargs="?", help="unit id (GAME-001) or name (Advisor.c)")
    p.add_argument("--stem", help="file stem (default: the unit name in lower case)")
    p.add_argument("--root", type=Path, default=ROOT)
    p.add_argument("--dry-run", action="store_true", help="plan and report; write nothing")
    p.add_argument("--json", action="store_true", help="machine-readable output")
    p.add_argument("--report", type=Path, help="also write the report to this file")
    p.add_argument("--no-footprint", action="store_true", help="skip the conversion footprint")
    p.add_argument("--survey", action="store_true", help="try every unrelocated unit and tabulate what blocks it")
    p.add_argument("--split-declarations", action="store_true",
                   help="only split the declarations the relocation needs split (a source change, "
                        "committed on its own), then stop")
    p.add_argument("--check-layout", action="store_true",
                   help="with --split-declarations: compile each split source before and after with MSVC 5.0 "
                        "and refuse if its data layout changes")
    p.add_argument("--migrate-private-headers", action="store_true",
                   help="split every pre-#496 <file>_shared.hpp into per-relocation headers, then stop")
    p.add_argument("--allow-layout-change", action="store_true",
                   help="with --check-layout: write the split even if the data layout changes")
    args = p.parse_args(argv)
    root = args.root.resolve()
    if args.migrate_private_headers:
        mig = plan_header_migration(root)
        for r in mig.reasons:
            print(f"refused: {r}", file=sys.stderr)
        if mig.reasons:
            return 2
        print(f"Private-header migration (#496): {len(mig.records)} per-unit snapshots of "
              f"{len({r['header'] for r in mig.records})} headers.")
        for n in mig.notes:
            print(f"> {n}")
        deleted = sorted(k for k, v in mig.files.items() if v is None)
        written = sorted(k for k, v in mig.files.items() if v is not None)
        if args.dry_run:
            print("Would write: " + ", ".join(written))
            print("Would delete: " + (", ".join(deleted) or "-"))
            return 0
        for rel in written:
            path = root / rel
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(mig.files[rel], encoding="utf-8")
        for rel in deleted:
            (root / rel).unlink()
        print("Wrote: " + ", ".join(written))
        if deleted:
            print("Deleted: " + ", ".join(deleted))
        return 0
    if args.survey:
        results = survey(root)
        text = json.dumps(results, indent=2) + "\n" if args.json else render_survey(results)
        sys.stdout.write(text)
        if args.report:
            args.report.write_text(text, encoding="utf-8")
        return 0
    if not args.unit:
        p.error("give a unit or --survey")
    if args.split_declarations:
        try:
            plan = plan_relocation(args.unit, root, stem=args.stem, split=True)
        except Refused as e:
            print(f"refused: {e.unit}:", file=sys.stderr)
            for r in e.reasons:
                print(f"  - {r}", file=sys.stderr)
            return 2
        layout = check_split_layout(plan, root) if args.check_layout and plan.splits else None
        report = render_split_report(plan, layout)
        sys.stdout.write(report)
        if args.report:
            args.report.write_text(report, encoding="utf-8")
        if layout and not all(same for _, same, _ in layout) and not args.allow_layout_change:
            print("\nrefused: a split changes the data layout (pass --allow-layout-change to write it anyway)",
                  file=sys.stderr)
            return 3
        if plan.splits and not args.dry_run:
            print("\nWrote: " + ", ".join(write(plan, root)))
        elif plan.splits:
            print("\nWould write: " + ", ".join(sorted(plan.files)))
        return 0
    footprint = None
    if not args.no_footprint:
        footprint = cf.compute_footprint(args.unit, root, stem=args.stem, with_data=False)
    try:
        plan = plan_relocation(args.unit, root, stem=args.stem)
    except Refused as e:
        print(f"refused: {e.unit} cannot be relocated safely:", file=sys.stderr)
        for r in e.reasons:
            print(f"  - {r}", file=sys.stderr)
        return 2
    if footprint is not None:
        footprint.new_files = [plan.target] + [h for s in plan.sources for h, on in
                                               ((s.for_header, s.shares_for), (s.from_header, s.shares_from)) if on]
    report = render_report(plan, footprint)
    written = [] if args.dry_run or plan.done else write(plan, root)
    if args.report:
        args.report.write_text(report, encoding="utf-8")
    if args.json:
        print(json.dumps({"unit": plan.unit.unit_id, "target": plan.target, "done": plan.done,
                          "files": sorted(plan.files), "written": written, "records": plan.records,
                          "notes": plan.notes}, indent=2))
    else:
        sys.stdout.write(report)
        if args.dry_run and not plan.done:
            print("\nWould write: " + ", ".join(sorted(plan.files)))
        elif written:
            print("\nWrote: " + ", ".join(written))
    return 0


if __name__ == "__main__":
    sys.exit(main())
