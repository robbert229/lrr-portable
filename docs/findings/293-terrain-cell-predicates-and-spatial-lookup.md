# Finding 293 — Terrain Cell Predicates and Spatial Lookup

Six anonymous helpers now have operation-level names. Numeric cell types and flag meanings remain explicit where the source does not identify their higher-level purpose.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x432a20 | TerrainCellAllowsPlacement | Checks cell bounds, flag `0x10` set, flag `0x20` clear, and excludes cell types 8, 10, and 11. |
| 0x431ab0 | AnyObjectCanOccupyTerrainCell | Iterates candidate objects and returns true when any object passes the placement predicate at the supplied coordinates. |
| 0x432b90 | TerrainCellTypeIs8Or10Or11 | Returns whether the cell type byte is exactly 8, 10, or 11. A movement path uses this test. |
| 0x433330 | TerrainCellTypeGetAtCoords | Bounds-checks coordinates and returns the terrain cell type byte. |
| 0x4329d0 | TerrainCellFlag8CheckIfInBounds | Checks grid bounds, then reports whether flag bit `0x08` is set. |
| 0x46e8a0 | WorldSpatialRecordFindAtGridCoordinate | Scans counted records, compares two converted coordinates, and returns a matching record and index. |

Bodies and caller context are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). The placement predicate and candidate-object scan are used in placement-command checks; the type test is used in movement collision logic; the type getter is used by terrain-view code; and the bit-8 check is used in a type-13 path. No matching retained direct-function records were found for this set. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
