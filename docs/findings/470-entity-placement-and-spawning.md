# Finding 470 — Entity Placement and Spawning

This finding maps the 29 assigned procedures in entity-placement-spawning. They cover entity-component access, placement-coordinate selection and previews, terrain sound configuration, random and configured spawns, entity pairing, permitted activities, and scene-tree attachment. Direct behavior below is from the canonical pseudocode; interpretations are labeled as inference. Function names are descriptive labels and do not establish undocumented gameplay meanings.

## Entity component access and position helpers

EntityTypeComponentGet @ 0x44a200 dispatches by entity kind: kinds 1, 2, 3, 4, and 10 return a component via their type-specific accessors; other kinds return the record at +0x1c when the +0x3e8 bit 0x20000 is set, or the original record otherwise. SceneObjectStateHandleApplyValue @ 0x449cd0 uses this result and, when non-null, calls FUN_00473b20 with the supplied value.

EntityTypeOperationDispatch @ 0x449f50 and EntityTypeOperationRelease @ 0x449ff0 route to kind-specific operation or release helpers for kinds 1, 2, 3, 4, and 10. For the remaining kinds, both use the +0x3e8 bit 0x20000 as a gate before applying/releasing the handle at +0x1c. The matching kind cases use different fields and helper families, so the operations are not one uniform vtable call.

EntityWorkObjectPositionGet @ 0x44a2a0 supports kinds 1–3: it obtains the kind-specific work-object handle, returns 0 if absent, otherwise reads the handle position into two outputs and returns 1. ObjectMovementInputAccumulate @ 0x44a310 redirects kind-2 records through a linked record at +0x36c when present; if entity flag bit 0 at +0x3e0 is set it resets the action sequence, then adds its two integer inputs and float input into accumulators at +0x2bc, +0x2c0, and +0x2c4. Its second integer input is suppressed unless FUN_00469ea0 reports bit 4.

FUN_0044a070 is still unresolved. It selects or refreshes a cached handle in the entity fields at +0x2dc/+0x2e0 based on entity kind and an index argument, reads a position and orientation through that handle, normalizes a returned vector, and negates the vector when a type-specific component lookup succeeds. The exact handle and vector semantics need a more focused analysis.

## Coordinate pools, candidate search, and placement preview

Type11CellCoordinateAppend @ 0x44a390 appends coordinate pairs to a global pool capped at 10 entries. PredugCoordinatePoolAppend @ 0x44a4b0 appends coordinate pairs to a separate pool capped at 20. PlaceCellNearestResolve @ 0x44a3d0 and NearestCellCoordinateWorldResolve @ 0x44a4f0 convert each stored coordinate to world space, compare its distance to the supplied entity position, and optionally return the nearest coordinate; both return 0 when their pool is empty.

NearestOpenCellCoordSearch @ 0x44a5d0 scans a 20-by-20 region around the entity's terrain cell. It filters cells through the terrain-record lookup and two optional state tests, converts remaining cells to world positions, and records up to 100 candidates with their distances. It sorts the candidates with FUN_0048de20 / LAB_0044a7a0 and returns the closest coordinate pair. When its distance option is enabled, a candidate is retained only if the current-level value at +0x10 is less than or equal to that candidate's distance; the meaning of that field is unresolved. Existing route-command evidence shows this helper used by RouteStepCommandIssue (Finding 251); the exact meanings of its two boolean-like filters remain unresolved.

PolyVariantSectionModelBuild @ 0x44a7d0 loads the configured section names for the requested quality level (MediumPoly, HighPoly, or indexed FPPoly_Camera entries), walks their entries, filters !-prefixed entries using a global flag, and builds/combines model records through FUN_00451ab0 or FUN_004519b0. It returns the first record handle it creates.

BuildingPlacementPreviewUpdate @ 0x44a910 handles entity kind 4 when its final enable argument is nonzero. It first checks whether the selected terrain coordinate is placeable; failure may dispatch a sound and update a preview handle. Otherwise it computes a direction from the selected point toward the supplied world point and tests up to five adjacent orientations with FUN_004641c0. In the placement-commit branch it creates/updates the object, tests nearby cell/entity conditions, changes cell flags through FUN_00408e50, updates the preview, and exits placement mode. Direct calls and flag writes are visible; interpreting this as the UI placement-preview/commit flow follows the function's caller context and helper arguments.

## Terrain sound configuration and spawn paths

TerrainObjectTypeSfxGridConfigLoad @ 0x44acc0 clears a 20-by-15 global table, reads ObjTtSFXs entries, parses two coordinates and a resource reference, and stores successful references by cell. TerrainObjectTypeSfxAtCellGet @ 0x44ad80 copies a coordinate pair through CopyTwoDwordPairToOutputs and returns the corresponding table entry.

ScatterSpawnGateSet @ 0x44adc0 sets or clears bit 0x20 in _DAT_004dee14. EntityKindOneRandomSpawnTrigger @ 0x44ade0 requires that gate to be clear, an entity kind of 1, and the entity's +0x3ec bit 0x800 clear. It optionally selects the linked entity at +0x2fc when +0x3ec bit 0x40 is set, obtains a count through FUN_00469af0, calls RandomKindSixScatterSpawn, then sets the 0x800 bit to mark this path as consumed. RandomKindSixScatterSpawn @ 0x44ae50 loops for the requested count, starts from the entity position (or a component position in the kind-4/flagged case), adds independently sampled offsets in [-5, 5], keeps the jittered location only when terrain lookup and placeability pass, creates a kind-6 entity, and calls FUN_00402720 on it. Finding 255 traces the caller and the later task registration path.

EntityAttachedSpawnRecordProcess @ 0x44af90 reads the count from the entity's +0x348 record and passes that count and the entity to RandomKindSixScatterSpawn.

ConfiguredTinyMonsterSpawnAndTeardown @ 0x44afb0 compares the supplied cell with configured RockMonster, Lavamonster, and Icemonster cells, chooses a TinyRM or TinyIM configuration, then attempts entity creation at the cell's world position with a random orientation. Each created entity gets -1.0 stored at +0x340; successful checks through FUN_00449700 set the source entity's +0x3e4 bit 0x20, with at most three such successful iterations before return. The remaining path calls FUN_00437540 on the last created record. The creation/eligibility/release sequence is direct; the precise lifecycle meaning of that final release call is unresolved.

SmallSpiderCellSwarmSpawn @ 0x44b140 loads the SmallSpider configuration and creates (param_3 & 3) + 2 entities around the coordinate, using the transformed cell bounds offset by one world unit and a randomized orientation. Each created record receives +0x3e0 bit 0x10000.

## Pairing, activity lists, target routing, and scene attachment

PairingActivityRaiseAndMutualLink @ 0x44b250 requires FUN_004385b0(param_2, 1) to return nonzero, then checks the observed flags on both entities: param_2 bits 0x100 and 2 must be clear, and param_1 bit 0 must be clear. It also requires FUN_0043baf0(param_1) to return zero and the first entity's distance to be below its +0x340 threshold. On success it updates both activity states, requests activity codes 0x25 and 0x26, registers both entities, sets reciprocal bits at +0x3e4, and stores mutual pointers at +0x3c0. Whether this represents a specific pair gameplay mechanic is an inference from the reciprocal state and activity requests.

EntityActivityTypeSupportsCapacityRule @ 0x44b350 returns true only for activity kinds 4, 5, and 7. EntityActivityListAppend @ 0x44b370 obtains the capacity from FUN_0046a060, clears +0x3f4, and returns if capacity is zero. Otherwise it recomputes the entity's activity capability bit at +0x3ec. For special kinds 4/5/7 it replaces an existing special entry rather than adding a duplicate; otherwise it appends when there is capacity, or rotates the full list before appending. Findings 128 and 131 trace this capacity to the Stats field NumOfToolsCanCarry. EntityPermittedActivityCheck @ 0x44b480 scans the list at +0x370 for the requested activity and returns whether it is present.

TypeTwoEntityTargetResolveAndApply @ 0x44b4c0 applies a target-membership check only for kind 2: it resolves an activity/type from the supplied argument and asks EntityPermittedActivityCheck; other kinds return 1 directly.

ObjectPositionStepCommandIssueAndLink @ 0x44b500 handles kind 2. It resolves a scene object near the entity, reads that object's position, converts both positions to terrain cells, and either directly calls FUN_00441700 when the cells match or asks FUN_004410f0 for a route first. On success it stores the resolved object at +0x2fc and writes step opcode 0xd into the current step record. This is an object-targeted route producer adjacent to the command/selection routing mapped in Findings 251 and 466.

EntitySceneObjectTreeAttach @ 0x44b680 obtains the entity component's scene handle, allocates a callback/state tree at entity +0x32c, associates the root with the scene handle, then loops over the reported child count and creates/attaches child transform records using the entity's scale and pose helpers. Finding 377 gives the callback/state allocator's role; exact child ordering and transform semantics remain open.

## Unresolved questions and follow-up slices

- Identify FUN_0044a070's cached handle and orientation/vector contract, and the consumers of its +0x2dc/+0x2e0 cache.
- Resolve the two optional terrain filters and distance threshold used by NearestOpenCellCoordSearch.
- Trace the consumers of the 20-by-15 ObjTtSFXs lookup table.
- Establish the exact lifecycle contract of FUN_00437540 in the tiny-monster path.
- Determine the scene-tree child ordering and transform behavior built by EntitySceneObjectTreeAttach.

**Evidence.** Canonical pseudocode in docs/decompiles/full/LEGORR.full.c, procedures 0x449cd0–0x44b680. Related existing traces: Findings 128, 131, 224, 251, 255, and 377. The function-area catalog assigns all 29 requested procedures to entity-placement-spawning.
