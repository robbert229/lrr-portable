# Finding 560 — Reward unit (GAME-037) rebuilt in original shape, with a port tail

Issue #535, under [ADR 0005](../adr/0005-original-shape-c-units.md). The
relocated `src/game/reward.cpp` (finding 547) is rebuilt as the C unit
[`src/game/reward.c`](../../src/game/reward.c), with the C boundary header
[`reward.h`](../../src/game/reward.h). The unit is text `0045f030`–`00462da0`:
the original's Reward.c, the end-of-level reward screen.

- **Configuration.** The ten items (Crystals, Ore, Diggable, Constructions,
  Caverns, Figures, RockMonsters, Oxygen, Timer, Score) take their text,
  images, box images, flics, fonts and sounds from the game block's Reward
  group, and their importance and quota from the level's.
- **Results.** At the end of a level the unit copies the running counts into
  a `RewardLevel`, scores every item against its quota and the score against
  the importances, and fills a scrolling summary.
- **The screen.** Items appear one by one on the timer, then the score and
  the save and advance buttons; the advance button commits the checkpoint.

"Before" is the *match baseline* at `9c7d2cd5`, the head of #548 (the
restored `0x00403c00`, on #549's G98-002 and G98-015 relocations). This
change is stacked on #548. "After" is this change. `make reccmp-check` and
`make match-function` give the same per-function scores.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, all 52 functions, 15,329 bytes) | 16.21% | **89.58%** |
| Exact functions | 11 | **23 of 52** |
| Mapped functions | 49 | 52 |
| Exact functions, full scope | 894 | 906 |
| Accuracy, full (`binary-progress.json`) | 60.48% | 61.58% |
| Accuracy, game-only | 56.28% | 57.50% |
| Exact-function code bytes | 76,650 | 77,536 |

Byte weights are the original's code bytes without the `0x90` fill; an
unmapped function counts as 0%.

| `0045f030` | `reward_initialise` | 447 | 13.49 | 97.17 |
| `0045f1f0` | `reward_load_item_sound` | 60 | 32.43 | 94.74 |
| `0045f230` | `reward_load_item_font` | 88 | 12.21 | 96.43 |
| `0045f290` | `reward_load_item_box_image` | 322 | 15.58 | 93.94 |
| `0045f3e0` | `reward_load_item_image` | 322 | 15.85 | 93.94 |
| `0045f530` | `reward_load_item_text` | 187 | 26.77 | 93.44 |
| `0045f5f0` | `reward_load_item_flic` | 343 | 20.63 | 94.90 |
| `0045f750` | `reward_shutdown` | 93 | 15.38 | **100.00** |
| `0045f7b0` | `reward_free_item_image` | 20 | 40.00 | **100.00** |
| `0045f7d0` | `reward_free_item_font` | 20 | 53.33 | **100.00** |
| `0045f7f0` | `reward_load_graphics` | 766 | 9.41 | 92.11 |
| `0045faf0` | `reward_load_buttons` | 679 | unmapped | 91.96 |
| `0045fda0` | `reward_prepare_level` | 755 | 13.71 | 92.82 |
| `004600a0` | `reward_load_item_importance` | 152 | 25.00 | 92.73 |
| `00460140` | `reward_load_item_quota` | 152 | 23.68 | 89.09 |
| `004601e0` | `reward_free_level` | 15 | 47.06 | **100.00** |
| `004601f0` | `reward_level_get` | 6 | 10.53 | **100.00** |
| `00460200` | `reward_current_level_get` | 23 | 47.62 | **100.00** |
| `00460220` | `reward_level_start` | 107 | unmapped | 94.59 |
| `00460290` | `reward_building_counts_get` | 121 | 73.24 | **100.00** |
| `00460310` | `reward_building_counts_callback` | 80 | 47.62 | **100.00** |
| `00460360` | `reward_prepare_results` | 1452 | 10.31 | 65.87 |
| `00460910` | `reward_object_count_get` | 64 | 100.00 | **100.00** |
| `00460950` | `reward_calculate_score` | 1453 | 6.52 | 90.86 |
| `00460f00` | `reward_create_scroll` | 353 | 13.20 | 99.09 |
| `00461070` | `reward_format_values` | 926 | 27.62 | 99.28 |
| `00461410` | `reward_loop` | 885 | 8.01 | 97.25 |
| `00461790` | `reward_draw_item` | 1265 | 4.96 | 92.46 |
| `00461c90` | `reward_draw_score` | 305 | 28.87 | 90.91 |
| `00461dd0` | `reward_do_buttons` | 597 | 12.21 | 96.12 |
| `00462030` | `reward_save_menu` | 575 | 18.18 | 61.54 |
| `00462270` | `reward_advance` | 287 | 30.15 | 82.83 |
| `00462390` | `reward_debug_keys` | 200 | 14.12 | 98.44 |
| `00462460` | `reward_restart_flic` | 53 | 34.29 | 94.74 |
| `004624a0` | `reward_update_state` | 350 | 24.69 | 79.23 |
| `00462600` | `reward_draw_values` | 245 | 11.28 | **100.00** |
| `00462700` | `reward_update_input` | 124 | 100.00 | **100.00** |
| `00462780` | `reward_check_skip` | 121 | 9.84 | 75.32 |
| `00462800` | `reward_score_add` | 39 | 88.89 | **100.00** |
| `00462830` | `global_float_at_00554adc_add_one` | 19 | 100.00 | **100.00** |
| `00462850` | `global_float_at_00554efc_add_one` | 19 | 100.00 | **100.00** |
| `00462870` | `global_counter_at_00555324_increment` | 7 | 100.00 | **100.00** |
| `00462880` | `global_counter_at_00555328_increment` | 7 | 100.00 | **100.00** |
| `00462890` | `global_counter_at_0055532c_increment` | 7 | 100.00 | **100.00** |
| `004628a0` | `global_float_accumulator_00555330_add` | 17 | 100.00 | **100.00** |
| `004628c0` | `global_float_accumulate_00555120` | 17 | 100.00 | **100.00** |
| `004628e0` | `reward_scroll_create` | 123 | 50.98 | **100.00** |
| `00462960` | `reward_scroll_free` | 101 | 39.18 | **100.00** |
| `004629d0` | `reward_scroll_add_label` | 222 | unmapped | 92.41 |
| `00462ab0` | `reward_scroll_length_set` | 12 | 100.00 | **100.00** |
| `00462ac0` | `reward_scroll_flags_set` | 12 | 100.00 | **100.00** |
| `00462ad0` | `reward_scroll_update` | 714 | 13.65 | 85.59 |

12 functions are newly exact and none of the 11 exact before is lost.
`0045faf0` (the button loader), `00460220` (level start) and `004629d0` (add
a scroll label) are mapped for the first time.

## The port tail

The port's reward screen is a different contract: `GameContext`'s task
records, the `RewardScreenState` loop with its ops table, the checkpoint
blob and the port's font and image records. As #537 did for GAME-026, its
functions move unannotated to the port tail
[`reward_port.cpp`](../../src/game/reward_port.cpp) under the port's names,
and the native callers and tests keep their behaviour. `reward.c` gives the
original functions descriptive `reward_*` names.

Eight functions keep their port names because their callers already use the
original's contract: `reward_score_add` and the seven counter functions
`00462830`–`004628c0`. Their state is now `rewardGlobs`' fields, so
`lego.cpp`, `legoobject.cpp`, `weapons.cpp`, `mission.cpp` and
`objective_unit.cpp` update the original's counts. The old `crt.cpp`
scalars and their peeks remain, unused, so that no shared header loses a
declaration (entropy rule 1). `test_crt` checks the fields instead.

The rest of `reward.c` has no caller yet; nothing in the port fills a
`RewardLevel`. Following the same contracts across, these are the native
callers to move when their units are converted: `mission.cpp`
(`mission_task_init_target`, the refreshers and the reward-screen loop),
`savegame.cpp` and `frontend.cpp` (the checkpoint blob and
`save_menu_open_from_reward`), and `engine.cpp` (`reward_task_block_finish`).

## State

The original keeps the whole unit's state in **one common struct**,
`rewardGlobs` at `00552980` (0x3250 bytes):

- `+0x0000`: Display, CentreText, the timer and the two `RewardLevel`
  pointers (the item configuration and the level being scored), and the
  wallpaper;
- `+0x001c`: an embedded 0x318c-byte `RewardLevel` holding the level's
  running counts (`0055299c`). Every scalar the port had annotated in this
  range is one of its fields: `00554adc`, `00554efc`, `00555118`,
  `00555120`, `00555324`–`00555330`, `00555738` and `0055573c`;
- `+0x31a8`: ScrollSpeed, VertSpacing, the fonts, the config, the scroll,
  the buttons, the three texts, TextPos, the status text and the three
  Display flags.

`reward.c` defines it as a tentative definition (common), and the ten old
annotations (`crt.cpp` ×9, `spawn.cpp` ×1) are removed. A `RewardLevel` holds
building names, a walls-and-reinforcements item at `+0x1ad4`, and the
item array at `+0x1cec`, pointed to by `+0x1ce8`. Some functions use the
pointer and others the array, as the listings show.

`make unit-data-offsets`, GAME-037 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-037 | partial (44, 0 unknown) gap 36 first 004a9370: 54/6c | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | 00552980:_rewardGlobs(size=12880) |

`.data` holds the unit's 44 string literals, each with a `// STRING` marker
directly above it. The pooled literals carry no marker: `"Reward"`,
`"Quota"`, `"Crystals"` (Panel.c), `"Ore"`, `"Text"`, `"|"`, `"%s"`,
`"!%s"`, `"Pilot"` and the empty string. The matching build does not pool
repeated literals, so the repeated `"Reward"` and friends shift the later
strings: the first gap is at `004a9370` (`"Sounds"`). Before this change
the unit's `.data` was eight annotated name arrays, which agreed. That is the
one fewer agreeing unit in `binary-progress.json` (8 to 7). The 19 x87
constants are literals; MSVC pools them in the original's order. Unit
summary: full 0, gaps-only 2, partial 30, n/a 193.

## Contracts and adapters

`extern "C"` is added in place for every C++ declaration the unit calls
(`level.hpp`, `entity.hpp`, `record.hpp`, `surface.hpp`, `view.hpp`,
`frontend.hpp`, `savegame.hpp`, `options.hpp`, `upgrades.hpp`,
`effects.hpp`, `video_graph.hpp`, `frameloop.hpp`, `sound.hpp`, `flic.hpp`,
`crt.hpp`, `terrain_grid.hpp`, `input_shared.hpp`). The unit's new C
boundary headers are included only by `reward.c` and `test_c_boundaries.cpp`:
`crt/crt_mouse.h`, `gods98/{input_keys,font_api,frameloop,sound_api,media,view_api,record_api,flic_api}.h`
and `game/{level,entity_api,terrain_grid,frontend_api,savegame,options,upgrades,effects,video_graph,mission,lego_api}.h`.
Existing C headers that other C units include are not extended, because
that would move those units' symbol counters.

**Frozen headers.** `entity.cpp` and `savegame.cpp` reach three callees only
through frozen `_for_`/`_from_` headers, which keep C++ linkage. VC5's
linker has no `/ALTERNATENAME`. Two of the annotated definitions therefore
get C linkage under a new name, and an unannotated C++ forwarder keeps the
port's name for those files:

- `0042e8b0` is now `entity_type_pair_lookup` (`entity_type_pair_for_name`
  forwards). Without this, `0x00460910` drops from 100% to 96.30%;
- `00416e20` is now `front_level_set_index_of` (`list_find_name_target`
  forwards; `0x00417090`'s call is renamed).

`mission_replace_char` and `0x00435550` take `extern "C"` on their
definitions, because their other declarations are in the port tail.

**Adapters** (thin, unannotated, at the end of their module, ADR 0005 step 6):

| Adapter | Original | Why |
| --- | --- | --- |
| `font_print_f` (`bitmap_font.cpp`) | `0047a180` Font_PrintF | the port's entry takes a render target |
| `tooltip_draw_box` (`tooltip.cpp`) | `0046b8b0` ToolTip_DrawBox | the original takes an `Area2F` and three colours by value; the port takes a state record and hooks the original ABI does not carry, so it describes the box but does not draw it |
| `lego_reset_level_and_exit` (`engine.cpp`) | `004250a0` | the port's is `static` |
| `front_campaign_snapshot_create` (`frontend_unit.cpp`) | `00418040` | the port's takes the `GameContext`; the adapter passes `save_current_context_get()` |
| `movie_play_wait` (`frontend_unit.cpp`) | `00415700` | the port has no movie wait; empty |

FrontEnd.c state that the port does not model is stood in for by port globals
at the end of `frontend_unit.cpp`: the end-game AVIs (`00556fb0`/`b4`), the
menu set (`00556fc4`), the save tree (`00556ff8`), the slot buffer
(`00557418`), and the save menu's result and open flag (`00557504`/`08`).
Nothing in the port sets them.

**Input.** `0x0076ab80` (`s_global_current_keys`) and `0x0076aa00` are now
annotated. The second is a new port array, `s_global_previous_keys`, which
`input_global_current_keys_copy` fills from `InputState.prev`.
`input_unit.cpp` includes `input_shared.hpp` before its frozen copy (they
are identical), so the array's C linkage reaches it with no new
declaration. The mouse words are read as `int`: the original loads them
with `fild dword`, and the port declares them `uint32_t`.

**Contract differences left as they are** (each costs operands, recorded for
the callees' conversions): `frontend_enabled(cfg)`, `menu_enter(fe, mode)`,
`level_objective_result_get(level)`, `save_mark_record(ctx, i)`,
`save_commit(ctx)` and `sav_tree_load`'s sixth argument each take one more
argument than the original. `TextWindow_Update` gets 1.0 where the original
reads `0050593c`, which is static in the winapi entry. Flic.c's current frame
is read at `+0x624` and Lego_Level's fields through a partial layout,
`RewardLegoLevel`; both are valid only for the original's records, which
nothing in the port passes yet.

## Shape notes

- **No final `return`.** Six functions (`00460220`, `00461c90`, `00461dd0`,
  `00462390`, `004624a0`, `00462600`) return 0 on their early exits and
  fall off the end otherwise. MSVC then keeps every register push in the
  prologue and duplicates the epilogue, which is the original's shape; a
  `void` function shrink-wraps instead. They are `int`, with
  `#if !defined(_MSC_VER) return 1;` for C++, as in `0047bc30` (finding 553).
- **Parentheses stop constant folding in C.** `(elapsed * 0.04f) * 1000.0f`
  keeps two `fmul`s; without the parentheses the C front end folds the two
  constants. This restored `0x00462700` to 100%.
- **Block-scoped locals** let the image positions, the flic area, the box
  and its colours share stack slots in `00461790` (60.87% → 90.75%).
- **`reward_item_percent`** is a `static __inline` helper in
  [`reward_maths.h`](../../src/game/reward_maths.h). As a macro, MSVC keeps
  the eight percentages in stack temporaries across the score loops
  (73.30%); through the pointer parameter it reloads them (88.95%). The loop
  body adds before it counts (90.86%).
- **Keys.** "Pressed" is `key && (prev ^ key)`; `!=` compiles to `cmp`, not
  the original's `xor`.
- **`00461070`.** The timer text uses `if/else` `sprintf` calls, which MSVC
  tail-merges into one call, rather than a `?:` format. The quotient goes to
  the loop counter first (99.28%).
- **`004628e0`** takes its area as an `Area2F` by value and stores it with one
  struct copy (53.49% → 100%). `RewardScroll` is laid out around it.
- **`00460290`** returns its output pointer, and `00460220` copies from the
  returned pointer.
- **`timeGetTime`** is declared without `dllimport` for MSVC
  (`gods98/frameloop.h`), so the call goes through the import thunk
  `00478000`, as in the original (like `DirectSoundCreate` in finding 553).
- **`reward_draw_item`** takes a fourth argument, the time, which every
  caller pushes and the function never reads.

## Near misses

| Address | Score | What is left |
| --- | ---: | --- |
| most loaders and `0045f7f0`/`0045faf0`/`0045fda0` | 89–97 | pooled-string and adapter operands only |
| `00460360` | 65.87 | the loop keeps its zero and `y` in other registers; the permuter (240 s, 9 candidates) found nothing |
| `00460950` | 90.86 | x87 scheduling in the score loops (an extra `fxch`) |
| `00461790` | 92.46 | the flic argument: the original branches to push 1 or 0, where this build emits `setl` |
| `00462030` | 58.46 | the extra-argument contracts above; duplicating the save tail as the original does scores lower (52%) |
| `00462270` | 81.82 | the same contracts, and the menu-set chain through stand-ins |
| `004624a0` | 79.23 | `count` and `state` in swapped registers; declaration orders and comparison forms change nothing |
| `00462780` | 75.32 | the original tests the space key a fourth, redundant time; every source form tried is folded away |
| `00462ad0` | 85.59 | `0050593c` and pooled operands, and the order of the centring arithmetic |

## Entropy

`make reccmp-check` against `9c7d2cd5`: 38 improved and 3 newly mapped (all in
the unit), and one drop, flagged:

| Function | File | Score | Instructions | Label |
| --- | --- | --- | --- | --- |
| `0x0041fa80` ledger_stub_0041fa80_StartLevelPilots | `game/lego.cpp` | 55.88 → 54.41 | +0 | `drift`; operand only |

**For the owner:** `0x0041fa80` stores to `00555118`. In the original that
address is a `rewardGlobs` field (`level.itemsArray[FIGURES].startCount`),
so the original side now names it `rewardGlobs+offset`. The port still
writes `spawn.cpp`'s `s_start_level_pilot_grid_count`, whose annotation this
change removes. The code is unchanged. Keeping the annotation would overlap
`rewardGlobs` and cost `00460220` and `00460360` the same operand. The
baseline is refreshed with this drop.

Header edits are `extern "C"` in place, two renames of annotated
definitions (same declaration count) and adapters after each file's last
function. `input.cpp` gains one declaration before its only annotated
function, `0x00483810`, whose score is unchanged. No other function changes
score or instruction count. No exact match is lost: full-scope exact
functions go from 894 to 906.

## Lint ratchet

4,117 → 4,167 entries (+50), with `--allow-growth`, as for #537 (finding
556). The 43 port-tail definitions lost their annotations but still link in
the matching build, which the port's callers need. The other 7 keys are the
adapters and forwarders above (`font_print_f`, `tooltip_draw_box`,
`lego_reset_level_and_exit`, `front_campaign_snapshot_create`,
`movie_play_wait`, `entity_type_pair_for_name`, and
`front_level_set_index_of`'s non-MSVC body, renamed from the existing
`list_find_name_target` key). **For the owner:** please confirm.

## Tooling notes

- `convert_unit.py` still refuses a relocated unit, as in findings 544–556.
  The `.cpp` entry was removed from `src/link_order.cmake` by hand, and
  `--stem reward` was given (the unit is unnamed in the ranges file).
- The scaffold's boundary-test include of `game/entity.hpp` conflicts with
  `steering.h`, as finding 549 found; it is dropped.
- `msvc_permuter.py` found nothing for `00460360` (9 candidates).
- The matching build has no `/Gf`, so literals the original pooled across
  and within objects stay separate.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox) against `9c7d2cd5`: the
  one flagged drop above. `make match-progress` (`pefile` from a
  worktree-local `build-venv`) regenerated the baseline, the listing digests
  and `binary-progress.json`.
- `make unit-data-offsets` gives the GAME-037 row above.
- `python3 tools/translation_units.py build`: 0 conflicts.
  `tools/binary_progress_inputs.txt` is regenerated.
- `make tools-test` passes (Meson parity, link order, lint).
- `reward.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only -Wall`. Its one warning is the original's empty `sprintf`
  format.
- `cmake -S . -B build && cmake --build build && ctest --test-dir build
  --output-on-failure`: 37/37 pass, including `c_boundaries` and `crt`.

## Reproducing

    python3 tools/convert_unit.py GAME-037 --stem reward   # after removing the .cpp entry
    make match-function SOURCE=src/game/reward.c
    make reccmp-check
    make match-progress BINARY_PROGRESS_PYTHON=build-venv/bin/python3
    make unit-data-offsets
