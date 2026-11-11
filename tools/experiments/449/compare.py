#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Compare mapped original-region globals with the matching build's map.

Run after make reccmp-check. Reccmp's entity sizes and aliases reflect the
reconstruction; this is an inventory, not proof of original common ownership.
"""
import json
from pathlib import Path
import re

root = Path(__file__).resolve().parents[3]
entities = json.loads((root / 'build-reccmp/reccmp-report/entities.json').read_text())
rows = sorted((e for e in entities if e.get('type') == 2 and
               0x549520 <= (e.get('orig') or 0) < 0x76e000 and e.get('recomp')),
              key=lambda e: e['orig'])
annotations = {}
for path in (root / 'src').rglob('*'):
    if path.suffix not in ('.c', '.cpp', '.h', '.hpp'):
        continue
    lines = path.read_text(errors='replace').splitlines()
    for i, line in enumerate(lines):
        match = re.search(r'GLOBAL: LEGORR 0x([0-9a-fA-F]+)', line)
        if match and i + 1 < len(lines):
            annotations.setdefault(int(match.group(1), 16), []).append(
                '%s:%d %s' % (path.relative_to(root), i + 2, lines[i + 1].strip()))
inv = sum(a['recomp'] > b['recomp'] for i, a in enumerate(rows) for b in rows[i + 1:])
print('%d mapped globals, %d/%d reversed pairs' % (len(rows), inv, len(rows) * (len(rows) - 1) // 2))
for e in rows:
    print('%08x %08x %s' % (e['orig'], e['recomp'], ' | '.join(annotations.get(e['orig'], ['<annotation not found>']))))
