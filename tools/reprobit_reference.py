#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Reference objects and grind plans for ReproBit's discovery (docs/building.md).

`rbit discover grind` searches donor compiles of one function's unit and
keeps a donor whose body equals a *reference object*'s. That object must be
a COFF compile of the unit: the original only exists as LegoRR.exe, and
relocated fields cannot be qualified from bare bytes. This tool finds one.

    python3 tools/reprobit_reference.py ADDR [ADDR ...] [--pads 0 128]
        [--counts FIRST LAST] [--jobs 8] [--out reference/grind] [--json]

For each N in --pads it compiles the unit from a scratch copy with N inert
`extern int` pads after its first #include (phase_probe.py's pads, finding
664), with the matching build's flags. The first compile whose function body
equals the original's bytes, outside relocated fields, is written unchanged
to OUT/<addr>.obj. Next to it, OUT/<addr>.json is a grind plan for that
function: an `extern_runs` search over --counts declarations (default: 64
lengths centred on the pad count) after the
unit's final include, or at its start when an include follows the function
(a different seat from the pads, so the counts differ).
Run it with

    rbit discover grind . --expert-plan OUT/<addr>.json [--accept-progress]

OUT must lie under reference/, which is never committed: the object is a
compile whose bytes equal the original's. The unit and its translation unit
come from the annotations and reprobit/build-plan.json. A function no pad
count reproduces gets no files, nor does one the unpadded unit already
compiles to (its remaining difference is elsewhere). Needs the MSVC 5.0
toolchain and reference/LegoRR.exe; never run it while a build compiles the
tree.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import msvc_coff  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
BUILD_PLAN = "reprobit/build-plan.json"
# IMAGE_REL_I386_SECTION fixes up a 2-byte section index; the other i386
# relocations used by MSVC 5.0 objects patch 4 bytes.
_REL_I386_SECTION = 0x000A


class ReferenceError(Exception):
    pass


# ---------------------------------------------------------------------------
# Pure helpers


def function_body(coff: msvc_coff.CoffObject, symbol: str) -> tuple[bytes, list[tuple[int, int]]] | None:
    """The function's COMDAT body and its relocated spans (body-relative).

    With /Gy each function has a section of its own, so the body runs from
    the symbol to the section's end, alignment fill included: grind compares
    the whole COMDAT body.
    """
    matches = [s for s in coff.symbols.values()
               if s.name == symbol and s.section > 0 and s.is_function]
    if len(matches) != 1:
        return None
    sym = matches[0]
    sect = coff.section(sym.section)
    body = sect.data[sym.value:sect.size]
    spans = []
    for reloc in sect.relocations:
        start = reloc.offset - sym.value
        width = 2 if reloc.type == _REL_I386_SECTION else 4
        if 0 <= start and start + width <= len(body):
            spans.append((start, start + width))
    return body, sorted(spans)


def masked(data: bytes, spans: list[tuple[int, int]]) -> bytes:
    out = bytearray(data)
    for start, end in spans:
        out[start:end] = bytes(end - start)
    return bytes(out)


def body_matches(body: bytes, spans: list[tuple[int, int]], original: bytes) -> bool:
    """Whether a compiled body equals the original's outside relocated fields."""
    return len(original) == len(body) and masked(body, spans) == masked(original, spans)


def synthetic_reference(data: bytes, symbol: str, original: bytes) -> bytes | None:
    """``data`` with ``symbol``'s body replaced by the original's bytes.

    The compile's relocated fields are kept: they hold addends, not linked
    addresses. None when the bodies differ in length (no reordering of the
    compile's instructions can reach the original) or already agree.
    """
    coff = msvc_coff.parse_coff(data, "synthetic reference")
    result = function_body(coff, symbol)
    if result is None:
        return None
    body, spans = result
    if len(original) != len(body):
        return None
    target = bytearray(original)
    for start, end in spans:
        target[start:end] = body[start:end]
    if bytes(target) == body:
        return None
    sym = next(s for s in coff.symbols.values()
               if s.name == symbol and s.section > 0 and s.is_function)
    optional_size = struct.unpack_from("<H", data, 16)[0]
    header = 20 + optional_size + (sym.section - 1) * 40
    raw = struct.unpack_from("<I", data, header + 20)[0] + sym.value
    out = bytearray(data)
    out[raw:raw + len(body)] = target
    return bytes(out)


def translation_unit(build_plan: dict, source: str) -> dict:
    units = [u for u in build_plan.get("translation_units", []) if u.get("source") == source]
    if len(units) != 1:
        raise ReferenceError(f"{BUILD_PLAN} has {len(units)} translation units for {source}")
    return units[0]


def run_placement(source_text: str, function_line: int) -> str:
    """Where the grind's extern run must sit to precede the function.

    ReproBit seats it after the unit's final #include by default; a unit that
    includes again after the function (terrain_map.c, frontend_unit.c) needs
    it at the unit's start (``prefix``).
    """
    includes = [number for number, line in enumerate(source_text.splitlines(), start=1)
                if line.startswith("#include")]
    return "prefix" if includes and includes[-1] > function_line else "after_includes"


def count_window(pads: int, width: int = 64) -> tuple[int, int]:
    """The extern-run lengths a grind plan searches for a reference found at `pads`.

    The run sits at a different seat from the pads, so its length differs a
    little; a window of `width` lengths (grind's limit) centred on the pad
    count, starting at 1 at the least.
    """
    start = max(1, pads - (width // 2 - 1))
    return start, start + width - 1


def grind_plan(*, unit: dict, symbol: str, reference: str, counts: tuple[int, int],
               placement: str = "after_includes") -> bytes:
    """The canonical JSON of an extern-run grind plan (ReproBit's ProjectGrindPlan)."""
    runs: dict = {"counts": {"start": counts[0], "stop": counts[1], "step": 1}}
    if placement != "after_includes":
        runs["placement"] = placement
    plan = {
        "extern_runs": runs,
        "reference_object": reference,
        "schema_version": 1,
        "symbol": symbol,
        "target": unit["target_id"],
        "translation_unit": unit["id"],
    }
    return (json.dumps(plan, sort_keys=True, separators=(",", ":")) + "\n").encode()


def original_bytes(exe: bytes, address: int, size: int) -> bytes:
    """`size` bytes of the original's image at virtual address `address`."""
    pe = struct.unpack_from("<I", exe, 0x3C)[0]
    base = struct.unpack_from("<I", exe, pe + 0x34)[0]
    count = struct.unpack_from("<H", exe, pe + 6)[0]
    offset = pe + 24 + struct.unpack_from("<H", exe, pe + 20)[0]
    rva = address - base
    for _ in range(count):
        vsize, va, rsize, raw = struct.unpack_from("<IIII", exe, offset + 8)
        if va <= rva < va + max(vsize, rsize):
            start = raw + rva - va
            return exe[start:start + size]
        offset += 40
    raise ReferenceError(f"{address:#x} is in no section of the original")


def output_dir(path: Path) -> Path:
    resolved = path.resolve()
    reference = (ROOT / "reference").resolve()
    if resolved != reference and reference not in resolved.parents:
        raise ReferenceError(f"--out must lie under reference/ (never committed): {path}")
    return resolved


# ---------------------------------------------------------------------------
# Compiling


@dataclass
class Found:
    address: int
    symbol: str
    pads: int | None
    placement: str = "after_includes"
    synthetic: bool = False
    object_path: str | None = None
    plan_path: str | None = None


def find_references(addresses: list[int], *, pads: tuple[int, int], jobs: int,
                    synthetic: bool = False) -> tuple[Path, list[Found], dict[int, bytes]]:
    """Sweep pad counts for one unit; the first exact compile per address."""
    import match_function as mf
    import phase_probe as pp

    source = pp.unit_for(addresses)
    unit = pp.Unit(source)
    try:
        guard = pp.InPlace(unit.source, unit.state)
        try:
            scored = unit.score(guard, guard.original, addresses)
        finally:
            guard.close()
        symbols = {}
        for address in addresses:
            symbol = scored[address].symbol
            if not symbol:
                raise ReferenceError(f"{address:#x}: {scored[address].error or 'no compiled symbol'}")
            symbols[address] = symbol
        exe = mf.find_orig(argparse.Namespace(orig=None)).read_bytes()
        lines = guard.original.splitlines(keepends=True)
        pad_line = pp.default_pad_line(lines)
        sweep = unit.workdir / "reference"
        sweep.mkdir(exist_ok=True)

        def compile_pads(n: int) -> tuple[int, bytes | None]:
            cand = sweep / f"n{n:05d}{source.suffix}"
            cand.write_text(pp.pad_text(lines, {pad_line: n}) if n else "".join(lines))
            try:
                obj, _ = mf.compile_source(unit.env, unit.command, sweep, n, cand)
                data = obj.read_bytes()
                obj.unlink(missing_ok=True)
                return n, data
            except mf.MatchError:
                return n, None
            finally:
                cand.unlink(missing_ok=True)

        found = {
            a: Found(a, symbols[a], None, run_placement(guard.original, unit.markers[a].line))
            for a in addresses
        }
        objects: dict[int, bytes] = {}
        counts = list(range(pads[0], pads[1] + 1))
        with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, jobs)) as pool:
            for start in range(0, len(counts), max(1, jobs)):
                for n, data in sorted(pool.map(compile_pads, counts[start:start + jobs])):
                    if data is None:
                        continue
                    coff = msvc_coff.parse_coff(data, f"pads {n}")
                    for address, item in found.items():
                        if item.pads is not None:
                            continue
                        result = function_body(coff, item.symbol)
                        if result is None:
                            continue
                        body, spans = result
                        if body_matches(body, spans, original_bytes(exe, address, len(body))):
                            item.pads = n
                            objects[address] = data
                if all(item.pads is not None for item in found.values()):
                    break
        if synthetic and any(item.pads is None for item in found.values()):
            _n, base = compile_pads(0)
            if base is not None:
                for address, item in found.items():
                    if item.pads is not None:
                        continue
                    result = function_body(msvc_coff.parse_coff(base, "unpadded compile"), item.symbol)
                    if result is None:
                        continue
                    data = synthetic_reference(base, item.symbol,
                                               original_bytes(exe, address, len(result[0])))
                    if data is not None:
                        item.pads, item.synthetic = 0, True
                        objects[address] = data
        return source, list(found.values()), objects
    finally:
        unit.close()


def write_outputs(source: Path, found: list[Found], objects: dict[int, bytes], *, out: Path,
                  counts: tuple[int, int] | None) -> None:
    build_plan = json.loads((ROOT / BUILD_PLAN).read_text(encoding="utf-8"))
    unit = translation_unit(build_plan, source.relative_to(ROOT).as_posix())
    out.mkdir(parents=True, exist_ok=True)
    for item in found:
        if not item.pads and not item.synthetic:
            continue  # None: no pad count fits; 0: the source already compiles to it
        obj = out / f"{item.address:08x}.obj"
        plan = out / f"{item.address:08x}.json"
        obj.write_bytes(objects[item.address])
        reference = obj.relative_to(ROOT).as_posix()
        plan.write_bytes(grind_plan(unit=unit, symbol=item.symbol, reference=reference,
                                    counts=counts or ((1, 32) if item.synthetic else count_window(item.pads)),
                                    placement=item.placement))
        item.object_path = reference
        item.plan_path = plan.relative_to(ROOT).as_posix()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("addresses", nargs="+", type=lambda s: int(s, 16), metavar="ADDR")
    parser.add_argument("--pads", nargs=2, type=int, default=(0, 128), metavar=("FIRST", "LAST"),
                        help="pad counts to compile (default 0 128)")
    parser.add_argument("--counts", nargs=2, type=int, default=None, metavar=("FIRST", "LAST"),
                        help="extern-run lengths the grind plan searches (default: 64 lengths "
                             "centred on the pad count found; at most 64)")
    parser.add_argument("--jobs", type=int, default=min(8, os.cpu_count() or 1))
    parser.add_argument("--synthetic", action="store_true",
                        help="for an address no pad count reproduces, write a synthetic reference: "
                             "the unpadded compile with the function's body replaced by the "
                             "original's (for grind's donor rewriting)")
    parser.add_argument("--out", type=Path, default=ROOT / "reference" / "grind")
    parser.add_argument("--json", action="store_true")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        if args.counts is not None and (
                not 1 <= args.counts[0] <= args.counts[1] or args.counts[1] - args.counts[0] >= 64):
            raise ReferenceError("--counts must name 1 to 64 lengths from 1")
        if args.pads[0] < 0 or args.pads[1] < args.pads[0]:
            raise ReferenceError("--pads must be an increasing range from 0")
        out = output_dir(args.out)
        source, found, objects = find_references(args.addresses, pads=tuple(args.pads), jobs=args.jobs,
                                                 synthetic=args.synthetic)
        write_outputs(source, found, objects, out=out,
                      counts=tuple(args.counts) if args.counts is not None else None)
    except Exception as exc:  # pylint: disable=broad-except
        if type(exc).__name__ not in ("ReferenceError", "ProbeError", "MatchError"):
            raise
        print(f"error: {exc}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps([vars(item) for item in found], indent=2))
    else:
        for item in found:
            if item.pads is None:
                print(f"{item.address:#010x} {item.symbol}: no pad count in {args.pads[0]}..{args.pads[1]} "
                      "reproduces the original's body")
            elif item.synthetic:
                print(f"{item.address:#010x} {item.symbol}: synthetic reference {item.object_path} "
                      "(the original's body in the unpadded compile)")
                print(f"  rbit discover grind . --expert-plan {item.plan_path} [--accept-progress]")
            elif item.pads == 0:
                print(f"{item.address:#010x} {item.symbol}: the unit already compiles to the original's "
                      "body (outside relocated fields); nothing for grind to find")
            else:
                print(f"{item.address:#010x} {item.symbol}: {item.pads} pads; reference {item.object_path}"
                      f"; extern run {item.placement.replace('_', ' ')}")
                print(f"  rbit discover grind . --expert-plan {item.plan_path} [--accept-progress]")
    return 0 if all(item.pads is not None for item in found) else 1


if __name__ == "__main__":
    sys.exit(main())
