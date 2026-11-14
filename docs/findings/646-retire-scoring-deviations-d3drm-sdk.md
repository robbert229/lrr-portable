# Finding 646 — Retiring declaration-only scoring deviations; why `d3drm.h` still cannot go into Bezier, Maths and Draw

Issues #453 and #481. Builds on findings [506](506-original-shape-pilot-priorities.md),
[524](524-d3drm-header-constants-in-rdata.md), [529](529-unit-relative-data-offsets.md),
[532](532-config-direct3drm-prefix.md) and [538](538-msvc5-symbol-counter-codegen-drift.md).

## Summary

- **#453 (done).** Every declaration-only `Deviation (scoring only)` in the
  shared C++ headers is gone: `view.hpp`, `record.hpp`, `entity.hpp`,
  `frontend.hpp`, `port_loop_ports.hpp`, `ui_list.hpp`, `util.hpp`,
  `resource.hpp`, `simtick.hpp`, `surface.hpp` and `view_shared.hpp`, plus
  the uncalled superseded definition in `resource.cpp`. No function changed
  score. The tag count in `src/` went from **54 to 17**. The 17 that remain
  are listed below with their causes.
- **#481 (not done).** Adding `d3drm.h` to GAME-003, G98-007 or G98-019
  still loses functions, so no include is committed. The toolchain's
  `d3drm.h` is DirectX 3's, while the original was built against the
  DirectX 6 headers (it calls `IDirect3DRM3` and `IDirect3DRMFrame3`;
  findings 561, 596, 604). The include moves MSVC 5.0's symbol counter
  (finding 538, rule B) by a different amount than the original's did.
  Restoring each unit's code would need extra declarations that only pads
  could supply. The measurements are below.

## #453: what was removed

Only the units that include a header can be affected, and only their
annotated functions are scored. A regex walk of `#include "…"` from every
`src/link_order.cmake` entry built for Windows found the annotated
includers of each header. The walk ignores `#if`, so it over-approximates.

| Header | Annotated includers |
| --- | --- |
| `view.hpp`, `record.hpp`, `resource.hpp` | `entity_cell_effects.cpp`, `level.cpp`, `level_id_remap.cpp`, `object_state_file.cpp`, `file_list_streams.cpp`, `sound3d.c` (`record.hpp`/`resource.hpp`/`view.hpp`) |
| `entity.hpp`, `frontend.hpp` | `entity_cell_effects.cpp`, `level.cpp`, `object_state_file.cpp`, `file_list_streams.cpp` |
| `port_loop_ports.hpp`, `util.hpp` | the same plus `level_id_remap.cpp` |
| `simtick.hpp` | `entity_cell_effects.cpp`, `level.cpp`, `file_list_streams.cpp`, `sound3d.c` |
| `surface.hpp` | `entity_cell_effects.cpp`, `level.cpp` |
| `ui_list.hpp`, `view_shared.hpp` | none |

Most units have become original-shape C files with their own headers
since these declarations were pinned. Few annotated functions still see
the shared headers.

Each removal was scored with `tools/match_function.py --source` over those
units and the other units the PR touches. That is 598 functions, compared
with an unchanged-tree run of the same tool.

| Removed | Header | Superseded by |
| --- | --- | --- |
| `viewport_rectangle_set`, `ViewportDimensionsQueryFn`, `viewport_dimensions_query_hook_set` | `view.hpp` | `draw.c` (G98-019) |
| `animation_object_transform_set`, `renderer_record_clone`, `animation_object_time_update`/`_time_get`/`_value_get`, `renderer_object_subobject_find_apply`, `renderer_object_subobject_dispatch`, `renderer_cached_handle_50_get_or_create`, `renderer_cached_handle_4c_is_not_one`, `activity_runner_state_read` | `view.hpp` | `animation_object.c` (GAME-004) |
| `SceneObjectListApplyHook`, `scene_object_list_apply_hook_set`, the four `scene_object_list_*` callbacks, `matrix_rotation_compose_xyz`, `vector_scale_components_in_place`, `set_global_scalar_to_one`, `scene_node_position_set`, `GlobalVectorNotifyFn` and the three `global_vector_*` functions | `view.hpp` | `steering.c` (GAME-016) |
| the hook's two `extern`s | `view_shared.hpp` and 12 `view_for_*.hpp` | `steering.c` (the `view_for_*` copies were untagged) |
| `vec2_dist_alt`, `vector2_set_length`, the four Bezier declarations | `view.hpp` | `bezier.c` (GAME-003) |
| `scene_act_frame_record_clone_or_create`, `com_child_items_visit`, `object_interface_array_resource_create`, `object_hierarchy_recursive_clone` | `view.hpp` | `animclone.c` (G98-025) |
| `map_row_span_*` (four) | `view.hpp` | `water.c` |
| `scene_object_mode_is_one` (C linkage, in place) | `record.hpp` | `bezier.c` |
| `renderer_object_update_from_record_field64`/`68`/`6c` | `record.hpp` | `animation_object.c` |
| `three_value_record_copy_and_mark_ready`, `object_vector3_set` | `record.hpp` | `steering.c` |
| `set_vector_and_notify_listeners`, `direction_driven_heading_angle_step`, `kind_two_steering_torque_apply`, `steering_physics_step` | `entity.hpp` | `steering.c` |
| `activity_runner_update_superseded` | `entity.hpp` | `animation_object.c` (the untagged definition in `entity.cpp` stays) |
| `PositionTrackerBoxDrawOps::outline_draw` | `frontend.hpp` | `draw.c`'s `surface_draw_polyline` |
| `buffered_char_stream_read_to_string` | `port_loop_ports.hpp` | `wad.c` |
| `ui_scroll_info_panel_initialize_superseded`, `ui_overlay_create_configured_element_superseded` | `ui_list.hpp` | `panel_unit.c` (GAME-032) |
| `CallbackOperationNode`, `CallbackOperationVisit`, `callback_operation_run_superseded` | `util.hpp` | `steering.c` |
| `ResourceTreeCallbackContext` and its three functions | `resource.hpp` | `animclone.c` |
| `wad_huffman_table_clear` (C++ copy) | `resource.hpp` | `rnc.h` |
| `owned_object_reference_chain_release_superseded` (declaration and definition) | `resource.hpp`, `resource.cpp` | `terrain_map.c` (GAME-026) |
| `simtick_manager_tick_superseded` | `simtick.hpp` | `panel_unit.c` |
| `surface_render_flags_peek`, `surface_pixel_writer_peek`, `SurfacePixelWriteFn`, `SurfacePixelWriterTable`, `SurfaceRasterContext`, `SurfacePixelSpan`, `SurfacePoint` | `surface.hpp` | `draw.c` |

### The one phase hazard: `osf_read`

`0x00459470` `osf_read` (`object_state_file.cpp`, 20.28%) is the case that
`view.hpp`'s `map_row_span_*` comment cited. Its code depends on the
counter's phase (rule B). With every removal except `util.hpp`'s, it fell
to 9.71%. Restoring any one of `entity.hpp`, `util.hpp`, `record.hpp` or
`resource.hpp` brought it back. Removing the `SceneObjectListApplyHook`
typedef as well flipped it again. With all removals together, `util.hpp`'s
included, it is back at 20.28%. The accepted set is therefore all of them.
It holds because of the total counter shift, not because of any single
removal. A later edit to these headers can flip `osf_read` either way.

## #453: what remains (17)

| Where | Kind | Why it stays |
| --- | --- | --- |
| `crt/crt.h` (1), `crt/crt.hpp` (2) | declaration views, superseded declarations, display-mode types | `src/crt/` belongs to #447, which runs in parallel. Not attempted. |
| `game/aitask_imports.h` (1) | `directDrawGlobs` as a four-word view | Including `gods98/directdraw.h` instead moves `0x00404fc0` from 98.19% to 97.96% (aitask.c's other 108 functions unchanged). |
| `game/lego_unit_imports.h` (2), `game/lego_unit.c` (1) | `rewardGlobs` as words; `movie_load`'s unnamed parameter; group 3C entries | Lego.c belongs to #740. Not attempted. |
| `game/legoobject_unit_imports.h` (1), `game/legoobject_unit.c` (2) | unnamed output parameters; a volatile reload; a folded getter | LegoObject.c belongs to #741. Not attempted. |
| `game/camera_control.c`, `game/helpwindow.c`, `game/lighteffects.c` (1 each) | code shape | Not declaration-only (outside #453). |
| `gods98/avi_player.cpp` (3), `gods98/movie_unit.cpp` (1) | full vtable declaration; class-scope `operator new`/`delete` | Not declaration-only (outside #453). |

## #481: `d3drm.h` in GAME-003, G98-007 and G98-019

Config's pattern (`#if defined(_MSC_VER) && _MSC_VER == 1100`, then
`<windows.h>` and `<d3drm.h>`) was tried in each unit. Maths.c's own
`D3DRMVectorRandom`/`Rotate` declarations conflict with the SDK's, so its
variant used the SDK's declarations, with `LPD3DVECTOR` casts at the two
calls. Scores from `make match-function`:

| Unit | Function | Before → with `d3drm.h` |
| --- | --- | --- |
| G98-019 | `0x004869a0` Draw_Pixel24 | 100.00 → 71.43 |
| G98-019 | `0x00485ea0` viewport_subrectangle_set | 100.00 → 97.40 |
| G98-007 | `0x00479ab0` point_along_ray | 100.00 → 91.30 |
| G98-007 | `0x00479af0` line_intersection_2d_compute | 86.67 → 81.67 |
| G98-007 | `0x00479630` surface_triangle_area_calculate | 42.30 → 63.47 (rises) |
| GAME-003 | `0x004065f0` cubic_bezier_point_evaluate | 56.88 → 53.21 |

Placing the include before the unit's own headers changes nothing. These
are the figures finding 532 reported.

### The counter shift each unit would need

For each unit, N inert `extern int` declarations were added after the
include, for N = 0–519. Each function's `/FAs` listing was then compared
with the current tree's, using finding 538's normalised fingerprints
(`tools/experiments/486`).

| Unit | Functions that vary | N where every function keeps its current code |
| --- | --- | --- |
| G98-019 Draw (18) | Draw_Pixel24 (period 512), viewport_subrectangle_set (16), Draw_Pixel16HalfTrans (64) | 32 values, the smallest 165: 165–171, 181–187, 229–235, 245–251, … (odd only) |
| GAME-003 Bezier (7) | cubic_bezier_point_evaluate | 157 values; the smallest is **2** (2–5, 10–15, 18–24, …) |
| G98-007 Maths (20) | surface_triangle_area_calculate, point_along_ray, line_intersection_2d_compute, ray_plane_intersection_parameter, sphere_ray_intersection_discriminant_positive | **none**: surface_triangle_area_calculate's current code does not recur for any N in 0–519 |

Draw and Bezier would need declarations that the units do not have.
Choosing them for their count would be a pad (ADR 0005 rule 3;
finding 538's "a pad by another name"). Maths has no such phase at all.

### Why the include is the wrong size

The original calls `IDirect3DRM3` and `IDirect3DRMFrame3` methods (findings
561 and 604). Those interfaces first appear in the DirectX 6 SDK. MSVC 5.0's
`d3drm.h`, `d3drmobj.h` and `ddraw.h` are DirectX 3's (findings 596 and
625) and declare neither. DirectX 6's `d3drm.h` chain declares many more
types, interfaces and GUIDs. So the original's include moved each unit's
counter by an amount the toolchain's header cannot reproduce. The
`{1, 2, 4, -1}` constants are the same in both (finding 524). The
counter shift is not. No DirectX 6 header is available to the matching
build, so this was not tested.

### GAME-003's extra four bytes

The original Bezier block is `0049e034` {1, 2, 4, -1}, then `0049e044` an
unreferenced IEEE-754 `1.0f`, then the three annotated floats from
`0049e048`. That accounts for `0x14` rather than `0x10`. A second
`1.0` is already at `0049e050` (`s_bezier_one`), so the word at `0049e044`
is a separate named constant, not a pooled literal. Its declaration is not
known (finding 514). As a `static const float` before the first function,
it would cost one counter unit. N = 1 is not in Bezier's safe set, so even
the d3drm prefix plus this word would cost `cubic_bezier_point_evaluate`.
Neither is reproduced.

### `.rdata` offsets (`make unit-data-offsets`)

| Unit | `main` `c46f8e97` | This change |
| --- | --- | --- |
| GAME-003 | partial, 3 anchors, first `0049e048`: `14/0` | same |
| G98-006 | full, 4 anchors | same |
| G98-007 | partial, 7 anchors, first `0049e920`: `10/0` | same |
| G98-019 | partial, 4 anchors, first `0049ea20`: `10/0` | same |

## Measurements

Measured with `make reccmp-check`, then `make winapi-link-check
reccmp-baseline-update binary-progress-compare` (`make match-progress`
without a second rebuild), `make unit-data-offsets`, `make opt-ref-callers
OPT_REF_ARGS=--summary` and `make pe-audit`. `main`'s `opt_ref` list was
measured with the same tools on a fresh build of `c46f8e97`.

| Measure | `main` `c46f8e97` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 303,156 (47.18%) | 303,156 (47.18%) |
| Reccmp mean, full / game-only | 96.48 / 96.20 | 96.48 / 96.20 |
| Functions whose score changed | | **0 of 2,766** |
| `opt_ref` removed / removed exact | 27 / 23 | **26 / 22** |
| Image end (gate) | `0x875000` | `0x875000` |
| `Deviation (scoring only)` tags in `src/` | 54 | **17** |

`make reccmp-check` passes. All 2,766 entries in Reccmp's `results.json`
are identical to the `main` build's. The match baseline and listing
digests changed only in their input hash. `binary-progress.json` changed
in its hashes, in the port tail's addresses (`0x10` lower, from the deleted
`resource.cpp` function) and in `opt_ref`.

The `opt_ref` change is `/OPT:ICF`'s choice among identical functions.
The removed declarations also shift the symbol counter in the port files
that include these headers, and their code shifts with it. Code with no
annotation has no score, but it can start or stop being byte-identical to an
original:

| Original | `main` | This change |
| --- | --- | --- |
| `00429160` lego_terrain_cell_palette_resource_at_grid_position (64 bytes, exact) | folded into `lego_port.cpp`'s copy | **live** |
| `00438ed0` legoobject_flags_contain_requested_single_bit (144 bytes, exact) | folded into `legoobject_port.cpp`'s copy | **live** |
| `004798a0` rng_thunk_draw15 (16 bytes, exact) | live | folded into `level.cpp`'s copy |

Native: `cmake --build build` succeeds and `ctest` passes 37 of 37.
`make tools-test` passes (no `--allow-growth`), and `make
winapi-link-check` passes. `make pe-audit` matches `main`, including its
existing warning that the image covers `0x800000` (finding 584).

## Reproducing

- Includers and scoring: see the method above. The phase case is
  `python3 tools/match_function.py 0x459470`.
- Counter shifts: compile the unit with the include and N `extern`s for
  N = 0–519, as `tools/experiments/486/sweep.py` does, and compare
  fingerprints with the unchanged unit's.
