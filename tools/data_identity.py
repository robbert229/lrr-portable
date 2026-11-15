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

Per region:

- `identical_prefix`: raw bytes identical from the region start, comparing
  each image's own region (section-relative offsets, not absolute
  addresses). For `.rdata` it compares the original-mode link
  (`opt_ref`, no `/DEBUG`) when one is given, because the gate's `.rdata`
  starts with the 84-byte debug directory. Not measured for the zero-filled
  regions.
- `items`: Reccmp's paired data entities (`entities.json`: globals, strings,
  floats, vtables) whose original address lies in the region. An item is at
  its original offset when its rebuilt offset from the rebuilt region start
  equals its original offset; at its original address when the two addresses
  are equal. The `.rdata` item offsets use the gate's addresses, so they
  include the debug directory.
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


def measure(original_pe, rebuilt_pe, map_text, entities, rows, rdata_pe=None):
    """The `data_identity` field of `binary-progress.json`.

    `original_pe` and `rebuilt_pe` are `pefile.PE` objects (the gate's image);
    `rdata_pe` optionally the original-mode relink, for the `.rdata` prefix.
    """
    groups, symbols = parse_map(map_text)
    addresses = [s[0] for s in symbols]

    def owner(address):
        i = bisect.bisect_right(addresses, address) - 1
        return symbols[i][2] if i >= 0 else None

    osecs, rsecs = sections(original_pe), sections(rebuilt_pe)
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
    relink = args.build / 'opt-ref/LegoRR.exe'
    result = measure(pefile.PE(str(args.original)), pefile.PE(str(args.build / 'LegoRR.exe')),
                     (args.build / 'LegoRR.map').read_text(errors='replace'),
                     json.loads((args.build / 'reccmp-report/entities.json').read_text()), rows,
                     pefile.PE(str(relink)) if relink.is_file() else None)
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
