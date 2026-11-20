#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Where an IA-32 instruction's 32-bit operand fields sit (tools/reprobit_reference.py).

`decode(code, offset)` decodes one instruction with the opcode tables of
ReproBit's fail-closed decoder (`src/reprobit/ia32_decode.py`,
`supported_ia32_instruction_length`), so it accepts exactly the encodings
ReproBit's donor rewriting accepts and gives the same lengths
(tools/tests/test_ia32_fields.py checks both against the fork when it is
importable). On top of the length it reports:

- the instruction's 32-bit fields that can hold an image address: a
  ModRM ``disp32``, a ``moffs32`` (``A0``-``A3``), a 32-bit immediate, and a
  ``rel32`` branch displacement;
- a *shape*: the prefixes, the opcode, the ModRM addressing form and opcode
  extension, and the field kinds, with registers, displacements and
  immediates left out. Two compiles of one statement under different
  register choices or stack frames have the same shapes, which is what the
  synthetic reference aligns.
"""
from __future__ import annotations

from dataclasses import dataclass

PREFIXES = frozenset({0x26, 0x2E, 0x36, 0x3E, 0x64, 0x65, 0x66, 0xF0, 0xF2, 0xF3})


def _one_byte_table() -> dict[int, tuple[str, ...]]:
    table: dict[int, tuple[str, ...]] = {}
    for base in range(0, 64, 8):
        for low in range(4):
            table[base + low] = ("M",)
        table[base + 4] = ("Ib",)
        table[base + 5] = ("Iz",)
    for opcode in (6, 7, 14, 22, 23, 30, 31, 39, 47, 55, 63):
        table[opcode] = ()
    for opcode in range(64, 98):
        table[opcode] = ()
    table.update({98: ("M",), 99: ("M",), 104: ("Iz",), 105: ("M", "Iz"), 106: ("Ib",),
                  107: ("M", "Ib")})
    for opcode in range(108, 112):
        table[opcode] = ()
    for opcode in range(112, 128):
        table[opcode] = ("Jb",)
    table.update({128: ("M", "Ib"), 129: ("M", "Iz"), 130: ("M", "Ib"), 131: ("M", "Ib")})
    for opcode in range(132, 144):
        table[opcode] = ("M",)
    for opcode in range(144, 154):
        table[opcode] = ()
    table[154] = ("Ap",)
    for opcode in range(155, 160):
        table[opcode] = ()
    for opcode in range(160, 164):
        table[opcode] = ("Ov",)
    for opcode in range(164, 168):
        table[opcode] = ()
    table[168] = ("Ib",)
    table[169] = ("Iz",)
    for opcode in range(170, 176):
        table[opcode] = ()
    for opcode in range(176, 184):
        table[opcode] = ("Ib",)
    for opcode in range(184, 192):
        table[opcode] = ("Iz",)
    table.update({192: ("M", "Ib"), 193: ("M", "Ib"), 194: ("Iw",), 195: (), 196: ("M",),
                  197: ("M",), 198: ("M", "Ib"), 199: ("M", "Iz"), 200: ("Iw", "Ib"), 201: (),
                  202: ("Iw",), 203: (), 204: (), 205: ("Ib",), 206: (), 207: ()})
    for opcode in range(208, 212):
        table[opcode] = ("M",)
    table[212] = ("Ib",)
    table[213] = ("Ib",)
    table[215] = ()
    for opcode in range(216, 224):
        table[opcode] = ("M",)
    for opcode in range(224, 228):
        table[opcode] = ("Jb",)
    for opcode in range(228, 232):
        table[opcode] = ("Ib",)
    table[232] = ("Jz",)
    table[233] = ("Jz",)
    table[234] = ("Ap",)
    table[235] = ("Jb",)
    for opcode in range(236, 240):
        table[opcode] = ()
    table[244] = ()
    table[245] = ()
    table[246] = ("M", "F6")
    table[247] = ("M", "F7")
    for opcode in range(248, 254):
        table[opcode] = ()
    table[254] = ("M",)
    table[255] = ("M",)
    return table


def _two_byte_table() -> dict[int, tuple[str, ...]]:
    table: dict[int, tuple[str, ...]] = {}
    for opcode in (0, 1, 2, 3, 13, 24, 31):
        table[opcode] = ("M",)
    for opcode in (6, 8, 9, 11, 48, 49, 50, 51, 119, 160, 161, 162, 168, 169, 170):
        table[opcode] = ()
    for opcode in (*range(16, 24), *range(32, 36), *range(40, 48), *range(64, 112)):
        table[opcode] = ("M",)
    for opcode in range(112, 116):
        table[opcode] = ("M", "Ib")
    for opcode in range(116, 119):
        table[opcode] = ("M",)
    table[126] = ("M",)
    table[127] = ("M",)
    for opcode in range(128, 144):
        table[opcode] = ("Jz",)
    for opcode in range(144, 160):
        table[opcode] = ("M",)
    table.update({163: ("M",), 164: ("M", "Ib"), 165: ("M",), 171: ("M",), 172: ("M", "Ib"),
                  173: ("M",), 174: ("M",), 175: ("M",)})
    for opcode in range(176, 184):
        table[opcode] = ("M",)
    table[185] = ("M",)
    table[186] = ("M", "Ib")
    for opcode in range(187, 194):
        table[opcode] = ("M",)
    table.update({194: ("M", "Ib"), 195: ("M",), 196: ("M", "Ib"), 197: ("M", "Ib"),
                  198: ("M", "Ib"), 199: ("M",)})
    for opcode in range(200, 208):
        table[opcode] = ()
    for opcode in range(208, 256):
        table[opcode] = ("M",)
    return table


ONE_BYTE = _one_byte_table()
TWO_BYTE = _two_byte_table()
# Opcodes whose ModRM reg field selects the operation (an opcode extension),
# so the shape keeps it.
_GROUP_ONE = frozenset({0x80, 0x81, 0x82, 0x83, 0x8F, 0xC0, 0xC1, 0xC6, 0xC7, 0xD0, 0xD1,
                        0xD2, 0xD3, *range(0xD8, 0xE0), 0xF6, 0xF7, 0xFE, 0xFF})
_GROUP_TWO = frozenset({0x00, 0x01, 0x18, 0x1F, 0x71, 0x72, 0x73, 0xBA, 0xC7})
# x87 register forms whose ModRM rm also names the operation (fchs, fld1,
# fnstsw ax, fcompp, ...), not a stack register.
_X87_RM_OPERATION = {0xD9: {4, 5, 6, 7}, 0xDA: {5}, 0xDB: {4}, 0xDE: {3}, 0xDF: {4}}


class DecodeError(ValueError):
    pass


@dataclass(frozen=True)
class Field:
    offset: int  # of the field's first byte, in the code passed to decode()
    kind: str  # "disp32" (ModRM or moffs32), "imm32" or "rel32"
    absolute: bool = False  # a disp32 with no base register (an index may scale)


@dataclass(frozen=True)
class Instruction:
    offset: int
    length: int
    shape: tuple
    fields: tuple[Field, ...]

    @property
    def end(self) -> int:
        return self.offset + self.length


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise DecodeError(message)


def decode(code: bytes, offset: int) -> Instruction:
    """The instruction at ``code[offset:]``, or DecodeError (as ReproBit refuses it)."""
    at = f"instruction at {offset}"
    cursor = offset
    operand_16 = False
    prefixes: list[int] = []
    while cursor < len(code) and code[cursor] in PREFIXES:
        _require(code[cursor] not in prefixes, f"{at}: repeated prefix")
        prefixes.append(code[cursor])
        operand_16 = operand_16 or code[cursor] == 0x66
        cursor += 1
    _require(cursor < len(code), f"{at}: truncated prefix")
    _require(len(prefixes) <= 4, f"{at}: prefix run")
    opcode = (code[cursor],)
    cursor += 1
    if opcode[0] == 0x0F:
        _require(cursor < len(code), f"{at}: truncated two-byte opcode")
        opcode = (0x0F, code[cursor])
        cursor += 1
        operands = TWO_BYTE.get(opcode[1])
        group = opcode[1] in _GROUP_TWO
        _require(operands is not None, f"{at}: unsupported two-byte opcode {opcode[1]:#04x}")
    else:
        operands = ONE_BYTE.get(opcode[0])
        group = opcode[0] in _GROUP_ONE
        _require(operands is not None, f"{at}: unsupported opcode {opcode[0]:#04x}")
    assert operands is not None
    form: tuple = ()
    fields: list[Field] = []
    modrm = 0
    for token in operands:
        if token == "M":
            _require(cursor < len(code), f"{at}: lacks ModRM")
            modrm = code[cursor]
            cursor += 1
            mode, reg, rm = modrm >> 6, modrm >> 3 & 7, modrm & 7
            sib = mode != 3 and rm == 4
            absolute = False
            if sib:
                _require(cursor < len(code), f"{at}: lacks SIB")
                base = code[cursor] & 7
                cursor += 1
                absolute = mode == 0 and base == 5
            else:
                absolute = mode == 0 and rm == 5
            if absolute or mode == 2:
                fields.append(Field(cursor, "disp32", absolute))
                cursor += 4
            elif mode == 1:
                cursor += 1
            extension = reg if group else None
            if mode == 3 and len(opcode) == 1 and reg in _X87_RM_OPERATION.get(opcode[0], ()):
                extension = (reg, rm)
            form = (mode, sib, absolute, extension)
        elif token in ("F6", "F7"):
            if modrm >> 3 & 7 in (0, 1):
                if token == "F7" and not operand_16:
                    fields.append(Field(cursor, "imm32"))
                cursor += 1 if token == "F6" else 2 if operand_16 else 4
        elif token in ("Ib", "Jb"):
            cursor += 1
        elif token == "Iw":
            cursor += 2
        elif token in ("Iz", "Jz"):
            if not operand_16:
                fields.append(Field(cursor, "imm32" if token == "Iz" else "rel32"))
            cursor += 2 if operand_16 else 4
        elif token == "Ov":
            fields.append(Field(cursor, "disp32", True))
            cursor += 4
        elif token == "Ap":
            cursor += 6
        else:  # pragma: no cover - table error
            raise DecodeError(f"{at}: decoder table error")
    _require(cursor <= len(code), f"{at}: truncated")
    _require(cursor - offset <= 15, f"{at}: longer than 15 bytes")
    shape = (tuple(prefixes), opcode, form, tuple((f.kind, f.absolute) for f in fields))
    return Instruction(offset, cursor - offset, shape, tuple(fields))
