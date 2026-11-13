# Finding 593 — Viewports unit (G98-003) rebuilt in original shape

Issue #624, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
G98-003 was relocated verbatim into `src/gods98/viewports.cpp`. Those
functions were port models: guarded wrappers over the port's viewport pool,
three of them naked asm. This change rebuilds the unit as the C unit
[`src/gods98/viewports.c`](../../src/gods98/viewports.c), with:

- the C boundary header [`viewports.h`](../../src/gods98/viewports.h);
- the unit-private imports header
  [`viewports_imports.h`](../../src/gods98/viewports_imports.h);
- the matching-build link names
  [`viewports_link_names.h`](../../src/gods98/viewports_link_names.h).

The port's models move, unannotated, to the port tail
[`viewports_port.cpp`](../../src/gods98/viewports_port.cpp) (`git mv` of
`viewports.cpp`; a `tail` link-order entry and a Meson source). The native
port keeps running them. The unit is text `00476d50`–`00477440`: gods98's
Viewports.c.

- **The pool.** A Viewport is 0x20 bytes: an unused rectangle, the
  IDirect3DRMViewport2 at `+0x10`, the target field of view at `+0x14`, the
  rendering flag at `+0x18` and the free-list link at `+0x1c`.
  Viewport_AddList allocates blocks of 1, 2, 4, … records into
  `viewportGlobs.listSet[32]`.
- **Create.** Viewport_Create scales its four fractions by the device size
  and calls Viewport_CreatePixel. That wraps negative positions, checks the
  bounds, pops a record and calls IDirect3DRM3::CreateViewport through
  Main.c's `lpD3DRM()` and `lpDevice()`.
- **Render.** Viewport_Render eases the field of view towards
  Viewport_SmoothSetField's target, renders the root container's frame and
  then runs Mesh_PostRenderAll (`00482d60`). On an error it calls
  Error_DirectX (`00489dd0`) with the unit's `__FILE__` and line 286.
- **Clear.** Viewport_Clear clears the full viewport through the immediate
  mode IDirect3DViewport3::Clear2, filled with the scene's background
  colour, or calls `Clear(D3DRMCLEAR_ALL)`.

"Before" is the *match baseline* on `main` at `12a0e950` (after #626 and #627). "After" is this
change, from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (22 functions, weighted by function span, 1,776 bytes) | 28.96% | **98.59%** |
| Exact functions in the unit | 4 (3 of them naked asm) | **20 of 22** (no asm) |
| Exact-function code bytes (`binary-progress.json`) | 169,567 (26.39%) | **170,533** (26.54%) |
| Accuracy, full / game-only | 81.32% / 79.49% | **81.83% / 80.06%** |
| `opt_ref.annotated.full`: removed / exact removed | 498 / 229 | **492 / 225** |
| Reccmp lint ledger | 1,290 | 1,288 |

The weights are the functions' spans up to the next function, padding
included. "Name (matching build)" is the symbol the unit's definition has
there: fifteen take the port's name (see [Link names](#link-names)).

| Address | gods98 name | Name (matching build) | before | after |
| --- | --- | --- | ---: | ---: |
| `00476d50` | Viewport_Initialise | `scene_resource_slots_initialize` | 100.00 | **100.00** |
| `00476d80` | Viewport_Shutdown | `viewport_shutdown` | 52.17 | **100.00** |
| `00476dc0` | Viewport_Create | `render_target_viewport_scale_set` | 14.55 | **100.00** |
| `00476e50` | Viewport_CreatePixel | `render_target_viewport_create_original` | 14.29 | **100.00** |
| `00476f10` | Viewport_GetSize | `viewport_get_size` | 55.00 | **100.00** |
| `00476f50` | Viewport_SetCamera | `resource_interface_call_slot48` | 27.03 | **100.00** |
| `00476f70` | Viewport_GetCamera | `scene_object_query_value_if_ready` | 20.78 | **100.00** |
| `00476fb0` | Viewport_SetBackClip | `renderer_object_set_clip_distance` | 28.57 | **100.00** |
| `00476fd0` | Viewport_GetBackClip | `resource_interface_call_slot8c` | 100.00 (asm) | **100.00** |
| `00476ff0` | Viewport_GetFrontClip | `resource_interface_call_slot90` | 100.00 (asm) | **100.00** |
| `00477010` | Viewport_Clear | `ledger_stub_00477010_DDiSceneDrawBegin` | 15.38 | 96.30 |
| `00477150` | Viewport_Render | `ddiscene_draw_end` | 30.00 | 93.65 |
| `00477220` | Viewport_Remove | `render_resource_track_and_retain` | 58.33 | **100.00** |
| `00477240` | Viewport_SmoothSetField | `scene_current_object_set_original` | 28.57 | **100.00** |
| `00477250` | Viewport_SetField | `resource_interface_call_slot40_and_clear_field14` | 41.38 | **100.00** |
| `00477270` | Viewport_GetField | `viewport_get_field` | 100.00 (asm) | **100.00** |
| `00477290` | Viewport_InverseTransform | `camera_screen_coordinate_project` | 10.26 | **100.00** |
| `004772b0` | Viewport_Transform | `render_interface_transform_point` | 22.22 | **100.00** |
| `004772d0` | Viewport_WorldToScreen | `viewport_world_to_screen` | 17.24 | **100.00** |
| `00477310` | Viewport_GetScene | `viewport_get_scene` | 16.87 | **100.00** |
| `00477370` | Viewport_AddList | `viewport_add_list` | 12.77 | **100.00** |
| `004773e0` | Viewport_RemoveAll | `viewport_remove_all` | 23.16 | **100.00** |

Three Lego.c callers rose with their call operands:

| Address | Function | before | after |
| --- | --- | ---: | ---: |
| `0041fb50` | `lego_game_config_init` | 93.71 | 93.74 |
| `004295f0` | `lego_sound_minimum_distance_update_mode` | 97.56 | 98.37 |
| `00433e80` | `lego_terrain_projected_cell_state_refresh` | 93.84 | 94.29 |

The other callers (`advisor.c`, `sfx.c`, `terrain_map.c`,
`legoobject_unit.c`, `tutorial_script.c` and the rest of `lego_unit.c`)
already named the old annotated port functions at these addresses. They now
name the unit's, and their scores do not change.

## Unit extent

The map gave 21 of the 22 functions low confidence. The extent is
confirmed, and no function moves:

- **`.data` string.** `"C:\Dev\SourceSafe\gods98_dx6\gods98\src\Viewports.c"`
  (`004aaba4`) is the unit's `__FILE__`. Only `00477150` references it. It
  follows Container.c's `"%s_%s"` (`004aab90`, used by `00475d10` and
  `00475e40`) and `"DDiScene"` (`004aab98`, used by `00476770`). It comes
  before `"FALSE"` (`004aabd8`), which `00477710` uses.
- **Commons.** A scan of `.text` for operands in `0076ac00`–`0076ae00`
  finds only the unit's functions on `0076ace0`–`0076ad6b`:
  - `00476d50`, `00476d80`, `00477370` and `004773e0` use `0076ace0`
    (`listSet`);
  - `00476d50`, `00476d80`, `00476e50`, `00477220` and `00477370` use
    `0076ad60` (`freeList`);
  - `00476d50`, `00477370` and `004773e0` use `0076ad64` (`listCount`);
  - `00476d50` and `00476d80` use `0076ad68` (`flags`).

  `0076ac80`–`0076acc4` belong to `0047c170`–`0047e300`, and
  `0076ad80`–`0076adfc` to Container.c (`00472710`–`004759f0`).
- **`.rdata`.** The floats at `0049e8ac`–`0049e8b8` (0.0, 1.0, 4.0 and −1.0)
  are used only by `00477150`. IID_IDirect3DViewport3 (`0049f778`) is
  shared with Mesh.c (`00483580`, `00483600`).
- **Call graph** (`docs/coverage/call-graph/edges.csv`). The 22 functions
  call only each other plus:
  - Mem_Alloc and Mem_Free (`0048dbd0`, `0048db80`) and `__ftol`
    (`0048e060`);
  - Mesh_PostRenderAll (`00482d60`) and Error_DirectX (`00489dd0`).

  Their 16 callers outside the unit are all game code (`00401210`–`00465f40`).
- **Edges.** `00476d10`, the function before the boundary, has no direct
  caller. `00476850` (Container.c) pushes it as a callback
  (`push 0x476d10`), so it is Container.c's. `00477440` is utils.c's
  `Util_Tokenise`, called from about 100 sites.

`translation-units.spec.csv` records this evidence in G98-003's note. The
boundary confidence is computed by `tools/translation_units.py` and stays
`low`, because padding cannot separate C objects (finding 505).
`python3 tools/translation_units.py build` reports 0 conflicts.

## The unit

### State

- **Common**: `Viewport_Globs viewportGlobs` (`0076ace0`, 0x8c bytes:
  `listSet[32]`, `freeList`, `listCount`, `flags`). It is a tentative
  definition, and so communal, as the original's is. It is annotated.
- **`.rdata`**: the four floats are literals, which MSVC pools in this
  object. `0049f778` is an annotated `static const` IID in the unit, as
  Lws.c does with `0049fb48` (finding 561).
- **String**: `004aaba4` has a `// STRING` marker.
- **Main.c**: `lpD3DRM()` is `s_main_d3drm` (`00505914`, `main.h`). The new
  stand-in `s_main_device` (`00505918`), annotated in `viewports_port.cpp`,
  is `lpDevice()`. Both stay NULL: the port's renderer keeps its own
  objects (`surface.cpp`).

`make unit-data-offsets`, G98-003 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| G98-003 | n/a (1, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | `0076ace0` `_viewportGlobs` (140) |

The port's old annotations of the pool are retired, and their storage stays
for the port:

- `view.cpp`: `s_render_surface_pool_blocks` (`0076ace0`),
  `s_render_surface_pool_level_count` (`0076ad64`) and
  `s_render_surface_pool_free_head` (`0076ad60`);
- `scene_resources.cpp`: `lego::s_slots_live` (`0076ad68`).

### Callees

- `model_render_pass_dispatch` (Mesh_PostRenderAll, `mesh_unit.cpp`)
  gains `extern "C"` in place in `surface.hpp`. `viewports_imports.h`
  declares it.
- `directx_result_diagnostic_dispatch` comes from `dxerror.h` (G98-027,
  converted).
- Mem_Alloc and Mem_Free are `malloc` and `free` through
  `crt/vc5names.h`.
- `test_c_boundaries.cpp` includes `viewports.h` and `viewports_imports.h`
  beside `surface.hpp` and `main.h`.

### Link names

The port's viewport record (`ScreenBoundedSurfaceRecord`, `surface.hpp`)
has the original Viewport's layout on 32-bit hosts. In the matching build
every caller of fifteen port names is an original-shape C unit. Each passes
a viewport that it got through those names, with the original's arguments:

- `lego_unit.c` creates and uses its own views;
- `advisor.c`, `sfx.c`, `terrain_map.c` and `legoobject_unit.c` read those
  views.

So, as in findings 589 and 590, `viewports_link_names.h` gives the unit's
definitions the port's names under `_MSC_VER < 1200`. The callers reach the
unit without an edit, including `legoobject_unit.c` (#626, now unowned). The
port tail compiles its own definitions of those names out of the matching
build (`VIEWPORTS_UNIT_DEFINES_PORT_NAMES`). Two of the names were
original-ABI adapters, and the unit replaces both:

- `render_target_viewport_create_original` (Viewport_CreatePixel);
- `scene_current_object_set_original` (Viewport_SmoothSetField).

Seven functions keep the unit's names:

| Function | Why | Matching-build caller switch |
| --- | --- | --- |
| Viewport_Shutdown | `engine.cpp`'s port shutdown calls `scene_resource_slots_release_all` to release the port's pool | `lego_unit_imports.h`: `scene_resource_slots_release_all` → `viewport_shutdown` |
| Viewport_GetSize | `lego_port.cpp` defines the adapter `lego_viewport_size_get_adapter`. Compiling it out there would move that file's symbol numbering | `lego_unit_imports.h`: `lego_viewport_size_get_adapter` → `viewport_get_size` |
| Viewport_GetField | the same, for `lego_viewport_field_get_adapter`, which returned 0 | `lego_unit_imports.h`: `lego_viewport_field_get_adapter` → `viewport_get_field` |
| Viewport_WorldToScreen | `entity.cpp` calls `camera_project_point_to_screen` with the port's `ViewProjectionAdapter` | `lego_unit_imports.h` and `tutorial_script_imports.h`: `camera_project_point_to_screen` → `viewport_world_to_screen` |
| Viewport_GetScene, Viewport_AddList, Viewport_RemoveAll | called only from the unit; the port's `com_property_result_get`, `ddiscene_render_surface_pool_grow` and `lego::scene_resource_tracked_references_release_all` serve `view.cpp` and `scene_resources.cpp` | none |

Each switch is a `#define` under `_MSC_VER < 1200`. A `#define` adds no
declaration, so the callers' symbol numbering does not move (finding 538).

`legoobject_unit.c` (converted by #615 and #626, now unowned) calls only
Viewport_InverseTransform and Viewport_Transform. Both are link names, so
it reaches the unit with no switch and no edit. Building.c (#627,
`building_model.c`) and `lighteffects.cpp` (#625) do not call the unit.

### DirectDraw (G98-010) coupling

#629 converts G98-010 (`gods98/directdraw.cpp`, text `0047c170`–`0047d410`),
and this change does not edit that file. There is no direct call between
the two units, in either direction (`docs/coverage/call-graph/edges.csv`).
They share no data. The commons `0076ac80`–`0076acc4`, used by
`0047c170`–`0047e300` (G98-010 and the unit after it), sit just before
`viewportGlobs`, but Viewports.c never touches them and G98-010 never
touches `viewportGlobs`. The
coupling is indirect:

- **Original:** Viewport_Create and Viewport_CreatePixel read Main.c's
  globals `lpD3DRM()` and `lpDevice()` (`00505914`, `00505918`). They call
  no DirectDraw function. Which code fills those globals is outside this
  unit and was not checked here.
- **Port:** `directdraw.cpp:293` passes the port's renderer and device to
  `screen_bounded_surface_device_set` (`surface.cpp`). The port tail's
  allocator (`screen_bounded_surface_allocate`) reads them, natively. The
  unit reads the Main.c stand-ins `s_main_d3drm` and `s_main_device`
  instead, which stay NULL. If #629 or a Main.c conversion fills those
  globals, the matching build's viewports get a real device with
  no change here.

So neither PR has a call to switch when the other lands.

### Port tail

`viewports_port.cpp` keeps every port model, unannotated. Each
`// FUNCTION` becomes a `/* Port model of 0x… */` comment. Natively every
definition there is the one the port calls, over the port's pool
(`view.cpp`) and renderer (`surface.cpp`).

- **Compiled out of the matching build**: the fifteen link names, including
  the two adapters. Two of them (`00476fd0`, `00476ff0`) have naked-asm
  branches, now only port code. `resource_interface_call_slot88`, the naked
  port model of `00477270`, stays in every build, unannotated: nothing
  calls it.
- **Deleted**: the ledger stub
  `unported_ledger_stubs::ledger_stub_00476e50_ScreenBoundedSurfaceResourceAllocate`.
  Its only caller, `render_target_viewport_create_original`, now calls
  `screen_bounded_surface_allocate` directly.
- **Diagnostics allowlist**: the pre-existing MSVC warning C4172 in
  `com_property_result_get` (finding 379) now comes from
  `viewports_port.cpp`. `tools/reccmp_diagnostics.py` and its test name the
  new path.

### Runtime

- **Native**: unchanged. `viewports.c` compiles natively as C++ under the
  unit's names, which nothing native calls. The port tail serves every
  port name as before, so native rendering does not change. No `port` gap
  is filed: the unit adds no behaviour the native port lacks.
- **Matching build**: the viewport calls now reach the unit. They run only
  inside Lego.c's `lego_game_config_init` and later. In the matching build
  that function stops at `container_initialise_original`, a NULL-returning
  adapter (`lego_port.cpp`), before any viewport is created. This change
  does not alter that. Once the init runs further, the unit needs Main.c's
  real `lpD3DRM()` and `lpDevice()` in `s_main_d3drm` and `s_main_device`.

## Shape notes

- **`00477010` Viewport_Clear**: the original keeps the four floats in the
  frame and the two IDirect3DViewport pointers in the dead `vp` and `full`
  argument slots. Separate `float` locals go to the argument slots
  (62–67%). A four-float struct (`VpArea2F`, gods98's Area2F) stays in the
  frame (71%). Declaring the `D3DRECT` in an inner block lets it share the
  width conversion's `qword` temporary (96.30%), and so does initialising
  `colour` after the Viewport_GetScene call.
- **`00477150` Viewport_Render**: `0.0f != vp->smoothFOV` gives the
  original's constant-first compare. By rule A (finding 538), declaring
  `smooth` before `fov` makes the original's x87 choice: `fov` stays on the
  stack, and the smoothing factor is read back from its slot. The −1.0
  constant is real: the original computes `1.0f / (smooth - -1.0f)`.

## Near misses

| Address | Score | What is left |
| --- | ---: | --- |
| `00477010` | 96.30 | The original converts `GetX`/`GetY` through the dead `vp` argument slot. The rebuild uses frame offset 0. 36 scope placements of the rectangle, the area and the interface pointers were tried, plus explicit `long` locals, `(float)(long)` casts and three `scene`/`colour` scopes. `msvc_permuter.py` (90 s) found no candidate. |
| `00477150` | 93.65 | The original stores `smooth * fov + smoothFOV` to the `vp` slot and multiplies by it (`fstp; fmul [esp+8]`). The rebuild keeps it on the stack (`fmulp`), 6 bytes shorter. 24 forms with two variables and 192 with four (a separate sum and reciprocal, every declaration order, both product orders) give at most 93.65%. `msvc_permuter.py` (90 s) found no candidate. |

No `Deviation (scoring only)` and no naked asm are in the unit, and no
filler declarations were added.

## Entropy

`make reccmp-check` against the baseline at `12a0e950` passes:

- 21 functions improved;
- no function fell, and no exact match was lost;
- the link-order map is OK, the plain-immediate check is OK, and
  `winapi-link-check` is OK.

Listing digests changed only for the unit's own functions, so no
instruction count changed outside the unit. Header edits:

- in place: `extern "C"` (`surface.hpp`) and comments (`view.cpp`,
  `scene_resources.cpp`);
- `#define`s under `_MSC_VER < 1200`: `lego_unit_imports.h` (four) and
  `tutorial_script_imports.h` (one);
- new headers: `viewports.h`, `viewports_imports.h` and
  `viewports_link_names.h`. Only `viewports.c` and `test_c_boundaries.cpp`
  include them.

## `/OPT:REF` (finding 579)

| | before (`main`) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full`: removed / exact removed | 498 / 229 | **492 / 225** |
| `opt_ref.annotated.game_only`: removed / exact removed | | 475 / 214 |
| The unit's object: removed | `viewports.cpp.obj` | `viewports.c.obj`: 2 (2 exact) |

`make opt-ref-callers OPT_REF_ARGS="--object viewports.c.obj"` lists two
functions. Both are exact, and both have a removed root in `lego_unit.c`:

- `00476f10` Viewport_GetSize;
- `00477270` Viewport_GetField.

Their only caller is `00433e80` (`lego_terrain_projected_cell_state_refresh`),
which is itself removed. The other twenty are kept.

## Reccmp lint ratchet

1,290 → 1,288 entries (−2, no growth). `lego::resource_interface_call_noargs`
and `render_surface_pool_stride`, statics in the old unit file, are now in
a tail file.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK, as above.
- `make match-progress`: the three generated files.
  `tools/binary_progress_inputs.txt` is regenerated
  (`binary_progress_hash.py --write-manifest`).
- `make unit-data-offsets`: the G98-003 row above.
- `make tools-test`: lint (ledger shrinks), link order, the winapi list,
  Meson parity (`viewports.c` added to `gods98_c_sources`) and the tool
  tests are OK.
- `make winapi-link-check`: OK.
- `viewports.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only`.
- Native: `cmake -S . -B build -DTEST_WADS_DIR=<shipped WADs> && cmake
  --build build && ctest --test-dir build`: 39/39, real-data tests
  included.

## Reproducing

    git mv src/gods98/viewports.cpp src/gods98/viewports_port.cpp   # and its link-order entry to the tail
    python3 tools/convert_unit.py G98-003 --stem viewports
    make match-function SOURCE=src/gods98/viewports.c
    make reccmp-check
    make match-progress
    make unit-data-offsets
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object viewports.c.obj"
