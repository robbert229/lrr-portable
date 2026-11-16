#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Minimal PE reader for the seam recorder: export and import tables.

    python3 tools/seamrec/pe.py exports DLL      # "ordinal name" lines (name '-' = NONAME)
    python3 tools/seamrec/pe.py imports EXE      # "dll name|#ordinal" lines

Used once to write tools/seamrec/exports/*.txt from the runner's real DLLs,
and by seamrec.py to check that a proxy covers every export of the DLL it
stands in for.
"""
import struct
import sys


class PE:
    def __init__(self, path):
        self.data = open(path, "rb").read()
        d = self.data
        pe = struct.unpack_from("<I", d, 0x3C)[0]
        if d[pe:pe + 4] != b"PE\0\0":
            raise ValueError(f"{path}: not a PE image")
        nsec, = struct.unpack_from("<H", d, pe + 6)
        optsz, = struct.unpack_from("<H", d, pe + 20)
        opt = pe + 24
        magic, = struct.unpack_from("<H", d, opt)
        ddir = opt + (96 if magic == 0x10B else 112)
        self.dirs = [struct.unpack_from("<II", d, ddir + 8 * i) for i in range(16)]
        self.sections = []
        sec = opt + optsz
        for i in range(nsec):
            # IMAGE_SECTION_HEADER: VirtualSize @8, VirtualAddress @12, SizeOfRawData @16, PointerToRawData @20
            vsize, vaddr, rawsz, rawptr = struct.unpack_from("<IIII", d, sec + 40 * i + 8)
            self.sections.append((vaddr, max(vsize, rawsz), rawptr))

    def off(self, rva):
        for vaddr, size, raw in self.sections:
            if vaddr <= rva < vaddr + size:
                return rva - vaddr + raw
        raise ValueError(f"rva {rva:#x} outside sections")

    def cstr(self, rva):
        o = self.off(rva)
        return self.data[o:self.data.index(b"\0", o)].decode("ascii")

    def exports(self):
        """[(ordinal, name or None)] sorted by ordinal; forwarders included."""
        rva, size = self.dirs[0]
        if not rva:
            return []
        o = self.off(rva)
        (_, _, _, _, _, base, nfuncs, nnames, aof, aon, aoo) = struct.unpack_from("<IIHHIIIIIII", self.data, o)
        names = {}
        for i in range(nnames):
            nrva, = struct.unpack_from("<I", self.data, self.off(aon) + 4 * i)
            idx, = struct.unpack_from("<H", self.data, self.off(aoo) + 2 * i)
            names[idx] = self.cstr(nrva)
        out = []
        for i in range(nfuncs):
            frva, = struct.unpack_from("<I", self.data, self.off(aof) + 4 * i)
            if frva:
                out.append((base + i, names.get(i)))
        return out

    def imports(self):
        """[(dll, name or '#ordinal')] in import-table order."""
        rva, _ = self.dirs[1]
        out = []
        o = self.off(rva)
        while True:
            ilt, _, _, name, iat = struct.unpack_from("<IIIII", self.data, o)
            if not name:
                break
            dll = self.cstr(name)
            t = self.off(ilt or iat)
            while True:
                ent, = struct.unpack_from("<I", self.data, t)
                if not ent:
                    break
                out.append((dll, f"#{ent & 0xFFFF}" if ent & 0x80000000 else self.cstr(ent + 2)))
                t += 4
            o += 20
        return out


def main(argv):
    if len(argv) != 3 or argv[1] not in ("exports", "imports"):
        raise SystemExit(__doc__)
    pe = PE(argv[2])
    if argv[1] == "exports":
        for ordinal, name in pe.exports():
            print(ordinal, name or "-")
    else:
        for dll, name in pe.imports():
            print(dll, name)


if __name__ == "__main__":
    main(sys.argv)
