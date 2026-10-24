# 451 — Terrain cell state, occupancy, neighbors, and traversal

*Target: `LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), Ghidra 12.1.4 via REA.*

This finding maps the 101 functions assigned to `terrain-cell-state`. They operate on terrain grid cells, linked per-cell records, map tracking bytes, update queues, and two classes of flag words. The names below follow the current function annotations; where the binary exposes only bit tests or numeric states, this finding records those operations without assigning an unsupported gameplay meaning.

## Cell records, occupancy, and traversal

| Address | Function | Observed responsibility |
| --- | --- | --- |
| `00402ae0` | `TerrainCellRefreshDispatch` | Dispatch the coordinate callback through `00406540`, then reset the cell's surface type with `0044f570`. |
| `00403810` | `TerrainCellRecordCountUpdate` | Update per-cell resource/record counts for a supplied cell and record kind. |
| `00403910` | `CellRecordCountsPruneForEntity` | Remove an entity's contribution from the tracked per-cell counts. |
| `00409cd0` | `CellCardinalNeighborApply` | Apply the callback to the four cardinal neighbors around a cell. |
| `0040ab30` | `GridRecordFlagMaskClear` | Clear selected bits in the global grid-record flag word. |
| `0040af90` | `TerrainCellRecordGet` | Resolve a coordinate to its terrain cell record and return requested record fields. |
| `0040b250` | `CellFlag4IsSet` | Test primary cell flag `0x4`. |
| `0040b2a0` | `CellFlag100Set` | Set primary cell flag `0x100` for a coordinate. |
| `0040b2e0` | `CellFlag100IsSet` | Test primary cell flag `0x100` for a coordinate. |
| `0040b5f0` | `TileModelEntityRegister` | Allocate/register the terrain-grid companion structures used to track tile/model entities and their per-cell records. |
| `0040b850` | `PackedGridCellValueGet` | Read a packed cell value from the grid descriptor's cell array. |
| `0040c750` | `TerrainPointObjectCreate` | Create a scene object at a terrain point using the terrain height query and supplied object parameters. |
| `0040c830` | `TerrainPointObjectsRemoveMatching` | Walk the terrain point-object collection and release entries matching the requested filter. |
| `0040ce80` | `MapCellStateBufferRebuild` | Rebuild the map-cell state buffer from the level's cell records. |
| `0040cea0` | `TerrainCellAndNeighborsUpdate` | Run `CellInitPass` (`0042f6f0`) for the target and each in-bounds cardinal neighbor. |
| `0040cf80` | `GridCellObjectLinkInsert` | Insert an allocated link into the per-cell object/record chain. |
| `0040d030` | `TerrainRecordPoolChunkAllocate` | Allocate and attach a new chunk to the terrain cell-record pool. |
| `0040d0a0` | `TerrainEntityCellRecordRemove` | Remove an entity's terrain-cell record links and update the associated record chain. |
| `0040d240` | `TerrainCellHasOccupiedOrFlaggedNeighbor` | Check nearby cells for occupancy or a qualifying cell flag; bounds are checked before reading. |
| `0040d450` | `FUN_0040d450` | Initialize several callback walks by calling `0040d490` and registering two additional callbacks. |
| `0040d490` | `FUN_0040d490` | Walk the registered record arrays and invoke the supplied callback for active records. |
| `0040d4f0` | `TerrainCellActivationTry` | Validate the target and mapped cell, then run activation and, on success, start neighbor flood traversal. |
| `0040d850` | `TerrainCellNeighborFloodWalk` | Recursively traverse four cardinal neighbors, mark visited cells, gate propagation with random draws, and emit directional edge effects. |
| `0040e460` | `GridCellRecordPresentCheck` | Report whether a cell coordinate has an associated record. |
| `0040ee00` | `CellCoordinateTimestampRemember` | Record a coordinate in the timestamp/deduplication structure. |
| `0040efb0` | `CellCoordinateTimestampExists` | Check whether a coordinate is already present in the timestamp/deduplication structure. |
| `004198f0` | `TerrainCellLinkedEntriesRemove` | Remove linked terrain-cell entries and release their associated payloads. |
| `00424600` | `TerrainCellFlagCleanup` | Clear cell-associated state during cleanup, including terrain point/effect objects where present. |
| `0042c5b0` | `TerrainCellByteStateClassify` | Classify a cell's state byte into a small return category; exact domain labels are unknown. |
| `0042f2e0` | `TerrainCellEdgeFlagSetAndUpdate` | Set the edge-related state for a cell and invoke the cell initialization/update pass. |
| `0042f350` | `TerrainNeighborCoordinateCheck` | Check a candidate neighbor coordinate and optionally return the qualifying direction/coordinate data. |
| `0042f6f0` | `CellInitPass` | Coordinate a multi-stage cell initialization/refresh pass across neighboring systems and flags. |
| `004302b0` | `MapCellTrackingFlagsClearAndRefresh` | Clear selected map-cell tracking flags and request the associated map-state refresh. |
| `00430320` | `TerrainCellWorkFlagClear` | Clear terrain work flags for a coordinate and update dependent map tracking state. |
| `00430470` | `TerrainNeighborFlagSet` | Set a neighbor flag selected by the supplied relative direction. |
| `00430530` | `TerrainCellMutation` | Apply a cell mutation, update neighbor work flags, and trigger dependent refresh/cleanup actions. |
| `00430df0` | `TerrainNeighborRefresh` | Walk the scheduled neighbor cells and dispatch their terrain refresh work. |
| `00430ee0` | `TerrainCellInitialize` | Initialize a cell's terrain state and surface data, then mark it for refresh. |
| `004310f0` | `TerrainCellFlag20ClearAndRefresh` | Clear primary flag `0x20` and refresh the affected neighbors. |
| `00431140` | `TerrainCellQueuedWorkTransition` | Transition a cell through queued work states and submit the associated terrain operation. |
| `00431450` | `TerrainNeighborFlagRefresh` | Propagate/refresh neighbor flags and update the related scene/terrain records. |
| `00431580` | `TerrainNeighborWorkFlagScan` | Scan neighboring coordinates for pending work flags and append matching cells to the work set. |
| `00431780` | `TerrainStampImpactHandler` | Apply terrain stamp/impact behavior at a cell, including surface/edge queries and positional feedback. |
| `004319b0` | `TerrainCellMaterialGet` | Return the cell's material/type byte. |
| `00431ab0` | `AnyObjectCanOccupyTerrainCell` | Test whether the supplied object category is eligible to occupy the target terrain cell. |
| `00432100` | `TerrainCellFlag100SetAndTrack` | Set flag `0x100` and add the cell to the corresponding tracking list. |
| `00432170` | `TerrainCellFlag100Test` | Test flag `0x100` on a previously resolved cell record. |
| `004321a0` | `TerrainCellUpdateQueueFlush` | Drain the terrain-cell update queue and dispatch each queued update. |
| `00432200` | `TerrainCellPreviousStateTrack` | Snapshot/track the previous cell-state value for later comparison. |
| `00432270` | `TerrainCellVisitedFlagSetAndTrack` | Set the visited flag and append the cell to the list used for clearing visited state. |
| `004322d0` | `TerrainCellVisitedFlagTest` | Test the cell's visited flag. |
| `00432300` | `TerrainCellVisitedFlagsClearAll` | Clear visited flags for cells remembered in the visited-cell list. |
| `00432360` | `TerrainCellPrimaryFlagsMaskClearAndRefresh` | Clear a primary-flag mask and dispatch any required refresh. |
| `004323c0` | `TerrainCellSecondaryFlagBit0Clear` | Clear bit 0 of the cell's secondary flag word. |
| `004323f0` | `TerrainCellFlagsSetAndRefresh` | Set requested primary flags and refresh the cell/neighbors. |
| `00432450` | `TerrainCellFlagsSetMask100400` | Set or clear primary-mask bits `0x100` and `0x400` for the coordinate. |
| `00432490` | `TerrainCellFlagsSetMask20108000` | Set or clear primary-mask bits `0x20108000` for the coordinate. |
| `004324d0` | `TerrainCellSecondaryFlag8SetOrClear` | Set or clear bit `0x8` in the secondary flag word. |
| `00432520` | `TerrainCellSecondaryFlag8Test` | Test secondary flag bit `0x8`. |
| `00432560` | `TerrainCellFlag40000000ClearOrSet` | Clear or set primary flag `0x40000000`. |
| `004325d0` | `TerrainCellSecondaryFlagBit0Set` | Set bit 0 of the cell's secondary flag word. |
| `00432600` | `TerrainCellVariantFlagPropagate` | Propagate the cell-variant flag to related cells and request dependent refresh work. |
| `00432710` | `TerrainCellFlag80000ClearOrSet` | Clear or set primary flag `0x80000`. |
| `004329d0` | `TerrainCellFlag8CheckIfInBounds` | Bounds-check a coordinate before testing its cell flag `0x8`. |
| `00432b00` | `TerrainCellFlag800TestInBounds` | Bounds-check a coordinate before testing primary flag `0x800`. |
| `00432b90` | `TerrainCellTypeIs8Or10Or11` | Test whether the cell type byte equals 8, 10, or 11. |
| `00432c20` | `TerrainCellFlag40000Test` | Test primary flag `0x40000`. |
| `00432c50` | `TerrainCellTypeSpeedClassGet` | Resolve the cell's speed class from its type/state. |
| `00432c90` | `TerrainCellStateUpdate` | Reconcile cell state with linked entries, terrain height, and dependent object/surface state. |
| `00432d90` | `TerrainCellFlag20TestInBounds` | Bounds-check a coordinate before testing primary flag `0x20`. |
| `00432dd0` | `TerrainCellFlag08000000Test` | Test primary flag `0x08000000`. |
| `00432e00` | `TerrainCellPredugEdgeWallFlagSet` | Set the predug edge/wall state for the cell. |
| `00432e60` | `TerrainCellFlagsMaskC0Test` | Test whether any bits in primary mask `0xc0` are set. |
| `00432e90` | `TerrainCellFlag04000000Test` | Test primary flag `0x04000000`. |
| `00432ec0` | `TerrainCellStateEquals1` | Test whether the cell state byte equals 1. |
| `00432f00` | `TerrainCellStateEquals6` | Test whether the cell state byte equals 6. |
| `00432f40` | `TerrainCellFlagsMask18ClearInBounds` | Bounds-check a coordinate and test the relevant primary mask `0x18`. |
| `00432f90` | `TerrainCellFlag4TestInBounds` | Bounds-check a coordinate before testing primary flag `0x4`. |
| `00432fd0` | `TerrainCellFlag2000Test` | Test primary flag `0x2000`. |
| `00433000` | `TerrainCellFlag80Test` | Test primary flag `0x80`. |
| `00433030` | `TerrainCellFlag8000Test` | Test primary flag `0x8000`. |
| `00433060` | `TerrainCellSecondaryFlagBit0Test` | Test bit 0 of the secondary flag word. |
| `004330e0` | `TerrainCellFlagsMask180000Test` | Test whether primary mask `0x180000` intersects the cell flags. |
| `00433120` | `TerrainCellFlag10000000SetOrClear` | Set or clear primary flag `0x10000000`. |
| `00433180` | `CellTypeSpeedMultiplierResolve` | Resolve a type-specific terrain speed multiplier, with entity/state gates and a configured rock-fall override. |
| `004332c0` | `TerrainCellRecordByte3Get` | Return byte 3 from the cell record. |
| `004332f0` | `TerrainCellWorkFlagSet` | Set the terrain work flag for a coordinate. |
| `00433330` | `TerrainCellTypeGetAtCoords` | Read the cell type at a coordinate and optionally return associated data. |
| `00433380` | `TerrainCellMutationProgress` | Advance a cell mutation/progress value and update the visible surface representation. |
| `00433be0` | `CurrentLevelTerrainGridPick` | Adapt the current level's terrain grid to the world-to-cell pick helper. |
| `00433e30` | `GridCellEntityEligibilityCheck` | Check grid-cell entity eligibility and its associated placement constraints. |
| `00433e80` | `TerrainProjectedCellStateRefresh` | Recompute projected cell state and queue surface changes for cells affected by the current view/state. |
| `00434480` | `CellSurfaceUpdateDispatch` | Dispatch the surface update selected by cell state and supplied update parameters. |
| `00434570` | `TerrainCellProgressUpdate` | Update a cell progress value and return whether the operation reached its completion condition. |
| `00435010` | `TerrainCellWorkFlagCircleSet` | Set terrain work flags over a bounded circle around the supplied cell. |
| `004350a0` | `SurfaceTypeDescriptionsConfigLoad` | Load localized/configured surface-type descriptions and their indexed values. |
| `00435170` | `TerrainSurfaceTypeNameGet` | Return a configured surface-type name, falling back to the missing-config string. |
| `00435190` | `SurfaceTypeConfiguredValueAtIndex` | Return a configured value for the requested surface-type index. |
| `00435230` | `RandomVariantCellScatter` | Sample candidate cells around a center, skip tried cells, test occupancy, and apply eligible variants. |
| `00435300` | `FUN_00435300` | Advance a global phase and scan cells to update per-cell surface state/type under runtime and cell-state gates; higher-level purpose remains unresolved. |
| `00435f28` | `TerrainGridBorderInsetGet` | Return the configured terrain-grid border inset used by neighbor/bounds calculations. |

## State flow and interpretation

The principal registration path allocates a terrain cell-record pool, inserts links by row-major cell coordinate, and updates/removes those links as entities or terrain states change. Occupancy/placement checks consult the cell's current material/state and related object eligibility. The traversal path marks visited cells, scans the four cardinal neighbors, and uses a visited-list clear pass to restore reusable state. The flood walker at `0040d850` is recursive and may emit per-edge effects; its caller/edge helper chain and random continuation gate are described in findings 242 and 249.

A separate update path sets/clears map tracking and work flags, scans adjacent cells, and dispatches queued work through `CellInitPass` (`0042f6f0`), `TerrainCellMutation` (`00430530`), `TerrainNeighborRefresh` (`00430df0`), and `TerrainCellStateUpdate` (`00432c90`). The primary per-cell flags are a 32-bit word; other helpers access a secondary word and a state/type byte. Accessors in the table expose exact masks and equality tests, but their names alone do not establish what every flag means. The count and variant/scatter helpers connect this state to resource bookkeeping and configured surface descriptions.

## Related findings, limits, and follow-ups

Several important slices already have focused evidence: [79](79-terrain-voxel-mutation-stages.md), [87](87-other-voxel-states-refresh-neighborhood-flags.md), [88](88-neighbor-scan-consumes-terrain-work-flags.md), [223](223-cell-init-reads-outside-grid-at-top-and-left-edges.md), [242](242-path-step-terrain-and-pending-record-helpers.md), [249](249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md), [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md), [253](253-terrain-scatter-erosion-propagation-cell-queries-menu-records-and-buffer-helpers.md), [255](255-visual-channel-relays-resource-tree-callback-walks-scene-pose-and-registry-helpers.md), [256](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md), and [379](379-terrain-ui-and-crt-support-helpers.md). Together these distinguish direct helper behavior from higher-level interpretations.

This is static analysis of the shipped executable. Flag labels are numeric unless a narrow behavior is directly established; several compact predicates and the exact lifecycle and callers for `FUN_0040d450`/`FUN_0040d490`, and the purpose of `FUN_00435300`, need more context before assigning domain names. Useful follow-ups are a focused bitfield ownership table (writers/readers per flag), and a boundary audit of coordinate helpers that call into neighbor/update passes.

## Evidence

The complete function bodies are in [`LEGORR.full.c`](../decompiles/full/LEGORR.full.c). Focused terrain helper evidence is retained in [`legorr-engine-helper-audit-terrain-expansion-8.bundle`](../evidence/legorr-engine-helper-audit-terrain-expansion-8.bundle), [`legorr-engine-helper-audit-terrain-expansion-9.bundle`](../evidence/legorr-engine-helper-audit-terrain-expansion-9.bundle), and the bundles cited from the related findings above. Target digest and Ghidra/REA versions are recorded with those evidence records.
