#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Compare annotated data gaps within original units and matching COFF objects.

The report is evidence, not a linker simulation: source-to-symbol association is
conservative and unmatched, ambiguous, pooled or displaced items remain n/a.
"""
import argparse
import collections
import csv
import json
import re
import struct
from pathlib import Path

import link_order

ROOT = Path(__file__).resolve().parent.parent
# Finding 524: only copies with an individually supported owner are included.
ANCHORS = dict(zip(('GAME-003', 'G98-002', 'G98-003', 'G98-005', 'G98-006',
                    'G98-007', 'G98-009', 'G98-011', 'G98-015', 'G98-019',
                    'G98-020', 'G98-031'),
                   (0x49e034, 0x49e87c, 0x49e89c, 0x49e8c0, 0x49e8e8,
                    0x49e910, 0x49e94c, 0x49e980, 0x49e9d0, 0x49ea10,
                    0x49ea38, 0x49eadc)))
MARK = re.compile(r'^\s*//\s*(GLOBAL|STRING):\s*LEGORR\s+0x([0-9a-f]+)', re.I)
IDENT = re.compile(r'\b([a-zA-Z_]\w*)\s*(?:\[[^\]]*\]\s*)*(?:=|;)')
MAP = re.compile(r'^\s*[0-9a-f]{4}:[0-9a-f]{8}\s+\S+\s+[0-9a-f]{8}\s+(?:[fi]\s+)*(\S+\.obj)\s*$', re.I | re.M)


def coff(data):
    """Return (sections, symbols) from an i386 COFF object, checking bounds."""
    def unpack(fmt, pos):
        size = struct.calcsize(fmt)
        if pos < 0 or pos + size > len(data):
            raise ValueError('truncated COFF object')
        return struct.unpack_from(fmt, data, pos)
    machine, count, _, symptr, nsyms, opt, _ = unpack('<HHIIIHH', 0)
    if machine != 0x14c or opt:
        raise ValueError('expected i386 COFF object')
    stringpos = symptr + 18 * nsyms
    strlen, = unpack('<I', stringpos)
    if strlen < 4 or stringpos + strlen > len(data):
        raise ValueError('invalid COFF string table')
    strings = data[stringpos:stringpos + strlen]
    def longname(offset):
        if offset < 4 or offset >= len(strings):
            raise ValueError('invalid COFF string offset')
        end = strings.find(b'\0', offset)
        if end < 0:
            raise ValueError('unterminated COFF string')
        return strings[offset:end].decode('ascii', 'replace')
    sections = [None]
    bases = collections.defaultdict(int)
    for i in range(count):
        raw, _, _, size, _, _, _, _, _, flags = unpack('<8sIIIIIIHHI', 20 + i * 40)
        name = raw.rstrip(b'\0').decode('ascii', 'replace')
        if name.startswith('/') and name[1:].isdigit():
            name = longname(int(name[1:]))
        group = name.split('$')[0]
        code = (flags >> 20) & 15
        if code == 15:
            raise ValueError('invalid COFF section alignment')
        align = 1 << (code - 1) if code else 1
        base = (bases[group] + align - 1) // align * align
        bases[group] = base + size
        sections.append({'name': name, 'group': group, 'base': base, 'size': size,
                         'align': align, 'comdat': bool(flags & 0x1000)})
    symbols = []
    i = 0
    while i < nsyms:
        raw, value, sec, typ, storage, aux = unpack('<8sIhHBB', symptr + 18 * i)
        if i + aux >= nsyms:
            raise ValueError('invalid COFF auxiliary count')
        zero, offset = struct.unpack('<II', raw)
        name = longname(offset) if zero == 0 else raw.rstrip(b'\0').decode('ascii', 'replace')
        symbols.append({'name': name, 'value': value, 'section': sec, 'storage': storage})
        i += 1 + aux
    return sections, symbols


def symbol_name(name):
    # C statics carry $Snnn; C++ data names begin ?name@@.
    if name.startswith('?'):
        return name[1:].split('@')[0]
    return name.lstrip('_').split('$')[0]


def source_annotations(root):
    found = collections.defaultdict(list)
    for path in (root / 'src').rglob('*'):
        if path.suffix not in ('.c', '.cpp', '.h', '.hpp'):
            continue
        lines = path.read_text(encoding='utf-8', errors='replace').splitlines()
        for i, line in enumerate(lines):
            m = MARK.match(line)
            if not m:
                continue
            # Only a single declarator directly attached to the marker is safe.
            following = lines[i + 1].strip() if i + 1 < len(lines) else ''
            names = IDENT.findall(following) if m.group(1).upper() == 'GLOBAL' else []
            found[int(m.group(2), 16)].append((path.relative_to(root).as_posix(),
                                               names[0] if len(names) == 1 else None))
    return found


def section_for(row, addr):
    result = set()
    for key, section in (('rdata_range', '.rdata'), ('data_range', '.data'),
                         ('string_range', '.data'), ('bss_range', '.bss')):
        if row[key]:
            lo, hi = (int(x, 16) for x in row[key].split('-'))
            if lo <= addr <= hi:
                result.add(section)
    return result


def classify(items, anchor=None):
    """Items are (original address, offset or None); unknowns prevent success."""
    known = sorted((a, b) for a, b in items if b is not None)
    missing = len(items) - len(known)
    if len(known) < 2:
        return {'status': 'n/a', 'count': len(known), 'missing': missing,
                'reason': 'fewer than two attributable items'}
    a0, b0 = known[0]
    gaps = [(a, a - a0, b - b0) for a, b in known if a - a0 != b - b0]
    absolute = [(a, a - anchor, b) for a, b in known if anchor is not None and a - anchor != b]
    if gaps or absolute:
        mismatch = (gaps or absolute)[0]
        return {'status': 'partial', 'count': len(known), 'missing': missing,
                'mismatches': len(gaps or absolute), 'kind': 'gap' if gaps else 'anchor',
                'first': mismatch}
    if missing:
        return {'status': 'n/a', 'count': len(known), 'missing': missing,
                'reason': 'some annotations cannot be attributed'}
    return {'status': 'full' if anchor is not None else 'gaps-only',
            'count': len(known), 'missing': 0}


def measure(root, build):
    with (root / link_order.RANGES).open(newline='', encoding='utf-8') as f:
        rows = list(csv.DictReader(f))
    annotations = source_annotations(root)
    units = {r['unit_id']: r for r in rows}
    groups = {u: collections.defaultdict(list) for u in units}
    listed = {e.key: e for e in link_order.load(root) if e.kind == 'unit' and e.applies_to('winapi')}
    path_units = {e.path: u for u, e in listed.items()}
    unknown_owner = []
    for addr, declarations in sorted(annotations.items()):
        owners = [(u, s) for u, row in units.items() for s in section_for(row, addr)]
        if not owners and addr >= 0x549520:
            # Finding 525: this region may contain commons. Ownership by a
            # dedicated source is provisional; the COFF symbol decides.
            paths = {path_units[p] for p, _ in declarations if p in path_units}
            if len(paths) == 1:
                owners = [(next(iter(paths)), '.bss')]
        if len(owners) != 1:
            unknown_owner.append(addr)
            continue
        unit, section = owners[0]
        groups[unit][section].append((addr, declarations))
    maptext = (build / 'LegoRR.map').read_text(errors='replace')
    linked = {m.lower() for m in MAP.findall(maptext)}
    result = []
    for unit in units:
        entry = listed.get(unit)
        object_name = Path(entry.source).name + '.obj' if entry else None
        candidates = list(build.glob('CMakeFiles/link_*.dir/src/**/' + object_name)) if object_name else []
        candidates = [p for p in candidates if p.as_posix().endswith('/src/' + entry.source + '.obj')] if entry else []
        reason = None
        if not entry:
            reason = 'unit has no dedicated link-order object'
        elif object_name.lower() not in linked:
            reason = 'object absent from map'
        elif len(candidates) != 1:
            reason = 'object missing or ambiguous'
        sections, symbols = coff(candidates[0].read_bytes()) if not reason else ([], [])
        byname = collections.defaultdict(list)
        for sym in symbols:
            if sym['storage'] in (2, 3) and not sym['name'].startswith('.'):
                byname[symbol_name(sym['name'])].append(sym)
        report = {'unit': unit, 'sections': {}, 'commons': [], 'reason': reason}
        for section in ('.data', '.rdata', '.bss'):
            items = []
            for addr, declarations in groups[unit][section]:
                names = {name for path, name in declarations if entry and path == entry.path and name}
                matches = [s for n in names for s in byname[n]]
                matches = [s for s in matches if s['section'] == 0 and s['value'] or
                           0 < s['section'] < len(sections) and sections[s['section']]['group'] in ('.data', '.rdata', '.bss')]
                if len(matches) != 1:
                    items.append((addr, None)); continue
                sym = matches[0]
                if sym['section'] == 0:
                    report['commons'].append({'address': addr, 'name': sym['name'], 'size': sym['value']})
                    continue
                sec = sections[sym['section']]
                if sec['group'] != section:
                    items.append((addr, None)); continue
                items.append((addr, sec['base'] + sym['value']))
            report['sections'][section] = classify(items, ANCHORS.get(unit) if section == '.rdata' else None)
            if reason:
                report['sections'][section]['reason'] = reason
        active = [v for v in report['sections'].values() if v['count'] or v['missing']]
        if any(v['status'] == 'partial' for v in active):
            report['status'] = 'partial'
        elif not active or any(v['status'] == 'n/a' for v in active):
            report['status'] = 'n/a'
        elif any(v['status'] == 'gaps-only' for v in active):
            report['status'] = 'gaps-only'
        else:
            report['status'] = 'full'
        result.append(report)
    return {'units': result, 'unattributed': unknown_owner}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--root', type=Path, default=ROOT)
    ap.add_argument('--build', type=Path, default=ROOT / 'build-reccmp')
    ap.add_argument('--json', action='store_true')
    args = ap.parse_args()
    result = measure(args.root, args.build)
    if args.json:
        print(json.dumps(result, indent=2))
        return
    print('unit | .data | .rdata | .bss | commons')
    for unit in result['units']:
        def fmt(sec):
            d = unit['sections'][sec]
            s = '%s (%d, %d unknown)' % (d['status'], d['count'], d['missing'])
            if 'first' in d:
                a, orig, rebuilt = d['first']
                s += ' %s %d first %08x: %x/%x' % (d['kind'], d['mismatches'], a, orig, rebuilt)
            elif d.get('reason'):
                s += ' ' + d['reason']
            return s
        print('%s | %s | %s | %s | %s' % (unit['unit'], fmt('.data'), fmt('.rdata'), fmt('.bss'), ', '.join('%08x:%s(size=%d)' % (c['address'], c['name'], c['size']) for c in unit['commons']) or '-'))
    counts = collections.Counter(u['status'] for u in result['units'])
    print('unit summary:', ', '.join('%s=%d' % (s, counts[s]) for s in ('full', 'gaps-only', 'partial', 'n/a')))
    print('unattributed annotations:', len(result['unattributed']),
          ' '.join('%08x' % a for a in result['unattributed']))


if __name__ == '__main__':
    main()
