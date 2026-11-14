# SPDX-License-Identifier: LGPL-3.0-or-later
import contextlib
import io
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
sys.path.insert(0, str(Path(__file__).resolve().parent))
import link_order
import pe_audit
from test_binary_progress import fixture_pe

ROOT = Path(__file__).resolve().parents[2]
ORIGINAL = ROOT / "reference/LegoRR.exe"

MAP = """\
 LegoRR

 Timestamp is 6af5563a (Wed Nov 11 22:01:30 2026)

 Preferred load address is 00400000

 Start         Length     Name                   Class
 0001:00000000 00000100H .text                   CODE
 0002:00000000 00000040H .rdata                  DATA
 0003:00000000 00000010H .data                   DATA
 0003:00000010 00000020H .bss                    DATA

  Address         Publics by Value              Rva+Base   Lib:Object

 0001:00000000       _advisor_init              00401000 f advisor.c.obj
 0001:00000040       _game_tick                 00401040 f game.cpp.obj
 0001:00000080       _malloc                    00401080 f libc:malloc.obj
 0001:000000f0       _Sleep@4                   004010f0 f kernel32:KERNEL32.dll
 0001:000000f6       _Direct3DRMCreate@4        004010f6 f d3drm:d3drm.dll
 0002:00000010       _table                     00402010   advisor.c.obj
 0003:00000000       _value                     00403000   game.cpp.obj
 0003:00000010       _pool                      00403010   level.cpp.obj
 0003:00000020       _key_names                 00403020   <common>

 entry point at        0001:00000000

 Static symbols

 0001:00000020       _helper                    00401020 f advisor.c.obj
"""


def entries():
    return link_order.parse(
        "lrr_link(unit   GAME-001 game          game/advisor.c)\n"
        "lrr_link(tail   -        game          game/game.cpp)\n"
        "lrr_link(tail   -        game          game/entity.cpp)\n"
        "lrr_link(tail   -        gods98        gods98/entity.cpp)\n"
        "lrr_link(unit   G98-001  gods98        gods98/x.cpp)\n"
        "lrr_link(tail   -        game          game/x.cpp)\n"
        "lrr_link(tail   -        game          game/native_only.cpp native)\n"
        "lrr_link(legacy 0048e160 game          game/level.cpp)\n")


class ComparisonTests(unittest.TestCase):
    def test_byte_difference_counts_size_difference(self):
        self.assertEqual(pe_audit.byte_difference(b"abcd", b"abXdef"),
                         {"original": 4, "rebuilt": 6, "equal": 3, "differing": 3})
        self.assertEqual(pe_audit.byte_difference(b"", b"")["differing"], 0)

    def test_field_differences(self):
        diff = pe_audit.field_differences({"a": 1, "b": 2, "c": 3}, {"a": 1, "b": 5, "d": 4})
        self.assertEqual(diff, {"b": {"original": 2, "rebuilt": 5},
                                "c": {"original": 3, "rebuilt": None},
                                "d": {"original": None, "rebuilt": 4}})

    def test_compare_imports(self):
        original = [("KERNEL32.dll", ["Sleep", "ExitProcess"]), ("ole32.dll", ["CoInitialize"]),
                    ("DSOUND.dll", ["#1"])]
        rebuilt = [("DSOUND.dll", ["DirectSoundCreate"]), ("KERNEL32.dll", ["Sleep", "ExitProcess"])]
        result = pe_audit.compare_imports(original, rebuilt)
        self.assertFalse(result["dll_order_equal"])
        self.assertFalse(result["common_dll_order_equal"])
        self.assertEqual(result["missing_dlls"], ["ole32.dll"])
        self.assertEqual(result["extra_dlls"], [])
        self.assertEqual((result["original_names"], result["rebuilt_names"]), (4, 3))
        self.assertTrue(result["per_dll"]["kernel32.dll"]["same_order"])
        self.assertEqual(result["per_dll"]["dsound.dll"]["missing"], ["#1"])
        self.assertEqual(result["per_dll"]["dsound.dll"]["extra"], ["DirectSoundCreate"])
        self.assertTrue(pe_audit.compare_imports(original, original)["dll_order_equal"])
        subset = pe_audit.compare_imports(original, [rebuilt[1], rebuilt[0]])
        self.assertFalse(subset["dll_order_equal"])
        self.assertTrue(subset["common_dll_order_equal"])

    def test_compare_resources(self):
        leaf = lambda t, n, lang, sha: {"type": t, "name": n, "lang": lang, "size": 4, "sha256": sha}
        result = pe_audit.compare_resources(
            [leaf(3, 1, 2057, "a"), leaf(5, 101, 1033, "b"), leaf(14, 113, 2057, "c")],
            [leaf(5, 101, 1033, "b"), leaf(14, 113, 1033, "c"), leaf(16, 1, 1033, "d")])
        states = {(r["type"], r["name"]): r["state"] for r in result["leaves"]}
        self.assertEqual(states, {(3, 1): "missing", (5, 101): "identical",
                                  (14, 113): "differs", (16, 1): "extra"})
        self.assertEqual((result["identical"], result["count"]), (1, 4))

    def test_repeated_strings(self):
        data = b"\0NULL\0abc\0NULL\0NULL\0Mode Selection\0\x01\x02xyzw\0"
        result = pe_audit.repeated_strings(data)
        self.assertEqual(result["strings"], 5)
        self.assertEqual(result["distinct"], 3)
        self.assertEqual(result["repeated_distinct"], 1)
        self.assertEqual(result["repeated_extra_copies"], 2)
        self.assertEqual(result["repeated_extra_bytes"], 10)


class MapTests(unittest.TestCase):
    def test_object_kinds(self):
        kinds = pe_audit.object_kinds(entries())
        self.assertEqual(kinds["advisor.c.obj"], "unit")
        self.assertEqual(kinds["entity.cpp.obj"], "tail")
        self.assertEqual(kinds["x.cpp.obj"], "ambiguous")
        self.assertNotIn("native_only.cpp.obj", kinds)
        self.assertEqual(kinds["level.cpp.obj"], "legacy")

    def test_classify_object(self):
        kinds = {"advisor.c.obj": "unit"}
        cases = {"advisor.c.obj": "unit", "libc:malloc.obj": "libc",
                 "kernel32:KERNEL32.dll": "import", "<common>": "common",
                 "<linker-defined>": "linker", "uuid:guid.obj": "library:uuid",
                 "unknown.cpp.obj": "other"}
        for obj, expected in cases.items():
            self.assertEqual(pe_audit.classify_object(obj, kinds), expected, obj)

    def test_parse_and_attribute(self):
        groups, symbols = pe_audit.parse_map(MAP)
        self.assertEqual([g["name"] for g in groups], [".text", ".rdata", ".data", ".bss"])
        self.assertEqual(len(symbols), 10)
        result = pe_audit.attribute_groups(groups, symbols, pe_audit.object_kinds(entries()))
        self.assertEqual(result[".text"]["by_category"],
                         {"unit": 0x40, "tail": 0x40, "libc": 0x70, "import": 0x10})
        self.assertEqual(result[".rdata"]["by_category"], {"unnamed": 0x10, "unit": 0x30})
        self.assertEqual(result[".data"]["by_category"], {"tail": 0x10})
        self.assertEqual(result[".bss"]["by_category"], {"legacy": 0x10, "common": 0x10})
        self.assertEqual(pe_audit.import_thunks(symbols), {"d3drm.dll": 1, "kernel32.dll": 1})


class ImageEndTests(unittest.TestCase):
    def test_constant_covered_once_rebuilt_image_passes_it(self):
        # finding 584: original ends at 0x771000, the rebuild past 0x800000.
        end = pe_audit.image_end_report(0x400000, 0x371000, 0x400000, 0x6d0000)
        self.assertEqual(end["original"], 0x771000)
        self.assertEqual(end["rebuilt"], 0xad0000)
        self.assertEqual(end["covered_constants"], [0x800000])
        self.assertEqual(end["margin_below_first_constant"], 0x800000 - 0xad0000)
        summary = pe_audit.summary({"file_size": {"original": 1, "rebuilt": 1},
                                    "header_differences": {}, "image_end": end,
                                    "section_bytes": {}, "section_virtual_sizes": {},
                                    "imports": {"original_dll_order": [], "rebuilt_dll_order": [],
                                                "original_names": 0, "rebuilt_names": 0,
                                                "missing_dlls": [], "extra_dlls": [],
                                                "dll_order_equal": True,
                                                "common_dll_order_equal": True},
                                    "resources": {"identical": 0, "count": 0, "leaves": []}})
        self.assertIn("ERROR", summary)
        self.assertIn("0x800000", summary)
        self.assertIn("finding 584", summary)
        self.assertEqual(len(pe_audit.image_end_errors(end)), 1)

    def test_no_constant_covered_below_it(self):
        end = pe_audit.image_end_report(0x400000, 0x371000, 0x400000, 0x3f0000)
        self.assertEqual(end["covered_constants"], [])
        self.assertEqual(end["margin_below_first_constant"], 0x800000 - 0x7f0000)
        self.assertEqual(pe_audit.image_end_errors(end), [])

    def test_image_ending_at_the_constant_does_not_cover_it(self):
        end = pe_audit.image_end_report(0x400000, 0x371000, 0x400000, 0x400000)
        self.assertEqual(end["covered_constants"], [])
        end = pe_audit.image_end_report(0x400000, 0x371000, 0x400000, 0x401000)
        self.assertEqual(end["covered_constants"], [0x800000])

    def _pe(self, path: Path, size_of_image: int) -> Path:
        fixture_pe(path)
        data = bytearray(path.read_bytes())
        data[0x98 + 56:0x98 + 60] = size_of_image.to_bytes(4, "little")
        path.write_bytes(data)
        return path

    def test_image_end_only_fails_past_0x800000(self):
        with tempfile.TemporaryDirectory() as tmp:
            orig = self._pe(Path(tmp) / "orig.exe", 0x371000)
            low = self._pe(Path(tmp) / "low.exe", 0x3f5000)
            high = self._pe(Path(tmp) / "high.exe", 0x401000)
            self.assertEqual(pe_audit.pe_image_range(high), (0x400000, 0x401000))
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(pe_audit.main(["--image-end-only", "--original", str(orig),
                                                "--rebuilt", str(low)]), 0)
            err = io.StringIO()
            with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(err):
                self.assertEqual(pe_audit.main(["--image-end-only", "--original", str(orig),
                                                "--rebuilt", str(high)]), 1)
            self.assertIn("0x800000", err.getvalue())
            self.assertIn("finding 584", err.getvalue())

    def test_full_audit_fails_past_0x800000(self):
        with tempfile.TemporaryDirectory() as tmp:
            orig = self._pe(Path(tmp) / "orig.exe", 0x371000)
            high = self._pe(Path(tmp) / "high.exe", 0x401000)
            with contextlib.redirect_stdout(io.StringIO()) as out:
                status = pe_audit.main(["--original", str(orig), "--rebuilt", str(high),
                                        "--map", str(Path(tmp) / "none.map"), "--no-functions"])
            self.assertEqual(status, 1)
            self.assertIn("ERROR", out.getvalue())


class AuditTests(unittest.TestCase):
    def test_identical_images(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / "a.exe"
            fixture_pe(exe)
            result = pe_audit.audit(exe, exe, None, with_functions=False)
            self.assertEqual(result["header_differences"], {})
            self.assertEqual(result["dos_stub_and_headers"]["differing"], 0)
            self.assertTrue(all(b["differing"] == 0 for b in result["section_bytes"].values()))
            json.dumps(result)

    def test_differing_images(self):
        with tempfile.TemporaryDirectory() as tmp:
            a, b = Path(tmp) / "a.exe", Path(tmp) / "b.exe"
            fixture_pe(a)
            fixture_pe(b)
            data = bytearray(b.read_bytes())
            data[0x88:0x8c] = b"\x11\x22\x33\x44"      # TimeDateStamp
            data[0x400:0x404] = b"XXXX"                  # first .text bytes
            b.write_bytes(data)
            result = pe_audit.audit(a, b, None, with_functions=False)
            self.assertIn("TimeDateStamp", result["header_differences"])
            self.assertEqual(result["section_bytes"][".text"]["differing"], 4)
            self.assertIn("TimeDateStamp", pe_audit.summary(result))

    @unittest.skipUnless(ORIGINAL.exists(), "reference/LegoRR.exe is not available")
    def test_original_against_itself(self):
        result = pe_audit.audit(ORIGINAL, ORIGINAL, None, with_functions=False)
        self.assertEqual(result["header_differences"], {})
        self.assertEqual(len(result["imports"]["original_dll_order"]), 11)
        self.assertEqual(result["resources"]["identical"], 3)
        self.assertEqual(result["original_text_by_unit_kind"]["import"], 0x49c7c0 - 0x49b6b0)
        self.assertEqual(sum(result["original_text_by_unit_kind"].values()), 642549)


if __name__ == "__main__":
    unittest.main()
