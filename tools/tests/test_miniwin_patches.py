# SPDX-License-Identifier: LGPL-3.0-or-later
"""tools/miniwin-patches.sh check rejects a series that is not numbered once.

Two PRs that each export a patch from the same series number theirs alike.
The script copy under test runs in a scratch tree, so these checks need no
miniwin checkout.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "miniwin-patches.sh"
SERIES = ROOT / "patches" / "miniwin"


def run_check(names):
    """Runs `check` in a scratch repository holding empty patches `names`."""
    with tempfile.TemporaryDirectory() as directory:
        tree = Path(directory)
        (tree / "tools").mkdir()
        shutil.copy(SCRIPT, tree / "tools" / SCRIPT.name)
        (tree / "patches" / "miniwin").mkdir(parents=True)
        for name in names:
            (tree / "patches" / "miniwin" / name).touch()
        return subprocess.run(
            ["bash", str(tree / "tools" / SCRIPT.name), "check"],
            capture_output=True,
            text=True,
        )


class DuplicateNumberTests(unittest.TestCase):
    def test_check_rejects_two_patches_with_one_number(self):
        result = run_check([
            "0031-feat-a.patch",
            "0032-build-b.patch",
            "0032-feat-c.patch",
            "0033-fix-d.patch",
        ])
        self.assertEqual(result.returncode, 1)
        self.assertIn("more than one patch is numbered 0032", result.stderr)
        self.assertIn("0032-build-b.patch", result.stderr)
        self.assertIn("0032-feat-c.patch", result.stderr)
        self.assertIn("Re-export", result.stderr)
        # The unique numbers are not named.
        self.assertNotIn("numbered 0031", result.stderr)
        self.assertNotIn("numbered 0033", result.stderr)

    def test_check_names_every_repeated_number(self):
        result = run_check([
            "0001-a.patch",
            "0001-b.patch",
            "0002-c.patch",
            "0002-d.patch",
        ])
        self.assertEqual(result.returncode, 1)
        self.assertIn("numbered 0001", result.stderr)
        self.assertIn("numbered 0002", result.stderr)

    def test_a_series_numbered_once_reaches_the_checkout_check(self):
        result = run_check(["0001-a.patch", "0002-b.patch", "0003-c.patch"])
        # The scratch tree has no submodule, so check fails there, not on numbers.
        self.assertEqual(result.returncode, 1)
        self.assertNotIn("more than one patch is numbered", result.stderr)
        self.assertIn("run tools/setup-miniwin.sh", result.stderr)

    def test_the_committed_series_is_numbered_once(self):
        names = sorted(path.name for path in SERIES.glob("*.patch"))
        self.assertTrue(names)
        for name in names:
            self.assertRegex(name, r"^\d{4}-")
        numbers = [name[:4] for name in names]
        self.assertEqual(numbers, sorted(set(numbers)))


if __name__ == "__main__":
    unittest.main()
