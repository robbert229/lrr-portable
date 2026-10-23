# Finding 227 — Newly Identified Engine and Terrain Helpers

The manifest audit and direct decompilation batch identified nineteen omitted
procedures. Their behavior is now named in the Ghidra target script and captured
in the generated engine-region corpus. Names describe observed behavior; details
that remain uncertain are qualified below.

## Simulation, selection, and terrain

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x42cf50 | MiniFigureTypeTableLoad | Enumerates `MiniFigureTypes`, allocates 0x74-byte parsed records and a parallel name table, parses each configuration record, and duplicates its name. |
| 0x42de40 | ViewportTargetPositionClamp | Derives a map-cell or entity target position, clamps cell coordinates to terrain bounds, stores the selected target slot, and marks target-position state dirty. |
| 0x42c5b0 | TerrainCellByteStateClassify | Maps a terrain cell's byte state at offset +5 to one of four caller-provided classification slots; returns false when the state has no mapping. |
| 0x428020 | EntityCommandStatusFeedbackDispatch | Builds and dispatches selected-entity interaction status text and message/audio cues, suppressing repeated feedback for the same entity. |
| 0x428330 | TerrainCommandFeedbackDispatch | Builds and dispatches terrain-cell command feedback text and cues, with duplicate-cell guards and outcome-specific message categories. |
| 0x44e880 | TerrainGridCellsInitialize | Traverses the terrain dimensions and invokes the per-cell initializer for each cell. |
| 0x44edf0 | TerrainCellOrientationToggleAndRebuild | Toggles a cell orientation bit, rotates four 9-dword data blocks, rebuilds surface vectors, and updates neighbors. The exact meaning of the blocks is unresolved. |
| 0x4514f0 | TerrainCellSurfaceVectorsRebuild | Transforms bounded groups of four 9-dword cell vectors. The evidence does not establish that these values are normals. |
| 0x46a4c0 | FlagMaskCallbacksAllPass | Iterates set bits in a mask, calls the supplied callback for each bit, and returns false as soon as the shared continuation flag is cleared. |
| 0x431a30 | SelectedEntitiesAnyPredicateMatch | Calls the supplied predicate for selected entities and returns 1 on the first nonzero result, otherwise 0. |

## File, renderer, and allocator helpers

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x48f650 | GameFileDelete | Wraps `DeleteFileA`, records the resulting error status, and reports success or failure. |
| 0x48f6d0 | GameDirectoryCreate | Wraps `CreateDirectoryA` and reports its result through the engine file-status path. |
| 0x48f700 | GameFileMetadataFind | Wraps `FindFirstFileA`, fills the engine file-metadata record, and updates global error status. |
| 0x48c120 | LwoTextureProjectionModeParse | Parses `PLANAR`, `CYLINDRICAL`, and `SPHERICAL` case-insensitively to projection modes 0, 1, and 2; unknown text maps to 0. |
| 0x489d90 | DirectXErrorMessageFormat | Assembles DirectX error context and forwards it to the shared formatted error reporter. |
| 0x48dbf0 | EngineHeapAllocateWithRetry | Attempts aligned engine allocation and, when retry is enabled, invokes the reclamation helper and retries. |
| 0x48dc40 | EngineHeapAllocateAligned | Rounds allocation size up to 16 bytes, uses the engine pool below its configured cutoff, and otherwise falls back to `HeapAlloc`. |
| 0x491900 | EnginePoolAllocate | Allocates from size-segregated 4 KB pages with 16-byte slots, obtaining backing pages from `VirtualAlloc` when needed. |
| 0x4909d0 | EngineFormatStringToStream | Formats printf-style integer, floating-point, and string conversions into the engine output stream. |

**Evidence.** Direct decompilation of all nineteen procedures from the shipped
`reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), retained in
[`legorr-engine-helpers-expansion-3.bundle`](../evidence/legorr-engine-helpers-expansion-3.bundle)
(`ev_69fcfae5b26c15d810f32f8ed1bb85b09bc4e84c49f617200767116102d47d36`). The
named pseudocode is in `docs/decompiles/legacy/engine-region.c` at the corresponding
addresses.
