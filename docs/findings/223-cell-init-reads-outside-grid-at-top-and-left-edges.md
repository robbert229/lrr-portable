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

**Movement does not supply the missing border reachability.** The simple
walkability branch of `FUN_00432770` checks only the upper bounds
(`x < width-2`, `y < height-2`) and the cell flags/type; it has no lower-bound
check, so it can report `x=0` or `y=0` as walkable. But both the target seed in
`FUN_00430ee0` and the traceback/update gate in `FUN_00430530` explicitly
reject `x=0` or `y=0`. The normal wave-flood path builder therefore cannot
establish a route onto the top or left border. A border entity would need
another source of placement or movement, or a callback would need to update
that border cell directly. In configured `Level22`, `Dugg_22.map` has raw
predug wall cells at `(1,18..24)` and `(1,37)`; the loader's eight-neighbor
walkability writes consequently mark twelve left-border cells (`x=0`,
`y=17..25` and `y=36..38`). Their decoded terrain types also pass the simple
walkability type filter. This is concrete evidence that a border cell can be
marked walkable in shipped data, but it does not overcome the path-builder's
lower-bound gates.

**Startup object positions do not place a unit on the border.** `FUN_004424f0`
reads an object's position and passes its two coordinates through
`FUN_0044f6d0` to `FUN_0044f700`; the latter converts them to integer grid
coordinates with `__ftol` before checking the map bounds. I scanned every
`OListFile` path in `Lego.cfg`: all 34 are present in `LegoRR0.wad`, containing
462 objects with both `xPos` and `yPos`. The smallest values are `xPos=5.203455`
and `yPos=3.39245`, so none of these startup objects converts to `x=0` or
`y=0`. This narrows the remaining entity route to positions created during
gameplay or otherwise changed after OList loading; it does not cover entities
spawned later by scripts, emerge triggers, or other runtime systems.

Blocked OList objects can be created later, but this does not introduce a new
coordinate source. In `FUN_0042e020`, an object whose converted cell lacks the
walkability flag is recorded by `FUN_00437c20`, including its original world
position and grid coordinate. Terrain updates call `FUN_00437940`; when the
updated coordinate matches a queued record, it creates the configured entity
and calls `FUN_004428a0` with the saved position. That deferred path does not
reconvert or range-check the saved coordinates before placing the entity.
Because every shipped OList position already converts to an interior cell, the
deferred route also does not place a shipped startup object on the border.
The `Precreate` branch in level-load routine `FUN_00429890` separately creates
ten type-2 entities and passes each to `FUN_00437540`; that pass contains no
position assignment, so it is not evidence of a border spawn.

The Teleport Man action provides a separate runtime type-2 creation path.
`FUN_0041a300` maps menu slot 1 to `Interface_MenuItem_TeleportMan`, and
`FUN_0041dca0` handles that slot by calling `FUN_00434e80(2,0)`. The helper
queues that type/subtype through `FUN_0043f150` when the matching teleport
ability is available. The entity callback at `0x43c810` directly calls
`FUN_0043f190` at `0x43ca38` when its teleporter-stat gate passes. The worker
consumes the queued type, creates the minifigure through `FUN_00438610`, reads
the servicing entity's scene-frame position, and passes those coordinates to
`FUN_004428a0` for the new entity. The traced worker does not call the
pathfinder or check the resulting grid coordinate against map bounds, so the
spawn bypasses the normal path-to-border restriction. It could place a
minifigure at `x=0` or `y=0` if the servicing entity's scene origin is there.
The shipped OList scan above establishes only that configured startup objects
are interior; whether gameplay can place a servicing teleporter on a border
remains open.
This route also does not establish that a teleported minifigure receives the
Place activity or reaches `FUN_00432600` before it moves away.

Vehicle `GetOut` supplies another gameplay-time position source. Menu slot
`0x19` dispatches to `FUN_004401b0(vehicle, 0)`. After confirming the vehicle
has a carried minifigure and converting the vehicle's scene position to an
in-bounds grid cell, that routine first tries the same cell for vehicles with
the `0x2000000` stat flag. Otherwise, when the vehicle's `+0x3ec` flag `0x40`
is clear, it may call `FUN_00439e10`; that helper requires vehicle stat flag
`0x10` and delegates to `FUN_00439d50` to select an adjacent unload cell.
Candidates are checked north, east, south, then west. `FUN_00432b50` checks
only the upper map limits and walkability flag `0x8`. The following
`FUN_00431da0(NULL, x, y, x, y, 0)` checks the candidate tile's flags and
occupancy but has no lower-bound test. If the earlier three candidates are
rejected and the west candidate at `(0,y)` passes those checks,
`FUN_004401b0` converts that selected cell to world coordinates and directly
assigns the carried minifigure's frame position; it does not pathfind the
minifigure onto the cell. The bounds check in the conversion helper accepts
zero coordinates. This establishes a conditional route for unloading onto
the left edge (and the analogous north candidate can reach `y=0`): a vehicle
must reach the adjacent interior cell, carry a minifigure, meet the unload
gates, and have a qualifying border tile while preceding candidates fail.
The `Level22` left-border cells described above provide shipped map cells
marked walkable, but this static trace does not establish a play-session
vehicle position, successful unload, or that the unloaded minifigure remains
there and executes the activity that reaches `FUN_00432600`.

The Place-command gate does not add an edge-coordinate rejection. In
`FUN_00404fc0`, command type `0x16` is accepted only for entity type 2, then
calls `FUN_00432550` with the command coordinates; that function is an
unconditional `return 1`. The dispatcher branch at `0x40468c` calls
`FUN_00448c50`, which sets `Activity_Place` and entity flag `+0x3e4 & 0x200`.
This makes a post-unload minifigure eligible through the traced type and
coordinate checks, subject to the dispatcher's other entity-state gates. The
callback still requires its timer and state-bit gates, a matching cell
descriptor, and descriptor byte `+0xd0 & 2`; these checks are not established
for an unloaded border minifigure.

The matching descriptor can also be created for an in-bounds border cell.
`FUN_0041ea50` stores the menu-selected map coordinate in `DAT_004ddcc4/8`;
`FUN_0041d040`'s menu-action case 7 requires no existing descriptor and a
cell type other than 6, without rejecting zero coordinates. The action
dispatcher `FUN_0041dca0` handles case 7 by calling `FUN_00409300`, which
uses `FUN_00433090` to require `0 <= x < width` and `0 <= y < height` and a
clear `0x20000000` cell flag. Thus `x=0` and `y=0` pass the coordinate bounds;
the creator adds the descriptor, sets its `+0xd0 & 2` callback bit, and sets
cell flag `0x40000000`. This removes descriptor creation as an inherent
interior-only gate, though it does not establish that the menu action is
taken for an unloaded minifigure's edge cell during play.

The writer's required cell flag `0x200000` also has a possible gameplay-time
edge source. The predug loader sets this flag through `FUN_00430530(..., 1)`,
but that routine rejects coordinates with `x=0` or `y=0`, so this loader path
does not set it on the left or top border. Separately, `FUN_00432c90` sets
`0x200000` when the cell's low two state bits are zero and the flag is not
already set; it contains no map-coordinate check. The lava-neighbor path can
call `FUN_0040f330` on a neighboring cell after `FUN_0040f2b0` accepts it via
`FUN_00432b50`, whose bounds test also omits lower limits. Thus an edge cell
can enter that mutator if the terrain and erosion gates pass. This remains a
static possibility: the traced maps and callbacks do not establish that the
lava-neighbor path sets `0x200000` on one of the relevant border cells during
a play session.

There is also a direct debug-input route to the same cell mutator. The level
input handler `FUN_00426520` converts the pointer position to map coordinates
in `local_34/local_38` with `FUN_004502e0`, then calls `FUN_004288e0` when
`DAT_00556ed0 & 0x10` is set. `FUN_004288e0` detects a newly pressed key at
DirectInput keyboard-buffer index `0x1e` and passes those cursor coordinates
to `FUN_0040f330`. That mutator repeatedly calls `FUN_00432c90`; once the
target cell's low two state bits reach zero, `FUN_00432c90` sets
`0x200000` and immediately calls `FUN_0042f6f0`. The path has no explicit
lower-bound rejection, so it can reach the edge-sensitive helper if the
cursor resolves to `x=0` or `y=0` and the cell reaches that state. The
`0x10` gate is loaded from `AllowDebugKeys` (or enabled by the debug-build
predicate); shipped `Lego.cfg` sets `AllowDebugKeys FALSE`. This therefore
establishes a debug-enabled code route, not ordinary shipped-config runtime
reachability or an observed crash.

The entity callback also has a deferred type-`0xc` cell-update branch. The
shipped-image disassembly at `0x43e824` calls `FUN_00403d10(entity, 0xc, 0,
1)` after the entity's `+0x3e0 & 0x40000` state is set and its timer expires.
`FUN_00403d10` passes the queued cell coordinate to `FUN_00432c90`, which in
turn calls `FUN_0042f6f0`. One concrete producer is `FUN_0043abf0`: it requires
the entity's current cell to carry `0x20000000`, then calls
`FUN_00430320(cell, 0x3f)`. `FUN_00430320` clears `0x20000000` before queuing
the type-`0xc` updates when its mutation gates pass. Since `FUN_0042f6f0`
calls the edge-sensitive `FUN_0042f350` only when the current cell still has
`0x20000000`, this
deferred route does not by itself establish the out-of-bounds read; that flag
would have to be restored before the event updates the cell. The callback's
coordinate conversion and bounds helper accept zero, so an edge cell is not
ruled out at the source, but no such interleaving is established here.

The setter census narrows the possible flag-restoration path. In the full
decompilation and shipped-image disassembly, the cell-grid stores that set
`0x20000000` are the PathMap loader `FUN_0042c760` and dynamic writer
`FUN_00432600`. The latter has two direct call sites: entity callback
`0x43e77f` and tutorial callback `0x456dcf`. The PathMap loader runs during
level construction, before the deferred update producer can clear the flag.
Consequently, a post-clear restore through the directly traced paths would
need one of those callback gates to run on the same cell before its queued
update is consumed. This direct-xref and literal-store census does not rule
out an indirect call or an aliased store, and it does not establish that the
required ordering occurs.

One remaining level-data spawn route is `EmergeMap`. `FUN_0042c120`
(`LoadLayerL`) scans the complete layer and groups nonzero cells into 0x4c-byte
records: slot 0 is the trigger coordinate, and slots 1–5 are candidate spawn
coordinates. The entity callback calls `FUN_0042c330` at `0x43c9cf`; that
routine waits for the current cell to match the record's trigger and its timer
to expire, then tries each populated candidate through `FUN_0043af30`. That
spawner requires the candidate to pass the walkability and occupancy gates;
the walkability check `FUN_00432bd0` tests only upper bounds, so `x=0` or
`y=0` can pass if its cell flags qualify. `Lego.cfg` configures ten
`EmergeMap`s. `Level09`, `Level13`, `Level16`, `Level20`, `Level21`,
`Level23`, `Level24`, and `Level25` are present in `LegoRR0.wad`; all eight
have zero-valued top and left borders. The configured `Level11` and `Level14`
maps are absent. Thus none of the available EmergeMap assets supplies a
border trigger or candidate; the two missing maps remain unknown. The seven
distinct configured ErodeMap files are all present and also have zero-valued
top and left borders. This narrows the shipped-data case but does not rule
out the callback-driven lava-neighbor path setting `0x200000` on an edge cell
during play.

That spawn route does not by itself reach the risky cell update. `FUN_0043af30`
creates entity type 3 and the creation path identifies it as a RockMonster,
LavaMonster, or IceMonster. The `0x16` command that dispatches to
`FUN_00448c50` (the setter for the entity's `Activity_Place` bit) is filtered
by `FUN_00404fc0`, which rejects that command unless the candidate entity type
is 2. Thus an emerged type-3 creature cannot receive the traced Place command
that leads to `FUN_00432600`. The missing emergence maps could still spawn
border creatures, but this route does not establish that those creatures call
the dynamic writer; a separate type change or callback path would need to be
found.

At `0x456dcf`, the writer is called from callback-dispatch branch 4 after
`FUN_00432c90` updates the cell. A static name/function pair associates the
string `SetTutorialBlockIsPath` and wrapper `FUN_00457420` at file offsets
`0xa4db4`/`0xa4db8`; the wrapper supplies context 4. `FUN_00456b80` builds
per-value coordinate lists from the level's voxel byte `+0x04`, and the
callback walker applies the update to the selected list. The NPL table
consumer is `FUN_00453320`. The shipped
`MoveTuto_01\M01.nrn` script contains conditional calls
`SetTutorialBlockIsPath 4 1` and `SetTutorialBlockIsPath 11 1`. Its configured
`TerrainMap`, `Surf_M01.map`, is 30 by 26: value 4 occurs only at `(18,12)` and
value 11 does not occur. Those script calls therefore identify no top- or
left-edge cell in this shipped tutorial map. The configured `M01.npl` contains
the corresponding function-table call words `0x20089` at indices 289 and 296,
followed by arguments 4 and 11. Table entry `0x89` at `0x4a5fb4` points to the
`SetTutorialBlockIsPath` name and wrapper `0x457420`. An archive-wide scan of
`.nrn` and `.npl` files found no other use of this command. The `.nrn` source's
conditional calls therefore correspond to the configured compiled script,
but whether either condition executes in a play session remains unobserved.

The tutorial callback dispatcher also has a separate
`SetRockMonsterAtTutorial` route. Its wrapper passes context 7 to
`FUN_00456cb0`; `FUN_00456b80` builds that routine's per-value coordinate lists
from the terrain byte at voxel `+4`, not from `BlockPointersMap`. The shipped
Level17 `17.nrn` source conditionally calls `SetRockMonsterAtTutorial 1` when
the stored crystal count reaches 10, and the configured 35-by-35 `Surf_17.map`
has material 1 along the complete top and left borders (69 distinct border
cells). The callback at `0x456e6c` passes each selected coordinate to the
type-3 spawner `FUN_0043af30`. That spawner first requires the selected cell to
pass `FUN_00432bd0`: its `0x10` walkability flag must be set and its `0x8` wall
flag clear. It then chooses one of the four adjacent cells for the actual
creature placement. The callback returns 1 both when the spawn succeeds and
when the spawner returns null, so `FUN_00456cb0` continues through the
material-1 coordinate list. The walkability helper accepts zero coordinates
but does not accept the final row or column; the available Level17 `Path_17.map` and
`Dugg_17.map` have zero-valued top/left borders and no predug edge-wall
markers adjacent to those borders. The script and terrain map therefore
provide a possible border-seed route, but the shipped data examined here does
not establish the required edge walkability state or a resulting border
placement. This type-3 spawn callback also does not call `FUN_00432600` or
`FUN_0042f6f0`, so it does not itself reach the edge-sensitive cell update.
The configured `17.npl` confirms the call family: its 32-bit words at indices
183, 189, 195, 201, 207, and 213 are all `0x2009f`, followed respectively by
arguments `1, 2, 3, 4, 3, 4`. `FUN_00452df0` loads the configured NERP file,
and `FUN_00453320` dispatches words with marker `0x20000` through the
12-byte function table at `0x4a5948`. Table entry `0x9f` at `0x4a60bc` points
to the `SetRockMonsterAtTutorial` name at `0x4a7454` and wrapper
`0x4572a0`. This matches the six calls in the shipped `.nrn` source and
confirms they are present in the configured compiled script. The source gives
their conditions, but whether any condition fires in a play session remains
unobserved. The `17.npl` payload SHA-256 is
`c47afcf846eaf6ad42dff4a65042831b6eaa341e51b2a51eb88d25e07e1ee830`.

An archive-wide `.npl` scan finds this function entry in two other configured
level scripts. Level14's `14.npl` contains calls with arguments 1 through 9;
its configured 37-by-35 `surf_14.map` has material 1 on 71 distinct top/left
border cells. The matching `Path_14.map` is absent from the shipped archives,
and `dugg_14.map` has zero-valued top/left borders with no predug edge-wall
markers adjacent to them. The spawner's required `0x10` walkability flag on
those candidate cells is therefore unestablished. DefenceTuto's `DE1.npl`
calls the function with arguments 4 and 8, but its configured `Surf_DE1.map`
contains neither material, so those calls have no coordinate-list entries in
the scanned terrain data. Level14 adds another shipped border-material
candidate route whose cell flags and runtime execution remain unknown; the
available data does not prove an edge spawn or the risky cell update.

The `0x10` walkability precondition has a runtime boundary-propagation path.
`FUN_00430470` writes this bit only to coordinates with nonzero `x/y` below
`width-2`/`height-2`, so it cannot set the border directly. `FUN_00430530`
also rejects an edge as its input coordinate, but its 8-neighbor pass can set
`0x10` on border neighbors when called for an interior cell at `x=1` or
`y=1`; that pass requires the center cell's type byte `+2` to be 8 or 10 and
its wall bit `0x8` to be clear. The predug loader calls it for `Dugg` values
1 or 3, but the Level14 and Level17 predug maps have no such values at
`x=1`/`y=1`, and Level14's corresponding `Erode` map has zero values there.
Other direct runtime call sites include `0x428dca`, `0x43cd3d`, `0x43ebbc`,
`0x4485d0`, `0x456d6a`, and `0x47018e`. Those callers leave a possible later
border-walkability source; their relevant live coordinates and cell states
remain unproven.

Neither call site proves that an out-of-range neighbor reaches
`FUN_0042f6f0` or that its later `0x400` read executes. These configured-level
script and map scans do not cover modified assets, all possible cell flags, or
all entity positions. The routes remain conditional code paths, not observed
runtime behavior.
