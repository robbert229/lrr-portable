# Finding 238 — Reward, Terrain, and Simulation Helpers

Direct code and caller context resolve eleven more anonymous procedures. Names
describe the observed work; command flags, task codes, and several record fields
remain unnamed where the code does not establish their meaning.

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x460950 | CheckpointBlobTaskProgressRecalculate | Recomputes per-category progress percentages in the checkpoint blob, accumulates weighted progress, bounds it to the available total, and adds the level's base completion contribution. Called by `CheckpointBlobTaskProgressRefresh`. |
| 0x460f00 | RewardStatsPanelDraw | Formats reward statistics, lays out the panel from current screen dimensions, selects a reward label by result code, and draws the summary. Called by the checkpoint/reward refresh path. |
| 0x461070 | RewardStatsTextFormat | Converts stored reward counters, percentages, and elapsed time into strings used by the statistics panel. Called by `RewardStatsPanelDraw`. |
| 0x45dbc0 | TerrainViewTileGeometryBuild | Walks the visible terrain-view grid, resolves tile style, projects tile geometry, and emits bounded batches of quads. Called from the view rendering path. |
| 0x45e9e0 | TerrainTileStyleResolve | Resolves whether a terrain-view tile is rendered and supplies its style data to the geometry builder. |
| 0x45e940 | ViewPointToScreenGeometryTransform | Applies view scale and screen offsets to view-space geometry, including the screen-axis orientation adjustment used by the tile builder. |
| 0x46d380 | AttachedModelOrientationUpdate | Derives orientation data from a model's geometry and updates attached-object orientation. Used by four entity/model update paths. |
| 0x405c10 | PendingCommandEntityAssignmentTick | Scans pending command records and eligible entities, evaluates distance/reachability, routes candidates, and records assignments. The precise action codes and eligibility flag meanings remain unresolved. |
| 0x41baa0 | SimulationPointerActionDispatch | Dispatches pointer input through the simulation action cases, using a screen-space box test and updating action state. Exact action IDs and associated flag meanings remain unresolved. |
| 0x407d60 | BuildingTypeConfigRecordParse | Parses a BuildingTypes record, including carry, camera, tool, deposit, fire, and entrance null names/frames, pivot bounds, shape entries, and power-level scene data. |
| 0x433e80 | TerrainProjectedCellStateRefresh | Scans a camera-bounded terrain region, tests projected cell geometry, and updates projection-related cell state while queuing renderer work. The meanings of the tested flag bits remain unresolved. |

**Evidence.** The reward routines' direct decompiles and caller observations are
in [legorr-engine-helpers-expansion-10.bundle](../evidence/legorr-engine-helpers-expansion-10.bundle):
`ev_dfaa900f43a3c9cfaab406565165614c41a8d6008e82b1a6e913e608c7c74a65`,
`ev_b08b6461bb53f6c169e90de71e8dcf9d5cecb53b0574e091cddcc92ff679ddd6`,
`ev_b1fe19b5edbfa154eccfa361ee49f13a02310e1536ac9feaa328be9a1247254c`, and
`ev_f3088aebbc4e647841af284626b4aeeab64a5fc5bf4f5662d669e52a2ed212f3`.
Terrain-view and attached-model evidence is in
[legorr-engine-helper-audit-render-expansion-10.bundle](../evidence/legorr-engine-helper-audit-render-expansion-10.bundle).
Simulation and building-config evidence is in
[legorr-engine-helper-audit-simulation-expansion-10.bundle](../evidence/legorr-engine-helper-audit-simulation-expansion-10.bundle).
All investigations used shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
