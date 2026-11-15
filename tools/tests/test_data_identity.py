# SPDX-License-Identifier: LGPL-3.0-or-later
import subprocess
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import data_identity as di


def fake_pe(sections, base=0x400000):
    """A stand-in for pefile.PE: (name, rva, virtual size, bytes) per section."""
    return SimpleNamespace(
        OPTIONAL_HEADER=SimpleNamespace(ImageBase=base),
        sections=[SimpleNamespace(Name=name.encode().ljust(8, b'\0'), VirtualAddress=rva,
                                  Misc_VirtualSize=size, get_data=lambda data=data: data)
                  for name, rva, size, data in sections])


MAP = """
 Start         Length     Name                   Class
 0003:00000000 00000010H .data                   DATA
 0003:00000010 00000040H .bss                    DATA

  Address         Publics by Value              Rva+Base   Lib:Object

 0003:00000000       _a_str                     00403000   a.obj
 0003:00000008       _b_str                     00403008   b.obj
 0003:00000010       _a_globs                   00403010   a.obj
 0003:00000020       _b_globs                   00403020   b.obj
 0003:00000030       _c_common                  00403030   <common>
"""


class RegionTests(unittest.TestCase):
    def test_original_bss_start_is_the_lowest_unit_bss(self):
        rows = [{'bss_range': '00403020-00403030'}, {'bss_range': ''}, {'bss_range': '00403010-00403010'}]
        self.assertEqual(di.original_bss_start(rows), 0x403010)
        self.assertIsNone(di.original_bss_start([{'bss_range': ''}]))

    def test_parse_map_groups_and_symbols(self):
        groups, symbols = di.parse_map(MAP)
        self.assertEqual(groups['.bss'], (3, 0x10))
        self.assertEqual(symbols[0], (0x403000, '_a_str', 'a.obj'))
        self.assertEqual(symbols[-1], (0x403030, '_c_common', '<common>'))

    def test_regions_split_the_data_section(self):
        secs = {'.rdata': (0x402000, 0x402010, b''), '.data': (0x403000, 0x403100, b'')}
        self.assertEqual(di.regions(secs, 0x403010, 0x403030),
                         {'.rdata': (0x402000, 0x402010), '.data': (0x403000, 0x403010),
                          '.bss': (0x403010, 0x403030), 'commons': (0x403030, 0x403100)})
        self.assertEqual(list(di.regions(secs, None, None)), ['.rdata'])

    def test_raw_prefix(self):
        self.assertEqual(di.raw_prefix(b'abcd', b'abXd'), 2)
        self.assertEqual(di.raw_prefix(b'ab', b'abc'), 2)


class MeasureTests(unittest.TestCase):
    def test_offsets_objects_and_first_difference(self):
        # Original: .data 0x10 bytes at 0x404000 then .bss from 0x404010, commons from ORIGINAL_COMMONS_START.
        # Rebuilt: everything one page later, b.obj's .bss item 4 bytes early.
        orig_data = b'Astr\0\0\0\0Bstr\0\0\0\0'
        new_data = b'Astr\0\0\0\0Bxtr\0\0\0\0'
        original = fake_pe([('.rdata', 0x2000, 4, b'\x01\x02\x03\x04'), ('.data', 0x4000, 0x200000, orig_data)])
        rebuilt = fake_pe([('.rdata', 0x2000, 4, b'\x01\x02\x09\x04'), ('.data', 0x3000, 0x200000, new_data)])
        rows = [{'bss_range': '00404010-00404010'}]
        common = di.ORIGINAL_COMMONS_START
        entities = [
            {'orig': 0x404000, 'recomp': 0x403000, 'type': 4, 'size': 5},   # a.obj string, in place
            {'orig': 0x404008, 'recomp': 0x403008, 'type': 4, 'size': 5},   # b.obj string, in place
            {'orig': 0x404010, 'recomp': 0x403010, 'type': 2, 'size': 16},  # a.obj globs, in place
            {'orig': 0x404024, 'recomp': 0x403020, 'type': 2, 'size': 16},  # b.obj globs, 4 early
            {'orig': common, 'recomp': 0x403030, 'type': 2, 'size': 4},     # the common, in place
            {'orig': 0x401000, 'recomp': 0x401000, 'type': 1, 'size': 8},   # a function: ignored
        ]
        result = di.measure(original, rebuilt, MAP, entities, rows)['regions']
        self.assertEqual(result['.rdata']['identical_prefix'], {'bytes': 2, 'first_difference': '0x402002',
                                                                'rebuilt_object': None})
        data = result['.data']
        self.assertEqual((data['start_delta'], data['identical_prefix']['bytes'],
                          data['identical_prefix']['first_difference'], data['identical_prefix']['rebuilt_object']),
                         (-0x1000, 9, '0x404009', 'b.obj'))
        self.assertEqual((data['items']['paired'], data['items']['at_original_offset'],
                          data['items']['at_original_address']), (2, 2, 0))
        self.assertIsNone(data['first_difference'])
        bss = result['.bss']
        self.assertEqual((bss['items']['at_original_offset'], bss['objects']['at_original_offset'],
                          bss['objects']['agreeing']), (1, 1, ['a.obj']))
        self.assertEqual(bss['first_difference'], {'original': '0x404024', 'rebuilt': '0x403020',
                                                   'rebuilt_region': '.bss', 'offset_delta': -4, 'object': 'b.obj'})
        self.assertNotIn('identical_prefix', bss)
        self.assertEqual(result['commons']['items']['at_original_offset_bytes'], 4)

    def test_item_in_another_region_has_no_offset_delta(self):
        original = fake_pe([('.rdata', 0x2000, 4, b'\0' * 4), ('.data', 0x4000, 0x20, b'\0' * 0x20)])
        rebuilt = fake_pe([('.rdata', 0x2000, 4, b'\0' * 4), ('.data', 0x3000, 0x20, b'\0' * 0x20)])
        entities = [{'orig': 0x404000, 'recomp': 0x402000, 'type': 2, 'size': 4}]
        result = di.measure(original, rebuilt, MAP, entities, [{'bss_range': '00404010-00404010'}])
        first = result['regions']['.data']['first_difference']
        self.assertEqual((first['rebuilt_region'], first['offset_delta']), ('.rdata', None))


class ReadmeStatusTests(unittest.TestCase):
    def test_committed_progress_prints_data_identity(self):
        root = Path(__file__).resolve().parents[2]
        out = subprocess.run([sys.executable, str(root / 'tools/readme_status.py')], check=True,
                             capture_output=True, text=True).stdout
        self.assertIn('Data at original offsets:', out)


if __name__ == '__main__':
    unittest.main()
