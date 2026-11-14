# SPDX-License-Identifier: LGPL-3.0-or-later
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import link_order
import opt_ref_link as opt

LIST = """\
lrr_link(unit   GAME-001 game          game/advisor.c)
lrr_link(legacy 004777a0 program       platform/winapi/entry.cpp winapi)
lrr_link(tail   -        game          game/game.cpp)
set(LRR_LINK_LIBRARIES kernel32.lib libc.lib)
"""

HEADER = """\
 LegoRR

 Start         Length     Name                   Class
 0001:00000000 {text:08x}H .text                   CODE

  Address         Publics by Value              Rva+Base   Lib:Object

"""

# The gate (/DEBUG, /OPT:NOREF) keeps every COMDAT.
GATE = HEADER.format(text=0x140) + """\
 0001:00000000       _advisor_init              00401000 f advisor.c.obj
 0001:00000040       _advisor_unused            00401040 f advisor.c.obj
 0001:00000060       _WinMain@16                00401060 f entry.cpp.obj
 0001:00000080       _port_helper               00401080 f game.cpp.obj
 0001:000000a0       __alloca_probe             004010a0   libc:chkstk.obj
 0001:000000a0       __chkstk                   004010a0   libc:chkstk.obj
 0001:000000c0       _unused_crt                004010c0 f libc:unused.obj
 0001:000000e0       _D3DRMVectorRandom@4       004010e0 f d3drm:d3drm.dll
 0001:000000f0       _Sleep@4                   004010f0 f kernel32:KERNEL32.dll
 0001:00000100       _c_dfDIMouse               00401100   dinput:dilib1.obj
 0001:00000130       _RtlUnwind@16              00401130 f kernel32:KERNEL32.dll
"""

# The relink (no /DEBUG, so /OPT:REF) drops what nothing references. The
# aliases at one address are listed in the other order.
REF = HEADER.format(text=0xb0) + """\
 0001:00000000       _advisor_init              00401000 f advisor.c.obj
 0001:00000020       _WinMain@16                00401020 f entry.cpp.obj
 0001:00000040       __chkstk                   00401040   libc:chkstk.obj
 0001:00000040       __alloca_probe             00401040   libc:chkstk.obj
 0001:00000060       _D3DRMVectorRandom@4       00401060 f d3drm:d3drm.dll
 0001:00000070       _c_dfDIMouse               00401070   dinput:dilib1.obj
 0001:000000a0       _RtlUnwind@16              004010a0 f kernel32:KERNEL32.dll
"""

ROWS = [
    {"address": "0x401000", "recomp": "0x401000", "matching": 1.0},
    {"address": "0x401100", "recomp": "0x401040", "matching": 1.0},   # removed, exact
    {"address": "0x401200", "recomp": "0x401060", "matching": 0.5},
    {"address": "0x48e250", "recomp": "0x4010a0", "matching": 1.0},   # __chkstk, kept
    {"address": "0x48f000", "recomp": "0x4010c0", "matching": 0.25},  # LIBC, removed
    {"address": "0x401300"},                                          # unmapped
]


class RelinkCommandTests(unittest.TestCase):
    LINK = ("/opt/msvc/link /nologo @CMakeFiles/program.dir/objects1.rsp  /out:LegoRR.exe "
            "/implib:LegoRR.lib /pdb:/b/LegoRR.pdb /version:0.0 /INCREMENTAL:YES "
            "/MAP:/b/LegoRR.map /ENTRY:WinMainCRTStartup kernel32.lib libc.lib /DEBUG "
            "/INCREMENTAL:NO\n")

    def test_drops_debug_and_redirects_outputs(self):
        cmd = opt.relink_command(self.LINK, Path("/s"))
        self.assertNotIn("/DEBUG", cmd)
        self.assertFalse(any(t.lower().startswith("/pdb:") for t in cmd))
        self.assertIn("/out:/s/LegoRR.exe", cmd)
        self.assertIn("/implib:/s/LegoRR.lib", cmd)
        self.assertIn("/MAP:/s/LegoRR.map", cmd)
        self.assertEqual(cmd[:3], ["/opt/msvc/link", "/nologo",
                                   "@CMakeFiles/program.dir/objects1.rsp"])
        self.assertIn("/INCREMENTAL:YES", cmd)
        self.assertEqual(cmd[-1], "/INCREMENTAL:NO")

    def test_keep_debug_and_flags(self):
        cmd = opt.relink_command(self.LINK, Path("/s"), keep_debug=True, flags=("/OPT:REF",))
        self.assertIn("/DEBUG", cmd)
        self.assertIn("/pdb:/s/LegoRR.pdb", cmd)
        self.assertEqual(cmd[-1], "/OPT:REF")

    def test_needs_one_link_command(self):
        with self.assertRaises(ValueError):
            opt.relink_command("echo nothing\n", Path("/s"))

    def test_noref_keeps_debug_and_replaces_opt(self):
        link = self.LINK.replace("/DEBUG", "/OPT:REF /OPT:NOICF /DEBUG")
        cmd = opt.relink_command(link, Path("/s"), noref=True)
        self.assertIn("/DEBUG", cmd)
        self.assertIn("/pdb:/s/LegoRR.pdb", cmd)
        self.assertNotIn("/OPT:REF", cmd)
        self.assertNotIn("/OPT:NOICF", cmd)
        self.assertEqual(cmd[-1], "/OPT:NOREF")


class SwitchedGateTests(unittest.TestCase):
    """After the gate links with /OPT:REF (finding 642)."""

    def test_unmapped_annotations(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "src").mkdir()
            (root / "src/a.c").write_text(
                "// FUNCTION: LEGORR 0x00401000\nint f(void) { return 0; }\n"
                "// FUNCTION: LEGORR 0x00401100\nint g(void) { return 0; }\n"
                "// LIBRARY: LEGORR 0x00490000 SYMBOL\n// __static\n", encoding="utf-8")
            report = root / "report"
            report.mkdir()
            (report / "entities.json").write_text(json.dumps([
                {"orig": 0x401000, "recomp": 0x401000, "type": 1},   # mapped
                {"orig": 0x401100, "recomp": None, "type": 1},       # removed
                {"orig": 0x490000, "recomp": None, "type": 1},       # LIBC static
                {"orig": 0x499660, "recomp": None, "type": 1},       # not annotated
                {"orig": 0x401100, "recomp": None, "type": 2},       # not a function
            ]), encoding="utf-8")
            self.assertEqual(opt.unmapped_annotations(report, root), [0x401100])
            self.assertEqual(opt.unmapped_annotations(root / "missing", root), [])

    def test_translate_rows(self):
        entries = link_order.parse(LIST)
        rows = [{"address": "0x401000", "recomp": "0x401000"},   # advisor_init
                {"address": "0x401200", "recomp": "0x401020"},   # WinMain in REF
                {"address": "0x401300"}]
        moved = opt.translate_rows(rows, REF, GATE, entries)
        self.assertEqual([r["recomp"] for r in moved], ["0x401000", "0x401060"])
        self.assertIsNone(opt.translate_rows(None, REF, GATE, entries))

    def test_gate_link_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            build = Path(tmp)
            self.assertFalse(opt.gate_removes_unreferenced(build))
            link = build / "CMakeFiles/program.dir/link.txt"
            link.parent.mkdir(parents=True)
            link.write_text(RelinkCommandTests.LINK, encoding="utf-8")
            self.assertFalse(opt.gate_removes_unreferenced(build))
            link.write_text(RelinkCommandTests.LINK.replace("/DEBUG", "/OPT:REF /DEBUG"),
                            encoding="utf-8")
            self.assertTrue(opt.gate_removes_unreferenced(build))


class RemovedCodeTests(unittest.TestCase):
    def setUp(self):
        self.entries = link_order.parse(LIST)

    def test_categories_and_annotated(self):
        result = opt.removed_code(GATE, REF, self.entries, ROWS, ROWS[:3])
        # advisor_unused, port_helper, unused_crt and the Sleep thunk.
        self.assertEqual(result["gate_text_symbols"], 10)
        self.assertEqual(result["removed_functions"], 4)
        self.assertEqual(result["removed_bytes"], 0x20 + 0x20 + 0x20 + 0x10)
        cats = result["by_category"]
        self.assertEqual(cats["original_annotated"],
                         {"functions": 3, "bytes": 0x80, "removed_functions": 1, "removed_bytes": 0x20})
        self.assertEqual(cats["port_tail"]["removed_bytes"], 0x20)
        self.assertEqual(result["annotated_by_kind"],
                         {"legacy": {"functions": 1, "bytes": 0x20, "removed_functions": 0,
                                     "removed_bytes": 0},
                          "unit": {"functions": 2, "bytes": 0x60, "removed_functions": 1,
                                   "removed_bytes": 0x20}})
        self.assertEqual(cats["libc"], {"functions": 2, "bytes": 0x40, "removed_functions": 1,
                                        "removed_bytes": 0x20})
        self.assertEqual(cats["import_thunks"]["removed_functions"], 1)
        self.assertEqual(cats["other"]["removed_functions"], 0)
        self.assertEqual(result["annotated"]["full"],
                         {"mapped": 5, "exact": 3, "removed": 2, "removed_exact": 1})
        self.assertEqual(result["annotated"]["game_only"],
                         {"mapped": 3, "exact": 2, "removed": 1, "removed_exact": 1})

    def test_alias_is_not_removed(self):
        result = opt.removed_code(GATE, REF, self.entries, ROWS)
        self.assertNotIn("game_only", result["annotated"])
        # __chkstk is kept although the relink lists its alias first.
        self.assertEqual(result["by_category"]["libc"]["removed_functions"], 1)
        self.assertEqual(result["annotated"]["full"]["removed"], 2)

    def test_without_report(self):
        result = opt.removed_code(GATE, GATE, self.entries, None)
        self.assertEqual(result["removed_functions"], 0)
        self.assertNotIn("annotated", result)
        self.assertNotIn("original_annotated", result["by_category"])

    def test_thunk_order(self):
        self.assertEqual(opt.thunk_order(REF), ["d3drm.dll:_D3DRMVectorRandom@4",
                                                "dinput-data:_c_dfDIMouse",
                                                "kernel32.dll:_RtlUnwind@16"])

    def test_summary(self):
        text = opt.format_summary(opt.removed_code(GATE, REF, self.entries, ROWS))
        self.assertIn("/OPT:REF removes 4 of 10 .text symbols", text)
        self.assertIn("annotated (full): 2 of 5 removed, 1 of 3 exact", text)


class CallerTests(unittest.TestCase):
    # Original 0x500 calls a, F, b; its rebuild calls a', P, b'.
    TO_REBUILT = {0x100: 0x1100, 0x200: 0x1200, 0x300: 0x1300}

    def test_substitute_at_the_call_site(self):
        subs = opt.call_site_substitutes([0x100, 0x400, 0x300], [0x1100, 0x9000, 0x1300],
                                         self.TO_REBUILT, 0x400)
        self.assertEqual(subs, [0x9000])

    def test_no_call_and_uneven_runs(self):
        self.assertEqual(opt.call_site_substitutes([0x100, 0x400, 0x300], [0x1100, 0x1300],
                                                   self.TO_REBUILT, 0x400), [None])
        self.assertEqual(opt.call_site_substitutes([0x100, 0x400, 0x300],
                                                   [0x1100, 0x9000, 0x9100, 0x1300],
                                                   self.TO_REBUILT, 0x400),
                         [[0x9000, 0x9100]])

    def test_agreeing_call_is_not_a_substitute(self):
        # The rebuild already calls the function (it is not removed then).
        self.assertEqual(opt.call_site_substitutes([0x100], [0x1100], self.TO_REBUILT, 0x100), [])

    def test_dead_roots(self):
        # 1 -> 2 -> 3, 4 -> 3; 5 alone; 6 <-> 7 (a removed cycle).
        callers = {2: {1}, 3: {2, 4}, 7: {6}, 6: {7}}
        roots = opt.dead_roots({1, 2, 3, 4, 5, 6, 7}, callers)
        self.assertEqual(roots[3], {1, 4})
        self.assertEqual(roots[2], {1})
        self.assertEqual(roots[5], {5})
        self.assertTrue(roots[6] and roots[7])

    def test_live_caller_is_not_a_root(self):
        # 9 is live (not removed): 3's root is 3 itself.
        self.assertEqual(opt.dead_roots({3}, {3: {9}})[3], {3})

    def test_root_causes(self):
        # Roots 1 (live caller 8), 4 (unmapped caller 9) and 5 (no caller).
        callers = {1: {8}, 2: {1}, 3: {2, 4}, 4: {9}}
        removed, mapped = {1, 2, 3, 4, 5, 6}, {1, 2, 3, 4, 5, 6, 8}
        roots = opt.dead_roots(removed, callers)
        self.assertEqual(opt.root_causes(roots[2], callers, removed, mapped), ["live_caller"])
        self.assertEqual(opt.root_causes(roots[3], callers, removed, mapped),
                         ["live_caller", "unmapped_caller"])
        self.assertEqual(opt.root_causes(roots[5], callers, removed, mapped), ["unreferenced"])
        # 6 is referred to only from a data table.
        self.assertEqual(opt.root_causes(roots[6], callers, removed, mapped, {6}),
                         ["data_reference"])
        self.assertEqual(opt.primary_cause(["live_caller", "unmapped_caller"]), "live_caller")
        self.assertEqual(opt.primary_cause([]), "unreferenced")

    def test_pointer_references(self):
        base = 0x400000
        # push 0x401010 at an odd offset, and a table entry.
        image = bytes(0x1000) + b"\x90\x68\x10\x10\x40\x00\x90\x90" + bytes(8) \
            + (0x401010).to_bytes(4, "little") + (0x401020).to_bytes(4, "little")
        refs = opt.pointer_references(image, base, {0x401010, 0x401030}, 0x401000,
                                      base + len(image))
        self.assertEqual(refs, {0x401010: [0x401002, 0x401010]})
        # A range that excludes the table.
        self.assertEqual(opt.pointer_references(image, base, {0x401010}, 0x401000, 0x401008),
                         {0x401010: [0x401002]})

    def test_summary_by_unit_and_cause(self):
        report = {"a.c.obj": {"functions": [
            {"address": "0x400", "exact": True, "size": 16, "causes": ["live_caller"]},
            {"address": "0x410", "exact": False, "size": 32, "causes": ["unreferenced"]}]},
            "libc:x.obj": {"functions": [
                {"address": "0x900", "exact": True, "size": 8,
                 "causes": ["unmapped_caller", "unreferenced"]},
                {"address": "0x910", "exact": True, "size": 4,
                 "causes": ["folded", "live_caller"]}]}}
        summary = opt.summarize(report, {0x400: "GAME-001 A.c", 0x410: "GAME-001 A.c"})
        unit = summary["by_unit"]["GAME-001 A.c"]
        self.assertEqual((unit["removed"], unit["exact"], unit["bytes"]), (2, 1, 48))
        self.assertEqual((unit["live_caller"], unit["live_caller_exact"]), (1, 1))
        self.assertEqual((unit["unreferenced"], unit["unreferenced_exact"]), (1, 0))
        self.assertEqual(summary["by_unit"]["unassigned"]["unmapped_caller"], 1)
        self.assertEqual(summary["by_unit"]["unassigned"]["folded"], 1)
        self.assertEqual(summary["total"]["removed"], 4)
        self.assertEqual(list(summary["by_unit"]), ["GAME-001 A.c", "unassigned"])
        text = opt.format_summary_table(summary)
        self.assertIn("| GAME-001 A.c | 2 | 1 | 48 | 0 | 1 (1) | 0 | 0 | 1 (0) |", text)
        self.assertIn("| **total** | 4 | 3 | 60 |", text)

    def test_load_units(self):
        import tempfile
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "u.csv"
            path.write_text("address,function_name,unit_order,unit_id,unit_name,current_source\n"
                            "00401000,F,0,GAME-001,Advisor.c,src/game/advisor.c\n"
                            "0046eb80,G,51,GAME-050,,src/game/weapons.c\n"
                            "00402000,H,1,GAME-002,,\n", encoding="utf-8")
            self.assertEqual(opt.load_units(path), {0x401000: "GAME-001 Advisor.c",
                                                    0x46eb80: "GAME-050 [weapons.c]",
                                                    0x402000: "GAME-002"})
            self.assertEqual(opt.load_units(Path(d) / "missing.csv"), {})
            path.with_name("translation-unit-ranges.csv").write_text(
                "unit_order,unit_id,unit_name,text_start,text_end\n"
                "51,GAME-050,,0046eb80,004721e0\n52,G98-001,Movie.cpp,004721e0,00472710\n",
                encoding="utf-8")
            units = opt.load_units(path)
            self.assertEqual(units.get(0x46f610), "GAME-050 [weapons.c]")
            self.assertEqual(units.get(0x472300), "G98-001 Movie.cpp")
            self.assertEqual(units.get(0x472710, "unassigned"), "unassigned")
            self.assertEqual(units.get(0x401000), "GAME-001 Advisor.c")

    def test_format(self):
        report = {"x.c.obj": {"removed": 1, "exact_removed": 1, "functions": [
            {"address": "0x400", "name": "f", "exact": True, "roots": ["y.c.obj"],
             "callers": [{"address": "0x500", "name": "g", "state": "live",
                          "rebuilt_calls": ["port_f (z.cpp.obj)", None]}]}]}}
        text = opt.format_callers(report)
        self.assertIn("x.c.obj: 1 removed, 1 exact", text)
        self.assertIn("0x400 E f  [roots: y.c.obj]", text)
        self.assertIn("<- 0x500 g (live) calls instead: port_f (z.cpp.obj); no call", text)
        report["x.c.obj"]["functions"][0]["callers"] = [
            {"address": "0x600", "name": "h", "state": "removed", "kind": "pointer"}]
        self.assertIn("<& 0x600 h (removed)", opt.format_callers(report))


if __name__ == "__main__":
    unittest.main()
