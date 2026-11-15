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



class RelinkTests(unittest.TestCase):
    """The original-mode link (finding 687): no debug directory before `.rdata`."""

    def test_debug_lead_counts_extra_leading_zeros(self):
        gate = {'.rdata': (0x402000, 0x402010, b'\0' * 8 + b'\0\0\x80\x3f')}
        relink = {'.rdata': (0x402000, 0x402008, b'\0\0\x80\x3f')}
        self.assertEqual(di.debug_lead(gate, relink), {'.rdata': 8})
        self.assertEqual(di.debug_lead(relink, relink), {})

    def test_translator_uses_lead_then_anchors_and_checks_bytes(self):
        # Gate .rdata: an 8-byte lead, 1.0 and 2.0, a 4-byte gap, then the
        # named 3.0. The relink has no lead but an 8-byte gap (alignment), so
        # 1.0 and 2.0 move by -8 and the named symbol by -4 only: the items
        # before the first anchor need the lead.
        one, two, three = b'\0\0\x80\x3f', b'\0\0\0\x40', b'\0\0\x40\x40'
        gate = {'.rdata': (0x402000, 0x402018, b'\0' * 8 + one + two + b'\0' * 4 + three)}
        relink = {'.rdata': (0x402000, 0x402014, one + two + b'\0' * 8 + three)}
        frm = [(0x402014, '_s', 'b.obj')]
        to = [(0x402010, '_s', 'b.obj')]  # moved by -4: the alignment absorbed 4 bytes
        t = di.translator(frm, to, gate, relink, di.debug_lead(gate, relink))
        self.assertEqual(t(0x402008, 4), 0x402000)   # before the first anchor: by the lead
        self.assertEqual(t(0x40200c, 4), 0x402004)
        self.assertEqual(t(0x402014, 4), 0x402010)   # at the anchor
        self.assertEqual(t(0x401000, 4), 0x401000)   # outside every section
        no_lead = di.translator(frm, to, gate, relink)
        self.assertEqual(no_lead(0x402008, 4), 0x402004)  # the anchor's shift, unchecked bytes disagree

    def test_measure_reads_the_relink_and_keeps_the_gate(self):
        # Gate: an 8-byte zero lead before .rdata's two constants; the relink has none.
        one, two = b'\x01\x02\x03\x04', b'\x05\x06\x07\x08'
        original = fake_pe([('.rdata', 0x2000, 8, one + two), ('.data', 0x4000, 0x200000, b'\0' * 0x10)])
        gate = fake_pe([('.rdata', 0x2000, 16, b'\0' * 8 + one + two), ('.data', 0x3000, 0x200000, b'\0' * 0x10)])
        relink = fake_pe([('.rdata', 0x2000, 8, one + two), ('.data', 0x3000, 0x200000, b'\0' * 0x10)])
        entities = [{'orig': 0x402000, 'recomp': 0x402008, 'type': 2, 'size': 4},
                    {'orig': 0x402004, 'recomp': 0x40200c, 'type': 2, 'size': 4}]
        rows = [{'bss_range': '00404010-00404010'}]
        result = di.measure(original, gate, MAP, entities, rows, relink, MAP)
        self.assertEqual(result['image'], 'original-mode link')
        rdata = result['regions']['.rdata']
        self.assertEqual((rdata['identical_prefix']['bytes'], rdata['items']['at_original_offset'],
                          rdata['items']['at_original_address']), (8, 2, 2))
        self.assertEqual(result['gate']['.rdata']['at_original_offset'], 0)
        self.assertEqual(result['gate']['.rdata']['identical_prefix_bytes'], 0)
        # Without the relink's map, the gate is measured and only the .rdata prefix reads the relink.
        gate_only = di.measure(original, gate, MAP, entities, rows, relink)
        self.assertEqual((gate_only['image'], gate_only['regions']['.rdata']['identical_prefix']['bytes'],
                          gate_only['regions']['.rdata']['items']['at_original_offset']), ('gate', 8, 0))
        self.assertNotIn('gate', gate_only)


class ReadmeStatusTests(unittest.TestCase):
    def test_committed_progress_prints_data_identity(self):
        root = Path(__file__).resolve().parents[2]
        out = subprocess.run([sys.executable, str(root / 'tools/readme_status.py')], check=True,
                             capture_output=True, text=True).stdout
        self.assertIn('Data at original offsets', out)


if __name__ == '__main__':
    unittest.main()
