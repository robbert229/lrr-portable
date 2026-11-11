#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Reproduce the two-sided GAME-021 boundary evidence (requires pefile/capstone)."""
import struct
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tools'))
import translation_units as tu

START, SPLIT, END = 0x41f870, 0x42f350, 0x435b20
binary = tu.Binary(ROOT / 'reference/LegoRR.exe')
funcs = tu.load_functions(binary)
ev = tu.Evidence(binary, funcs)
selected = [f for f in funcs if START <= f.address < END]
print('Functions:', sum(f.address < SPLIT for f in selected), sum(f.address >= SPLIT for f in selected))
for boundary in (0x41f870, 0x42f2e0, SPLIT, 0x4065f0, 0x406990, END):
    prev = next((f for f in funcs if f.next == boundary), None)
    if prev:
        print('Boundary %08x: preceding %08x end %08x fill %s %s' %
              (boundary, prev.address, prev.end, prev.fill, prev.pad.hex()))
print('\nCross-boundary direct calls:')
for f in selected:
    for dest in sorted(set(f.calls)):
        if START <= dest < END and (f.address < SPLIT) != (dest < SPLIT):
            print('%08x %s -> %08x %s' % (f.address, f.name, dest, ev.by_addr[dest].name if dest in ev.by_addr else '?'))
for region in ('rdata', 'string', 'data', 'bss', 'common'):
    print('\n%s: exact operand addresses, L/R function counts, first users' % region)
    addresses = sorted({a for f in selected for a in ev.refs[f.index][region]})
    for a in addresses:
        users = [f for f in selected if a in ev.refs[f.index][region]]
        left = [f for f in users if f.address < SPLIT]
        right = [f for f in users if f.address >= SPLIT]
        first = min(ev.refby[a], key=lambda i: funcs[i].address)
        value = binary.cstring(a) if region == 'string' else ''
        if region == 'rdata':
            raw = binary.read(a, 4)
            value = '%s float=%g' % (raw.hex(), struct.unpack('<f', raw)[0])
        print('%08x %3d %3d first=%08x %s' % (a, len(left), len(right), funcs[first].address, repr(value) if value else ''))
print('\nD3DRM copies in .rdata:')
pat = bytes.fromhex('010000000200000004000000ffffffff')
raw = binary.read(*[tu.RDATA[0], tu.RDATA[1] - tu.RDATA[0]])
off = raw.find(pat)
while off >= 0:
    print('%08x' % (tu.RDATA[0] + off))
    off = raw.find(pat, off + 1)

# Jump tables are embedded in .text, not TU-specific metadata.
from capstone import CS_ARCH_X86, CS_MODE_32, Cs
md = Cs(CS_ARCH_X86, CS_MODE_32)
print('\nIndirect jumps in candidate function 0042f350:')
for ins in md.disasm(binary.read(SPLIT, 0x42f6f0 - SPLIT), SPLIT):
    if ins.mnemonic == 'jmp' and '[' in ins.op_str:
        print('%08x %s %s' % (ins.address, ins.mnemonic, ins.op_str))
