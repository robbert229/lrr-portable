# Finding 631 — Objective.c (GAME-029) and Message.c (GAME-027) rebuilt in original shape

Issue #710, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
Both units had been relocated verbatim (`objective_unit.cpp`,
`message_unit.cpp`) and never converted. This change:

- rebuilds GAME-029 Objective.c as the C unit
  [`src/game/objective_unit.c`](../../src/game/objective_unit.c), with
  [`objective_unit.h`](../../src/game/objective_unit.h),
  [`objective_unit_imports.h`](../../src/game/objective_unit_imports.h) and
  [`objective_unit_link_names.h`](../../src/game/objective_unit_link_names.h);
- rebuilds GAME-027 Message.c as the C unit
  [`src/game/message_unit.c`](../../src/game/message_unit.c), with
  [`message_unit.h`](../../src/game/message_unit.h),
  [`message_unit_imports.h`](../../src/game/message_unit_imports.h) and
  [`message_unit_link_names.h`](../../src/game/message_unit_link_names.h);
- moves the relocated files, unannotated, to the port tail
  (`git mv` to [`objective_port.cpp`](../../src/game/objective_port.cpp)
  and [`message_port.cpp`](../../src/game/message_port.cpp), both `tail`
  entries in `src/link_order.cmake`).

"Before" is `main` `7600fb4c`; "after" is this change from the full
matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Message.c similarity (26 functions, byte-weighted, 4,185 bytes) | 35.61% | **98.67%** |
| Message.c exact | 9 | **23** |
| Objective.c similarity (15 functions, byte-weighted, 7,189 bytes) | 12.14% | **80.61%** |
| Objective.c exact | 5 | **11** |
| Mapped functions (both units) | 36 | **41** |
| Exact-function code bytes (`binary-progress.json`) | 244,057 | **247,409** |
| `opt_ref` annotated (full): removed / exact removed | 327 / 198 | **269 / 168** |
| `/OPT:REF` relink `.bss` (`opt_ref.image.bss_bytes`) | 1,111,760 | 1,113,328 |
| Gate image end (`make pe-audit`) | `0x86d000` | `0x86e000` |
| Reccmp lint ledger (entries) | 983 | **971** |

Byte weights are the original sizes from the single-function loop; an
unmapped function counts as 0%. Names are this repository's snake case of
OpenLRR's (`inferred`); "link name" is the name the definition takes in the
matching build.

### Message.c

| Address | Function | Link name | Before | After |
| --- | --- | --- | ---: | ---: |
| `00451cd0` | `message_initialise` | `message_string_table_initialize` | unmapped | **100.00** |
| `00451f60` | `message_remove_events_with_object` | `cmd_queue_cancel_match` | 18.18 | 76.47 |
| `00451fd0` | `message_lookup_physical_name` | (own) | unmapped | **100.00** |
| `00452010` | `message_add_message_action` | `lego_key_callback_set_adapter` | 28.57 | **100.00** |
| `00452060` | `message_post_event` | `cmd_produce` | 35.48 | **100.00** |
| `004520d0` | `message_update` | `cmd_queue_tick` | 21.98 | **100.00** |
| `00452430` | `message_pick_random_floor` | `entity_random_valid_cell_command_create` | 28.12 | **100.00** |
| `004524b0` | `message_remove_object_reference` | `selection_object_detach` | 72.22 | **100.00** |
| `00452520` | `message_copy_selected_units` | `selection_snapshot_create` | 15.73 | **100.00** |
| `00452580` | `message_get_selected_units2` | `selection_active_context_record_get` | 100.00 | 100.00 |
| `004525b0` | `message_any_unit_selected` | `selection_active_has_any_get` | 100.00 | 100.00 |
| `004525c0` | `message_get_primary_selected_unit` | `selection_active_entity_get` | 100.00 (asm) | 100.00 (C) |
| `004525e0` | `message_get_selected_units` | `selection_active_records_get` | 100.00 | 100.00 |
| `004525f0` | `message_get_num_selected_units` | `selection_active_count_get` | 100.00 | 100.00 |
| `00452600` | `message_cleanup_selected_units_count` | `selection_active_count_reset` | 100.00 (asm) | 100.00 (C) |
| `00452610` | `message_is_unit_selected` | `selection_contains_entity_active_target` | 100.00 | 100.00 |
| `00452650` | `message_find_index_of_object` | `cmd_array_find_value_index` | 79.31 | **100.00** |
| `00452690` | `message_is_selected_or_flags4_2` | `selection_active_candidate_already_selected_or_marked` | 100.00 | 100.00 |
| `004526c0` | `message_select_object` | `selection_candidate_filter_default` | 87.50 | **100.00** |
| `004526e0` | `message_select_object2` | `selection_candidate_filter_active` | 73.73 | 97.52 |
| `00452870` | `message_can_select` | `selection_candidate_eligibility_check` | 100.00 | 100.00 |
| `004528c0` | `message_reduce_selected_units` | `selection_state_tick_active` | 32.77 | **100.00** |
| `00452be0` | `message_clear_selected_units` | `selection_clear_active` | 40.62 | **100.00** |
| `00452c50` | `message_deselect_object` | `selection_entity_remove_active` | 97.06 | **100.00** |
| `00452cc0` | `message_kill_all_selected_units` | `selected_entity_state_monitor` | 27.35 | 97.92 |
| `00452d60` | `message_enter_first_person_view` | `selection_command_dispatch` | 29.20 | 86.44 |

### Objective.c

| Address | Function | Link name | Before | After |
| --- | --- | --- | ---: | ---: |
| `004574e0` | `objective_load_objective_text` | (own) | unmapped | 96.23 |
| `00457d40` | `objective_load_level` | `objective_load_original` | 9.21 | 60.88 |
| `00458580` | `objective_set_cry_ore_objectives` | (own) | 100.00 | 100.00 |
| `004585c0` | `objective_set_block_objective` | (own) | 100.00 | 100.00 |
| `004585f0` | `objective_set_timer_objective` | (own) | 100.00 | 100.00 |
| `00458620` | `objective_set_construction_objective` | (own) | 100.00 | 100.00 |
| `00458650` | `objective_has_achieved` | `level_objective_result_get` | 10.53 | **100.00** |
| `00458660` | `objective_set_end_teleport_enabled` | `global_value_004ffe48_set` | 100.00 | 100.00 |
| `00458670` | `objective_set_status` | `objective_announce_current` | 30.99 | 99.43 |
| `004588e0` | `objective_stop_showing_objective` | (own) | 31.43 | **100.00** |
| `004589a0` | `objective_is_ended` | `objective_is_ended_original` | 17.39 | **100.00** |
| `004589c0` | `objective_handle_keys` | `lego_quit_poll_adapter` | unmapped | 97.50 |
| `00458be0` | `objective_update` | `objective_tick_target` | 15.03 | 56.52 |
| `00459050` | `objective_check_completed` | (own) | 36.13 | **100.00** |
| `00459100` | `objective_callback_check_completed_object` | (own) | unmapped | **100.00** |

Four Lego.c callers rose with the link names (`0041fb50` 98.18 to 98.35,
`004232e0` 98.15 to 98.37, `004250c0` 89.06 to 89.58, `00429890` 92.56 to
92.60). `make reccmp-check`: 26 improved, 5 newly mapped, **no function
below the baseline, no exact match lost**, no drop to label. No function
outside the two units changed instruction count. No naked asm (the port's
two asm functions, `004525c0` and `00452600`, are now C), and no
`Deviation (scoring only)`.

## Unit extent

Both boundaries were `low`. Neither unit changes extent except that
Objective.c gains a function the map missed.

### Objective.c: `004574e0`-`00459190`, 15 functions

- **Call graph.** A scan of `.text` for references into the range finds the
  map's 14 functions plus `00459100`, Objective_Callback_CheckCompletedObject,
  which only `00459050` references (pushed at `004590e7` for
  LegoObject_RunThroughListsSkipUpgradeParts). Nothing else in the range
  is referenced.
- **The start.** `00456d00`-`004574d9`, between the map's last GAME-028
  function (`00456cb0`) and `004574e0`, holds NERPs functions missing from
  the corpus: they are reached only through NERPs' function table
  (`004a5948`-`004a6768`: `00457130`, `00457260`-`004574a0`) and the
  pointer at `004a676c` (`00456d00`), and they call NERPs' block-list
  helpers `00456c30`/`00456cb0`. So GAME-028 ends at `004574e0`.
- **Literal pool.** `.rdata` `0049e56c`-`0049e5a4` holds -60, -220, 320,
  20, 390, 180, -40, -34, 422, 21, 0.5, 1.0, 25.0, 0.0 and 1.5, with no
  repeated value, after NERPs' pool (`0049e52c`-`0049e568`, which has its
  own 20.0 at `0049e568`). Only `004574e0`, `00457d40`, `00458be0` and
  `00459050` read it, so it is one object (findings 599 and 603's test).
- **Data.** `.bss` `004ffbc0`-`004ffe4c` (`objectiveGlobs`, 0x28c bytes)
  is referenced by no code outside the range. The strings run
  `004a8088`-`004a8284`.
- **The end.** `00459190` opens the `.osf` unit (GAME-030, `.bss`
  `004ffe50`).

### Message.c: `00451cd0`-`00452df0`, 26 functions

- **Data.** `.bss` `004eaf20`-`004ff2a0` (`messageGlobs`, 0x14380 bytes:
  two banks of 2,048 0x14-byte events, their counts and the bank index, the
  100 selected units, 10 key actions, the 65 `Message_*` names, the
  selection count and the key-action count) is referenced by no code
  outside the range. The 65 names at `004a5308`-`004a5938` are used only by
  `00451cd0`. The unit has no `.rdata` or `.data`.
- **Call graph.** No function in the range is referenced only by address
  (Message_Update's jump table is inside it). `00451fd0` is called only from
  GAME-035's `0045d7e0`.
- **Edges.** `004519b0`-`00451cd0` before it (the model-record helpers the
  map leaves in GAME-026) reads none of this state, and `00452df0` after it
  is NERPs' first function.

`translation-units.spec.csv` records this evidence; `python3
tools/translation_units.py build`: 0 conflicts.

## How the conversion is done

As findings 617, 623 and 624:

- **Records.** `objective_unit.h` declares the level's objective block
  (`Objective_Data`, Lego_Level `+0xbc`, 0x54 bytes); the unit reaches it
  through `lego_unit.h`'s `Lego_Level::f_0bc`. `message_unit.h` declares
  `Message_Event` (0x14 bytes); the units keep `Objective_Globs` and
  `Message_Globs` private.
- **State.** One `static` struct per unit at its `.bss` address. Input.c's
  `0076ab48` (Objective_SetStatus waits on it), which no unit defined, is a
  common definition in `objective_unit.c`.
- **Strings.** Each unit's own literals carry `// STRING` markers. `""`
  is `config.c`'s pooled `s_config_angle_empty` (finding 581).

### What the shapes needed

- **Objective_IsEnded** tests the three status bits separately
  (`a & 1 || a & 2 || a & 4`); a mask gives `neg`/`sbb`.
- **Objective_LoadLevel**: HitTimeFail is a function-scope flag set by
  `!_strcmpi(...)`, which gives the original's `mov eax, 1` / reload.
  Writing the level's fields directly, not through a local pointer, keeps
  `ebp` on the objective block. Left at 60.88%: the original frame has one
  local fewer (`0x51c` against `0x520`), which shifts every stack operand.
  Tried: the colour channels as floats, dropping the timer temporary,
  inline arguments; and `msvc_permuter.py` (600 s, no candidate).
- **Objective_LoadObjectiveText**: the level-name length is computed once;
  the `'_'`-to-space copy is `(unsigned char)` (the original's `and eax,
  0xff`); `beginArea` is declared first (finding 538 rule A for the
  `fadd`).
- **Objective_HandleKeys**: the "next page" test is the original's shared
  tail (a `goto` into the sound-and-advance block), and the back button's
  bounds are `> 129` / `> 314` with `currentPage > 0`. Left: two register
  choices in the back-button test.
- **Objective_Update** (56.52%): the sound ID and the button position live
  in inner blocks, which gives the original's 0x10-byte frame. Left: the
  original keeps the page type in memory and `3` in `edi`; ours gives the
  type a register. Tried: declaration orders, branch inversions and the
  permuter (best +0.9 by inverting one `if`, not applied).
- **Message_Update**: index the event list (`list[loop]`), which gives the
  original's `+8`-biased pointer and down-counter; Goto and RockMonsterGoto
  are separate cases with the same body, so the jump table keeps two
  entries (99.68% to 100%).
- **Message_ReduceSelectedUnits**: the flags are declared in the original's
  order and the drivable test checks the driver first.
- **Lego_GetMap** (`00431570`) takes no argument; the port's entry under
  that address takes a cell, so the unit calls it through a cast, as
  `lego_unit_imports.h` does.
- Left: `00451f60` (76.47%, the original computes the destination pointer
  before the compare; four shapes tried), `00452d60` (86.44%, block
  placement of the `return 0` tail; a `goto` and a `break` form are lower).

## Link names and callers

In the matching build both units' functions take the names their callers
already use (the tables above); every such caller passes the original's
arguments. The port's definitions of those names are compiled out of the
matching build (`LRR_OBJECTIVE_PORT_NAMES`, `LRR_MESSAGE_PORT_NAMES`):

- `objective_port.cpp`: `objective_load_original`,
  `level_objective_result_get`, `global_value_004ffe48_set`,
  `objective_announce_current`, `objective_tick_target`;
- `lego_port.cpp`: `objective_is_ended_original`, `lego_quit_poll_adapter`
  (both adapters had no port behaviour: the quit poll never quit);
- `message_port.cpp`: every annotated model except `cmd_bind_key` (another
  contract, kept for `frameloop.cpp`), the `lego_key_callback_set_adapter`
  adapter, and the 81,920-byte event queue only those models used;
- `resource.cpp`: `message_string_table_initialize`.

Natively all of them stay live and the units keep their own names.

**Caller switched.** `lego_unit.c` calls Objective_IsEnded through two
names; `lego_unit_imports.h` maps `lego_level_low_bits_enabled_adapter` to
`objective_is_ended_original` in the matching build (one `#define`; the
adapter in `objective_port.cpp` stays for the native build). This is the
only edit in another thread's area (`lego_unit_imports.h`, #708's unit).

**Adapters added** (`objective_port.cpp`, `PORT-ONLY`, unannotated):
`objective_osf_store_adapter` (`004592e0`, the port's `osf_snapshot` takes
the context), `objective_osf_restore_adapter` (`00459360`, no port model)
and `objective_image_get_screenshot_adapter` (`0047e300`, the port's entry
has another contract). The unit's calls to them are the remaining call
mismatches in `00458670` and `00458be0`.

## `/OPT:REF` (finding 579)

`opt_ref` falls from 327/198 to 269/168: the unit functions are now the
definitions Lego.c, AITask.c, LegoObject.c, Interface.c, Panel.c and the
port's C++ callers reach. `make opt-ref-callers` lists one removed unit
function, `00451fd0`, whose only caller is GAME-035's port stub
`0045d7e0` (not converted). Objective.c has none.

## `.bss` and the image

The relink `.bss` grows by 1,568 bytes: the units add their own state
(82,816 and 652 bytes), and `message_port.cpp`'s 81,920-byte queue leaves
the matching build. The gate image end moves from `0x86d000` to `0x86e000`.

## Native behaviour

Unchanged: the port's objective and message code stays live, and the units
compile as C++ but are not called. `ctest`: 37 of 37 pass.

## Checks

- `make reccmp-check`: no function below the baseline; plain-immediate,
  winapi-link and link-order map checks OK.
- `make match-progress`, `make unit-data-offsets`: regenerated; the three
  progress files committed.
- `make tools-test` (no `--allow-growth`): OK; lint ledger 983 to 971.
- `make winapi-link-check`: OK. Native build and `ctest`: OK.

## Follow-ups

- Convert GAME-035's `0045d7e0`, which calls Message_LookUpPhysicalName.
- Image_GetScreenshot (`0047e300`) and the `.osf` store/restore
  (`004592e0`, `00459360`) need original-ABI entries when Images.c (#711)
  and GAME-030 are converted; the adapters then retire.
