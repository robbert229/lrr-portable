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
`0x20000000`. A packed-asset scan found that the configured PathMap files
present in the shipped archives cannot set this flag on those borders during
initial PathMap loading: all their top/left values are zero and their config
entries have no bias suffix. Three other configured PathMap files are absent
from the archives. The binary also has a separate dynamic writer, so the
static data scan does not rule out a later boundary cell reaching the helper.
The trace does not establish that the read crashes; its result depends on
bytes preceding the grid allocation.

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

**Packaged-data check.** `Lego.cfg` in `reference/LegoRR1.wad` contains 20
distinct `PathMap` paths; none has a `:<bias>` suffix, so the path-loader bias
defaults to zero. Seventeen of those paths resolve in `reference/LegoRR0.wad`;
all cells on each map's `x=0` or `y=0` border are zero. The other three
configured paths (`Level13`, `Level14`, and `Level24`) are absent from that
archive. The scan used the row-major u16 cells in each `MAP ` image, as
documented in Finding 7. Archive SHA-256 hashes are
`e02fa495f144d31142ef01a15221b1b7865d2a8264c43700523f2b52f91eaf68` for
`LegoRR0.wad` and
`5a967d3147533c61390120f923cea2bd275cc29bc7a8563de21c62bd9de6229d` for
`LegoRR1.wad`.

The separate writer `FUN_00432600` (`0x432600`) can set the same cell bit
when the target has flag `0x200000`, its type byte is not 6, and flag
`0x80000000` is clear. It calls `FUN_0042f6f0` for the target and four adjacent
cells; the writer itself does not check those coordinates against map
dimensions. The helper's own checks still apply, including its acceptance of
coordinate zero. At `0x43e77f`, the entity callback at `0x43c810` processes
`+0x3e4` bit `0x200` when the `+0x2d4` timer has expired and bits `0x100000`,
`0x4`, and `0x8` in `+0x3e4`, bit `0x20000` in `+0x3e0`, and bit `0x10000`
in `+0x3e0` are clear. The command-type `0x16` handler at `0x40468c` calls
`FUN_00448c50`, which sets `Activity_Place` (activity index 30) and raises
`+0x3e4` bit `0x200`. The callback derives the entity cell coordinate with
`FUN_004424f0`, exact-searches the cell-descriptor list with `FUN_004090c0`,
and calls the writer only if a descriptor exists and bit 2 is set in its byte
at `+0xd0`. The conversion and exact list lookup have no map-range check.
This is a conditional route for a boundary entity to reach the writer; it does
not prove one does so at runtime.

The descriptor gate is not restricted to interior cells in the traced
creator. `FUN_00409300` creates a descriptor for `DAT_004ddcc4` and sets its
`+0xd0` bit `0x2` when `FUN_00433090` returns zero; that helper bounds-checks
the coordinates and, for an in-bounds cell, returns its `0x20000000` flag.
Thus an in-bounds edge coordinate such as `x=0` or `y=0` with that flag clear
passes this condition. The UI-action dispatcher `FUN_0041dca0` calls the
creator for cases 5 and 7 after confirming no descriptor already exists at
that coordinate. These static gates permit a boundary descriptor, but do not
establish that the action is taken there or that a matching entity reaches
the later callback.

At `0x456dcf`, the writer is called from callback-dispatch branch 4 after
`FUN_00432c90` updates the cell. A static name/function pair associates the
string `SetTutorialBlockIsPath` and wrapper `FUN_00457420` at file offsets
`0xa4db4`/`0xa4db8`; the wrapper supplies context 4. `FUN_00456b80` builds
per-value coordinate lists from the level's voxel byte `+0x04`, and the
callback walker applies the update to the selected list. The table consumer
and runtime script invocation remain untraced. The shipped
`MoveTuto_01\M01.nrn` script contains conditional calls
`SetTutorialBlockIsPath 4 1` and `SetTutorialBlockIsPath 11 1`. Its configured
`TerrainMap`, `Surf_M01.map`, is 30 by 26: value 4 occurs only at `(18,12)` and
value 11 does not occur. Those script calls therefore identify no top- or
left-edge cell in this shipped tutorial map. The static script text does not
establish that either conditional call executes in a play session.

Neither call site proves that an out-of-range neighbor reaches
`FUN_0042f6f0` or that its later `0x400` read executes. The tutorial-map scan
also does not cover levels with different `TerrainMap` contents, modified
assets, or all possible entity positions. These remain conditional code paths,
not observed runtime behavior.
