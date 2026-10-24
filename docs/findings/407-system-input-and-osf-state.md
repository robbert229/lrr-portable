# Finding 407 — System Input and OSF State

This finding connects the 13 assigned routines in Issue #205 across two related
but distinct state machines: the objective/input control word and the objective
sidecar table. The checked-in pseudocode is in
[`LEGORR.full.c`](../decompiles/full/LEGORR.full.c); the observations below are
from the shipped `LegoRR.exe` decompilation, with focused evidence in
[`legorr-osf-restore.bundle`](../evidence/legorr-osf-restore.bundle),
[`legorr-checkpoint-reader-expansion.bundle`](../evidence/legorr-checkpoint-reader-expansion.bundle),
and [`legorr-save-write-results-and-count-reset.bundle`](../evidence/legorr-save-write-results-and-count-reset.bundle).

## Control and input flow

`DAT_004ffbc0` is the shared control word read by `GlobalOptionLowBitsEnabled`
(`0x004589a0`), `QuitPoll` (`0x004589c0`), `ObjectiveTick`
(`0x00458be0`), and `ObjectiveTimerEvaluate` (`0x00459050`). The getter only
tests whether any of its low three bits are set; it does not identify which
option is active. The other observed bits gate objective completion and
transition paths, but their complete configuration-level meanings are not
established here. This is the objective-state word discussed in
[`Finding 22`](22-research-crumbs.md), not an input-device mode word.

`QuitPoll` receives two mode gates and an output pointer. It selects one of
five dwell-counter slots from the low control bits and a high-byte bit, clears
the output, then samples the cursor against two screen-corner rectangles. The
counter pair at `DAT_004ffdd8` / `DAT_004ffdf8` tracks entering and leaving the
corner; the function injects event values `0x21` and `0x22` through
`0x0046b770`. On its qualifying leave path it calls `InputDeviceRelease`
(`0x004588e0`), then sets the output according to the result of
`0x00435940`. The caller `MainSimStep` (`0x004250c0`) passes those gates and
uses the output as part of its quit decision; the cursor gesture is therefore
only one conditional route through the frame's quit gate.

`InputDeviceRelease` is broader than its name suggests. It conditionally
releases the active input device and clears bits in `DAT_004ffbc0`. It also
clears an advisor-state marker; if `DAT_00556ed0 & 0x800` is set, it calls
`CDAudioWatchdogModeUpdate(1)` (`0x004297b0`), whose nonzero path starts a
selected CD-audio track. Separately, when the objective stream handle is not
`-1`, it stops streamed slot 0 and resets that handle. It then resets the
screen transition, drains a sound queue, and conditionally clears a game-state
flag. `ObjectiveTick` (`0x00458be0`) also calls it when the objective-state
counter exceeds its observed bound. The precise meaning of each device mode
and several cleared global bits remain unresolved. See
[`Finding 214`](214-objective-voice-timer-does-not-stop-stream.md) for the
distinct advisor-state marker and stream-stop behavior.

## Objective tick and sidecar state

The objective tick consumes `DAT_004ffbc0` in priority order. It evaluates
normal objective conditions through `ObjectiveTimerEvaluate`, handles the
observed result-state branches, updates transition/audio state, and consumes a
pending objective voice. `ObjectiveTimerEvaluate` rejects checks under bit
`0x10`, decrements the level timer while bit `0x800` is set, and can report
expiry before checking the crystal (`0x100`), ore (`0x200`), and delegated
entity predicate. The timer outcome bit at `0x20` selects the output result.
See [`Finding 45`](45-timer-objective-short-circuits-other-checks.md) for the
timer short-circuit and [`Finding 214`](214-objective-voice-timer-does-not-stop-stream.md)
for the voice-stream boundary.

The sidecar table is a separate five-word-per-entry list. Its state is held in
these globals:

| Global | Observed role |
| --- | --- |
| `DAT_004ffe50` | live entry allocation |
| `DAT_004ffe54` / `DAT_004ffe58` | live count / capacity |
| `DAT_004ffe5c` / `DAT_004ffe60` | restart snapshot allocation / count |
| `DAT_004ffe64` | dirty flag returned by `OsfReadDirtyFlagGet` (`0x004593d0`) |

`OsfAdd` (`0x00459190`) appends a 20-byte record only for kind-2 entities whose
saved values or name pointer qualify; capacity grows by ten entries. The
record contains the values at entity offsets `+0x3f0` and `+0x330`, followed
by up to eleven bytes from the name. `PopEntityStateOverride`
(`0x00459240`) consumes the last live entry for a kind-2 entity, restores the
first value and name, and delegates application of the second value. The
append/pop pair is LIFO; it is not an indexed entity lookup. See
[`Finding 46`](46-objective-sidecar-append-gates.md) for the observed append
gate.

`OsfSnapshot` (`0x004592e0`) copies a nonempty live table over the previous
shadow, sets the shadow count, and marks the table dirty. When the live table
is empty, it does nothing, so an older shadow can remain. `OsfSnapshotTableBuild`
(`0x00459360`) clears and rebuilds the live table from a nonempty shadow; it
does not clear the shadow or dirty flag. This distinction matters on level
restart: the live list can be reset and repopulated from the last nonempty
snapshot. The clear routine `OsfReadStateReset` (`0x004592a0`) frees and zeros
only the live allocation and count/capacity; it does not reset shadow state.

`OsfWrite` (`0x004593e0`) writes the `0x534f5252` header, live count, and
20-byte entries. If opening the path succeeds, it closes the stream, snapshots
the live table, clears the dirty flag, and returns success; the pseudocode does
not check the individual write results. `OsfRead` (`0x00459470`) first clears
the live table, then checks the header and exact `8 + count*0x14` file size
before allocating and reading entries. It snapshots a valid read and clears
the dirty flag. Its return value means the file opened, not that its contents
passed validation; see [`Finding 72`](72-osf-reader-return-means-open-not-validated.md)
for the parse-result boundary and [`Finding 08`](08-save-and-checkpoints.md)
for the full save/checkpoint lifecycle.

## Function map

| Address | Function | Observed responsibility |
| --- | --- | --- |
| `0x004588e0` | `InputDeviceRelease` | Releases selected input state, clears an advisor marker, may start CD audio, may stop streamed slot 0, and resets transition/sound state. |
| `0x004589a0` | `GlobalOptionLowBitsEnabled` | Returns whether any of bits 0–2 in `DAT_004ffbc0` are set. |
| `0x004589c0` | `QuitPoll` | Updates cursor dwell counters, injects cursor events, and may release input state and set the quit output. |
| `0x00458be0` | `ObjectiveTick` | Advances objective result, transition, and pending voice state from the objective control word. |
| `0x00459050` | `ObjectiveTimerEvaluate` | Applies timer and resource thresholds, then delegates the remaining entity predicate. |
| `0x00459190` | `OsfAdd` | Appends a qualifying kind-2 entity's compact state/name record to the live sidecar table. |
| `0x00459240` | `PopEntityStateOverride` | Restores and removes the most recently appended sidecar record for a kind-2 entity. |
| `0x004592a0` | `OsfReadStateReset` | Frees and clears only the live sidecar table. |
| `0x004592e0` | `OsfSnapshot` | Copies a nonempty live table to the shadow and marks it dirty. |
| `0x00459360` | `OsfSnapshotTableBuild` | Rebuilds the live table from a nonempty shadow snapshot. |
| `0x004593d0` | `OsfReadDirtyFlagGet` | Returns the sidecar dirty flag. |
| `0x004593e0` | `OsfWrite` | Writes the sidecar header/table; on successful open, snapshots and clears dirty state. |
| `0x00459470` | `OsfRead` | Clears live state, validates file header/length, then reads and snapshots valid entries. |

## Limits and follow-ups

- The meaning of individual low control bits, the five cursor dwell slots, and
  several input-mode branches is not fully recovered; the findings retain the
  observed bit tests rather than assigning unsupported names.
- The `OsfWrite` body does not branch on write counts. Whether short writes
  occur in practice is untested; a focused follow-up could verify the file-I/O
  helper contract and failure behavior.
- The reader's opened-but-invalid return and the stale-shadow possibility are
  documented in Findings 72 and 08; no malformed-file runtime experiment was
  performed for this map.
