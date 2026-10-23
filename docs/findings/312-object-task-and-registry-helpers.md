# Finding 312 — Object Task and Registry Helpers

Seven low-address routines create tasks, submit paths, filter action targets, and update object registries/groups.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x403320 | ObjectCommandTaskCreate | Allocates a task record for supported object types, fills its payload/command fields, and attaches it to the object's task chain. |
| 0x404e00 | ObjectPathSubmitToGridTarget | Gets the object's current grid position, skips if already at target, otherwise obtains a route, submits its points, and frees temporary arrays. No direct caller was resolved. |
| 0x404f50 | ObjectFilterMatchesActionTarget | Filters selected task-record types, validates the referenced object, and compares the stored target tuple. |
| 0x407410 | Type4ObjectRegistryInsert | Inserts a type-4 object into the first free entry of a 20-slot table, avoiding duplicates. |
| 0x4074b0 | Type2TimedRegistryInsert | Inserts a type-2 object and a fixed duration into the first free entry of a 50-slot table. |
| 0x407540 | FlaggedObjectTimerRegistryInsert | Inserts an object with flag `0x80` and a fixed timer value into a 20-slot table. |
| 0x408ce0 | OperationGroupAppendObject | Finds a group, appends an object reference, increments counters, and updates completion state; on lookup failure it emits a type-dependent event. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). `0x403320` is reached from command dispatch `0x41dca0`; `0x407410` is used by `0x43c570`, `0x407540` by `0x43f6a0`, and `0x408ce0` by the placement workflow `0x44a910`. No matching retained direct-procedure records were found for these functions. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
