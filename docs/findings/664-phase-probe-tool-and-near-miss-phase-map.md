# Finding 664 — The symbol-counter phase probe as a tool, and the near-miss pool's phase map

Issue #783, part of #388 and ADR 0005. Follows finding 663, which found
that about one cause-C near miss in six is a finding 538 rule B phase
case, and left its probe in `tools/experiments/776/`. This change makes the
probe a supported tool and runs it over every 80–99% near miss outside the
units #784, #785 and #786 own.

**Result: the tool is in place and the pool is mapped. No gap could be tied
to one of the original's declarations, so no source changes.**

- **The tool.** `tools/phase_probe.py` (`make phase-probe`) merges finding
  663's `phase_probe.py` and `pads_at.py`. It finds each function's codes
  over a counter range, scores each code in place (lock and backup), and
  scores whole units with pads at chosen lines. Its tests are in `make
  tools-test`.
- **The pool.** 207 near misses in 50 units were probed over N = 0–255.
  **19 are phase-sensitive**: some shift scores higher than the current
  code. In 7 of them a shift gives 100% (one of the 7 is an effective
  match). 16 more have several codes, but the current one is the best.
  **172 keep one code**, so the source fixes their code (finding 663 rule
  3).
- **The gaps are not where a declaration can be named.** In all 19, the
  function rises wherever the shift goes before it. In 7 nothing else
  falls anywhere, because the unit's other functions do not depend on the
  phase, so the gap cannot be narrowed to one stretch of the file. In 9
  the shift costs another function wherever it goes. Only 3 have edges:
  FrontEnd.c `0x411870` and LegoObject.c `0x445df0`/`0x4460f0`. Six need
  shifts of 50–255. They behave like thresholds on the counter's absolute
  value, not like short periods. These are header-sized gaps. The units
  include no `<windows.h>` or `<d3drm.h>`, which the original's includes
  certainly brought in (finding 646). Adding `<windows.h>` alone to
  scrollinfo.c lowers two functions (`00463c90` 100 → 92, `00463d20`
  86.46 → 82.29) and raises none. DirectX 6's header chain is not
  available to the matching build. Choosing any declaration for its count
  would be a pad (ADR 0005 rule 3; finding 538's "a pad by another name"),
  so none was committed.

| Measure | `main` `b928af37` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 336,831 (52.42%) | 336,831 (52.42%) |
| Accuracy, full / game-only | 97.07% / 96.74% | 97.07% / 96.74% |
| Functions that rose / fell | | 0 / 0 |
| Image end (gate) | `0x7f5000` | `0x7f5000` |

No source or build input changes; this is tooling and documentation only.
`make reccmp-check` passes with no function lower. `make match-progress`,
`make unit-data-offsets` and `make pe-audit` were run and change no
generated file. `make tools-test` (with the new tests), `make
winapi-link-check` and native ctest (37 of 37) pass, and the native build
starts.

## The tool

```sh
make phase-probe ADDR=0x00479a30
make phase-probe ADDR="0x0044fab0 0x0044fe70" PROBE_ARGS="--range 0 128 1 --at 820"
make phase-probe PADS_AT="16:52 16:52,901:3" UNIT=src/game/terrain_map.c
```

`probe` puts N inert `extern int phase_pad_k;` declarations on one
file-scope line. By default that is the first blank line after the unit's
first `#include`; `--at LINE` names another. For every N in `--range` it
compiles a scratch copy with `/FAs` (parallel, `--jobs`) and fingerprints
each function's listing with the counter's numbers removed. It then
compiles the first N of each distinct code at the real path and scores it
with Reccmp. The function is classed as `single-code`, `phase-sensitive`
(a code scores higher than N = 0) or `current-best`, with the shifts that
reach the best code and the row's period. `pads-at` scores every
annotated function of a unit with `LINE:N` pads at chosen lines and prints
rises and falls. See [`tools/README.md`](../../tools/README.md#phase_probepy--symbol-counter-phase-probe-findings-538-663-664).

What changed from the experiment kit:

- **Pads share a line with existing text.** The line count stays the same,
  so the annotations and the COFF line records hold. A pad line is checked
  first. It must be at file scope, outside comments and `#if` blocks, and
  not a directive. Braces inside `#ifdef __cplusplus` (the `extern "C" {`
  wrappers) do not count as scope.
- **In-place scoring, as `match-search` does.** Each scored candidate is
  written over the real source under an exclusive `flock`. The candidate
  is recorded in `in-place.lock`, with `original.c` kept beside it in
  `build-match/phase/<unit>/`, and the file is restored before analysis. A
  probe killed with a candidate in place is undone on the next run. So
  `__FILE__` and the line records are the original's.
- **The fingerprint removes block-scope locals' counter suffixes**
  (`_buff$23614`) as well as `$S`/`$L` numbers and goto labels. Without
  that, a function with a block-scope local shows a new "code" at every N.
  A first pass of 9 units hit this: 24-plus codes for 17 functions in
  Lego.c, LegoObject.c, FrontEnd.c and Container.c. All 9 were probed
  again, and all 17 now have one or two codes.
- **Static C functions** (no Reccmp symbol) are fingerprinted under `_name`.
  Functions with no compiled code are listed as `skipped`.

`tools/experiments/776/phase_probe.py` and `pads_at.py` are removed, and
that kit's README points here.

## The probe table

Every 80–99.99% function in the match baseline, in the units listed below,
was probed over N = 0–255. Units owned elsewhere this wave were skipped:
AITask.c, Flic\*, LWO.c, Config.c (block_open), Interface.c, Reward.c,
Panel.c, Objective.c and NERPs (#784, #785). "Now" is the single-function
score at N = 0. It can sit below the full report where a callee resolves
only at link time. `*` is an effective match.

| Address | Function | Unit | Kind | Codes | Now | Best | Shifts (first) | Period |
| --- | --- | --- | --- | ---: | ---: | ---: | --- | ---: |
| `00409b30` | construction_zone_complete_building | `game/construction.c` | phase-sensitive | 2 | 88.89 | 90.97 | 172, 173, 174 | — |
| `00411360` | front_menu_item_select_test_string_collision | `game/frontend_unit.c` | phase-sensitive | 11 | 91.84 | 94.26 | 100, 101, 102, 103, 104, 105 | — |
| `00411870` | front_menu_item_slider_handle_input | `game/frontend_unit.c` | phase-sensitive | 11 | 90.70 | 100.00 | 46, 47, 48, 49, 50, 51 | — |
| `00442b10` | legoobject_movement_accumulator_apply | `game/legoobject_unit.c` | phase-sensitive | 2 | 84.31 | 90.27 | 7, 8, 9, 10, 11, 12 | — |
| `00445df0` | legoobject_flag_gated_callback_b | `game/legoobject_unit.c` | phase-sensitive | 2 | 72.52 | 79.82 | 2, 6, 7, 8, 9, 10 | 64 |
| `004460f0` | legoobject_flag_gated_callback_a | `game/legoobject_unit.c` | phase-sensitive | 2 | 70.88 | 71.91 | 2, 3, 4, 5, 6, 10 | 128 |
| `00463d20` | view_blit_rect_positions_apply | `game/scrollinfo.c` | phase-sensitive | 10 | 86.46 | 100.00* | 8, 9, 10, 11, 12, 13 | — |
| `00465430` | transient_effect_emitter_create | `game/sfx.c` | phase-sensitive | 2 | 92.81 | 94.09 | 50, 51, 52, 53, 54, 55 | — |
| `0040f920` | kind_two_steering_torque_apply | `game/steering.c` | phase-sensitive | 11 | 81.48 | 83.33 | 8, 9, 10, 11, 12, 13 | — |
| `0044e6d0` | terrain_map_block_texture_fade_set | `game/terrain_map.c` | phase-sensitive | 2 | 94.21 | 95.87 | 243, 244, 245, 246, 247, 248 | — |
| `0044fab0` | terrain_map_world_normal_get | `game/terrain_map.c` | phase-sensitive | 2 | 91.78 | 100.00 | 52, 180 | 128 |
| `0044fe70` | terrain_map_world_z_unmodified_get | `game/terrain_map.c` | phase-sensitive | 2 | 97.99 | 100.00 | 3, 7, 11, 15, 19, 23 | 128 |
| `0046bc30` | tooltip_draw | `game/tooltip.c` | phase-sensitive | 2 | 87.22 | 93.33 | 191, 192, 193, 194, 195, 196 | — |
| `0046f3b0` | weapons_projectile_hit_check | `game/weapons.c` | phase-sensitive | 2 | 87.75 | 92.43 | 233, 234, 235, 236, 237, 238 | — |
| `0046faf0` | weapons_projectile_create | `game/weapons.c` | phase-sensitive | 2 | 84.50 | 86.05 | 105, 106, 107, 108, 109, 110 | — |
| `00473e10` | lego_fog_colour_set_adapter | `gods98/container.c` | phase-sensitive | 2 | 82.35 | 100.00 | 55, 56, 57, 58, 59, 60 | — |
| `004750b0` | render_object_set_packed_rgba_from_float_channels | `gods98/container.c` | phase-sensitive | 2 | 90.32 | 100.00 | 65, 66, 67, 68, 69, 70 | — |
| `00479a30` | ray_plane_intersection_parameter | `gods98/maths.c` | phase-sensitive | 8 | 93.62 | 100.00 | 2, 3, 6, 7, 18, 19 | 128 |
| `0047ad70` | s_play_seam | `gods98/sound3d.c` | phase-sensitive | 2 | 96.73 | 99.18 | 255 | — |
| `00411a00` | front_menu_update_menu_items_input | `game/frontend_unit.c` | current-best | 2 | 84.38 | 84.38 | — | — |
| `0044c1b0` | legoobject_heading_step_move | `game/legoobject_unit.c` | current-best | 2 | 97.75 | 97.75 | — | — |
| `0045dbc0` | radar_map_tv_position_set_adapter | `game/radarmap.c` | current-best | 4 | 93.50 | 93.50 | — | — |
| `0045e940` | radarmap_transform_rect | `game/radarmap.c` | current-best | 2 | 84.11 | 84.11 | — | — |
| `00465c50` | transient_effect_particle_slot_initialize | `game/sfx.c` | current-best | 2 | 95.51 | 95.51 | — | 64 |
| `00465e00` | transient_effect_segment_advance | `game/sfx.c` | current-best | 2 | 89.04 | 89.04 | — | 64 |
| `0040fb90` | steering_physics_step | `game/steering.c` | current-best | 2 | 100.00 | 100.00* | — | — |
| `0040fd00` | steering_item_wander | `game/steering.c` | current-best | 2 | 89.66 | 89.66 | — | — |
| `0046e610` | water_update_flow_blocks | `game/water.c` | current-best | 15 | 100.00 | 100.00* | — | — |
| `00471630` | weapons_ray_block_sweep | `game/weapons.c` | current-best | 2 | 100.00 | 100.00* | — | 8 |
| `00476410` | Container_Frame_SearchCallback | `gods98/container.c` | current-best | 2 | 100.00 | 100.00 | — | — |
| `0047e190` | Image_DDColorMatch | `gods98/images_unit.c` | current-best | 6 | 98.06 | 98.06 | — | — |
| `004876c0` | lws_initialise | `gods98/lws.c` | current-best | 2 | 100.00 | 100.00 | — | — |
| `004789c0` | audio_mixer_control_init | `gods98/main.c` | current-best | 2 | 91.29 | 91.29 | — | 8 |
| `00479af0` | line_intersection_2d_compute | `gods98/maths.c` | current-best | 8 | 86.67 | 86.67 | — | — |
| `00477670` | game_string_hash_compute | `gods98/utils.c` | current-best | 2 | 93.44 | 93.44 | — | — |

The 172 single-code functions:

- `game/advisor.c` (2): `00401270`, `00401990`
- `game/animation_object.c` (2): `00406990`, `00406f80`
- `game/bezier.c` (2): `004067b0`, `00406820`
- `game/bubbles.c` (5): `004073a0`, `004074b0`, `00407510`, `004075a0`, `00407960`
- `game/building_model.c` (1): `004085f0`
- `game/collision.c` (1): `004089d0`
- `game/construction.c` (2): `00408ec0`, `00409a90`
- `game/detail.c` (1): `0040b5f0`
- `game/erode.c` (1): `0040ea10`
- `game/frontend_unit.c` (14): `00410490`, `00410c70`, `00413a60`, `00413b50`, `004140c0`, `004157c0`, `00416d70`, `00417090`, `00417250`, `00417430`, `00417460`, `00417eb0`, `00417f40`, `00417f90`
- `game/helpwindow.c` (3): `004181b0`, `00418a00`, `00418da0`
- `game/info_unit.c` (2): `00419530`, `00419da0`
- `game/lego_unit.c` (25): `00424730`, `004250c0`, `00425d90`, `00426420`, `00428020`, `004288e0`, `004291a0`, `00429470`, `00429890`, `0042bf40`, `0042cb70`, `0042cda0`, `0042cf50`, `0042d100`, `0042d2b0`, `0042d460`, `0042de40`, `0042e8b0`, `0042f0c0`, `0042f350`, `0042f6f0`, `00430530`, `00430df0`, `00431b20`, `00433e80`
- `game/legoobject_unit.c` (29): `00437940`, `004385b0`, `0043a120`, `0043ac90`, `0043ad50`, `0043aea0`, `0043b3b0`, `0043b8d0`, `0043c2f0`, `0043c3e0`, `0043c810`, `0043eea0`, `0043f6a0`, `0043fe70`, `0043fff0`, `004401b0`, `004403d0`, `00440800`, `00441ed0`, `004428a0`, `00445770`, `004469c0`, `00447210`, `004477d0`, `004492b0`, `00449cf0`, `0044a200`, `0044afb0`, `0044bf00`
- `game/lighteffects.c` (3): `0044d280`, `0044d760`, `0044d9a0`
- `game/message_unit.c` (2): `004526e0`, `00452d60`
- `game/objinfo.c` (5): `00459560`, `00459900`, `00459a50`, `00459f50`, `00459fd0`
- `game/pointer.c` (1): `0045cc70`
- `game/priorities.c` (3): `0045cdc0`, `0045d060`, `0045d550`
- `game/radarmap.c` (2): `0045e460`, `0045e9e0`
- `game/roof.c` (1): `00463590`
- `game/screen_effect_quads.c` (3): `0040a5c0`, `0040a5e0`, `0040aa10`
- `game/scrollinfo.c` (2): `004638a0`, `00463c00`
- `game/selectplace.c` (1): `00464110`
- `game/sfx.c` (1): `00464c70`
- `game/steering.c` (1): `0040f6c0`
- `game/terrain_cell_objects.c` (6): `0040cea0`, `0040cf50`, `0040d0a0`, `0040d5e0`, `0040d720`, `0040de40`
- `game/terrain_map.c` (5): `0044e8c0`, `0044f700`, `004502e0`, `00450f30`, `004512d0`
- `game/upgrade.c` (1): `0046c160`
- `game/vehicle_model.c` (1): `0046c6f0`
- `game/water.c` (1): `0046e390`
- `game/weapons.c` (7): `0046eb80`, `00471370`, `004718d0`, `00471960`, `00471d20`, `00472080`, `00472130`
- `gods98/avi_player.cpp` (2): `0047e480`, `0047eb20`
- `gods98/container.c` (5): `00472710`, `00472cd0`, `00473370`, `00473690`, `00473c60`
- `gods98/files.c` (2): `0047f590`, `0047f980`
- `gods98/font.c` (1): `0047a1e0`
- `gods98/g98_024.c` (2): `004893f0`, `00489410`
- `gods98/images_unit.c` (1): `0047e300`
- `gods98/keys.c` (1): `00485e30`
- `gods98/lws.c` (1): `004869f0`
- `gods98/main.c` (6): `00477bf0`, `00477f80`, `004780b0`, `004781d0`, `004783f0`, `00478940`
- `gods98/maths.c` (2): `00479510`, `004798e0`
- `gods98/mesh_unit.c` (3): `00481820`, `00481c50`, `00482040`
- `gods98/movie_unit.cpp` (1): `004724a0`
- `gods98/sound3d.c` (3): `0047baa0`, `0047bc30`, `0047bdb0`
- `gods98/textwindow.c` (2): `00485390`, `00485710`
- `gods98/viewports.c` (2): `00477010`, `00477150`
- `gods98/wad.c` (2): `0048bd30`, `0048be00`

## The gap map

For each phase-sensitive function F with best shift S, S pads were put on
the blank line before each function boundary up to F (24 sampled
boundaries plus the last 4 in the large units), and the whole unit was
scored (`pads-at`). F rose at every boundary tried. The table gives what
else moved:

| F | Unit | S | Clean from | Other rises | Falls (and where they stop) |
| --- | --- | ---: | --- | --- | --- |
| `00463d20` | scrollinfo.c | +8 | line 19 (file start) | — | none |
| `00473e10`, `004750b0` | container.c | +70 | line 32 (file start) | each other (both → 100) | none |
| `0044fe70` | terrain_map.c | +3 | line 20 (file start) | — | none |
| `0046bc30` | tooltip.c | +191 | file start | — | none |
| `00409b30` | construction.c | +172 | file start | — | none |
| `0047ad70` | sound3d.c | +255 | file start | — | none |
| `00411870` | frontend_unit.c | +51 | line 1222 (after `0x411530`'s start) | `00412c00` 46.64 → 49.42 | before 1156: `00411360` 91.84 → 90.63; before 1222: `00411530` 57.85 → 57.44 |
| `00411360` | frontend_unit.c | +100 | — | `00411870`, `00411530`, `00412c00` | `004177e0` 100 → 69.33, `00411a00` everywhere |
| `0044fab0` | terrain_map.c | +52 | — | — | `00450700` 92.31 → 69.23 everywhere; `0044e0c0`, `0044f5c0` before lines 151 / 671 |
| `0044e6d0` | terrain_map.c | +243 | — | `0044fe70` | 9–10 functions everywhere |
| `00479a30` | maths.c | +3 | — | — | `00479c10` 49.35 → 29.68 everywhere |
| `00465430` | sfx.c | +50 | — | — | `00465c50`, `00465e00` everywhere |
| `0040f920` | steering.c | +8 | — | — | `0040fcc0` 100 → 91.30 everywhere |
| `0046f3b0` | weapons.c | +233 | — | `0046faf0`, `0046fc70` | `0046ffb0`, `00471a60` |
| `0046faf0` | weapons.c | +105 | — | `0046fc70` | `0046ffb0` |
| `00442b10` | legoobject_unit.c | +7 | — | `00445df0` | `0043c200` before line 3272, `00445830` after line 388 |
| `00445df0`, `004460f0` | legoobject_unit.c | +2 | line 3272 | each other | `0043af30` before line 2692, `0043c200` before line 3272 |

Reading it:

- **Seven are clean anywhere** (six rows). Their unit's other functions keep one code,
  so the shift's place is not constrained. The tool shows that the
  original numbers more symbols before them than the port does. It does
  not show which declarations those are. For scrollinfo.c the needed
  residue is +8 to +20 (mod 32, `D` in its row), or −12 to −24.
- **LegoObject.c `0x445df0`/`0x4460f0`** (+2, both rise together) are
  clean only when the shift comes after `0x43c200` (line 3156; the first
  clean sample is line 3272). That leaves lines 3156–7447, about 120
  functions, so the gap is not narrow.
- **FrontEnd.c `0x411870` is the narrowest gap.** The shift must come
  after `0x411360`'s start (line 1086) and is cleanest after
  `0x411530`'s (line 1157). Lines 1157–1266 hold `0x411530`, `0x411790`
  and `0x411840`, with 10, 6 and 1 locals. Only +51 or +55 (mod 64) also
  keeps the exact `0x4177e0` (its phase is `ABBABBBA`… with period 128).
  That is 13 or 9 fewer symbols than the port declares, or 51 more. Without
  pads, no evidence was found for either.
- **Terrain_map.c `0x44fab0`** (finding 663: +52 mod 128, one value in 128)
  costs `0x450700` 23 points wherever the shift goes. `0x44fe70`'s +3
  (mod 4) is clean anywhere. So finding 663's guess that `0x44fb90`'s
  locals hold the second gap is not confirmed. A −1 or +3 anywhere before
  line 901 does it, and `0x44fb90`'s extra pushes (`edi`, `ebx`) are saved
  registers, not locals.
- **Six need 50 or more.** construction.c, tooltip.c, sfx.c, sound3d.c,
  weapons.c and terrain_map.c's `0x44e6d0` have codes that change once
  over 0–255 (`A…AB…B`), not periodically. Container.c's two switch at 55
  and 65, and `0x473e10` switches back at 183. These follow the counter's
  absolute value, so they wait on the header set.
- **Rule A is not the lever.** In scrollinfo.c `0x463d20` every
  difference is x87 operand order. Swapping the source operands of the
  three additions leaves it at 86.46. The order there comes from the
  phase (rule B), not from the source (rule A).

## Leads (not applied)

- **The SDK headers.** None of the units above includes `<windows.h>` or
  `<d3drm.h>`. Eight gods98 units do (Config.c's `_MSC_VER == 1100`
  pattern, finding 646, and Init, Images, DirectDraw, Files, Flic, Mesh
  and Main). The original
  called Win32 and DirectX 6 interfaces from these units. Its counter
  therefore stood thousands of symbols past the port's before the first
  function. Rebuilding the original's include set needs DirectX 6's
  `d3drm.h` chain (finding 646), which the toolchain lacks.
- **Maths.h's full inline set.** The per-unit `*_maths.h` headers each copy
  only the Maths.h helpers that their unit expands (findings 510 and 544).
  The original's Maths.h defined every helper in each unit that included it.
  Unreferenced `static __inline` functions leave no code, so the binary
  cannot say what the full set was. It is a real declaration difference
  of unknown size, not a fix.
- **FrontEnd.c `0x411530`** (57.85%) sits in the one localised gap. Its
  shape differs (the original calls `font_get_height` where the port
  calls `object_field_be4_dword_get`, and the slider branch is ordered
  differently), so a rewrite to the original's shape may change its local
  set. Re-probe `0x411870` after any rewrite there.

## Unfinished

- Periods above 256 were not probed (finding 663's caveat stands). The
  single-code list is "one code over 0–255".
- `current-best` functions with codes above the 24-code cap were not
  seen. After the fingerprint fix, no function reached the cap.
- No probe hit the 20-minute cap. The largest units (LegoObject.c,
  Lego.c) took 3–4 minutes each.

## Files

- `tools/phase_probe.py` (new), `tools/tests/test_phase_probe.py` (new, in
  `make tools-test`), `Makefile` (`phase-probe`), `tools/README.md`.
- `tools/experiments/776/phase_probe.py`, `pads_at.py` (removed; promoted),
  `tools/experiments/776/README.md`.
- `docs/findings/664-…` (this file) and its `README.md` entry. The
  match baseline, listing digests and `binary-progress.json` are
  unchanged.

No file owned by #784, #785 or #786 is touched.

## Reproducing

```sh
make phase-probe ADDR=0x00463d20
make phase-probe ADDR="0x00473e10 0x004750b0"
make phase-probe PADS_AT="1222:51 1156:51 1085:51" UNIT=src/game/frontend_unit.c
python3 tools/phase_probe.py probe 0x0044fab0 0x0044fe70 --json --output /tmp/terrain.json
```
