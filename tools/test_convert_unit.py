# SPDX-License-Identifier: LGPL-3.0-or-later
"""Tests for conversion_footprint.py and convert_unit.py (issue #405).

    python3 -m unittest discover -s tools -p 'test_convert_unit.py'
    make tools-test

The footprint tests run on a small synthetic repository. The validation tests
compare the scaffold with the hand conversions committed in this checkout
(Priorities.c, Config.c, utils.c and maths.c) and need ``reference/LegoRR.exe``.
"""

from __future__ import annotations

import contextlib
import io
import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import conversion_footprint as cf  # noqa: E402
import convert_unit as cu  # noqa: E402

REPO = cf.ROOT
HAVE_EXE = (REPO / "reference/LegoRR.exe").exists()

RANGES_HEADER = ("unit_order,unit_id,unit_name,unit_kind,name_basis,text_start,text_end,functions,"
                 "anchored_functions,boundary_confidence,boundary_evidence,boundary_slack,rdata_range,"
                 "data_range,string_range,bss_range,note\n")
FUNCS_HEADER = "address,function_name,unit_order,unit_id,unit_name,unit_kind,confidence,evidence,current_source\n"


def write(root: Path, rel: str, text: str) -> None:
    path = root / rel
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def make_repo(root: Path) -> None:
    """Units A (leaf, game), B (game, calls A and C), C (gods98, converted), L (library)."""
    write(root, cf.UNIT_RANGES, RANGES_HEADER + "\n".join([
        "0,GAME-A,Alpha.c,game,none,00401000,00402000,2,2,high,x,0,,,,,note a",
        "1,GAME-B,,game,none,00402000,00403000,1,1,high,x,0,,,,,note b",
        "2,G98-C,,gods98,none,00403000,00404000,1,1,high,x,0,,,,,note c",
        "3,CRT-1,,crt,none,00404000,00405000,1,1,high,x,0,,,,,note crt",
    ]) + "\n")
    write(root, cf.UNIT_FUNCTIONS, FUNCS_HEADER + "\n".join([
        "00401000,AlphaOne,0,GAME-A,Alpha.c,game,high,x,src/game/old.cpp",
        "00401100,AlphaTwo,0,GAME-A,Alpha.c,game,high,x,src/game/old.cpp",
        "00402000,BetaMain,1,GAME-B,,game,high,x,src/game/beta.cpp",
        "00403000,GammaFn,2,G98-C,,gods98,high,x,src/gods98/gamma.c",
        "00404000,libcall,3,CRT-1,,crt,high,x,",
    ]) + "\n")
    write(root, cf.EDGES, "caller,callee\n" + "\n".join([
        "0x00401000,0x00403000",     # A -> C (converted)
        "0x00401000,0x00404000",     # A -> CRT
        "0x00401100,0x00402000",     # A -> B (needs C linkage)
        "0x00402000,0x00401000",     # B -> A (caller)
        "0x00401100,0x00401000",     # A -> A (internal)
    ]) + "\n")
    write(root, "src/game/old.cpp",
          "// FUNCTION: LEGORR 0x00401000\nint alpha_one(int x)\n{\n    return x;\n}\n\n"
          "// FUNCTION: LEGORR 0x00401100\n__declspec(naked) void alpha_two(void)\n{\n}\n")
    write(root, "src/game/beta.cpp",
          "// FUNCTION: LEGORR 0x00402000\nvoid beta_main(void)\n{\n    alpha_one(1);\n}\n")
    write(root, "src/gods98/gamma.c", "// FUNCTION: LEGORR 0x00403000\nvoid gamma_fn(void)\n{\n}\n")
    write(root, "src/game/alpha.hpp",
          "#pragma once\nint alpha_one(int x);\nvoid alpha_two(void);\n"
          "inline int helper() { return alpha_one(2); }\n")
    write(root, "src/game/beta.hpp", "#pragma once\nvoid beta_main(void);\n")
    write(root, "src/gods98/gamma.h", '#ifdef __cplusplus\nextern "C" {\n#endif\nvoid gamma_fn(void);\n'
          '#ifdef __cplusplus\n}\n#endif\n')
    write(root, "src/game/test_alpha.cpp", "int main() { return alpha_one(1); }\n")
    write(root, "src/link_order.cmake", "")


class PortOnlyChecklistTest(unittest.TestCase):
    def test_conversion_checklist_marks_adapters_and_forwarders(self):
        self.assertTrue(any("adapter and forwarder" in item and "// PORT-ONLY: <reason>" in item
                            for item in cu.CHECKLIST))


    def test_conversion_checklist_requires_scoring_only_phrase(self):
        self.assertTrue(any("`Deviation (scoring only)`" in item for item in cu.CHECKLIST))


class HeaderScanTest(unittest.TestCase):
    def test_linkage_and_bodies(self):
        scan = cf.HeaderScan(
            'void plain(int);\n'
            'extern "C" void one(void);\n'
            '#ifdef __cplusplus\nextern "C" {\n#endif\n'
            'void blocked(void);\n'
            '#ifdef __cplusplus\n}\n#endif\n'
            'inline int body(void) { return called(); }\n'
            'struct S { void member(void); };\n'
            '/* commented(void); */\n'
            'extern int shared_value;\nextern "C" float table[4];\n')
        fn = {k: [c for _, c in v] for k, v in scan.functions.items()}
        self.assertEqual(fn["plain"], [False])
        self.assertEqual(fn["one"], [True])
        self.assertEqual(fn["blocked"], [True])
        self.assertNotIn("called", fn)          # call inside a body
        self.assertNotIn("member", fn)          # member of an aggregate
        self.assertNotIn("commented", fn)
        self.assertEqual(scan.variables["shared_value"], [(scan.variables["shared_value"][0][0], False)])
        self.assertTrue(scan.variables["table"][0][1])


class FootprintTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        make_repo(self.root)
        self.fp = cf.compute_footprint("GAME-A", self.root, with_data=False)

    def tearDown(self):
        self.tmp.cleanup()

    def test_functions_in_address_order_with_existing_names(self):
        self.assertEqual([f.address for f in self.fp.functions], [0x401000, 0x401100])
        # `__declspec(naked)` in front of the definition must not become its name.
        self.assertEqual([f.name for f in self.fp.functions], ["alpha_one", "alpha_two"])
        self.assertEqual([f.size for f in self.fp.functions], [0x100, 0xf00])
        self.assertEqual(self.fp.current_sources, ["src/game/old.cpp"])
        self.assertEqual(self.fp.functions[0].current_line, 1)      # the annotation's line

    def test_new_files_named_by_the_unit(self):
        self.assertEqual(self.fp.new_files, ["src/game/alpha.c", "src/game/alpha.h"])
        unnamed = cf.compute_footprint("GAME-B", self.root, with_data=False)
        self.assertEqual(unnamed.new_files, ["src/game/game_b.c", "src/game/game_b.h"])
        renamed = cf.compute_footprint("GAME-B", self.root, stem="beta", with_data=False)
        self.assertEqual(renamed.new_files[0], "src/game/beta.c")

    def test_callee_classification(self):
        by = {c.name: c for c in self.fp.callees}
        self.assertEqual(set(by), {"gamma_fn", "libcall", "beta_main"})
        self.assertEqual(by["gamma_fn"].status, "converted")
        self.assertFalse(by["gamma_fn"].needs_extern_c or by["gamma_fn"].needs_boundary_decl)
        self.assertEqual(by["libcall"].status, "library")
        beta = by["beta_main"]
        self.assertEqual(beta.status, "needs-c-linkage")
        self.assertTrue(beta.needs_extern_c and beta.needs_boundary_decl)
        self.assertEqual(beta.edit_headers, ["src/game/beta.hpp"])
        self.assertEqual(beta.new_headers, ["src/game/beta.h"])       # no boundary header yet
        self.assertEqual(beta.called_by, ["alpha_two"])

    def test_boundary_header_that_exists_is_an_edit(self):
        write(self.root, "src/game/beta.h", "void beta_main(void);\n")
        fp = cf.compute_footprint("GAME-A", self.root, with_data=False)
        beta = next(c for c in fp.callees if c.name == "beta_main")
        self.assertEqual(beta.status, "c-declared")
        self.assertFalse(beta.needs_boundary_decl)

    def test_callers_and_internal_calls(self):
        self.assertEqual([c.name for c in self.fp.callers], ["beta_main"])
        self.assertEqual(self.fp.callers[0].calls, ["alpha_one"])
        self.assertEqual(self.fp.caller_sources, ["src/game/beta.cpp"])
        # The A -> A edge is internal and is neither a caller nor a callee.
        self.assertNotIn("alpha_one", [c.name for c in self.fp.callees])

    def test_header_set(self):
        self.assertEqual(self.fp.own_declaration_headers, ["src/game/alpha.hpp"])
        self.assertEqual(self.fp.headers, ["src/game/alpha.hpp", "src/game/beta.hpp"])
        self.assertEqual(self.fp.test_files, ["src/game/test_alpha.cpp"])
        self.assertIn("src/game/test_alpha.cpp", self.fp.sources)
        self.assertNotIn("src/game/alpha.h", self.fp.headers)          # new file, not an edit
        self.assertEqual(self.fp.shared_files, list(cf.SHARED_FILES))

    def test_leaf(self):
        # Only the CRT and a converted unit: A calls B, which is not converted.
        self.assertFalse(self.fp.leaf)
        leaf = cf.compute_footprint("G98-C", self.root, with_data=False)
        self.assertTrue(leaf.leaf)

    def test_overlap_for_planning(self):
        a = self.fp
        b = cf.compute_footprint("GAME-B", self.root, with_data=False)
        # A calls B and B calls A: each edits the other's header.
        self.assertEqual(cf.header_overlap(a, b), frozenset({"src/game/alpha.hpp", "src/game/beta.hpp"}))
        c = cf.compute_footprint("G98-C", self.root, with_data=False)
        self.assertEqual(cf.header_overlap(a, c), frozenset())

    def test_json_round_trip(self):
        doc = json.loads(self.fp.to_json())
        self.assertEqual(doc["unit_id"], "GAME-A")
        self.assertEqual(doc["functions"][0]["address"], "0x00401000")
        self.assertEqual(doc["headers"], ["src/game/alpha.hpp", "src/game/beta.hpp"])
        self.assertIn("Call edges are the direct calls", doc["limitations"][0])
        self.assertIn("function pointers", doc["limitations"][0])
        self.assertFalse(doc["leaf"])

    def test_text_report_mentions_the_call_graph_limit(self):
        text = cf.render_text(self.fp)
        self.assertIn("Calls through function pointers and callbacks are missing", text)

    def test_unit_lookup(self):
        units = cf.load_units(self.root)
        self.assertEqual(cf.find_unit(units, "alpha.c").unit_id, "GAME-A")
        self.assertEqual(cf.find_unit(units, "Alpha").unit_id, "GAME-A")
        with self.assertRaises(SystemExit):
            cf.find_unit(units, "nothing")


class LayoutTest(unittest.TestCase):
    def refs(self, *specs):
        out = {}
        for addr, width, kw in specs:
            out[addr] = cu.Ref(addr, width, **kw)
        return out

    def test_scalars_arrays_and_padding(self):
        refs = self.refs((0x1000, 4, {}), (0x1004, 4, {"scale": 4}), (0x1020, 4, {"is_float": True}))
        fields = cu.layout(refs, 0x1024)
        self.assertEqual([(f.start, f.ctype, f.count) for f in fields],
                         [(0x1000, "unsigned int", 1), (0x1004, "unsigned int", 7), (0x1020, "float", 1)])

    def test_struct_stride_cluster(self):
        refs = self.refs((0x2000, 4, {"scale": 8}), (0x2004, 4, {"scale": 8}))
        fields = cu.layout(refs, 0x2000 + 8 * 10 + 4)
        self.assertEqual(fields[0].ctype, "struct")
        self.assertEqual((fields[0].count, fields[0].stride), (10, 8))
        self.assertEqual(fields[0].members, [(0, "unsigned int"), (4, "unsigned int")])
        self.assertEqual((fields[1].ctype, fields[1].count), ("unsigned char", 4))   # remainder

    def test_address_taken_buffer(self):
        refs = self.refs((0x3000, 0, {"taken": True}), (0x3400, 4, {}))
        fields = cu.layout(refs, 0x3404)
        self.assertEqual((fields[0].ctype, fields[0].count), ("unsigned char", 0x400))

    def test_adjacent_scalars_merge_only_when_asked(self):
        refs = self.refs(*[(0x4000 + 4 * i, 4, {}) for i in range(6)])
        self.assertEqual(len(cu.layout(refs, 0x4018)), 1)
        self.assertEqual(len(cu.layout(refs, 0x4018, merge=False)), 6)

    def test_common_item_is_one_element_unless_indexed(self):
        refs = self.refs((0x5000, 4, {}), (0x5100, 4, {"scale": 4}))
        fields = cu.layout(refs, 0x5104, common=True, clusters=False)
        self.assertEqual([(f.start, f.count) for f in fields], [(0x5000, 1), (0x5100, 1)])

    def test_region_end_adds_the_last_width(self):
        refs = self.refs((0x6000, 4, {}), (0x6020, 8, {"is_float": True}))
        self.assertEqual(cu.region_end(refs, 0x6020), 0x6028)
        self.assertEqual(cu.region_end(refs, 0x7000), 0x7004)       # unknown width: a word


class WiringTest(unittest.TestCase):
    LIST = ("# game\n"
            "lrr_link(legacy 00401800 game          game/old.cpp)\n"
            "lrr_link(unit   GAME-B   game          game/beta.c)\n"
            "\n# gods98\n"
            "lrr_link(legacy 00403800 gods98        gods98/mixed.cpp)\n"
            "\n# Port tail\n"
            "lrr_link(tail   -        program       main.cpp)\n"
            "lrr_link(legacy 00404800 gods98        crt/crt.cpp)\n")
    STARTS = {"GAME-A": 0x401000, "GAME-B": 0x402000, "G98-C": 0x403000, "G98-D": 0x404000}

    def wire(self, text, unit, lib, source):
        return cu.wire_link_order(text, cu.Wiring(unit, lib, source, []), self.STARTS)

    def test_unit_goes_to_its_original_position(self):
        first = self.wire(self.LIST, "GAME-A", "game", "game/alpha.c")
        self.assertIn("# game\nlrr_link(unit   GAME-A   game          game/alpha.c)\n"
                      "lrr_link(legacy 00401800", first)
        late = self.wire(self.LIST, "G98-D", "gods98", "gods98/delta.c")
        self.assertIn("gods98/mixed.cpp)\nlrr_link(unit   G98-D    gods98        gods98/delta.c)\n"
                      "\n# Port tail", late)             # before the tail, not by crt.cpp's key

    def test_wiring_is_idempotent_and_refuses_duplicates(self):
        once = self.wire(self.LIST, "G98-C", "gods98", "gods98/gamma.c")
        self.assertEqual(self.wire(once, "G98-C", "gods98", "gods98/gamma.c"), once)
        with self.assertRaises(ValueError):
            self.wire(once, "G98-C", "gods98", "gods98/other.c")
        with self.assertRaises(ValueError):
            self.wire(once, "GAME-A", "game", "game/old.cpp")

    def test_boundary_test_includes(self):
        text = '#include "a.h"\n#include "b.h"\n\nint main(void)\n{\n    return 0;\n}\n'
        once = cu.wire_boundary_test(text, ["gods98/view.hpp", "gods98/keys.h", "a.h"])
        self.assertIn('#include "b.h"\n#include "gods98/view.hpp"\n#include "gods98/keys.h"\n\nint main', once)
        self.assertEqual(cu.wire_boundary_test(once, ["gods98/keys.h"]), once)

    def test_relocation_private_headers_are_not_boundary_includes(self):
        # A relocation can move another unit's prototype into a private
        # <file>_shared.hpp (#446: util.cpp's random_float_range, a G98-007
        # function). Only util.cpp includes that header. Since #496 the units
        # relocated from util.cpp include per-unit snapshots of it instead.
        from types import SimpleNamespace
        fp = SimpleNamespace(new_files=["src/gods98/maths.c", "src/gods98/maths.h"],
                             own_declaration_headers=["src/game/util_shared.hpp",
                                                      "src/game/util_for_bubbles.hpp",
                                                      "src/game/util_from_bubbles.hpp",
                                                      "src/gods98/maths.c",
                                                      "src/gods98/view.hpp"])
        self.assertEqual(cu.boundary_test_headers(fp), ["src/gods98/view.hpp"])


class RenderTest(unittest.TestCase):
    def test_comment_safety(self):
        self.assertNotIn("*/", cu.comment_safe("a */ b /* c"))

    def test_float_literals(self):
        self.assertEqual(cu.float_literal(0.0, False), "0.0f")
        self.assertEqual(cu.float_literal(255.0, True), "255.0")
        single = struct.unpack("<f", struct.pack("<f", 1.0 / 360.0))[0]
        self.assertEqual(cu.float_literal(single, False), "0.00277777785f")


@unittest.skipUnless(HAVE_EXE, "reference/LegoRR.exe is needed (symlink reference/ into the worktree)")
class HandConversionValidationTest(unittest.TestCase):
    """The scaffold's mechanical output against the merged hand conversions."""

    @classmethod
    def setUpClass(cls):
        cls.index = cf.SourceIndex(REPO)
        cls.results = {}
        for unit, stem in (("GAME-034", "priorities"), ("G98-006", "config"), ("G98-004", "utils"),
                            ("G98-007", "maths")):
            sc = cu.build_scaffold(unit, REPO, stem=stem, index=cls.index)
            cls.results[unit] = (sc, cu.compare_with_hand(sc, REPO))

    def kinds(self, unit):
        return {d["kind"] for d in self.results[unit][1]["differences"]}

    def test_function_annotations_and_order_match(self):
        for unit in self.results:
            res = self.results[unit][1]
            self.assertTrue(res["functions"]["same_order"], unit)
            self.assertEqual(res["functions"]["hand"], res["functions"]["tool"], unit)
            self.assertEqual(res["header_declares"]["declared"], res["header_declares"]["unit_functions"], unit)

    def test_link_order_wiring_and_boundary_includes_match(self):
        for unit in self.results:
            res = self.results[unit][1]
            self.assertTrue(res["link_order_wiring_matches"], unit)
            self.assertEqual(res["boundary_includes"]["missing_from_committed_test"], [], unit)

    def test_priorities_globals_range_and_layout(self):
        sc, res = self.results["GAME-034"]
        self.assertEqual((res["struct"]["hand_start"], res["struct"]["tool_start"]), ("00500f00", "00500f00"))
        self.assertEqual(res["struct"]["tool_end"], "005013c0")
        # Every hand-made field starts a tool field; the tool only splits aggregates.
        self.assertNotIn("struct-fields-only-in-hand", self.kinds("GAME-034"))
        self.assertEqual(res["struct"]["split_inside_hand_fields"],
                         ["00501118", "00501264", "005013ac", "005013b0", "005013b4"])
        self.assertEqual(res["globals"]["hand"], ["004a8ee8", "004a8eec", "00500f00"])
        # The one extra GLOBAL is the x87 constant that the hand version writes as a literal.
        self.assertEqual([d["detail"] for d in res["differences"] if d["kind"] == "globals-only-in-tool"],
                         [["0049e654"]])

    def test_config_layout_is_identical(self):
        sc, res = self.results["G98-006"]
        self.assertEqual(res["struct"]["hand_fields"], res["struct"]["tool_fields"])
        self.assertEqual(res["struct"]["split_inside_hand_fields"], [])
        self.assertEqual(res["struct"]["tool_end"], "00506524")
        self.assertEqual({d["kind"] for d in res["differences"]}, {"globals-order", "strings-only-in-tool"})
        # The hand version's shared literals and the empty string are all reported by the tool.
        self.assertEqual(res["globals"]["covered_by_tool_as_shared"], ["004a1cd4", "004aab18", "004b8a14"])

    def test_utils_stateless_unit_with_string_markers(self):
        sc, res = self.results["G98-004"]
        self.assertEqual(res["differences"], [])          # incl. no false global for 0x600937
        self.assertEqual(res["struct"]["tool_fields"], 0)
        self.assertEqual({f"{a:08x}" for a, _ in sc.string_literals}, {"004aabd8", "004aabe0", "004aabe4", "004aabec"})
        self.assertEqual({f"{a:08x}" for a, _, _ in sc.shared_literals}, {"004a4120", "004a4124"})


    def test_maths_rdata_constants_and_no_bss(self):
        """maths.c: 20 functions, seven annotated .rdata floats (one read by no instruction), no .bss."""
        sc, res = self.results["G98-007"]
        self.assertEqual(res["differences"], [])
        self.assertEqual(res["functions"]["tool"], 20)
        self.assertEqual(res["struct"]["tool_fields"], 0)
        self.assertEqual(res["globals"]["tool"], res["globals"]["hand"])
        # 0049e934 is the 2.0f that MSVC folded into fadd: only the unit's range shows it.
        self.assertIn("0049e934", res["globals"]["tool"])
        text = sc.files["src/gods98/maths.c"]
        self.assertIn("static const float s_rdata_0049e934 = 2.0f;", text)
        # The D3DRM import-thunk callees are library calls: no header work.
        self.assertFalse(any(c.needs_extern_c or c.needs_boundary_decl
                             for c in sc.footprint.callees if c.unit_kind not in cf.CONVERTIBLE_KINDS))


@unittest.skipUnless(HAVE_EXE, "reference/LegoRR.exe is needed (symlink reference/ into the worktree)")
class ScaffoldOutputTest(unittest.TestCase):
    def test_keys_scaffold_text(self):
        sc = cu.build_scaffold("Keys.c", REPO)
        text = sc.files["src/gods98/keys.c"]
        self.assertTrue(text.startswith("// SPDX-License-Identifier: LGPL-3.0-or-later"))
        self.assertLess(text.index("// FUNCTION: LEGORR 0x00485a20"), text.index("// FUNCTION: LEGORR 0x00485e30"))
        self.assertIn("Ghidra pseudocode", text)
        self.assertNotIn("*/ */", text)
        self.assertTrue(sc.files["src/gods98/keys.h"].startswith("// SPDX-License-Identifier"))
        self.assertIn("#ifdef __cplusplus", sc.files["src/gods98/keys.h"])
        # The common-region key-name table is scaffolded as one array.
        self.assertRegex(text, r"// GLOBAL: LEGORR 0x005765e0\nunsigned int g_005765e0\[84\]")

    def test_report_and_stubs_say_the_gate_fails_until_old_code_is_deleted(self):
        sc = cu.build_scaffold("G98-029", REPO)
        report = cu.render_report(sc, [], "")
        self.assertIn("gate will fail until the old definitions are deleted", report)
        self.assertIn("FAILS (the old function drops out of the report)", sc.files["src/gods98/g98_029.c"])

    def test_nothing_is_written_in_dry_run(self):
        import subprocess
        before = subprocess.run(["git", "status", "--porcelain"], cwd=REPO, capture_output=True, text=True).stdout
        with contextlib.redirect_stdout(io.StringIO()):
            rc = cu.main(["G98-029", "--dry-run", "--scores", "none"])
        after = subprocess.run(["git", "status", "--porcelain"], cwd=REPO, capture_output=True, text=True).stdout
        self.assertEqual(rc, 0)
        self.assertEqual(before, after)

    def test_scratch_output_leaves_the_checkout_alone(self):
        original = (REPO / "src/gods98/g98_029.c").read_bytes() if (REPO / "src/gods98/g98_029.c").exists() else None
        with tempfile.TemporaryDirectory() as tmp:
            with contextlib.redirect_stdout(io.StringIO()):
                rc = cu.main(["G98-029", "--out", tmp, "--scores", "none"])
            self.assertEqual(rc, 0)
            self.assertTrue((Path(tmp) / "src/gods98/g98_029.c").exists())
            listing = (Path(tmp) / "src/link_order.cmake").read_text()
            self.assertIn("lrr_link(unit   G98-029  gods98        gods98/g98_029.c)", listing)
            checkout = REPO / "src/gods98/g98_029.c"
            self.assertEqual(checkout.read_bytes() if checkout.exists() else None, original)

    def test_leaf_units_are_leaves(self):
        # The wave-reader and MCI units of the first leaf wave (G98-022 and
        # G98-023) were merged into G98-021, which is not a leaf (#409/#410).
        index = cf.SourceIndex(REPO)
        for unit in ("G98-019", "G98-024", "G98-026", "G98-027", "G98-029",
                     "Keys.c", "GAME-003"):
            fp = cf.compute_footprint(unit, REPO, index=index, with_data=False)
            self.assertTrue(fp.leaf, unit)
            self.assertFalse(any(c.needs_extern_c for c in fp.callees), unit)


if __name__ == "__main__":
    unittest.main()
