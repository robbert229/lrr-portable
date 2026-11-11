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

Constructs it refuses rather than guesses at: definitions inside ``#if`` blocks
(except a group of alternative definitions of one function), directives that
split a declaration, functions defined in headers, qualified or macro-built
definitions, named namespaces, shared ``inline`` functions, shared statics that
need a declaration it cannot write, and name clashes between moved files.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import io
import json
import re
import sys
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
            for it in scan.items:
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
    """X for a member of ``namespace X {}`` or of an anonymous namespace
    inside it (``namespace X { namespace { } }``), else None."""
    if container and container.startswith("ns:"):
        if "/" not in container:
            return container[3:]
        if re.fullmatch(r"ns:[A-Za-z_]\w*/anon", container):
            return container[3:].split("/")[0]
    return None


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
    wrap: str | None = None   # extern-c or anon

    def before(self, text: str) -> str:
        return self.item.code(text)


@dataclass
class Source:
    rel: str
    entry: link_order.Entry
    text: str
    scan: cx.Scan
    moves: list[Move] = field(default_factory=list)
    header_items: list[cx.Item] = field(default_factory=list)   # types/macros/decls to the private header
    strip_static: list[cx.Item] = field(default_factory=list)   # staying items that get external linkage
    declarations: list[Decl] = field(default_factory=list)

    @property
    def shared_header(self) -> str:
        p = Path(self.rel)
        return (p.parent / f"{p.stem}_shared.hpp").as_posix()

    @property
    def shares(self) -> bool:
        return bool(self.header_items or self.declarations)


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
                    headers: Headers | None = None) -> Plan:
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
    for s in sources:
        _plan_source(plan, s, [a for a in func_ann if a.file == s.rel],
                     [a for a in glob_ann if a.file == s.rel], headers, reasons)
    if reasons:
        raise Refused(u.unit_id, reasons)
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
        for k, it in enumerate(items):
            it.scope = _scope(it, namespaces)
            it.qnames = [] if it.category in self.SKIP else [_qualify(it.scope, n) for n in it.names]
            for name in it.qnames:
                if name in owner:
                    parent[find(k)] = find(owner[name])
                else:
                    owner[name] = k
        self.owner = owner
        for it in items:
            it.ruses = {self.resolve(u, it.scope) for u in it.uses}
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
        if scope and f"{scope}::{use}" in self.owner:
            return f"{scope}::{use}"
        if use in self.owner:
            return use
        for kind, target in self.usings:
            if kind == "namespace" and f"{target}::{use}" in self.owner:
                return f"{target}::{use}"
            if kind == "name" and target.rsplit("::", 1)[-1] == use and target in self.owner:
                return target
        return use


def _kind(items: list[cx.Item], members: list[int]) -> str:
    """local (type/macro), internal (static), declaration (no definition here), external."""
    its = [items[k] for k in members]
    cats = {i.category for i in its}
    if cats <= {"type", "macro"}:
        return "local"
    if any(i.static or _anonymous(i.container) for i in its):
        return "internal"
    if not any(i.category == "function" or (i.category == "var" and not _is_extern_decl(i)) for i in its):
        return "declaration"
    return "external"


def _is_extern_decl(item: cx.Item) -> bool:
    code = getattr(item, "text_code", "")
    return item.category == "var" and bool(re.match(r"\s*extern\b", code)) and "=" not in code


def _plan_source(plan: Plan, s: Source, funcs: list[cf.Annotation], globs: list[cf.Annotation],
                 headers: Headers, reasons: list[str]) -> None:
    items = s.scan.items
    for it in items:
        it.text_code = s.text[it.start:it.end]
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
        if role == "global" and it.multi:
            reasons.append(f"{s.rel}:{it.line}: GLOBAL 0x{a.address:08x} is a declaration of several variables")
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
        if kinds[c] == "internal":
            exported.add(c)
        elif kinds[c] == "external" and not (cl.names[c] & visible_here):
            s.declarations.extend(_declarations(s, items, cl, c, "moved; remaining code uses it", reasons))
    for c in sorted(reach - candidates, key=first):
        if kinds[c] == "external" and not (cl.names[c] & visible_here):
            s.declarations.extend(_declarations(s, items, cl, c, "stays; moved code uses it", reasons))
    for c in sorted(exported, key=first):
        if any(_anonymous(items[k].container) for k in cl.members[c]):
            reasons.append(f"{s.rel}:{items[cl.members[c][0]].line}: {', '.join(sorted(cl.names[c]))} is in an "
                           "anonymous namespace and used by moved and remaining code")
            continue
        if any(items[k].inline for k in cl.members[c]):
            reasons.append(f"{s.rel}:{items[cl.members[c][0]].line}: inline {', '.join(sorted(cl.names[c]))} is used by "
                           "moved and remaining code")
            continue
        s.declarations.extend(_declarations(s, items, cl, c, "static shared by moved and remaining code", reasons))

    # Types, macros and declarations the private header needs.
    need = set()
    for d in s.declarations:
        need |= {cl.resolve(u, d.ns) for u in cx._uses(cx.mask(d.text))}
    for c in to_header:
        need |= cl.uses[c]
    while True:
        add = set()
        for n in need:
            d = cl.definer.get(n)
            if d is None or d in to_header:
                continue
            if kinds[d] in ("local", "declaration"):
                add.add(d)
            elif d in moving or d in exported or kinds[d] in ("internal", "external"):
                if kinds[d] == "internal" and d not in exported:
                    reasons.append(f"{s.rel}: the private header needs {n}, a static that is not shared")
        if not add:
            break
        to_header |= add
        moving -= add
        for d in add:
            need |= cl.uses[d]

    # Problems of every item that changes place or linkage.
    involved = set(moving) | to_header | exported
    for c in involved:
        for k in cl.members[c]:
            it = items[k]
            where_ = f"{s.rel}:{it.line} ({', '.join(it.names) or it.category})"
            if it.cond:
                reasons.append(f"{where_}: inside an #if block")
            if it.qualifier and not it.scope:
                reasons.append(f"{where_}: out-of-line member of {it.qualifier} (a class, not a namespace)")
            for p in it.problems:
                reasons.append(f"{where_}: {p}")
            if c in to_header and any(kind == "GLOBAL" or kind in FUNCTION_KINDS for kind, _ in it.annotations):
                reasons.append(f"{where_}: an annotated item would move into the private header")
            if it.category == "macro" and _redefined(items, it.names):
                reasons.append(f"{where_}: macro {it.names[0]} is #undef'd or redefined in the file")
    if reasons:
        return

    for c in sorted(moving, key=lambda c: cl.members[c][0]):
        for k in cl.members[c]:
            it = items[k]
            if k in seeds:
                role, addr = seeds[k]
            else:
                role, addr = f"closure-{it.category}", None
            s.moves.append(Move(s.rel, it, role, addr, ",".join(it.names) or it.category,
                                static_removed=c in exported and it.static,
                                wrap=it.container if it.container in ("extern-c", "anon")
                                or _container_namespace(it.container) else None))
    for c in sorted(to_header, key=lambda c: cl.members[c][0]):
        s.header_items.extend(items[k] for k in cl.members[c])
    for c in exported - moving:
        s.strip_static.extend(items[k] for k in cl.members[c] if items[k].static)


def _redefined(items: list[cx.Item], names: list[str]) -> bool:
    n = 0
    for it in items:
        if it.category == "macro" and it.names == names:
            n += 1
        if it.category == "undef" and set(names) & it.uses:
            return True
    return n > 1


def _head_decl(s: Source, it: cx.Item) -> str:
    """An item's head as a declaration: comments, ``__declspec(naked)``, the
    namespace qualifier of an out-of-line definition removed."""
    head = cx.strip_comments(it.head(s.text))
    head = NAKED.sub("", head)
    if it.qualifier and it.scope and it.names:
        head = re.sub(r"\b%s\s*::\s*(?=%s\b)" % (re.escape(it.qualifier), re.escape(it.names[0])), "", head)
    return head


def _declarations(s: Source, items: list[cx.Item], cl: _Clusters, c: int, why: str,
                  reasons: list[str]) -> list[Decl]:
    """External declarations for the private header, from a cluster's definition."""
    defs = [items[k] for k in cl.members[c] if items[k].category in ("function", "var")
            and not _is_extern_decl(items[k])]
    protos = [items[k] for k in cl.members[c] if items[k].category == "proto"]
    it = (defs or protos or [None])[0]
    if it is None:
        return []
    where = f"{s.rel}:{it.line}"
    if it.multi:
        reasons.append(f"{where}: {', '.join(it.names)} share one declaration and would need external linkage")
        return []
    if it.defines_type:
        reasons.append(f"{where}: {', '.join(it.names)}: a type and a variable defined in one declaration "
                       "would need sharing; declaring it would define the type twice")
        return []
    head = re.sub(r"\bstatic\s+", "", _head_decl(s, it), count=1)
    prefix = 'extern "C" ' if it.container == "extern-c" else ""
    name = it.qnames[0]
    if it.category in ("function", "proto"):
        params = head[head.find("("):]
        if re.search(r"=", params):
            reasons.append(f"{where}: {it.names[0]} has default arguments; a declaration cannot repeat them")
            return []
        return [Decl(name, f"{prefix}{head};", why, it.scope)]
    decl = cx._split_top(head, "=")[0].strip()
    if not decl.startswith("extern"):
        decl = "extern " + decl
    if prefix:
        decl = prefix + decl[len("extern "):]
    return [Decl(name, decl + ";", why, it.scope)]


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
    exported = {d.name: s.rel for s in plan.sources for d in s.declarations if d.why.startswith("static")}
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
                    reasons.append(f"{n} would get external linkage but {rel}:{it.line} defines it too")


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


def _wrap(text: str, wrap: str | None) -> str:
    """``text`` inside the block a wrap names (extern-c, anon, ns:X)."""
    body = text.rstrip("\n")
    if wrap == "extern-c":
        return 'extern "C" {\n' + body + '\n}\n'
    if wrap == "anon":
        return "namespace {\n" + body + "\n} // namespace\n"
    if wrap and wrap.endswith("/anon"):
        ns = wrap[3:].split("/")[0]
        return f"namespace {ns} {{\nnamespace {{\n" + body + f"\n}} // namespace\n}} // namespace {ns}\n"
    ns = _container_namespace(wrap)
    if ns:
        return f"namespace {ns} {{\n" + body + f"\n}} // namespace {ns}\n"
    return text


def _move_text(s: Source, m: Move) -> str:
    """A moved item with its leading comments, unwrapped."""
    lead = s.text[m.item.lead:m.item.start]
    code = m.item.code(s.text)
    if m.static_removed:
        code = _strip_static(code)
    text = lead + code
    return text if text.endswith("\n") else text + "\n"


def _blocks(chunks: list[tuple[str | None, str]]) -> list[str]:
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
        out.append(_wrap("\n".join(t.rstrip("\n") + "\n" for t in group), wrap))
    return out


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

    # Includes: the primary source's, in order, then the others' new ones.
    includes, seen, skipped = [], set(), []
    for s in ordered_sources:
        for it in s.scan.items:
            if it.category != "include" or not it.include:
                continue
            if it.cond or it.container:
                skipped.append(f"{s.rel}:{it.line} {it.include}")
                continue
            resolved = headers.resolve(it.include, s.rel, s.entry.library) or it.include
            if resolved in seen:
                continue
            seen.add(resolved)
            includes.append(_spell(it.include, s.rel, plan.target, s.entry.library, headers))
    if skipped:
        plan.notes.append("Conditional includes not copied (the build checks the result): " + ", ".join(skipped))
    private = [s.shared_header for s in ordered_sources if s.shares or (root / s.shared_header).exists()
               and any(i.include and Path(i.include[1:-1]).name == Path(s.shared_header).name
                       for i in s.scan.items)]

    # The unit file: types and macros, globals, variables, forward
    # declarations, the unit's functions in address order, static helpers.
    def section(pred):
        return [m for s in ordered_sources for m in s.moves if pred(m)]
    types = section(lambda m: m.item.category in ("type", "macro"))
    globals_ = sorted(section(lambda m: m.role == "global"), key=lambda m: m.address)
    variables = section(lambda m: m.role.startswith("closure") and m.item.category == "var")
    protos = section(lambda m: m.item.category == "proto")
    functions = sorted(section(lambda m: m.role == "function"), key=lambda m: m.address)
    helpers = section(lambda m: m.role.startswith("closure") and m.item.category == "function")
    data = _data_order(globals_ + variables)
    globals_, variables = data, []
    plan.order = types + globals_ + variables + protos + functions + helpers
    if len(plan.order) != len(plan.moves):
        reasons.append("internal error: moved items not all placed")
        return

    # Forward declarations for functions used before their definition.
    src_of = {id(m): next(s for s in plan.sources if m in s.moves) for m in plan.order}
    visible = set()
    for s in ordered_sources:
        visible |= headers.visible([it.include for it in s.scan.items if it.category == "include" and it.include],
                                   s.rel, s.entry.library)
        visible |= {d.name for d in s.declarations}
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
            if _anonymous(target_move.wrap):
                reasons.append(f"{n} is in an anonymous namespace and used before its definition in {plan.target}")
                continue
            s = src_of[id(target_move)]
            head = _head_decl(s, target_move.item)
            if target_move.static_removed:
                head = re.sub(r"\bstatic\s+", "", head, count=1)
            decl = head + ";"
            if target_move.wrap == "extern-c":
                decl = 'extern "C" ' + decl
            forward.append((target_move.item.scope, decl))
            declared.add(n)
    plan.forward = [f"namespace {ns} {{ {d} }}" if ns else d for ns, d in forward]

    # using directives and declarations the moved global-scope code needs.
    usings = _needed_usings(plan, ordered_sources, visible, reasons)
    plan.usings = usings

    title = u.unit_name or u.unit_id
    out = ["// SPDX-License-Identifier: LGPL-3.0-or-later",
           f"/* {u.unit_id} {title}: the unit's functions in original address order and the",
           " * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).",
           f" * Body hashes: {RECORDS}/{u.unit_id}.csv. */"]
    out += [f"#include {i}" for i in includes]
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
    for m in types + globals_ + variables + protos:
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
            edits.append((it.start, it.end, _strip_static(it.code(text))))
        pieces, last = [], 0
        for a, b, new in sorted(edits):
            pieces.append(text[last:a])
            pieces.append(new)
            last = b
        pieces.append(text[last:])
        new_text = "".join(pieces)
        # Recompute spans after the static edits (edits never overlap spans).
        delta = []
        for a, b, new in sorted(edits):
            delta.append((a, len(new) - (b - a)))

        def moved(p):
            return p + sum(d for a, d in delta if a < p)
        spans = [(moved(a), moved(b)) for a, b in spans]
        new_text = _remove(new_text, spans)
        seen_ns = {n.split("::")[0] for n in headers.visible(
            [it.include for it in s.scan.items if it.category == "include" and it.include],
            s.rel, s.entry.library) if "::" in n}
        new_text = _drop_emptied_namespaces(s.text, new_text, seen_ns)
        if s.shares:
            line = _private_include(s.shared_header, s.rel)
            plan.files[s.shared_header] = header = _render_header(root, s, plan)
            names = {n for it in cx.scan(header).items for n in it.names}
            new_text = _place_private_include(new_text, line, names)
        plan.files[s.rel] = new_text

    plan.files.update(_wiring(plan, root))
    _verify(plan, reasons)


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


def _render_header(root: Path, s: Source, plan: Plan) -> str:
    rel = s.shared_header
    guard = "LRR_" + re.sub(r"\W", "_", Path(rel).relative_to("src").as_posix()).upper()
    existing = (root / rel).read_text(encoding="utf-8") if (root / rel).exists() else None
    norm = cx.normalise(existing) if existing else ""
    # Types and macros, then declarations; members of a namespace inside it.
    chunks: list[tuple[str | None, str]] = []
    for it in s.header_items:
        text = it.text(s.text).rstrip("\n") + "\n"
        if cx.normalise(text) not in norm:
            chunks.append((it.container if _container_namespace(it.container) else None, text))
    parts = _blocks(chunks)
    decls: list[tuple[str | None, str]] = []
    for d in s.declarations:
        if cx.normalise(d.text) in norm:
            continue
        decls.append((f"ns:{d.ns}" if d.ns else None, d.text))
    decl_text = "".join(_blocks([(w, t + "\n") for w, t in decls]))
    add = "\n".join(p.rstrip("\n") + "\n" for p in parts)
    if decl_text:
        add += ("\n" if add else "") + decl_text
    if existing is None:
        head = ["// SPDX-License-Identifier: LGPL-3.0-or-later",
                f"/* Private to {s.rel} and the units relocated out of it (ADR 0006",
                " * decision 4; tools/relocate_unit.py). Statics both sides use have",
                " * external linkage through the declarations below. Include it only from",
                " * those files: it is not a shared header. */",
                f"#ifndef {guard}",
                f"#define {guard}", ""]
        return "\n".join(head) + "\n" + add + f"\n#endif /* {guard} */\n"
    end = existing.rstrip().rfind("#endif")
    return existing[:end].rstrip("\n") + "\n" + ("\n" + add if add else "") + "\n" + existing[end:]


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
        expected = _strip_static(before) if m.static_removed else before
        if m.address is not None:
            after_item = by_addr.get(m.address)
        else:
            cands = [it for it in by_name.get(m.item.names[0] if m.item.names else "", [])
                     if it.category == m.item.category and cx.normalise(it.code(plan.files[plan.target]))
                     == cx.normalise(expected)]
            after_item = cands[0] if cands else None
        after = after_item.code(plan.files[plan.target]) if after_item else ""
        ok = cx.normalise(after) == cx.normalise(expected)
        if after_item is not None and after_item.container != m.item.container:
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
            "edit": "static removed (shared with remaining code)" if m.static_removed else "",
        })
    for s in plan.sources:
        new_text = plan.files[s.rel]
        for it in s.header_items:
            header = plan.files[s.shared_header]
            ok = cx.normalise(it.text(s.text)) in cx.normalise(header)
            if not ok:
                reasons.append(f"{s.rel}:{it.line}: the header text differs from the original")
            records.append({"role": f"shared-{it.category}", "address": "", "name": ",".join(it.names),
                            "from": f"{s.rel}:{it.line}", "to": s.shared_header,
                            "before_sha256": sha(it.code(s.text)), "after_sha256": sha(it.code(s.text)) if ok else "",
                            "edit": ""})
        for it in s.strip_static:
            records.append({"role": f"stays-{it.category}", "address": "", "name": ",".join(it.names),
                            "from": f"{s.rel}:{it.line}", "to": s.rel,
                            "before_sha256": sha(it.code(s.text)), "after_sha256": sha(_strip_static(it.code(s.text))),
                            "edit": "static removed (shared with the unit file)"})
        # The private header must come before any code that uses its names.
        if s.shares:
            include = _private_include(s.shared_header, s.rel)
            at = new_text.find(include)
            names = {d.name.rsplit("::", 1)[-1] for d in s.declarations} | {n for it in s.header_items for n in it.names}
            early = [it for it in cx.scan(new_text[:max(at, 0)]).items
                     if it.kind in ("function", "declaration") and set(it.uses) & names]
            if at < 0 or early:
                reasons.append(f"{s.rel}: the private header is included after code that uses "
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
        w(f"Shared with the rest of `{s.rel}` through `{s.shared_header}`:")
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
    private = ", ".join(f"`{s.shared_header}`" for s in plan.sources if s.shares) or "none"
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
            results.append({"unit": u.unit_id, "name": u.unit_name, "status": "refused",
                            "functions": 0, "sources": [], "shared": 0, "reasons": e.reasons})
    return results


def classify_reason(reason: str) -> str:
    table = (
        ("inside an #if block", "#if block"),
        ("is inside an #if block", "#if block"),
        ("directive splits", "directive inside a declaration"),
        ("interrupts the declaration", "directive inside a declaration"),
        ("annotated definitions in a header", "function defined in a header"),
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
    args = p.parse_args(argv)
    root = args.root.resolve()
    if args.survey:
        results = survey(root)
        text = json.dumps(results, indent=2) + "\n" if args.json else render_survey(results)
        sys.stdout.write(text)
        if args.report:
            args.report.write_text(text, encoding="utf-8")
        return 0
    if not args.unit:
        p.error("give a unit or --survey")
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
        footprint.new_files = [plan.target] + [s.shared_header for s in plan.sources if s.shares]
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
