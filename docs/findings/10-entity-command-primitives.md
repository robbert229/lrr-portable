# Finding 10 — Entity Command Primitives & the Opcode Map

*Anchors: `CmdAlloc @ 0x406440`, `CmdFree @ 0x406480`, `CmdPoolGrow @ 0x4064c0`,
`CmdRegistryInit @ 0x401cf0`, `CmdKindByName @ 0x401cb0`, `CmdCreateMove @ 0x4035c0`,
`CmdCreateTarget @ 0x4036c0`, `CmdCreateTask @ 0x402720`, `CmdCreateGated @ 0x403610`,
`CmdLinkHead @ 0x404030`, `CmdLinkTail @ 0x404070`, `CmdTickGlobal @ 0x402220`,
`CmdChainPurge @ 0x402310`, `IdleQueueRegister @ 0x407450`, `FUN_004520d0` (queue executor).*

## TL;DR

The command layer has two halves that doc 06 left disconnected:

1. **The message→command queue** (`FUN_004520d0`): a per-player ring of 5-dword records
   whose first word is an opcode; the executor switch calls entity-command primitives for
   the four "command" opcodes (5/6 → move, 0x17/0x1d → task, 0x19 → gated, 0x2b → target).
2. **The entity command subsystem** (`0x401–0x407`): allocation, two delivery routes,
   completion/cancel, and the AI task-type/priority registry.

## Command record (`CmdAlloc @ 0x406440`)

Records are **104 bytes** (`0x68`), allocated from a **doubling pool**: the first alloc
triggers `CmdPoolGrow`, which `malloc`s `2^N × 104` byte blocks (N tracked by
`DAT_004b31fc`, block table `DAT_004b31c8`), threads them into the free list
`DAT_004b31f8` via the link field `+100`. `CmdFree` pushes back the same way (and drops
cross-references, see below).

| Off | Field | Evidence |
| --- | --- | --- |
| +0x00 | record type (0=move, 2=target, 3=task, 5=gated, 7=zone?, 0x13=ref) | primitives |
| +0x04/08/0c | params (coords for move) | `CmdCreateMove` |
| +0x10 | payload ptr (entity / target) | task/target primitives |
| +0x14 | float "now" (`timeGetTime` on alloc, overwritten `1500.0` on link) | alloc/link |
| +0x1c | behaviour id = `DAT_004b32e8[kind]` | `CmdLinkHead`/`CmdSetKind` |
| +0x3c | extra slot (type-8 sets `8`) | `CmdCreateTask` |
| +0x40 | owned buffer (freed on `CmdFree`) | `CmdFree` |
| +0x4c | kind index | link functions |
| +0x5c | state flags: `\|0x10` pending, `\|0x20` done, `\|0x200` buffer-external, `\|0x2000` minion-extra, `\|0x10000` cancelled-mark | all consumers |
| +0x60 | next link (entity chain **and** global list) | chains |
| +0x64 | freelist link while unallocated | `CmdAlloc` |

## Delivery routes

- **Entity chain**: entity `+0x2f0` is the chain head; new commands append after the tail
  (`CmdChainWalk @ 0x404190` just walks to the end for primitives that skip appending when a
  current command `+0x60` exists). Purged by `CmdChainPurge`, which unlinks records with
  `+0x5c & 0x20` and frees them.
- **Global list**: `DAT_004b3354/4b3358` = head/tail. `CmdLinkHead` pushes LIFO (`+0x60` =
  old head, sets flag `0x10`), `CmdLinkTail` appends FIFO. `CmdTickGlobal @ 0x402220`
  iterates the list each frame.
- `IdleQueueRegister @ 0x407450`: type-2 (minion) entities with a freshly attached command
  register into the 20-entry idle table `DAT_00557a84` (stride 8, entry = entity + timestamp
  `12.5f`), used for poll the idle minions when the list is full.

## Kind registry (`CmdRegistryInit @ 0x401cf0`)

`DAT_4b32e8[]` is not function pointers but numeric behaviour ids (default `0x31`), indexed
by kind. Names live in two string tables resolved by `CmdKindByName`:

- **31 AI task types**: `Goto, Follow, FollowAttack, Collect, Gather, Clear, Deposit, Dump,
  Request, Repair, Dig, Dynamite, Reinforce, Wait, AnimationWait, ElecFence, Eat, GotoEat,
  FindDriver, GetTool, BirdScarer, Upgrade, BuildPath, Train, Depart, AttackPath,
  AttackRockMonster, Recharge, Dock, AttackObject, FindLoad`
- **25 AI priorities**: `Crystal, Ore, DefaultCollect, Destruction, Construction, Request,
  Reinforce, Repair, Clearing, Storage, Refining, HealthLow, GetIn, Upgrade, BuildPath,
  AttackRockMonster, Barrier, FindLoad, Recharge, UpgradeBuilding, Gather, Steal, Punch,
  Depart, AttackPath, AttackObject, Congregate`

`CmdKindByName @ 0x401cb0` does `__strcmpi` against the priority table (27 slots) to map a
config/name string to an index. The task-primitive `CmdCreateTask @ 0x402720` maps *entity
type → kind*: type 7 → 1, type 8 → 3 (+record `+0x3c = 8`), type 9 → 0x10, type 6 → 0x12
(0x2 if entity `+0x3e0` high bit set); then `MinionCompanionCmd @ 0x402850` adds a second
type-3 command with kind `0x15` and flag `0x2000` for type-6 minions.

## The queue executor (`FUN_004520d0`, per frame)

Layout: per-player command block `DAT_004eaf20 + player*0xa000`, count slots
`DAT_004fef20` with ping-pong index `DAT_004fef28`; 5-dword records = `[-2] opcode,
[-1] entity, [0] param, [1..] payload`. Before the switch, entries whose byte-indexed key
into the key-state table `DAT_0076ab80` is still down get executed early via `FUN_00452060`
(this is the "held key repeats command" behaviour, answering doc 06's cheat-key question).

| Opcode | Call | Meaning (evidenced) |
| --- | --- | --- |
| 1 | `FUN_004526c0` | (manager cmd) |
| 3 | `FUN_00452be0` | global action |
| 4 | `FUN_00452c50` | per-entity |
| 5, 6 | **`CmdCreateMove`** | move command: coords+flag |
| 8 | `FUN_00403270` | selection-drag area |
| 9 | `FUN_00452d60` | |
| 0xb | `FUN_004295f0(1,0,0)` | |
| 0xc | `FUN_00464fa0` | |
| 0x13 | `FUN_00431780(x, y, lo16, hi16)` | two-target/camera op |
| 0x15/0x1b/0x34/0x36 | `FUN_00439370/004394b0/00439340/00439280` | build/placement family |
| 0x17, 0x1d | **`CmdCreateTask`** (kind 0) | default task command |
| 0x19 | **`CmdCreateGated`** | only if entity `+0x3e0 & 0x400` |
| 0x1f | `FUN_0043aea0` | |
| 0x21/0x23/0x25 | `FUN_004470d0/00452430/00447210` | |
| 0x2b | **`CmdCreateTarget`** | attack/build target ptr |
| 0x2d | `FUN_004528c0` | |
| 0x40 | `FUN_00452cc0` | |

Cancel helpers run from `CmdFree`: `CmdCancelRef @ 0x402cd0` marks matching type-`0x13`
commands cancelled (`|0x20`); `CmdCancelByCoords @ 0x403700` does the same for type-7 zone
commands by position. `CmdMarkMatching @ 0x402000` flags commands matching type+params.

## Cross-links

- Opcodes 5/6/0x17/0x1d/0x19/0x2b confirm doc 06's guess that the queue "executes commands"
  through these four primitives; everything else in the switch calls managers directly.
- `IdleQueueRegister` is the only writer of the minion idle table (`DAT_00557a84`) — relevant
  to the `DAT_00555be0` remap-table question if it ever surfaces.
- The entity+0x3e0 flag bits (`& 0x400` gates type-5 commands, high bit toggles kind for
  type-6) extend the voxel/entity flag family from doc 07.

## Still open

- Semantics of record `+0x14` (1500.0f vs `timeGetTime`).
- Whether opcode bytes are *also* key codes in the pre-switch pass (the byte at
  `DAT_004ff0bc + i` keys the table — producer still unnamed).

## Entity-tick anatomy (closed 2026-10-21)

The open question — "which code reads record `+0x1c` and runs the AI task
switch" — has a **negative answer: none does.** Decompiles of the shipped exe
(SHA-256 `c3d8ce23…581e0f`) show:

1. `FUN_0043b270` (scene pass) = flag word `DAT_004dee14` (bit 4 pairs
   `FUN_00432200`/`FUN_004321a0`, bit 8 `FUN_00439990`) → entity walker
   `FUN_004377d0(&LAB_0043c810, &dt, 0)` → `FUN_0044bf00(dt)` → a 125 s
   broadcast timer calling `FUN_00425b40(1)`.
2. The walker (documented in doc 07) iterates the `DAT_004de790` block table,
   stride `0x40c`, skipping entities with `+0x3e8 & 0x80000` or dead links
   (`*ent + 0x408 != ent`); a nonzero callback return ends the walk.
3. The callback (entry `0x43c810`, ~15 KB, left unattributed by Ghidra) is a
   per-entity **flag-word behaviour evaluator** on `+0x3e0/+0x3e4/+0x3ec/
   +0x3e8` — not a command-record switch. `DAT_004b32e8` (numeric behaviour
   ids) has xrefs *only* inside the command-creation layer
   (`0x401c00–0x4043ff`), and the callback reads no record `+0x1c` (all
   `+0x1c` mentions in it are stack-local `+0x1c(%esp)`).
4. Behaviour lives in three entity slots instead: flag bits, timers
   (`+0x368`, `+0x2d4`), and the **activity triple** `+0x2e8` (name pointer)
   `+0x2d8` (param) `+0x2d4` (timer), set by `FUN_004409e0(ent, idx, p)` =
   `ent+0x2e8 = DAT_004decc4[idx]; ent+0x2d8 = p; ent+0x2d4 = 0`. The table
   `DAT_004decc4[0..76]` is filled by the initializer at `0x436c2f` with the
   `Activity_*` name strings (`Activity_Stand`, `Activity_Route`,
   `Activity_RouteRubble`, … `Activity_Waiting21`; the ordered 77-entry dump
   reproduces in `src/game/ai.cpp`).
5. `FUN_00447c40` is activity bookkeeping: early-return when `+0x2e8` equals
   its mirror `+0x2ec`; otherwise run `FUN_00403f30` (attach a type-`0xe`
   idle record head-first if the chain head is not `0xe`) unless the name is
   one of the exclusion set (table entries 0/1/4/6/8/12/17/23), progress the
   per-type runner, then `+0x2ec = +0x2e8`, `+0x2d4 = 0`. `+0x2ec` is written
   only at transition sites (`0x438199` pairs it with `+0x2e8` in the
   minion task-state machine, `0x43c0fe`, `0x43e6cc` immediately before a
   `FUN_00447c40` call, and inside `FUN_00447c40` itself).
6. Idiom note: the x87 pattern `fcomps 0.0; fnstsw %ax; test $0x41, %ah`
   behaves as `je` skip ⇒ value > 0, fall-through ⇒ value <= 0 (verified
   empirically on this platform); used for timer-expiry tests like
   `+0x368 <= 0` before `FUN_0044c110`.

Implemented in the port (`src/game/ai.cpp`, corpus branch order):
activity completion `0x43c845` (incl. bookkeeping + idle-record attach), wake
timer `0x43c870` (`+0x3e4` bit `0x8000000`; decrement + early return),
target acquisition `0x43c8b2` (`+0x3ec` `0x40000` → link handle `+0x354`,
flag `0x80000`), cheat-word flag `0x43c90b` (`DAT_00556ed0` bit 0 gates
`+0x3ec` bit `0x20` via `FUN_0044c530`), type-1 idle registration
`0x43c94d` (`FUN_0043a960` into `DAT_004eabcc`, cap 100). The activity mirror
folds `+0x2ec` sync into `ai_set_activity` because corpus set-sites pair the
words (point 5). Path-list movement (below) mirrors the move branch's
entity state; its map-side machinery stays TODO.

### The move branch / path list (closed 2026-10-21)

The move half of the callback consumes an **entity path list**, not record
coordinates directly (`callback.s`, branch region `0x43cb40–0x43ce88`):

1. `0x43cb98` reads `ent+0x24` = waypoint buffer pointer and `ent+0x2c` =
   current index; entries have **stride 5 dwords** (`lea (%eax,%eax,4)`), and
   the walked target is entry[0]/entry[1] (`ebx`/`ebp` at `0x43cba5–a8`). A
   null buffer falls back to the stack/command coordinates
   (`0x43cbb1`–`0x43cbb5`).
2. Progress is map-driven: `FUN_0048e060` converts float positions to cell
   indices, `FUN_00432770` answers walkability from the tile record
   (`DAT_005560d4+0xb8` array, stride `0x48`, width at `+0x14`), and
   `FUN_0044a2a0` yields the work-object position. When step progress comes
   back from `FUN_00430ee0` (or `FUN_00431560` + `FUN_00430530`) *nonzero*,
   the arrival block runs (point 3); a zero return leaves the mover idle.
3. Arrival (`0x43cd4d–0x43cd90`): `FUN_00403d10(ent, 8, 0, 1)` removes the
   kind-8 (move) entry from the entity `+0xbc` record list; `+0x3e0` bit
   `0x8` is cleared (`and $0xfffffff7` at `0x43cd66`); `FUN_00432e00(cell, 0)`
   clears the tile occupancy bit `0x8000000`; and the path entry's *pending
   byte flag* is popped — `list + +0x28*20 − 12` (i.e. entry-relative byte 8),
   read and, when `1`, zeroed (`0x43cd83–0x43cd90`). **The pop is a flag
   clear, not an index increment or removal** — consumed entries are skipped
   by their cleared flag.
4. Port mirror (`src/game/game.hpp` `AiWaypoint`/`EntitySlot.wp`, capped
   `AI_WAYPOINTS_MAX`): `wp[] = {x, y, pending}` mirrors entry[0]/[1] plus
   the flag byte; `wp_count`/`wp_index` mirror `+0x28`/`+0x2c` (the corpus
   `+0x28` doubles as the *entry-to-pop* index at `0x43cd77` — the port keeps
   one walk index, the exact dual-index semantics remain TODO). The creator
   side (`cmd_create_move`) raises `+0x3e0` bit 8 and seeds the list with one
   hop; `ai_tick` walks `wp[wp_index]`, clears `pending` on each arrival, and
   completes the command with the flag clear when the list runs out
   (`FUN_0044a2a0`-style carry position and the fallback path stay inside
   the same branch shape).

### Tile queries on the move branch (closed 2026-10-21)

The map-side queries the move branch calls are resolved semantics, ported
against the port's byte-identical voxel mirror (`LEVEL_VOXEL_STRIDE` = the
corpus `0x48` record, finding 07):

- `FUN_00432770(ent, cx, cy, mode, gated)` is the walkability check; the
  move branch calls it with `mode = entitystate & 0x2000000`, `gated = 1`,
  on the walker's **current** cell (converted by `FUN_0048e060`, which is
  the x87 round-to-nearest `fistp` trick — a plain round). In the common
  `mode == 0` shape the answer is: `cx < w-2` and `cy < h-2` (unsigned, so
  negatives fail), flag-word (`record+8`) bit `0x10` set and bit `0x40`
  clear, and the type byte (`record+2`) not `1`, `7`, or `11`. The
  entity-conditional gates — the `+0x334` state object (`FUN_00469e60`
  reads `+0x334+0x144`; `FUN_00469cc0` reads a per-terrain-type speed table
  at `+0x334+0x24` and gates on `speed != 0`), the `+0x3e8` bit-1 walker
  bit, and `+0x3e0` bit `0x400` in gated mode — live on the unported
  `+0x334` state objects (TODO, same family as the minion state machine),
  *except* the speed probe itself (closed 2026-10-21): `ai_stats_speed`
  ports `FUN_00469cc0`'s table read (`GameContext.stats_speed_sec`,
  indexed by the cell's terrain type; corpus units = seconds per cell)
  and `step_path` now steps the mover at that speed instead of the flat
  port constant — see §"Per-terrain mover speed" below.
  The nonzero-`mode` branch inspects the four neighbors of a `record+0 == 0`
  cell (spawn-style) and is not exercised by movement.
- `FUN_00432e00(cell, v)` sets/clears flag `0x8000000` — the entity
  *reservation* bit. The mover-state dispatch (`0x43deed`, same tick) marks
  the walker's own cell while `+0x3e0` bit 8 is up; the arrival block
  clears the **arrival** cell only (`0x43cd72`), so departure marks persist
  until reused. Nothing on the walkable path consults `0x8000000` — it
  feeds the pathfinder (TODO) and the spawn checks.
- `FUN_0043a960`'s idle probe reads `record+2 == '\t'` (9) at the entity
  cell (`FUN_004424f0` returns the cell pair) before appending to the
  `DAT_004eabcc` table — the '\t' TODO in `0x43c94d`'s port branch is
  closed.

Port surface (`src/game/level.*`): `level_cell_of` (`FUN_0048e060`),
`level_walkable` (`FUN_00432770` simple mode), `level_tile_mark`
(`FUN_00432e00`), `level_tile_type`; `ai_tick`'s `step_path` marks the
current cell occupied and refuses to step off an unwalkable tile (the
command stays live), clears the occupancy flag on arrival, and the idle
branch registers only on `'\t'` cells. Without a bound grid every map
operation falls back to the port-side default (the corpus tick cannot walk
an empty grid), which is what the mapless unit tests exercise.

### Per-terrain mover speed (closed 2026-10-21)

`FUN_00469cc0(ent, type)` is a two-instruction read:

```
mov  ecx, [ent+0x334]            ; the entity stats block
flds [ecx+0x24+type*4]           ; speed table entry by terrain type
```

On the move branch (`0x43cc35`–`0x43cc5c`) the walker's *current* cell is
resolved by `FUN_0048e060`, its type byte by `FUN_004319b0`, and that
entry drives the step: `FUN_00434570` accumulates `dt / entry` into cell
progress and completes a cell once it passes `1.0` — so the entry is
**seconds to cross one cell** of that terrain, and one tick can cross
several cells when the terrain is fast (`+0x3e8` bit `0x2000000` multiplies
the entry by the constant at `0x49e3b4` — the runner scale, value not
recorded in the bundles, TODO).

Port shape: the stats block itself stays TODO (finding 07's stats-block
note), so the table is modelled as `GameContext.stats_speed_sec` — one
game-wide float array indexed by the tile-type byte (`AI_STATS_TERRAIN_TYPES
= 16` port bound; the corpus indexes blindly) — read through the terrain
accessors exactly corpus-ordered (`underfoot_cps`: cell via
`level_cell_of`, type via `level_tile_type`, entry via `ai_stats_speed`
≡ `FUN_00469cc0`). `step_path` steps at that rate: the current waypoint
completes after `entry / 1` seconds under foot and the `dt` overflow
carries into the following entries, so slow terrain visibly slows the
walk and fast terrain visibly quickens it; unset (zero) entries, an
unbound grid, and out-of-range types fall back to `AI_MOVE_SPEED_CPS`,
which keeps every mapless walker test byte-identical. The corpus's
zero-entry *gate* (`speed != 0` inside `FUN_00432770`) belongs to the
unported `+0x334` walker bit and stays a pass. `ai_test` pins the probe,
the default fallbacks, and slow/fast terrain bands against a plain-row
walker with the arrival pops and occupancy clear unchanged.

### Pathfinder builder (port-shape closed 2026-10-21)

How a move command fills the entity buffer is fully evidenced, and the
port now mirrors the lifecycle:

- Install (`0x4418d0`): the dispatch sets entity `+0x24` = buffer, `+0x28` =
  count, `+0x2c` = index 0, clears `+0x28c`/`+0x290`, raises the mover
  flag, and installs the buffer — the list is *installed per command*,
  replacing the previous one.
- Append (`0x44693b`): the builder grows the heap buffer with
  `realloc(list, count*20)` (`lea (%eax,%eax,4),%edx; shl $2`) and writes
  one entry per *walked cell*: `{x, y, 0.5f}` (third dword the constant
  `0x3f000000`), stride 5 dwords — the same entry layout the walker pops.
- Finish (`0x441a50`): frees the buffer (`0x48db80`), resets
  `+0x24`/`+0x28`/`+0x2c`/`+0x28c`/`+0x290`, and clears the mover bits
  (`and $0xff7ffffe`) — a command ending releases its list.

The corpus *algorithm* between install and append is the wave-flood system
on the grid handle at `level+4` (`FUN_0044f380`/`FUN_0044efb0`): the seed
(`FUN_00430ee0`) validates the target cell (`record+0 == 0`), stamps it 1,
and raises flag `0x40000` across the four neighbours as waves, while
`FUN_00430530` marks cells `record+0 = 5` on the type-8/10 traceback and
refuses blocked (`flags & 8`) cells. That system is **not ported**
(TODO(corpus)) — its cell state byte, wave flag, and traceback live in the
voxel records the port mirrors but whose path-search fields stay empty.

Port stand-in (`ai_path_build`, called from `cmd_create_move` through the
module context): install-reset the waypoint list per command, run a
4-neighbour BFS over the grid through `level_walkable` cells only, and
append the path cells front-to-back (start excluded) corpus-style. No
grid, blocked start/target, unreachability, or an over-long path
(> `AI_WAYPOINTS_MAX`; the corpus buffer is heap-unbounded) returns 0 and
the caller seeds the one-hop fallback. The consumption side is unchanged,
so multi-entry lists flow through the already-mirrored pop-on-arrival and
completion semantics. `ai_test` covers a dispatch that reroutes around a
blocked column without ever landing on it.

Still TODO inside movement: the **wave-flood pathfinder** replacing the
port BFS stand-in (the algorithm half of this section), and the `+0x334`
state objects behind the walkability gates above (now known: `+0x334` is an
entity stats block — instance stats from the `DAT_00502bd8[type]` table at
stride 336 (attacher writes it at `0x469c98`, called from the spawn
dispatch's default arm `0x43818a`), minion types taking static templates by
the table at `0x4382ac` — type 8 → `0x4a98f0`, 9 → `0x4a9a40`, 12–14 and
16–19 → `0x4a9b90`, everything else instance stats; templates all-zero in
the file; filled at runtime, so the table values are config-driven, TODO).
Of what rides these, the speed table `+0x24 + terrain*4` is now ported as
the game-wide `stats_speed_sec` mirror (§"Per-terrain mover speed");
the per-type instance stats fill, the state word `+0x144` behind
`FUN_00469e60`, and the runner scale constant (`0x49e3b4`, unrecorded)
stay TODO.
The old "`ent+0xbc` record list" TODO resolves away: `param_1[0xbc]` is
just the dword-indexed view of `ent + 0x2f0`, i.e. the *same* command
chain the port mirrors (kinds: 8=move, 12=tile flags, 0xE=idle, `0x17`;
`FUN_00403d10` unlinks by kind with per-kind side effects).

Wake and riding (closed 2026-10-21):

- `FUN_0044c110` (wake mirror): when an entity's expired `+0x368` wake
  timer runs, its attached target (`+0x364`) is woken — `+0x340 = 100.0`
  (`0x42c80000`) and `+0x3e8 |= 0x800000` — **unless** the target carries
  `+0x3e0` bit `0x200000` (the attached special state). The `+0x364`
  pointer is filled by the attach-pair writer (`0x44c0e2`/`0x44c0ea`:
  mutual A↔B store, `+0x3e0` bit `0x100` — a riding-attach fragment,
  TODO); the port mirrors the field and the writes.
- `FUN_0044c530` → `FUN_0043c440` (can-do / riding probe): type-2 entities
  answer via `FUN_0044b480`, which scans the entity's permitted-activity
  list (`+0x370` slots, count `+0x384`) for activities 4/5/7 (drill,
  teleport, reinforce). Ported as the real multi-slot list — see
  "Permitted-activity list" below.

Permitted-activity list (`+0x370` ids / `+0x384` count; closed 2026-10-21):

- `FUN_0044b480` (scan): a zero count answers 0 without reading; otherwise
  each of the `count` dwords in the inline `+0x370` span is compared with
  the requested activity. The port models the span as five id slots
  (`AI_ACTIVITY_SLOTS`, matching the inline span's dwords before `+0x384`).
- `FUN_0043c440` (can-do): answers 1 for drill (4), 2 for teleport (5),
  3 for reinforce (7), else 0; `FUN_0044c530` wraps that behind the type-2
  gate and the cheat-word branch (0x43c90b) reads it from the list.
- `FUN_0044b370` (the install appender, e.g. the append write at `0x44b44a`):
  capacity is `FUN_0046a060(ent)` = `*(state + 0xcc)` — the stats-block
  instance fill stays TODO (the port models `ctx->stats_activity_cap`,
  defaulting to the span width); the setter's `+0x3f4 = 0` write has no
  port-side reader (TODO(corpus)); it sets `+0x3ec` bit `0x4` iff the list
  already carries a can-do value or the grant is special (`FUN_0044b350` =
  the {4,5,7} predicate). A special grant *replaces* the first special entry
  already on the list in place (`0x44b463`, count untouched); otherwise the
  value appends — slot `[count]`, `count++` while `count != cap` (`0x44b44a`)
  — and once full the list shifts left dropping the oldest (`0x44b417`) and
  the value lands in the last slot.
- Call sites: the spawn dispatch's type-2 arm (`0x437eb6`) grants
  `Activity_Stand` (value 0) to every vehicle spawn; the callback's deferred
  install branch (`0x43da62`, `+0x3e4` bit `0x40`) installs the chain-head
  command record's `+0x38` grant through the setter and returns early, with
  the kind-0x13 grant-record unlink (`FUN_00403d10(ent, 0x13, 0, 1)`)
  deferred to the port's record chain. The record's `+0x38` is the port's
  `grant_act` field (`cmd_rec_activity` / `cmd_rec_set_activity`). The
  raisers of the `0x3e4` grant bit belong to the minion task-state machine
  at `0x438150` (TODO below) — the ported list is where those grants land.

Documented TODO branches (not guessed): projectile arcs (`+0x3e0`
`0x400000`, `FUN_004755e0`/`FUN_00475120`); screen
shake (`+0x3e4` `0x40000000`, `DAT_004a48b0`); the riding-attach writer
(`0x44c0a0` fragment — attach-pair `+0x364` install, `+0x3e0` bit `0x100`);
the minion task-state machine at `0x438150` (spawn-dispatch switch — see
the stats-block note above; the task switch itself still unpinned — it is
also what raises `+0x3e4` bit `0x40` for deferred grants); the stats-block
`+0xcc` permitted-list capacity fill and the setter's `+0x3f4` write.
