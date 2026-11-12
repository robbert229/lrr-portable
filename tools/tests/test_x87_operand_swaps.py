# SPDX-License-Identifier: LGPL-3.0-or-later
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import x87_operand_swaps as swaps


def diff(orig, recomp, both_before=(), both_after=()):
    """A one-hunk Reccmp diff: shared rows, one replace block, shared rows."""
    blocks = []
    if both_before:
        blocks.append({"both": [["0x1", row, "0x1"] for row in both_before]})
    blocks.append({"orig": [["0x2", row] for row in orig],
                   "recomp": [["0x2", row] for row in recomp]})
    if both_after:
        blocks.append({"both": [["0x3", row, "0x3"] for row in both_after]})
    return [["@@ -0x1,1 +0x1,1 @@", blocks]]


def entry(orig, recomp, **kw):
    return {"address": "0x401000", "name": "f", "matching": 0.5,
            "diff": diff(orig, recomp, **kw)}


class SwapTest(unittest.TestCase):
    def test_load_and_multiply_swap(self):
        found = swaps.survey([entry(
            ["fld dword ptr [a (DATA)]", "fmul dword ptr [b (DATA)]"],
            ["fld dword ptr [b (DATA)] \t(x.c:3)", "fmul dword ptr [a (DATA)]"])], {})
        self.assertEqual(len(found), 1)
        self.assertEqual(found[0]["swaps"][0]["kind"], "rule-a")
        self.assertEqual(found[0]["swaps"][0]["operands"],
                         ["dword ptr [a (DATA)]", "dword ptr [b (DATA)]"])

    def test_chained_products_and_integer_code_between(self):
        found = swaps.survey([entry(
            ["fild dword ptr [esp + 4]", "fmul dword ptr [a (DATA)]", "mov eax, 1",
             "fmul dword ptr [esp + 8]"],
            ["fild dword ptr [esp + 4]", "fmul dword ptr [esp + 8]", "mov eax, 1",
             "fmul dword ptr [a (DATA)]"])], {})
        self.assertEqual(len(found), 1)

    def test_reversed_subtraction(self):
        found = swaps.find_swaps(
            [("fld", "dword ptr [a (DATA)]"), ("fsub", "dword ptr [esp + 4]")],
            [("fld", "dword ptr [esp + 4]"), ("fsubr", "dword ptr [a (DATA)]")])
        self.assertEqual([s["kind"] for s in found], ["rule-a"])

    def test_value_on_the_stack(self):
        found = swaps.find_swaps(
            [("fld", "st(0)"), ("fmul", "dword ptr [eax]")],
            [("fld", "dword ptr [eax]"), ("fmul", "st(1)")])
        self.assertEqual(len(found), 1)

    def test_comparison_is_not_rule_a(self):
        found = swaps.survey([entry(
            ["fld dword ptr [a (DATA)]", "fcomp dword ptr [esp + 4]"],
            ["fld dword ptr [esp + 4]", "fcomp dword ptr [a (DATA)]"])], {})
        self.assertEqual(found[0]["swaps"][0]["kind"], "compare")
        self.assertEqual(found[0]["expected_gain"], 0)

    def test_identical_order_is_not_a_swap(self):
        found = swaps.survey([entry(
            ["fld dword ptr [a (DATA)]", "fmul dword ptr [b (DATA)]", "nop"],
            ["fld dword ptr [a (DATA)]", "fmul dword ptr [b (DATA)]", "int3"])], {})
        self.assertEqual(found, [])

    def test_non_commutative_order_is_not_a_swap(self):
        found = swaps.find_swaps(
            [("fld", "dword ptr [a (DATA)]"), ("fsub", "dword ptr [b (DATA)]")],
            [("fld", "dword ptr [b (DATA)]"), ("fsub", "dword ptr [a (DATA)]")])
        self.assertEqual(found, [])

    def test_literal_numbering_differs_per_side(self):
        found = swaps.survey([entry(
            ["fmul dword ptr [<OFFSET3>]", "fmul dword ptr [esp + 8]"],
            ["fmul dword ptr [esp + 8]", "fmul dword ptr [<OFFSET9>]"])], {})
        self.assertEqual(len(found), 1)

    def test_unannotated_global_is_matched_through_aligned_rows(self):
        # The original names the global only as <OFFSET13>; the aligned
        # compare rows show it is the recomp's s_timer.
        found = swaps.survey([entry(
            ["fld dword ptr [<OFFSET13>]", "fcomp dword ptr [<OFFSET2>]", "fnstsw ax",
             "fld dword ptr [<OFFSET13>]", "fadd dword ptr [esp + 4]"],
            ["fld dword ptr [s_timer (DATA)]", "fcomp dword ptr [<OFFSET1>]", "fnstsw ax",
             "fld dword ptr [esp + 4]", "fadd dword ptr [s_timer (DATA)]"])], {})
        self.assertEqual(len(found), 1)
        self.assertIn("s_timer (DATA)", found[0]["swaps"][0]["orig"])

    def test_loose_mode_masks_frame_and_registers(self):
        e = entry(["fld dword ptr [esp + 0x1c]", "fadd dword ptr [edi]"],
                  ["fld dword ptr [esi]", "fadd dword ptr [esp + 0x34]"])
        self.assertEqual(swaps.survey([e], {}), [])
        found = swaps.survey([e], {}, loose=True)
        self.assertEqual(len(found), 1)
        self.assertTrue(found[0]["swaps"][0]["loose"])

    def test_ranking_by_share_of_instructions(self):
        small = entry(["fld dword ptr [a (DATA)]", "fmul dword ptr [b (DATA)]"],
                      ["fld dword ptr [b (DATA)]", "fmul dword ptr [a (DATA)]"])
        large = dict(entry(["fld dword ptr [a (DATA)]", "fmul dword ptr [b (DATA)]"],
                           ["fld dword ptr [b (DATA)]", "fmul dword ptr [a (DATA)]"],
                           both_after=["nop"] * 20), address="0x402000")
        found = swaps.survey([large, small], {})
        self.assertEqual([f["address"] for f in found], ["0x00401000", "0x00402000"])


if __name__ == "__main__":
    unittest.main()
