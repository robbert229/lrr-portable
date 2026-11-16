# SPDX-License-Identifier: LGPL-3.0-or-later
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import image_identity  # noqa: E402


def fake_pe(stamp=0, checksum=0, body=b'\x90' * 32):
    """A PE32 header with one section holding `body`."""
    data = bytearray(0x200)
    struct.pack_into('<I', data, 0x3c, 0x80)
    data[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<HHI', data, 0x84, 0x14c, 1, stamp)
    struct.pack_into('<H', data, 0x80 + 20, 0xe0)
    struct.pack_into('<H', data, 0x98, 0x10b)
    struct.pack_into('<I', data, 0x98 + 64, checksum)
    entry = 0x98 + 0xe0
    data[entry:entry + 8] = b'.text\0\0\0'
    struct.pack_into('<IIII', data, entry + 8, len(body), 0x1000, len(body), 0x200)
    return bytes(data) + body


class ImageIdentityTest(unittest.TestCase):
    def test_ignores_timestamp_and_checksum(self):
        report = image_identity.compare(fake_pe(stamp=1, checksum=2), fake_pe(stamp=3, checksum=4))
        self.assertTrue(report['identical'])
        self.assertEqual(report['differing_bytes'], 0)
        self.assertIsNone(report['first_difference'])

    def test_counts_code_and_length_differences(self):
        report = image_identity.compare(fake_pe(body=b'\x90' * 31 + b'\xcc'), fake_pe(body=b'\x90' * 32))
        self.assertFalse(report['identical'])
        self.assertEqual(report['differing_bytes'], 1)
        self.assertEqual(report['first_difference']['section'], '.text')
        self.assertEqual(report['first_difference']['address'], hex(0x401000 + 31))
        short = image_identity.compare(fake_pe(body=b'\x90' * 28), fake_pe(body=b'\x90' * 32))
        self.assertEqual(short['differing_bytes'], 2 + 4)  # the two section-size bytes, four missing bytes
        self.assertEqual(short['sections'][0]['differing_bytes'], 4)

    def test_other_header_bytes_count(self):
        a = bytearray(fake_pe())
        a[0x98 + 16] = 1  # AddressOfEntryPoint
        self.assertEqual(image_identity.compare(bytes(a), fake_pe())['differing_bytes'], 1)


if __name__ == '__main__':
    unittest.main()
