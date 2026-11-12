# SPDX-License-Identifier: LGPL-3.0-or-later
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import reshape_check as rc


def obj(data=b'\x90', symbol=b'foo', reloc_type=6, debug=b'one'):
    # Two sections, a relocation and one external symbol; all offsets are
    # deliberately constructed without relying on the parser under test.
    header = struct.pack('<HHIIIHH', 0x14c, 2, 0, 20 + 80 + len(data) + len(debug) + 10, 1, 0, 0)
    text = struct.pack('<8sIIIIIIHHI', b'.text', 0, 0, len(data), 100, 100 + len(data) + len(debug), 0, 1, 0, 0x20)
    dbg = struct.pack('<8sIIIIIIHHI', b'.debug$S', 0, 0, len(debug), 100 + len(data), 0, 0, 0, 0, 0x40)
    relocation = struct.pack('<IIH', 0, 0, reloc_type)
    sym = struct.pack('<8sIhHBB', symbol, 0, 1, 0, 2, 0)
    return header + text + dbg + data + debug + relocation + sym + struct.pack('<I', 4)


class CoffTests(unittest.TestCase):
    def test_identical_ignores_debug(self):
        self.assertEqual(rc.compare_objects(obj(), obj(debug=b'different')), [])

    def test_each_object_difference(self):
        for other, expected in [(obj(data=b'\xcc'), 'bytes'), (obj(reloc_type=20), 'relocations'), (obj(symbol=b'bar'), 'symbol table')]:
            with self.subTest(expected=expected):
                self.assertIn(expected, '; '.join(rc.compare_objects(obj(), other)))

    def test_rename_maps_base_names(self):
        self.assertIn('symbol table', '; '.join(rc.compare_objects(obj(symbol=b'old'), obj(symbol=b'new'))))
        self.assertEqual(rc.compare_objects(obj(symbol=b'old'), obj(symbol=b'new'), renames={'old': 'new'}), [])
        # Whole identifiers only: a longer name is not renamed.
        self.assertIn('symbol table', '; '.join(rc.compare_objects(obj(symbol=b'old_x'), obj(symbol=b'new_x'), renames={'old': 'new'})))

    def test_rename_identifier_in_mangled_name(self):
        self.assertEqual(rc.rename_identifier('?s_old@@3PAHA', {'s_old': 's_new'}), '?s_new@@3PAHA')
        self.assertEqual(rc.rename_identifier('?s_old_x@@3HA', {'s_old': 's_new'}), '?s_old_x@@3HA')
        # File statics are `_name$S<n>`, C names `_name`.
        self.assertEqual(rc.rename_identifier('_s_old$S46642', {'s_old': 's_new'}), '_s_new$S46642')
        self.assertEqual(rc.rename_identifier('_s_old', {'s_old': 's_new'}), '_s_new')
        self.assertEqual(rc.rename_identifier('_s_old_x$S1', {'s_old': 's_new'}), '_s_old_x$S1')
        self.assertEqual(rc.rename_identifier('_x_s_old', {'s_old': 's_new'}), '_x_s_old')


class TokenTests(unittest.TestCase):
    def test_markers_and_tokens(self):
        source = Path('/tmp/source.cpp')
        text = '# 1 "/tmp/source.cpp"\nint x = 1 + 2;\n# 1 "/tmp/header.h" 1\nint hidden;\n#line 3 "/tmp/source.cpp"\nconst char *s = "a b";\n'
        self.assertEqual(rc.tokens_from_file(text, source), ['int', 'x', '=', '1', '+', '2', ';', 'const', 'char', '*', 's', '=', '"a b"', ';'])

    def test_missing_marker_fails(self):
        with self.assertRaises(RuntimeError):
            rc.tokens_from_file('# 1 "other.h"\nint x;\n', Path('/tmp/source.cpp'))

    def test_strip_includes_preserves_lines(self):
        source = b'#include "one.h"\n#include \\\n "two.h"\nint x;\n'
        stripped, includes = rc.strip_includes(source)
        self.assertEqual(stripped, b'\n\n\nint x;\n')
        self.assertEqual(len(includes), 2)

    @unittest.skipUnless(shutil.which('g++'), 'host g++ unavailable')
    def test_emulated_reference_defines(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / 'fixture.cpp'
            source.write_text("#if defined(__i386__) && defined(_WIN32) && defined(__MINGW32__) && defined(WIN32) && defined(_X86_) && defined(LEGORRR_BACKEND_WINAPI) && !defined(__x86_64__) && !defined(__linux__) && !defined(__unix__)\nint reference;\n#else\nint wrong;\n#endif\n")
            self.assertEqual(rc.tokens_from_file(rc.preprocess_reference(source), source), ['int', 'reference', ';'])


class CompileDatabaseTests(unittest.TestCase):
    def test_relative_file_resolves_against_directory(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp).resolve()
            (root / 'src').mkdir()
            (root / 'build').mkdir()
            source = root / 'src' / 'unit.cpp'
            source.write_text('int x;\n')
            entry = {'directory': str(root / 'build'), 'file': '../src/unit.cpp',
                     'arguments': ['c++', '-c', '../src/unit.cpp', '-o', 'unit.o']}
            self.assertIs(rc.entry_for([entry], source), entry)


class IntegrationTests(unittest.TestCase):
    def test_same_revision(self):
        toolchain = Path(os.environ.get('REPROBIT_MSVC_5_0_RTM_ROOT') or Path.home() / '.local/share/reprobit/toolchains/msvc_5_0_rtm')
        if not (toolchain / 'wine/x86/cl').exists() or not (shutil.which('wine') or shutil.which('distrobox')):
            self.skipTest('matching toolchain unavailable')
        proc = subprocess.run([sys.executable, str(rc.ROOT / 'tools/reshape_check.py'), 'src/game/select.cpp', '--base', 'HEAD', '--head', 'HEAD', '--json'], cwd=rc.ROOT, capture_output=True, text=True)
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
        import json
        checks = json.loads(proc.stdout)['results'][0]['checks']
        self.assertTrue(checks['native'].startswith('identical'))
        self.assertTrue(checks['matching'].startswith('identical'))
        self.assertTrue(checks['object'].startswith('identical'))
        self.assertTrue(checks['mingw'].startswith('identical'))


if __name__ == '__main__':
    unittest.main()
