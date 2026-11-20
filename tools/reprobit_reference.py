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
    """Original addresses of the matching build's functions Reccmp pairs, and of
    its .data and .bss symbols, by COFF name.

    Read from ``build``'s Reccmp report and link map; empty when either is
    missing. They pin a synthetic reference's calls and global references
    where alignment alone cannot tell two apart, such as two fields of one
    global struct.
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
        match = re.match(r"\s*([0-9a-f]{4}):[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\b", line)
        if not match:
            continue
        place = int(match.group(3), 16)
        if place in pairs:
            known.setdefault(match.group(2), pairs[place])
        elif match.group(1) == "0003":
            # .data and .bss sit at the original's addresses (make
            # unit-data-offsets checks it), so a data symbol's address in the
            # matching build is its original address. .rdata is not pinned:
            # its literal pools may still differ in length.
            known.setdefault(match.group(2), place)
    return known


def function_starts(build: Path) -> list[int]:
    """The original addresses of the functions ``build``'s Reccmp report lists, sorted.

    Every annotated function is listed, matched or not, so the next one
    bounds a function's slot in the original. Empty when the report is missing.
    """
    results = build / "reccmp-report" / "results.json"
    if not results.is_file():
        return []
    return sorted({int(e["address"], 16) for e in json.loads(results.read_text())["data"]
                   if e.get("type", 1) == 1})


def original_slot(exe: bytes, address: int, starts: list[int]) -> int | None:
    """The length of the original's slot at ``address``: up to the next of
    ``starts``, within the image section holding it; None when unknown."""
    import bisect

    section = next(((lo, hi) for lo, hi in image_ranges(exe) if lo <= address < hi), None)
    at = bisect.bisect_right(starts, address)
    if section is None or at == len(starts) or starts[at - 1:at] != [address]:
        return None
    return min(starts[at], section[1]) - address


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


# MSVC 5.0 pads each function COMDAT to 16 bytes with NOPs after the
# procedure; a section the compiler left unpadded is padded in the image by
# the linker, with int3.
_COMDAT_FILL, _LINKER_FILL = 0x90, 0xCC
# CodeView records an associated .debug$S may hold besides the procedure: the
# ones that state no code offset, and the labels and data records whose
# offset is relocated against a symbol (an $L label carries their place).
_CV_PROCEDURES = (msvc_coff.S_LPROC32, msvc_coff.S_GPROC32, 0x0204, 0x0205)
_CV_OFFSET_FREE = frozenset({0x0001, 0x0002, 0x0003, 0x0004, 0x0006, 0x0200,
                             0x1001, 0x1002, 0x1003, 0x1006})
_CV_SYMBOL_RELATIVE = {0x0209: 4, msvc_coff.S_LDATA32: 8, msvc_coff.S_GDATA32: 8}


@dataclass
class _Procedure:
    """Where the seed object records its function's procedure size.

    Each place is a file offset into the seed, or None when the object has
    no such record. ``problem`` says why the records cannot be rewritten.
    """
    size: int  # the procedure size, or the body's length when nothing records one
    # MSVC 5.0's NOP fill follows the procedure; None when the body has none
    # but is a multiple of 16, so either may hold
    padded: bool | None
    recorded: bool = False  # some record states the procedure size
    total: int | None = None  # the function symbol's aux TotalSize
    end: int | None = None  # the .ef symbol's value
    length: int | None = None  # the section symbol's aux Length
    fpo: int | None = None  # the FPO record's cbProcSize
    codeview: int | None = None  # the CodeView procedure record's cbProc, DbgStart, DbgEnd
    problem: str | None = None


def _section_header(data: bytes, index: int) -> int:
    return 20 + struct.unpack_from("<H", data, 16)[0] + (index - 1) * 40


def _seed_procedure(data: bytes, coff: msvc_coff.CoffObject, sym: msvc_coff.Symbol,
                    sect: msvc_coff.Section) -> _Procedure:
    symptr = _u32(data, 8)
    body_length = sect.size - sym.value
    problems = []
    sizes: dict[str, int] = {}
    proc = _Procedure(body_length, False)
    if sym.aux:
        proc.total = symptr + (sym.index + 1) * 18 + 4
        sizes["TotalSize"] = _u32(data, proc.total)
    ends = [s for s in coff.symbols.values() if s.section == sym.section and s.name == ".ef"]
    if len(ends) == 1:
        proc.end = symptr + ends[0].index * 18 + 8
        sizes[".ef"] = ends[0].value
    elif ends:
        problems.append("the seed's section has several .ef symbols")
    section_symbols = [s for s in coff.symbols.values()
                       if s.section == sym.section and s.name == sect.name and s.value == 0
                       and s.storage_class == msvc_coff.IMAGE_SYM_CLASS_STATIC and s.aux]
    if len(section_symbols) == 1:
        proc.length = symptr + (section_symbols[0].index + 1) * 18
    for child in coff.sections:
        if child.comdat_associated != sym.section:
            continue
        raw = _u32(data, _section_header(data, child.index) + 20)
        if child.name == ".debug$F":
            if proc.fpo is not None or child.size != 16:
                problems.append("the seed's FPO data is not one 16-byte record")
                continue
            proc.fpo = raw + 4
            sizes["FPO cbProcSize"] = _u32(data, proc.fpo)
        elif child.name == ".debug$S":
            cursor = 0
            while cursor + 4 <= child.size:
                reclen, rectype = struct.unpack_from("<HH", child.data, cursor)
                if reclen < 2:
                    break
                if rectype in _CV_PROCEDURES:
                    if proc.codeview is not None:
                        problems.append("the seed's .debug$S holds several procedure records")
                    proc.codeview = raw + cursor + 16
                    sizes["CodeView cbProc"] = _u32(data, proc.codeview)
                elif rectype in _CV_SYMBOL_RELATIVE:
                    if _u32(child.data, cursor + _CV_SYMBOL_RELATIVE[rectype]) != 0:
                        problems.append(f"the seed's CodeView record {rectype:#x} at {cursor} "
                                        "states a code offset of its own")
                elif rectype not in _CV_OFFSET_FREE:
                    problems.append(f"the seed's CodeView record {rectype:#x} at {cursor} "
                                    "may state a code offset")
                cursor += reclen + 2
    if sizes:
        proc.recorded = True
        proc.size = sizes.get("TotalSize", next(iter(sizes.values())))
        if any(size != proc.size for size in sizes.values()):
            problems.append("the seed's records give different procedure sizes ("
                            + ", ".join(f"{k} {v}" for k, v in sizes.items()) + ")")
    body = sect.data[sym.value:sect.size]
    if proc.size < body_length:
        proc.padded = True
        if (body_length != (proc.size + 15) // 16 * 16
                or any(byte != _COMDAT_FILL for byte in body[proc.size:])):
            problems.append(f"the seed's body after its procedure ({proc.size} of {body_length} "
                            "bytes) is not MSVC 5.0's alignment fill")
    elif proc.size > body_length:
        problems.append(f"the seed's procedure size {proc.size} exceeds its body ({body_length})")
    elif body_length % 16 == 0:
        proc.padded = None
    proc.problem = "; ".join(problems) or None
    return proc


def _original_procedure(original: bytes, padded: bool | None) -> tuple[int, int, str | None]:
    """The original's procedure size, its length in the reference, and why not.

    ``original`` is the original's slot: its bytes up to the next function.
    Its procedure ends before the fill that pads the slot, the compiler's
    NOPs (``padded``) or else the linker's int3, and the reference's body is
    that procedure padded the way the seed's is. A seed that shows neither
    (``None``) takes the slot's own fill.
    """
    if padded is None:
        padded = original[-1:] == bytes([_COMDAT_FILL])
    fill = _COMDAT_FILL if padded else _LINKER_FILL
    proc = len(original)
    while proc and original[proc - 1] == fill:
        proc -= 1
    if len(original) - proc >= 16:
        return proc, proc, (f"the original's slot ends in {len(original) - proc} bytes of fill "
                            "(another function may lie in it)")
    if not padded:
        return proc, proc, None
    length = (proc + 15) // 16 * 16
    if length != len(original):
        return proc, length, (f"the original's slot ({len(original)} bytes) is not its procedure "
                              f"({proc} bytes) padded to 16")
    return proc, length, None


def _terminal(code: bytes, ins) -> bool:
    """A ret, or an unconditional jump, ends the code before it."""
    at = ins.offset + len(ins.shape[0])  # past the prefixes
    opcode = code[at]
    return (opcode in (0xC2, 0xC3, 0xE9, 0xEB)
            or (opcode == 0xFF and at + 1 < len(code) and code[at + 1] >> 3 & 7 in (4, 5)))


def _inner_function_start(code: bytes, instructions: list, address: int) -> int | None:
    """An offset where fill after a ret or jump reaches a 16-byte boundary
    with code after it: where another function would start."""
    for i, ins in enumerate(instructions):
        if not _terminal(code, ins):
            continue
        k = i + 1
        while (k < len(instructions) and instructions[k].length == 1
               and code[instructions[k].offset] in (_COMDAT_FILL, _LINKER_FILL)):
            k += 1
        if i + 1 < k < len(instructions) and (address + instructions[k].offset) % 16 == 0:
            return instructions[k].offset
    return None


def _relaid_object(data: bytes, coff: msvc_coff.CoffObject, sym: msvc_coff.Symbol,
                   sect: msvc_coff.Section, proc: _Procedure, *, new_body: bytes,
                   original_proc: int, relocations: list[tuple[int, int, int, int]],
                   labels: dict[int, int], carry) -> bytes | str:
    """The seed with ``new_body`` as the function's section and every record of
    its procedure following it; or why not.

    ``relocations`` are the section's relocation entries as (entry, body
    offset, symbol, type); ``labels`` the new values of the section's label
    symbols; ``carry`` maps a seed code offset to the original's (None when
    it has no place there). The section's raw data is resized in place and
    every file pointer past it moves by the difference.
    """
    header = _section_header(data, sym.section)
    raw = _u32(data, header + 20)
    relptr = _u32(data, header + 24)
    lnptr = _u32(data, header + 28)
    old_end = raw + sect.size
    symptr = _u32(data, 8)
    out = bytearray(data)
    for entry, offset, symbol, kind in relocations:
        struct.pack_into("<IIH", out, relptr + entry * 10, sym.value + offset, symbol, kind)
    for index, value in labels.items():
        struct.pack_into("<I", out, symptr + index * 18 + 8, value)
    for place in (proc.total, proc.end, proc.fpo):
        if place is not None:
            struct.pack_into("<I", out, place, original_proc)
    if proc.codeview is not None:
        start, end = struct.unpack_from("<II", data, proc.codeview + 4)
        start, end = carry(start), carry(end)
        if start is None or end is None:
            return "the seed's CodeView debug range has no place in the original"
        struct.pack_into("<III", out, proc.codeview, original_proc, start, end)
    for row, (offset, line) in enumerate(sect.linenumbers):
        if line == 0:
            continue
        moved = carry(offset)
        if moved is None:
            return f"the seed's line row at {offset} has no place in the original"
        struct.pack_into("<I", out, lnptr + row * 6, moved)
    delta = len(new_body) - (sect.size - sym.value)
    if proc.length is not None:
        struct.pack_into("<I", out, proc.length, sym.value + len(new_body))
    struct.pack_into("<I", out, header + 16, sym.value + len(new_body))
    if not delta:
        out[raw + sym.value:old_end] = new_body
        return bytes(out)
    # Every file pointer past the section's raw data moves with it.
    pointers = [8]
    for other in coff.sections:
        place = _section_header(data, other.index)
        pointers += [place + 20, place + 24, place + 28]
    for item in coff.symbols.values():
        if item.is_function and item.aux and item.section > 0:
            pointers.append(symptr + (item.index + 1) * 18 + 8)
    for place in pointers:
        value = _u32(data, place)
        if value >= old_end:
            struct.pack_into("<I", out, place, value + delta)
        elif raw < value < old_end:
            return "another record of the seed lies inside the function's raw data"
    return bytes(out[:raw + sym.value]) + new_body + bytes(out[old_end:])


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
    ``address`` up to the next function (its slot); ``ranges`` are the
    original image's sections. Its relocations sit at the original's own
    address fields:

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
    outside its relocated fields.

    The original's procedure ends before the fill that pads its slot:
    MSVC 5.0's NOPs when the seed's section is padded, else the linker's
    int3. When that size, or the padded length, differs from the seed's,
    the reference carries the original's: the FPO record's cbProcSize, the
    CodeView procedure's cbProc, the function's aux TotalSize and the
    ``.ef`` value are the original's procedure size, and the section (its
    header and its section symbol's Length) is resized, every file pointer
    past its raw data moving with it. Offsets inside the function follow the
    alignment: the entry stays at 0, line rows and the CodeView debug range
    go to the original instruction their seed instruction pairs with (one in
    a gap goes to the gap's first original instruction), switch tables keep their place after
    the code, and each ``$L`` label moves to where the original's fields put
    it (or, unreferenced, follows the alignment). That needs one function in
    its section, at its start, and records that state the size the same.

    None when the bodies already agree, or any step is ambiguous or
    inconsistent.
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
    proc = _seed_procedure(data, coff, sym, sect)
    original_proc, original_length, problem = _original_procedure(original, proc.padded)
    if problem:
        return refuse(problem)
    original = original[:original_length]
    resized = original_length != len(body) or (proc.recorded and original_proc != proc.size)
    if resized:
        if proc.problem:
            return refuse(proc.problem)
        if sym.value or any(s.section == sym.section and s.is_function and s.index != sym.index
                            for s in coff.symbols.values()):
            return refuse("several functions share the seed's section, so its length cannot change")
    length = len(body)
    olen = len(original)
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

    if not resized:
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
        return value - address if address <= value < address + olen else None

    try:
        seed_code, seed_end = _decode_code(body, seed_internal, "seed")
        orig_code, orig_end = _decode_code(original, original_internal, "original")
    except SyntheticRefusal as exc:
        return refuse(str(exc))
    if resized:
        # The code ends in a ret or jump: at the procedure's end, or up to
        # 3 bytes of alignment before its switch tables.
        code_end = min(orig_end, original_proc)
        slack = 3 if orig_end < original_proc else 0
        code = [ins for ins in orig_code if ins.end <= code_end]
        while code and code[-1].end >= code_end - slack and not _terminal(original, code[-1]):
            code.pop()
        if not code or not _terminal(original, code[-1]) or code_end - code[-1].end > slack:
            return refuse("the original's code does not end in a ret or jump (its slot may be "
                          "cut short)")
        inner = _inner_function_start(original, orig_code, address)
        if inner is not None:
            return refuse(f"the original's slot holds fill up to {inner}, where another function "
                          "may start")

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
                if not 0 <= target < olen or recursion:
                    item = _OriginalField(fld.offset, _REL32, value,
                                          (address + fld.offset + 4 + value) & _MASK, False)
            elif in_image(value):
                item = _OriginalField(fld.offset, _DIR32, value, value,
                                      address <= value < address + olen)
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
    if resized and (seed_end < proc.size or orig_end < original_proc) \
            and proc.size - seed_end != original_proc - orig_end:
        return refuse(f"the original's switch tables ({max(0, original_proc - orig_end)} bytes) "
                      f"differ in length from the seed's ({max(0, proc.size - seed_end)})")
    for reloc in data_relocs:
        offset = reloc.offset - seed_end + orig_end
        if offset + 4 > (original_proc if resized else olen):
            return refuse(f"the seed's table entry at {reloc.offset} falls past the original's "
                          "procedure")
        value = _u32(original, offset)
        if not address <= value < address + olen:
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

    moved = sorted(((offset, reloc) for key, offsets in by_key.items()
                    for offset, reloc in zip(offsets, seed_by_key[key])), key=lambda item: item[0])
    # The section's labels (jump tables, case and goto labels): where the
    # original's fields put them, or where the alignment carries them.
    labels: dict[int, int] = {}
    if resized:
        seed_index = {ins.offset: j for j, ins in enumerate(seed_code)}
        paired = {j: i for i, j in pairs}

        def carry(offset: int) -> int | None:
            if offset in (0, proc.size):  # the entry and the procedure's end
                return 0 if offset == 0 else original_proc
            if seed_end <= offset < proc.size:
                return offset - seed_end + orig_end
            j = seed_index.get(offset)
            if j is None:
                return None
            if j in paired:
                return orig_code[paired[j]].offset
            after = max((i for i, other in pairs if other < j), default=-1) + 1
            return orig_code[after].offset if after < len(orig_code) else orig_end

        for offset, reloc in moved:
            target = coff.symbols[reloc.symbol]
            if reloc.internal is None or target.storage_class != msvc_coff.IMAGE_SYM_CLASS_LABEL:
                continue
            value = (fields[offset].value - address + sym.value - reloc.addend) & _MASK
            if labels.setdefault(target.index, value) != value:
                return refuse(f"the original's fields put the label {target.name} at "
                              f"{labels[target.index]} and {value}")
        for item in coff.symbols.values():
            if (item.section == sym.section and item.storage_class == msvc_coff.IMAGE_SYM_CLASS_LABEL
                    and item.index not in labels):
                value = carry(item.value - sym.value)
                if value is None:
                    return refuse(f"the seed's label {item.name} at {item.value} has no place "
                                  "in the original")
                labels[item.index] = value + sym.value

    new_body = bytearray(original)
    for offset, reloc in moved:
        if reloc.internal is not None:
            target = labels.get(reloc.symbol, coff.symbols[reloc.symbol].value)
            addend = (fields[offset].value - address + sym.value - target) & _MASK
        else:
            addend = reloc.addend
        struct.pack_into("<I", new_body, offset, addend)
    if bytes(new_body) == body and all(offset == reloc.offset for offset, reloc in moved):
        return refuse("the original's body already equals the seed's outside relocated fields")
    slots = sorted(reloc.entry for _offset, reloc in moved)
    if resized:
        out = _relaid_object(data, coff, sym, sect, proc, new_body=bytes(new_body),
                             original_proc=original_proc, labels=labels, carry=carry,
                             relocations=[(slot, offset, reloc.symbol, reloc.type)
                                          for slot, (offset, reloc) in zip(slots, moved)])
        if isinstance(out, str):
            return refuse(out)
        return out, "written"

    optional_size = struct.unpack_from("<H", data, 16)[0]
    header = 20 + optional_size + (sym.section - 1) * 40
    raw = struct.unpack_from("<I", data, header + 20)[0] + sym.value
    relptr = struct.unpack_from("<I", data, header + 24)[0]
    out = bytearray(data)
    out[raw:raw + length] = new_body
    for slot, (offset, reloc) in zip(slots, moved):
        struct.pack_into("<IIH", out, relptr + slot * 10, sym.value + offset, reloc.symbol, reloc.type)
    return bytes(out), "written"


def legacy_synthetic_reference(data: bytes, symbol: str, original: bytes) -> bytes | None:
    """The seed with ``symbol``'s body replaced by the original's, at the
    seed's own relocation offsets (the tool's behaviour before field
    placement).

    The seed's relocated field bytes are kept where the seed has them, so it
    is right only when both bodies put their address fields at the same
    offsets: equal lengths, the instructions merely reordered or renamed
    around them. ``explain_synthetic_reference`` falls back to it when its
    alignment refuses such a body (a reordered x87 window can pair no field
    across it); grind then proves whatever record it authors against the
    donor, so a wrong guess here can only lead to a refusal. None when the
    bodies differ in length or already agree.
    """
    coff = msvc_coff.parse_coff(data, "synthetic reference")
    result = function_body(coff, symbol)
    if result is None:
        return None
    body, spans = result
    if len(original) < len(body):
        return None
    # Each kept relocation must start a 32-bit address field of the
    # original's own instructions (decoded linearly; a field past the last
    # decodable instruction is the data tail's): otherwise the original
    # does not put its fields where the seed does, and the reference would
    # relocate the middle of an instruction.
    fields: set[int] = set()
    decoded = 0
    try:
        while decoded < len(body):
            instruction = ia32_fields.decode(original[:len(body)], decoded)
            fields.update(field.offset for field in instruction.fields)
            decoded = instruction.end
    except ia32_fields.DecodeError:
        pass
    if any(start < decoded and start not in fields for start, _ in spans):
        return None
    target = bytearray(original[:len(body)])
    for start, end in spans:
        target[start:end] = body[start:end]
    if bytes(target) == body:
        return None
    sym = next(s for s in coff.symbols.values()
               if s.name == symbol and s.section > 0 and s.is_function)
    optional_size = struct.unpack_from("<H", data, 16)[0]
    header = 20 + optional_size + (sym.section - 1) * 40
    raw = struct.unpack_from("<I", data, header + 20)[0] + sym.value
    out = bytearray(data)
    out[raw:raw + len(body)] = target
    return bytes(out)


def explain_reference_with_fallback(data: bytes, symbol: str, original: bytes, *, address: int,
                                    ranges: list[tuple[int, int]],
                                    known: dict[str, int] | None = None
                                    ) -> tuple[bytes | None, str]:
    """``explain_synthetic_reference``, falling back to
    ``legacy_synthetic_reference`` when field placement refuses a body
    whose padded slot is the seed's own length (the procedures agree in
    size, so the seed's field offsets may still be the original's)."""
    placed, reason = explain_synthetic_reference(data, symbol, original, address=address,
                                                 ranges=ranges, known=known)
    if placed is not None:
        return placed, reason
    coff = msvc_coff.parse_coff(data, "synthetic reference")
    result = function_body(coff, symbol)
    if result is None:
        return None, reason
    body = result[0]
    padded = (len(body) + 15) // 16 * 16
    if len(original) != padded:
        return None, reason
    legacy = legacy_synthetic_reference(data, symbol, original)
    if legacy is None:
        return None, reason
    return legacy, f"{reason}; used the seed's field offsets (same-length body)"


def synthetic_reference(data: bytes, symbol: str, original: bytes, *, address: int,
                        ranges: list[tuple[int, int]],
                        known: dict[str, int] | None = None) -> bytes | None:
    """``explain_reference_with_fallback`` without the reason."""
    return explain_reference_with_fallback(data, symbol, original, address=address,
                                           ranges=ranges, known=known)[0]


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
    # Under the checkout's own reference/, even when that is a symlink (a
    # helper worktree's), so the plans' paths stay project-relative.
    return ROOT / "reference" / resolved.relative_to(reference)


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
            starts = function_starts(ROOT / "build-reccmp")
            if base is not None:
                for address, item in found.items():
                    if item.pads is not None:
                        continue
                    result = function_body(msvc_coff.parse_coff(base, "unpadded compile"), item.symbol)
                    if result is None:
                        continue
                    # The original's whole slot: its procedure may differ in length.
                    slot = original_slot(exe, address, starts) or len(result[0])
                    data, item.refusal = explain_reference_with_fallback(
                        base, item.symbol, original_bytes(exe, address, slot),
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
                             "original's, at the original's length (for grind's donor and "
                             "region-resize rewriting)")
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
