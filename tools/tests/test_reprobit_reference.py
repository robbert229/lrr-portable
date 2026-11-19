# SPDX-License-Identifier: LGPL-3.0-or-later
"""tools/reprobit_reference.py: body extraction, masking, plans and PE reads."""
import json
import os
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


ADDRESS = 0x401000  # the original function's address in these tests
RANGES = [(0x401000, 0x402000), (0x410000, 0x411000)]
D, G, H = 0x410100, 0x401800, 0x401900  # the original's _d, _g and _h
DIR32, REL32 = 0x06, 0x14


def _object(body: bytes, relocations: list[tuple[int, int, int]], names=("_d", "_g", "_h")) -> bytes:
    """An i386 COFF: ``_f`` (symbol 0) holding ``body``, then undefined ``names``."""
    raw = 20 + 40
    rel = raw + len(body)
    sym = rel + 10 * len(relocations)
    header = struct.pack("<HHIIIHH", 0x14C, 1, 0, sym, 1 + len(names), 0, 0)
    section = b".text\0\0\0" + struct.pack("<IIIIIIHHI", 0, 0, len(body), raw, rel, 0,
                                              len(relocations), 0, msvc_coff.IMAGE_SCN_CNT_CODE)
    table = b"".join(struct.pack("<IIH", *r) for r in relocations)
    symbols = b"_f\0\0\0\0\0\0" + struct.pack("<IhHBB", 0, 1, FUNCTION, 2, 0)
    for name in names:
        symbols += name.encode().ljust(8, b"\0") + struct.pack("<IhHBB", 0, 0, FUNCTION, 2, 0)
    return header + section + body + table + symbols + struct.pack("<I", 4)


def rel32(target: int, field: int) -> bytes:
    """The original's rel32 at body offset ``field`` that branches to ``target``."""
    return struct.pack("<I", (target - (ADDRESS + field + 4)) & 0xFFFFFFFF)


def u32(value: int) -> bytes:
    return struct.pack("<I", value)


def relocations(data: bytes) -> list[tuple[int, str, int, int]]:
    """(offset, symbol, type, addend) of ``_f``'s relocations."""
    coff = msvc_coff.parse_coff(data)
    body = coff.section(1).data
    return [(r.offset, coff.symbols[r.symbol].name, r.type, struct.unpack_from("<I", body, r.offset)[0])
            for r in coff.section(1).relocations]


def explain(data: bytes, original: bytes, **kwargs):
    return rr.explain_synthetic_reference(data, "_f", original, address=ADDRESS, ranges=RANGES, **kwargs)


class SyntheticReferenceTest(unittest.TestCase):
    def test_the_original_body_replaces_the_compile_outside_relocations(self):
        compiled = bytes.fromhex("8d4c24088d542408e800000000c3")
        original = bytes.fromhex("8d5424088d4c2408e8aabbccddc3")
        data = rr.synthetic_reference(_coff_with_function(compiled), "_f", original,
                                      address=ADDRESS, ranges=RANGES)
        body, spans = rr.function_body(msvc_coff.parse_coff(data), "_f")
        self.assertEqual(spans, [(9, 13)])
        self.assertEqual(body, bytes.fromhex("8d5424088d4c2408e800000000c3"))

    def test_falls_back_to_seed_offsets_for_a_same_length_body(self):
        # A placement refusal (here forced) on a body of the seed's padded
        # length gives the seed-offset reference, with the reason noted.
        compiled = bytes.fromhex("8d4c24088d542408e800000000c3")
        original = bytes.fromhex("8d5424088d4c2408e8aabbccddc3") + b"\x90\x90"
        obj = _coff_with_function(compiled)
        saved = rr.explain_synthetic_reference
        rr.explain_synthetic_reference = lambda *a, **k: (None, "forced refusal")
        try:
            data, reason = rr.explain_reference_with_fallback(obj, "_f", original,
                                                              address=ADDRESS, ranges=RANGES)
            shorter, _ = rr.explain_reference_with_fallback(obj, "_f", original[:-2],
                                                            address=ADDRESS, ranges=RANGES)
        finally:
            rr.explain_synthetic_reference = saved
        self.assertIn("forced refusal", reason)
        self.assertIn("seed's field offsets", reason)
        body, _spans = rr.function_body(msvc_coff.parse_coff(data), "_f")
        self.assertEqual(body, bytes.fromhex("8d5424088d4c2408e800000000c3"))
        self.assertEqual(data, rr.legacy_synthetic_reference(obj, "_f", original))
        # a slot of another length is not the seed's layout: still refused
        self.assertIsNone(shorter)

    def test_fallback_refuses_seed_offsets_inside_the_originals_instructions(self):
        # The seed's call field is at 9; the original's (two nops earlier, one
        # push later) is at 10, so the seed's offset is inside the original's
        # call instruction: the fallback has no reference to give.
        compiled = bytes.fromhex("8d4c24088d542408" "e800000000" "c3")
        original = bytes.fromhex("8d5424088d4c2408" "90" "e8aabbccdd" "c3") + b"\x90"
        obj = _coff_with_function(compiled)
        saved = rr.explain_synthetic_reference
        rr.explain_synthetic_reference = lambda *a, **k: (None, "forced refusal")
        try:
            data, reason = rr.explain_reference_with_fallback(obj, "_f", original,
                                                              address=ADDRESS, ranges=RANGES)
        finally:
            rr.explain_synthetic_reference = saved
        self.assertIsNone(data)
        self.assertEqual(reason, "forced refusal")
        self.assertIsNone(rr.legacy_synthetic_reference(obj, "_f", original))

    def test_no_reference_when_lengths_differ_or_bodies_agree(self):
        compiled = bytes.fromhex("8d4c24088d542408e800000000c3")
        obj = _coff_with_function(compiled)
        self.assertIsNone(rr.synthetic_reference(obj, "_f", compiled[:-1], address=ADDRESS, ranges=RANGES))
        self.assertIsNone(rr.synthetic_reference(obj, "_f", compiled, address=ADDRESS, ranges=RANGES))
        linked = compiled[:9] + rel32(G, 9) + compiled[13:]
        self.assertIsNone(rr.synthetic_reference(obj, "_f", linked, address=ADDRESS, ranges=RANGES))

    def test_same_layout_is_the_seed_with_the_original_outside_its_relocations(self):
        # mov ecx, [_d+4]; push ecx; call _g; add esp, 4; ret -- the original uses eax.
        seed = bytes.fromhex("8b0d04000000" "51" "e800000000" "83c404" "c3")
        original = bytes.fromhex("8b05") + u32(D + 4) + bytes.fromhex("50" "e8") + rel32(G, 8) \
            + bytes.fromhex("83c404" "c3")
        data = _object(seed, [(2, 1, DIR32), (8, 2, REL32)])
        got, reason = explain(data, original)
        self.assertEqual(reason, "written")
        # What the reference was before fields were placed by alignment: the
        # seed's relocated fields copied over the original's bytes.
        expected = bytearray(data)
        legacy = bytearray(original)
        legacy[2:6], legacy[8:12] = seed[2:6], seed[8:12]
        expected[60:60 + len(seed)] = legacy
        self.assertEqual(got, bytes(expected))

    def test_shifted_fields_take_their_aligned_targets(self):
        # sub esp, 8; mov ecx, [_d+4]; push ecx; call _g; mov eax, [_d+8]; add esp, 0xc; ret
        seed = bytes.fromhex("83ec08" "8b0d04000000" "51" "e800000000" "a108000000" "83c40c" "c3")
        # sub esp, 0x10; push esi; mov eax, [d+8]; mov ecx, [d+4]; push ecx; call g; pop esi; ret; nop
        original = bytes.fromhex("83ec10" "56" "a1") + u32(D + 8) + bytes.fromhex("8b0d") + u32(D + 4) \
            + bytes.fromhex("51" "e8") + rel32(G, 17) + bytes.fromhex("5e" "c3" "90")
        self.assertEqual(len(seed), len(original))
        data = _object(seed, [(5, 1, DIR32), (11, 2, REL32), (16, 1, DIR32)])
        got, reason = explain(data, original)
        self.assertEqual(reason, "written")
        self.assertEqual(relocations(got), [(5, "_d", DIR32, 8), (11, "_d", DIR32, 4), (17, "_g", REL32, 0)])
        body = rr.function_body(msvc_coff.parse_coff(got), "_f")[0]
        self.assertEqual(body, original[:5] + u32(8) + original[9:11] + u32(4) + original[15:17] + u32(0)
                         + original[21:])

    def test_jump_tables_follow_the_original_code_end(self):
        # mov eax, [esp+4]; jmp [eax*4 + table]; case 0: mov eax, 1; ret; case 1: mov eax, 2; ret;
        # fill; table
        seed = bytes.fromhex("8b442404" "ff2485" "18000000" "b801000000" "c3" "b802000000" "c3" "90"
                             "0b000000" "11000000")
        # The original's first case is two bytes shorter, so its table starts at 22 and fill ends it.
        original = bytes.fromhex("8b442404" "ff2485") + u32(ADDRESS + 22) + bytes.fromhex(
            "6a01" "58" "c3" "b802000000" "c3" "90") + u32(ADDRESS + 11) + u32(ADDRESS + 15) + b"\x90\x90"
        self.assertEqual(len(seed), len(original))
        data = _object(seed, [(7, 0, DIR32), (24, 0, DIR32), (28, 0, DIR32)])
        got, reason = explain(data, original)
        self.assertEqual(reason, "written")
        self.assertEqual(relocations(got), [(7, "_f", DIR32, 22), (22, "_f", DIR32, 11), (26, "_f", DIR32, 15)])

    def test_refuses_an_address_field_the_seed_lacks(self):
        seed = bytes.fromhex("b878563412" "c3")  # mov eax, 0x12345678 (no relocation)
        original = bytes.fromhex("b8") + u32(D) + bytes.fromhex("c3")  # mov eax, offset d
        got, reason = explain(_object(seed, []), original)
        self.assertIsNone(got)
        self.assertIn("no aligned seed relocation", reason)

    def test_refuses_a_relocation_the_original_lacks(self):
        seed = bytes.fromhex("a100000000" "8bc1" "c3")  # mov eax, [_d]; mov eax, ecx
        original = bytes.fromhex("a110000000" "8bc2" "c3")  # [0x10] is no image address
        got, reason = explain(_object(seed, [(1, 1, DIR32)]), original)
        self.assertIsNone(got)
        self.assertIn("the original has 0 fields for _d and the seed 1 relocations", reason)

    def test_refuses_a_symbol_at_two_addresses(self):
        seed = bytes.fromhex("8b0500000000" "8b0d04000000" "c3")  # [_d]; [_d+4]
        original = bytes.fromhex("8b15") + u32(D) + bytes.fromhex("8b0d") + u32(D + 0x100) + b"\xc3"
        got, reason = explain(_object(seed, [(2, 1, DIR32), (8, 1, DIR32)]), original)
        self.assertIsNone(got)
        self.assertIn("give _d two addresses", reason)

    def test_refuses_an_ambiguous_alignment(self):
        seed = bytes.fromhex("e800000000" "e800000000" "c3")  # call _g; call _h
        original = bytes.fromhex("e8") + rel32(H, 1) + bytes.fromhex("9090909090" "c3")
        got, reason = explain(_object(seed, [(1, 2, REL32), (6, 3, REL32)]), original)
        self.assertIsNone(got)
        self.assertIn("aligns with seed relocations to different targets", reason)

    def test_known_addresses_pin_swapped_calls(self):
        seed = bytes.fromhex("e800000000" "e800000000" "8bc1" "c3")  # call _g; call _h
        original = bytes.fromhex("e8") + rel32(H, 1) + bytes.fromhex("e8") + rel32(G, 6) \
            + bytes.fromhex("8bc2" "c3")
        data = _object(seed, [(1, 2, REL32), (6, 3, REL32)])
        got, reason = explain(data, original, known={"_g": G, "_h": H})
        self.assertEqual(reason, "written")
        self.assertEqual(relocations(got), [(1, "_h", REL32, 0), (6, "_g", REL32, 0)])

    def test_known_addresses_rule_out_a_wrong_alignment(self):
        seed = bytes.fromhex("e800000000" "e800000000" "8bc1" "c3")  # call _g; call _h
        other = 0x401A00  # the original calls a function the seed does not
        original = bytes.fromhex("e8") + rel32(other, 1) + bytes.fromhex("e8") + rel32(H, 6) \
            + bytes.fromhex("8bc2" "c3")
        data = _object(seed, [(1, 2, REL32), (6, 3, REL32)])
        self.assertEqual(explain(data, original)[1], "written")  # alignment alone pairs _g with it
        got, reason = explain(data, original, known={"_g": G, "_h": H})
        self.assertIsNone(got)
        self.assertIn("the original's field at 1 has no aligned seed relocation", reason)

    def test_known_addresses_from_the_report_and_map(self):
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            build = Path(tmp)
            (build / "reccmp-report").mkdir()
            (build / "reccmp-report" / "results.json").write_text(json.dumps({"data": [
                {"address": "0x401800", "recomp": "0x401810"}, {"address": "0x402000"}]}))
            (build / "LegoRR.map").write_text(
                " 0001:00000810       _g 00401810 f a.obj\n 0001:00000900       _h 00401900 f a.obj\n"
                " 0002:00000010       _lit 0049e010   a.obj\n"
                " 0003:000b50c0       _globs 005560c0   <common>\n")
            # Paired functions take their original address; .data and .bss
            # symbols keep theirs (the layouts match); .rdata is not pinned.
            self.assertEqual(rr.known_addresses(build), {"_g": 0x401800, "_globs": 0x5560c0})
            self.assertEqual(rr.known_addresses(build / "missing"), {})


class StaticSymbolTest(unittest.TestCase):
    def test_static_function_named_from_the_link_map(self) -> None:
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "docs" / "coverage").mkdir(parents=True)
            (root / "build-reccmp").mkdir()
            (root / "docs" / "coverage" / "match-baseline.txt").write_text(
                "[full]\n0x00471370 99.55 weapons_callback_ray_candidate\n0x00401000 100.00 advisor_init\n")
            (root / "build-reccmp" / "LegoRR.map").write_text(
                " 0001:00000000       _advisor_init 00401000 f advisor.c.obj\n\n Static symbols\n\n"
                " 0001:000701f0       _weapons_callback_ray_candidate 004711f0 f weapons.c.obj\n")
            self.assertEqual(rr.static_symbol(0x471370, root), "_weapons_callback_ray_candidate")
            self.assertIsNone(rr.static_symbol(0x401000, root))
            self.assertIsNone(rr.static_symbol(0x123456, root))


# ---------------------------------------------------------------------------
# Synthetic references whose procedure size differs from the seed's.

COMDAT = msvc_coff.IMAGE_SCN_LNK_COMDAT
TEXT_FLAGS = msvc_coff.IMAGE_SCN_CNT_CODE | COMDAT
DEBUG_FLAGS = msvc_coff.IMAGE_SCN_CNT_INITIALIZED_DATA | COMDAT
DATA_FLAGS = msvc_coff.IMAGE_SCN_CNT_INITIALIZED_DATA
STATIC, LABEL, FUNCTION_CLASS = 3, 6, 101
SECREL, SECTION_REL, DIR32NB = 0x0B, 0x0A, 0x07
TRAILER = b"DATA" * 3  # a .data section after the function's records


def _msvc_object(body: bytes, proc: int, relocations: list[tuple[int, str, int]], *,
                 labels: dict[str, int] = None, lines: list[tuple[int, int]] = (),
                 debug_range: tuple[int, int] = (0, None), records: bool = True,
                 extra_function: bool = False) -> bytes:
    """An MSVC 5.0-shaped object: ``_f`` alone in a COMDAT .text holding
    ``body`` (procedure ``proc``), with its section symbol, TotalSize, .bf,
    .lf and .ef, ``labels``, line rows, an associated .debug$S procedure
    record and .debug$F FPO record, then a .data section; ``relocations``
    name their targets: _f, the labels, or the undefined _d, _g and _h."""
    labels = labels or {}
    names = ["_d", "_g", "_h"]
    symbols, index = [], {}

    def add(name, value, section, type_, sclass, aux=()):
        index[name] = sum(1 + len(a) for _n, *_r, a in symbols)
        symbols.append((name, value, section, type_, sclass, list(aux)))

    text_size = len(body)
    line_rows = [(0, 0)] + list(lines)  # the sentinel's symbol index is patched below
    add(".text", 0, 1, 0, STATIC, [struct.pack("<IHHIHBxxx", text_size, len(relocations),
                                               len(line_rows), 0, 0, 2)])
    add("_f", 0, 1, FUNCTION, 2, [struct.pack("<IIIIxx", 0, proc if records else 0, 0, 0)]
        if records else [])
    if extra_function:
        add("_f2", 8, 1, FUNCTION, 2)
    if records:
        add(".bf", 0, 1, 0, FUNCTION_CLASS, [bytes(18)])
        add(".lf", len(lines), 1, 0, FUNCTION_CLASS)
        add(".ef", proc, 1, 0, FUNCTION_CLASS, [bytes(18)])
    for name, value in labels.items():
        add(name, value, 1, 0, LABEL)
    if records:
        add(".debug$S", 0, 2, 0, STATIC, [struct.pack("<IHHIHBxxx", 0, 2, 0, 0, 1, 5)])
        add(".debug$F", 0, 3, 0, STATIC, [struct.pack("<IHHIHBxxx", 16, 1, 0, 0, 1, 5)])
    add(".data", 0, 4 if records else 2, 0, STATIC, [struct.pack("<IHHIHBxxx", len(TRAILER), 0, 0, 0, 0, 0)])
    for name in names:
        add(name, 0, 0, FUNCTION, 2)
    line_rows[0] = (index["_f"], 0)

    start, end = debug_range
    proc_record = struct.pack("<HHIIIIIIIIHB", 39, 0x100B, 0, 0, 0, proc, start,
                              proc if end is None else end, 0x1019, 0, 0, 0) + b"\x01f"
    cv = proc_record + struct.pack("<HH", 2, 6)
    fpo = struct.pack("<IIIHBB", 0, proc, 0, 1, 3, 0)
    sections = [(".text", TEXT_FLAGS, body,
                 [(o, index[n], t) for o, n, t in relocations], line_rows)]
    if records:
        sections += [(".debug$S", DEBUG_FLAGS, cv, [(32, index["_f"], SECREL), (36, index["_f"], SECTION_REL)], []),
                     (".debug$F", DEBUG_FLAGS, fpo, [(0, index["_f"], DIR32NB)], [])]
    sections.append((".data", DATA_FLAGS, TRAILER, [], []))

    cursor = 20 + 40 * len(sections)
    headers, payload = b"", b""
    for name, flags, raw, relocs, rows in sections:
        rawptr = cursor
        relptr = rawptr + len(raw) if relocs else 0
        lnptr = rawptr + len(raw) + 10 * len(relocs) if rows else 0
        headers += name.encode().ljust(8, b"\0") + struct.pack(
            "<IIIIIIHHI", 0, 0, len(raw), rawptr, relptr, lnptr, len(relocs), len(rows), flags)
        payload += raw + b"".join(struct.pack("<IIH", *r) for r in relocs) \
            + b"".join(struct.pack("<IH", *r) for r in rows)
        cursor += len(raw) + 10 * len(relocs) + 6 * len(rows)
    table = b""
    for name, value, section, type_, sclass, aux in symbols:
        table += name.encode().ljust(8, b"\0") + struct.pack("<IhHBB", value, section, type_, sclass, len(aux))
        table += b"".join(aux)
    count = sum(1 + len(s[5]) for s in symbols)
    header = struct.pack("<HHIIIHH", 0x14C, len(sections), 0, cursor, count, 0, 0)
    out = bytearray(header + headers + payload + table + struct.pack("<I", 4))
    if records:  # TotalSize's PointerToLinenumber: the .text line table
        lnptr = struct.unpack_from("<I", out, 20 + 28)[0]
        struct.pack_into("<I", out, cursor + (index["_f"] + 1) * 18 + 8, lnptr)
    return bytes(out)


def _records(data: bytes, symbol: str = "_f") -> dict:
    """Everything a resize must keep consistent, read back from ``data``."""
    coff = msvc_coff.parse_coff(data)
    sym = next(s for s in coff.symbols.values() if s.name == symbol and s.section > 0)
    text = coff.section(sym.section)
    by_name = {s.name: s for s in coff.symbols.values()}
    out = {
        "body": text.data,
        "size": text.size,
        "section_length": struct.unpack_from("<I", by_name[".text"].aux[0], 0)[0],
        "relocations": [(r.offset, coff.symbols[r.symbol].name, r.type,
                         struct.unpack_from("<I", text.data, r.offset)[0]) for r in text.relocations],
        "lines": text.linenumbers[1:],
        "labels": {s.name: s.value for s in coff.symbols.values() if s.storage_class == LABEL},
        "trailer": next(s for s in coff.sections if s.name == ".data").data,
    }
    if sym.aux:
        total, lnptr = struct.unpack_from("<II", sym.aux[0], 4)
        lines = struct.unpack_from("<I", data, 20 + 28)[0]
        out.update(total=total, line_pointer_ok=lnptr == lines, ef=by_name[".ef"].value)
        debug = next(s for s in coff.sections if s.name == ".debug$S")
        out["codeview"] = struct.unpack_from("<III", debug.data, 16)
        out["codeview_relocations"] = [(r.offset, coff.symbols[r.symbol].name) for r in debug.relocations]
        fpo = next(s for s in coff.sections if s.name == ".debug$F")
        out["fpo"] = struct.unpack_from("<I", fpo.data, 4)[0]
    return out


def _pad(code: bytes) -> bytes:
    return code + b"\x90" * (-len(code) % 16)


# mov ecx, [_d+4]; push ecx; call _g; add esp, 4; ret -- 16 bytes, its own slot.
SEED = bytes.fromhex("8b0d04000000" "51" "e800000000" "83c404" "c3")
SEED_RELOCS = [(2, "_d", DIR32), (8, "_g", REL32)]


# The original saves esi around it and returns 0: 20 bytes, a 32-byte slot.
LONGER = bytes.fromhex("56" "8b0d") + u32(D + 4) + bytes.fromhex("51" "e8") + rel32(G, 9) \
    + bytes.fromhex("83c404" "33c0" "5e" "c3")


class SyntheticResizeTest(unittest.TestCase):
    def explain(self, data, original, **kwargs):
        return rr.explain_synthetic_reference(data, "_f", original, address=ADDRESS, ranges=RANGES,
                                              **kwargs)

    def seed(self, **kwargs):
        return _msvc_object(_pad(SEED), len(SEED), SEED_RELOCS,
                            lines=[(0, 10), (7, 11), (15, 12)], debug_range=(0, 15), **kwargs)

    def test_equal_sizes_keep_the_seed_records(self):
        # The original uses eax: the same lengths, so only the body changes.
        original = _pad(bytes.fromhex("8b05") + u32(D + 4) + bytes.fromhex("50" "e8") + rel32(G, 8)
                        + bytes.fromhex("83c404" "c3"))
        data = self.seed()
        got, reason = self.explain(data, original)
        self.assertEqual(reason, "written")
        expected = bytearray(data)
        raw = struct.unpack_from("<I", data, 20 + 20)[0]
        legacy = bytearray(original)
        legacy[2:6], legacy[8:12] = _pad(SEED)[2:6], _pad(SEED)[8:12]
        expected[raw:raw + 16] = legacy
        self.assertEqual(got, bytes(expected))

    def test_same_slot_with_a_shorter_procedure(self):
        # The original pops the argument: 14 bytes and two NOPs in the same 16-byte slot.
        original = _pad(bytes.fromhex("8b0d") + u32(D + 4) + bytes.fromhex("51" "e8") + rel32(G, 8)
                        + bytes.fromhex("59" "c3"))
        self.assertEqual(len(original), 16)
        got, reason = self.explain(self.seed(), original)
        self.assertEqual(reason, "written")
        records = _records(got)
        self.assertEqual(records["body"][:14], original[:2] + u32(4) + original[6:8] + u32(0) + original[12:14])
        self.assertEqual((records["size"], records["section_length"]), (16, 16))
        self.assertEqual((records["total"], records["ef"], records["fpo"]), (14, 14, 14))
        self.assertEqual(records["codeview"], (14, 0, 13))  # the ret pairs with the ret
        self.assertEqual(records["lines"], [(0, 10), (7, 11), (13, 12)])
        self.assertEqual(records["relocations"], [(2, "_d", DIR32, 4), (8, "_g", REL32, 0)])
        self.assertEqual(records["trailer"], TRAILER)

    def test_a_larger_padded_slot_resizes_the_section(self):
        # push esi; ... pop esi -- 20 bytes, so the original's slot is 32.
        code = LONGER
        self.assertEqual(len(code), 20)
        original = _pad(code)
        data = self.seed()
        got, reason = self.explain(data, original)
        self.assertEqual(reason, "written")
        records = _records(got)
        self.assertEqual(records["body"], original[:3] + u32(4) + original[7:9] + u32(0) + original[13:])
        self.assertEqual((records["size"], records["section_length"]), (32, 32))
        self.assertEqual((records["total"], records["ef"], records["fpo"]), (20, 20, 20))
        self.assertEqual(records["codeview"], (20, 0, 19))
        self.assertEqual(records["lines"], [(0, 10), (8, 11), (19, 12)])  # the entry stays
        self.assertEqual(records["relocations"], [(3, "_d", DIR32, 4), (9, "_g", REL32, 0)])
        self.assertTrue(records["line_pointer_ok"])
        self.assertEqual(records["trailer"], TRAILER)
        self.assertEqual(records["codeview_relocations"], [(32, "_f"), (36, "_f")])
        self.assertEqual(len(got), len(data) + 16)

    def test_a_smaller_padded_slot_shrinks_the_section(self):
        # The seed saves esi around the call (20 bytes, slot 32); the original's 16 fill 16.
        seed = bytes.fromhex("56" "8b0d04000000" "51" "e800000000" "83c404" "5e" "33c0" "c3")
        self.assertEqual(len(seed), 20)
        data = _msvc_object(_pad(seed), len(seed), [(3, "_d", DIR32), (9, "_g", REL32)],
                            lines=[(0, 10), (8, 11), (17, 12)], debug_range=(0, 19))
        original = bytes.fromhex("8b0d") + u32(D + 4) + bytes.fromhex("51" "e8") + rel32(G, 8) \
            + bytes.fromhex("83c404" "c3")
        got, reason = self.explain(data, original)
        self.assertEqual(reason, "written")
        records = _records(got)
        self.assertEqual((records["size"], records["section_length"]), (16, 16))
        self.assertEqual((records["total"], records["ef"], records["fpo"]), (16, 16, 16))
        self.assertEqual(records["codeview"], (16, 0, 15))
        # push esi and xor eax, eax have no instruction of their own: their
        # lines go to their gap's first original instruction (the mov, the ret).
        self.assertEqual(records["lines"], [(0, 10), (7, 11), (15, 12)])
        self.assertEqual(records["relocations"], [(2, "_d", DIR32, 4), (8, "_g", REL32, 0)])
        self.assertEqual(records["trailer"], TRAILER)
        self.assertEqual(len(got), len(data) - 16)

    def test_a_switch_table_keeps_its_place_after_the_code(self):
        # mov eax, [esp+4]; jmp [eax*4 + $L3]; $L1: mov eax, 1; ret; $L2: mov eax, 2; ret;
        # nop; $L3: dd $L1, $L2 -- procedure 32, so no fill.
        seed = bytes.fromhex("8b442404" "ff2485" "00000000" "b801000000" "c3" "b802000000" "c3" "90"
                             "00000000" "00000000")
        labels = {"$L1": 11, "$L2": 17, "$L3": 24}
        data = _msvc_object(seed, 32, [(7, "$L3", DIR32), (24, "$L1", DIR32), (28, "$L2", DIR32)],
                            labels=labels, lines=[(0, 5), (11, 6), (17, 7)], debug_range=(0, 32))
        # The original tests the argument first: its code is 8 bytes longer.
        code = bytes.fromhex("8b442404" "85c0" "7c07" "ff2485") + u32(ADDRESS + 32) + bytes.fromhex(
            "33c0" "c3" "b801000000" "c3" "b802000000" "c3" "8bff")
        self.assertEqual(len(code), 32)
        original = _pad(code + u32(ADDRESS + 18) + u32(ADDRESS + 24))
        got, reason = self.explain(data, original)
        self.assertEqual(reason, "written")
        records = _records(got)
        self.assertEqual((records["size"], records["total"], records["fpo"]), (48, 40, 40))
        self.assertEqual(records["labels"], {"$L1": 18, "$L2": 24, "$L3": 32})
        self.assertEqual(records["relocations"],
                         [(11, "$L3", DIR32, 0), (32, "$L1", DIR32, 0), (36, "$L2", DIR32, 0)])
        self.assertEqual(records["lines"], [(0, 5), (18, 6), (24, 7)])
        self.assertEqual(records["codeview"], (40, 0, 40))

    def test_an_unpadded_seed_drops_the_linkers_int3_fill(self):
        seed = bytes.fromhex("8b0d04000000" "51" "e800000000" "59" "c3")  # 14 bytes, no fill
        data = _msvc_object(seed, len(seed), SEED_RELOCS, lines=[(0, 10)], debug_range=(0, 13))
        code = bytes.fromhex("8b0d") + u32(D + 4) + bytes.fromhex("51" "e8") + rel32(G, 8) \
            + bytes.fromhex("83c404" "33c0" "c3")
        got, reason = self.explain(data, code + b"\xcc" * 14)
        self.assertEqual(reason, "written")
        records = _records(got)
        self.assertEqual((records["size"], records["total"], records["section_length"]), (18, 18, 18))
        self.assertEqual(records["codeview"], (18, 0, 17))
        self.assertEqual(records["body"], code[:2] + u32(4) + code[6:8] + u32(0) + code[12:])

    def test_refuses_several_functions_in_one_section(self):
        data = _msvc_object(_pad(SEED), len(SEED), SEED_RELOCS, extra_function=True)
        got, reason = self.explain(data, _pad(LONGER))
        self.assertIsNone(got)
        self.assertIn("several functions share the seed's section", reason)

    def test_refuses_records_that_disagree(self):
        data = bytearray(self.seed())
        coff = msvc_coff.parse_coff(bytes(data))
        fpo = next(s for s in coff.sections if s.name == ".debug$F")
        struct.pack_into("<I", data, struct.unpack_from("<I", data, 20 + 2 * 40 + 20)[0] + 4, 14)
        self.assertEqual(fpo.index, 3)
        got, reason = self.explain(bytes(data), _pad(LONGER))
        self.assertIsNone(got)
        self.assertIn("different procedure sizes", reason)

    def test_refuses_a_slot_holding_another_function(self):
        # The original's slot runs on past a ret and fill into a second function.
        original = bytes.fromhex("8b0d") + u32(D + 4) + bytes.fromhex("51" "e8") + rel32(G, 8) \
            + bytes.fromhex("59" "c3" "9090") + bytes.fromhex("33c0" "c3") + b"\x90" * 13
        got, reason = self.explain(self.seed(), original)
        self.assertIsNone(got)
        self.assertIn("where another function may start", reason)

    def test_refuses_a_slot_of_fill_or_one_cut_short(self):
        got, reason = self.explain(self.seed(), _pad(SEED) + b"\x90" * 16)
        self.assertIsNone(got)
        self.assertIn("bytes of fill", reason)
        cut = bytes.fromhex("8b0d") + u32(D + 4) + bytes.fromhex("51" "e8") + rel32(G, 8) \
            + bytes.fromhex("83c404" "33c0" "33c9" "33d2")
        got, reason = self.explain(self.seed(), _pad(cut))
        self.assertIsNone(got)
        self.assertIn("does not end in a ret or jump", reason)

    def test_refuses_switch_tables_of_another_length(self):
        seed = bytes.fromhex("8b442404" "ff2485" "00000000" "b801000000" "c3" "b802000000" "c3" "90"
                             "00000000" "00000000")
        data = _msvc_object(seed, 32, [(7, "$L3", DIR32), (24, "$L1", DIR32), (28, "$L2", DIR32)],
                            labels={"$L1": 11, "$L2": 17, "$L3": 24})
        code = bytes.fromhex("8b442404" "ff2485") + u32(ADDRESS + 24) + bytes.fromhex(
            "b801000000" "c3" "b802000000" "c3" "90")
        original = _pad(code + u32(ADDRESS + 11) + u32(ADDRESS + 17) + u32(ADDRESS + 17))
        got, reason = self.explain(data, original)
        self.assertIsNone(got)
        self.assertIn("switch tables (12 bytes) differ in length from the seed's (8)", reason)


class SyntheticResizeForkTest(unittest.TestCase):
    """ReproBit's own reading of a resized reference, when the fork is importable."""

    def test_the_fork_reads_the_original_procedure(self):
        sys.path.insert(0, str(Path(os.environ.get("REPROBIT_SRC", Path.home() / "source" / "reprobit" / "src"))))
        try:
            from reprobit.coff_format import CoffObject, coff_body, detailed_relocations
            from reprobit.discovery_region_resize import _instruction_keys, _procedure
        except ImportError:
            self.skipTest("ReproBit is not importable")
        finally:
            sys.path.pop(0)
        test = SyntheticResizeTest()
        got, reason = test.explain(test.seed(), _pad(LONGER))
        self.assertEqual(reason, "written")
        coff = CoffObject(got)
        section = coff.function_section("_f")
        proc, code_end, fill = _procedure(coff, section, "reference")
        self.assertEqual((proc, code_end, fill), (20, 20, 16))
        _instruction_keys(bytes(coff_body(coff, section)), detailed_relocations(coff, section), code_end,
                          "reference")
