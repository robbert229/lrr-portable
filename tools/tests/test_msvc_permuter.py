# SPDX-License-Identifier: LGPL-3.0-or-later
import sys
import subprocess
import tempfile
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

    def test_independent_statement_order(self):
        result = self.variants('int a;\nint b;\na = 1;\nb = 2;\n')
        self.assertTrue(any(k == 'statements' and 'b = 2;\na = 1;' in s for k, s in result))
        self.assertFalse(any(k == 'statements' for k, _ in self.variants('int a;\nint b;\na = 1;\nb = a;\n')))
        self.assertFalse(any(k == 'statements' for k, _ in self.variants('int a;\nint b;\na = foo();\nb = 2;\n')))

    def test_temporary_introduction_and_removal(self):
        result = self.variants('int a;\nreturn a;')
        self.assertTrue(any(k == 'temporary-introduce' for k, _ in result))
        result = self.variants('int a;\n{ int tmp = a; return tmp; }')
        self.assertTrue(any(k == 'temporary-remove' for k, _ in result))
        self.assertFalse(any(k == 'temporary-remove' for k, _ in self.variants('int a;\n{ char tmp = a; return tmp; }')))
        source = '// FUNCTION: LEGORR 0x1234\nint f(int x) { return x + 1; }'
        self.assertTrue(any(k == 'return-temporary' and '= (x + 1)' in s for k, s in perm.rewrites(source, 0x1234)))
        self.assertFalse(any(k == 'return-temporary' for k, _ in perm.rewrites(source.replace('int f', 'float f'), 0x1234)))

    def test_unsigned_arithmetic(self):
        result = self.variants('unsigned int a, b, c;\nreturn (a + b) + c;')
        self.assertTrue(any(k == 'association' and 'a + (b + c)' in s for k, s in result))
        result = self.variants('unsigned int a, b;\nreturn a * b;')
        self.assertTrue(any(k == 'commutation' and 'b * a' in s for k, s in result))
        self.assertFalse(any(k in ('association', 'commutation') for k, _ in self.variants('int a, b;\nreturn a + b;')))
        self.assertFalse(any(k in ('association', 'commutation') for k, _ in self.variants('float a, b;\n{ unsigned int a, b; }\nreturn a + b;')))

    def test_for_while_and_do(self):
        self.assertTrue(any(k == 'for-to-while' for k, _ in self.variants('for (; x;) { continue; }')))
        self.assertTrue(any(k == 'while-to-do' for k, _ in self.variants('while (x) { x--; }')))
        result = self.variants('do { x--; if (x) continue; } while (x);')
        self.assertTrue(any(k == 'do-to-for' and '_perm_first' in s for k, s in result))
        self.assertFalse(any(k == 'do-to-for' for k, _ in self.variants('do { goto label; } while (x);')))

    def test_do_while_continue_executes_update(self):
        source = ('// FUNCTION: LEGORR 0x1234\nint f(int x) { int n = 0; '
                  'do { x--; if (x == 2) continue; n++; } while (x); return n; }\n'
                  'int main(void) { return f(4) != 3; }\n')
        candidate = next(s for k, s in perm.rewrites(source, 0x1234) if k == 'do-to-for')
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            for index, text in enumerate((source, candidate)):
                (path / 'test.c').write_text(text)
                subprocess.run(['cc', '-std=gnu89', str(path / 'test.c'), '-o', str(path / 'test')], check=True, capture_output=True)
                subprocess.run([str(path / 'test')], check=True, cwd=path)

    def test_only_target_and_braces_in_strings(self):
        text = '// FUNCTION: LEGORR 0x1234\nvoid f() { char *s = "}"; if (x == 0) x++; }\n// FUNCTION: LEGORR 0x1235\nvoid g() { if (y == 0) y++; }'
        variants = list(perm.rewrites(text, 0x1234))
        self.assertEqual(len(variants), 1)
        self.assertIn('if (y == 0)', variants[0][1])


if __name__ == '__main__':
    unittest.main()
