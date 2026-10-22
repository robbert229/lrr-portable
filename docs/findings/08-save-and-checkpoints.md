# Finding 08 — Save/Load and Checkpoints

*Anchors: `SavReadSlot @ 0x417a90`, `SavWriteSlot @ 0x417bd0`, `SavCommit @ 0x417fc0`,
`SavMarkRecord @ 0x417eb0`, `SavWriteRecord @ 0x417f40`, `SavBuildCtx @ 0x417e70`,
`CheckpointStep @ 0x462270`, `CheckpointBlobBuild @ 0x45fda0`,
`OsfAdd @ 0x459190`, `OsfWrite @ 0x4593e0`, `OsfRead @ 0x459470`,
`SaveMenuBuild @ 0x415a10`.
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

### `.osf` load-side restore (closed — the sidecar comes back on level load)

The write side is not the whole story: **`LevelLoad @ 0x429890` re-reads the
active slot's `.osf` and reinstalls the completed-objective table**, gated on
the in-memory table being clean and a slot being active. The reader is
`FUN_00459470` (`OsfRead`), the *only* consumer of the `'RO'` file format
aside from `OsfWrite` — it is built on the same `FUN_0047f6e0`/`FUN_0047f980`
(fopen/fread) primitives the `.sav` reader uses, which is why the earlier
write-only description missed it.

The gate, from `LevelLoad @ 0x429890` (FUN_0048d9e0 `"%s\\%i.osf"` at
`0x4a2788`):

```
FUN_004593d0();   // OsfReadGate: returns DAT_004ffe64 (the dirty flag)
if (dirty == 0 && FUN_00417e90() != -1) {   // clean table AND active slot
    OsfRead("Saves\<slot>.osf");
}
```

`FUN_00459470(path)` — full reader, mirrors the port's `osf_read`:

| Step | Corpus behavior | Evidence |
| --- | --- | --- |
| 0 | clear live table: count/capacity = 0, free `DAT_004ffe50` | `OsfClear @ 0x4592a0` |
| 1 | `fopen(path, "rb")`; fail → return 0, table stays cleared | `FUN_0047f6e0` |
| 2 | file size via seek-end (`FUN_0047fca0`) | `0x459470` body |
| 3 | read 8-byte header: u32 magic + u32 count | `FUN_0047f980(&local_8,4,2,fp)` |
| 4 | **magic must be `0x534F5252`** (`'RO'` LE) | `local_8 == 0x534f5252` |
| 5 | **size must equal `8 + count*0x14` exactly** (any truncated/padded/count-mismatched file rejected) | `iVar2 + -8 == local_4 * 0x14` |
| 6 | alloc `count*0x14`, read entries into live `DAT_004ffe50`, set count/capacity `DAT_004ffe54/58` | `OsfRead` body |
| 7 | snapshot live → shadow (`DAT_004ffe5c`, count `DAT_004ffe60`), then `DAT_004ffe64 = 0` | `OsfSnapshot @ 0x4592e0` |
| 8 | `fclose`; return 1 **whenever the file opened** (valid or not) — the *installed state* (`DAT_004ffe54 == 0` on a bad sidecar) is what signals success | `uVar3 = 1` |

**Failure handling**: a missing `.osf` returns 0 and leaves the cleared table;
a corrupt file (bad magic, size-count mismatch, short read) *also* returns 1
(the file existed) but leaves the table empty — completed objectives degrade
to none, no crash, no partial install. `DAT_004ffe5c` (the shadow) is only
touched by the success-path snapshot, so a failed read cannot leave a stale
shadow behind the empty live table.

**What the restored table drives**: `FUN_00459360` (copy `DAT_004ffe5c →
`DAT_004ffe50`, called from `FrontEndStartLevel @ 0x415220` and
`ObjectiveTick @ 0x458be0`) rolls the live table back to that snapshot —
the "repeatable mission" restart point — and `OsfWrite` serialises the live
table on the next commit. So the restore reinstalls both the re-serialisable
completed list **and** the level-restart snapshot in one read.

**Port**: `osf_read`/`osf_read_path` in `savegame.cpp` reproduce steps 0–8
exactly (fixed-array `GAME_OSF_MAX` cap in place of the corpus's realloc);
the frameloop slot-load calls `osf_read_path` after `checkpoint_blob_apply`
under the same clean-table/active-slot gate. The savegame suite proves the
strict round trip: write slot → reload → live and shadow contents identical,
both dirty flags clear, and the corrupt/absent sidecar cases stay empty.

## In-memory checkpoint machinery

| Global | Meaning |
| --- | --- |
| `DAT_005574d0` | active slot (−1 = none; `CheckpointStep` defaults to **slot 5**) |
| ctx `+0x18` / `+0x34` | record count / record array pointer (per slot) |
| `DAT_00552990` | live task-table block pointer (written only by `TaskBlockInit`; source of the blob's task table) |
| `DAT_00552994` | checkpoint-blob pointer (12,688 B); rebuilt *and freed inside every `KeepRunning` tick pass `0x435940` while a game is loaded; read only within that pass (dangling between rebuilds — never dereferenced outside it). Getter `FUN_004601f0`. |

### CheckpointBlobBuild (`0x45fda0`, called from `0x435940` @ `0x435964`)

`0x435940` is the per-frame checkpoint tick (registered callback, entered from
`0x4589c0`/`0x4250c0`). While a game is loaded (`FUN_00416d00` true) it, per frame:
allocates a fresh 0x318c block via `FUN_0048dbd0`, zero-fills it (`0xc63` dwords),
stores the pointer in `DAT_00552994`, then fills:

| Blob off | Source | Notes |
| --- | ---| --- |
| +0x00 | config `Reward/Enable` | tri-state, `2→0` |
| +0x1C0 | config `Reward/Modifier` | handle |
| +0x1CE4 | constant `10` | task count |
| +0x1CE8 | self-ref → blob+0x1CEC | record-array pointer |
| +0x1CEC | tasks ×10, each 0x210 B | copied dword-verbatim from the live task block (`[DAT_00552990]+0x1CE8`), then `FUN_004600a0`/`FUN_00460140` (importance/quota refreshers) re-run over the blob records |
| (globals) | `Reward/Timer` (default **76.9375**), `DisplayText/Images/Flics` | kept in `DAT_0055298c`, `DAT_00555bc4/c8/cc` |

After the copy `0x435940` calls `CheckpointProgressClamp @ 0x460360` (clamps every blob
record's float at +0x38 into `0.0..100.0` — see §Task table block below), then `0x461410`
(the reward display handlers below), and finally `CheckpointBlobFree @ 0x4601e0`
(`FUN_0048db80`), all inside the same tick — the blob is per-frame scratch, not
persistent state.

Level-name match against `"Levels::Level03"` (`FUN_0048ff10`, case-folded compare) forces
the **dword** at blob record base `+0x868` = 1 (i.e. task slot 4 "Caverns" quota,
`4*0x210 + 0x28`) — the tutorial-fix hard-code at `0x460066-0x46008c`.

### Flow

**Capture** — every frame the checkpoint tick (`0x435940`, entered from `0x4589c0` /
`0x4250c0` while a game is loaded) rebuilds and frees the blob in place;
**Save** — on reward-zone events (`RewardZoneCheck @ 0x461dd0`, `FUN_00462780 @ 0x4627f1`)
`CheckpointStep` maps level state → list index (`ListFindName`), marks the record dirty when
state+0x27c == 1, copies blob→record (`SavWriteRecord`), commits (`SavCommit` →
`.sav` + `.osf`), and reloads slot 5 metadata. If no slot was active it bootstraps slot 5
with a zero-filled record array first.
**Load** — `SavTreeLoad(root, records)` walks the config-built level tree
(`DAT_00556ff8`; roots `DAT_00556fe4/fd0` = menu/game list pairs), consulting each record's
dirty flag; restore then `FUN_00418110(0)`. The `.osf` sidecar is restored
separately, inside `LevelLoad` (see §".osf load-side restore" above).

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

## Task table block — full field layout (closed)

The live block is 0x318c bytes: a 0x1CEC header plus ten task records at
block `+0x1CEC`, stride `0x210` (exactly `10 × 0x210 = 0x14A0`).
Header fields: `+0x1CE4` = record count (`10`), `+0x1CE8` = self-ref pointing
at the first record (`block + 0x1CEC`). Only ten functions ever deref the
record base (`e81c0000` byte search — the complete consumer set):
`0x45f030/0x45f750/0x45fda0/0x460360/0x461410/0x461790/0x461c90/0x462390/
0x4624a0/0x462600`; every record byte range below was attributed against
their disassembly (all remaining ranges are provably unreferenced).

Record layout (offsets relative to record base; the blob copy is
dword-verbatim identical):

| Off | Field | Evidence |
| --- | --- | --- |
| +0x00 | u32 flags: `1` drawn-as-achieved/text gate, `2` Images entry, `4` Flics entry, `0x10` text overlay, `0x20` boxed text, `0x40` BoxImages entry | loaders OR bits (FUN_0045f3e0 `2`, FUN_0045f5f0 `4`, FUN_0045f530 `0x31`, FUN_0045f290 `0x40`); `RewardDisplayTick` tests `flag & bit` paired with the `DAT_00555bc4/c8/cc` config gates |
| +0x04 | name string (≤0x20 bytes: the 10 names from the `TaskBlockInit` name table `0x4a8a08…0x4a931c`) | FUN_0048d9e0 `"%s"` per slot |
| +0x24 | float importance (`Reward Importance.<name>`) | FUN_004600a0 |
| +0x28 | int32 quota (`Reward Quota.<name>`); Level03 blob forces slot-4 quota = 1 | FUN_00460140; blob tail `+0x868` |
| +0x2C | *(unreferenced — reconstruction-side value slot only)* | no consumer in the 10 functions |
| +0x30 | float progress, recomputed from quota each frame on the blob side (`FILD` of `+0x28`, scaled) | `0x460360` blob pass |
| +0x34 | dword scratch (written alongside `+0x30`) | `0x460360` |
| +0x38 | float progress-clamp target, forced into `0.0..100.0` every frame on all 10 blob records | `0x46089b-0x4608e5` loop (`EDX 0..0x14A0` step 0x210) |
| +0x3C..+0x50 | *(unreferenced)* | — |
| +0x54 | display text, `Reward Text.<name>` field 0 with `_`→' ' replaced over the first 0x20 bytes (`FUN_00413f00`); key literal at `DAT_004a26b0` = `"Text"` | FUN_0045f530 (`0x45f58c: LEA EAX,[ESI+0x54]`) |
| +0x5C..+0xD0 | *(unreferenced)* | — |
| +0xD4 | display-string staging read by `RewardDisplayTick` (`param_1 + 0x35` as draw string) — **no writer in the shipped corpus**, so it stays zero (empty) at runtime; text drawing uses +0x54 | display tick, engine-region `6086/6106` |
| +0xD8..+0x150 | *(unreferenced)* | — |
| +0x154/+0x158 | text draw x/y ints (`Reward Text.<name>` fields 1/2) | FUN_0045f530; drawn at `FUN_0047a180` |
| +0x15C/+0x160 | `Images.<name>` x/y | FUN_0045f3e0; blit `FUN_0047dcb0` |
| +0x164/+0x168 | `BoxImages.<name>` x/y | FUN_0045f290 |
| +0x16C | flic path (`Flics.<name>` field 0) | FUN_0045f5f0 |
| +0x170..+0x1EB | *(unreferenced padding before the rect)* | — |
| +0x1EC..+0x1F8 | flic rect, 4 ints (`Flics.<name>` fields 1..4) | drawn via `FUN_00484070` |
| +0x1FC | flic handle — lazily opened `FUN_00483c80`, released `FUN_00483f00`, zeroed by `0x461410` | |
| +0x200 | image surface (`FUN_0047d9d0` of `Images.<name>` path; freed `FUN_0047d470` by `TaskBlockFinish`) | read as `param_1[0x80]` |
| +0x204 | font handle (`FUN_00479ee0` of `Fonts.<name>`, default `DAT_00555b40`; freed `FUN_0047a550`) | read as `param_1[0x81]` |
| +0x208 | sound token (`Sounds.<name>`; consumed via `FUN_00464c70` in `0x4624a0`) | |
| +0x20C | boximage surface (blitted at `FUN_0047dcb0`) | read as `param_1[0x83]` |
| +0x20D..+0x210 | *(end padding)* | — |

`!`-prefixed `Images`/`Flics` config keys are additionally gated by
`DAT_00505f84` bits `0x1000000`/`0x2000000` (shipped-asset checks).

### Ownership / lifetime (closes the `DAT_00552990` question)

- **Created**: `TaskBlockInit @ 0x45f030` allocates the block via `FUN_0048dbd0`
  (12,688-byte factory alloc), zero-fills 0xc63 dwords, stores the pointer in
  `DAT_00552990` (the *only* writer), then runs the name pass + six loaders.
  It is called once per **game session** from the run-start callback
  `FUN_0041fb50` at `0x422651` (right after `RewardInit @ 0x422649`), registered
  through `FUN_0041fa20` → `FUN_00477fd0` (start/tick/finish slots
  `DAT_0050592c/30/34`, driven by `FUN_004777a0`).
- **Destroyed**: `TaskBlockFinish @ 0x45f750` flushes each record's image /
  font handles (`FUN_0047d470`, `FUN_0047a550`, `FUN_00483f00`) and frees the
  block via `FUN_0048db80`; called from the run-finish callback `FUN_00424d00`
  at `0x424d07`.
- **Not** owned by `LevelLoad @ 0x429890` (its three call sites touch nothing
  here) and not per level — the block survives level changes inside one
  session; per-level variation is only the config-driven loader/refresh pass.
- `DAT_00552994` (blob) is distinct, per-frame scratch (see
  §CheckpointBlobBuild above).

## Still open

- Record flag `|2` semantics (`SavMarkRecord` sets `1` (and `2` when name is empty) — likely "present/empty").
- `.dat` slot thumbnails: writer closed (see section above); the save-menu
  draw side (`DAT_004db890` blits, SaveImage `Width/Height/BigWidth/BigHeight/
  Pos1..5` rects + the scroll-plane scale) is not ported yet.
