# Finding 356 — Animation and Low-Address State Helpers

This pass names animation-object forwarding wrappers, terrain and grid predicates, pending-record cleanup, and guarded global-state operations.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x00406c90` | `AnimationObjectVisibilitySet` | Forwards the requested state and the wrapped object to the visibility helper; the underlying flag and object semantics remain generic. |
| `0x004086a0` | `AnimationObjectVisibilityGet` | Returns the visibility bit extracted from the wrapped object flags. |
| `0x00406d10` | `AnimationObjectTimeUpdate` | Forwards the wrapped animation object and supplied value to its timing update helper. |
| `0x00406da0` | `AnimationObjectTimeGet` | Returns the timing value derived from the wrapped animation object and current resource. |
| `0x00406dc0` | `AnimationObjectTransformSet` | Forwards position/rotation inputs and fixed arguments to the wrapped object transform method. |
| `0x00406e30` | `AnimationObjectHandleGet` | Returns the handle stored at offset +4. |
| `0x00407090` | `AnimationObjectValueGet` | Returns the value produced by the wrapped object’s value helper. |
| `0x00407360` | `GlobalTaskRefreshFlagGet` | Returns the global refresh flag used by task and entity refresh paths. |
| `0x004090a0` | `TerrainCellPredicateCheck` | Returns whether the terrain-cell predicate helper with selector zero is nonzero. |
| `0x0040e460` | `GridCellRecordPresentCheck` | Returns whether the first field of the grid record at the computed row/column slot is nonzero. |
| `0x0040f7b0` | `OptionalResourceDestroy` | Calls the resource free helper only when the supplied pointer is nonzero. |
| `0x00419e60` | `PendingRecordReleaseIfPresent` | When the pending-record global is nonzero, calls the pending-record release helper. |
| `0x00419f10` | `PendingRecordReleaseAll` | Repeatedly releases pending records until the pending-record global is empty. |
| `0x0041b470` | `CategoryValueIsNeitherZeroNorFifteen` | Returns true exactly when the supplied value is neither 0 nor 15. |
| `0x00409d40` | `LevelStateStoredValueSet` | Stores the supplied value in the global level-state slot; its domain is unresolved. |
| `0x0040ab10` | `GlobalModeZeroInputSet` | Stores whether the supplied input equals zero in a global mode slot; polarity and meaning are unresolved. |
| `0x0040e7e0` | `GlobalFlagBit0Clear` | Clears bit 0 of the observed global flag. |
| `0x0040e910` | `GlobalValueClearIfEqual` | Clears the observed global value only when it equals the supplied value. |

These operations are directly visible in the checked-in pseudocode and caller context. No matching direct evidence-bundle records were found for these addresses.

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/legacy/engine-region.c). Names retain operation-level or offset-based descriptions where semantic fields and interface details are unresolved.
