# Finding 587 — The camera unit (GAME-022) rebuilt in original shape, compiled without optimisation

Issue #607, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-022 is the original's game camera (LegoCamera): create and free, the
camera settings from Lego.cfg, tilt, rotation and zoom with their ranges, the
top-down scroll, first-person and radar tracking, and the shake. Text
`00435b20`-`00436c20`, 27 functions. It was relocated verbatim into
`src/game/camera_control.cpp`. This change rebuilds it as the C unit
[`src/game/camera_control.c`](../../src/game/camera_control.c), with the
unit-private imports header
[`camera_control_imports.h`](../../src/game/camera_control_imports.h). The
port's camera moves, unannotated, to the port tail
[`camera_control_port.cpp`](../../src/game/camera_control_port.cpp).

Stacked on #610 (LegoObject.c part 2, `575a5443`), on `main` `31da2ab6`
(#608). "Before" is #610's head; "after" is this change on it, from the full
matching build (`make reccmp-check`). The unit's own scores are the same on
`main`.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, 27 functions, 4,352 original code bytes) | 18.64% | **100.00%** |
| Exact functions in the unit | 9 (all naked asm) | **27 of 27** (no asm) |
| Exact-function code bytes (`binary-progress.json`) | 139,499 (21.71%) | **143,569** (22.34%) |
| Accuracy, full / game-only | 76.07% / 73.68% | 76.63% / 74.30% |
| `opt_ref` annotated (full): removed / exact removed | 604 / 279 | **596 / 276** |
| `opt_ref` annotated (game only): removed / exact removed | 586 / 267 | 579 / 265 |
| Reccmp lint ledger | 1,337 | 1,333 |

Exact bytes grow by 4,070: the unit's 4,038 newly exact bytes and
`lego_render_record_set_from_input` (`00425d50`, 32 bytes), whose call to
`Camera_TrackObject` now maps.

| Address | Function (original) | Port name | Bytes | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `00435b20` | `camera_create` (Camera_Create) | `scene_object_chain_record_create` | 625 | 8.55 | **100.00** |
| `00435d91` | `camera_enable_free_movement` | `entity_b8_bit3_set_or_clear` | 55 | 100.00 | **100.00** |
| `00435dc8` | `camera_free` | `camera_view_context_close` | 70 | 9.09 | **100.00** |
| `00435e0e` | `camera_init_camera_movements` | `camera_view_context_set` | 39 | 38.10 | **100.00** |
| `00435e35` | `camera_copy_fp_position_orientation` | `model_transform_copy_to_scene_node` | 134 | 27.87 | **100.00** |
| `00435ebb` | `camera_track_object` | `screen_effect_record_set` | 57 | 15.79 | **100.00** |
| `00435ef4` | `camera_set_fp_object` | `record_fields_4_and_14_set` | 23 | 100.00 | **100.00** |
| `00435f0b` | `camera_get_fp_camera_frame` | `record_field14_get` | 11 | 100.00 | **100.00** |
| `00435f16` | `camera_stop_movement` | `cursor_screen_ray_context_reset` | 18 | 100.00 | **100.00** |
| `00435f28` | `camera_get_mouse_scroll_indent` | `view_grid_edge_get` | 10 | 100.00 | **100.00** |
| `00435f32` | `camera_shake` | `entity_fields_a0_a4_store_a8_clear` | 42 | 100.00 | **100.00** |
| `00435f5c` | `camera_update` | `view_tick` | 2261 | 6.42 | **100.00** |
| `00436831` | `camera_set_tilt_range` | `camera_tilt_bounds_set` | 47 | 26.09 | **100.00** |
| `00436860` | `camera_set_tilt` | `camera_tilt_set` | 160 | 35.90 | **100.00** |
| `00436900` | `camera_add_tilt` | `camera_pan_axis_update` | 33 | 10.53 | **100.00** |
| `00436921` | `camera_set_rotation_range` | `camera_scalar_bounds_set` | 50 | 26.09 | **100.00** |
| `00436953` | `camera_set_rotation` | `camera_rotation_set` | 161 | 36.84 | **100.00** |
| `004369f4` | `camera_add_rotation` | `camera_tilt_axis_update` | 36 | 10.53 | **100.00** |
| `00436a18` | `camera_set_zoom_range` | `camera_distance_bounds_set` | 50 | 100.00 | **100.00** |
| `00436a4a` | `camera_set_zoom` | `camera_distance_set` | 91 | 100.00 | **100.00** |
| `00436aa5` | `camera_add_zoom` | `camera_zoom_update` | 36 | 29.63 | **100.00** |
| `00436ac9` | `camera_add_translation_2d` | `renderer_node_relative_position_set` | 32 | 20.69 | **100.00** |
| `00436ae9` | `camera_get_topdown_position` | `selection_record_point_get` | 34 | 27.59 | **100.00** |
| `00436b0b` | `camera_set_topdown_position` | `selection_record_point_set` | 40 | 25.81 | **100.00** |
| `00436b33` | `camera_get_topdown_world_pos` | `camera_get_position_with_terrain_height` | 70 | 17.24 | **100.00** |
| `00436b79` | `camera_get_rotation` | `camera_rotation_get` | 14 | 100.00 | **100.00** |
| `00436b87` | `camera_move` | `camera_pointer_pan_update` | 153 | 21.33 | **100.00** |

The nine "before" exact matches were `__declspec(naked)` copies of the
original's bytes. They are gone: every function is C.

## The unit was compiled without optimisation

Finding 505 (rule 5) noted that this unit's functions are packed with `int3`
fill, "compiled with different options". The options are no optimisation:
every function keeps its frame pointer (`push ebp; mov ebp, esp`), reloads
each operand from memory before every use, never keeps a value in a
register across statements, and calls Maths.h's inline vector functions
out of line (the COMDAT copies the linker kept in Advisor.c, `004013a0`-
`00401750`). This is why the port needed naked asm: no source shape gives
that code under `/O2`.

- The matching build compiles the unit with `/Od`. `CMakeLists.txt` lists it
  in `LRR_UNOPTIMISED_UNITS`, whose objects take `/Od` after the
  configuration's `/O2` (MSVC 5.0 applies the last; plain `/Od` gives
  identical code). The resulting command-line warning `D4025` is
  allowed, by its exact text, in `tools/reccmp_diagnostics.py`, with a test.
- `/Od` also drops `/Gy`, so one `.text` section holds every function.
  `tools/msvc_coff.py` read one `.bf` base line per code section, which
  holds only with `/Gy`; every function after the first got wrong line
  numbers and `make match-function` reported none of them. It now takes
  each function's own `.bf` record.
- With `/Od`, every function except `camera_update` matched from a direct
  reading of the disassembly, with no tuning (`cameraGlobs` needed its
  annotation on a one-line definition so Reccmp names it).

### `/Od` frame layout follows the locals' names

`camera_update` (2,261 bytes) first scored 85.93%, all of it stack slots:
which local is at which frame offset. Swapping declarations changed
nothing; renaming did. Probes compiled with `/Od` show the rule:

- each block's locals are laid out lowest address first in descending
  order of a 16-way hash bucket of the name;
- locals in the same bucket keep their declaration order, earlier lower;
- a nested block's locals follow its enclosing block's, and sibling blocks
  do not share slots.

The bucket function is not identified (for one-character names it is
`ord(c) % 16`; for longer names higher bits matter, and no shift-and-add
family fitted). [`tools/experiments/607/frame_buckets.py`](../../tools/experiments/607/frame_buckets.py)
measures it instead: a probe declares a ladder of 16 one-letter locals, one
per bucket, then the candidates, and each candidate lands right after its
bucket's letter. The model reproduced every slot of the first compile.

To match, each block's names must have non-increasing buckets along the
original's slot order. A small search over synonyms found such names for all
three blocks (the radar block has 14 locals):

| Block | Slot order (lowest first) and chosen names (bucket) |
| --- | --- |
| top-down, inner | `camDist` (14), `height` (13) |
| first-person | `destPos`, `destDir` (15), `dot` (8), `oldPos`, `oldDir`, `dist`, `t` (4) |
| radar | `totalDist`, `startZoom` (13), `camPos2` (10), `move`, `movement`, `zdiff` (8), `rootPos`, `objDirection` (4), `camPos`, `rootDist` (3), `factor`, `distance`, `maxMove` (2), `objPos` (1) |

The radar block's declarations carry a comment pointing here. This applies to
any other unit compiled with `/Od`; with `/O2` the slots come from use
weights instead (finding 581).

### One scoring-only deviation

In the top-down branch the original passes `Map3D_GetWorldZ`'s result
straight to `Container_SetPosition` as `call; add esp, 0xc; push ecx; fstp`.
Our compile folds the cleanup and the push into `add esp, 8`, with any flags
(`/O2 /Od`, plain `/Od`) and as C or C++. A cast, a `double` return, an
unprototyped call, `+ 0.0f`, `* 1.0f`, `-(-x)` and a conditional all give
other code; only a comma expression, `(0, Map3D_GetWorldZ(...))`, reproduces
the original's. It is tagged `Deviation (scoring only)`, and changes no
storage.

## How the conversion is done

### The port tail

The port's camera is another contract: the port's `ViewContext` (its own
layout and three `CameraHandle`s), the engine's 0xb9-byte camera controller
buffer, NULL guards and hook tables (`s_view_context_release_hook`,
`s_camera_pose_dispatch_hook`, `s_scene_object_chain_hooks`). Native code
(`frameloop.cpp`, `view.cpp`, `engine.cpp`, `lego_port.cpp`, the tutorial)
and the tests use it. As in findings 570 and 582, `camera_control.cpp` is
renamed `camera_control_port.cpp`, a `tail` entry, and each `// FUNCTION`
becomes a `/* Port model of 0x… */` comment. Its naked-asm branches stay as
they were (port code); native behaviour does not change.

`view.cpp`'s `g_view_pan_cap`, `g_view_follow_scale`, `g_view_pan_scale` and
`g_view_grid_edge` lose their annotations: they are the port's copies of
`cameraGlobs` (`005560a0`-`005560ac`), which the unit now defines.

### State

The unit has no `.bss` of its own. `cameraGlobs` (`005560a0`, 16 bytes:
speed, drop-off, acceleration, mouse scroll indent) is a common symbol in
the original, outside every unit's attributed range, and is a tentative
definition here (`make unit-data-offsets`: `005560a0:_cameraGlobs(size=16)`
under GAME-022's commons). `legoGlobs.cameraMain` (`005560e4`) stays Lego.c's.
The `.rdata` constants (`0049e378`-`0049e39c`) are literals, which MSVC pools
in this object.

`LegoCamera` (0xbc bytes) is declared in the unit at the original's offsets:
the type, tracked object and radar/first-person settings, the move vector,
five containers (`+0x24` camera, `+0x28` root, `+0x2c` sound listener,
`+0x30` tilt, `+0x34` ear), tilt, rotation and zoom with their ranges
(`+0x78`-`+0x98`), move speed, shake (`+0xa0`-`+0xb4`) and the flags byte
(`+0xb8`). `Lego_Level` and `Map3D` are prefix views.

### Callees

- **`extern "C"` in place** on four existing declarations:
  `ddiscene_object_allocate` (Container_Create, `scene_pool.hpp`, inside
  `namespace lego`), `scene_object_camera_teleport_or_offset` and
  `ddiscene_scene_object_attach_or_unlink` (`view.hpp`), and
  `entity_follow_target_pose_get` (`entity.hpp`). No declaration is added.
- `camera_control_imports.h` declares the rest with the port's types and is
  checked in `test_c_boundaries.cpp`. Container_Create is declared in the
  unit, with the original's parent argument: the port types it as the
  parent's scene-object descriptor, which is the same record
  (`lego_port.cpp`'s `lego_container_create_adapter` casts it the same way).
- Lego_GetMap (`00431570`) is called through an unprototyped type in C and
  `level_current_terrain_get_original` natively, as `weapons_imports.h` does.
- The unit calls the out-of-line vector functions in Advisor.c directly, as
  the original's `/Od` object did; there is no `*_maths.h`.

### Callers

- **`lego_unit.c`** (no owner): 23 `#define`s at the top of
  `lego_unit_imports.h`, under `_MSC_VER < 1200`, give the port names it
  calls the unit's names in the matching build: create, free, settings,
  free movement, track, first-person object and frame, stop, scroll indent,
  shake, update (`view_tick_original`), the tilt, rotation and zoom setters
  and adders, the top-down position functions and `camera_move`. A
  `#define` adds no declaration, so its symbol numbering does not move
  (finding 582 found that a new `#include` does). Seven Lego.c functions
  improve, `00425d50` to exact.
- **`panel_unit.c`** (no owner): `camera_control.h` keeps its three
  declarations and `s_zoom_step_context` unchanged and adds three matching-
  build `#define`s (`camera_add_tilt`, `camera_add_rotation`,
  `camera_add_zoom`); `camera_control.c` defines
  `LEGORRR_GAME_CAMERA_CONTROL_UNIT` to keep its own names.
- **Callers in files other threads own:** none edited. `tutorial_script.cpp`
  (#606) calls the port's `camera_zoom_update` and `camera_rotation_set` on
  the engine's controller buffer; `legoobject*.c/.cpp` (#588) do not call
  the unit.
- C++ port callers (`engine.cpp`, `view.cpp`, `lego_port.cpp`,
  `tutorial_hud.cpp`, `record_chains.cpp`) keep the port's names: they pass
  a `ViewContext` or the controller buffer.

## Entropy and drops

`make reccmp-check` against #610's baseline (`575a5443`): 25 improved,
one below baseline, **no exact match lost**, no instruction count changed.

| Address | Function | Before | After | Cause |
| --- | --- | ---: | ---: | --- |
| `0046a420` | `selected_global_point_transform_and_return_x` (`record_chains.cpp`) | 14.04 | 10.53 | operand only: the port model still calls the port's `camera_get_position_with_terrain_height`, now unannotated; the original calls Camera_GetTopdownWorldPos (`00436b33`). It passes a `ViewContext` and the engine context, so it is not switched (finding 579 class (c)). |

The classifier labels it "drift"; the listing shows only the moved call
target. The baseline is refreshed for it; it is not owner-reviewed. No
`0x800000` immediate is involved (#608's check passes).

## `opt_ref` (finding 579, `make opt-ref-callers`)

`/OPT:REF` keeps **all 27** unit functions: `lego_unit.c`'s config init,
main loop, interaction update and level free (live since #587) now call
them. Before, those calls went to the port's adapters, and finding 579
counted 15 of `camera_control.cpp`'s annotated functions (6 exact) removed.
Overall the annotated removals fall from 604 (279 exact) to 596 (276).
`opt-ref-callers --object camera_control.c.obj` lists nothing.

## Native behaviour

Unchanged. Native callers reach the port tail under the port's names; the
unit compiles natively as C++ and is not called there. No `port` gap is
filed: the native camera already runs on the port's model, and the unit
adds no behaviour it lacks.

## Validation

On #610's head (`575a5443`):

- `make reccmp-check`: OK after the baseline refresh, with the one labelled
  drop above; plain-immediate check OK.
- `make match-progress` (baseline, listing digests, `binary-progress.json`)
  and `make unit-data-offsets`.
- `python3 tools/translation_units.py build`; `make reccmp-lint-update`
  (ledger 1,337 to 1,333: the tail's four static helpers);
  `tools/binary_progress_hash.py --write-manifest`.
- `make tools-test`, `make winapi-link-check`, native build and `ctest`
  (37/37, synthetic fixtures).
- `gcc -std=gnu89 -Werror=declaration-after-statement -fsyntax-only` on
  `camera_control.c`.

## Reproducing

    python3 tools/convert_unit.py GAME-022 --stem camera_control   # after moving camera_control.cpp to the tail
    make match-function SOURCE=src/game/camera_control.c
    python3 tools/experiments/607/frame_buckets.py camDist height destPos oldPos
    make reccmp-check
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object camera_control.c.obj"
    make match-progress
    make unit-data-offsets
