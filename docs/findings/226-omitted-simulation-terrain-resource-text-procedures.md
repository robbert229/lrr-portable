+# Finding 226 — Omitted Simulation, Terrain, Resource, and Text Procedures

A function inventory audit surfaced the procedures below. Several already had
generic entries in the target manifest; those entries now have behavior-based names
and retain their existing decompiles. The remaining procedures were added to the
manifest and corpus. Names follow observed behavior and call sites; semantic detail
that remains inferred is called out.

## Startup loading and entity configuration

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x422850 | MiscObjectResourcesLoad | Loads configured miscellaneous model and animation assets, including boulder, explosion, fence, web, and laser-shot resources; startup calls it while initializing the global resource handles. |
| 0x42d100 | RockMonsterTypesLoad | Enumerates and parses the configured RockMonsterTypes records. |
| 0x42d2b0 | BuildingTypesLoad | Enumerates and parses the configured BuildingTypes records. |
| 0x42d460 | UpgradeTypesLoad | Enumerates and parses the configured UpgradeTypes records. |
| 0x42d600 | ObjectNamesLoad | Builds the configured object-name table and its associated handles. |
| 0x42da20 | ObjectTheNamesLoad | Builds the parallel configured “TheNames” table and handles. |
| 0x42e020 | OListEntityRecordParse | Parses an .ol entity record, including optional health defaults and entity-field writes documented in finding 91. LevelLoad calls it for loaded entity definitions. |
| 0x42e8b0 | ObjectListEntityTypeResolve | Resolves an entity/type name to its configured record. Callers include LevelLoad and the .ol parser. |
| 0x42c440 | LevelRecordTimerCountdown | Walks a counted record array and subtracts the elapsed time from positive timer fields at record offset +0x0c. |
| 0x42cb70 | LavaErosionCellTimersAdvance | Advances per-cell erosion timers and, when a cell’s threshold expires, clears the timer and submits the corresponding terrain update. |

## Per-frame interaction and terrain

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x426520 | MainSimInteractionAndCellUpdate | A large per-frame processor for pointer-derived map coordinates, cell selection, eligibility, commands, and UI/message branches. EngineTick calls it directly. |
| 0x431560 | CurrentLevelGet | Returns the global terrain/map context pointer. |
| 0x431570 | CurrentLevelHeightGet | Reads the dword at context offset +4; TerrainGridPathSearch uses it as a Y bound, supporting the height interpretation. |
| 0x44f4e0 | TerrainCellSurfaceTypeSet | Replaces a cell’s surface-type byte, returns the prior value, and updates related visual attributes. |
| 0x44f540 | TerrainCellSurfaceTypeGet | Reads the same per-cell surface-type byte. |
| 0x44f570 | TerrainCellSurfaceTypeReset | Applies the white/default surface appearance and clears the per-cell type byte. |
| 0x44f5c0 | TerrainCellSurfaceAppearanceApply | Writes a lookup-selected set of surface appearance values to a cell record. |
| 0x44f870 | TerrainWorldPointToCell | Converts world X/Z into cell coordinates and can also return the within-cell offset. |
| 0x44fb90 | TerrainSurfacePointProjectOffset | Selects a cell triangle/edge, projects a point, and offsets the output along the terrain normal; the exact selector meaning remains uncertain. |
| 0x4500d0 | TerrainCellSurfaceVerticesGet | Bounds-checks a cell, reads its 12 surface-vertex values, and swaps vertex groups according to the cell orientation flag. |
| 0x46a730 | CameraMoveCommandRoute | Runs the camera-context update and conditionally schedules an update callback. The callback’s full role is not established here. |
| 0x435a50 | MainSimFeatureFlagLatch | If a global feature bit is set and predicate 0x46a4c0(0x40b) passes, latches bit 1 in a simulation-state word. |

## Resource, text, and rendering helpers

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x480220 | ResourcePathResolveAgainstBase | Resolves a relative path against the global base path, normalizes it, checks that the result remains under that base, and returns the shared path buffer or null. |
| 0x4850f0 | TextBufferCreate | Allocates and initializes a text object and its owned content buffer. |
| 0x485190 | TextBufferDestroy | Frees the text object’s owned content buffer and object. |
| 0x485390 | TextBufferDrawWrapped | Renders the control-byte text stream, treating 0x07 as termination and 0x0a as a line break, with glyph advance and wrap handling. |
| 0x479170 | ConfigRgbVectorRead | Parses a three-component config value and converts the integer channels to normalized float outputs. |
| 0x483270 | RenderObjectColorVectorSet | Clamps three color components and writes a selected 12-byte color vector into a render record; meanings of the vector slots are not fully established. |
| 0x477610 | CaseInsensitiveSubstringFind | Returns the first case-insensitive occurrence of one string within another. |

**Evidence:** [legorr-engine-helpers-expansion-2.bundle](../evidence/legorr-engine-helpers-expansion-2.bundle)
contains the batch decompilation for these procedures and the follow-up terrain/tick procedures.
