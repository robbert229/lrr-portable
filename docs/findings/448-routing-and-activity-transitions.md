# Finding 448 — Routing and Activity Transitions

This issue's 41 assigned functions span route selection, command-record construction, activity dispatch, linked-entity responses, and terrain eligibility. The decompilation establishes the writes and branches; semantic names for many entity fields, activity IDs, and flag bits remain provisional.

| Address | Name | Observed behavior |
| --- | --- | --- |
| `0x004468c0` | `FUN_004468c0` | Appends a half-cell route record only when entity flag `+0x3e0 bit 0` is set and the requested tile is cardinally adjacent to the previous record; clears `+1000 bit 0x400` after append. |
| `0x004469c0` | `EntityNearestConnectedTerrainCellRouteSelect` | Checks four or eight neighboring cells; tests direct route eligibility first, then probes each candidate with `TerrainNeighborPathCheck`, ranks successful candidates by returned path length, commits the best route, and returns its candidate index through an output. |
| `0x00446e40` | `EntityNearestCellRouteCommit` | Validates an entity/target pair, selects a route with the helper above, appends the selected step, writes interpolation values, stores target coordinates at `+0x2f4/+0x2f8`, and toggles `+1000 bit 0x2000000` by mode. |
| `0x0044702d` | `RegisterRelativeStateUpdateFragment` | Unnamed decompiler fragment calling `FUN_004468c0`, writing randomized route interpolation values and target coordinates, then clearing `+1000 bit 0x2000000`. Caller context is not recoverable from this standalone fragment. |
| `0x004470d0` | `EntityConditionalTypeFiveSpawn` | Under flag `+1000 bit 0x4000000` and a comparison of `FUN_0046a000` with the source field at `+0x340`, finds a location, creates a type-5 entity, links it at that position, and clears source field `+0x33c` on success; the field's meaning is unresolved. |
| `0x004471b0` | `TryCreateLinkedEntityAtPosition` | If the source lacks `+0x3e0 bit 0x400`, appends a weighted route step to the requested cell, marks that record type 3, and links the two entities through `+0x2fc/+0x300`. |
| `0x00447210` | `EntityAdjacentPlacementCellSearch` | For an entity kind 3, searches a randomized order of twelve neighboring offsets in two passes; candidates are filtered by `FUN_004330e0` on the first pass and submitted through `RouteToCellCommandIssue`. |
| `0x004473b0` | `RouteToCellCommandIssue` | Requires link flag `+0x3e0 bit 0x400` and a clear `FUN_0043baf0` gate; routes the linked entity toward a cell, appends the path, marks the record type 5, and associates the route owner. |
| `0x004474f0` | `LinkedEntityProximityResponse` | Clears the source link, compares entity positions against combined radii plus a doubled radius, and on proximity starts a directional activity on the linked entity and applies a helper update. |
| `0x004475c0` | `EntityProgressAndNegativeDeltaHandler` | Processes progress on a linked record subject to flags; positive deltas increase a value capped at 100, negative deltas trigger a helper and set a flag, while alternate flag branches perform wake/timeout handling. |
| `0x00447730` | `EntityProgressAdvanceBy25` | Clears the link field `+0x2fc`, increases the same entity's value at `+0x344` by 25, clamps it to that entity's `+0x340` value, and reports whether it reached that bound. |
| `0x00447780` | `EntityClearLinkedObjectState` | When link flag `+0x3e0 bit 0x400` is set, clears link fields, marks the linked entity with `+1000 bit 0x2000`, and dispatches event 4. |
| `0x004477d0` | `PathStepLookaheadEntryWrite` | Computes a point 0.1 units along the vector toward the linked entity, converts it to cell/height data, writes the current route record, sets record flag `0x08`, updates target coordinates, and clears the link. |
| `0x00447900` | `TypeFourEntityCallbackDispatch` | For entity kind 4 only, invokes a callback through `FUN_004377b0`. |
| `0x00447950` | `EntityDirectionalActivityStart` | Redirects to a linked entity when a flag/pointer pair is set; if activity is available, selects a directional activity from a vector (or a default from a mode), dispatches it, ticks once, and sets activity flag `+0xf8 bit 0x1000`. |
| `0x00447b00` | `EntityDispatchEvent5AndSetFlag` | Dispatches event 5, updates activity dispatch, and sets `+0x3e0 bit 0x200000`. |
| `0x00447b30` | `EntityActivityTick` | Advances the activity object selected by entity kind; kind-specific update helpers return remaining progress, positive values trigger `FUN_00403f60`, and an additional flag path delegates to `EntityTypeBehaviorUpdate`. |
| `0x00447c40` | `SceneActivityUpdateDispatch` | Applies changed activity names to the kind-specific activity object, saves a related state value, optionally mirrors activity text/state to an associated object, then clears pending activity state. |
| `0x00447ea0` | `EntityTypeBehaviorUpdate` | Dispatches time-based behavior by entity kind, including boulder movement/effects, kind-6 animation, tick-down/teleport transitions, and activity-specific updates. Several state IDs and flags remain unnamed. |
| `0x004487c0` | `EntityClearNestedState8Fields` | For a linked object of kind 8, clears two nested fields and invokes a state reset helper. |
| `0x00448800` | `EntityMoveToCellCoordinates` | For nested kind 8, validates target coordinates and creates a weighted route step; marks it type 9 and stores target coordinates. |
| `0x00448880` | `RecordGatedStepCommandIssue` | For nested kind 13, validates the cell and routes from the current cell to target (or appends a direct step); on success marks type 9 and stores the target. |
| `0x004489a0` | `EntityKind6TickDownMarkerSpawn` | When a type-6 task check succeeds, creates a kind-13 entity at the current world position and height, assigns the `TickDown` activity text, and sets a nested flag. |
| `0x00448a60` | `EntityNeighborCellMoveCommandHandle` | For nested kind 11, validates and routes to a target cell, marks the step type 9, optionally randomizes interpolation for kind 1, and stores target coordinates. |
| `0x00448c50` | `EntitySetActivityPlace` | Dispatches activity 0x1e, refreshes activity state, and sets `+0x3e4 bit 0x200`. |
| `0x00448c90` | `CardinalStepCommandIssueAndLink` | For a kind-4 target, selects a direct/jittered route by source kind; successful routes create type-0xf records with directional flags and establish reciprocal entity links. |
| `0x00448eb0` | `ProximityStepCommandIssueWithJitter` | For kind 2 approaching kind 4, chooses a nearest connected cell and emits a type-0xe or type-0xf record with randomized edge interpolation and direction flags. |
| `0x00449010` | `EntityKind6RecordApproachRouteIssue` | For the specified kind/flag combination, finds an approach cell, appends a weighted route record, marks it type 0x12, and stores target coordinates. |
| `0x004490a0` | `ProximityStepCommandIssueWithLink` | For kind 2 or 3 targeting kind 4, selects a nearby connected cell, writes an edge-interpolated type-6 record, sets directional and optional record flags, and stores the target link. |
| `0x00449240` | `EntityWalkableTargetCellRouteIssue` | Validates a target cell, appends a weighted route record, marks it type 2, and stores target coordinates. |
| `0x004492b0` | `CellStepCommandIssueWithTargetMark` | After a cell test and entity flag gate, routes to the target or appends a direct step, marks it type 4, and for kind 1 updates two cell-side markers. |
| `0x004493e0` | `SceneObjectHasTaskType4_5_7` | Returns true only for kind 2 when one of task IDs 4, 5, or 7 is present. |
| `0x00449430` | `EntityTargetCellRouteAndAssociate` | Routes to another entity's cell (or appends directly), marks the record type 0x11, and stores that entity in `+0x2fc`. |
| `0x00449520` | `EntityTargetCellRouteMarker14` | Routes to another entity's cell and marks a successful record type 0x14. |
| `0x00449610` | `EntityCoordinateRouteAndMark` | Routes to explicit cell coordinates, marks the record type 0x10, and stores the target coordinates. |
| `0x00449700` | `RouteStepCommandIssue` | Chooses a candidate cell via one of two helper routes according to flag `FUN_00469e80 bit 0x100000`; appends a route, marks it type 10, and stores target coordinates. |
| `0x00449880` | `BlockedCellApproachWeightWrite` | Requires a cell predicate and nearest-connected-cell selection, then writes direction-specific interpolation weights and sets route-record flag `0x08`. |
| `0x00449980` | `EntityActivityTerrainEligibilityTransition` | For entities with `+0x3e0 bit 0x40`, checks the truncated coordinates with a terrain predicate; an invalid cell dispatches activity 0xb and sets a high flag, while a valid cell with positive field `+0x2d4` clears the gate, resets activity/coordinates, and signals the output. |
| `0x00449a70` | `EntityActivity23StateTransition` | Sets a kind-2 flag, then when a `+0xf8 bit 0x400` gate is clear sets `+0xf8 bit 0x40000000`, dispatches activity 0x23, refreshes it, and applies another state helper. |
| `0x00449ac0` | `ProximityStepCommandIssueClearingGate` | For kind 2 targeting kind 4, selects a nearby connected cell and creates a jittered type-0xb route record; clears a source flag when route selection fails. |
| `0x00449c00` | `FUN_00449c00` | Registers a callback (`LAB_00449cf0`) through `FUN_004377d0`; callback body is outside the assigned slice. |

## Cross-function flow

`EntityNearestConnectedTerrainCellRouteSelect` (`0x004469c0`) and the route issue helpers call `TerrainNeighborPathCheck` (`0x004410f0`, also covered in findings 242 and 246) to obtain candidate paths, then pass path arrays to `FUN_00441700` to append route records and release temporary buffers. The route record consists of coordinates and interpolation/flag/type fields at a 0x14-byte stride, directly visible in the writes. Helpers such as `BlockedCellApproachWeightWrite` and `FUN_004468c0` append or adjust the next record under their own adjacency/eligibility checks.

Direct incoming-call examples clarify where these helpers sit: `EntityConditionalTypeFiveSpawn` and `LevelDebugInputCellMutatorDispatch` call `TryCreateLinkedEntityAtPosition`; `EntityAdjacentPlacementCellSearch` calls `RouteToCellCommandIssue`; `PathStepAdvanceAndArrivalCheck` calls `PathStepLookaheadEntryWrite`; `EntityTeardown` calls `TypeFourEntityCallbackDispatch`; `LinkedEntityProximityResponse` calls `EntityDirectionalActivityStart`; `EntityActivityTick` calls `EntityTypeBehaviorUpdate`; `ActivityTimeoutAccumulateAndRaise`, `CreatureSpawnFromEmergence`, and `EntityActivity23StateTransition` call `SceneActivityUpdateDispatch`; and `PlaceCommandEligibilityCheck` calls `SceneObjectHasTaskType4_5_7`. These are direct pseudocode references; not every assigned routine has a statically resolved caller because several are callback or dispatch entry points.

Activity changes flow through `FUN_004409e0` and `SceneActivityUpdateDispatch`, while `EntityActivityTick` calls kind-specific activity update helpers. Linked-object helpers use entity fields at `+0x2fc`, `+0x300`, and `+0x308`; some callers establish reciprocal links, route toward the linked entity, or clear source-side fields while flagging the linked entity and dispatching an event. The pointer relationships are direct evidence; the exact domain meaning of each link mode remains uncertain.

Terrain eligibility and movement use multiple predicates (`FUN_00432950`, `FUN_004329d0`, `FUN_00432a20`, `FUN_00432bd0`, `FUN_004330e0`) whose exact flag semantics are not uniformly established. Activity/task IDs and many bit fields are recorded literally above rather than assigned speculative names. `EntityTypeBehaviorUpdate` contains string-backed states such as `TickDown`, `Expand`, `Teleport`, `Normal`, `Disintegrate`, and `FlapInWind`; the strings are direct evidence, while their wider gameplay contract is not.

## Unresolved questions and follow-up slices

- Name the route-record type codes and interpolation/record flags by tracing their consumers and serializers.
- Identify the terrain and occupancy semantics of the route candidate predicates and their differing coordinate arguments.
- Trace the reciprocal-link lifecycle for `+0x2fc`, `+0x300`, and `+0x308`, including cleanup on entity destruction.
- Identify the source of the opaque `FUN_0044702d` decompiler fragment and callback registered by `FUN_00449c00`.
- Map the activity IDs and flag transitions in `EntityTypeBehaviorUpdate` to configuration/resource data and runtime traces.

## Related findings

Findings [10](10-entity-command-primitives.md), [225](225-terrain-objective-level-and-diagnostic-helpers.md), [242](242-path-step-terrain-and-pending-record-helpers.md), [246](246-entity-move-route-command-and-object-clone-helpers.md), [248](248-config-band-loaders-display-mode-directional-activity-and-flight-record-helpers.md), [256](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md), [380](380-scene-objects-terrain-and-matrix-helpers.md), and [430](430-scene-object-action-and-animation-lifecycle.md).

Canonical pseudocode: [LEGORR.full.c](../decompiles/full/LEGORR.full.c).
