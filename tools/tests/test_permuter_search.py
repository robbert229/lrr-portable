# SPDX-License-Identifier: LGPL-3.0-or-later
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import permuter_search as ps  # noqa: E402

HEAD = '// FUNCTION: LEGORR 0x1234\n'


def fn(body, signature='int f(int x)'):
    return HEAD + signature + '\n{\n' + body + '}\n'


def candidates(mode, text, c=True, phase='single', exhaustive=6):
    out = []
    for group in ps.groups_for(mode, text, 0x1234, c, exhaustive, phase):
        out.extend((group.label, desc, cand) for desc, cand in group.candidates(text))
    return out


def run_c(sources, driver):
    """Compile each source with a driver main and return their outputs."""
    if shutil.which('cc') is None:
        raise unittest.SkipTest('no host C compiler')
    outputs = []
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory)
        for text in sources:
            (path / 't.c').write_text(text + driver)
            subprocess.run(['cc', '-std=gnu89', '-w', str(path / 't.c'), '-o', str(path / 't')], check=True, capture_output=True)
            outputs.append(subprocess.run([str(path / 't')], check=True, capture_output=True, text=True).stdout)
    return outputs


class StoreTests(unittest.TestCase):
    def test_independent_stores_are_permuted_exhaustively(self):
        text = fn('    int a;\n    int b;\n    int c;\n    a = x;\n    b = 2;\n    c = 3;\n    return a + b + c;\n')
        found = candidates('stores', text)
        self.assertEqual(len(found), 5)  # 3! - 1
        for _, _, cand in found:
            self.assertEqual(cand.count('\n'), text.count('\n'))
        self.assertTrue(any('    c = 3;\n    b = 2;\n    a = x;\n' in c for _, _, c in found))

    def test_dependences_keep_their_order(self):
        # b reads a; c writes the same field as d; the call is a barrier.
        text = fn('    int a;\n    int b;\n    a = x;\n    b = a;\n    return b;\n')
        self.assertEqual(candidates('stores', text), [])
        text = fn('    p->f = 1;\n    q->f = 2;\n    return 0;\n', 'int f(S *p, S *q)')
        self.assertEqual(candidates('stores', text), [])
        text = fn('    p->f = 1;\n    q->g = 2;\n    return 0;\n', 'int f(S *p, S *q)')
        self.assertEqual(len(candidates('stores', text)), 1)
        text = fn('    int a;\n    int b;\n    a = 1;\n    g(a);\n    b = 2;\n    return b;\n')
        self.assertEqual(candidates('stores', text), [])
        # A global may be the pointee of *p; a local whose address escapes too.
        text = fn('    g_value = 1;\n    *p = 2;\n    return 0;\n', 'int f(int *p)')
        self.assertEqual(candidates('stores', text), [])
        text = fn('    int a;\n    int *p = &a;\n    a = 1;\n    p[0] = 2;\n    return a;\n')
        self.assertEqual(candidates('stores', text), [])
        text = fn('    int a;\n    a = 1;\n    *p = 2;\n    return a;\n', 'int f(int *p)')
        self.assertEqual(len(candidates('stores', text)), 1)

    def test_declarations_and_unbraced_bodies_are_not_statements(self):
        self.assertEqual(candidates('stores', fn('    int a = 1;\n    int b = 2;\n    return a + b;\n')), [])
        text = fn('    int a;\n    int b;\n    if (x)\n        a = 1;\n    b = 2;\n    return a;\n')
        self.assertEqual(candidates('stores', text), [])
        text = fn('    int a;\n    int b;\n    a = 1 +\n        x;\n    b = 2;\n    return a;\n')
        self.assertEqual(candidates('stores', text), [])

    def test_insertion_moves_above_the_exhaustive_limit(self):
        stores = ''.join(f'    s->f{i} = {i};\n' for i in range(5))
        text = fn(stores + '    return 0;\n', 'int f(S *s)')
        self.assertEqual(len(candidates('stores', text, exhaustive=6)), 119)
        self.assertEqual(len(candidates('stores', text, exhaustive=3)), 16)  # 5*4 moves, 4 duplicates

    def test_store_orders_preserve_behaviour(self):
        text = ('struct S { int a, b, c; };\n' + HEAD + 'int f(struct S *s, int x)\n{\n    int t;\n'
                '    s->a = x;\n    t = x * 3;\n    s->b = t;\n    s->c = s->a + 1;\n    x++;\n    return x + t;\n}\n')
        found = candidates('stores', text)
        self.assertTrue(found)
        driver = ('\n#include <stdio.h>\nint main(void) { struct S s; int r = f(&s, 5); '
                  'printf("%d %d %d %d\\n", r, s.a, s.b, s.c); return 0; }\n')
        outputs = run_c([text] + [c for _, _, c in found], driver)
        self.assertEqual(len(set(outputs)), 1)


SWITCH = '''    int r = 0;
    switch (x)
    {
    case 1:
        r = 10;
        break;
    case 2:
    case 3:
        r = 20;
        break;
    case 4:
        r = 30;
    case 5:
        r += 40;
        break;
    default:
        r = 50;
        return r;
    case 7:
        r = 70;
    }
    return r;
'''


class CaseTests(unittest.TestCase):
    def test_blocks_fallthrough_and_pinned_last(self):
        text = fn(SWITCH)
        groups = ps.groups_for('cases', text, 0x1234, True, 6)
        self.assertEqual(len(groups), 1)
        group = groups[0]
        # case 1 | case 2/3 | case 4 falling into 5 | default | case 7 (pinned).
        self.assertEqual(len(group.items), 5)
        self.assertTrue(group.pinned_last)
        self.assertEqual(group.size(), 23)  # 4! - 1
        self.assertTrue(group.items[2].lstrip().startswith('case 4:') and 'case 5:' in group.items[2])
        self.assertTrue(group.items[1].lstrip().startswith('case 2:') and 'case 3:' in group.items[1])

    def test_case_orders_preserve_behaviour(self):
        text = fn(SWITCH)
        found = candidates('cases', text)
        driver = ('\n#include <stdio.h>\nint main(void) { int i; for (i = 0; i < 9; i++) printf("%d ", f(i)); '
                  'return 0; }\n')
        outputs = run_c([text] + [c for _, _, c in found], driver)
        self.assertEqual(len(set(outputs)), 1)

    def test_refusals(self):
        # A label after other text, and a label inside a nested block.
        text = fn('    switch (x)\n    {\n    case 1: x++; break; case 2:\n        break;\n    case 3:\n        break;\n    }\n    return x;\n')
        self.assertEqual(candidates('cases', text), [])
        text = fn('    switch (x)\n    {\n    case 1:\n        if (x) {\n    case 2:\n            x++;\n        }\n        break;\n'
                  '    case 3:\n        break;\n    }\n    return x;\n')
        self.assertEqual(candidates('cases', text), [])

    def test_braced_case_bodies_and_conditional_breaks(self):
        text = fn('    switch (x)\n    {\n    case 1:\n    {\n        x++;\n        break;\n    }\n    case 2:\n        if (x) break;\n'
                  '    case 3:\n        x--;\n        break;\n    }\n    return x;\n')
        group = ps.groups_for('cases', text, 0x1234, True, 6)[0]
        self.assertEqual(len(group.items), 2)  # case 2's conditional break falls into case 3


class DeclarationTests(unittest.TestCase):
    def test_lines_and_declarators(self):
        text = fn('    int a;\n    char *p, q[4];\n    long n = 3;\n    a = 1;\n    return a;\n')
        found = candidates('decls', text)
        labels = {label.split('@')[0] for label, _, _ in found}
        self.assertEqual(labels, {'decls', 'declarators'})
        self.assertTrue(any('char q[4], *p;' in c for _, _, c in found))
        self.assertEqual(sum(label.startswith('decls') for label, _, _ in found), 5)

    def test_initializer_dependences(self):
        text = fn('    int a = x;\n    int b = a + 1;\n    return b;\n')
        self.assertEqual(candidates('decls', text), [])
        text = fn('    int a = g();\n    int b = h();\n    int c;\n    return a + b;\n')
        found = [c for label, _, c in candidates('decls', text)]
        self.assertTrue(all(c.index('int a') < c.index('int b') for c in found))
        self.assertEqual(len(found), 2)

    def test_only_at_block_start_and_cpp_scalars(self):
        text = fn('    int a;\n    a = 1;\n    int b;\n    int c;\n    return a;\n')
        self.assertEqual(candidates('decls', text), [])
        text = fn('    Thing t;\n    Other o;\n    int a;\n    int *p;\n    return 0;\n')
        self.assertEqual(len(candidates('decls', text, c=True)), 23)
        found = candidates('decls', text, c=False)
        self.assertEqual(found, [])  # constructors may run in any order only in C
        text = fn('    int a;\n    int *p;\n    return 0;\n')
        self.assertEqual(len(candidates('decls', text, c=False)), 1)


class OperandTests(unittest.TestCase):
    def sites(self, body, signature='int f(int a, int b, int c)', c=True):
        return {label: cand[cand.index('{\n') + 2:-2] for label, _, cand in candidates('operands', fn(body, signature), c)}

    def test_commutation_mirror_and_parentheses(self):
        found = self.sites('    return a - b + c * 2;\n')
        self.assertIn('    return c * 2 + (a - b);\n', found.values())
        self.assertIn('    return a - b + 2 * c;\n', found.values())
        found = self.sites('    if (a < b) return 1;\n    return 0;\n')
        self.assertIn('    if (b > a) return 1;\n    return 0;\n', found.values())
        self.assertFalse(self.sites('    if (a < b) return 1;\n    return 0;\n', c=False))

    def test_unsafe_operands_are_left_alone(self):
        self.assertFalse(self.sites('    return a++ + b;\n'))
        self.assertFalse(self.sites('    return g(a) + h(b);\n'))
        self.assertFalse(self.sites('    return g(a) + b;\n'))
        self.assertTrue(self.sites('    return g(a) + 1;\n'))
        self.assertFalse(self.sites('    return (int)*p * 2;\n', 'int f(int *p)'))
        self.assertFalse(self.sites('    S *p;\n    p = 0;\n    return 0;\n'))
        self.assertFalse(self.sites('    p = (T **)malloc(4);\n    return 0;\n'))
        self.assertFalse(self.sites('    return a && b;\n'))
        found = self.sites('    if (p != 0) *p = a;\n    while (a) *p += 2 * a;\n    return 0;\n', 'int f(int *p, int a)')
        self.assertEqual(sorted(k.split('@')[0] for k in found), ['!=', '*'])
        self.assertIn('2 * a', next(k for k in found if k.startswith('*')))

    def test_macro_operands_are_parenthesised(self):
        found = self.sites('    return a * SCALE;\n')
        self.assertIn('    return (SCALE) * a;\n', found.values())
        found = self.sites('    return a == NULL;\n')
        self.assertIn('    return NULL == a;\n', found.values())

    def test_pairs(self):
        text = fn('    return (a + b) * (c + 1);\n')
        pairs = candidates('operands', text, phase='pairs')
        self.assertTrue(any('(b + a) * (1 + c)' in c for _, _, c in pairs))

    def test_operand_swaps_preserve_behaviour(self):
        text = fn('    int r = (a - b + c * 3) ^ (a & 6) | (b < c);\n    if (a * 2 != b + c) r += 1;\n'
                  '    return r + (a >= c) - (b <= a);\n', 'int f(int a, int b, int c)')
        singles = [c for _, _, c in candidates('operands', text)]
        pairs = [c for _, _, c in candidates('operands', text, phase='pairs')]
        self.assertGreater(len(singles), 8)
        driver = ('\n#include <stdio.h>\nint main(void) { int i, j, k; for (i = -2; i < 3; i++) for (j = -2; j < 3; j++) '
                  'for (k = -2; k < 3; k++) printf("%d ", f(i, j, k)); return 0; }\n')
        outputs = run_c([text] + singles + pairs[:40], driver)
        self.assertEqual(len(set(outputs)), 1)


class SearchTests(unittest.TestCase):
    TEXT = fn('    S s;\n    s.a = 1;\n    s.b = 2;\n    s.c = 3;\n    s.d = 4;\n    return 0;\n')
    TARGET = ('s.d = 4', 's.b = 2', 's.a = 1', 's.c = 3')

    def score(self, text):
        positions = [text.index(t) for t in self.TARGET]
        inversions = sum(1 for i in range(4) for j in range(i + 1, 4) if positions[i] > positions[j])
        self.calls += 1
        return 1.0 - inversions / 10.0, False

    def search(self, out, seconds=60, **kw):
        source = Path(out) / 'unit.c'
        return ps.Search(0x1234, 'stores', source, Path(out), self.score, self.TEXT, seconds=seconds, rel='unit.c', log=lambda *_: None, **kw)

    def setUp(self):
        self.calls = 0

    def test_finds_the_best_order_and_reports_it(self):
        with tempfile.TemporaryDirectory() as out:
            search = self.search(out)
            search.init_state(*self.score(self.TEXT))
            self.assertEqual(search.run(), 0)
            self.assertEqual(search.state['best'], 1.0)
            self.assertIn('    s.d = 4;\n    s.b = 2;\n    s.a = 1;\n    s.c = 3;\n', search.best)
            self.assertLess(search.state['attempts'], 23)  # stops at the exact order
            report = (Path(out) / 'result.json').read_text()
            self.assertIn('"finished": true', report)
            self.assertIn('+    s.d = 4;', (Path(out) / 'best.patch').read_text())

    def test_budget_and_resume_reach_the_same_result(self):
        with tempfile.TemporaryDirectory() as out:
            full = self.search(out)
            full.init_state(*self.score(self.TEXT))
            full.state['best'] = full.state['initial'] = 0.0  # force a full enumeration
            full.max_rounds = 1
            full.run()
            total = full.state['attempts']
        with tempfile.TemporaryDirectory() as out:
            first = self.search(out)
            first.init_state(*self.score(self.TEXT))
            first.state['best'] = first.state['initial'] = 0.0
            first.max_rounds = 1
            original = first.score_fn

            def limited(text):
                if first.state['attempts'] >= 7:
                    first.deadline = 0
                return original(text)
            first.score_fn = limited
            self.assertEqual(first.run(), 3)
            self.assertFalse(first.state['finished'])
            second = self.search(out)
            second.load_state()
            second.max_rounds = 1
            self.assertEqual(second.run(), 0)
            self.assertEqual(second.state['attempts'], total)
            self.assertEqual(second.state['best'], 1.0)

    def test_resume_refuses_a_changed_source(self):
        with tempfile.TemporaryDirectory() as out:
            search = self.search(out)
            search.init_state(0.5, False)
            search.save()
            other = ps.Search(0x1234, 'stores', Path(out) / 'unit.c', Path(out), self.score, self.TEXT + '\n', seconds=1, rel='unit.c')
            with self.assertRaises(ValueError):
                other.load_state()


if __name__ == '__main__':
    unittest.main()
