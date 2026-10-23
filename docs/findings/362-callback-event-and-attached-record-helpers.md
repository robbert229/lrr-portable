# Finding 362 — Callback, Event, and Attached-Record Helpers

This pass names observed global flag and handle cleanup, generic callback dispatch, event/task queueing, and attached-record allocation. Callback effects and numeric event meanings remain unspecified.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x00401c20` | `GlobalFlagBit0IsSet` | Returns whether bit 0 is set in the observed global flag. |
| `0x004020c0` | `HandleArrayReleaseAndReset` | Releases each nonzero handle in the observed global array, then clears two associated globals. |
| `0x00402a40` | `CoordinatePairCallbackDispatch` | Packages the coordinate pair with a zero value and dispatches the fixed callback. |
| `0x00402aa0` | `ArgumentCallbackDispatch` | Dispatches the supplied argument through the fixed callback. |
| `0x00402b30` | `CoordinateCallbackDispatch` | Packages the coordinate pair and dispatches the fixed callback. |
| `0x00402b60` | `CoordinateCallbackDispatchByMode` | Dispatches one of two fixed callbacks according to whether the supplied mode is zero. |
| `0x00402ec0` | `EntityConditionalTaskSchedule` | Checks object predicates and flags, allocates and links a task record, marks an entity flag, and schedules event code 0x11. |
| `0x004030b0` | `CoordinatePairEventQueue` | Allocates a record containing two coordinates and the supplied event data, then queues event kind 8. |
| `0x00403190` | `ObjectEventType2Queue` | Allocates a record, stores the supplied value, updates two record flags, schedules event kind 2, and returns 1. |
| `0x004033c0` | `ObjectAttachedRecordAllocate` | Allocates a record and stores it in an existing attached-record slot or the entity field at +0x2f0. |

Direct pseudocode and caller context are in the full corpus. Direct evidence records for the coordinate command helpers are documented in finding 359; no matching direct bundle records were located for the callback/event wrappers in this set.

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/engine-region.c). Field, flag, and callback semantics remain at the operation level where callers do not establish more.
