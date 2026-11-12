# SPDX-License-Identifier: LGPL-3.0-or-later
import json
import struct
import sys
import tempfile
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


def pooled_fixture():
    """One .data piece, then two string COMDATs (finding 563)."""
    def section(size, flags):
        return struct.pack('<8sIIIIIIHHI', b'.data', 0, 0, size, 0, 0, 0, 0, 0, flags)
    comdat = 0xc0301040  # read/write initialised data, 4-byte aligned, COMDAT
    sections = section(5, 0xc0300040) + section(7, comdat) + section(6, comdat)
    symbols = b''.join([
        struct.pack('<8sIhHBB', b'_table', 0, 1, 0, 2, 0),
        struct.pack('<8sIhHBB', b'??_C@x', 0, 2, 0, 2, 0),
        struct.pack('<8sIhHBB', b'??_C@y', 0, 3, 0, 2, 0),
    ])
    header = struct.pack('<HHIIIHH', 0x14c, 3, 0, 20 + 3 * 40, 3, 0, 0)
    return header + sections + symbols + struct.pack('<I', 4)


class CoffTests(unittest.TestCase):
    def test_comdat_taken_from_another_object_takes_no_space(self):
        sections, _ = offsets.coff(pooled_fixture())
        self.assertEqual([s['base'] for s in sections[1:]], [0, 8, 16])
        sections, _ = offsets.coff(pooled_fixture(), lambda name: name == '??_C@x')
        self.assertEqual([s['discarded'] for s in sections[1:]], [False, True, False])
        self.assertEqual(sections[3]['base'], 8)

    def test_sections_symbols_and_commons(self):
        sections, symbols = offsets.coff(fixture())
        self.assertEqual(sections[1]['base'], 0)
        self.assertEqual(sections[2]['name'], '.rdata$long')
        self.assertEqual(sections[2]['base'], 8)
        self.assertTrue(sections[2]['comdat'])
        self.assertEqual(symbols[0]['name'], 'long_static_name')
        self.assertEqual(sections[2]['base'] + symbols[1]['value'], 10)
        self.assertEqual((symbols[2]['section'], symbols[2]['value']), (0, 20))

    def test_measure_uses_linked_object_not_stale_directory(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            build = root / 'build'
            (root / 'src/game').mkdir(parents=True)
            (root / 'docs/coverage').mkdir(parents=True)
            (root / 'src/link_order.cmake').write_text(
                'lrr_link(unit GAME-999 game game/probe.c)\n')
            (root / 'src/game/probe.c').write_text(
                '// GLOBAL: LEGORR 0x1004\nstatic int long_static_name;\n'
                '// STRING: LEGORR 0x100a\nsecond;\n')
            (root / 'docs/coverage/translation-unit-ranges.csv').write_text(
                'unit_id,rdata_range,data_range,string_range,bss_range\n'
                'GAME-999,00001004-0000100a,,,,\n')
            for target in ('link_001_probe', 'link_099_probe'):
                obj = build / ('CMakeFiles/' + target + '.dir/src/game/probe.c.obj')
                obj.parent.mkdir(parents=True)
                obj.write_bytes(fixture())
            response = build / 'CMakeFiles/program.dir/objects1.rsp'
            response.parent.mkdir(parents=True)
            response.write_text('CMakeFiles/link_001_probe.dir/src/game/probe.c.obj')
            (build / 'LegoRR.map').write_text(
                ' 0001:00000000 _probe 00401000 f probe.c.obj\n'
                ' 0002:0000000a _second 0040200a probe.c.obj\n')
            (build / 'reccmp-report').mkdir()
            (build / 'reccmp-report/entities.json').write_text(json.dumps(
                [{'orig': 0x100a, 'recomp': 0x40200a, 'type': 4}]))
            result = offsets.measure(root, build)['units'][0]
            self.assertEqual(result['sections']['.rdata']['status'], 'gaps-only')
            self.assertEqual(result['sections']['.rdata']['count'], 2)

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
