#!/usr/bin/env python3
"""Build a cross-reference index for DAT-labeled globals in the full decompile.

Usage: python3 tools/dat_inventory.py [OUTPUT.csv]

The index is a review aid, not an interpretation of a global's semantics.
"""

from __future__ import annotations

import argparse
import csv
import re
import struct
from collections import Counter, defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DECOMPILE = ROOT / "docs/decompiles/full/LEGORR.full.c"
FINDINGS = ROOT / "docs/findings"
DEFAULT_OUTPUT = ROOT / "docs/dat/globals.csv"
ANNOTATIONS = ROOT / "docs/dat/annotations.csv"
DEFAULT_BINARY = Path.home() / "source/lrr-portable/reference/LegoRR.exe"

FUNCTION = re.compile(
    r"^// ==== (?P<name>.*?) @ (?P<address>[0-9A-Fa-f]+) "
    r"\[Ghidra: (?P<symbol>.*?)\] ====$",
    re.MULTILINE,
)
DAT = re.compile(r"(?<![A-Za-z0-9])_?DAT_([0-9A-Fa-f]{8})")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", nargs="?", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--binary", type=Path, default=DEFAULT_BINARY)
    args = parser.parse_args()
    output = args.output.resolve()
    source = DECOMPILE.read_text(encoding="utf-8")
    matches = list(FUNCTION.finditer(source))
    counts: Counter[str] = Counter()
    functions: dict[str, list[str]] = defaultdict(list)

    for index, match in enumerate(matches):
        end = matches[index + 1].start() if index + 1 < len(matches) else len(source)
        body = source[match.end():end]
        refs = Counter(address.lower() for address in DAT.findall(body))
        for address, count in refs.items():
            counts[address] += count
            functions[address].append(
                f"{match.group('name')}@0x{match.group('address').lower()}"
            )

    finding_text = {
        path: path.read_text(encoding="utf-8", errors="replace")
        for path in sorted(FINDINGS.glob("*.md"))
    }
    finding_mentions: dict[str, list[str]] = defaultdict(list)
    finding_address = re.compile(r"(?i)(?:DAT_)?(?:0x)?([0-9a-f]{8})(?![0-9a-f])")
    for path, text in finding_text.items():
        for address in set(found.lower() for found in finding_address.findall(text)):
            if address in counts:
                finding_mentions[address].append(path.relative_to(ROOT).as_posix())

    annotations: dict[str, dict[str, str]] = {}
    if ANNOTATIONS.exists():
        with ANNOTATIONS.open(newline="", encoding="utf-8") as stream:
            annotations = {
                row["address"].lower().removeprefix("0x"): row
                for row in csv.DictReader(stream)
            }

    image_base, sections = read_pe_sections(args.binary) if args.binary.is_file() else (None, [])
    binary_data = args.binary.read_bytes() if image_base is not None else b""

    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream, lineterminator="\n")
        writer.writerow(
            [
                "address",
                "decompile_reference_count",
                "function_count",
                "functions",
                "finding_mentions",
                "image_section",
                "storage_kind",
                "file_offset",
                "initial_bytes",
                "review_status",
                "interpretation",
                "confidence",
                "evidence",
            ]
        )
        for address in sorted(counts):
            mentions = finding_mentions[address]
            annotation = annotations.get(address, {})
            section, storage, offset, initial = locate_address(
                int(address, 16), image_base, sections, binary_data
            )
            writer.writerow(
                [
                    f"0x{address}",
                    counts[address],
                    len(functions[address]),
                    "; ".join(functions[address]),
                    "; ".join(mentions),
                    section,
                    storage,
                    offset,
                    initial,
                    annotation.get("review_status", "mentioned" if mentions else "unreviewed"),
                    annotation.get("interpretation", ""),
                    annotation.get("confidence", ""),
                    annotation.get("evidence", ""),
                ]
            )

    print(f"wrote {len(counts)} DAT-labeled addresses to {output}")


def read_pe_sections(path: Path) -> tuple[int, list[dict[str, int | str]]]:
    data = path.read_bytes()
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe_offset:pe_offset + 4] != b"PE\0\0":
        raise ValueError(f"{path}: invalid PE signature")
    section_count = struct.unpack_from("<H", data, pe_offset + 6)[0]
    optional_size = struct.unpack_from("<H", data, pe_offset + 20)[0]
    optional = pe_offset + 24
    if struct.unpack_from("<H", data, optional)[0] != 0x10B:
        raise ValueError(f"{path}: expected PE32 optional header")
    image_base = struct.unpack_from("<I", data, optional + 28)[0]
    section_table = optional + optional_size
    sections = []
    for index in range(section_count):
        header = section_table + index * 40
        name = data[header:header + 8].split(b"\0", 1)[0].decode("ascii", "replace")
        virtual_size, virtual_address, raw_size, raw_pointer = struct.unpack_from(
            "<IIII", data, header + 8
        )
        sections.append(
            {
                "name": name,
                "virtual_size": virtual_size,
                "virtual_address": virtual_address,
                "raw_size": raw_size,
                "raw_pointer": raw_pointer,
            }
        )
    return image_base, sections


def locate_address(
    address: int,
    image_base: int | None,
    sections: list[dict[str, int | str]],
    binary_data: bytes,
) -> tuple[str, str, str, str]:
    if image_base is None:
        return "", "binary unavailable", "", ""
    rva = address - image_base
    for section in sections:
        delta = rva - int(section["virtual_address"])
        if 0 <= delta < max(int(section["virtual_size"]), int(section["raw_size"])):
            if delta >= int(section["raw_size"]):
                return str(section["name"]), "zero-filled virtual tail", "", ""
            offset = int(section["raw_pointer"]) + delta
            count = min(16, int(section["raw_size"]) - delta)
            raw = binary_data[offset:offset + count].hex()
            return str(section["name"]), "file-backed", f"0x{offset:x}", raw
    return "", "outside mapped sections", "", ""


if __name__ == "__main__":
    main()
