#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""The original's link mode, applied to the matching build's objects (#562, finding 576).

The gate build links with ``/DEBUG``, which makes LINK 5.0 default to
``/OPT:NOREF``: every function COMDAT stays, whether or not anything
references it. The original was linked without ``/DEBUG``, so ``/OPT:REF``
was in effect (finding 563). This tool relinks the gate build's objects in
that mode and measures what it removes.

    python3 tools/opt_ref_link.py relink [--build build-reccmp]
        [--out build-reccmp/opt-ref] [--keep-debug] [--flag /X ...]
        Rerun the gate's link command (``link.txt``) with its outputs in
        ``--out``. By default ``/DEBUG`` and ``/pdb:`` are dropped, which is
        the original's mode. ``--keep-debug`` keeps them (and the PDB), and
        ``--flag`` appends options such as ``/OPT:REF``. It needs the
        toolchain; it takes about 30 s.

    python3 tools/opt_ref_link.py measure [--build build-reccmp]
        [--out build-reccmp/opt-ref] [--json out.json]
        Compare the gate's map with the relink's: the ``.text`` bytes and
        functions ``/OPT:REF`` removes, by link-order category (the
        categories of ``winapi_link_audit.py``), the annotated functions it
        removes and how many of them are exact, and the relinked image's
        sections, thunks, entry point and import order.

``binary_progress.py`` records ``measure``'s result as ``opt_ref`` in
``binary-progress.json`` when the relink exists; ``make match-progress``
runs ``relink`` first.
"""

from __future__ import annotations

import argparse
import collections
import json
import shlex
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import link_order  # noqa: E402
import pe_audit  # noqa: E402
import winapi_link_audit  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_BUILD = ROOT / "build-reccmp"
OUT_NAME = "opt-ref"
CATEGORIES = ("original_annotated", "port_unit", "port_legacy", "port_tail", "libc",
              "import_thunks", "other")


# ---------------------------------------------------------------------------
# Relink


def relink_command(link_text: str, out: Path, keep_debug: bool = False,
                   flags: tuple[str, ...] = ()) -> list[str]:
    """The gate's link command, with outputs in ``out`` and the link mode changed.

    ``link_text`` is CMake's ``link.txt``. Without ``keep_debug`` the command
    loses ``/DEBUG`` and ``/pdb:``, so LINK 5.0 applies ``/OPT:REF``.
    """
    lines = [l for l in link_text.splitlines() if "/out:" in l.lower()]
    if len(lines) != 1:
        raise ValueError("link.txt must hold exactly one link command with /out:")
    out = Path(out)
    replace = {"/out:": out / "LegoRR.exe", "/implib:": out / "LegoRR.lib",
               "/pdb:": out / "LegoRR.pdb", "/map:": out / "LegoRR.map"}
    command = []
    for token in shlex.split(lines[0], posix=True):
        low = token.lower()
        if not keep_debug and (low == "/debug" or low.startswith("/pdb:")):
            continue
        prefix = next((p for p in replace if low.startswith(p)), None)
        command.append(token[:len(prefix)] + str(replace[prefix]) if prefix else token)
    return command + list(flags)


def relink(build: Path, out: Path, keep_debug: bool = False,
           flags: tuple[str, ...] = ()) -> int:
    link = build / "CMakeFiles/program.dir/link.txt"
    if not link.is_file():
        print(f"missing {link}; build the matching build first (make reccmp-report)",
              file=sys.stderr)
        return 2
    out.mkdir(parents=True, exist_ok=True)
    for name in ("LegoRR.exe", "LegoRR.map", "LegoRR.pdb", "LegoRR.lib", "LegoRR.exp"):
        (out / name).unlink(missing_ok=True)
    command = relink_command(link.read_text(encoding="utf-8"), out.resolve(), keep_debug, flags)
    result = subprocess.run(command, cwd=build)
    if result.returncode or not (out / "LegoRR.map").is_file():
        print(f"relink failed ({result.returncode})", file=sys.stderr)
        return result.returncode or 1
    return 0


# ---------------------------------------------------------------------------
# Measure


def text_symbols(map_text: str, entries, annotated: set[int] | None) -> dict:
    """``.text`` map symbols keyed by (name, object), with category, address and size."""
    return {(s["name"], s["object"]): s
            for s in winapi_link_audit.text_symbols(map_text, entries, annotated)}


def removed_code(gate_map: str, ref_map: str, entries, rows: list[dict] | None,
                 game_rows: list[dict] | None = None) -> dict:
    """What the relink removes from the gate's ``.text``.

    A gate symbol is removed when the relink's map has no ``.text`` symbol of
    the same name from the same object. Aliases at one address (LIBC's
    ``__chkstk`` and ``__alloca_probe``) count once, as the sized symbol. ``rows`` are Reccmp's full-report rows of the
    gate build (``results.json``); a removed symbol at a row's rebuilt address
    is a removed annotated function.
    """
    annotated = {int(r["recomp"], 16) for r in rows or [] if r.get("recomp")}
    gate = text_symbols(gate_map, entries, annotated if rows is not None else None)
    _, ref_symbols = pe_audit.parse_map(ref_map)
    kept = {(s["name"], s["object"]) for s in ref_symbols if s["section"] == 1}
    gone = [s for key, s in gate.items() if key not in kept]
    by_category = {}
    for cat in CATEGORIES:
        before = [s for s in gate.values() if s["category"] == cat]
        lost = [s for s in gone if s["category"] == cat]
        if before:
            by_category[cat] = {"functions": len(before), "bytes": sum(s["size"] for s in before),
                                "removed_functions": len(lost),
                                "removed_bytes": sum(s["size"] for s in lost)}
    # Annotated code by the link-order kind of its object.
    by_kind = {}
    for s in gate.values():
        if s["category"] == "original_annotated":
            row = by_kind.setdefault(s["kind"], {"functions": 0, "bytes": 0,
                                                 "removed_functions": 0, "removed_bytes": 0})
            row["functions"] += 1
            row["bytes"] += s["size"]
            if (s["name"], s["object"]) not in kept:
                row["removed_functions"] += 1
                row["removed_bytes"] += s["size"]
    result = {"gate_text_symbols": len(gate), "annotated_by_kind": dict(sorted(by_kind.items())), "removed_functions": len(gone),
              "removed_bytes": sum(s["size"] for s in gone), "by_category": by_category}
    removed_addresses = {s["address"] for s in gone}

    def annotated_loss(report_rows):
        mapped = [r for r in report_rows if r.get("recomp")]
        lost = [r for r in mapped if int(r["recomp"], 16) in removed_addresses]
        return {"mapped": len(mapped), "exact": sum(r.get("matching") == 1.0 for r in mapped),
                "removed": len(lost), "removed_exact": sum(r.get("matching") == 1.0 for r in lost)}
    if rows is not None:
        result["annotated"] = {"full": annotated_loss(rows)}
        if game_rows is not None:
            result["annotated"]["game_only"] = annotated_loss(game_rows)
    return result


def thunk_order(map_text: str) -> list[str]:
    """Import thunks and import-library data in ``.text``, in address order."""
    _, symbols = pe_audit.parse_map(map_text)
    out = []
    for s in sorted((s for s in symbols if s["section"] == 1), key=lambda s: s["offset"]):
        obj = s["object"]
        if pe_audit.IMPORT_OBJECT.match(obj) and not s["name"].startswith("__imp_"):
            out.append(f"{obj.split(':', 1)[1].lower()}:{s['name']}")
        elif obj.lower().startswith("dinput:"):
            out.append(f"dinput-data:{s['name']}")
    return out


def map_group_lengths(map_text: str) -> dict:
    groups, _ = pe_audit.parse_map(map_text)
    out = collections.Counter()
    for g in groups:
        out[g["name"]] += g["length"]
    return out


def image_summary(exe: Path, map_text: str, original: Path | None = None) -> dict:
    """Sections, entry point, thunks and import order of one linked image."""
    import pefile
    pe = pefile.PE(str(exe))
    groups = map_group_lengths(map_text)
    sections = {pe_audit.section_name(s): {"virtual_size": s.Misc_VirtualSize,
                                           "raw_size": s.SizeOfRawData}
                for s in pe.sections}
    thunks = [t for t in thunk_order(map_text) if not t.startswith("dinput-data:")]
    out = {"sections": sections,
           "data_initialised_bytes": sum(v for k, v in groups.items()
                                         if k.startswith(".data")),
           "bss_bytes": groups.get(".bss", 0),
           "entry_point_rva": pe.OPTIONAL_HEADER.AddressOfEntryPoint,
           "characteristics": pe.FILE_HEADER.Characteristics,
           "debug_directory": tuple(pe.OPTIONAL_HEADER.DATA_DIRECTORY[6].dump_dict()[k]["Value"]
                                    for k in ("VirtualAddress", "Size")),
           "import_thunks": len(thunks),
           "thunk_order": thunk_order(map_text),
           "import_dlls": [d.lower() for d, _ in pe_audit.imports(pe)],
           "imported_names": sum(len(n) for _, n in pe_audit.imports(pe))}
    if original is not None and Path(original).is_file():
        cmp = pe_audit.compare_imports(pe_audit.imports(pefile.PE(str(original))),
                                       pe_audit.imports(pe))
        out["common_dll_order_equal"] = cmp["common_dll_order_equal"]
    return out


def load_rows(path: Path) -> list[dict] | None:
    if not path.is_file():
        return None
    data = json.loads(path.read_text(encoding="utf-8"))
    return data["data"] if isinstance(data, dict) else data


def measure(build: Path, out: Path, original: Path | None = None,
            report_dir: Path | None = None) -> dict:
    """The ``opt_ref`` record of ``binary-progress.json``."""
    report_dir = report_dir or build / "reccmp-report"
    gate_map = (build / "LegoRR.map").read_text(encoding="latin-1")
    ref_map = (out / "LegoRR.map").read_text(encoding="latin-1")
    entries = link_order.load()
    result = removed_code(gate_map, ref_map, entries, load_rows(report_dir / "results.json"),
                          load_rows(report_dir / "game-only.json"))
    result["image"] = image_summary(out / "LegoRR.exe", ref_map, original)
    return result


def format_summary(result: dict) -> str:
    lines = [f"/OPT:REF removes {result['removed_functions']:,} of "
             f"{result['gate_text_symbols']:,} .text symbols, "
             f"{result['removed_bytes']:,} bytes:"]
    for cat, row in result["by_category"].items():
        lines.append(f"  {cat:20} {row['removed_bytes']:>9,} of {row['bytes']:>9,} bytes "
                     f"{row['removed_functions']:>6,} of {row['functions']:>6,} functions")
    for kind, row in result["annotated_by_kind"].items():
        lines.append(f"    annotated in {kind:8} {row['removed_bytes']:>9,} of {row['bytes']:>9,} bytes "
                     f"{row['removed_functions']:>6,} of {row['functions']:>6,} functions")
    for view, row in result.get("annotated", {}).items():
        lines.append(f"annotated ({view}): {row['removed']:,} of {row['mapped']:,} removed, "
                     f"{row['removed_exact']:,} of {row['exact']:,} exact")
    img = result.get("image")
    if img:
        sizes = ", ".join(f"{k} {v['virtual_size']:,}" for k, v in img["sections"].items())
        lines.append(f"image: {sizes}; .data initialised {img['data_initialised_bytes']:,}, "
                     f".bss {img['bss_bytes']:,}")
        lines.append(f"entry point {img['entry_point_rva']:#x}, thunks {img['import_thunks']}, "
                     f"dlls {' '.join(img['import_dlls'])}"
                     + (f", common order equal {img['common_dll_order_equal']}"
                        if "common_dll_order_equal" in img else ""))
        lines.append("thunk order: " + " ".join(img["thunk_order"]))
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = parser.add_subparsers(dest="command", required=True)
    for name in ("relink", "measure"):
        p = sub.add_parser(name)
        p.add_argument("--build", type=Path, default=DEFAULT_BUILD)
        p.add_argument("--out", type=Path)
        if name == "relink":
            p.add_argument("--keep-debug", action="store_true",
                           help="keep /DEBUG and the PDB (LINK then defaults to /OPT:NOREF)")
            p.add_argument("--flag", action="append", default=[],
                           help="append a link option, e.g. /OPT:REF")
        else:
            p.add_argument("--original", type=Path, default=ROOT / "reference/LegoRR.exe")
            p.add_argument("--json", type=Path)
    args = parser.parse_args(argv)
    out = args.out or args.build / OUT_NAME
    if args.command == "relink":
        return relink(args.build, out, args.keep_debug, tuple(args.flag))
    for path in (args.build / "LegoRR.map", out / "LegoRR.map", out / "LegoRR.exe"):
        if not path.is_file():
            print(f"missing {path}; run make opt-ref-link first", file=sys.stderr)
            return 2
    result = measure(args.build, out, args.original)
    if args.json:
        args.json.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n",
                             encoding="utf-8")
    print(format_summary(result))
    return 0


if __name__ == "__main__":
    sys.exit(main())
