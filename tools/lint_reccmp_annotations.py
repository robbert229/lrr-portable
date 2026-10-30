#!/usr/bin/env python3
"""Require a Reccmp function or library annotation on source definitions.

This is intentionally a small source-level lint rather than a C++ parser. It
recognizes function bodies in production C/C++ translation units under ``src``
and checks for a Reccmp annotation near each definition.
"""

from __future__ import annotations

import argparse
import csv
import re
import sys
from pathlib import Path


ANNOTATION = re.compile(
    r"^\s*//\s*(?:FUNCTION|STUB|LIBRARY):\s+[A-Z0-9_]+\s+"
    r"0x[0-9a-fA-F]+(?:\s+SYMBOL)?\s*$"
)
LEGORR_ADDRESS_ANNOTATION = re.compile(
    r"^\s*//\s*(?:FUNCTION|STUB|LIBRARY):\s+LEGORR\s+"
    r"0x([0-9a-fA-F]+)(?:\s+SYMBOL)?\s*$"
)
IDENTIFIER = re.compile(r"[A-Za-z_][A-Za-z_0-9]*\s*$")
CONTROL_NAMES = {
    "if",
    "for",
    "while",
    "switch",
    "catch",
    "sizeof",
    "alignof",
    "decltype",
    "static_assert",
}
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx"}
DECLARATOR_SUFFIX = re.compile(
    r"\s*(?:(?:const|volatile|override|final|noexcept|&|&&)\s*)*"
    r"(?:->\s*[A-Za-z_][A-Za-z_0-9:\s*&<>]*)?\s*$"
)


def mask_comments_and_literals(source: str) -> str:
    """Replace comments and literals with spaces, preserving offsets/newlines."""
    chars = list(source)
    i = 0
    state = "code"
    quote = ""
    while i < len(source):
        ch = source[i]
        nxt = source[i + 1] if i + 1 < len(source) else ""
        if state == "code":
            if ch == "/" and nxt == "/":
                chars[i] = chars[i + 1] = " "
                i += 2
                state = "line_comment"
                continue
            if ch == "/" and nxt == "*":
                chars[i] = chars[i + 1] = " "
                i += 2
                state = "block_comment"
                continue
            if ch in "'\"":
                quote = ch
                chars[i] = " "
                state = "literal"
        elif state == "line_comment":
            if ch == "\n":
                state = "code"
            else:
                chars[i] = " "
        elif state == "block_comment":
            if ch == "*" and nxt == "/":
                chars[i] = chars[i + 1] = " "
                i += 2
                state = "code"
                continue
            if ch != "\n":
                chars[i] = " "
        else:
            if ch == "\\":
                chars[i] = " "
                if i + 1 < len(source) and source[i + 1] != "\n":
                    chars[i + 1] = " "
                i += 2
                continue
            if ch == quote:
                chars[i] = " "
                state = "code"
            elif ch != "\n":
                chars[i] = " "
        i += 1
    return "".join(chars)


def matching_open_paren(masked: str, close: int) -> int | None:
    depth = 0
    for i in range(close, -1, -1):
        if masked[i] == ")":
            depth += 1
        elif masked[i] == "(":
            depth -= 1
            if depth == 0:
                return i
    return None


def function_definitions(source: str) -> list[tuple[int, str]]:
    """Return (definition offset, function name) pairs for recognizable bodies."""
    masked = mask_comments_and_literals(source)
    found: list[tuple[int, str]] = []
    seen: set[int] = set()

    for brace, char in enumerate(masked):
        if char != "{":
            continue
        boundary = max(masked.rfind(";", 0, brace), masked.rfind("{", 0, brace), masked.rfind("}", 0, brace)) + 1
        header = masked[boundary:brace]
        # A constructor's member initializer list may contain further calls;
        # only inspect the declarator before its top-level colon.
        depth = 0
        declarator_end = len(header)
        for i, token in enumerate(header):
            if token == "(":
                depth += 1
            elif token == ")":
                depth -= 1
            elif token == ":" and depth == 0:
                declarator_end = i
                break
        header = header[:declarator_end]

        # Function declarations have a parameter list; control-flow blocks,
        # lambdas, and aggregate initializers are filtered by the preceding name.
        close = header.rfind(")")
        if close < 0:
            continue
        open_paren = matching_open_paren(header, close)
        if open_paren is None:
            continue
        if not DECLARATOR_SUFFIX.fullmatch(header[close + 1 :]):
            continue
        before = header[:open_paren]
        name_match = IDENTIFIER.search(before)
        if name_match is None:
            continue
        name = name_match.group(0).strip()
        if name in CONTROL_NAMES:
            continue
        # A lambda's parameter list is preceded by `]`, not a function name.
        name_start = name_match.start()
        if before[:name_start].rstrip().endswith("]"):
            continue

        offset = boundary
        for line_match in re.finditer(r"[^\n]*\n|[^\n]+$", masked[boundary:brace]):
            if line_match.group(0).strip() and not line_match.group(0).lstrip().startswith("#"):
                offset = boundary + line_match.start()
                break
        if offset not in seen:
            found.append((offset, name))
            seen.add(offset)
    return found


def has_nearby_annotation(source: str, definition_offset: int) -> bool:
    line = source.count("\n", 0, definition_offset)
    lines = source.splitlines()
    # Permit blank lines between the marker and declaration, but don't let an
    # unrelated marker several comments away annotate this function.
    for prior in range(line - 1, max(-1, line - 5), -1):
        text = lines[prior].strip()
        if not text:
            continue
        if ANNOTATION.fullmatch(lines[prior]):
            return True
        if text.startswith("//"):
            # LIBRARY and name-reference annotations use a second comment
            # line for the exact PDB symbol; keep scanning that comment block.
            continue
        return False
    return False


def source_files(root: Path) -> list[Path]:
    return sorted(
        path
        for path in root.rglob("*")
        if path.is_file()
        and path.suffix.lower() in SOURCE_SUFFIXES
        and not path.name.lower().startswith("test_")
        and ".git" not in path.parts
        and "build" not in path.parts
        and not ("platform" in path.parts and "native" in path.parts)
    )


def ledger_functions(ledger_path: Path, statuses: set[str]) -> list[tuple[str, str]]:
    """Return corpus functions in the requested ledger statuses."""
    with ledger_path.open(newline="", encoding="utf-8") as ledger_file:
        reader = csv.DictReader(ledger_file)
        required = {"address", "status", "corpus_name", "symbol"}
        missing = required.difference(reader.fieldnames or ())
        if missing:
            fields = ", ".join(sorted(missing))
            raise ValueError(f"{ledger_path}: missing required CSV column(s): {fields}")

        functions = []
        for row in reader:
            if row["status"] not in statuses:
                continue
            address = row["address"].strip().lower().removeprefix("0x").zfill(8)
            name = row["corpus_name"].strip() or row["symbol"].strip()
            functions.append((address, name))
    return sorted(functions)


def annotated_ledger_addresses(root: Path) -> set[str]:
    """Return ledger addresses with a Reccmp function or library marker in src."""
    addresses = set()
    for path in source_files(root / "src"):
        for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
            match = LEGORR_ADDRESS_ANNOTATION.fullmatch(line)
            if match:
                addresses.add(match.group(1).lower().removeprefix("0x").zfill(8))
    return addresses


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", nargs="?", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument(
        "--ledger",
        type=Path,
        help="coverage ledger CSV; report named functions missing a FUNCTION/STUB/LIBRARY marker",
    )
    args = parser.parse_args()
    root = args.root.resolve()
    missing: list[tuple[Path, int, str]] = []

    for path in source_files(root / "src"):
        source = path.read_text(encoding="utf-8", errors="replace")
        for offset, name in function_definitions(source):
            if not has_nearby_annotation(source, offset):
                line = source.count("\n", 0, offset) + 1
                missing.append((path, line, name))

    if args.ledger:
        ledger_path = args.ledger if args.ledger.is_absolute() else root / args.ledger
        try:
            annotated = annotated_ledger_addresses(root)
            unported = [
                row for row in ledger_functions(ledger_path, {"named"})
                if row[0] not in annotated
            ]
        except (OSError, ValueError, csv.Error) as exc:
            print(f"Could not read Reccmp coverage ledger: {exc}", file=sys.stderr)
            return 2
        print(f"\nNamed functions missing FUNCTION/STUB/LIBRARY annotations: {len(unported)} from {ledger_path}")
        for address, name in unported:
            print(f"0x{address} {name}")

    if missing:
        print(f"Reccmp annotation lint failed: {len(missing)} function(s) lack FUNCTION/STUB/LIBRARY annotations", file=sys.stderr)
        for path, line, name in missing:
            print(f"{path.relative_to(root)}:{line}: {name}", file=sys.stderr)
        return 1

    print("Reccmp annotation lint passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
