# SPDX-License-Identifier: LGPL-3.0-or-later
"""Write an i386 COFF object that holds only weak externals.

usage: coff_weak_external.py OUT WEAK=DEFAULT [WEAK=DEFAULT ...]

Each WEAK symbol becomes a weak external (storage class 105) whose default
is DEFAULT, with IMAGE_WEAK_EXTERN_SEARCH_NOLIBRARY: LINK resolves a
reference to WEAK to DEFAULT and does not search the libraries for WEAK.
The object has no sections, so it adds nothing to the image.

The matching build links one to give the original's one function two names.
LINK 5.0 has no /ALTERNATENAME, but it resolves weak externals. The original's
operator delete is 0045daf0, which C code also calls as a plain function, so
the build aliases ??3@YAXPAX@Z to _ui_render_record_free (finding 696).
"""

from __future__ import annotations

import struct
import sys
from pathlib import Path

IMAGE_FILE_MACHINE_I386 = 0x014C
IMAGE_SYM_CLASS_EXTERNAL = 2
IMAGE_SYM_CLASS_WEAK_EXTERNAL = 105
IMAGE_WEAK_EXTERN_SEARCH_NOLIBRARY = 1


def build(aliases: list[tuple[str, str]]) -> bytes:
    strtab = bytearray()

    def name(text: str) -> bytes:
        raw = text.encode("ascii")
        if len(raw) <= 8:
            return raw.ljust(8, b"\0")
        offset = 4 + len(strtab)
        strtab.extend(raw + b"\0")
        return struct.pack("<II", 0, offset)

    symbols = bytearray()
    count = 0
    defaults: dict[str, int] = {}
    for _weak, default in aliases:
        if default not in defaults:
            defaults[default] = count
            symbols += name(default) + struct.pack(
                "<IhHBB", 0, 0, 0, IMAGE_SYM_CLASS_EXTERNAL, 0)
            count += 1
    for weak, default in aliases:
        symbols += name(weak) + struct.pack(
            "<IhHBB", 0, 0, 0, IMAGE_SYM_CLASS_WEAK_EXTERNAL, 1)
        symbols += struct.pack("<II", defaults[default],
                               IMAGE_WEAK_EXTERN_SEARCH_NOLIBRARY) + bytes(10)
        count += 2
    header = struct.pack("<HHIIIHH", IMAGE_FILE_MACHINE_I386, 0, 0, 20,
                         count, 0, 0)
    return header + bytes(symbols) + struct.pack("<I", 4 + len(strtab)) + bytes(strtab)


def main(argv: list[str]) -> int:
    if len(argv) < 3 or any("=" not in arg for arg in argv[2:]):
        print(__doc__.strip().splitlines()[2], file=sys.stderr)
        return 2
    aliases = [tuple(arg.split("=", 1)) for arg in argv[2:]]
    Path(argv[1]).write_bytes(build(aliases))  # type: ignore[arg-type]
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
