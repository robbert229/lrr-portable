# Finding 325 — Terrain-Cell Refresh and Grid Pick

Two routines clear a terrain-cell mask with a refresh and forward grid-pick arguments through the current level’s terrain context.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4310f0 | TerrainCellFlag20ClearAndRefresh | Refreshes neighbors, clears cell mask `0x20`, and updates the terrain cell in mode 0. |
| 0x433be0 | CurrentLevelTerrainGridPick | Calls `TerrainGridPick` on the terrain object at `DAT_005560d4 + 4`, forwarding all four arguments. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). No matching direct retained evidence records were found. The bit meaning remains unknown. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
