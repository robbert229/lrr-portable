# Finding 313 — Selection, Resource, and Transform Helpers

Three routines now have behavior-based names grounded in their direct bodies and caller use.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x429160 | TerrainCellPaletteResourceAtGridPosition | Computes a grid-cell index, reads its byte, and resolves a resource through a global palette table. Object creation uses the result to assign a resource. |
| 0x4295c0 | EntityMatchesCurrentSelectionOrOwner | Checks whether an entity or its nonzero owner matches the current selection, subject to the current-mode check. |
| 0x4247d0 | ApplyDefaultTransformToFlaggedRenderObject | Checks object bit 0 at offset `+0x3ec`, then calls the outline/transform helper with fixed default values. |

Bodies and callers are in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Direct records for the palette lookup and selection/owner predicate are in [legorr-engine-helper-audit-low.bundle](../evidence/legorr-engine-helper-audit-low.bundle); the transform helper appears in [legorr-engine-helper-audit-low-expansion-5.bundle](../evidence/legorr-engine-helper-audit-low-expansion-5.bundle). The transform wrapper relationship is visible in the full corpus even though the direct evidence record reported no callers. Static analysis target: `reference/LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
