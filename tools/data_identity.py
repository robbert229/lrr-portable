#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Data identity metrics: where data lands, region by region (issue #817, finding 680).

`binary_progress.py` records these in `binary-progress.json` (schema 6) as
`data_identity`. The `.text` identical prefix stops on the first absolute data
operand (finding 668), so data placement decides how far it can grow. Every
data address depends on two things: the start of its region, and its offset
from that start. This module measures both, for four regions:

- `.rdata`: the section;
- `.data`: the initialised part of the `.data` section (the `.CRT$X*` table
  pointers, then each object's initialised data and string literals);
- `.bss`: the uninitialised part, up to the first common symbol;
- `commons`: the common symbols, which LINK allocates after every object's
  `.bss`.

Region starts. The rebuilt `.bss` starts at the link map's `.bss` group, and
its commons at the first `<common>` symbol. The original has no map: its
`.bss` starts at the first unit's `.bss` (`translation-unit-ranges.csv`,
GAME-001's `advisorGlobs`, `004b2db0`), and its commons at `00549520`
(finding 516, ADR 0006 decision 5).

The image measured. With the original-mode link (`opt_ref`, no `/DEBUG`)
and its map, every region is measured on that link (`image`; finding 687),
because the gate's `.rdata` starts with the 84-byte debug directory, so no
`.rdata` item of the gate can sit at its original offset. Reccmp pairs items
with the gate's addresses; `translator` moves each one to the relink by the
nearest map symbol both links define (same name and object), since both
links lay out the same objects. `gate` keeps the gate's per-region figures.
Without the relink's map, everything is the gate's, and only the `.rdata`
prefix reads the relink.

Per region:

- `identical_prefix`: raw bytes identical from the region start, comparing
  each image's own region (section-relative offsets, not absolute
  addresses). Not measured for the zero-filled regions.
- `items`: Reccmp's paired data entities (`entities.json`: globals, strings,
  floats, vtables) whose original address lies in the region. An item is at
  its original offset when its rebuilt offset from the rebuilt region start
  equals its original offset; at its original address when the two addresses
  are equal.
- `objects`: the rebuilt objects that define those items (the link map
  symbol at or below each rebuilt address). An object agrees when every one
  of its paired items in the region is at its original offset.
- `first_difference`: the first paired item, in original address order, that
  is not at its original offset: its addresses, the offset delta (rebuilt
  minus original) and the rebuilt object.
"""
import bisect
import collections
import csv
import re

ORIGINAL_COMMONS_START = 0x549520
DATA_TYPES = (2, 3, 4, 5, 11)  # Reccmp entity types that are data, not functions or imports
REGIONS = ('.rdata', '.data', '.bss', 'commons')

MAP_GROUP = re.compile(r'^\s*([0-9a-f]{4}):([0-9a-f]{8})\s+([0-9a-f]{8})H\s+(\S+)\s+\w+\s*$', re.I | re.M)
MAP_SYMBOL = re.compile(r'^\s*([0-9a-f]{4}):[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s+(?:[fi]\s+)*(\S+)\s*$', re.I | re.M)


def original_bss_start(rows):
    """The original `.bss` start: the lowest unit `.bss` range start."""
    starts = [int(r['bss_range'].split('-')[0], 16) for r in rows if r.get('bss_range')]
    return min(starts) if starts else None


def parse_map(text):
    """(section groups by name, symbols) of a LINK 5.0 map.

    Groups map a name to `(section, offset)` of its first occurrence;
    symbols are `(address, name, object)` sorted by address.
    """
    groups = {}
    for m in MAP_GROUP.finditer(text):
        groups.setdefault(m.group(4), (int(m.group(1), 16), int(m.group(2), 16)))
    symbols = sorted({(int(m.group(3), 16), m.group(2), m.group(4)) for m in MAP_SYMBOL.finditer(text)})
    return groups, symbols


def sections(pe):
    """Section name -> (virtual start, virtual end, bytes) of a `pefile.PE`."""
    base = pe.OPTIONAL_HEADER.ImageBase
    out = {}
    for s in pe.sections:
        name = s.Name.rstrip(b'\0').decode('ascii', 'replace')
        start = base + s.VirtualAddress
        out[name] = (start, start + s.Misc_VirtualSize, s.get_data()[:s.Misc_VirtualSize])
    return out


def regions(secs, bss_start, commons_start):
    """Region name -> (start, end); a region whose bounds are unknown is omitted."""
    out = {}
    if '.rdata' in secs:
        out['.rdata'] = secs['.rdata'][:2]
    if '.data' in secs:
        start, end, _ = secs['.data']
        if bss_start is not None:
            out['.data'] = (start, bss_start)
            out['.bss'] = (bss_start, commons_start if commons_start is not None else end)
        if commons_start is not None:
            out['commons'] = (commons_start, end)
    return out


def region_bytes(secs, name, lo, hi):
    """The bytes of one region, read from the section that holds it."""
    start, _, data = secs['.rdata' if name == '.rdata' else '.data']
    return data[lo - start:hi - start]


def raw_prefix(original, rebuilt):
    """Leading bytes equal in both sequences."""
    n, limit = 0, min(len(original), len(rebuilt))
    while n < limit and original[n] == rebuilt[n]:
        n += 1
    return n


def _region_of(address, bounds):
    for name, (lo, hi) in bounds.items():
        if lo <= address < hi:
            return name
    return None


def items(entities, original_bounds, rebuilt_bounds, owner):
    """Paired data items per original region: (original, rebuilt, size, rebuilt region, object)."""
    out = collections.defaultdict(list)
    for e in entities:
        if e.get('type') not in DATA_TYPES or e.get('orig') is None or e.get('recomp') is None:
            continue
        region = _region_of(e['orig'], original_bounds)
        if region is None:
            continue
        out[region].append((e['orig'], e['recomp'], e.get('size') or 0,
                            _region_of(e['recomp'], rebuilt_bounds), owner(e['recomp'])))
    for v in out.values():
        v.sort()
    return out


def region_items(paired, region, original_start, rebuilt_start):
    """The item, object and first-difference figures of one region."""
    at_offset = [i for i in paired if i[3] == region and i[1] - rebuilt_start == i[0] - original_start]
    at_address = [i for i in paired if i[0] == i[1]]
    objects = collections.OrderedDict()
    for i in paired:
        ok = i[3] == region and i[1] - rebuilt_start == i[0] - original_start
        objects.setdefault(i[4], []).append(ok)
    first = next((i for i in paired if not (i[3] == region and i[1] - rebuilt_start == i[0] - original_start)), None)
    result = {'items': {'paired': len(paired), 'paired_bytes': sum(i[2] for i in paired),
                        'at_original_offset': len(at_offset), 'at_original_offset_bytes': sum(i[2] for i in at_offset),
                        'at_original_address': len(at_address),
                        'at_original_address_bytes': sum(i[2] for i in at_address)},
              'objects': {'with_paired_items': len(objects),
                          'at_original_offset': sum(all(v) for v in objects.values()),
                          'agreeing': [o for o, v in objects.items() if all(v)]},
              'first_difference': None}
    if first is not None:
        result['first_difference'] = {
            'original': hex(first[0]), 'rebuilt': hex(first[1]), 'rebuilt_region': first[3],
            'offset_delta': (first[1] - rebuilt_start) - (first[0] - original_start) if first[3] == region else None,
            'object': first[4]}
    return result


def translator(from_symbols, to_symbols, from_secs, to_secs, lead=None):
    """A function moving an item of one link to the other link of the same objects.

    Anchors are the map symbols whose (name, object) occurs once in each map.
    An item moves with the nearest anchor at or below it in its section;
    before the section's first anchor, by the bytes `lead` names (section
    name -> bytes that only the first link puts at that section's start,
    such as the gate's debug directory), then with the first anchor, then
    with the section's start. When both links hold the section's bytes, the
    first candidate whose bytes equal the item's is taken: a lead absorbed by
    a later contribution's alignment moves the items after it by less.
    Addresses outside every section are returned unchanged.
    """
    def unique(symbols):
        count = collections.Counter((n, o) for _, n, o in symbols)
        return {(n, o): a for a, n, o in symbols if count[(n, o)] == 1}
    src, dst = unique(from_symbols), unique(to_symbols)
    anchors = sorted((a, dst[k] - a) for k, a in src.items() if k in dst)
    keys = [a for a, _ in anchors]
    bounds = [(lo, hi, name) for name, (lo, hi, _) in from_secs.items() if name in to_secs]
    lead = lead or {}

    def read(secs, name, address, size):
        lo, hi, data = secs[name]
        return data[address - lo:address - lo + size] if lo <= address and address + size <= lo + len(data) else None

    def translate(address, size=0):
        sec = next(((lo, hi, name) for lo, hi, name in bounds if lo <= address < hi), None)
        if sec is None:
            return address
        lo, hi, name = sec
        i = bisect.bisect_right(keys, address) - 1
        if i >= 0 and keys[i] >= lo:
            shifts = [anchors[i][1]]
        else:
            shifts = [to_secs[name][0] - lo - lead[name]] if name in lead else []
            if i + 1 < len(keys) and keys[i + 1] < hi:
                shifts.append(anchors[i + 1][1])
        shifts.append(to_secs[name][0] - lo)
        want = read(from_secs, name, address, size) if size else None
        if want:
            for shift in shifts:
                if read(to_secs, name, address + shift, size) == want:
                    return address + shift
        return address + shifts[0]
    return translate


def debug_lead(from_secs, to_secs, name='.rdata'):
    """{name: n} when the first link's section opens with n zero bytes more
    than the second's: the gate's debug directory, which `tools/reccmp.sh`
    zeroes (`strip_pe_debug.py`) but leaves in place."""
    def zeros(data):
        return len(data) - len(data.lstrip(b'\0'))
    if name not in from_secs or name not in to_secs:
        return {}
    n = zeros(from_secs[name][2]) - zeros(to_secs[name][2])
    return {name: n} if n > 0 else {}


def _compact(regions_out):
    """The gate's per-region figures kept alongside the relink's."""
    out = {}
    for name, r in regions_out.items():
        entry = {'rebuilt_start': r['rebuilt_start'], 'start_delta': r['start_delta'],
                 'at_original_offset': r['items']['at_original_offset'],
                 'at_original_offset_bytes': r['items']['at_original_offset_bytes'],
                 'at_original_address': r['items']['at_original_address']}
        if 'identical_prefix' in r and 'prefix_image' not in r:
            entry['identical_prefix_bytes'] = r['identical_prefix']['bytes']
        out[name] = entry
    return out


def measure(original_pe, rebuilt_pe, map_text, entities, rows, relink_pe=None, relink_map=None):
    """The `data_identity` field of `binary-progress.json`.

    `original_pe` and `rebuilt_pe` are `pefile.PE` objects (the gate's image)
    and `map_text` the gate's map. `relink_pe` and `relink_map` are the
    original-mode link's; with both, every region is measured on that link
    and the gate's figures are kept as `gate`. With `relink_pe` alone, only
    the `.rdata` prefix reads it.
    """
    if relink_pe is None or not relink_map:
        out = _measure(original_pe, rebuilt_pe, map_text, entities, rows, relink_pe)
        out['image'] = 'gate'
        return out
    gate = _measure(original_pe, rebuilt_pe, map_text, entities, rows)
    gate_secs, relink_secs = sections(rebuilt_pe), sections(relink_pe)
    translate = translator(parse_map(map_text)[1], parse_map(relink_map)[1], gate_secs, relink_secs,
                           debug_lead(gate_secs, relink_secs))
    moved = [{**e, 'recomp': translate(e['recomp'], e.get('size') or 0)}
             if e.get('type') in DATA_TYPES and e.get('recomp') is not None else e for e in entities]
    out = _measure(original_pe, relink_pe, relink_map, moved, rows)
    out['image'] = 'original-mode link'
    out['method'] += ("; measured on the original-mode link (no /DEBUG), Reccmp's gate addresses "
                      "moved to it by the nearest map symbol both links define")
    out['gate'] = _compact(gate['regions'])
    return out


def _measure(original_pe, rebuilt_pe, map_text, entities, rows, rdata_pe=None):
    """The regions of one rebuilt image; `rdata_pe` optionally the image whose
    `.rdata` the `.rdata` prefix reads."""
    groups, symbols = parse_map(map_text)
    addresses = [s[0] for s in symbols]

    osecs, rsecs = sections(original_pe), sections(rebuilt_pe)

    def owner(address):
        # The map symbol at or below, in the same section: an item before its
        # section's first symbol has no known owner.
        i = bisect.bisect_right(addresses, address) - 1
        if i < 0:
            return None
        sec = next(((lo, hi) for lo, hi, _ in rsecs.values() if lo <= address < hi), None)
        return symbols[i][2] if sec is None or symbols[i][0] >= sec[0] else None

    rebuilt_bss = None
    if '.bss' in groups and '.data' in groups and '.data' in rsecs and groups['.bss'][0] == groups['.data'][0]:
        rebuilt_bss = rsecs['.data'][0] + groups['.bss'][1]
    commons = [s[0] for s in symbols if s[2] == '<common>']
    rebuilt_commons = min(commons) if commons else None
    obounds = regions(osecs, original_bss_start(rows), ORIGINAL_COMMONS_START)
    rbounds = regions(rsecs, rebuilt_bss, rebuilt_commons)
    paired = items(entities, obounds, rbounds, owner)
    out = {'method': 'regions measured from their own starts; items are Reccmp-paired data entities, '
                     'objects the rebuilt definers of those items (link map)',
           'regions': {}}
    for name in REGIONS:
        if name not in obounds or name not in rbounds:
            continue
        (olo, ohi), (rlo, rhi) = obounds[name], rbounds[name]
        entry = {'original_start': hex(olo), 'original_bytes': ohi - olo,
                 'rebuilt_start': hex(rlo), 'rebuilt_bytes': rhi - rlo,
                 'start_delta': rlo - olo}
        if name in ('.rdata', '.data'):
            obytes = region_bytes(osecs, name, olo, ohi)
            if name == '.rdata' and rdata_pe is not None:
                rsec = sections(rdata_pe)['.rdata']
                rbytes = region_bytes({'.rdata': rsec}, name, *rsec[:2])
                entry['prefix_image'] = 'original-mode link'
            else:
                rbytes = region_bytes(rsecs, name, rlo, rhi)
            n = raw_prefix(obytes, rbytes)
            different = n < len(obytes)
            entry['identical_prefix'] = {
                'bytes': n, 'first_difference': hex(olo + n) if different else None,
                'rebuilt_object': owner(rlo + n) if different and 'prefix_image' not in entry else None}
        entry.update(region_items(paired.get(name, []), name, olo, rlo))
        out['regions'][name] = entry
    return out


def main():
    import argparse
    import json
    from pathlib import Path
    import pefile
    root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--original', type=Path, default=root / 'reference/LegoRR.exe')
    parser.add_argument('--build', type=Path, default=root / 'build-reccmp')
    args = parser.parse_args()
    with (root / 'docs/coverage/translation-unit-ranges.csv').open(newline='', encoding='utf-8') as f:
        rows = list(csv.DictReader(f))
    relink, relink_map = args.build / 'opt-ref/LegoRR.exe', args.build / 'opt-ref/LegoRR.map'
    result = measure(pefile.PE(str(args.original)), pefile.PE(str(args.build / 'LegoRR.exe')),
                     (args.build / 'LegoRR.map').read_text(errors='replace'),
                     json.loads((args.build / 'reccmp-report/entities.json').read_text()), rows,
                     pefile.PE(str(relink)) if relink.is_file() else None,
                     relink_map.read_text(errors='replace') if relink_map.is_file() else None)
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
