# Finding 301 — Scene Geometry and Resource-Slot Helpers

Eight routines now have names based on direct geometric, lifetime, and resource-table behavior.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x479b80 | PointInPolygon2D | Uses even/odd ray crossings over polygon vertex pairs to test point inclusion. |
| 0x479d50 | Matrix4SetRotationZ | Builds an identity matrix and writes cosine/sine values into the 4×4 Z-rotation slots. |
| 0x4760e0 | AttachedObjectDataReleaseAndClear | Releases an optional nested object and its record, then clears the object slot. |
| 0x476d50 | ResourceSlotTableInitialize | Clears the 32-entry global resource table and its counters. |
| 0x476d80 | ResourceSlotTableReleaseAll | Calls cleanup and releases each populated resource-table entry. |
| 0x47ab00 | VideoResourceSlotRelease | Clears the active flag, frees the global memory block, releases three COM references, and resets slot state. |
| 0x47b050 | VideoResourceSlotEnabledSet | Normalizes and stores an enable value and forwards it to three interfaces when the slot is active. |
| 0x47b2a0 | KeyedResourceEntryRemove | Recursively unlinks and frees a list node matching the supplied key. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Direct video-resource caller context is retained in [legorr-engine-helpers-expansion-9.bundle](../evidence/legorr-engine-helpers-expansion-9.bundle), and keyed-removal caller context in [legorr-sound-instance-spatial-expansion.bundle](../evidence/legorr-sound-instance-spatial-expansion.bundle). The remaining names rely on their direct pseudocode and caller context. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
