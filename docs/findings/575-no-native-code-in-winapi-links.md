# Finding 575 — Native-backend code in the winapi links

Issue #576. Measured on `main` at `9b30b563`. The matching build
(`LRR_CLASSIC_MSVC5`, `PLATFORM=winapi`) linked no SDL3 or miniwin object,
library or symbol, and no object from `src/platform/native/`. It did link the
native entry chain (`main.cpp`'s `LegorrrMain`, `LegorrrRun`) and about
28 KB of functions that only the native backend or the native tests call.
This change removes 18,752 bytes of `.text` from the link, and keeps the
annotated code and every score unchanged. The new `tools/winapi_link_audit.py`
fails any winapi link that contains native-only code again.

## Summary

1. **No SDL or miniwin code reached a winapi link.** The link-order list
   already tagged `platform/native/*`, `container_native.cpp`,
   `container_route.cpp`, `creature_scene.cpp` and `lwsc_miniwin.cpp` as
   `native`. Their headers do not exist in the winapi toolchains, so they
   cannot compile there either.
2. **The native entry chain did.** The winapi `WinMain` runs the frame loop
   through `frameloop_application_*` (`platform/winapi/entry.cpp`). Only
   `platform/native/entry.cpp` calls `LegorrrMain` (`main.cpp`), which calls
   `LegorrrRun` and the two scaffold-name functions
   `gods98_engine_name` (`gods98/gods98.cpp`) and `game_layer_name`.
   - `main.cpp` and `gods98/gods98.cpp` are now tagged `native` in
     `src/link_order.cmake`.
   - `LegorrrRun` and `game_layer_name` are guarded in their shared files.
3. **Native-only and test-only functions were spread through the shared
   files.** 90 functions (8,352 bytes) are reachable only from the native
   backend, and 399 (19,584 bytes) only from the native tests. This change
   removes 266 of them (17,200 bytes): two whole objects leave the winapi
   list, and `#if !defined(LEGORRR_BACKEND_WINAPI)` guards the rest, in 38
   files. 12 unreachable port functions (1,552 bytes) inside the guarded
   runs go with them.
4. **No score moved.** `make reccmp-check` passes, and all 2,470 mapped
   functions have the same score as before. Every guard sits after its
   file's last annotated function (rule B, finding 538), or in a file
   with no annotated function.
5. **What stays** (10,736 bytes of the two categories) has a reason. It
   sits before annotated functions, it lives in a file another PR is editing,
   or an unreachable or annotated caller still references it. It is listed
   below.

## Method

Every function in the matching build's `.text` was sorted into one category,
in this order:

| Category | Rule |
| --- | --- |
| (a) original, annotated | its rebuilt address is a Reccmp-mapped function (`reccmp-report/results.json`) |
| (b) port tail, reachable | not (a), and it survives a scratch relink of the same objects with `/OPT:REF` (#562's method), so the winapi entry point reaches it |
| (d) native-backend only | not (a) or (b), and a scratch native link with `-O0 -fno-inline -ffunction-sections -Wl,--gc-sections` keeps a function of that base name |
| (e) tests only | not (a), (b) or (d), and a native test object (`test_*.cpp`) references that base name |
| (c) port tail, unreachable | everything else in unit, legacy and tail objects |
| LIBC, import thunks | the map's library objects |

- Each map symbol owns the bytes up to the next one, as in `pe_audit.py`.
- (d) and (e) match by base name, because the native and MSVC decorations
  differ. A static that shares a base name with a native-reachable function
  can therefore land in (d). The four `platform/winapi/frameloop.cpp`
  entries in the "after" residue are such cases: they are winapi
  implementations of the platform API that only `LegorrrRun` called, so
  they are really (c).
- The `/OPT:REF` relink is unreachable code by #562's measure. This finding
  uses it only to split (b) from the rest. It does not report it again.
- The MinGW reference build (`PLATFORM=winapi` without
  `LRR_CLASSIC_MSVC5`) could not be configured here: neither the host nor the
  `developer` container has `i686-w64-mingw32`. It compiles the same sources
  from the same list, with the same `LEGORRR_BACKEND_WINAPI` guards, and its
  executable no longer compiles `main.cpp` directly. So the removals apply to
  it too, and `check-list` covers its source list.

## Inventory, matching build `.text`

| Category | Before: functions | objects | bytes | After: functions | objects | bytes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| (a) original, annotated | 2,233 | 87 | 469,872 | 2,233 | 87 | 469,872 |
| (b) port tail, reachable | 2,297 | 96 | 387,120 | 2,297 | 97 | 387,120 |
| (c) port tail, unreachable | 1,383 | 92 | 101,168 | 1,371 | 92 | 99,616 |
| (d) native-backend only | 90 | 35 | 8,352 | 53 | 24 | 4,544 |
| (e) tests only | 399 | 37 | 19,584 | 170 | 19 | 6,192 |
| LIBC | 441 | 159 | 69,344 | 441 | 159 | 69,344 |
| import thunks | 151 | 7 | 906 | 151 | 7 | 906 |
| **`.text`** | | | **1,056,346** | | | **1,037,594** |

- No category held an SDL or miniwin symbol, or an object from
  `src/platform/native/`, before or after.
- (b) gains one object because a guard split a run in `view.cpp`. Its byte
  count does not change.
- The data sections shrink with the removed code: `.rdata` by 128 bytes,
  `.data` by 176 and `.bss` by 16.
- (c) by link-order kind, after: units 7,824, legacy 26,816, tail 64,976.
  The largest objects are `entity.cpp` (game) 9,504, `view.cpp` 8,704,
  `resource.cpp` 6,480, `surface.cpp` 5,968, `lego.cpp` 5,792,
  `level.cpp` 5,616, `terrain_grid.cpp` 5,488 and `frontend.cpp` 5,120.
  #562 drops this code through `/OPT:REF`; the conversions retire it.

## What is guarded

The functions guarded are those that the winapi link does not reference at
all. Reachability is not enough: an unreachable function is still referenced
when an unreachable or annotated caller names it, and the link then fails.
The first pass guarded every unreachable (d) and (e) function. Each
unresolved external was then unguarded, until the matching build linked.

| File | Guarded runs | What |
| --- | ---: | --- |
| `game/ai.cpp` | 6 | test accessors, and `ai_path_build` (tests only) |
| `game/cmd.cpp` | 14 | test accessors; `cmd_create_move` (native only) |
| `game/entity.cpp` | 8 | test accessors; the activity-runner update and stats readers that only the native creature scenes call |
| `game/game.cpp` | 1 | `game_layer_name` (native entry chain) |
| `game/interface_port.cpp` | 10 | selection and HUD adapters that only native code calls; two test accessors |
| `game/level.cpp` | 19 | test accessors; `map_cell_tracking_flags_clear_and_refresh` (native only) |
| `game/priorities.c` | 1 | three test accessors at the end of the unit |
| `game/savegame.cpp` | 6 | test accessors |
| `game/select.cpp` | 3 | test accessors; `selection_clear_marker_and_reset_hud` (native only) |
| `game/startup_config.cpp` | 1 | the dependency-grid config hook and loader (native and tests) |
| `game/terrain_map_port.cpp`, `reward_port.cpp`, `upgrades.cpp` | 4, 3, 3 | test accessors |
| `game/terrain_grid.cpp`, `util.cpp` | 1, 2 | native-only helpers (quad reset, motion jitter) |
| `game/info_port.cpp`, `mission.cpp`, `objective.cpp`, `options.cpp`, `terrain_cell_objects_port.cpp` | 1 each | test accessors |
| `gods98/config.c` | 1 | `block_parse` and the token-pool peeks, after the unit's functions |
| `gods98/frameloop.cpp` | 1 | `LegorrrRun`, its last function |
| `gods98/lwsc_scene.cpp` | 4 | four scene and instance calls that only `container_native.cpp` makes |
| `gods98/simtick.cpp` | 24 | test accessors and hook setters |
| `gods98/engine.cpp`, `input.cpp`, `media.cpp`, `message.cpp` | 4, 4, 3, 5 | test accessors |
| `gods98/sound.cpp` | 4 | test accessors; three sound-bank readers (native only) |
| `gods98/view.cpp` | 5 | test accessors; two native-only scene helpers |
| `gods98/mainsim.cpp`, `scene_resources.cpp` | 1 each | native-only (`main_sim_step`, `scene_node_transform_matrix_apply`) |
| `gods98/entity.cpp`, `flic.cpp`, `model.cpp`, `record.cpp`, `resource.cpp`, `texture.cpp` | 1 or 2 each | test accessors and digests |

The native build is unchanged: the guards are true there, and the native
tests still link every accessor.

## What stays

After this change, (d) and (e) hold 10,736 bytes:

| Reason | Bytes | Where |
| --- | ---: | --- |
| File edited by a parallel PR | 2,720 | `frontend.cpp` 2,320 (68 test accessors; #561), `lego.cpp` 400 (#574) |
| Before annotated functions, so guarding would renumber them (rule B) | 3,040 | `crt.cpp` 1,600 (55 test accessors), `audio.cpp` 832 (the native PCM audio player), `resource.cpp` 576, `gods98/frameloop.cpp` 32 |
| Referenced by an unreachable or annotated caller | 4,272 | `sound.cpp` (`sound_parse` chain), `sound_wave.cpp`, `entity_config.cpp`, `level.cpp`, `entity.cpp`, `effects.cpp`, `view.cpp`, `texture.cpp` and others |
| Base-name collisions, really (c) | 704 | `platform/winapi/frameloop.cpp`'s `platform_*` functions |

- The first two rows can go once those PRs land, or when the units' files
  are converted and the accessors move to the port tail.
- The third row leaves with its callers: through #562's `/OPT:REF` once
  conversions give the annotated callers their original callees, or when the
  callers are converted.
- The debug hooks inside live functions (`LEGORRR_LEVEL`,
  `LEGORRR_AUTOSTOP_FRAMES`, `LEGORRR_NO_MOVIES` in `gods98/frameloop.cpp`)
  are inside reachable (b) code and before annotated functions. They are
  left in place.

## The guard: `tools/winapi_link_audit.py`

- **`check-list`** (in `make tools-test`, no toolchain) checks the link-order
  list's winapi view:
  - No source under `platform/native/` or `miniwin/`, and no `*_miniwin.*`,
    `*_native.*` or SDL-named source, without a `native` tag.
  - No source in its native-only table (`main.cpp`, `gods98/gods98.cpp`,
    and the native backend files) without a `native` tag.
  - No SDL or miniwin library in `LRR_LINK_LIBRARIES`.
- **`check-link`** (in `make reccmp-check` and `make match-progress`, as
  `make winapi-link-check`) checks an existing matching build. It reads
  `objects1.rsp`, `link.txt` and `LegoRR.map`. It fails on:
  - an object from a native-tagged or native-only source;
  - an SDL or miniwin library, map object or symbol;
  - a native entry-chain symbol (`LegorrrMain`, `LegorrrRun`,
    `gods98_engine_name`, `game_layer_name`);
  - a map object that is not a winapi entry of the list.

  It then prints `.text` by category: annotated original code, port code by
  link-order kind (unit, legacy, tail), LIBC and import thunks. On this
  build: 469,872 annotated, port code 46,128 in units, 125,120 in legacy files
  and 326,224 in the tail, LIBC 69,344, and 906 in thunks. How much of the
  port code `/OPT:REF` would drop is #562's figure.
- Its tests (`tools/tests/test_winapi_link_audit.py`) use synthetic lists,
  response files and maps, and check the repository's own list. Run against
  `main`'s list and build, the check reports `main.cpp`, `gods98.cpp` and
  the four entry-chain symbols.

## Reproducing

```sh
make reccmp-check            # builds, gates scores, runs winapi-link-check
make winapi-link-check       # the link check and category report alone
```

For the inventory:
1. Relink with `/OPT:REF` as finding 563 describes.
2. Configure a scratch native build with
   `-DCMAKE_BUILD_TYPE=None -DCMAKE_CXX_FLAGS="-O0 -fno-inline -ffunction-sections -fdata-sections" -DCMAKE_EXE_LINKER_FLAGS="-Wl,--gc-sections"`.
   Collect the function names that `nm -C --defined-only` lists in its
   `LegoRR`.
3. Collect the undefined symbols of the native test objects (`nm -C -u` on
   `test_*.cpp.o`).
4. Sort each map symbol by the table in Method.
