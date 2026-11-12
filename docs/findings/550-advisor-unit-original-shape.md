# Finding 550 — Advisor unit (GAME-001) rebuilt in original shape

Issue #512, under [ADR 0005](../adr/0005-original-shape-c-units.md). R1
relocated GAME-001 verbatim into `src/game/advisor.cpp`. This change rebuilds
it as the C unit [`src/game/advisor.c`](../../src/game/advisor.c), with the C
boundary header [`advisor.h`](../../src/game/advisor.h). The unit is text
`00401000`–`00401cb0`, the first object in the image.

- **The advisor.** A figure that plays one of eleven LWS animations
  (`Advisor_Anim_*`) at one of 21 configured screen positions
  (`Advisor_*Point*`, `Advisor_Objective*`, `Advisor_TalkInGame`), lit by a
  light container of its own.
- **Other code in the object.** It also holds the out-of-line copies of
  gods98 Maths.h's vector helpers (`vec_*`), which other units call, and the
  Main.h and Font.h header inlines that nothing in the unit calls.

"Before" is the *match baseline* at `0fc2e60d`, the head of #515 (GAME-032
relocated, on `main` `02ee3fcf`), on which this change is stacked. "After"
is this change. `make reccmp-check` and `make match-function` give the same
per-function scores.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, all 32 functions, 3,016 bytes) | 50.66% | **94.85%** |
| Exact functions | 13 | **24 of 31 mapped** |
| Mapped functions | 31 | 31 (`00401c00` not written, below) |
| Accuracy, full (`binary-progress.json`) | 56.34% | 56.66% |
| Accuracy, game-only | 51.66% | 52.01% |
| Exact-function code bytes | 65,750 | 66,839 |
| Fully identical functions at their original address | 0 | 8 |
| `.text` bytes identical at original addresses | 18,505 | 20,748 |

Byte weights are the original's code bytes, without the `0x90` fill. The
unmapped `00401c00` (20 bytes) counts as 0% on both sides. The unit is the
image's first object, so its functions are the first to land at their
original addresses: eight of them are byte-identical in the linked binary.

| Address | Function (original name) | Bytes | before | after |
| --- | --- | ---: | ---: | ---: |
| `00401000` | `advisor_scene_controller_initialize` (Advisor_Initialise) | 437 | 75.97 | 98.48 |
| `004011c0` | `advisor_animation_end_cleanup` (Advisor_End) | 70 | 90.00 | **100.00** |
| `00401210` | `advisor_scene_visual_state_initialize` (Advisor_InitViewport) | 44 | 76.19 | 78.26 |
| `00401240` | `vec2_dist` (Maths_Vector2DDistance) | 45 | 100.00 | **100.00** |
| `00401270` | `advisor_config_slots_load_target` (Advisor_LoadAnims) | 296 | 11.16 | 95.96 |
| `004013a0` | `vec_len` (Maths_Vector3DModulus) | 55 | 100.00 | **100.00** |
| `004013e0` | `advisor_position_name_to_index` (Advisor_GetAdvisorType) | 63 | 89.66 | **100.00** |
| `00401420` | `vec_dist` (Maths_Vector3DDistance) | 69 | 100.00 | **100.00** |
| `00401470` | `vec_cross` (Maths_Vector3DCrossProduct) | 65 | 100.00 | **100.00** (asm, below) |
| `004014c0` | `advisor_animation_name_to_index` (Advisor_GetAnimType) | 63 | 89.66 | **100.00** |
| `00401500` | `vec_add` (Maths_Vector3DAdd) | 37 | 100.00 | **100.00** |
| `00401530` | `advisor_position_config_load` (Advisor_LoadPositions) | 398 | 29.33 | 97.69 |
| `004016c0` | `vec_sub` (Maths_Vector3DSubtract) | 37 | 100.00 | **100.00** |
| `004016f0` | `vec_dot` (Maths_Vector3DDotProduct) | 39 | 100.00 | **100.00** |
| `00401720` | `vec_scale` (Maths_Vector3DScale) | 37 | 100.00 | **100.00** |
| `00401750` | `vec_normalize` (Maths_Vector3DNormalise) | 91 | 95.52 | **100.00** |
| `004017b0` | `advisor_position_record_initialize` (Advisor_InitPosition) | 131 | 17.39 | **100.00** |
| `00401840` | `advisor_action_position_and_mode_set` (Advisor_SetParameters) | 77 | 26.32 | **100.00** |
| `00401890` | `advisor_action_position_get` (Advisor_GetOrigPos) | 35 | 32.56 | **100.00** |
| `004018c0` | `advisor_request_admit` (Advisor_Start) | 107 | 13.79 | **100.00** |
| `00401930` | `advisor_request_reset_and_submit_sound` (Advisor_PlayAnim) | 88 | 81.58 | **100.00** |
| `00401990` | `advisor_transition_worker` (Advisor_Update) | 212 | 37.18 | 86.36 |
| `00401a70` | `advisor_scene_advance_completion_check` (Advisor_MoveAnimation) | 174 | 28.82 | **100.00** |
| `00401b20` | `advisor_clear_active_state` (Advisor_EndLoop) | 13 | 100.00 | **100.00** |
| `00401b30` | `advisor_apply_slot_transform` (Advisor_PositionUpdate) | 190 | 16.67 | **100.00** |
| `00401bf0` | `engine_app_mode_get` (Main.h inline) | 6 | 100.00 | **100.00** |
| `00401c00` | Main_GetStartLevel (Main.h inline) | 20 | unmapped | unmapped |
| `00401c20` | `global_flag_bit0_is_set` (Advisor.h inline) | 9 | 100.00 | **100.00** |
| `00401c30` | `display_width_get` (Main.h inline) | 6 | 100.00 | **100.00** |
| `00401c40` | `display_height_get` (Main.h inline) | 6 | 100.00 | **100.00** |
| `00401c50` | `formatted_text_printf_width` (Font.h inline) | 48 | 23.08 | 29.17 |
| `00401c80` | `formatted_text_printf` (Font.h inline) | 48 | 26.92 | 26.92 |

Eleven functions are newly exact. None falls. The exported functions keep
the port's names. The original names in the table are descriptive and do not
appear in the source.

## The unit

### State

- **`.bss`**: the 1,040 bytes at `004b2db0` are one static `AdvisorGlobs`:
  - the 21 position names;
  - 21 `AdvisorPositionData` records of 0x24 bytes (animation, message and
    sound indices, panel type, current and configured positions, flags);
  - the 11 animation names;
  - 11 `AdvisorAnimData` (container, loop start, loop end);
  - the current position, the camera container, the viewport, the depth
    scalar, the light container and the flags word `004b31bc`.

  The port had split this into three owners. `objective.cpp` had the
  action records, the animation slots and one copy of `004b31bc`.
  `crt.cpp` had another copy of `004b31bc`, and `advisor.cpp` had the name
  tables. The two copies of the flags word are one field now.
- **`.rdata`**: `0049e004` (25.0, frames per second) is an annotated
  static. MSVC multiplies the `double` from `atof` by it as a `float`. The
  literal `25.0f` compiles to a `qword` constant instead.
  - The 1.0 at `0049e008` (`vec_normalize`) and the 0.0 at `0049e00c`
    (`advisor_scene_advance_completion_check`) stay literals.
  - MSVC pools them in the same order. Annotated statics change no
    instruction, but they add two declarations and move `vec_dot` out of its
    phase (rule B, below).
- **Strings**: the 32 advisor names, `"Advisor"`, `"!%s"`, `"LWS"` and
  `"AdvisorPositions%ix%i"` are literals with `STRING` markers. MSVC emits
  each function's literals in reverse order of use, which is the original
  layout.
  - `","` (`004a13e4`) and `"NULL"` (`004a13f4`) carry no marker. The
    original's other units reuse them: `","` from 34 sites, `"NULL"` from
    five.
  - A marker names the operand on the original side in every one of them.
    That moved two `frontend.cpp` functions by about one point each:
    `0x0041afd0` 12.53 → 11.49 and `0x004345f0` 38.46 → 37.50. The cost
    inside the unit is the same label: `0x00401530` would be 100%, and
    `0x00401270` 96.97%.
  - `"LWS"` and `"!%s"` are reused too, but their markers move nothing
    down.

`make unit-data-offsets`, GAME-001 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-001 | gaps-only (36, 0 unknown) | n/a (1, 0 unknown) fewer than two attributable items | n/a (1, 0 unknown) fewer than two attributable items | - |

The 36 `.data` items are the string literals, all at their original gaps.
`.rdata` and `.bss` have one item each: the 25.0 and the globals struct.

### The out-of-line Maths.h helpers

The original expands Maths.h's modulus and scale inline in
`vec_normalize`. Its out-of-line copies of the other helpers sit between the
advisor's functions, and the linker kept Advisor.obj's copies because it is
the first object.

- **Marking them `__inline` does not work.** With `vec_len` and `vec_scale`
  marked `__inline`, MSVC 5.0 inlines them into `vec_normalize`, which
  becomes exact, but it then emits no out-of-line copy at all.
- **The arrangement used.** The exported helpers are ordinary functions. The
  inline copies are `static __inline` functions in
  [`advisor_maths.h`](../../src/game/advisor_maths.h), which `advisor.c`
  includes just before `vec_normalize`. In a header they add no lint debt,
  as for `steering_maths.h` (finding 544).
- **Placement costs nothing.** Moving the two static inline helpers anywhere
  in the file changed no score, so they cost no counter.

Source shapes that matter:

- **`dst` in `eax`.** `vec_add`, `vec_sub`, `vec_cross` and `vec_scale`
  return their destination, like Maths.h's `LPVector3F` helpers. With `void`
  returns, MSVC rotates the three pointer registers, and no counter phase
  restores them. The C++ declarations in `view.hpp` change to `float*` in
  place.
- **Repeated subexpressions.** `vec2_dist` and `vec_dist` are exact with
  each difference written twice, as in
  `sqrt((a[0] - b[0]) * (a[0] - b[0]) + ...)`. With locals, the best of 18
  orders scores 54%.
- **`vec_cross` stays asm.** The C body compiles to the same operations, but
  with the operands of the last two products loaded the other way round
  (68%). Nothing tried changes that:
  - all operand orders;
  - named `x`/`y`/`z` members through a `Vector3F` cast (rule A);
  - a temporary vector;
  - counter phases 0–516;
  - the permuter (0 applicable rewrites).

  Under MSVC the unit therefore keeps the port's `__declspec(naked)` body
  for this one function. Other compilers use the C body.

### Rule B in the unit (finding 538)

`vec_dot` and `vec_add` depend only on the counter phase:

- `vec_dot` has period 16 and is exact at one phase only. `vec_add` has
  period 4 and is exact at two phases.
- Both are exact in the final layout. Two legitimate edits put them there:
  writing `vec2_dist` without locals, and the x/y pair as `float pos[2]` in
  `advisor_position_config_load` (that change alone took it from 92.31% to
  100%).
- Later edits above `004016f0` should re-check `vec_dot`. It is about two
  declarations from the edge.

### Near misses

| Address | Function | Score | What's left |
| --- | --- | ---: | --- |
| `00401000` | Advisor_Initialise | 98.48 | the call to `004734f0` names `ddiscene_light_create`; the port annotates that address on a ledger stub (G98-002's to fix) |
| `00401530` | Advisor_LoadPositions | 97.69 | the unmarked pooled `","` and `"NULL"` operands (above) |
| `00401270` | Advisor_LoadAnims | 95.96 | the `","` operand, and the call to the `container_load` adapter instead of `00472cd0` (below) |
| `00401990` | Advisor_Update | 86.36 | `ebx`/`edi` swap the position index and the container; five declaration forms, phases 0–15 and the permuter change nothing |
| `00401210` | Advisor_InitViewport | 78.26 | the original stores the front clip to a dead local. Only `volatile` keeps the store, which is not a plausible original, so it is not used; the permuter finds nothing |
| `00401c50` | Font_GetStringWidth | 29.17 | the port's text contract (below) |
| `00401c80` | Font_PrintF | 26.92 | the port's text contract |

### Signatures and contracts

The original signatures, recovered from the disassembly and from the
caller, GameConfigInit (`0x0041fb50`): `FUN_00401000(gameName, cameraMain, viewMain)`,
`FUN_00401210(200.0f)`, `FUN_00401270(config, gameName)`:

- `advisor_scene_controller_initialize(gameName, camera, viewMain)` reads the
  camera's container at `+0x24`. `camera` is a `void*` read through a
  local `AdvisorCamera` prefix, so the shared header gains no type.
- `advisor_scene_visual_state_initialize(float)` takes an argument it does
  not use. It reads the viewport's back and front clip and stores 0.96.
- `advisor_config_slots_load_target(config, gameName)` and
  `advisor_position_config_load(config, gameName, width, height)` take the
  config and the game name. The original's `!%s` fallback formats the
  name and then looks up the unformatted one again, and that is kept.
- `advisor_position_record_initialize`, `advisor_action_position_and_mode_set`
  and `advisor_action_position_get` take float positions, not dwords.
  `advisor_action_position_get` reads the configured position
  (`+0x18`/`+0x1c`), which `Advisor_SetParameters` does not change.
- `advisor_request_admit` and `advisor_transition_worker` read the common
  `00556ec8` (`s_context_mode`) directly. It is now an external C variable
  in `engine.cpp`, declared in [`gods98/engine.h`](../../src/gods98/engine.h).
- No range checks or NULL guards remain.

Callee contracts that differ from the original are used as they are:

- **`00472cd0`, Container_Load.** The port's function at this address is
  `entity_assembly_parse(BlockFile*, root, feature)`, a different contract.
  The unit calls an unannotated adapter with the original signature,
  `container_load` (ADR 0005 step 6), in `gods98/entity.cpp` and declared
  in [`gods98/entity.h`](../../src/gods98/entity.h). The port has no
  container loader, so it returns NULL.
- **`004734f0`, `00475470`, `004734c0`.** These take `uint32_t` colour bits,
  float orientation components and a `void**` container. The arguments
  compile to the original pushes.
- **`0045a700`, Panel_GetPosition.** It writes an integer pair, which holds
  the float bits.
- **`00476fd0` and `00476ff0`, the viewport clip getters.** They now return
  `float`. The naked bodies leave the value in `st(0)` as before. The
  native fallback, in `viewports.cpp` since #516, returns 0.
- **`0047a150`, Font_VPrintF.** The port's renderer has an 11-argument
  software-target contract. The two Font.h inlines keep the port's
  signatures, which its callers in `frontend.cpp`, `mission.cpp` and
  `frameloop.cpp` use. Their scores rise or stay.

### `00401c00` is not written

Main_GetStartLevel returns `0x00505f8c`, Main.c's start-level name, when
flag `0x200` is set. The port keeps that buffer only as a static in the
Win32 entry point (`platform/winapi/entry.cpp`). The native build has none.
It stays unmapped until Main.c's globals are converted.

### Static and external functions

Nothing in the unit is `static`, so MSVC emits all 31 functions in source
order (finding 544). The only file-scope static is the globals struct, which
must not be a common: an external uninitialised definition in C is a
tentative definition, which the linker puts in the common region.

## Callers and tests

- `frameloop.cpp` passes the config's root name to
  `advisor_position_config_load`.
- `objective.cpp`'s `0x0041c4f0` passes floats to the position get and set.
  The `memcpy` round trips go, 254 → 250 instructions, and the score rises
  13.10% → 13.23%.
- **Removed port code.**
  - From `objective.cpp`: the port's scene, animation, transform and config
    hooks, their setters, `advisor_animation_slot_install`,
    `advisor_scene_scale_get`, the two config adapters and the duplicate
    flags word.
  - From `crt.cpp`: the flags-word accessors.
  - `advisor_state_word_peek`/`_set` and `advisor_action_flags_peek` are
    now port-only test accessors at the end of `advisor.c`, compiled out of
    the matching build (ADR 0006).
- **Native behaviour.** Nothing in the port calls
  `advisor_scene_controller_initialize`, which needs a camera and a viewport
  the native renderer does not have. The name tables therefore stay empty,
  `advisor_position_config_load` loads no position, and
  `advisor_request_admit` returns 0.
  - Before, the loader filled the name tables itself. The first admit then
    set the animating bit, and with no `Advisor_Update` running nothing
    cleared it. That blocked `0x0041c4f0`'s later requests.
  - The advisor was never visible natively.
- `test_objective.cpp` tests the float positions, the configured-position
  read and `Advisor_InitPosition`'s flags. `test_crt.cpp` uses the moved
  accessors unchanged.

## Entropy

All edits to shared C++ headers keep every includer's symbol count. The
check uses a probe: a `static` after a unit's includes, whose `$S` number is
the counter (finding 538's kit, `tools/experiments/486`).

- **`extern "C"` in place costs nothing.** With every other header edit
  applied, `frontend.cpp`'s probe reads 49,903 with `main`'s headers and
  with this change's.
- **`objective.hpp`.** `advisor_position_config_load` gains a parameter (+1).
  An unused enumerator of the old scene-action enum offsets it (−1).
  `lava_erosion.cpp`'s probe reads 49,661 on both sides.
  - A function declaration costs at least 2: 2 with no or one parameter, 3
    with two. So no deletion of a whole declaration can offset +1.
  - Deleting the superseded setters instead had dropped `0x0040e9c0`
    (100 → 84.21) and `0x0045e940` (22.05 → 17.05).
  - Keeping them all and not compensating had dropped `0x004089d0`
    (43.56 → 40.71) and `0x0044fb90` (23.65 → 23.19).

  The superseded declarations and hook types stay, uncalled and commented
  (rule 1). Their definitions are gone. Parameter names cost nothing, as
  finding 538 says.
- **New C boundary headers.** `gods98/view.h`, `scene_resources.h`,
  `message.h`, `engine.h`, `entity.h` and `font.h` are included only by
  `advisor.c` and the boundary test. `panel.h`, `sound.h` and `crt/crt.h`
  gain declarations; their C includers `priorities.c` and `draw.c` keep
  every score.

`make reccmp-check` against `0fc2e60d` reports:

- **no function below baseline**;
- 19 improved, all in the unit except the three below;
- `link order check-map: OK`.

Outside the unit:

| Address | Function | File | Score | Instructions | Change |
| --- | --- | --- | --- | --- | --- |
| `0x0041c4f0` | `advisor_animation_position_and_request_update_internal` | `objective.cpp` | 13.10 → 13.23 | 254 → 250 | caller migrated (source changed) |
| `0x0040c0d0` | `scene_act_slot_load_and_register` | `rock_fall_styles.cpp` | 17.59 → 18.52 | = | the `"LWS"` operand is now named on both sides |
| `0x00476fd0`, `0x00476ff0` | `resource_interface_call_slot8c`/`90` | `viewports.cpp` | 100 = | = | names lose `lego::` (C linkage) |

**For the owner:** there are no drops. The only instruction-count change
outside the unit is `0x0041c4f0`, whose source this change edits.

## Tooling notes

- **`convert_unit.py` still refuses a relocated unit** (finding 544). The
  `.cpp` entry was removed from the link-order list by hand, and the tool
  re-inserted `game/advisor.c` at the same position. #514 addresses this.
- **The scaffold's stubs repeat the old annotations.** Its report listed
  `00401c00` (no port function) and the `.rdata` items. Its proposed
  `STRING` markers include the two pooled literals, which needed the review
  above.
- **The annotation lint ratchet** loses 14 entries: the hook setters,
  config adapters, `obj_strcasecmp`, the token splitter, the name-table
  initialiser and the two `vec_normalize` fallbacks. It gains one: the
  `container_load` adapter. ADR 0005 step 6 requires an unannotated
  adapter, and the ratchet has no exclusion for one, so the entry was
  accepted with `--allow-growth`.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox) passes against
  `0fc2e60d`: 19 improved, none below baseline, link-order map OK.
- `make match-progress` regenerated the baseline, the listing digests and
  `binary-progress.json`. `pr_match_progress.py check --base
  origin/relocate/021-032-split --head .` reports them fresh.
- `make unit-data-offsets` gives the GAME-001 row above.
- `python3 tools/translation_units.py build` reports 0 conflicts.
  `tools/binary_progress_inputs.txt` was regenerated.
- `make tools-test` passes, including Meson parity and the lint ratchet
  (above).
- `advisor.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only`. Its one warning is InitViewport's set-but-unused front clip.
- `cmake -S . -B build && cmake --build build && ctest --test-dir build
  --output-on-failure`: 36/36 pass, including `c_boundaries`.

## Reproducing

    python3 tools/convert_unit.py GAME-001 --stem advisor   # after removing the .cpp entry
    make match-function SOURCE=src/game/advisor.c
    make reccmp-check
    make match-progress
    make unit-data-offsets
