#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-or-later
"""Seconds-fast single-function Reccmp loop.

Given an original LegoRR.exe function address, compile only the translation
unit whose `// FUNCTION: LEGORR 0x...` annotation names it (same MSVC 5.0
command line as the matching build), lay the unlinked object out as a
synthetic image, and score it with Reccmp's own entity database, matchers
and FunctionComparator. No link, no cvdump, no full Reccmp run.

See tools/README.md ("match_function.py") for usage and the parity notes.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import fcntl
import json
import logging
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass, field
from pathlib import Path, PureWindowsPath
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))

import msvc_coff  # noqa: E402
from synthetic_image import (  # noqa: E402
    EXTERN_DATA_SLOT,
    SyntheticImage,
    collect_nodes,
)

PROJECT_ROOT = Path(__file__).resolve().parent.parent
TARGET_ID = "LEGORR"
DEFAULT_TOOLCHAIN = Path.home() / ".local/share/reprobit/toolchains/msvc_5_0_rtm"
DEFAULT_CONTAINER = "developer"
WINESERVER_PERSIST_SECONDS = 900


MARKER_RE = re.compile(
    r"//\s*(FUNCTION|LIBRARY|STUB|TEMPLATE|SYNTHETIC)\s*:\s*"
    + TARGET_ID
    + r"\s+(0x[0-9a-fA-F]+)(?:\s+(\w+))?"
)

log = logging.getLogger("match_function")


class MatchError(Exception):
    """A user-facing failure (bad address, compile error, missing tool)."""


# ---------------------------------------------------------------------------
# Environment: Wine (directly or through distrobox), toolchain, CMake flags


@dataclass
class Environment:
    root: Path
    toolchain: Path
    build_dir: Path
    container: str | None
    persist: int

    @classmethod
    def detect(cls, args: argparse.Namespace) -> "Environment":
        root = PROJECT_ROOT
        toolchain = Path(
            os.environ.get("REPROBIT_MSVC_5_0_RTM_ROOT") or DEFAULT_TOOLCHAIN
        ).expanduser()
        if not (toolchain / "wine/x86/cl").exists():
            raise MatchError(
                f"MSVC 5.0 wrapper not found at {toolchain}/wine/x86/cl; "
                "set REPROBIT_MSVC_5_0_RTM_ROOT"
            )
        build_dir = Path(
            args.build_dir or os.environ.get("MATCH_FN_BUILD_DIR") or root / "build-match"
        )
        if not build_dir.is_absolute():
            build_dir = root / build_dir

        container: str | None = None
        if shutil.which("wine") is None:
            container = os.environ.get("MATCH_FN_DISTROBOX", DEFAULT_CONTAINER) or None
            if container is None or shutil.which("distrobox") is None:
                raise MatchError(
                    "wine is not on PATH and no distrobox container is configured "
                    "(set MATCH_FN_DISTROBOX, or run inside the container)"
                )
        persist = int(os.environ.get("MATCH_FN_WINESERVER_PERSIST", WINESERVER_PERSIST_SECONDS))
        return cls(root, toolchain, build_dir, container, persist)

    def wrap(self, argv: list[str]) -> list[str]:
        """Prefix a command so it runs where Wine lives."""
        if self.container:
            return ["distrobox", "enter", self.container, "--", *argv]
        return argv

    def warm_script(self) -> str:
        """Shell snippet that keeps one wineserver (and its services) alive.

        The MSVC wrapper pipes Wine's stdout through sed. If Wine has to start
        its services during a compile, they inherit that pipe and, under a
        persistent wineserver, hold it open. Starting the services here with
        all stdio on /dev/null avoids that, and keeps later compiles warm."""
        if self.persist <= 0:
            return ":"
        # `wineserver -p` exits 0 only when it started a new server for this
        # WINEPREFIX (2 when one is already running): only then boot services.
        return (
            f"if wineserver -p{self.persist} </dev/null >/dev/null 2>&1; then "
            "wine cmd /c exit </dev/null >/dev/null 2>&1; fi"
        )

    def run(self, argv: list[str], *, cwd: Path, log_path: Path, timeout: int) -> int:
        script = self.warm_script() + '; cd "$1" && shift && exec "$@"'
        cmd = self.wrap(["sh", "-c", script, "sh", str(cwd), *argv])
        env = dict(os.environ)
        env["WINE_MSVC_RAW_STDOUT"] = "1"
        env.setdefault("WINEDEBUG", "-all")
        with open(log_path, "wb") as out:
            try:
                proc = subprocess.run(
                    cmd,
                    stdin=subprocess.DEVNULL,
                    stdout=out,
                    stderr=subprocess.STDOUT,
                    env=env,
                    timeout=timeout,
                    check=False,
                )
            except subprocess.TimeoutExpired as ex:
                raise MatchError(
                    f"timed out after {timeout}s: {shlex.join(argv)} (log: {log_path})"
                ) from ex
        return proc.returncode


def _locked(path: Path, exclusive: bool):
    path.parent.mkdir(parents=True, exist_ok=True)
    handle = open(path, "a+")
    fcntl.flock(handle, fcntl.LOCK_EX if exclusive else fcntl.LOCK_SH)
    return handle


@dataclass
class CompileCommand:
    argv: list[str]
    directory: Path
    source: Path
    output: str

    @property
    def target(self) -> str:
        m = re.match(r"CMakeFiles/([^/]+)\.dir/", self.output)
        return m.group(1) if m else ""


def cmake_args(env: Environment, cmake_dir: Path) -> list[str]:
    """The configure line of tools/reccmp.sh, plus compile_commands.json.

    The -D options are read from reccmp.sh itself so the two never drift."""
    script = (env.root / "tools/reccmp.sh").read_text()
    m = re.search(r'^cmake -S "\$\{project_root\}".*?(?<!\\)\n', script, re.M | re.S)
    options = re.findall(r"(-D[A-Za-z0-9_]+=\S+)", m.group(0)) if m else []
    if not options:
        raise MatchError("could not read the CMake configure options from tools/reccmp.sh")
    values = {"project_root": str(env.root), "toolchain_root": str(env.toolchain)}
    resolved = [
        re.sub(r"\$\{(\w+)\}", lambda v: values.get(v.group(1), v.group(0)), opt.replace('"', "").replace("'", ""))
        for opt in options
    ]
    return ["cmake", "-S", str(env.root), "-B", str(cmake_dir), *resolved, "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"]


def load_compile_commands(env: Environment, timings: dict[str, float]) -> list[CompileCommand]:
    cmake_dir = env.build_dir / "cmake"
    db_path = cmake_dir / "compile_commands.json"
    stamp = cmake_dir / "match-function-configure.json"
    args = cmake_args(env, cmake_dir)
    inputs = [env.root / "CMakeLists.txt", env.root / "cross/msvc5-toolchain.cmake", env.root / "tools/reccmp.sh"]

    def stale() -> bool:
        if not db_path.exists() or not stamp.exists():
            return True
        if json.loads(stamp.read_text()) != args:
            return True
        built = db_path.stat().st_mtime
        return any(p.exists() and p.stat().st_mtime > built for p in inputs)

    lock = env.build_dir / "configure.lock"
    if stale():
        with _locked(lock, exclusive=True):
            if stale():
                start = time.monotonic()
                cmake_dir.mkdir(parents=True, exist_ok=True)
                log_path = env.build_dir / "configure.log"
                rc = env.run(args, cwd=env.root, log_path=log_path, timeout=600)
                if rc != 0 or not db_path.exists():
                    raise MatchError(f"CMake configure failed (see {log_path})")
                stamp.write_text(json.dumps(args))
                timings["configure"] = time.monotonic() - start

    with _locked(lock, exclusive=False):
        entries = json.loads(db_path.read_text())
    commands = []
    for entry in entries:
        commands.append(
            CompileCommand(
                argv=shlex.split(entry["command"]),
                directory=Path(entry["directory"]),
                source=Path(entry["file"]).resolve(),
                output=entry.get("output", ""),
            )
        )
    return commands


def compiled_sources(build_dir: str | None = None) -> list[Path]:
    """Every source file the matching build compiles (configuring if needed)."""
    env = Environment.detect(argparse.Namespace(build_dir=build_dir))
    return sorted({c.source for c in load_compile_commands(env, {})})


def compile_source(
    env: Environment, command: CompileCommand, workdir: Path, index: int = 0
) -> tuple[Path, str]:
    """Compile one source file into workdir with the configured flags."""
    obj = workdir / f"{index:02d}-{command.source.stem}.obj"
    argv: list[str] = []
    for arg in command.argv:
        if arg.startswith("/Fo") or arg.startswith("-Fo"):
            arg = "/Fo" + str(obj)
        elif arg.startswith("/Fd") or arg.startswith("-Fd"):
            arg = "/Fd" + str(obj) + ".pdb"
        argv.append(arg)
    log_path = obj.with_suffix(".log")
    rc = env.run(argv, cwd=workdir, log_path=log_path, timeout=600)
    output = log_path.read_bytes().decode("latin1").replace("\r", "")
    output = re.sub(r"(?i)\bz:([\\/])", r"\1", output)
    if rc != 0 or not obj.exists():
        raise MatchError(f"compile failed for {command.source} (exit {rc}):\n{output}")
    return obj, output


# ---------------------------------------------------------------------------
# Annotation lookup


@dataclass
class Marker:
    address: int
    kind: str
    path: Path
    line: int
    extra: str | None


def scan_markers(code_files) -> dict[int, Marker]:
    markers: dict[int, Marker] = {}
    for tf in code_files:
        for lineno, text in enumerate(tf.text.splitlines(), start=1):
            if "LEGORR" not in text:
                continue
            m = MARKER_RE.search(text)
            if m is None:
                continue
            addr = int(m.group(2), 16)
            markers.setdefault(addr, Marker(addr, m.group(1), Path(tf.path), lineno, m.group(3)))
    return markers


_SIG_NAME_RE = re.compile(r"(~?[A-Za-z_]\w*(?:\s*::\s*~?[A-Za-z_]\w*)*)\s*\(")


def signature_name(sig: str) -> str | None:
    """Function name from a reccmp `function_sig` declaration line."""
    sig = re.sub(r"//.*$|/\*.*?\*/", "", sig)
    for m in _SIG_NAME_RE.finditer(sig):
        name = re.sub(r"\s+", "", m.group(1))
        if name.split("::")[-1] in {"__declspec", "if", "while", "sizeof", "defined"}:
            continue
        return name
    return None


# ---------------------------------------------------------------------------
# Project-wide context shared by every object analysed in one run


@dataclass
class ProjectContext:
    orig_bin: Any
    code_files: list
    markers: dict[int, Marker]
    program_sources: set[Path]
    libc_symbols: set[str]
    # Line-referenced FUNCTION/STUB annotations by unqualified name.
    libc_data: bytes | None = None
    libc_index: dict[str, int] = field(default_factory=dict)
    _libc_absolute: dict[str, int | None] = field(default_factory=dict)
    lineref_names: dict[str, list[tuple[int, Path]]] = field(default_factory=dict)
    lineref_by_addr: dict[int, str] = field(default_factory=dict)
    link_end: int = 0
    nameref_names: set[str] = field(default_factory=set)
    orig_imports: dict[str, Any] = field(default_factory=dict)

    @classmethod
    def build(cls, orig_bin, code_files, markers, program_sources, libc_symbols):
        from reccmp.compare import ingest
        from reccmp.parser import DecompCodebase
        from reccmp.parser.node import ParserFunction

        ctx = cls(orig_bin, code_files, markers, program_sources, libc_symbols)
        codebase = DecompCodebase(code_files, TARGET_ID)

        # load_markers() parses the whole tree again for every object; hand it
        # this parse instead (its pruning steps are idempotent).
        def cached_codebase(files, target_id, aliases=None):
            if files is code_files and target_id == TARGET_ID and not aliases:
                return codebase
            return DecompCodebase(files, target_id, aliases=aliases)

        ingest.DecompCodebase = cached_codebase
        texts = {Path(tf.path): tf.text.splitlines() for tf in code_files}
        for sym in codebase.iter_line_functions():
            name = signature_name(sym.name or "")
            lines = texts.get(Path(sym.filename), [])
            excluded = False
            if not name:
                # Reccmp records the first line after the marker as the
                # signature; skip preprocessor lines to find the real one.
                for text in lines[sym.line_number - 1 : sym.line_number + 12]:
                    stripped = text.strip()
                    if stripped.startswith("#") and re.search(r"!\s*defined\s*\(?\s*_MSC_VER", stripped):
                        excluded = True
                    if not stripped or stripped.startswith(("#", "//", "/*", "*")):
                        continue
                    name = signature_name(stripped)
                    if name:
                        break
            if name:
                key = name.split("::")[-1]
                ctx.lineref_names.setdefault(key, []).append((sym.offset, Path(sym.filename).resolve()))
                if not excluded:
                    ctx.lineref_by_addr.setdefault(sym.offset, name)
        for sym in codebase.iter_name_functions():
            if isinstance(sym, ParserFunction) and sym.name:
                ctx.nameref_names.add(sym.name)
        for var in codebase.iter_variables():
            if var.name:
                ctx.nameref_names.add(var.name)
        for imp in orig_bin.imports:
            if imp.name:
                ctx.orig_imports.setdefault(imp.name.upper(), imp)
        return ctx

    def libc_absolute(self, name: str) -> int | None:
        """Value of a LIBC.LIB absolute symbol (e.g. __except_list = 0)."""
        if name in self._libc_absolute:
            return self._libc_absolute[name]
        value = None
        if self.libc_data is not None and name in self.libc_index:
            member = msvc_coff.archive_member_at(self.libc_data, self.libc_index[name])
            try:
                obj = msvc_coff.parse_coff(member.data, member.name)
                for sym in obj.symbols.values():
                    if sym.name == name and sym.section == -1:
                        value = sym.value
                        break
            except msvc_coff.CoffError:
                pass
        self._libc_absolute[name] = value
        return value

    def resolve_lineref(self, friendly: str, exclude: Path | None) -> int | None:
        """Original address of the line-referenced annotation whose function
        name matches, when that is unambiguous."""
        candidates = [
            (addr, path)
            for addr, path in self.lineref_names.get(friendly.split("::")[-1], [])
            if path != exclude
        ]
        addrs = {addr for addr, _ in candidates}
        if len(addrs) == 1:
            return candidates[0][0]
        return None



# ---------------------------------------------------------------------------
# The Reccmp pipeline over (original image, synthetic image)


@dataclass
class Analysis:
    db: Any
    lines_db: Any
    image: SyntheticImage
    events: list[dict[str, Any]]


def analyse(img: SyntheticImage, ctx: ProjectContext) -> Analysis:
    # pylint: disable=too-many-locals
    from reccmp.compare import analyze as an
    from reccmp.compare import match_msvc as mm
    from reccmp.compare import mutate as mu
    from reccmp.compare.db import EntityDb
    from reccmp.compare.ingest import load_markers
    from reccmp.compare.lines import LinesDb
    from reccmp.types import EntityType, ImageId

    events: list[dict[str, Any]] = []

    def report(event, orig_addr, msg=""):
        events.append({"event": getattr(event, "name", str(event)), "address": orig_addr, "message": msg})

    db = EntityDb()
    lines_db = LinesDb()
    orig = ctx.orig_bin

    nodes, function_starts = collect_nodes(img)
    with db.batch() as batch:
        for addr in sorted(nodes):
            n = nodes[addr]
            attrs: dict[str, Any] = {}
            if n.type is not None:
                attrs["type"] = n.type
            if n.name is not None and n.type not in (EntityType.STRING, EntityType.WIDECHAR, EntityType.FLOAT):
                attrs["name"] = n.name
            if n.symbol is not None:
                attrs["symbol"] = n.symbol
            batch.set(ImageId.RECOMP, addr, size=n.size, **attrs)

    # load_cvdump_lines: the object's COFF line table stands in for the PDB.
    source_name = img.obj.source_name or str(img.source)
    foreign = PureWindowsPath(source_name.replace("/", "\\"))
    for index, pairs in img.obj.function_lines().items():
        base = img.section_addr.get(index)
        if base is None:
            continue
        lines_db.add_lines(foreign, [(line, base + offset) for line, offset in pairs])
    lines_db.mark_function_starts(tuple(function_starts))

    load_markers(ctx.code_files, lines_db, orig, TARGET_ID, db, "latin1", {}, report)

    # Calls into other translation units: the full link matches those
    # functions through their own line tables; mirror that by name.
    with db.batch() as batch:
        for n in nodes.values():
            ent = db.get(ImageId.ORIG, n.orig_match) if n.orig_match is not None else None
            if ent is not None and ent.recomp_addr is None:
                batch.match(n.orig_match, n.addr)

    mm.match_symbols(db, report, truncate=True)
    mm.match_functions(db, report, truncate=True)
    mm.match_vtables(db, report)
    mm.match_static_variables(db, report)
    mm.match_variables(db, report)
    mm.match_lines(db, lines_db, report)

    for img_id, binfile in ((ImageId.ORIG, orig), (ImageId.RECOMP, img)):
        an.create_imports(db, img_id, binfile)
        if img_id == ImageId.ORIG:
            an.create_import_thunks(db, img_id, binfile)
        else:
            _create_recomp_import_thunks(db, img)
        an.create_seh_entities(db, img_id, binfile)
        an.create_thunks(db, img_id, binfile)
        if img_id == ImageId.ORIG:
            an.create_analysis_vtordisps(db, img_id, binfile)
            an.create_crt_functions(db, img_id, binfile)
        an.import_sections(db, img_id, binfile)

    mm.match_imports(db)

    # External data: the linker PDB knows each variable's type size. Use the
    # room the original leaves before its next entity as the best estimate.
    with db.batch() as batch:
        for n in nodes.values():
            if n.type == EntityType.DATA and n.size is None:
                ent = db.get(ImageId.RECOMP, n.addr)
                if ent is not None and ent.orig_addr is not None:
                    size = db.get_max_size(ImageId.ORIG, ent.orig_addr)
                    batch.set(ImageId.RECOMP, n.addr, size=min(size or 4, EXTERN_DATA_SLOT))
                elif n.addr >= img.extern_data_base:
                    # Unknown type size: let any offset render as name+N.
                    batch.set(ImageId.RECOMP, n.addr, size=EXTERN_DATA_SLOT)

    for img_id in (ImageId.ORIG, ImageId.RECOMP):
        mu.set_max_size(db, img_id)

    _check_vtables(db)
    mm.match_ref(db, report)
    mu.unique_names_for_overloaded_functions(db)
    mu.name_thunks(db)

    for img_id, binfile in ((ImageId.ORIG, orig), (ImageId.RECOMP, img)):
        an.create_analysis_floats(db, img_id, binfile)
        an.create_analysis_strings(db, img_id, binfile, "latin1")
        an.complete_partial_floats(db, img_id, binfile)
        an.complete_partial_strings(db, img_id, binfile, "latin1")

    mm.match_strings(db, report)

    # The full link matches every line-referenced FUNCTION annotation in the
    # other translation units, so those original entities carry a name there.
    # Give them one here too; only equality of the rendered text matters.
    with db.batch() as batch:
        for orig_addr, name in ctx.lineref_by_addr.items():
            ent = db.get(ImageId.ORIG, orig_addr)
            if ent is not None and ent.recomp_addr is None and ent.best_name() is None:
                batch.set(ImageId.ORIG, orig_addr, name=name)

    return Analysis(db, lines_db, img, events)


def _check_vtables(db) -> None:
    try:
        from reccmp.compare.verify import check_vtables
    except ImportError:
        return
    check_vtables(db)


def _create_recomp_import_thunks(db, img: SyntheticImage) -> None:
    from reccmp.analysis import find_import_thunks
    from reccmp.types import EntityType, ImageId

    with db.batch() as batch:
        for thunk in find_import_thunks(img):
            batch.set(ImageId.RECOMP, thunk.addr, type=EntityType.IMPORT_THUNK, skip=True, size=thunk.size)
            batch.set_ref(ImageId.RECOMP, thunk.addr, ref=thunk.import_addr)


# ---------------------------------------------------------------------------
# Comparison


@dataclass
class FunctionResult:
    address: int
    name: str | None = None
    source: str | None = None
    line: int | None = None
    origin: str | None = None
    symbol: str | None = None
    matching: float | None = None
    effective: bool = False
    stub: bool = False
    library: bool = False
    orig_size: int | None = None
    recomp_size: int | None = None
    diff: list | None = None
    baseline: str | None = None
    error: str | None = None
    warnings: list[str] = field(default_factory=list)

    @property
    def score_text(self) -> str | None:
        """Two-decimal percent, rounded like the match baseline file."""
        if self.matching is None:
            return None
        from match_baseline import format_score

        return format_score({"matching": self.matching})

    @property
    def below_baseline(self) -> bool | None:
        if self.baseline is None or self.score_text is None:
            return None
        return float(self.score_text) < float(self.baseline)

    def to_json(self) -> dict[str, Any]:
        out: dict[str, Any] = {
            "address": f"0x{self.address:x}",
            "name": self.name,
            "source": self.source,
            "line": self.line,
            "origin": self.origin,
            "symbol": self.symbol,
            "matching": self.matching,
            "effective": self.effective,
            "percent": None if self.matching is None else _percent(self),
            "stub": self.stub,
            "library": self.library,
            "orig_size": self.orig_size,
            "recomp_size": self.recomp_size,
            "baseline": self.baseline,
            "below_baseline": self.below_baseline,
            "diff": self.diff,
        }
        if self.error:
            out["error"] = self.error
        if self.warnings:
            out["warnings"] = self.warnings
        return out


def _percent(res: FunctionResult) -> str:
    ratio = 1.0 if res.effective else (res.matching or 0.0)
    return f"{ratio * 100:.2f}%" + ("*" if res.effective else "")


def compare_function(analysis: Analysis, ctx: ProjectContext, address: int) -> FunctionResult:
    import reccmp.compare.functions as rf
    from reccmp.compare.diff import raw_diff_to_udiff
    from reccmp.cvdump import CvdumpTypesParser
    from reccmp.types import ImageId

    res = FunctionResult(address)
    db = analysis.db
    match = db.get_one_match(address)
    if match is None:
        ent = db.get(ImageId.ORIG, address)
        if ent is None:
            res.error = "no annotation for this address was loaded"
        else:
            res.error = (
                "the annotated function was not found in the compiled object "
                "(inlined, renamed, or excluded by preprocessor conditions)"
            )
        return res

    res.name = match.best_name()
    res.stub = bool(match.get("stub", False))
    res.library = bool(match.get("library", False))
    res.symbol = match.get("symbol")
    if match.any_size() == 0:
        res.error = "the recomp function has no size"
        return res

    # Reccmp applies assert fix-ups to images with a debug directory: the
    # matching build links with /DEBUG; the shipped executable has none.
    original_has_asserts = rf.has_asserts
    rf.has_asserts = lambda image: image is analysis.image or original_has_asserts(image)
    try:
        comparator = rf.FunctionComparator(
            db, analysis.lines_db, ctx.orig_bin, analysis.image, _null_report, CvdumpTypesParser()
        )
        result = comparator.compare_function(match)
    finally:
        rf.has_asserts = original_has_asserts

    res.matching = result.match_ratio
    res.effective = bool(result.is_effective_match)
    # The byte counts FunctionComparator read (see its compare_function).
    res.recomp_size = match.size(ImageId.RECOMP)
    res.orig_size = match.size(ImageId.ORIG)
    if res.orig_size is None and res.recomp_size is not None:
        orig_max = match.max_size(ImageId.ORIG)
        res.orig_size = min(orig_max, res.recomp_size) if orig_max is not None else res.recomp_size

    # Show recomp addresses as offsets from the function start: the
    # synthetic addresses mean nothing outside this run.
    start = match.recomp_addr
    rdiff = result.diff
    rdiff.recomp_inst = [
        (_recomp_label(a, start), text) for a, text in rdiff.recomp_inst
    ]
    res.diff = _clean_names([list(group) for group in raw_diff_to_udiff(rdiff)])
    res.name = _clean_names(res.name)
    for ev in analysis.events:
        if ev.get("address") == address:
            res.warnings.append(f"{ev['event']}: {ev['message']}")
    return res


def _clean_names(value):
    """Apply tools/clean_reccmp_report_names.py's label clean-up so names
    read the same as in the published Reccmp report."""
    from clean_reccmp_report_names import PREFIX

    if isinstance(value, str):
        return value.replace(PREFIX, "")
    if isinstance(value, list):
        return [_clean_names(v) for v in value]
    if isinstance(value, tuple):
        return tuple(_clean_names(v) for v in value)
    if isinstance(value, dict):
        return {k: _clean_names(v) for k, v in value.items()}
    return value


def _recomp_label(addr: str, start: int) -> str:
    if not addr:
        return addr
    value = int(addr, 16)
    delta = value - start
    if 0 <= delta < 0x100000:
        # Fixed width: Reccmp sorts these strings to build the @@ header.
        return f"+0x{delta:04x}"
    return addr


def _null_report(*_args, **_kwargs):
    return None


# ---------------------------------------------------------------------------
# Output


def _color(enabled: bool):
    if not enabled:
        return {"red": "", "green": "", "blue": "", "reset": "", "bold": ""}
    return {
        "red": "\033[31m",
        "green": "\033[32m",
        "blue": "\033[34m",
        "reset": "\033[0m",
        "bold": "\033[1m",
    }


def print_result(res: FunctionResult, *, color: bool, side_by_side: bool, width: int) -> None:
    c = _color(color)
    where = f"{res.source}:{res.line}" if res.source else "?"
    title = f"0x{res.address:08x} {res.name or '?'}"
    print(f"{c['bold']}{title}{c['reset']}  [{where}]" + (f" ({res.origin})" if res.origin else ""))
    if res.error:
        print(f"  error: {res.error}")
        for w in res.warnings:
            print(f"  warning: {w}")
        return
    flags = []
    if res.stub:
        flags.append("stub")
    if res.library:
        flags.append("library")
    if res.effective:
        flags.append("effective match")
    size_text = ""
    if res.orig_size is not None and res.recomp_size is not None:
        size_text = f"  orig 0x{res.orig_size:x} bytes, recomp 0x{res.recomp_size:x} bytes"
    print(f"  similarity: {_percent(res)}{size_text}" + (f"  [{', '.join(flags)}]" if flags else ""))
    if res.baseline is not None:
        delta = float(res.score_text) - float(res.baseline)
        tone = c["red"] if delta < 0 else (c["green"] if delta > 0 else "")
        print(f"  baseline:   {res.baseline}%  ({tone}{delta:+.2f}{c['reset']} vs docs/coverage/match-baseline.txt)")
    for w in res.warnings:
        print(f"  warning: {w}")
    if res.matching == 1.0 or not res.diff:
        return
    print()
    if side_by_side:
        _print_side_by_side(res.diff, c, width)
    else:
        _print_unified(res.diff, c)


def _print_unified(udiff, c) -> None:
    for slug, groups in udiff:
        print(f"{c['blue']}{slug}{c['reset']}")
        for group in groups:
            if "both" in group:
                for orig_addr, text, recomp_addr in group["both"]:
                    print(f"  {orig_addr:>10} {recomp_addr:>7} :  {text}")
            else:
                for orig_addr, text in group["orig"]:
                    print(f"  {orig_addr:>10} {'':>7} : {c['red']}-{text}{c['reset']}")
                for recomp_addr, text in group["recomp"]:
                    print(f"  {'':>10} {recomp_addr:>7} : {c['green']}+{text}{c['reset']}")


def _print_side_by_side(udiff, c, width: int) -> None:
    col = max(30, (width - 26) // 2)

    def cut(text: str) -> str:
        text = text.replace("\t", " ")
        return text if len(text) <= col else text[: col - 1] + "~"

    for slug, groups in udiff:
        print(f"{c['blue']}{slug}{c['reset']}")
        for group in groups:
            if "both" in group:
                for orig_addr, text, recomp_addr in group["both"]:
                    print(f"  {orig_addr:>10} {cut(text):<{col}} = {recomp_addr:>7} {cut(text)}")
                continue
            orig = group["orig"]
            recomp = group["recomp"]
            for i in range(max(len(orig), len(recomp))):
                left = orig[i] if i < len(orig) else ("", "")
                right = recomp[i] if i < len(recomp) else ("", "")
                mark = "|" if left[1] and right[1] else ("<" if left[1] else ">")
                print(
                    f"  {left[0]:>10} {c['red']}{cut(left[1]):<{col}}{c['reset']} {mark} "
                    f"{right[0]:>7} {c['green']}{cut(right[1])}{c['reset']}"
                )


# ---------------------------------------------------------------------------
# Driver


def parse_address(text: str) -> int:
    from match_baseline import parse_address as parse_hex

    try:
        return parse_hex(text)
    except ValueError as ex:
        raise argparse.ArgumentTypeError(f"not a hex address: {text}") from ex


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Compile the translation unit that holds an original LegoRR.exe "
            "function with the Reccmp MSVC 5.0 flags and score it with Reccmp's "
            "comparator, without linking."
        )
    )
    parser.add_argument("addresses", nargs="*", type=parse_address, help="original function address(es), e.g. 0x478e60")
    parser.add_argument("--source", type=Path, help="source file to compile (default: the file whose annotation names the address); with no address, compare every annotated function in it")
    parser.add_argument("--json", action="store_true", help="print one JSON document instead of text")
    parser.add_argument("--side-by-side", action="store_true", help="print the diff as aligned orig | recomp columns")
    parser.add_argument("--no-diff", action="store_true", help="print scores only")
    parser.add_argument("--color", choices=("auto", "always", "never"), default="auto")
    parser.add_argument("--orig", type=Path, help="original executable (default: reference/LegoRR.exe)")
    parser.add_argument("--build-dir", help="cache directory for the configured compile flags (default: build-match/, or MATCH_FN_BUILD_DIR)")
    parser.add_argument("--keep", action="store_true", help="keep the per-run work directory (object, log)")
    parser.add_argument("--warm", action="store_true", help="only start the persistent wineserver and configure the flags cache, then exit")
    parser.add_argument("--timings", action="store_true", help="print phase timings in text mode")
    parser.add_argument("-v", "--verbose", action="store_true", help="show Reccmp log messages")
    return parser


def find_orig(args) -> Path:
    if args.orig:
        return args.orig
    user_yml = PROJECT_ROOT / "reccmp-user.yml"
    if user_yml.exists():
        m = re.search(r"^\s*path:\s*(\S+)", user_yml.read_text(), re.M)
        if m:
            p = Path(m.group(1))
            return p if p.is_absolute() else PROJECT_ROOT / p
    return PROJECT_ROOT / "reference/LegoRR.exe"


def load_baseline_scores(path: Path) -> dict[int, str]:
    """Full-scope scores from the committed match baseline, if present."""
    if not path.exists():
        return {}
    from match_baseline import load_baseline

    try:
        return {addr: score for addr, (score, _name) in load_baseline(path)["full"].items()}
    except (ValueError, KeyError):
        return {}


# End of the matching build's linked address span: 0x400000..0x9c1000 at the
# time of writing, since its .bss is far larger than the original's. Taken
# from an existing matching-build LegoRR.exe when there is one; otherwise this
# default. MATCH_FN_LINK_END overrides both.
DEFAULT_LINK_END = 0xA00000


def link_end(orig_bin, root: Path) -> int:
    value = os.environ.get("MATCH_FN_LINK_END")
    if value:
        return int(value, 0)
    orig_end = max(s.virtual_range.stop for s in orig_bin.sections)
    exe = Path(os.environ.get("RECCMP_BUILD_DIR") or root / "build-reccmp") / "LegoRR.exe"
    if exe.is_file():
        try:
            from reccmp.formats import detect_image

            linked = detect_image(filepath=exe)
            return max(orig_end, max(s.virtual_range.stop for s in linked.sections))
        except Exception:  # pylint: disable=broad-except
            pass
    return max(orig_end, DEFAULT_LINK_END)


def load_libc_index(env: Environment) -> tuple[set[str], bytes | None, dict[str, int]]:
    lib = env.toolchain / "lib" / "LIBC.LIB"
    if not lib.exists():
        for candidate in (env.toolchain / "lib").glob("*"):
            if candidate.name.upper() == "LIBC.LIB":
                lib = candidate
                break
    if not lib.exists():
        return set(), None, {}
    data = lib.read_bytes()
    index = msvc_coff.archive_symbol_index(data)
    return set(index), data, index


def run(args: argparse.Namespace) -> tuple[dict[str, Any], list[FunctionResult]]:
    # pylint: disable=too-many-locals,too-many-branches,too-many-statements
    timings: dict[str, float] = {}
    t_start = time.monotonic()
    env = Environment.detect(args)
    commands = load_compile_commands(env, timings)
    if args.warm:
        if "configure" not in timings:
            # Still start the persistent wineserver.
            log_path = env.build_dir / "warm.log"
            env.run(["true"], cwd=env.root, log_path=log_path, timeout=120)
        timings["total"] = time.monotonic() - t_start
        return {"warm": True, "build_dir": str(env.build_dir), "timings": timings}, []
    t0 = time.monotonic()

    from reccmp.dir import source_code_search
    from reccmp.formats import TextFile, detect_image

    orig_path = find_orig(args)
    if not orig_path.exists():
        raise MatchError(f"original executable not found: {orig_path} (symlink reference/ into the worktree)")
    orig_bin = detect_image(filepath=orig_path)

    code_paths = source_code_search([env.root / "src"])
    code_files = list(TextFile.from_files(code_paths, allow_error=True, encoding="utf-8"))
    markers = scan_markers(code_files)
    by_source = {c.source: c for c in commands}
    program_sources = {c.source for c in commands if c.target == "program"}

    # Work out which source file(s) to compile.
    plan: dict[Path, list[int]] = {}
    results: dict[int, FunctionResult] = {}
    if args.source is not None:
        source = (args.source if args.source.is_absolute() else Path.cwd() / args.source).resolve()
        addrs = list(args.addresses) or sorted(
            a for a, m in markers.items() if m.path.resolve() == source and m.kind in ("FUNCTION", "STUB", "LIBRARY", "TEMPLATE", "SYNTHETIC")
        )
        if not addrs:
            raise MatchError(f"no annotated functions in {source}")
        plan[source] = addrs
    else:
        if not args.addresses:
            raise MatchError("give an original function address, or --source FILE")
        for addr in args.addresses:
            marker = markers.get(addr)
            if marker is None:
                res = FunctionResult(addr, error="no // FUNCTION/LIBRARY: LEGORR annotation names this address in src/")
                results[addr] = res
                continue
            plan.setdefault(marker.path.resolve(), []).append(addr)
    timings["locate"] = time.monotonic() - t0

    libc_symbols, libc_data, libc_index = load_libc_index(env)
    t = time.monotonic()
    ctx = ProjectContext.build(orig_bin, code_files, markers, program_sources, libc_symbols)
    ctx.libc_data, ctx.libc_index = libc_data, libc_index
    ctx.link_end = link_end(orig_bin, env.root)
    timings["annotations"] = time.monotonic() - t

    env.build_dir.mkdir(parents=True, exist_ok=True)
    runs_dir = env.build_dir / "runs"
    runs_dir.mkdir(parents=True, exist_ok=True)
    workdir = Path(tempfile.mkdtemp(prefix="run-", dir=runs_dir))
    objects: list[str] = []
    try:
        # Compile every planned translation unit at once; Wine copes fine with
        # parallel cl.exe processes and this keeps multi-file runs short.
        t = time.monotonic()
        jobs: dict[Path, Any] = {}
        with concurrent.futures.ThreadPoolExecutor(max_workers=min(4, max(1, len(plan)))) as pool:
            for index, source in enumerate(plan):
                command = by_source.get(source)
                if command is not None:
                    jobs[source] = pool.submit(compile_source, env, command, workdir, index)
        timings["compile"] = time.monotonic() - t

        for source, addrs in plan.items():
            rel = os.path.relpath(source, env.root)
            if source not in jobs:
                for addr in addrs:
                    results[addr] = FunctionResult(addr, source=rel, error=f"{rel} is not compiled by the matching build (no compile command)")
                continue
            try:
                obj_path, _output = jobs[source].result()
            except MatchError as ex:
                for addr in addrs:
                    marker = markers.get(addr)
                    results[addr] = FunctionResult(addr, source=rel, line=marker.line if marker else None, error=str(ex))
                continue
            objects.append(str(obj_path))

            t = time.monotonic()
            obj = msvc_coff.read_coff(obj_path)
            img = SyntheticImage(obj, ctx, source, source in program_sources)
            analysis = analyse(img, ctx)
            timings["analyse"] = timings.get("analyse", 0.0) + time.monotonic() - t

            t = time.monotonic()
            pending_libc: list[int] = []
            for addr in addrs:
                res = compare_function(analysis, ctx, addr)
                marker = markers.get(addr)
                res.source = rel
                res.line = marker.line if marker else None
                res.origin = "compiled"
                if res.error and marker is not None and marker.kind == "LIBRARY" and libc_data is not None:
                    pending_libc.append(addr)
                results[addr] = res
            for addr in pending_libc:
                lib_res = compare_libc(addr, ctx, markers[addr], libc_data, libc_index)
                if lib_res is not None:
                    lib_res.source = rel
                    lib_res.line = markers[addr].line
                    results[addr] = lib_res
            timings["compare"] = timings.get("compare", 0.0) + time.monotonic() - t
    finally:
        if args.keep:
            log.info("work directory kept: %s", workdir)
        else:
            shutil.rmtree(workdir, ignore_errors=True)

    from match_baseline import DEFAULT_BASELINE

    baseline = load_baseline_scores(env.root / DEFAULT_BASELINE)
    for res in results.values():
        entry = baseline.get(res.address)
        if entry is not None:
            res.baseline = entry

    timings["total"] = time.monotonic() - t_start
    ordered = [results[a] for a in (args.addresses or sorted(results)) if a in results]
    if not args.addresses:
        ordered = [results[a] for a in sorted(results)]
    meta = {
        "orig": str(orig_path),
        "objects": objects if args.keep else [],
        "workdir": str(workdir) if args.keep else None,
        "timings": {k: round(v, 3) for k, v in timings.items()},
    }
    return meta, ordered


def compare_libc(addr: int, ctx: ProjectContext, marker: Marker, libc_data: bytes, libc_index: dict[str, int]) -> FunctionResult | None:
    """LIBRARY functions that the matching build links from LIBC.LIB rather than
    from the reconstruction: score the library member's code."""
    from reccmp.parser import DecompCodebase

    codebase = DecompCodebase([tf for tf in ctx.code_files if Path(tf.path) == marker.path], TARGET_ID)
    symbol = None
    for fun in codebase.iter_name_functions():
        if fun.offset == addr:
            symbol = fun.name
    if not symbol or symbol not in libc_index:
        return None
    member = _pick_libc_member(libc_data, libc_index, symbol)
    obj = msvc_coff.parse_coff(member.data, f"LIBC.LIB({msvc_coff.member_basename(member)})")
    img = SyntheticImage(obj, ctx, marker.path, False)
    analysis = analyse(img, ctx)
    res = compare_function(analysis, ctx, addr)
    res.origin = f"LIBC.LIB({msvc_coff.member_basename(member)})"
    return res


# Startup symbols (__amsg_exit, _WinMainCRTStartup, ...) are defined by every
# crt0 flavour in LIBC.LIB; a /subsystem:windows ANSI link pulls wincrt0.obj.
PREFERRED_LIBC_MEMBERS = ("wincrt0.obj",)


def _pick_libc_member(libc_data: bytes, libc_index: dict[str, int], symbol: str):
    default = msvc_coff.archive_member_at(libc_data, libc_index[symbol])
    for member in msvc_coff._archive_members(libc_data):  # pylint: disable=protected-access
        base = msvc_coff.member_basename(member).lower()
        if base not in PREFERRED_LIBC_MEMBERS:
            continue
        try:
            obj = msvc_coff.parse_coff(member.data, member.name)
        except msvc_coff.CoffError:
            continue
        if any(s.name == symbol and s.section > 0 for s in obj.symbols.values()):
            return member
    return default


def main(argv: list[str] | None = None) -> int:
    parser = build_arg_parser()
    args = parser.parse_args(argv)
    logging.basicConfig(
        level=logging.INFO if args.verbose else logging.CRITICAL,
        format="%(levelname)s %(name)s: %(message)s",
    )
    if not args.verbose:
        logging.getLogger("reccmp").setLevel(logging.CRITICAL)
        logging.getLogger().setLevel(logging.CRITICAL)
    try:
        meta, results = run(args)
    except MatchError as ex:
        if args.json:
            print(json.dumps({"error": str(ex)}, indent=2))
        else:
            print(f"match_function: {ex}", file=sys.stderr)
        return 2

    if args.json:
        doc = dict(meta)
        doc["functions"] = [r.to_json() for r in results]
        if args.no_diff:
            for f in doc["functions"]:
                f.pop("diff", None)
        print(json.dumps(doc, indent=2))
    elif meta.get("warm"):
        print(f"warm: flags cached in {meta['build_dir']}; wineserver kept alive")
    else:
        color = args.color == "always" or (args.color == "auto" and sys.stdout.isatty())
        width = shutil.get_terminal_size((160, 40)).columns
        for res in results:
            if args.no_diff:
                res = FunctionResult(**{**res.__dict__, "diff": None})
            print_result(res, color=color, side_by_side=args.side_by_side, width=width)
            print()
        if args.timings:
            print("timings: " + ", ".join(f"{k} {v:.2f}s" for k, v in meta["timings"].items()))
    return 1 if any(r.error for r in results) else 0


if __name__ == "__main__":
    sys.exit(main())
