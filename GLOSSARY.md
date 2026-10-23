# lrr-portable

Portable reconstruction of *Lego Rock Raiders* (1999, Windows 95/98) from its
shipped binary and data archives — the end goal is a **native Linux port** (no
Wine layer) by swapping _platform_ adapters behind the DX6-era _seam_, with a
Win32/MinGW *reference build* kept for A/B against the original. Everything
here derives from interoperability research; the repo contains no original
game content (LGPL-3.0-or-later).

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

**seam**:
The DX6-era API surface recorded in finding 15 §1 — the set of entry points
(DDraw, d3drm, DInput, DSOUND, WINMM, AVIFIL32…) the game logic was written
against. Everything above it is platform-neutral; the *platform* option
picks which adapter satisfies it. `src/platform/platform.h` re-exports that
surface under one roof.
_Avoid_: "boundary", "port" when you mean the seam

**platform**:
The meson option (`-Dplatform=native|winapi`) that selects which adapter
satisfies the seam at build time. Named `platform` rather than "backend"
because Meson reserves `backend` for its own build backend; the concept the
docs otherwise call *backend* (the adapter) maps one-to-one on to it.
_Avoid_: "the backend option" (misleading — the option is `platform`)

**decompile corpus**:
The named Ghidra pseudocode dump at `docs/decompiles/legacy/engine-region.c`
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
(Linux first) with **no Wine layer**, via the `native` platform adapter
(miniwin) at the *seam*. Not to be confused with _port run_.
_Avoid_: "the Wine build", "native Linux build" when you mean the product

**miniwin**:
The LGPL-3.0 source-compatible *platform* adapter from the
isledecomp/isle-portable project that reimplements the DX6-era WinAPI surface
(DirectDraw, D3DRM skeleton, etc.) over SDL3. Brought in as a sparse submodule
limited to `miniwin/` and extended by upstream contributions, not forks.
_Avoid_: "the SDL layer", "the shim" when context needs the product name

**reference build**:
The `winapi` platform adapter (Win32/MinGW cross build) that compiles the same
source against real DirectX headers. Its purpose is validation: A/B comparison
against the shipped binary in the *same* harness (A.2, _port run_), not
shipping. The _port_ and the _reference build_ are the two adapters of one
source tree at one seam (ADR 0004).

**port run**:
Launching a binary rebuilt from the reconstruction through the A.2 harness,
as opposed to launching the shipped binary. Only possible via an explicit
`--run-port` flag that bypasses the SHA-256 integrity gate and labels the run
in the log, so a port run can never be mistaken for evidence from the shipped
binary (ADR 0002). A *port run* exercises the *reference build*, not the
native *port*.
_Avoid_: "rebuilt run", "dev run"