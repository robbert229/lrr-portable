# SPDX-License-Identifier: LGPL-3.0-or-later
"""tools/reprobit_reference.py: body extraction, masking, plans and PE reads."""
import json
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import msvc_coff  # noqa: E402
import reprobit_reference as rr  # noqa: E402

FUNCTION = 0x20  # IMAGE_SYM_DTYPE_FUNCTION << 4


def coff_with(body: bytes, relocations: list[msvc_coff.Relocation], symbol: str = "_f") -> msvc_coff.CoffObject:
    text = msvc_coff.Section(1, ".text", msvc_coff.IMAGE_SCN_CNT_CODE, body, len(body), relocations)
    symbols = {
        0: msvc_coff.Symbol(0, symbol, 0, 1, FUNCTION, 2, []),
        1: msvc_coff.Symbol(1, "_g", 0, 0, FUNCTION, 2, []),
    }
    return msvc_coff.CoffObject("x.obj", [text], symbols, [], [], None)


def _coff_with_function(body: bytes, reloc_offset: int = 9) -> bytes:
    """A minimal i386 COFF: one code section holding ``_f``, one REL32 to ``_g``."""
    raw = 20 + 40
    rel = raw + len(body)
    sym = rel + 10
    header = struct.pack("<HHIIIHH", 0x14C, 1, 0, sym, 2, 0, 0)
    section = b".text\0\0\0" + struct.pack("<IIIIIIHHI", 0, 0, len(body), raw, rel, 0, 1, 0,
                                              msvc_coff.IMAGE_SCN_CNT_CODE)
    relocation = struct.pack("<IIH", reloc_offset, 1, 0x14)
    symbols = (b"_f\0\0\0\0\0\0" + struct.pack("<IhHBB", 0, 1, FUNCTION, 2, 0)
               + b"_g\0\0\0\0\0\0" + struct.pack("<IhHBB", 0, 0, FUNCTION, 2, 0))
    return header + section + body + relocation + symbols + struct.pack("<I", 4)


class BodyTests(unittest.TestCase):
    def test_body_and_relocated_spans(self):
        body = bytes(range(16))
        coff = coff_with(body, [msvc_coff.Relocation(4, 1, 0x14), msvc_coff.Relocation(10, 1, 0x0A)])
        got, spans = rr.function_body(coff, "_f")
        self.assertEqual(got, body)
        self.assertEqual(spans, [(4, 8), (10, 12)])

    def test_unknown_symbol_has_no_body(self):
        self.assertIsNone(rr.function_body(coff_with(b"\x90" * 4, []), "_missing"))

    def test_match_ignores_only_relocated_fields(self):
        body = bytes.fromhex("e8 00000000 c3")
        spans = [(1, 5)]
        self.assertTrue(rr.body_matches(body, spans, bytes.fromhex("e8 78563412 c3")))
        self.assertFalse(rr.body_matches(body, spans, bytes.fromhex("e9 78563412 c3")))
        self.assertFalse(rr.body_matches(body, spans, bytes.fromhex("e8 78563412")))


class PlanTests(unittest.TestCase):
    PLAN = {"translation_units": [
        {"id": "tu.a", "source": "src/gods98/maths.c", "target_id": "program"},
        {"id": "tu.b", "source": "src/game/lego.c", "target_id": "program"},
    ]}

    def test_translation_unit_by_source(self):
        self.assertEqual(rr.translation_unit(self.PLAN, "src/gods98/maths.c")["id"], "tu.a")
        with self.assertRaises(rr.ReferenceError):
            rr.translation_unit(self.PLAN, "src/missing.c")

    def test_grind_plan_is_canonical_extern_run_plan(self):
        unit = self.PLAN["translation_units"][0]
        text = rr.grind_plan(unit=unit, symbol="_f", reference="reference/grind/00479a30.obj", counts=(1, 64))
        self.assertTrue(text.endswith(b"\n"))
        self.assertEqual(json.loads(text), {
            "extern_runs": {"counts": {"start": 1, "step": 1, "stop": 64}},
            "reference_object": "reference/grind/00479a30.obj",
            "schema_version": 1,
            "symbol": "_f",
            "target": "program",
            "translation_unit": "tu.a",
        })
        self.assertNotIn(b" ", text)

    def test_prefix_placement_when_an_include_follows_the_function(self):
        text = '#include "a.h"\nint f(void)\n{\n}\n#include "b.h"\n'
        self.assertEqual(rr.run_placement(text, 2), "prefix")
        self.assertEqual(rr.run_placement(text, 6), "after_includes")
        self.assertEqual(rr.run_placement("int f(void);\n", 1), "after_includes")

    def test_prefix_plan_names_its_placement(self):
        unit = self.PLAN["translation_units"][0]
        plan = json.loads(rr.grind_plan(unit=unit, symbol="_f", reference="reference/grind/x.obj",
                                        counts=(1, 8), placement="prefix"))
        self.assertEqual(plan["extern_runs"]["placement"], "prefix")

    def test_output_must_lie_under_reference(self):
        self.assertEqual(rr.output_dir(rr.ROOT / "reference" / "grind"), (rr.ROOT / "reference" / "grind").resolve())
        with self.assertRaises(rr.ReferenceError):
            rr.output_dir(rr.ROOT / "docs")


class OriginalBytesTests(unittest.TestCase):
    def test_reads_by_virtual_address(self):
        pe = 0x40
        image = bytearray(0x600)
        struct.pack_into("<I", image, 0x3C, pe)
        struct.pack_into("<H", image, pe + 6, 1)  # one section
        struct.pack_into("<H", image, pe + 20, 0xE0)  # optional header size
        struct.pack_into("<I", image, pe + 0x34, 0x400000)  # image base
        section = pe + 24 + 0xE0
        struct.pack_into("<IIII", image, section + 8, 0x100, 0x1000, 0x200, 0x400)
        image[0x410:0x414] = b"\xde\xad\xbe\xef"
        self.assertEqual(rr.original_bytes(bytes(image), 0x401010, 4), b"\xde\xad\xbe\xef")
        with self.assertRaises(rr.ReferenceError):
            rr.original_bytes(bytes(image), 0x405000, 4)


if __name__ == "__main__":
    unittest.main()


class CountWindowTest(unittest.TestCase):
    def test_small_pad_counts_start_at_one(self):
        self.assertEqual(rr.count_window(3), (1, 64))
        self.assertEqual(rr.count_window(31), (1, 64))

    def test_large_pad_counts_are_centred(self):
        self.assertEqual(rr.count_window(162), (131, 194))
        self.assertEqual(rr.count_window(187), (156, 219))

    def test_window_holds_at_most_64_lengths(self):
        start, stop = rr.count_window(121)
        self.assertEqual(stop - start + 1, 64)
        self.assertTrue(start <= 121 <= stop)


class SyntheticReferenceTest(unittest.TestCase):
    def _object(self, body: bytes) -> bytes:
        return _coff_with_function(body)

    def test_the_original_body_replaces_the_compile_outside_relocations(self):
        compiled = bytes.fromhex("8d4c24088d542408e800000000c3")
        original = bytes.fromhex("8d5424088d4c2408e8aabbccddc3")
        data = rr.synthetic_reference(self._object(compiled), "_f", original)
        body, spans = rr.function_body(msvc_coff.parse_coff(data), "_f")
        self.assertEqual(spans, [(9, 13)])
        self.assertEqual(body, bytes.fromhex("8d5424088d4c2408e800000000c3"))

    def test_no_reference_when_lengths_differ_or_bodies_agree(self):
        compiled = bytes.fromhex("8d4c24088d542408e800000000c3")
        self.assertIsNone(rr.synthetic_reference(self._object(compiled), "_f", compiled[:-1]))
        self.assertIsNone(rr.synthetic_reference(self._object(compiled), "_f", compiled))
