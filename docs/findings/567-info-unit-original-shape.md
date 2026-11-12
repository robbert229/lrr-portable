# Finding 567 — Info unit (GAME-019) rebuilt in original shape, with a port tail

Issue #557, under [ADR 0005](../adr/0005-original-shape-c-units.md). The
relocated `src/game/info.cpp` is rebuilt as the C unit
[`src/game/info_unit.c`](../../src/game/info_unit.c), with the C boundary
header [`info_unit.h`](../../src/game/info_unit.h). The unit is text
`00419230`–`0041a300`: the original's Info.c, the in-game information
messages.

- **Types.** Forty message types (`Info_CrystalFound` …
  `Info_GenericMonster`) each have a compiled text, an icon image, a sound,
  a sound cool-down timer and flags.
- **Sending.** `info_send` (`00419b80`) drops duplicates (the same object or
  block under the same type, except the four death types), plays the type's
  sound on its cool-down, queues an instance under its type (at most 40
  types; a long instance list drops its oldest entry) and can slow the
  game to 1x.
- **Display.** The queued types stack as icons down the left of the screen
  with their instance counts. The first one's text goes to the information
  panel, and clicking an icon brings its type to the front.

"Before" is the *match baseline* on `main` at `b141a808`. "After" is this
change. `make reccmp-check` and `make match-function` give the same
per-function scores.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, all 39 functions, 4,031 bytes) | 33.93% | **97.38%** |
| Exact functions in the unit | 5 | **32 of 39** |
| Mapped functions in the unit | 35 | 39 |
| Exact functions, game-only / full scope | 751 / 951 | 778 / 978 |
| Accuracy, full (`binary-progress.json`) | 63.83% | 64.56% |
| Accuracy, game-only | 60.05% | 60.86% |
| Exact-function code bytes | 80,442 | 82,815 |

Byte weights are the original's code bytes without the `0x90` fill; an
unmapped function counts as 0%.

| Address | Function | Bytes | Before | After |
| --- | --- | ---: | ---: | ---: |
| `00419230` | `info_initialise` | 422 | 30.00 | **100.00** |
| `004193e0` | `info_get_type` | 63 | 66.67 | **100.00** |
| `00419420` | `info_get_image` | 15 | 100.00 | **100.00** |
| `00419430` | `info_set_overflow_image_file` | 32 | 90.91 | 90.91 |
| `00419450` | `info_set_auto_game_speed` | 29 | 100.00 | **100.00** |
| `00419470` | `info_set_type_auto_game_speed` | 50 | 32.43 | **100.00** |
| `004194b0` | `info_set_text` | 51 | 52.00 | **100.00** |
| `004194f0` | `info_set_type_play_always` | 50 | 32.43 | **100.00** |
| `00419530` | `info_compile_text` | 281 | 18.80 | 95.02 |
| `00419650` | `info_set_image_file` | 41 | 28.07 | 92.31 |
| `00419680` | `info_set_sfx` | 19 | 43.48 | **100.00** |
| `004196a0` | `info_message_enumerate` | 66 | 71.60 | **100.00** |
| `004196f0` | `info_message_add_instance` | 129 | 29.63 | 55.91 |
| `00419780` | `info_message_get_instance` | 46 | 24.39 | **100.00** |
| `004197b0` | `info_message_remove_instance` | 93 | 35.96 | **100.00** |
| `00419810` | `info_callback_find_object` | 31 | unmapped | **100.00** |
| `00419830` | `info_remove_object_references` | 139 | 8.82 | **100.00** |
| `004198c0` | `info_callback_find_block` | 45 | unmapped | **100.00** |
| `004198f0` | `info_remove_block_references` | 168 | 9.26 | **100.00** |
| `004199a0` | `info_callback_find_duplicate` | 67 | unmapped | **100.00** |
| `004199f0` | `info_is_duplicate` | 132 | 94.55 | **100.00** |
| `00419a80` | `info_create_instance` | 95 | 26.23 | **100.00** |
| `00419ae0` | `info_enable` | 29 | 100.00 | **100.00** |
| `00419b00` | `info_has_text` | 24 | unmapped | **100.00** |
| `00419b20` | `info_find_message` | 39 | 100.00 | **100.00** |
| `00419b50` | `info_get_next_y` | 43 | 30.00 | **100.00** |
| `00419b80` | `info_send` | 538 | 15.53 | 96.77 |
| `00419da0` | `incident_message_panel_refresh_if_active` | 55 | 20.00 | 95.24 |
| `00419de0` | `pending_record_list_entry_move_to_front` | 128 | 43.84 | **100.00** |
| `00419e60` | `pending_record_release_if_present` | 20 | 71.43 | **100.00** |
| `00419e80` | `pending_record_list_entry_remove_and_release` | 133 | 26.79 | **100.00** |
| `00419f10` | `pending_record_release_all` | 29 | 35.29 | **100.00** |
| `00419f30` | `info_draw` | 321 | 43.77 | 97.35 |
| `0041a080` | `info_draw_panel` | 280 | 27.49 | **100.00** |
| `0041a1a0` | `ui_entry_hit_test_and_dispatch` | 175 | 35.47 | **100.00** |
| `0041a250` | `scroll_offset_from_cursor_update` | 64 | 91.30 | **100.00** |
| `0041a290` | `screen_right_edge_cache_refresh` | 43 | 33.33 | **100.00** |
| `0041a2c0` | `info_update_timers` | 40 | 92.31 | **100.00** |
| `0041a2f0` | `info_get_font` | 6 | 100.00 | **100.00** |

27 functions are newly exact and none of the 5 exact before is lost. The
three enumeration callbacks (`00419810`, `004198c0`, `004199a0`), which
Ghidra left as labels inside their callers' ranges, and `00419b00`
(`info_has_text`) are mapped for the first time.

## The port tail

The port's message queue is a different contract. It uses message.hpp's
`MessageConfig` and queue records, the panel-refresh and tick hooks, and
a sidecar table of compiled texts. `message_dispatch`, the ticker and the
tests use it, through `message.hpp` and the frozen `message_for_*.hpp`
headers. As #537 and #535 did, its functions move unannotated to the port
tail [`info_port.cpp`](../../src/game/info_port.cpp) under the port's
names, so native behaviour and tests are unchanged. `info_unit.c` gives the
original functions descriptive `info_*` names.

Nine functions keep their port names because their callers already use the
original's contract (`void (void)` or `void (unsigned int)`); the port tail
no longer defines them:

- `00419da0` `incident_message_panel_refresh_if_active`,
  `00419de0` `pending_record_list_entry_move_to_front` and
  `00419e60` `pending_record_release_if_present`: Panel.c (GAME-032) calls
  these through `game/info.h`;
- `00419e80` and `00419f10` (`pending_record_*`), declared in `util.hpp`;
- `0041a1a0` `ui_entry_hit_test_and_dispatch` (Info_CheckMouse), which had
  no caller;
- `0041a250` `scroll_offset_from_cursor_update` and `0041a290`
  `screen_right_edge_cache_refresh` (`frontend.hpp`). Panel.c calls the
  second; the port tail's text consumer calls the first.

Their state is now `infoGlobs`. Natively, Panel.c's info-dock buttons now act
on `infoGlobs`, which the port never fills, where before they acted on the
port's `s_pending_record_rows`, which only a test helper filled. The scroll
offset moves from `s_screen_right_edge` to `infoGlobs.scrollOffset`. The
port tail's text consumer and `test_frontend` read it through the port-only
accessor `info_scroll_offset_get` (`// PORT-ONLY`, after the original
functions). `frontend.cpp`'s `screen_right_edge_cache_peek` still returns the
old scalar, which no longer changes.

`0041a2f0` returns Interface.c's font (`interfaceGlobs.font`, `004dcd58`).
#559 made `interfaceGlobs` an external C struct, so the unit reads it
directly as `info_get_font`. The port's `incident_message_state_get` stays in
the tail, unannotated, for `test_message`.

**For the owner:** the remaining native callers of the original contract
are the ones to move when their units are converted. `lego.cpp`'s Lego.c
loader `0x00434c10` (the per-type setters `004194b0`–`00419680`) and the
`message_dispatch` callers in `lava_erosion.cpp`, `lego.cpp` and
`legoobject.cpp` (Info_Send) should call `info_set_*` and `info_send`.
`tickers.cpp` and `simtick.cpp` should call `info_draw`, `info_draw_panel`
and `info_update_timers`.

## State

Info.c keeps its whole state in one 0x6f4-byte struct, `infoGlobs` at
`004dc658`, in the object's own `.bss` (`static`):

| Offset | Address | Field |
| --- | --- | --- |
| `+0x000` | `004dc658` | `infoName[40]`, the type names |
| `+0x0a0` | `004dc6f8` | `infoDataTable[40]`: text, image, sfx, timer, flags (0x14 each) |
| `+0x3c0` | `004dca18` | `infoMessageTable[40]`: instance list, count, type, icon x, y (0x14 each) |
| `+0x6e0` | `004dcd38` | `infoMessageCount` |
| `+0x6e4` | `004dcd3c` | the initialise argument |
| `+0x6e8` | `004dcd40` | `overflowImage` |
| `+0x6ec` | `004dcd44` | `scrollOffset` |
| `+0x6f0` | `004dcd48` | `flags` |

The nine old `// GLOBAL` annotations in `info.cpp` (`0049e264`, `004dc658`,
`004dc6fc`, `004dc704`, `004dca20`, `004dcd38`, `004dcd40`, `004dcd44`,
`004dcd48`) are removed; their port variables stay in the tail under the
same names. `s_queue_count` (`004dcd38`), the name #541 kept by renaming
`cmd.cpp`'s, keeps its name there. The address is `infoGlobs.infoMessageCount`.

`make unit-data-offsets`, GAME-019 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-019 | gaps-only (41, 0 unknown) | n/a (0, 0 unknown) fewer than two attributable items | n/a (1, 0 unknown) fewer than two attributable items | - |

The 41 `.data` items are the 40 type names and the `"%i"` count format
(`004a2b54`). Each has a `// STRING` marker directly above it, and none is
pooled with another unit. The six x87 constants (`1.0`, `333.0`, `0.0`,
`25.0`, `-8.0`, `-3.0`) are literals. Whole-binary `.data` identity falls
from 13,338 to 12,934 bytes, and `.rdata` rises from 1,133 to 1,140. Unit
summary: full 0, gaps-only 2, partial 30, n/a 193.

## Contracts and adapters

`extern "C"` is added in place on `EngineTimeScale(void)` and
`scroll_scale_set_and_clamp` (`engine.hpp`), `sound_submit_with_global_cooldown`
(`sound.hpp`), `indexed_global_value_at_get` and
`indexed_text_metrics_pair_get`/`set` (`crt.hpp`),
`pointer_cursor_config_parse_target` and `scroll_offset_from_cursor_update`
(`frontend.hpp`), `pending_record_list_entry_remove_and_release` and
`pending_record_release_all` (`util.hpp`) and `resource_record_add_ref`
(`record.hpp`). Only the `void` overload of `EngineTimeScale` gets C
linkage. The C declarations are in the new
[`info_api.h`](../../src/game/info_api.h), which only `info_unit.c` and
`test_c_boundaries.cpp` include. The existing `game/info.h`, which Panel.c
includes, is unchanged, so `panel_unit.c`'s symbol counter does not move.
For the same reason the unit's header is `info_unit.h`, not `info.h`.

| Callee | Original | Contract |
| --- | --- | --- |
| `lego_goto_info` (`engine.cpp`, new adapter) | `0042de40` Lego_GotoInfo | the port's model at that address takes a clamp state; the port has no camera goto, so the adapter is empty |
| `image_load_bmp_scaled`, `image_display_scaled`, `font_print_f` | `0047d9d0`, `0047dcb0`, `0047a180` | existing unannotated adapters (findings 554, 560) |
| `resource_record_add_ref` | `0047dbc0` Image_SetPenZeroTrans | the port's annotated entry takes its `TextureSurface`. The unit passes the `Image`, as the original does, because the adapter `image_set_pen_zero_trans` is unannotated and dropped `0x00419430` below its baseline (90.91% to 81.82%). The unit's image setters have no native caller |
| `system_state_flags_get` | `00456240` | the original pushes a 0 argument; the port's takes none (−2 instructions in `00419b80`) |

## Shape notes

- **Dead parameter slots.** MSVC places one local in a dead parameter's
  stack slot. In `00419de0` the saved icon `y` goes into `index`'s slot,
  with `x` held on the x87 stack across the `rep movs`. In `00419b80` the
  image goes into `type`'s slot. Both fall out of plain locals. In
  `00419530` the original puts the pointer type in `text`'s slot. With a
  separate `s` pointer, MSVC chose the count instead (93.75%). Walking
  `text` itself, with the pointer type a plain local, gives 95.02%.
- **Early returns.** `004197b0` and `00419a80` are exact only with an early
  `return NULL` (53% and 91% as nested `if`s). `004199a0` is exact with its
  three tests in one `||` condition, which MSVC tail-merges into one
  `return 1`.
- **`0041a1a0`**: the image's width and height load into locals before the
  tests; `y` declared before them and `height + y` give the original's
  `add edi, eax`.
- **`0041a080`**: `len` declared before `text` gives the original's
  `[edx + ecx]` index order; the text is `unsigned char` (the original
  zero-extends the five trailer bytes and tests the count with `jbe`).
- **`0041a250`**: the difference goes to a third local; reusing `lines`
  stores it back (86.96%).
- **`00419b80`**: `engine_app_mode_get()` is compared unsigned, `<= 4`.
- **`004193e0`**: the index is `unsigned` (`jb`).
- **`00419230`** raises flags `0x18` (sounds and auto game speed), not 4.
- **`00419f30`**: the running `pos.y` limit stays on the x87 stack across
  the loop. That falls out of `pos.y = 333.0f` and `pos.y -= height`.

## Near misses

| Address | Score | What is left |
| --- | ---: | --- |
| `00419430`, `00419650` | 90.91, 92.31 | the Image_LoadBMPScaled adapter's call target |
| `00419da0`, `00419f30` | 95.24, 97.35 | adapter call targets only (`lego_goto_info`; `image_display_scaled` and `font_print_f`) |
| `00419b80` | 96.77 | the `system_state_flags_get` argument |
| `00419530` | 95.02 | the loop top reloads `*s` in the original, and one store of the count is scheduled differently; `msvc_permuter.py` (150 s) found nothing |
| `004196f0` | 55.91 | registers only: index, offset and count are in `ebx`/`ebp`/`edi` in the original and `ebp`/`edi`/`ebx` here (rule B). The duplicated tails match. Declaration orders, signedness and loop forms change nothing. A single-tail form scores 71.79% but has a different shape (26 bytes shorter), so it is not used. The permuter had no candidates |

Finding 558's detector (`x87_operand_swaps.py --loose` on the full report)
lists no function in the unit.

## Entropy

`make reccmp-check` against `b141a808`: 29 improved and 4 newly mapped (all
in the unit), no exact match lost, link-order map OK. 13 functions outside
the unit fall below the baseline. All keep their instruction count, the
classifier labels each `drift`, and no listing digest outside the unit
changes. In each, the only change is a call operand: the caller still calls
the port's `message_dispatch` (or, in `0x00434c10`, the port's setters),
now unannotated in the tail, where the original calls `00419b80` (or
`004194b0`–`00419680`). That follows from the port-tail decision, as in
finding 556. **For the owner:**

| Function | File | Score | Instructions |
| --- | --- | --- | --- |
| `0x0040eab0` lava_erosion_pending_cell_tick | `lava_erosion.cpp` | 41.00 → 40.59 | = |
| `0x0040f0e0` kind_six_random_sweep_dispatch | `lava_erosion.cpp` | 34.52 → 33.33 | = |
| `0x0040f330` lava_erosion_neighbor_mutate | `lava_erosion.cpp` | 21.13 → 20.66 | = |
| `0x0042cb70` camera_grid_expiry | `lego.cpp` | 26.21 → 25.24 | = |
| `0x00430530` terrain_cell_mutate | `lego.cpp` | 12.41 → 12.20 | = |
| `0x00431140` terrain_cell_queued_work_transition_current | `lego.cpp` | 42.55 → 40.43 | = |
| `0x00434c10` message_load_infomessages | `lego.cpp` | 18.80 → 17.75 | = |
| `0x00437540` entity_teardown | `legoobject.cpp` | 22.96 → 22.57 | = |
| `0x00439370` EntityBuildPlacementCommandSetup | `legoobject.cpp` | 17.65 → 17.06 | = |
| `0x0043af30` creature_spawn_from_emergence | `legoobject.cpp` | 22.07 → 20.03 | = |
| `0x0043bc40` objective_sidecar_append_gate | `legoobject.cpp` | 39.95 → 39.43 | = |
| `0x0043c6b0` entity_activity_counters_advance | `legoobject.cpp` | 46.41 → 44.73 | = |
| `0x004475c0` EntityProgressAndNegativeDeltaHandler | `legoobject.cpp` | 41.90 → **26.09** | = |

`0x004475c0` loses 16 points for one operand. Reccmp aligns the listings on
call targets, so the changed target misaligns the rest of the diff; the
listing digest is unchanged. Retargeting these callers means editing
`lego.cpp` (#544) and `legoobject.cpp` (#555), which this change may not
do. The baseline is refreshed with these drops.

All header edits are `extern "C"` in place on existing declarations. No
matching-build header gains or loses a declaration: `info_api.h` and
`info_unit.h` are new and included only by the unit, the tail and the
tests. No instruction count changes outside the unit.

## Reccmp lint ratchet

1,527 → 1,521 entries (−6, no growth). With #558's port-tail exemption, the
moved port functions need no entries. The six removed keys are static helpers
of the old `info.cpp` that now sit in the tail. The adapter
`lego_goto_info` and the `// PORT-ONLY` accessor need none.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox) on `b141a808`: the 13
  flagged drops above; link-order map OK. `make match-progress` regenerated
  the baseline, the listing digests and `binary-progress.json`, all three
  with the same input hash.
- `make unit-data-offsets` gives the GAME-019 row above.
- `python3 tools/translation_units.py build`: 0 conflicts;
  `tools/binary_progress_inputs.txt` regenerated.
- `make tools-test` passes (Meson parity, link order, lint).
- `info_unit.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only -Wall`.
- `cmake -S . -B build && cmake --build build && ctest --test-dir build
  --output-on-failure`, with `TEST_WADS_DIR`: 39/39 pass, including
  `c_boundaries`, `frontend` and `message`.

## Tooling notes

- `convert_unit.py` still refuses a relocated unit (findings 544–564). The
  `.cpp` entry was removed by hand, and `--stem info_unit` was given because
  `game/info.h` already exists as Panel.c's boundary header.
- `match_function.py` reports the original's size as the rebuilt size when
  the rebuilt function is shorter, so a shorter, differently shaped
  candidate can score higher than a faithful one (`004196f0` above).

## Reproducing

    python3 tools/convert_unit.py GAME-019 --stem info_unit   # after removing the .cpp entry
    make match-function SOURCE=src/game/info_unit.c
    make reccmp-check
    make match-progress
    make unit-data-offsets
