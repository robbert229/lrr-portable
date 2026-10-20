# Rock Raider Reversed

Reverse engineering of **Lego Rock Raiders**, a real-time strategy game published by
LEGO Media International in 1999 for Windows 95/98.

This repository is a research workspace: the goal is to understand how the game works —
its executable, data formats, and engine behavior — from the shipped binary artifacts.

## The game

Lego Rock Raiders cast the player as a team of LEGO minifigures mining a planet for
"power rocks." It shipped in 1999, is a 32-bit Windows binary using DirectX, and has been
largely forgotten by official channels ever since. It is of interest here for preservation
and curiosity; nothing in this repository is affiliated with or endorsed by the LEGO Group.

## Repository layout

```
README.md           This file
docs/findings/      Reverse-engineering findings, 01–12 (see index below)
docs/decompiles/    Named Ghidra pseudocode corpus (engine-region.c)
docs/evidence/      Exported REA evidence bundle (address-level citations)
tools/              Analysis tooling: unwad.py, Ghidra function-naming script
source/             (gitignored) the original, unmodified game files
unpacked/           (gitignored) 5,845 assets extracted from the .wad archives
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
- [ ] Document asset formats: 3D models, animations, sounds, videos
      — partial: models/scenes (11) and `Lego*` block-text configs (12);
      sounds/videos/UV grammar open
- [x] Understand mission / level scripting and data-driven behavior — findings 07, 09
- [x] Understand the savegame format — finding 08
- [ ] Eventually: reimplementations or tooling that let the game run on modern systems

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

Named-pseudocode corpus: [`docs/decompiles/engine-region.c`](docs/decompiles/engine-region.c)
(114 identified functions, ~7.6k lines; all addresses in the findings resolve here).
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
