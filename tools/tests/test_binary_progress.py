# SPDX-License-Identifier: LGPL-3.0-or-later
import json
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import binary_progress as progress
import binary_progress_hash as hashing
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

    def test_manifest_is_current(self):
        root = Path(__file__).resolve().parents[2]
        self.assertEqual((root / hashing.MANIFEST).read_text().splitlines(), hashing.input_paths(root))

    def test_report_synthetic_pe(self):
        try:
            import pefile
        except ImportError:
            self.skipTest('pefile is needed for synthetic PE report test')
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp); original = root / 'original.exe'; rebuilt = root / 'rebuilt.exe'
            fixture_pe(original); fixture_pe(rebuilt)
            pe = pefile.PE(str(original))
            class Original:
                def __init__(self, path):
                    self.pe = pe
                    self.text = (0x401000, 0x401200)
                def read(self, va, size):
                    return pe.get_memory_mapped_image()[va - 0x400000:va - 0x400000 + size]
            entries = [{'address': '0x401000', 'recomp': '0x401000', 'matching': 1.0},
                       {'address': '0x401020', 'recomp': '0x401020', 'matching': 0.5, 'effective': True}]
            for name in ('results.json', 'game-only.json'):
                (root / name).write_text(json.dumps({'data': entries}))
            funcs = [SimpleNamespace(address=0x401000, end=0x401010), SimpleNamespace(address=0x401020, end=0x401030)]
            with patch.object(progress, 'Binary', Original), patch.object(progress, 'load_functions', return_value=funcs), patch.object(progress, 'input_hash', return_value='hash'):
                result = progress.report(original, rebuilt, root)
            self.assertEqual(result['text_at_original_addresses']['identical_percent'], 100)
            self.assertEqual(result['fully_identical_functions_at_original_address'], {'count': 1, 'bytes': 16})
            self.assertEqual(result['accuracy_percent']['full'], 75.0)
            self.assertEqual(result['exact_function_code_bytes']['bytes'], 16)
            self.assertEqual(result['function_order_inversions']['game_only'], {'adjacent': 0, 'pairs': 0})

if __name__ == '__main__':
    unittest.main()
