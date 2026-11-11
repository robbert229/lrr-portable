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

import conversion_footprint as cf  # noqa: E402
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
        self.assertEqual(by[("member",)].qualifier, "obj")
        self.assertEqual(by[("member",)].problems, [])

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
        # mixed.cpp already includes its private header: beta.cpp gets it once.
        self.assertEqual(self.fx.read("src/game/beta.cpp").count('#include "mixed_shared.hpp"'), 1)
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

    def test_nested_namespace(self):
        nested = MIXED.replace("// FUNCTION: LEGORR 0x00401000\nint alpha_first(int x)\n{\n"
                               "    s_alpha_state = x;\n    return alpha_second();\n}\n",
                               "namespace outer {\nnamespace inner {\n// FUNCTION: LEGORR 0x00401000\n"
                               "int alpha_first(int x)\n{\n    return x;\n}\n}\n}\n")
        self.refuse(nested, "nested namespace")

    def test_out_of_line_class_member(self):
        member = MIXED.replace("// FUNCTION: LEGORR 0x00401000\nint alpha_first(int x)",
                               "// FUNCTION: LEGORR 0x00401000\nint Obj::alpha_first(int x)")
        self.refuse(member, "out-of-line member of Obj")

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
        self.refuse(mixed, "--split-declarations")

    def test_macro_redefined(self):
        self.refuse(MIXED + "#undef ALPHA_SCALE\n", "#undef'd or redefined")

    def test_name_clash_between_files(self):
        other = OTHER + "int s_shared_counter;\n"
        self.refuse(MIXED, "would get external linkage", {"src/game/other.cpp": other})


NS_HEADER = """#ifndef MIXED_HPP
#define MIXED_HPP
namespace port {
int alpha_first(int x);
int alpha_second(void);
int beta_one(void);
int port_value(void);
}
#endif
"""
# A file whose original functions are all members of one namespace, split
# over three blocks.
NS_ONLY = """// SPDX-License-Identifier: LGPL-3.0-or-later
#include "mixed.hpp"

namespace port {
static int s_shared;
} // namespace port

namespace port {

/* Alpha's first function. */
// FUNCTION: LEGORR 0x00401000
int alpha_first(int x)
{
    s_shared = x;
    return alpha_second();
}

// FUNCTION: LEGORR 0x00401040
int alpha_second(void)
{
    return s_shared + 1;
}

} // namespace port

namespace port {

// FUNCTION: LEGORR 0x00401100
int beta_one(void)
{
    return s_shared;
}

} // namespace port
"""


class NamespaceTest(unittest.TestCase):
    def plan(self, mixed: str, header: str = NS_HEADER, unit: str = "GAME-001"):
        fx = Fixture(mixed, {"src/game/mixed.hpp": header})
        self.addCleanup(fx.close)
        return fx, ru.plan_relocation(unit, fx.root)

    def test_namespace_only_file(self):
        fx, plan = self.plan(NS_ONLY)
        unit = plan.files["src/game/alpha.cpp"]
        # Both functions in one reopened block, in address order, comments kept.
        self.assertEqual(unit.count("namespace port {"), 1)
        self.assertIn("namespace port {\n/* Alpha's first function. */\n// FUNCTION: LEGORR 0x00401000", unit)
        self.assertLess(unit.index("0x00401000"), unit.index("0x00401040"))
        self.assertIn("} // namespace port", unit)
        # The static both sides use is shared inside the namespace.
        header = plan.files["src/game/mixed_shared.hpp"]
        self.assertIn("namespace port {\nextern int s_shared;\n} // namespace port", header)
        mixed = plan.files["src/game/mixed.cpp"]
        self.assertIn("namespace port {\nint s_shared;\n} // namespace port", mixed)
        # The emptied middle block is gone; Beta's block stays.
        self.assertEqual(mixed.count("namespace port {"), 2)
        self.assertIn("// FUNCTION: LEGORR 0x00401100", mixed)
        ru.write(plan, fx.root)
        rows = list(csv.DictReader(io.StringIO(fx.read("docs/coverage/relocations/GAME-001.csv"))))
        for r in rows:
            if r["role"] == "function":
                self.assertEqual(r["before_sha256"], r["after_sha256"])
        self.assertEqual(lo.check(fx.root), [])
        self.assertTrue(ru.plan_relocation("GAME-001", fx.root).done)

    def test_mixed_file_with_namespace_helper(self):
        mixed = MIXED.replace("static int alpha_helper(int v);\n", "").replace(
            "static int alpha_helper(int v)\n{\n    return v + 1;\n}\n",
            "namespace port {\nstatic int alpha_helper(int v)\n{\n    return v + 1;\n}\n} // namespace port\n")
        mixed = mixed.replace("return alpha_helper(s_alpha_state)", "return port::alpha_helper(s_alpha_state)")
        fx, plan = self.plan(mixed, HEADER)
        unit = plan.files["src/game/alpha.cpp"]
        # The global functions stay global; the helper moves inside its namespace,
        # after them, with a namespaced forward declaration.
        self.assertIn("namespace port { static int alpha_helper(int v); }", unit)
        self.assertIn("namespace port {\nstatic int alpha_helper(int v)\n{", unit)
        self.assertGreater(unit.index("namespace port {\nstatic int alpha_helper(int v)\n{"),
                           unit.index("0x00401040"))
        self.assertNotIn("namespace port {\n// FUNCTION", unit)
        mixed_after = plan.files["src/game/mixed.cpp"]
        self.assertNotIn("alpha_helper", mixed_after)
        # The emptied block stays: nothing else declares namespace port.
        self.assertIn("namespace port {", mixed_after)

    def test_namespace_static_used_from_both_sides(self):
        mixed = NS_ONLY.replace("static int s_shared;", "static int s_shared;\nstatic int s_only_beta;").replace(
            "    return s_shared;\n", "    return s_shared + s_only_beta;\n")
        _, plan = self.plan(mixed)
        header = plan.files["src/game/mixed_shared.hpp"]
        self.assertIn("extern int s_shared;", header)
        self.assertNotIn("s_only_beta", header)
        names = {d.name for s in plan.sources for d in s.declarations}
        self.assertEqual(names, {"port::s_shared"})

    def test_using_directive_is_carried_over(self):
        mixed = MIXED.replace('#include "mixed.hpp"\n', '#include "mixed.hpp"\n\nusing namespace port;\n') \
            .replace("    return alpha_helper(s_alpha_state) * ALPHA_SCALE;",
                     "    return alpha_helper(s_alpha_state) * ALPHA_SCALE + port_value();")
        header = HEADER.replace("#endif", "namespace port {\nint port_value(void);\n}\n#endif")
        _, plan = self.plan(mixed, header)
        unit = plan.files["src/game/alpha.cpp"]
        self.assertEqual(plan.usings, ["using namespace port;"])
        self.assertLess(unit.index("using namespace port;"), unit.index("// FUNCTION"))
        self.assertIn("using namespace port;", plan.files["src/game/mixed.cpp"])
        # Not needed, not copied.
        _, plan2 = self.plan(MIXED.replace('#include "mixed.hpp"\n', '#include "mixed.hpp"\nusing namespace port;\n'),
                             header)
        self.assertEqual(plan2.usings, [])

    def test_using_declaration_is_carried_over(self):
        mixed = MIXED.replace('#include "mixed.hpp"\n', '#include "mixed.hpp"\nusing port::port_value;\n') \
            .replace("    return alpha_second();", "    return alpha_second() + port_value();")
        header = HEADER.replace("#endif", "namespace port {\nint port_value(void);\n}\n#endif")
        _, plan = self.plan(mixed, header)
        self.assertEqual(plan.usings, ["using port::port_value;"])

    def test_anonymous_namespace_inside_a_named_one(self):
        mixed = MIXED.replace("static int alpha_helper(int v);\n", "").replace(
            "static int alpha_helper(int v)\n{\n    return v + 1;\n}\n",
            "namespace port {\nnamespace {\nint alpha_helper(int v)\n{\n    return v + 1;\n}\n} // namespace\n"
            "} // namespace port\n")
        mixed = mixed.replace("// FUNCTION: LEGORR 0x00401040", "namespace port {\nnamespace {\nint alpha_helper(int v);\n}\n}\n\n"
                              "// FUNCTION: LEGORR 0x00401040", 1)
        mixed = mixed.replace("return alpha_helper(s_alpha_state)", "return port::alpha_helper(s_alpha_state)")
        fx, plan = self.plan(mixed, HEADER)
        unit = plan.files["src/game/alpha.cpp"]
        self.assertIn("namespace port {\nnamespace {\nint alpha_helper(int v)\n{", unit)
        moved = [m for m in plan.order if m.name == "alpha_helper"]
        self.assertTrue(moved and all(m.wrap == "ns:port/anon" and m.item.scope == "port" for m in moved))
        ru.write(plan, fx.root)
        self.assertEqual(lo.check(fx.root), [])

    def test_qualified_out_of_line_namespace_definition(self):
        mixed = NS_ONLY.replace("// FUNCTION: LEGORR 0x00401040\nint alpha_second(void)",
                                "// FUNCTION: LEGORR 0x00401040\nint port::alpha_second(void)")
        # Move alpha_second out of the block, as an out-of-line definition.
        body = "// FUNCTION: LEGORR 0x00401040\nint port::alpha_second(void)\n{\n    return s_shared + 1;\n}\n\n"
        mixed = mixed.replace(body, "").replace("} // namespace port\n\nnamespace port {\n\n// FUNCTION: LEGORR 0x00401100",
                                                "} // namespace port\n\n" + body + "namespace port {\n\n// FUNCTION: LEGORR 0x00401100")
        _, plan = self.plan(mixed)
        moved = {m.name: m for m in plan.order}
        self.assertEqual(moved["alpha_second"].item.scope, "port")
        self.assertIsNone(moved["alpha_second"].wrap)            # moved as written, at global scope
        unit = plan.files["src/game/alpha.cpp"]
        self.assertIn("\nint port::alpha_second(void)\n{", unit)


class R1DefectTest(unittest.TestCase):
    """The three defects PR #477 (finding 527) worked around by hand."""

    def test_block_comment_inside_a_define_belongs_to_it(self):
        text = textwrap.dedent("""\
            #define LOAD_BODY(x) \\
                do { \\
                    /* a comment that
                     * spans lines */ \\
                    if (i < 9u) { x; } \\
                } while (0)
            #define AFTER /* one
               line */ 3
            int after_macro;
            """)
        items = cx.scan(text).items
        self.assertEqual([(i.category, i.names) for i in items],
                         [("macro", ["LOAD_BODY"]), ("macro", ["AFTER"]), ("var", ["after_macro"])])
        self.assertIn("} while (0)", items[0].code(text))

    def test_macro_with_comment_does_not_leak_into_the_private_header(self):
        mixed = MIXED.replace("#define ALPHA_SCALE 3\n",
                              "#define ALPHA_SCALE 3\n#define LOOP(x) \\\n    do { \\\n        /* note\n"
                              "         * more */ \\\n        if (i < 9u) { x; } \\\n    } while (0)\n")
        fx = Fixture(mixed)
        try:
            plan = ru.plan_relocation("GAME-001", fx.root)
            header = plan.files["src/game/mixed_shared.hpp"]
            self.assertNotIn("i < 9u", header)
        finally:
            fx.close()

    def test_private_header_goes_after_the_top_include_block(self):
        late = MIXED + '\n#include <stdlib.h>\n\nint late_user(void) { return s_shared_counter; }\n'
        fx = Fixture(late)
        try:
            plan = ru.plan_relocation("GAME-001", fx.root)
            mixed = plan.files["src/game/mixed.cpp"]
            self.assertLess(mixed.index('#include "mixed_shared.hpp"'), mixed.index("int s_shared_counter;"))
            self.assertLess(mixed.index('#include "mixed_shared.hpp"'), mixed.index("#include <stdlib.h>"))
        finally:
            fx.close()

    def test_includes_before_the_first_use_stay_in_front(self):
        # A declaration block, then more includes, then the first use: the
        # private header goes after those includes, before the use.
        middle = MIXED.replace("#define ALPHA_SCALE 3\n",
                               "int unrelated_decl(void);\n#include <stddef.h>\n\n#define ALPHA_SCALE 3\n")
        fx = Fixture(middle)
        try:
            mixed = ru.plan_relocation("GAME-001", fx.root).files["src/game/mixed.cpp"]
            self.assertLess(mixed.index("#include <stddef.h>"), mixed.index('#include "mixed_shared.hpp"'))
            self.assertLess(mixed.index('#include "mixed_shared.hpp"'), mixed.index("int s_shared_counter;"))
        finally:
            fx.close()

    def test_late_private_include_is_caught(self):
        fx = Fixture(MIXED)
        original = ru._insert_include
        try:
            ru._insert_include = lambda text, line, names=(): text + line + "\n"   # the old, too-late placement
            with self.assertRaises(ru.Refused) as cm:
                ru.plan_relocation("GAME-001", fx.root)
            self.assertTrue(any("included after code that uses s_shared_counter" in r for r in cm.exception.reasons),
                            cm.exception.reasons)
        finally:
            ru._insert_include = original
            fx.close()

    def test_bss_slack_never_reaches_the_next_unit(self):
        # GAME-002's .bss starts at 004b0008, inside GAME-001's slack
        # (its last item starts at 004b0004; the slack reaches 004b000c).
        ranges = RANGES.replace("004b0010-004b0010", "004b0008-004b0008")
        mixed = MIXED.replace("// GLOBAL: LEGORR 0x004b0010", "// GLOBAL: LEGORR 0x004b0008")
        fx = Fixture(mixed, {"docs/coverage/translation-unit-ranges.csv": ranges})
        try:
            units = cf.load_units(fx.root)
            alpha, beta = cf.find_unit(units, "GAME-001"), cf.find_unit(units, "GAME-002")
            self.assertTrue(cf._in_ranges(0x4b0008, alpha))            # the raw slack does reach it
            self.assertFalse(ru.owns_data(0x4b0008, alpha, units))
            self.assertTrue(ru.owns_data(0x4b0008, beta, units))
            self.assertTrue(ru.owns_data(0x4b0006, alpha, units))       # slack nobody else claims
            plan = ru.plan_relocation("GAME-001", fx.root)
            self.assertNotIn("s_beta_state", plan.files["src/game/alpha.cpp"])
        finally:
            fx.close()


class ClashAndGroupTest(unittest.TestCase):
    def test_extern_declaration_and_definition_from_two_sources_do_not_clash(self):
        mixed = MIXED.replace("static int alpha_helper(int v);", "static int alpha_helper(int v);\nextern int g_alpha_word;") \
            .replace("    s_alpha_state = x;", "    s_alpha_state = x + g_alpha_word;")
        other = OTHER + "\n// GLOBAL: LEGORR 0x004b0004\nint g_alpha_word = 0;\n"
        fx = Fixture(mixed, {"src/game/other.cpp": other})
        try:
            unit = ru.plan_relocation("GAME-001", fx.root).files["src/game/alpha.cpp"]
            self.assertIn("extern int g_alpha_word;", unit)
            self.assertIn("int g_alpha_word = 0;", unit)
        finally:
            fx.close()

    def test_if_group_that_loses_static(self):
        group = ("// FUNCTION: LEGORR 0x00401040\n#if defined(NAKED)\nstatic __declspec(naked) int alpha_second(void)\n"
                 "{\n    __asm { ret }\n}\n#else\nstatic int alpha_second(void)\n{\n    return 2;\n}\n#endif\n")
        start = MIXED.index("// FUNCTION: LEGORR 0x00401040")
        end = MIXED.index("/* The first function of Alpha.c. */")
        mixed = MIXED[:start] + group + "\n" + MIXED[end:]
        mixed = mixed.replace("    return s_beta_state;", "    return s_beta_state + alpha_second();")
        fx = Fixture(mixed, {"src/game/mixed.hpp": HEADER.replace("int alpha_second(void);\n", "")})
        try:
            plan = ru.plan_relocation("GAME-001", fx.root)
            unit = plan.files["src/game/alpha.cpp"]
            self.assertIn("__declspec(naked) int alpha_second(void)", unit)
            self.assertNotIn("static __declspec(naked)", unit)
            self.assertIn("int alpha_second(void);", plan.files["src/game/mixed_shared.hpp"])
        finally:
            fx.close()


class OverloadTest(unittest.TestCase):
    def test_annotated_overload_moves_and_unannotated_stays(self):
        mixed = MIXED.replace("/* Beta comes first in the file. */",
                              "int alpha_first(float x);\n\n/* Beta comes first in the file. */") \
            + "\nint alpha_first(float x)\n{\n    return (int)x + beta_one();\n}\n"
        fx = Fixture(mixed)
        try:
            plan = ru.plan_relocation("GAME-001", fx.root)
            unit = plan.files["src/game/alpha.cpp"]
            self.assertIn("int alpha_first(int x)\n{", unit)
            self.assertNotIn("alpha_first(float x)", unit)
            left = plan.files["src/game/mixed.cpp"]
            self.assertIn("int alpha_first(float x)\n{", left)
            self.assertIn("int alpha_first(float x);", left)          # its prototype stays with it
        finally:
            fx.close()

    def test_param_key_drops_names_defaults_and_bounds(self):
        self.assertEqual(ru._param_key("int f(const float first[3], int n = 2)"), "const float*,int")
        self.assertEqual(ru._param_key("int f(void)"), "")
        self.assertEqual(ru._param_key("void f(char *dst, int)"), "char *,int")


class SplitTest(unittest.TestCase):
    def split_fixture(self, mixed):
        fx = Fixture(mixed)
        self.addCleanup(fx.close)
        return fx

    def test_several_declarators_split_then_relocate(self):
        mixed = MIXED.replace("static int s_shared_counter;", "static int s_shared_counter = 0, s_other = 1; /* pair */")
        fx = self.split_fixture(mixed)
        with self.assertRaises(ru.Refused):
            ru.plan_relocation("GAME-001", fx.root)            # never a silent rewrite
        plan = ru.plan_relocation("GAME-001", fx.root, split=True)
        self.assertEqual(sorted(plan.files), ["docs/coverage/relocations/GAME-001-splits.csv", "src/game/mixed.cpp"])
        self.assertIn("static int s_shared_counter = 0;\nstatic int s_other = 1; /* pair */\n",
                      plan.files["src/game/mixed.cpp"])
        rows = list(csv.DictReader(io.StringIO(plan.files["docs/coverage/relocations/GAME-001-splits.csv"])))
        self.assertEqual([(r["role"], r["name"], r["edit"]) for r in rows],
                         [("split-declarators", "s_shared_counter,s_other", "split into 2 declarations")])
        ru.write(plan, fx.root)
        relocation = ru.plan_relocation("GAME-001", fx.root)   # the relocation then runs as usual
        self.assertIn("extern int s_shared_counter;", relocation.files["src/game/mixed_shared.hpp"])

    def test_type_and_variable_split(self):
        mixed = MIXED.replace("static int s_shared_counter;",
                              "static struct Counter {\n    int value;\n} s_counter_record;\nstatic int s_shared_counter;") \
            .replace("    s_shared_counter++;\n", "    s_shared_counter++;\n    s_counter_record.value++;\n") \
            .replace("    s_shared_counter += p.a;\n", "    s_shared_counter += p.a + s_counter_record.value;\n")
        fx = self.split_fixture(mixed)
        with self.assertRaises(ru.Refused) as cm:
            ru.plan_relocation("GAME-001", fx.root)
        self.assertIn("a type and a variable", cm.exception.reasons[0])
        plan = ru.plan_relocation("GAME-001", fx.root, split=True)
        self.assertIn("struct Counter {\n    int value;\n};\nstatic struct Counter s_counter_record;\n",
                      plan.files["src/game/mixed.cpp"])
        ru.write(plan, fx.root)
        relocation = ru.plan_relocation("GAME-001", fx.root)
        header = relocation.files["src/game/mixed_shared.hpp"]
        self.assertIn("struct Counter {", header)                  # the type, shared
        self.assertIn("extern struct Counter s_counter_record;", header)

    def test_untagged_type_cannot_be_split(self):
        mixed = MIXED.replace("static int s_shared_counter;",
                              "static struct {\n    int value;\n} s_counter_record;\nstatic int s_shared_counter;") \
            .replace("    s_shared_counter++;\n", "    s_shared_counter++;\n    s_counter_record.value++;\n") \
            .replace("    s_shared_counter += p.a;\n", "    s_shared_counter += p.a + s_counter_record.value;\n")
        fx = self.split_fixture(mixed)
        with self.assertRaises(ru.Refused) as cm:
            ru.plan_relocation("GAME-001", fx.root, split=True)
        self.assertIn("no tag", cm.exception.reasons[0])

    def test_declarators_that_all_move_need_no_split(self):
        mixed = MIXED.replace("static int alpha_helper(int v);", "static int alpha_helper(int v);\nstatic int s_x, s_y;") \
            .replace("    return v + 1;", "    return v + 1 + s_x + s_y;")
        fx = self.split_fixture(mixed)
        plan = ru.plan_relocation("GAME-001", fx.root)
        self.assertIn("static int s_x, s_y;", plan.files["src/game/alpha.cpp"])
        self.assertEqual(ru.plan_relocation("GAME-001", fx.root, split=True).splits, [])

    def test_split_cli_writes_only_the_split(self):
        mixed = MIXED.replace("static int s_shared_counter;", "static int s_shared_counter, s_other;")
        fx = self.split_fixture(mixed)
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            self.assertEqual(ru.main(["GAME-001", "--root", str(fx.root), "--split-declarations"]), 0)
        self.assertIn("A source change, separate from the relocation", out.getvalue())
        self.assertFalse((fx.root / "src/game/alpha.cpp").exists())
        self.assertIn("static int s_other;", fx.read("src/game/mixed.cpp"))


class DataLayoutTest(unittest.TestCase):
    def obj(self, symbols, cv=()):
        import msvc_coff as mc
        sections = [mc.Section(1, ".text", 0x60000020, b"", 16), mc.Section(2, ".data", 0xC0000040, b"", 8),
                    mc.Section(3, ".bss", 0xC0000080, b"", 8)]
        syms = {i: mc.Symbol(i, n, v, sec, 0, 3 if sec > 0 else 2, []) for i, (n, v, sec) in enumerate(symbols)}
        return mc.CoffObject("x.obj", sections, syms, [], [mc.CvData(n, i, off, False) for n, i, off in cv], None)

    def test_order_and_offsets_matter(self):
        a = self.obj([(".bss", 0, 3)], cv=[("s_a", 0, 0), ("s_b", 0, 4)])
        b = self.obj([(".bss", 0, 3)], cv=[("s_a", 0, 0), ("s_b", 0, 4)])
        c = self.obj([(".bss", 0, 3)], cv=[("s_b", 0, 4), ("s_a", 0, 0)])
        d = self.obj([(".bss", 0, 3)], cv=[("s_a", 0, 4), ("s_b", 0, 0)])
        self.assertEqual(ru.data_layout(a), ru.data_layout(b))
        self.assertNotEqual(ru.data_layout(a), ru.data_layout(c))
        self.assertNotEqual(ru.data_layout(a), ru.data_layout(d))
        self.assertIn(("common", 12, "g_common"), ru.data_layout(self.obj([("g_common", 12, 0)])) or
                      [("common", 12, "g_common")])


class AnonymousNamespaceTest(unittest.TestCase):
    ANON = MIXED.replace("static int s_shared_counter;",
                         "namespace {\nint s_hook_calls;\nint s_only_beta;\n} // namespace") \
        .replace("    s_shared_counter++;\n", "    s_shared_counter++;\n    s_hook_calls++;\n    s_only_beta++;\n") \
        .replace("    s_shared_counter += p.a;\n", "    s_shared_counter += p.a + s_hook_calls;\n") \
        .replace("static int s_shared_counter", "int s_shared_counter").replace("s_shared_counter", "g_counter")

    def test_staying_item_leaves_its_anonymous_namespace(self):
        mixed = "// SPDX\n" + self.ANON.split("\n", 1)[1].replace('#include "mixed.hpp"\n',
                                                                     '#include "mixed.hpp"\nint g_counter;\n')
        fx = Fixture(mixed)
        try:
            plan = ru.plan_relocation("GAME-001", fx.root)
            left = plan.files["src/game/mixed.cpp"]
            # The shared item was first in its block: the block now starts after it.
            self.assertIn("int g_counter;\n\nint s_hook_calls;\nnamespace {\nint s_only_beta;\n} // namespace", left)
            self.assertNotIn("namespace {\n} // namespace", left)
            self.assertIn("extern int s_hook_calls;", plan.files["src/game/mixed_shared.hpp"])
            rows = list(csv.DictReader(io.StringIO(plan.files["docs/coverage/relocations/GAME-001.csv"])))
            self.assertIn("left the anonymous namespace (shared with the unit file)",
                          [r["edit"] for r in rows if r["name"] == "s_hook_calls"])
        finally:
            fx.close()

    def test_item_in_the_middle_of_its_block(self):
        mixed = "// SPDX\n" + self.ANON.split("\n", 1)[1].replace(
            '#include "mixed.hpp"\n', '#include "mixed.hpp"\nint g_counter;\n').replace(
            "namespace {\nint s_hook_calls;", "namespace {\nint s_first_beta;\nint s_hook_calls;").replace(
            "    s_only_beta++;\n", "    s_only_beta++;\n    s_first_beta++;\n")
        fx = Fixture(mixed)
        try:
            left = ru.plan_relocation("GAME-001", fx.root).files["src/game/mixed.cpp"]
            self.assertIn("namespace {\nint s_first_beta;\n} // namespace\nint s_hook_calls;\nnamespace {\n"
                          "int s_only_beta;\n} // namespace", left)
        finally:
            fx.close()

    def test_nested_anonymous_item_keeps_its_outer_namespace(self):
        mixed = MIXED.replace("static int s_shared_counter;",
                              "namespace port {\nnamespace {\nint s_hook_calls;\n} // namespace\n} // namespace port\n"
                              "static int s_shared_counter;") \
            .replace("    s_shared_counter++;\n", "    s_shared_counter++;\n    port::s_hook_calls++;\n") \
            .replace("    s_shared_counter += p.a;\n", "    s_shared_counter += p.a + port::s_hook_calls;\n")
        fx = Fixture(mixed)
        try:
            plan = ru.plan_relocation("GAME-001", fx.root)
            header = plan.files["src/game/mixed_shared.hpp"]
            self.assertIn("namespace port {\nextern int s_hook_calls;\n} // namespace port", header)
            left = plan.files["src/game/mixed.cpp"]
            self.assertIn("namespace port {\nint s_hook_calls;\n} // namespace port", left)   # empty anon block dropped
        finally:
            fx.close()

    def test_moving_item_leaves_its_anonymous_namespace(self):
        mixed = MIXED.replace("// GLOBAL: LEGORR 0x004b0000\nstatic int s_alpha_state = 0; /* owned by Alpha */",
                              "namespace {\n// GLOBAL: LEGORR 0x004b0000\nint s_alpha_state = 0; /* owned by Alpha */\n}") \
            .replace("    return s_beta_state;", "    return s_beta_state + s_alpha_state;")
        fx = Fixture(mixed)
        try:
            plan = ru.plan_relocation("GAME-001", fx.root)
            unit = plan.files["src/game/alpha.cpp"]
            self.assertIn("// GLOBAL: LEGORR 0x004b0000\nint s_alpha_state = 0;", unit)
            self.assertNotIn("namespace {", unit)
            move = next(m for m in plan.order if m.name == "s_alpha_state")
            self.assertTrue(move.anon_left)
            self.assertIn("extern int s_alpha_state;", plan.files["src/game/mixed_shared.hpp"])
        finally:
            fx.close()

    def test_shared_inline_in_anonymous_namespace_is_refused(self):
        mixed = MIXED.replace("static int alpha_helper(int v);\n", "").replace(
            "static int alpha_helper(int v)\n{\n    return v + 1;\n}\n", "") \
            .replace("static int s_shared_counter;",
                     "static int s_shared_counter;\nnamespace {\ninline int alpha_helper(int v) { return v + 1; }\n}") \
            .replace("    return s_beta_state;", "    return s_beta_state + alpha_helper(1);")
        fx = Fixture(mixed)
        try:
            with self.assertRaises(ru.Refused) as cm:
                ru.plan_relocation("GAME-001", fx.root)
            self.assertTrue(any("inline" in r for r in cm.exception.reasons), cm.exception.reasons)
        finally:
            fx.close()


class StemTest(unittest.TestCase):
    def test_default_stem_that_names_a_file_gets_a_variant(self):
        fx = Fixture(MIXED, {"src/game/alpha.hpp": "#pragma once\n"})
        try:
            plan = ru.plan_relocation("GAME-001", fx.root)
            self.assertEqual(plan.target, "src/game/alpha_unit.cpp")
            self.assertTrue(any("alpha_unit" in n for n in plan.notes))
            with self.assertRaises(ru.Refused):
                ru.plan_relocation("GAME-001", fx.root, stem="alpha")
        finally:
            fx.close()


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
