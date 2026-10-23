# Finding 304 — Terrain Visited-Cell Tracking

Two routines manage a terrain-cell visited bit and its coordinate list.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4322d0 | TerrainCellVisitedFlagTest | Returns whether cell flag bit `0x02` is set. Recursive neighbor search uses it to avoid revisiting a cell. |
| 0x432300 | TerrainCellVisitedFlagsClearAll | Walks the tracked coordinate list, clears bit `0x02` on each cell, and resets the list count. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Caller evidence is retained in [legorr-engine-helpers-expansion-27.bundle](../evidence/legorr-engine-helpers-expansion-27.bundle), record `ev_5ded34f9759671489f31901c479e6f4a16faf768640de2f39b1fb090323bd427`; no matching direct-function records were found for these two addresses. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
