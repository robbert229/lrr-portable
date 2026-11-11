# SPDX-License-Identifier: LGPL-3.0-or-later
import shutil
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import meson_source_parity as parity


class SourceParityTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        for name in ('CMakeLists.txt', 'src/link_order.cmake', 'src/meson.build',
                     'src/game/meson.build', 'src/gods98/meson.build',
                     'src/platform/meson.build'):
            dest = self.root / name
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy(parity.ROOT / name, dest)

    def test_matching_and_drift(self):
        self.assertEqual([], parity.compare(self.root))
        path = self.root / 'src/game/meson.build'
        path.write_text(path.read_text().replace("  'bezier.c',", "  'invented.c',"))
        errors = parity.compare(self.root)
        self.assertTrue(any('missing from Meson: src/game/bezier.c' in e for e in errors))
        self.assertTrue(any('extra in Meson: src/game/invented.c' in e for e in errors))

    def test_relocation_and_platform_filter(self):
        order = self.root / 'src/link_order.cmake'
        order.write_text(order.read_text().replace('game/cmd.cpp', 'game/advisor.cpp'))
        meson = self.root / 'src/game/meson.build'
        meson.write_text(meson.read_text().replace("'cmd.cpp'", "'advisor.cpp'"))
        self.assertEqual([], parity.compare(self.root))

    def test_unrecognized_expression_fails(self):
        path = self.root / 'src/game/meson.build'
        path.write_text(path.read_text().replace("  'bezier.c',", "  new_sources,"))
        with self.assertRaises(ValueError):
            parity.compare(self.root)


if __name__ == '__main__':
    unittest.main()
