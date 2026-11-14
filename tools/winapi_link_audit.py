#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Keep native-backend code out of winapi links (issue #576, finding 575).

The matching build (``LRR_CLASSIC_MSVC5``) and the MinGW reference build
(``PLATFORM=winapi``) should link only the original's code and the port tail
that the original-shaped units still call. Code that serves only the native
backend (SDL3, miniwin, ``src/platform/native/``, the native entry chain)
must never reach them.

    python3 tools/winapi_link_audit.py check-list
        Static check of the link-order list, no toolchain: no winapi entry is
        native-only, and the matching build's libraries name no SDL or
        miniwin library. Runs in ``make tools-test``.

    python3 tools/winapi_link_audit.py check-link --build build-reccmp \\
        [--report-dir build-reccmp/reccmp-report] [--json out.json]
        Check an existing matching build: its linker response file, link
        command and map. Fails on any object from a native-only source, any
        SDL or miniwin library, object or symbol, and any native entry-chain
        symbol. Then prints the linked ``.text`` by category. Runs in
        ``make reccmp-check`` and ``make match-progress``.

The category report splits ``.text`` into annotated original code, port code
(non-annotated code in unit, legacy and tail objects), LIBC and import
thunks. How much of it ``/OPT:REF`` would drop is #562's measure
(``opt_ref_link.py``, finding 576); it reuses these categories.
"""

from __future__ import annotations

import argparse
import json
import re
import shlex
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import link_order  # noqa: E402
import pe_audit  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
TEXT_BASE = 0x401000

# A source path that only the native backend may link.
NATIVE_PATH = re.compile(r"(^|/)platform/native/|(^|/)miniwin/|_miniwin\.|_native\.|(^|[/_])sdl",
                         re.I)
# Sources whose only callers are native. Each must be tagged ``native`` in
# the link-order list. The reasons are finding 575's inventory.
NATIVE_ONLY_SOURCES = {
    "main.cpp": "LegorrrMain; only platform/native/entry.cpp calls it",
    "gods98/gods98.cpp": "gods98_engine_name; only main.cpp calls it",
    "game/creature_scene.cpp": "native creature scenes (finding 555)",
    "gods98/container_native.cpp": "native container backend",
    "gods98/container_route.cpp": "native container routing",
    "gods98/lwsc_miniwin.cpp": "miniwin scene backend",
    "platform/native/entry.cpp": "native entry point",
    "platform/native/frameloop.cpp": "SDL3 frame loop and platform glue",
}
# A library, object or symbol from SDL or miniwin.
NATIVE_LIBRARY = re.compile(r"sdl|miniwin", re.I)
NATIVE_SYMBOL = re.compile(r"(?:^|[_?@])SDL_|miniwin", re.I)
# The native entry chain: present in a winapi link only if native-only code
# leaked back in.
NATIVE_ENTRY_SYMBOLS = ("LegorrrMain", "LegorrrRun", "gods98_engine_name", "game_layer_name")


def symbol_base(name: str) -> str:
    """Undecorated base name of a map symbol (C or MSVC C++)."""
    if name.startswith("?"):
        m = re.match(r"\?([A-Za-z_]\w*)@", name)
        return m.group(1) if m else name
    return name.lstrip("_").split("@")[0]


def winapi_entries(entries) -> list:
    return [e for e in entries if e.applies_to("winapi")]


def check_list(entries, libraries: list[str]) -> list[str]:
    """Problems in the link-order list's winapi view and its libraries."""
    problems = []
    for e in entries:
        reason = NATIVE_ONLY_SOURCES.get(e.source)
        if e.platform == "native":
            continue
        if NATIVE_PATH.search(e.source):
            problems.append(f"{link_order.LIST}:{e.line + 1}: native-only source {e.source} "
                            "is linked into winapi builds; tag it native")
        elif reason:
            problems.append(f"{link_order.LIST}:{e.line + 1}: {e.source} serves only the "
                            f"native backend ({reason}); tag it native")
    for lib in libraries:
        if NATIVE_LIBRARY.search(lib):
            problems.append(f"{link_order.LIST}: LRR_LINK_LIBRARIES names {lib}")
    return problems


def link_inputs(rsp_text: str, link_text: str) -> tuple[list[str], list[str]]:
    """Objects (response file) and libraries (link command) of a link."""
    objects = [t.replace("\\", "/") for t in shlex.split(rsp_text, posix=True)]
    libraries = [t for t in shlex.split(link_text, posix=True) if t.lower().endswith(".lib")
                 and not t.lower().startswith("/implib:")]
    return objects, libraries


def native_paths(entries) -> set[str]:
    """Source paths (under src/) that winapi links must not contain."""
    paths = {e.source for e in entries if e.platform == "native"}
    return paths | set(NATIVE_ONLY_SOURCES)


def check_link(entries, objects: list[str], libraries: list[str], map_text: str) -> list[str]:
    """Problems in one winapi link: its objects, libraries and map."""
    problems = []
    forbidden = native_paths(entries)
    for obj in objects:
        rel = obj.split("/src/", 1)[1] if "/src/" in obj else obj
        source = re.sub(r"\.(obj|o|res)$", "", rel)
        if NATIVE_PATH.search(rel) or source in forbidden:
            problems.append(f"object {obj} comes from a native-only source")
    for lib in libraries:
        if NATIVE_LIBRARY.search(lib):
            problems.append(f"library {lib} is a native-backend library")
    kinds = pe_audit.object_kinds(entries)
    _, symbols = pe_audit.parse_map(map_text)
    unlisted = set()
    for s in symbols:
        obj = s["object"]
        if NATIVE_LIBRARY.search(obj):
            problems.append(f"map object {obj} is native-backend code ({s['name']})")
        if NATIVE_SYMBOL.search(s["name"]):
            problems.append(f"map symbol {s['name']} ({obj}) is SDL or miniwin code")
        if symbol_base(s["name"]) in NATIVE_ENTRY_SYMBOLS:
            problems.append(f"map symbol {s['name']} ({obj}) belongs to the native entry chain")
        if pe_audit.classify_object(obj, kinds) == "other":
            unlisted.add(obj)
    for obj in sorted(unlisted):
        problems.append(f"map object {obj} is not a winapi entry of the link-order list")
    return problems


def text_symbols(map_text: str, entries, annotated: set[int] | None) -> list[dict]:
    """The linked .text's map symbols, each with its size and category.

    Each map symbol owns the bytes up to the next one. ``annotated`` holds the
    rebuilt addresses of Reccmp's mapped functions; without it every unit,
    legacy and tail function counts as port code.
    """
    groups, symbols = pe_audit.parse_map(map_text)
    kinds = pe_audit.object_kinds(entries)
    text = sorted((s for s in symbols if s["section"] == 1), key=lambda s: s["offset"])
    ends = [g["offset"] + g["length"] for g in groups if g["section"] == 1]
    end = max(ends) if ends else (text[-1]["offset"] if text else 0)
    out = []
    for i, s in enumerate(text):
        size = (text[i + 1]["offset"] if i + 1 < len(text) else end) - s["offset"]
        if size <= 0:
            continue
        kind = pe_audit.classify_object(s["object"], kinds)
        address = TEXT_BASE + s["offset"]
        if kind in ("unit", "legacy", "tail"):
            if annotated is not None and address in annotated:
                category = "original_annotated"
            else:
                category = "port_" + kind
        elif kind == "libc":
            category = "libc"
        elif kind == "import":
            category = "import_thunks"
        else:
            category = "other"
        out.append(dict(s, size=size, address=address, kind=kind, category=category))
    return out


def text_categories(map_text: str, entries, annotated: set[int] | None) -> dict:
    """Bytes and function counts of the linked .text, by category (``text_symbols``)."""
    out: dict[str, dict] = {}
    for s in text_symbols(map_text, entries, annotated):
        row = out.setdefault(s["category"], {"bytes": 0, "functions": 0})
        row["bytes"] += s["size"]
        row["functions"] += 1
    out["total"] = {"bytes": sum(r["bytes"] for r in out.values()),
                    "functions": sum(r["functions"] for r in out.values())}
    return out


def annotated_addresses(report_dir: Path) -> set[int] | None:
    path = report_dir / "results.json"
    if not path.is_file():
        return None
    data = json.loads(path.read_text(encoding="utf-8"))
    rows = data["data"] if isinstance(data, dict) else data
    return {int(r["recomp"], 16) for r in rows if r.get("recomp")}


def format_categories(categories: dict) -> str:
    lines = ["winapi link .text by category (what /OPT:REF would drop: "
             "tools/opt_ref_link.py):"]
    for name in ("original_annotated", "port_unit", "port_legacy", "port_tail", "libc",
                 "import_thunks", "other", "total"):
        if name in categories:
            row = categories[name]
            lines.append(f"  {name:20} {row['bytes']:>10,} bytes {row['functions']:>6} functions")
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = parser.add_subparsers(dest="command", required=True)
    p_list = sub.add_parser("check-list", help="check the link-order list's winapi view")
    p_list.add_argument("--root", type=Path, default=ROOT)
    p_link = sub.add_parser("check-link", help="check an existing matching build")
    p_link.add_argument("--build", type=Path, default=ROOT / "build-reccmp")
    p_link.add_argument("--report-dir", type=Path)
    p_link.add_argument("--json", type=Path)
    args = parser.parse_args(argv)

    if args.command == "check-list":
        text = (args.root / link_order.LIST).read_text(encoding="utf-8")
        problems = check_list(link_order.parse(text), link_order.libraries(text))
        for p in problems:
            print(p, file=sys.stderr)
        print("winapi link list check: " + ("FAILED" if problems else "OK"))
        return 1 if problems else 0

    entries = link_order.load()
    program = args.build / "CMakeFiles/program.dir"
    rsp = program / "objects1.rsp"
    link = program / "link.txt"
    map_path = args.build / "LegoRR.map"
    for path in (rsp, link, map_path):
        if not path.is_file():
            print(f"missing {path}; build the matching build first (make reccmp-report)",
                  file=sys.stderr)
            return 2
    objects, libraries = link_inputs(rsp.read_text(encoding="utf-8"),
                                     link.read_text(encoding="utf-8"))
    map_text = map_path.read_text(encoding="latin-1")
    problems = check_link(entries, objects, libraries, map_text)
    report_dir = args.report_dir or args.build / "reccmp-report"
    categories = text_categories(map_text, entries, annotated_addresses(report_dir))
    for p in problems:
        print(p, file=sys.stderr)
    print(format_categories(categories))
    print("winapi link check: " + ("FAILED" if problems else "OK"))
    if args.json:
        args.json.write_text(json.dumps({"problems": problems, "text": categories}, indent=2)
                             + "\n", encoding="utf-8")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
