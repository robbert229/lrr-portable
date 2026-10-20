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

- Actual execution of `+0x1c` behaviour ids (which code reads record `+0x4c/+0x1c` and runs
  the AI task switch) — likely in the entity-manager tick, 0x43xxxx.
- Semantics of record `+0x14` (1500.0f vs `timeGetTime`).
- Whether opcode bytes are *also* key codes in the pre-switch pass (the byte at
  `DAT_004ff0bc + i` keys the table — producer still unnamed).
