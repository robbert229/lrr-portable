# Finding 408 — Save Slot Context and Level Record Lifecycle

Issue #136's 22 functions form a slot-context and campaign-tree layer around the
per-level checkpoint records. This finding maps those responsibilities and
their call flow; [`Finding 08`](08-save-and-checkpoints.md) retains the detailed
file layouts and checkpoint-blob analysis, while [`Finding 71`](71-save-slot-read-restores-level-markers-not-the-live-task-block.md)
traces what the slot-load tree walk restores. The decompiled routines are in
[`LEGORR.full.c`](../decompiles/full/LEGORR.full.c). Focused artifact evidence
includes [`legorr-checkpoint-reader-expansion.bundle`](../evidence/legorr-checkpoint-reader-expansion.bundle),
[`legorr-save-write-results-and-count-reset.bundle`](../evidence/legorr-save-write-results-and-count-reset.bundle),
and [`legorr-options-context-expansion.bundle`](../evidence/legorr-options-context-expansion.bundle).

## State model

Six save contexts begin at `DAT_00557080`, with stride `0xb8` bytes. Each
context stores a per-level record count at `+0x18`, option values in the header,
and a record-array pointer at `+0x34`. Each record is `0x3190` bytes: a flag
dword followed by a `0x318c`-byte payload. The active slot selector is
`DAT_005574d0` (`-1` means none); this differs from `SaveSlotOpen`
(`0x00416d50`), which tests the separate front-end selector
`DAT_00557020 != -1`.

Two config-built level trees, rooted at `DAT_00556ff8` and `DAT_00556ffc`, map
level names to record indices and connect `LevelLinks` children. The save path
therefore stores a record array in each slot context and uses each tree node's
index to select its corresponding record. It is campaign/level state: the
separate `SavTreeLoad` walk reads the record flag to update the level-list
marker, but does not copy the record payload into the live reward/task block
(Finding 71).
The `.osf` objective sidecar is handled by the separate routines described in
Finding 08.

## Tree and lookup flow

`LevelNodeGet` (`0x00417090`) returns an existing name-to-node entry or creates
one. For a new entry it assigns the name's index in the supplied level list,
reads that level's `LevelLinks` values, and recursively builds child nodes.
Invalid names or a failed list lookup produce a null result. The two root trees
are built during front-end initialization, so later traversal and save code
share the same name/index topology.

`SavTreeWalk` (`0x004171c0`) is a recursive depth-first visitor. It sets each
node's `+0x10` visitation field before calling the supplied callback, stops when
the callback reports a match, and otherwise visits children. The
`RegistryValueLookupWithLock` wrapper (`0x00417280`) passes the requested value
to a callback at `0x00417250`, then clears visit state with `SavClearVisited`
(`0x00416e70`) on both success and failure. The callback's full predicate is
not separately represented as a named function in the checked-in pseudocode,
so this finding does not assign it a more specific meaning. The adjacent
`RegistryValueIndexLookupOrNotFound` (`0x004172d0`) returns the first dword of
the lookup result, or `-1` when no result pointer is returned.

`SavTreeWalk2` (`0x004172f0`), wrapped by `SavTreeLoad` (`0x004173e0`), walks
the level tree after the slot records are in memory. It uses each node's index
to inspect the corresponding record flag and update a `0x20`-byte level-list
entry. It sets/clears bit `0x4`
according to the record's low dirty bit and the supplied mode gates, then
recurses. It does not restore the `0x318c` payload. `SavClearVisited` resets
the per-node visit fields and companion per-index traversal values in both
root trees, which permits later independent walks.

## Slot read, refresh, and commit

`SavReadSlot` (`0x00417a90`) accepts slot indices 0–5, builds
`Saves\\<slot>.sav`, reads the fixed header into the slot context, and walks
both level trees with a counting callback. In strict mode, its rejection check
compares the stored record count only with the first tree's count; it computes
the second count but does not use it in this comparison. On rejection it
initializes the slot and returns failure. Otherwise it allocates
`recordCount * 0x3190`, reads the per-level records, closes the file, and
reports success. The pseudocode does not inspect the read counts or compare
the `.sav` file length to the header's record count.

`SavRefreshSlots` (`0x00417df0`) iterates all six contexts. Normally it loads
each slot; under the observed `DAT_00505f84 & 0x400` mode it first initializes
the slot and then attempts the read. `SlotSummary` (`0x004152c0`) scans the
context's records, counts low-bit-marked records, sums their score values via
`0x00417430`, and formats a summary only when at least one record is marked.

`SavWriteSlot` (`0x00417bd0`) owns the `.sav` path and has separate
initialization and context-write paths. Its context-write path serializes the
slot header and per-level records, then calls `OsfWrite` for the sidecar.
`SavCommit` (`0x00417fc0`) obtains the active context with `SavBuildCtx`
(`0x00417e70`) and passes it with the active slot to `SavWriteSlot`. The
new-file initialization writes a `0xb4`-byte header while the reader and
existing-file update path use `0xb0`; Finding 67 records this observed size
difference. Write helpers' completion counts were not checked in this static
trace, and no runtime save-file comparison was performed.

## Per-record mutation and transactional snapshots

`SavMarkRecord` (`0x00417eb0`) sets the low record flag and, in the observed
indices/modes, also sets flag bit `0x2`; campaign indices above seven use a
separate bound check. `SavWriteRecord` (`0x00417f40`) copies `0x318c` bytes
from a checkpoint blob to the selected record after its flag dword.
`SavRecordPtr` (`0x00417f90`) returns that payload address for an in-range
record. The checkpoint caller (`0x00462270`) maps the active level to a tree
index, marks it when the level state qualifies, copies the checkpoint payload,
and commits the active slot; when no slot is active, it selects slot 5 and
initializes that context before committing.

Before a save-menu operation, `CampaignChainSnapshotCreate`
(`0x00418040`) deep-copies the active context header and record array into a
caller-provided `0xb8`-byte snapshot. The paired restore helper
`0x004180c0` can replace the active context with that snapshot; this preserves
slot state across a front-end selection flow. `SaveStateEventContextSet`
(`0x00418110`) sets `DAT_0055781c`, which `FrontEndStateChange`
(`0x004169c0`) consults when choosing the tree-load arguments. The precise
user-visible meaning of the event-context values remains unassigned here.

`SavSlotCtx` (`0x00417e50`) returns the context pointer only for slots 0–5;
`SavBuildCtx` returns the currently selected context only when the active slot
is not `-1`. `SavGetSlot` and `SavSetSlot` (`0x00417e90` / `0x00417ea0`) are
the getter and setter for that active-slot selector.

## Function map

| Address | Function | Observed responsibility |
| --- | --- | --- |
| `0x004152c0` | `SlotSummary` | Counts marked level records, sums score values, and formats the slot summary. |
| `0x00416d50` | `SaveSlotOpen` | Tests whether front-end slot selector `DAT_00557020` is not `-1`. |
| `0x00416e70` | `SavClearVisited` | Clears traversal markers and companion per-index values in both level trees. |
| `0x00417090` | `LevelNodeGet` | Gets or recursively constructs a level-name node and its `LevelLinks` children. |
| `0x004171c0` | `SavTreeWalk` | Performs a visit-marked depth-first walk with callback early exit. |
| `0x00417280` | `RegistryValueLookupWithLock` | Runs the lookup callback through `SavTreeWalk`, clears visitation, and returns its result pointer. |
| `0x004172d0` | `RegistryValueIndexLookupOrNotFound` | Returns the first dword of a lookup result or `-1`. |
| `0x004172f0` | `SavTreeWalk2` | Maps node indices to record flags and updates associated level-list marker bits. |
| `0x004173e0` | `SavTreeLoad` | Wraps `SavTreeWalk2` and clears tree visitation state afterward. |
| `0x00417a90` | `SavReadSlot` | Reads a slot header and per-level record array, with a tree-count compatibility check. |
| `0x00417bd0` | `SavWriteSlot` | Initializes or serializes a slot file and invokes the OSF sidecar writer on context updates. |
| `0x00417df0` | `SavRefreshSlots` | Reinitializes/reads the six slot contexts according to the observed mode bit. |
| `0x00417e50` | `SavSlotCtx` | Returns a slot-context pointer for indices 0–5. |
| `0x00417e70` | `SavBuildCtx` | Returns the active slot context or null when no slot is selected. |
| `0x00417e90` | `SavGetSlot` | Returns the active slot index. |
| `0x00417ea0` | `SavSetSlot` | Stores the active slot index. |
| `0x00417eb0` | `SavMarkRecord` | Sets per-record state bits subject to mode-specific index bounds. |
| `0x00417f40` | `SavWriteRecord` | Copies the `0x318c`-byte checkpoint payload into a selected record. |
| `0x00417f90` | `SavRecordPtr` | Returns the payload pointer for an in-range active-slot record. |
| `0x00417fc0` | `SavCommit` | Writes the active slot context through `SavWriteSlot`. |
| `0x00418040` | `CampaignChainSnapshotCreate` | Copies the active context header and record array into a caller snapshot. |
| `0x00418110` | `SaveStateEventContextSet` | Stores the event-context value used by front-end state transitions. |

## Boundaries and follow-ups

- The record flag bits and level-list marker bit are described only by their
  observed tests and writes; their complete UI/config semantics are not
  established by this slice.
- `RegistryValueLookupWithLock`'s callback body at `0x00417250` is not a named
  pseudocode function, so its exact lookup predicate remains open.
- The reader and writer wrappers do not expose checked byte counts in these
  functions. Finding 67 records the header-size split; runtime save/load and
  malformed-file experiments were not run here.
- Finding 71 bounds the load claim: this tree walk updates level markers, not
  the live task block. The full `.osf` sidecar flow and its stale-shadow
  behavior are in Finding 08 and the related sidecar findings.
