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

    python3 tools/opt_ref_link.py callers [--build build-reccmp]
        [--out build-reccmp/opt-ref] [--object NAME ...] [--min N] [--json out.json]
        For each object with at least N removed annotated functions
        (default 15), list them with their callers in the original (from
        its direct calls, so not through tables) and the state of each
        caller in the rebuild: ``live`` (kept by the relink), ``removed``
        or ``unmapped``. For a live caller it names what the rebuilt caller
        calls at the original's call site instead, which is the port
        substitute to switch (#589, finding 579). ``roots`` names the
        objects of the topmost removed callers: the removed functions a
        live caller has to reach first. It needs ``capstone`` and
        ``reference/LegoRR.exe``. ``--summary`` instead prints every
        removed annotated function, whatever ``--min``, grouped by owning
        unit (``docs/coverage/translation-units.csv``) and by cause (#562,
        finding 637): ``folded`` (``/OPT:ICF`` folded it into an identical
        function of another object), ``live_caller`` (a root's original
        caller is live, so the rebuilt caller calls a port substitute),
        ``unmapped_caller`` (a root's original caller is unannotated or
        unmapped), ``data_reference`` (a root is referred to only from a data
        table) or ``unreferenced`` (nothing in the original refers to a
        root). Callers include functions whose code holds a pointer to the
        callee (callbacks).

``binary_progress.py`` records ``measure``'s result as ``opt_ref`` in
``binary-progress.json`` when the relink exists; ``make match-progress``
runs ``relink`` first.
"""

from __future__ import annotations

import argparse
import bisect
import collections
import difflib
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


# ---------------------------------------------------------------------------
# Callers of the removed functions (#589, finding 579)


def call_site_substitutes(original_calls: list[int], rebuilt_calls: list[int],
                          to_rebuilt: dict[int, int], target: int) -> list:
    """What a rebuilt caller calls where the original calls ``target``.

    ``original_calls`` and ``rebuilt_calls`` are the direct call targets of
    one function in the original and in the rebuild, in address order.
    ``to_rebuilt`` maps original addresses to rebuilt ones. The two
    sequences are aligned on the calls that agree; inside a differing run of
    equal length each original call pairs with the rebuilt call at the same
    position, otherwise with the whole run (a list). ``None`` means the
    rebuild has no call there.
    """
    mapped = [to_rebuilt.get(t, ("original", t)) for t in original_calls]
    out = []
    matcher = difflib.SequenceMatcher(None, mapped, rebuilt_calls, autojunk=False)
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        for k in range(i1, i2):
            if original_calls[k] != target or tag == "equal":
                continue
            if tag == "delete":
                out.append(None)
            elif i2 - i1 == j2 - j1:
                out.append(rebuilt_calls[j1 + k - i1])
            else:
                out.append(list(rebuilt_calls[j1:j2]))
    return out


def dead_roots(removed: set[int], callers: dict[int, set[int]]) -> dict[int, set[int]]:
    """The topmost removed callers of each removed function.

    A root is a removed function that no removed function calls; reviving
    it (a live caller) revives everything below it. A removed function
    without removed callers is its own root.
    """
    memo: dict[int, set[int]] = {}

    def walk(addr, stack):
        if addr in memo:
            return memo[addr]
        up = [c for c in callers.get(addr, ()) if c in removed and c not in stack]
        if not up:
            result = {addr}
        else:
            result = set()
            for c in up:
                result |= walk(c, stack | {addr})
        memo[addr] = result
        return result
    return {a: walk(a, frozenset()) for a in removed}


def _direct_calls(md, image: bytes, base: int, start: int, end: int) -> list[int]:
    return [int(i.op_str, 16) for i in md.disasm(image[start - base:end - base], start)
            if i.mnemonic == "call" and i.op_str.startswith("0x")]


CAUSES = ("folded", "live_caller", "unmapped_caller", "data_reference",
          "unreferenced")


def pointer_references(image: bytes, base: int, targets: set[int],
                       start: int, end: int) -> dict[int, list[int]]:
    """32-bit little-endian values in ``image[start:end]`` (virtual addresses)
    equal to one of ``targets``, at any byte offset: target -> referring
    addresses. In code these are pushed or stored function pointers."""
    found = collections.defaultdict(list)
    lo = start - base
    data = image[lo:end - base]
    for k in range(4):
        n = (len(data) - k) // 4
        values = memoryview(data[k:k + 4 * n]).cast("I")
        for i, v in enumerate(values):
            if v in targets:
                found[v].append(start + k + 4 * i)
    return {t: sorted(v) for t, v in found.items()}


def root_causes(roots: set[int], callers: dict[int, set[int]], removed: set[int],
                mapped: set[int], data_referenced: set[int] = frozenset()) -> list[str]:
    """Why the roots of one removed function have no live caller in the relink.

    A root's original callers (direct calls, or pointers to it in code) are
    not removed (that is what makes it a root), so each one is either live
    in the rebuild (and calls a port substitute there) or unmapped. A root
    without one is reached only through a pointer in data
    (``data_reference``), or nothing in the original refers to it
    (``unreferenced``): the original's own floor.
    """
    causes = set()
    for r in roots:
        up = [c for c in callers.get(r, ()) if c not in removed]
        if not up:
            causes.add("data_reference" if r in data_referenced else "unreferenced")
        if any(c in mapped for c in up):
            causes.add("live_caller")
        if any(c not in mapped for c in up):
            causes.add("unmapped_caller")
    return [c for c in CAUSES if c in causes]


def primary_cause(causes: list[str]) -> str:
    """One cause per function, for counting: the first in ``CAUSES`` order."""
    return causes[0] if causes else "unreferenced"


def load_units(path: Path = ROOT / "docs/coverage/translation-units.csv") -> dict[int, str]:
    """Original address -> owning unit (``GAME-021 Lego.c``). A unit whose
    original file name is unknown is named after its rebuilt source,
    in brackets (``GAME-050 [weapons.c]``)."""
    import csv
    if not path.is_file():
        return {}

    def name(r):
        if r.get("unit_name"):
            return f"{r['unit_id']} {r['unit_name']}"
        source = Path(r.get("current_source") or "").name
        return f"{r['unit_id']} [{source}]" if source else r["unit_id"]
    with path.open(encoding="utf-8", newline="") as f:
        units = {int(r["address"], 16): name(r) for r in csv.DictReader(f) if r.get("address")}
    # Addresses the per-function map does not list fall back on the unit
    # ranges, named as the unit's listed functions are.
    ranges = path.with_name("translation-unit-ranges.csv")
    if ranges.is_file():
        names = {}
        for label in units.values():
            names.setdefault(label.split(" ", 1)[0], label)
        with ranges.open(encoding="utf-8", newline="") as f:
            spans = sorted((int(r["text_start"], 16), int(r["text_end"], 16),
                            names.get(r["unit_id"], f"{r['unit_id']} {r['unit_name']}".strip()))
                           for r in csv.DictReader(f) if r.get("text_start") and r.get("text_end"))
        return UnitMap(units, spans)
    return units


class UnitMap(dict):
    """``load_units``'s mapping, which also answers addresses inside a unit's
    ``.text`` range."""

    def __init__(self, units: dict[int, str], spans: list[tuple[int, int, str]]):
        super().__init__(units)
        self.spans = spans
        self.starts = [s[0] for s in spans]

    def get(self, address, default=None):
        if address in self:
            return self[address]
        i = bisect.bisect_right(self.starts, address) - 1
        if i >= 0 and address < self.spans[i][1]:
            return self.spans[i][2]
        return default


def summarize(report: dict, units: dict[int, str]) -> dict:
    """Removed annotated functions by owning unit and by cause."""
    def blank():
        return {"removed": 0, "exact": 0, "bytes": 0,
                **{c: 0 for c in CAUSES}, **{c + "_exact": 0 for c in CAUSES}}
    by_unit = collections.defaultdict(blank)
    total = blank()
    for obj, data in report.items():
        for f in data["functions"]:
            unit = units.get(int(f["address"], 16), "unassigned")
            cause = primary_cause(f.get("causes", []))
            for row in (by_unit[unit], total):
                row["removed"] += 1
                row["exact"] += f["exact"]
                row["bytes"] += f.get("size", 0)
                row[cause] += 1
                row[cause + "_exact"] += f["exact"]
    return {"total": total,
            "by_unit": dict(sorted(by_unit.items(), key=lambda kv: (-kv[1]["removed"], kv[0])))}


def format_summary_table(summary: dict) -> str:
    head = ["unit", "removed", "exact", "bytes", *CAUSES]
    lines = ["| " + " | ".join(head) + " |", "|" + " --- |" + " ---: |" * (len(head) - 1)]

    def row(name, r):
        cells = [name, str(r["removed"]), str(r["exact"]), f"{r['bytes']:,}"]
        cells += [f"{r[c]} ({r[c + '_exact']})" if r[c] else "0" for c in CAUSES]
        return "| " + " | ".join(cells) + " |"
    for unit, r in summary["by_unit"].items():
        lines.append(row(unit, r))
    lines.append(row("**total**", summary["total"]))
    return "\n".join(lines)


def caller_report(build: Path, out: Path, original: Path, objects=None,
                  min_removed: int = 15) -> dict:
    """Removed annotated functions per object, with their original callers."""
    import capstone
    import pefile
    rows = load_rows(build / "reccmp-report/results.json") or []
    gate_map = (build / "LegoRR.map").read_text(encoding="latin-1")
    _, ref_symbols = pe_audit.parse_map((out / "LegoRR.map").read_text(encoding="latin-1"))
    # Removal is by (name, object), as in ``removed_code``. /OPT:ICF folds an
    # identical function into another object's: its name stays in the map,
    # listed under the object whose copy is kept.
    kept = {(s["name"], s["object"]) for s in ref_symbols if s["section"] == 1}
    kept_names = {s["name"]: s["object"] for s in ref_symbols if s["section"] == 1}
    annotated = {int(r["recomp"], 16): r for r in rows if r.get("recomp")}
    gate = winapi_link_audit.text_symbols(gate_map, link_order.load(), set(annotated))
    by_object = collections.defaultdict(list)
    removed = {}
    sizes = {}
    folded_into = {}
    for s in gate:
        row = annotated.get(s["address"])
        if row is not None and (s["name"], s["object"]) not in kept:
            orig = int(row["address"], 16)
            removed[orig] = s["object"]
            sizes[orig] = s["size"]
            if s["name"] in kept_names:
                folded_into[orig] = kept_names[s["name"]]
            by_object[s["object"]].append((orig, row, s["name"]))
    to_rebuilt = {int(r["address"], 16): int(r["recomp"], 16) for r in rows if r.get("recomp")}
    by_original = {int(r["address"], 16): r for r in rows}
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    o_pe, r_pe = pefile.PE(str(original)), pefile.PE(str(build / "LegoRR.exe"))
    o_img, r_img = o_pe.get_memory_mapped_image(), r_pe.get_memory_mapped_image()
    base = o_pe.OPTIONAL_HEADER.ImageBase
    o_text = next(s for s in o_pe.sections if pe_audit.section_name(s) == ".text")
    t0 = base + o_text.VirtualAddress
    t1 = t0 + o_text.Misc_VirtualSize
    # Function starts: the annotated ones and every direct call target.
    starts = set(by_original) | {t for t in _direct_calls(md, o_img, base, t0, t1) if t0 <= t < t1}
    starts = sorted(starts)

    def o_end(a):
        i = bisect.bisect_right(starts, a)
        return starts[i] if i < len(starts) else min(a + 0x1000, t1)
    callers = collections.defaultdict(set)
    o_calls = {}
    # Every function start can call; one without a report row is unannotated
    # and counts as an unmapped caller.
    for a in starts:
        if not t0 <= a < t1:
            continue
        o_calls[a] = _direct_calls(md, o_img, base, a, o_end(a))
        for t in o_calls[a]:
            if t in by_original and t != a:
                callers[t].add(a)
    # Pointers to annotated functions: in code they make the containing
    # function a caller; elsewhere (tables) they only mark the target.
    data_referenced = set()
    targets = set(by_original)
    for sec in o_pe.sections:
        s0 = base + sec.VirtualAddress
        s1 = s0 + max(sec.Misc_VirtualSize, 0)
        for t, refs in pointer_references(o_img, base, targets, s0, min(s1, s0 + len(o_img))).items():
            if s0 == t0:
                for ref in refs:
                    i = bisect.bisect_right(starts, ref) - 1
                    if i >= 0 and starts[i] != t and o_end(starts[i]) > ref:
                        callers[t].add(starts[i])
            else:
                data_referenced.add(t)
    r_text = sorted((s["address"], s["name"], s["object"]) for s in gate)
    r_addr = [t[0] for t in r_text]

    def r_name(a):
        i = bisect.bisect_right(r_addr, a) - 1
        return f"{r_text[i][1]} ({r_text[i][2]})" if i >= 0 and r_addr[i] == a else hex(a)

    def r_end(a):
        i = bisect.bisect_right(r_addr, a)
        return r_addr[i] if i < len(r_addr) else a + 0x1000
    roots = dead_roots(set(removed), callers)
    mapped = {a for a, r in by_original.items() if r.get("recomp")}
    removed_set = set(removed)
    report = {}
    for obj, entries in sorted(by_object.items(), key=lambda kv: (-len(kv[1]), kv[0])):
        if objects and obj not in objects:
            continue
        if not objects and len(entries) < min_removed:
            continue
        functions = []
        for orig, row, symbol in sorted(entries, key=lambda e: e[0]):
            calls = []
            for c in sorted(callers.get(orig, ())):
                crow = by_original.get(c, {})
                entry = {"address": f"{c:#x}", "name": crow.get("name")}
                if orig not in o_calls.get(c, ()):
                    entry["kind"] = "pointer"
                if not crow.get("recomp"):
                    entry["state"] = "unmapped"
                elif c in removed:
                    entry["state"] = "removed"
                else:
                    entry["state"] = "live"
                    r = int(crow["recomp"], 16)
                    subs = call_site_substitutes(
                        o_calls.get(c, []), _direct_calls(md, r_img, base, r, r_end(r)),
                        to_rebuilt, orig)
                    entry["rebuilt_calls"] = [
                        None if x is None else [r_name(y) for y in x] if isinstance(x, list)
                        else r_name(x) for x in subs]
                calls.append(entry)
            functions.append({"address": f"{orig:#x}", "name": row.get("name"),
                              "symbol": symbol, "exact": row.get("matching") == 1.0,
                              "size": sizes[orig], "callers": calls,
                              "causes": (["folded"] if orig in folded_into else [])
                              + root_causes(roots[orig], callers, removed_set, mapped, data_referenced),
                              **({"folded_into": folded_into[orig]} if orig in folded_into else {}),
                              "roots": sorted({removed[r] for r in roots[orig]}),
                              "root_functions": [f"{r:#x}" for r in sorted(roots[orig])]})
        report[obj] = {"removed": len(functions),
                       "exact_removed": sum(f["exact"] for f in functions),
                       "functions": functions}
    return report


def format_callers(report: dict) -> str:
    lines = []
    for obj, data in report.items():
        lines.append(f"{obj}: {data['removed']} removed, {data['exact_removed']} exact")
        for f in data["functions"]:
            roots = ", ".join(r for r in f["roots"] if r != obj) or "this object"
            lines.append(f"  {f['address']} {'E' if f['exact'] else ' '} {f['name']}"
                         f"  [roots: {roots}]")
            for c in f["callers"]:
                arrow = "<& " if c.get("kind") == "pointer" else "<- "
                text = f"      {arrow}{c['address']} {c['name']} ({c['state']})"
                if c.get("rebuilt_calls"):
                    text += " calls instead: " + "; ".join(
                        "no call" if x is None else " | ".join(x) if isinstance(x, list) else x
                        for x in c["rebuilt_calls"])
                lines.append(text)
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = parser.add_subparsers(dest="command", required=True)
    for name in ("relink", "measure", "callers"):
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
        if name == "callers":
            p.add_argument("--object", action="append", default=[],
                           help="only this object (map name, e.g. reward.c.obj)")
            p.add_argument("--min", type=int, default=15,
                           help="objects with at least this many removed functions")
            p.add_argument("--summary", action="store_true",
                           help="all removed functions, by owning unit and cause")
    args = parser.parse_args(argv)
    out = args.out or args.build / OUT_NAME
    if args.command == "relink":
        return relink(args.build, out, args.keep_debug, tuple(args.flag))
    for path in (args.build / "LegoRR.map", out / "LegoRR.map", out / "LegoRR.exe"):
        if not path.is_file():
            print(f"missing {path}; run make opt-ref-link first", file=sys.stderr)
            return 2
    if args.command == "callers":
        report = caller_report(args.build, out, args.original, set(args.object),
                               1 if args.summary else args.min)
        if args.summary:
            summary = summarize(report, load_units())
            if args.json:
                args.json.write_text(json.dumps({"summary": summary, "objects": report},
                                                indent=1) + "\n", encoding="utf-8")
            print(format_summary_table(summary))
            return 0
        if args.json:
            args.json.write_text(json.dumps(report, indent=1) + "\n", encoding="utf-8")
        print(format_callers(report))
        return 0
    result = measure(args.build, out, args.original)
    if args.json:
        args.json.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n",
                             encoding="utf-8")
    print(format_summary(result))
    return 0


if __name__ == "__main__":
    sys.exit(main())
