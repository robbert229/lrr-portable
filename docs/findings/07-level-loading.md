# Finding 07 — Level Loading and the `.map` File Format

*Anchors: `LevelLoad @ 0x429890`, `MapLoadPipeline @ 0x42b510`, `TerrainCreate @ 0x44e0c0`,
`MapFileReadHeader @ 0x4515a0`, `MapFileGetCell @ 0x4515c0`,
layer loaders `0x42bd20/0x42c480/0x42bf40/0x42c6a0/0x42c760/0x42c9d0/0x42cc90/0x42c120/0x42c060`,
`CellInitPass @ 0x42f6f0`. Pseudocode: `docs/decompiles/legacy/engine-region.c`.*

## TL;DR

A level is defined entirely as a **config-DB group** (`Lego.cfg`, one group per level
name) holding ~35 scalar parameters and *file paths to twelve `.map` grid files*.
`KeepRunning` decides a load is pending (`PendingLevelName @ 0x42f0c0`), calls `LevelLoad`,
and `MapLoadPipeline` materializes every grid into a **2-D voxel array of 0x48-byte records**.
`.map` is read **raw into RAM and accessed in place** — the file layout *is* the memory layout
(magic `MAP `, length, u32 w/h, u16 cells row-major). No code anywhere references a `.blx`
file; the per-level `.blx` files in the WAD are constant-per-kind, editor-era leftovers.

## Call chain

```
KeepRunning (0x435940)                       doc 06: sole quit/load decider
  └─ PendingLevelName (0x42f0c0)             resolves "which level next" from config
  └─ LevelLoad (0x429890, ~786 lines)        cleanup cluster → read config keys → file check → pipeline
       ├─ FUN_00477440(value, parts, delimiter)  splits comma-delimited level config tuples
       └─ MapLoadPipeline (0x42b510)            17 params = state + 16 map-name/filehandle args
            ├─ TerrainCreate (0x44e0c0)      builds terrain from the SurfaceMap grid
            ├─ LoadPredugLayer (0x42bd20)    predug flags → voxel +8/+C bits
            ├─ LoadErodeCryoreLayers (0x42c480) erosion/cryo byte → voxel +2
            ├─ LoadEmergeLayer (0x42bf40)    emerge value>>1 → voxel +6
            ├─ LoadPathAiLayers (0x42c6a0)   path/AI byte − bias → voxel +5
            ├─ LoadFallinLayer (0x42c760)    fallin flags → voxel +8
            ├─ LoadBlockPointers (0x42c9d0)  pointer count → voxel +3C/+38
            ├─ LoadTerrainMap (0x42cc90)     surface type byte − bias → voxel +4
            ├─ LoadLayerL (0x42c120)         +0xC flags + appends records (stride 0x4C) into state+8 array
            ├─ LoadLayerM (0x42c060)         byte → voxel +34
            └─ per-cell sweep CellInitPass (0x42f6f0)  for all (x,y)
```

All of the above pseudocode is now in `docs/decompiles/legacy/engine-region.c`.

## What a level is (config side)

`LevelLoad` reads the level group's keys from the config DB (`DAT_005560c4`, backed by
`Lego.cfg` in `LegoRR1.wad`) via `FUN_00478f50` / `FUN_004790b0` / `FUN_00479050` into the
level state block `DAT_005560d4` (0x284 bytes). Observed keys: `blocksize`, `digdepth`,
`roofheight`, `roughlevel`, `StartFP`, `NoDrain`, `OxygenRate`, `UpgradeTime`,
`MinifigureUpgradeTime`, `EmergeCreature`, `SafeCaverns`, `IntroAVI`, `PTLFile`, `NERPFile`,
`NERPMessageFile`, `olistfile`, `elistfile`, `textureset`, `selboxheight`, and the twelve map
keys: `SurfaceMap`, `PredugMap`, `TerrainMap`, `BlockPointersMap`, `CryOreMap`, `ErodeMap`,
`EmergeMap`, `AIMap`, `PathMap`, `FallinMap` (values are full resource paths ending `.map`).

Grid dimensions are **not** in the config — they come from the first map file's header and
every other layer file must match (`+state 0x14/0x18` = w/h; each loader bails on mismatch).

Note the file-prefix inversion in the shipped config: `SurfaceMap → High_<lvl>.map`
(height/visual surface grid, values 0–13) and `TerrainMap → Surf_<lvl>.map`
(material types, values 1–11). So "surface" is the height layer and "terrain" the material
layer.

## `.map` file format (confirmed, direct-mapped)

Empirically parsed from 240 files, then *confirmed by code*: `MapOpen`
(`FUN_004800a0 → FUN_004800c0 @ 0x4800c0`) calls `ResourceOpen(name,"rb")`, mallocs the file
size, reads it whole — **no parsing, no magic check** — so accessors index the file image
directly:

| Offset | Field | Code |
| --- | --- | --- |
| 0x00 | `MAP ` magic (4B) | unchecked in code |
| 0x04 | u32 body length | |
| 0x08 | u32 width | `MapFileReadHeader (0x4515a0)` reads image+8 |
| 0x0C | u32 height | reads image+0xC |
| 0x10 | u16 cells, row-major `y*w+x` | `MapFileGetCell (0x4515c0)` reads `+0x10 + (y*w+x)*2` |

A second, container-handle flavor is used by the layer loaders:
`MapOpen2 (0x480170)` allocates via `FUN_00489410`, reads with `FUN_004894a0`, and
releases the allocated resource slot through `MapFileResourceSlotRelease @ 0x489460`;
it uses the same image layout;
`FUN_004334c0(h,&w,&h)` for dims and `FUN_004319e0(h,x,y)` → u16 cell (same `+0x10 + idx*2`).

Cell semantics per layer (value distributions from all 240 files):
`Surf` 1–11 materials · `High` 0–13 heights · `Dugg` predug flags 0–4 ·
`Cror/Cryo` ore cells · `Erode` 0–10 · `Fall`/`Emerge`/`Path`/`AI` sparse 0–n.

## Voxel record (stride 0x48)

`MapLoadPipeline` allocates `w*h*0x48` records at `state+0xB8` and initializes a
`+0x20` u16 per-cell seed (`thunk_FUN_0048e170`). Loader-written fields:

| Off | Written by | Meaning |
| --- | --- | --- |
| +0x02 | LoadErodeCryoreLayers | erosion / cryo char |
| +0x04 | LoadTerrainMap | surface material − bias |
| +0x05 | LoadPathAiLayers | path/AI − bias |
| +0x06 | LoadEmergeLayer | emerge value >> 1 |
| +0x08 | LoadPredugLayer / LoadFallinLayer | flag bits (`0x4000000` predug edge wall, `0x20000`, `0x20000000` seen in `CellInitPass`, and `0x10` traversable on the eight neighbours of `v == 1/3` edge-wall cells through the `FUN_00430530` cell update — the load-time source of walkability, found by the T3 real-data probe) |
| +0x0C | LoadPredugLayer (`0x20` immediate / `0x200` deferred coordinate-pool entry; finding 224) / LoadLayerL | extra flags |
| +0x34 | LoadLayerM | byte layer |
| +0x38/0x3C | LoadBlockPointers | count/pointer (>4 sets count−4 + flag) |
| +0x40 | LoadBlockPointers | float `hash(x) % (ptrs * DAT_00556f14 * 25)` |

TerrainCreate also builds a parallel physical block table (per cell stride 0x28):
+0x19 is an attribute byte initialized to 0x60; +0x1A is the map-derived height byte;
+0x1C..+0x20 hold the float normal pair, and +0x24 is a flag. Finding 114 documents the
diagonal selector, height consumer, and orientation/normal fix-ups.

## The `.blx` question — resolved as *unused*

- 220 `.blx` files exist (`BLK ` + u32 kind + 16×16 + payload sized by kind:
  1560/2584/4632/5656/6168/8728/10776/26136/31256 bytes for kinds 3/5/9/11/12/17/21/51/61).
- **Payload is byte-identical across all files of a kind** (47 files of kind 3 → 2 distinct
  payloads; every other kind → 1). Real per-level data cannot be constant.
- Zero references: no `"blx"` byte sequence anywhere in `LegoRR.exe`, none in `Lego.cfg`,
  and no layer loader ever opens anything but the config-supplied `.map` names.
- The dev-era `PAUL.LEV` in the WAD pairs each `.MAP` with a `.BLK`
  (`TERRAIN.MAP…TERRAIN.BLK`, `PREDUG.MAP…PREDUG.BLK`), confirming the pairing is an artifact
  of the *level editor* toolchain, not the shipped runtime.

Conclusion: `.map` is the live format; `.blx`/`.BLK` are editor chunk files ignored by the
game.

## Still open

- Voxel-flag meanings partially closed by the move-branch decompile
  (finding 10 §tile queries): `+8` bit `0x10` = traversable layer, bit
  `0x40` = blocks walking, bit `0x8000000` = entity reservation
  (`FUN_00432e00`); `0x4000000` (Predug edge-wall) is written by the port
  loader, and its intended reader — the wave-flood pathfinder — is now
  ported (finding 10 §Pathfinder builder). Closed 2026-10-22 (issue #72):
  the *writer* of `0x10` during loading is the predug loader running the
  cell update `FUN_00430530` (flag side, `predug_wall_cell` in
  `level.cpp`) on `v == 1/3` edge-wall cells; REA on the shipped
  `LegoRR.exe` (`FUN_0042bd20` LoadPredugLayer → `FUN_00430530`, whose
  else-branch marks the eight non-cave neighbours `| 0x10 & ~0x400000`)
  and the full-map cell-init pass (`FUN_0042f6f0`, which only clears and
  re-derives `0x8`/`0x400000`/`0x20000000`) back it up. Without that call
  a shipped level has no walkable cells at all — the T3 real-data probe
  found the gap. Remaining:
  `0x20`, `0x200`. The `0x20000000` neighbor-mask path, including its
  conditional out-of-grid `0x400` read at top/left edges, is documented in
  finding 223.
- DAT_00556f14 is loaded from FallinMultiplier and zero-normalized to 1 (finding 117); the gameplay rationale remains unknown.
- .ol/.nrn resources (olistfile, NERPFile) and the NERP interpreter's direct load path remain open. The PTLFile consumer and pair-table construction are documented in finding 115.
