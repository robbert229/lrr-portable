#!/usr/bin/env python3
"""Lego Rock Raiders `.wad` (WWAD) archive tool: list / unpack / pack / verify.

Format (docs/findings/01-binary-and-loader-map.md, validated byte-for-byte):

    "WWAD" | u32 count | count*NUL names | count*NUL dev-paths | count*16B TOC | data
    TOC entry: flags u32 | orig_len u32 | stored_len u32 | file_offset u32

Data payloads are contiguous, ascending in TOC order, uncompressed. Repacking
keeps names/dev-paths/flags verbatim from a manifest, so an untouched archive
round-trips byte-identical (`verify`).

Usage:
    python3 tools/rrwad.py list source/LegoRR0.wad
    python3 tools/rrwad.py unpack source/LegoRR0.wad unpacked/LegoRR0
    python3 tools/rrwad.py pack unpacked/LegoRR0/.rrwad.json -o rebuilt.wad
    python3 tools/rrwad.py verify source/LegoRR0.wad
"""
import argparse
import hashlib
import json
import os
import struct
import sys

WAD_MAGIC = b"WWAD"
MANIFEST_VERSION = "wwad-manifest/1"
TOC_SIZE = 16


class Wad:
    """Lazily-parsed WWAD archive."""

    def __init__(self, path):
        self.path = path
        with open(path, "rb") as f:
            self.image = f.read()
        if self.image[:4] != WAD_MAGIC:
            raise ValueError(f"{path}: bad magic {self.image[:4]!r}, expected WWAD")
        self.count = struct.unpack_from("<I", self.image, 4)[0]
        pos = 8
        self.names, pos = self._read_strings(self.image, pos, self.count)
        self.build_paths, pos = self._read_strings(self.image, pos, self.count)
        self.toc_off = pos
        self.entries = [struct.unpack_from("<IIII", self.image, pos + i * TOC_SIZE)
                        for i in range(self.count)]
        self.size = len(self.image)

    @staticmethod
    def _read_strings(buf, pos, n):
        items = []
        for _ in range(n):
            end = buf.index(b"\x00", pos)
            items.append(buf[pos:end].decode("latin1"))
            pos = end + 1
        return items, pos

    # -- payload access -----------------------------------------------------

    def payload(self, index):
        _, _, stored, off = self.entries[index]
        return self.image[off:off + stored]

    def index_for(self, name):
        want = name.lower()
        for i, n in enumerate(self.names):
            if n.lower() == want:
                return i
        raise KeyError(name)


# -- manifest ---------------------------------------------------------------

def manifest_for(wad, data_dir=None):
    """Serialize full index; data entries reference files under data_dir when given."""
    entries = []
    for i, (name, (flags, orig, stored, off)) in enumerate(zip(wad.names, wad.entries)):
        entry = {
            "name": name,
            "build_path": wad.build_paths[i],
            "flags": flags,
            "orig_len": orig,
            "stored_len": stored,
            "offset": off,
        }
        if data_dir is not None:
            entry["data_file"] = os.path.relpath(os.path.join(data_dir, local_path(name)), data_dir)
        entries.append(entry)
    return {
        "version": MANIFEST_VERSION,
        "source": os.path.basename(wad.path),
        "source_size": wad.size,
        "entry_count": wad.count,
        "entries": entries,
    }


def local_path(name):
    """Archive path -> host-relative path ('\\' handled, guarded against escapes)."""
    rel = name.replace("\\", os.sep).replace("/", os.sep)
    parts = [p for p in rel.split(os.sep) if p not in ("", ".")]
    if os.path.isabs(rel) or any(p == ".." for p in parts) or not parts:
        raise ValueError(f"unsafe archive path: {name!r}")
    return os.sep.join(parts)


# -- commands ----------------------------------------------------------------

def cmd_list(args):
    wad = Wad(args.wad)
    total = 0
    for i, (flags, orig, stored, off) in enumerate(wad.entries):
        flag_s = "" if flags == 1 else f" flags={flags}"
        print(f"{i:6d} {stored:10d} @{off:10d}{flag_s}  {wad.names[i]}")
        total += stored
    print(f"-- {wad.count} entries, {total:,} payload bytes, file {wad.size:,} B")


def cmd_unpack(args):
    wad = Wad(args.wad)
    dest = args.dest or os.path.splitext(os.path.basename(args.wad))[0]
    if os.path.isdir(dest) and os.listdir(dest) and not args.force:
        raise SystemExit(f"{dest}: not empty (use --force to unpack into it)")
    manifest = manifest_for(wad, data_dir=dest)
    for i, entry in enumerate(manifest["entries"]):
        target = os.path.join(dest, local_path(entry["name"]))
        os.makedirs(os.path.dirname(target) or dest, exist_ok=True)
        with open(target, "wb") as out:
            out.write(wad.payload(i))
    mpath = args.manifest or os.path.join(dest, ".rrwad.json")
    with open(mpath, "w") as f:
        json.dump(manifest, f, indent=1, ensure_ascii=False)
    print(f"unpacked {wad.count} entries -> {dest}/ (manifest {mpath})")


def pack_bytes(manifest, base_dir):
    """Rebuild a WWAD from manifest + data files; returns bytes."""
    entries = manifest["entries"]
    names = [e["name"].encode("latin1") + b"\x00" for e in entries]
    builds = [e["build_path"].encode("latin1") + b"\x00" for e in entries]
    count = len(entries)
    data_blobs = []
    for e in entries:
        p = os.path.join(base_dir, e["data_file"]) if "data_file" in e else None
        if p is None:
            raise ValueError("manifest entry has no data source; unpack first")
        with open(p, "rb") as f:
            data_blobs.append(f.read())
    header_len = 8 + sum(len(n) for n in names) + sum(len(b) for b in builds) + TOC_SIZE * count
    # offsets: data must stay contiguous & TOC-ordered, matching the original
    # layout. Recompute; identical to original iff payloads unchanged.
    toc = bytearray()
    data = bytearray()
    cursor = header_len
    for e, blob in zip(entries, data_blobs):
        flags = e.get("flags", 1)
        orig = len(blob) if e.get("orig_len", len(blob)) == e.get("stored_len", len(blob)) else e["orig_len"]
        toc += struct.pack("<IIII", flags, orig, len(blob), cursor)
        data += blob
        cursor += len(blob)
    return WAD_MAGIC + struct.pack("<I", count) + b"".join(names) + b"".join(builds) + bytes(toc) + bytes(data)


def cmd_pack(args):
    with open(args.manifest) as f:
        manifest = json.load(f)
    if manifest.get("version") != MANIFEST_VERSION:
        raise SystemExit(f"unsupported manifest version {manifest.get('version')!r}")
    blob = pack_bytes(manifest, os.path.dirname(os.path.abspath(args.manifest)))
    with open(args.output, "wb") as f:
        f.write(blob)
    print(f"packed {manifest['entry_count']} entries -> {args.output} ({len(blob):,} B)")


def cmd_verify(args):
    """unpack (in-memory semantics via manifest) -> pack -> byte-compare."""
    import tempfile

    wad = Wad(args.wad)
    with tempfile.TemporaryDirectory() as td:
        manifest = manifest_for(wad, data_dir=td)
        for i, e in enumerate(manifest["entries"]):
            target = os.path.join(td, local_path(e["name"]))
            os.makedirs(os.path.dirname(target) or td, exist_ok=True)
            with open(target, "wb") as f:
                f.write(wad.payload(i))
        rebuilt = pack_bytes(manifest, td)
    original = open(args.wad, "rb").read()
    if rebuilt == original:
        print(f"verify: OK — {args.wad} round-trips byte-identical "
              f"({wad.count} entries, {len(original):,} B)")
        return
    # locate first difference to help debug
    for i, (a, b) in enumerate(zip(original, rebuilt)):
        if a != b:
            print(f"verify: MISMATCH first diff at {i:#x}: orig={a:#02x} pack={b:#02x}")
            return
    print(f"verify: length differs orig={len(original):,} pack={len(rebuilt):,}")


def cmd_extract(args):
    import signal
    if hasattr(signal, "SIGPIPE"):
        signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    wad = Wad(args.wad)
    for name in args.names:
        i = wad.index_for(name)
        _, _, _, off = wad.entries[i]
        print(f"# {name} (stored {len(wad.payload(i))} B @ {off})", file=sys.stderr)
        sys.stdout.buffer.write(wad.payload(i))


def main(argv=None):
    ap = argparse.ArgumentParser(prog="rrwad", description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("list", help="index listing")
    p.add_argument("wad"); p.set_defaults(fn=cmd_list)
    p = sub.add_parser("unpack", help="extract to a directory tree + manifest")
    p.add_argument("wad"); p.add_argument("dest", nargs="?")
    p.add_argument("--manifest"); p.add_argument("--force", action="store_true")
    p.set_defaults(fn=cmd_unpack)
    p = sub.add_parser("pack", help="rebuild .wad from manifest")
    p.add_argument("manifest"); p.add_argument("-o", "--output", required=True)
    p.set_defaults(fn=cmd_pack)
    p = sub.add_parser("verify", help="round-trip byte-identity check")
    p.add_argument("wad"); p.set_defaults(fn=cmd_verify)
    p = sub.add_parser("extract", help="print one entry to stdout")
    p.add_argument("wad"); p.add_argument("names", nargs="+"); p.set_defaults(fn=cmd_extract)
    args = ap.parse_args(argv)
    args.fn(args)


if __name__ == "__main__":
    main()
