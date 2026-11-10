# SPDX-License-Identifier: LGPL-3.0-or-later
"""Minimal reader for MSVC 5.0 i386 COFF objects and LIB archives.

Only what tools/match_function.py needs: sections with raw bytes,
relocations and COFF line numbers, the symbol table (with the auxiliary
records for section definitions and .bf), and the CodeView 5 records
(signature 2) that MSVC 5.0 writes to .debug$S under /Zi.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass, field
from pathlib import Path

IMAGE_SCN_CNT_CODE = 0x00000020
IMAGE_SCN_CNT_INITIALIZED_DATA = 0x00000040
IMAGE_SCN_CNT_UNINITIALIZED_DATA = 0x00000080
IMAGE_SCN_LNK_INFO = 0x00000200
IMAGE_SCN_LNK_REMOVE = 0x00000800
IMAGE_SCN_LNK_COMDAT = 0x00001000
IMAGE_SCN_LNK_NRELOC_OVFL = 0x01000000
IMAGE_SCN_MEM_DISCARDABLE = 0x02000000
IMAGE_SCN_MEM_EXECUTE = 0x20000000
IMAGE_SCN_MEM_WRITE = 0x80000000

IMAGE_REL_I386_ABSOLUTE = 0x0000
IMAGE_REL_I386_DIR32 = 0x0006
IMAGE_REL_I386_DIR32NB = 0x0007
IMAGE_REL_I386_SECTION = 0x000A
IMAGE_REL_I386_SECREL = 0x000B
IMAGE_REL_I386_REL32 = 0x0014

IMAGE_SYM_CLASS_EXTERNAL = 2
IMAGE_SYM_CLASS_STATIC = 3
IMAGE_SYM_CLASS_LABEL = 6
IMAGE_SYM_CLASS_FUNCTION = 101
IMAGE_SYM_CLASS_FILE = 103
IMAGE_SYM_DTYPE_FUNCTION = 0x20

# CodeView 5 symbol records (32-bit type indices) as written by MSVC 5.0.
S_COMPILE = 0x0001
S_END = 0x0006
S_OBJNAME = 0x0009
S_LDATA32 = 0x1007
S_GDATA32 = 0x1008
S_LPROC32 = 0x100A
S_GPROC32 = 0x100B
# Older 16-bit-type procedure, thunk, block and with records also open a
# scope that a later S_END closes.
_SCOPE_STARTS = {0x0204, 0x0205, 0x0206, 0x0207, 0x0208}


class CoffError(Exception):
    pass


@dataclass
class Relocation:
    offset: int  # offset within the section
    symbol: int  # symbol table index
    type: int


@dataclass
class Section:
    index: int  # 1-based
    name: str
    characteristics: int
    data: bytes
    size: int
    relocations: list[Relocation] = field(default_factory=list)
    # (offset in section or symbol index when line == 0, line)
    linenumbers: list[tuple[int, int]] = field(default_factory=list)
    comdat_selection: int | None = None
    comdat_associated: int | None = None

    @property
    def is_code(self) -> bool:
        return bool(self.characteristics & IMAGE_SCN_CNT_CODE)

    @property
    def is_bss(self) -> bool:
        return bool(self.characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)

    @property
    def is_writable(self) -> bool:
        return bool(self.characteristics & IMAGE_SCN_MEM_WRITE)

    @property
    def is_debug(self) -> bool:
        return self.name.startswith(".debug") or bool(
            self.characteristics & (IMAGE_SCN_LNK_INFO | IMAGE_SCN_LNK_REMOVE)
        )


@dataclass
class Symbol:
    index: int
    name: str
    value: int
    section: int  # 1-based; 0 = undefined/common, -1 = absolute, -2 = debug
    type: int
    storage_class: int
    aux: list[bytes]

    @property
    def is_function(self) -> bool:
        return (self.type & 0x30) == IMAGE_SYM_DTYPE_FUNCTION

    @property
    def is_external(self) -> bool:
        return self.storage_class == IMAGE_SYM_CLASS_EXTERNAL

    @property
    def is_undefined(self) -> bool:
        return self.section == 0


@dataclass
class CvProc:
    name: str
    symbol: int  # COFF symbol index the S_*PROC32 offset is relocated against
    offset: int  # extra offset added to that symbol (normally 0)
    length: int
    is_global: bool
    static_locals: list["CvData"] = field(default_factory=list)


@dataclass
class CvData:
    name: str
    symbol: int
    offset: int
    is_global: bool


@dataclass
class CoffObject:
    path: Path | str
    sections: list[Section]
    symbols: dict[int, Symbol]
    procs: list[CvProc]
    data: list[CvData]
    source_name: str | None

    def section(self, index: int) -> Section:
        return self.sections[index - 1]

    def function_lines(self) -> dict[int, list[tuple[int, int]]]:
        """Absolute (line, offset) pairs per code section, resolved from
        the COFF line-number table and the .bf base line of each function."""
        base_lines: dict[int, int] = {}
        for sym in self.symbols.values():
            if sym.name == ".bf" and sym.aux and sym.section > 0:
                (line,) = struct.unpack_from("<H", sym.aux[0], 4)
                base_lines.setdefault(sym.section, line)

        result: dict[int, list[tuple[int, int]]] = {}
        for sect in self.sections:
            if not sect.linenumbers:
                continue
            base = base_lines.get(sect.index)
            if base is None:
                continue
            pairs: list[tuple[int, int]] = []
            func_offset = 0
            for addr, line in sect.linenumbers:
                if line == 0:
                    sym = self.symbols.get(addr)
                    func_offset = sym.value if sym is not None else 0
                    pairs.append((base, func_offset))
                else:
                    pairs.append((base + line, addr))
            result[sect.index] = pairs
        return result


def _read_name(raw: bytes, strtab: bytes) -> str:
    if raw[:4] == b"\0\0\0\0":
        (offset,) = struct.unpack_from("<I", raw, 4)
        end = strtab.index(b"\0", offset)
        return strtab[offset:end].decode("latin1")
    return raw.rstrip(b"\0").decode("latin1")


def _pascal(buf: bytes, offset: int) -> str:
    length = buf[offset]
    return buf[offset + 1 : offset + 1 + length].decode("latin1")


def parse_coff(data: bytes, path: Path | str = "<memory>") -> CoffObject:
    if len(data) < 20:
        raise CoffError(f"{path}: too small for a COFF header")
    machine, nsec, _ts, symptr, nsym, optsize, _chars = struct.unpack_from(
        "<HHIIIHH", data, 0
    )
    if machine != 0x14C:
        raise CoffError(f"{path}: not an i386 COFF object (machine 0x{machine:x})")

    strtab_off = symptr + nsym * 18
    if strtab_off + 4 <= len(data):
        (strtab_size,) = struct.unpack_from("<I", data, strtab_off)
        strtab = data[strtab_off : strtab_off + strtab_size]
    else:
        strtab = b""

    sections: list[Section] = []
    for i in range(nsec):
        off = 20 + optsize + i * 40
        name_raw = data[off : off + 8]
        (
            _vsize,
            _vaddr,
            rawsize,
            rawptr,
            relptr,
            lnptr,
            nrel,
            nln,
            chars,
        ) = struct.unpack_from("<IIIIIIHHI", data, off + 8)
        name = name_raw.rstrip(b"\0").decode("latin1")
        if name.startswith("/") and name[1:].isdigit():
            start = int(name[1:])
            name = strtab[start : strtab.index(b"\0", start)].decode("latin1")
        raw = b"" if chars & IMAGE_SCN_CNT_UNINITIALIZED_DATA else data[rawptr : rawptr + rawsize]
        sect = Section(i + 1, name, chars, raw, rawsize)
        if chars & IMAGE_SCN_LNK_NRELOC_OVFL and nrel == 0xFFFF:
            (nrel,) = struct.unpack_from("<I", data, relptr)
            relptr += 10
            nrel -= 1
        for r in range(nrel):
            roff, rsym, rtype = struct.unpack_from("<IIH", data, relptr + r * 10)
            sect.relocations.append(Relocation(roff, rsym, rtype))
        for n in range(nln):
            addr, line = struct.unpack_from("<IH", data, lnptr + n * 6)
            sect.linenumbers.append((addr, line))
        sections.append(sect)

    symbols: dict[int, Symbol] = {}
    i = 0
    source_name = None
    while i < nsym:
        off = symptr + i * 18
        name = _read_name(data[off : off + 8], strtab)
        value, secnum, stype, sclass, naux = struct.unpack_from("<IhHBB", data, off + 8)
        aux = [data[off + 18 * (k + 1) : off + 18 * (k + 2)] for k in range(naux)]
        sym = Symbol(i, name, value, secnum, stype, sclass, aux)
        symbols[i] = sym
        if sclass == IMAGE_SYM_CLASS_FILE and aux and source_name is None:
            source_name = b"".join(aux).rstrip(b"\0").decode("latin1")
        if (
            sclass == IMAGE_SYM_CLASS_STATIC
            and secnum > 0
            and aux
            and value == 0
            and name == sections[secnum - 1].name
        ):
            # Section definition auxiliary record.
            _len, _nrel, _nln, _chk, number, selection = struct.unpack_from(
                "<IHHIHB", aux[0], 0
            )
            sect = sections[secnum - 1]
            if sect.characteristics & IMAGE_SCN_LNK_COMDAT:
                sect.comdat_selection = selection
                if selection == 5:  # IMAGE_COMDAT_SELECT_ASSOCIATIVE
                    sect.comdat_associated = number
        i += 1 + naux

    procs: list[CvProc] = []
    cvdata: list[CvData] = []
    for sect in sections:
        if sect.name != ".debug$S" or not sect.data:
            continue
        procs_here, data_here = _parse_cv_symbols(sect)
        procs.extend(procs_here)
        cvdata.extend(data_here)

    return CoffObject(path, sections, symbols, procs, cvdata, source_name)


def _parse_cv_symbols(sect: Section) -> tuple[list[CvProc], list[CvData]]:
    buf = sect.data
    pos = 0
    # The primary .debug$S begins with a CodeView signature; the COMDAT
    # associated ones written by MSVC 5.0 do not.
    if not sect.characteristics & IMAGE_SCN_LNK_COMDAT and len(buf) >= 4:
        (sig,) = struct.unpack_from("<I", buf, 0)
        if sig in (1, 2, 4):
            pos = 4
    relocs = {r.offset: r for r in sect.relocations if r.type == IMAGE_REL_I386_SECREL}
    procs: list[CvProc] = []
    data: list[CvData] = []
    depth = 0
    current: CvProc | None = None
    while pos + 4 <= len(buf):
        reclen, rectype = struct.unpack_from("<HH", buf, pos)
        if reclen < 2:
            break
        rec = pos
        if rectype in (S_GPROC32, S_LPROC32):
            (_p, _e, _n, length, _ds, _de, _ti, offset, _seg, _flags) = struct.unpack_from(
                "<IIIIIIIIHB", buf, rec + 4
            )
            name = _pascal(buf, rec + 4 + 35)
            reloc = relocs.get(rec + 32)
            if reloc is not None:
                current = CvProc(name, reloc.symbol, offset, length, rectype == S_GPROC32)
                procs.append(current)
            depth += 1
        elif rectype in (S_GDATA32, S_LDATA32):
            (_ti, offset, _seg) = struct.unpack_from("<IIH", buf, rec + 4)
            name = _pascal(buf, rec + 14)
            reloc = relocs.get(rec + 8)
            # Unnamed records inside procedures describe jump tables.
            if reloc is not None and name:
                entry = CvData(name, reloc.symbol, offset, rectype == S_GDATA32)
                if depth > 0 and current is not None:
                    current.static_locals.append(entry)
                else:
                    data.append(entry)
        elif rectype in _SCOPE_STARTS:
            depth += 1
        elif rectype == S_END:
            depth = max(0, depth - 1)
            if depth == 0:
                current = None
        pos += 2 + reclen
    return procs, data


def read_coff(path: Path | str) -> CoffObject:
    return parse_coff(Path(path).read_bytes(), path)


# ---------------------------------------------------------------------------
# LIB archives (for LIBC.LIB members that provide CRT LIBRARY functions)


@dataclass
class ArchiveMember:
    name: str
    offset: int
    data: bytes


def _archive_members(data: bytes) -> list[ArchiveMember]:
    if not data.startswith(b"!<arch>\n"):
        raise CoffError("not an archive")
    pos = 8
    members: list[ArchiveMember] = []
    longnames = b""
    while pos + 60 <= len(data):
        header = data[pos : pos + 60]
        name = header[:16].decode("latin1").rstrip()
        size = int(header[48:58].decode("latin1").strip())
        body = data[pos + 60 : pos + 60 + size]
        if name == "//":
            longnames = body
        elif name.startswith("/") and name[1:].isdigit():
            start = int(name[1:])
            end = longnames.find(b"\0", start)
            if end < 0:
                end = longnames.find(b"\n", start)
            name = longnames[start:end].decode("latin1").rstrip("/")
        members.append(ArchiveMember(name, pos, body))
        pos += 60 + size + (size & 1)
    return members


def archive_symbol_index(data: bytes) -> dict[str, int]:
    """Map each public symbol to the archive offset of its member, using the
    first linker member (big-endian offsets)."""
    members = _archive_members(data)
    if not members or members[0].name != "/":
        return {}
    body = members[0].data
    (count,) = struct.unpack_from(">I", body, 0)
    offsets = struct.unpack_from(f">{count}I", body, 4)
    names = body[4 + 4 * count :].split(b"\0")
    return {names[i].decode("latin1"): offsets[i] for i in range(count)}


def archive_member_at(data: bytes, offset: int) -> ArchiveMember:
    """The member whose header starts at `offset`, with its long name resolved."""
    for member in _archive_members(data):
        if member.offset == offset:
            return member
    raise CoffError(f"no archive member at offset {offset}")


def member_basename(member: ArchiveMember) -> str:
    return member.name.rstrip("/").replace("\\", "/").split("/")[-1]
