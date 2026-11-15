# SPDX-License-Identifier: LGPL-3.0-or-later
import subprocess
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import identity_metrics as im


def reader(base, data):
    return lambda a, n: bytes(data[a - base:a - base + n])


# jmp [eax*4+0x100c]; ret; ret; nop x3; table dd 0x1007, 0x1008; int3 padding.
SWITCH = (bytes.fromhex('ff24850c100000') + b'\xc3\xc3' + b'\x90' * 3 +
          (0x1007).to_bytes(4, 'little') + (0x1008).to_bytes(4, 'little') + b'\xcc' * 12)


class CodeExtentTests(unittest.TestCase):
    def setUp(self):
        try:
            import capstone  # noqa: F401
        except ImportError:
            self.skipTest('capstone required')

    def test_plain_function_excludes_padding(self):
        code = bytes.fromhex('55 8bec 5d c3') + b'\xcc' * 11
        self.assertEqual(im.code_extent(reader(0x1000, code), 0x1000, 0x1010), 5)

    def test_switch_table_counts_with_its_function(self):
        self.assertEqual(im.code_extent(reader(0x1000, SWITCH), 0x1000, 0x1020), 20)

    def test_stops_at_next_known_start(self):
        code = bytes.fromhex('c3') + bytes.fromhex('c3') + b'\xcc' * 14
        self.assertEqual(im.code_extent(reader(0x1000, code), 0x1000, 0x1001), 1)


class SizeTests(unittest.TestCase):
    def setUp(self):
        try:
            import capstone  # noqa: F401
        except ImportError:
            self.skipTest('capstone required')

    def test_function_sizes_and_correct_size(self):
        # Original: two 16-byte slots; the rebuilt second function is one byte longer.
        orig = bytes.fromhex('c3') + b'\xcc' * 15 + bytes.fromhex('5dc3') + b'\xcc' * 14
        new = bytes.fromhex('c3') + b'\xcc' * 15 + bytes.fromhex('5d5dc3') + b'\xcc' * 13
        entries = [{'address': '0x1000', 'recomp': '0x1000', 'matching': 1.0},
                   {'address': '0x1010', 'recomp': '0x1010', 'matching': 0.5},
                   {'address': '0x1020', 'recomp': None}]
        entities = [{'orig': 0x1000, 'recomp': 0x1000, 'type': 1, 'size': 1},
                    {'orig': 0x1010, 'recomp': 0x1010, 'type': 1, 'size': 3}]
        sizes = im.function_sizes(entries, entities, (reader(0x1000, orig), {0x1000, 0x1010}, 0x1020),
                                  (reader(0x1000, new), {0x1000, 0x1010}, 0x1020))
        self.assertEqual(sizes, {0x1000: (1, 1), 0x1010: (2, 3)})
        result = im.correct_size(entries + [{'address': '0x1030', 'recomp': '0x1030'}], sizes)
        self.assertEqual(result, {'count': 1, 'bytes': 1, 'compared': 2, 'uncompared': 1,
                                  'mapped': 3, 'exact_functions': 1})

    def test_unit_size_agreement(self):
        funcs = [SimpleNamespace(address=a) for a in (0x1000, 0x1010, 0x1020, 0x1030)]
        rows = [{'unit_id': 'A', 'text_start': '00001000', 'text_end': '00001020'},
                {'unit_id': 'B', 'text_start': '00001020', 'text_end': '00001040'},
                {'unit_id': 'C', 'text_start': '00001040', 'text_end': '00001050'}]
        entries = [{'address': hex(a), 'recomp': hex(r)} for a, r in
                   ((0x1000, 0x1000), (0x1010, 0x1010), (0x1020, 0x1024), (0x1030, 0x1030))]
        sizes = {0x1000: (4, 4), 0x1010: (6, 6), 0x1020: (8, 8), 0x1030: (5, 4)}
        result = im.unit_size_agreement(rows, funcs, entries, sizes)
        self.assertEqual((result['units'], result['eligible_units'], result['correct_size_units'],
                          result['start_agreeing_units'], result['end_agreeing_units']), (1, 2, 1, 1, 1))
        self.assertEqual(result['agreeing_unit_ids'], ['A'])
        b = result['per_unit'][1]
        self.assertEqual((b['correct_size'], b['start_agrees'], b['end_agrees']), (False, False, False))
        sizes[0x1030] = (5, 5); entries[2]['recomp'] = '0x1020'
        self.assertEqual(im.unit_size_agreement(rows, funcs, entries, sizes)['units'], 2)


class PrefixTests(unittest.TestCase):
    funcs = [SimpleNamespace(address=0x1000, end=0x100b, name='f'), SimpleNamespace(address=0x1010, end=0x1011, name='g')]
    # mov eax, [0x2000]; call +0; ret  /  padding  /  ret
    orig = bytes.fromhex('a100200000 e800000000 c3') + b'\xcc' * 5 + b'\xc3'

    def test_raw_prefix_and_attribution(self):
        new = bytes.fromhex('a104200000 e800000000 c3') + b'\xcc' * 5 + b'\xc3'
        result = im.identical_prefix(self.orig, new, 0x1000, self.funcs)
        self.assertEqual((result['bytes'], result['first_difference'], result['function'], result['offset_in_function']),
                         (1, '0x1001', 'f', 1))
        whole = im.identical_prefix(self.orig, self.orig, 0x1000, self.funcs)
        self.assertEqual((whole['bytes'], whole['first_difference'], whole['percent']), (17, None, 100.0))
        short = im.identical_prefix(self.orig, self.orig[:4], 0x1000, self.funcs)
        self.assertEqual(short['first_difference'], '0x1004')

    def test_masked_prefix(self):
        try:
            import capstone  # noqa: F401
        except ImportError:
            self.skipTest('capstone required')
        image = (0x1000, 0x3000)
        moved = bytes.fromhex('a104200000 e810000000 c3') + b'\xcc' * 5 + b'\xc3'
        self.assertEqual(im.masked_prefix(self.orig, moved, 0x1000, self.funcs, image, image)['first_difference'], None)
        changed = bytes.fromhex('a104200000 e810000000 c3') + b'\xcc' * 5 + b'\xc2'
        result = im.masked_prefix(self.orig, changed, 0x1000, self.funcs, image, image)
        self.assertEqual((result['bytes'], result['function']), (16, 'g'))
        padded = bytes.fromhex('a104200000 e810000000 c3') + b'\x90' + b'\xcc' * 4 + b'\xc3'
        self.assertEqual(im.masked_prefix(self.orig, padded, 0x1000, self.funcs, image, image)['first_difference'], '0x100b')


class PrefixRecordTests(unittest.TestCase):
    """The identical prefix reads the original-mode link when given (finding 687)."""

    def test_relink_measured_gate_kept(self):
        try:
            import capstone  # noqa: F401
        except ImportError:
            self.skipTest('capstone required')
        code = bytes.fromhex('a100204000 c3')  # mov eax, [0x402000]; ret
        def pe(text):
            image = bytearray(0x3000)
            image[0x1000:0x1000 + len(text)] = text
            return SimpleNamespace(OPTIONAL_HEADER=SimpleNamespace(ImageBase=0x400000, SizeOfImage=0x3000),
                                   get_memory_mapped_image=lambda: bytes(image))
        original = SimpleNamespace(text=(0x401000, 0x401006), pe=pe(code),
                                   read=lambda a, n: code[a - 0x401000:a - 0x401000 + n])
        funcs = [SimpleNamespace(address=0x401000, end=0x401006, name='f')]
        gate = pe(bytes.fromhex('a154204000 c3'))     # the operand 0x54 bytes late
        relink = pe(code)
        record = im.prefix_record(original, gate, funcs, relink)
        self.assertEqual((record['image'], record['bytes'], record['first_difference']),
                         ('original-mode link', 6, None))
        self.assertEqual((record['gate']['bytes'], record['gate']['first_difference']), (1, '0x401001'))
        self.assertEqual(record['gate']['address_operands_masked']['first_difference'], None)
        alone = im.prefix_record(original, gate, funcs)
        self.assertEqual((alone['image'], alone['bytes']), ('gate', 1))
        self.assertNotIn('gate', alone)


class ReadmeStatusTests(unittest.TestCase):
    def test_committed_progress_prints_identity_metrics(self):
        root = Path(__file__).resolve().parents[2]
        out = subprocess.run([sys.executable, str(root / 'tools/readme_status.py')], check=True,
                             capture_output=True, text=True).stdout
        for label in ('Correct-size functions:', 'Identical prefix', 'Units with size agreement:'):
            self.assertIn(label, out)


if __name__ == '__main__':
    unittest.main()
