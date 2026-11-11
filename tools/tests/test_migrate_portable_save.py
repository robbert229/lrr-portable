# SPDX-License-Identifier: LGPL-3.0-or-later
import importlib.util
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

TOOL = Path(__file__).resolve().parents[1] / "migrate_portable_save.py"
spec = importlib.util.spec_from_file_location("migrate_portable_save", TOOL)
migration = importlib.util.module_from_spec(spec)
spec.loader.exec_module(migration)


def fixture():
    data = bytearray(0xb0 + 0x3190)
    struct.pack_into("<4i", data, 4, 0, 6, 2, 7)
    struct.pack_into("<i", data, 0x18, 1)
    data[0xb0:] = bytes(range(256)) * (0x3190 // 256) + bytes(range(0x3190 % 256))
    return bytes(data) + b"opaque trailer"


class MigrationTests(unittest.TestCase):
    def test_only_two_numeric_words_change(self):
        data = fixture()
        result = migration.convert(data, 3)
        self.assertEqual(struct.unpack_from("<i", result, 4)[0], 7)
        self.assertEqual(struct.unpack_from("<i", result, 0x10)[0], 3)
        allowed = set(range(4, 8)) | set(range(0x10, 0x14))
        self.assertTrue(all(a == b or i in allowed for i, (a, b) in enumerate(zip(data, result))))
        self.assertEqual(result[0xb0:], data[0xb0:])

    def test_rejects_gops_and_nonzero_retail_music(self):
        for offset, value in [(0x20, b"GOPS"), (4, struct.pack("<i", 5))]:
            data = bytearray(fixture())
            data[offset:offset+4] = value
            with self.assertRaises(ValueError):
                migration.convert(data, 3)

    def test_rejects_truncated_payload(self):
        with self.assertRaises(ValueError):
            migration.convert(fixture()[:0xb0], 3)

    def test_rejects_invalid_values(self):
        for brightness in (-1, 11):
            with self.assertRaises(ValueError):
                migration.convert(fixture(), brightness)
        data = bytearray(fixture())
        struct.pack_into("<i", data, 0x10, 99)
        with self.assertRaises(ValueError):
            migration.convert(data, 3)

    def test_cli_preserves_source_and_refuses_overwrite(self):
        with tempfile.TemporaryDirectory() as directory:
            src = Path(directory) / "old.sav"
            dst = Path(directory) / "new.sav"
            data = fixture()
            src.write_bytes(data)
            argv = [sys.executable, str(TOOL), "--legacy-music-header", "--brightness", "3", str(src), str(dst)]
            self.assertEqual(subprocess.run(argv, capture_output=True).returncode, 0)
            output = dst.read_bytes()
            self.assertEqual(subprocess.run(argv, capture_output=True).returncode, 1)
            self.assertEqual(src.read_bytes(), data)
            self.assertEqual(dst.read_bytes(), output)
            self.assertNotEqual(subprocess.run([sys.executable, str(TOOL), "--brightness", "3", str(src), str(dst)], capture_output=True).returncode, 0)


if __name__ == "__main__":
    unittest.main()
