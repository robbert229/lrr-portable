# Finding 300 — Terrain-Queue and Particle-Slot Helpers

Four routines now have names grounded in direct state updates and their caller paths. Numeric flags remain explicit where their higher-level meaning is unknown.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4302b0 | MapCellTrackingFlagsClearAndRefresh | Clears two cell-record flags, refreshes related state, and updates the cell; its caller then sets the high tracking bit. |
| 0x432100 | TerrainCellFlag100SetAndTrack | Sets cell bit `0x100`, appends coordinates to a work queue, increments its count, and refreshes terrain. |
| 0x432270 | TerrainCellVisitedFlagSetAndTrack | Sets cell bit `0x02`, appends coordinates to a visited queue, and increments its count. |
| 0x465c50 | ParticleSlotInitializeForEmitter | Initializes a particle slot, creates and positions its render object, sets initial motion and randomized lifetime, and returns a variation value. |

Bodies and queue drain/caller paths are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). The particle initializer is called by emitter controller `0x465b00` when a slot is free and the emission timer expires; direct pseudocode is retained in [legorr-engine-helpers-expansion-22.bundle](../evidence/legorr-engine-helpers-expansion-22.bundle), record `ev_e0c58e5bb04ee83c0327854fd9e50249b5fc8c65c5e199787622b70bf41060d8`. No address-specific retained record was found for the three terrain helpers. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
