#!/usr/bin/env python3
"""Require a Reccmp function or library annotation on source definitions.

This is intentionally a small source-level lint rather than a C++ parser. It
recognizes function bodies in production C/C++ translation units under ``src``
and checks for a Reccmp annotation near each definition.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
from collections import Counter, defaultdict
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


def function_definitions(source: str) -> list[tuple[int, str, int]]:
    """Return (definition offset, function name, body brace) for recognizable bodies."""
    masked = mask_comments_and_literals(source)
    found: list[tuple[int, str, int]] = []
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
            found.append((offset, name, brace))
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


BASELINE = "docs/coverage/reccmp-lint-baseline.txt"
HEADER = ("# Reccmp lint debt; sorted keys, repeated for duplicate definitions.\n"
          "# Run make reccmp-lint-update to remove fixed entries; use\n"
          "# make reccmp-lint-update RECCMP_LINT_UPDATE_ARGS=--allow-growth to add debt.\n")


def scopes_at_braces(masked: str) -> dict[int, str]:
    """Track named namespace and type scopes; other braces only balance the stack."""
    stack: list[str] = []
    scopes = {}
    boundary = 0
    for i, char in enumerate(masked):
        if char == "{":
            header = masked[boundary:i]
            match = re.search(r"\b(namespace|class|struct|union)\s+([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*(?:final\s*)?(?::[^{}]*)?$", header)
            stack.append(match.group(2) if match else "")
            scopes[i] = "::".join(part for part in stack[:-1] if part)
            boundary = i + 1
        elif char == "}":
            if stack:
                stack.pop()
            boundary = i + 1
        elif char == ";":
            boundary = i + 1
    return scopes


def collect(root: Path, ledger_path: Path | None) -> dict[str, list[str]]:
    violations: dict[str, list[str]] = defaultdict(list)
    for path in source_files(root / "src"):
        source = path.read_text(encoding="utf-8", errors="replace")
        masked = mask_comments_and_literals(source)
        scopes = scopes_at_braces(masked)
        for offset, name, brace in function_definitions(source):
            if has_nearby_annotation(source, offset):
                continue
            # Ignore whitespace and directives, not parameter names or types.
            signature = re.sub(r"(?m)^\s*\#.*$", "", masked[offset:brace])
            signature = " ".join(re.findall(r"[A-Za-z_]\w*|\d+|::|\S", signature))
            digest = hashlib.sha256(signature.encode()).hexdigest()[:16]
            scope = scopes.get(brace, "")
            key = f"definition {scope + '::' if scope else ''}{name} {digest}"
            line = source.count("\n", 0, offset) + 1
            violations[key].append(f"{path.relative_to(root)}:{line}: {name}")
    if ledger_path:
        annotated = annotated_ledger_addresses(root)
        for address, name in ledger_functions(ledger_path, {"named"}):
            if address not in annotated:
                violations[f"ledger 0x{address} {name}"].append(f"0x{address} {name}")
    return violations


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", nargs="?", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--ledger", type=Path, help="coverage ledger CSV")
    parser.add_argument("--baseline", type=Path, default=Path(BASELINE))
    parser.add_argument("--update", action="store_true", help="rewrite the baseline")
    parser.add_argument("--allow-growth", action="store_true", help="allow update to add debt")
    args = parser.parse_args()
    root = args.root.resolve()
    baseline = args.baseline if args.baseline.is_absolute() else root / args.baseline
    ledger = args.ledger if not args.ledger or args.ledger.is_absolute() else root / args.ledger
    try:
        violations = collect(root, ledger)
        current = Counter({key: len(locations) for key, locations in violations.items()})
        if baseline.exists():
            previous = Counter(line for line in baseline.read_text(encoding="utf-8").splitlines()
                               if line and not line.startswith("#"))
        elif args.update and args.allow_growth:
            previous = Counter()
        else:
            raise ValueError(f"baseline missing: {baseline}")
        new, fixed = current - previous, previous - current
        for key, count in sorted(new.items()):
            for location in violations[key][-count:]:
                print(f"new: {location} [{key}]")
        for key, count in sorted(fixed.items()):
            print(f"can be removed: {key}" + (f" (x{count})" if count > 1 else ""))
        if args.update:
            if new and not args.allow_growth:
                print("Refusing to add baseline entries; pass --allow-growth to accept them.", file=sys.stderr)
                return 1
            baseline.parent.mkdir(parents=True, exist_ok=True)
            baseline.write_text(HEADER + "".join(key + "\n" for key in sorted(current.elements())), encoding="utf-8")
            print(f"Updated {baseline}: {sum(current.values())} entries")
        elif new:
            print(f"Reccmp annotation lint failed: {sum(new.values())} new violation(s)", file=sys.stderr)
            return 1
        else:
            print(f"Reccmp annotation lint passed: {sum(current.values())} existing violation(s), {sum(fixed.values())} can be removed")
        return 0
    except (OSError, ValueError, csv.Error) as exc:
        print(f"Reccmp annotation lint: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
