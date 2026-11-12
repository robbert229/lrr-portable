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

    def test_block_scope_typedef_is_not_a_use_of_the_file_scope_name(self):
        # surface.cpp's font_device_pixel_format_query (#525) declares its own
        # SurfaceLockFn; the winapi region's file-scope one is not used.
        text = textwrap.dedent("""\
            typedef int (*LockFn)(void*);
            typedef struct Desc { int size; } Desc;
            static int query(void* device)
            {
                {
                    typedef int (__stdcall *LockFn)(void*, void*);
                    typedef struct { int a; int b; } Pair;
                    LockFn lock = (LockFn)device;
                    Pair p;
                    return lock(device, &p) + (int)sizeof(Desc);
                }
            }
            """)
        query = next(i for i in cx.scan(text).items if i.names == ["query"])
        self.assertNotIn("LockFn", query.uses)
        self.assertNotIn("Pair", query.uses)
        self.assertIn("Desc", query.uses)

    def test_single_branch_group_around_one_annotated_datum_moves_whole(self):
        # frontend.cpp's flow words (#525): the annotation sits on the
        # declaration, inside the #if, because Reccmp reads the next line.
        text = textwrap.dedent("""\
            #if defined(WINAPI)
            // GLOBAL: LEGORR 0x0055701c
            static int s_flow_quit;
            #endif
            #if defined(WINAPI)
            // GLOBAL: LEGORR 0x00557020
            static int s_a;
            #define A_REF s_a
            #endif
            """)
        items = cx.scan(text).items
        one = next(i for i in items if "s_flow_quit" in i.names)
        self.assertTrue(one.cgroup)
        self.assertEqual(one.problems, [])
        self.assertEqual(one.annotations, [("GLOBAL", 0x0055701c)])
        two = next(i for i in items if "s_a" in i.names)
        self.assertTrue(any("only unannotated groups move whole" in p for p in two.problems), two.problems)

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
        # A single conditional datum is a one-item group: it merges whole (#501).
        only = next(i for i in items if i.names == ["only_x"])
        self.assertTrue(only.cgroup)
        self.assertEqual(only.cond, 0)
        self.assertEqual(only.condition, "#if defined(X)\n")

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
        self.assertIn('#include "mixed_for_alpha.hpp"', unit)
        # Leading comments travel with their item.
        self.assertIn("/* The first function of Alpha.c. */\n// FUNCTION: LEGORR 0x00401000", unit)

    def test_shared_statics_types_and_macros_go_to_the_private_header(self):
        plan = ru.plan_relocation("GAME-001", self.root)
        header = plan.files["src/game/mixed_for_alpha.hpp"]
        self.assertIn("extern int s_shared_counter;", header)
        self.assertNotIn("#define ALPHA_SCALE", header)        # only moved code uses it
        unit = plan.files["src/game/alpha.cpp"]
        self.assertIn("#define ALPHA_SCALE 3", unit)
        self.assertIn("typedef struct AlphaPair", unit)
        mixed = plan.files["src/game/mixed.cpp"]
        self.assertIn("\nint s_shared_counter;", mixed)          # static removed, stays
        self.assertIn('#include "mixed_for_alpha.hpp"', mixed)
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
        header = self.fx.read("src/game/mixed_for_alpha.hpp")
        self.assertEqual(header.count("extern int s_shared_counter;"), 1)
        # mixed.cpp already includes its private header: beta.cpp gets it once.
        self.assertEqual(self.fx.read("src/game/beta.cpp").count('#include "mixed_for_alpha.hpp"'), 1)
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

    def test_linkage_block_inside_a_namespace(self):
        nested = MIXED.replace("// FUNCTION: LEGORR 0x00401000\nint alpha_first(int x)\n{\n"
                               "    s_alpha_state = x;\n    return alpha_second();\n}\n",
                               "namespace outer {\nextern \"C\" {\n// FUNCTION: LEGORR 0x00401000\n"
                               "int alpha_first(int x)\n{\n    return x;\n}\n}\n}\n")
        self.refuse(nested, "nested namespace")

    def test_function_defined_in_a_header(self):
        self.refuse(MIXED, "edit a shared header",
                    {"src/game/inline.hpp": "// FUNCTION: LEGORR 0x00401080\ninline int alpha_missing(void) { return 0; }\n"})

    def test_annotation_not_attached(self):
        self.refuse(MIXED.replace("// FUNCTION: LEGORR 0x00401040\n", "// FUNCTION: LEGORR 0x00401040\n\n"),
                    "not attached")

    def test_shared_annotated_inline(self):
        # An annotated inline function shared by both files cannot go to a
        # header (#514 moves only unannotated ones).
        mixed = MIXED.replace("// FUNCTION: LEGORR 0x00401040\nint alpha_second(void)",
                              "// FUNCTION: LEGORR 0x00401040\ninline int alpha_second(void)") \
            .replace("    return s_beta_state;", "    return s_beta_state + alpha_second();")
        self.refuse(mixed, "inline alpha_second")

    def test_several_declarators_shared(self):
        mixed = MIXED.replace("static int s_shared_counter;", "static int s_shared_counter, s_other;")
        self.refuse(mixed, "--split-declarations")

    def test_macro_redefined(self):
        # An #undef before a later use, or a second definition, refuses; an
        # #undef after the last use does not (ConditionalGroupTest).
        early = MIXED.replace("static int s_shared_counter;", "#undef ALPHA_SCALE\nstatic int s_shared_counter;")
        self.refuse(early, "#undef'd or redefined")
        self.refuse(MIXED + "#define ALPHA_SCALE 4\n", "defined more than once")

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
        header = plan.files["src/game/mixed_for_alpha.hpp"]
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
        header = plan.files["src/game/mixed_for_alpha.hpp"]
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
            header = plan.files["src/game/mixed_for_alpha.hpp"]
            self.assertNotIn("i < 9u", header)
        finally:
            fx.close()

    def test_private_header_goes_after_the_top_include_block(self):
        late = MIXED + '\n#include <stdlib.h>\n\nint late_user(void) { return s_shared_counter; }\n'
        fx = Fixture(late)
        try:
            plan = ru.plan_relocation("GAME-001", fx.root)
            mixed = plan.files["src/game/mixed.cpp"]
            self.assertLess(mixed.index('#include "mixed_for_alpha.hpp"'), mixed.index("int s_shared_counter;"))
            self.assertLess(mixed.index('#include "mixed_for_alpha.hpp"'), mixed.index("#include <stdlib.h>"))
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
            self.assertLess(mixed.index("#include <stddef.h>"), mixed.index('#include "mixed_for_alpha.hpp"'))
            self.assertLess(mixed.index('#include "mixed_for_alpha.hpp"'), mixed.index("int s_shared_counter;"))
        finally:
            fx.close()

    def test_include_only_if_group_is_copied_in_place(self):
        # G98-014's resource.cpp (#525): a platform include group the moved
        # code needs is copied verbatim, in order; a mixed group is not.
        grouped = MIXED.replace('#include <string.h>\n',
                                '#include <string.h>\n#if defined(_WIN32)\n#include <windows.h>\n'
                                '#else\n#include <dirent.h>\n#endif\n'
                                '#if defined(PORT_EXTRA)\n#include <stddef.h>\n#define PORT_EXTRA_ON 1\n#endif\n')
        fx = Fixture(grouped)
        try:
            plan = ru.plan_relocation("GAME-001", fx.root)
            unit = plan.files["src/game/alpha.cpp"]
            block = "#if defined(_WIN32)\n#include <windows.h>\n#else\n#include <dirent.h>\n#endif\n"
            self.assertEqual(unit.count(block), 1)
            self.assertLess(unit.index("#include <string.h>"), unit.index(block))
            self.assertLess(unit.index(block), unit.index('#include "mixed.hpp"'))
            self.assertNotIn("<stddef.h>", unit)
        finally:
            fx.close()

    def test_shared_unbounded_array_is_declared_with_its_bound(self):
        # GAME-020's s_interface_menu_action_labels (#525): a moved user takes
        # sizeof, so ``extern T a[];`` would be an incomplete type there.
        labels = MIXED.replace("static int s_shared_counter;",
                               'static int s_shared_counter;\nstatic const char* const s_labels[] = {\n'
                               '    "a,b", /* one, */\n    "c",\n};') \
            .replace("    s_shared_counter++;\n", "    s_shared_counter += (int)sizeof(s_labels[0][0]);\n") \
            .replace("    s_shared_counter += p.a;\n", "    s_shared_counter += p.a + (int)sizeof(s_labels);\n")
        fx = Fixture(labels)
        try:
            header = ru.plan_relocation("GAME-001", fx.root).files["src/game/mixed_for_alpha.hpp"]
            self.assertIn("extern const char* const s_labels[2];", header)
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
            # alpha_second moved and beta_one (staying) calls it: declared for mixed.cpp only.
            self.assertIn("int alpha_second(void);", plan.files["src/game/mixed_from_alpha.hpp"])
            self.assertNotIn("mixed_from_alpha.hpp", unit)
        finally:
            fx.close()


class PerRelocationHeaderTest(unittest.TestCase):
    """#496: each relocation writes its own frozen pair of private headers."""

    def tree(self, root):
        return {p.relative_to(root).as_posix(): p.read_text() for p in root.rglob("*") if p.is_file()}

    def test_second_relocation_leaves_the_first_unit_file_untouched(self):
        mixed = MIXED.replace("    return s_beta_state;", "    return s_beta_state + s_shared_counter;")
        fx = Fixture(mixed)
        try:
            ru.write(ru.plan_relocation("GAME-001", fx.root), fx.root)
            first = self.tree(fx.root)
            alpha_includes = [line for line in first["src/game/alpha.cpp"].splitlines() if line.startswith("#include")]
            self.assertIn('#include "mixed_for_alpha.hpp"', alpha_includes)
            ru.write(ru.plan_relocation("GAME-002", fx.root), fx.root)
            second = self.tree(fx.root)
            # The first unit file and every private header it includes are byte-identical.
            for rel in ("src/game/alpha.cpp", "src/game/mixed_for_alpha.hpp"):
                self.assertEqual(first[rel], second[rel], rel)
            self.assertNotIn("src/game/mixed_shared.hpp", second)
            # s_shared_counter already has external linkage and alpha's frozen
            # header declares it: beta.cpp reuses that header (it uses the name)
            # and nothing is appended to it.
            self.assertIn('#include "mixed_for_alpha.hpp"', second["src/game/beta.cpp"])
            self.assertNotIn("src/game/mixed_for_beta.hpp", second)
            self.assertIn('#include "mixed_for_alpha.hpp"', second["src/game/mixed.cpp"])
        finally:
            fx.close()

    def test_unit_never_sees_declarations_of_its_own_definitions(self):
        # s_alpha_state moves to alpha.cpp and beta_one (staying) uses it:
        # its extern goes to mixed_from_alpha.hpp, which alpha.cpp does not include.
        mixed = MIXED.replace("    return s_beta_state;", "    return s_beta_state + s_alpha_state;")
        fx = Fixture(mixed)
        try:
            plan = ru.plan_relocation("GAME-001", fx.root)
            self.assertIn("extern int s_alpha_state;", plan.files["src/game/mixed_from_alpha.hpp"])
            self.assertNotIn("mixed_from_alpha.hpp", plan.files["src/game/alpha.cpp"])
            self.assertIn('#include "mixed_from_alpha.hpp"', plan.files["src/game/mixed.cpp"])
        finally:
            fx.close()

    def test_const_object_definition_sees_its_extern(self):
        # A const object needs its extern first for external linkage.
        mixed = MIXED.replace("// GLOBAL: LEGORR 0x004b0000\nstatic int s_alpha_state = 0; /* owned by Alpha */",
                              "// GLOBAL: LEGORR 0x004b0000\nstatic const int s_alpha_state = 0;") \
            .replace("    s_alpha_state = x;\n", "") \
            .replace("    return s_beta_state;", "    return s_beta_state + s_alpha_state;")
        fx = Fixture(mixed)
        try:
            plan = ru.plan_relocation("GAME-001", fx.root)
            self.assertIn('#include "mixed_from_alpha.hpp"', plan.files["src/game/alpha.cpp"])
        finally:
            fx.close()

    def test_const_object_keeps_an_earlier_private_header_with_its_extern(self):
        # GAME-017's s_map_open_binary_mode (#525): an earlier relocation's
        # frozen header of the legacy file declares the const object for its
        # own unit file. The moved definition needs that extern first.
        earlier = ("// SPDX-License-Identifier: LGPL-3.0-or-later\n#ifndef MIXED_FOR_GAMMA_HPP\n"
                   "#define MIXED_FOR_GAMMA_HPP\nextern const char s_alpha_mode[];\n#endif\n")
        mixed = MIXED.replace('#include "mixed.hpp"\n', '#include "mixed.hpp"\n#include "mixed_for_gamma.hpp"\n') \
            .replace("// GLOBAL: LEGORR 0x004b0000\nstatic int s_alpha_state = 0; /* owned by Alpha */",
                     '// GLOBAL: LEGORR 0x004b0000\nconst char s_alpha_mode[] = "rb";\n'
                     "static int s_alpha_state = 0; /* owned by Alpha */")
        fx = Fixture(mixed, {"src/game/mixed_for_gamma.hpp": earlier})
        try:
            unit = ru.plan_relocation("GAME-001", fx.root).files["src/game/alpha.cpp"]
            self.assertIn('const char s_alpha_mode[] = "rb";', unit)
            self.assertLess(unit.index('#include "mixed_for_gamma.hpp"'), unit.index("s_alpha_mode"))
        finally:
            fx.close()

    def test_function_declared_only_in_the_from_header_gets_a_forward_declaration(self):
        # alpha_second moves (annotated) and beta_one, which stays, calls it;
        # no public header declares it, so its declaration goes to
        # mixed_from_alpha.hpp, which alpha.cpp does not include. alpha_first
        # calls it before its definition: alpha.cpp needs a forward declaration.
        mixed = MIXED.replace("    return s_beta_state;", "    return s_beta_state + alpha_second();")
        header = HEADER.replace("int alpha_second(void);\n", "")
        fx = Fixture(mixed, {"src/game/mixed.hpp": header})
        try:
            plan = ru.plan_relocation("GAME-001", fx.root)
            self.assertIn("int alpha_second(void);", plan.files["src/game/mixed_from_alpha.hpp"])
            self.assertIn("int alpha_second(void);", plan.forward)
            self.assertNotIn("mixed_from_alpha.hpp", plan.files["src/game/alpha.cpp"])
        finally:
            fx.close()

    def test_frozen_header_is_never_rewritten(self):
        fx = Fixture(MIXED)
        try:
            (fx.root / "src/game/mixed_for_alpha.hpp").write_text("// someone else's\n")
            with self.assertRaises(ru.Refused) as cm:
                ru.plan_relocation("GAME-001", fx.root)
            self.assertIn("frozen", " ".join(cm.exception.reasons))
        finally:
            fx.close()

    def test_is_private_header(self):
        fx = Fixture(MIXED)
        try:
            for name, expected in (("mixed_shared.hpp", True), ("mixed_for_alpha.hpp", True),
                                   ("mixed_from_alpha.hpp", True), ("mixed.hpp", False), ("other_for_x.hpp", True),
                                   ("nolegacy_for_x.hpp", False)):
                self.assertEqual(ru.is_private_header(fx.root, f"src/game/{name}"), expected, name)
        finally:
            fx.close()


class PerRelocationHeaderSecondUnitTest(unittest.TestCase):
    def test_second_relocation_writes_its_own_pair(self):
        # A static used by beta's code and by code that stays: the second
        # relocation writes mixed_for_beta.hpp; alpha's header is untouched.
        mixed = MIXED.replace("static int s_shared_counter;", "static int s_shared_counter;\nstatic int s_beta_only;") \
            .replace("    return s_beta_state;", "    return s_beta_state + s_beta_only;") \
            + "\nint keep_beta_only(void)\n{\n    return s_beta_only;\n}\n"
        fx = Fixture(mixed)
        try:
            ru.write(ru.plan_relocation("GAME-001", fx.root), fx.root)
            alpha = (fx.root / "src/game/alpha.cpp").read_text()
            alpha_header = (fx.root / "src/game/mixed_for_alpha.hpp").read_text()
            ru.write(ru.plan_relocation("GAME-002", fx.root), fx.root)
            self.assertEqual(alpha, (fx.root / "src/game/alpha.cpp").read_text())
            self.assertEqual(alpha_header, (fx.root / "src/game/mixed_for_alpha.hpp").read_text())
            beta_header = (fx.root / "src/game/mixed_for_beta.hpp").read_text()
            self.assertIn("extern int s_beta_only;", beta_header)
            self.assertNotIn("s_beta_only", alpha_header)
            self.assertNotIn("mixed_for_beta.hpp", alpha)
            left = (fx.root / "src/game/mixed.cpp").read_text()
            self.assertIn('#include "mixed_for_alpha.hpp"', left)
            self.assertIn('#include "mixed_for_beta.hpp"', left)
        finally:
            fx.close()


class MigrationTest(unittest.TestCase):
    """#496: the pre-#496 per-source headers become per-unit snapshots."""

    OLD_HEADER = ("// SPDX-License-Identifier: LGPL-3.0-or-later\n/* old */\n#ifndef LRR_GAME_MIXED_SHARED_HPP\n"
                  "#define LRR_GAME_MIXED_SHARED_HPP\n\n#define MIXED_ROWS 4\nextern int s_rows[MIXED_ROWS];\n"
                  "namespace port {\nextern int s_port_word;\n} // namespace port\n\n#endif /* LRR_GAME_MIXED_SHARED_HPP */\n")

    def fixture(self):
        unit = '// SPDX\n#include "mixed.hpp"\n#include "mixed_shared.hpp"\n\n// FUNCTION: LEGORR 0x{0}\nint {1}(void)\n{{\n    return s_rows[0];\n}}\n'
        extra = {
            "src/game/mixed_shared.hpp": self.OLD_HEADER,
            "src/game/mixed.cpp": '// SPDX\n#include "mixed.hpp"\n#include "mixed_shared.hpp"\n\nint s_rows[MIXED_ROWS];\n'
                                  '// FUNCTION: LEGORR 0x00401100\nint beta_one(void)\n{\n    return s_rows[1];\n}\n',
            "src/game/alpha.cpp": unit.format("00401000", "alpha_first"),
            "src/game/gamma.cpp": unit.format("00402000", "gamma_one").replace(
                '#include "mixed_shared.hpp"\n', '#include "mixed_shared.hpp"\n#include "game/mixed_shared.hpp"\n'),
            "src/link_order.cmake": LIST.replace("lrr_link(legacy 00401040 game          game/mixed.cpp)",
                                                 "lrr_link(unit   GAME-001 game          game/alpha.cpp)\n"
                                                 "lrr_link(legacy 00401100 game          game/mixed.cpp)").replace(
                "lrr_link(legacy 00402000 game          game/other.cpp)", "lrr_link(unit   GAME-003 game          game/gamma.cpp)"),
        }
        fx = Fixture(MIXED, extra)
        (fx.root / "src/game/other.cpp").unlink()
        self.addCleanup(fx.close)
        return fx

    def test_each_unit_gets_a_verbatim_snapshot(self):
        fx = self.fixture()
        mig = ru.plan_header_migration(fx.root)
        self.assertEqual(mig.reasons, [])
        self.assertEqual(sorted(k for k in mig.files if k.endswith(".hpp")),
                         ["src/game/mixed_for_alpha.hpp", "src/game/mixed_for_gamma.hpp"])
        body = self.OLD_HEADER.split("#define LRR_GAME_MIXED_SHARED_HPP\n", 1)[1].rsplit("#endif", 1)[0]
        for rel in ("src/game/mixed_for_alpha.hpp", "src/game/mixed_for_gamma.hpp"):
            self.assertIn(body, mig.files[rel])
        self.assertIn('#include "mixed_for_alpha.hpp"\n', mig.files["src/game/alpha.cpp"])
        self.assertNotIn("mixed_shared.hpp", mig.files["src/game/alpha.cpp"])
        # A duplicate include of the old header is kept, pointing at the
        # snapshot: it still costs MSVC a symbol-counter step (finding 538).
        gamma = mig.files["src/game/gamma.cpp"]
        self.assertEqual(gamma.count("#include"), 3)
        # Each include keeps its spelling; only the file name changes.
        self.assertEqual(gamma.count('#include "mixed_for_gamma.hpp"\n'), 1)
        self.assertEqual(gamma.count('#include "game/mixed_for_gamma.hpp"\n'), 1)
        # The legacy file and the old header do not change.
        self.assertNotIn("src/game/mixed.cpp", mig.files)
        self.assertNotIn("src/game/mixed_shared.hpp", mig.files)
        rows = list(csv.DictReader(io.StringIO(mig.files["docs/coverage/relocations/private-header-migration.csv"])))
        self.assertEqual([(r["unit"], r["includes_replaced"]) for r in rows],
                         [("src/game/alpha.cpp", "1"), ("src/game/gamma.cpp", "2")])

    def test_migration_cli_writes_and_is_then_a_no_op(self):
        fx = self.fixture()
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(ru.main(["--root", str(fx.root), "--migrate-private-headers"]), 0)
        self.assertTrue((fx.root / "src/game/mixed_for_alpha.hpp").exists())
        again = ru.plan_header_migration(fx.root)
        self.assertEqual([k for k in again.files if not k.endswith(".csv")], [])

    def test_header_included_elsewhere_is_refused(self):
        fx = self.fixture()
        (fx.root / "src/game/stray.cpp").write_text('#include "mixed_shared.hpp"\nint stray;\n')
        self.assertTrue(any("stray.cpp" in r for r in ru.plan_header_migration(fx.root).reasons))


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
        self.assertIn("extern int s_shared_counter;", relocation.files["src/game/mixed_for_alpha.hpp"])

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
        header = relocation.files["src/game/mixed_for_alpha.hpp"]
        self.assertIn("struct Counter {", header)                  # the type, shared
        self.assertIn("extern struct Counter s_counter_record;", header)

    def test_type_and_variable_split_moves_the_annotation_onto_the_variable(self):
        # GAME-020's s_handle_set_band (#525): the GLOBAL names the variable.
        mixed = MIXED.replace("static int s_shared_counter;",
                              "/* The band. */\n// GLOBAL: LEGORR 0x004b0008\nstatic struct Counter {\n"
                              "    struct { int x; } inner;\n    int value;\n} s_counter_record; /* tail */\n"
                              "static int s_shared_counter;") \
            .replace("    s_shared_counter++;\n", "    s_shared_counter++;\n    s_counter_record.value++;\n") \
            .replace("    s_shared_counter += p.a;\n", "    s_shared_counter += p.a + s_counter_record.value;\n")
        fx = self.split_fixture(mixed)
        plan = ru.plan_relocation("GAME-001", fx.root, split=True)
        self.assertIn("/* The band. */\nstruct Counter {\n    struct { int x; } inner;\n    int value;\n};\n"
                      "// GLOBAL: LEGORR 0x004b0008\nstatic struct Counter s_counter_record; /* tail */\n",
                      plan.files["src/game/mixed.cpp"])

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
            self.assertIn("extern int s_hook_calls;", plan.files["src/game/mixed_for_alpha.hpp"])
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
            header = plan.files["src/game/mixed_for_alpha.hpp"]
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
            self.assertIn("extern int s_alpha_state;", plan.files["src/game/mixed_from_alpha.hpp"])
        finally:
            fx.close()

    def test_shared_inline_in_anonymous_namespace_goes_to_the_header(self):
        mixed = MIXED.replace("static int alpha_helper(int v);\n", "").replace(
            "static int alpha_helper(int v)\n{\n    return v + 1;\n}\n", "") \
            .replace("static int s_shared_counter;",
                     "static int s_shared_counter;\nnamespace {\ninline int alpha_helper(int v) { return v + 1; }\n}") \
            .replace("    return s_beta_state;", "    return s_beta_state + alpha_helper(1);")
        fx = Fixture(mixed)
        try:
            # #514: an unannotated inline function both files use goes to the
            # for header, verbatim; it leaves its anonymous namespace (an inline
            # function may be defined in every translation unit).
            plan = ru.plan_relocation("GAME-001", fx.root)
            header = plan.files["src/game/mixed_for_alpha.hpp"]
            self.assertIn("inline int alpha_helper(int v) { return v + 1; }", header)
            self.assertNotIn("alpha_helper(int v) {", plan.files["src/game/mixed.cpp"])
            self.assertNotIn("alpha_helper(int v) {", plan.files["src/game/alpha.cpp"])
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


    def test_survey_plans_the_relocation_behind_a_split(self):
        # #514 (GAME-021, G98-009): a unit that needs --split-declarations is
        # "ok after" only if the relocation itself plans on the split text.
        mixed = MIXED.replace("static int s_shared_counter;", "static int s_shared_counter, s_other;")
        for frozen, expected in ((None, "ok after --split-declarations"),
                                 ("// someone else's\n", "refused after --split-declarations")):
            extra = {"src/game/mixed_for_alpha.hpp": frozen} if frozen else None
            fx = Fixture(mixed, extra)
            try:
                result = next(r for r in ru.survey(fx.root) if r["unit"] == "GAME-001")
                self.assertEqual(result["status"], expected)
                if frozen:
                    self.assertTrue(any("frozen" in r for r in result["reasons"]), result["reasons"])
                    self.assertFalse(any(fx.dir.name in r for r in result["reasons"]))
                # The survey never writes the real tree.
                self.assertEqual(fx.read("src/game/mixed.cpp"), mixed)
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


ALTERNATIVES = """#if defined(_MSC_VER)
#define ALPHA_SCALE 3
#elif defined(__GNUC__)
#define ALPHA_SCALE (2 + 1)
#else
#define ALPHA_SCALE 3
#endif
"""


class ConditionalGroupTest(unittest.TestCase):
    """#501 (T1, T2): whole conditional groups, guarded declarations, statics
    a header item needs."""

    def plan(self, mixed: str, unit: str = "GAME-001", extra: dict | None = None):
        fx = Fixture(mixed, extra)
        self.addCleanup(fx.close)
        return fx, ru.plan_relocation(unit, fx.root)

    def refused(self, mixed: str) -> list[str]:
        fx = Fixture(mixed)
        self.addCleanup(fx.close)
        with self.assertRaises(ru.Refused) as cm:
            ru.plan_relocation("GAME-001", fx.root)
        return cm.exception.reasons

    # -- the scanner

    def test_macro_alternatives_merge_into_one_group(self):
        items = cx.scan(ALTERNATIVES + "int use = ALPHA_SCALE;\n").items
        group = items[0]
        self.assertTrue(group.cgroup)
        self.assertEqual((group.category, group.names, group.branches), ("macro", ["ALPHA_SCALE"], 3))
        self.assertEqual([p.branch for p in group.parts], [0, 1, 2])
        self.assertEqual(group.condition, "#if defined(_MSC_VER)\n")
        self.assertEqual(group.code(ALTERNATIVES), ALTERNATIVES)
        self.assertIn("_MSC_VER", group.uses)

    def test_groups_that_do_not_merge_say_why(self):
        unrelated = "#if A\nint a;\nint b;\n#endif\n"
        with_function = "#if A\nint a;\n// FUNCTION: LEGORR 0x00401000\nint f(void) { return a; }\n#endif\n"
        guard = "#ifndef H\n#define H\nint a;\n#endif\n"
        nested = "#if A\n#if B\nint a;\n#endif\n#endif\n"
        for text, why in ((unrelated, "2 unrelated clusters"), (with_function, "annotated function definition f"),
                          (guard, "include guard"), (nested, "nested #if")):
            items = cx.scan(text).items
            self.assertFalse(any(i.cgroup for i in items), text)
            self.assertTrue(any(why in i.group_why for i in items), (text, [i.group_why for i in items]))

    def test_two_macros_per_branch_merge(self):
        # P2 with two macros (GAME-043's ENTITY_STATS_*_READ): header-safe items
        # of one group belong together even when they do not use each other.
        text = ("#if defined(V)\n#define READ_F(r) f_v(r)\n#define READ_W(r) w_v(r)\n#else\n"
                "#define READ_F(r) f_c(r)\n#define READ_W(r) w_c(r)\n#endif\n")
        (group,) = cx.scan(text).items
        self.assertTrue(group.cgroup)
        self.assertEqual(group.names, ["READ_F", "READ_W"])
        # Data unrelated to the macros does not merge.
        items = cx.scan("#if defined(V)\n#define READ_F 1\nint unrelated;\n#endif\n").items
        self.assertFalse(any(i.cgroup for i in items))

    def test_includes_come_with_their_group(self):
        text = ("#if defined(W)\n#include <direct.h>\n#define MKDIR(p) _mkdir(p)\n#else\n"
                "#include <sys/stat.h>\n#define MKDIR(p) mkdir(p, 0777)\n#endif\n")
        (group,) = cx.scan(text).items
        self.assertTrue(group.cgroup)
        self.assertEqual([p.category for p in group.parts], ["include", "macro", "include", "macro"])

    # -- (1) whole groups move verbatim

    def test_group_used_only_by_moved_code_moves_whole_to_the_unit(self):
        fx, plan = self.plan(MIXED.replace("#define ALPHA_SCALE 3\n", ALTERNATIVES))
        self.assertIn(ALTERNATIVES, plan.files["src/game/alpha.cpp"])
        self.assertNotIn("ALPHA_SCALE", plan.files["src/game/mixed.cpp"])
        # The hash check covers the group's whole text.
        rows = list(csv.DictReader(io.StringIO(plan.files["docs/coverage/relocations/GAME-001.csv"])))
        row = next(r for r in rows if r["name"] == "ALPHA_SCALE")
        self.assertEqual(row["before_sha256"], ru.sha(ALTERNATIVES))
        self.assertEqual(row["after_sha256"], row["before_sha256"])

    def test_group_both_sides_use_goes_whole_to_the_for_header(self):
        mixed = MIXED.replace("#define ALPHA_SCALE 3\n", ALTERNATIVES) \
            .replace("    return s_beta_state;", "    return s_beta_state * ALPHA_SCALE;")
        fx, plan = self.plan(mixed)
        header = plan.files["src/game/mixed_for_alpha.hpp"]
        self.assertIn(ALTERNATIVES, header)
        legacy = plan.files["src/game/mixed.cpp"]
        self.assertLess(legacy.index('#include "mixed_for_alpha.hpp"'), legacy.index("ALPHA_SCALE"))

    def test_group_with_includes_moves_whole(self):
        group = ("#if defined(W)\n#include <direct.h>\n#define ALPHA_SCALE 3\n#else\n"
                 "#include <sys/stat.h>\n#define ALPHA_SCALE 3\n#endif\n")
        fx, plan = self.plan(MIXED.replace("#define ALPHA_SCALE 3\n", group))
        self.assertIn(group, plan.files["src/game/alpha.cpp"])
        self.assertNotIn("direct.h", plan.files["src/game/mixed.cpp"])

    # -- (2) the redefined-macro check

    def test_undef_after_the_last_use_is_accepted_and_reported(self):
        mixed = MIXED.replace("#define ALPHA_SCALE 3\n", ALTERNATIVES) + "#undef ALPHA_SCALE\n"
        fx, plan = self.plan(mixed)
        self.assertTrue(plan.files["src/game/mixed.cpp"].rstrip().endswith("#undef ALPHA_SCALE"))
        self.assertEqual(sum("#undef ALPHA_SCALE after its last use stays in place" in n for n in plan.notes), 1,
                         plan.notes)

    def test_two_definitions_in_one_branch_refuse(self):
        twice = ALTERNATIVES.replace("#define ALPHA_SCALE (2 + 1)\n",
                                     "#define ALPHA_SCALE (2 + 1)\n#define ALPHA_SCALE 4\n")
        reasons = self.refused(MIXED.replace("#define ALPHA_SCALE 3\n", twice))
        self.assertTrue(any("defined twice in one branch" in r for r in reasons), reasons)

    # -- (3) guarded private-header declarations

    def guarded(self, group: str) -> str:
        return MIXED.replace("static int s_shared_counter;", group + "static int s_shared_counter;") \
            .replace("    s_shared_counter += p.a;\n",
                     "    s_shared_counter += p.a;\n#if defined(FEATURE)\n    s_cond++;\n#endif\n") \
            .replace("    return s_beta_state;", "#if defined(FEATURE)\n    s_cond--;\n#endif\n    return s_beta_state;")

    def test_shared_static_in_a_single_branch_group_gets_a_guarded_declaration(self):
        group = "#if defined(FEATURE) /* port only */\nstatic int s_cond;\n#endif /* FEATURE */\n"
        fx, plan = self.plan(self.guarded(group))
        header = plan.files["src/game/mixed_for_alpha.hpp"]
        self.assertIn("#if defined(FEATURE) /* port only */\nextern int s_cond;\n#endif /* FEATURE */\n", header)
        legacy = plan.files["src/game/mixed.cpp"]
        self.assertIn("#if defined(FEATURE) /* port only */\nint s_cond;\n#endif /* FEATURE */\n", legacy)
        rows = list(csv.DictReader(io.StringIO(plan.files["docs/coverage/relocations/GAME-001.csv"])))
        guarded = [r for r in rows if r["role"] == "guarded-declaration"]
        self.assertEqual([r["name"] for r in guarded], ["s_cond"])
        self.assertEqual(guarded[0]["to"], "src/game/mixed_for_alpha.hpp")
        self.assertIn("#if defined(FEATURE) /* port only */", guarded[0]["edit"])
        self.assertTrue(any(r["role"] == "stays-var" and "static removed" in r["edit"] for r in rows), rows)

    def test_first_branch_static_gets_a_guarded_declaration(self):
        group = "#if defined(FEATURE)\nstatic int s_cond;\n#else\n#define S_COND_ABSENT 1\n#endif\n"
        group = group.replace("#define S_COND_ABSENT 1", "#define s_cond_absent s_cond")
        fx, plan = self.plan(self.guarded(group))
        self.assertIn("#if defined(FEATURE)\nextern int s_cond;\n#endif\n",
                      plan.files["src/game/mixed_for_alpha.hpp"])

    def test_macro_group_sharing_a_name_with_a_data_group(self):
        # frontend.cpp (#525): a winapi-only `#define s_cond …` and a
        # non-winapi `static int s_cond;` share a name and so a cluster; the
        # data group still gets its guarded declaration.
        group = ("#if defined(OTHER)\n#define s_cond (s_shared_counter + 1)\n#endif\n"
                 "#if defined(FEATURE)\nstatic int s_cond;\n#endif\n")
        fx, plan = self.plan(self.guarded(group))
        header = plan.files["src/game/mixed_for_alpha.hpp"]
        self.assertIn("#if defined(FEATURE)\nextern int s_cond;\n#endif\n", header)

    def test_static_outside_the_first_branch_refuses(self):
        group = "#if defined(OTHER)\n#define s_cond_alias 1\n#else\nstatic int s_cond;\n#endif\n"
        group = group.replace("#define s_cond_alias 1", "#define s_cond_alias s_cond")
        reasons = self.refused(self.guarded(group))
        self.assertTrue(any("branch 2 of the #if group" in r and "synthesised" in r for r in reasons), reasons)

    # -- (4) anything else refuses, naming the group

    def test_unmergeable_group_is_named(self):
        group = ("#if defined(FEATURE)\nstatic int s_cond;\n#if defined(MORE)\nstatic int s_more;\n#endif\n"
                 "#endif\n")
        reasons = self.refused(self.guarded(group))
        line = self.guarded(group).splitlines().index("#if defined(FEATURE)") + 1
        self.assertTrue(any(f"the group at line {line} cannot move whole" in r and "nested #if" in r
                            for r in reasons), reasons)

    # -- T2: a static that a private-header item needs is exported

    def test_static_a_header_macro_needs_is_exported(self):
        mixed = MIXED.replace("#define ALPHA_SCALE 3\n", "static int s_scale_base = 3;\n#define ALPHA_SCALE (s_scale_base + 1)\n") \
            .replace("    return s_beta_state;", "    return s_beta_state * ALPHA_SCALE;")
        fx, plan = self.plan(mixed)
        header = plan.files["src/game/mixed_for_alpha.hpp"]
        self.assertIn("#define ALPHA_SCALE (s_scale_base + 1)", header)
        self.assertIn("extern int s_scale_base;", header)
        self.assertIn("\nint s_scale_base = 3;", plan.files["src/game/mixed.cpp"])
        rows = list(csv.DictReader(io.StringIO(plan.files["docs/coverage/relocations/GAME-001.csv"])))
        self.assertTrue(any(r["name"] == "s_scale_base" and "static removed" in r["edit"] for r in rows), rows)

    def test_inline_a_header_macro_needs_goes_to_the_header(self):
        # #514: an inline function is header-safe; it follows its macro.
        mixed = MIXED.replace("#define ALPHA_SCALE 3\n",
                              "static inline int scale_base(void) { return 3; }\n#define ALPHA_SCALE (scale_base() + 1)\n") \
            .replace("    return s_beta_state;", "    return s_beta_state * ALPHA_SCALE;")
        fx, plan = self.plan(mixed)
        header = plan.files["src/game/mixed_for_alpha.hpp"]
        self.assertIn("static inline int scale_base(void) { return 3; }", header)
        self.assertIn("#define ALPHA_SCALE (scale_base() + 1)", header)

    def test_anonymous_static_a_header_needs_still_refuses(self):
        mixed = MIXED.replace("#define ALPHA_SCALE 3\n",
                              "namespace {\nint s_scale_base = 3;\n}\n#define ALPHA_SCALE (s_scale_base + 1)\n") \
            .replace("    return s_beta_state;", "    return s_beta_state * ALPHA_SCALE;")
        reasons = self.refused(mixed)
        self.assertTrue(any("a static that is not shared" in r for r in reasons), reasons)

    def test_pinned_external_used_by_moved_code_is_declared(self):
        # beta_one carries GAME-002's annotation (pinned) and stays; alpha_second
        # moves and calls it; no header declares it: the for header must.
        mixed = MIXED.replace("    AlphaPair p = { 1, 2 };\n", "    AlphaPair p = { 1, 2 };\n    p.b += beta_one();\n")
        header = HEADER.replace("int beta_one(void);\n", "")
        fx, plan = self.plan(mixed, extra={"src/game/mixed.hpp": header})
        self.assertIn("int beta_one(void);", plan.files["src/game/mixed_for_alpha.hpp"])

    def test_shared_group_mixing_data_and_macros_refuses(self):
        # The moved code uses the group's macro, but only the datum's
        # declaration reaches the unit: refuse.
        group = "#if defined(FEATURE)\nstatic int s_cond;\n#define COND_NEXT (s_cond + 1)\n#endif\n"
        mixed = self.guarded(group).replace("    s_cond++;\n", "    s_cond = COND_NEXT;\n")
        reasons = self.refused(mixed)
        self.assertTrue(any("split across the files" in r and "COND_NEXT" in r for r in reasons), reasons)
        # Using only the datum is fine.
        fx, plan = self.plan(self.guarded(group))
        self.assertIn("extern int s_cond;", plan.files["src/game/mixed_for_alpha.hpp"])


class RemainingRefusalsTest(unittest.TestCase):
    """#514: nested namespaces, out-of-line class members, anonymous-namespace
    forward declarations, helper functions in whole groups, name clashes."""

    def plan(self, mixed: str, unit: str = "GAME-001", extra: dict | None = None):
        fx = Fixture(mixed, extra)
        self.addCleanup(fx.close)
        return ru.plan_relocation(unit, fx.root)

    def refused(self, mixed: str, extra: dict | None = None) -> list[str]:
        fx = Fixture(mixed, extra)
        self.addCleanup(fx.close)
        with self.assertRaises(ru.Refused) as cm:
            ru.plan_relocation("GAME-001", fx.root)
        return cm.exception.reasons

    def test_nested_named_namespaces_move_verbatim(self):
        nested = MIXED.replace("// FUNCTION: LEGORR 0x00401000\nint alpha_first(int x)\n{\n"
                               "    s_alpha_state = x;\n    return alpha_second();\n}\n",
                               "namespace outer {\nnamespace inner {\n// FUNCTION: LEGORR 0x00401000\n"
                               "int alpha_first(int x)\n{\n    return x;\n}\n}\n}\n")
        plan = self.plan(nested)
        unit = plan.files["src/game/alpha.cpp"]
        self.assertIn("namespace outer {\nnamespace inner {\n// FUNCTION: LEGORR 0x00401000\nint alpha_first", unit)
        self.assertIn("} // namespace inner\n} // namespace outer", unit)
        self.assertNotIn("namespace outer::inner", unit)     # MSVC 5.0 has no nested namespace definitions
        self.assertEqual(cx.scan(unit).problems, [])

    def test_nested_namespace_declarations_nest(self):
        # A staying member of outer::inner that moved code uses gets its
        # declaration in nested blocks, never `namespace outer::inner`.
        nested = MIXED.replace("static int alpha_helper(int v)\n{\n    return v + 1;\n}\n",
                               "namespace outer {\nnamespace inner {\nint deep(int v)\n{\n    return v + 1;\n}\n}\n}\n") \
            .replace("    return s_beta_state;", "    return s_beta_state + outer::inner::deep(1);") \
            .replace("    return alpha_helper(s_alpha_state) * ALPHA_SCALE;",
                     "    return outer::inner::deep(s_alpha_state) * ALPHA_SCALE;") \
            .replace("static int alpha_helper(int v);\n", "")
        plan = self.plan(nested)
        header = plan.files["src/game/mixed_for_alpha.hpp"]
        self.assertIn("namespace outer {\nnamespace inner {\nint deep(int v);\n} // namespace inner\n"
                      "} // namespace outer", header)

    def test_out_of_line_class_member_moves_with_its_class(self):
        member = MIXED.replace("static int s_shared_counter;",
                               "class Obj {\npublic:\n    int alpha_first(int x);\n};\nstatic int s_shared_counter;") \
            .replace("// FUNCTION: LEGORR 0x00401000\nint alpha_first(int x)",
                     "// FUNCTION: LEGORR 0x00401000\nint Obj::alpha_first(int x)")
        plan = self.plan(member)
        unit = plan.files["src/game/alpha.cpp"]
        self.assertIn("int Obj::alpha_first(int x)", unit)
        self.assertIn("class Obj {", unit)                       # only moved code uses it
        for path, text in plan.files.items():
            if path.endswith(".hpp"):
                self.assertNotRegex(text, r"\bint alpha_first\(int x\);", path)  # the class declares it

    def test_shared_class_goes_to_the_header(self):
        member = MIXED.replace("static int s_shared_counter;",
                               "class Obj {\npublic:\n    int alpha_first(int x);\n};\nstatic int s_shared_counter;") \
            .replace("// FUNCTION: LEGORR 0x00401000\nint alpha_first(int x)",
                     "// FUNCTION: LEGORR 0x00401000\nint Obj::alpha_first(int x)") \
            .replace("    return s_beta_state;", "    Obj o;\n    return s_beta_state + o.alpha_first(1);")
        plan = self.plan(member)
        self.assertIn("class Obj {", plan.files["src/game/mixed_for_alpha.hpp"])
        self.assertNotIn("class Obj {", plan.files["src/game/mixed.cpp"])

    def test_anonymous_namespace_function_used_before_its_definition(self):
        anon = MIXED.replace("static int alpha_helper(int v);\n", "") \
            .replace("static int alpha_helper(int v)\n{\n    return v + 1;\n}\n",
                     "namespace {\nint alpha_helper(int v)\n{\n    return v + 1;\n}\n}\n")
        plan = self.plan(anon)
        self.assertIn("namespace { int alpha_helper(int v); }", plan.forward)
        unit = plan.files["src/game/alpha.cpp"]
        self.assertLess(unit.index("namespace { int alpha_helper(int v); }"), unit.index("int alpha_second(void)"))

    def test_helper_functions_in_a_whole_group(self):
        # G98-009's shape: alternative static helpers, with typedefs in the
        # second branch, used only by moved code. The group moves whole and
        # lands before the unit's functions (no forward declarations).
        group = ("#if defined(_MSC_VER)\nstatic int alpha_helper(int v)\n{\n    return v + 1;\n}\n"
                 "#elif defined(__GNUC__)\ntypedef int HelperInt;\nstatic int alpha_helper(int v)\n{\n"
                 "    return (HelperInt)v + 1;\n}\n#endif\n")
        mixed = MIXED.replace("static int alpha_helper(int v);\n", "") \
            .replace("static int alpha_helper(int v)\n{\n    return v + 1;\n}\n", "") \
            .replace("static int s_shared_counter;", group + "static int s_shared_counter;")
        plan = self.plan(mixed)
        unit = plan.files["src/game/alpha.cpp"]
        self.assertIn(group, unit)
        self.assertLess(unit.index(group), unit.index("int alpha_second(void)"))
        self.assertEqual(plan.forward, [])
        self.assertNotIn("alpha_helper", plan.files["src/game/mixed.cpp"])

    def test_shared_helper_group_with_alternatives_refuses(self):
        # G98-009's shape (typedefs in the second branch, so not plain
        # alternatives of one function): shared, it would need a guarded
        # declaration for both branches.
        group = ("#if defined(_MSC_VER)\nstatic int alpha_helper(int v)\n{\n    return v + 1;\n}\n"
                 "#elif defined(__GNUC__)\ntypedef int HelperInt;\nstatic int alpha_helper(int v)\n{\n"
                 "    return (HelperInt)v + 2;\n}\n#endif\n")
        mixed = MIXED.replace("static int alpha_helper(int v);\n", "") \
            .replace("static int alpha_helper(int v)\n{\n    return v + 1;\n}\n", "") \
            .replace("static int s_shared_counter;", group + "static int s_shared_counter;") \
            .replace("    return s_beta_state;", "    return s_beta_state + alpha_helper(1);")
        reasons = self.refused(mixed)
        self.assertTrue(any("branch 2 of the #if group" in r for r in reasons), reasons)

    def test_name_clash_between_two_original_globals_says_rename(self):
        mixed = MIXED.replace("static int s_shared_counter;", "// GLOBAL: LEGORR 0x004c0004\nstatic int s_shared_counter;")
        other = OTHER + "// GLOBAL: LEGORR 0x004c0000\nint s_shared_counter;\n"
        reasons = self.refused(mixed, {"src/game/other.cpp": other})
        self.assertTrue(any("would get external linkage" in r and "two original globals" in r
                            and "rename" in r for r in reasons), reasons)

    def test_name_clash_with_one_original_global_says_unify(self):
        mixed = MIXED.replace("static int s_shared_counter;", "// GLOBAL: LEGORR 0x004c0000\nstatic int s_shared_counter;")
        other = OTHER + "// GLOBAL: LEGORR 0x004c0000\nint s_shared_counter;\n"
        reasons = self.refused(mixed, {"src/game/other.cpp": other})
        self.assertTrue(any("one original global" in r and "unify" in r for r in reasons), reasons)

    def test_name_clash_with_a_port_only_definition_says_rename_it(self):
        mixed = MIXED.replace("static int s_shared_counter;", "// GLOBAL: LEGORR 0x004c0000\nstatic int s_shared_counter;")
        other = OTHER + "int s_shared_counter;\n"
        reasons = self.refused(mixed, {"src/game/other.cpp": other})
        self.assertTrue(any("port-only" in r and "src/game/other.cpp" in r for r in reasons), reasons)

    def test_namespace_nested_in_itself_refuses(self):
        nested = MIXED.replace("// FUNCTION: LEGORR 0x00401000\nint alpha_first(int x)\n{\n"
                               "    s_alpha_state = x;\n    return alpha_second();\n}\n",
                               "namespace port {\nnamespace port {\n// FUNCTION: LEGORR 0x00401000\n"
                               "int alpha_first(int x)\n{\n    return x;\n}\n}\n}\n")
        reasons = self.refused(nested)
        self.assertTrue(any("nested in itself" in r for r in reasons), reasons)

    def test_anonymous_namespace_inside_a_named_namespace(self):
        # GAME-021's shape (#515): lego::{anonymous}::prefix_equal, a closure
        # helper used before its definition in the unit file.
        anon = MIXED.replace("static int alpha_helper(int v);\n", "") \
            .replace("static int alpha_helper(int v)\n{\n    return v + 1;\n}\n",
                     "namespace lego {\nnamespace {\nint alpha_helper(int v)\n{\n    return v + 1;\n}\n}\n}\n") \
            .replace("    return alpha_helper(s_alpha_state) * ALPHA_SCALE;",
                     "    return lego::alpha_helper(s_alpha_state) * ALPHA_SCALE;")
        plan = self.plan(anon)
        self.assertIn("namespace lego { namespace { int alpha_helper(int v); } }", plan.forward)
        unit = plan.files["src/game/alpha.cpp"]
        self.assertIn("namespace lego {\nnamespace {\nint alpha_helper(int v)", unit)
        self.assertEqual(cx.scan(unit).problems, [])
