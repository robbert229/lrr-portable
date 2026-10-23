# Finding 349 — Scene-Property and Render-Resource Helpers

This pass names a scene-root global accessor, property-record accessors and cleanup, and render-resource tracking/release helpers. The property offsets and renderer vtable slots remain unnamed semantically. Checked-in direct pseudocode and caller context are in `LEGORR.full.c`; no matching direct retained evidence records were found for these addresses.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x00472940` | `SceneRootObjectGet` | Returns the scene-root global. Scene initialization assigns the global, and scene-object constructors pass it as a parent; the exact object type is unresolved. |
| `0x00476120` | `ScenePropertyPrimaryValueGet` | Calls the object vtable slot at +0x1c and returns the first dword of its record, or zero when no record is returned. |
| `0x00476140` | `ScenePropertySecondaryValueGet` | Calls the object vtable slot at +0x1c and returns record offset +4, or zero when no record is returned. |
| `0x004761c0` | `ScenePropertyFloatAt14Get` | Calls the object vtable slot at +0x1c and returns record float offset +0x14, or 0.0 when no record is returned. |
| `0x004762f0` | `ScenePropertyOwnedBufferRelease` | Obtains the property buffer through the companion accessor and frees it when non-null. |
| `0x00476d10` | `RenderResourcePairReleaseAndClear` | Frees the wrapped resource and the first stored pointer, then clears both fields; exact resource types are unresolved. |
| `0x00476f10` | `RenderSurfaceGetValuePair80And84` | Writes results from wrapped-object vtable slots +0x80 and +0x84 into optional output pointers; the values’ meanings are unresolved. |
| `0x00477220` | `RenderResourceTrackAndRetain` | Links the wrapper onto the global tracking list and invokes the wrapped object’s vtable slot +8. |
| `0x004773e0` | `RenderResourceTrackedReferencesRelease` | Walks allocated 0x20-byte records and invokes slot +8 for each tracked wrapped object; the caller later frees the backing blocks. |

The checked-in pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c) and the named excerpts are in [engine-region.c](../decompiles/legacy/engine-region.c). Names describe observed operations and offsets; unresolved record fields, enum meanings, and vtable slots remain intentionally unspecified.
