# SPDX-License-Identifier: LGPL-3.0-or-later
import copy
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import pr_match_progress as progress
import match_baseline
from types import SimpleNamespace


class ProgressTest(unittest.TestCase):
    def test_direction_and_baseline_movers(self):
        data = json.loads((Path(__file__).resolve().parents[2] / progress.PROGRESS).read_text())
        new = copy.deepcopy(data)
        new['accuracy_percent']['full'] += 1
        new['function_order_inversions']['full']['adjacent'] += 1
        with tempfile.TemporaryDirectory() as tmp:
            base, head = Path(tmp) / 'base', Path(tmp) / 'head'
            for root in (base, head):
                (root / progress.PROGRESS).parent.mkdir(parents=True)
                (root / progress.BASELINE).write_text('[game-only] 1 mapped functions\n0x00401000 50.00 name\n[full] 1 mapped functions\n0x00401000 50.00 name\n')
            (base / progress.PROGRESS).write_text(json.dumps(data))
            (head / progress.PROGRESS).write_text(json.dumps(new))
            (head / progress.BASELINE).write_text('[game-only] 1 mapped functions\n0x00401000 60.00 name\n[full] 1 mapped functions\n0x00401000 40.00 name\n')
            with patch.object(progress, 'input_hash', return_value='same'):
                result = progress.render(base, head)
            self.assertIn('⬆️ improvement (+1.00)', result)
            self.assertIn('⬇️ regression (+1.00)', result)
            self.assertIn('1 rose, 0 fell', result)
            self.assertIn('0 rose, 1 fell', result)
            self.assertIn('net -10.00', result)

    def test_baseline_writer_header_and_reader(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for _, filename in match_baseline.SCOPES:
                (root / filename).write_text(json.dumps([{'address': '0x401000', 'matching': 0.5, 'name': 'test'}]))
            path = root / 'baseline.txt'
            with patch.object(match_baseline, 'input_hash', return_value='a' * 64):
                self.assertEqual(match_baseline.command_update(SimpleNamespace(report_dir=str(root), baseline=str(path))), 0)
            self.assertTrue(path.read_text().startswith('# input_sha256: ' + 'a' * 64 + '\n'))
            self.assertEqual(match_baseline.load_baseline(path)['full'][0x401000], ('50.00', 'test'))

    def test_freshness_guards_baseline(self):
        with tempfile.TemporaryDirectory() as tmp:
            head = Path(tmp)
            (head / progress.PROGRESS).parent.mkdir(parents=True)
            fingerprint = 'a' * 64
            baseline = ('# input_sha256: ' + fingerprint + '\nscore 50.00\n').encode()
            def write(b, fingerprint_value=fingerprint, digest=None):
                (head / progress.BASELINE).write_bytes(b)
                (head / progress.PROGRESS).write_text(json.dumps({
                    'schema_version': 2, 'input_sha256': fingerprint_value,
                    'baseline_sha256': digest or hashlib.sha256(baseline).hexdigest()}))
            with patch.object(progress, 'input_hash', return_value=fingerprint):
                write(baseline)
                progress.check(head, head)
                for bad in (baseline.replace(b'50.00', b'51.00'),
                            baseline.replace(b'a' * 64, b'b' * 64),
                            b'score 50.00\n'):
                    write(bad)
                    with self.assertRaisesRegex(ValueError, 'never hand-merge'):
                        progress.check(head, head)
                write(baseline, fingerprint_value='b' * 64)
                with self.assertRaisesRegex(ValueError, 'never hand-merge'):
                    progress.check(head, head)
                write(baseline)
                with patch.object(progress, 'input_hash', return_value='c' * 64):
                    with self.assertRaisesRegex(ValueError, 'never hand-merge'):
                        progress.check(head, head)


if __name__ == '__main__':
    unittest.main()
