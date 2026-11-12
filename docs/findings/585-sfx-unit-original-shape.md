# Finding 585 — SFX.c (GAME-041) rebuilt in original shape, with a port tail

Issue #605, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-041 is the original's `SFX.c` (text `00464740`-`004661c0`): the sound
effect ids and their hash list, each id's chain of Sound3D samples, the random
pick among them, the global sample, the play queue and the queue mode. Its
second half (`00465380`-`004661c0`) runs the smoke emitters. It was relocated
verbatim into `src/game/sfx.cpp` (#496). This change rebuilds it as the C unit
[`src/game/sfx.c`](../../src/game/sfx.c), with the unit-private imports header
[`sfx_imports.h`](../../src/game/sfx_imports.h), the inline vector helpers
[`sfx_maths.h`](../../src/game/sfx_maths.h) and the matching-build link names
[`sfx_link_names.h`](../../src/game/sfx_link_names.h). The port's
implementation moves, unannotated, to the port tail
[`sfx_port.cpp`](../../src/game/sfx_port.cpp).

This change is stacked (stack #613) on #609 (GAME-022, the camera unit,
`7181f2fc`), which sits on #610 (LegoObject.c part 2). Whole-binary "before"
figures are #609's head; "after" is this change rebased onto it, from the
full matching build (`make reccmp-check`). No GAME-041 score differs between
`main` `05cd8938`, where the change was first made and measured, `31da2ab6`
(#608) and #609's head, so the per-function "before" column is unchanged.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, all 37 functions, 6,500 original code bytes) | 29.03% | **91.20%** |
| Exact functions in the unit | 10 | **30 of 37** |
| Mapped functions in the unit | 34 | 37 |
| Exact-function code bytes (`binary-progress.json`) | 143,569 (22.34%) | **146,620** (22.82%) |
| Accuracy, full / game-only | 76.63% / 74.30% | 77.18% / 74.91% |
| `opt_ref` annotated (full): removed / exact removed | 596 / 276 | **589 / 276** |
| SFX.c functions removed under `/OPT:REF` | 2 (1 exact) | **0** |
| Reccmp lint ledger | 1,333 | 1,322 |

Byte weights are the single-function loop's original sizes; an unmapped
function counts as 0%. Three functions are newly mapped. `00465250` is in the
unit map but had no definition: SFX_Update's call to `Sound3D_Update`, a
one-instruction `jmp`. `00464c20` (the container sound-trigger callback that
SFX_Initialise registers) and `00465ee0` (the smoke mesh's render callback that
Smoke_CreateSmokeArea registers) are reached only through function pointers, so
Ghidra folded them into their neighbours and the unit map does not list them.

The names are OpenLRR's, in the repository's snake case.

| Address | Function | Port name before | Bytes | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `00464740` | `sfx_initialise` | `sound_alias_registry_init` | 1242 | 8.70 | **100.00** |
| `00464c20` | `sfx_container_sound_trigger_callback` | `-` | 47 | unmapped | **100.00** |
| `00464c50` | `sfx_set_sample_populate_mode` | `sound_alias_registration_set` | 29 | 100.00 | **100.00** |
| `00464c70` | `sfx_get_type` | `sound_alias_lookup` | 131 | 28.35 | 84.00 |
| `00464d00` | `sfx_load_sample_property` | `sound_sample_marker_parse` | 275 | 25.09 | **100.00** |
| `00464e20` | `sfx_random_get_sound3d_handle` | `sound_variant_select` | 94 | 9.41 | **100.00** |
| `00464e80` | `sfx_stop_global_sample` | `sound_deferred_play_expire` | 60 | 92.86 | **100.00** |
| `00464ec0` | `sfx_set_global_sample_duration_if_le0` | `sound_deferred_timer_set_if_idle` | 47 | 100.00 | **100.00** |
| `00464ef0` | `sfx_random_set_and_play_global_sample` | `sound_submit_with_global_cooldown` | 99 | 45.71 | **100.00** |
| `00464f60` | `sfx_add_to_queue` | `sound_request_queue_submit` | 50 | 44.44 | **100.00** |
| `00464fa0` | `sfx_random_play_sound_normal` | `sound_alias_play` | 102 | 40.00 | **100.00** |
| `00465010` | `sfx_random_set_buffer_volume` | `sound_variant_property_set` | 28 | 100.00 | **100.00** |
| `00465030` | `sfx_random_get_buffer_volume` | `sound_variant_property_read` | 23 | 100.00 | **100.00** |
| `00465050` | `sfx_random_play_sound3d_on_container` | `sound_submission_route_wrapper` | 51 | 64.86 | **100.00** |
| `00465090` | `sfx_random_play_sound3d_on_frame_internal` | `sound_positional_sound_submit` | 205 | 41.10 | **100.00** |
| `00465160` | `sfx_random_get_sample_play_time` | `sound_object_duration_get` | 34 | 100.00 | **100.00** |
| `00465190` | `sfx_sound3d_stop_sound` | `sound_handle_stop_forward` | 14 | 100.00 | **100.00** |
| `004651a0` | `sfx_update` | `sound_subsystem_frame_tick` | 163 | 58.95 | **100.00** |
| `00465250` | `sfx_sound3d_update` | `-` | 5 | unmapped | **100.00** |
| `00465260` | `sfx_set_sound_on` | `sound_subsystem_enable_set` | 60 | 81.08 | **100.00** |
| `004652a0` | `sfx_set_sound_on_and_stop_all` | `sound_subsystem_enable_set_and_stop` | 16 | 100.00 | **100.00** |
| `004652b0` | `sfx_is_queue_mode` | `sound_batch_mode_get` | 9 | 100.00 | **100.00** |
| `004652c0` | `sfx_set_queue_mode_and_flush` | `sound_deferred_queue_drain` | 16 | 100.00 | **100.00** |
| `004652d0` | `sfx_set_queue_mode` | `sound_batch_queue_drain` | 153 | 53.21 | **100.00** |
| `00465370` | `sfx_is_sound_on` | `sound_subsystem_enabled` | 9 | 100.00 | **100.00** |
| `00465380` | `smoke_initialise` | `scene_effect_registry_reset` | 23 | 50.00 | **100.00** |
| `004653a0` | `smoke_load_textures` | `transient_effect_textures_preload` | 136 | 23.53 | 97.78 |
| `00465430` | `smoke_create_smoke_area` | `transient_effect_emitter_create` | 1342 | 23.50 | 94.12 |
| `00465970` | `smoke_remove_all` | `transient_effect_registry_release_all` | 50 | 30.43 | **100.00** |
| `004659b0` | `smoke_remove` | `transient_effect_instance_release` | 213 | 13.08 | 96.72 |
| `00465a90` | `smoke_hide_all` | `transient_effect_registry_enable_set` | 39 | 34.04 | **100.00** |
| `00465ac0` | `smoke_hide` | `transient_effect_enable_set` | 58 | 74.07 | **100.00** |
| `00465b00` | `smoke_update` | `transient_effect_instances_tick` | 322 | 62.93 | **100.00** |
| `00465c50` | `smoke_group_show` | `transient_effect_particle_slot_initialize` | 423 | 18.18 | 96.33 |
| `00465e00` | `smoke_group_update` | `transient_effect_segment_advance` | 209 | 27.91 | 87.67 |
| `00465ee0` | `smoke_mesh_render_callback` | `-` | 93 | unmapped | **100.00** |
| `00465f40` | `smoke_group_set_vertices` | `transient_effect_instance_quad_vertices_build` | 630 | 21.46 | 33.15 |

### What is left

| Function | Score | Remaining difference and what was tried |
| --- | ---: | --- |
| `00464c70` | 84.00 | register choice only: the original keeps the loop bound in `esi` from `edx`, the counter in `ecx`; ours loads the count into `ecx`. A `count` local (as the original's `lea esi, [edx + 0x2d]`) took it from 74.00; declaration order, `unsigned` hash and operand order score the same or lower; permuter (240 s): 4 candidates, none better |
| `004653a0` | 97.78 | operand only: `00482e70` (Mesh_LoadTexture) has no annotated rebuild entry; the unit calls the `PORT-ONLY` adapter `sfx_mesh_load_texture_adapter` |
| `00465430` | 94.12 | the `(seed & 3) == 0` test compiles to `test byte ptr [mem], 3` against the original's `mov edx, [mem]; and edx, 3; test dl, dl` (char casts, `%`, `!` all tried); one x87 operand choice in the inline normalise (`x` from memory against the stack); the Container_Create and Mesh_SetGroupTexture call operands (adapters). Component copies of `dir` (not a struct copy) and the loop counter initialised at its declaration took it from 70.29 |
| `004659b0` | 96.72 | operand only: both `004820d0` (Mesh_Remove) calls go to `lws_port.cpp`'s unannotated `mesh_remove` |
| `00465c50` | 96.33 | where `smoke->flags = 4` and the velocity copy are scheduled. `(-smoke->speed) / vec.z` (parenthesised; without the parentheses MSVC divides first and negates) took it from 38.36 |
| `00465e00` | 87.67 | x87 scheduling of `group->time += elapsed` against the alpha product. A `move` vector, `sfx_vector3d_scale` then `_add` and an `alpha` local (stored in `elapsed`'s home, as the original) took it from 38.85; permuter (240 s) found no candidates |
| `00465f40` | 33.15 | the frame is 0x88 bytes against 0x84: MSVC gives the first inline scale's factor its own slot where the original packs it into `scale`'s, which shifts every stack offset; the scheduler also pushes the call's arguments before the vertex arithmetic. The `up` vector initialised with constants before the inline cross product (the original folds the products by 0) and a separate camera statement took it from 22.22; scale variable and declaration orders, explicit width and height locals and inline arguments all score 33.15; permuter (240 s) found no candidates |

No unit function uses the flag `0x800000`, and #608's plain-immediate check
passes.

## How the conversion is done

### The port tail

The port's sound and smoke code is another contract: its own alias hashing
(`toupper` and a running factor, not `Util_HashString`), a 16,384-node chain
pool with NULL and bound guards, a 64-bit link table beside each smoke
emitter, and renderer hooks for the smoke mesh. `test_sound.cpp` and the
native sound bank depend on it. As in findings 570 and 582 it stays a port:
`sfx.cpp` is renamed `sfx_port.cpp`, every `// FUNCTION` and `// GLOBAL`
becomes a `/* Port model of 0x... */` comment, and the file is a `tail`
entry in the link-order list.

### Link names, not caller aliases

All 34 port entries with an original counterpart take the original's
arguments at every caller: SFX ids, sound handles, flags, the container or
frame a sound plays on, and smoke emitters, whose only creator is the same
unit. The sample handles are Sound3D.c's (`sound3d.c`), which the port's
sound bank already is, so the port and the original agree on them. So, as
finding 581 did for Stats.c, `sfx_link_names.h` renames the unit's own
definitions to the port's names under `#if defined(_MSC_VER) && _MSC_VER <
1200`: `sfx_random_play_sound_normal` links as `sound_alias_play`, and so on
for 34 names. Only `sfx.c` includes it, first. `sfx_port.cpp` compiles its
definitions of those names, and the static state only they use, out of the
matching build; the globals `sound.cpp` reads (`s_batch_entry`,
`s_cooldown_timer`, …) stay defined in every build. As a result:

- every caller reaches the unit's functions in the matching build without an
  edit or an include, C units (`lego_unit.c`, `legoobject_unit.c`,
  `interface_unit.c`, `info_unit.c`, `reward.c`, `advisor.c`, `priorities.c`,
  `entity_stats.c`, `weapons.c`) and C++ files (`frontend_unit.cpp`,
  `objective_unit.cpp`, `message_unit.cpp`, `tutorial_script.cpp`,
  `entity.cpp`, `legoobject.cpp`, `effects.cpp`, `simtick.cpp`, …) alike;
- no caller's call operand changed, so no caller dropped;
- natively the port tail still serves every caller under the same names.

A macro renames a definition one for one, so the symbol counter does not
move (finding 538). Names differ in two signatures only, both
ABI-compatible: `00465160` returns `float` (the port `double`; both return
on the x87 stack) and `00465030` returns the volume (the port `void`).

Four port declarations in `sound.hpp` had C++ linkage
(`sound_deferred_play_expire`, `sound_deferred_timer_set_if_idle`,
`sound_variant_property_read`, `sound_deferred_queue_drain`) and get
`extern "C"` in place, so `tutorial_script.cpp` and `objective.cpp` reach the
unit through the header they already include.

`lego_unit.c`'s engine tick (`004232e0`) called the `PORT-ONLY` adapter
`sound_variant_property_read_original` for `00465030`. A matching-build
`#define` in `aitask_port_aliases.h`, which `lego_unit.c` already includes,
points it at the unit; the adapter is compiled out of the matching build.
`004232e0` rose from 96.45% to 96.52% (96.80% to 96.88% on #609's head). No new `#include` is added to any
caller (finding 582).

### State

SFX.c's `.bss` is one `static SFX_Globs sfxGlobs` at `00501468`, 0x1770
bytes, laid out as the original: 495 chain heads (`00501468`), 200 chain links
(`005023e0`), the hash list pointer, the added-sample count, the link count and
the flags (`00502a20`-`00502a2c`), ten queued instances of 0x18 bytes
(`00502a30`), their count, the global sample's duration, handle and type
(`00502b20`-`00502b2c`), and the two play queues of ten (`00502b30`-`00502bd4`).
The queued instance's flags are three signed one-bit fields (sound 3D, loop,
on container), as the original's shifts show (`shl 0x1e; sar 0x1f`).

Smoke's state is four commons: the texture table (`00552100`, ten entries),
the emitter list head and tail (`00552128`, `0055212c`) and the texture count
(`00552130`). `effects.cpp`'s port globals lose their `0x00552100` and
`0x00552130` annotations. The smoke emitter is the original's 0x2a8-byte
`Smoke` (ten 0x3c-byte particle groups, then the delay, position, direction,
colour, counts, speed, container, mesh, flags, list links, SFX id and sound
handle).

The float constants are literals. The original's `.rdata` has two `0.0f`
constants, `0049e73c` (used by SFX's functions) and `0049e750` (used by the
smoke functions); one translation unit pools them into one. With the smoke
half having only commons, this suggests the smoke functions are the
original's `Smoke.c`, a unit of its own after SFX.c. The unit map keeps
them in GAME-041; splitting them is a follow-up.

The 45 `SFX_*` names are marked `// STRING` where they are used.
`"%s\\"` (`004a1d88`) and `"%s%i.bmp"` (`004a1d7c`), pooled with GAME-009, had
no marker and are marked here; `","` (`004a13e4`) stays `advisor.c`'s.

`make unit-data-offsets`, GAME-041 row: `.data` n/a (44 strings, 1 unknown),
`.rdata` n/a (literals), `.bss` n/a (one item); commons `00552100`
`_smokeTextures` (40), `00552128` `_smokeListHead`, `0055212c`
`_smokeListTail`, `00552130` `_smokeTextureCount`.

### Callees

- **`extern "C"` in place** on six declarations: `scene_sample_hook_register`
  (`crt.hpp`), `scene_object_wrapped_pointer_get`,
  `indexed_record_flag6c_bit0_set`, `record_flag2c_bit0_set`,
  `instance_quad_vertex_data_fill` (`record.hpp`) and
  `scene_object_query_value_if_ready` (`view.hpp`). No declaration is added.
- **Adapters.** `00472950` (Container_Create) has C++ linkage in namespace
  `lego`; the unit calls `lego_port.cpp`'s existing
  `lego_container_create_adapter`. Two new `PORT-ONLY` adapters at the end of
  `sfx_port.cpp`: `sfx_mesh_load_texture_adapter` for `00482e70`
  (Mesh_LoadTexture; the port's `texture_get_or_load` takes two arguments and
  is unannotated) and `sfx_mesh_set_group_texture_adapter` for `004830c0`
  (namespace `lego`). `lws_port.cpp`'s `mesh_remove` and
  `mesh_group_material_values_set` stand in for `004820d0` and `00483420`.
  These are the remaining operand differences.
- `00431570` (Lego_GetMap) is called through an unprototyped type in C and
  through `level_current_terrain_get_original` in C++, as `weapons_imports.h`
  does.

### Original behaviour kept

- `SFX_LoadSampleProperty` copies a `#volume#` marker into a buffer it never
  terminates before `atoi`.
- `SFX_Random_PlaySoundNormal` returns a handle only when it plays; the
  queued and silent paths set no return value.
- `Smoke_Group_Show` sets the emitter's flags to 4, dropping the one-shot and
  ending bits it has just set; `Smoke_Remove` frees the last emitter without
  stopping its sound.
- `Smoke_CreateSmokeArea` uses 10 groups only when `density <= 0 && density
  >= 1`, which never holds, so it always takes `density * 10`.

## Entropy and drops

`make reccmp-check` against #609's baseline (`7181f2fc`): 25 improved and 3
newly mapped, **none below baseline**, no exact match lost, so no instruction count
changed in a dropped function. The one function outside the unit that moved
is `004232e0` `lego_engine_tick` (96.80% to 96.88%, the `00465030` alias).
The header edits are `extern "C"` in place and one `#define` in an alias header
a caller already includes. No `Deviation (scoring only)` is added.

## `opt_ref` (finding 579, `make opt-ref-callers`)

| | #609 `7181f2fc` | This change |
| --- | ---: | ---: |
| `opt_ref.annotated.full`: removed / exact removed | 596 / 276 | **589 / 276** |
| `opt_ref.annotated.game_only`: removed / exact removed | 579 / 265 | 572 / 265 |
| GAME-041's object | `sfx.cpp.obj` | `sfx.c.obj`: **0 removed** |

Measured first on `main` `05cd8938`, the change took `opt_ref` from 762/346
to 752/343, with `sfx.cpp.obj`'s 2 removed functions (1 exact) going to 0;
the analysis below is from that measurement.

All 37 unit functions are live under `/OPT:REF`, and so are eight callees the
unit now reaches through the original's calls (by symbol, against `main`):
`004295a0` (exact), `00472920` (exact, Container_SetSoundTriggerCallback),
`00476f70` (Viewport_GetCamera), `00477670` (Util_HashString), `00480870`
(Mesh_CreateOnFrame) and its two record helpers `004807a0`, `004807f0`, and
`00482500` (Mesh_SetVertices). Nothing became removed. Three of the ten
newly live functions are exact (`004295a0`, `00465030`, `00472920`), so the
exact-removed count falls by three.

## Files outside the unit

| File | Change |
| --- | --- |
| `src/game/aitask_port_aliases.h` | matching-build `#define` of `sound_variant_property_read_original` (for `lego_unit.c`) |
| `src/game/effects.cpp` | two `// GLOBAL` annotations (`00552100`, `00552130`) become comments |
| `src/gods98/sound.hpp` | `extern "C"` on four declarations |
| `src/crt/crt.hpp`, `src/gods98/record.hpp`, `src/gods98/view.hpp` | `extern "C"` on six callee declarations |
| `src/game/test_c_boundaries.cpp` | includes `sfx.h` and `sfx_imports.h` |
| `src/link_order.cmake`, `src/game/meson.build` | `unit GAME-041 game/sfx.c`; `tail game/sfx_port.cpp` |

**Files other threads own.** None is edited: `legoobject_unit.c`,
`legoobject.cpp`, `legoobject_port_aliases.h` (#588), `entity.cpp` (#604),
`tutorial_script.cpp` (#606) and `camera_control.cpp` (#607). Their callers
reach the unit through the link names alone: `legoobject_unit.c`
(`00465190`, `00465260`, `00465370`), `legoobject.cpp` (`00464c70`),
`entity.cpp` (`00464c70`, `00465050`, `00465160`) and `tutorial_script.cpp`
(`00464e80`, `00464ec0`, through `sound.hpp`'s new `extern "C"`).
`camera_control.cpp` calls none. No caller is left for later.

**On the stack (#610, #609).** #610 converted LegoObject.c part 2 into
`legoobject_unit.c`, which now also calls `00464c70`, `00464fa0`,
`00465050` and `00465160` besides `00465190`, `00465260` and `00465370`.
All of them call the port's names, which the link names give the unit, so
they already reach SFX.c in the matching build and nothing needs switching:
no edit, no new include (rule B), and natively they keep the port tail. No
alias header on the stack defines any of the 34 names. #609's
`camera_control.c` calls no SFX.c function.

## Native behaviour

Unchanged. The native port keeps the port tail under the same names;
`sfx.c` compiles natively as C++ under its own names, which nothing calls. No
new native gap.

## Validation

On #609's head (`7181f2fc`), after the rebase:

- `make reccmp-check`: OK, none below #609's baseline, no exact match lost; the plain-immediate check
  (finding 584), `winapi-link-check` and the link-order map check OK.
- `make match-progress` (baseline, listing digests, `binary-progress.json`),
  `make unit-data-offsets`, `python3 tools/translation_units.py build`,
  `make reccmp-lint-update` (ledger 1,366 to 1,355), and
  `tools/binary_progress_hash.py --write-manifest`, then `make match-progress`
  again so all three files carry the final input hash;
  `tools/pr_match_progress.py check` against `main`: fresh.
- `make tools-test` (no `--allow-growth`), native build and `ctest` (37/37,
  synthetic fixtures).
- `gcc -std=gnu89 -Werror=declaration-after-statement -fsyntax-only` on
  `sfx.c`: clean.

## Follow-ups

- The unit map's GAME-041 holds two original units: SFX.c (`00464740`-
  `00465380`) and, by its separate `.rdata` and commons, Smoke.c
  (`00465380`-`004661c0`).
- When Mesh.c (G98-015) is converted, its entries for `00482e70`, `004830c0`,
  `004820d0` and `00483420`, and Container.c's `00472950`, can replace the
  adapters, which removes the remaining operand differences.

## Reproducing

    git mv src/game/sfx.cpp src/game/sfx_port.cpp   # and make it a tail entry
    python3 tools/convert_unit.py GAME-041 --stem sfx
    make match-function SOURCE=src/game/sfx.c
    make reccmp-check
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object sfx.c.obj"
    make match-progress
    make unit-data-offsets
