#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Remove VC5 LINK's debug directory and file-only debug overlay, keeping its PDB."""
import struct
import sys
from pathlib import Path


def strip(path):
    try:
        _strip(path)
    except (struct.error, IndexError) as error:
        raise ValueError("truncated PE image") from error


def _strip(path):
    data = bytearray(path.read_bytes())
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    if data[pe:pe + 4] != b'PE\0\0':
        raise ValueError('not a PE image')
    count, opt_size = struct.unpack_from('<H12xH', data, pe + 6)
    opt = pe + 24
    if struct.unpack_from('<H', data, opt)[0] != 0x10b:
        raise ValueError('expected PE32')
    dirs = opt + 96
    if struct.unpack_from('<II', data, dirs + 4 * 8) != (0, 0):
        raise ValueError('refusing to remove signed image overlay')
    debug_rva, debug_size = struct.unpack_from('<II', data, dirs + 6 * 8)
    end = 0
    debug_offset = None
    for i in range(count):
        sec = opt + opt_size + i * 40
        virtual_size, rva, raw_size, raw = struct.unpack_from('<IIII', data, sec + 8)
        end = max(end, raw + raw_size)
        if rva <= debug_rva and debug_rva + debug_size <= rva + raw_size:
            debug_offset = raw + debug_rva - rva
    if debug_size and debug_offset is None:
        raise ValueError('debug directory outside sections')
    if end > len(data):
        raise ValueError('truncated PE image')
    if debug_size:
        data[debug_offset:debug_offset + debug_size] = bytes(debug_size)
    struct.pack_into('<II', data, dirs + 6 * 8, 0, 0)
    path.write_bytes(data[:end])

if __name__ == '__main__':
    strip(Path(sys.argv[1]))
