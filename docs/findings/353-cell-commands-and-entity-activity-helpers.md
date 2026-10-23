# Finding 353 — Cell Commands and Entity Activity Helpers

This pass names deferred entity work, cell-command record operations, and entity activity-state updates. Numeric command codes and record fields retain their observed values without guessing semantic labels.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x004026c0` | `EntityDeferredWorkSchedule` | When the entity fields permit it, queues a deferred worker; if enqueueing fails, directly calls the worker. The entity field and kind-selected record semantics are unresolved. |
| `0x00402e40` | `CellCommandType13Issue` | Checks the cell coordinates, allocates and links a command record carrying code 0xd, then dispatches that code to the cell. |
| `0x00402fb0` | `CellCommandType11Issue` | Allocates and links a command record carrying code 0xb, dispatches it to the cell, and invokes an adjacent coordinate helper. |
| `0x00403030` | `CellCommandType3Issue` | Checks cell predicates, allocates and links a command record, then dispatches code 3 and forwards a returned value to another helper. |
| `0x00403b40` | `EntityPendingTaskFlagClear` | Clears bit 0 at offset 0x5c of the first record referenced by the entity. |
| `0x00403b60` | `EntityActivityRecordsRefresh` | When the observed entity flags and field gate pass, visits a fixed list of activity codes and calls the activity-record helper for each. |

Direct pseudocode evidence for `0x004026c0` is in `legorr-ui-render-audit-2026-10-23.bundle` (`ev_6a3aeb4f859f6d3a6ac8545c39bbc1795914422bbacd4eec89b86da756cb1237`). Caller context is in `legorr-engine-helper-audit-low-expansion-6.bundle` (`ev_7591fa8a4540d933717c05c043b2a1a0239fae2ffe1dae3045c0cf3433760b0a`). Other listed bodies and callers are supported by checked-in pseudocode; no matching direct records were located.

The complete checked-in pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); named excerpts are in [engine-region.c](../decompiles/engine-region.c). Names preserve unresolved command codes, fields, flags, and interface semantics.
