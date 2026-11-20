# SPDX-License-Identifier: LGPL-3.0-or-later
"""iat_order.py reproduces the scratch-link probes of finding 694."""
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import iat_order


def by_dll(a, b):
    return (a[0] > b[0]) - (a[0] < b[0])


def single_dll(n):
    """n imports of one DLL in load order, then its null thunk."""
    return [('KERNEL32.dll', i) for i in range(n)] + [('KERNEL32.dll', None)]


def imports_only(items):
    return [s for _, s in items if s is not None]


class QsortTest(unittest.TestCase):
    def test_microsoft_rotates_a_short_run(self):
        # 6 imports and the null thunk: shortsort moves the first import last.
        out = imports_only(iat_order.microsoft_qsort(single_dll(6), by_dll))
        self.assertEqual(out, [1, 2, 3, 4, 5, 0])

    def test_microsoft_swaps_the_pivot(self):
        # 20 imports and the null thunk: the middle element goes to the front.
        out = imports_only(iat_order.microsoft_qsort(single_dll(20), by_dll))
        expected = list(range(20))
        expected[0], expected[10] = 10, 0
        self.assertEqual(out, expected)

    def test_wine_keeps_a_long_run(self):
        out = imports_only(iat_order.wine_qsort(single_dll(20), by_dll))
        self.assertEqual(out, list(range(20)))

    def test_two_dlls_in_both_library_orders(self):
        # The probe's observed IATs (finding 694): the null thunks tie with
        # their DLL's imports.
        k = lambda i: ('KERNEL32.dll', 'K%d' % i)
        u = lambda i: ('USER32.dll', 'U%d' % i)
        kn, un = ('KERNEL32.dll', None), ('USER32.dll', None)
        first = [k(0), k(1), k(2), kn, u(3), u(4), u(5), un]
        self.assertEqual(imports_only(iat_order.microsoft_qsort(first, by_dll)),
                         ['K1', 'K2', 'K0', 'U4', 'U5', 'U3'])
        second = [u(0), u(1), u(2), un, k(3), k(4), k(5), kn]
        self.assertEqual(imports_only(iat_order.microsoft_qsort(second, by_dll)),
                         ['K5', 'K4', 'K3', 'U2', 'U1', 'U0'])


class VerboseLogTest(unittest.TestCase):
    LOG = """\
Start Pass1
Searching Libraries
    Searching kernel32.lib:
      Found __imp__GetVersion@0
        Referenced in a.obj
        Loaded kernel32.lib(KERNEL32.dll)
      Found __IMPORT_DESCRIPTOR_KERNEL32
        Referenced in kernel32.lib(KERNEL32.dll)
        Loaded kernel32.lib(KERNEL32.dll)
      Found __NULL_IMPORT_DESCRIPTOR
        Referenced in kernel32.lib(KERNEL32.dll)
        Loaded kernel32.lib(KERNEL32.dll)
      Found \x7fKERNEL32_NULL_THUNK_DATA
        Referenced in kernel32.lib(KERNEL32.dll)
        Loaded kernel32.lib(KERNEL32.dll)
    Searching libc.lib:
      Found _strlen
        Referenced in a.obj
        Loaded libc.lib(strlen.obj)
"""

    def test_load_order_keeps_imports_and_null_thunks(self):
        with tempfile.TemporaryDirectory() as tmp:
            log = Path(tmp) / 'link.log'
            log.write_text(self.LOG)
            order = iat_order.load_order(str(log))
        self.assertEqual(order, [('KERNEL32.dll', '__imp__GetVersion@0'),
                                 ('KERNEL32.dll', None)])
        self.assertEqual(iat_order.predict(order, 'microsoft'),
                         {'KERNEL32.DLL': ['GetVersion']})


if __name__ == '__main__':
    unittest.main()
