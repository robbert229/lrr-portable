# Finding 314 — Terrain Grid, Record, and Flag Helpers

Three routines expose a terrain-record byte, terrain-cell flag operation, and grid-resource cleanup behavior. Numeric flags remain unresolved.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x433120 | TerrainCellFlag10000000SetOrClear | Sets or clears primary cell flag `0x10000000` at the selected grid coordinate. |
| 0x4332c0 | TerrainCellRecordByte3Get | Returns the byte at terrain-cell record offset `+3`; callers use it as a directional-vector index. |
| 0x434530 | TerrainGridEntryResourcesReleaseAll | Iterates the grid dimensions and releases each non-null resource pointer stored in the grid array. No direct caller was resolved. |

Bodies and caller paths are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). No matching direct-function retained records were found for these routines. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
