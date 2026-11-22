#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Model and measure the order of import slots within each DLL (finding 694).

LINK 5.0 builds each DLL's import address table (`.idata$5`, and `.idata$4`
in step) by sorting every import member's contribution with the C runtime's
`qsort`. The input is the order LINK loads the members, as `/VERBOSE` prints
it ("Found ..." then "Loaded lib(DLL)"), null thunks included. The key is the
member's DLL name alone, compared case-sensitively. Imports of one DLL compare
equal, so their order is whatever `qsort` leaves; the null thunk is written
last whatever its sorted place. Under `/OPT:REF` the unreferenced slots are
then dropped, so members only removed code references still take part.

Microsoft's MSVCRT 5.00 `qsort` and Wine's builtin one permute equal keys
differently, so the order also depends on which runtime LINK.EXE ran on.

    iat_order.py slots ORIGINAL REBUILT       slot agreement per DLL
    iat_order.py predict VERBOSE_LOG [--crt microsoft|wine] [--exe EXE]
        predict the IAT from a /VERBOSE link log; with --exe, compare
"""

import argparse
import re
import sys


def microsoft_qsort(items, cmp):
    """MSVCRT 5.00 qsort: CUTOFF 8, middle element swapped to lo, unstable."""
    a = list(items)
    if len(a) < 2:
        return a

    def shortsort(lo, hi):
        while hi > lo:
            mx = lo
            for p in range(lo + 1, hi + 1):
                if cmp(a[p], a[mx]) > 0:
                    mx = p
            a[mx], a[hi] = a[hi], a[mx]
            hi -= 1

    stack = []
    lo, hi = 0, len(a) - 1
    while True:
        size = hi - lo + 1
        if size <= 8:
            shortsort(lo, hi)
        else:
            mid = lo + size // 2
            a[mid], a[lo] = a[lo], a[mid]
            loguy, higuy = lo, hi + 1
            while True:
                loguy += 1
                while loguy <= hi and cmp(a[loguy], a[lo]) <= 0:
                    loguy += 1
                higuy -= 1
                while higuy > lo and cmp(a[higuy], a[lo]) >= 0:
                    higuy -= 1
                if higuy < loguy:
                    break
                a[loguy], a[higuy] = a[higuy], a[loguy]
            a[lo], a[higuy] = a[higuy], a[lo]
            if higuy - 1 - lo >= hi - loguy:
                if lo + 1 < higuy:
                    stack.append((lo, higuy - 1))
                if loguy < hi:
                    lo = loguy
                    continue
            else:
                if loguy < hi:
                    stack.append((loguy, hi))
                if lo + 1 < higuy:
                    hi = higuy - 1
                    continue
        if not stack:
            return a
        lo, hi = stack.pop()


def wine_qsort(items, cmp):
    """Wine 11's builtin msvcrt qsort (dlls/msvcrt/misc.c, quick_sort)."""
    a = list(items)

    def small_sort(base, n):
        for e in range(n, 1, -1):
            mx = base
            p = None
            for i in range(1, e):
                p = base + i
                if cmp(a[p], a[mx]) > 0:
                    mx = p
            if p != mx:
                a[p], a[mx] = a[mx], a[p]

    if len(a) < 2:
        return a
    stack = [(0, len(a) - 1)]
    while stack:
        beg, end = stack.pop()
        if end - beg < 8:
            small_sort(beg, end - beg + 1)
            continue
        lo, hi = beg, end
        med = lo + (hi - lo + 1) // 2
        if cmp(a[lo], a[med]) > 0:
            a[lo], a[med] = a[med], a[lo]
        if cmp(a[lo], a[hi]) > 0:
            a[lo], a[hi] = a[hi], a[lo]
        if cmp(a[med], a[hi]) > 0:
            a[med], a[hi] = a[hi], a[med]
        lo += 1
        hi -= 1
        while True:
            while lo <= hi:
                if lo != med and cmp(a[lo], a[med]) > 0:
                    break
                lo += 1
            while med != hi:
                if cmp(a[hi], a[med]) <= 0:
                    break
                hi -= 1
            if hi < lo:
                break
            a[lo], a[hi] = a[hi], a[lo]
            if hi == med:
                med = lo
            lo += 1
            hi -= 1
        while hi > beg:
            if hi != med and cmp(a[hi], a[med]) != 0:
                break
            hi -= 1
        if hi - beg >= end - lo:
            stack.append((beg, hi))
            stack.append((lo, end))
        else:
            stack.append((lo, end))
            stack.append((beg, hi))
    return a


SORTS = {'microsoft': microsoft_qsort, 'wine': wine_qsort}


def load_order(verbose_log):
    """[(dll, symbol or None for a null thunk)] in the order LINK loads them."""
    order = []
    found = None
    with open(verbose_log, errors='replace') as log:
        for line in log:
            line = line.rstrip('\r\n')
            m = re.match(r'\s*Found (\S+)', line)
            if m:
                found = m.group(1)
                continue
            m = re.match(r'\s*Loaded .*\((.*)\)$', line)
            if m and found:
                member = m.group(1)
                if member.lower().endswith('.dll') and not (
                        found.startswith('__IMPORT_DESCRIPTOR_')
                        or found == '__NULL_IMPORT_DESCRIPTOR'):
                    null = found.endswith('_NULL_THUNK_DATA')
                    order.append((member, None if null else found))
                found = None
    return order


def import_name(symbol):
    name = symbol[len('__imp_'):] if symbol.startswith('__imp_') else symbol
    return name.lstrip('_').split('@')[0]


def predict(order, crt):
    """{DLL (upper case): [import names]} before /OPT:REF drops slots."""
    out = SORTS[crt](order, lambda x, y: (x[0] > y[0]) - (x[0] < y[0]))
    table = {}
    for dll, symbol in out:
        if symbol:
            table.setdefault(dll.upper(), []).append(import_name(symbol))
    return table


def pe_imports(path):
    import pefile
    pe = pefile.PE(path, fast_load=True)
    pe.parse_data_directories(directories=[
        pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_IMPORT']])
    table = {}
    for entry in pe.DIRECTORY_ENTRY_IMPORT:
        table[entry.dll.decode().upper()] = [
            imp.name.decode() if imp.name else '#%d' % imp.ordinal
            for imp in entry.imports]
    return table


def agreement(expected, actual):
    rows = []
    for dll, names in expected.items():
        other = actual.get(dll, [])
        same = sum(1 for a, b in zip(names, other) if a == b)
        rows.append((dll, same, len(names), names == other))
    return rows


def print_rows(rows):
    total = sum(r[1] for r in rows)
    count = sum(r[2] for r in rows)
    for dll, same, n, exact in rows:
        print('%-14s %3d of %3d%s' % (dll, same, n, '  exact' if exact else ''))
    print('%-14s %3d of %3d' % ('total', total, count))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest='command', required=True)
    s = sub.add_parser('slots', help='slot agreement per DLL')
    s.add_argument('original')
    s.add_argument('rebuilt')
    p = sub.add_parser('predict', help='predict the IAT from a /VERBOSE log')
    p.add_argument('verbose_log')
    p.add_argument('--crt', choices=sorted(SORTS), default='microsoft')
    p.add_argument('--exe', help='compare with this image (slots it keeps)')
    args = parser.parse_args(argv)

    if args.command == 'slots':
        print_rows(agreement(pe_imports(args.original), pe_imports(args.rebuilt)))
        return 0
    table = predict(load_order(args.verbose_log), args.crt)
    if not args.exe:
        for dll, names in sorted(table.items()):
            print(dll, ' '.join(names))
        return 0
    actual = pe_imports(args.exe)
    kept = {dll: [n for n in names if n in set(actual.get(dll, []))]
            for dll, names in table.items()}
    # An import by ordinal (DSOUND #1) has no name to match in the log; a
    # DLL whose names cannot all be matched is compared as the image has it.
    for dll, names in actual.items():
        if dll in kept and len(kept[dll]) != len(names):
            kept[dll] = [n for n in names]
    rows = agreement(actual, kept)
    print_rows(rows)
    return 0 if all(r[3] for r in rows) else 1


if __name__ == '__main__':
    sys.exit(main())
