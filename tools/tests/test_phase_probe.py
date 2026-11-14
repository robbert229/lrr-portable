# SPDX-License-Identifier: LGPL-3.0-or-later
"""tools/phase_probe.py: pad placement, listings, rows and the in-place guard."""
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import phase_probe as pp  # noqa: E402

UNIT = """\
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "a.h"
#include "b.h"

/* a comment

   spanning lines */
static int s_count;

#if defined(X)

#endif
int f(int a)
{
    const char *s = "{";

    return a;
}

int g(void) { return 1; }
"""


CXX_WRAPPED = """\
#include "a.h"
#ifdef __cplusplus
extern "C" {
#endif

int f(void) { return 0; }

#ifdef __cplusplus
}
#endif
"""


class PadPlacement(unittest.TestCase):
    def test_extern_c_wrapper_is_file_scope(self):
        lines = CXX_WRAPPED.splitlines(keepends=True)
        pp.check_pad_line(lines, 4)
        pp.check_pad_line(lines, 5)
        pp.check_pad_line(lines, 6)
        with self.assertRaises(pp.ProbeError):
            pp.check_pad_line(lines, 2)  # inside the #ifdef block

    def setUp(self):
        self.lines = UNIT.splitlines(keepends=True)

    def test_default_is_first_blank_after_include(self):
        self.assertEqual(pp.default_pad_line(self.lines), 3)

    def test_rejects_comment_scope_directive_and_if_block(self):
        for line in (6, 10, 15, 17, 2):  # comment, #if block, body, body, #include
            with self.subTest(line=line + 1), self.assertRaises(pp.ProbeError):
                pp.check_pad_line(self.lines, line)

    def test_accepts_file_scope_lines(self):
        for line in (3, 7, 12, 18, 19):
            pp.check_pad_line(self.lines, line)

    def test_string_brace_does_not_change_depth(self):
        pp.check_pad_line(self.lines, 19)

    def test_pads_keep_line_count_and_number_across_file(self):
        out = pp.pad_text(self.lines, {3: 2, 12: 1})
        self.assertEqual(out.count("\n"), UNIT.count("\n"))
        out_lines = out.splitlines()
        self.assertEqual(out_lines[3], "extern int phase_pad_0; extern int phase_pad_1; ")
        self.assertTrue(out_lines[12].startswith("extern int phase_pad_2; int f(int a)"))

    def test_zero_pads_is_identity(self):
        self.assertEqual(pp.pad_text(self.lines, {3: 0}), UNIT)

    def test_parse_spec(self):
        self.assertEqual(pp.parse_spec("16:52,901:3"), {15: 52, 900: 3})
        self.assertEqual(pp.parse_spec("4:1,4:2"), {3: 3})
        with self.assertRaises(pp.ProbeError):
            pp.parse_spec("16")


LISTING = """\
_f\tPROC NEAR\t\t\t\t\t; COMDAT
; 12   : {
\tmov\teax, DWORD PTR $S123
$L4567:
\tjmp\tSHORT $L4567
$exit$8910:
\tlea\tedx, DWORD PTR _buff$23614[esp+204]
\tret\t0
_f\tENDP
_g\tPROC NEAR
\tret\t0
_g\tENDP
"""


class Listings(unittest.TestCase):
    def test_body_strips_counter_numbers(self):
        body = pp.listing_body(LISTING, "_f")
        self.assertEqual(body, ["mov eax, DWORD PTR $N", "$N:", "jmp SHORT $N", "$exit$N:",
                                "lea edx, DWORD PTR _buff$N[esp+204]", "ret 0"])

    def test_counter_shift_keeps_fingerprint(self):
        shifted = LISTING.replace("123", "999").replace("4567", "4600").replace("8910", "8911").replace("23614", "23615")
        self.assertEqual(pp.fingerprint(pp.listing_body(LISTING, "_f")),
                         pp.fingerprint(pp.listing_body(shifted, "_f")))

    def test_missing_function(self):
        self.assertIsNone(pp.listing_body(LISTING, "_h"))
        self.assertEqual(pp.fingerprint(None), "ERR")

    def test_exact_symbol_match(self):
        self.assertEqual(pp.listing_body(LISTING, "_g"), ["ret 0"])


class Rows(unittest.TestCase):
    def test_letters(self):
        self.assertEqual(pp.letters_for(["x", "y", "x", "ERR", "z"]), "ABA!C")

    def test_period(self):
        self.assertEqual(pp.row_period("ABABABAB"), 2)
        self.assertEqual(pp.row_period("AAAA"), 1)
        self.assertEqual(pp.row_period("AB!BAB"), 2)
        self.assertIsNone(pp.row_period("ABCDEFGH"))

    def _row(self, row, scores):
        ns = list(range(len(row)))
        pr = pp.ProbeRow(0x401000, "f", row, ns, pp.build_codes(row, ns, 24))
        for code in pr.codes:
            code.matching = scores[code.letter]
        return pr.summary()

    def test_single_code(self):
        s = self._row("AAAA", {"A": 0.9})
        self.assertEqual((s["kind"], s["codes"], s["now"], s["shifts"]), ("single-code", 1, 90.0, []))

    def test_phase_sensitive_lists_every_best_shift(self):
        s = self._row("ABCABC", {"A": 0.9, "B": 1.0, "C": 1.0})
        self.assertEqual(s["kind"], "phase-sensitive")
        self.assertEqual((s["now"], s["best"], s["shifts"], s["period"]), (90.0, 100.0, [1, 2, 4, 5], 3))

    def test_current_best(self):
        s = self._row("ABAB", {"A": 0.95, "B": 0.8})
        self.assertEqual((s["kind"], s["shifts"]), ("current-best", []))

    def test_max_codes(self):
        self.assertEqual([c.letter for c in pp.build_codes("ABCD", [0, 1, 2, 3], 2)], ["A", "B"])


class InPlaceGuard(unittest.TestCase):
    def test_put_restore_and_line_count(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "u.c"
            src.write_text("a\nb\n")
            guard = pp.InPlace(src, Path(tmp) / "state")
            guard.put("x a\nb\n")
            self.assertEqual(src.read_text(), "x a\nb\n")
            self.assertTrue(guard.lock.exists())
            with self.assertRaises(pp.ProbeError):
                guard.put("a\nb\nc\n")
            guard.close()
            self.assertEqual(src.read_text(), "a\nb\n")
            self.assertFalse(guard.lock.exists())

    def test_killed_probe_is_undone(self):
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / "u.c"
            src.write_text("a\nb\n")
            state = Path(tmp) / "state"
            guard = pp.InPlace(src, state)
            guard.put("pad a\nb\n")
            # Simulate a kill: drop the flock without restoring.
            guard.flock.close()
            again = pp.InPlace(src, state)
            self.assertTrue(again.recovered)
            self.assertEqual(src.read_text(), "a\nb\n")
            again.close()


if __name__ == "__main__":
    unittest.main()
