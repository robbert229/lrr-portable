# SPDX-License-Identifier: LGPL-3.0-or-later
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from permuter_cohort import classify


def entry(orig, recomp):
    return {'matching': .8, 'diff': [['hunk', [{'orig': [['0', x] for x in orig], 'recomp': [['1', x] for x in recomp]}]]]}


class CohortTests(unittest.TestCase):
    def test_register_constant_and_order(self):
        self.assertTrue(classify(entry(['mov eax, 0x10', 'add ebx, 2'], ['add esi, 3', 'mov ecx, 0x20'])))

    def test_changed_opcode_or_length_rejected(self):
        self.assertFalse(classify(entry(['mov eax, 1'], ['add eax, 1'])))
        self.assertFalse(classify(entry(['mov eax, 1'], ['mov eax, 1', 'ret'])))

    def test_exact_and_empty_rejected(self):
        self.assertFalse(classify({'matching': 1, 'diff': []}))
        self.assertFalse(classify(entry([], [])))
