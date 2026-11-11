# SPDX-License-Identifier: LGPL-3.0-or-later
"""Data accesses of an original translation unit's functions (issue #405).

Disassembles the unit's functions in ``reference/LegoRR.exe`` and lists every
absolute data address they touch with the width and kind of the access. The
scaffold tool lays out the unit's globals from this, and the footprint uses it
to find globals that belong to other units.

Requires ``pefile`` and ``capstone`` (the same as ``translation_units.py``).
"""

from __future__ import annotations

import sys
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import translation_units as tu  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent


# An immediate is an address only where code loads, pushes or compares one. An
# `imul` or `and` immediate that happens to fall in the data span (the hash
# multiplier 0x600937 in finding 509) is a number.
ADDRESS_MNEMONICS = frozenset({"push", "mov", "cmp"})


@dataclass(frozen=True)
class Access:
    address: int      # absolute data address
    size: int         # bytes read or written; 0 when the address is only taken
    kind: str         # "int", "float" (x87 operand) or "addr" (lea / immediate)
    scale: int        # index scale when the access is indexed (1 for base+offset), else 0
    function: int     # address of the function containing the instruction
    insn: int         # instruction address
    region: str       # rdata, data, bss or common (finding 505 section layout)


def _region(addr: int) -> str | None:
    if tu.RDATA[0] <= addr < tu.RDATA[1]:
        return "rdata"
    if tu.DATA_INIT[0] <= addr < tu.DATA_INIT[1]:
        return "data"
    if tu.BSS_OBJ[0] <= addr < tu.BSS_OBJ[1]:
        return "bss"
    if tu.COMMON[0] <= addr < tu.COMMON[1]:
        return "common"
    return None


def open_binary(root: Path = ROOT, exe: Path | None = None) -> "tu.Binary":
    return tu.Binary(exe or root / "reference/LegoRR.exe")


def scan_unit_accesses(
    function_ranges: list[tuple[int, int]],
    root: Path = ROOT,
    binary: "tu.Binary | None" = None,
) -> list[Access]:
    """Accesses made by the functions in ``[(start, end), ...]`` (text ranges)."""
    from capstone import CS_ARCH_X86, CS_MODE_32, Cs
    from capstone.x86 import X86_OP_IMM, X86_OP_MEM

    binary = binary or open_binary(root)
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    found: dict[tuple[int, int], Access] = {}

    for start, nxt in function_ranges:
        code = binary.read(start, nxt - start)
        seen: set[int] = set()
        work = [start]
        while work:
            a = work.pop()
            while start <= a < nxt and a not in seen:
                ins = next(md.disasm(code[a - start:a - start + 16], a), None)
                if ins is None:
                    break
                seen.add(a)
                m = ins.mnemonic
                for op in ins.operands:
                    if op.type == X86_OP_MEM:
                        addr = op.mem.disp & 0xFFFFFFFF
                        region = _region(addr)
                        if region is None:
                            continue
                        if m == "lea":
                            kind, size = "addr", 0
                        elif m.startswith("f") and not m.startswith("fi"):
                            kind, size = "float", op.size
                        else:
                            kind, size = "int", op.size
                        # `[reg*4 + base]` is indexed; so is `[reg + base]` (a computed
                        # byte offset), reported as scale 1.
                        scale = op.mem.scale if op.mem.index else (1 if op.mem.base else 0)
                        found.setdefault((addr, a), Access(addr, size, kind, scale, start, a, region))
                    elif op.type == X86_OP_IMM and m in ADDRESS_MNEMONICS:
                        addr = op.imm & 0xFFFFFFFF
                        region = _region(addr)
                        # `mov edi, 0x600937` loads the hash multiplier of finding
                        # 509. An unaligned immediate in the common region is a number.
                        if region == "common" and addr & 3:
                            continue
                        if region is not None:
                            found.setdefault((addr, a), Access(addr, 0, "addr", 0, start, a, region))
                if m == "call" and ins.operands and ins.operands[0].type == X86_OP_IMM:
                    pass
                if m.startswith("j") and ins.operands and ins.operands[0].type == X86_OP_IMM:
                    t = ins.operands[0].imm
                    if start <= t < nxt:
                        work.append(t)
                    if m == "jmp":
                        break
                elif m == "jmp":
                    op = ins.operands[0] if ins.operands else None
                    if op is not None and op.type == X86_OP_MEM and start <= op.mem.disp < nxt:
                        p = op.mem.disp            # in-function jump table
                        while start <= p < nxt:
                            t = int.from_bytes(binary.read(p, 4), "little")
                            if not start <= t < nxt:
                                break
                            work.append(t)
                            p += 4
                    break
                if m in ("ret", "retn", "int3", "hlt"):
                    break
                a += ins.size
    return sorted(found.values(), key=lambda x: (x.address, x.insn))
