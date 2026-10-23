# Finding 360 — Grid Animation and Resource Cleanup Helpers

This pass names grid-cell eligibility, resource cleanup, queued animation processing, grid-record cleanup, and an observed global animation-state slot.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x00433e30` | `GridCellEntityEligibilityCheck` | Checks a grid-record flag, resolves the cell object, and returns whether the object eligibility helper succeeds. |
| `0x00437050` | `EntityPoolResourcesRelease` | Releases each nonzero pointer in the observed pool range, clears adjacent globals, and clears bit 0 of another global. |
| `0x00437300` | `ProcessQueuedGridAnimations` | Advances an accumulator and, for each elapsed unit, finds an animation, marks its resource, dispatches it, and subtracts one. |
| `0x004378e0` | `GridObjectRecordsRelease` | Releases two optional pointers per record, resets the record count, clears a fixed global table, and calls a cleanup helper. |
| `0x004383a0` | `GlobalAnimationStateSet` | Stores the supplied value in the observed global animation-state slot. |

The listed bodies and caller context are explicit in the complete pseudocode. No matching direct retained evidence records were found for these exact procedure addresses.

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/legacy/engine-region.c). Field and interface meanings remain unspecified where callers do not establish them.
