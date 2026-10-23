# Finding 223 — Cell Initialization Reads Before the Grid at Top and Left Edges

The cell-variant helper checks whether each of four neighbors is inside the
map before reading that neighbor's `0x20000000` flag. If the neighbor is
outside the map, it sets that flag result to zero, then still checks the
neighbor's `0x400` flag using the original, unchecked coordinates. During
cell initialization, the helper is called for cells carrying `0x20000000`.
At the top or left edge, one or more of its neighbor coordinates are negative,
so the `0x400` check reads before the allocated cell grid. The read contributes
to the four-bit neighbor mask used to select the cell variant.

This is conditional on a cell at the top or left edge carrying
`0x20000000`. The static trace establishes the path and out-of-grid address;
it does not establish that a shipped level has such a cell at an edge or that
the read crashes. The result depends on bytes preceding the grid allocation.

**Evidence.** In shipped `reference/LegoRR.exe` (SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
`FUN_0042f350` (`0x42f350`) checks neighbor coordinates against zero and the
map dimensions before loading that cell's `0x20000000` bit. At `0x42f3d3`,
if the bit result is zero, execution falls through to `0x42f3d7`, which
calculates the index again from the unchecked coordinates and executes a
memory `test` of the `0x400` bit at `0x42f3e8`. Its four offsets are
`(-1,-1)`, `(0,-1)`, `(-1,0)`, `(0,0)`. `FUN_0042f6f0` (`0x42f6f0`) calls
the helper when the current cell has `0x20000000`; its bounds checks reject
the last row and column but permit coordinate zero. The map-load pass invokes
it over every row and column. The cell grid is allocated at exactly
`width * height * 0x48` bytes in `FUN_0042b510`, so a negative
row-major index points before that allocation. `FUN_0042c760` sets
`0x20000000` for a qualifying path/AI layer cell when the layer value minus
its bias equals 2 and the existing cell flag `0x8` is set. The disassembly
and corresponding decompilation are in `docs/decompiles/full/LEGORR.full.c`.
