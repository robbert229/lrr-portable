#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Whole-image identity: how far a rebuilt LegoRR.exe is from the original.

The end state of the matching work is an original-mode link (no /DEBUG, the
``opt-ref`` relink of ``tools/opt_ref_link.py``) that is byte-identical to
``reference/LegoRR.exe``. Only two PE header fields are ignored, because they
record the link and not the source: the COFF ``TimeDateStamp`` and the
optional header's ``CheckSum``.

``python3 tools/image_identity.py [--rebuilt EXE] [--original EXE] [--json]``
prints the file sizes, the differing bytes left (compared by file offset over
the longer file, so missing or extra bytes count), the first difference with
its section and RVA, and the differing bytes per section compared by RVA. It
exits 0 only when the images are identical apart from the two ignored fields.
"""
import argparse
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_REBUILT = ROOT / 'build-reccmp' / 'opt-ref' / 'LegoRR.exe'
DEFAULT_ORIGINAL = ROOT / 'reference' / 'LegoRR.exe'


def ignored_ranges(data):
    """File offsets of the COFF TimeDateStamp and the optional CheckSum."""
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    if data[pe:pe + 4] != b'PE\0\0':
        raise ValueError('not a PE image')
    return [(pe + 8, pe + 12), (pe + 24 + 64, pe + 24 + 68)]


def sections(data):
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    count, opt_size = struct.unpack_from('<H12xH', data, pe + 6)
    table = pe + 24 + opt_size
    out = []
    for i in range(count):
        entry = table + 40 * i
        name = data[entry:entry + 8].rstrip(b'\0').decode('ascii', 'replace')
        vsize, va, rsize, raw = struct.unpack_from('<IIII', data, entry + 8)
        out.append({'name': name, 'va': va, 'vsize': vsize, 'raw': raw, 'rsize': rsize})
    return out


def masked(data):
    buf = bytearray(data)
    for start, end in ignored_ranges(data):
        buf[start:end] = b'\0' * (end - start)
    return bytes(buf)


def locate(secs, offset):
    for sec in secs:
        if sec['raw'] <= offset < sec['raw'] + sec['rsize']:
            return sec['name'], 0x400000 + sec['va'] + offset - sec['raw']
    return 'headers' if offset < (secs[0]['raw'] if secs else 0) else 'overlay', None


def compare(rebuilt, original):
    a, b = masked(rebuilt), masked(original)
    n = max(len(a), len(b))
    common = min(len(a), len(b))
    diff = sum(1 for i in range(common) if a[i] != b[i]) + (n - common)
    first = next((i for i in range(common) if a[i] != b[i]), common if len(a) != len(b) else None)
    osecs, rsecs = sections(original), sections(rebuilt)
    report = {
        'identical': diff == 0,
        'original_bytes': len(original),
        'rebuilt_bytes': len(rebuilt),
        'differing_bytes': diff,
        'first_difference': None,
        'sections': [],
    }
    if first is not None:
        name, va = locate(osecs, first)
        report['first_difference'] = {'file_offset': hex(first), 'section': name,
                                      'address': hex(va) if va is not None else None}
    rmap = {s['name']: s for s in rsecs}
    for sec in osecs:
        other = rmap.get(sec['name'])
        size = sec['rsize']
        if other is None:
            report['sections'].append({'name': sec['name'], 'original_bytes': size,
                                       'rebuilt_bytes': 0, 'differing_bytes': size,
                                       'va_agrees': False})
            continue
        sa = original[sec['raw']:sec['raw'] + size]
        sb = rebuilt[other['raw']:other['raw'] + other['rsize']]
        m = min(len(sa), len(sb))
        d = sum(1 for i in range(m) if sa[i] != sb[i]) + abs(len(sa) - len(sb))
        prefix = next((i for i in range(m) if sa[i] != sb[i]), m)
        report['sections'].append({'name': sec['name'], 'original_bytes': size,
                                   'rebuilt_bytes': other['rsize'], 'differing_bytes': d,
                                   'identical_prefix': prefix,
                                   'va_agrees': sec['va'] == other['va']})
    return report


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--rebuilt', type=Path, default=DEFAULT_REBUILT)
    parser.add_argument('--original', type=Path, default=DEFAULT_ORIGINAL)
    parser.add_argument('--json', action='store_true')
    args = parser.parse_args(argv)
    report = compare(args.rebuilt.read_bytes(), args.original.read_bytes())
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print(f"original {report['original_bytes']} B, rebuilt {report['rebuilt_bytes']} B, "
              f"differing {report['differing_bytes']} B")
        if report['first_difference']:
            fd = report['first_difference']
            print(f"first difference: file {fd['file_offset']} ({fd['section']} {fd['address']})")
        for sec in report['sections']:
            print(f"  {sec['name']:8} {sec['differing_bytes']:>8} differ of {sec['original_bytes']:>8}"
                  f" (rebuilt {sec['rebuilt_bytes']}, va {'ok' if sec['va_agrees'] else 'moved'},"
                  f" prefix {sec.get('identical_prefix', 0)})")
    return 0 if report['identical'] else 1


if __name__ == '__main__':
    sys.exit(main())
