# SPDX-License-Identifier: LGPL-3.0-or-later
"""The coverage comment (tools/coverage.py) from synthetic gcovr and
diff-cover reports: no build, gcovr or diff-cover needed."""
from pathlib import Path
import sys
import unittest

TOOLS = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(TOOLS))
import coverage  # noqa: E402


def summary(*files):
    """A gcovr JSON summary with (filename, covered, total) per file."""
    return {"files": [{"filename": name, "line_covered": covered,
                       "line_total": total}
                      for name, covered, total in files]}


class HelperTests(unittest.TestCase):
    def test_area(self):
        self.assertEqual(coverage.area("src/game/Lego.cpp"), "src/game")
        self.assertEqual(coverage.area("src/platform/native/entry.cpp"), "src/platform")
        self.assertEqual(coverage.area("src/top.cpp"), "src")

    def test_by_area(self):
        areas = coverage.by_area(summary(("src/game/a.c", 1, 4),
                                         ("src/game/b.c", 2, 6),
                                         ("src/gods98/c.c", 0, 0)))
        self.assertEqual(areas, {"src/game": (3, 10), "src/gods98": (0, 0)})

    def test_ranges(self):
        self.assertEqual(coverage.ranges([9, 3, 4, 5, 4]), "3-5, 9")
        self.assertEqual(coverage.ranges([]), "")
        many = list(range(0, 2 * (coverage.MAX_RANGES + 3), 2))
        self.assertTrue(coverage.ranges(many).endswith(", and 3 more"))

    def test_change(self):
        self.assertEqual(coverage.change(50.0, 50.04), "±0.0")
        self.assertEqual(coverage.change(51.25, 50.0), "+1.2")
        self.assertEqual(coverage.change(48.0, 50.0), "-2.0")


class RenderTests(unittest.TestCase):
    SUMMARY = summary(("src/game/a.c", 30, 100), ("src/gods98/b.c", 45, 50))

    def test_marker_comes_first(self):
        # The workflow finds its own comment again by this prefix.
        self.assertTrue(coverage.render(self.SUMMARY).startswith(coverage.MARKER))

    def test_totals_without_base(self):
        text = coverage.render(self.SUMMARY)
        self.assertIn("**All of `src/`:** 50.0% (75 of 150 lines).", text)
        self.assertIn("| `src/game` | 100 | 30.0% |", text)
        self.assertIn("| `src/gods98` | 50 | 90.0% |", text)
        self.assertNotIn("Changed lines", text)

    def test_totals_against_base(self):
        base = summary(("src/game/a.c", 20, 100))
        text = coverage.render(self.SUMMARY, base=base)
        self.assertIn("+30.0 points against `main`", text)
        self.assertIn("| `src/game` | 100 | 30.0% | +10.0 |", text)
        self.assertIn("| `src/gods98` | 50 | 90.0% | new |", text)

    def test_changed_lines(self):
        diff = {"src_stats": {
            "src/game/a.c": {"covered_lines": [10, 11], "violation_lines": [12, 13, 20]},
            "src/gods98/b.c": {"covered_lines": [5], "violation_lines": []},
            "docs/notes.md": {"covered_lines": [], "violation_lines": []},
        }}
        text = coverage.render(self.SUMMARY, diff=diff)
        self.assertIn("**Changed lines:** 50.0% covered (3 of 6 instrumented "
                      "lines changed against `main`).", text)
        self.assertIn("| `src/game/a.c` | 2/5 | 12-13, 20 |", text)
        self.assertIn("| `src/gods98/b.c` | 1/1 | none |", text)
        self.assertNotIn("notes.md", text)
        # The file with the most missed lines comes first.
        self.assertLess(text.index("a.c` |"), text.index("b.c` |"))

    def test_no_changed_lines(self):
        text = coverage.render(self.SUMMARY, diff={"src_stats": {}}, base_name="develop")
        self.assertIn("changes no instrumented lines in `src/` against `develop`", text)

    def test_long_file_list_is_cut(self):
        stats = {f"src/game/f{i:02}.c": {"covered_lines": [], "violation_lines": [1]}
                 for i in range(coverage.MAX_FILES + 2)}
        text = coverage.render(self.SUMMARY, diff={"src_stats": stats})
        self.assertIn("| 2 more files | | |", text)
        self.assertNotIn(f"f{coverage.MAX_FILES + 1:02}.c", text)

    def test_run_link(self):
        text = coverage.render(self.SUMMARY, run_url="https://example.invalid/run/1")
        self.assertIn("[this run](https://example.invalid/run/1)", text)


if __name__ == "__main__":
    unittest.main()
