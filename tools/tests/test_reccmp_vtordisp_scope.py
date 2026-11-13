# SPDX-License-Identifier: LGPL-3.0-or-later
"""Tests for reccmp_vtordisp_scope.py (finding 616)."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import reccmp_vtordisp_scope as rvs

try:
    from reccmp.analysis.vtordisp import find_displacements
except ImportError:  # Reccmp lives in the matching-build environment.
    find_displacements = None

ORIG, RECOMP = "orig", "recomp"


class OriginalOnlyTest(unittest.TestCase):
    def setUp(self):
        self.calls = []

        def create(db, img_id, binfile):
            self.calls.append((db, img_id, binfile))
            return "created"

        self.create = create

    def test_runs_for_the_original_image(self):
        wrapped = rvs.original_only(self.create, ORIG)
        self.assertEqual(wrapped("db", ORIG, "orig.exe"), "created")
        self.assertEqual(self.calls, [("db", ORIG, "orig.exe")])

    def test_skips_the_rebuilt_image(self):
        wrapped = rvs.original_only(self.create, ORIG)
        self.assertIsNone(wrapped("db", RECOMP, "LegoRR.exe"))
        self.assertEqual(self.calls, [])

    def test_installing_twice_wraps_once(self):
        once = rvs.original_only(self.create, ORIG)
        self.assertIs(rvs.original_only(once, ORIG), once)
        self.assertIs(once.__wrapped__, self.create)


@unittest.skipIf(find_displacements is None, "Reccmp is not installed")
class JumpTableFalsePositiveTest(unittest.TestCase):
    def test_adjacent_jump_table_entries_read_as_a_vtordisp(self):
        # Four jump-table entries of the rebuilt 0x00489dd0 (at 0x493860).
        table = bytes.fromhex("e42b4900e9254900fb2549000d264900")
        found = list(find_displacements(table, 0x493860))
        self.assertEqual([hit.addr for hit in found], [0x493861])
        self.assertEqual(found[0].size, 8)


if __name__ == "__main__":
    unittest.main()
