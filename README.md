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
docs/findings/      Reverse-engineering findings, 01–22 (see index below)
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
      (Fine detail closed: `.flh` container — 21 (was 14 §5). `.uv` grammar
      closed — 19; `.x` identified as XOF-text leftovers the engine never
      loads — 11 §6.)
- [x] Understand mission / level scripting and data-driven behavior — findings 07, 09
- [x] Understand the savegame format — finding 08
- [x] Eventually: reimplementations or tooling that let the game run on modern systems
      — A.1 audit (finding 15) → A.2 live test (finding 16): **the shipped exe launches
      and renders under GE-Proton via `tools/compat/a2.py run`** (2026-10-20)

## Current status (2026-10-21)

Research is complete: all 22 findings shipped, every format shipped in the wads is
documented address-level — including the two that exist but are never loaded
(`.x` XOF-text leftovers, `.blx` editor-only) — and the `Lego*` block-text system that
`.ae` belongs to (finding 12). The game runs and is playable through the Proton wrapper,
and the toolchain (unpack/pack byte-exact, model+uv convert **and** write, live launcher)
is regression-verified against the shipped corpus.

On the reconstruction side, the engine scaffold, frame loop, input, audio and
now the **game layer** (level `.map` loading, objectives/advisor, savegames —
findings 07/08/22) build and pass their suites natively; see the
*Reconstruction build* section below.

The fresh verify pass (finding 20, 2026-10-20) re-derived the critical path — PE entry
(`0x48f000`) → WinMain (`FUN_004777a0`) → init → frame loop → engine tick (`0x4232e0`) —
in a clean Ghidra session: no contradicting evidence, and the named corpus reproduces
byte-identical (7,653 lines, `diff` exit 0). It corrected one finding-02 reading:
`DAT_00505814` holds the exe basename minus `.EXE` (the `.wad` prefix), not the install
directory; the real data directory is `cwd\Data` (`DAT_005342a8`).

Open threads, in order of value:
1. **Live mod proof** — the chain grammar → writer → byte-exact repack → launch is proven
   only link-by-link; repacking a modified `.uv` (e.g. a `GC_Larm.uv` pilot re-skin) and
   observing the change in-game has not been done.
2. Deferred: dgVoodoo as a perf option; the `Steam.lwo` point-only sprites as glTF.
3. Reconstruction next: the entity/AI layer (findings 09/10) that consumes the loaded
   grid + objectives, and the winapi reference build against real DirectX headers.

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
| 20 | [`findings/20-verify-pass.md`](docs/findings/20-verify-pass.md) | **Fresh verify pass** (2026-10-20): critical path re-derived in a clean session; corpus byte-identical; `DAT_00505814` = `.wad` prefix, not install dir; CRT entry `0x48f000` |
| 21 | [`findings/21-flh-container.md`](docs/findings/21-flh-container.md) | **`.flh` closed**: FLIC-family overlay container — 136 B header, `0xF1FA` chunk stream from `0x80`, RGB555 FLI16 keyframe (opcode 25) + word-diff deltas (opcode 27) + hold chunks, 555→565 on write; validated 18/18 files, 469/469 chunks, rendered frame check |
| 22 | [`findings/22-research-crumbs.md`](docs/findings/22-research-crumbs.md) | **Last open questions closed**: message registry = startup catalog (`FUN_00419230`, 40 `Info_*` names) + `InfoMessages` config population (`FUN_00434c10`); `DAT_004ffbc0` relabeled as the objective state word (not input devices); D3DRM is software-path-only (no HAL device referenced anywhere) |

Named-pseudocode corpus: [`docs/decompiles/engine-region.c`](docs/decompiles/engine-region.c)
(114 named functions covering the engine/game-logic region — findings 01–10).
Functions named by findings 11–19 (loaders, parsers, the asset side) are cited
with pseudocode in each finding's function map.
Machine-readable evidence: [`docs/evidence/legorr-rev.bundle`](docs/evidence/legorr-rev.bundle)
(236 recorded REA observations against `LegoRR.exe`,
SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).

## Reproducibility

- **Unpacking**: `python3 tools/unwad.py source/LegoRR1.wad unpacked/` extracts both archives.
- **Function naming**: `tools/ghidra/CreateEngineFunctions.java` is a Ghidra headless
  post-script that creates/renames the analyzed functions and emits the decompile corpus;
  batch runs use `support/analyzeHeadless` with `-postScript`.
- **Interactive analysis**: REA (Ghidra provider). Findings cite addresses, not guesses:
  every claim traces to a pseudocode block or an evidence record.

## Reconstruction build

Meson build with one source tree and two platform backends (ADR 0004). The
`native` build is green and the driver (`src/main.cpp`) now exercises the
**engine scaffold** plus the **game layer** (levels, objectives, savegames —
findings 07/08/22): `EngineTick`/`EnginePreTick`/`KeepRunning`/
`MainSimStep`/`EngineShutdown` in `src/gods98/engine.*` mirror the corpus's
critical path (`0x4232e0`, `0x4231f0`, `0x435940`, `0x4250c0`, `0x424cf0/0x424d00`)
over the `DAT_005560d4` state block, and `src/gods98/tickers.cpp` reproduces the
tick's ordered subsystem dispatch (finding 03 §4) as fill-in stubs
(`ticker_<FUN-hex>` ↔ corpus `FUN_<hex>`).

**Resource loader** (`src/gods98/resource.*`, finding 01): mounts
`<dir>/<prefix>N.wad` for N = 0..9 (the corrected exe-basename prefix), parses
the WWAD header + TOC, and opens entries by case-insensitive name with
later-mounts-shadow-earlier resolution (STD/loose-file branch + dormant-codec
stub included). Validated by `test_resource`: synthetic WWAD round trip always,
plus real-wad checks (counts 5631/214, payload tags, `.flh` via finding 21) when
`-Dtest_wads_dir=<dir>` is set:

```sh
meson setup build -Dtest_wads_dir=/path/to/shipped/game
meson test -C build --suite resource
```

**Texture loader** (`src/gods98/texture.*`, finding 13): decodes Windows BMPs —
BITMAPINFOHEADER (>= 40) and OS/2 core (12) headers, BI_RGB, 1/4/8-bit paletted
and 16/24/32-bit — loaded through the resource layer into ARGB8888 surfaces,
plus the engine's case-insensitive name-keyed get-or-load cache (finding 13 §2).
`texture_test` (suite `texture`) covers synthetic BMP round trips (all depths,
palette clamping, RLE/truncation rejection) and, with `test_wads_dir` set,
decodes five shipped entries and matches their pixel hashes (reference hashes
derived pixel-exact from the corpus via Pillow).

**Sound loader** (`src/gods98/sound.*`, finding 14 §1–2): order-independent
RIFF/WAVE chunk walk (`fmt` → `data`; `LIST`/`cue`/`smpl`/`acid`/`fact`
tolerated), PCM only, through the resource layer, plus the 600-slot sound bank
with the corpus's loop/stream flag bits. `sound_test` (suite `sound`) covers
synthetic WAVs (8/16-bit, stereo, odd-size padding, chunk-order variants,
junk rejection) and, with `test_wads_dir` set, parses **all 387 shipped
.wav files** and byte-matches five reference entries against `wave`-module-
derived PCM hashes.

**Input core + CD/STD fallback** (findings 04/05, 22, 01): `src/gods98/input.*`
models the corpus's three 256-byte key tables (`DAT_0076ab80/aa00/4de430`),
edge semantics (`cur && !prev`), `KeyToAscii`/`KeyContextCheck`, the
`EnginePreTick` text pump + mirror/clear, and the `QuitPoll` corner-dwell
machine. The `_DAT_004ffbc0` word behind the dwell slots was reclassified by
finding 22 as the **objective/advisor flag word** (not an input device), so
the dwell slot is caller-supplied here; the OS-level device *writer* rides
with the frame-loop milestone. The resource layer gained the CD/STD fallback
chain (`resource_set_cd_drive/root/data_dir`, `%c:\%s\%s`, `cd.key`/`delme.dat`
probes) per finding 01. `input_test` (suite `input`, no game data needed) and a
`test_cd_fallback` unit in `test_resource` cover both.

**Block-text config parser** (`src/gods98/block.*`, finding 12): the single
`Lego*` scanner every data file reads through — comment stripping (`;` to end
of line) with ` \t\r\n`-only tokenizing, nested `name { value }` records with
container value `"{"`, and rooted case-insensitive path lookup (`block_find`,
`"LegoRR Activities Activity_Teleport"` — the query side is the exe basename,
which is why data spells its roots `Lego*` with a prefix-wildcard that the
corpus gates to the first segment). Typed getters cover string/int/float/bool
with the corpus vocabulary `{TRUE,YES,ON}→1`, `{FALSE,NO,OFF}→0`, else/missing
`2`. `block_test` (suite `block`) runs synthetic grammar cases and, with
`test_wads_dir` set, parses **all 138 shipped block-text entries** (56 `.ae`,
44 `.ol`, 37 `.ptl`, `Lego.cfg`): 12,876 records digest-identical to an
independent token model, plus value spot checks (`Barracks.ae`
`Activity_Teleport = Create`, `IceMonster.ae` `HIGHPOLY` bone map,
`Lego.cfg Main TextureUsage = 3145728`).

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
tools/build-miniwin.sh                     # builds the real miniwin archive —
meson compile -C build-native              # meson's cmake bridge alone leaves
                                           # libminiwin.a empty (EXCLUDE_FROM_ALL)

# winapi reference build
meson setup build-winapi --cross-file cross/i686.ini -Dplatform=winapi
meson compile -C build-winapi
```

**Frame loop** (2026-10-21): the native build opens a 640×480 window, mounts
the shipped wads, renders real corpus textures (`ROCKROOF.BMP`,
`info_path.bmp`) through the texture loader onto a miniwin DirectDraw
surface, and flips at ~30 fps — the first visible proof of the port. Env
knobs: `LEGORRR_WAD_DIR`, `LEGORRR_AUTOSTOP_FRAMES` (headless/CI), and
`LEGORRR_SCREENSHOT=<path>` (PPM of the last frame). Run it from the game
directory (or set `LEGORRR_WAD_DIR`) — it exits cleanly on ESC/close.

**Interactive input** (2026-10-21): the window now captures keyboard/mouse
into the engine's `input.*` tables (the finding-05 device-writer half):
platform events pump into a canonical-key vocabulary, `input_capture_events`
fills `cur` (edges vs `prev` per findings 04/05), and the **arrow keys pan
the blitted texture**. Unit coverage for the capture/mirror/two-frame flow
lives in `input_test`.

**Audio playback** (2026-10-21): the merged sound bank now plays. The seam
gains a DirectSound-analog audio surface (`platform.h` `PlatformAudio`, one
shot = mono 16-bit PCM over SDL), and `src/gods98/audio.*` converts any bank
sound (8/16-bit, mono/stereo, any rate) and submits it. The frame loop plays
a startup SFX and re-triggers it with **Space**. `audio_test` (suite
`audio`, `SDL_AUDIODRIVER=dummy`) covers the conversion (incl. 8-bit and
stereo-mixdown) and a real `.wav` play.

**Audio depth** (2026-10-21): volume, looping, and 3D-ish panning.
`platform.h` gains gain (`SDL_SetAudioStreamGain`), a continuous **loop
track** (stereo PCM re-seeded on change + per-frame refill), and stereo
submits; `audio.*` adds `audio_set_volume` / `audio_play_sound_3d` /
`audio_loop_named` / `audio_update`. The demo loops `Sounds\Iambloop.wav`
spatialized by the texture's pan position (the blit is the sound emitter),
with **`[` `]`** adjusting volume.

**Engine simulation fills** (2026-10-21, first tranche): the EngineTick
camera/move cluster (`FUN_0046a730`, `FUN_00435a50`, `FUN_00431560`,
`FUN_0042c440`, `FUN_0042cb70`, `FUN_004261c0` — finding 03 §3) is ported
into `src/gods98/camera.*` with its observed structure over the `EngineState`
grid/move-list fields; cross-cutting helpers (message bus `FUN_00419b80`,
spawn `FUN_0040f190`/`FUN_00438960`, move `FUN_0046a700`) remain stub
boundaries with corpus addresses. `camera_test` (suite `camera`) covers the
timer-decrement, grid-expiry, and spawn-countdown logic.

**Tranche 2 — the view contexts** (2026-10-21): `src/gods98/view.*` ports
`ViewContextTick` (`0x435f5c`, finding 04) — the ×3-per-frame camera state
machine (modes 1 follow/edge-clamped, 2 scripted-lag, 3 free/cruise with the
glide-to-stop tail). Camera handles are record-backed (`cam_*`), the pure
vector math is implemented, and world/object boundaries (grid pick
`FUN_0044f6d0`, terrain `FUN_0044f940`, EMA blend `FUN_0044a070`) stay stubs.
`EngineTick` now drives the three views (`DAT_005560e4/e8/ec`); `view_test`
(suite `view`) covers the vector/glide/edge/blend paths.

**Tranche 3 — the message bus** (2026-10-21): `src/gods98/message.*` ports
`MessageDispatch` (`0x419b80`) + `MessagePreCheck` (`0x4199f0`, finding 04)
— the 40-id dedupe-with-live-callback notification bus, seeded with the
finding-22 `Info*` catalog, with the auto-cooldown rule + linked family reset
({10,11,12,38}), the exception-id dedupe skip, and the 40-slot active queue.
It replaces the camera module's `message_dispatch` stub (the grid-expiry now
fires into the real bus) and the air-beat tick path dispatches `0x11`/`0x12`.
`message_test` (suite `message`) covers gates, dedupe/exceptions, cooldown and
the cap.

**Tranche 4 — air-beat sound hook** (2026-10-21): `EngineAirBeat` drives the
looped `SND_AirBeat` through the audio seam when the air supply runs low
(`airbeat_set_loop`), and the finding-03 notifications `0x11`/`0x12` dispatch
through the bus; the demo drains air so you can hear the cooler-to-air-beat
change (`airbeat_test`, suite `airbeat`).

**Tranche 5 — `MainSimStep`** (2026-10-21): `src/gods98/mainsim.*` ports the
input/app-state step (`0x4250c0`, finding 04) — quit gate (keyed to the named
quit key + `EngineKeepRunning`), the debug/cheat toggles (Frame Rate Monitor
`0x40`, Memory `0x80`, Sound Effects `0x10`, Music) gated by the debug word
`0x10`, the air cheat, camera keys (per-view step boundary), and the o1/o2/o3
edge outputs. The corpus's raw per-key scan slots aren't pinned by the
findings, so named canonical key bindings are used (documented). `mainsim_test`
(suite `mainsim`) covers the toggles, outs, air cheat and quit gate.

**Tranche 6 — first subsystem tickers** (2026-10-21):
`src/gods98/simtick.*` ports a first batch of the remaining dispatch-table
tickers with fresh decompiles: the mute timer (`FUN_0041ba10`), the
effect-table dispatch (`FUN_0040aa10`), the bobbin list flag machine
(`FUN_004091e0`), the post-step cluster (`FUN_0040d450`), and the grid marker
sweep (`FUN_0044eff0`). Their ticker-table entries now delegate to the
module; helpers stay hook boundaries. `simtick_test` (suite `simtick`)
covers all five.

**Game layer — levels, objectives, savegames** (2026-10-21): the mission
plumbing that sits on top of the engine (findings 07/08/22) is now
reconstructed, on top of the merged block-text parser (`src/gods98/block.*`,
finding 12, #14). Three new modules in `src/game/`:

- **Level loading** (`src/game/level.*`, finding 07): `level_load` reads a
  level group's ~35 config keys from `Lego.cfg` into the 0x284-byte level
  block, then the 17-arg `MapLoadPipeline` port materialises the ten `.map`
  grids into the w×h×0x48 voxel array — TerrainCreate (grid + physical 0x28
  block table), the nine layer writers (erosion/cryo, surface, path/AI,
  emerge, predug, fallin flags, block pointers, layer L/M records) and the
  CellInitPass sweep, all in the corpus's positional key→loader order.
  `.map` files are read raw and cells accessed in place (`level_map_cell`,
  finding 07 §format).
- **Objective/advisor** (`src/game/objective.*`, finding 22): the
  `fun_00457d40` config loader (advisor-suppression keys → word bits
  0x10/0x20/0x80, crystal/ore/block/timer/construction targets at the
  corpus offsets), the checker/announcer/state-machine triplet with the
  full runtime bit flow (0x1 pending / 0x2 achieved / 0x4 failed /
  failed-with-crystals, 0x8 announced-once) and the seconds×25 timer rule.
- **Savegames** (`src/game/savegame.*`, finding 08): `Saves\\<slot>.sav`
  (0xB0 header + 5 × 0x3190 records) and `Saves\\<slot>.osf` (`'RO'` magic,
  count, 20-byte entries) with the slot context, mark/write/commit, the
  12,684-byte checkpoint blob (reward + 10-task table, Level03 tutorial
  byte), the `CheckpointStep` bootstrap-commit flow, and `OsfAdd`/snapshot/
  write.

`GameContext` (`src/game/game.hpp`) carries the corpus globals these
modules share (`DAT_005560c4` config, `DAT_005560d4` level block,
`DAT_00556ed0` cheat word, `DAT_004ffbc0` objective word, the save tables)
so tests run isolated. The frame loop now loads `LEGORRR_LEVEL` (default
`Level01`) from `Lego.cfg`, runs the objective tick per frame, and exposes
`S` (build blob + commit slot) and `L` (`CheckpointStep`). Three suites use
synthetic fixtures only:

```sh
meson test -C build --suite level
meson test -C build --suite objective
meson test -C build --suite savegame
```

Passing `-Dtest_wads_dir=<game>` adds real-wad probing to `level_test`,
mirroring the resource/texture/sound/block suites: it mounts the shipped
wads and reports how many Lego.cfg groups resolve a `SurfaceMap`.
Best-effort — the fixtures are the guaranteed coverage, and the probe is
skipped when no game data is present.

```sh
cd path/to/legally-owned/game
LEGORRR_AUTOSTOP_FRAMES=600 ../lrr-portable/build/src/LegoRR
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
