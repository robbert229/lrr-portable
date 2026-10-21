# lrr-portable

Portable reconstruction of *Lego Rock Raiders* (1999, Windows 95/98) from its
shipped binary and data archives — the end goal is a **native Linux port** (no
Wine layer) by adapting the DX6-era API surface to another backend, with a
Win32/MinGW *reference build* kept for A/B against the original. Everything
here derives from interoperability research; the repo contains no original
game content (LGPL-3.0-or-later, ADR 0005).

## Language

**gods98**:
The original in-house engine codeline the game was built on, seen in dev-tree
strings like `C:\Dev\SourceSafe\gods98_dx6\gods98\src\{Main,Viewports,Images,Input}.c`.
The reconstruction's source layout mirrors this engine/game split.
_Avoid_: "engine code" when you mean the literal `gods98` subtree

**WAD**:
The game's archive container format (`WWAD` magic, `LegoRR0.wad`/`LegoRR1.wad`),
documented in finding 01 and round-tripped byte-identical by `tools/rrwad.py`.
_Avoid_: "zip", "resource file"

**A.2 harness**:
The compat test runner `tools/compat/a2.py` that launches the game under
GE-Proton/Wine with log capture and triage. Since ADR 0004 it is the
validation rig for the *reference build*, not a shipping path.
_Avoid_: "the wine script", "the compat tool"

**decompile corpus**:
The named Ghidra pseudocode dump at `docs/decompiles/engine-region.c`
(114 functions) — the raw material the reconstruction is written from.
_Avoid_: "the decomp", "engine-region"

**reconstruction**:
The recompilable C++ source tree that reimplements `LegoRR.exe` from the
decompile corpus. Lives under `src/` (relocatable via the `src_root` meson
option), split into per-subsystem static libraries mirroring gods98. Written
in C++ with discipline (plain structs, C++ where the seam demands it — ADR
0004), compiled against the DX6-era API surface from finding 15.
_Avoid_: "the decompiled source" when you mean the new build tree

**port**:
The end-goal product: the reconstruction running natively on a modern host
(Linux first) with **no Wine layer**, via a backend that satisfies the DX6-era
API surface. The primary build is the `native` meson platform (miniwin).
Not to be confused with _port run_.
_Avoid_: "the Wine build", "native Linux build" when you mean the product

**miniwin**:
The LGPL-3.0 source-compatible shim from the isledecomp/isle-portable project
that reimplements the DX6-era WinAPI surface (DirectDraw, D3DRM skeleton,
etc.) over SDL3; our `native` backend. Brought in as a sparse submodule
limited to `miniwin/` and extended by upstream contributions, not forks.
_Avoid_: "the SDL layer", "the shim" when context needs the product name

**reference build**:
The Win32/MinGW cross build (`winapi` meson platform) that compiles the same
source against real DirectX headers. Its purpose is validation: A/B comparison
against the shipped binary in the *same* harness (A.2, `port run`), not
shipping. The terms _port_ and _reference build_ are the two adapters of one
source tree (ADR 0004).

**port run**:
Launching a binary rebuilt from the reconstruction through the A.2 harness,
as opposed to launching the shipped binary. Only possible via an explicit
`--run-port` flag that bypasses the SHA-256 integrity gate and labels the run
in the log, so a port run can never be mistaken for evidence from the shipped
binary (ADR 0002). A *port run* exercises the *reference build*, not the
native *port*.
_Avoid_: "rebuilt run", "dev run"