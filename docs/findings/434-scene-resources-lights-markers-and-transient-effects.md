# Finding 434 — Scene Resources, Lights, Markers, and Transient Effects

Issue #182 covers 33 procedures around scene-resource registration, temporary
scene objects, lighting, terrain markers, screen effects, and fence sparks. The
functions sit at a boundary between data/config loading and runtime scene
state: resource handles are loaded and retained in global tables, effects
instantiate short-lived scene objects from those handles, and update sweeps
expire the objects and make their slots available again. Several neighboring
procedures were already described in findings 13, 240, 249, 251, 253, 372,
374, 376, 378–381, 388, and 416–433; this finding connects the assigned slice
and records its remaining gaps.

**Evidence.** The primary source is the checked-in Ghidra pseudocode in
[`LEGORR.full.c`](../decompiles/full/LEGORR.full.c), which labels each procedure
with its address. Statements below distinguish visible operations from
interpretations of globals and call-site roles. The legacy extracts and
earlier findings linked in the cross-reference table provide adjacent caller
context. The original binary was not needed to make the claims below; the exact
meaning of several anonymous resource subtypes, flags, and callbacks remains
unresolved.

## Resource registration and lifetime

`RockFallStylesConfigLoad` (`0x0040bfd0`) walks the `RockFallStyles` config
entries, stores each style name, tokenizes its resource list, and calls
`SceneActSlotLoadAndRegister` (`0x0040c0d0`) for each listed resource.
The latter loads a scene resource (trying the ordinary scene descriptor and a
fallback descriptor), enables it, creates four associated handles, and
registers the requested name on the selected style/slot. `RockFallStyleNameLookup`
(`0x0040bf70`) is a case-insensitive lookup over the stored names, while
`RockFallStyleTableSet` (`0x0040bfc0`) sets the global style-table selector.
`RockFallStyleSceneResourcesRelease` (`0x0040c6c0`) releases and clears the
four child handles in each of three active style groups. This connects the
config loader to the fixed slot tables; it does not establish what each
style's resource represents in gameplay.

Two other registration paths use fixed global tables. `IndexedResourceHandleSetCreate`
(`0x0040c1b0`) loads and enables a parent handle, then creates/enables 30
associated handles in the selected `0x3d`-stride group. `GlobalResourceHandleLoadAndEnable`
(`0x0040c720`) loads one fixed resource and enables its handle. The ten-case
`GetResourceSlotAddressByIndex` (`0x0040c8a0`) returns addresses for ten
misc-effect slot records; out-of-range indexes return null.
`ResourceHandleLoadWithFallback` (`0x0040c920`) tries a primary resource
descriptor and then a fallback, writes the successful handle at record `+0x30`,
and enables it. `MiscObjectsEffectSlotBind` (`0x0040c990`) binds ten named
`MiscObjects` entries (including laser/pusher/freezer hits, path dust, four
lava-erosion smoke slots, bird scarer, and upgrade effect) through that
fallback loader.

`ResourceSlotRegisterName` (`0x0040c5b0`) searches the three active style
groups and their four slots for a matching resource handle, finds the next
unregistered name index, sets its bit, formats the resource name, and applies
it to the handle. Its concrete table and bit behavior is detailed in finding
251. `SceneResourceSingletonPoolRelease` (`0x0040c690`) releases and clears
the four-entry singleton pool; findings 374 and 416 cover the associated
resource ownership questions.

## Temporary scene objects and expiry

`SceneLightCreateAtPosition` (`0x0040be10`) claims the first free slot in a
four-entry pool, clones one of two loaded scene resources according to the
current scene field at `+0x274`, then positions and orients the clone.
`SceneMarkerCreateAtObjectPosition` (`0x0040bde0`) reads a scene object's
position and passes it to that helper. `SceneMarkerCreateAtPositionOrObject`
(`0x0040beb0`) takes explicit coordinates when its object argument is zero;
otherwise it reads the object's position, then creates/positions objects from
the resource in a parallel four-entry pool. The table layout and calls are
directly visible; the semantic labels “light” and “marker” follow the
callers/configuration and remain uncertain for some resource subtypes.

`SceneLightPoolExpireSweep` and `SceneLightSecondaryPoolExpireSweep`
(`0x0040c4d0`, `0x0040c520`) advance handles in those parallel four-entry pools
and release/clear a slot when the time helper reports a nonzero result.
`LavaEffectSelectAndPlace` (`0x0040cce0`) selects one of ten loaded effect
slots, clones its resource, positions/orients the clone, and increments the
slot count; it returns failure for an invalid index, unloaded resource, or a
full slot. `SceneTimeoutEntrySweep` (`0x0040cb70`) applies the same timed
advance/release pattern to a ten-entry effect list, using swap-with-last when
an entry expires. For the bird-scarer list only, it also accumulates elapsed
time and, every 12.5 time units, reads an object position and dispatches a
command-like helper; the exact gameplay meaning of that dispatch is not
identified here. `TerrainMarkerSetsClear` (`0x0040cc40`) invokes that sweep for
all ten effect lists. `SelectionMarkerResourcesRelease` (`0x0040ce30`)
releases the selection-marker resource array and its associated grid buffer.

`ResourceAvailabilitySweep` (`0x0040c3a0`) first calls cleanup for terrain
point objects, both light pools, and the ten marker/effect lists. It then
advances the three active four-slot style groups and the two 30-slot indexed
resource groups; completed handles are enabled and marked available. For the
four-slot groups it also returns the available `(group, slot)` pairs through
output arrays. The fuller slot arithmetic and callers are recorded in finding
249. `SceneSlotPoseScaleRegister` (`0x0040c230`) claims the first free one of
four slots in a selected style group, sets a child handle's position and pose,
stores two associated values, and marks the slot occupied. These routines
show fixed-capacity pools with explicit reuse; they do not reveal the semantic
units of the stored time values.

## Configured scene lighting and visual effects

`SceneLightsBuildFromConfig` (`0x00433770`) reads the configured top spot,
track spot, ambient, first-person light, and optional first-person rotation
light RGB values. It creates scene lights with types 2, 2, 0, 1, and 3
respectively, creates an additional default white type-3 light, and creates a
root scene object containing another default white type-2 light. It then
positions, orients, and enables the handles and sets attenuation or range for
the top spot, track spot, and root light. Missing required RGB
settings or required light creation causes an early failure; the final
rotation-light setting is optional. The prior behavioral summary is in finding
240.

The root scene object's default white type-2 light is stored at
`DAT_00556104`. `ContextSelectedGlobalPointerGet` returns this pointer when
`DAT_00556ec8` is nonzero, and `MainSimInteractionAndCellUpdate` adjusts its
light values under that mode.

`PreloadNumberedBitmaps` (`0x0040a400`) formats and loads eleven numbered
bitmap resources into a global cache: ten occupy consecutive slots and the
eleventh has a separate slot. `LoadTileMeshes` (`0x0042b330`) allocates a
tile-grid handle array and loads paired `a`/`b` bitmap resources for each cell;
when the editor flag is set, it leaves the initialized array empty instead.
Those routines connect data-driven resource naming to cached UI and terrain
visuals; findings 13 and 378 describe their neighboring resource paths.

`TickEntityScreenEffects` (`0x004075a0`) ticks four entity/effect slot tables.
It decrements lifetime values, checks entity and game-mode flags before
projecting positions through `CameraProjectPointToScreen`, submits screen
effects or overlay draws, and clears expired slots. `CreateRepeatedResourceInstances`
(`0x00408e50`) updates an entity-side count and invokes the repeated-instance
creator the requested number of times; one `(kind, variant)` pair also raises
a flag on the associated record. Neither helper exposes a complete name or
asset mapping for the resources it consumes.

## Terrain markers and fence sparks

`SceneMarkerUpdateAndDraw` (`0x004291a0`) has distinct modes for an active
cell marker, a global/current-cell marker, and a selected-object marker.
Depending on mode and state flags it maps scene/world positions to cells and
calls the cell-marking or object-draw path. The exact flag meanings are
described in findings 03 and 380; this function alone does not prove all
visible marker semantics.

`CellPairEdgeEffectApply` (`0x0040dc20`) maps a neighboring cell pair to a
direction, collects edge parameters, converts both cells to world-space
centers, samples terrain height, and configures a transient render slot; its
caller is `TerrainCellNeighborFloodWalk` (finding 249). The adjacent fence
spark path begins with `EntityFenceSparkTerrainInteraction` (`0x0040de40`):
it maps an entity position to a terrain cell, checks cell flags and the
four-neighbor records, and when the state and a neighbor timer permit, calls
`FenceSparkLaunchRegister` (`0x0040e0c0`) and updates the cell timer. The
launch helper converts the cell to world coordinates, configures a transient
render slot, instantiates the `FenceSpark` action resource, attaches it to the
entity, and updates the entity. The exact trigger encoded by the cell flags
and timer remains unknown. Findings 233 and 253 provide the surrounding
terrain-state and effect-helper context.

## Open questions and follow-up

- Which exact resource types and gameplay objects correspond to the two
  scene-object pools, the three style groups, and the indexed 30-handle sets?
- What units and completion conventions does the scene-handle time helper use
  across the marker, light, and resource-availability sweeps?
- What command and behavior does the bird-scarer timeout dispatch trigger?
- Which terrain flags and neighbor-timer states trigger the fence-spark path?
- Which caller-side state selects each `SceneMarkerUpdateAndDraw` mode?

These are questions for focused follow-up investigations, not established
defects. No behavior change is proposed by this investigation.

## Assigned function cross-reference

All 33 assigned addresses already have generated rows in
`docs/coverage/function-areas.csv` and `docs/coverage/ledger.csv`. The 32
functions still marked named-but-unported also appear in
`docs/coverage/README.md`; `SceneMarkerUpdateAndDraw` is already marked ported
there through five source citations, so the coverage README correctly omits
it. This finding adds the README findings-index entry; no generated coverage
index changes are needed.

| Address | Assigned function | Existing behavior map |
| --- | --- | --- |
| `0x004075a0` | `TickEntityScreenEffects` | [240](240-cursor-interaction-overlay-menu-and-scene-light-helpers.md), [388](388-geometry-and-matrix-primitives.md) |
| `0x00408e50` | `CreateRepeatedResourceInstances` | [252](252-config-band-texture-loaders-route-commands-and-surface-menu-helpers.md), [376](376-selection-cell-and-overlay-state-helpers.md) |
| `0x0040a400` | `PreloadNumberedBitmaps` | [13](13-texture-system.md) |
| `0x0040bde0` | `SceneMarkerCreateAtObjectPosition` | [380](380-scene-objects-terrain-and-matrix-helpers.md) |
| `0x0040be10` | `SceneLightCreateAtPosition` | [374](374-terrain-interface-and-media-helpers.md) |
| `0x0040beb0` | `SceneMarkerCreateAtPositionOrObject` | [380](380-scene-objects-terrain-and-matrix-helpers.md) |
| `0x0040bf70` | `RockFallStyleNameLookup` | [225](225-terrain-objective-level-and-diagnostic-helpers.md) |
| `0x0040bfc0` | `RockFallStyleTableSet` | [381](381-selection-resource-runtime-and-grid-helpers.md) |
| `0x0040bfd0` | `RockFallStylesConfigLoad` | [249](249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md) |
| `0x0040c0d0` | `SceneActSlotLoadAndRegister` | [249](249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md) |
| `0x0040c1b0` | `IndexedResourceHandleSetCreate` | [372](372-incident-messages-options-and-resource-helpers.md) |
| `0x0040c230` | `SceneSlotPoseScaleRegister` | [253](253-terrain-scatter-erosion-propagation-cell-queries-menu-records-and-buffer-helpers.md) |
| `0x0040c3a0` | `ResourceAvailabilitySweep` | [249](249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md) |
| `0x0040c4d0` | `SceneLightPoolExpireSweep` | [374](374-terrain-interface-and-media-helpers.md) |
| `0x0040c520` | `SceneLightSecondaryPoolExpireSweep` | [374](374-terrain-interface-and-media-helpers.md) |
| `0x0040c5b0` | `ResourceSlotRegisterName` | [251](251-command-band-kind-dispatch-item-name-lookup-route-and-record-helpers.md) |
| `0x0040c690` | `SceneResourceSingletonPoolRelease` | [374](374-terrain-interface-and-media-helpers.md) |
| `0x0040c6c0` | `RockFallStyleSceneResourcesRelease` | [374](374-terrain-interface-and-media-helpers.md) |
| `0x0040c720` | `GlobalResourceHandleLoadAndEnable` | [372](372-incident-messages-options-and-resource-helpers.md) |
| `0x0040c8a0` | `GetResourceSlotAddressByIndex` | [376](376-selection-cell-and-overlay-state-helpers.md) |
| `0x0040c920` | `ResourceHandleLoadWithFallback` | [372](372-incident-messages-options-and-resource-helpers.md) |
| `0x0040c990` | `MiscObjectsEffectSlotBind` | [372](372-incident-messages-options-and-resource-helpers.md) |
| `0x0040cb70` | `SceneTimeoutEntrySweep` | [256](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md) |
| `0x0040cc40` | `TerrainMarkerSetsClear` | [379](379-terrain-ui-and-crt-support-helpers.md) |
| `0x0040cce0` | `LavaEffectSelectAndPlace` | [90](90-lava-erosion-ticker-advances-pending-cells.md) |
| `0x0040ce30` | `SelectionMarkerResourcesRelease` | [378](378-input-selection-and-action-state-helpers.md) |
| `0x0040dc20` | `CellPairEdgeEffectApply` | [249](249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md) |
| `0x0040de40` | `EntityFenceSparkTerrainInteraction` | [233](233-terrain-rendering-and-stream-primitives.md) |
| `0x0040e0c0` | `FenceSparkLaunchRegister` | [252](252-config-band-texture-loaders-route-commands-and-surface-menu-helpers.md) |
| `0x00422850` | `MiscObjectResourcesLoad` | [226](226-omitted-simulation-terrain-resource-text-procedures.md) |
| `0x004291a0` | `SceneMarkerUpdateAndDraw` | [03](03-engine-tick.md), [228](228-entity-pool-terrain-and-timed-effect-helpers.md) |
| `0x0042b330` | `LoadTileMeshes` | [13](13-texture-system.md), [256](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md) |
| `0x00433770` | `SceneLightsBuildFromConfig` | [240](240-cursor-interaction-overlay-menu-and-scene-light-helpers.md) |
