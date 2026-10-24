# 453 — Object search and terrain placement eligibility

*Target: `LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), Ghidra 12.1.4 via REA.*

The 29 assigned procedures cover three connected jobs: matching object/action records, walking cell/path records to find nearby candidates, and deciding whether a terrain cell can be reached or occupied. The table records direct pseudocode behavior; gameplay meanings of several flags and descriptor fields remain unknown. Existing findings 242, 246, 249, 289, 312, and 381 retain deeper slices referenced below.

## Function map

| Address | Function | Observed responsibility |
| --- | --- | --- |
| `00404e00` | `ObjectPathSubmitToGridTarget` | Read the object's current grid position; if it differs from the target, obtain a route, submit the route points, and free temporary arrays. |
| `00404ed0` | `CheckObjectCandidate` | Special-case object type 6 with a high flag; otherwise send the object type/ID/owner tuple to `FindMatchingObjectRecord`. |
| `00404f10` | `FindMatchingObjectRecord` | Build a three-field type/ID/owner key and scan registered entities with `ObjectFilterMatchesActionTarget`; the routine itself returns no record. |
| `00404f50` | `ObjectFilterMatchesActionTarget` | Accept action-record types 5, 7, 9, and 15; for non-type-5 records with a nonzero referenced-object field, check that reference, then compare key fields only when field `+8` is nonzero, with ID `-1` acting as a wildcard. |
| `004091e0` | `FUN_004091e0` | Walk the global path-descriptor list and process each record according to descriptor bits `0x1`, `0x2`, and `0x4`; matching records update path/terrain state or are finalized. |
| `00409270` | `FUN_00409270` | Run a callback search for the supplied descriptor and return true when no matching object is found. |
| `00409350` | `PathRecordAllocateAndLink` | Allocate and zero a `0xd4`-byte path record, assign an incrementing ID and starting coordinate, and prepend it to the global list. |
| `004093b0` | `PassableAdjacentCellSearchExpand` | Recursively inspect four adjacent cells; mark cells that pass the terrain/flag tests and expand from those candidates. |
| `00409450` | `FindNearbyPlacementCandidate` | Call the recursive adjacent-cell entity search, then clear the visited-cell flags before returning its result. |
| `00409470` | `NeighborCellEntitySearchRecurse` | Search four cardinal neighbors for a matching entity; recursively expand into qualifying unvisited cells and stop on a match. |
| `00409550` | `FUN_00409550` | For a descriptor with the relevant state, resolve configured path/resource values, update its fields, and set descriptor bit `0x4`. |
| `00409790` | `CellPathRecordRegisterApply` | Allocate/link a path record, copy its cell-pair list, apply the list's height adjustment, then mark/register each cell and refresh associated state. |
| `00409940` | `RefreshEightNeighborCells` | Apply the cell mutation-progress update to the eight surrounding cells. |
| `00409f20` | `EntityPlacementCommandsBuildOnce` | If guard bit `+0x3ec/0x800` is clear, read the entity cell, query three kind-4 configured counts, call the two placement helpers with zero offsets when the global gate is clear, then set the guard bit. |
| `0040b1b0` | `DependencyGridOccupancyPass` | Scan the dependency table with nested loops (15 groups × 16 × 16 slots), call `FUN_0040aea0`, and update slot flag bits `0x4`/`0x8` from its result, with a special skip for one populated slot class. |
| `0040e1e0` | `CellCornerOccupancyScan` | Inspect the four corner cells around a position and report whether the cell-state predicate finds a blocker. |
| `00418920` | `CategoryCellCandidateListBuild` | Build the candidate list for a category by scanning eligible cell records and appending their coordinates. |
| `0041f740` | `CellFourNeighborWalkCheck` | Check four adjacent cells with occupancy and state-mask predicates, returning whether the requested walk condition holds. |
| `00431b20` | `PlacementBlockedAdjacentCellScan` | Validate the target cell's occupancy/flags and placement mode, then inspect cardinal neighbors for the disqualifying marker type. |
| `00431c70` | `FirstFreeNeighborCellPick` | Rotate through four cardinal neighbors and return the first coordinate accepted by the direct flag check or route/occupancy helper. |
| `00431da0` | `TerrainCellRouteAndOccupancyCheck` | Validate movement into a target cell using the entity's current position, occupancy, and terrain-state predicates. |
| `00432770` | `TerrainCellWalkabilityQuery` | Combine the cell walkability result with entity/type-specific speed or movement conditions. |
| `00432950` | `EntityMoveDestinationCellIsUsable` | Check whether a destination coordinate is usable for the supplied entity. |
| `00432a20` | `TerrainCellAllowsPlacement` | Check the target cell against the supplied placement category using the shared cell-state predicates. |
| `00432aa0` | `TerrainCellOccupancyOrFlagCheck` | Test cell occupancy or the requested cell flag/mode. |
| `00432b50` | `TerrainWalkableNeighborSearch` | Bounds-check the supplied coordinate and return that cell's flag bit `0x8`. Despite the annotation name, this helper does not iterate neighbors. |
| `00432bd0` | `TerrainCellWalkableFlagCheck` | Test the cell's walkability flags. |
| `00433090` | `TerrainCoordinateAndOccupancyCheck` | Validate coordinate bounds and return the cell's `0x20000000` flag state (the flag's semantic meaning is not established here). |
| `00433c10` | `EntityNeighborBlockedCellStepProbe` | Under entity/tick gates, inspect four cardinal neighbors for the blocked-step condition and trigger the corresponding entity response. |

## Search and placement flow

The object-record path is `CheckObjectCandidate` (`00404ed0`) → `FindMatchingObjectRecord` (`00404f10`) → `EntityPoolCallbackSearch` (`00406540`) → `ObjectFilterMatchesActionTarget` (`00404f50`). The filter accepts selected action kinds. For non-type-5 records with a nonzero referenced-object field, it checks the reference with `FUN_004385b0`; it compares key fields only when field `[8]` is nonzero, and ID `-1` is a wildcard. The compared fields are `[8..10]` and the three-field search tuple. It returns a predicate result; `FindMatchingObjectRecord` itself is void. `ObjectPathSubmitToGridTarget` (`00404e00`) is a separate route-submission path; the existing analysis did not resolve a direct static caller.

Path and nearby-cell searches use linked records and temporary visited flags. `PathRecordAllocateAndLink` (`00409350`) creates descriptors. `PassableAdjacentCellSearchExpand` (`004093b0`) recursively marks qualifying cardinal cells after `TerrainCoordinateAndOccupancyCheck`/`TerrainCellOccupancyOrFlagCheck`. `NeighborCellEntitySearchRecurse` (`00409470`) checks each cardinal neighbor for a matching entity, then recursively explores qualifying unvisited cells; `FindNearbyPlacementCandidate` (`00409450`) clears visited flags when the search returns. The movement/placement caller `FUN_0043c570` reaches the adjacent-search helpers. `CellPathRecordRegisterApply` (`00409790`), called by `BuildingPlacementPreviewUpdate`, copies a selected cell list, invokes the terrain height-adjustment helper, and updates each listed cell. The detailed recursive conditions are in findings 246, 249, and 289.

The placement path layers checks rather than using one universal predicate. `TerrainCoordinateAndOccupancyCheck`, `TerrainCellOccupancyOrFlagCheck`, and `TerrainCellAllowsPlacement` check coordinate, occupancy, and placement conditions; `TerrainCellWalkableFlagCheck` requires an in-bounds cell with flag `0x10` and without flag `0x8`. `TerrainCellRouteAndOccupancyCheck` combines that terrain state with entity and occupancy checks. `TerrainCellWalkabilityQuery` adds type/speed conditions. Higher-level callers include `PlacementBlockedAdjacentCellScan` from `BuildingPlacementFootprintRenderAndValidate`, `FirstFreeNeighborCellPick` from `PendingCommandEntityAssignmentTick`, `EntityNeighborBlockedCellStepProbe` from `PathStepAdvanceAndArrivalCheck`, and the cell-four-neighbor check used by route/placement logic. Some masks vary by caller and are kept numeric.

## Limits and follow-ups

This is static analysis of the shipped executable. It establishes the observed comparisons and call paths but not the domain meaning of every terrain flag, path-descriptor field, or object category. Focused follow-ups could map ownership of the placement-related flag masks and document the descriptor lifecycle from allocation through `FUN_004091e0` cleanup. See [242](242-path-step-terrain-and-pending-record-helpers.md), [246](246-entity-move-route-command-and-object-clone-helpers.md), [249](249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md), [289](289-statistics-and-adjacent-cell-helpers.md), [312](312-object-task-and-registry-helpers.md), and [381](381-selection-resource-runtime-and-grid-helpers.md) for narrower findings and caller evidence.

The complete pseudocode is in [`LEGORR.full.c`](../decompiles/full/LEGORR.full.c). The linked findings retain their cited REA evidence bundles and call-site context.
