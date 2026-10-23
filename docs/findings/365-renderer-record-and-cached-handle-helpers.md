# Finding 365 — Renderer Record and Cached-Handle Helpers

This pass names renderer-record attachment/cloning, field-based object update wrappers, and lazy cached-handle accessors. Exact resource and field meanings remain unspecified.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x00406360` | `EntityRecordAttachAndRelink` | Conditionally removes and attaches a record or clones it before attachment, then updates associated references; ownership and task semantics are unresolved. |
| `0x004065d0` | `GlobalDwordGet` | Returns the observed global dword. |
| `0x00406c00` | `RendererObjectUpdateFromRecordField64` | Forwards the object and record field at +0x64 to a shared update helper. |
| `0x00406c30` | `RendererObjectUpdateFromRecordField68` | Forwards the object and record field at +0x68 to a shared update helper. |
| `0x00406c60` | `RendererObjectUpdateFromRecordField6c` | Forwards the object and record field at +0x6c and an additional argument to the shared update helper. |
| `0x00406cb0` | `RendererRecordClone` | Copies 29 dwords, clears bit 0 at destination +0x70, and clones three renderer handles and the associated object. |
| `0x00406d30` | `RendererObjectTimingValueAdjustByCount` | Queries a float and subtracts a helper-derived value multiplied by count minus one when the count exceeds one. |
| `0x00406f50` | `RendererObjectSubobjectDispatch` | Passes the object and parameter to a creation helper, then forwards its result to a second helper. |
| `0x00406fb0` | `RendererCachedHandle40GetOrCreate` | Returns the cached field at +0x40, or creates and stores a handle using the descriptor at +0x2c. |
| `0x00406fe0` | `RendererCachedHandle48GetOrCreate` | Returns the cached field at +0x48, or creates and stores a handle using the descriptor at +0x34. |
| `0x00407010` | `RendererCachedHandle50GetOrCreate` | Returns the cached field at +0x50, or creates and stores a handle using the descriptor at +0x3c. |
| `0x00407040` | `RendererCachedHandle4cIsNotOne` | Lazily creates and stores a handle at +0x4c using the descriptor at +0x38, then returns whether its helper-derived value differs from 1.0. |

The checked-in pseudocode and caller context are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/legacy/engine-region.c). No exact matching direct evidence records were found for these candidates.
