# lrr-portable

Portable reconstruction of **Lego Rock Raiders**, a real-time strategy game
published by LEGO Media International in 1999 for Windows 95/98.

The goal is a **native Linux port with no Wine layer** (ADR 0004), built from
the decompile corpus, by recompiling the game's logic against a
source-compatible shim (**miniwin**, LGPL, over SDL3) instead of the original
DirectX. A Win32/MinGW build is kept as a *reference build* for A/B
comparison against the shipped binary (ADR 0002, 0004).

## The game

Lego Rock Raiders cast the player as a team of LEGO minifigures mining a planet for
"power rocks." It shipped in 1999, is a 32-bit Windows binary using DirectX, and has been
largely forgotten by official channels ever since. It is of interest here for preservation
and curiosity; nothing in this repository is affiliated with or endorsed by the LEGO Group.

## Repository layout

```
README.md           This file
docs/findings/      Reverse-engineering findings, 01–19 (see index below)
tools/              WAD tooling (rrwad.py: unpack/pack/verify — see tools/README.md)
                    + model converter (tools/rrlwo.py: .lwo → glTF 2.0)
                    + compat harness (tools/compat/a2.py — see tools/compat/README.md)
docs/decompiles/    Named Ghidra pseudocode corpus (engine-region.c)
docs/evidence/      Exported REA evidence bundle (address-level citations)
tools/              Analysis tooling: unwad.py, Ghidra function-naming script
docs/adr/           Architecture decision records (build/port decisions)
source/             (gitignored) the original, unmodified game files
unpacked/           (gitignored) 5,845 assets extracted from the .wad archives
cross/              Meson cross files (i686.ini — MinGW-w64 Win32 toolchain)
meson.build         Reconstruction build (top-level; platform=native|winapi)
meson_options.txt   Build options (src_root, platform)
src/                Reconstruction source tree (stubs + gods98 / game / platform)
subprojects/        miniwin CMake bridge (sparse isle-portable submodule)
3rdparty/           Dependency submodules (isle-portable, sparse → miniwin/)
```

Everything under `source/` is the original game as shipped. It is **not** committed to git
(copyright), and is expected to be provided by each person working in this repo from a
legally obtained copy of the game.

### What's in `source/`

| Path | Description |
| --- | --- |
| `LegoRR.exe` | The main game executable — primary reverse engineering target |
| `LegoRR0.wad`, `LegoRR1.wad` | Game data archives (assets and possibly logic/data) |
| `LegoRR.icd` | Installation/configuration data |
| `D3DRM.DLL` | DirectX 5 render-mesh library dependency |
| `CLGen.exe`, `CLGen.dat` | Install-time generator component |
| `Data/` | Loose runtime data — streamed sounds (`Data/Sounds/Streamed/`), video (`Data/AVI/`) |
| `Readme.txt` | Original publisher readme |

## Goals

- [x] Map the high-level architecture of `LegoRR.exe` (engine, game logic, rendering)
      — findings 02–06, 10
- [x] Document the `.wad` archive container format(s) — finding 01
- [x] Document asset formats: 3D models, animations, sounds, videos
      — findings 11–14: models/scenes, `Lego*` configs, textures, audio, video.
      (Fine detail open: `.flh` container — 14 §5. `.uv` grammar closed — 19;
      `.x` identified as XOF-text leftovers the engine never loads — 11 §6.)
- [x] Understand mission / level scripting and data-driven behavior — findings 07, 09
- [x] Understand the savegame format — finding 08
- [x] Eventually: reimplementations or tooling that let the game run on modern systems
      — A.1 audit (finding 15) → A.2 live test (finding 16): **the shipped exe launches
      and renders under GE-Proton via `tools/compat/a2.py run`** (2026-10-20)

## Current status (2026-10-20)

Research is complete: all 19 findings shipped, every format shipped in the wads is
documented address-level — including the two that exist but are never loaded
(`.x` XOF-text leftovers, `.blx` editor-only) — and the `Lego*` block-text system that
`.ae` belongs to (finding 12). The game runs and is playable through the Proton wrapper,
and the toolchain (unpack/pack byte-exact, model+uv convert **and** write, live launcher)
is regression-verified against the shipped corpus.

Open threads, in order of value:
1. **Live mod proof** — the chain grammar → writer → byte-exact repack → launch is proven
   only link-by-link; repacking a modified `.uv` (e.g. a `GC_Larm.uv` pilot re-skin) and
   observing the change in-game has not been done.
2. `.flh` container file layout (finding 14 §5).
3. Small research questions: the 40-entry config population site (finding 04 §Open),
   the input-device writer region (finding 05), D3DRM HAL-vs-software (perf only).
4. Deferred: dgVoodoo as a perf option; the `Steam.lwo` point-only sprites as glTF.

## Findings index

| # | File | Subject |
| --- | --- | --- |
| 01 | [`findings/01-binary-and-loader-map.md`](docs/findings/01-binary-and-loader-map.md) | PE structure, WinMain, resource loader, WAD container format |
| 02 | [`findings/02-startup-and-main-loop.md`](docs/findings/02-startup-and-main-loop.md) | Startup sequence and frame loop, callback triplet |
| 03 | [`findings/03-engine-tick.md`](docs/findings/03-engine-tick.md) | Engine tick, state block, command queue, quit poll |
| 04 | [`findings/04-sim-input-and-events.md`](docs/findings/04-sim-input-and-events.md) | Input system, key table, edge detection, transitions |
| 05 | [`findings/05-exit-selection-and-text-input.md`](docs/findings/05-exit-selection-and-text-input.md) | Exit paths, selection, text input |
| 06 | [`findings/06-command-queue-and-selection.md`](docs/findings/06-command-queue-and-selection.md) | Command queue, selection manager, `KeepRunning` |
| 07 | [`findings/07-level-loading.md`](docs/findings/07-level-loading.md) | `.map` format, voxel grid, ten-layer loading, terrain |
| 08 | [`findings/08-save-and-checkpoints.md`](docs/findings/08-save-and-checkpoints.md) | `.sav`/`.osf` save format, checkpoint blob, no entity dump |
| 09 | [`findings/09-reward-task-manager.md`](docs/findings/09-reward-task-manager.md) | Mission task block, reward overlay, reward-zone save trigger |
| 10 | [`findings/10-entity-command-primitives.md`](docs/findings/10-entity-command-primitives.md) | Command records, AI task/priority registry, opcode map |
| 11 | [`findings/11-model-and-scene-formats.md`](docs/findings/11-model-and-scene-formats.md) | LightWave LWO loader, LWSC scene parser, surface/UV pipeline |
| 12 | [`findings/12-block-text-format.md`](docs/findings/12-block-text-format.md) | `Lego*` block-text config format: scanner, path-lookup API, `.ae` consumers |
| 13 | [`findings/13-texture-system.md`](docs/findings/13-texture-system.md) | BMP surface loader, name cache, LWO material binding, world tile sets, VRAM budget |
| 14 | [`findings/14-sound-and-video.md`](docs/findings/14-sound-and-video.md) | WAVE/mmio sound bank, CD-DA music via MCI, AVIFIL32 movies, logo splash |
| 15 | [`findings/15-compat-audit.md`](docs/findings/15-compat-audit.md) | Modern-compat API-surface audit: wrapper coverage matrix, launch-gate scan, friction list |
| 16 | [`findings/16-a2-live-test.md`](docs/findings/16-a2-live-test.md) | **A.2 live test passed**: game launches under GE-Proton10-25; DDraw interface climbing observed (→ `IDirectDraw4`); movies via quartz; CDDA question deferred (→ 17: resolved — digital release carries no CDDA media) |
| 17 | [`findings/17-interactive-a2-session.md`](docs/findings/17-interactive-a2-session.md) | **Playable**: first mission completes; music is streamed WAVs (no CDDA media on digital release); movies skip on missing Indeo 5 decoder; NVIDIA Xid driver-incident notes |
| 18 | [`findings/18-movies-libav-alias.md`](docs/findings/18-movies-libav-alias.md) | **Movies fixed**: Indeo block was two missing sonames (`libbz2.so.1.0`, `libvpx.so.6`) silently killing wine's ffmpeg plugin — aliases + registry rebuild, automated in `a2.py` |
| 19 | [`findings/19-uv-grammar.md`](docs/findings/19-uv-grammar.md) | **Last format closed**: `.uv` sidecar grammar in `FUN_00481c50` — surface texture-path overrides + per-corner UV (v negated, vertex duplication); names/keyframe tail structurally unread; validated 60/60 |

Named-pseudocode corpus: [`docs/decompiles/engine-region.c`](docs/decompiles/engine-region.c)
(114 named functions covering the engine/game-logic region — findings 01–10).
Functions named by findings 11–19 (loaders, parsers, the asset side) are cited
with pseudocode in each finding's function map.
Machine-readable evidence: [`docs/evidence/legorr-rev.bundle`](docs/evidence/legorr-rev.bundle)
(148 recorded REA observations against `LegoRR.exe`,
SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).

## Reproducibility

- **Unpacking**: `python3 tools/unwad.py source/LegoRR1.wad unpacked/` extracts both archives.
- **Function naming**: `tools/ghidra/CreateEngineFunctions.java` is a Ghidra headless
  post-script that creates/renames the analyzed functions and emits the decompile corpus;
  batch runs use `support/analyzeHeadless` with `-postScript`.
- **Interactive analysis**: REA (Ghidra provider). Findings cite addresses, not guesses:
  every claim traces to a pseudocode block or an evidence record.

## Reconstruction build (stub)

Meson build with one source tree and two platform backends (ADR 0004):

- **`native`** (primary, end goal): compiles the reconstruction C++ against
  **miniwin** (SDL3) — no Wine layer. Host compiler, no cross file. miniwin
  comes in as a sparse git submodule — bootstrap with `tools/setup-miniwin.sh`
  (blob-none + cone sparse to `miniwin/` only; the sibling game's decompiled
  source is never written to the working tree).
- **`winapi`** (reference): cross-compiles the same source to a 32-bit Win32
  `LegoRR.exe` (MinGW-w64) for A/B against the shipped binary under the A.2
  harness — one environment, direct comparison (ADR 0002).

```sh
# native (primary)
meson setup build-native -Dplatform=native
meson compile -C build-native

# winapi reference build
meson setup build-winapi --cross-file cross/i686.ini -Dplatform=winapi
meson compile -C build-winapi
```

Toolchain requirements: `native` needs SDL3 devel headers and cmake
(miniwin), `winapi` needs `i686-w64-mingw32-gcc`/`windres`/`ar` (this box is
immutable — install inside a distrobox/podman Fedora container). Rebuilt
Win32 binaries run through `a2.py` only via the (not yet wired)
`run --run-port` interface — ADR 0002, ADR 0004.

As discoveries are made, they should be documented here in the repo (e.g. `docs/` per
format) with byte-level field tables where possible.

## Working in this repo

1. Place a legally owned copy of the game in `source/` (see layout above).
2. Do not commit anything under `source/`.
3. Analysis output — notes, decompiler dumps, parsers, experiments — lives in the tracked
   part of the repo.

## Legal note

Lego Rock Raiders is © 1999 LEGO Media International. All original game assets are the
property of their respective owners and remain copyright LEGO. This repository exists
solely for interoperability research, documentation, and preservation purposes and
contains no original game content. Do not upload copies of the game anywhere.

The reconstruction source, build system, tooling and documentation in this
repository are licensed **LGPL-3.0-or-later** (see `LICENSE`); the
miniwin adapter is similarly LGPL-3.0. That license covers *our code*, not the
game's assets or binary.
