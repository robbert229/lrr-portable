# SPDX-License-Identifier: LGPL-3.0-or-later
"""match_function.with_extra_flags: per-unit option experiments (finding 663)."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import match_function as mf  # noqa: E402


class WithExtraFlags(unittest.TestCase):
    ARGV = ["cl", "/nologo", "/TC", "-Isrc", "/O2", "/Ob1", "/Fofoo.obj", "-c", "/x/src/lwo.c"]

    def test_empty_keeps_argv(self):
        self.assertEqual(mf.with_extra_flags(self.ARGV, []), self.ARGV)
        self.assertEqual(mf.with_extra_flags(self.ARGV, None), self.ARGV)

    def test_appended_after_the_configured_options(self):
        out = mf.with_extra_flags(self.ARGV, ["/Oa", "/G6"])
        self.assertEqual(out, self.ARGV + ["/Oa", "/G6"])
        self.assertLess(out.index("/O2"), out.index("/Oa"))

    def test_argv_not_modified(self):
        argv = list(self.ARGV)
        mf.with_extra_flags(argv, ["/Od"])
        self.assertEqual(argv, self.ARGV)


if __name__ == "__main__":
    unittest.main()
