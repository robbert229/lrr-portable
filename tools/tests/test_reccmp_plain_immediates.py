# SPDX-License-Identifier: LGPL-3.0-or-later
"""Tests for reccmp_plain_immediates.py (#604, finding 584)."""
import contextlib
import io
import json
import struct
import sys
import tempfile
import unittest
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import reccmp_plain_immediates as rpi

try:
    from reccmp.compare.asm.parse import ParseAsm
except ImportError:  # Reccmp lives in the matching-build environment.
    ParseAsm = None

ORIG_END = 0x771000
# The rebuilt global that happens to cover 0x800000.
RUNTIME = range(0x781e20, 0x781e20 + 824320)
# or ecx, 0x800000 ; mov eax, [0x781e20] ; ret
CODE = bytes.fromhex("81c900008000") + bytes.fromhex("a1201e7800") + b"\xc3"


def recomp_is_addr(value):
    return value in RUNTIME


def recomp_lookup(addr, exact=False, indirect=False):
    if addr in RUNTIME:
        return f"s_runtime+{addr - RUNTIME.start:#x} (OFFSET)"
    return None


@dataclass
class _Section:
    virtual_range: range


class _Image:
    def __init__(self, end):
        self.imagebase = 0x400000
        self.sections = [_Section(range(0x401000, end))]


class FakeParse:
    """The ParseAsm surface the module wraps, sanitising every hex value."""

    def __init__(self, addr_test=None, name_lookup=None):
        self.addr_test = addr_test
        self.name_lookup = name_lookup

    def parse_asm(self, data, start_addr):
        out = []
        for value in data:
            if callable(self.addr_test) and self.addr_test(value):
                out.append((start_addr, self.name_lookup(value) or "<OFFSET1>"))
            else:
                out.append((start_addr, f"or ecx, {value:#x}"))
        return out


class Comparator:
    def __init__(self, parse_cls, orig_end=ORIG_END):
        self.orig_bin = _Image(orig_end)
        self.orig_sanitize = parse_cls()
        self.recomp_sanitize = parse_cls(addr_test=recomp_is_addr, name_lookup=recomp_lookup)

    def compare(self, orig, recomp):
        o = self.orig_sanitize.parse_asm(orig, 0x402510)
        r = self.recomp_sanitize.parse_asm(recomp, 0x401000)
        return [t for _, t in o], [t for _, t in r]


class PureTests(unittest.TestCase):
    def test_image_end_from_sections(self):
        self.assertEqual(rpi.image_end(_Image(ORIG_END)), ORIG_END)

    def test_plain_constants_keep_only_values_past_the_image(self):
        listing = [(1, "or ecx, 0x800000"), (2, "mov eax, dword ptr [esp + 0x1c]"),
                   (3, "push 0x4a2000"), (None, "Jump table:"), (4, "")]
        self.assertEqual(rpi.plain_constants(listing, ORIG_END), {0x800000})

    def test_pe_image_range(self):
        header = bytearray(0x200)
        struct.pack_into("<I", header, 0x3C, 0x80)
        struct.pack_into("<I", header, 0x80 + 24 + 28, 0x400000)
        struct.pack_into("<I", header, 0x80 + 24 + 56, 0x371000)
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "a.exe"
            path.write_bytes(bytes(header))
            self.assertEqual(rpi.pe_image_range(path), (0x400000, ORIG_END))


class AttachTests(unittest.TestCase):
    def test_original_constant_stays_plain_on_rebuilt_side(self):
        comparator = Comparator(FakeParse)
        rpi.attach(comparator)
        orig, recomp = comparator.compare([0x800000], [0x800000])
        self.assertEqual(orig, recomp)

    def test_rebuilt_pointer_without_original_constant_is_still_named(self):
        comparator = Comparator(FakeParse)
        rpi.attach(comparator)
        _orig, recomp = comparator.compare([0x4a2000], [0x800000])
        self.assertIn("s_runtime", recomp[0])

    def test_constants_do_not_leak_into_the_next_function(self):
        comparator = Comparator(FakeParse)
        rpi.attach(comparator)
        comparator.compare([0x800000], [0x800000])
        _orig, recomp = comparator.compare([0x4a2000], [0x800000])
        self.assertIn("s_runtime", recomp[0])

    def test_without_attach_the_artifact_appears(self):
        comparator = Comparator(FakeParse)
        orig, recomp = comparator.compare([0x800000], [0x800000])
        self.assertNotEqual(orig, recomp)

    def test_install_wraps_new_comparators_once(self):
        @dataclass
        class Cls:
            orig_bin: object

            def __post_init__(self):
                self.orig_sanitize = FakeParse()
                self.recomp_sanitize = FakeParse(recomp_is_addr, recomp_lookup)

        rpi.install(Cls)
        rpi.install(Cls)
        comparator = Cls(_Image(ORIG_END))
        orig = comparator.orig_sanitize.parse_asm([0x800000], 0)
        recomp = comparator.recomp_sanitize.parse_asm([0x800000], 0)
        self.assertEqual(orig, recomp)

    @unittest.skipIf(ParseAsm is None, "reccmp is not installed")
    def test_with_reccmp_parse_asm(self):
        comparator = Comparator(ParseAsm)
        before_orig, before_recomp = comparator.compare(CODE, CODE)
        self.assertEqual(before_orig[0], "or ecx, 0x800000")
        self.assertIn("s_runtime", before_recomp[0])
        comparator = Comparator(ParseAsm)
        rpi.attach(comparator)
        orig, recomp = comparator.compare(CODE, CODE)
        self.assertEqual(orig[0], recomp[0])
        # A real rebuilt address in the same function is still named.
        self.assertIn("s_runtime", recomp[1])


def _report(orig_text, recomp_text):
    return {"data": [{"address": "0x4667e0", "name": "stats_initialise", "diff": [
        ["@@", [{"both": [["0x4667e0", "push ebx", "0x45f6d0"]]},
                {"orig": [["0x4667e1", orig_text]], "recomp": [["0x45f6d1", recomp_text]]}]]]}]}


class CheckTests(unittest.TestCase):
    def test_symbolized_constant_is_reported(self):
        hits = rpi.symbolized_constants(_report(
            "or dword ptr [esp + 0x1c], 0x800000 \t(entity_stats.c:9)",
            "or dword ptr [esp + 0x1c], s_runtime[771].samples+308 (OFFSET)"), ORIG_END)
        self.assertEqual(len(hits), 1)
        self.assertEqual(hits[0]["address"], "0x4667e0")

    def test_placeholder_is_reported(self):
        hits = rpi.symbolized_constants(_report("or ecx, 0x800000", "or ecx, <OFFSET3>"), ORIG_END)
        self.assertEqual(len(hits), 1)

    def test_real_code_differences_are_not_reported(self):
        for recomp in ("or ecx, 0x400000", "push ebx", "mov dword ptr [esp + 0x34], 0"):
            self.assertEqual(rpi.symbolized_constants(
                _report("or ecx, 0x800000", recomp), ORIG_END), [])

    def test_in_image_original_address_is_not_reported(self):
        self.assertEqual(rpi.symbolized_constants(
            _report("push 0x4a2000", "push s_label (DATA)"), ORIG_END), [])

    def test_check_command(self):
        header = bytearray(0x200)
        struct.pack_into("<I", header, 0x3C, 0x80)
        struct.pack_into("<I", header, 0x80 + 24 + 28, 0x400000)
        struct.pack_into("<I", header, 0x80 + 24 + 56, 0x371000)
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / "a.exe"
            exe.write_bytes(bytes(header))
            report = Path(tmp) / "results.json"
            report.write_text(json.dumps(_report("or ecx, 0x800000", "or ecx, 0x800000")))
            out, err = io.StringIO(), io.StringIO()
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
                self.assertEqual(rpi.main(["check", "--report-dir", tmp, "--original", str(exe)]), 0)
                report.write_text(json.dumps(_report("or ecx, 0x800000", "or ecx, s_x (OFFSET)")))
                self.assertEqual(rpi.main(["check", "--report-dir", tmp, "--original", str(exe)]), 1)
            self.assertIn("OK", out.getvalue())
            self.assertIn("0x4667e0", err.getvalue())


if __name__ == "__main__":
    unittest.main()
