# Finding 284 — Movement, Lighting, and Action Gates

Three anonymous routines have names supported by direct pseudocode and caller behavior.

| Address | Name | Observed behavior and uncertainty |
| --- | --- | --- |
| 0x44cc10 | LightEffectFadeParametersStore | Stores fade colors and timing values in globals, then sets a pending flag. Its caller reads the matching `LightEffects` configuration fields. |
| 0x432950 | EntityMoveDestinationCellIsUsable | Checks entity state and destination-cell bounds, flags, and excluded cell types before movement. The caller uses it before assigning a destination and starting movement. |
| 0x43fdc0 | EntityActionTargetCellIsValid | Checks entity and attached-object state, resolves the attached object’s map cell, and validates it through a specialized cell predicate. Command cases `0x16` and `0x17` use it as a gate. |

Bodies and caller paths are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). The exact `0x432950` caller analysis is retained in [legorr-sim-terrain-fourth-pass-2026-10-23.bundle](../evidence/legorr-sim-terrain-fourth-pass-2026-10-23.bundle) (`ev_480d3088028e61342fd1740a35c0b7a03e989d58c49d8eb9b1a702b21617e9cb`). The light-effect setter appears in the address inventory of [legorr-engine-helper-audit-render-expansion-9.bundle](../evidence/legorr-engine-helper-audit-render-expansion-9.bundle), but that record does not establish its semantic role; the role follows from `0x44c9c0` and its direct body. No matching direct semantic record was found for `0x43fdc0`. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
