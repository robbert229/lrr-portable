# Finding 307 — Terrain-Cell Flag Operations and Refresh

Eleven routines operate on terrain-cell flag words and refresh cell state. Flags remain numeric because their game meanings are unresolved.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x432360 | TerrainCellPrimaryFlagsMaskClearAndRefresh | Clears masks `0xdfef7bff` and `0x4` in the two cell flag words, then refreshes the cell. |
| 0x4323c0 | TerrainCellSecondaryFlagBit0Clear | Clears bit `0x1` in the secondary flag word. |
| 0x4323f0 | TerrainCellFlagsSetAndRefresh | Sets `0x108000` in the primary flags and bit `0x4` in secondary flags, then refreshes. |
| 0x432450 | TerrainCellFlagsSetMask100400 | Sets primary mask `0x100400` and refreshes. |
| 0x432490 | TerrainCellFlagsSetMask20108000 | Sets primary mask `0x20108000` and refreshes. |
| 0x4324d0 | TerrainCellSecondaryFlag8SetOrClear | Sets or clears bit `0x8` in secondary flags. |
| 0x432520 | TerrainCellSecondaryFlag8Test | Tests bit `0x8` in secondary flags. |
| 0x432560 | TerrainCellFlag40000000ClearOrSet | Sets or clears primary bit `0x40000000` and refreshes. |
| 0x4325d0 | TerrainCellSecondaryFlagBit0Set | Sets bit `0x1` in secondary flags. |
| 0x432710 | TerrainCellFlag80000ClearOrSet | Sets or clears primary bit `0x80000`. |
| 0x432d90 | TerrainCellFlag20TestInBounds | Checks grid bounds, then tests primary bit `0x20`. |

Bodies and caller paths are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c); callers apply the operations during terrain-coordinate, activity, route, and cell-state updates. No matching retained direct-function evidence records were located for these exact entries. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
