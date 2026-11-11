# SPDX-License-Identifier: LGPL-3.0-or-later
"""Tests for tools/relocate_unit.py and tools/cxx_items.py on synthetic sources.

    python3 -m unittest discover -s tools/tests -p 'test_relocate_unit.py'
"""

from __future__ import annotations

import contextlib
import csv
import io
import sys
import tempfile
import textwrap
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import cxx_items as cx  # noqa: E402
import link_order as lo  # noqa: E402
import relocate_unit as ru  # noqa: E402

RANGES_HEADER = ("unit_order,unit_id,unit_name,unit_kind,name_basis,text_start,text_end,functions,"
                 "anchored_functions,boundary_confidence,boundary_evidence,boundary_slack,rdata_range,"
                 "data_range,string_range,bss_range,note\n")
RANGES = RANGES_HEADER + (
    "0,GAME-001,Alpha.c,game,string-prefix,00401000,00401100,3,3,high,x,0,,,,004b0000-004b0004,\n"
    "1,GAME-002,Beta.c,game,string-prefix,00401100,00402000,2,2,high,x,0,,,,004b0010-004b0010,\n"
    "2,GAME-003,,game,none,00402000,00403000,1,1,high,x,0,,,,,\n")
FUNCTIONS = (
    "address,function_name,unit_order,unit_id,unit_name,unit_kind,confidence,evidence,current_source\n"
    "00401000,AlphaFirst,0,GAME-001,Alpha.c,game,high,bss,src/game/mixed.cpp\n"
    "00401040,AlphaSecond,0,GAME-001,Alpha.c,game,high,bss,src/game/mixed.cpp\n"
    "00401080,AlphaMissing,0,GAME-001,Alpha.c,game,high,bss,\n"
    "00401100,BetaOne,1,GAME-002,Beta.c,game,high,bss,src/game/mixed.cpp\n"
    "00402000,GammaOne,2,GAME-003,,game,high,bss,src/game/other.cpp\n")
SPEC = ("start,unit_id,name,kind,name_basis,note\n"
        "00401000,GAME-001,Alpha.c,game,string-prefix,\n"
        "00401100,GAME-002,Beta.c,game,string-prefix,\n"
        "00402000,GAME-003,,game,none,\n"
        "00480000,G98-001,,gods98,none,\n"
        "00491000,CRT-001,,crt,none,\n")
LIST = """# Game segment
lrr_link(legacy 00401040 game          game/mixed.cpp)
lrr_link(legacy 00402000 game          game/other.cpp)

# Port tail
lrr_link(tail   -        program       main.cpp)

# Legacy files whose functions lie in the CRT range
lrr_link(legacy 00491450 gods98        crt/crt.cpp)
"""
MESON = """game_sources = [
  'mixed.cpp',
  'other.cpp',
]
game_lib = static_library('game', game_sources, install : false)
"""
HEADER = """#ifndef MIXED_HPP
#define MIXED_HPP
int alpha_first(int x);
int alpha_second(void);
int beta_one(void);
#endif
"""
MIXED = """// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>
#include "mixed.hpp"

#define ALPHA_SCALE 3

typedef struct AlphaPair {
    int a;
    int b;
} AlphaPair;

static int s_shared_counter;
// GLOBAL: LEGORR 0x004b0000
static int s_alpha_state = 0; /* owned by Alpha */
// GLOBAL: LEGORR 0x004b0010
static int s_beta_state;

static int alpha_helper(int v);

/* Beta comes first in the file. */
// FUNCTION: LEGORR 0x00401100
int beta_one(void)
{
    s_shared_counter++;
    return s_beta_state;
}

// FUNCTION: LEGORR 0x00401040
int alpha_second(void)
{
    AlphaPair p = { 1, 2 };
    s_shared_counter += p.a;
    return alpha_helper(s_alpha_state) * ALPHA_SCALE;
}

/* The first function of Alpha.c. */
// FUNCTION: LEGORR 0x00401000
int alpha_first(int x)
{
    s_alpha_state = x;
    return alpha_second();
}

static int alpha_helper(int v)
{
    return v + 1;
}
"""
OTHER = """// SPDX-License-Identifier: LGPL-3.0-or-later
// FUNCTION: LEGORR 0x00402000
int gamma_one(void)
{
    return 3;
}
"""


class Fixture:
    def __init__(self, mixed: str = MIXED, extra: dict[str, str] | None = None):
        self.dir = tempfile.TemporaryDirectory()
        self.root = Path(self.dir.name)
        files = {
            "docs/coverage/translation-unit-ranges.csv": RANGES,
            "docs/coverage/translation-units.csv": FUNCTIONS,
            "docs/coverage/translation-units.spec.csv": SPEC,
            "src/link_order.cmake": LIST,
            "src/game/meson.build": MESON,
            "src/game/mixed.hpp": HEADER,
            "src/game/mixed.cpp": mixed,
            "src/game/other.cpp": OTHER,
            "src/main.cpp": "int main(void) { return 0; }\n",
            "src/crt/crt.cpp": "// LIBRARY: LEGORR 0x00491450\nvoid crt_fn(void) {}\n",
        }
        files.update(extra or {})
        for rel, text in files.items():
            path = self.root / rel
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text, encoding="utf-8")

    def read(self, rel: str) -> str:
        return (self.root / rel).read_text(encoding="utf-8")

    def close(self):
        self.dir.cleanup()


class ScannerTest(unittest.TestCase):
    def test_mask_keeps_offsets(self):
        text = 'int a; // c { }\nconst char* s = "{ /* }";\n/* { */ int b;\n'
        masked = cx.mask(text)
        self.assertEqual(len(masked), len(text))
        self.assertNotIn("{", masked)
        self.assertEqual(masked.count("\n"), text.count("\n"))

    def test_items_names_and_linkage(self):
        text = textwrap.dedent("""\
            #define N 4
            typedef void (*Hook)(int);
            typedef struct S { int x; } S, *PS;
            enum Mode { MODE_A, MODE_B };
            static int s_a[N], s_b = 2;
            extern int g_ext;
            static void (*s_fp)(int) = 0;
            int proto(int);
            __declspec(naked) static void naked_fn(void)
            {
                __asm { ret }
            }
            int obj::member(void) { return 0; }
            """)
        items = cx.scan(text).items
        by = {tuple(i.names): i for i in items}
        self.assertEqual(by[("N",)].category, "macro")
        self.assertEqual(by[("Hook",)].category, "type")
        self.assertEqual(by[("S", "PS")].category, "type")
        self.assertEqual(by[("Mode", "MODE_A", "MODE_B")].category, "type")
        multi = by[("s_a", "s_b")]
        self.assertTrue(multi.static and multi.multi)
        self.assertEqual(multi.category, "var")
        self.assertEqual(by[("s_fp",)].category, "var")
        self.assertEqual(by[("proto",)].category, "proto")
        naked = by[("naked_fn",)]
        self.assertEqual(naked.category, "function")
        self.assertTrue(naked.static)
        self.assertIn("qualified (member or namespace) definition", by[("member",)].problems)

    def test_if_group_of_alternatives_merges(self):
        text = textwrap.dedent("""\
            // FUNCTION: LEGORR 0x00401000
            #if defined(_MSC_VER)
            __declspec(naked) void f(void)
            {
                __asm { ret }
            }
            #else
            void f(void)
            {
            }
            #endif
            #if defined(X)
            int only_x;
            #endif
            """)
        items = cx.scan(text).items
        f = next(i for i in items if i.names == ["f"])
        self.assertTrue(f.group)
        self.assertEqual(f.cond, 0)
        self.assertEqual(f.annotations, [("FUNCTION", 0x401000)])
        only = next(i for i in items if i.names == ["only_x"])
        self.assertEqual(only.cond, 1)

    def test_directive_inside_declaration_is_a_problem(self):
        text = "int x =\n#if A\n1\n#else\n2\n#endif\n;\n"
        item = cx.scan(text).items[0]
        self.assertTrue(any("splits the declaration" in p for p in item.problems))

    def test_leading_comments_attach_without_blank_line(self):
        text = "/* detached */\n\n/* attached */\n// GLOBAL: LEGORR 0x004b0000\nint g;\n"
        item = cx.scan(text).items[0]
        self.assertTrue(item.text(text).startswith("/* attached */"))
        self.assertEqual(item.annotations, [("GLOBAL", 0x4b0000)])

    def test_strip_static_only_at_declaration_level(self):
        code = "static int f(void)\n{\n    static int n;\n    return n;\n}\n"
        self.assertEqual(ru._strip_static(code), "int f(void)\n{\n    static int n;\n    return n;\n}\n")


class RelocateTest(unittest.TestCase):
    def setUp(self):
        self.fx = Fixture()
        self.root = self.fx.root

    def tearDown(self):
        self.fx.close()

    def test_dry_run_plan_writes_nothing(self):
        before = {p: p.read_text() for p in self.root.rglob("*") if p.is_file()}
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = ru.main(["GAME-001", "--root", str(self.root), "--dry-run", "--no-footprint"])
        self.assertEqual(code, 0)
        after = {p: p.read_text() for p in self.root.rglob("*") if p.is_file()}
        self.assertEqual(before, after)
        self.assertIn("Would write:", out.getvalue())
        self.assertIn("Relocation checklist", out.getvalue())

    def test_moves_functions_in_address_order_with_closure(self):
        plan = ru.plan_relocation("GAME-001", self.root)
        roles = {m.name: m.role for m in plan.order}
        self.assertEqual(roles["alpha_first"], "function")
        self.assertEqual(roles["s_alpha_state"], "global")
        helper = sorted(m.item.category for m in plan.order if m.name == "alpha_helper")
        self.assertEqual(helper, ["function", "proto"])          # its prototype and definition
        self.assertEqual(roles["alpha_helper"], "closure-function")
        self.assertEqual(plan.missing, [(0x401080, "AlphaMissing")])
        unit = plan.files["src/game/alpha.cpp"]
        self.assertLess(unit.index("0x00401000"), unit.index("0x00401040"))
        # The static helper follows the unit's functions; its prototype precedes them.
        self.assertLess(unit.index("static int alpha_helper(int v);"), unit.index("0x00401000"))
        self.assertGreater(unit.index("static int alpha_helper(int v)\n{"), unit.index("0x00401040"))
        # Includes keep the source's order.
        self.assertLess(unit.index("#include <string.h>"), unit.index('#include "mixed.hpp"'))
        self.assertIn('#include "mixed_shared.hpp"', unit)
        # Leading comments travel with their item.
        self.assertIn("/* The first function of Alpha.c. */\n// FUNCTION: LEGORR 0x00401000", unit)

    def test_shared_statics_types_and_macros_go_to_the_private_header(self):
        plan = ru.plan_relocation("GAME-001", self.root)
        header = plan.files["src/game/mixed_shared.hpp"]
        self.assertIn("extern int s_shared_counter;", header)
        self.assertNotIn("#define ALPHA_SCALE", header)        # only moved code uses it
        unit = plan.files["src/game/alpha.cpp"]
        self.assertIn("#define ALPHA_SCALE 3", unit)
        self.assertIn("typedef struct AlphaPair", unit)
        mixed = plan.files["src/game/mixed.cpp"]
        self.assertIn("\nint s_shared_counter;", mixed)          # static removed, stays
        self.assertIn('#include "mixed_shared.hpp"', mixed)
        self.assertNotIn("alpha_first", mixed)
        self.assertNotIn("ALPHA_SCALE", mixed)
        self.assertIn("// FUNCTION: LEGORR 0x00401100", mixed)
        self.assertNotIn("mixed.hpp", [p for p in plan.files])   # shared header untouched

    def test_records_hashes_and_wiring(self):
        plan = ru.plan_relocation("GAME-001", self.root)
        ru.write(plan, self.root)
        rows = list(csv.DictReader(io.StringIO(self.fx.read("docs/coverage/relocations/GAME-001.csv"))))
        fn = [r for r in rows if r["role"] == "function"]
        self.assertEqual([r["address"] for r in fn], ["0x00401000", "0x00401040"])
        for r in rows:
            if not r["edit"]:
                self.assertEqual(r["before_sha256"], r["after_sha256"], r)
        stays = [r for r in rows if r["role"].startswith("stays")]
        self.assertEqual([r["name"] for r in stays], ["s_shared_counter"])
        entries = lo.parse(self.fx.read("src/link_order.cmake"))
        self.assertEqual((entries[0].kind, entries[0].key, entries[0].source),
                         ("unit", "GAME-001", "game/alpha.cpp"))
        # mixed.cpp's lower median moves from Alpha's 00401040 to Beta's 00401100.
        self.assertEqual((entries[1].key, entries[1].source), ("00401100", "game/mixed.cpp"))
        self.assertEqual(lo.check(self.root), [])
        import meson_source_parity
        sources = meson_source_parity.meson_array(self.fx.read("src/game/meson.build"), "game_sources")
        self.assertEqual(sources, ["alpha.cpp", "mixed.cpp", "other.cpp"])
        self.assertEqual({"src/" + e.source for e in entries if e.library == "game"},
                         {"src/game/" + source for source in sources})
        tu = self.fx.read("docs/coverage/translation-units.csv")
        self.assertIn("00401000,AlphaFirst,0,GAME-001,Alpha.c,game,high,bss,src/game/alpha.cpp", tu)
        self.assertIn("00401100,BetaOne,1,GAME-002,Beta.c,game,high,bss,src/game/mixed.cpp", tu)

    def test_second_run_is_a_no_op(self):
        ru.write(ru.plan_relocation("GAME-001", self.root), self.root)
        snapshot = {p: p.read_text() for p in self.root.rglob("*") if p.is_file()}
        plan = ru.plan_relocation("GAME-001", self.root)
        self.assertTrue(plan.done)
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(ru.main(["GAME-001", "--root", str(self.root), "--no-footprint"]), 0)
        self.assertEqual(snapshot, {p: p.read_text() for p in self.root.rglob("*") if p.is_file()})

    def test_second_unit_appends_to_the_private_header(self):
        ru.write(ru.plan_relocation("GAME-001", self.root), self.root)
        plan = ru.plan_relocation("GAME-002", self.root)
        ru.write(plan, self.root)
        header = self.fx.read("src/game/mixed_shared.hpp")
        self.assertEqual(header.count("extern int s_shared_counter;"), 1)
        self.assertEqual(header.count("#endif"), 1)
        # mixed.cpp is drained of original functions: it joins the port tail.
        entries = lo.parse(self.fx.read("src/link_order.cmake"))
        mixed = next(e for e in entries if e.source == "game/mixed.cpp")
        self.assertEqual((mixed.kind, mixed.key), ("tail", "-"))
        self.assertEqual([e.source for e in entries if e.kind == "unit"], ["game/alpha.cpp", "game/beta.cpp"])
        self.assertEqual(lo.check(self.root), [])

    def test_partial_state_is_refused(self):
        ru.write(ru.plan_relocation("GAME-001", self.root), self.root)
        path = self.root / "src/game/other.cpp"
        path.write_text(path.read_text() + "// FUNCTION: LEGORR 0x004010c0\nint late(void) { return 0; }\n")
        with self.assertRaises(ru.Refused) as cm:
            ru.plan_relocation("GAME-001", self.root)
        self.assertIn("still elsewhere", cm.exception.reasons[0])

    def test_unnamed_unit_needs_a_stem(self):
        with self.assertRaises(ru.Refused) as cm:
            ru.plan_relocation("GAME-003", self.root)
        self.assertIn("--stem", cm.exception.reasons[0])
        plan = ru.plan_relocation("GAME-003", self.root, stem="gamma")
        self.assertIn("src/game/gamma.cpp", plan.files)


class RefusalTest(unittest.TestCase):
    def refuse(self, mixed: str, needle: str, extra: dict | None = None):
        fx = Fixture(mixed, extra)
        try:
            with self.assertRaises(ru.Refused) as cm:
                ru.plan_relocation("GAME-001", fx.root)
            self.assertTrue(any(needle in r for r in cm.exception.reasons), cm.exception.reasons)
            out = io.StringIO()
            with contextlib.redirect_stderr(out):
                self.assertEqual(ru.main(["GAME-001", "--root", str(fx.root), "--no-footprint"]), 2)
            self.assertIn("refused", out.getvalue())
        finally:
            fx.close()

    def test_definition_inside_if_block(self):
        self.refuse(MIXED.replace("// FUNCTION: LEGORR 0x00401040\nint alpha_second(void)",
                                  "#if defined(A)\n#endif\n#if defined(B)\n// FUNCTION: LEGORR 0x00401040\n"
                                  "int alpha_second(void)").replace(
                                      "    return alpha_helper(s_alpha_state) * ALPHA_SCALE;\n}\n",
                                      "    return alpha_helper(s_alpha_state) * ALPHA_SCALE;\n}\nint b2;\n#endif\n"),
                    "inside an #if block")

    def test_macro_built_definition(self):
        self.refuse(MIXED.replace("// FUNCTION: LEGORR 0x00401000\nint alpha_first(int x)",
                                  "// FUNCTION: LEGORR 0x00401000\nDECLARE_IT(x) int alpha_first(int x)"),
                    "macro or complex declarator")

    def test_named_namespace(self):
        helper_in_ns = MIXED.replace("static int alpha_helper(int v);\n", "").replace(
            "static int alpha_helper(int v)\n{\n    return v + 1;\n}\n", "")
        helper_in_ns = helper_in_ns.replace("// FUNCTION: LEGORR 0x00401040",
                                            "namespace port {\nint alpha_helper(int v) { return v; }\n}\n\n"
                                            "// FUNCTION: LEGORR 0x00401040")
        self.refuse(helper_in_ns, "defined in namespace port")
        definition_in_ns = MIXED.replace("// FUNCTION: LEGORR 0x00401000\nint alpha_first(int x)\n{\n"
                                         "    s_alpha_state = x;\n    return alpha_second();\n}\n",
                                         "namespace port {\n// FUNCTION: LEGORR 0x00401000\nint alpha_first(int x)\n{\n"
                                         "    s_alpha_state = x;\n    return alpha_second();\n}\n}\n")
        self.refuse(definition_in_ns, "inside namespace port")

    def test_function_defined_in_a_header(self):
        self.refuse(MIXED, "edit a shared header",
                    {"src/game/inline.hpp": "// FUNCTION: LEGORR 0x00401080\ninline int alpha_missing(void) { return 0; }\n"})

    def test_annotation_not_attached(self):
        self.refuse(MIXED.replace("// FUNCTION: LEGORR 0x00401040\n", "// FUNCTION: LEGORR 0x00401040\n\n"),
                    "not attached")

    def test_shared_inline(self):
        mixed = MIXED.replace("static int alpha_helper(int v)\n{", "static inline int alpha_helper(int v)\n{") \
            .replace("static int alpha_helper(int v);", "static inline int alpha_helper(int v);") \
            .replace("    return s_beta_state;", "    return s_beta_state + alpha_helper(1);")
        self.refuse(mixed, "inline alpha_helper")

    def test_several_declarators_shared(self):
        mixed = MIXED.replace("static int s_shared_counter;", "static int s_shared_counter, s_other;")
        self.refuse(mixed, "share one declaration")

    def test_macro_redefined(self):
        self.refuse(MIXED + "#undef ALPHA_SCALE\n", "#undef'd or redefined")

    def test_name_clash_between_files(self):
        other = OTHER + "int s_shared_counter;\n"
        self.refuse(MIXED, "would get external linkage", {"src/game/other.cpp": other})


class OrderAndSurveyTest(unittest.TestCase):
    def test_variable_moves_before_the_global_that_uses_it(self):
        mixed = MIXED.replace("// GLOBAL: LEGORR 0x004b0000\nstatic int s_alpha_state = 0; /* owned by Alpha */",
                              "static int s_seed = 4;\n// GLOBAL: LEGORR 0x004b0000\n"
                              "static int* s_alpha_state_ptr = &s_seed;\nstatic int s_alpha_state = 0;")
        mixed = mixed.replace("    s_alpha_state = x;", "    s_alpha_state = x + *s_alpha_state_ptr;")
        fx = Fixture(mixed)
        try:
            unit = ru.plan_relocation("GAME-001", fx.root).files["src/game/alpha.cpp"]
            self.assertLess(unit.index("static int s_seed = 4;"), unit.index("s_alpha_state_ptr = &s_seed"))
        finally:
            fx.close()

    def test_if_wrapped_definition_names_the_if_block(self):
        mixed = MIXED.replace("// FUNCTION: LEGORR 0x00401040\nint alpha_second(void)",
                              "// FUNCTION: LEGORR 0x00401040\n#if defined(A)\n__declspec(naked)\n#endif\n"
                              "int alpha_second(void)")
        fx = Fixture(mixed)
        try:
            with self.assertRaises(ru.Refused) as cm:
                ru.plan_relocation("GAME-001", fx.root)
            self.assertIn("inside an #if block", cm.exception.reasons[0])
            self.assertEqual(ru.classify_reason(cm.exception.reasons[0]), "#if block")
        finally:
            fx.close()

    def test_globals_in_another_units_file_stay(self):
        extra = {"src/game/beta.c": "// GLOBAL: LEGORR 0x004b0004\nstatic const char s_pooled[4] = { 0 };\n",
                 "src/link_order.cmake": LIST.replace("# Port tail",
                                                      "lrr_link(unit   GAME-002 game          game/beta.c)\n\n# Port tail")}
        fx = Fixture(MIXED, extra)
        try:
            plan = ru.plan_relocation("GAME-001", fx.root)
            self.assertNotIn("src/game/beta.c", plan.files)
            self.assertTrue(any("stays in src/game/beta.c" in n for n in plan.notes))
        finally:
            fx.close()

    def test_survey_tabulates_every_unrelocated_unit(self):
        fx = Fixture()
        try:
            results = {r["unit"]: r for r in ru.survey(fx.root)}
            self.assertEqual(sorted(results), ["GAME-001", "GAME-002", "GAME-003"])
            self.assertEqual(results["GAME-001"]["status"], "ok")
            self.assertEqual(results["GAME-001"]["functions"], 2)
            self.assertIn("Units blocked by each construct", ru.render_survey(list(results.values())))
        finally:
            fx.close()


class LinkOrderPlaceTest(unittest.TestCase):
    STARTS = {"GAME-001": 0x401000, "GAME-002": 0x401100, "GAME-003": 0x402000}

    def test_place_legacy_rekeys_and_reorders(self):
        text = lo.place_legacy(LIST, "game/mixed.cpp", 0x402100, self.STARTS, 0x491000)
        entries = lo.parse(text)
        self.assertEqual([(e.key, e.source) for e in entries][:2],
                         [("00402000", "game/other.cpp"), ("00402100", "game/mixed.cpp")])
        self.assertEqual(entries[2].kind, "tail")

    def test_place_legacy_in_the_crt_range_goes_after_the_tail(self):
        text = lo.place_legacy(LIST, "game/other.cpp", 0x491500, self.STARTS, 0x491000)
        self.assertEqual([e.source for e in lo.parse(text)],
                         ["game/mixed.cpp", "main.cpp", "crt/crt.cpp", "game/other.cpp"])

    def test_lower_median(self):
        text = "// FUNCTION: LEGORR 0x00401000\n// LIBRARY: LEGORR 0x00402000\n// FUNCTION: LEGORR 0x00403000\n" \
               "// FUNCTION: LEGORR 0x00404000\n// GLOBAL: LEGORR 0x00500000\n"
        self.assertEqual(lo.lower_median(text), 0x402000)
        self.assertIsNone(lo.lower_median("// GLOBAL: LEGORR 0x00500000\n"))


class LinkOrderRetireTest(unittest.TestCase):
    def test_retire_legacy_moves_to_tail(self):
        text = lo.retire_legacy(LIST, "game/other.cpp")
        entries = lo.parse(text)
        self.assertEqual([e.source for e in entries],
                         ["game/mixed.cpp", "main.cpp", "game/other.cpp", "crt/crt.cpp"])
        self.assertEqual(entries[2].kind, "tail")
        self.assertEqual(lo.retire_legacy(text, "game/other.cpp"), text)


if __name__ == "__main__":
    unittest.main()
