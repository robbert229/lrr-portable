#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Symbol-counter phase probe (finding 538 rule B; findings 663 and 664).

Is a near miss a phase case, whose code depends only on how many symbols the
compiler numbered before it? Two subcommands:

  probe ADDR [ADDR ...] [--range 0 256 1] [--at LINE]
      N inert `extern int` pads go on one file-scope line of the unit (by
      default the first blank line after its first #include; --at LINE puts
      them in front of that line). Every N in the range is compiled from a
      scratch copy with /FAs and each function's listing is fingerprinted
      (symbol numbers removed). The first N of each distinct code is then
      compiled in place and scored with Reccmp. A function with one code
      over a full period takes its code from the source; one whose best code
      scores above N = 0 is phase-sensitive, and the report gives the shifts
      that reach that code, and its period.

  pads-at UNIT SPEC [SPEC ...]
      SPEC is 'LINE:N[,LINE:N...]': N pads in front of each LINE. Scores
      every annotated function of UNIT in place and prints the rises and
      falls against the unpadded unit. Pads before a function move only that
      function and later ones, so comparing specs localises the gap where
      the original's declaration count differs from the port's.

Pads keep the line count (they share a line with existing text), so the
COFF line records and the annotations still map. In-place compiles take a
lock and keep a backup under build-match/phase/<unit>/; a probe killed with a
candidate in place is undone on the next run. Never run a probe while a
build compiles the same tree. Pads are a diagnostic only: ADR 0005 entropy
rules 3 and 4 forbid committing them or any filler declaration.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import fcntl
import hashlib
import json
import os
import re
import shutil
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

ROOT = Path(__file__).resolve().parent.parent
PAD = "extern int phase_pad_{i};"


class ProbeError(Exception):
    pass


# ---------------------------------------------------------------------------
# Pad placement (pure text)


def _scan_state(lines: list[str], upto: int) -> tuple[bool, int, int]:
    """(inside a block comment, brace depth, #if depth) at the start of line
    index `upto` (0-based). Strings and character literals are skipped, and
    so are braces inside `#ifdef __cplusplus` blocks (the `extern "C" {`
    wrappers, which the C compile never sees)."""
    in_comment = False
    depth = 0
    stack: list[bool] = []  # one entry per open #if: is it `#ifdef __cplusplus`?
    for line in lines[:upto]:
        stripped = line.strip()
        if not in_comment and stripped.startswith("#"):
            word = re.match(r"#\s*(\w+)\s*(.*)", stripped)
            if word:
                if word.group(1) in ("if", "ifdef", "ifndef"):
                    stack.append(word.group(1) == "ifdef" and word.group(2).split()[:1] == ["__cplusplus"])
                elif word.group(1) == "endif" and stack:
                    stack.pop()
            continue
        skip_braces = any(stack)
        i = 0
        n = len(line)
        while i < n:
            c = line[i]
            if in_comment:
                if line.startswith("*/", i):
                    in_comment = False
                    i += 2
                    continue
                i += 1
                continue
            if line.startswith("//", i):
                break
            if line.startswith("/*", i):
                in_comment = True
                i += 2
                continue
            if c in "\"'":
                j = i + 1
                while j < n and line[j] != c:
                    j += 2 if line[j] == "\\" else 1
                i = j + 1
                continue
            if not skip_braces:
                if c == "{":
                    depth += 1
                elif c == "}":
                    depth -= 1
            i += 1
    return in_comment, depth, len(stack)


def check_pad_line(lines: list[str], index: int) -> None:
    """Raise ProbeError unless pads can go in front of line `index` (0-based):
    file scope, outside comments and conditional blocks, not a directive."""
    if not 0 <= index < len(lines):
        raise ProbeError(f"line {index + 1} is outside the file")
    in_comment, depth, pp = _scan_state(lines, index)
    if in_comment:
        raise ProbeError(f"line {index + 1} is inside a comment")
    if depth != 0:
        raise ProbeError(f"line {index + 1} is not at file scope")
    if pp != 0:
        raise ProbeError(f"line {index + 1} is inside an #if block")
    if lines[index].lstrip().startswith("#"):
        raise ProbeError(f"line {index + 1} is a preprocessor line")
    if lines[index].rstrip().endswith("\\"):
        raise ProbeError(f"line {index + 1} continues onto the next line")


def default_pad_line(lines: list[str]) -> int:
    """0-based index of the first usable blank line after the first #include."""
    first = next((i for i, l in enumerate(lines) if l.startswith("#include")), None)
    if first is None:
        raise ProbeError("no #include line; give --at LINE")
    for k in range(first + 1, len(lines)):
        if lines[k].strip():
            continue
        try:
            check_pad_line(lines, k)
        except ProbeError:
            continue
        return k
    raise ProbeError("no blank file-scope line after the first #include; give --at LINE")


def pad_text(lines: list[str], pads: dict[int, int]) -> str:
    """The unit with pads[k] declarations in front of line index k. The line
    count is unchanged. Pad names are numbered across the whole file."""
    out = []
    k = 0
    for i, line in enumerate(lines):
        n = pads.get(i, 0)
        if n:
            out.append(" ".join(PAD.format(i=k + j) for j in range(n)) + " " + line)
            k += n
        else:
            out.append(line)
    return "".join(out)


def parse_spec(spec: str) -> dict[int, int]:
    """'LINE:N,LINE:N' (1-based lines) -> {0-based index: N}."""
    pads: dict[int, int] = {}
    for item in filter(None, (s.strip() for s in spec.split(","))):
        m = re.fullmatch(r"(\d+):(\d+)", item)
        if not m:
            raise ProbeError(f"bad pad spec {item!r} (want LINE:N)")
        pads[int(m[1]) - 1] = pads.get(int(m[1]) - 1, 0) + int(m[2])
    return pads


# ---------------------------------------------------------------------------
# Listings and rows (pure text)


def listing_body(text: str, symbol: str) -> list[str] | None:
    """`symbol`'s instructions from a /FAs listing, with comments and the
    symbol counter's numbers ($S12, $L345, goto labels, block-scope locals)
    removed."""
    out: list[str] = []
    inside = False
    for line in text.splitlines():
        m = re.match(r"(\S+)\s+PROC\b", line)
        if m and m.group(1) == symbol:
            inside = True
            continue
        if inside and re.match(r"\S+\s+ENDP\b", line):
            return out
        if inside:
            line = re.sub(r"\$[A-Z]\d+", "$N", line.split(";", 1)[0]).strip()
            # Goto labels and block-scope locals carry the counter too:
            # $exit$8910, _buff$23614.
            line = re.sub(r"(\w)\$\d+\b", r"\1$N", line)
            if line:
                out.append(re.sub(r"\s+", " ", line))
    return None


def fingerprint(body: list[str] | None) -> str:
    if body is None:
        return "ERR"
    return hashlib.sha1("\n".join(body).encode()).hexdigest()[:10]


def letters_for(fps: list[str]) -> str:
    """One letter per distinct code in order of appearance; '!' = no code."""
    seen: dict[str, str] = {}
    row = ""
    for fp in fps:
        if fp == "ERR":
            row += "!"
            continue
        if fp not in seen:
            seen[fp] = chr(ord("A") + len(seen)) if len(seen) < 26 else chr(ord("a") + len(seen) - 26)
        row += seen[fp]
    return row


def row_period(row: str) -> int | None:
    """Smallest p such that row[i] == row[i + p] wherever both are codes and
    the row holds at least two whole periods; None if there is none."""
    n = len(row)
    for p in range(1, n // 2 + 1):
        if all(row[i] == row[i + p] or "!" in (row[i], row[i + p]) for i in range(n - p)):
            return p
    return None


@dataclass
class Code:
    letter: str
    first: int
    shifts: list[int]
    matching: float | None = None
    effective: bool = False

    @property
    def percent(self) -> float | None:
        if self.matching is None:
            return None
        return 100.0 if self.effective else round(self.matching * 100, 2)


@dataclass
class ProbeRow:
    address: int
    name: str
    row: str
    ns: list[int]
    codes: list[Code] = field(default_factory=list)

    def summary(self) -> dict:
        base = next((c for c in self.codes if 0 in c.shifts), self.codes[0] if self.codes else None)
        scored = [c for c in self.codes if c.percent is not None]
        best = max(scored, key=lambda c: (c.percent, c.matching or 0), default=None)
        period = row_period(self.row)
        if len(self.codes) <= 1:
            kind = "single-code"
        elif best is not None and base is not None and base.percent is not None and best.percent > base.percent + 1e-9:
            kind = "phase-sensitive"
        else:
            kind = "current-best"
        return {
            "address": f"0x{self.address:08x}",
            "name": self.name,
            "kind": kind,
            "codes": len(self.codes),
            "now": base.percent if base else None,
            "best": best.percent if best else None,
            "best_effective": bool(best.effective) if best else False,
            "shifts": (sorted(n for c in scored if c.percent == best.percent for n in c.shifts)[:12]
                       if kind == "phase-sensitive" else []),
            "period": period,
            "row": self.row,
            "code_scores": {c.letter: [c.first, c.percent] for c in self.codes},
        }


def build_codes(row: str, ns: list[int], max_codes: int) -> list[Code]:
    codes: dict[str, Code] = {}
    for i, ch in enumerate(row):
        if ch == "!":
            continue
        if ch not in codes:
            if len(codes) >= max_codes:
                continue
            codes[ch] = Code(ch, ns[i], [])
        codes[ch].shifts.append(ns[i])
    return list(codes.values())


# ---------------------------------------------------------------------------
# In-place guard (lock + backup)


class InPlace:
    """Write candidates over the real source and always put it back.

    An exclusive flock serialises probes on one unit; the lock file's
    existence plus the backup record a candidate in place, so a killed probe
    is undone by the next one."""

    def __init__(self, source: Path, state_dir: Path):
        self.source = source
        state_dir.mkdir(parents=True, exist_ok=True)
        self.flock = open(state_dir / "probe.flock", "a+")
        fcntl.flock(self.flock, fcntl.LOCK_EX)
        self.lock = state_dir / "in-place.lock"
        self.backup = state_dir / ("original" + source.suffix)
        self.recovered = False
        if self.lock.exists() and self.backup.exists():
            source.write_text(self.backup.read_text())
            self.lock.unlink()
            self.recovered = True
        self.original = source.read_text()
        self.backup.write_text(self.original)

    def put(self, text: str) -> None:
        if text.count("\n") != self.original.count("\n"):
            raise ProbeError("candidate changes the line count")
        if text != self.original:
            self.lock.write_text(str(self.source) + "\n")
            self.source.write_text(text)

    def restore(self) -> None:
        if self.source.read_text() != self.original:
            self.source.write_text(self.original)
        self.lock.unlink(missing_ok=True)

    def close(self) -> None:
        try:
            self.restore()
        finally:
            fcntl.flock(self.flock, fcntl.LOCK_UN)
            self.flock.close()


# ---------------------------------------------------------------------------
# Compiling and scoring (needs the MSVC 5.0 toolchain and reference/)


class Unit:
    """One translation unit's compile command and Reccmp context."""

    def __init__(self, source: Path, build_dir: str | None = None):
        import logging

        import match_function as mf
        from reccmp.dir import source_code_search
        from reccmp.formats import TextFile, detect_image

        logging.getLogger("reccmp").setLevel(logging.CRITICAL)
        logging.getLogger().setLevel(logging.CRITICAL)
        self.mf = mf
        args = argparse.Namespace(build_dir=build_dir, orig=None)
        self.env = mf.Environment.detect(args)
        commands = mf.load_compile_commands(self.env, {})
        self.source = source.resolve()
        self.rel = os.path.relpath(self.source, self.env.root)
        self.command = {c.source: c for c in commands}.get(self.source)
        if self.command is None:
            raise ProbeError(f"{self.rel} is not compiled by the matching build")
        orig_path = mf.find_orig(args)
        if not orig_path.exists():
            raise ProbeError(f"original executable not found: {orig_path} (symlink reference/ into the worktree)")
        orig_bin = detect_image(filepath=orig_path)
        code_files = list(TextFile.from_files(source_code_search([self.env.root / "src"]), allow_error=True, encoding="utf-8"))
        self.markers = mf.scan_markers(code_files)
        libc_symbols, libc_data, libc_index = mf.load_libc_index(self.env)
        self.ctx = mf.ProjectContext.build(orig_bin, code_files, self.markers, libc_symbols)
        self.ctx.libc_data, self.ctx.libc_index = libc_data, libc_index
        self.ctx.link_end = mf.link_end(orig_bin, self.env.root)
        self.state = self.env.build_dir / "phase" / self.source.stem
        runs = self.env.build_dir / "runs"
        runs.mkdir(parents=True, exist_ok=True)
        self.workdir = Path(tempfile.mkdtemp(prefix="phase-", dir=runs))

    def addresses(self) -> list[int]:
        return sorted(a for a, m in self.markers.items()
                      if m.path.resolve() == self.source and m.kind in ("FUNCTION", "STUB"))

    def score(self, guard: InPlace, text: str, addrs: list[int]) -> dict[int, object]:
        """Compile `text` at the real path and score `addrs` (FunctionResults)."""
        from synthetic_image import SyntheticImage

        try:
            guard.put(text)
            obj_path, _ = self.mf.compile_source(self.env, self.command, self.workdir, 0)
        finally:
            guard.restore()
        try:
            obj = self.mf.msvc_coff.read_coff(obj_path)
            img = SyntheticImage(obj, self.ctx, self.source)
            analysis = self.mf.analyse(img, self.ctx)
            return {a: self.mf.compare_function(analysis, self.ctx, a) for a in addrs}
        finally:
            obj_path.unlink(missing_ok=True)

    def listings(self, texts: dict[int, str], jobs: int) -> dict[int, str]:
        """Compile each text from a scratch copy with /FAs; N -> listing text."""
        sweep = self.workdir / "sweep"
        sweep.mkdir(exist_ok=True)

        def one(n: int) -> tuple[int, str]:
            cand = sweep / f"n{n:05d}{self.source.suffix}"
            cand.write_text(texts[n])
            asm = sweep / f"n{n:05d}.asm"
            try:
                obj, _ = self.mf.compile_source(self.env, self.command, sweep, n, cand, ["/FAs", "/Fa" + str(asm)])
                obj.unlink(missing_ok=True)
                text = asm.read_text(errors="replace") if asm.exists() else ""
            except self.mf.MatchError:
                text = ""
            for p in (cand, asm):
                p.unlink(missing_ok=True)
            return n, text

        with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, jobs)) as pool:
            return dict(pool.map(one, list(texts)))

    def close(self) -> None:
        shutil.rmtree(self.workdir, ignore_errors=True)


def unit_for(addrs: list[int]) -> Path:
    import match_function as mf
    from reccmp.dir import source_code_search
    from reccmp.formats import TextFile

    code_files = list(TextFile.from_files(source_code_search([ROOT / "src"]), allow_error=True, encoding="utf-8"))
    markers = mf.scan_markers(code_files)
    paths = set()
    for a in addrs:
        if a not in markers:
            raise ProbeError(f"no annotation names {a:#x}")
        paths.add(markers[a].path.resolve())
    if len(paths) != 1:
        raise ProbeError("all addresses must be in one translation unit")
    return paths.pop()


def probe(args) -> int:
    addrs = [int(a, 16) for a in args.addresses]
    unit = Unit(Path(args.source) if args.source else unit_for(addrs), args.build_dir)
    guard = InPlace(unit.source, unit.state)
    try:
        if guard.recovered:
            print(f"restored {unit.rel} from an earlier killed probe", file=sys.stderr)
        lines = guard.original.splitlines(keepends=True)
        at = args.at - 1 if args.at else default_pad_line(lines)
        check_pad_line(lines, at)
        ns = list(range(*args.range))
        base = unit.score(guard, guard.original, addrs)
        symbols = {}
        skipped = []
        for a in addrs:
            res = base[a]
            if not res.error and not res.symbol and res.name and unit.source.suffix == ".c":
                res.symbol = "_" + res.name  # a static C function: Reccmp names it without a symbol
            if res.error or not res.symbol:
                print(f"0x{a:08x}: skipped: {res.error or 'no symbol'}", file=sys.stderr)
                skipped.append({"address": f"0x{a:08x}", "error": res.error or "no symbol"})
                continue
            symbols[a] = res.symbol
        addrs = [a for a in addrs if a in symbols]
        texts = {n: pad_text(lines, {at: n}) for n in ns}
        listings = unit.listings(texts, args.jobs)
        rows = []
        for a in addrs:
            row = letters_for([fingerprint(listing_body(listings[n], symbols[a])) for n in ns])
            pr = ProbeRow(a, base[a].name or symbols[a], row, ns, build_codes(row, ns, args.max_codes))
            for code in pr.codes:
                res = base[a] if code.first == 0 else unit.score(guard, texts[code.first], [a])[a]
                code.matching, code.effective = res.matching, bool(res.effective)
            rows.append(pr.summary())
            s = rows[-1]
            if not args.json:
                shifts = f" shifts {s['shifts']}" if s["shifts"] else ""
                print(f"{s['address']} {s['name']}: {s['kind']}, {s['codes']} codes, now {s['now']}, "
                      f"best {s['best']}{'*' if s['best_effective'] else ''}{shifts}, period {s['period']}", flush=True)
                for i in range(0, len(row), 64):
                    print(f"    {ns[i]:5d} {row[i:i + 64]}")
                print(f"    codes: {s['code_scores']}", flush=True)
        out = {"unit": unit.rel, "pad_line": at + 1, "range": args.range, "functions": rows, "skipped": skipped}
        if args.output:
            Path(args.output).write_text(json.dumps(out, indent=1) + "\n")
        if args.json:
            print(json.dumps(out, indent=1))
    finally:
        guard.close()
        unit.close()
    return 0


def pads_at(args) -> int:
    unit = Unit(Path(args.unit), args.build_dir)
    guard = InPlace(unit.source, unit.state)
    try:
        lines = guard.original.splitlines(keepends=True)
        addrs = unit.addresses()
        specs = [parse_spec(s) for s in args.specs]
        for pads in specs:
            for index in pads:
                check_pad_line(lines, index)

        def pct(res):
            if res.matching is None:
                return None
            return 100.0 if res.effective else round(res.matching * 100, 2)

        base = {a: pct(r) for a, r in unit.score(guard, guard.original, addrs).items()}
        report = []
        for spec, pads in zip(args.specs, specs):
            got = {a: pct(r) for a, r in unit.score(guard, pad_text(lines, pads), addrs).items()}
            up = {f"0x{a:08x}": (base[a], v) for a, v in got.items() if v is not None and base[a] is not None and v > base[a] + 1e-9}
            dn = {f"0x{a:08x}": (base[a], v) for a, v in got.items() if v is not None and base[a] is not None and v < base[a] - 1e-9}
            report.append({"spec": spec, "up": up, "down": dn})
            if not args.json:
                print(f"{spec}: UP {up} DOWN {dn}", flush=True)
        if args.json:
            print(json.dumps({"unit": unit.rel, "base": {f"0x{a:08x}": v for a, v in base.items()}, "specs": report}, indent=1))
    finally:
        guard.close()
        unit.close()
    return 0


def build_parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--build-dir", help="match_function.py's flags cache (default build-match/)")
    sub = ap.add_subparsers(dest="command", required=True)
    p = sub.add_parser("probe", help="sweep the counter in front of one line; classify functions")
    p.add_argument("addresses", nargs="+", help="original addresses, all in one unit")
    p.add_argument("--source", help="the unit (default: found by annotation)")
    p.add_argument("--range", nargs=3, type=int, default=[0, 256, 1], metavar=("START", "STOP", "STEP"))
    p.add_argument("--at", type=int, help="1-based line the pads go in front of (default: first blank line after the first #include)")
    p.add_argument("--jobs", type=int, default=4)
    p.add_argument("--max-codes", type=int, default=24)
    p.add_argument("--output", help="write the JSON result here")
    p.add_argument("--json", action="store_true")
    p.set_defaults(func=probe)
    q = sub.add_parser("pads-at", help="score a whole unit with pads at chosen lines")
    q.add_argument("unit")
    q.add_argument("specs", nargs="+", help="'LINE:N[,LINE:N...]'")
    q.add_argument("--json", action="store_true")
    q.set_defaults(func=pads_at)
    return ap


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        return args.func(args)
    except ProbeError as ex:
        print(f"phase_probe: {ex}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
