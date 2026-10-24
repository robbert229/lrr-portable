# 04 — Sim Step, Input System, Trackers & Message Events

Decompiled with the same headless pipeline (`tools/ghidra/CreateEngineFunctions.java`,
pseudocode in [`../decompiles/legacy/engine-region.c`](../decompiles/legacy/engine-region.c)).
All five functions resolve names inside the Ghidra project, so the pseudocode
uses the names below.

## `MainSimStep` (`0x4250c0`, called `MainSimStep(scale, dt, &o1, &o2, &o3)` in EngineTick)

Despite being the quit-gate of the frame, this is **not** the simulation —
it is the **input + app-state step**. Returning 0 is how any quit request
terminates the game loop. It is dominated by the keyboard state block:

| Global | Role | Evidence |
| --- | --- | --- |
| `DAT_0076ab80` (256 B) | current key snapshot (one byte per key) | cleared after consumption; indexed 0..`0xff` |
| `DAT_0076aa00` (256 B) | previous frame snapshot | every key test is `(cur != 0 && prev != cur)` = **key-down edge detection** (e.g. `(DAT_0076abb9 != 0) && (DAT_0076aa39 != DAT_0076abb9)`, delta 0x180 between tables) |
| `DAT_004de430` (256 B) | second mirror used by the text-input path | copied + current cleared in `EnginePreTick` |

Behavior clusters observed:

- **Quit paths:** `FUN_004589c0(a, b, &flag)` → sets an out float; `if (param_2 != 0.0) return 0;`
  ends the game. Other returns 0 go through `FUN_00435940()` (the "keep running?"
  probe also seen in `EngineShutdown`).
- **Focus loss:** when `DAT_00505808 == 0`, it builds a menu from string
  `"LoseFocusAndPause"` (`FUN_00478f50` + `ConfigBooleanGetDefault2 @ 0x4790d0`) and on choice
  starts the fade transition (`FUN_00425c30(0,1)` + bit `0x2000000`).
  So `FUN_00425c30(mode, dir)` is the **screen-transition controller**.
- **Camera keys (in-game):** `FUN_00436900 / 004369f4 / 00436aa5(DAT_005560e4, dt*k)`
  from edge-detected keys — pan/rotate/zoom the active view. Editor keys
  instead call `FUN_0044a310(DAT_00556160, …)` (scene editor gizmo).
- **Debug/cheat keys** (gated `DAT_00556ed0 & 0x10`): toggles for
  `Frame Rate Monitor`, `Memory Monitor`, `Sound Effects`, `Music`
  (each prints `… : on/off` via `FUN_00485230`), plus:
  - `DAT_0076ab98` held → `state+0x1fc -= _DAT_0049e2b0` each frame —
    an **air-level accelerator**: driving the oxygen threshold down toward
    the current time so the air warning/beat fires (debug/cheat).
- **Object rename mode:** with debug bit `0x20000`, grabbing a type-2 entity
  (`FUN_004525c0()` = selection ptr; `*sel == 2`) inside screen bounds calls
  `FUN_00437880(sel, "Object")` (rename) and publishes `sel+0x8` name pointer
  into `DAT_00556fa0` — the same global `EnginePreTick` edits typed
  characters into. `DebugPanelTextDrawUpdate` also displays the pointed-to
  string at the projected coordinates stored beside it. (See below.)
- Outputs `param_3/4/5` = edge flags forwarded to `EngineTick`
  (`FUN_00426520(fVar2, dt, o1, o2, o3)`).

## `EnginePreTick` (`0x4231f0`) — keyboard-to-text pump

Runs first in `EngineTick`. When `DAT_00556fa0` is non-null, it treats it as a
mutable **string pointer** (observed in object rename mode), walks the
256-byte key table, and for
each newly pressed key calls `FUN_00411f00(key)` (scan-code→context check) and
`FUN_0048e380(key)` (key→ASCII), appending the char to the string and NULing.
Afterwards it snapshots `DAT_0076ab80…` into `DAT_004de430…` and clears the
current table — the mirror trick that makes edge detection work in
`MainSimStep`. If certain "menu" keys were pressed (`FUN_0047a800` +
`FUN_004250a0` = abort-edit), text mode is cancelled.

## `ManagerTick` (`0x45a730(index, dt)`) — 12-slot positional trackers

Not subsystem managers — a table of **12 tracked objects** (stride `0x30`,
block at `DAT_00500110`), per index:

- `+0x110` handle (object/sprite), `+0x114` mode (0/1), `+0x118/1c` target x/y
  (source A) — or `+0x120/24` when flag `& 4` selects source B,
  `+0x128/2c` **smoothed** x/y, `+0x13c` flags.
- Smoothing is exponential decay:
  `prev = ((A/dt)·prev + target) / (A/dt − B)` with `A=_DAT_0049e5e8`,
  `B=_DAT_0049e5ec`, snapping when `|Δ| ≤ _DAT_0049e5e0`.
- Mode 0: `FUN_0047dcb0(handle, 0, (x,y), 0)` — reposition object.
  Mode 1: read two int-returning probes (`FUN_004897d0`, `FUN_004850e0` —
  likely screen/cursor metrics) then `FUN_00484070(handle, (x,y), ingame, 1)`
  — attach/spawn effect at position.
- Index 9 has special duty: on entry it **stops four timers**
  (`_DAT_00500860` bits 0x10–0x80 → `FUN_0047dcb0(timer, 0, …)` = kill).

Likely the in-game radar / follow-camera / cursor-follow markers. The 12
indices are all exercised from `EngineTick`'s `0x800` block (0,1,2,4,5,6,7,8,9,10,11).

## `MessageDispatch` (`0x419b80(id, p2, p3, p4)`) — event/notification bus

Config-driven message queue (like `SND_*`/HUD events), max **40 message ids**
(`DAT_004dcd38` capped at `0x28`, config stride `0x14` at `DAT_004dc6fc`):

- config+`0x00`: (id slot), +`0x04` **sound handle**, +`0x08` float cooldown
  timer, +`0x0c` flags (`0x10000` = repeatable?, `0x20000` = suppress/one-shot).
- Global enables: `DAT_004dcd48 & 4` (system on), `& 8` (auto-cooldown rule);
  `FUN_00456240(0) & 0x100` gates everything (game not live).
- Cooldown: when `flags & 0x20000` and timer `<= now`, resets timer to
  `25.0` (`0x41c80000`) and mutes via `FUN_00464ef0(sound, 0)`; ids 10–12/0x26
  reset a cluster of four timers together (`DAT_004dc7cc…` = linked message family).
- Queue: up to 40 active entries (`DAT_004dca20` ids stride 5, `+0x4` negative
  age, `+0x8` float timestamp via `FUN_00419b50()`); `InfoMessageQueueFindId @ 0x419b20` searches,
  `InfoMessageQueueTick @ 0x419f30` advances message effects, and `InfoMessageTimerDecay @ 0x41a2c0`
  decrements timers; `DAT_004dcd38` is the write cursor.

So: `MessageDispatch(0x10..0x13,…)` = "air low / air beat" notifications the
tick fires (doc 03 §7); id table and linked families come from level data.
**Closed (2026-10-20, finding 22):** population is at **startup**, not in the level-
load path — the 40-name catalog is hardcoded in `FUN_00419230` (installed via the
message-init callback `FUN_0041fb50`), and sound handles/flags come from the
`InfoMessages` config section (`FUN_00434c10`). The full catalog is in 22 §1.

## `ViewContextTick` (`0x435f5c(ctx, state, dt, scale)` — ticked ×3/frame)

The three view objects `DAT_005560e4/e8/ec` are state machines in
`*ctx[0] ∈ {1,2,3}`:

- **1 = follow mode:** projects the followed entity (`param_2+4` = level root
  via `FUN_0044f6d0/FUN_0044f940` grid pick) and clamps the camera at world
  edges (`DAT_005560ac` = tile-count clamp) — classic edge-locked camera.
- **2 = scripted/follow path with lag:** interpolates toward a target with
  the distance-based formula `k = |d|/(|d|+A)`, updates `ctx[0x24]`.
- **3 = free/cruise:** reads velocity `param_1[5]`, EMA-smooths
  (`FUN_0044a070` blend), with `*ctx+0xb8 & 0x10` dirty flag.
- Common tail: velocity ramp `v *= ((a−b)/a)·k(dt)` for glide-to-stop.

Field `ctx[9]/[10]` are two camera handles (`FUN_004755e0 = teleport/offset`,
`FUN_004754c0/15430/15470 = move/rotate/look-at`), `ctx[0x2b..0x2d]` = velocity.

## Corrected ledger

| Function | Earlier guess | Now |
| --- | --- | --- |
| `FUN_004250c0` | "main sim" | **input/app-state step** (quit gate, key edges, debug keys, camera keys) |
| `FUN_0045a730` | "subsystem managers" | **12-slot positional trackers** |
| `FUN_00419b80` | message dispatcher | confirmed, config-driven, 40 ids |
| `FUN_00435f5c` | view contexts | confirmed: camera state machines |

New primitives identified: `FUN_00425c30` (transition controller),
`FUN_004525c0` (selection pointer), `FUN_0041fb40` (time scale),
`FUN_004589c0` (quit poll), `FUN_004772d0` (screen→world),
`FUN_00437880` (entity rename), `FUN_0048e380` (key→ASCII).

## Next targets

1. `FUN_004589c0` — done → [05 — Exit, Selection & Text Input](05-exit-selection-and-text-input.md).
2. Message-config origin — done: module-static, not level data (doc 05).
3. Entity root / selection cluster `0x4520d0…0x452da0` (doc 05 §`SelectedEntity`).
4. `FUN_00411f00` / `FUN_0048e380` — done (doc 05 §`KeyContextCheck`, `KeyToAscii`).
