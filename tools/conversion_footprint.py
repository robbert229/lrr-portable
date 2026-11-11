#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Conversion footprint of one original translation unit (issue #405).

The *conversion footprint* of a unit is the set of source files and headers an
ADR 0005 conversion will touch: its own new files, the files that hold its
functions today, its callees' headers and its callers. Two conversions whose
footprints share no header can proceed in parallel without conflicts.

This module is the library behind ``tools/convert_unit.py``. It is stdlib-only
and reads three committed inputs, so a conversion-order planner can import it
without a toolchain or the reference binary::

    import sys; sys.path.insert(0, "tools")
    from conversion_footprint import compute_footprint, header_overlap

    a = compute_footprint("G98-029")
    b = compute_footprint("GAME-003")
    shared = header_overlap(a, b)          # empty set: they can run in parallel
    print(a.to_json())

Inputs
  docs/coverage/translation-unit-ranges.csv   unit table (finding 505)
  docs/coverage/translation-units.csv         per-function map, with the
                                              ``current_source`` of each function
  docs/coverage/call-graph/edges.csv          (caller, callee) call edges
  src/**                                      declarations and annotations

Limits, repeated in every report (``Footprint.limitations``):
  * ``edges.csv`` holds only direct calls that Ghidra resolved. Function
    pointers and callbacks (window procedures, enumeration callbacks, tables
    of handlers) are missing, so a unit can have callers or callees that the
    footprint does not list.
  * Declarations are found by name in headers with a small scanner, not a C++
    parser. Overloads and macros can hide a declaration; such a callee is
    reported as ``undeclared``.
  * Data references (``with_data``) come from the instructions of the unit's
    functions in ``reference/LegoRR.exe`` and need ``pefile`` and ``capstone``.
"""

from __future__ import annotations

import csv
import dataclasses
import json
import re
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
UNIT_RANGES = "docs/coverage/translation-unit-ranges.csv"
UNIT_FUNCTIONS = "docs/coverage/translation-units.csv"
EDGES = "docs/coverage/call-graph/edges.csv"
SOURCE_SUFFIXES = (".c", ".cpp", ".h", ".hpp")
CONVERTIBLE_KINDS = ("game", "gods98")
SOURCE_DIRS = {"game": "src/game", "gods98": "src/gods98"}

# Files that every conversion touches (CMake wiring, boundary test). They
# conflict only mechanically, so they are left out of the header overlap.
SHARED_FILES = ("CMakeLists.txt", "src/game/test_c_boundaries.cpp")

LIMITATIONS = (
    "Call edges are the direct calls Ghidra resolved (docs/coverage/call-graph/"
    "edges.csv). Calls through function pointers and callbacks are missing, so "
    "callers and callees reached only that way are not listed.",
    "Declarations are matched by name in src/**/*.h and *.hpp with a small "
    "scanner; check callees reported as 'undeclared' by hand.",
)

ANNOTATION = re.compile(
    r"^\s*//\s*(FUNCTION|STUB|LIBRARY|GLOBAL|STRING|VTABLE|TEMPLATE|SYNTHETIC):"
    r"\s+LEGORR\s+0x([0-9a-fA-F]+)(?:\s+SYMBOL)?\s*$"
)
CONTROL_NAMES = {"if", "for", "while", "switch", "catch", "sizeof", "return"}


# --------------------------------------------------------------------------
# Tables


@dataclass
class UnitRanges:
    """One row of translation-unit-ranges.csv."""

    unit_order: int
    unit_id: str
    unit_name: str
    unit_kind: str
    text_start: int
    text_end: int
    functions: int
    confidence: str
    # Region name -> (first item address, last item address). The end is the
    # start of the last item in the range, not one past it.
    ranges: dict[str, tuple[int, int]]
    note: str

    @property
    def stem(self) -> str:
        """Default file stem: the unit name without ``.c``, else the id."""
        if self.unit_name:
            return re.sub(r"\.c$", "", self.unit_name, flags=re.I).lower()
        return self.unit_id.lower().replace("-", "_")


def _hex(text: str) -> int | None:
    text = text.strip()
    return int(text, 16) if text else None


def _range(text: str) -> tuple[int, int] | None:
    if not text.strip():
        return None
    lo, _, hi = text.partition("-")
    return int(lo, 16), int(hi or lo, 16)


def read_csv(path: Path) -> list[dict]:
    with path.open(newline="", encoding="utf-8") as fh:
        return list(csv.DictReader(fh))


def load_units(root: Path = ROOT) -> list[UnitRanges]:
    units = []
    for row in read_csv(root / UNIT_RANGES):
        ranges = {}
        for region, column in (("rdata", "rdata_range"), ("data", "data_range"),
                               ("string", "string_range"), ("bss", "bss_range")):
            r = _range(row[column])
            if r:
                ranges[region] = r
        units.append(UnitRanges(
            unit_order=int(row["unit_order"]), unit_id=row["unit_id"],
            unit_name=row["unit_name"], unit_kind=row["unit_kind"],
            text_start=int(row["text_start"], 16), text_end=int(row["text_end"], 16),
            functions=int(row["functions"]), confidence=row["boundary_confidence"],
            ranges=ranges, note=row["note"]))
    return units


def find_unit(units: list[UnitRanges], key: str) -> UnitRanges:
    """Resolve ``G98-029``, ``Keys.c`` or ``keys`` to a unit."""
    wanted = key.strip().lower()
    for u in units:
        if u.unit_id.lower() == wanted:
            return u
    named = [u for u in units if u.unit_name and
             (u.unit_name.lower() == wanted or re.sub(r"\.c$", "", u.unit_name.lower()) == wanted)]
    if len(named) == 1:
        return named[0]
    if named:
        raise SystemExit(f"{key!r} names several units: " + ", ".join(u.unit_id for u in named))
    raise SystemExit(f"no unit {key!r} in {UNIT_RANGES}")


@dataclass
class FunctionRow:
    address: int
    ghidra_name: str
    unit_id: str
    unit_name: str
    unit_kind: str
    confidence: str
    current_source: str


def load_function_rows(root: Path = ROOT) -> list[FunctionRow]:
    return [FunctionRow(int(r["address"], 16), r["function_name"], r["unit_id"],
                        r["unit_name"], r["unit_kind"], r["confidence"],
                        r["current_source"]) for r in read_csv(root / UNIT_FUNCTIONS)]


def load_edges(root: Path = ROOT) -> list[tuple[int, int]]:
    with (root / EDGES).open(newline="", encoding="utf-8") as fh:
        rd = csv.reader(fh)
        next(rd)
        return [(int(a, 16), int(b, 16)) for a, b in rd]


# --------------------------------------------------------------------------
# Source index: annotations and header declarations


def mask_comments(text: str) -> str:
    """Blank out comments, keeping offsets and newlines."""
    def blank(m):
        return re.sub(r"[^\n]", " ", m.group(0))
    return re.sub(r"/\*.*?\*/|//[^\n]*", blank, text, flags=re.S)


def _mask_preprocessor(text: str) -> str:
    """Blank preprocessor lines (with continuations) in comment-masked text."""
    def blank(m):
        return re.sub(r"[^\n]", " ", m.group(0))
    return re.sub(r"^[ \t]*#(?:[^\n]*\\\n)*[^\n]*", blank, text, flags=re.M)


@dataclass(frozen=True)
class Decl:
    file: str
    line: int
    in_c: bool          # declared with C linkage (extern "C" block or prefix)

    def to_dict(self):
        return dataclasses.asdict(self)


@dataclass(frozen=True)
class Annotation:
    kind: str
    address: int
    file: str
    line: int
    name: str | None


class HeaderScan:
    """Declaration-level names of one header, with their linkage."""

    def __init__(self, text: str):
        masked = _mask_preprocessor(mask_comments(text))
        self.functions: dict[str, list[tuple[int, bool]]] = {}
        self.variables: dict[str, list[tuple[int, bool]]] = {}
        # (kind, in_c) per open brace; 'decl' braces keep declarations visible.
        stack: list[tuple[str, bool]] = []
        stmt_start = 0
        i, n = 0, len(masked)
        while i < n:
            ch = masked[i]
            if ch == "{":
                prefix = masked[stmt_start:i]
                if re.search(r'\bextern\s*"C"\s*$', prefix):
                    kind = "decl"
                    in_c = True
                elif re.search(r"\bnamespace\b[^;()]*$", prefix):
                    kind = "decl"
                    in_c = False
                elif ")" in prefix.rstrip()[-40:] or re.search(r"\)\s*(const)?\s*$", prefix):
                    kind = "body"
                    in_c = False
                else:
                    kind = "agg"
                    in_c = False
                stack.append((kind, in_c))
                stmt_start = i + 1
            elif ch == "}":
                if stack:
                    stack.pop()
                stmt_start = i + 1
            elif ch == ";":
                self._statement(masked, stmt_start, i, stack)
                stmt_start = i + 1
            i += 1

    def _visible(self, stack):
        return all(kind == "decl" for kind, _ in stack)

    def _statement(self, masked, start, end, stack):
        if not self._visible(stack):
            return
        text = masked[start:end]
        in_c = any(c for _, c in stack) or bool(re.search(r'\bextern\s*"C"', text))
        line = masked.count("\n", 0, start + len(text) - len(text.lstrip())) + 1
        paren = text.find("(")
        if paren >= 0:
            m = re.search(r"([A-Za-z_]\w*)\s*$", text[:paren])
            if m and m.group(1) not in CONTROL_NAMES:
                self.functions.setdefault(m.group(1), []).append((line, in_c))
            return
        if re.match(r"\s*(?:extern\b)", text):
            m = re.search(r"([A-Za-z_]\w*)\s*(?:\[[^\]]*\]\s*)*$", text)
            if m:
                self.variables.setdefault(m.group(1), []).append((line, in_c))


def _name_after(lines: list[str], index: int, kind: str) -> str | None:
    """The identifier declared by the first code after an annotation."""
    chunk = []
    in_block = False
    for raw in lines[index + 1:index + 24]:
        s = raw.strip()
        if in_block:
            if "*/" in s:
                in_block = False
            continue
        if not s or s.startswith("//") or s.startswith("#"):
            continue
        if s.startswith("/*"):
            if "*/" not in s:
                in_block = True
            continue
        chunk.append(s)
        joined = re.sub(r"__declspec\s*\([^)]*\)|__attribute__\s*\(\([^)]*\)\)|\b__cdecl\b|\b__stdcall\b",
                        " ", " ".join(chunk))
        if kind == "GLOBAL":
            m = re.search(r"([A-Za-z_]\w*)\s*(?:\[[^\]]*\]\s*)*(?:=|;|\{|$)", joined)
            if "(" not in joined.split("=")[0] and re.search(r"[=;{]", joined):
                m = re.search(r"([A-Za-z_]\w*)\s*(?:\[[^\]]*\]\s*)*(?:=|;|\{)", joined)
                return m.group(1) if m else None
        elif kind in ("FUNCTION", "STUB", "LIBRARY"):
            p = joined.find("(")
            if p >= 0:
                m = re.search(r"([A-Za-z_]\w*)\s*$", joined[:p])
                if m and m.group(1) not in CONTROL_NAMES:
                    return m.group(1)
                return None
        else:
            return None
    return None


class SourceIndex:
    """Annotations, header declarations and function-name mentions in src/."""

    def __init__(self, root: Path = ROOT):
        self.root = root
        self.annotations: dict[int, list[Annotation]] = {}
        self.functions: dict[str, list[Decl]] = {}      # declaration-level names in headers
        self.variables: dict[str, list[Decl]] = {}
        self.files: dict[str, str] = {}
        src = root / "src"
        for path in sorted(src.rglob("*")):
            if path.suffix.lower() not in SOURCE_SUFFIXES or not path.is_file():
                continue
            rel = path.relative_to(root).as_posix()
            text = path.read_text(encoding="utf-8", errors="replace")
            self.files[rel] = text
            lines = text.split("\n")
            for number, line in enumerate(lines):
                if "LEGORR" not in line:
                    continue
                m = ANNOTATION.match(line)
                if m:
                    kind, addr = m.group(1), int(m.group(2), 16)
                    a = Annotation(kind, addr, rel, number + 1, _name_after(lines, number, kind))
                    self.annotations.setdefault(addr, []).append(a)
            if path.suffix.lower() in (".h", ".hpp"):
                scan = HeaderScan(text)
                for name, entries in scan.functions.items():
                    self.functions.setdefault(name, []).extend(Decl(rel, ln, c) for ln, c in entries)
                for name, entries in scan.variables.items():
                    self.variables.setdefault(name, []).extend(Decl(rel, ln, c) for ln, c in entries)

    def function_annotation(self, address: int) -> Annotation | None:
        for a in self.annotations.get(address, ()):
            if a.kind in ("FUNCTION", "STUB", "LIBRARY"):
                return a
        return None

    def global_annotation(self, address: int) -> Annotation | None:
        for a in self.annotations.get(address, ()):
            if a.kind == "GLOBAL":
                return a
        return None

    def mentions(self, names: list[str], test_only: bool = True) -> dict[str, list[str]]:
        """Test files (``test_*``) that mention each name as a whole word."""
        result: dict[str, list[str]] = {}
        wanted = {n for n in names if n}
        if not wanted:
            return result
        pattern = re.compile(r"\b(" + "|".join(re.escape(n) for n in sorted(wanted)) + r")\b")
        for rel, text in self.files.items():
            if test_only and not Path(rel).name.startswith("test_"):
                continue
            for name in set(pattern.findall(text)):
                result.setdefault(name, []).append(rel)
        return {k: sorted(v) for k, v in result.items()}


# --------------------------------------------------------------------------
# Footprint


@dataclass
class UnitFunction:
    address: int
    name: str                # name of the existing definition, else the corpus name
    ghidra_name: str
    current_source: str
    current_line: int | None
    size: int
    declared_in: list[Decl]
    callers_outside: int
    callees_outside: int


@dataclass
class Callee:
    address: int
    name: str
    ghidra_name: str
    unit_id: str
    unit_name: str
    unit_kind: str
    current_source: str
    declared_in: list[Decl]
    status: str              # converted, c-declared, needs-c-linkage, undeclared, library, unmapped
    needs_extern_c: bool
    needs_boundary_decl: bool
    edit_headers: list[str]
    new_headers: list[str]   # boundary header to create (does not exist yet)
    called_by: list[str]     # unit function names (this unit) that call it


@dataclass
class Caller:
    address: int
    name: str
    unit_id: str
    unit_name: str
    current_source: str
    calls: list[str]         # unit function names it calls


@dataclass
class ExternalGlobal:
    address: int
    owner_unit: str          # unit id from the range table, "" when unattributed
    region: str              # rdata, data, string, bss, common
    name: str | None         # from a GLOBAL annotation at the address
    annotated_in: str | None
    declared_in: list[Decl]
    needs_extern_c: bool
    accessed_by: list[str]            # this unit's functions
    shared_with: list[str]            # other units whose functions use the same address
    literal_candidate: bool           # only its address is taken, in initialised .data
    needs_declaration: bool = False   # shared, but no header declares it: it needs an extern


@dataclass
class OwnData:
    """Data the unit's functions use that no other unit's function touches."""

    address: int
    region: str                       # data, bss or common
    size: int                         # widest access, 0 when only the address is taken
    name: str | None                  # from a GLOBAL annotation at the address


@dataclass
class Footprint:
    unit_id: str
    unit_name: str
    unit_kind: str
    stem: str
    confidence: str
    text_start: int
    text_end: int
    ranges: dict[str, tuple[int, int]]
    new_files: list[str]
    functions: list[UnitFunction]
    current_sources: list[str]
    callees: list[Callee]
    callers: list[Caller]
    caller_sources: list[str]
    external_globals: list[ExternalGlobal]
    unattributed_own_data: list[OwnData]
    test_files: list[str]
    own_declaration_headers: list[str]
    headers: list[str]       # the full set of existing headers the conversion is expected to edit
    sources: list[str]       # existing non-header files to edit
    shared_files: list[str]
    limitations: list[str]
    with_data: bool

    # -- analysis helpers
    @property
    def header_set(self) -> frozenset[str]:
        return frozenset(self.headers)

    @property
    def leaf(self) -> bool:
        """True when the unit calls no function of another convertible unit."""
        return not any(c.unit_kind in CONVERTIBLE_KINDS for c in self.callees)

    def to_dict(self) -> dict:
        def conv(o):
            if dataclasses.is_dataclass(o):
                return {k: conv(v) for k, v in dataclasses.asdict(o).items()}
            return o
        d = conv(self)

        def hexify(o):
            if isinstance(o, dict):
                out = {}
                for k, v in o.items():
                    if k in ("address", "text_start", "text_end") and isinstance(v, int):
                        v = f"0x{v:08x}"
                    elif k == "ranges":
                        v = {r: [f"0x{a:08x}", f"0x{b:08x}"] for r, (a, b) in v.items()}
                    out[k] = hexify(v)
                return out
            if isinstance(o, list):
                return [hexify(x) for x in o]
            return o
        d = hexify(d)
        d["leaf"] = self.leaf
        return d

    def to_json(self, indent: int = 2) -> str:
        return json.dumps(self.to_dict(), indent=indent)


def header_overlap(a: Footprint, b: Footprint) -> frozenset[str]:
    """Headers both conversions are expected to edit (empty: can run in parallel)."""
    return a.header_set & b.header_set


def _module_header(decl_file: str) -> str:
    """The C boundary header that mirrors a C++ header: ``x.hpp`` -> ``x.h``."""
    return str(Path(decl_file).with_suffix(".h").as_posix())


def _in_ranges(addr: int, unit: UnitRanges):
    for region, (lo, hi) in unit.ranges.items():
        if lo <= addr < hi + 4 or (lo <= addr <= hi + 8 and region in ("bss", "data")):
            return region
    return None


def owner_of(addr: int, units: list[UnitRanges]) -> tuple[str, str] | None:
    """(unit id, region) of the unit whose attributed data range holds ``addr``."""
    for u in units:
        region = _in_ranges(addr, u)
        if region:
            return u.unit_id, region
    return None


def compute_footprint(
    unit: str,
    root: Path = ROOT,
    *,
    stem: str | None = None,
    with_data: bool | None = None,
    index: SourceIndex | None = None,
    units: list[UnitRanges] | None = None,
    function_rows: list[FunctionRow] | None = None,
    edges: list[tuple[int, int]] | None = None,
    accesses: list | None = None,
) -> Footprint:
    """Compute the conversion footprint of ``unit`` (``"G98-029"``, ``"Keys.c"``).

    ``with_data`` adds the globals of other units that the unit's functions
    read or write. ``None`` (default) does it when ``reference/LegoRR.exe`` and
    the Python packages it needs are present. Pass ``accesses`` (from
    ``unit_accesses.scan_unit_accesses``) to reuse an earlier scan.
    """
    root = Path(root)
    units = units if units is not None else load_units(root)
    u = find_unit(units, unit)
    rows = function_rows if function_rows is not None else load_function_rows(root)
    edges = edges if edges is not None else load_edges(root)
    index = index if index is not None else SourceIndex(root)
    stem = stem or u.stem

    by_addr = {r.address: r for r in rows}
    mine = sorted((r for r in rows if r.unit_id == u.unit_id), key=lambda r: r.address)
    if not mine:
        raise SystemExit(f"{u.unit_id} has no functions in {UNIT_FUNCTIONS}")
    mine_addrs = {r.address for r in mine}
    ends = {r.address: (mine[i + 1].address if i + 1 < len(mine) else u.text_end)
            for i, r in enumerate(mine)}

    def name_of(addr: int) -> str:
        a = index.function_annotation(addr)
        if a and a.name:
            return a.name
        r = by_addr.get(addr)
        return r.ghidra_name if r else f"FUN_{addr:08x}"

    def kind_dir():
        return SOURCE_DIRS.get(u.unit_kind, "src/gods98")

    new_files = [f"{kind_dir()}/{stem}.c", f"{kind_dir()}/{stem}.h"]

    # Calls: unit -> other units, other units -> unit.
    callee_calls: dict[int, set[int]] = {}
    caller_calls: dict[int, set[int]] = {}
    for src, dst in edges:
        if src in mine_addrs and dst not in mine_addrs:
            callee_calls.setdefault(dst, set()).add(src)
        elif dst in mine_addrs and src not in mine_addrs:
            caller_calls.setdefault(src, set()).add(dst)

    fn_decls = lambda name: [d for d in index.functions.get(name, []) if d.file.endswith((".h", ".hpp"))]

    functions = []
    for r in mine:
        a = index.function_annotation(r.address)
        name = name_of(r.address)
        outside_callers = sum(1 for dsts in caller_calls.values() if r.address in dsts)
        outside_callees = sum(1 for srcs in callee_calls.values() if r.address in srcs)
        functions.append(UnitFunction(
            r.address, name, r.ghidra_name, r.current_source,
            a.line if a else None, ends[r.address] - r.address,
            fn_decls(name), outside_callers, outside_callees))

    current_sources = sorted({f.current_source for f in functions if f.current_source})
    own_headers = sorted({d.file for f in functions for d in f.declared_in})

    # Callees.
    callees = []
    for addr in sorted(callee_calls):
        r = by_addr.get(addr)
        name = name_of(addr)
        called_by = sorted({name_of(s) for s in callee_calls[addr]})
        if r is None:
            callees.append(Callee(addr, name, "", "", "", "", "", [], "unmapped",
                                  False, False, [], [], called_by))
            continue
        decls = fn_decls(name)
        status, need_c, need_decl = "c-declared", False, False
        edit: list[str] = []
        new: list[str] = []
        if r.unit_kind not in CONVERTIBLE_KINDS:
            status = "library"
        elif r.current_source.endswith(".c"):
            status = "converted"
        elif any(d.file.endswith(".h") for d in decls):
            status = "c-declared"
        else:
            cxx_only = [d for d in decls if not d.in_c]
            need_decl = True
            if not decls:
                status = "undeclared"
                module = Path(r.current_source).with_suffix(".h").as_posix() if r.current_source else ""
                boundary = module
            else:
                status = "needs-c-linkage" if cxx_only else "needs-boundary-decl"
                boundary = _module_header(decls[0].file)
                if cxx_only:
                    need_c = True
                    edit.extend(sorted({d.file for d in cxx_only}))
            if boundary:
                (edit if (root / boundary).exists() else new).append(boundary)
        callees.append(Callee(addr, name, r.ghidra_name, r.unit_id, r.unit_name, r.unit_kind,
                              r.current_source, decls, status, need_c, need_decl,
                              sorted(set(edit)), sorted(set(new)), called_by))

    # Callers.
    callers = []
    for addr in sorted(caller_calls):
        r = by_addr.get(addr)
        callers.append(Caller(addr, name_of(addr), r.unit_id if r else "",
                              r.unit_name if r else "", r.current_source if r else "",
                              sorted({name_of(d) for d in caller_calls[addr]})))
    caller_sources = sorted({c.current_source for c in callers if c.current_source})

    # Data of other units that the unit's functions touch, and data outside
    # the unit's attributed ranges that no other unit touches (the common
    # region is not attributed per unit by finding 505).
    ext_globals: list[ExternalGlobal] = []
    own_data: list[OwnData] = []
    if with_data is None:
        with_data = accesses is not None or _binary_available(root)
    if with_data:
        if accesses is None:
            accesses = all_accesses(root, rows)
        unit_of = {r.address: r.unit_id for r in rows}
        users: dict[int, set[str]] = {}
        for acc in accesses:
            uid = unit_of.get(acc.function, "")
            if uid and uid != u.unit_id:
                users.setdefault(acc.address, set()).add(uid)
        own = {region: (lo, hi + 8) for region, (lo, hi) in u.ranges.items()}
        mine_acc = [a for a in accesses if a.function in mine_addrs]
        seen: dict[int, ExternalGlobal] = {}
        own_seen: dict[int, OwnData] = {}
        for acc in mine_acc:
            if any(lo <= acc.address < hi for lo, hi in own.values()):
                continue
            fn = name_of(acc.function)
            owner = owner_of(acc.address, [x for x in units if x.unit_id != u.unit_id])
            shared = sorted(users.get(acc.address, ()))
            ann = index.global_annotation(acc.address)
            name = ann.name if ann else None
            if owner is None and not shared:
                o = own_seen.get(acc.address)
                if o is None:
                    own_seen[acc.address] = OwnData(acc.address, acc.region, acc.size, name)
                else:
                    o.size = max(o.size, acc.size)
                continue
            g = seen.get(acc.address)
            if g is not None:
                if fn not in g.accessed_by:
                    g.accessed_by.append(fn)
                g.literal_candidate = g.literal_candidate and acc.kind == "addr" and acc.region == "data"
                continue
            region = owner[1] if owner else acc.region
            decls = list(index.variables.get(name, [])) if name else []
            literal = acc.kind == "addr" and acc.region == "data"
            seen[acc.address] = ExternalGlobal(
                acc.address, owner[0] if owner else "", region, name,
                ann.file if ann else None, decls,
                bool(decls) and not any(d.in_c for d in decls) and not literal,
                [fn], shared, literal, needs_declaration=not decls and not literal)
        ext_globals = [seen[a] for a in sorted(seen)]
        own_data = [own_seen[a] for a in sorted(own_seen)]

    tests = index.mentions([f.name for f in functions])
    test_files = sorted({t for ts in tests.values() for t in ts})

    # The full set of existing headers the conversion is expected to edit.
    headers = set(own_headers)
    for c in callees:
        headers.update(c.edit_headers)
    for g in ext_globals:
        if g.needs_extern_c:
            headers.update(d.file for d in g.declared_in)
    headers -= set(new_files)
    sources = set(current_sources) | set(caller_sources) | set(test_files)

    return Footprint(
        unit_id=u.unit_id, unit_name=u.unit_name, unit_kind=u.unit_kind, stem=stem,
        confidence=u.confidence, text_start=u.text_start, text_end=u.text_end,
        ranges=dict(u.ranges), new_files=new_files, functions=functions,
        current_sources=current_sources, callees=callees, callers=callers,
        caller_sources=caller_sources, external_globals=ext_globals, unattributed_own_data=own_data,
        test_files=test_files, own_declaration_headers=own_headers,
        headers=sorted(headers), sources=sorted(sources),
        shared_files=list(SHARED_FILES), limitations=list(LIMITATIONS),
        with_data=bool(with_data))


_ACCESS_CACHE: dict[Path, list] = {}


def all_accesses(root: Path, rows: list[FunctionRow]) -> list:
    """Data accesses of every function in the binary (about 3 seconds, cached)."""
    root = Path(root).resolve()
    if root not in _ACCESS_CACHE:
        import unit_accesses
        binary = unit_accesses.open_binary(root)
        addrs = sorted({r.address for r in rows})
        ranges = [(a, addrs[i + 1] if i + 1 < len(addrs) else binary.text[1])
                  for i, a in enumerate(addrs)]
        _ACCESS_CACHE[root] = unit_accesses.scan_unit_accesses(ranges, root=root, binary=binary)
    return _ACCESS_CACHE[root]


def _binary_available(root: Path) -> bool:
    if not (root / "reference/LegoRR.exe").exists():
        return False
    try:
        import capstone  # noqa: F401
        import pefile  # noqa: F401
    except ImportError:
        return False
    return True


# --------------------------------------------------------------------------
# Text report


def render_text(fp: Footprint) -> str:
    out = []
    w = out.append
    name = fp.unit_name or "(unnamed)"
    w(f"Conversion footprint: {fp.unit_id} {name} [{fp.unit_kind}, boundary {fp.confidence}]")
    w(f"  text 0x{fp.text_start:08x}-0x{fp.text_end:08x}, {len(fp.functions)} functions"
      f"{', leaf (no callees in other convertible units)' if fp.leaf else ''}")
    w("  new files: " + ", ".join(fp.new_files))
    w("")
    w("Functions (current definition = the code to replace)")
    for f in fp.functions:
        where = f"{f.current_source}:{f.current_line}" if f.current_line else (f.current_source or "-")
        w(f"  0x{f.address:08x} {f.name:<44} {f.size:>5}B  {where}")
    w("")
    w("Current source files (hold the code to replace): " + (", ".join(fp.current_sources) or "-"))
    w("")
    w(f"Callees in other units ({len(fp.callees)})")
    for c in fp.callees:
        act = []
        if c.needs_extern_c:
            act.append('extern "C"')
        if c.needs_boundary_decl:
            act.append("boundary decl")
        hdr = ", ".join(f"{d.file}:{d.line}" + ("" if d.in_c else " (C++)") for d in c.declared_in) or "-"
        w(f"  0x{c.address:08x} {c.name:<40} {c.unit_id:<8} {c.status:<18} {hdr}"
          + (f"  -> {', '.join(act)}" if act else ""))
    if not fp.callees:
        w("  none")
    w("")
    w(f"Callers to migrate ({len(fp.callers)})")
    for c in fp.callers:
        w(f"  0x{c.address:08x} {c.name:<40} {c.unit_id:<8} {c.current_source}  calls {', '.join(c.calls)}")
    if not fp.callers:
        w("  none in the direct call graph")
    if fp.with_data:
        w("")
        shown = [g for g in fp.external_globals if not g.literal_candidate]
        lits = [g for g in fp.external_globals if g.literal_candidate]
        w(f"Globals of other units used by the unit ({len(shown)})")
        for g in shown:
            decl = ", ".join(f"{d.file}:{d.line}" for d in g.declared_in) or "-"
            who = g.owner_unit or ("shared " + ",".join(g.shared_with))
            w(f"  0x{g.address:08x} {g.region:<7} {who:<18} "
              f"{g.name or '-':<32} {decl}" + ('  -> extern "C"' if g.needs_extern_c else "")
              + ("  -> declare in a header" if g.needs_declaration else ""))
        if not shown:
            w("  none")
        if lits:
            w(f"String literals pooled with other units ({len(lits)}): "
              + ", ".join(f"0x{g.address:08x}" for g in lits))
        w(f"Data outside the unit's attributed ranges that only this unit uses "
          f"({len(fp.unattributed_own_data)}): scaffolded into its globals")
        for o in fp.unattributed_own_data[:8]:
            w(f"  0x{o.address:08x} {o.region:<7} {o.size}B {o.name or ''}")
        if len(fp.unattributed_own_data) > 8:
            w(f"  ... {len(fp.unattributed_own_data) - 8} more")
    w("")
    w("Test files that mention the unit's functions: " + (", ".join(fp.test_files) or "-"))
    w("")
    w("Headers the conversion is expected to edit (" + str(len(fp.headers)) + ")")
    for h in fp.headers:
        w("  " + h)
    if not fp.headers:
        w("  none")
    w("Source files to edit: " + (", ".join(fp.sources) or "-"))
    w("Always touched: " + ", ".join(fp.shared_files))
    w("")
    for item in fp.limitations:
        w("Note: " + item)
    return "\n".join(out) + "\n"


# --------------------------------------------------------------------------
# Command line


def render_table(prints: list[Footprint]) -> str:
    """A markdown summary of several footprints, with header overlaps."""
    out = ["| Unit | Functions | Leaf | Callees in other units | Callers | Own declaration headers"
           " | Headers to edit |",
           "| --- | ---: | :-: | ---: | ---: | --- | --- |"]
    for fp in prints:
        label = f"{fp.unit_id} {fp.unit_name}".strip()
        own = ", ".join(f"`{h}`" for h in fp.own_declaration_headers) or "-"
        hdr = ", ".join(f"`{h}`" for h in fp.headers) or "-"
        needs = sum(1 for c in fp.callees if c.needs_extern_c or c.needs_boundary_decl)
        out.append(f"| {label} | {len(fp.functions)} | {'yes' if fp.leaf else 'no'} | "
                   f"{len(fp.callees)} ({needs} need work) | {len(fp.callers)} | {own} | {hdr} |")
    overlaps = []
    for i, a in enumerate(prints):
        for b in prints[i + 1:]:
            shared = sorted(header_overlap(a, b))
            if shared:
                overlaps.append(f"* {a.unit_id} and {b.unit_id}: " + ", ".join(f"`{h}`" for h in shared))
    out.append("")
    out.append("Header overlaps (these pairs conflict when converted in parallel):")
    out.extend(overlaps or ["* none"])
    return "\n".join(out) + "\n"


def main(argv: list[str] | None = None) -> int:
    import argparse
    import sys

    p = argparse.ArgumentParser(description="Print the conversion footprint of one or more units.")
    p.add_argument("units", nargs="+", help="unit ids or names (G98-029, Keys.c)")
    p.add_argument("--json", action="store_true", help="JSON: one object, or a list for several units")
    p.add_argument("--table", action="store_true", help="markdown summary table with header overlaps")
    p.add_argument("--no-data", action="store_true", help="skip the data analysis (no binary needed)")
    p.add_argument("--root", type=Path, default=ROOT)
    args = p.parse_args(argv)
    index = SourceIndex(args.root)
    prints = [compute_footprint(u, args.root, index=index, with_data=False if args.no_data else None)
              for u in args.units]
    if args.json:
        docs = [fp.to_dict() for fp in prints]
        print(json.dumps(docs[0] if len(docs) == 1 else docs, indent=2))
    elif args.table:
        sys.stdout.write(render_table(prints))
    else:
        for fp in prints:
            sys.stdout.write(render_text(fp) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
