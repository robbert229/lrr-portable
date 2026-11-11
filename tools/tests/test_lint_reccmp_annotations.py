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

    def test_relocation_dropping_static_keeps_key(self):
        # relocate_unit.py removes `static` from definitions it shares.
        self.source.write_text("static inline int helper(int x) { return x; }\n")
        self.baseline()
        self.source.write_text("")
        (self.root / "src" / "unit.cpp").write_text("int helper(int x) { return x; }\n")
        result = self.run_lint()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn("can be removed:", result.stdout)
        (self.root / "src" / "unit.cpp").write_text("int helper(long x) { return x; }\n")
        self.assertEqual(self.run_lint().returncode, 1)

    def test_define_continuation_is_not_the_definition(self):
        # A multi-line #define between the previous body and an annotated
        # definition must not hide the annotation.
        self.source.write_text("void old() {}\n")
        self.baseline()
        self.source.write_text(
            "void old() {}\n"
            "#define TWICE(x) \\\n    ((x) + (x))\n"
            "// LIBRARY: LEGORR 0x1234 SYMBOL\n// _annotated\n"
            "int annotated(int x) { return TWICE(x); }\n")
        result = self.run_lint()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn("annotated", result.stdout)

    def test_annotation_above_if_group_covers_every_branch(self):
        # ADR 0006 reshape commits: one annotation above a group of whole
        # alternative definitions, nested groups and unconditional
        # directives between them included.
        self.source.write_text("")
        self.baseline()
        self.source.write_text(
            "// FUNCTION: LEGORR 0x1234\n"
            "#define HELPER(x) \\\n    ((x) + 1)\n\n"
            "#if defined(A)\n"
            "int alt(int x) { return HELPER(x); }\n"
            "#elif defined(B)\n"
            "#ifdef C\nint alt(int x) { return 2; }\n#else\nint alt(int x) { return 3; }\n#endif\n"
            "#else\n"
            "int alt(int x) { return 4; }\n"
            "#endif\n")
        result = self.run_lint()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn("alt", result.stdout)

    def test_annotation_separated_from_if_group_does_not_cover(self):
        self.source.write_text("")
        self.baseline()
        for between in ("/* note */\n", "int x;\n", "#if X\n#endif\n"):
            self.source.write_text(
                "// FUNCTION: LEGORR 0x1234\n" + between +
                "#if defined(A)\nint alt(int x) { return 1; }\n"
                "#else\nint alt(int x) { return 2; }\n#endif\n"
                "int after(int x) { return x; }\n")
            result = self.run_lint()
            self.assertEqual(result.returncode, 1, between)
            self.assertEqual(result.stdout.count("new: src/one.cpp"), 3, between + result.stdout)

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
