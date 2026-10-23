# Finding 367 — Scene Resources, Entity Predicates, and Offset Accessors

This pass names scene-resource list rebuilds and lazy slots, structural entity predicates, geometry callbacks, global parameter dispatch, and nested-record field accessors. The accessor names preserve offsets because the field layout is unresolved.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x0046a330` | `EntityRecordFlag400ToggleAndRefresh` | Toggles only bit 0x400 at nested entity-record offset +0x148, then calls the refresh helper. |
| `0x0046c3d0` | `RecordFieldD8EqualsOne` | Returns whether the dword at record offset +0xd8 equals 1. |
| `0x0046c340` | `IndexedRecordListRebuildForValue` | Releases the current linked list, resets list state, and rebuilds nodes for matching 0x14-byte entries. |
| `0x0046cf40` | `RecordFlagBitTest` | Tests the selected bit in record offset +0x130 and optionally rejects bits already set in the associated mask. |
| `0x0046cf90` | `RecordIndexedEntriesRebuildAndRefresh` | Rebuilds the indexed record list and refreshes associated slots. |
| `0x0046d260` | `SceneResourceSlotsRebuild` | Resolves or creates six entries from two handle fields, fills slots beginning at +0x28, and increments the count at +0x40. |
| `0x0046d2c0` | `SceneResourceVisibilitySet` | Sets or clears raw bit 0x2 at +0x1e8 and updates two primary handles and each populated slot. |
| `0x0046d9f0` | `SceneResourceNamedEntryResolveWithFallback` | Looks up an entry on an optional secondary handle, then retries on the primary handle when the first lookup is null. |
| `0x0046da90` | `SceneResourceCachedSlotB4Get` | Returns the cached slot at +0xb4 or resolves and stores it when the corresponding source field is present. |
| `0x0046dac0` | `SceneResourceCachedSlotB8Get` | Returns the cached slot at +0xb8 or resolves and stores it when the corresponding source field is present. |
| `0x0046daf0` | `SceneResourceCachedSlotC0Get` | Returns the cached slot at +0xc0 or resolves and stores it when the corresponding source field is present. |
| `0x0046d350` | `SceneResourceDrawAtOrigin` | Forwards a handle and fixed origin/rotation values to the scene-object transform helper. |
| `0x0046e330` | `GridSurfaceEntryReset` | Looks up a grid record, updates raw bits at +0x424, disables the resource at +0x420, and clears an indexed slot at +0x338. |
| `0x0043c650` | `EntityCapacityOrStatePredicate` | Requires mask bit 0x80000 and a resolved count above the referenced limit, then checks that the entity field at +0x1000 has no sign bit. |
| `0x0043b6c0` | `ClippedRectangleCallbackDispatch` | Computes componentwise min/max bounds for two float pairs, dispatches the rectangle callback, and calls the fixed helper with code 0x2d. |
| `0x0043a9f0` | `EntityPairInteractionGate` | Checks entity state and flags, reads both positions, and on an identifier mismatch requests code 0x24 and sets flag 0x200000. |
| `0x0046e1c0` | `GlobalEntityRenderParameterModeSet` | Iterates fixed-stride global records and dispatches one of two fixed float values according to the supplied mode. |
| `0x0046a000` | `Entity334FloatField90Get` | Returns the float at nested entity-record offset +0x90. |
| `0x0046a020` | `Entity334FloatField98Get` | Returns the float at nested entity-record offset +0x98. |
| `0x0046a040` | `Entity334FloatField94Get` | Returns the float at nested entity-record offset +0x94. |
| `0x0046a080` | `Entity334FloatFieldF4Get` | Returns the float at nested entity-record offset +0xf4. |
| `0x0046a170` | `Entity334FloatField130Get` | Returns the float at nested entity-record offset +0x130. |
| `0x0046a190` | `Entity334FloatField134Get` | Returns the float at nested entity-record offset +0x134. |
| `0x0046a1b0` | `Entity334FloatField120Get` | Returns the float at nested entity-record offset +0x120. |
| `0x0046a1d0` | `Entity334FloatField128Get` | Returns the float at nested entity-record offset +0x128. |
| `0x0046a1f0` | `Entity334FloatField124Get` | Returns the float at nested entity-record offset +0x124. |
| `0x0046a210` | `Entity334WordField13CGet` | Returns the dword at nested entity-record offset +0x13c. |
| `0x0046a230` | `Entity334FloatField12CGet` | Returns the float at nested entity-record offset +0x12c. |
| `0x0046a250` | `Entity334FloatField138Get` | Returns the float at nested entity-record offset +0x138. |
| `0x0046a290` | `Entity334FloatField114Get` | Returns the float at nested entity-record offset +0x114. |
| `0x0046a2b0` | `Entity334FloatField118Get` | Returns the float at nested entity-record offset +0x118. |
| `0x0046a2d0` | `Entity334FloatField11CGet` | Returns the float at nested entity-record offset +0x11c. |
| `0x0046a2f0` | `Entity334FloatField110Get` | Returns the float at nested entity-record offset +0x110. |
| `0x0046a310` | `Entity334FloatField10CGet` | Returns the float at nested entity-record offset +0x10c. |
| `0x0046a0c0` | `MappedIndexedRecordWordAtFCGet` | Returns the indexed record dword at offset +0xfc. |
| `0x0046a0f0` | `MappedIndexedRecordWordAt100Get` | Returns the indexed record dword at offset +0x100. |

Direct pseudocode for `0x0043c200` is in `legorr-energy-low-target-routing.bundle` (`ev_5a52a3fd5529b9b47b20edd10e55c3dd92e757e7af32bc2da7f06275d2beb1fe`); other routines are supported by complete pseudocode and caller context. No direct evidence IDs were found for this candidate set.

The complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/engine-region.c). Numeric task kinds and unresolved field semantics remain literal.
