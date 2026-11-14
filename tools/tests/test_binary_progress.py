# SPDX-License-Identifier: LGPL-3.0-or-later
import hashlib
import json
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import binary_progress as progress
import binary_progress_hash as hashing
import translation_units
import layout_progress
from types import SimpleNamespace
from strip_pe_debug import strip


def fixture_pe(path, signed=False):
    """Minimal PE32 with three sections and a debug directory plus overlay."""
    data = bytearray(0xa20)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 0x3c, 0x80)
    data[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<HHIIIHH', data, 0x84, 0x14c, 3, 0, 0, 0, 224, 0x102)
    opt = 0x98
    struct.pack_into('<H', data, opt, 0x10b)
    struct.pack_into('<I', data, opt + 16, 0x1000)
    struct.pack_into('<I', data, opt + 28, 0x400000)
    struct.pack_into('<II', data, opt + 32, 0x1000, 0x200)
    struct.pack_into('<II', data, opt + 56, 0x4000, 0x400)
    struct.pack_into('<H', data, opt + 68, 2)
    struct.pack_into('<I', data, opt + 92, 16)
    if signed:
        struct.pack_into('<II', data, opt + 96 + 4 * 8, 0xa00, 32)
    struct.pack_into('<II', data, opt + 96 + 6 * 8, 0x2020, 28)
    for i, name in enumerate((b'.text', b'.rdata', b'.data')):
        off = opt + 224 + i * 40
        data[off:off + len(name)] = name
        struct.pack_into('<IIII', data, off + 8, 0x200, (i + 1) * 0x1000, 0x200, 0x400 + i * 0x200)
        struct.pack_into('<I', data, off + 36, 0x40000040 if i else 0x60000020)
    data[0x400:0x600] = bytes(range(256)) * 2
    data[0x400:0x410] = b'\x90' * 15 + b'\xc3'
    data[0x420:0x430] = b'\x90' * 15 + b'\xc3'
    data[0x620:0x63c] = b'D' * 28
    data[0xa00:] = b'Z' * 32
    path.write_bytes(data)


class ProgressTests(unittest.TestCase):
    def test_runs_and_chance(self):
        result = progress.compare_bytes(bytes(range(64)), bytes(range(32)) + b'X' * 32)
        self.assertEqual(result['run_identical_bytes'], 32)
        self.assertEqual(result['run_chance_percent'], 0)
        self.assertEqual(result['run_excess_percent_points'], 50)
        self.assertEqual(progress.run_bytes(b'a' * 15, b'a' * 15), 0)
        self.assertEqual(progress.percent(0, 0), 0)

    def test_inversions(self):
        entries = [{'address': hex(i), 'recomp': hex(n)} for i, n in enumerate([3, 1, 2, 2])]
        self.assertEqual(progress.inversions(entries), {'adjacent': 1, 'pairs': 3})
        self.assertEqual(progress.inversions(list(reversed(entries))), {'adjacent': 1, 'pairs': 3})

    def test_tracked_inputs_and_archive_fallback(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            subprocess.run(['git', 'init', '-q', str(root)], check=True)
            files = {'CMakeLists.txt': '${SRC_ROOT}/main.cpp', 'src/main.cpp': 'main',
                     'src/unused.cpp': 'unused', 'src/test_foo.cpp': 'test',
                     'src/platform/native/foo.cpp': 'native', 'src/a.h': 'header',
                     'tools/binary_progress.py': 'metric'}
            for name, content in files.items():
                p = root / name; p.parent.mkdir(parents=True, exist_ok=True); p.write_text(content)
            subprocess.run(['git', '-C', str(root), 'add', '.'], check=True)
            initial = hashing.input_hash(root)
            (root / 'src/main.cpp').write_text('changed')
            self.assertNotEqual(initial, hashing.input_hash(root))
            (root / 'src/main.cpp').write_text('main')
            (root / 'src/untracked.h').write_text('ignored')
            (root / 'src/test_foo.cpp').write_text('changed')
            (root / 'src/unused.cpp').write_text('changed')
            (root / 'src/platform/native/foo.cpp').write_text('changed')
            self.assertEqual(initial, hashing.input_hash(root))
            (root / 'tools/binary_progress.py').write_text('changed')
            self.assertNotEqual(initial, hashing.input_hash(root))
            (root / 'tools/binary_progress.py').write_text('metric')
            selected = hashing.input_paths(root)
            manifest = root / hashing.MANIFEST
            manifest.write_text('\n'.join(selected) + '\n')
            with patch('binary_progress_hash.subprocess.run', side_effect=FileNotFoundError):
                self.assertEqual(initial, hashing.input_hash(root))

    def test_strip_debug_and_reject_invalid(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'test.exe'
            fixture_pe(path)
            strip(path)
            data = path.read_bytes()
            self.assertEqual(len(data), 0xa00)
            self.assertEqual(data[0x620:0x63c], bytes(28))
            self.assertEqual(struct.unpack_from('<II', data, 0x98 + 96 + 6 * 8), (0, 0))
            fixture_pe(path, signed=True)
            before = path.read_bytes()
            with self.assertRaisesRegex(ValueError, 'signed'):
                strip(path)
            self.assertEqual(path.read_bytes(), before)
            fixture_pe(path)
            path.write_bytes(path.read_bytes()[:0x900])
            before = path.read_bytes()
            with self.assertRaisesRegex(ValueError, 'truncated'):
                strip(path)
            self.assertEqual(path.read_bytes(), before)
            path.write_bytes(b'MZ')
            with self.assertRaisesRegex(ValueError, 'truncated'):
                strip(path)

    def test_layout_function_attribution(self):
        try:
            import pefile
            import capstone
        except ImportError:
            self.skipTest('pefile and capstone required')
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); path = root / 'x.exe'; fixture_pe(path)
            pe = pefile.PE(str(path))
            original = SimpleNamespace(pe=pe, read=lambda a, n: pe.get_data(a - 0x400000, n))
            funcs = [SimpleNamespace(address=a, end=a + 16) for a in (0x401000, 0x401020)]
            rows = [dict(unit_id='A', text_start='00401000', text_end='00401020', rdata_range='', data_range='', string_range='', bss_range=''),
                    dict(unit_id='B', text_start='00401020', text_end='00401040', rdata_range='', data_range='', string_range='', bss_range='')]
            entries = [dict(address=hex(a), recomp=hex(b)) for a, b in ((0x401000, 0x401020), (0x401020, 0x401000))]
            entities = [dict(orig=int(e['address'], 16), recomp=int(e['recomp'], 16), type=1, size=16) for e in entries]
            mapfile = root / 'x.map'; mapfile.write_text(' 0001:00000000 _one 00401000 f one.obj\n')
            result = layout_progress.measure(original, pe, funcs, entries, entities, rows, set(), mapfile, [])
            self.assertEqual(result['unit_order_inversions']['pairs'], 1)
            self.assertEqual(result['unit_contiguity']['percent'], 100)
            self.assertEqual(result['unit_relative_identity']['percent'], 100)
            self.assertEqual(result['size_exact_prefix']['units'], 2)
            entities[0]['size'] = 15
            result = layout_progress.measure(original, pe, funcs, entries, entities, rows, set(), mapfile, [])
            self.assertEqual(result['size_exact_prefix']['units'], 0)
            self.assertEqual(result['size_exact_prefix']['size_exact_units'], 1)
            self.assertEqual(result['unit_relative_identity']['percent'], 50)

    def test_unlisted_annotation_counts_with_its_unit(self):
        # A mapped annotation that starts no listed function (a secondary
        # entry point) belongs to the unit whose range holds it: between its
        # unit's functions it keeps the run whole; elsewhere it breaks it.
        try:
            import pefile
            import capstone
        except ImportError:
            self.skipTest('pefile and capstone required')
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); path = root / 'x.exe'; fixture_pe(path)
            pe = pefile.PE(str(path))
            original = SimpleNamespace(pe=pe, read=lambda a, n: pe.get_data(a - 0x400000, n))
            funcs = [SimpleNamespace(address=a, end=a + 16) for a in (0x401000, 0x401020, 0x401040)]
            rows = [dict(unit_id='A', text_start='00401000', text_end='00401040', rdata_range='', data_range='', string_range='', bss_range=''),
                    dict(unit_id='B', text_start='00401040', text_end='00401060', rdata_range='', data_range='', string_range='', bss_range='')]
            def run(pairs):
                entries = [dict(address=hex(a), recomp=hex(b)) for a, b in pairs]
                entities = [dict(orig=a, recomp=b, type=1, size=16) for a, b in pairs]
                mapfile = root / 'x.map'; mapfile.write_text(' 0001:00000000 _one 00401000 f one.obj\n')
                return layout_progress.measure(original, pe, funcs, entries, entities, rows, set(), mapfile, [])
            inside = run([(0x401000, 0x401000), (0x401010, 0x401010), (0x401020, 0x401020), (0x401040, 0x401040)])
            self.assertEqual(inside['unit_contiguity']['units'], 2)
            self.assertEqual(inside['unlisted_mapped_functions'], 1)
            self.assertEqual(inside['unattributed_mapped_functions'], 0)
            after = run([(0x401000, 0x401000), (0x401010, 0x401048), (0x401020, 0x401020), (0x401040, 0x401040)])
            self.assertEqual(after['unit_contiguity']['units'], 1)
            reversed_ = run([(0x401000, 0x401010), (0x401010, 0x401000), (0x401020, 0x401020), (0x401040, 0x401040)])
            self.assertEqual(reversed_['unit_contiguity']['units'], 1)

    def test_anchored_data_offsets(self):
        try:
            import pefile
            import capstone
        except ImportError:
            self.skipTest('pefile and capstone required')
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); path = root / 'x.exe'; fixture_pe(path)
            pe = pefile.PE(str(path))
            original = SimpleNamespace(pe=pe, read=lambda a, n: pe.get_data(a - 0x400000, n))
            rows = [dict(unit_id='A', text_start='00401000', text_end='00401200', rdata_range='', data_range='00403010-00403030', string_range='', bss_range='')]
            entities = [dict(orig=a, recomp=b, type=2, size=4) for a,b in ((0x403010, 0x403040), (0x403020, 0x403050))]
            mapfile = root / 'x.map'; mapfile.write_text('')
            result = layout_progress.measure(original, pe, [], [], entities, rows, {0x403010, 0x403020}, mapfile, [])
            self.assertTrue(result['units'][0]['data_offset_agreement'])
            entities[1]['recomp'] += 4
            result = layout_progress.measure(original, pe, [], [], entities, rows, {0x403010, 0x403020}, mapfile, [])
            self.assertFalse(result['units'][0]['data_offset_agreement'])
            result = layout_progress.measure(original, pe, [], [], entities, rows, {0x403010}, mapfile, [])
            self.assertIsNone(result['units'][0]['data_offset_agreement'])

    def test_mask_absolute_and_relative_operands(self):
        try:
            import capstone
        except ImportError:
            self.skipTest('capstone required')
        a = bytes.fromhex('a100304000 e800100000 c3')
        b = bytes.fromhex('a100404000 e800200000 c3')
        self.assertTrue(layout_progress.identical(a, b, 0x401000, 0x402000, (0x400000, 0x500000), (0x400000, 0x500000)))
        self.assertFalse(layout_progress.identical(a, b[:-1] + b'\x90', 0x401000, 0x402000, (0x400000, 0x500000), (0x400000, 0x500000)))

    def test_manifest_is_current(self):
        root = Path(__file__).resolve().parents[2]
        self.assertEqual((root / hashing.MANIFEST).read_text().splitlines(), hashing.input_paths(root))

    def test_report_synthetic_pe(self):
        try:
            import pefile
            import capstone
        except ImportError:
            self.skipTest('pefile and capstone are needed for synthetic PE report test')
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); original = root / 'original.exe'; rebuilt = root / 'rebuilt.exe'
            fixture_pe(original); fixture_pe(rebuilt)
            entries = [{'address': '0x401000', 'recomp': '0x401000', 'matching': 1.0},
                       {'address': '0x401020', 'recomp': '0x401020', 'matching': 0.5, 'effective': True}]
            for name in ('results.json', 'game-only.json'):
                (root / name).write_text(json.dumps({'data': entries}))
            (root / 'entities.json').write_text('[]')
            manifest = root / 'manifest.csv'; ledger = root / 'ledger.csv'; areas = root / 'areas.csv'
            manifest.write_text('address\n00401000\n00401020\n')
            ledger.write_text('address,corpus_name,symbol,status\n00401000,first,,mapped\n00401020,second,,mapped\n')
            areas.write_text('address,area_id\n00401000,a\n00401020,a\n')
            with patch.multiple(translation_units, EXPECTED_SHA256=hashlib.sha256(original.read_bytes()).hexdigest(),
                                DATA_INIT=(0x403000, 0x403000), MANIFEST=manifest, LEDGER=ledger, AREAS=areas), \
                    patch.object(progress, 'input_hash', return_value='hash'), \
                    patch.object(layout_progress, 'measure', return_value={}), \
                    patch.object(layout_progress, 'read_rows', return_value=[]), \
                    patch.object(layout_progress, 'annotation_addresses', return_value=set()):
                result = progress.report(original, rebuilt, root)
            self.assertEqual(result['text_at_original_addresses']['identical_percent'], 100)
            self.assertEqual(result['fully_identical_functions_at_original_address'], {'count': 1, 'bytes': 16})
            self.assertEqual(result['accuracy_percent']['full'], 75.0)
            self.assertEqual(result['exact_function_code_bytes']['bytes'], 16)
            self.assertEqual(result['function_order_inversions']['game_only'], {'adjacent': 0, 'pairs': 0})

if __name__ == '__main__':
    unittest.main()
