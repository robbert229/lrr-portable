# SPDX-License-Identifier: LGPL-3.0-or-later
"""Tests for tools/link_order.py and the committed link-order list (ADR 0006).

    python3 -m unittest discover -s tools/tests -p 'test_link_order.py'
"""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import link_order as lo  # noqa: E402

LIST = """# comment
lrr_link(legacy 00401800 game          game/old.cpp)
lrr_link(unit   GAME-B   game          game/beta.c)  # trailing comment
lrr_link(legacy 00403800 program       platform/winapi/entry.cpp winapi)
lrr_link(tail   -        program       platform/native/entry.cpp native)
lrr_link(tail   -        program       main.cpp)
set(LRR_LINK_LIBRARIES libc.lib oldnames.lib
  d3drm.lib kernel32.lib)
"""
STARTS = {"GAME-A": 0x401000, "GAME-B": 0x402000, "G98-C": 0x403000, "G98-D": 0x405000}


class ParseTest(unittest.TestCase):
    def test_fields_platforms_and_libraries(self):
        entries = lo.parse(LIST)
        self.assertEqual([e.kind for e in entries], ["legacy", "unit", "legacy", "tail", "tail"])
        self.assertEqual(entries[1].key, "GAME-B")
        self.assertEqual(entries[2].platform, "winapi")
        self.assertEqual(lo.sources(entries), ["src/game/old.cpp", "src/game/beta.c",
                                               "src/platform/winapi/entry.cpp", "src/main.cpp"])
        self.assertEqual(len(lo.sources(entries, platform=None)), 5)
        self.assertEqual(lo.libraries(LIST), ["libc.lib", "oldnames.lib", "d3drm.lib", "kernel32.lib"])

    def test_malformed_entries_are_errors(self):
        for bad in ("lrr_link(unit GAME-B game)\n", "lrr_link(odd - game game/a.c)\n",
                    "lrr_link(tail - nowhere a.c)\n", "lrr_link(tail - game a.c dos)\n",
                    "lrr_link(tail - game\n  a.c)\n"):
            with self.assertRaises(ValueError, msg=bad):
                lo.parse(bad)

    def test_insert_keeps_address_order_before_the_tail(self):
        text = lo.insert_unit(LIST, "G98-C", "gods98", "gods98/gamma.c", STARTS)
        self.assertIn("beta.c)  # trailing comment\n"
                      "lrr_link(unit   G98-C    gods98        gods98/gamma.c)\n"
                      "lrr_link(legacy 00403800", text)
        late = lo.insert_unit(LIST, "G98-D", "gods98", "gods98/delta.c", STARTS)
        self.assertLess(late.index("delta.c"), late.index("lrr_link(tail"))
        first = lo.insert_unit(LIST, "GAME-A", "game", "game/alpha.c", STARTS)
        self.assertLess(first.index("alpha.c"), first.index("old.cpp"))
        self.assertEqual(lo.insert_unit(first, "GAME-A", "game", "game/alpha.c", STARTS), first)


class CommittedListTest(unittest.TestCase):
    """The committed list parses, names existing files once, and is ordered."""

    def setUp(self):
        self.entries = lo.load()

    def test_files_exist_once(self):
        paths = [e.path for e in self.entries]
        self.assertEqual(len(paths), len(set(paths)))
        for path in paths:
            self.assertTrue((lo.ROOT / path).is_file(), path)

    def test_units_and_legacy_files_ascend_before_the_tail(self):
        starts = lo.unit_starts()
        kinds = [e.kind for e in self.entries]
        first_tail = kinds.index("tail")
        last_tail = len(kinds) - 1 - kinds[::-1].index("tail")
        self.assertNotIn("tail", kinds[:first_tail])
        self.assertEqual(set(kinds[first_tail:last_tail + 1]), {"tail"})
        for part in (self.entries[:first_tail], self.entries[last_tail + 1:]):
            addresses = [lo.entry_address(e, starts) for e in part]
            self.assertNotIn(None, addresses)
            self.assertEqual(addresses, sorted(addresses))
        units = [e.key for e in self.entries if e.kind == "unit"]
        self.assertEqual(units, sorted(units, key=starts.__getitem__))

    def test_matching_libraries_start_with_libc(self):
        libs = lo.libraries((lo.ROOT / lo.LIST).read_text(encoding="utf-8"))
        self.assertEqual(libs[:2], ["libc.lib", "oldnames.lib"])


if __name__ == "__main__":
    unittest.main()
