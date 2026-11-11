#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Scaffold an original-shape conversion of one translation unit (issue #405).

Step 0 of the ADR 0005 conversion recipe. For one *original translation unit*
it writes the mechanical starting state, so that the converting agent spends
its time on matching functions:

  * ``<stem>.c`` and ``<stem>.h`` in ``src/game`` or ``src/gods98`` (SPDX
    headers; C89 and C++ compatible)
  * a typed globals struct laid out to the unit's ``.bss`` range, with fields
    named by offset, plus the unit's ``.data``/``.rdata`` items and its
    unattributed (common) data, each with ``// GLOBAL: LEGORR 0x...``
  * one stub per function in original order with ``// FUNCTION: LEGORR 0x...``
    and the Ghidra pseudocode as a comment
  * the unit's entry in the link-order list ``src/link_order.cmake``, in its
    original position (ADR 0006), and the ``c_boundaries`` test includes
  * a report: the conversion footprint, a "before" score table and a recipe
    checklist for the PR description

It does not write function bodies, choose signatures, migrate callers or edit
existing implementations.

    tools/convert_unit.py G98-029 --stem registry          # scaffold in place
    tools/convert_unit.py G98-029 --dry-run                # report only, no writes
    tools/convert_unit.py G98-029 --out /tmp/scratch       # scaffold into a copy
    tools/convert_unit.py G98-029 --footprint-only --json  # footprint as JSON
    tools/convert_unit.py GAME-034 --compare               # diff vs a hand conversion

See tools/README.md. The footprint library is ``conversion_footprint.py``.
"""

from __future__ import annotations

import argparse
import json
import re
import struct
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import conversion_footprint as cf  # noqa: E402
import link_order  # noqa: E402

ROOT = cf.ROOT
LIBRARIES = {"game": "game", "gods98": "gods98"}     # unit kind -> library
LINK_ORDER = link_order.LIST
BOUNDARY_TEST = "src/game/test_c_boundaries.cpp"
STUB_PREFIX = "scaffold_"
INIT_LIMIT = 512            # largest initialised item written out, in bytes
COMMON_ARRAY_CAP = 4096
STRING_LIST_LIMIT = 40


# --------------------------------------------------------------------------
# Data layout


@dataclass
class Ref:
    """Everything the unit's code does to one data address."""

    address: int
    width: int = 0
    is_float: bool = False
    scale: int = 0
    taken: bool = False

    def add(self, acc) -> None:
        self.width = max(self.width, acc.size)
        self.is_float = self.is_float or acc.kind == "float"
        self.scale = max(self.scale, acc.scale)
        self.taken = self.taken or acc.kind == "addr"


@dataclass
class Field:
    start: int
    size: int                    # bytes occupied
    ctype: str                   # element type, or "struct" for a stride cluster
    count: int = 1               # array length; 1 means a scalar
    width: int = 0               # element width
    is_float: bool = False
    members: list[tuple[int, str]] = field(default_factory=list)   # struct cluster: (offset, ctype)
    stride: int = 0
    note: str = ""


def ctype_for(width: int, is_float: bool) -> tuple[str, int]:
    """C type for an access of ``width`` bytes and the element width it implies."""
    if is_float and width == 4:
        return "float", 4
    if is_float and width == 8:
        return "double", 8
    if width in (1, 2, 4):
        return {1: "unsigned char", 2: "unsigned short", 4: "unsigned int"}[width], width
    return "unsigned char", 1


def collect_refs(accesses, lo: int, hi: int) -> dict[int, Ref]:
    refs: dict[int, Ref] = {}
    for a in accesses:
        if lo <= a.address <= hi:
            refs.setdefault(a.address, Ref(a.address)).add(a)
    return refs


def layout(refs: dict[int, Ref], end: int, *, clusters: bool = True,
           common: bool = False, merge: bool = True) -> list[Field]:
    """Turn referenced addresses into fields up to ``end`` (exclusive).

    Each referenced address starts a field that runs to the next one. Its
    element type is the widest access seen. Indexed accesses with a stride
    larger than the access (``[ecx*8 + base]``) make a struct array. In the
    unattributed common region (``common=True``) a field is one element unless
    code indexes it, since nothing bounds a gap there.
    """
    addrs = sorted(refs)
    items: list[tuple[int, int, str, list[Ref]]] = []    # start, stride, kind, refs
    used: set[int] = set()
    if clusters:
        for a in addrs:
            r = refs[a]
            if a in used or r.scale <= max(r.width, 1) or r.scale not in (2, 4, 8):
                continue
            grp = [refs[b] for b in addrs
                   if b not in used and a <= b < a + r.scale and refs[b].scale == r.scale]
            if len(grp) > 1 or r.scale > max(r.width, 1):
                items.append((a, r.scale, "cluster", grp))
                used.update(g.address for g in grp)
    for a in addrs:
        if a not in used:
            items.append((a, 0, "plain", [refs[a]]))
    items.sort(key=lambda t: t[0])

    fields: list[Field] = []
    for i, (start, stride, kind, grp) in enumerate(items):
        nxt = items[i + 1][0] if i + 1 < len(items) else end
        span = nxt - start
        if span <= 0:
            continue
        if kind == "cluster":
            members = []
            for g in sorted(grp, key=lambda g: g.address):
                t, _ = ctype_for(g.width or 4, g.is_float)
                members.append((g.address - start, t))
            count = max(span // stride, 1)
            fields.append(Field(start, count * stride, "struct", count, stride=stride,
                                members=members, note=f"stride {stride}"))
            used_size = count * stride
        else:
            r = grp[0]
            if r.width == 0 and common:
                # Only the address is taken and nothing bounds the gap: one word.
                fields.append(Field(start, 4, "unsigned int", 1, width=4, note="address taken"))
                used_size = 4
            elif r.width == 0:
                # Only the address is taken: a byte buffer up to the next field.
                cap = span
                fields.append(Field(start, cap, "unsigned char", cap, width=1,
                                    note="address taken, element type unknown"))
                used_size = cap
            else:
                t, w = ctype_for(r.width, r.is_float)
                room = min(span, COMMON_ARRAY_CAP) if common else span
                if w > room:
                    t, w = ctype_for(room if room in (1, 2, 4) else 1, False)
                count = max(room // w, 1)
                if common and not r.scale:
                    count = 1
                if r.scale and r.scale > w and not clusters:
                    pass
                if count > 1 and not r.scale and not clusters:
                    # .rdata/.data words that nothing indexes are separate constants,
                    # even when no instruction reads the later ones (finding 510's 2.0f).
                    for k in range(count):
                        fields.append(Field(start + k * w, w, t, 1, width=w, is_float=r.is_float,
                                            note="" if k == 0 else "not read by the unit's code"))
                else:
                    fields.append(Field(start, count * w, t, count, width=w, is_float=r.is_float,
                                        note=(f"indexed, stride {r.scale}" if r.scale > w else "")))
                used_size = count * w
        gap = span - used_size
        if gap > 0 and not common:
            fields.append(Field(start + used_size, gap, "unsigned char", gap, width=1, note="padding"))
    return merge_runs(fields) if merge else fields


def merge_runs(fields: list[Field], minimum: int = 4) -> list[Field]:
    """Merge runs of adjacent scalars of one type into an array."""
    out: list[Field] = []
    i = 0
    while i < len(fields):
        f = fields[i]
        j = i
        plain = ("", "address taken")
        if f.count == 1 and f.ctype != "struct" and f.note in plain:
            while (j + 1 < len(fields) and fields[j + 1].count == 1 and fields[j + 1].ctype == f.ctype
                   and fields[j + 1].note in plain and fields[j + 1].start == fields[j].start + f.width):
                j += 1
        if j - i + 1 >= minimum:
            out.append(Field(f.start, f.width * (j - i + 1), f.ctype, j - i + 1,
                             width=f.width, is_float=f.is_float, note="adjacent scalars merged"))
        else:
            out.extend(fields[i:j + 1])
        i = j + 1
    return out


def region_end(refs: dict[int, Ref], hi: int) -> int:
    """End of a range whose upper bound is the start of its last item."""
    last = refs.get(hi)
    width = last.width if last and last.width else 4
    return hi + width


# --------------------------------------------------------------------------
# Pseudocode


CORPUS = "docs/decompiles/full/LEGORR.full.c"
CORPUS_HEADER = re.compile(r"^// ==== (\S+) @ ([0-9A-Fa-f]{8}) \[Ghidra: (\S+)\] ====$", re.M)


def load_pseudocode(root: Path, addresses: set[int]) -> dict[int, str]:
    text = (root / CORPUS).read_text(encoding="utf-8", errors="replace")
    heads = list(CORPUS_HEADER.finditer(text))
    out = {}
    for i, m in enumerate(heads):
        addr = int(m.group(2), 16)
        if addr in addresses:
            end = heads[i + 1].start() if i + 1 < len(heads) else len(text)
            out[addr] = text[m.end():end].strip("\n")
    return out


def comment_safe(text: str) -> str:
    """Make text safe inside a C block comment."""
    return text.replace("*/", "* /").replace("/*", "/ *")


# --------------------------------------------------------------------------
# Generation


def identifier(name: str) -> str:
    name = re.sub(r"\W", "_", name)
    return name if re.match(r"[A-Za-z_]", name) else "_" + name


def camel(stem: str) -> str:
    return "".join(p.capitalize() for p in stem.split("_"))


def hexrange(r) -> str:
    lo, hi = r
    return f"{lo:08x}-{hi:08x}" if hi != lo else f"{lo:08x}"


def u32_literal(v: int) -> str:
    return f"0x{v:08x}u"


def float_literal(v: float, double: bool) -> str:
    text = repr(v) if double else f"{v:.9g}"
    if "inf" in text or "nan" in text:
        return "0"
    if not re.search(r"[.eE]", text):
        text += ".0"
    return text if double else text + "f"


@dataclass
class Scaffold:
    footprint: cf.Footprint
    files: dict[str, str]                 # new files: relative path -> text
    patches: dict[str, str]               # existing files: relative path -> patched text
    wiring: "Wiring"
    stub_names: dict[int, str]
    globals_start: int | None             # first address of the bss struct
    globals_end: int | None
    struct_fields: list[Field]
    data_items: list[tuple[str, Field]]   # (region, field) for rdata/data/common
    existing_globals: list[tuple[int, str]]  # (address, file:line) of annotations already in src/
    string_literals: list[tuple[int, str]]
    shared_literals: list[tuple[int, str, str]]   # (address, text, owning unit id)
    warnings: list[str]


def build_scaffold(unit: str, root: Path = ROOT, stem: str | None = None,
                   index: cf.SourceIndex | None = None) -> Scaffold:
    root = Path(root)
    units = cf.load_units(root)
    u = cf.find_unit(units, unit)
    if u.unit_kind not in cf.CONVERTIBLE_KINDS:
        raise SystemExit(f"{u.unit_id} is a {u.unit_kind} unit; only game and gods98 units are converted")
    index = index or cf.SourceIndex(root)
    rows = cf.load_function_rows(root)
    fp = cf.compute_footprint(u.unit_id, root, stem=stem, index=index, units=units,
                              function_rows=rows, with_data=True)
    stem = fp.stem
    directory = cf.SOURCE_DIRS[u.unit_kind]
    c_path, h_path = fp.new_files
    warnings: list[str] = []
    if not u.unit_name and not stem_given(stem, u):
        warnings.append(f"{u.unit_id} has no attested name; the file stem is {stem!r}. "
                        "Pass --stem to choose a descriptive one.")
    if u.confidence != "high":
        warnings.append(f"Boundary confidence is {u.confidence}: {u.note}")

    import unit_accesses
    binary = unit_accesses.open_binary(root)
    mine = [f for f in fp.functions]
    ends = {f.address: (mine[i + 1].address if i + 1 < len(mine) else fp.text_end)
            for i, f in enumerate(mine)}
    accesses = [a for a in cf.all_accesses(root, rows) if a.function in ends]

    # -- state
    existing: list[tuple[int, str]] = []
    struct_fields: list[Field] = []
    g_start = g_end = None
    data_items: list[tuple[str, Field]] = []
    ranges = fp.ranges
    bss_refs: dict[int, Ref] = {}
    if "bss" in ranges:
        lo, hi = ranges["bss"]
        bss_refs = collect_refs(accesses, lo, hi)
        if not bss_refs:
            bss_refs = {lo: Ref(lo)}
            warnings.append("The .bss range has no direct accesses in the unit's code; "
                            "it is scaffolded as one word.")
        g_start = min(bss_refs)
        g_end = region_end(bss_refs, max(bss_refs))
        struct_fields = layout(bss_refs, g_end)
    for region in ("rdata", "data"):
        if region in ranges:
            lo, hi = ranges[region]
            refs = collect_refs(accesses, lo, hi)
            if not refs:
                refs = {lo: Ref(lo, width=4)}
                warnings.append(f"The .{region} range has no direct accesses in the unit's code.")
            end = region_end(refs, max(refs))
            for f in layout(refs, end, clusters=False, merge=False):
                data_items.append((region, f))
    common_refs = {o.address: Ref(o.address, o.size, False, 0, o.size == 0)
                   for o in fp.unattributed_own_data}
    for acc in accesses:
        r = common_refs.get(acc.address)
        if r is not None:
            r.add(acc)
    # Unattributed data inside initialised .data or .bss is also an own global.
    if common_refs:
        order = sorted(common_refs)
        end = region_end(common_refs, order[-1])
        for f in layout(common_refs, end, clusters=False, common=True):
            reg = next((o.region for o in fp.unattributed_own_data if o.address == f.start), "common")
            data_items.append((reg if reg != "bss" else "bss-extra", f))
    for _, f in data_items:
        a = index.global_annotation(f.start)
        if a:
            existing.append((f.start, f"{a.file}:{a.line}"))
    if g_start is not None:
        a = index.global_annotation(g_start)
        if a:
            existing.append((g_start, f"{a.file}:{a.line}"))

    # -- strings
    literals: list[tuple[int, str]] = []
    if "string" in ranges:
        lo, hi = ranges["string"]
        addr = lo
        while addr <= hi:
            s = binary.cstring(addr, 512)
            if s is None:
                addr += 4
                continue
            literals.append((addr, s))
            addr = (addr + len(s) + 1 + 3) & ~3

    shared: list[tuple[int, str, str]] = []
    for g in fp.external_globals:
        if g.literal_candidate:
            text = binary.cstring(g.address, 512)
            if text is not None:
                shared.append((g.address, text, g.owner_unit))

    # -- pseudocode and names
    pseudo = load_pseudocode(root, {f.address for f in mine})
    stub_names: dict[int, str] = {}
    taken: set[str] = set()
    for f in mine:
        base = identifier(f.name)
        name = STUB_PREFIX + base
        n = 2
        while name in taken:
            name = f"{STUB_PREFIX}{base}_{n}"
            n += 1
        taken.add(name)
        stub_names[f.address] = name

    files = {
        c_path: render_source(u, fp, stem, ranges, struct_fields, bss_refs, g_start, g_end,
                              data_items, literals, shared, mine, ends, stub_names, pseudo, binary),
        h_path: render_header(u, fp, stem, mine, stub_names, directory),
    }
    own_hpp = boundary_test_headers(fp)
    wiring = Wiring(u.unit_id, LIBRARIES[u.unit_kind], rel_include(c_path),
                    [rel_include(h) for h in own_hpp] + [rel_include(h_path)])
    patches = apply_wiring(wiring, root)
    return Scaffold(fp, files, patches, wiring, stub_names, g_start, g_end, struct_fields,
                    data_items, existing, literals, shared, warnings)


def boundary_test_headers(fp, root: Path = ROOT) -> list[str]:
    """The existing C++ headers that declare the unit's functions and that the
    boundary test includes. A relocation's private headers
    (tools/relocate_unit.py: ``<file>_shared.hpp``, ``<file>_for_<unit>.hpp``,
    ``<file>_from_<unit>.hpp``) are left out: only the legacy file and its
    relocated units include them."""
    from relocate_unit import is_private_header
    return [h for h in fp.own_declaration_headers
            if h.endswith(".hpp") and h not in fp.new_files and not is_private_header(root, h)]


def stem_given(stem: str, u: cf.UnitRanges) -> bool:
    return stem != u.stem


def rel_include(path: str) -> str:
    """Include path relative to ``src/`` as the tests spell it."""
    return path[len("src/"):] if path.startswith("src/") else path


def render_source(u, fp, stem, ranges, struct_fields, bss_refs, g_start, g_end,
                  data_items, literals, shared, funcs, ends, stub_names, pseudo, binary) -> str:
    label = u.unit_name or u.unit_id
    parts = [f"text {fp.text_start:08x}-{fp.text_end:08x}"]
    for region, title in (("rdata", ".rdata"), ("data", ".data"), ("string", "literals")):
        if region in ranges:
            parts.append(f"{title} {hexrange(ranges[region])}")
    if g_start is not None:
        parts.append(f".bss {g_start:08x}-{g_end:08x}")
    out = []
    w = out.append
    w("// SPDX-License-Identifier: LGPL-3.0-or-later")
    what = f"{u.unit_name} — original translation unit {u.unit_id}" if u.unit_name else \
        f"Original translation unit {u.unit_id}"
    w(f"/* {what} ({', '.join(parts)};")
    w(" * finding 505).")
    if u.note:
        w(" *")
        w(" * " + comment_safe(u.note).replace("\n", " "))
    w(" *")
    w(" * SCAFFOLD (tools/convert_unit.py, issue #405). Rebuild it in the original's")
    w(" * shape (ADR 0005): original signatures, direct reads and writes of the")
    w(" * globals below, original address order, the C89 subset. Each stub carries")
    w(f" * the original address and the Ghidra pseudocode. The stubs are named {STUB_PREFIX}*")
    w(" * so that they build beside the existing definitions. When you write one:")
    w(" * give it the final name, delete the old definition and its declaration,")
    w(" * and update the callers. Until the old definition is deleted, its")
    w(" * annotation and this one name the same address, and `make reccmp-check`")
    w(" * FAILS (the old function drops out of the report). Only the final")
    w(" * conversion PR has to pass the gate.")
    w(" * Delete this comment with the last stub. */")
    w(f'#include "{stem}.h"')
    w("")
    # state
    rdata_items = [(r, f) for r, f in data_items if r == "rdata"]
    data_only = [(r, f) for r, f in data_items if r == "data"]
    extra = [(r, f) for r, f in data_items if r not in ("rdata", "data")]
    if g_start is not None:
        w(render_struct(stem, struct_fields, g_start, g_end))
        w("")
    if any(f.ctype in ("float", "double") for _, f in rdata_items):
        w("/* x87 constants. MSVC pools float literals itself, so a literal in the code")
        w(" * may replace an item (priorities.c does); an annotated static (config.c)")
        w(" * binds the address explicitly. Keep whichever matches. */")
    for region, f in rdata_items + data_only:
        w(render_data_item(region, f, binary, fp))
    if rdata_items or data_only:
        w("")
    if g_start is not None:
        w(f"// GLOBAL: LEGORR 0x{g_start:08x}")
        w(f"static {camel(stem)}Globs {stem}_globs;")
        w("")
    if extra:
        w("/* Data outside the unit's attributed ranges that no other unit's code")
        w(" * touches (finding 505 does not attribute the common region). Plain")
        w(" * file-scope definitions: the original defines them as common symbols.")
        w(" * Names are by address; rename them. A field you find to be shared with")
        w(" * another unit needs an extern declaration in the header. */")
        for region, f in extra:
            w(render_common_item(f))
        w("")
    if literals or shared:
        w("/* String literals. Annotate each where the code uses it, as")
        w(" * `// STRING: LEGORR 0x...` on the line before the literal (utils.c). The")
        w(" * ones pooled with another object cannot be bound by position: use an")
        w(" * annotated static (config.c) or the same STRING marker. */")
        for addr, text in literals[:STRING_LIST_LIMIT]:
            w("/*   STRING: LEGORR 0x%08x  %s */" % (addr, comment_safe(json.dumps(text))))
        if len(literals) > STRING_LIST_LIMIT:
            w(f"/*   ... and {len(literals) - STRING_LIST_LIMIT} more up to 0x{literals[-1][0]:08x}"
              " (strings in LegoRR.exe; the pseudocode shows the ones the code uses) */")
        for addr, text, owner in shared:
            w("/*   STRING: LEGORR 0x%08x  %s  (pooled with %s) */"
              % (addr, comment_safe(json.dumps(text)), owner or "an earlier object"))
        w("")
    for f in funcs:
        name = stub_names[f.address]
        where = f"{f.current_source}:{f.current_line}" if f.current_line else (f.current_source or "none")
        w("/* 0x%08x %s (corpus name %s), %d bytes." % (f.address, f.name, f.ghidra_name, f.size))
        w(f" * Current definition: {where}")
        w(" *")
        w(" * Ghidra pseudocode (docs/decompiles/full/LEGORR.full.c):")
        for line in comment_safe(pseudo.get(f.address, "(not in the corpus)")).split("\n"):
            w((" * " + line).rstrip())
        w(" */")
        w(f"// FUNCTION: LEGORR 0x{f.address:08x}")
        w(f"void {name}(void)")
        w("{")
        w("    /* TODO: write the body from the pseudocode and the disassembly. */")
        w("}")
        w("")
    return "\n".join(out).rstrip("\n") + "\n"


def c_decl(ctype: str, name: str, count: int) -> str:
    return f"{ctype} {name}[{count}]" if count > 1 else f"{ctype} {name}"


def render_struct(stem: str, fields: list[Field], start: int, end: int) -> str:
    out = [f"typedef struct {camel(stem)}Globs {{"]
    for f in fields:
        off = f.start - start
        name = f"f_{off:04x}"
        comment = f"{f.start:08x}" + (f": {f.note}" if f.note else "")
        if f.ctype == "struct":
            members = " ".join(f"{t} f_{o:02x};" for o, t in f.members)
            out.append(f"    struct {{ {members} }} {name}[{f.count}];  /* {comment} */")
        else:
            out.append(f"    {c_decl(f.ctype, name, f.count)};  /* {comment} */")
    out.append(f"}} {camel(stem)}Globs;  /* {end - start} bytes */")
    return "\n".join(out)


def read_words(binary, f: Field) -> list[int | float]:
    raw = binary.read(f.start, f.size)
    fmt = {("float", 4): "<f", ("double", 8): "<d", ("unsigned int", 4): "<I",
           ("unsigned short", 2): "<H", ("unsigned char", 1): "<B"}[(f.ctype, f.width or 1)]
    n = f.width or 1
    return [struct.unpack(fmt, raw[i:i + n])[0] for i in range(0, len(raw) - n + 1, n)]


def value_literal(f: Field, v) -> str:
    if f.ctype == "float":
        return float_literal(v, False)
    if f.ctype == "double":
        return float_literal(v, True)
    if f.ctype == "unsigned int":
        return u32_literal(v)
    return f"0x{v:02x}"


def render_data_item(region: str, f: Field, binary, fp) -> str:
    name = f"s_{region}_{f.start:08x}"
    const = "const " if region == "rdata" else ""
    head = f"// GLOBAL: LEGORR 0x{f.start:08x}\n"
    if f.ctype != "struct" and f.size <= INIT_LIMIT:
        words = read_words(binary, f)
        if (region == "rdata" and f.ctype == "unsigned int" and words
                and all(fp.text_start <= v < fp.text_end for v in words)):
            return (f"/* 0x{f.start:08x}: {f.size}-byte switch table inside the unit's code, "
                    "generated by the compiler. Do not declare it. */")
        note = ""
        if f.ctype == "unsigned int" and any(0x00401000 <= v < 0x0076E000 for v in words):
            note = "  /* holds addresses: use symbols */"
        if f.count == 1:
            return f"{head}static {const}{f.ctype} {name} = {value_literal(f, words[0])};{note}"
        body = ", ".join(value_literal(f, v) for v in words)
        return f"{head}static {const}{f.ctype} {name}[{f.count}] = {{ {body} }};{note}"
    size = f.size
    return (f"/* TODO: {size} bytes of .{region} at 0x{f.start:08x} are too large to scaffold "
            f"(limit {INIT_LIMIT}); read them from reference/LegoRR.exe. */\n"
            f"{head}static {c_decl('unsigned int', name, max(size // 4, 1))};")


def render_common_item(f: Field) -> str:
    name = f"g_{f.start:08x}"
    note = f"  /* {f.note} */" if f.note else ""
    return f"// GLOBAL: LEGORR 0x{f.start:08x}\n{c_decl(f.ctype, name, f.count)};{note}"


def render_header(u, fp, stem, funcs, stub_names, directory) -> str:
    label = u.unit_name or u.unit_id
    guard = f"LEGORRR_{directory.split('/')[-1].upper()}_{stem.upper()}_H"
    out = []
    w = out.append
    w("// SPDX-License-Identifier: LGPL-3.0-or-later")
    w(f"/* {label} (original translation unit {u.unit_id}, finding 505).")
    w(" *")
    w(f" * C-compatible header (ADR 0005). {stem}.c is compiled as C in the matching")
    w(" * build and as C++ in the native port. Shared C++ headers do not include this")
    w(" * file: every include and declaration a header adds shifts MSVC's symbol")
    w(" * numbering (finding 506). src/game/test_c_boundaries.cpp checks it against")
    w(" * the C++ declarations of the same functions.")
    w(" *")
    w(f" * SCAFFOLD: the {STUB_PREFIX}* names are placeholders. Declare the final names")
    w(" * and signatures here as you write each function. */")
    w(f"#ifndef {guard}")
    w(f"#define {guard}")
    w("")
    w("#ifdef __cplusplus")
    w('extern "C" {')
    w("#endif")
    w("")
    for f in funcs:
        w(f"/* FUN_{f.address:08x} ({f.name}). */")
        w(f"void {stub_names[f.address]}(void);")
        w("")
    w("#ifdef __cplusplus")
    w("}")
    w("#endif")
    w("")
    w(f"#endif /* {guard} */")
    return "\n".join(out) + "\n"


# --------------------------------------------------------------------------
# Wiring


@dataclass
class Wiring:
    """The edits to files that already exist."""

    unit_id: str              # G98-018
    library: str              # game or gods98
    source: str               # under src/: `gods98/keys.c`
    includes: list[str]       # for test_c_boundaries.cpp, relative to src/


def apply_wiring(wiring: Wiring, root: Path, starts: dict[str, int] | None = None) -> dict[str, str]:
    """The patched text of the link-order list and the boundary test under ``root``.

    ``starts`` are the unit start addresses (default: ``root``'s unit map)."""
    if starts is None:
        starts = link_order.unit_starts(root)
    listing = wire_link_order((root / LINK_ORDER).read_text(encoding="utf-8"), wiring, starts)
    test = wire_boundary_test((root / BOUNDARY_TEST).read_text(encoding="utf-8"), wiring.includes)
    return {LINK_ORDER: listing, BOUNDARY_TEST: test}


def wire_link_order(text: str, wiring: Wiring, starts: dict[str, int]) -> str:
    """Add the unit's entry to the link-order list at its original position."""
    return link_order.insert_unit(text, wiring.unit_id, wiring.library, wiring.source, starts)


def wire_boundary_test(text: str, includes: list[str]) -> str:
    """Add ``#include`` lines after the last include of the boundary test."""
    have = set(re.findall(r'#include "([^"]+)"', text))
    new = [i for i in dict.fromkeys(includes) if i not in have]
    if not new:
        return text
    matches = list(re.finditer(r'^#include "[^"]+"\n', text, re.M))
    pos = matches[-1].end()
    return text[:pos] + "".join(f'#include "{i}"\n' for i in new) + text[pos:]


# --------------------------------------------------------------------------
# Scores


def before_scores(fp: cf.Footprint, root: Path, mode: str) -> tuple[list[dict], str]:
    """The "before" table: single-function match scores, else the baseline."""
    addrs = [f"0x{f.address:08x}" for f in fp.functions]
    note = ""
    results: dict[str, dict] = {}
    if mode == "match":
        try:
            proc = subprocess.run(
                [sys.executable, str(root / "tools/match_function.py"), *addrs, "--json", "--no-diff"],
                capture_output=True, text=True, cwd=root, timeout=900)
            start = proc.stdout.find("{")
            doc = json.loads(proc.stdout[start:]) if start >= 0 else {}
            for r in doc.get("functions", []):
                results[r["address"].lower()] = r
            if not results:
                note = "single-function match unavailable: " + (proc.stderr.strip().splitlines() or ["no output"])[-1]
                mode = "baseline"
        except (OSError, ValueError, subprocess.SubprocessError) as ex:
            note = f"single-function match unavailable: {ex}"
            mode = "baseline"
    baseline = {}
    if mode in ("baseline", "match"):
        try:
            import match_baseline
            data = match_baseline.load_baseline(root / match_baseline.DEFAULT_BASELINE)
            baseline = {a: s for a, (s, _n) in data["full"].items()}
        except Exception as ex:  # noqa: BLE001 - report and carry on
            note = (note + "; " if note else "") + f"no baseline: {ex}"
    rows = []
    for f in fp.functions:
        key = f"0x{f.address:x}"
        r = results.get(key) or results.get(f"0x{f.address:08x}")
        entry = {"address": f"0x{f.address:08x}", "name": f.name, "source": f.current_source}
        if r and "matching" in r:
            entry.update(score=r.get("percent"), baseline=r.get("baseline"), origin=r.get("origin"))
        elif r and r.get("error"):
            entry.update(score="unmapped", baseline=baseline.get(f.address), error=r["error"])
        else:
            base = baseline.get(f.address)
            entry.update(score=None, baseline=base)
        rows.append(entry)
    return rows, note


# --------------------------------------------------------------------------
# Report


CHECKLIST = (
    "Pick the unit (`{id}`, boundary confidence **{conf}**)",
    "Create `{c}` and `{h}`; add the unit to the link-order list `src/link_order.cmake` "
    "in its original position, in the `{lib}` library (scaffolded)",
    "State: globals struct laid out as the original `.bss`; `.data`/`.rdata` items; "
    "each with `// GLOBAL: LEGORR 0x…` (scaffolded; rename the fields)",
    "Functions in original address order with original signatures and direct global "
    "access; port-only accessors last",
    "C subset: `gcc -std=gnu89 -Werror=declaration-after-statement -fsyntax-only`",
    "Callees: C linkage for every callee and global ({callees} callee(s) need work, "
    "{globals} global(s)); boundary declarations; pairs in `test_c_boundaries.cpp`; "
    "VC5 LIBC names",
    "Callers and tests: switch {callers} caller(s) to the original signatures; delete the "
    "superseded definitions in {sources} and their declarations",
    "Measure and gate: `make unit-data-offsets`; review gaps, anchors, unknowns and commons; `make reccmp-check`; compare the unit's functions before and after; "
    "classify drops outside the unit under the entropy policy; "
    "`make reccmp-baseline-update`; `python3 tools/translation_units.py build`",
    "Native: the unit and every touched file compile; `c_boundaries` and the native "
    "tests pass",
)


def render_report(sc: Scaffold, scores: list[dict], score_note: str) -> str:
    fp = sc.footprint
    out = []
    w = out.append
    w(f"## Conversion scaffold: {fp.unit_id} {fp.unit_name or '(unnamed)'}")
    w("")
    w(f"Generated by `tools/convert_unit.py {fp.unit_id}` (ADR 0005, recipe step 0). "
      f"Text `{fp.text_start:08x}`–`{fp.text_end:08x}`, {len(fp.functions)} functions, "
      f"boundary confidence **{fp.confidence}**.")
    w("")
    for item in sc.warnings:
        w(f"> {item}")
    if sc.warnings:
        w("")
    w("> **The matching-build gate will fail until the old definitions are deleted.** The stubs "
      "repeat the old functions' `// FUNCTION` (and `// GLOBAL`) annotations; Reccmp keeps one "
      "per address, so the old functions drop out of the report as unmapped and their callers "
      "can fall below baseline. Delete the old definitions listed below in the same conversion "
      "PR. Only the final PR has to pass `make reccmp-check`.")
    w("")
    w("### Conversion footprint")
    w("")
    w(f"* New files: {', '.join('`%s`' % f for f in fp.new_files)}")
    w(f"* Code to replace (current sources): {', '.join('`%s`' % f for f in fp.current_sources) or '-'}")
    w(f"* Callers to migrate: {', '.join('`%s`' % f for f in fp.caller_sources) or '-'}"
      f" ({len(fp.callers)} functions)")
    w(f"* Tests that mention the unit's functions: {', '.join('`%s`' % f for f in fp.test_files) or '-'}")
    w(f"* Headers expected to be edited: {', '.join('`%s`' % f for f in fp.headers) or 'none'}")
    w(f"* Always touched: {', '.join('`%s`' % f for f in fp.shared_files)}")
    w("")
    w("| Address | Function | Bytes | Current source |")
    w("| --- | --- | ---: | --- |")
    for f in fp.functions:
        where = f"{f.current_source}:{f.current_line}" if f.current_line else (f.current_source or "-")
        w(f"| `{f.address:08x}` | `{f.name}` | {f.size} | `{where}` |")
    w("")
    w(f"Callees in other units: {len(fp.callees)}")
    if fp.callees:
        w("")
        w("| Callee | Unit | Status | Declared in | Needs |")
        w("| --- | --- | --- | --- | --- |")
        for c in fp.callees:
            needs = ", ".join(x for x, on in (('`extern "C"`', c.needs_extern_c),
                                              ("boundary declaration", c.needs_boundary_decl)) if on) or "-"
            decl = ", ".join(f"`{d.file}:{d.line}`" for d in c.declared_in) or "-"
            w(f"| `{c.name}` (`{c.address:08x}`) | {c.unit_id} | {c.status} | {decl} | {needs} |")
    shown = [g for g in fp.external_globals if not g.literal_candidate]
    if shown:
        w("")
        w("Globals owned by or shared with other units:")
        w("")
        for g in shown:
            decl = ", ".join(f"`{d.file}:{d.line}`" for d in g.declared_in) or "not declared in a header"
            who = g.owner_unit or "shared with " + ", ".join(g.shared_with)
            w(f"* `{g.address:08x}` {g.name or ''} ({g.region}, {who}): {decl}"
              + (" (needs an `extern` declaration in a header)" if g.needs_declaration else ""))
    if sc.existing_globals:
        w("")
        w("Existing `// GLOBAL` annotations at addresses the scaffold now defines "
          "(delete the old ones):")
        w("")
        for a, where in sc.existing_globals:
            w(f"* `{a:08x}` at `{where}`")
    w("")
    for item in fp.limitations:
        w(f"> {item}")
    w("")
    w("### Before: single-function match scores")
    w("")
    if score_note:
        w(f"> {score_note}")
        w("")
    w("| Address | Function | Before | Baseline |")
    w("| --- | --- | ---: | ---: |")
    for r in scores:
        w(f"| `{r['address'][2:]}` | `{r['name']}` | {r.get('score') or 'n/a'} | {r.get('baseline') or '-'} |")
    w("")
    w("### Conversion recipe (ADR 0005)")
    w("")
    w("- [x] 0. Scaffold generated with `tools/convert_unit.py`")
    lib = LIBRARIES[fp.unit_kind]
    for n, text in enumerate(CHECKLIST, 1):
        text = text.format(id=fp.unit_id, conf=fp.confidence, c=fp.new_files[0], h=fp.new_files[1],
                           lib=lib, callees=sum(1 for c in fp.callees if c.needs_extern_c or c.needs_boundary_decl),
                           globals=len(shown), callers=len(fp.callers),
                           sources=", ".join(fp.current_sources) or "-")
        w(f"- [{'x' if n in (1, 2, 3) else ' '}] {n}. {text}")
    return "\n".join(out) + "\n"


# --------------------------------------------------------------------------
# Comparison with a hand conversion


def _addresses(text: str, kind: str) -> list[int]:
    return [int(m.group(1), 16) for m in
            re.finditer(r"^\s*//\s*" + kind + r":\s+LEGORR\s+0x([0-9a-fA-F]+)", text, re.M)]


def _struct_starts(text: str) -> list[int]:
    """Field start addresses from the `/* 00500f00 */` comments of the first globals struct."""
    m = re.search(r"typedef struct \w*Globs \{(.*?)\n\} \w+;", text, re.S)
    if not m:
        return []
    return [int(x, 16) for x in re.findall(r"/\*\s*([0-9a-f]{8})\b", m.group(1))]


def compare_with_hand(sc: Scaffold, root: Path = ROOT, hand_c: str | None = None,
                      hand_h: str | None = None) -> dict:
    """Compare the scaffold's mechanical output with a hand-converted unit."""
    fp = sc.footprint
    hand_c = hand_c or fp.new_files[0]
    hand_h = hand_h or fp.new_files[1]
    text = (root / hand_c).read_text(encoding="utf-8")
    scaffold_c = sc.files[fp.new_files[0]]
    result: dict = {"unit": fp.unit_id, "hand_c": hand_c, "hand_h": hand_h, "differences": []}
    diff = result["differences"]

    def note(kind, detail):
        diff.append({"kind": kind, "detail": detail})

    hand_fn = _addresses(text, "FUNCTION") + _addresses(text, "LIBRARY") + _addresses(text, "STUB")
    tool_fn = _addresses(scaffold_c, "FUNCTION")
    result["functions"] = {"hand": len(hand_fn), "tool": len(tool_fn), "same_order": hand_fn == tool_fn}
    if hand_fn != tool_fn:
        note("functions", f"hand {[hex(a) for a in hand_fn]} != tool {[hex(a) for a in tool_fn]}")

    hand_g = _addresses(text, "GLOBAL")
    tool_g = _addresses(scaffold_c, "GLOBAL")
    result["globals"] = {"hand": [f"{a:08x}" for a in hand_g], "tool": [f"{a:08x}" for a in tool_g]}
    covered = ({a for a, _, _ in sc.shared_literals} |
               {g.address for g in fp.external_globals})
    result["globals"]["covered_by_tool_as_shared"] = [f"{a:08x}" for a in hand_g
                                                      if a not in tool_g and a in covered]
    only_hand = [a for a in hand_g if a not in tool_g and a not in covered]
    only_tool = [a for a in tool_g if a not in hand_g]
    if only_hand:
        note("globals-only-in-hand", [f"{a:08x}" for a in only_hand])
    if only_tool:
        note("globals-only-in-tool", [f"{a:08x}" for a in only_tool])
    if [a for a in hand_g if a in tool_g] != [a for a in tool_g if a in hand_g]:
        note("globals-order", "shared GLOBAL annotations are in a different order")

    hand_s = _addresses(text, "STRING")
    tool_s = [a for a, _ in sc.string_literals] + [a for a, _, _ in sc.shared_literals]
    result["strings"] = {"hand": [f"{a:08x}" for a in hand_s], "tool": [f"{a:08x}" for a in tool_s]}
    if hand_s or tool_s:
        miss = [a for a in hand_s if a not in tool_s]
        extra = [a for a in tool_s if a not in hand_s]
        if miss:
            note("strings-only-in-hand (shared with an earlier object)", [f"{a:08x}" for a in miss])
        if extra:
            note("strings-only-in-tool", [f"{a:08x}" for a in extra])

    hs, ts = _struct_starts(text), _struct_starts(scaffold_c)
    result["struct"] = {"hand_fields": len(hs), "tool_fields": len(ts),
                        "hand_start": f"{hs[0]:08x}" if hs else None,
                        "tool_start": f"{ts[0]:08x}" if ts else None}
    if hs or ts:
        if hs and ts and hs[0] != ts[0]:
            note("struct-start", f"hand {hs[0]:08x}, tool {ts[0]:08x}")
        missing = [a for a in hs if a not in ts]
        extra = [a for a in ts if a not in hs]
        if missing:
            note("struct-fields-only-in-hand", [f"{a:08x}" for a in missing])
        # A tool field that starts inside a hand field splits an aggregate or
        # an array (the tool sees constant-index accesses and typed members
        # such as Area2F as separate words). The layout is the same; report it.
        result["struct"]["split_inside_hand_fields"] = [f"{a:08x}" for a in extra if hs and a > hs[0]]
        result["struct"]["tool_end"] = f"{sc.globals_end:08x}" if sc.globals_end else None

    # Link-order wiring: drop the unit's entry from the committed list, rewire,
    # compare (same position, kind, library and file).
    listing = (root / LINK_ORDER).read_text(encoding="utf-8")
    wiring = Wiring(fp.unit_id, LIBRARIES[fp.unit_kind], rel_include(hand_c), [])
    kept = [line for line in listing.splitlines(keepends=True)
            if not any(e.source == wiring.source for e in link_order.parse(line))]
    try:
        rewired = wire_link_order("".join(kept), wiring, link_order.unit_starts(root))
        ok = link_order.parse(rewired) == link_order.parse(listing)
    except ValueError:
        ok = False
    result["link_order_wiring_matches"] = ok
    if not ok:
        note("link-order", "the unit's entry in src/link_order.cmake differs in position, "
                           "kind, library or file")

    # Boundary-test entries: the tool's includes must be in the committed test.
    test = (root / BOUNDARY_TEST).read_text(encoding="utf-8")
    have = set(re.findall(r'#include "([^"]+)"', test))
    own_hpp = boundary_test_headers(fp, root)
    want = [rel_include(h) for h in own_hpp] + [rel_include(fp.new_files[1])]
    missing = [i for i in want if i not in have]
    result["boundary_includes"] = {"tool": want, "missing_from_committed_test": missing}
    if missing:
        note("c_boundaries-includes-only-in-tool", missing)

    # Header: the committed header exists and declares the unit's functions.
    hand_header = (root / hand_h).read_text(encoding="utf-8") if (root / hand_h).exists() else ""
    declared = [f.name for f in fp.functions if re.search(r"\b" + re.escape(f.name) + r"\s*\(", hand_header)]
    result["header_declares"] = {"unit_functions": len(fp.functions), "declared": len(declared)}
    if len(declared) != len(fp.functions):
        note("header", "functions not declared in the hand header: " +
             ", ".join(f.name for f in fp.functions if f.name not in declared))
    return result


# --------------------------------------------------------------------------
# CLI


def write_files(sc: Scaffold, target: Path, force: bool) -> list[str]:
    written = []
    for rel, text in sc.files.items():
        path = target / rel
        if path.exists() and not force:
            raise SystemExit(f"{rel} exists; pass --force to overwrite it")
    for rel, text in sc.files.items():
        path = target / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
        written.append(rel)
    for rel, text in sc.patches.items():
        path = target / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
        written.append(rel)
    return written


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description=__doc__.split("\n\n")[0],
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("unit", help="unit id (G98-029, GAME-003) or name (Keys.c)")
    p.add_argument("--stem", help="file stem (default: the unit name, else the lower-case id)")
    p.add_argument("--root", type=Path, default=ROOT, help="repository root (default: this checkout)")
    mode = p.add_mutually_exclusive_group()
    mode.add_argument("--dry-run", action="store_true", help="write nothing; print the report")
    mode.add_argument("--out", type=Path, metavar="DIR",
                      help="write the new files and patched link_order.cmake/test_c_boundaries.cpp "
                           "under DIR (a scratch copy), not into the checkout")
    mode.add_argument("--footprint-only", action="store_true", help="print the conversion footprint only")
    mode.add_argument("--compare", action="store_true",
                      help="compare the scaffold with the already hand-converted unit in the checkout")
    p.add_argument("--json", action="store_true", help="machine-readable output")
    p.add_argument("--force", action="store_true", help="overwrite existing new files")
    p.add_argument("--scores", choices=("match", "baseline", "none"), default="match",
                   help="'before' table source: single-function match (default; falls back to the "
                        "committed baseline) or the baseline only")
    p.add_argument("--report", type=Path, help="also write the markdown report to this file")
    args = p.parse_args(argv)
    root = args.root.resolve()

    if args.footprint_only:
        fp = cf.compute_footprint(args.unit, root, stem=args.stem)
        sys.stdout.write(fp.to_json() + "\n" if args.json else cf.render_text(fp))
        return 0

    sc = build_scaffold(args.unit, root, stem=args.stem)
    if args.compare:
        res = compare_with_hand(sc, root)
        print(json.dumps(res, indent=2))
        return 0 if not res["differences"] else 1

    scores, note = ([], "") if args.scores == "none" else before_scores(sc.footprint, root, args.scores)
    report = render_report(sc, scores, note)
    written: list[str] = []
    if not args.dry_run:
        target = args.out.resolve() if args.out else root
        if args.out:
            # A scratch directory accumulates units: patch its own copies.
            for rel in (LINK_ORDER, BOUNDARY_TEST):
                dst = target / rel
                if not dst.exists():
                    dst.parent.mkdir(parents=True, exist_ok=True)
                    dst.write_text((root / rel).read_text(encoding="utf-8"), encoding="utf-8")
            sc.patches = apply_wiring(sc.wiring, target, link_order.unit_starts(root))
        written = write_files(sc, target, args.force)
    if args.report:
        args.report.write_text(report, encoding="utf-8")
    if args.json:
        doc = {"footprint": sc.footprint.to_dict(), "written": written, "scores": scores,
               "warnings": sc.warnings, "stub_names": {f"0x{a:08x}": n for a, n in sc.stub_names.items()},
               "existing_globals": [{"address": f"0x{a:08x}", "at": w} for a, w in sc.existing_globals]}
        print(json.dumps(doc, indent=2))
    else:
        sys.stdout.write(report)
        if written:
            print("\nWrote: " + ", ".join(written))
    return 0


if __name__ == "__main__":
    sys.exit(main())
