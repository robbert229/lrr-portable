# Finding 447 — Terrain Grid Path Search

This pass maps the three assigned route-search routines. It records their grid bounds, expansion order, visited/cost storage, callbacks, and output buffers directly from pseudocode; the meanings of the terrain predicate and per-cell cost modifier remain only partly resolved.

| Address | Name | Observed behavior | Evidence and caller context |
| --- | --- | --- | --- |
| `0x00440c30` | `GridRouteSearchAdapter` | Forwards its eight arguments to `TerrainNeighborPathCheck` and supplies null callback/context arguments. | Body pseudocode; called by `ObjectPathSubmitToGridTarget` and a simulation candidate route path. |
| `0x00440c70` | `TerrainGridPathSearch` | Bounds-checks the start and, when no callback is supplied, the target and target-cell predicate. It performs a breadth-first expansion over four cardinal neighbors using a reusable per-cell visited array and two alternating work queues. Neighbor acceptance is delegated to `FUN_00431da0`; a supplied callback can terminate the search at an accepted coordinate. On success it allocates parallel X/Y route arrays, reconstructs a path by following decreasing visit depths, and writes both arrays and their count through output pointers. | Body pseudocode; existing overview in finding 225; underlying cell predicate's dimensions/context in findings 225/226. |
| `0x004410f0` | `TerrainNeighborPathCheck` | Uses a separate reusable per-cell float map and two alternating FIFO queues to expand four cardinal neighbors, re-enqueuing a cell when a lower cost is found. It accumulates cost 1.0 per step, reduced to 0.5 when `FUN_00433090` accepts the cell one further step in the same cardinal direction beyond the candidate neighbor; the predicate checks that probe cell's `0x20000000` flag. Candidate cells are accepted through `FUN_00431da0`. It can stop via a supplied callback or when the target is reached, then reconstructs parallel coordinate arrays by following lower-cost neighboring cells. If the first reconstructed pair does not describe a cardinally adjacent step, it frees those arrays and retries through `TerrainGridPathSearch`. | Body pseudocode; called from entity routing/command paths summarized in findings 242, 246, 248, and 256. The semantic meaning of the probe flag and the half-cost rule remain unresolved. |

### State and data flow

`GridRouteSearchAdapter` provides a no-callback convenience route into `TerrainNeighborPathCheck`. The two search implementations share the four-cardinal-neighbor terrain predicate but use distinct reusable global maps: `TerrainGridPathSearch` stores visit depth in `DAT_004dee04`, while `TerrainNeighborPathCheck` stores accumulated float costs in `DAT_004dee00`. Both alternate fixed-size queues during expansion and return newly allocated parallel coordinate arrays plus a count through caller-owned output locations. Callers then pass the arrays to route setup and free them; examples include `ObjectPathSubmitToGridTarget`, `EntityNeighborCellMoveCommandHandle`, and other route command helpers.

The direct evidence is in control flow, array writes, allocator calls, and caller sites. “Breadth-first” describes the alternating frontier and unit-depth labels in `TerrainGridPathSearch`; `TerrainNeighborPathCheck` performs repeated cost relaxation over alternating FIFO queues, without extracting the lowest-cost queued cell. The flags and precise semantic distinction behind `FUN_00433090` remain open.

### Unresolved questions and follow-up slices

- Identify the terrain meaning of `FUN_00433090` and why its accepted neighbors receive half the accumulated cost.
- Trace `FUN_00431da0` and `CurrentLevelHeightGet` consumers far enough to name the terrain-cell eligibility rules and grid bounds.
- Each local work queue appears to hold 100 five-word records (500 words divided by five), with no visible capacity guard before record writes/count increments. Confirm the capacity behavior for large maps and allocation-failure handling for the paired route arrays; runtime impact is unconfirmed.
- Compare reconstructed paths and fallback behavior with live movement traces across blocked cells and equal-cost alternatives.

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Related findings include [225](225-terrain-objective-level-and-diagnostic-helpers.md), [226](226-omitted-simulation-terrain-resource-text-procedures.md), [242](242-path-step-terrain-and-pending-record-helpers.md), [246](246-entity-move-route-command-and-object-clone-helpers.md), [248](248-config-band-loaders-display-mode-directional-activity-and-flight-record-helpers.md), and [256](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md).
