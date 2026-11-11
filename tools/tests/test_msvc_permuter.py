# SPDX-License-Identifier: LGPL-3.0-or-later
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import msvc_permuter as perm


class RewriteTests(unittest.TestCase):
    def variants(self, body, c=True):
        source = '// FUNCTION: LEGORR 0x1234\nvoid f(void) {\n' + body + '\n}\n'
        return list(perm.rewrites(source, 0x1234, c))

    def test_comparison(self):
        result = self.variants('if (value < 3) return;')
        self.assertTrue(any('if (3 > value)' in s for _, s in result))
        self.assertFalse(self.variants('if (value < 3 + offset) return;'))
        self.assertFalse(self.variants('if (value < 3) return;', False))
        self.assertFalse(self.variants('/* if (value < 3) */'))

    def test_loop_and_do_while(self):
        self.assertTrue(any('for (;x;)' in s for _, s in self.variants('while (x) x--;')))
        self.assertFalse(any(k == 'loop' for k, _ in self.variants('do x--;\nwhile (x);')))

    def test_declarations(self):
        result = self.variants('int a;\nlong b;\n')
        self.assertTrue(any('long b;\nint a;' in s for _, s in result))
        self.assertFalse(any(k == 'declarations' for k, _ in self.variants('int a = 1;\nint b;')))
        result = self.variants('float a, b, c;')
        self.assertTrue(any('float b, a, c;' in s for _, s in result))
        self.assertFalse(any(k == 'declarators' for k, _ in self.variants('float a = 1, b;')))

    def test_comparison_in_short_circuit_expression(self):
        result = self.variants('if (x != 0 && y) return;')
        self.assertTrue(any('0 != x && y' in s for _, s in result))
        self.assertFalse(self.variants('if (x + y < 3) return;'))

    def test_only_target_and_braces_in_strings(self):
        text = '// FUNCTION: LEGORR 0x1234\nvoid f() { char *s = "}"; if (x == 0) x++; }\n// FUNCTION: LEGORR 0x1235\nvoid g() { if (y == 0) y++; }'
        variants = list(perm.rewrites(text, 0x1234))
        self.assertEqual(len(variants), 1)
        self.assertIn('if (y == 0)', variants[0][1])


if __name__ == '__main__':
    unittest.main()
