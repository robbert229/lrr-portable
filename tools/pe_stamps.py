#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""The original's resources and header stamps for the matching build (#560).

``icon ORIGINAL OUT.ico`` rebuilds the shipped icon file from the original's
``GROUP_ICON 113`` and its ``ICON`` leaves. The icon is licensed art, so it is
never committed: the matching build extracts it from ``reference/LegoRR.exe``
before RC compiles ``src/version.rc``.

``stamp EXE`` is the post-link step after ``strip_pe_debug.py``. It writes the
original's COFF ``TimeDateStamp``, the resource directories' ``TimeDateStamp``
and sets ``IMAGE_FILE_LINE_NUMS_STRIPPED``, which LINK leaves clear under
``/DEBUG`` (finding 651). The checksum is 0 in both images and is left alone.
"""
import argparse
import struct
import sys
from pathlib import Path

COFF_TIMESTAMP = 0x37EEB911
RESOURCE_TIMESTAMP = 0x37EEB90E
LINE_NUMS_STRIPPED = 0x0004

RT_ICON = 3
RT_GROUP_ICON = 14
GROUP_ICON_ID = 113


class Image:
    """A PE32 image's headers, its section table and its resource directory."""

    def __init__(self, data):
        self.data = data
        try:
            self.pe = struct.unpack_from('<I', data, 0x3c)[0]
            if data[self.pe:self.pe + 4] != b'PE\0\0':
                raise ValueError('not a PE image')
            count, opt_size = struct.unpack_from('<H12xH', data, self.pe + 6)
            opt = self.pe + 24
            if struct.unpack_from('<H', data, opt)[0] != 0x10b:
                raise ValueError('expected PE32')
            self.sections = [struct.unpack_from('<IIII', data, opt + opt_size + i * 40 + 8)
                             for i in range(count)]
            self.rsrc_rva = struct.unpack_from('<I', data, opt + 96 + 2 * 8)[0]
        except struct.error as error:
            raise ValueError('truncated PE image') from error

    def offset(self, rva):
        for virtual_size, start, raw_size, raw in self.sections:
            if start <= rva < start + max(virtual_size, raw_size):
                if rva - start >= raw_size:
                    break
                return raw + rva - start
        raise ValueError(f'RVA {rva:#x} outside the sections\' file data')

    def resource_directories(self):
        """File offsets of every resource directory table, root first."""
        if not self.rsrc_rva:
            return []
        base = self.offset(self.rsrc_rva)
        found, pending = [], [0]
        while pending:
            rel = pending.pop(0)
            found.append(base + rel)
            named, ids = struct.unpack_from('<HH', self.data, base + rel + 12)
            for i in range(named + ids):
                target = struct.unpack_from('<I', self.data, base + rel + 16 + i * 8 + 4)[0]
                if target & 0x80000000:
                    pending.append(target & 0x7fffffff)
        return found

    def resources(self):
        """{(type, name): (language, bytes)} for ID-named leaves, first language each."""
        if not self.rsrc_rva:
            return {}
        base = self.offset(self.rsrc_rva)

        def entries(rel):
            named, ids = struct.unpack_from('<HH', self.data, base + rel + 12)
            for i in range(named + ids):
                name, target = struct.unpack_from('<II', self.data, base + rel + 16 + i * 8)
                yield name, target

        found = {}
        for kind, kind_dir in entries(0):
            if not kind_dir & 0x80000000:
                continue
            for name, name_dir in entries(kind_dir & 0x7fffffff):
                if not name_dir & 0x80000000:
                    continue
                for language, leaf in entries(name_dir & 0x7fffffff):
                    if leaf & 0x80000000 or (kind, name) in found:
                        continue
                    rva, size = struct.unpack_from('<II', self.data, base + leaf)
                    start = self.offset(rva)
                    found[kind, name] = (language, bytes(self.data[start:start + size]))
        return found


def icon_file(image, group_id=GROUP_ICON_ID):
    """The .ico file holding the image's icon group ``group_id``."""
    resources = image.resources()
    if (RT_GROUP_ICON, group_id) not in resources:
        raise ValueError(f'no GROUP_ICON {group_id}')
    group = resources[RT_GROUP_ICON, group_id][1]
    reserved, kind, count = struct.unpack_from('<HHH', group, 0)
    if reserved != 0 or kind != 1 or len(group) != 6 + 14 * count:
        raise ValueError(f'malformed GROUP_ICON {group_id}')
    head = bytearray(struct.pack('<HHH', 0, 1, count))
    images = bytearray()
    offset = 6 + 16 * count
    for i in range(count):
        width, height, colours, zero, planes, bits, size, icon_id = struct.unpack_from(
            '<BBBBHHIH', group, 6 + 14 * i)
        if (RT_ICON, icon_id) not in resources:
            raise ValueError(f'GROUP_ICON {group_id} names missing ICON {icon_id}')
        body = resources[RT_ICON, icon_id][1]
        if len(body) != size:
            raise ValueError(f'ICON {icon_id} is {len(body)} bytes, its group says {size}')
        head += struct.pack('<BBBBHHII', width, height, colours, zero, planes, bits, size,
                            offset + len(images))
        images += body
    return bytes(head + images)


def extract_icon(original, out):
    try:
        data = Path(original).read_bytes()
    except FileNotFoundError:
        raise SystemExit(f'{original} is missing: the matching build takes its icon from the '
                         'original executable (licensed, never committed); place it under '
                         'reference/ first') from None
    content = icon_file(Image(data))
    out = Path(out)
    if not out.exists() or out.read_bytes() != content:
        out.write_bytes(content)


def stamp(path):
    path = Path(path)
    data = bytearray(path.read_bytes())
    image = Image(data)
    struct.pack_into('<I', data, image.pe + 8, COFF_TIMESTAMP)
    characteristics = struct.unpack_from('<H', data, image.pe + 22)[0]
    struct.pack_into('<H', data, image.pe + 22, characteristics | LINE_NUMS_STRIPPED)
    for table in image.resource_directories():
        struct.pack_into('<I', data, table + 4, RESOURCE_TIMESTAMP)
    path.write_bytes(data)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest='command', required=True)
    icon = sub.add_parser('icon', help='write the original\'s icon file')
    icon.add_argument('original', type=Path)
    icon.add_argument('out', type=Path)
    stamps = sub.add_parser('stamp', help='write the original\'s header stamps')
    stamps.add_argument('exe', type=Path)
    args = parser.parse_args(argv)
    try:
        if args.command == 'icon':
            extract_icon(args.original, args.out)
        else:
            stamp(args.exe)
    except ValueError as error:
        raise SystemExit(f'pe_stamps.py: {error}') from None


if __name__ == '__main__':
    sys.exit(main())
