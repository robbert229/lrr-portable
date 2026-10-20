# 06 — The Command Queue & Selection Manager

Two structural discoveries this round, both from xref-mining the selection
cluster (`0x452xxx`) plus targeted REA decompiles. Pseudocode additions:
[`../decompiles/engine-region.c`](../decompiles/engine-region.c)
(`KeepRunning`, `SelectionRoot`, plus queue functions quoted below).

## The command queue (`FUN_004520d0` = `SelectionRoot` → renamed **CommandQueueTick**)

`EngineTick` calls `FUN_004520d0()` every frame (doc 03 step 4). Its true
nature: a **double-buffered deferred-command queue** — the engine's game-logic
command layer. This is the answer to "where does game logic live": most of it
arrives through this queue.

**Structure** (`0x4eaf20` base, entry stride `0x14` = 5 dwords):

| Field | Meaning |
| --- | --- |
| entry+`0x00` | opcode |
| entry+`0x04` | arg1 (entity ptr for entity opcodes) |
| entry+`0x08` | arg2 |
| entry+`0x0c/0x10` | arg3/arg4 (or 8-byte arg pair) |

- Two buffers of **2048 (`0x800`) entries**, chosen by `DAT_004fef28` (0/1);
  counts per buffer in the `DAT_004fef20` pair. Buffer base =
  `DAT_004eaf20 + buf*0x800*0x14` (bytes).
- **Producers** (callers of `FUN_00452060(op, a1, a2, &extra)`):
  cheat-key table below; the `FUN_0041e8xx` cluster — i.e. level/script logic;
  queue full ⇒ silent drop.
- **Cancel:** `FUN_00451f60(op)` removes all pending entries with `opcode == op`
  (swap-and-shrink).
- **Tick sequence (`SelectionRoot`):**
  1. If not time-frozen, fire cheat keybinds (below) — each pressed key enqueues.
  2. Pre-flight every pending entry through `FUN_0045d870` (entry remap, below).
  3. Swap buffers atomically (flip `DAT_004fef28`, zero new count) — commands
     enqueued *during* dispatch run next frame.
  4. Dispatch every entry with a giant opcode switch.
- **`FUN_0045d870`**: rewrites entry arg1 through a live mapping table
  (`DAT_00555be0`, stride 8 = from/to, count `DAT_00555d20`) — an entity-ID
  indirection layer (stable IDs → live pointers; likely populated on level
  transition so queued references survive).

**Opcode map (observed dispatch):**

| Opcode | Handler | Inferred |
| --- | --- | --- |
| 1 | `FUN_004526c0(ent, n)` | entity command (flags op) |
| 3 | `FUN_00452be0()` | global op |
| 4 | `FUN_00452c50(ent)` | entity op |
| 5,6 | `FUN_004035c0(ent, arg+1, n)` | position set/step |
| 8 | `FUN_00403270(&selection, n, args…)` | **add N entities to selection** |
| 9 | `FUN_00452d60(n)` | |
| 10 | `FUN_00425d50(selection[0])` | command to first selected |
| 0xb | `FUN_004295f0(1,0,0)` | audio |
| 0xc | `FUN_00464fa0(soundSlot, 0)` | **stop sound slot** |
| 0x13 | `FUN_00431780(a, b, lo16, hi16)` | camera/viewport split-word |
| 0x15/0x1b | `FUN_00439370/FUN_004394b0(arg,0,0,1)` | |
| 0x17/0x1d | `FUN_00402720(ent, 0)` | entity op ×2 |
| 0x19 | `FUN_00403610(ent, n)` | |
| 0x1f | `FUN_0043aea0()` | global op |
| 0x21 | `FUN_004470d0(ent)` | |
| 0x23 | `FUN_00452430(ent)` | |
| 0x25 | `FUN_00447210(ent, n)` | |
| 0x2b | `FUN_004036c0(ent, n)` | |
| 0x2d | `FUN_004528c0()` | selection filter pass |
| 0x34/0x36 | `FUN_00439340/FUN_00439280(args)` | |
| 0x40 | `FUN_00452cc0()` | global op |

**Cheat/debug keybinds:** key→command table at `DAT_004ff0bc` (key byte) +
`DAT_004ff0c8` (opcode+args, stride 5), count `DAT_004ff29c`, installed by
`FUN_00452010(key, op, a1, a2, extra)` — its only callers sit inside
`EnginePreFrame` (`0x421e8x`), i.e. registered each init frame while debug
mode is live. On keydown the mapped command enqueues. This is the same debug
key family as the monitors in `MainSimStep`, now wired to concrete commands.

## Selection manager (`DAT_004fef2c` array + `DAT_004ff298` count + `DAT_004ff298` gate)

Note: the count/gate is the same dword used as a boolean in
`SelectedEntity` — it's the **number of selected entities**, and `SelectedEntity`
just returns `selection[0]` when nonzero.

- `DAT_004fef2c` = selection array of entity pointers (`(*e & 0x3fffffff)` —
  pointer, count ≤ `0x3fffffff` masking odd).
- **`FUN_00452520`**: snapshot selection (`*count, *ents = malloc-copy`).
- **`FUN_004526e0(ent, mode, …)`**: add-to-selection. Sets `e+0xfa |= 0x200000`
  (selected flag), rejects duplicates (`FUN_00452610`), max 100 per add
  (`DAT_004ff298 < 100`), and for type-2 (`*e == 2`, the vehicle/hover class
  from doc 04) recenters relative to camera. Entity flags seen: `+0xf8`/`+0xf9`
  per-type bitfields, `+0xfa` state bits, `+0xdb` sub-object pointer
  (nonzero = deployed/in-use).
- **`FUN_004528c0`** (opcode `0x2d`): the **selection filter** — classifies the
  whole selection by dominant entity type (`2`=vehicle, `1`, `4`=rock?,
  `0xb`=?, with `FUN_00469e60(e)` flags `&0x20000`/`&0x2000000` and `e+0xfb & 0x40`)
  and posts the resulting context to the HUD via `FUN_0041b2d0(ctx, 0)` with
  ctx ∈ {9=vehicle?, 0xa, 0xb, 0xc, 0xd, 0xe} — the **HUD panel mode** for the
  command card. Then prunes deselected/stale entries and clears their `+0xfa`
  selected bit.

## `KeepRunning` (`0x435940`) — the "should the app continue?" pump

Also the last-resort quit probe (`QuitPoll` calls it): ticks open menus
(`FUN_00416d00` gate → `FUN_0045fda0`…`FUN_004601e0` menu updaters), picks a
**pending-transition result** (`FUN_0042f0c0` or `FUN_00416d70` after
`FUN_00416c80(mode)`), validates it with `FUN_00429890` (load/verify — returns
0 ⇒ quit), frees it, returns 1 to keep running. `FUN_00416d50/60/dd0` are
menu-state probes; `FUN_00416dd0` returns nonzero ⇒ *quit now*.
Net: quit paths (menu "Quit", focus-loss choice, corner gesture) all reduce to
`KeepRunning == 0` ⇒ `MainSimStep` returns 0 ⇒ loop exits.

## Ledger

| Claim | State |
| --- | --- |
| Double-buffered 2048×5-dword command queue, per-frame dispatch | observed |
| Buffer swap semantics (enqueue-during-dispatch ⇒ next frame) | observed |
| Entity-ID remap table before dispatch | observed (table purpose inferred) |
| Opcode→handler map table above | observed; handler semantics mostly inferred |
| Selection = pointer array + count, HUD ctx 9–0xE | observed |
| HUD ctx mapping per dominant type | inferred |
| Keybind table installed from `EnginePreFrame` | observed (xrefs 0x421e8x) |

## Next targets

1. The `FUN_0041e8xx` producer cluster (`EnginePreFrame` region) — what the
   cheat keys actually enqueue (op + args per key).
2. `FUN_004035c0/00402720/00403610/004036c0` — entity command primitives
   (position/anim/lifecycle), keys the entity struct.
3. `FUN_00429890` — the load/verify that gates `KeepRunning`.
4. `DAT_00555be0` mapping-table writers — when IDs remap.
