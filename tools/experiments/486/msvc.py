# SPDX-License-Identifier: LGPL-3.0-or-later
"""Compile C sources with the matching build's MSVC 5.0 flags and fingerprint
one function's code from the /FAs listing (issue #486).

Runs the compiler directly when `wine` is on PATH, otherwise through
`distrobox enter developer` (override with MSVC486_DISTROBOX; empty disables).
"""
from __future__ import annotations

import hashlib
import json
import os
import re
import shlex
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
TOOLCHAIN = Path(os.environ.get(
    "REPROBIT_MSVC_5_0_RTM_ROOT",
    Path.home() / ".local/share/reprobit/toolchains/msvc_5_0_rtm"))
CL = TOOLCHAIN / "wine/x86/cl"
# The matching build's flags for a C unit (build-match/cmake/compile_commands.json),
# minus /Zi, /Fo and /Fd, plus /FAs for the listing.
FLAGS = ["/nologo", "/TC", "/DWIN32", "/D_WINDOWS", "/ML", "/O2", "/Ob1",
         "/DNDEBUG", "/FAs", "/c"]
INCLUDES = ["-DLEGORRR_BACKEND_WINAPI=1",
            "-I" + str(ROOT / "src/compat/msvc5"), "-I" + str(ROOT / "src"),
            "-I" + str(ROOT / "src/gods98"), "-I" + str(ROOT / "src/game")]


def _wrap(script: str) -> list[str]:
    box = os.environ.get("MSVC486_DISTROBOX", "developer")
    if subprocess.run(["sh", "-c", "command -v wine"], capture_output=True).returncode == 0 or not box:
        return ["bash", "-c", script]
    return ["distrobox", "enter", box, "--", "bash", "-lc", script]


def flags_for(source: str) -> list[str]:
    """The matching build's flags for `source` (repo-relative), from the cached
    configure of tools/match_function.py (run `make match-function` once), minus
    /Zi, /Fo, /Fd and the source path, plus /FAs."""
    db = ROOT / "build-match/cmake/compile_commands.json"
    target = (ROOT / source).resolve()
    if not db.exists():
        raise SystemExit("run `make match-function ADDR=0x004869a0` once to cache the flags")
    for entry in json.loads(db.read_text()):
        if Path(entry["file"]).resolve() == target:
            argv = shlex.split(entry["command"])[1:]
            keep = [a for a in argv if not a.startswith(("/Fo", "/Fd", "/Zi", "-c", "/c"))
                    and Path(a).resolve() != target]
            return keep + ["/FAs", "/c"]
    if target.suffix == ".c":
        return FLAGS + INCLUDES  # a standalone experiment file: the C-unit flags
    raise KeyError(source)


def compile_many(workdir: Path, names: list[str], extra: list[str] | None = None,
                 preprocess: bool = False, flags: list[str] | None = None,
                 ext: str = ".c") -> None:
    """Compile workdir/<name><ext> for every name in one container session.

    `flags` defaults to the C-unit flags (FLAGS + INCLUDES); pass flags_for()
    to reproduce another unit. With preprocess=True, write workdir/<name>.i
    (cl /EP) instead."""
    flags = (flags or FLAGS + INCLUDES) + (extra or [])
    cl = " ".join(map(shlex.quote, [str(CL)] + flags))
    cmds = []
    for n in names:
        q = shlex.quote
        if preprocess:
            cmds.append(f"{cl} /EP {q(n + ext)} > {q(n + '.i')} 2>/dev/null")
        else:
            cmds.append(f"rm -f {q(n + '.asm')}; {cl} {q(n + ext)} > {q(n + '.log')} 2>&1 || true")
    jobs = workdir / "_jobs.txt"
    jobs.write_text("\n".join(cmds) + "\n")
    par = int(os.environ.get("MSVC486_JOBS", "4"))
    script = (f"cd {shlex.quote(str(workdir))} && "
              f"tr '\\n' '\\0' < _jobs.txt | xargs -0 -P {par} -n 1 bash -c")
    subprocess.run(_wrap(script), check=True)


def function_body(listing: Path, func: str) -> list[str] | None:
    """Instructions of `func` from a /FAs listing, symbol numbers and comments removed."""
    if not listing.exists():
        return None
    text = listing.read_text(errors="replace").splitlines()
    out, inside = [], False
    for line in text:
        if re.match(rf"_?{re.escape(func)}\s+PROC", line):
            inside = True
            continue
        if inside and re.match(rf"_?{re.escape(func)}\s+ENDP", line):
            return out
        if not inside:
            continue
        line = line.split(";", 1)[0].strip()
        if not line or line.endswith(":") and " " not in line:
            if line:
                out.append("L:")
            continue
        line = re.sub(r"\$[A-Z]\d+", "$N", line)
        line = re.sub(r"\$L\d+", "$L", line)
        line = re.sub(r"\s+", " ", line)
        out.append(line)
    return None


def fingerprint(body: list[str] | None) -> str:
    if body is None:
        return "ERR"
    return hashlib.sha1("\n".join(body).encode()).hexdigest()[:8]
