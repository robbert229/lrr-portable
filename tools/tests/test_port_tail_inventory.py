# SPDX-License-Identifier: LGPL-3.0-or-later
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import msvc_coff
import port_tail_inventory as inv

CODE, DATA = msvc_coff.IMAGE_SCN_CNT_CODE, 0x40


def obj(path, sections, symbols):
    """A CoffObject from (name, flags, size, comdat, [(symbol, type)]) sections
    and (name, section, storage) symbols; relocations name symbols."""
    syms = {i: msvc_coff.Symbol(i, n, 0, sec, 0x20 if sec and sections[sec - 1][1] == CODE else 0, cls, [])
            for i, (n, sec, cls) in enumerate(symbols)}
    index = {s.name: i for i, s in syms.items()}
    out = []
    for i, (name, flags, size, comdat, relocs) in enumerate(sections, 1):
        sect = msvc_coff.Section(i, name, flags, b'', size,
                                 [msvc_coff.Relocation(0, index[r], 6) for r in relocs])
        sect.comdat_selection = 2 if comdat else None
        out.append(sect)
    return msvc_coff.CoffObject(path, out, syms, [], [], None)


class ReachabilityTests(unittest.TestCase):
    def setUp(self):
        # The unit calls tail A's `a`. A's discarded `dead` references B's
        # `b`, which pulls B; B's table (non-COMDAT data) keeps `t`, which
        # calls C's `c`. C's `unused` is dropped.
        self.objects = {
            Path('unit.obj'): obj('unit.obj', [('.text', CODE, 16, True, ['_a'])],
                                  [('_u', 1, 2), ('_a', 0, 2)]),
            Path('a.obj'): obj('a.obj', [('.text', CODE, 32, True, []), ('.text', CODE, 48, True, ['_b'])],
                               [('_a', 1, 2), ('_dead', 2, 2), ('_b', 0, 2)]),
            Path('b.obj'): obj('b.obj', [('.text', CODE, 64, True, []), ('.text', CODE, 80, True, ['_c']),
                                         ('.rdata', DATA, 8, False, ['_t'])],
                               [('_b', 1, 2), ('_t', 2, 3), ('_table', 3, 3), ('_c', 0, 2)]),
            Path('c.obj'): obj('c.obj', [('.text', CODE, 96, True, []), ('.text', CODE, 112, True, [])],
                               [('_c', 1, 2), ('_unused', 2, 2)]),
        }
        self.libraries = {Path('unit.obj'): None, Path('a.obj'): 'lrr_tail',
                          Path('b.obj'): 'lrr_tail', Path('c.obj'): 'lrr_tail'}

    def run_model(self):
        return inv.reachability(self.libraries, lambda p: 'UNIT', read=lambda p: self.objects[p])

    def test_archive_resolution_and_anchor(self):
        r = self.run_model()
        # a (32), t (80) through b's table, c (96); not dead, b or unused.
        self.assertEqual(r['kept_code_bytes'], 32 + 80 + 96)
        self.assertEqual(r['compiled_code_bytes'], 32 + 48 + 64 + 80 + 96 + 112)
        self.assertEqual(r['kept_by_object']['b.obj'], 80)
        self.assertEqual([a['object'] for a in r['anchors']], ['b.obj'])
        self.assertEqual(r['anchors'][0]['code_bytes_only_through'], 80 + 96)
        self.assertEqual(r['code_bytes_without_anchors'], 32)
        self.assertEqual(r['by_referrer']['UNIT']['code_bytes_only_through'], 32 + 80 + 96)
        self.assertEqual(r['root_objects']['a.obj']['code_bytes_kept_alone'], 32 + 80 + 96)
        self.assertEqual(r['greedy_order'][-1]['code_bytes_left'], 0)


class HelperTests(unittest.TestCase):
    def test_data_references_are_aligned_image_dwords(self):
        data = (0x401000).to_bytes(4, 'little') + (5).to_bytes(4, 'little') + (0x402000).to_bytes(4, 'little')
        read = lambda a, n: data[a - 0x500000:a - 0x500000 + n]
        self.assertEqual(inv.data_references(read, 0x500000, 12, (0x400000, 0x500000)), [0x401000, 0x402000])

    def test_text_runs(self):
        symbols = [(0, 4, 'a', 'x', 'unit', '.text'), (4, 8, 'b', 'y', 'unit', '.text'),
                   (8, 12, 'c', 'z', 'tail', '.text'), (12, 16, 'd', 'w', 'tail', '.data')]
        self.assertEqual(inv.text_runs(symbols), [['unit', 0, 8, 2], ['tail', 8, 12, 1]])

    def test_unit_spans(self):
        symbols = [(0x401000, 0x401010, 'u.c.obj', 'f', 'unit', '.text'),
                   (0x401010, 0x401020, 'v.c.obj', 'g', 'unit', '.text')]
        rows = [{'unit_id': 'U', 'unit_name': 'U.c', 'text_start': '00401000', 'text_end': '00401020'}]
        self.assertEqual(inv.unit_spans(symbols, {'u.c.obj': 'U'}, rows)[0]['delta'], -16)


if __name__ == '__main__':
    unittest.main()
