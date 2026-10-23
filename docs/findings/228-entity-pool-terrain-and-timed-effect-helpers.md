# Finding 228 — Entity-Pool, Terrain, and Timed-Effect Helpers

Three address-range audits and a caller-frequency pass surfaced fourteen useful
procedures missing from the manifest. The work also corrected two semantic names:
`0x4377d0` is a general entity-pool callback search, not a timed-only dispatcher,
and `0x46b770` sets a timed-effect update flag, not a camera cursor state.

## Entity pool and transform access

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4377b0 | EntityPoolFindByCallbackFiltered | Calls the pool callback walker with its optional `0x80000`-flag exclusion enabled. It has dozens of direct callers across startup, simulation, and rendering. |
| 0x4377d0 | EntityPoolFindByCallback | Walks configured entity-pool slots, accepts records whose self-pointer at +0x408 matches the slot, optionally excludes records with bit `0x80000` at +0x3e8, and stops when the callback returns nonzero. |
| 0x442260 | EntityTransformPositionGet | Resolves the entity's type-dependent transform object and reads its slot-zero position components into caller outputs. |

## Tick, pointer, and terrain operations

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x4291a0 | SceneMarkerUpdateAndDraw | In the tick paths, projects a scene/camera anchor into screen coordinates, optionally applies a forward offset, then updates or draws the marker according to mode and state flags. |
| 0x425d90 | TerrainPointerCommandUpdate | Tracks pointer movement and hovered terrain cells, applies input/state gates, and dispatches cell commands and feedback. EngineTick calls it in the simulation path. |
| 0x4230c0 | DebugPanelTextDrawUpdate | Advances a 12.5-unit blink phase, selects and positions debug text, and renders it to the debug panel surface. EngineTick calls it before the main render path. |
| 0x42b850 | SurfaceTextCharacterMapInitialize | Initializes a broad set of character-code remappings for the surface text renderer. The exact atlas semantics remain inferred. |
| 0x42f2e0 | TerrainCellEdgeFlagSetAndUpdate | Sets or clears cell flag `0x200` and invokes the terrain-cell update routine; an edge/face traversal calls it for affected cells. |
| 0x449f50 | EntityTypeOperationDispatch | Dispatches an operation to the payload handler selected by entity type; the fallback uses the entity's `0x20000` flag. |
| 0x449ff0 | EntityTypeOperationRelease | Releases the type-specific payload selected by the same entity-type cases, with a flag-gated fallback release. |

## Timed effects and grid rendering

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x46b700 | TimedEffectCueRequestSet | Stores a per-slot cue value and sets request flags `0x3` in the timed-effect record. |
| 0x46b730 | TimedEffectPersistenceSet | Sets or clears per-slot persistence flag `0x10`. |
| 0x46b770 | TimedEffectUpdateRequestSet | Sets per-slot update flag `0x4`; the timed-effect updater consumes this record state. |
| 0x46b7a0 | TimedEffectTimestampReset | Copies the current engine time into a timed-effect slot's timestamp. |
| 0x46b8b0 | RectangularGridOverlayGeometryDraw | Builds repeated perimeter vertex sequences across the configured grid layers and submits them through the geometry/render helpers. |

**Evidence.** Direct decompiles of the terrain, timed-effect, dispatch, and grid
helpers are in REA batch `ev_18bb4a99cf18173dd43b407151b6f8fff9f55df03a39c63c6aca53acf8a3bebf`;
the entity-pool walker and transform accessor are in
`ev_9141e41bb2abcd1ed6c0f2f81d7fd89f6aaaa7c58cb90259ce61dd8c48257e9b`. The
direct decompiles are preserved in
[`legorr-engine-helpers-expansion-4.bundle`](../evidence/legorr-engine-helpers-expansion-4.bundle).
range-audit bundles preserve caller context and the independent decompiles:
[`low range`](../evidence/legorr-engine-helper-audit-low.bundle),
[`simulation and terrain`](../evidence/legorr-engine-helper-audit-mid.bundle),
and [`resource and renderer`](../evidence/legorr-engine-helper-audit-high.bundle).
These observations are from shipped `reference/LegoRR.exe`, SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`.
