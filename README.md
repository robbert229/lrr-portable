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
source/             (gitignored) the original, unmodified game files
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

- [ ] Map the high-level architecture of `LegoRR.exe` (engine, game logic, rendering)
- [ ] Document the `.wad` archive container format(s)
- [ ] Document asset formats: 3D models, animations, sounds, videos
- [ ] Understand mission / level scripting and data-driven behavior
- [ ] Understand the savegame format
- [ ] Eventually: reimplementations or tooling that let the game run on modern systems

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
