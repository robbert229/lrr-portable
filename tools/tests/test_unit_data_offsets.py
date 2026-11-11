# SPDX-License-Identifier: LGPL-3.0-or-later
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import unit_data_offsets as offsets


def fixture():
    strings = b'\0\0\0\0long_static_name\0.rdata$long\0'
    strings = struct.pack('<I', len(strings)) + strings[4:]
    section1 = struct.pack('<8sIIIIIIHHI', b'.rdata', 0, 0, 5, 0, 0, 0, 0, 0, 0x00300000)
    section2 = struct.pack('<8sIIIIIIHHI', b'/21', 0, 0, 8, 0, 0, 0, 0, 0, 0x00401000)
    symbols = b''.join([
        struct.pack('<8sIhHBB', struct.pack('<II', 0, 4), 4, 1, 0, 3, 0),
        struct.pack('<8sIhHBB', b'_second', 2, 2, 0, 2, 0),
        struct.pack('<8sIhHBB', b'_common', 20, 0, 0, 2, 0),
    ])
    return struct.pack('<HHIIIHH', 0x14c, 2, 0, 100, 3, 0, 0) + section1 + section2 + symbols + strings


class CoffTests(unittest.TestCase):
    def test_sections_symbols_and_commons(self):
        sections, symbols = offsets.coff(fixture())
        self.assertEqual(sections[1]['base'], 0)
        self.assertEqual(sections[2]['name'], '.rdata$long')
        self.assertEqual(sections[2]['base'], 8)
        self.assertTrue(sections[2]['comdat'])
        self.assertEqual(symbols[0]['name'], 'long_static_name')
        self.assertEqual(sections[2]['base'] + symbols[1]['value'], 10)
        self.assertEqual((symbols[2]['section'], symbols[2]['value']), (0, 20))

    def test_bad_coff_fails(self):
        with self.assertRaises(ValueError):
            offsets.coff(fixture()[:-3])

    def test_classification(self):
        items = [(0x1014, 0), (0x1018, 4), (0x101c, 8)]
        self.assertEqual(offsets.classify(items)['status'], 'gaps-only')
        anchored = offsets.classify(items, 0x1000)
        self.assertEqual((anchored['status'], anchored['kind'], anchored['mismatches']), ('partial', 'anchor', 3))
        self.assertEqual(offsets.classify([(0x1000, 0), (0x1004, 4)], 0x1000)['status'], 'full')
        mismatch = offsets.classify([(0x1000, 0), (0x1004, 8)])
        self.assertEqual(mismatch['first'], (0x1004, 4, 8))
        self.assertEqual(offsets.classify(items + [(0x1020, None)])['status'], 'n/a')
        self.assertEqual(offsets.classify([(1, 1)])['status'], 'n/a')


if __name__ == '__main__':
    unittest.main()
