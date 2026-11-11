"""Exercise the lint CLI against small source trees."""
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "lint_reccmp_annotations.py"


class RatchetTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        (self.root / "src").mkdir()
        self.source = self.root / "src" / "one.cpp"

    def run_lint(self, *args):
        return subprocess.run([sys.executable, str(SCRIPT), str(self.root), *args],
                              text=True, capture_output=True)

    def baseline(self):
        result = self.run_lint("--update", "--allow-growth")
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_new_fixed_and_update_guard(self):
        self.source.write_text("void old() {}\n")
        self.baseline()
        self.source.write_text("void added() {}\n")
        result = self.run_lint()
        self.assertEqual(result.returncode, 1)
        self.assertIn("new:", result.stdout)
        self.assertIn("can be removed:", result.stdout)
        baseline = self.root / "docs/coverage/reccmp-lint-baseline.txt"
        before = baseline.read_text()
        self.assertEqual(self.run_lint("--update").returncode, 1)
        self.assertEqual(before, baseline.read_text())
        self.assertEqual(self.run_lint("--update", "--allow-growth").returncode, 0)
        self.source.write_text("// FUNCTION: LEGORR 0x1234\nvoid added() {}\n")
        result = self.run_lint()
        self.assertEqual(result.returncode, 0)
        self.assertIn("can be removed:", result.stdout)
        self.assertEqual(self.run_lint("--update").returncode, 0)
        self.assertNotIn("can be removed:", self.run_lint().stdout)

    def test_verbatim_move_and_scopes(self):
        first = "namespace A {\nstatic void helper(int x) {}\n}\n"
        second = "namespace B {\nstatic void helper(int x) {}\n}\n"
        self.source.write_text(first + second)
        self.baseline()
        self.source.write_text(second)
        (self.root / "src" / "two.cpp").write_text(first)
        self.assertEqual(self.run_lint().returncode, 0)
        (self.root / "src" / "two.cpp").write_text("")
        result = self.run_lint()
        self.assertEqual(result.returncode, 0)
        self.assertIn("definition A::helper", result.stdout)
        self.assertNotIn("definition B::helper", result.stdout)

    def test_duplicate_counts_and_ledger(self):
        self.source.write_text("void same() {}\n")
        ledger = self.root / "ledger.csv"
        ledger.write_text("address,status,corpus_name,symbol\n0x1234,named,Missing,\n")
        self.assertEqual(self.run_lint("--ledger", str(ledger), "--update", "--allow-growth").returncode, 0)
        (self.root / "src" / "two.cpp").write_text("void same() {}\n")
        self.assertEqual(self.run_lint("--ledger", str(ledger)).returncode, 1)
        self.source.write_text("// FUNCTION: LEGORR 0x1234\nvoid same() {}\n")
        result = self.run_lint("--ledger", str(ledger))
        self.assertEqual(result.returncode, 0)
        self.assertIn("ledger 0x00001234 Missing", result.stdout)


if __name__ == "__main__":
    unittest.main()
