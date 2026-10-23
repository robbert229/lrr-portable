# Finding 233 — Terrain, Rendering, and Stream Primitives

Three focused audits linked remaining terrain state checks, LWO mesh access,
render-state caching, formatted diagnostic buffers, and low-level file/text
helpers to their callers.

## Terrain

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x40d240 | TerrainCellHasOccupiedOrFlaggedNeighbor | Checks the center cell and scans its eight neighbors for occupancy or a qualifying special-state flag. Callers use it in terrain action and entity action eligibility paths. |
| 0x40d3f0 | TerrainCellSpecialStatePredicate | Checks the cell's occupancy/type bits and, when requested, its 0x100 state bit. Callers use it in terrain-cell eligibility and neighborhood decisions. |
| 0x40de40 | EntityFenceSparkTerrainInteraction | Resolves an entity's terrain cell, checks nearby cell state, updates the local terrain neighborhood, and sets a timer using the FenceSpark type. The exact trigger semantics remain uncertain. |
| 0x432aa0 | TerrainCellOccupancyOrFlagCheck | With its mode clear, tests cell word bit 0x400; with mode set, tests that bit or the byte flag at cell offset +0xc, bit 2. |
| 0x432170 | TerrainCellFlag100Test | Tests bit 0x100 in the terrain cell word at offset +0xc. |

## Rendering and mesh data

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x474c40 | LwoLayerPolygonVertexCount | Returns a layer's polygon vertex count through a cached helper or the layer interface. A caller uses it to size a 36-byte-per-vertex buffer. |
| 0x474cc0 | LwoLayerPolygonVertexDataRead | Reads polygon vertex records from cached layer storage or dispatches through the layer interface. Records are nine dwords (36 bytes); one caller mirrors UVs in the generated layer. |
| 0x4788d0 | Direct3DRenderStateSetCached | Sets a Direct3D render state and updates the per-state shadow table used to avoid redundant state changes. |
| 0x485ff0 | SurfaceDrawPolyline | Packs the requested surface color and walks line segments through the clipped line callback helper. Callers draw selection and terrain outlines. |
| 0x486720 | BresenhamLineWalkClipped | Walks a clipped Bresenham-style line and invokes the supplied callback for covered grid points. |

## Text and runtime helpers

| Address | Name | Observed behavior |
| --- | --- | --- |
| 0x47f600 | DebugOutputFormat | Formats a variadic message into a stack buffer and sends it to OutputDebugStringA. File routines use it for operation diagnostics. |
| 0x485230 | TextBufferAppendFormat | Variadic wrapper for appending formatted text to a ring buffer. |
| 0x485250 | TextBufferAppendVFormat | Formats into a temporary buffer, skips through a requested number of BEL markers, then appends bytes to a ring buffer, wrapping and setting its pending flag at capacity. |
| 0x493c10 | FpuControlWordUpdateMasked | Updates selected x87 control-word bits and returns the previous control word. The Ghidra pseudocode omits the stack arguments; the evidence bundle includes the instruction-level observation. |
| 0x494c70 | ScanStreamGetChar | Returns the next buffered scan byte, refilling the stream when its current buffer is empty. |

**Evidence.** Direct pseudocode for all fifteen targets is in
ev_69fb8af69a0830d77ac3a29d14aacffc74f55955d86c67211a5a89bb1712f501,
preserved in
[legorr-engine-helpers-expansion-8.bundle](../evidence/legorr-engine-helpers-expansion-8.bundle).
Caller audits for terrain, rendering/mesh, and text/stream routines are in the
[terrain](../evidence/legorr-engine-helper-audit-terrain-expansion-8.bundle),
[rendering](../evidence/legorr-engine-helper-audit-render-expansion-8.bundle),
and [text/IO](../evidence/legorr-engine-helper-audit-text-io-expansion-8.bundle)
bundles. Analyses used shipped reference/LegoRR.exe, SHA-256
c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f.
