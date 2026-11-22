# SPDX-License-Identifier: LGPL-3.0-or-later
"""tools/ia32_fields.py: lengths, field positions and shapes."""
import os
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import ia32_fields as f  # noqa: E402

FORK = Path(os.environ.get("REPROBIT_SRC", Path.home() / "source" / "reprobit" / "src"))


def fields(hexcode: str) -> list[tuple[int, str, bool]]:
    ins = f.decode(bytes.fromhex(hexcode), 0)
    return [(x.offset, x.kind, x.absolute) for x in ins.fields]


class FieldTests(unittest.TestCase):
    def test_absolute_and_based_displacements(self):
        self.assertEqual(fields("8b0d54040000"), [(2, "disp32", True)])  # mov ecx, [abs]
        self.assertEqual(fields("8b0485a0000000"), [(3, "disp32", True)])  # [eax*4 + abs]
        self.assertEqual(fields("8b80a0000000"), [(2, "disp32", False)])  # [eax + disp32]
        self.assertEqual(fields("8b4424 04".replace(" ", "")), [])  # [esp + disp8]
        self.assertEqual(fields("a1 78563412".replace(" ", "")), [(1, "disp32", True)])  # moffs

    def test_immediates_and_branches(self):
        self.assertEqual(fields("b878563412"), [(1, "imm32", False)])
        self.assertEqual(fields("c705 00000000 01000000".replace(" ", "")),
                         [(2, "disp32", True), (6, "imm32", False)])
        self.assertEqual(fields("e800000000"), [(1, "rel32", False)])
        self.assertEqual(fields("0f8400000000"), [(2, "rel32", False)])
        self.assertEqual(fields("f7c078563412"), [(2, "imm32", False)])  # test eax, imm32
        self.assertEqual(fields("66b83412"), [])  # 16-bit immediate
        self.assertEqual(fields("6a01"), [])

    def test_shape_ignores_registers_and_values(self):
        a = f.decode(bytes.fromhex("8b442414"), 0).shape  # mov eax, [esp+0x14]
        b = f.decode(bytes.fromhex("8b4c241c"), 0).shape  # mov ecx, [esp+0x1c]
        self.assertEqual(a, b)
        self.assertNotEqual(f.decode(bytes.fromhex("d81d00000000"), 0).shape,  # fcomp
                            f.decode(bytes.fromhex("d81500000000"), 0).shape)  # fcom
        self.assertNotEqual(f.decode(bytes.fromhex("d9e0"), 0).shape,  # fchs
                            f.decode(bytes.fromhex("d9e1"), 0).shape)  # fabs

    def test_refuses_what_reprobit_refuses(self):
        for code in ("0f0400", "6767", "8b", "d6"):
            with self.subTest(code=code), self.assertRaises(f.DecodeError):
                f.decode(bytes.fromhex(code), 0)


@unittest.skipUnless((FORK / "reprobit" / "ia32_decode.py").is_file(), "ReproBit fork not found")
class ForkParityTests(unittest.TestCase):
    """The decoder accepts what ReproBit's does, with the same lengths."""

    def test_lengths_match_reprobit(self):
        sys.path.insert(0, str(FORK))
        from reprobit.binary import ByteIdentityError
        from reprobit.ia32_decode import supported_ia32_instruction_length

        tail = bytes(range(0x10, 0x30))
        samples = []
        for prefix in (b"", b"\x66", b"\xf3", b"\x64"):
            for opcode in [bytes([op]) for op in range(256)] + [bytes([0x0F, op]) for op in range(256)]:
                for modrm in (0x00, 0x05, 0x04, 0x44, 0x84, 0xC0, 0x0D, 0x3D, 0xC8):
                    for sib in (0x24, 0x25, 0x85):
                        samples.append(prefix + opcode + bytes([modrm, sib]) + tail)
        for code in samples:
            try:
                want = supported_ia32_instruction_length(code, "x")
            except ByteIdentityError:
                want = None
            try:
                got = f.decode(code, 0).length
            except f.DecodeError:
                got = None
            self.assertEqual(got, want, code[:6].hex())


if __name__ == "__main__":
    unittest.main()
