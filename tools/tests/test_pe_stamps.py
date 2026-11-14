# SPDX-License-Identifier: LGPL-3.0-or-later
import struct
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import pe_stamps

ROOT = Path(__file__).resolve().parents[2]
ORIGINAL = ROOT / "reference/LegoRR.exe"
RSRC_RVA = 0x2000
ICON = bytes(range(40)) + b'\xaa' * 24


def resource_tree(leaves):
    """A resource directory for {(type, name): bytes}, language 2057, data after it."""
    kinds = sorted({kind for kind, _ in leaves})
    names = {kind: sorted(name for k, name in leaves if k == kind) for kind in kinds}
    table = lambda count: 16 + 8 * count
    layout, pos = {}, table(len(kinds))
    for kind in kinds:
        layout[kind] = pos
        pos += table(len(names[kind]))
    for key in sorted(leaves):
        layout[key] = pos
        pos += table(1)
    for key in sorted(leaves):
        layout[key, 'leaf'] = pos
        pos += 16
    data_at = pos
    out = bytearray(pos)

    def directory(at, entries):
        struct.pack_into('<IIHHHH', out, at, 0, 0x1234, 4, 0, 0, len(entries))
        for i, (name, target) in enumerate(entries):
            struct.pack_into('<II', out, at + 16 + 8 * i, name, target)

    directory(0, [(kind, 0x80000000 | layout[kind]) for kind in kinds])
    for kind in kinds:
        directory(layout[kind], [(name, 0x80000000 | layout[kind, name]) for name in names[kind]])
    for key in sorted(leaves):
        directory(layout[key], [(2057, layout[key, 'leaf'])])
        struct.pack_into('<IIII', out, layout[key, 'leaf'], RSRC_RVA + data_at + len(out) - pos,
                         len(leaves[key]), 0, 0)
        out += leaves[key]
    return bytes(out)


def fixture_pe(path, leaves=None):
    """A PE32 with .text and, when ``leaves`` is given, a .rsrc section."""
    rsrc = resource_tree(leaves) if leaves is not None else b''
    rsrc_raw = (len(rsrc) + 0x1ff) & ~0x1ff
    count = 2 if leaves is not None else 1
    data = bytearray(0x600 + rsrc_raw)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 0x3c, 0x80)
    data[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<HHIIIHH', data, 0x84, 0x14c, count, 0x6af5563a, 0, 0, 224, 0x10b)
    opt = 0x98
    struct.pack_into('<H', data, opt, 0x10b)
    struct.pack_into('<I', data, opt + 92, 16)
    if leaves is not None:
        struct.pack_into('<II', data, opt + 96 + 2 * 8, RSRC_RVA, len(rsrc))
    sections = [(b'.text', 0x200, 0x1000, 0x200, 0x400)]
    if leaves is not None:
        sections.append((b'.rsrc', len(rsrc), RSRC_RVA, rsrc_raw, 0x600))
    for i, (name, vsize, rva, raw_size, raw) in enumerate(sections):
        off = opt + 224 + 40 * i
        data[off:off + len(name)] = name
        struct.pack_into('<IIII', data, off + 8, vsize, rva, raw_size, raw)
    data[0x600:0x600 + len(rsrc)] = rsrc
    path.write_bytes(data)


def group(*entries):
    out = struct.pack('<HHH', 0, 1, len(entries))
    for icon_id, size in entries:
        out += struct.pack('<BBBBHHIH', 32, 32, 0, 0, 1, 8, size, icon_id)
    return out


class StampTests(unittest.TestCase):
    def test_stamp_sets_timestamps_and_line_numbers_bit(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / 'a.exe'
            fixture_pe(exe, {(3, 1): ICON, (14, 113): group((1, len(ICON)))})
            before = exe.read_bytes()
            pe_stamps.stamp(exe)
            after = exe.read_bytes()
            self.assertEqual(struct.unpack_from('<I', after, 0x88)[0], 0x37EEB911)
            self.assertEqual(struct.unpack_from('<H', after, 0x96)[0], 0x10F)
            image = pe_stamps.Image(after)
            tables = image.resource_directories()
            self.assertEqual(len(tables), 5)
            for table in tables:
                self.assertEqual(struct.unpack_from('<I', after, table + 4)[0], 0x37EEB90E)
            changed = [i for i in range(len(before)) if before[i] != after[i]]
            allowed = set(range(0x88, 0x8c)) | {0x96} | {t + j for t in tables for j in range(4, 8)}
            self.assertLessEqual(set(changed), allowed)
            pe_stamps.stamp(exe)
            self.assertEqual(exe.read_bytes(), after)

    def test_stamp_without_resources(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / 'a.exe'
            fixture_pe(exe)
            pe_stamps.stamp(exe)
            self.assertEqual(struct.unpack_from('<I', exe.read_bytes(), 0x88)[0], 0x37EEB911)

    def test_rejects_non_pe(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / 'a.exe'
            exe.write_bytes(b'MZ' + bytes(0x80))
            with self.assertRaises(ValueError):
                pe_stamps.stamp(exe)


class IconTests(unittest.TestCase):
    def test_icon_file_from_group(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / 'a.exe'
            fixture_pe(exe, {(3, 1): ICON, (5, 101): b'dialog', (14, 113): group((1, len(ICON)))})
            ico = pe_stamps.icon_file(pe_stamps.Image(exe.read_bytes()))
            self.assertEqual(ico[:6], struct.pack('<HHH', 0, 1, 1))
            self.assertEqual(ico[6:22], struct.pack('<BBBBHHII', 32, 32, 0, 0, 1, 8, len(ICON), 22))
            self.assertEqual(ico[22:], ICON)

    def test_icon_errors(self):
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / 'a.exe'
            fixture_pe(exe, {(3, 1): ICON})
            with self.assertRaisesRegex(ValueError, 'no GROUP_ICON 113'):
                pe_stamps.icon_file(pe_stamps.Image(exe.read_bytes()))
            fixture_pe(exe, {(14, 113): group((2, len(ICON)))})
            with self.assertRaisesRegex(ValueError, 'missing ICON 2'):
                pe_stamps.icon_file(pe_stamps.Image(exe.read_bytes()))
            fixture_pe(exe, {(3, 1): ICON, (14, 113): group((1, len(ICON) + 1))})
            with self.assertRaisesRegex(ValueError, 'its group says'):
                pe_stamps.icon_file(pe_stamps.Image(exe.read_bytes()))

    def test_missing_original_fails_clearly(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(SystemExit, 'licensed, never committed'):
                pe_stamps.extract_icon(Path(tmp) / 'LegoRR.exe', Path(tmp) / 'LegoRR.ico')


@unittest.skipUnless(ORIGINAL.is_file(), 'needs reference/LegoRR.exe')
class OriginalTests(unittest.TestCase):
    def test_constants_are_the_originals(self):
        data = ORIGINAL.read_bytes()
        image = pe_stamps.Image(data)
        self.assertEqual(struct.unpack_from('<I', data, image.pe + 8)[0], pe_stamps.COFF_TIMESTAMP)
        characteristics = struct.unpack_from('<H', data, image.pe + 22)[0]
        self.assertTrue(characteristics & pe_stamps.LINE_NUMS_STRIPPED)
        for table in image.resource_directories():
            self.assertEqual(struct.unpack_from('<I', data, table + 4)[0],
                             pe_stamps.RESOURCE_TIMESTAMP)
        with tempfile.TemporaryDirectory() as tmp:
            exe = Path(tmp) / 'LegoRR.exe'
            exe.write_bytes(data)
            pe_stamps.stamp(exe)
            self.assertEqual(exe.read_bytes(), data)

    def test_original_icon(self):
        resources = pe_stamps.Image(ORIGINAL.read_bytes()).resources()
        ico = pe_stamps.icon_file(pe_stamps.Image(ORIGINAL.read_bytes()))
        self.assertEqual(len(ico), 6 + 16 + 2216)
        self.assertEqual(ico[22:], resources[3, 1][1])
        self.assertEqual(resources[3, 1][0], 2057)
        self.assertEqual(resources[14, 113][0], 2057)


if __name__ == '__main__':
    unittest.main()
