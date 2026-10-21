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
words (point 5).

Documented TODO branches (not guessed): path-list movement internals
(`+0x24` waypoint list, `+0x3e0` bit 8, `FUN_00432e00` pop, `FUN_00433be0`);
projectile arcs (`+0x3e0` `0x400000`, `FUN_004755e0`/`FUN_00475120`); screen
shake (`+0x3e4` `0x40000000`, `DAT_004a48b0`); riding probe `FUN_0043c440`
(inside `FUN_0044c530`); attached wake target (`+0x364`, `FUN_0044c110`);
tile `'\t'` probe inside `FUN_0043a960`; the minion task-state machine at
`0x438150` (indirect jump table `0x438298`, sets activities per state via
`FUN_00447c40` — the closest thing to the missing "task switch").
