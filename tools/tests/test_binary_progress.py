# SPDX-License-Identifier: LGPL-3.0-or-later
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from binary_progress import compare_bytes, inversions, percent
from binary_progress_hash import input_hash


class ProgressTests(unittest.TestCase):
    def test_missing_bytes_are_not_identical(self):
        self.assertEqual(compare_bytes(b'abc', b'ab'), {
            'original_bytes': 3, 'rebuilt_bytes': 2,
            'identical_bytes': 2, 'identical_percent': 66.67})
        self.assertEqual(percent(0, 0), 0)

    def test_inversions_are_pairs_and_ties_do_not_count(self):
        entries = [{'address': hex(i), 'recomp': hex(n)}
                   for i, n in enumerate([3, 1, 2, 2])]
        self.assertEqual(inversions(entries), 3)
        self.assertEqual(inversions(list(reversed(entries))), 3)

    def test_hash_tracks_paths_content_and_deletions(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for name in ('src/a.c', 'CMakeLists.txt', 'cross/msvc5-toolchain.cmake', 'tools/reccmp.sh'):
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('same')
            first = input_hash(root)
            self.assertEqual(first, input_hash(root))
            (root / 'src/a.c').write_text('changed')
            self.assertNotEqual(first, input_hash(root))
            (root / 'src/a.c').write_text('same')
            (root / 'src/a.c').rename(root / 'src/b.c')
            self.assertNotEqual(first, input_hash(root))
            (root / 'src/b.c').unlink()
            self.assertNotEqual(first, input_hash(root))

if __name__ == '__main__':
    unittest.main()
