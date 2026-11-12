# SPDX-License-Identifier: LGPL-3.0-or-later
import sys
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

    def test_format(self):
        report = {"x.c.obj": {"removed": 1, "exact_removed": 1, "functions": [
            {"address": "0x400", "name": "f", "exact": True, "roots": ["y.c.obj"],
             "callers": [{"address": "0x500", "name": "g", "state": "live",
                          "rebuilt_calls": ["port_f (z.cpp.obj)", None]}]}]}}
        text = opt.format_callers(report)
        self.assertIn("x.c.obj: 1 removed, 1 exact", text)
        self.assertIn("0x400 E f  [roots: y.c.obj]", text)
        self.assertIn("<- 0x500 g (live) calls instead: port_f (z.cpp.obj); no call", text)


if __name__ == "__main__":
    unittest.main()
