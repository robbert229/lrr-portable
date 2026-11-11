# SPDX-License-Identifier: LGPL-3.0-or-later
"""Synthetic recomp image for one unlinked MSVC 5.0 object.

Lays a COFF object out the way a linker would, applies its relocations, and
gives undefined symbols and Win32 imports the addresses and names that the
linker PDB of the matching build would give them. Like the matching build
(linked with /INCREMENTAL:NO) and the original, the image has no base
relocations and no incremental-link thunks. The result is a duck-typed Reccmp
`Image`, plus the recomp entities (`collect_nodes`) that stand in for Reccmp's
`load_cvdump`. Used by tools/match_function.py.
"""

from __future__ import annotations

import bisect
import re
import struct
from collections.abc import Iterator
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import msvc_coff
from msvc_coff import CoffObject, Section

# Synthetic recomp address space, laid out like a small linked program:
# code, const data, data, then areas for undefined symbols, the IAT and import
# thunks. The base sits outside the original image (so a hard-coded original
# address in the source never lands on a synthetic symbol) and is not a
# plausible flag, colour, FourCC or float constant.
IMAGEBASE = 0x0E400000
CODE_BASE = 0x0E401000
EXTERN_CODE_SLOT = 0x10
EXTERN_DATA_SLOT = 0x10000


# ---------------------------------------------------------------------------
# Symbol naming (what the linker PDB would call each COFF symbol)


def _strip_c_decoration(sym: str) -> str:
    name = sym
    if name.startswith("_") or name.startswith("@"):
        name = name[1:]
    m = re.match(r"^(.*)@\d+$", name)
    if m:
        name = m.group(1)
    return name


_SPECIAL_NAMES = {
    "0": None,  # constructor
    "1": None,  # destructor
    "2": "operator new",
    "3": "operator delete",
    "4": "operator=",
    "5": "operator>>",
    "6": "operator<<",
    "7": "operator!",
    "8": "operator==",
    "9": "operator!=",
    "A": "operator[]",
    "C": "operator->",
    "D": "operator*",
    "E": "operator++",
    "F": "operator--",
    "G": "operator-",
    "H": "operator+",
    "I": "operator&",
    "M": "operator<",
    "N": "operator<=",
    "O": "operator>",
    "P": "operator>=",
    "R": "operator()",
    "Y": "operator+=",
    "Z": "operator-=",
}


def friendly_name(sym: str) -> str:
    """Undecorated, qualified name as MSVC 5.0 CodeView records it
    (e.g. `lego::foo`, `Foo::~Foo`, `g_value`). Falls back to the
    decorated symbol for shapes this helper does not understand."""
    if not sym.startswith("?"):
        return _strip_c_decoration(sym)
    body = sym[1:]
    special: str | None = None
    if body.startswith("?") and not body.startswith("?$"):
        code = body[1]
        if code == "_":
            return sym  # vtables, RTTI, string literals and friends
        special = code
        body = body[2:]
    parts: list[str] = []
    names: list[str] = []
    pos = 0
    first = special is None
    while pos < len(body) and body[pos] != "@":
        ch = body[pos]
        if ch.isdigit():
            idx = int(ch)
            if idx >= len(names):
                return sym
            parts.append(names[idx])
            pos += 1
            continue
        if body.startswith("?$", pos) or ch == "?":
            return sym  # templates and nested special scopes
        end = body.find("@", pos)
        if end < 0:
            return sym
        frag = body[pos:end]
        parts.append(frag)
        names.append(frag)
        pos = end + 1
        first = False
    del first
    if not parts:
        return sym
    scopes = list(reversed(parts))
    if special is not None:
        cls = scopes[-1]
        if special == "0":
            scopes.append(cls)
        elif special == "1":
            scopes.append("~" + cls)
        elif special in _SPECIAL_NAMES and _SPECIAL_NAMES[special]:
            scopes.append(_SPECIAL_NAMES[special])
        else:
            return sym
    return "::".join(scopes)


# ---------------------------------------------------------------------------
# Synthetic recomp image


@dataclass
class _Region:
    start: int
    data: bytearray
    size: int
    kind: str  # code, rdata, data, extern, iat
    name: str

    @property
    def end(self) -> int:
        return self.start + self.size


@dataclass
class Node:
    """One recomp entity, the equivalent of a CvdumpNode."""

    addr: int
    type: Any = None
    name: str | None = None
    symbol: str | None = None
    size: int | None = None
    orig_match: int | None = None


def _align(value: int, alignment: int = 16) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


class SyntheticImage:
    """Duck-typed reccmp Image built from one unlinked COFF object.

    Sections are laid out like a linker would (code / const / data areas),
    relocations are applied, undefined symbols get their own slots, and Win32
    imports get an IAT and jmp thunks. The matching build links with
    /INCREMENTAL:NO, so no call goes through an incremental-link thunk."""

    def __init__(self, obj: CoffObject, ctx: Any, source: Path):
        # ctx: match_function.ProjectContext (orig_bin, orig_imports,
        # libc_* lookups, markers, resolve_lineref).
        from reccmp.formats.image import ImageImport

        self.filepath = Path(str(obj.path))
        self.obj = obj
        self.ctx = ctx
        self.source = source
        self.regions: list[_Region] = []
        self.section_addr: dict[int, int] = {}
        # The matching build has no .reloc section, so Reccmp finds no float
        # constants, strings or import thunks through relocations there.
        self.relocations: set[int] = set()
        self.import_list: list[ImageImport] = []
        self.thunks: list[tuple[int, int]] = []
        self.exports: list[tuple[int, bytes]] = []
        self.is_debug = False  # tools/reccmp.sh strips the debug directory
        self.nodes: dict[int, Node] = {}
        self.extern_addr: dict[str, int] = {}
        self._layout()
        self._starts = [r.start for r in self.regions]

    # -- reccmp Image protocol -------------------------------------------
    @property
    def imagebase(self) -> int:
        # Reccmp only consults relocations for values above the image base.
        # Report the linked program's base so hard-coded image addresses in
        # the source (see is_relocated_addr) are tested too.
        return self.ctx.orig_bin.imagebase

    @property
    def entry(self) -> int:
        return CODE_BASE

    @property
    def imports(self):
        return iter(self.import_list)

    @property
    def sections(self):
        from reccmp.formats.image import ImageSection

        return tuple(
            ImageSection(
                virtual_range=range(r.start, r.end),
                physical_range=range(0, len(r.data)),
                view=memoryview(bytes(r.data)),
                name=r.name,
            )
            for r in self.regions
        )

    def _region(self, vaddr: int) -> _Region:
        from reccmp.formats.exceptions import InvalidVirtualAddressError

        i = bisect.bisect_right(self._starts, vaddr) - 1
        if i >= 0 and vaddr < self.regions[i].end:
            return self.regions[i]
        raise InvalidVirtualAddressError(f"0x{vaddr:x} is not in the synthetic image")

    def seek(self, vaddr: int) -> tuple[memoryview, int]:
        region = self._region(vaddr)
        offset = vaddr - region.start
        return memoryview(bytes(region.data))[offset:], region.size - offset

    def read(self, vaddr: int, size: int) -> bytes:
        from reccmp.formats.exceptions import InvalidVirtualReadError

        region = self._region(vaddr)
        offset = vaddr - region.start
        if size < 0 or offset + size > region.size:
            raise InvalidVirtualReadError(f"cannot read {size} bytes from 0x{vaddr:x}")
        data = bytes(region.data[offset : offset + size])
        return data + b"\0" * (size - len(data))

    def read_string(self, vaddr: int) -> bytes:
        from reccmp.formats.image import r_szstring

        view, _ = self.seek(vaddr)
        match = r_szstring.match(view)
        return match.group(0)

    def read_widechar(self, vaddr: int) -> bytes:
        from reccmp.formats.exceptions import InvalidStringError
        from reccmp.formats.image import r_widestring

        view, _ = self.seek(vaddr)
        match = r_widestring.match(view)
        if match is None or len(match.group(0)) % 2 != 0:
            raise InvalidStringError(f"Cannot read widechar string at {vaddr:x}")
        return match.group(0)

    def is_valid_vaddr(self, vaddr: int) -> bool:
        try:
            self._region(vaddr)
            return True
        except Exception:  # pylint: disable=broad-except
            return False

    def is_relocated_addr(self, addr: int) -> bool:
        # A constant inside the span of the linked matching build (an original
        # address cast to a pointer, or a flag such as 0x800000 that happens
        # to land in its large .bss) falls inside some entity of that link,
        # and Reccmp then renders it as an address.
        return self.ctx.orig_bin.imagebase < addr < self.ctx.link_end

    def _regions_of(self, *kinds: str):
        from reccmp.formats.image import ImageRegion

        for r in self.regions:
            if r.kind in kinds:
                yield ImageRegion(r.start, bytes(r.data), r.size)

    def get_code_regions(self):
        return self._regions_of("code")

    def get_const_regions(self):
        return self._regions_of("rdata")

    def get_data_regions(self):
        return self._regions_of("rdata", "data")

    def iter_string(self, encoding: str = "ascii") -> Iterator[tuple[int, str]]:
        # Reccmp finds strings at relocated addresses; without base relocations
        # only the PDB's string symbols (collect_nodes) name them.
        del encoding
        return iter(())

    # -- layout -------------------------------------------------------------
    def _add_region(self, start: int, data: bytes | bytearray, size: int, kind: str, name: str) -> _Region:
        region = _Region(start, bytearray(data), size, kind, name)
        self.regions.append(region)
        return region

    def _layout(self) -> None:
        obj = self.obj
        placed: dict[str, list[Section]] = {"code": [], "rdata": [], "data": []}
        for sect in obj.sections:
            if sect.is_debug or sect.name == ".drectve" or sect.size == 0:
                continue
            if sect.is_code:
                placed["code"].append(sect)
            elif sect.is_writable or sect.is_bss:
                placed["data"].append(sect)
            else:
                placed["rdata"].append(sect)
        cursor = CODE_BASE
        for kind in ("code", "rdata", "data"):
            for sect in placed[kind]:
                start = _align(cursor)
                data = b"" if sect.is_bss else sect.data
                self._add_region(start, data, sect.size, kind, sect.name)
                self.section_addr[sect.index] = start
                cursor = start + sect.size
            cursor = _align(cursor, 0x1000)

        # Reserve the areas for undefined symbols and thunks up front.
        undefined = sum(1 for s in obj.symbols.values() if s.is_undefined) + 1
        self.extern_code_base = cursor
        cursor = _align(cursor + EXTERN_CODE_SLOT * undefined, 0x1000)
        self.iat_base = cursor
        cursor = _align(cursor + 4 * undefined, 0x1000)
        self.import_thunk_base = cursor
        cursor = _align(cursor + 8 * undefined, 0x1000)
        self.extern_data_base = cursor

        self._classify_externals()
        self.regions.sort(key=lambda r: r.start)
        self._starts = [r.start for r in self.regions]
        self._apply_relocations()

    # -- external symbols -----------------------------------------------------
    def _classify_externals(self) -> None:
        from reccmp.formats.image import ImageImport

        obj = self.obj
        cv_names: dict[str, str] = {}
        for entry in obj.data:
            sym = obj.symbols.get(entry.symbol)
            if sym is not None and sym.is_undefined:
                cv_names.setdefault(sym.name, entry.name)

        undefined = sorted(
            {
                s.name: s
                for s in obj.symbols.values()
                if s.is_undefined and s.storage_class == msvc_coff.IMAGE_SYM_CLASS_EXTERNAL
            }.values(),
            key=lambda s: s.index,
        )
        # Assembler objects leave the function type off undefined symbols;
        # anything reached by a rel32 call/jmp is code all the same.
        rel32_targets = {
            obj.symbols[r.symbol].name
            for sect in obj.sections
            if sect.is_code
            for r in sect.relocations
            if r.type == msvc_coff.IMAGE_REL_I386_REL32 and r.symbol in obj.symbols
        }
        code_slots = 0
        data_slots = 0
        iat: dict[str, int] = {}
        import_thunks: list[tuple[int, int]] = []

        def iat_slot(api: str) -> int:
            if api not in iat:
                addr = self.iat_base + 4 * len(iat)
                iat[api] = addr
                orig = self.ctx.orig_imports.get(api.upper())
                module = orig.module if orig is not None else "UNKNOWN.dll"
                self.import_list.append(ImageImport(addr=addr, module=module, name=api))
            return iat[api]

        for sym in undefined:
            name = sym.name
            absolute = self.ctx.libc_absolute(name) if name in self.ctx.libc_symbols else None
            if absolute is not None:
                self.extern_addr[name] = absolute
                continue
            if name.startswith("__imp_"):
                api = _strip_c_decoration(name[len("__imp_") :])
                self.extern_addr[name] = iat_slot(api)
                continue

            is_function = sym.is_function or name in rel32_targets
            if not is_function and sym.value == 0 and name not in cv_names:
                # MSVC 5.0 marks undefined functions with DTYPE_FUNCTION; data
                # externals named by CodeView are handled below.
                is_function = name.startswith("?") and "(" in self._demangled(name)

            if is_function:
                api = _strip_c_decoration(name)
                resolved, fname, ftype = self._resolve_function(name)
                if (
                    resolved is None
                    and not name.startswith("?")
                    and api.upper() in self.ctx.orig_imports
                    and fname == api
                ):
                    # A Win32 API called without dllimport: the linker emits a
                    # `jmp [__imp_X]` thunk and the call goes there. Without
                    # base relocations Reccmp cannot recognise the thunk; the
                    # linker PDB names it with its public symbol instead.
                    thunk = self.import_thunk_base + 8 * len(import_thunks)
                    import_thunks.append((thunk, iat_slot(api)))
                    self.nodes[thunk] = Node(thunk, self._c_type(name), name, name, None)
                    self.extern_addr[name] = thunk
                    continue
                addr = self.extern_code_base + EXTERN_CODE_SLOT * code_slots
                code_slots += 1
                self.nodes[addr] = Node(addr, ftype, fname, name, None, resolved)
                self.extern_addr[name] = addr
            else:
                from reccmp.types import EntityType

                addr = self.extern_data_base + EXTERN_DATA_SLOT * data_slots
                data_slots += 1
                if name in cv_names:
                    # Declared in this TU: the PDB GLOBALS carry the CodeView name.
                    self.nodes[addr] = Node(addr, EntityType.DATA, cv_names[name], name, None)
                elif name in self.ctx.libc_symbols and name not in self.ctx.nameref_names:
                    # Library data without CodeView: only the public symbol.
                    self.nodes[addr] = Node(addr, None, name, name, None)
                else:
                    self.nodes[addr] = Node(addr, EntityType.DATA, friendly_name(name), name, None)
                self.extern_addr[name] = addr

        if code_slots:
            self._add_region(self.extern_code_base, b"", EXTERN_CODE_SLOT * code_slots, "extern", "extern-code")
        if data_slots:
            self._add_region(self.extern_data_base, b"", EXTERN_DATA_SLOT * data_slots, "extern", "extern-data")
        if iat:
            self._add_region(self.iat_base, b"", 4 * len(iat), "iat", ".idata")
        if import_thunks:
            buf = bytearray()
            for thunk, slot in import_thunks:
                buf += b"\xff\x25" + struct.pack("<I", slot) + b"\xcc\xcc"
            self._add_region(self.import_thunk_base, buf, len(buf), "code", "import-thunks")

    @staticmethod
    def _demangled(sym: str) -> str:
        from reccmp.cvdump.demangler import msvc_demangle

        try:
            return msvc_demangle(sym)
        except Exception:  # pylint: disable=broad-except
            return ""

    def _resolve_function(self, sym: str) -> tuple[int | None, str, Any]:
        """Decide the name a full PDB would give an external function and,
        for line-referenced annotations, which original address it is."""
        from reccmp.types import EntityType

        friendly = friendly_name(sym)
        if sym in self.ctx.nameref_names:
            return None, sym, self._c_type(sym)
        if friendly in self.ctx.nameref_names:
            return None, friendly, EntityType.FUNCTION
        resolved = self.ctx.resolve_lineref(friendly, exclude=self.source.resolve())
        if resolved is not None:
            return resolved, friendly, EntityType.FUNCTION
        if sym in self.ctx.libc_symbols:
            # Library code without CodeView: Reccmp only sees the public name.
            return None, sym, self._c_type(sym)
        return None, friendly, EntityType.FUNCTION

    @staticmethod
    def _c_type(sym: str):
        from reccmp.types import EntityType

        # CvdumpNode.set_decorated: only C symbols with an @ get a type.
        if not sym.startswith("?") and "@" in sym:
            return EntityType.FUNCTION
        return None

    # -- relocations ------------------------------------------------------------
    def symbol_address(self, index: int) -> int | None:
        sym = self.obj.symbols.get(index)
        if sym is None:
            return None
        if sym.section > 0:
            base = self.section_addr.get(sym.section)
            return None if base is None else base + sym.value
        if sym.section == -1:
            return sym.value
        return self.extern_addr.get(sym.name)

    def _apply_relocations(self) -> None:
        for sect in self.obj.sections:
            base = self.section_addr.get(sect.index)
            if base is None:
                continue
            region = self._region(base)
            for rel in sect.relocations:
                target = self.symbol_address(rel.symbol)
                site = base + rel.offset
                off = site - region.start
                if target is None:
                    continue
                (addend,) = struct.unpack_from("<i", region.data, off)
                if rel.type == msvc_coff.IMAGE_REL_I386_DIR32:
                    value = (target + addend) & 0xFFFFFFFF
                    struct.pack_into("<I", region.data, off, value)
                elif rel.type == msvc_coff.IMAGE_REL_I386_DIR32NB:
                    struct.pack_into("<I", region.data, off, (target + addend - IMAGEBASE) & 0xFFFFFFFF)
                elif rel.type == msvc_coff.IMAGE_REL_I386_REL32:
                    struct.pack_into("<i", region.data, off, target + addend - (site + 4))


# ---------------------------------------------------------------------------
# Recomp entities (the equivalent of Reccmp's load_cvdump for one object)


def collect_nodes(img: SyntheticImage) -> tuple[dict[int, Node], list[int]]:
    from reccmp.cvdump.analysis import get_size_from_float_symbol
    from reccmp.cvdump.demangler import demangle_vtable
    from reccmp.types import EntityType

    obj = img.obj
    nodes: dict[int, Node] = {}
    function_starts: list[int] = []

    def node(addr: int) -> Node:
        return nodes.setdefault(addr, Node(addr))

    # Defined symbols give the publics' view.
    for sym in obj.symbols.values():
        if sym.section <= 0 or sym.section not in img.section_addr:
            continue
        if sym.storage_class not in (msvc_coff.IMAGE_SYM_CLASS_EXTERNAL, msvc_coff.IMAGE_SYM_CLASS_STATIC):
            continue
        if sym.aux and sym.value == 0 and sym.name == obj.section(sym.section).name:
            continue  # section definition
        addr = img.section_addr[sym.section] + sym.value
        name = sym.name
        if name.startswith("??_C@_0"):
            n = node(addr)
            n.type, n.symbol, n.size = EntityType.STRING, name, obj.section(sym.section).size
        elif name.startswith("??_C@_1"):
            n = node(addr)
            n.type, n.symbol, n.size = EntityType.WIDECHAR, name, obj.section(sym.section).size
        elif name.startswith("__real@"):
            size = get_size_from_float_symbol(name)
            if size is not None:
                n = node(addr)
                n.type, n.symbol, n.size = EntityType.FLOAT, name, size
        elif name.startswith("??_7"):
            n = node(addr)
            n.type, n.symbol, n.name = EntityType.VTABLE, name, demangle_vtable(name)
        elif sym.is_external:
            n = node(addr)
            if n.symbol is not None and n.symbol != name:
                # Aliases (e.g. __strcmpi/__stricmp): keep the one an
                # annotation asks for.
                if n.symbol in img.ctx.nameref_names or name not in img.ctx.nameref_names:
                    continue
                n.name = None
            n.symbol = name
            if n.name is None:
                n.name = name
                if not name.startswith("?") and "@" in name:
                    n.type = EntityType.FUNCTION

    cv_data: set[int] = set()
    for proc in obj.procs:
        addr = img.symbol_address(proc.symbol)
        if addr is None:
            continue
        addr += proc.offset
        n = node(addr)
        n.type, n.name, n.size = EntityType.FUNCTION, proc.name, proc.length
        function_starts.append(addr)
        for var in proc.static_locals:
            vaddr = img.symbol_address(var.symbol)
            if vaddr is None:
                continue
            vaddr += var.offset
            v = node(vaddr)
            v.type, v.name = EntityType.DATA, var.name
            cv_data.add(vaddr)
            v.symbol = f"{var.name}___{proc.name}"

    for var in obj.data:
        sym = obj.symbols.get(var.symbol)
        if sym is None or sym.is_undefined:
            continue
        addr = img.symbol_address(var.symbol)
        if addr is None:
            continue
        addr += var.offset
        n = node(addr)
        n.type, n.name = EntityType.DATA, var.name
        cv_data.add(addr)

    # Estimated sizes, as CvdumpAnalysis makes them: the distance to the next
    # known symbol (publics, CodeView procedures and data) in the same section.
    by_region: dict[int, list[int]] = {}
    for addr in nodes:
        region = img._region(addr)  # pylint: disable=protected-access
        if region.kind in ("code", "rdata", "data"):
            by_region.setdefault(region.start, []).append(addr)
    estimates: dict[int, int] = {}
    for start, addrs in by_region.items():
        addrs = sorted(addrs)
        end = img._region(start).end  # pylint: disable=protected-access
        for a, b in zip(addrs, addrs[1:] + [end]):
            estimates[a] = b - a

    # CodeView variables have a type size in the linker PDB. Without the
    # types, the next COFF symbol of any kind (labels, $T/$S statics, other
    # variables) is the closest bound available.
    all_symbols: dict[int, list[int]] = {}
    for sym in obj.symbols.values():
        if sym.section > 0 and sym.section in img.section_addr:
            start = img.section_addr[sym.section]
            all_symbols.setdefault(start, []).append(start + sym.value)
    tight: dict[int, int] = {}
    for start, addrs in all_symbols.items():
        addrs = sorted(set(addrs) | set(by_region.get(start, [])))
        end = img._region(start).end  # pylint: disable=protected-access
        for a, b in zip(addrs, addrs[1:] + [end]):
            tight[a] = b - a

    for addr, n in nodes.items():
        if n.size is None and n.type != EntityType.FUNCTION:
            n.size = tight.get(addr) if addr in cv_data else estimates.get(addr)

    # Library objects have no CodeView: size functions by the next symbol.
    if not obj.procs:
        for addr, n in nodes.items():
            if img._region(addr).kind == "code" and n.symbol:  # pylint: disable=protected-access
                n.size = estimates.get(addr)
                function_starts.append(addr)

    nodes.update(img.nodes)
    return nodes, function_starts
