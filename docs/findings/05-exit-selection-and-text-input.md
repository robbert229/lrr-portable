# 05 — Exit, Selection, Text Input & State Globals

Tier-3 batch (8 more functions) plus xref/byte evidence from the live REA
session. Pseudocode: [`../decompiles/engine-region.c`](../decompiles/engine-region.c)
(lines 1399+, `QuitPoll` … `MessagePreCheck`).

## `QuitPoll` (`0x4589c0`) — quit detection via mouse-corner dwell

Signature `QuitPoll(edgeA, edgeB, &quitFlag)`. Mechanics:

- Mouse position: `DAT_0076ab24/ab28` = x, y. `DAT_00556eec` / `DAT_00556ef0` =
  screen-corner rectangles (rect + `0x1d6` × `0x13b` = width/height margins —
  the "corner zone").
- Cursor state machine on `_DAT_004ffbc0` **objective state word**
  (bits 1/2/4 = result states, bit 8 = advisor-hint-pending; all xrefs sit in the
  objective layer `0x457d–0x4591` — finding 22 corrected the earlier "input-device
  mode word" label):
  `uVar2` ∈ {0,1,2,3,4} selects a per-mode dwell slot in two counter arrays,
  `DAT_004ffdd8[uVar2]` (enter-count) and `DAT_004ffdf8[uVar2]` (leave-count).
- Cursor inside corner zone: inject `FUN_0046b770('!')` (0x21), increment
  enter-count. Cursor leaves: inject `FUN_0046b770('"')` (0x22), decrement.
- When counts balance and mouse leaves the quit rectangle
  (`0x82 × 0x13b`, i.e. past the edge), calls `FUN_004588e0()` (device release)
  then `quitFlag = (FUN_00435940() == 0)` — the "keep running?" probe decides.
- Device mode 1 path: releases device, `TransitionControl(0,0)`, centers the
  window (`FUN_0047f0d0(w/2, h/2)`), checks `FUN_00407360` (menu).
- `FUN_004588e0()` (decompiled): clears device flags 1/2/4 (`FUN_004567c0`
  closes device 1), releases audio `FUN_0047ba20(0)` if `DAT_004ffe3c != -1`,
  `FUN_004652c0(0)`, and if `FUN_00456240()` returns 0 clears debug bit `0x800`.

## `SelectedEntity` (`0x4525c0`)

`return -(uint)(DAT_004ff298 != 0) & DAT_004fef2c;` — the selection global is
`DAT_004fef2c` with enable-gate `DAT_004ff298`. **All** 28/29 xrefs to these
two live in `0x4521fe–0x452d9d`, one dense function cluster — the selection
manager. (No writes from anywhere else; nothing else changes "what's
selected".) This is where entity selection/pick logic lives.

## `TransitionControl(mode, dir)` (`0x425c30`)

Tiny and definitive: sets **`DAT_00556ecc |= 0x100000` (time freeze)** +
`DAT_00556ed0 |= 0x1000` when entering, and on exit calls `FUN_00425b40(0)`,
`FUN_00464fa0(7, 1)` (silences sound slot 7 — music), clears both.
Confirmed: the entire fade/transition machine is a *time-freeze flag*, and
the music ducks via sound slot 7.

## `EntityRename` (`0x437880`)

Allocates an 11-byte buffer at `entity+0x8`, `strncpy(…, 11)`, NUL-terminate.
**Entity names are 11 chars max** (`entity+0x8` = name pointer, lazily
allocated by `FUN_0048dbd0(0xb)`). This is what the debug-key rename mode and
`EnginePreTick` text-pump write into.

## `KeyToAscii` (`0x48e380`)

Windows-layout-aware key→character: without an HKL (`DAT_00549180 == 0`) it
simply upper-cases `a–z`; with an HKL it consults shift state via the scancode
table `PTR_DAT_004afd68` and calls `FUN_00493ca0(hkl, 0x200, …)`
(= `ToUnicodeEx`-style translation). Deadkeys / multi-char results handled.

## `KeyContextCheck` (`0x411f00`)

`return table[key]` from `DAT_004a1f60` — 256-entry, 4-byte entries:
per-key text-input context filter used by `EnginePreTick`. Static data in
`.rdata` (no code xrefs other than the getter).

## `SystemStateFlags` (`0x456240`) + its setter

Returns `DAT_004ff980` verbatim. Exactly one writer in the binary:
`FUN_00456550(*p)` (`DAT_004ff980 = *p`). So the game-state bitmask (the one
gating messages: `&0x100`, editor: `&0x8`?) is installed wholesale from one
place — worth tracing its argument later.

## Message-config origin — resolved: NOT level data

Byte-read of the whole config block (`0x4dc6fc…0x4dcaf0`) shows **all zeros**
in the shipped image, and **every** xref to the config, count
(`DAT_004dcd38`, 25 xrefs) and flags (`DAT_004dcd48`, 23 xrefs) lands inside
`0x419243–0x41a226` — the message module itself. Conclusion: message configs
are built by the message module's own code (a small self-contained cluster,
one likely init/def table around `0x419243/0x4193d0` touching the flags), not
streamed from level files. The 40-id catalog is hard logic + defaults.

`MessagePreCheck` (`0x4199f0`, decompiled) is now clear too: ids
{10, 11, 12, 38} are the exception list that skips dedupe — for all others, if
the id is already in the active queue, the queue entry's stored callback runs
immediately (`MessageSubscriberDispatch`, `0x4196a0`, with `LAB_004199a0` and the message args).
So `MessageDispatch` = **dedupe-with-live-callback** message bus.

## Ledger updates

| Item | State |
| --- | --- |
| Corner-dwell quit gesture (counts + key injection `'!'`/`'"'`) | observed, purpose inferred |
| `DAT_004ffbc0` = objective state word (result bits + config flags; **not** input devices) | observed: four writer/reader functions, all in the objective layer `0x457d7e–0x459157` (finding 22) |
| Selection cluster `0x4521fe–0x452d9d` = selection manager | observed (xref density) |
| Message config populated by module code, not level data | observed (all-zero image + xref census) |
| `DAT_004ff980` installed by `FUN_00456550` | observed |

## Next targets

1. ~~Selection cluster~~ done → [06 — Command Queue & Selection](06-command-queue-and-selection.md).
2. ~~Input-device region `0x457d7e…`~~ done: the word is the level-objective state
   word, not input devices — [22 — Research crumbs](22-research-crumbs.md) §2.
3. ~~`FUN_00435940`~~ done: `KeepRunning` — doc 06.
4. `FUN_00456550` callers — who installs `SystemStateFlags`.
