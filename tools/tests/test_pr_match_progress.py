# SPDX-License-Identifier: LGPL-3.0-or-later
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import pr_match_progress as progress


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

    def test_freshness_and_no_impact(self):
        with tempfile.TemporaryDirectory() as tmp:
            base, head = Path(tmp) / 'base', Path(tmp) / 'head'
            for root in (base, head):
                (root / progress.PROGRESS).parent.mkdir(parents=True)
                (root / progress.PROGRESS).write_text(json.dumps({'schema_version': 2, 'input_sha256': 'new'}))
            with patch.object(progress, 'input_hash', side_effect=['old', 'new']):
                progress.check(base, head)
            with patch.object(progress, 'input_hash', side_effect=['old', 'stale']):
                with self.assertRaisesRegex(ValueError, 'stale'):
                    progress.check(base, head)
            with patch.object(progress, 'input_hash', return_value='old'):
                progress.check(base, head)


if __name__ == '__main__':
    unittest.main()
