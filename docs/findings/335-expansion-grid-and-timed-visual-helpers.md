# Finding 335 — Expansion-Grid and Timed-Visual Helpers

Three helpers record an expansion entity in a grid cell, check neighboring cell flags for its placement, and advance/fade timed visual segments. Raw terrain masks remain literal.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x466380 | SetExpansionEntityAtGridCell | Stores the entity pointer in the cell record, sets raw cell mask `0x100`, and writes `100.0` into the record. |
| 0x4663f0 | CanSpawnExpansionEntityAtGridCell | Checks current and neighboring cell flags and returns an output angle when the pattern matches. |
| 0x465e00 | AdvanceTimedVisualSegmentAndFade | Advances position values by velocity and delta, expires the segment at its distance limit, or reduces opacity and updates the visual. |

Caller `0x466250` creates an `Expand` entity and uses the two grid helpers; caller `0x465b00` advances active visual segments. Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). No matching direct retained procedure records were found; the visual subsystem’s exact effect type remains uncertain. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
