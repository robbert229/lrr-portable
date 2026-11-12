# SPDX-License-Identifier: LGPL-3.0-or-later
import sys
import subprocess
import tempfile
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from reccmp_diagnostics import unexpected

class DiagnosticsTest(unittest.TestCase):
    def test_diagnostics_and_allowlist(self):
        lines = ['[ERROR] Failed to find function symbol\n',
                 '[WARNING] new Reccmp diagnostic\n',
                 'file.cpp(12) : warning C4091: bad typedef\n',
                 'z:/src/gods98/viewports.cpp(508) : warning C4172: returning address of local variable or temporary\n',
                 'WARNING:  missing cvtres.err; displaying error numbers without messages.\n',
                 'ordinary output\n']
        self.assertEqual(unexpected(lines), [x.rstrip() for x in lines[:3]])
    def test_cli_fails_for_new_diagnostic(self):
        with tempfile.NamedTemporaryFile(mode='w', encoding='utf-8') as fixture:
            fixture.write('[ERROR] Dropped duplicate address 0x1234\n')
            fixture.flush()
            result = subprocess.run([sys.executable, str(Path(__file__).resolve().parents[1] / 'reccmp_diagnostics.py'), fixture.name], capture_output=True, text=True)
        self.assertEqual(result.returncode, 1)
        self.assertIn('Dropped duplicate', result.stderr)

    def test_coloured_reccmp_levels_are_diagnostics(self):
        line = '[\x1b[31mERROR\x1b[0m] Dropped duplicate address 0x4a89f8 on STRING annotation\n'
        self.assertEqual(unexpected([line, '[\x1b[33mWARNING\x1b[0m] x\n', '\x1b[32m100.00%\x1b[0m similar\n']),
                         ['[ERROR] Dropped duplicate address 0x4a89f8 on STRING annotation', '[WARNING] x'])

    def test_allowlist_is_specific(self):
        self.assertEqual(len(unexpected(['other.cpp(1) : warning C4172: returning address of local variable or temporary'])), 1)
        self.assertEqual(len(unexpected(['z:/src/gods98/view.cpp(1373) : warning C4172: returning address of local variable or temporary'])), 1)

if __name__ == '__main__':
    unittest.main()
