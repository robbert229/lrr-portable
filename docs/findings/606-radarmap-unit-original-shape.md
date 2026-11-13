# Finding 606 — RadarMap.c (GAME-036) rebuilt in original shape, with a port tail

Issue #651, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-036 was the unit map's "screen-coordinate map view rendering", 11
functions `0045d8b0`-`0045f030`. It is the original's radar map: the map
panel in the corner of the game screen.

- **RadarMap_Initialise** (`0045d8b0`) scales the 30-colour table from
  0..255 to 0..1, and lays out a seven-line arrow in 640x480 screen units
  scaled to the display.
- **RadarMap_Create** (`0045da90`) allocates the 0x3c-byte `RadarMap`. Lego.c
  keeps the level's radar map at `Lego_Level` +0x84. **RadarMap_Free**
  (`0045daf0`) frees it. The original's DirectShow and AVI players
  (`004726f0`, `0047e460`, `0047e980`) call the same address as their
  delete, so it is shared code (OpenLRR's `RadarMap_Free_UnwindMultiUse`).
- **RadarMap_Draw** (`0045dbc0`, 2,112 bytes, 0xb710 bytes of stack) fills one
  rectangle per visible block, coloured by **RadarMap_GetBlockColour**
  (`0045e9e0`). It outlines the blocks for which Lego.c's `00432d90` is set,
  and draws one rectangle per object through the RunThroughLists callback
  `0045e460`. In view mode 1 it also projects the screen's corners and the
  arrow from the main view onto the ground and draws them: the camera's
  footprint.
- **RadarMap_DrawSurveyDotCircle** (`0045db00`) draws a dotted circle, which
  LegoObject.c uses for an object's scan radius.
- **RadarMap_InsideRadar** (`0045e660`), **RadarMap_ScreenToBlockPos**
  (`0045e6d0`) and **RadarMap_TrySelectObject** (`0045e820`, with the callback
  `0045e8e0`) answer Lego.c's mouse queries. **RadarMap_TransformRect**
  (`0045e940`) maps a world rectangle to the screen.

#496's relocation put the unit verbatim into `src/game/map_view.cpp`. Those
functions were the port's models, with hook tables, guards and an LP64
record. This change rebuilds the unit as the C unit
[`src/game/radarmap.c`](../../src/game/radarmap.c), with:

- the C header [`radarmap.h`](../../src/game/radarmap.h);
- the unit-private imports header
  [`radarmap_imports.h`](../../src/game/radarmap_imports.h);
- the matching-build link names
  [`radarmap_link_names.h`](../../src/game/radarmap_link_names.h).

The port's implementation moves, unannotated, to the port tail
[`map_view_port.cpp`](../../src/game/map_view_port.cpp) (`git mv`, a `tail`
link-order entry and a Meson source).

"Before" is the *match baseline* on `main` at `18cc9483`. "After" is this
change, from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, 13 functions, 6,016 bytes) | 14.46% | **92.09%** |
| Exact functions in the unit | 1 | **7** (no asm) |
| Mapped functions in the unit | 10 | 13 |
| Exact-function code bytes (`binary-progress.json`) | 183,830 (28.61%) | **184,753 (28.75%)** |
| Exact functions, full / game-only | 1,582 / 1,380 | 1,588 / 1,386 |
| Accuracy, full / game-only | 84.26% / 82.73% | 84.45% / 82.94% |
| `opt_ref` annotated (full, by address): removed / exact removed | 500 / 243 | **490 / 236** |
| `opt_ref` annotated (game-only, by address) | 481 / 230 | 471 / 223 |
| Reccmp lint ledger (entries) | 1,239 | **1,237** |

The weights are the functions' spans up to the next function, padding
included. An unmapped function counts as 0%.

The names follow OpenLRR's `RadarMap_*` functions, in the repository's snake
case. "Link name" is the name the unit's definition takes in the matching
build (see [Link names](#link-names)).

| Address | Function | Link name | Bytes | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `0045d8b0` | `radarmap_initialise` | `screen_coord_normalize_init` | 480 | 8.51 | **100.00** |
| `0045da90` | `radarmap_create` | `ui_render_record_create` | 96 | 25.40 | **100.00** |
| `0045daf0` | `radarmap_free` | `ui_render_record_free` | 16 | 100.00 | **100.00** |
| `0045db00` | `radarmap_draw_survey_dot_circle` | `textured_scene_object_draw_at_position` | 192 | 18.29 | **100.00** |
| `0045dbc0` | `radarmap_draw` | `radar_map_tv_position_set_adapter` | 2,112 | unmapped | 94.05 |
| `0045e400` | `radarmap_can_show_object` | — | 96 | 91.67 | **100.00** |
| `0045e460` | `radarmap_callback_draw_object` | — (`static`) | 512 | unmapped | 98.20 |
| `0045e660` | `radarmap_inside_radar` | `rect_contains_point_half_open` | 112 | 92.31 (asm) | **100.00** |
| `0045e6d0` | `radarmap_screen_to_block_pos` | `terrain_point_pick_resolve_original` | 336 | 33.49 | 46.49 |
| `0045e820` | `radarmap_try_select_object` | `viewport_point_callback_dispatch` | 192 | 26.58 | 67.65 |
| `0045e8e0` | `radarmap_callback_try_select_object` | — (`static`) | 96 | unmapped | **100.00** |
| `0045e940` | `radarmap_transform_rect` | — | 160 | 22.05 | 84.11 |
| `0045e9e0` | `radarmap_get_block_colour` | — | 1,616 | 22.48 | 95.46 |

- **The function with no source** was `0045dbc0` RadarMap_Draw. Lego.c
  called a `PORT-ONLY` no-op adapter, `radar_map_tv_position_set_adapter`.
  It is now written and mapped.
- **Two functions the unit map did not list** are the RunThroughLists
  callbacks `0045e460` (draw one object) and `0045e8e0` (find the object under
  the mouse). Ghidra left them as labels. Each one's only reference is a
  `push` immediate in `0045dbc0` and `0045e820`. They are written as `static`
  functions with `// FUNCTION` annotations, and both are newly mapped. The
  unit is 13 functions.
- **The naked asm** of `0045e660` (92.31%) is gone. The C definition is exact.

Two callers outside the unit rose with no source change. Both call functions
that are now exact:

- `00425d90` `lego_terrain_pointer_command_update`: 88.26% to 89.26%;
- `00426250` `lego_update_render_viewport_state`: 94.74% to 97.37%.

### What is left

| Function | Score | Remaining difference and what was tried |
| --- | ---: | --- |
| `0045e6d0` RadarMap_ScreenToBlockPos | 46.49 | The original keeps the screen centre's two sums, `(CX - x) * scale` and `(CY - y) * scale` in named slots, loads a copied `blockSize` twice and negates the x product with `fchs`. MSVC folds every source form tried here: it pushes the negation into the shared `width * 0.5f` and keeps the products in registers. Tried (39.56%-46.49%): one expression per axis in both orders; `+ -(…)` and `- (…)`; `(x - CX)` against `(CX - x)`; scalar and `Point2F` world positions; `scale`, `blockSize`, `dx`/`dy` and `Point2F offset` locals in several declaration orders; `blockSize` before and after the offsets. `msvc_permuter.py` tried 2 rewrites, no gain. |
| `0045e820` RadarMap_TrySelectObject | 67.65 | The original zeroes the search position and radius through `xor eax, eax` before the first push and the result with an immediate just before the call; this compile uses four immediates after the pushes. Tried: field-by-field in both orders, chained assignment, an aggregate initialiser, integer stores. Register choices for the block position follow. |
| `0045e940` RadarMap_TransformRect | 84.11 | The original spills the centre's x to its slot and adds it as a memory operand (`fld [x]; fadd [esp]`); this compile keeps x in a register, spills y, and adds a redundant `fxch` pair after `y -= centre.y`. Tried: about 20 orders of the eight statements and the two centre assignments; `rect->x = rect->x + c`, `c + rect->x`; float, `Point2F` and four-float locals; `0.5f` and `/ 2`. Rule B: 0-129 probe declarations change nothing. `msvc_permuter.py` found no candidate. |
| `0045e9e0` RadarMap_GetBlockColour | 95.46 | MSVC tail-merges identical colour blocks. The original merges the ground switch's `11` into the wall switch's and keeps the wall switch's `9` (lake); this compile does the opposite. Tried: 11 positions of `case 6` in the wall switch; 24 orders of the wall cases 7/9/8 and the third switch's 6/7/9; `default` labels and explicit returns; the third switch's default as code after the switch; the top-level `else` as an early return. |
| `0045dbc0` RadarMap_Draw | 94.05 | Three local choices: `-(by * blockSize) + worldY` becomes `fsubr` where the original keeps `fchs; fadd`; the two `-= 1.0f` loads are scheduled together; and the two ray loops stride from the far point where the original strides from the near one. Tried: six forms of the y expression, a pointer and an index for the rectangle, a `nearPt` pointer, a `[2][4]` world-point array. |
| `0045e460` the draw callback | 98.20 | Two loads of the rectangle list are scheduled one multiply earlier than in the original. Tried: `size / 2`, a `half` local, the size assigned before the colour, the locals in three orders, the rectangle pointer removed. `msvc_permuter.py` tried 3 rewrites, no gain. |

No `Deviation (scoring only)`. The unit does not use the flag `0x800000`, and
the plain-immediate check (finding 584) passes.

## Unit extent

The high-confidence boundary at `0045d8b0`-`0045f030` is confirmed, with two
more functions inside it.

- **The data.** RadarMap.c's state is one `.data` block,
  `004a8f28`-`004a931c` (`radarmapGlobs`, 0x3f4 bytes). Only this unit's
  functions read or write it. Its colour table is initialised (30 RGB
  triples, 0..255). The rest is zero until RadarMap_Initialise runs.
- **The literal pool.** The `.rdata` run `0049e65c`-`0049e6ac` holds the
  unit's float literals: 1/255, the arrow's coordinates, 1/640, 1/480, 1.0,
  0.0, 0.7, -0.3, 0.9, 0.5, -1.0, 1/3, 0.25 and 1/6. GAME-037 starts a new run
  at `0049e6b4` and its `.data` at `004a931c`.
- **The previous function.** `0045d8a0` (GAME-035, ObjectField14Set) stores
  RadarMap_SetZoom's `+0x14`, but it reads no data and no literal. Lego.c
  calls it through `level_id_remap.cpp`'s adapter. It stays in GAME-035: an
  unreferenced header inline closing that object fits it as well as a first
  RadarMap.c function does, and nothing in the data decides it.
- **The calls.** The unit's functions call each other, Lego.c's block
  queries, Map3D, LegoObject.c, Stats.c, Main.c, Interface.c, Draw.c,
  DirectDraw.c, Viewports.c and Maths.c. Nothing outside calls the
  callbacks.
- **The name.** The binary does not name the file. The stem follows the
  Radar panel's identifiers (`PanelButton_Radar_*`,
  `Advisor_PanelPoint_Radar*`) and OpenLRR.

`translation-units.spec.csv` records this evidence and names the unit
`RadarMap.c` (`inferred`). `python3 tools/translation_units.py build`: 0
conflicts.

## How the conversion is done

### State

`RadarMap_Globs radarmapGlobs` (`004a8f28`), laid out as the original
`.data` and initialised with the image's colour table:

| Offset | Address | Field |
| ---: | --- | --- |
| `0x000` | `004a8f28` | `colourTable[30]` (RGB floats) |
| `0x168` | `004a9090` | `highlightX`: the block that blinks red, -1 when none |
| `0x16c` | `004a9094` | `highlightY` |
| `0x170` | `004a9098` | `arrowLines[2][20]`: the arrow's starts and ends (Point4F) |
| `0x3f0` | `004a9318` | `arrowLineCount` (7) |

No function writes `highlightX`/`highlightY` except RadarMap_Initialise.
They stay -1 in the shipped game.

`RadarMap` (0x3c bytes) is in `radarmap.h`: the map, the screen rectangle,
the zoom, the block size, the centre, the world bounds drawn, and the
object-rectangle list that the draw callback fills.

`make unit-data-offsets`:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-036 | n/a (1, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | - |

The unit's one `.data` item is `radarmapGlobs`; its `.rdata` floats are
literals in the code.

The image's `.data` grows by 0x3f4 bytes: `radarmapGlobs` sits beside the
port's own copy of the band in `frontend.cpp`, which stays in the port tail.

### The port tail

The port's radar map is another contract:

- an LP64 `UiRenderRecord` and the `TerrainPointPickRegion` view of the
  record;
- the hook tables that the native renderer installs (draw, pick, select);
- NULL guards;
- the explicit-grid `terrain_view_tile_geometry_build` and the palette words
  of `terrain_tile_style_resolve`.

As in findings 590 and 592-601, `map_view.cpp` is renamed `map_view_port.cpp`
and becomes a `tail` entry. Each `// FUNCTION` becomes a
`/* Port model of 0x… */` comment. The two static helpers
`terrain_tile_palette_index` and `terrain_tile_palette_copy` leave the lint
ledger, because tail files are exempt.

### Link names

In the matching build, `radarmap_link_names.h` gives eight unit functions
the port's names:

| Unit function | Link name | Callers |
| --- | --- | --- |
| `radarmap_initialise` | `screen_coord_normalize_init` | `lego_unit.c` (config init `0041fb50`) |
| `radarmap_create` | `ui_render_record_create` | `lego_unit.c` (`0042b510`) |
| `radarmap_free` | `ui_render_record_free` | `lego_unit.c` (`0042f0c0`), `avi_player.cpp`, `video_graph.cpp` |
| `radarmap_draw_survey_dot_circle` | `textured_scene_object_draw_at_position` | `legoobject_unit.c` |
| `radarmap_draw` | `radar_map_tv_position_set_adapter` | `lego_unit.c` (`00426250`) |
| `radarmap_inside_radar` | `rect_contains_point_half_open` | `lego_unit.c` (`00425d90`, `00426520`) |
| `radarmap_screen_to_block_pos` | `terrain_point_pick_resolve_original` | `lego_unit.c` (`00425d90`) |
| `radarmap_try_select_object` | `viewport_point_callback_dispatch` | `lego_unit.c` (`00425d90`) |

Every one of these names already had C linkage, and every caller passes the
original's arguments. RadarMap_Create's zoom arrives as the same stack word
that `lego_unit.c` passes as a `uint32_t`. So the callers reach the unit
with **no caller edit**:

- `map_view_port.cpp` compiles its definitions of the seven port names out
  of the matching build behind `LRR_MAP_VIEW_PORT_NAMES`, set by an explicit
  `#if` (finding 590). That includes the naked-asm
  `rect_contains_point_half_open` and the `PORT-ONLY` adapter
  `terrain_point_pick_resolve_original`.
- `lego_port.cpp`'s `PORT-ONLY` no-op `radar_map_tv_position_set_adapter` is
  compiled out of the matching build with a plain `#if`. Lego.c's
  `00426250` now draws the radar map in the matching build, as the original
  does.

Three unit functions keep their own names in every build:
`radarmap_can_show_object` (`0045e400`), `radarmap_transform_rect`
(`0045e940`) and `radarmap_get_block_colour` (`0045e9e0`). Only the unit
calls them in the original. Their port names
(`entity_type_and_state_flag_predicate`,
`view_point_to_screen_geometry_transform`, `terrain_tile_style_resolve`) have
C++ linkage and the port's contracts. The port tail keeps serving them for
the native renderer and the tests.

Natively the unit keeps its own names and nothing calls it. The port tail
serves every port name, so native behaviour is unchanged.

### Callees

All the unit's callees are converted, apart from the C library:

| Original | Called as | Unit |
| --- | --- | --- |
| Main_appWidth/Height `00401c30`/`00401c40` | `display_width_get`/`display_height_get` | GAME-001 |
| `0041b9b0` (the highlight switch) | `sim_status_banner_word_get` | GAME-020 |
| Lego_GetMap `00431570` and the block queries `00432170`, `00432aa0`-`00433330` | their `lego_unit.c` names | Lego.c |
| LegoObject_RunThroughListsSkipUpgradeParts `004377b0`, LegoObject_GetPosition `00442260` | `legoobject_unit.c` names | GAME-023 |
| Map3D_WorldToBlockPos_NoZ `0044f6d0`, `0044f810`, Map3D_BlockSize `00450890` | `terrain_map.c` names | GAME-026 |
| StatsObject_GetStatsFlags2 `00469e80` | `entity_state_flags_get` (link name) | GAME-043 |
| Viewport_InverseTransform `00477290` | `camera_screen_coordinate_project` (link name) | G98-003 |
| Maths_RayPlaneIntersection `004798e0` | `view_ray_intersection_test` | G98-007 |
| DirectDraw_Clear `0047ce20` | `directdraw_clear` | G98-010 |
| Draw_SetClipWindow, GetClipWindow, LineListEx, RectListEx, DotCircle | `draw.c` names | G98-019 |
| Mem_Alloc/Mem_Free (`malloc`/`free`) | `engine_heap_alloc`/`engine_heap_free` (`crt/vc5names.h`) | CRT |

Lego_GetMap (`00431570`) takes no arguments. As in `fallin_imports.h`, the C
matching build calls `lego_unit.c`'s definition through an unprototyped
function type, and the native C++ compile goes through `lego_port.cpp`'s
adapter.

No `extern "C"` was added to any existing declaration. The imports header
repeats the existing C declarations exactly. `test_c_boundaries.cpp` includes
`radarmap.h` and `radarmap_imports.h`. Step 0 had also added
`game/entity.hpp` there, which conflicts with `steering.h`; that include is
not kept.

### Caller switches

None. Every C caller already called the port name that the unit now takes.
The only Lego.c and LegoObject.c edits are comments:
`lego_unit_imports.h` and `legoobject_unit_imports.h` now say where the
radar map's names are defined. Comments do not move MSVC's symbol counter.
`lego_port.cpp` gains the `#if` around its adapter.

No file owned by #648 (`placement.cpp`), #649 (`gods98/avi_player.cpp`),
#650 (`gods98/bitmap_font.cpp`) or #652 (`text.cpp`) is edited.
`avi_player.cpp` calls `ui_render_record_free` through `heap.hpp` and is not
touched.

### Source shapes that matched

- **RadarMap_Draw** (`0045dbc0`, 69.16% at first):
  - **One Vector3F for two jobs** (to 89.78%). The original's frame has the
    half extents (`B+0x28`, `B+0x2c`) in the same slots as the frustum rays'
    direction vector, which stays live after the extents are last read.
    Separate locals make the frame 8 bytes larger and shift every array.
    The unit keeps one `vec`: `vec.x`/`vec.y` are the half extents, then each
    ray's direction.
  - **The outline corners inline** (to 93.69%). `right` and `bottom` locals
    compute both corners first. Writing `(rect.width - 1.0f) + rect.x` at
    each use lets MSVC compute the right edge, store it three times, then
    compute the bottom, as the original does.
  - **The counters zeroed as statements** after the centre is copied:
    `rectCount = 0; lineCount = 0; startY = 0; startX = 0;` (to 94.05%).
- **RadarMap_GetBlockColour** (`0045e9e0`, 92.12% at first): the wall
  switch lists lava (`case 6`) last, after 7, 9, 8, 10 and 11 (to 95.46%).
  MSVC then merges the wall and the ground switches' identical colour
  blocks the way the original does, except two (below).
- **RadarMap_TransformRect** (`0045e940`, 41.10% at first): the screen centre
  as a `Point2F` local computed first, `(width / 2) + x` (the original's 8
  bytes of frame), with the width scaled before `x += centre.x` and the
  height before `y = centre.y - y` (to 84.11%).
- **RadarMap_DrawSurveyDotCircle** (`0045db00`, 89.09% at first): the
  brightness in a new local, `brightness * 0.7f - -0.3f`, which MSVC places in
  the dead `wPos` slot as the original does (to 100%).
- **RadarMap_Initialise** (`0045d8b0`): `((float)width * (1.0f / 640.0f)) * x`
  in that order (97.62% to 100%).
- **The draw callback** (`0045e460`): an early `return 0` when the object
  cannot be shown (93.33% to 98.20%).
- **RadarMap_TrySelectObject** (`0045e820`): the block position as a
  `Point2I`, so it takes the original's 8 bytes of frame rather than dead
  argument slots (57.35% to 67.65%).

**Rule B (finding 538).** Sweeps of 0-129 probe declarations before the unit
(a throwaway experiment, not kept) move no function in the unit except
`0045dbc0`, and that one only down (94.05% to 93.32% from +42). The
remaining differences are source shape or the compiler's choices, not the
counter's phase.

## Entropy and drops

`make reccmp-check` against the baseline at `18cc9483`:

- 11 improved: 9 of the unit's addresses and the two Lego.c callers above;
- 3 newly mapped: `0045dbc0`, `0045e460`, `0045e8e0`;
- **none below baseline**, and no exact match lost. No drop to label or flag.

Instruction counts changed only at the unit's addresses (listing digests):

| Address | before | after |
| --- | ---: | ---: |
| `0045d8b0` | 58 | 84 |
| `0045da90` | 33 | 33 |
| `0045db00` | 85 | 55 |
| `0045dbc0` | — | 545 |
| `0045e400` | 35 | 33 |
| `0045e460` | — | 167 |
| `0045e6d0` | 104 | 96 |
| `0045e820` | 78 | 64 |
| `0045e8e0` | — | 38 |
| `0045e940` | 61 | 54 |
| `0045e9e0` | 211 | 460 |

`0045daf0` and `0045e660` keep their listings (`0045e660` was naked asm with
the same instructions). No other listing changed.

## `opt_ref` (finding 579, `make opt-ref-callers`)

| | before (`main` `18cc9483`) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full` (by address): removed / exact removed | 500 / 243 | **490 / 236** |
| `opt_ref.annotated.game_only` (by address) | 481 / 230 | 471 / 223 |

`make opt-ref-callers` lists no removed function in `radarmap.c.obj`: all
13 are kept, and so is every callee in the table above. The gain comes from
the link names. On `main`, Lego.c's `00426250` called the no-op adapter, so
RadarMap_Draw had no live caller and the port's helpers were what the
radar-map names reached. Now Lego.c's config init, level loader, level free,
mouse update and world update reach the unit, and the unit reaches its
converted callees. The change was not itemised per function against
`main`.

## Reccmp lint ratchet

1,239 → 1,237 entries (−2, no growth): `terrain_tile_palette_copy` and
`terrain_tile_palette_index`, now in a tail file.

## Native behaviour

Unchanged:

- the port tail serves every port name natively;
- `lego_port.cpp` keeps its no-op `radar_map_tv_position_set_adapter`
  natively;
- the unit compiles natively as C++ under its own names, and nothing native
  calls it.

There is no new native gap. The native radar map still runs on the port's
hook-table renderer. Retiring that waits on Lego.c's world update running
natively (#552).

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK. None below
  baseline and no exact match lost. The plain-immediate check (finding
  584), `winapi-link-check` and the link-order map check pass.
- `make match-progress` (baseline, listing digests, `binary-progress.json`)
  and `make unit-data-offsets`. `tools/binary_progress_inputs.txt` matches
  `binary_progress_hash.py --write-manifest`.
- `python3 tools/translation_units.py build`: 0 conflicts.
- `make tools-test`, without `--allow-growth`.
- `radarmap.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only`.
- Native: `cmake -S . -B build && cmake --build build && ctest --test-dir
  build`: 37/37 (synthetic fixtures).

## Rebased onto stack #656

These are the figures after rebasing onto #654 (`d37e2be9`, Font.c). That
base contains #655 (Text.c) on `main` `321fdbf1`. Neither touches the
radar map's files. The shared lists (`link_order.cmake`, `meson.build`,
`test_c_boundaries.cpp`, the lint ledger, `binary_progress_inputs.txt`)
keep both sides. #654's new `advisor.h` prototype for `00401c80` does not
reach `radarmap.c`, which does not include `advisor.h`. Every per-function
score above is unchanged on the rebase.

`make reccmp-check` against #654's baseline: the same 11 improved and 3
newly mapped, **none below baseline, no exact match lost**. Instruction
counts changed only at the unit's addresses.

| | #654 head | this change |
| --- | ---: | ---: |
| Exact-function code bytes | 185,499 (28.87%) | **186,422 (29.01%)** |
| Exact functions, full / game-only | 1,599 / 1,397 | 1,605 / 1,403 |
| Accuracy, full / game-only | 84.77% / 83.29% | 84.97% / 83.51% |
| `opt_ref` full: removed / exact removed | 497 / 244 | **488 / 238** |
| `opt_ref` game-only | 478 / 231 | 469 / 225 |
| Lint ledger | 1,233 | **1,231** |

`make opt-ref-callers` lists no removed function in `radarmap.c.obj`.

## Reproducing

    git mv src/game/map_view.cpp src/game/map_view_port.cpp   # and make it a tail entry
    # name GAME-036 RadarMap.c in the spec, then:
    python3 tools/translation_units.py build
    python3 tools/convert_unit.py GAME-036 --stem radarmap
    make match-function SOURCE=src/game/radarmap.c
    make reccmp-check
    make opt-ref-callers
    make match-progress
    make unit-data-offsets
