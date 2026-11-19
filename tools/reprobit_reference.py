#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Reference objects and grind plans for ReproBit's discovery (docs/building.md).

`rbit discover grind` searches donor compiles of one function's unit and
keeps a donor whose body equals a *reference object*'s. That object must be
a COFF compile of the unit: the original only exists as LegoRR.exe, and
relocated fields cannot be qualified from bare bytes. This tool finds one.

    python3 tools/reprobit_reference.py ADDR [ADDR ...] [--pads 0 128]
        [--counts FIRST LAST] [--jobs 8] [--out reference/grind] [--json]

For each N in --pads it compiles the unit from a scratch copy with N inert
`extern int` pads after its first #include (phase_probe.py's pads, finding
664), with the matching build's flags. The first compile whose function body
equals the original's bytes, outside relocated fields, is written unchanged
to OUT/<addr>.obj. Next to it, OUT/<addr>.json is a grind plan for that
function: an `extern_runs` search over --counts declarations (default: 64
lengths centred on the pad count) after the
unit's final include, or at its start when an include follows the function
(a different seat from the pads, so the counts differ).
Run it with

    rbit discover grind . --expert-plan OUT/<addr>.json [--accept-progress]

OUT must lie under reference/, which is never committed: the object is a
compile whose bytes equal the original's. The unit and its translation unit
come from the annotations and reprobit/build-plan.json. A function no pad
count reproduces gets no files, nor does one the unpadded unit already
compiles to (its remaining difference is elsewhere). Needs the MSVC 5.0
toolchain and reference/LegoRR.exe; never run it while a build compiles the
tree.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import ia32_fields  # noqa: E402
import msvc_coff  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
BUILD_PLAN = "reprobit/build-plan.json"
# IMAGE_REL_I386_SECTION fixes up a 2-byte section index; the other i386
# relocations used by MSVC 5.0 objects patch 4 bytes.
_REL_I386_SECTION = 0x000A


class ReferenceError(Exception):
    pass


# ---------------------------------------------------------------------------
# Pure helpers


def function_body(coff: msvc_coff.CoffObject, symbol: str) -> tuple[bytes, list[tuple[int, int]]] | None:
    """The function's COMDAT body and its relocated spans (body-relative).

    With /Gy each function has a section of its own, so the body runs from
    the symbol to the section's end, alignment fill included: grind compares
    the whole COMDAT body.
    """
    matches = [s for s in coff.symbols.values()
               if s.name == symbol and s.section > 0 and s.is_function]
    if len(matches) != 1:
        return None
    sym = matches[0]
    sect = coff.section(sym.section)
    body = sect.data[sym.value:sect.size]
    spans = []
    for reloc in sect.relocations:
        start = reloc.offset - sym.value
        width = 2 if reloc.type == _REL_I386_SECTION else 4
        if 0 <= start and start + width <= len(body):
            spans.append((start, start + width))
    return body, sorted(spans)


def masked(data: bytes, spans: list[tuple[int, int]]) -> bytes:
    out = bytearray(data)
    for start, end in spans:
        out[start:end] = bytes(end - start)
    return bytes(out)


def body_matches(body: bytes, spans: list[tuple[int, int]], original: bytes) -> bool:
    """Whether a compiled body equals the original's outside relocated fields."""
    return len(original) == len(body) and masked(body, spans) == masked(original, spans)


def image_ranges(exe: bytes) -> list[tuple[int, int]]:
    """The original image's sections as virtual-address ranges [start, end)."""
    pe = struct.unpack_from("<I", exe, 0x3C)[0]
    base = struct.unpack_from("<I", exe, pe + 0x34)[0]
    count = struct.unpack_from("<H", exe, pe + 6)[0]
    offset = pe + 24 + struct.unpack_from("<H", exe, pe + 20)[0]
    ranges = []
    for _ in range(count):
        vsize, va, rsize, _raw = struct.unpack_from("<IIII", exe, offset + 8)
        ranges.append((base + va, base + va + max(vsize, rsize)))
        offset += 40
    return ranges


def known_addresses(build: Path) -> dict[str, int]:
    """Original addresses of the matching build's functions Reccmp pairs, by COFF name.

    Read from ``build``'s Reccmp report and link map; empty when either is
    missing. They pin a synthetic reference's calls where alignment alone
    cannot tell two calls apart.
    """
    import re

    results = build / "reccmp-report" / "results.json"
    link_map = build / "LegoRR.map"
    if not results.is_file() or not link_map.is_file():
        return {}
    pairs = {int(e["recomp"], 16): int(e["address"], 16)
             for e in json.loads(results.read_text())["data"] if e.get("recomp")}
    known: dict[str, int] = {}
    for line in link_map.read_text(errors="replace").splitlines():
        match = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\b", line)
        if match and int(match.group(2), 16) in pairs:
            known.setdefault(match.group(1), pairs[int(match.group(2), 16)])
    return known


class SyntheticRefusal(ReferenceError):
    """Why no synthetic reference can be written for a function."""


_DIR32 = msvc_coff.IMAGE_REL_I386_DIR32
_REL32 = msvc_coff.IMAGE_REL_I386_REL32
_MASK = 0xFFFFFFFF


@dataclass(frozen=True)
class _SeedReloc:
    entry: int  # index in the section's relocation table
    offset: int  # body-relative field offset
    symbol: int
    type: int
    addend: int  # the field's contents
    internal: int | None  # body-relative target when it points into the function's own section

    @property
    def target(self) -> tuple[int, int]:
        """The symbol plus addend an external relocation denotes, whatever its type."""
        return self.symbol, self.addend

    @property
    def key(self) -> tuple:
        """What the reference's relocation must reproduce: an internal target's
        addend follows the original's layout, an external one is the seed's."""
        if self.internal is not None:
            return ("internal", self.symbol, self.type)
        return ("external", self.symbol, self.addend, self.type)


@dataclass(frozen=True)
class _OriginalField:
    offset: int  # body-relative
    type: int  # the relocation type it needs
    value: int  # the field's contents in the original
    resolved: int  # the address it denotes (symbol plus addend)
    internal: bool  # it denotes an address inside the function


def _u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def _decode_code(body: bytes, internal_target, context: str) -> tuple[list, int]:
    """The function's instructions and where its code ends.

    Decoding stops at the first internal data an instruction addresses (a
    switch's jump or index table, which MSVC places after the code), or at
    the body's end.
    """
    code_end = len(body)
    out = []
    cursor = 0
    while cursor < code_end:
        try:
            ins = ia32_fields.decode(body, cursor)
        except ia32_fields.DecodeError as exc:
            try:
                ins = ia32_fields.decode(body + bytes(15), cursor)
            except ia32_fields.DecodeError:
                raise SyntheticRefusal(f"{context}: {exc}") from None
        if ins.end > len(body):
            raise SyntheticRefusal(f"{context}: the instruction at {cursor} runs past the body's end "
                                   "(the functions differ in length)")
        if ins.end > code_end:
            raise SyntheticRefusal(f"{context}: instruction at {cursor} runs into the data at {code_end}")
        out.append(ins)
        for fld in ins.fields:
            if fld.kind != "rel32":
                target = internal_target(fld.offset)
                if target is not None and ins.end <= target < code_end:
                    code_end = target
        cursor = ins.end
    return out, code_end


def _optimal_partners(a: list[int], b: list[int],
                      wanted: list[int]) -> tuple[dict[int, list[int]], list[tuple[int, int]]]:
    """For each index in ``wanted``, the ``b`` indices it is paired with in some
    longest common subsequence of ``a`` and ``b``; and the pairs of one such
    subsequence."""
    from array import array

    n, m = len(a), len(b)
    forward = [array("I", bytes(4 * (m + 1)))]
    for i in range(n):
        prev, cur, ai = forward[i], array("I", bytes(4 * (m + 1))), a[i]
        for j in range(m):
            if ai == b[j]:
                cur[j + 1] = prev[j] + 1
            else:
                up, left = prev[j + 1], cur[j]
                cur[j + 1] = up if up > left else left
        forward.append(cur)
    backward = [array("I", bytes(4 * (m + 1))) for _ in range(n + 1)]
    for i in range(n - 1, -1, -1):
        nxt, cur, ai = backward[i + 1], backward[i], a[i]
        for j in range(m - 1, -1, -1):
            if ai == b[j]:
                cur[j] = nxt[j + 1] + 1
            else:
                down, right = nxt[j], cur[j + 1]
                cur[j] = down if down > right else right
    total = forward[n][m]
    where: dict[int, list[int]] = {}
    for j, token in enumerate(b):
        where.setdefault(token, []).append(j)
    partners = {i: [j for j in where.get(a[i], [])
                    if forward[i][j] + 1 + backward[i + 1][j + 1] == total] for i in wanted}
    pairs = []
    i, j = n, m
    while i and j:
        if a[i - 1] == b[j - 1] and forward[i][j] == forward[i - 1][j - 1] + 1:
            pairs.append((i - 1, j - 1))
            i, j = i - 1, j - 1
        elif forward[i - 1][j] >= forward[i][j - 1]:
            i -= 1
        else:
            j -= 1
    return partners, pairs[::-1]


def explain_synthetic_reference(data: bytes, symbol: str, original: bytes, *, address: int,
                                ranges: list[tuple[int, int]],
                                known: dict[str, int] | None = None) -> tuple[bytes | None, str]:
    """A synthetic reference object for ``symbol``, or None, and why.

    The reference is the compile ``data`` (the seed) with ``symbol``'s body
    replaced by ``original``, the original's bytes at virtual address
    ``address``; ``ranges`` are the original image's sections. Its
    relocations sit at the original's own address fields:

    - every ModRM disp32, moffs32 and imm32 whose value lies in ``ranges``,
      every rel32 branching out of the function, and every jump-table entry;
    - each takes the target symbol of the seed relocation it aligns with:
      the instruction shapes (``ia32_fields``) of both bodies are aligned by
      longest common subsequence, and a field's candidates are the seed
      relocations at the same operand of the instructions it pairs with in
      some longest alignment (all same-shape instructions if none);
    - candidates are narrowed until each field has one: a symbol resolves to
      one address (address minus addend) and an address to one symbol plus
      addend. The seed's relocations and the original's fields must then
      correspond one to one.

    An external target keeps the seed's addend; an internal one (a jump
    table, a case label) takes the original's offset in the function. When
    both bodies have the same layout this is the seed's relocations
    unchanged, so the object equals the seed with the original's bytes
    outside its relocated fields. None when the bodies differ in length,
    already agree, or any step is ambiguous or inconsistent.
    """
    def refuse(reason: str) -> tuple[None, str]:
        return None, reason

    coff = msvc_coff.parse_coff(data, "synthetic reference")
    matches = [s for s in coff.symbols.values()
               if s.name == symbol and s.section > 0 and s.is_function]
    if len(matches) != 1:
        return refuse(f"{symbol} is not one function of the seed")
    sym = matches[0]
    sect = coff.section(sym.section)
    if sect.characteristics & msvc_coff.IMAGE_SCN_LNK_NRELOC_OVFL:
        return refuse("the seed's section has an overflowed relocation count")
    body = sect.data[sym.value:sect.size]
    if len(original) != len(body):
        return refuse("the original and the seed differ in length")
    length = len(body)
    seed_relocs: dict[int, _SeedReloc] = {}
    for entry, reloc in enumerate(sect.relocations):
        start = reloc.offset - sym.value
        if not 0 <= start < length:
            continue
        if reloc.type not in (_DIR32, _REL32) or start + 4 > length:
            return refuse(f"seed relocation type {reloc.type:#x} at {start}")
        target = coff.symbols.get(reloc.symbol)
        if target is None:
            return refuse(f"seed relocation at {start} names no symbol")
        addend = _u32(body, start)
        internal = None
        if target.section == sym.section and not (reloc.type == _REL32 and target.index == sym.index):
            if reloc.type != _DIR32:
                return refuse(f"seed rel32 relocation at {start} into its own section")
            internal = (target.value + addend - sym.value) & _MASK
            if internal >= length:
                return refuse(f"seed relocation at {start} points outside the function")
        seed_relocs[start] = _SeedReloc(entry, start, reloc.symbol, reloc.type, addend, internal)

    legacy = bytearray(original)
    for start in seed_relocs:
        legacy[start:start + 4] = body[start:start + 4]
    if bytes(legacy) == body:
        return refuse("the original's body already equals the seed's outside relocated fields")

    def seed_internal(offset: int) -> int | None:
        reloc = seed_relocs.get(offset)
        return None if reloc is None else reloc.internal

    def original_internal(offset: int) -> int | None:
        value = _u32(original, offset)
        return value - address if address <= value < address + length else None

    try:
        seed_code, seed_end = _decode_code(body, seed_internal, "seed")
        orig_code, orig_end = _decode_code(original, original_internal, "original")
    except SyntheticRefusal as exc:
        return refuse(str(exc))

    # The seed's address fields are exactly its relocations.
    seed_tokens, seed_fields = [], []
    placed = set()
    for ins in seed_code:
        flags = []
        for fld in ins.fields:
            reloc = seed_relocs.get(fld.offset)
            if reloc is not None:
                if (reloc.type == _REL32) != (fld.kind == "rel32"):
                    return refuse(f"seed relocation at {fld.offset} has the wrong type for its operand")
                placed.add(fld.offset)
            flags.append(reloc is not None)
        seed_tokens.append(ins.shape + (tuple(flags),))
        seed_fields.append([seed_relocs.get(fld.offset) for fld in ins.fields])
    data_relocs = [r for r in seed_relocs.values() if r.offset >= seed_end]
    for reloc in seed_relocs.values():
        if reloc.offset < seed_end and reloc.offset not in placed:
            return refuse(f"seed relocation at {reloc.offset} is not an operand field")
        if reloc.offset >= seed_end and reloc.internal is None:
            return refuse(f"seed relocation at {reloc.offset} in its table data is not internal")

    # The original's address fields.
    in_image = lambda value: any(lo <= value < hi for lo, hi in ranges)  # noqa: E731
    orig_tokens, orig_fields = [], []
    for ins in orig_code:
        flags, here = [], []
        for fld in ins.fields:
            value = _u32(original, fld.offset)
            item = None
            if fld.kind == "rel32":
                target = fld.offset + 4 + (value - (1 << 32) if value & 0x80000000 else value)
                recursion = target == 0 and original[fld.offset - 1] == 0xE8
                if not 0 <= target < length or recursion:
                    item = _OriginalField(fld.offset, _REL32, value,
                                          (address + fld.offset + 4 + value) & _MASK, False)
            elif in_image(value):
                item = _OriginalField(fld.offset, _DIR32, value, value,
                                      address <= value < address + length)
            flags.append(item is not None)
            here.append(item)
        orig_tokens.append(ins.shape + (tuple(flags),))
        orig_fields.append(here)

    intern: dict[tuple, int] = {}
    a = [intern.setdefault(t, len(intern)) for t in orig_tokens]
    b = [intern.setdefault(t, len(intern)) for t in seed_tokens]
    wanted = [i for i, here in enumerate(orig_fields) if any(here)]
    partners, pairs = _optimal_partners(a, b, wanted)
    # The seed instructions between the aligned pairs around each original
    # instruction no longest alignment pairs (its gap).
    bounds = [(-1, -1)] + pairs + [(len(a), len(b))]

    def gap(i: int) -> range:
        for (left_i, left_j), (right_i, right_j) in zip(bounds, bounds[1:]):
            if left_i < i < right_i:
                return range(left_j + 1, right_j)
        return range(0)

    code_relocs = [r for r in seed_relocs.values() if r.offset < seed_end]

    def known_at(reloc: _SeedReloc) -> int | None:
        """The original address ``known`` gives an external relocation, if any."""
        if not known or reloc.internal is not None:
            return None
        place = known.get(coff.symbols[reloc.symbol].name)
        return None if place is None else (place + reloc.addend) & _MASK

    def pinned(field: _OriginalField, keyed: dict[tuple, _SeedReloc]) -> dict[tuple, _SeedReloc]:
        """A seed relocation whose known address is the field's wins over
        alignment; one whose known address is another field's cannot be it."""
        exact = {r.key: r for r in code_relocs
                 if compatible(field, r) and known_at(r) == field.resolved}
        return exact or {key: r for key, r in keyed.items()
                         if known_at(r) in (None, field.resolved)}

    def compatible(field: _OriginalField, reloc: _SeedReloc | None) -> bool:
        return (reloc is not None and reloc.type == field.type
                and (reloc.internal is not None) == field.internal)

    fields: dict[int, _OriginalField] = {}
    candidates: dict[int, dict[tuple, _SeedReloc]] = {}
    for i in wanted:
        for k, field in enumerate(orig_fields[i]):
            if field is None:
                continue
            if partners[i]:  # the same operand of each instruction it aligns with
                pool = [seed_fields[j][k] for j in partners[i]]
            else:  # an operand of the same kind in its gap
                pool = [seed_fields[j][q] for j in gap(i)
                        for q, other in enumerate(seed_code[j].fields)
                        if other.kind == orig_code[i].fields[k].kind]
            keyed = {r.key: r for r in pool if compatible(field, r)}
            if not keyed:  # the same operand of any instruction of its shape
                keyed = {r.key: r for r in (seed_fields[j][k] for j, token in enumerate(b)
                                            if token == a[i]) if compatible(field, r)}
            fields[field.offset] = field
            candidates[field.offset] = pinned(field, keyed)
    # Table entries keep their place after the code.
    for reloc in data_relocs:
        offset = reloc.offset - seed_end + orig_end
        if offset + 4 > length:
            return refuse(f"the seed's table entry at {reloc.offset} falls past the original's end "
                          "(the functions differ in length)")
        value = _u32(original, offset)
        if not address <= value < address + length:
            return refuse(f"the original's table entry at {offset} points outside the function")
        fields[offset] = _OriginalField(offset, _DIR32, value, value, True)
        candidates[offset] = {reloc.key: reloc}

    def base(field: _OriginalField, reloc: _SeedReloc) -> int:
        return (field.resolved - reloc.addend) & _MASK

    while True:
        bases: dict[int, int] = {}
        claims: dict[int, tuple] = {}
        first: dict = {}  # the field that set each base or claim
        for offset, keyed in sorted(candidates.items()):
            if len(keyed) != 1:
                continue
            (key, reloc), = keyed.items()
            if reloc.internal is not None:
                continue
            field = fields[offset]
            if bases.setdefault(reloc.symbol, base(field, reloc)) != base(field, reloc):
                return refuse(f"the original's fields at {first[reloc.symbol]} and {offset} give "
                              f"{coff.symbols[reloc.symbol].name} two addresses")
            if claims.setdefault(field.resolved, reloc.target) != reloc.target:
                return refuse(f"the original's fields at {first[field.resolved, 0]} and {offset} "
                              f"(address {field.resolved:#x}) have two seed targets")
            first.setdefault(reloc.symbol, offset)
            first.setdefault((field.resolved, 0), offset)
        changed = False
        for offset, keyed in candidates.items():
            if len(keyed) < 2:
                continue
            field = fields[offset]
            kept = {key: reloc for key, reloc in keyed.items()
                    if reloc.internal is not None
                    or (bases.get(reloc.symbol, base(field, reloc)) == base(field, reloc)
                        and claims.get(field.resolved, reloc.target) == reloc.target)}
            if len(kept) != len(keyed):
                candidates[offset] = kept
                changed = True
        if not changed:
            break
    for offset in sorted(candidates):
        if not candidates[offset]:
            return refuse(f"the original's field at {offset} has no aligned seed relocation")
        if len(candidates[offset]) > 1:
            return refuse(f"the original's field at {offset} aligns with seed relocations to different targets")

    # One to one: each seed relocation moves to an original field with its key.
    by_key: dict[tuple, list[int]] = {}
    for offset in sorted(candidates):
        by_key.setdefault(next(iter(candidates[offset])), []).append(offset)
    seed_by_key: dict[tuple, list[_SeedReloc]] = {}
    for reloc in sorted(seed_relocs.values(), key=lambda r: r.offset):
        seed_by_key.setdefault(reloc.key, []).append(reloc)
    for key in set(by_key) | set(seed_by_key):
        here, there = len(by_key.get(key, [])), len(seed_by_key.get(key, []))
        if here != there:
            name = coff.symbols[key[1]].name
            return refuse(f"the original has {here} fields for {name} and the seed {there} relocations")

    new_body = bytearray(original)
    moved = []  # (new body offset, seed relocation)
    for key, offsets in by_key.items():
        for offset, reloc in zip(offsets, seed_by_key[key]):
            if reloc.internal is not None:
                target = coff.symbols[reloc.symbol].value
                addend = (fields[offset].value - address + sym.value - target) & _MASK
            else:
                addend = reloc.addend
            struct.pack_into("<I", new_body, offset, addend)
            moved.append((offset, reloc))
    moved.sort(key=lambda item: item[0])
    if bytes(new_body) == body and all(offset == reloc.offset for offset, reloc in moved):
        return refuse("the original's body already equals the seed's outside relocated fields")

    optional_size = struct.unpack_from("<H", data, 16)[0]
    header = 20 + optional_size + (sym.section - 1) * 40
    raw = struct.unpack_from("<I", data, header + 20)[0] + sym.value
    relptr = struct.unpack_from("<I", data, header + 24)[0]
    out = bytearray(data)
    out[raw:raw + length] = new_body
    slots = sorted(reloc.entry for _offset, reloc in moved)
    for slot, (offset, reloc) in zip(slots, moved):
        struct.pack_into("<IIH", out, relptr + slot * 10, sym.value + offset, reloc.symbol, reloc.type)
    return bytes(out), "written"


def synthetic_reference(data: bytes, symbol: str, original: bytes, *, address: int,
                        ranges: list[tuple[int, int]],
                        known: dict[str, int] | None = None) -> bytes | None:
    """``explain_synthetic_reference`` without the reason."""
    return explain_synthetic_reference(data, symbol, original, address=address, ranges=ranges,
                                       known=known)[0]


def translation_unit(build_plan: dict, source: str) -> dict:
    units = [u for u in build_plan.get("translation_units", []) if u.get("source") == source]
    if len(units) != 1:
        raise ReferenceError(f"{BUILD_PLAN} has {len(units)} translation units for {source}")
    return units[0]


def run_placement(source_text: str, function_line: int) -> str:
    """Where the grind's extern run must sit to precede the function.

    ReproBit seats it after the unit's final #include by default; a unit that
    includes again after the function (terrain_map.c, frontend_unit.c) needs
    it at the unit's start (``prefix``).
    """
    includes = [number for number, line in enumerate(source_text.splitlines(), start=1)
                if line.startswith("#include")]
    return "prefix" if includes and includes[-1] > function_line else "after_includes"


def count_window(pads: int, width: int = 64) -> tuple[int, int]:
    """The extern-run lengths a grind plan searches for a reference found at `pads`.

    The run sits at a different seat from the pads, so its length differs a
    little; a window of `width` lengths (grind's limit) centred on the pad
    count, starting at 1 at the least.
    """
    start = max(1, pads - (width // 2 - 1))
    return start, start + width - 1


def grind_plan(*, unit: dict, symbol: str, reference: str, counts: tuple[int, int],
               placement: str = "after_includes") -> bytes:
    """The canonical JSON of an extern-run grind plan (ReproBit's ProjectGrindPlan)."""
    runs: dict = {"counts": {"start": counts[0], "stop": counts[1], "step": 1}}
    if placement != "after_includes":
        runs["placement"] = placement
    plan = {
        "extern_runs": runs,
        "reference_object": reference,
        "schema_version": 1,
        "symbol": symbol,
        "target": unit["target_id"],
        "translation_unit": unit["id"],
    }
    return (json.dumps(plan, sort_keys=True, separators=(",", ":")) + "\n").encode()


def original_bytes(exe: bytes, address: int, size: int) -> bytes:
    """`size` bytes of the original's image at virtual address `address`."""
    pe = struct.unpack_from("<I", exe, 0x3C)[0]
    base = struct.unpack_from("<I", exe, pe + 0x34)[0]
    count = struct.unpack_from("<H", exe, pe + 6)[0]
    offset = pe + 24 + struct.unpack_from("<H", exe, pe + 20)[0]
    rva = address - base
    for _ in range(count):
        vsize, va, rsize, raw = struct.unpack_from("<IIII", exe, offset + 8)
        if va <= rva < va + max(vsize, rsize):
            start = raw + rva - va
            return exe[start:start + size]
        offset += 40
    raise ReferenceError(f"{address:#x} is in no section of the original")


def output_dir(path: Path) -> Path:
    resolved = path.resolve()
    reference = (ROOT / "reference").resolve()
    if resolved != reference and reference not in resolved.parents:
        raise ReferenceError(f"--out must lie under reference/ (never committed): {path}")
    return resolved


# ---------------------------------------------------------------------------
# Compiling


@dataclass
class Found:
    address: int
    symbol: str
    pads: int | None
    placement: str = "after_includes"
    synthetic: bool = False
    refusal: str | None = None  # why --synthetic wrote no reference
    object_path: str | None = None
    plan_path: str | None = None


def static_symbol(address: int, root: Path | None = None) -> str | None:
    """A static function's COFF name, from the matching build's link map.

    A single-unit compile has no map, so Reccmp cannot name a static function
    there. The full matching build's map lists it under "Static symbols";
    the match baseline gives the address's name.
    """
    root = root or Path(__file__).resolve().parent.parent
    name = None
    baseline = root / "docs" / "coverage" / "match-baseline.txt"
    link_map = root / "build-reccmp" / "LegoRR.map"
    if not baseline.is_file() or not link_map.is_file():
        return None
    for line in baseline.read_text().splitlines():
        parts = line.split()
        if len(parts) >= 3 and parts[0].startswith("0x") and int(parts[0], 16) == address:
            name = parts[2]
            break
    if name is None:
        return None
    statics = False
    for line in link_map.read_text(errors="replace").splitlines():
        if line.strip() == "Static symbols":
            statics = True
        elif statics and len(line.split()) >= 2 and line.split()[1] == "_" + name:
            return "_" + name
    return None


def find_references(addresses: list[int], *, pads: tuple[int, int], jobs: int,
                    synthetic: bool = False) -> tuple[Path, list[Found], dict[int, bytes]]:
    """Sweep pad counts for one unit; the first exact compile per address."""
    import match_function as mf
    import phase_probe as pp

    source = pp.unit_for(addresses)
    unit = pp.Unit(source)
    try:
        guard = pp.InPlace(unit.source, unit.state)
        try:
            scored = unit.score(guard, guard.original, addresses)
        finally:
            guard.close()
        symbols = {}
        for address in addresses:
            symbol = scored[address].symbol or static_symbol(address)
            if not symbol:
                raise ReferenceError(f"{address:#x}: {scored[address].error or 'no compiled symbol'}")
            symbols[address] = symbol
        exe = mf.find_orig(argparse.Namespace(orig=None)).read_bytes()
        lines = guard.original.splitlines(keepends=True)
        pad_line = pp.default_pad_line(lines)
        sweep = unit.workdir / "reference"
        sweep.mkdir(exist_ok=True)

        def compile_pads(n: int) -> tuple[int, bytes | None]:
            cand = sweep / f"n{n:05d}{source.suffix}"
            cand.write_text(pp.pad_text(lines, {pad_line: n}) if n else "".join(lines))
            try:
                obj, _ = mf.compile_source(unit.env, unit.command, sweep, n, cand)
                data = obj.read_bytes()
                obj.unlink(missing_ok=True)
                return n, data
            except mf.MatchError:
                return n, None
            finally:
                cand.unlink(missing_ok=True)

        found = {
            a: Found(a, symbols[a], None, run_placement(guard.original, unit.markers[a].line))
            for a in addresses
        }
        objects: dict[int, bytes] = {}
        counts = list(range(pads[0], pads[1] + 1))
        with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, jobs)) as pool:
            for start in range(0, len(counts), max(1, jobs)):
                for n, data in sorted(pool.map(compile_pads, counts[start:start + jobs])):
                    if data is None:
                        continue
                    coff = msvc_coff.parse_coff(data, f"pads {n}")
                    for address, item in found.items():
                        if item.pads is not None:
                            continue
                        result = function_body(coff, item.symbol)
                        if result is None:
                            continue
                        body, spans = result
                        if body_matches(body, spans, original_bytes(exe, address, len(body))):
                            item.pads = n
                            objects[address] = data
                if all(item.pads is not None for item in found.values()):
                    break
        if synthetic and any(item.pads is None for item in found.values()):
            _n, base = compile_pads(0)
            if base is not None:
                for address, item in found.items():
                    if item.pads is not None:
                        continue
                    result = function_body(msvc_coff.parse_coff(base, "unpadded compile"), item.symbol)
                    if result is None:
                        continue
                    data, item.refusal = explain_synthetic_reference(
                        base, item.symbol, original_bytes(exe, address, len(result[0])),
                        address=address, ranges=image_ranges(exe),
                        known=known_addresses(ROOT / "build-reccmp"))
                    if data is not None:
                        item.refusal = None
                        item.pads, item.synthetic = 0, True
                        objects[address] = data
        return source, list(found.values()), objects
    finally:
        unit.close()


def write_outputs(source: Path, found: list[Found], objects: dict[int, bytes], *, out: Path,
                  counts: tuple[int, int] | None) -> None:
    build_plan = json.loads((ROOT / BUILD_PLAN).read_text(encoding="utf-8"))
    unit = translation_unit(build_plan, source.relative_to(ROOT).as_posix())
    out.mkdir(parents=True, exist_ok=True)
    for item in found:
        if not item.pads and not item.synthetic:
            continue  # None: no pad count fits; 0: the source already compiles to it
        obj = out / f"{item.address:08x}.obj"
        plan = out / f"{item.address:08x}.json"
        obj.write_bytes(objects[item.address])
        reference = obj.relative_to(ROOT).as_posix()
        plan.write_bytes(grind_plan(unit=unit, symbol=item.symbol, reference=reference,
                                    counts=counts or ((1, 32) if item.synthetic else count_window(item.pads)),
                                    placement=item.placement))
        item.object_path = reference
        item.plan_path = plan.relative_to(ROOT).as_posix()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("addresses", nargs="+", type=lambda s: int(s, 16), metavar="ADDR")
    parser.add_argument("--pads", nargs=2, type=int, default=(0, 128), metavar=("FIRST", "LAST"),
                        help="pad counts to compile (default 0 128)")
    parser.add_argument("--counts", nargs=2, type=int, default=None, metavar=("FIRST", "LAST"),
                        help="extern-run lengths the grind plan searches (default: 64 lengths "
                             "centred on the pad count found; at most 64)")
    parser.add_argument("--jobs", type=int, default=min(8, os.cpu_count() or 1))
    parser.add_argument("--synthetic", action="store_true",
                        help="for an address no pad count reproduces, write a synthetic reference: "
                             "the unpadded compile with the function's body replaced by the "
                             "original's (for grind's donor rewriting)")
    parser.add_argument("--out", type=Path, default=ROOT / "reference" / "grind")
    parser.add_argument("--json", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        if args.counts is not None and (
                not 1 <= args.counts[0] <= args.counts[1] or args.counts[1] - args.counts[0] >= 64):
            raise ReferenceError("--counts must name 1 to 64 lengths from 1")
        if args.pads[0] < 0 or args.pads[1] < args.pads[0]:
            raise ReferenceError("--pads must be an increasing range from 0")
        out = output_dir(args.out)
        source, found, objects = find_references(args.addresses, pads=tuple(args.pads), jobs=args.jobs,
                                                 synthetic=args.synthetic)
        write_outputs(source, found, objects, out=out,
                      counts=tuple(args.counts) if args.counts is not None else None)
    except Exception as exc:  # pylint: disable=broad-except
        if type(exc).__name__ not in ("ReferenceError", "ProbeError", "MatchError"):
            raise
        print(f"error: {exc}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps([vars(item) for item in found], indent=2))
    else:
        for item in found:
            if item.pads is None:
                print(f"{item.address:#010x} {item.symbol}: no pad count in {args.pads[0]}..{args.pads[1]} "
                      "reproduces the original's body")
                if item.refusal:
                    print(f"  no synthetic reference: {item.refusal}")
            elif item.synthetic:
                print(f"{item.address:#010x} {item.symbol}: synthetic reference {item.object_path} "
                      "(the original's body in the unpadded compile)")
                print(f"  rbit discover grind . --expert-plan {item.plan_path} [--accept-progress]")
            elif item.pads == 0:
                print(f"{item.address:#010x} {item.symbol}: the unit already compiles to the original's "
                      "body (outside relocated fields); nothing for grind to find")
            else:
                print(f"{item.address:#010x} {item.symbol}: {item.pads} pads; reference {item.object_path}"
                      f"; extern run {item.placement.replace('_', ' ')}")
                print(f"  rbit discover grind . --expert-plan {item.plan_path} [--accept-progress]")
    return 0 if all(item.pads is not None for item in found) else 1


if __name__ == "__main__":
    sys.exit(main())
