# 452 — Lava erosion uses timed cell stages and neighbor propagation

*Target: `LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), Ghidra 12.1.4 via REA.*

The 13 assigned procedures form a delayed terrain-state pipeline. Level loading initializes interval state, seeds per-cell stages, and inserts coordinates into bounded tables. Engine tickers advance those timers, test cardinal and diagonal neighbors, mutate cells, and propagate to further candidates. Finding 81 documents the EmergeMap seed; finding 90 records the pending-cell trigger ticker. This finding joins those slices and adds the neighbor mutation and cryore-layer paths.

## Assigned function map

| Address | Function | Observed responsibility |
| --- | --- | --- |
| `0040e930` | `ErosionTimingStateInit` | Scale the trigger/erosion/lock intervals into tick units and clear the 2,000-entry stage-active and 1,000-entry timestamp tables. |
| `0040e990` | `FUN_0040e990` | Find and return the first free slot in the 2,000-entry stage-active table. |
| `0040e9c0` | `LavaErosionDelayFromCell` | Derive a delay from `6 - cell[+6]` multiplied by the configured erosion interval. |
| `0040ea10` | `LavaErosionStageRegister` | Store the supplied stage in cell byte `+7`, record coordinates and stage timer, mark the slot active, and add the cell to pending coordinates. |
| `0040eab0` | `LavaErosionPendingCellTick` | Accumulate trigger time, scan pending cells from a randomized start, gate and dispatch event `0x1e`, then decrement active stage timers and emit stage effects/events as values cross integer boundaries. |
| `0040ee50` | `LavaErosionPendingCellAdd` | Add a coordinate once to the bounded 2,000-entry pending list and set cell flag `0x80000000`; removal swap-compacts the list, clears the flag/active slot, and remembers the coordinate for the lock interval. |
| `0040f000` | `LavaErosionNeighborCheck` | Require source cell state 6, then scan four cardinal neighbors for unqueued, flagged, nonzero-stage cells not present in the timestamp table. |
| `0040f190` | `FUN_0040f190` | Check the target with `LavaErosionNeighborEligible`, inspect four cardinal neighbors, form a direction mask from qualifying neighbors, and call `LavaErosionNeighborMutate` when the mask and neighbor-state gates pass. |
| `0040f2b0` | `LavaErosionNeighborEligible` | Bounds-check and reject cells with flag `0x08000000` or excluded types (6, 7, 9); apply an additional flag gate when configured. |
| `0040f330` | `LavaErosionNeighborMutate` | Advance the cell state, call `TerrainCellWorkFlagClear` (`00430320`) with selector `0x2f`, map the supplied direction mask to an edge, apply the edge effect, set flags/sound when successful, and count repeated mutations before propagation/event dispatch. |
| `0040f5f0` | `ErosionEightNeighborPropagate` | Scan all eight surrounding coordinates; for in-bounds cells that pass free-cell and erosion-eligibility checks, recursively invoke the mutator. |
| `0042c480` | `LoadErodeCryoreLayers` | Load a dimension-matched layer, subtract the selected layer value from each cell type (mapping result 5 to 4), set stage byte 4 on type 6 cells, and enqueue coordinates of type 11. |
| `0042cb70` | `LavaErosionCellTimersAdvance` | For active levels, advance per-cell timers; when a cell-specific threshold expires, clear the timer, call the eligibility/mutation check, and dispatch event 6 on success. |

## State and call flow

`ErosionTimingStateInit` (`0040e930`) is called by the configuration loader `FUN_00429890` after it reads `ErodeTriggerTime`, `ErodeErodeTime`, and `ErodeLockTime` (defaults 10, 5, and 60 seconds in that loader). It scales those values and clears the active/timestamp records. The EmergeMap load path (`0042bf40`, documented in finding 81) stores derived voxel state and calls `LavaErosionStageRegister` for selected cells. Stage registration calls `FUN_0040e990` for a free active slot, obtains the cell-derived delay from `LavaErosionDelayFromCell`, stores the coordinate/stage timer, then calls `LavaErosionPendingCellAdd` to prevent duplicate pending entries. The timer ticker also calls `FUN_0040e990` and `LavaErosionDelayFromCell` while selecting and advancing pending stages.

The tick path has two related consumers. `LavaErosionPendingCellTick` (`0040eab0`) waits for the trigger interval, tests pending coordinates with `LavaErosionNeighborCheck`, and queues event `0x1e` for a qualifying coordinate while resetting its stage timer/state. It also advances per-stage timers and selects the effect variant when the integer stage changes. Separately, `LavaErosionCellTimersAdvance` (`0042cb70`) walks cells with a nonzero timer, advances elapsed time, and runs `FUN_0040f190`; successful checks dispatch event 6. Engine tick registration for both consumers is documented in [03-engine-tick](03-engine-tick.md).

Mutation flows through `FUN_0040f190` and `LavaErosionNeighborEligible` into `LavaErosionNeighborMutate`. The eligibility helper rejects out-of-bounds cells, cells with flag `0x08000000`, and cell types 6, 7, and 9; another flag gate applies when the grid context has the associated option set. The coordinator checks four cardinal neighbors, skips state-1 or flag-0x20 neighbors, builds a bitmask for considered directions, and only calls the mutator when its mask and remaining neighbor test pass. The mutator advances cell state, sends the direction mask through its direction-to-edge switch, increments a per-cell mutation counter, and once the configured threshold is reached calls `ErosionEightNeighborPropagate` (`0040f5f0`). That propagation checks each of eight neighbors before recursively mutating it. Findings 223 and 253 contain the detailed transition and recursive-neighbor evidence; finding 90 covers the candidate gate and timer-driven path.

## Limits and follow-ups

The evidence establishes numeric stage values, cell offsets, masks, and event IDs, but does not establish friendly names for every stage, the visible meaning of event `0x1e`, or the full gameplay meaning of the configured layer values. The candidate and mutation paths also share flags with other terrain operations. Useful follow-ups are to correlate the two erosion tickers with runtime cadence and to map all writers/readers of stage bytes `+6/+7` and the `0x80000000`/`0x10` flag bits.

This is static analysis of the shipped executable; no runtime erosion transition was observed. Further focused details and evidence are in findings [81](81-emergemap-seeds-delayed-lava-erosion-stages.md), [90](90-lava-erosion-ticker-advances-pending-cells.md), [223](223-cell-init-reads-outside-grid-at-top-and-left-edges.md), and [253](253-terrain-scatter-erosion-propagation-cell-queries-menu-records-and-buffer-helpers.md). They cite the relevant REA evidence bundles and call relationships.

## Evidence

The EmergeMap seeding and pending-cell timer evidence is retained in [`legorr-lava-erosion-focused-expansion.bundle`](../evidence/legorr-lava-erosion-focused-expansion.bundle). The neighboring mutation and propagation decompiles are in [`LEGORR.full.c`](../decompiles/full/LEGORR.full.c) and are summarized with focused evidence in findings 223 and 253. The target digest is recorded above.
