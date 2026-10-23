# Finding 357 — Entity Fields and Global Counter Helpers

This pass names entity radius/mask accessors, a scene-object resource getter, and five global accumulator increments. The global counters retain their exact addresses because their meanings are unresolved.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x0046a270` | `EntityCollisionRadiusGet` | Returns a float from the nested entity record at offset +0xe4; a named caller compares it with a distance between entity centers. |
| `0x0046cf80` | `EntityTypeMaskGet` | Returns the dword at offset +0x12c; callers use it in mask operations, but individual bits are unresolved. |
| `0x0046d330` | `EntityTypeFlagGetBit1` | Returns bit 1 of the dword at offset +0x1e8; flag meaning is unresolved. |
| `0x0046d9e0` | `SceneObjectResourceGet` | Returns the pointer stored at offset +8. |
| `0x00462830` | `GlobalFloatAt00554adcAddOne` | Adds 1.0 to global float DAT_00554adc. |
| `0x00462850` | `GlobalFloatAt00554efcAddOne` | Adds 1.0 to global float DAT_00554efc. |
| `0x00462870` | `GlobalCounterAt00555324Increment` | Increments global integer DAT_00555324. |
| `0x00462880` | `GlobalCounterAt00555328Increment` | Increments global integer DAT_00555328. |
| `0x00462890` | `GlobalCounterAt0055532cIncrement` | Increments global integer DAT_0055532c. |

These operations are directly visible in the checked-in pseudocode. Named callers establish radius/mask usage for some accessors; no matching direct evidence-bundle records were found for these addresses.

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/engine-region.c). Names retain operation-level or offset-based descriptions where semantic fields and interface details are unresolved.
