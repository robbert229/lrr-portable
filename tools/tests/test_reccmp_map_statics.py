# SPDX-License-Identifier: LGPL-3.0-or-later
"""Tests for reccmp_map_statics.py (issue #447, finding 643)."""
import struct
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import reccmp_map_statics as rms  # noqa: E402

MAP = """\
  Address         Publics by Value              Rva+Base   Lib:Object

 0001:00000000       _main                      00401000 f main.obj
 0001:00000040       __global_unwind2           00401040 f libc:exsup.obj
 0001:000000a0       __get_fname_caller         004010a0 f libc:fpexcept.obj
 0001:000000d0       _strupr                    004010d0 f libc:strupr.obj
 0001:000000d0       __strupr                   004010d0 f libc:strupr.obj
 0003:00000000       __d_inf                    00403000   libc:fpexcept.obj

 entry point at        0001:00000000

 Static symbols

 0001:00000060       __unwind_handler           00401060 f libc:exsup.obj
 0001:00000080       __get_fname                00401080 f libc:fpexcept.obj
 0001:00000090       _strncnt                   00401090 f libc:aw_cmp.obj
 0001:00000098       _strncnt                   00401098 f libc:aw_map.obj
 0001:000000c0       _local                     004010c0 f game.obj
"""


class ParseMapTest(unittest.TestCase):
    def test_publics_and_statics(self):
        publics, statics = rms.parse_map(MAP)
        self.assertEqual(publics[1], (0x401040, "__global_unwind2", "libc:exsup.obj", True))
        self.assertEqual(publics[5], (0x403000, "__d_inf", "libc:fpexcept.obj", False))
        self.assertEqual([s[1] for s in statics],
                         ["__unwind_handler", "__get_fname", "_strncnt", "_strncnt", "_local"])

    def test_library_statics_are_unique_libc_functions(self):
        statics = rms.library_statics(MAP)
        # _strncnt is static in two members, _local is not LIBC's; the
        # oldnames alias pair shares one address.
        self.assertEqual(sorted(statics), ["__get_fname", "__strupr", "__unwind_handler",
                                           "_strupr"])
        self.assertEqual(statics["__strupr"], statics["_strupr"])
        # Sizes run to the next symbol of either kind.
        self.assertEqual(statics["__unwind_handler"], (0x401060, 0x20))
        self.assertEqual(statics["__get_fname"], (0x401080, 0x10))

    def test_member_functions(self):
        functions = rms.library_member_functions(MAP)
        self.assertEqual(functions["fpexcept.obj"],
                         {"__get_fname_caller": 0x4010a0, "__get_fname": 0x401080})


class FakeEntity:
    def __init__(self, recomp_addr=None, orig_addr=None, size=None, **attrs):
        self.recomp_addr = recomp_addr
        self.orig_addr = orig_addr
        self._size = size
        self.attrs = attrs

    def get(self, key):
        return self.attrs.get(key)

    def size(self, image_id):
        return self._size if image_id == "recomp" else None


class FakeBatch:
    def __init__(self, db):
        self.db = db

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        return False

    def set(self, image_id, addr, **values):
        self.db.sets.append((image_id, addr, values))


class FakeDb:
    def __init__(self, orig, recomp):
        self.orig, self.recomp, self.sets = orig, recomp, []

    def unmatched(self, image_id):
        return self.orig if image_id == "orig" else self.recomp

    def get_all(self):
        return self.orig + self.recomp

    def batch(self):
        return FakeBatch(self)


IDS = SimpleNamespace(ORIG="orig", RECOMP="recomp")


class AddStaticsTest(unittest.TestCase):
    def test_only_asked_and_unknown_symbols_are_added(self):
        db = FakeDb(orig=[FakeEntity(orig_addr=0x48ef30, symbol="__unwind_handler"),
                          FakeEntity(orig_addr=0x4939d0, symbol="__get_fname")],
                    recomp=[FakeEntity(recomp_addr=0x401080, symbol="__get_fname")])
        statics = rms.library_statics(MAP)
        added = rms.add_statics(db, statics, IDS, "FUNCTION")
        self.assertEqual(added, ["__unwind_handler"])
        self.assertIn(("recomp", 0x401060, {"type": "FUNCTION", "name": "__unwind_handler",
                                            "symbol": "__unwind_handler", "size": 0x20}),
                      db.sets)

    def test_a_public_that_runs_over_the_static_ends_there(self):
        # __global_unwind2's PDB size covers exsup.obj's whole contribution.
        db = FakeDb(orig=[FakeEntity(orig_addr=0x48ef30, symbol="__unwind_handler")],
                    recomp=[FakeEntity(recomp_addr=0x401040, size=0x42,
                                       symbol="__global_unwind2")])
        rms.add_statics(db, rms.library_statics(MAP), IDS, "FUNCTION")
        self.assertIn(("recomp", 0x401040, {"size": 0x20}), db.sets)

    def test_data_is_added_with_its_size(self):
        db = FakeDb(orig=[FakeEntity(orig_addr=0x4b2170, symbol="_names")], recomp=[])
        added = rms.add_statics(db, {}, IDS, "FUNCTION", {"_names": (0x403100, 0xd8)}, "DATA")
        self.assertEqual(added, ["_names"])
        self.assertEqual(db.sets, [("recomp", 0x403100, {"type": "DATA", "name": "_names",
                                                         "symbol": "_names", "size": 0xd8})])


class StaticDataTest(unittest.TestCase):
    """A function of the member refers to the static through DIR32."""

    def coff(self, reloc_symbol):
        text = SimpleNamespace(index=2, is_code=True, size=16,
                               data=b"\xb8" + struct.pack("<I", 0x10) + b"\x90" * 11,
                               relocations=[SimpleNamespace(offset=1, symbol=reloc_symbol,
                                                            type=rms.IMAGE_REL_I386_DIR32)])
        data = SimpleNamespace(index=1, is_code=False, size=0x20, data=b"", relocations=[])
        symbols = {
            0: SimpleNamespace(name=".data", section=1, value=0, is_function=False),
            2: SimpleNamespace(name="_names", section=1, value=0x10, is_function=False),
            3: SimpleNamespace(name="_tail", section=1, value=0x18, is_function=False),
            4: SimpleNamespace(name="__get_fname", section=2, value=0, is_function=True),
        }
        sections = [data, text]
        return SimpleNamespace(symbols=symbols, sections=sections,
                               section=lambda i: sections[i - 1])

    def read(self, va, size):
        # The rebuilt __get_fname at 0x401080 loads 0x403110.
        self.assertEqual((va, size), (0x401081, 4))
        return struct.pack("<I", 0x403110)

    def test_section_relative_reference(self):
        members = [({"__get_fname": 0x401080}, self.coff(reloc_symbol=0))]
        # .data+0x10 -> .data starts at 0x403100; _names is at +0x10.
        self.assertEqual(rms.static_data({"_names"}, members, self.read),
                         {"_names": (0x403110, 8)})

    def test_symbol_reference(self):
        coff = self.coff(reloc_symbol=2)
        coff.sections[1].data = b"\xb8" + struct.pack("<I", 0) + b"\x90" * 11
        members = [({"__get_fname": 0x401080}, coff)]
        self.assertEqual(rms.static_data({"_names"}, members, self.read),
                         {"_names": (0x403110, 8)})


class InstallTest(unittest.TestCase):
    def test_wrapping_twice_wraps_once(self):
        calls = []

        def match_symbols(db, *args, **kwargs):
            calls.append(db)

        once = rms.wrap_match_symbols(match_symbols, dict, IDS, "FUNCTION")
        self.assertIs(rms.wrap_match_symbols(once, dict, IDS, "FUNCTION"), once)
        once(FakeDb([], []))
        self.assertEqual(len(calls), 1)


if __name__ == "__main__":
    unittest.main()
