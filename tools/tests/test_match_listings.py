# SPDX-License-Identifier: LGPL-3.0-or-later
import json
from pathlib import Path
import sys
import subprocess
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import match_listings as listings


class ListingTest(unittest.TestCase):
    def test_classifications(self):
        a = listings.fingerprint(['mov eax, dword ptr [esp + 4]', 'add eax, 3', 'jne 0x12'])
        same = listings.fingerprint(['mov ecx, dword ptr [esp + 0xc]', 'add ecx, 3', 'jne 0x20'])
        reordered = listings.fingerprint(['add edx, 3', 'mov edx, dword ptr [esp + 8]', 'jne 0x90'])
        extra = listings.fingerprint(['mov ecx, dword ptr [esp + 8]', 'add ecx, 3', 'fxch st(1)', 'jne 0x90'])
        self.assertEqual(listings.classify(a, same, False), ('drift', 0))
        self.assertEqual(listings.classify(a, reordered, False), ('drift-reordered', 0))
        self.assertEqual(listings.classify(a, extra, False), ('drift-review', 1))
        self.assertEqual(listings.classify(a, extra, True), ('source-changed', 1))
        self.assertEqual(listings.classify(a, same, None), ('source-unknown', 0))
        self.assertNotEqual(listings.fingerprint(['add eax, 3']), listings.fingerprint(['add eax, 4']))
        self.assertNotEqual(listings.fingerprint(['call function_a']), listings.fingerprint(['call function_b']))

    def test_symbolic_operands_preserve_shape_and_opcode(self):
        a = listings.fingerprint(['fsub dword ptr [1.0 (FLOAT)]'])
        self.assertEqual(a, listings.fingerprint(['fsub dword ptr [<OFFSET1>]']))
        self.assertNotEqual(a, listings.fingerprint(['fadd dword ptr [<OFFSET1>]']))
        self.assertNotEqual(a, listings.fingerprint(['fsub qword ptr [<OFFSET1>]']))
        self.assertNotEqual(a, listings.fingerprint(['fsub <OFFSET1>']))

    def test_anonymous_namespace_and_numeric_offset(self):
        a = ['mov eax, dword ptr [?A0x1b2c3d4e::value (DATA)]',
             'push 440 (OFFSET)']
        b = ['mov eax, dword ptr [?A0x98765432::value (DATA)]',
             'push <OFFSET1>']
        self.assertEqual(listings.fingerprint(a), listings.fingerprint(b))
        self.assertNotEqual(listings.fingerprint(a), listings.fingerprint([
            'mov eax, ?A0x98765432::value (DATA)', 'push <OFFSET1>']))

    def test_msvc_mangled_static_offset(self):
        prefix = 'fld dword ptr [eax*8 + ?s_weapon@?%z:/src/weapon.cpp'
        suffix = '@lego@@3PAUWeapon@2@A[0].damage (OFFSET)]'
        self.assertEqual(listings.normalize(prefix + '316842078' + suffix),
                         listings.normalize(prefix + '104802270' + suffix))
        self.assertEqual(listings.normalize(prefix + '316842078' + suffix),
                         listings.normalize('fld dword ptr [eax*8 + <OFFSET3>]'))

    def test_old_digest_format_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'old'
            path.write_text('# input_sha256: ' + 'a' * 64 + '\n')
            with self.assertRaisesRegex(ValueError, 'format'):
                listings.load(path)

    def test_library_marker(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            subprocess.run(['git', 'init', '-q', tmp], check=True)
            (root / 'src').mkdir()
            (root / 'src/a.c').write_text('// LIBRARY: LEGORR 0x00497ec0 SYMBOL\n')
            subprocess.run(['git', '-C', tmp, 'add', 'src/a.c'], check=True)
            self.assertEqual(listings.source_index(root=root)[0x497ec0], '<library>')

    def test_historical_listings(self):
        fixtures = json.loads((Path(__file__).parent / 'fixtures/match-listings-history.json').read_text())
        for case in fixtures:
            with self.subTest(pr=case['pr'], address=case['address']):
                a = listings.fingerprint(case['before'])
                b = listings.fingerprint(case['after'])
                # Counts describe the whole disassembly; the fixture contains only
                # the real differing excerpt, not hundreds of unrelated lines.
                a = (case['base_count'], *a[1:])
                b = (case['head_count'], *b[1:])
                self.assertEqual(listings.classify(a, b, case['source_changed']),
                                 (case['classification'], case['head_count'] - case['base_count']))

    def test_source_comparison_follows_moves_and_detects_changes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            subprocess.run(['git', 'init', '-q', str(root)], check=True)
            (root / 'src').mkdir()
            source = '// FUNCTION: LEGORR 0x00446610\nint f(void) { return 1; }\n'
            (root / 'src/a.c').write_text(source)
            subprocess.run(['git', '-C', tmp, 'add', 'src/a.c'], check=True)
            subprocess.run(['git', '-C', tmp, '-c', 'user.name=test', '-c', 'user.email=test@example.com', 'commit', '-qm', 'base'], check=True)
            before = listings.source_index('HEAD', root)
            (root / 'src/a.c').unlink()
            (root / 'src/b.c').write_text(source)
            subprocess.run(['git', '-C', tmp, 'add', '-A'], check=True)
            self.assertEqual(before[0x446610], listings.source_index(root=root)[0x446610])
            (root / 'src/b.c').write_text(source.replace('return 1', 'return 2'))
            self.assertNotEqual(before[0x446610], listings.source_index(root=root)[0x446610])

    def test_report_and_sidecar(self):
        row = {'address': '0x401000', 'diff': [['@@', [{'both': [['0x401000', 'push eax', '0x500000']]}, {'orig': [['0x401001', 'ret']], 'recomp': [['0x500001', 'pop eax'], ['0x500002', 'ret']]}]]]}
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp)
            for _, filename in listings.SCOPES:
                (path / filename).write_text(json.dumps({'data': [row]}))
            actual = listings.report_listings(path)
            self.assertEqual(actual['full'][0x401000][0], 3)
            sidecar = path / 'digests'
            sidecar.write_text(listings.render(actual, 'a' * 64))
            self.assertEqual(actual, listings.load(sidecar))
            row.pop('diff')
            (path / 'results.json').write_text(json.dumps([row]))
            with self.assertRaisesRegex(ValueError, 'complete diff'):
                listings.report_listings(path)


if __name__ == '__main__':
    unittest.main()
