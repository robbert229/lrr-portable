# Finding 445 — Terrain Scene Geometry

This pass maps the 12 assigned routines that stage, select, emit, and update terrain scene geometry. The write-up stays close to checked-in pseudocode; numeric cell types, render flags, and staged-record fields are left literal where their broader meaning is not established.

| Address | Name | Observed behavior | Evidence and flow |
| --- | --- | --- | --- |
| `0x0046db90` | `CellQuadRunStage` | Scans the grid row by row and accumulates contiguous cells whose record byte is 7 into staged row spans, flushing a span when a later non-7 cell is encountered. It merges/reuses staged records where possible, sorts and trims the record list, then calls candidate selection and run emission. The loop body has no explicit flush for a span that reaches the row's right edge. | Body pseudocode; `MapLoadPipeline` caller and run-record context in finding 241. Cell type 7 remains unresolved. |
| `0x0046dd10` | `StagedCellCandidateSelect` | For each staged entry, clears its selected-candidate count and checks up to four coordinate pairs per source item; records the first in-bounds candidate whose cell byte is neither 7 nor 1. | Body pseudocode; called by `CellQuadRunStage`; overlapping detail in finding 250. |
| `0x0046de80` | `CellQuadRunEmit` | For each staged run, samples corner terrain heights, derives the run height floor and a flag from cell occupancy, creates a quad render entity, sets its texture/scale values, and emits four vertices for each run item. | Body pseudocode; called by `CellQuadRunStage`; geometry call context in finding 241. |
| `0x0046e1c0` | `GlobalEntityRenderParameterModeSet` | Iterates staged run entities and calls a render-parameter setter with zero values when the argument is zero, or fixed float constants otherwise. | Body pseudocode; parameter meanings remain unresolved. |
| `0x0046e220` | `CellSurfaceQuadHeightFill` | Finds the staged record containing a grid coordinate, marks its record flags, disables one render handle, sets fields in the selected per-record item, samples terrain corner heights, and stores the selected corner height. | Body pseudocode; lookup is through `WorldSpatialRecordFindAtGridCoordinate`. |
| `0x0046e330` | `GridSurfaceEntryReset` | Finds the staged record containing a grid coordinate, adjusts its flags, disables the associated render handle, and clears one field in the selected item. | Body pseudocode; lookup is through `WorldSpatialRecordFindAtGridCoordinate`. |
| `0x0046e390` | `FUN_0046e390` | Advances staged-run item values using the supplied delta, clamps two per-item values at 240, advances or lowers each run's stored height according to its flags and observed items, updates a render parameter, and rewrites vertex heights. It then calls `TerrainEdgeFlagMidpointUpdate`. | Body pseudocode; caller is the terrain/geometry update path identified in finding 244. The precise meaning of its flags and values remains unresolved. |
| `0x0046e610` | `TerrainEdgeFlagMidpointUpdate` | For each staged item, derives two edge-midpoint positions from its orientation and stored offsets, maps each position to a grid coordinate, then applies terrain-cell edge updates with flags 1 and 0. | Body pseudocode; adjacent geometry update context in finding 244. |
| `0x0046e8a0` | `WorldSpatialRecordFindAtGridCoordinate` | Scans staged records and their counted coordinate pairs, converts each pair to integer coordinates, and returns the containing record while optionally writing the matching item index. | Body pseudocode; used by `CellSurfaceQuadHeightFill` and `GridSurfaceEntryReset`; related spatial lookup in finding 293. |
| `0x0046e9d0` | `StagedCellCandidateCollect` | For a nonzero row, scans existing staged records for matching adjacent coordinate pairs; appends a record-pair link for each match where the record pointers differ, increments a shared count, and returns the last matched record pointer. No deduplication check is visible in the loop. | Body pseudocode; caller is `CellQuadRunStage`; overlapping detail in finding 251. |
| `0x0046ead0` | `AppendMapRowSpanToBuffer` | Appends `(column, row)` float pairs for a half-open column range and advances the buffer count. | Body pseudocode; used by both row-span creation and merge paths; finding 322. |
| `0x0046eb30` | `StartMapRowSpanBufferRecord` | Starts a new staged record, writes its initial row span through `AppendMapRowSpanToBuffer`, and increments the staged-record count. | Body pseudocode; fallback from `CellQuadRunStage`; finding 322. |

### State flow

`CellQuadRunStage` scans the map's cell-record grid, then uses `StagedCellCandidateCollect` to associate spans with existing staged records or `StartMapRowSpanBufferRecord`/`AppendMapRowSpanToBuffer` to create spans. After sorting and trimming the staged table, `StagedCellCandidateSelect` records usable per-item candidates and `CellQuadRunEmit` creates the render geometry. Later updates locate a staged record by grid coordinate through `WorldSpatialRecordFindAtGridCoordinate`; the two item helpers then update or clear item data. The animation/update path in `FUN_0046e390` adjusts stored heights and vertices before refreshing edge flags through `TerrainEdgeFlagMidpointUpdate`.

These call edges, field accesses, constants, and mutations are direct evidence from the decompile. Labels such as “terrain type,” “height animation,” or “edge state” are descriptive shorthand only; cell byte 7, per-record flags, and several float fields are not fully decoded here.

### Unresolved questions and follow-up slices

- Determine what cell byte values 7 and 1 represent in the map/terrain schema and why type 7 forms contiguous run spans.
- Check whether a type-7 span reaching the right edge is handled elsewhere: `CellQuadRunStage` flushes on a later non-7 cell, but its row loop shows no end-of-row flush.
- Trace the render-parameter setter and staged-record allocation/layout to name the render flags and the values advanced by `FUN_0046e390`.
- Follow the terrain-cell edge-update consumer to establish the observable effect of the paired flag writes.
- Compare emitted vertex positions with a live scene or captured mesh to validate the four-vertex update and row-span geometry.

Complete pseudocode is in [LEGORR.full.c](../decompiles/full/LEGORR.full.c). Related slices are findings [241](241-terrain-quad-run-upgrade-and-scene-slot-helpers.md), [244](244-config-file-text-layout-and-record-emit-helpers.md), [250](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md), [251](251-command-band-kind-dispatch-item-name-lookup-route-and-record-helpers.md), [293](293-terrain-cell-predicates-and-spatial-lookup.md), and [322](322-resource-binding-and-map-row-span-helpers.md).
