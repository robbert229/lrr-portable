# Finding 555 — Native creature scenes through GAME-004, with the native dispatch outside the annotated bodies

This reworks #527 ("Native creature scenes: animate and draw Raiders through
GAME-004", commit `d597a911`) onto `main` at `1896fc32`, after #528, #526,
#529 and #530. The behaviour is #527's: on the native (miniwin) backend,
Rock Raiders and rock monsters load through Creature_Load (0x406990), get
a Creature_Clone (0x406cb0) each, animate through the GAME-004 unit
(`game/animation_object.c`) and draw as their textured meshes. What changes
is where the code sits.

## Summary

- **Edits follow the relocated code.** #527's call-site edits to
  `ai.cpp` and `game/entity.cpp` are now in `game/legoobject.cpp`
  (GAME-023, #526). Its loose-file fix to `resource.cpp` is now in
  `gods98/files.cpp` (G98-014, #528). No frozen per-relocation header is
  edited.
- **No backend code inside the annotated Container functions.** #527 put
  MINIWIN prologues inside 15 annotated or ledger functions in `view.cpp`,
  `texture.cpp`, `resource.cpp` and `record.cpp`, and inside
  `container_load`. Here 14 of them are reached through a port-only
  forwarding layer, and `ddiscene_object_position_get` becomes a whole
  alternative definition. No Container function body changes.
- **Port-only code is in the port tail.** `gods98/container_native`,
  `gods98/lwsc_miniwin`, `gods98/container_route` and
  `game/creature_scene` are native-only tail entries in
  `link_order.cmake`. They are also in the Meson source lists and
  `tools/binary_progress_inputs.txt`.
- **The lint ratchet shrinks** from 4,110 to 4,061 entries. #527 grew it
  by 67.
- **Matching build:** no score moved. `make match-progress` changed only
  the input hashes.

## Design decisions

### 1. The seam: a forwarding layer that GAME-004's C boundary header routes to

GAME-004 calls the Container functions through its C boundary headers
(`gods98/container.h`, `view.h`, `record.h`, `texture.h`, `entity.h`).
Of all native code, only GAME-004 creates native Containers
(`container_load`), clones them and searches them for null frames. So only
GAME-004's calls need to reach the native ones.

- `gods98/container_route.{h,cpp}` (port tail, native only) defines one
  forwarder per call, such as `container_route_activity_set` for
  `scene_action_name_set`. Each forwarder answers for a native Container
  under #527's conditions (`container_native_owns`; `alternate_frame ==
  NULL` for the pose calls; `mode == 0` for the tree search;
  `container_native_object_known` for the frame scale). Otherwise it calls
  the gods98 function unchanged.
- `gods98/container.h` ends with a `#if defined(LEGORRR_BACKEND_MINIWIN)`
  block. The block includes `container_route.h` and `#define`s the 15
  names GAME-004 calls to their forwarders. `animation_object.c`
  includes `container.h` before `entity.h`, `record.h`, `texture.h` and
  `view.h`, so their later declarations name the forwarders, with
  identical signatures.
- Only `animation_object.c` and `test_c_boundaries.cpp` include
  `container.h`. The test includes it after every C++ header and
  references none of the routed names. `nm -u` on the native
  `animation_object.c.o` lists exactly the 15 `container_route_*` symbols.
- `container_route.cpp` includes `container.h` last and `#undef`s the 15
  names, so its forwarders call the real functions.

**Why this seam.** The brief offered three options.

- A **whole-function `#if` group per function** would duplicate each
  body, because native builds still need the original body for
  non-native objects. That is about 550 lines over 15 functions, among
  them `xf_texture_name_format` (101 lines) and
  `ddiscene_frame_update_and_callbacks` (80).
- A **native-only definition in a separate translation unit** cannot
  replace a function whose original body native builds still call. Doing
  that would need a per-file symbol rename or `ld --wrap`. `--wrap` is
  GNU-only (the native build supports macOS), and it misses calls within
  one object.
- The **forwarding layer** keeps every original body as the single
  definition, adds no code to any annotated file and puts the dispatch
  in one native-only file.

`container_load` (`gods98/entity.cpp`) is no longer edited; its forwarder
loads `"ACT"` objects natively.

### 2. `ddiscene_object_position_get`: one whole alternative definition

C++ code also reaches this function with a native null frame:
`entity_work_object_position_get` (`legoobject.cpp`) asks for the
position of GAME-004's drill null. Routing GAME-004 alone would miss that
call. Without #527's check, the original body dereferences a miniwin
frame as a pose interface.

The function becomes a group of whole alternative definitions under its
one annotation:

- `#if !defined(LEGORRR_BACKEND_MINIWIN)`: the original, verbatim;
- `#else`: #527's prologue, then the same body.

The relocation tool moves such a group whole, and the lint accepts it.

**The matching branch must come first.** In the opposite order (MINIWIN
first), the first gate run reported `0x004754c0` *newly unmapped*.
Reccmp maps an annotation to the definition on the line below it, and the
PDB has no function on that line. Losing the callee's name then dropped 11
of its callers by 0.5–5 points ("drift", same instruction count), in
`legoobject.cpp` and `entity.cpp`. `match_function.py 0x004754c0` gave the
cause: "the annotated function was not found in the compiled object".
With the matching branch first, as `game_file_system_type_get` (Files.c)
already does, nothing moves. `reshape_check` cannot see this, for the
reason finding 552 gives for `GLOBAL`s: annotations are comments.

`view.cpp` includes `container_native.hpp` in a MINIWIN-only include
group, a kind the relocation tool copies (#528).

### 3. Call sites: one `#elif` arm beside the existing WinAPI arm

The call sites sit in annotated functions that already have a
`LEGORRR_BACKEND_WINAPI` group calling `actor_scene` (EntityTeardown,
EntityWorldPositionSet, EntityActivityTick, FUN_00447c40) or
`building_scene`/`actor_scene` binding (`entity_create_with_position_subtype`).
These arms are compiled into the matching build, so they cannot be
replaced without changing it.

The native behaviour is therefore an `#elif defined(LEGORRR_BACKEND_MINIWIN)`
arm of the same group. Each arm is one call into `game/creature_scene`
(`creature_scene_entity_release`, `creature_scene_pose_apply`,
`creature_scene_advance`, `creature_scene_activity_set`). No new
conditional region enters an annotated body. #527 differed in two ways:

- It nested a new `#if` inside FUN_00447c40's `#else` branch, with an
  inline fallback. The fallback (the port's activity runner, for an entity
  without a creature scene) is now inside `creature_scene_activity_set`.
- It defined `entity_creature_pose_apply` and its terrain-height callback
  as statics in `entity.cpp`. EntityWorldPositionSet has since moved to
  the GAME-023 unit file, which may hold only original functions (ADR 0006
  decision 4). Both are now in `creature_scene`.

`entity_config.cpp` and `frameloop.cpp` are unannotated port files; their
arms are #527's.

**The loose-file fix** (`game_file_open_dispatch`, `files.cpp`) is an OS
difference, not a backend one: glibc opens a directory for reading and
Windows refuses. It stays an `#if !defined(_WIN32)` block in the body,
like the unit's other `_WIN32` splits (for example the CD drive probe).
#527's static helper `game_file_stream_is_regular` is gone, so no
port-only function enters the unit file. The check is inline and uses the
unit's existing `<sys/stat.h>` include.

### 4. The lint: native-only branches need no annotation

`tools/lint_reccmp_annotations.py` already skipped `src/platform/native/`.
It now also skips definitions in an `#ifdef LEGORRR_BACKEND_MINIWIN`
branch, or in an `#if`/`#elif` branch that has
`defined(LEGORRR_BACKEND_MINIWIN)` as a top-level `&&` conjunct and no
top-level `||`. The matching build (PLATFORM=winapi) never defines that
macro, so such a definition can never reach Reccmp.

- `#else` branches, negations and `||` alternatives still count. Fixtures
  cover each form (`make tools-test`).
- Native-only port files wrap their body in that guard. The four new
  files, `creature_scene`'s helpers and `scene_render`'s drawing code
  therefore add no entries. #527 added 67.
- 49 existing entries, all in MINIWIN-only regions of `movie.cpp`,
  `surface.cpp`, `draw_port.cpp`, `images_unit.cpp` and the sound code,
  drop out. The ratchet goes from 4,110 to **4,061**.

Keeping the files in `gods98/` and `game/`, beside `lwsc_winapi.cpp` and
`actor_scene.cpp`, rather than in `platform/native/` keeps them in the
binary-progress input manifest, as ADR 0006 asks for port-tail objects.

### 5. Other changes from #527

- `scene_render.cpp` (port tail, linked into the matching build) loses
  #527's unconditional `#include <stdio.h>`. Nothing used it, and it was
  the only part of #527 the matching build's preprocessor saw in that
  file.

## Gate

| Check | Result |
| --- | --- |
| `make reccmp-check` (distrobox) | OK: no function below the baseline; link-order map check OK. game-only 2,173 mapped, mean 54.75%; full 2,409 mapped, mean 59.11% |
| `make match-progress` | baseline, listing digests and binary progress: only `input_sha256`, `baseline_sha256` and `listing_digests_sha256` change. No score, digest or layout metric moved |
| `make unit-data-offsets` | full 0, gaps-only 2, partial 25, n/a 198 |
| `make tools-test` | OK; lint 4,061 entries, none new |
| `link_order.py check`, Meson parity | OK / OK |
| Native build + `ctest` | 37/37 |

`binary_progress.py` needs `pefile`, which the distrobox lacks, so
`binary-progress-compare` ran on the host, as in finding 552.

`tools/reshape_check.py --base origin/main` on every touched matching
input (`files.cpp`, `frameloop.cpp`, `view.cpp`, `entity.cpp`,
`entity_config.cpp`, `scene_render.cpp`, `animation_object.c`, which
reads `container.h`) reports identical matching tokens and an identical
object. `legoobject.cpp` has identical tokens but a different object, and
`--base origin/main --head origin/main` reports the same difference. Its
matching object is not reproducible between compiles; the full gate
shows no change.

## Behaviour check

As in #527: the native build under Xvfb (`SDL_VIDEO_DRIVER=x11`,
`LEGORRR_LEVEL=Tutorial01`, the Moving Rock Raiders tutorial,
`LEGORRR_NO_MOVIES=1`, the mission brief dismissed with SPACE, 1,400
frames).

- The Raiders render as their minifigure meshes, with authored textures
  (yellow helmets, orange suits), not the box stand-in.
- The log has no `creature-scene` or `container` load failure.

## Not done (as in #527)

- **No in-game walk check.** On native, the tutorial's NEXT arrow does not
  respond, so the movement step cannot be reached. Native Raiders start
  with no activity (-1) and keep their first authored pose until the AI
  sets one.
- **No SAMPLE sounds.** SAMPLE callbacks (footsteps) are not dispatched
  for native Containers: the sound route reads the master frame as a
  32-bit position handle.
- **WinAPI unchanged.** The WinAPI/Wine runtime still uses `actor_scene`.
- **Not on native yet:** buildings, vehicles, LOD mesh lists
  (`mesh_lod_load` is still a no-op adapter) and D3DRM viewport rendering.

## Commits

| Commit | Content |
| --- | --- |
| reccmp lint | native-only branches need no annotation; ratchet −49 |
| native level loading | `files.cpp` directory check; `frameloop.cpp` `Main::SharedObjects` |
| backend | `gods98/lwsc_miniwin`, `gods98/container_native` (port tail) |
| seams | `gods98/container_route`, the `container.h` routing block, `position_get` alternatives |
| call sites | `game/creature_scene`; `legoobject.cpp`, `entity.cpp`, `entity_config.cpp` arms |
| drawing | `scene_render.cpp` |
| regenerated | match baseline, listing digests, binary progress |
| finding | this file |
