# Finding 369 — Task Queue and Entity State Helpers

This pass names task-record queue and cleanup operations, a pointer stack, and task/global accessors. Task kinds and payload meaning remain unspecified.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x004034e0` | `TaskPairRecordEnqueue` | Allocates a task record, copies two dwords to offsets +8/+0xc, sets subtype 0x18, and queues the record. |
| `0x00403650` | `EntityTaskBatchAppend` | For each entity in the supplied array, allocates a task record, stores the parameter and zero value, runs an entity helper, and appends the task. |
| `0x004040a0` | `EntityTaskQueueFlush` | When the global mask permits, processes the entity queue, conditionally processes event code 9 for the observed flag/kind combination, then runs two cleanup helpers. |
| `0x00403af0` | `EntityTaskStateUpdateAfterCleanup` | Clears a referenced task flag for the zero case; otherwise may pop a kind-zero task and dispatch a fixed event for entity kind 3. |

Direct pseudocode and caller context are in the full corpus. No matching direct evidence-bundle records were located for these exact procedure entries.

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/engine-region.c). Numeric record kinds remain literal where semantics are unresolved.
