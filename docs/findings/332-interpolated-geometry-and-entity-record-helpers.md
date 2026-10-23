# Finding 332 — Interpolated Geometry and Entity-Record Helpers

Three helpers test an interpolated coordinate, check a literal entity mask with a shared eligibility predicate, and fetch a dword from an indexed pair-slot table.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x471330 | InterpolatedZWithinBounds | Computes `z0 + deltaZ × t` and checks whether the result lies between `upper - width` and `upper`. |
| 0x471a20 | ReadIndexedPairSlotDword | Reads a dword at `base + index × 8 + component × 4`. |
| 0x471ce0 | EntityFlag100AndSharedEligibilityCheck | Requires entity mask `0x100` and a successful shared eligibility check. |

Bodies and available callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). No matching direct retained analysis records were found. The indexed table’s field meaning and entity mask `0x100` remain unresolved. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
