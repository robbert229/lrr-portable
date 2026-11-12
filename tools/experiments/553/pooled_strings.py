#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Finding 569: how LegoRR.exe stores repeated string literals.

Scans .text for 4-byte values that point at the start of a printable,
NUL-terminated string in .data or .rdata, attributes each site to its
function and unit (docs/coverage/translation-units.csv), and reports
literals shared across units, texts stored more than once, and pooled
literals owned by a converted (.c) unit that no source annotation marks.

    python3 -I tools/experiments/553/pooled_strings.py [ROOT]
"""
import bisect
import collections
import csv
import re
import sys
from pathlib import Path

import pefile

root = Path(sys.argv[1] if len(sys.argv) > 1 else '.')
pe = pefile.PE(str(root / 'reference/LegoRR.exe'))
base = pe.OPTIONAL_HEADER.ImageBase
blobs = {}
for s in pe.sections:
    name = s.Name.rstrip(b'\0').decode()
    blobs[name] = (base + s.VirtualAddress, s.get_data()[:s.SizeOfRawData])
text_va, text = blobs['.text']

funcs, sources = [], collections.defaultdict(set)
with open(root / 'docs/coverage/translation-units.csv', newline='') as f:
    for r in csv.DictReader(f):
        funcs.append((int(r['address'], 16), r['unit_id']))
        sources[r['unit_id']].add(r['current_source'])
funcs.sort()
starts = [a for a, _ in funcs]


def function_at(addr):
    i = bisect.bisect_right(starts, addr) - 1
    return funcs[i] if i >= 0 else (None, None)


def string_at(va, section):
    lo, b = blobs[section]
    off = va - lo
    if not 0 <= off < len(b) or (off and b[off - 1] and va % 4):
        return None
    end = b.find(b'\0', off)
    s = b[off:end] if end >= 0 else None
    if not s or any((c < 0x20 and c not in (9, 10, 13)) or c > 0x7e for c in s):
        return None
    return s


refs = collections.defaultdict(list)
for i in range(len(text) - 3):
    v = int.from_bytes(text[i:i + 4], 'little')
    for section in ('.data', '.rdata'):
        lo, b = blobs[section]
        if lo <= v < lo + len(b) and string_at(v, section) is not None:
            refs[(v, section)].append(function_at(text_va + i))

by_text = collections.defaultdict(set)
for (v, section) in refs:
    by_text[string_at(v, section)].add((v, section))
shared = [(k, s) for k, s in refs.items() if len({u for _, u in s}) > 1]
print('referenced string addresses: %d (.data %d, .rdata %d)' % (
    len(refs), sum(k[1] == '.data' for k in refs), sum(k[1] == '.rdata' for k in refs)))
print('used by >1 function: %d; by >1 unit: %d' % (
    sum(len({f for f, _ in s}) > 1 for s in refs.values()), len(shared)))
print('texts stored at >1 address:')
for t, addrs in sorted(by_text.items()):
    if len(addrs) > 1:
        print('  %r: %s' % (t, ', '.join('%08x%s' % (v, '' if s == '.data' else ' (.rdata)')
                                        for v, s in sorted(addrs))))
print('most shared across units:')
for (v, section), sites in sorted(shared, key=lambda x: -len(x[1]))[:12]:
    print('  %08x %-14r sites=%d units=%d' % (v, string_at(v, section), len(sites),
                                              len({u for _, u in sites})))

marked = set()
for path in (root / 'src').rglob('*'):
    if path.suffix in ('.c', '.cpp', '.h', '.hpp'):
        marked.update(int(m, 16) for m in re.findall(
            r'//\s*(?:STRING|GLOBAL):\s*LEGORR\s+0x([0-9a-f]+)', path.read_text(errors='replace')))
ranges = []
with open(root / 'docs/coverage/translation-unit-ranges.csv', newline='') as f:
    for r in csv.DictReader(f):
        for key in ('string_range', 'data_range'):
            if r.get(key):
                lo, hi = (int(x, 16) for x in r[key].split('-'))
                ranges.append((lo, hi, r['unit_id']))
print('pooled literals owned by a converted unit, unmarked:')
for (v, section), sites in sorted(shared):
    owner = next((u for lo, hi, u in ranges if lo <= v <= hi), None)
    if section == '.data' and v not in marked and owner and \
            any(s.endswith('.c') for s in sources[owner]) and owner in {u for _, u in sites}:
        print('  %08x %-8s %r' % (v, owner, string_at(v, section)))
