#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Whole-binary audit: what separates the matching build's PE from the original.

Function code is measured by Reccmp and `binary_progress.py`. This tool
measures the rest of the image (issue #554, finding 563):

- PE/COFF header fields and the section table;
- each section's size and its bytes that differ at the same file offset;
- what fills each rebuilt section, attributed to link-order categories from
  the link map (unit, legacy, port tail, LIBC, import libraries, commons);
- the original's `.text` composition by unit kind, and its inter-function fill;
- import thunks in `.text`, and which of them any object references;
- the import directory: DLL order, imported names, IAT size;
- resources, base relocations, the debug directory;
- repeated string literals in the data sections (cross-reference #553).

`make pe-audit` prints a summary; `--json` writes every measurement.
"""
from __future__ import annotations

import argparse
import collections
import csv
import hashlib
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RANGES = ROOT / "docs/coverage/translation-unit-ranges.csv"

# Header fields compared one by one. Data directories are compared separately.
FILE_FIELDS = ("Machine", "NumberOfSections", "TimeDateStamp", "PointerToSymbolTable",
               "NumberOfSymbols", "SizeOfOptionalHeader", "Characteristics")
OPTIONAL_FIELDS = ("MajorLinkerVersion", "MinorLinkerVersion", "SizeOfCode",
                   "SizeOfInitializedData", "SizeOfUninitializedData", "AddressOfEntryPoint",
                   "BaseOfCode", "BaseOfData", "ImageBase", "SectionAlignment", "FileAlignment",
                   "MajorOperatingSystemVersion", "MinorOperatingSystemVersion",
                   "MajorImageVersion", "MinorImageVersion", "MajorSubsystemVersion",
                   "MinorSubsystemVersion", "SizeOfImage", "SizeOfHeaders", "CheckSum",
                   "Subsystem", "DllCharacteristics", "SizeOfStackReserve", "SizeOfStackCommit",
                   "SizeOfHeapReserve", "SizeOfHeapCommit", "NumberOfRvaAndSizes")
DIRECTORY_NAMES = ("export", "import", "resource", "exception", "security", "basereloc",
                   "debug", "copyright", "globalptr", "tls", "load_config", "bound_import",
                   "iat", "delay_import", "com_descriptor", "reserved")

# Link-map lines: "Start Length Name Class" groups and symbol lines.
MAP_GROUP = re.compile(r"^\s*([0-9a-f]{4}):([0-9a-f]{8})\s+([0-9a-f]{8})H\s+(\S+)\s+(\w+)\s*$",
                       re.I | re.M)
MAP_SYMBOL = re.compile(r"^\s*([0-9a-f]{4}):([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{8})\s+"
                        r"(?:[fi]\s+)*(\S+)\s*$", re.I | re.M)
# An import library member: "<lib>:<DLL>.dll" in the Lib:Object column.
IMPORT_OBJECT = re.compile(r"^[^:]+:[^:]+\.dll$", re.I)
STRING = re.compile(rb"[\x20-\x7e\t\r\n]{4,}\x00")


# ---------------------------------------------------------------------------
# Generic comparisons


def byte_difference(original: bytes, rebuilt: bytes) -> dict:
    """Bytes equal at the same offset, and the bytes that differ.

    `differing` counts every offset of the longer input whose byte is not equal
    on the other side, so a size difference counts in full.
    """
    equal = sum(a == b for a, b in zip(original, rebuilt))
    return {"original": len(original), "rebuilt": len(rebuilt), "equal": equal,
            "differing": max(len(original), len(rebuilt)) - equal}


def field_differences(original: dict, rebuilt: dict) -> dict:
    return {k: {"original": original.get(k), "rebuilt": rebuilt.get(k)}
            for k in original.keys() | rebuilt.keys() if original.get(k) != rebuilt.get(k)}


def compare_imports(original: list[tuple[str, list[str]]],
                    rebuilt: list[tuple[str, list[str]]]) -> dict:
    """Compare import directories given as [(dll, [names in ILT order])]."""
    def norm(entries):
        return [(dll.lower(), names) for dll, names in entries]
    o, r = norm(original), norm(rebuilt)
    od, rd = dict(o), dict(r)
    per_dll = {}
    for dll in [d for d, _ in o] + [d for d, _ in r if d not in od]:
        a, b = od.get(dll, []), rd.get(dll, [])
        per_dll[dll] = {"original": len(a), "rebuilt": len(b),
                        "missing": [n for n in a if n not in b],
                        "extra": [n for n in b if n not in a],
                        "same_order": a == b}
    return {"original_dll_order": [d for d, _ in o], "rebuilt_dll_order": [d for d, _ in r],
            "dll_order_equal": [d for d, _ in o] == [d for d, _ in r],
            "missing_dlls": [d for d, _ in o if d not in rd],
            "extra_dlls": [d for d, _ in r if d not in od],
            "original_names": sum(len(n) for _, n in o),
            "rebuilt_names": sum(len(n) for _, n in r),
            "per_dll": per_dll}


def compare_resources(original: list[dict], rebuilt: list[dict]) -> dict:
    """Compare resource leaves given as dicts with type, name, lang, size, sha256."""
    def key(leaf):
        return (leaf["type"], leaf["name"])
    o, r = {key(x): x for x in original}, {key(x): x for x in rebuilt}
    rows = []
    for k in list(o) + [k for k in r if k not in o]:
        a, b = o.get(k), r.get(k)
        state = ("missing" if b is None else "extra" if a is None else
                 "identical" if a["sha256"] == b["sha256"] and a["lang"] == b["lang"] else
                 "differs")
        rows.append({"type": k[0], "name": k[1], "state": state,
                     "original": a and {"lang": a["lang"], "size": a["size"]},
                     "rebuilt": b and {"lang": b["lang"], "size": b["size"]}})
    return {"leaves": rows,
            "identical": sum(r["state"] == "identical" for r in rows),
            "count": len(rows)}


def repeated_strings(data: bytes) -> dict:
    """NUL-terminated printable strings of 4+ characters, and the bytes repeats take.

    A literal repeated at several addresses is what string pooling would merge
    (issue #553). Only whole strings are counted: tail sharing is not.
    """
    seen = collections.Counter(m.group(0) for m in STRING.finditer(data))
    repeats = {s: n for s, n in seen.items() if n > 1}
    return {"strings": sum(seen.values()), "distinct": len(seen),
            "repeated_distinct": len(repeats),
            "repeated_extra_copies": sum(n - 1 for n in repeats.values()),
            "repeated_extra_bytes": sum((n - 1) * len(s) for s, n in repeats.items())}


# ---------------------------------------------------------------------------
# Link map


def parse_map(text: str) -> tuple[list[dict], list[dict]]:
    """Section groups and symbols (publics and statics) from a LINK 5.0 map."""
    groups = [{"section": int(m.group(1), 16), "offset": int(m.group(2), 16),
               "length": int(m.group(3), 16), "name": m.group(4), "class": m.group(5)}
              for m in MAP_GROUP.finditer(text)]
    symbols, seen = [], set()
    for m in MAP_SYMBOL.finditer(text):
        sym = (int(m.group(1), 16), int(m.group(2), 16), m.group(3), m.group(5))
        if sym not in seen:
            seen.add(sym)
            symbols.append({"section": sym[0], "offset": sym[1], "name": sym[2], "object": sym[3]})
    return groups, symbols


def classify_object(obj: str, kinds: dict[str, str]) -> str:
    """Link-order category of one map object (Lib:Object column)."""
    if obj == "<common>":
        return "common"
    if obj.startswith("<"):
        return "linker"
    if IMPORT_OBJECT.match(obj):
        return "import"
    if ":" in obj:
        lib = obj.split(":", 1)[0].lower()
        return "libc" if lib == "libc" else "library:" + lib
    return kinds.get(obj.lower(), "other")


def object_kinds(entries) -> dict[str, str]:
    """Map object basename -> link-order kind (unit, legacy, tail).

    Two sources with one basename (game/entity.cpp and gods98/entity.cpp) keep
    one kind when they agree and become "ambiguous" otherwise.
    """
    kinds: dict[str, str] = {}
    for e in entries:
        if e.platform not in (None, "winapi"):
            continue
        name = Path(e.source).name.lower() + ".obj"
        kinds[name] = e.kind if kinds.get(name, e.kind) == e.kind else "ambiguous"
    return kinds


def attribute_groups(groups: list[dict], symbols: list[dict], kinds: dict[str, str]) -> dict:
    """Bytes of each map section group, attributed to object categories.

    Each symbol owns the bytes from its offset to the next symbol's offset or
    the group's end. Bytes before a group's first symbol count as "unnamed".
    """
    by_section = collections.defaultdict(list)
    for s in symbols:
        by_section[s["section"]].append(s)
    for syms in by_section.values():
        syms.sort(key=lambda s: s["offset"])
    out = {}
    for g in groups:
        start, end = g["offset"], g["offset"] + g["length"]
        inside = [s for s in by_section[g["section"]] if start <= s["offset"] < end]
        counts = collections.Counter()
        if inside and inside[0]["offset"] > start:
            counts["unnamed"] += inside[0]["offset"] - start
        for i, s in enumerate(inside):
            stop = inside[i + 1]["offset"] if i + 1 < len(inside) else end
            counts[classify_object(s["object"], kinds)] += stop - s["offset"]
        if not inside:
            counts["unnamed"] += g["length"]
        entry = out.setdefault(g["name"], {"section": g["section"], "length": 0, "by_category": {}})
        entry["length"] += g["length"]
        for k, v in counts.items():
            entry["by_category"][k] = entry["by_category"].get(k, 0) + v
    return out


def import_thunks(symbols: list[dict], code_section: int = 1) -> dict:
    """Import thunks in the code section, per DLL."""
    per = collections.Counter(s["object"].split(":", 1)[1].lower() for s in symbols
                              if s["section"] == code_section and IMPORT_OBJECT.match(s["object"])
                              and not s["name"].startswith("__imp_"))
    return dict(sorted(per.items()))


# ---------------------------------------------------------------------------
# PE extraction (pefile)


def section_name(section) -> str:
    return section.Name.rstrip(b"\0").decode("ascii", "replace")


def headers(pe) -> dict:
    out = {f: getattr(pe.FILE_HEADER, f) for f in FILE_FIELDS}
    out.update({f: getattr(pe.OPTIONAL_HEADER, f) for f in OPTIONAL_FIELDS})
    for name, d in zip(DIRECTORY_NAMES, pe.OPTIONAL_HEADER.DATA_DIRECTORY):
        out[f"dir_{name}"] = (d.VirtualAddress, d.Size)
    return out


def section_table(pe) -> list[dict]:
    return [{"name": section_name(s), "rva": s.VirtualAddress, "virtual_size": s.Misc_VirtualSize,
             "raw_offset": s.PointerToRawData, "raw_size": s.SizeOfRawData,
             "characteristics": s.Characteristics} for s in pe.sections]


def section_data(pe, name: str) -> bytes:
    for s in pe.sections:
        if section_name(s) == name:
            return s.get_data()[:min(s.SizeOfRawData, s.Misc_VirtualSize)]
    return b""


def imports(pe) -> list[tuple[str, list[str]]]:
    if not hasattr(pe, "DIRECTORY_ENTRY_IMPORT"):
        pe.parse_data_directories(directories=[1])
    out = []
    for e in getattr(pe, "DIRECTORY_ENTRY_IMPORT", []):
        out.append((e.dll.decode("ascii"),
                    [i.name.decode("ascii") if i.name else f"#{i.ordinal}" for i in e.imports]))
    return out


def resources(pe) -> list[dict]:
    if not hasattr(pe, "DIRECTORY_ENTRY_RESOURCE"):
        pe.parse_data_directories(directories=[2])
    leaves = []

    def label(entry):
        return str(entry.name) if entry.name is not None else entry.id

    for t in getattr(pe, "DIRECTORY_ENTRY_RESOURCE", None) and pe.DIRECTORY_ENTRY_RESOURCE.entries or []:
        for n in t.directory.entries:
            for lang in n.directory.entries:
                d = lang.data.struct
                blob = pe.get_data(d.OffsetToData, d.Size)
                leaves.append({"type": label(t), "name": label(n), "lang": lang.id,
                               "size": d.Size, "sha256": hashlib.sha256(blob).hexdigest()})
    return leaves


def resource_timestamp(pe):
    d = getattr(pe, "DIRECTORY_ENTRY_RESOURCE", None)
    return d.struct.TimeDateStamp if d else None


def import_timestamps(pe) -> list[int]:
    return [e.struct.TimeDateStamp for e in getattr(pe, "DIRECTORY_ENTRY_IMPORT", [])]


# ---------------------------------------------------------------------------
# Original-side composition (translation-unit-ranges.csv, ledger padding)


def original_text_composition(rows: list[dict]) -> dict:
    """.text bytes of the original per unit kind, from the unit ranges."""
    kinds = collections.Counter()
    for r in rows:
        kinds[r["unit_kind"]] += int(r["text_end"], 16) - int(r["text_start"], 16)
    return dict(sorted(kinds.items()))


def function_fill(funcs) -> dict:
    """Bytes between the original's functions, by fill byte kind."""
    fill = collections.Counter()
    for f in funcs:
        fill[f.fill] += len(f.pad)
    body = sum(f.end - f.address for f in funcs)
    return {"function_bytes": body, "fill_bytes": dict(fill),
            "fill_total": sum(fill.values())}


# ---------------------------------------------------------------------------
# Report


def audit(original_path: Path, rebuilt_path: Path, map_path: Path | None,
          ranges_path: Path = RANGES, with_functions: bool = True) -> dict:
    import pefile
    o = pefile.PE(str(original_path))
    r = pefile.PE(str(rebuilt_path))
    od, rd = Path(original_path).read_bytes(), Path(rebuilt_path).read_bytes()
    result: dict = {
        "file_size": {"original": len(od), "rebuilt": len(rd)},
        "dos_stub_and_headers": byte_difference(od[:o.OPTIONAL_HEADER.SizeOfHeaders],
                                                rd[:r.OPTIONAL_HEADER.SizeOfHeaders]),
        "header_differences": field_differences(headers(o), headers(r)),
        "sections": {"original": section_table(o), "rebuilt": section_table(r)},
    }
    names = [s["name"] for s in result["sections"]["original"]]
    names += [s["name"] for s in result["sections"]["rebuilt"] if s["name"] not in names]
    result["section_bytes"] = {n: byte_difference(section_data(o, n), section_data(r, n))
                               for n in names}
    vsize = {side: {s["name"]: s["virtual_size"] for s in result["sections"][side]}
             for side in ("original", "rebuilt")}
    result["section_virtual_sizes"] = {n: {"original": vsize["original"].get(n, 0),
                                           "rebuilt": vsize["rebuilt"].get(n, 0)} for n in names}
    result["imports"] = compare_imports(imports(o), imports(r))
    result["imports"]["descriptor_timestamps"] = {"original": sorted(set(import_timestamps(o))),
                                                  "rebuilt": sorted(set(import_timestamps(r)))}
    result["resources"] = compare_resources(resources(o), resources(r))
    result["resources"]["directory_timestamp"] = {"original": resource_timestamp(o),
                                                  "rebuilt": resource_timestamp(r)}
    result["strings"] = {side: {n: repeated_strings(section_data(pe, n)) for n in (".rdata", ".data")}
                         for side, pe in (("original", o), ("rebuilt", r))}
    if ranges_path.exists():
        with ranges_path.open(newline="") as fh:
            result["original_text_by_unit_kind"] = original_text_composition(list(csv.DictReader(fh)))
    if with_functions:
        try:
            from translation_units import Binary, load_functions
            binary = Binary(Path(original_path))
            result["original_text_fill"] = function_fill(load_functions(binary))
        except (ImportError, SystemExit, FileNotFoundError) as exc:
            result["original_text_fill"] = {"skipped": str(exc)}
    if map_path and Path(map_path).exists():
        import link_order
        groups, symbols = parse_map(Path(map_path).read_text(encoding="latin-1"))
        result["rebuilt_map"] = {
            "groups": attribute_groups(groups, symbols, object_kinds(link_order.load())),
            "import_thunks": import_thunks(symbols),
        }
    return result


def summary(result: dict) -> str:
    lines = [f"file size: original {result['file_size']['original']}, "
             f"rebuilt {result['file_size']['rebuilt']}",
             "header fields that differ:"]
    for k, v in sorted(result["header_differences"].items()):
        fmt = (lambda x: f"{x[0]:#x}+{x[1]:#x}") if isinstance(v["original"], tuple) else hex
        lines.append(f"  {k}: {fmt(v['original'])} -> {fmt(v['rebuilt'])}")
    lines.append("section  orig_vsize  rebuilt_vsize  orig_raw  rebuilt_raw  equal  differing")
    for n, b in result["section_bytes"].items():
        v = result["section_virtual_sizes"][n]
        lines.append(f"  {n:8} {v['original']:10} {v['rebuilt']:14} {b['original']:9} "
                     f"{b['rebuilt']:12} {b['equal']:6} {b['differing']:10}")
    imp = result["imports"]
    lines.append(f"imports: dlls {len(imp['original_dll_order'])} -> {len(imp['rebuilt_dll_order'])}, "
                 f"names {imp['original_names']} -> {imp['rebuilt_names']}, "
                 f"missing dlls {imp['missing_dlls']}, extra {imp['extra_dlls']}, "
                 f"order equal {imp['dll_order_equal']}")
    res = result["resources"]
    lines.append(f"resources: {res['identical']}/{res['count']} leaves identical: " +
                 ", ".join(f"{x['type']}/{x['name']} {x['state']}" for x in res["leaves"]))
    if "rebuilt_map" in result:
        lines.append("rebuilt map groups by category:")
        for n, g in result["rebuilt_map"]["groups"].items():
            cats = ", ".join(f"{k} {v}" for k, v in sorted(g["by_category"].items(), key=lambda x: -x[1]))
            lines.append(f"  {n:16} {g['length']:9}  {cats}")
        lines.append(f"import thunks in .text: {result['rebuilt_map']['import_thunks']}")
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--original", type=Path, default=ROOT / "reference/LegoRR.exe")
    parser.add_argument("--rebuilt", type=Path, default=ROOT / "build-reccmp/LegoRR.exe")
    parser.add_argument("--map", type=Path, default=ROOT / "build-reccmp/LegoRR.map")
    parser.add_argument("--json", type=Path, help="write every measurement as JSON")
    parser.add_argument("--no-functions", action="store_true",
                        help="skip the original's per-function fill (needs capstone)")
    args = parser.parse_args(argv)
    for p in (args.original, args.rebuilt):
        if not p.exists():
            print(f"{p} does not exist", file=sys.stderr)
            return 2
    result = audit(args.original, args.rebuilt, args.map, with_functions=not args.no_functions)
    if args.json:
        args.json.write_text(json.dumps(result, indent=2, sort_keys=True, default=list) + "\n")
    print(summary(result))
    return 0


if __name__ == "__main__":
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    sys.exit(main())
