# SPDX-License-Identifier: LGPL-3.0-or-later
"""Tests for tools/link_order.py and the committed link-order list (ADR 0006).

    python3 -m unittest discover -s tools/tests -p 'test_link_order.py'
"""

from __future__ import annotations

import sys
import tempfile
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

    def test_portable_entries_leave_only_the_matching_build(self):
        # src/crt/'s stand-ins (#447): built natively and by the MinGW
        # reference build, never by the MSVC 5.0 matching build.
        entries = lo.parse("lrr_link(legacy 00493ef0 gods98        crt/crt.cpp portable)\n")
        self.assertEqual(entries[0].platform, "portable")
        self.assertEqual(lo.sources(entries), [])
        self.assertEqual(lo.sources(entries, platform="native"), ["src/crt/crt.cpp"])
        self.assertEqual(lo.sources(entries, platform=None), ["src/crt/crt.cpp"])

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

    def test_matching_build_links_nothing_from_src_crt(self):
        # Issue #447: the matching build takes the C runtime from LIBC.LIB.
        crt = [p for p in lo.sources(self.entries) if p.startswith("src/crt/")]
        self.assertEqual(crt, [])

    def test_matching_libraries_put_libc_after_kernel32(self):
        # Finding 574: kernel32's descriptor comes first, and LIBC follows it
        # so that RtlUnwind's thunk is pulled after the DirectX thunks.
        libs = lo.libraries((lo.ROOT / lo.LIST).read_text(encoding="utf-8"))
        self.assertEqual(libs[:3], ["kernel32.lib", "libc.lib", "oldnames.lib"])


class CheckTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        for d in ("src/game", "src/gods98", "docs/coverage"):
            (self.root / d).mkdir(parents=True)
        (self.root / "docs/coverage/translation-units.spec.csv").write_text(
            "start,unit_id,kind\n00401000,GAME-001,game\n00402000,G98-001,gods98\n00403000,CRT-001,crt\n")
        (self.root / lo.RANGES).write_text(
            "unit_id,text_start,text_end\nGAME-001,00401000,00402000\nG98-001,00402000,00403000\nCRT-001,00403000,00404000\n")
        (self.root / "src/game/a.c").write_text("// FUNCTION: LEGORR 0x00401010\n")
        (self.root / "src/gods98/b.cpp").write_text("// FUNCTION: LEGORR 0x00402010\n")
        (self.root / "src/game/tail.cpp").write_text("")
        self.lines = ["lrr_link(unit GAME-001 game game/a.c)",
                      "lrr_link(legacy 00402010 gods98 gods98/b.cpp)",
                      "lrr_link(tail - game game/tail.cpp)"]

    def errors(self, lines=None):
        (self.root / lo.LIST).write_text("\n".join(lines or self.lines) + "\n")
        return "\n".join(lo.check(self.root))

    def test_valid_and_relocated_unit(self):
        self.assertEqual(self.errors(), "")
        self.assertEqual(self.errors([self.lines[0], "lrr_link(unit G98-001 gods98 gods98/b.cpp)", self.lines[2]]), "")

    def test_duplicate_and_missing_sources(self):
        self.assertIn("listed twice", self.errors(self.lines + [self.lines[0]]))
        self.assertIn("missing", self.errors(self.lines[:2]))

    def test_invalid_duplicate_and_wrong_unit_marker(self):
        self.assertIn("not a game, gods98 or late unit", self.errors([self.lines[0].replace("GAME-001", "BAD"), *self.lines[1:]]))
        self.assertIn("unit GAME-001 is listed twice", self.errors([self.lines[0], self.lines[0].replace("game/a.c", "gods98/b.cpp"), self.lines[2]]))
        (self.root / "src/game/a.c").write_text("// FUNCTION: LEGORR 0x00402010\n")
        self.assertIn("outside GAME-001 range", self.errors())

    def test_late_unit_sits_in_the_crt_range(self):
        # Finding 610: a late unit is a unit entry after the port tail.
        spec = self.root / "docs/coverage/translation-units.spec.csv"
        spec.write_text(spec.read_text() + "00404000,LATE-001,late\n")
        ranges = self.root / lo.RANGES
        ranges.write_text(ranges.read_text() + "LATE-001,00404000,00405000\n")
        (self.root / "src/gods98/late.c").write_text("// FUNCTION: LEGORR 0x00404010\n")
        late = "lrr_link(unit LATE-001 gods98 gods98/late.c)"
        self.assertEqual(self.errors(self.lines + [late]), "")
        self.assertIn("wrong segment", self.errors([self.lines[0], self.lines[1], late, self.lines[2]]))

    def test_stale_median_order_and_segments(self):
        self.assertIn("set it to 00402010", self.errors([self.lines[0], self.lines[1].replace("00402010", "00402011"), self.lines[2]]))
        self.assertIn("wrong segment", self.errors([self.lines[1], self.lines[0], self.lines[2]]))
        self.assertIn("wrong segment", self.errors([self.lines[0], self.lines[2], self.lines[1]]))
        (self.root / "src/game/c.cpp").write_text("// FUNCTION: LEGORR 0x00401020\n")
        lines = ["lrr_link(legacy 00401020 game game/c.cpp)", self.lines[0], *self.lines[1:]]
        self.assertIn("out of order", self.errors(lines))

    def test_map_order_and_skipped_objects(self):
        entries = lo.parse("\n".join(self.lines))
        header = " 0001:00000000 00000100H .text CODE\n"
        def symbol(offset, obj):
            return f" 0001:{offset:08x} _name 00401000 f {obj}\n"
        a = symbol(0, "a.c.obj")
        b = symbol(16, "b.cpp.obj")
        lib = symbol(32, "libc:mem.obj")
        self.assertEqual(lo.check_map(entries, header + a + b + lib), [])
        self.assertEqual(lo.check_map(entries, header + b + lib), [])
        self.assertIn("out-of-order", " ".join(lo.check_map(entries, header + symbol(0, "b.cpp.obj") + symbol(16, "a.c.obj"))))
        self.assertIn("after a library", " ".join(lo.check_map(entries, header + symbol(0, "libc:mem.obj") + symbol(16, "a.c.obj"))))
        self.assertIn("no .text", " ".join(lo.check_map(entries, "")))


if __name__ == "__main__":
    unittest.main()
