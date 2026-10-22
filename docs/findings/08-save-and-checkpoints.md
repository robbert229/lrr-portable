# Finding 08 — Save/Load and Checkpoints

*Anchors: `SavReadSlot @ 0x417a90`, `SavWriteSlot @ 0x417bd0`, `SavCommit @ 0x417fc0`,
`SavMarkRecord @ 0x417eb0`, `SavWriteRecord @ 0x417f40`, `SavBuildCtx @ 0x417e70`,
`CheckpointStep @ 0x462270`, `CheckpointBlobBuild @ 0x45fda0`,
`OsfAdd @ 0x459190`, `OsfWrite @ 0x4593e0`, `SaveMenuBuild @ 0x415a10`.
Pseudocode: `docs/decompiles/engine-region.c`.*

## TL;DR

Persistence is **not** an entity-state dump. A save slot stores one fixed-size
**12,688-byte checkpoint record per level** plus a sidecar of completed objectives:

- `Saves\<slot>.sav` — header (0xB0) + records × `0x3190`, slots 0–5 (slot 5 is the default)
- `Saves\<slot>.osf` — completed-objective table: `'RO'`, u32 count, count × 20-byte entries

The in-game capture unit is the **checkpoint blob**: a 12,684-byte snapshot built every
frame from the config DB's `Reward` group and the live task table, stored verbatim into the
current level's record.

## File layouts

### `.sav` (`FUN_0048d9e0("%s\\%i.sav", "Saves", slot)` at 0x417a90/0x417bd0)

| Offset | Bytes | Field | Evidence |
| --- | --- | --- | --- |
| 0x00 | 0xB0 | header blob | `SavReadSlot` reads 0xB0 first |
| 0x18 | 4 | record count | `puVar4[6]`, compared/used everywhere |
| 0x34 | 4 | (loaded as RAM pointer to record array) | |
| 0xB0 | `0x3190` × N | records | read after header |

Record = `u32 flags` (`|1` dirty, `|2` also-set by `SavMarkRecord`) + `0x318c`-byte payload
(= the checkpoint blob, see below).

### `.osf` (`OsfWrite @ 0x4593e0`)

| Offset | Bytes | Field |
| --- | --- | --- |
| 0x00 | 4 | magic `RO` (`0x534F5252` little-endian → `"ROS"` u32: bytes `RO`, then count) |
| 0x04 | 4 | count (`DAT_004ffe54`) |
| 0x08 | 20 × count | entries: `id`, `ref`, `name[11]`, `u16` ×3 pad (`OsfAdd`: entity+0xFC, entity+0xCC, 11-char name) |

A completed objective entity (`type==2`, flag `&8`) is appended by the entity-death handler
(`0x43bd4b → OsfAdd`). `OsfSnapshot` clones the table to a shadow copy at `DAT_004ffe5c`;
the `DAT_004ffe64` "dirty" flag is set on snapshot and cleared on write.

## In-memory checkpoint machinery

| Global | Meaning |
| --- | --- |
| `DAT_005574d0` | active slot (−1 = none; `CheckpointStep` defaults to **slot 5**) |
| ctx `+0x18` / `+0x34` | record count / record array pointer (per slot) |
| `DAT_00552990+0x1CE8` | live per-task block base (source of the blob's task table) |
| `DAT_00552994` | the blob itself (12,688 B, rebuilt each `KeepRunning` tick) |

### CheckpointBlobBuild (`0x45fda0`, called from `KeepRunning @ 0x435964`)

Zeroes a 0x318c-byte block, then fills:

| Blob off | Source | Notes |
| --- | ---| --- |
| +0x00 | config `Reward/Enable` | tri-state, `2→0` |
| +0x1C0 | config `Reward/Modifier` | handle |
| +0x1CEC | tasks ×10, each 0x210 B | copied from live task block (`DAT_00552990+0x1CE8`), then normalized by `FUN_004600a0`/`FUN_00460140` |
| (globals) | `Reward/Timer` (default **76.9375**), `DisplayText/Images/Flics` | kept in `DAT_0055298c`, `DAT_00555bc4/c8/cc` |

Level name `Levels\Level03` forces blob byte +0x868 = 1 (hard-coded tutorial fix).

### Flow

**Capture** — every tick `KeepRunning` calls `CheckpointBlobBuild`;
**Save** — on reward-zone events (`RewardZoneCheck @ 0x461dd0`, `FUN_00462780 @ 0x4627f1`)
`CheckpointStep` maps level state → list index (`ListFindName`), marks the record dirty when
state+0x27c == 1, copies blob→record (`SavWriteRecord`), commits (`SavCommit` →
`.sav` + `.osf`), and reloads slot 5 metadata. If no slot was active it bootstraps slot 5
with a zero-filled record array first.
**Load** — `SavTreeLoad(root, records)` walks the config-built level tree
(`DAT_00556ff8`; roots `DAT_00556fe4/fd0` = menu/game list pairs), consulting each record's
dirty flag; restore then `FUN_00418110(0)`.

**Record ↔ level** — the index is *per level*, not global: `CheckpointStep` takes it from
`ListFindName(ActiveObjectList(levelState))` (the current level's position in the active
sav list), and `SavTreeWalk2` reads the record slot from the tree node itself
(`*node * 0x3190 + records`), so each level checkpoints to its own record and the array
widens past the 5-record bootstrap as levels enumerate. The port implements this as
`fe_record_for_level`/`fe_level_for_record` (port decision: tutorials enumerate records
`0..tutorial_n-1`, then the campaign chain — one progression-order stand-in for the
corpus's tree-node ordering), `current_record` is set on every level start so the
reward-zone autosave and in-game `S` land in the right record, `save_write_record`
grows the array on demand, and load-slot resume maps the highest completed record
back through the same table (`resume = max dirty index` remains a port stand-in for
the tree walk).

### Menu layer

`SaveMenuBuild @ 0x415a10` builds the save/load lists from config keys `Save_Game_%i` /
`Load_Game_%i`, positions entries by `FrontEndX/Y/Open` and `MenuBMP` config keys
(`SavObjSerialize @ 0x4177e0` — confusingly named; it is the *menu-item* callback, not a
serializer). Preview scores come from scanning each record's float at blob +0x1BC38
(`FUN_00413a60`), and slot thumbnails are `.dat` files (`%s\\%d.dat`).

## Negative result: no entity dump

Nothing in the save path serializes vehicles, buildings, or terrain deltas —
12,684 B per level record cannot hold them, and no code writes entity state into any
`fopen(...,"wb")` target. The persistence model is: **per-level reward/task checkpoint +
completed-objective list**, matching the game's "mission checkpoint" design where gameplay
state resets when the checkpoint is applied.

## Slot thumbnails — the `.dat` framebuffer dumps (`FUN_00413b80` @ 0x413b80)

**The write side is closed.** The `.dat` writer is the save-menu confirm path:
`FUN_00413b80` is the modal save-menu loop (entered via `FUN_00413e20` from
`FUN_00416c80`, the slot-save action — driven from `FUN_00435940` /
`FUN_00462030` and boot). It opens by refreshing the slot tables with
`FUN_00413a60` — per record: `FUN_0048d9e0("%s\\%d.dat", <SaveImage path>,
slot)` -> `FUN_0047d9d0` (the BMP surface loader) into
`DAT_004db890[0..7]`, plus the `+0x1BC38` float-scan completion score — and
on exit, if the selection is in the save list (`DAT_00556fcc`), the slot
`DAT_004a1f5c >= 0`, and a live game is loaded (`FUN_00460200`'s object with
`+4 != 0`), it calls `FUN_0047e440(path, game+0x10-surface)` to dump the
framebuffer once.

`FUN_0047e440 -> FUN_0047c8f0` is a plain BMP writer — the `.dat` is just an
uncompressed 24bpp BMP with a made-up extension, field for field: `BM`, file
size = pixels + 54, data offset 54, DIB header 40, positive width/height
(**bottom-up** rows), planes 1, bpp 24 (`0x18`), comp 0 (BI_RGB), image size
0, XPel/YPel 1, palette counts 0; surface pixels are component-shifted to
BGR and copied dword-padded, each surface row `y` landing at file offset
`(h − 1 − y) × row_bytes`.

The loaded thumbnails are *used* in the item draw path (xref `0x412518`):
destination rects come from the `Menu::SaveImage` config keys (`Path Saves`,
`Width/Height 60×45` normal, `BigWidth/BigHeight 80×60` selected, `Pos1..5`
at `40|95 … 40|335`) scaled by the scroll-plane constants `0x49e190/194`,
blitted through `FUN_0047dcb0`.

**Port**: `save_write_thumbnail(ctx, slot, fb, pitch, w, h)` in
`savegame.cpp` reproduces `FUN_0047c8f0`'s layout byte-for-byte (the
savegame suite round-trips it through `texture_decode`). `LegorrrRun` dumps
`Saves\<slot>.dat` for the frame it presents when the reward screen's Save
click commits the checkpoint (`game.save_slot`, defaulting to 5) — the port
deviation is that it has no save-to-slot menu confirm, so the dump hangs off
the commit instead. The read/draw side (rendering `Saves/<N>.dat` behind the
save-menu rows with the SaveImage rects) is not implemented; see Still open.

## Still open

- Task-table block (`DAT_00552990+0x1CE8`, 0x210/task) field layout.
- Record flag `|2` semantics (`SavMarkRecord` sets `1` (and `2` when name is empty) — likely "present/empty").
- `FUN_0047fa50`-based `.osf` restore path (load side only shows the `'RO'` write).
- Who owns `DAT_00552990` lifetime (created in `LevelLoad`? via `FUN_0048dbd0` — verify xref).
- `.dat` slot thumbnails: writer closed (see section above); the save-menu
  draw side (`DAT_004db890` blits, SaveImage `Width/Height/BigWidth/BigHeight/
  Pos1..5` rects + the scroll-plane scale) is not ported yet.
