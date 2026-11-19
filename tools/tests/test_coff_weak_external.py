# SPDX-License-Identifier: LGPL-3.0-or-later
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import coff_weak_external
import msvc_coff


class WeakExternalObjectTest(unittest.TestCase):
    def test_weak_external_names_its_default(self):
        data = coff_weak_external.build(
            [("??3@YAXPAX@Z", "_ui_render_record_free")])
        obj = msvc_coff.parse_coff(data)
        self.assertEqual(obj.sections, [])
        default, weak = (obj.symbols[i] for i in sorted(obj.symbols))
        self.assertEqual(default.name, "_ui_render_record_free")
        self.assertTrue(default.is_external and default.is_undefined)
        self.assertEqual(weak.name, "??3@YAXPAX@Z")
        self.assertEqual(weak.storage_class, 105)
        tag, flags = struct.unpack_from("<II", weak.aux[0])
        self.assertEqual(tag, 0)  # the default's symbol index
        self.assertEqual(flags, 1)  # IMAGE_WEAK_EXTERN_SEARCH_NOLIBRARY

    def test_shared_default_is_written_once(self):
        data = coff_weak_external.build([("_a", "_c"), ("_b", "_c")])
        symbols = msvc_coff.parse_coff(data).symbols
        names = [symbols[i].name for i in sorted(symbols)]
        self.assertEqual(names, ["_c", "_a", "_b"])

    def test_committed_object_is_the_tools_output(self):
        # CMakeLists.txt links this committed copy (ReproBit imports it as a
        # source input); it must stay exactly what the tool writes.
        committed = Path(__file__).resolve().parents[2] / "cross" / "msvc5-importlibs" / \
            "weak_operator_delete.obj"
        self.assertEqual(committed.read_bytes(), coff_weak_external.build(
            [("??3@YAXPAX@Z", "_ui_render_record_free")]))


if __name__ == "__main__":
    unittest.main()
