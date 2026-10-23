# Finding 310 — Terrain-Cell Flag and State Tests

Fourteen terrain helpers now have literal names grounded in their bit tests and callers. The numeric masks are preserved because their broader game meanings are unresolved.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x432b00 | TerrainCellFlag800TestInBounds | Checks grid bounds and primary flag bit `0x800`. |
| 0x432c20 | TerrainCellFlag40000Test | Tests primary flag mask `0x40000`. |
| 0x432c50 | TerrainCellTypeSpeedClassGet | Returns low flag bits plus one, except when mask `0x200000` is set; callers use the result for speed scaling. |
| 0x432dd0 | TerrainCellFlag08000000Test | Tests primary flag mask `0x08000000`. |
| 0x432e60 | TerrainCellFlagsMaskC0Test | Tests whether either bit in mask `0xc0` is set. |
| 0x432e90 | TerrainCellFlag04000000Test | Tests primary flag mask `0x04000000`. |
| 0x432ec0 | TerrainCellStateEquals1 | Tests whether the terrain-cell state byte equals 1. |
| 0x432f40 | TerrainCellFlagsMask18ClearInBounds | Returns true out of bounds; in bounds, requires both bits in mask `0x18` to be clear. |
| 0x432f90 | TerrainCellFlag4TestInBounds | Checks grid bounds and primary flag bit `0x4`. |
| 0x432fd0 | TerrainCellFlag2000Test | Tests primary flag mask `0x2000`. |
| 0x433000 | TerrainCellFlag80Test | Tests primary flag mask `0x80`. |
| 0x433030 | TerrainCellFlag8000Test | Tests primary flag mask `0x8000`. |
| 0x433060 | TerrainCellSecondaryFlagBit0Test | Tests bit `0x1` in the secondary flag word. |
| 0x4330e0 | TerrainCellFlagsMask180000Test | Tests whether either bit in mask `0x180000` is set. |

Bodies and caller paths are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Callers use these results in placement, movement, terrain interaction, and cell-eligibility paths. No matching address-specific retained evidence records were found. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
