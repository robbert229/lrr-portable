# Finding 594 — LightEffects.c (GAME-024) rebuilt in original shape, with a port tail

Issue #625, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-024 is the original's `LightEffects.c` (text `0044c710`-`0044db50`): the
level's ambient light. Lego.c creates one coloured light under a root
container and hands both to LightEffects_Initialise. Lego.cfg's
`LightEffects` block configures three effects:

- blink: a random colour step every so often;
- fade: wait, fade to a random colour, hold, fade back;
- move: wait, then step the light in a random direction within MoveLimit.

A dimmer fades the light out while Lego.c finds no block under the cursor.
Every update clamps the colour to 0-1 and sets it on the light.

The unit was relocated verbatim into `src/game/lighteffects.cpp` (#496). This
change rebuilds it as the C unit
[`src/game/lighteffects.c`](../../src/game/lighteffects.c), with the
unit-private imports header
[`lighteffects_imports.h`](../../src/game/lighteffects_imports.h). The port's
implementation moves, unannotated, to the port tail
[`lighteffects_port.cpp`](../../src/game/lighteffects_port.cpp).

Stacked on #630 (G98-003 Viewports.c, `e6bf040c`, on `main` `12a0e950`).
Measured from the full matching build (`make reccmp-check`) of this change
rebased onto #630's head. The whole-binary "before" column is #630's
`binary-progress.json`. The change was first measured on `main` `2f988e92`
(#621), then on `12a0e950` (#627, after #626's LegoObject.c part 3); every
GAME-024 score is the same on all three bases.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, all 21 functions, 5,003 code bytes) | 22.45% | **98.03%** |
| Exact functions in the unit | 3 | **18 of 21** |
| Mapped functions in the unit | 18 | 21 |
| Exact functions, full / game-only (`binary-progress.json`) | 1,506 / 1,306 | **1,521 / 1,321** |
| Exact-function code bytes | 170,533 (26.54%) | **173,540** (27.01%) |
| Accuracy, full / game-only | 81.83% / 80.06% | 82.26% / 80.53% |
| `opt_ref` annotated, full (by address): removed / exact removed | 492 / 225 | 497 / 233 |
| `opt_ref` annotated, game-only (by address) | 475 / 214 | 480 / 222 |
| `opt_ref` by symbol (`callers --min 1`): removed / exact removed | 480 / 214 | **479 / 215** |
| Reccmp lint ledger | 1,288 | **1,283** |

Byte weights are the single-function loop's original sizes; an unmapped
function counts as 0%. The scaffold listed 20 functions; there are 21 (see
[the unit's edges](#the-units-edges)). Three are newly mapped:

- `0044c760`, a helper that two functions tail-call and nothing annotated;
- `0044cfa0` and `0044da50`, which had no source. The port modelled them in
  `util.cpp` under other contracts, behind block comments that Reccmp does
  not read.

The names are OpenLRR's, in the repository's snake case. "Port name" is the
name the port and `lego_unit.c` use. The unit keeps its own names in every
build (see [Callers](#callers-aliases-not-link-names)).

| Address | Function | Port name | Bytes | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `0044c710` | `light_effects_initialise` | `scene_transform_state_initialize` | 70 | 15.56 | **100.00** |
| `0044c760` | `light_effects_reset_colour` | — | 40 | unmapped | **100.00** |
| `0044c790` | `light_effects_set_disabled` | `scene_transform_enable_set` | 35 | 20.00 | **100.00** |
| `0044c7c0` | `light_effects_load` | `light_effect_configuration_load` | 43 | 100.00 | **100.00** |
| `0044c7f0` | `light_effects_load_blink` | `light_effect_blink_config_load` | 372 | 20.69 | **100.00** |
| `0044c970` | `light_effects_set_blink` | `light_effect_blink_parameters_store` | 71 | 50.00 | **100.00** |
| `0044c9c0` | `light_effects_load_fade` | `light_effect_fade_config_load` | 592 | 13.79 | **100.00** |
| `0044cc10` | `light_effects_set_fade` | `light_effect_fade_parameters_store` | 129 | 50.00 | **100.00** |
| `0044cca0` | `light_effects_load_move` | `light_effect_move_config_load` | 582 | 12.42 | **100.00** |
| `0044cef0` | `light_effects_set_move` | `motion_jitter_parameters_store` | 126 | 65.52 | **100.00** |
| `0044cf70` | `light_effects_invalidate_position` | `simtick_scene_position_reset_and_effect_motion_cancel` | 39 | 27.59 | **100.00** |
| `0044cfa0` | `light_effects_update` | `motion_gameplay_tick` (`util.cpp`) | 67 | unmapped | **100.00** |
| `0044cff0` | `light_effects_update_colour` | `scene_transform_rgb_clamp_apply` | 220 | 25.00 | **100.00** |
| `0044d0d0` | `light_effects_update_blink` | `motion_drift_offset_step` | 373 | 13.27 | **100.00** |
| `0044d250` | `light_effects_flip_sign` | `float_negate_absolute_value` | 35 | 58.82 | **100.00** (C; was naked asm) |
| `0044d280` | `light_effects_update_fade` | `motion_waypoint_timer_tick` | 1166 | 15.28 | 99.28 |
| `0044d710` | `light_effects_set_fade_speed` | `motion_waypoint_vector_random_scale_apply` | 66 | 100.00 | **100.00** |
| `0044d760` | `light_effects_update_move` | `motion_jitter_step` | 569 | 35.83 | 86.17 |
| `0044d9a0` | `light_effects_check_move_limit` | `motion_jitter_candidate_valid_retail` | 128 | 24.00 | 91.11 |
| `0044da20` | `light_effects_set_dimmer_mode` | `simtick_route_position_blocked_state_set` | 34 | 100.00 | **100.00** |
| `0044da50` | `light_effects_update_dimmer` | `motion_three_channel_tween_step` (`util.cpp`) | 246 | unmapped | **100.00** |

No function is naked asm. The port's naked-asm `0044d250` stays in the port
tail, unannotated; the unit's C form matches.

### What is left

| Function | Score | Instructions (orig / rebuilt) | Remaining difference and what was tried |
| --- | ---: | ---: | --- |
| `0044d280` Update_Fade | 99.28 | 276 / 276 | One x87 scheduling swap where the fade target is computed: the original does the three `* rnd` multiplies, then the three `+ min` adds; the rebuild interleaves one add. Tried: `min + fabs(…) * rnd`, `rnd * fabs(…) + min`, a `double` product and no cast (94.01, 90.51), zeroing `fadePosRGB` before the random draw (98.19), `rnd` as a `double` (80.64) and assigned inline (93.45). Permuter (240 s): no candidates |
| `0044d760` Update_Move | 86.17 | 161 / 158 | Two x87 spots. (1) The original loads all three components of `pos` before adding `moveVector`, and pops the Container_GetPosition arguments after two of the stores; the rebuild finishes `x` first. (2) The original loads `moveSpeed` onto the x87 stack before the square root and multiplies by `st(3)` three times; the rebuild multiplies by the global each time. Tried, all 86.17 or lower: inline Maths.h-style `add`/`subtract`/`scale`/`normalise` helpers (including Normalise then Scale by `moveSpeed`), the add inside the CheckMoveLimit argument, swapped `pos`/`newPos` declarations, `moveVector + pos`, `*=` in two passes, a `float` or `double` `speed`/`scale` local in four placements (77.12-81.35), and `speed * (scale * x)`. A 0-32 inert-declaration sweep changes nothing (no rule B phase), and a separate global for the speed shows the store-to-same-struct aliasing is not the cause. Permuter (300 s): no candidates |
| `0044d9a0` CheckMoveLimit | 91.11 | 44 / 45 | The original keeps `|dx|` after its compare (`fcom`, dropped later by `fstp st(1)`), keeps `dy` on the stack across the first return and spills `dz`. The plain form `if ((float)fabs(diff.x) > limit.x) return 0;` ×3 gives 85.71. All 64 combinations of "abs in place / abs at the compare" × "with / without `(float)`" for the three axes were compiled: 12 reach 91.11 (each has one in-place abs; the source keeps `x` and `y` in place), none higher. Also tried: `limit < fabs`, `!(fabs <= limit)`, `else if` chains, `||`/`&&` forms, a float array, scalars and the subtraction as an inline helper (60.27-85.71). Permuter (200 s): no candidates |

## The unit's edges

The unit map gives GAME-024 a high-confidence boundary, and the scaffold's
range is right:

- **Code.** Every function reads or writes the 0xf4-byte LightEffects_Globs
  at `004eadd8`. `0044c710` is not 128 bytes: it ends with `jmp 0x44c760`.
  `0044c760` is its own, 16-byte-aligned function (Container_SetColourAlpha
  with the initial colour), which `0044c710` and `0044c790` tail-call.
  MSVC turns `f(); }` into `jmp f` for a function with no arguments. It is
  `light_effects_reset_colour` here.
- **The next function.** `0044db50` (`loading_progress.cpp`) opens a file
  by key and reads none of the unit's data.
- **Data.** `.rdata` `0049e4b0`-`0049e4cc` holds eight float constants (`0.0`,
  `1/255`, `25.0`, `1.0`, `0.001`, `2.0`, `-1.0`, `0.1`). The unit's strings are
  `004a5184`-`004a528c`, plus the pooled `":"` (`004a1cd4`) and `""` (`004b8a14`).
  The `.bss` is the single struct, `004eadd8`-`004eaecc`.

## How the conversion is done

### The port tail

The port's light effects are another contract. Their state is spread over:

- `effects.cpp`'s `s_scene_transform_state` and config-load hook;
- `util.cpp`'s drift, waypoint and jitter records, with random-range hooks;
- a set of `_WIN32`-only module statics, which carried the original
  addresses as `// GLOBAL` annotations.

Native code calls this model (`engine.cpp`, `frameloop.cpp`, `level.cpp`,
`util.cpp`, `entity.cpp`, `tickers.cpp`), and so do the tests
(`test_simtick.cpp`, `test_view.cpp`). As in findings 582, 585 and 590,
`lighteffects.cpp` is renamed `lighteffects_port.cpp` and becomes a `tail`
entry. Each `// FUNCTION` becomes a `/* Port model of 0x… */` comment, and
the 26 `// GLOBAL` lines in the unit's `.bss` range are removed. No
definition changed. `util.cpp`'s two block-comment "annotations"
(`0044cfa0`, `0044da50`) also become `Port model of` comments.

### Callers: aliases, not link names

Every original caller of the unit is in Lego.c:

| Caller | Calls |
| --- | --- |
| `0041fb50` Lego config init | `0044c7c0` Load |
| `004232e0` Lego_MainLoop (`lego_engine_tick`) | `0044cfa0` Update |
| `00426520` the block-under-cursor pick | `0044da20` SetDimmerMode |
| `004288e0` debug keys | `0044cf70` InvalidatePosition, `0044c790` SetDisabled |
| `00433770` scene lights | `0044c710` Initialise |

`lego_unit.c` passes the original's arguments to all of them. The port's
other C++ callers run the port's own state, so unlike findings 581, 585 and
590 the unit does not take the port's names. Instead, as for finding 589's three
entries, `aitask_port_aliases.h` gets six matching-build `#define`s. It is
already included first by `lego_unit.c`, `legoobject_unit.c`, `priorities.c`
and `tutorial_script.c`, and none of the last three uses these names. The
`#define`s map `scene_transform_state_initialize`,
`scene_transform_enable_set`, `light_effect_configuration_load`,
`simtick_scene_position_reset_and_effect_motion_cancel`,
`motion_gameplay_tick` and `simtick_route_position_blocked_state_set` to the
unit's names. They add no `#include` and no declaration, so no symbol counter
moves (finding 582). In the matching build `lego_unit.c` initialises,
loads, updates and dims the unit's light. Natively the port keeps serving
every name, and nothing calls the unit.

`lego_engine_tick` (`004232e0`) rose from 97.02% to 97.09% (96.88% to 96.95% on `2f988e92`). Its `0044cfa0`
call operand now maps. Its listing digest is unchanged. The other four
callers already called annotated port entries at the same addresses, so
their scores did not move.

### State

The `.bss` is one `static LightEffects_Globs lightEffectsGlobs` at
`004eadd8`, 0xf4 bytes, laid out as the original:

- the light and its root (`+0x00`, `+0x04`); the initial and current colour
  (`+0x08`, `+0x14`);
- blink: the per-channel maximum step (`+0x20`), the timer and its range
  (`+0x2c`, `+0x30`), the red drift so far and its limit (`+0x38`, `+0x3c`);
- fade: the target (`+0x40`), the range of targets (`+0x4c`, `+0x58`), the
  wait and its range (`+0x64`, `+0x68`), the per-channel speed (`+0x70`), the
  fade-time range (`+0x7c`), the hold and its range (`+0x84`, `+0x88`), and
  the progress (`+0x90`);
- move: the start position and limit (`+0x9c`, `+0xa8`), the wait, speed and
  distance with their ranges (`+0xb4`-`+0xe0`), and the step (`+0xcc`);
- 12 unused bytes (`+0xe4`), and the flags (`+0xf0`).

The `.rdata` constants are literals. The 13 strings are marked
`// STRING` where they are used, one per line. `":"` and `""` are
`config.c`'s `s_config_rgb_separator` and `s_config_angle_empty` (finding
581). A `// STRING` marker for `004a1cd4` made Reccmp drop it as a duplicate
of `config.c`'s `// GLOBAL`. With a literal `":"` instead, the three loaders
scored 99.19%, 98.43% and 97.85%; with `config.c`'s symbol all three are exact.

`make unit-data-offsets`, GAME-024 row: `.data` gaps-only (13 strings, 0
unknown), so every string gap matches the original's. `.rdata` and `.bss` are
n/a (literals; one item).

### Source shapes that matched

- **The three loaders' frames** (`0044c7f0`, `0044c9c0`, `0044cca0`, about
  95% to 100%). Each tokenises into `char* argv[10]`. That is the original's
  0x28-byte array, and the `[2]` and `[3]` arrays first written left the
  frames 0x20 and 0x1c bytes short.
- **MaxChangeAllowed.** The original compares the `atof` result before
  storing it (`fst`, `fcompp`):
  `if (0.0f != (maxChange = (float)atof(…)))`, the same shape as finding
  590's Vehicle_Load. `Config_GetRealValue`'s double lookup of the key
  (`Config_GetTempStringValue(…) ? Config_GetTempStringValue(…) : ""`) is
  written out.
- **Update_Fade's end tests** (88.04% to 99.28%). The original tests
  `step >= 0.0f` first and puts the `< 0` case in `else`. Writing the test as
  `if (step < 0.0f)` reverses the blocks.
- **Update_Blink's second limit test** needs `(float)fabs(change) < max`, as
  the first does (93.21% to 100%).
- **Update_Dimmer.** When the light is back up, the original sets red to the
  initial colour's *blue* (`004eade8`). This is the original's behaviour and
  is kept.
- **UpdateColour** (`0044cff0`, 45.61% to 100%). The original copies the
  three channels into locals (red at `[esp+8]`) and compares each local from
  memory. With a float assignment MSVC compares the first channel against the
  global instead (84.48%), with any declaration order, an array or an inline
  helper taking `&r`. A struct copy gets the slots in the wrong order
  (45.61%). Copying each channel as a raw dword
  (`*(unsigned int*)&r = *(unsigned int*)&…currentRGB.r`) matches exactly; so
  does `memcpy`. It is marked `Deviation (scoring only)`. No storage changes.

### Callees

All but one callee already had C declarations: `gods98/config.h`, `maths.h`,
`utils.h` and `view.h` (Container_SetPosition). `lighteffects_imports.h`
declares the other two with the port's types:

- Container_GetPosition (`004754c0`);
- Container_SetColourAlpha (`004750b0`,
  `render_object_set_packed_rgba_from_float_channels`). Its `view.hpp`
  declaration gets `extern "C"` in place. Nothing else is added to a shared
  header.

The imports header is checked in `test_c_boundaries.cpp`.

## Entropy and drops

`make reccmp-check` against `main`'s baseline: 16 improved and 3 newly
mapped, **none below baseline, no exact match lost**. The plain-immediate
check, `winapi-link-check` and the link-order map check pass.

- **Outside the unit**, only `lego_engine_tick` moved (operand only, above).
  `match-listing-digests.txt` changes no listing outside the unit, so no
  instruction count changed there.
- **Inside the unit**, the instruction counts that differ from the
  original's are `0044d760` (−3) and `0044d9a0` (+1), both listed under
  [What is left](#what-is-left).

One `Deviation (scoring only)` is added (UpdateColour). It changes no storage.

## `opt_ref` (finding 579, `make opt-ref-callers`)

| | #630 `e6bf040c` | This change |
| --- | ---: | ---: |
| `opt_ref.annotated.full` (by address): removed / exact removed | 492 / 225 | 497 / 233 |
| `opt_ref.annotated.game_only` (by address) | 475 / 214 | 480 / 222 |
| By symbol (`callers --min 1`) | 480 / 214 | **479 / 215** |
| GAME-024's object | `lighteffects.cpp.obj` | `lighteffects.c.obj`: 1 removed (1 exact) |
| On `main` `12a0e950`: by address / by symbol | 498 / 229; 482 / 213 | 499 / 231; 481 / 214 |
| On `main` `2f988e92` (by address) | 571 / 280 | 564 / 276 |

#630's head was rebuilt once into a scratch build directory of this
worktree for the by-symbol comparison. On every base the removed sets
differ in the same three functions:

- **No longer removed:** the port's `0044c970`
  (`light_effect_blink_parameters_store`) and `0044d250`
  (`float_negate_absolute_value`). Both were dead in the port. Now the unit's
  Load and Update call these addresses.
- **Newly removed:** Initialise (`0044c710`), now exact. Its only original
  caller, `00433770` (`lego_scene_lights_build_from_config`), is removed. That
  caller is rooted in `lego_unit.c`, whose own conversion revives it. Before
  this change, the port's `0044c710` was kept live by `engine.cpp`, a port
  model.

So 20 of the 21 unit functions are live. Lego_MainLoop calls Update, which
reaches every update function, and the config init calls Load.

The by-address count is the one `binary-progress.json` records, and it rises
where the by-symbol count falls by one. That is finding 579's caveat about
aliases at shared addresses (as in finding 590). A Reccmp row counts as
removed when *any* dropped symbol sits at its rebuilt address. On this
change's relink, 18 exact rows count as removed by address although
`/OPT:REF` keeps their own symbols. None of them is a GAME-024 function:
`engine_memory_allocate`, `engine_memory_free`, `rng_thunk_draw15`,
`global_cursor_word_get`, `flh_magic_uu_noop`, six `legoobject_unit.c`
flag tests and setters, three `lego_unit.c` cell helpers and four others
(`ui_render_record_free`, `scene_object_wrapped_pointer_get` and two FLIC
stubs).
Which of them land on a dropped symbol varies from one relink to the next.
So the by-symbol figures are the change's effect.

## Files outside the unit

| File | Change |
| --- | --- |
| `src/game/aitask_port_aliases.h` | six matching-build `#define`s for `lego_unit.c` (above) |
| `src/game/util.cpp` | two block comments become `Port model of` comments; no code change |
| `src/gods98/view.hpp` | `extern "C"` in place on `render_object_set_packed_rgba_from_float_channels` |
| `src/game/test_c_boundaries.cpp` | includes `lighteffects.h` and `lighteffects_imports.h` |
| `src/link_order.cmake`, `src/game/meson.build` | `unit GAME-024 game/lighteffects.c`; `tail game/lighteffects_port.cpp` |
| `docs/coverage/reccmp-lint-baseline.txt` | five entries retired: the relocated file's unannotated static helpers, now in an exempt tail file |

**LegoObject.c and other units.** #626 converted LegoObject.c fully
(`legoobject_port.cpp` is a tail entry). In the original, no LegoObject.c
function calls LightEffects.c: the six direct calls to the unit all come from
Lego.c (`004212c1`, `00423688`, `00426751`, `00428b32`, `00428b5d`,
`00433ad0`). So there is no `legoobject_unit.c` caller to switch. No other C
unit uses the port's names. `gods98/viewports.cpp` (#624) is not edited.
`lego_unit.c` is not edited either; it reaches the unit through
`aitask_port_aliases.h`, which it already includes.

## Native behaviour

Unchanged. The port tail serves every port name natively, with no definition
changed. The unit compiles natively as C++ under its own names, and nothing
calls it. The aliases are matching-build only. There is no new native gap,
so no `port` issue is filed.

## Validation

On #630's head `e6bf040c` (and earlier on `main` `12a0e950` and `2f988e92`):

- `make reccmp-check`: OK (above).
- `make match-progress` (baseline, listing digests, `binary-progress.json`),
  `make unit-data-offsets`, `python3 tools/translation_units.py build` and
  `tools/binary_progress_hash.py --write-manifest` were run, then
  `make match-progress` again, so all three files carry the final input hash.
- `make reccmp-lint-update`: the ledger shrinks by five, from 1,288 to 1,283 (1,290 to 1,285 on `12a0e950`).
- `make tools-test`, without `--allow-growth`.
- `make winapi-link-check`.
- Native build and `ctest`: 37/37, synthetic fixtures.
- `gcc -std=gnu89 -Werror=declaration-after-statement -fsyntax-only` on
  `lighteffects.c`: clean.

## Follow-ups

- **Lego.c (`lego_unit.c`).** Its conversion can call the unit by name and
  drop the six aliases. Reviving `00433770` keeps Initialise live.
- **Update_Move's** two x87 spots and **CheckMoveLimit's** kept `|dx|` are
  source shapes this search did not find. A wider search should start from
  the listed forms.

## Reproducing

    git mv src/game/lighteffects.cpp src/game/lighteffects_port.cpp   # and make it a tail entry
    python3 tools/convert_unit.py GAME-024 --stem lighteffects
    make match-function SOURCE=src/game/lighteffects.c
    make reccmp-check
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object lighteffects.c.obj"
    make match-progress
    make unit-data-offsets
