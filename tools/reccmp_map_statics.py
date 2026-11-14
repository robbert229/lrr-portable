#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Name LIBC.LIB's static functions on the rebuilt side from the link map.

Issue #447, finding 643. The matching build takes the whole C runtime from
LIBC.LIB. LIBC's objects carry no debug information, so the rebuilt PDB has a
public symbol for each external LIBC function but none for a static one, such
as ``cvt.obj``'s ``_shift`` (``__shift``) or ``frame.obj``'s ``FindHandler``.
A ``LIBRARY ... SYMBOL`` marker or a ``src/crt/libc_symbols.csv`` row that
names such a static therefore stays unmapped, although the function is linked.

LINK writes every static function to the map, under "Static symbols", with
its address and its object (``libc:cvt.obj``). A member reached through
``oldnames.lib`` has two publics at one address (``_strupr``, ``__strupr``),
and the PDB names only one of them; the map has both. Before Reccmp matches symbols,
this module adds a rebuilt-side entity for each LIBC static function that an
original-side entity asks for by symbol, at the map's address. Nothing else
changes: a static that no marker or data source names stays anonymous, so the
operands that refer to it compare as before. A name that the map gives to
more than one static (``_strncnt`` in two objects) is skipped.

Static *data* is not in the map. For a static data symbol that an
original-side entity names (``_get_fname``'s handler table, finding 643), the
address comes from the LIBC member's own COFF object instead: a function of
that member whose address the map gives refers to the data through a DIR32
relocation, so the rebuilt operand minus the object's addend is the data's
rebuilt address. The library is the toolchain's ``lib/libc.lib``
(``RECCMP_LIBC_LIB`` overrides it).

This is not a scoring deviation: it reads the link's own map and the
library it linked, and each name lands on what LIBC linked there. ``RECCMP_MAP_STATICS=0``
turns it off, to measure what it changes.
"""
from __future__ import annotations

import os
import re
from collections import Counter
from pathlib import Path

# Set on the wrapper, so repeated installs are no-ops.
_INSTALLED = "_lrr_map_statics_installed"

# " 0001:00146130       __shift                    00547130 f libc:cvt.obj"
_SYMBOL = re.compile(
    r"^\s*([0-9a-fA-F]{4}):([0-9a-fA-F]{8})\s+(\S+)\s+([0-9a-fA-F]{8})"
    r"((?:\s+[fi])*)\s+(\S+)\s*$")


def parse_map(text: str) -> tuple[list[tuple[int, str, str, bool]], list[tuple[int, str, str, bool]]]:
    """Public and static symbols of a LINK 5.0 map.

    Each is (address, symbol, object, is_function), in map order."""
    publics: list[tuple[int, str, str, bool]] = []
    statics: list[tuple[int, str, str, bool]] = []
    target = publics
    for line in text.splitlines():
        if line.strip() == "Static symbols":
            target = statics
            continue
        m = _SYMBOL.match(line)
        if not m:
            continue
        flags = m.group(5).split()
        target.append((int(m.group(4), 16), m.group(3), m.group(6), "f" in flags))
    return publics, statics


def library_statics(text: str, library: str = "libc") -> dict[str, tuple[int, int | None]]:
    """``{symbol: (address, size)}`` for the unique static functions of a
    library's members, and for its public functions that share their address
    with another public: an old name that oldnames.lib aliases (``_strupr``
    and ``__strupr``), of which the PDB keeps one. The size runs to the next
    symbol in the map; it is None for the last one."""
    publics, statics = parse_map(text)
    prefix = library.lower() + ":"
    shared = Counter(s[0] for s in publics)
    wanted = [s for s in statics + [p for p in publics if shared[p[0]] > 1]
              if s[3] and s[2].lower().startswith(prefix)]
    counts = Counter(s[1] for s in wanted)
    section_of = {int(m.group(4), 16): m.group(1)
                  for m in map(_SYMBOL.match, text.splitlines()) if m}
    starts = sorted({s[0] for s in publics + statics})
    result: dict[str, tuple[int, int | None]] = {}
    for addr, symbol, _obj, _is_function in wanted:
        if counts[symbol] != 1:
            continue
        later = [a for a in starts if a > addr and section_of.get(a) == section_of.get(addr)]
        result[symbol] = (addr, (later[0] - addr) if later else None)
    return result


IMAGE_REL_I386_DIR32 = 0x0006


def library_member_functions(text: str, library: str = "libc") -> dict[str, dict[str, int]]:
    """``{member basename: {symbol: address}}`` for the map's functions."""
    publics, statics = parse_map(text)
    prefix = library.lower() + ":"
    result: dict[str, dict[str, int]] = {}
    for addr, symbol, obj, is_function in publics + statics:
        if is_function and obj.lower().startswith(prefix):
            result.setdefault(obj.split(":", 1)[1].lower(), {})[symbol] = addr
    return result


def static_data(wanted: set[str], members, read_rebuilt) -> dict[str, tuple[int, int]]:
    """Rebuilt ``(address, size)`` of the static data symbols in ``wanted``.
    The size runs to the next symbol in the object's section.

    ``members`` yields ``(functions, coff)`` per linked library member, with
    ``functions`` the map's ``{symbol: address}`` for it; ``read_rebuilt(va,
    4)`` reads the rebuilt image. A symbol that two members define, or whose
    references disagree, is left out."""
    import struct

    found: dict[str, set[int]] = {}
    sizes: dict[str, int] = {}
    for functions, coff in members:
        symbols = coff.symbols
        statics = {s.name: s for s in symbols.values()
                   if s.name in wanted and s.section > 0 and not s.is_function}
        if not statics:
            continue
        for sym in statics.values():
            later = [s.value for s in symbols.values()
                     if s.section == sym.section and s.value > sym.value]
            sizes[sym.name] = (min(later) if later else coff.section(sym.section).size) - sym.value
        section_syms = {i: s for i, s in symbols.items()
                        if s.section > 0 and s.value == 0 and s.name.startswith(".")}
        for sect in coff.sections:
            if not sect.is_code or not sect.relocations:
                continue
            entries = sorted((s.value, s.name) for s in symbols.values()
                             if s.section == sect.index and s.is_function
                             and s.name in functions)
            for reloc in sect.relocations:
                if reloc.type != IMAGE_REL_I386_DIR32:
                    continue
                target = symbols.get(reloc.symbol)
                if target is None:
                    continue
                owner = [e for e in entries if e[0] <= reloc.offset]
                if not owner:
                    continue
                start, name = owner[-1]
                (addend,) = struct.unpack_from("<I", sect.data, reloc.offset)
                (operand,) = struct.unpack("<I", read_rebuilt(functions[name] + reloc.offset - start, 4))
                if target.name in statics and target is statics[target.name]:
                    found.setdefault(target.name, set()).add(operand - addend)
                elif reloc.symbol in section_syms:
                    base = operand - addend
                    for sym in statics.values():
                        if sym.section == target.section:
                            found.setdefault(sym.name, set()).add(base + sym.value)
    return {name: (addrs.pop(), sizes[name]) for name, addrs in found.items() if len(addrs) == 1}


def default_libc_path() -> Path:
    """The toolchain's LIBC.LIB, as tools/reccmp.sh finds the toolchain."""
    path = os.environ.get("RECCMP_LIBC_LIB")
    if path:
        return Path(path)
    root = os.environ.get("REPROBIT_MSVC_5_0_RTM_ROOT") or os.path.expanduser(
        "~/.local/share/reprobit/toolchains/msvc_5_0_rtm")
    return Path(root) / "lib" / "libc.lib"


def linked_members(map_text: str, library_path: Path, wanted: set[str]):
    """``(functions, coff)`` for each linked LIBC member that defines one of
    ``wanted``."""
    import msvc_coff

    functions = library_member_functions(map_text)
    data = library_path.read_bytes()
    for member in msvc_coff._archive_members(data):
        base = msvc_coff.member_basename(member).lower()
        if base not in functions or not base.endswith(".obj"):
            continue
        if not any(w.encode("latin1") in member.data for w in wanted):
            continue
        yield functions[base], msvc_coff.parse_coff(member.data, base)


def default_map_path(recomp_path: Path) -> Path:
    """The gate writes LegoRR.map next to LegoRR.exe (CMakeLists.txt)."""
    return Path(recomp_path).with_suffix(".map")


def asked_symbols(db, image_ids) -> set[str]:
    """Symbols that unmatched original-side entities ask for."""
    asked = set()
    for ent in db.unmatched(image_ids.ORIG):
        symbol = ent.get("symbol")
        if symbol:
            asked.add(symbol)
    return asked


def add_statics(db, statics: dict[str, tuple[int, int | None]], image_ids, entity_function,
                data: dict[str, tuple[int, int]] | None = None, entity_data=None) -> list[str]:
    """Add a rebuilt-side entity for each static an original entity names:
    functions from ``statics``, data from ``data``.

    Returns the symbols added."""
    asked = asked_symbols(db, image_ids)
    known = set()
    for ent in db.unmatched(image_ids.RECOMP):
        symbol = ent.get("symbol")
        if symbol:
            known.add(symbol)
    added = []
    # A static function starts inside the PDB's extent of the public before
    # it when that public's size is its object's whole section contribution
    # (an .asm member: __unwind_handler follows __global_unwind2 in
    # exsup.obj). The map proves where the static starts, so the public ends
    # there.
    extents = [(ent.recomp_addr, ent.size(image_ids.RECOMP)) for ent in db.get_all()
               if ent.recomp_addr is not None and ent.size(image_ids.RECOMP)]
    with db.batch() as batch:
        for symbol in sorted(asked & set(statics)):
            if symbol in known:
                continue
            addr, size = statics[symbol]
            values = {"type": entity_function, "name": symbol, "symbol": symbol}
            if size is not None:
                values["size"] = size
            batch.set(image_ids.RECOMP, addr, **values)
            for start, length in extents:
                if start < addr < start + length:
                    batch.set(image_ids.RECOMP, start, size=addr - start)
            added.append(symbol)
        for symbol in sorted(asked & set(data or {})):
            if symbol in known or symbol in statics:
                continue
            addr, size = data[symbol]
            batch.set(image_ids.RECOMP, addr, type=entity_data,
                      name=symbol, symbol=symbol, size=size)
            added.append(symbol)
    return added


def wrap_match_symbols(match_symbols, get_statics, image_ids, entity_function,
                       get_data=None, entity_data=None):
    """Wrap Reccmp's ``match_symbols(db, report, *, truncate)`` so that the
    LIBC statics are named first."""
    if getattr(match_symbols, _INSTALLED, False):
        return match_symbols

    def match_with_statics(db, *args, **kwargs):
        statics = get_statics()
        data = get_data(asked_symbols(db, image_ids) - set(statics)) if get_data else {}
        if statics or data:
            add_statics(db, statics, image_ids, entity_function, data, entity_data)
        return match_symbols(db, *args, **kwargs)

    setattr(match_with_statics, _INSTALLED, True)
    match_with_statics.__wrapped__ = match_symbols
    return match_with_statics


def install() -> None:
    """Patch the names Reccmp's Compare calls (reccmp.compare.core)."""
    if os.environ.get("RECCMP_MAP_STATICS", "1") == "0":
        return
    import reccmp.compare.core as core
    from reccmp.types import EntityType, ImageId

    state: dict[str, object] = {}
    original_init = core.Compare.__init__
    if not getattr(original_init, _INSTALLED, False):
        def init(self, *args, **kwargs):
            original_init(self, *args, **kwargs)
            state["recomp"] = getattr(self.recomp_bin, "filepath", None)
            state["image"] = self.recomp_bin
        setattr(init, _INSTALLED, True)
        core.Compare.__init__ = init

    def map_text():
        recomp = state.get("recomp")
        path = os.environ.get("RECCMP_MAP")
        map_path = Path(path) if path else (default_map_path(recomp) if recomp else None)
        if map_path is None or not map_path.is_file():
            return None
        return map_path.read_text(encoding="latin-1")

    def get_statics():
        text = map_text()
        return library_statics(text) if text else {}

    def get_data(wanted):
        text = map_text()
        library = default_libc_path()
        image = state.get("image")
        if not wanted or not text or image is None or not library.is_file():
            return {}
        return static_data(wanted, linked_members(text, library, wanted), image.read)

    core.match_symbols = wrap_match_symbols(core.match_symbols, get_statics,
                                            ImageId, EntityType.FUNCTION,
                                            get_data, EntityType.DATA)
