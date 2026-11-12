# Finding 570 — AITask.c (GAME-002) rebuilt in original shape, with the command record's `+0x60` link merged

Issue #570, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-002 is the original's `AITask.c`: the AI task (command record) pool,
the two global task lists, the task-type and priority name tables, and the
per-object task chains (text `00401cb0`–`004065f0`). It was relocated
verbatim into `src/game/aitask.cpp` (#541). This change rebuilds it as the C
unit [`src/game/aitask.c`](../../src/game/aitask.c), with the boundary header
[`aitask.h`](../../src/game/aitask.h) and the unit-private imports header
[`aitask_imports.h`](../../src/game/aitask_imports.h). The port's
implementation moves, unannotated, to the port tail
[`aitask_port.cpp`](../../src/game/aitask_port.cpp).

It also does the follow-up that [finding 565](565-entity-link-pair-2fc-300-merge.md)
left blocked on a frozen header: the command record's `+0x60` link is merged
to the original's single link, with the owner's option 2 (no frozen-header
exception; `cmd_for_aitask.hpp` is not edited).

"Before" is the *match baseline* of #584 (`1dddd451`), on which this change
is stacked (#581 GAME-023 part 1 → #583 GAME-021 part 2 → #584 link mode).
"After" is this change, from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, 109 functions, 17,804 bytes) | 29.46% | **91.31%** |
| Exact functions in the unit | 17 | **96 of 109** (plus 2 effective matches) |
| Mapped functions in the unit | 95 | 109 |
| Exact functions, game-only / full scope | 895 / 1,095 | 974 / 1,174 |
| Accuracy, full / game-only (`binary-progress.json`) | 69.83% / 66.74% | **71.58% / 68.69%** |
| Exact-function code bytes | 98,353 | 105,485 |
| `opt_ref` annotated removed, full / game-only | 1,272 / 1,235 | 1,304 / 1,267 |
| `opt_ref` exact removed, full / game-only | 546 / 517 | 586 / 557 |
| Reccmp lint ledger | 1,476 | 1,445 |

The unit map lists 94 functions. Fifteen more are written and annotated,
mapped for the first time unless noted:

- `00404250` and `00404280`, which Ghidra merged into `004041e0` (both
  were already annotated);
- thirteen enumeration callbacks and one pool walker that the unit pushes or
  calls but Ghidra left as labels: `004021f0`, `00402700`, `00402a70`,
  `00402ac0`, `00402bb0`, `00402c20`, `00402c70`, `00402ca0`, `00402d30`,
  `00403a30`, `00403ad0`, `00403f10`, `00403f80` (the purge comparator), and
  `00406540` (`EntityPoolCallbackSearch`).

Byte weights are the original's code bytes; an unmapped function counts as 0%.

| Address | Function | Bytes | Before | After |
| --- | --- | ---: | ---: | ---: |
| `00401cb0` | `aitask_cmd_kind_by_name` | 63 | 93.10 | **100.00** |
| `00401cf0` | `aitask_cmd_registry_init_target` | 781 | 36.96 | **100.00** |
| `00402000` | `aitask_cmd_mark_matching` | 84 | 37.50 | **100.00** |
| `00402060` | `aitask_cmd_set_kind` | 42 | 51.85 | **100.00** |
| `00402090` | `priority_entry_order_value_set` | 16 | 100.00 | **100.00** |
| `004020a0` | `priority_entry_state_set` | 16 | 100.00 | **100.00** |
| `004020b0` | `priority_entry_state_get` | 12 | 100.00 | **100.00** |
| `004020c0` | `aitask_handle_array_release_and_reset` | 54 | 37.04 | **100.00** |
| `00402100` | `aitask_cmd_flush_all` | 97 | 72.73 | **100.00** |
| `00402170` | `aitask_level_no_gather_object_setup` | 105 | 94.29 | **100.00** |
| `004021e0` | `engine_memory_allocate` | 14 | 100.00 | **100.00** |
| `004021f0` | `aitask_callback_004021f0` | 18 | unmapped | **100.00** |
| `00402210` | `engine_memory_free` | 14 | 100.00 | **100.00** |
| `00402220` | `aitask_cmd_tick_global_delta` | 236 | 51.09 | **100.00** |
| `00402310` | `aitask_cmd_global_list_purge` | 503 | 17.86 | **100.00** |
| `00402510` | `aitask_terrain_cell_command_create` | 226 | 44.19 | **100.00** |
| `00402600` | `aitask_entity_record_array_apply_zero_state` | 107 | 19.30 | **100.00** |
| `00402670` | `aitask_entity_command15_validated_record_queue` | 70 | 92.59 | **100.00** |
| `004026c0` | `aitask_entity_deferred_work_schedule` | 56 | 70.00 | **100.00** |
| `00402700` | `aitask_callback_00402700` | 29 | unmapped | **100.00** |
| `00402720` | `aitask_cmd_create_task` | 118 | 24.56 | **100.00** |
| `004027a0` | `aitask_bobbin_clear_delay` | 81 | 26.56 | **100.00** |
| `00402800` | `aitask_entity_command14_batch_issue` | 79 | 100.00 | **100.00** |
| `00402850` | `aitask_cmd_minion_companion` | 58 | 100.00 | **100.00** |
| `00402890` | `aitask_task_type23_create_for_category_mask` | 200 | 58.82 | **100.00** |
| `00402960` | `aitask_entity_kind_command_issue` | 219 | 60.99 | **100.00** |
| `00402a40` | `aitask_coordinate_pair_callback_dispatch` | 41 | 81.82 | **100.00** |
| `00402a70` | `aitask_callback_00402a70` | 43 | unmapped | **100.00** |
| `00402aa0` | `aitask_argument_callback_dispatch` | 19 | 66.67 | **100.00** |
| `00402ac0` | `aitask_callback_00402ac0` | 27 | unmapped | **100.00** |
| `00402ae0` | `aitask_terrain_cell_refresh_dispatch_current` | 71 | 88.46 | **100.00** |
| `00402b30` | `aitask_coordinate_callback_dispatch_current` | 41 | 19.35 | **100.00** |
| `00402b60` | `aitask_coordinate_callback_dispatch_by_mode` | 71 | 25.00 | **100.00** |
| `00402bb0` | `aitask_callback_00402bb0` | 111 | unmapped | **100.00** |
| `00402c20` | `aitask_callback_00402c20` | 64 | unmapped | 31.37 |
| `00402c70` | `aitask_callback_00402c70` | 43 | unmapped | **100.00** |
| `00402ca0` | `aitask_callback_00402ca0` | 46 | unmapped | **100.00** |
| `00402cd0` | `aitask_cmd_cancel_ref` | 85 | 20.34 | **100.00** |
| `00402d30` | `aitask_callback_00402d30` | 19 | unmapped | **100.00** |
| `00402d50` | `aitask_kind_nine_cell_command_issue` | 225 | 26.17 | **100.00** |
| `00402e40` | `aitask_cell_command_type13_issue` | 118 | 45.87 | **100.00** |
| `00402ec0` | `aitask_entity_conditional_task_schedule` | 102 | 91.43 | **100.00** |
| `00402f30` | `aitask_entity_threshold_progress_event_queue` | 60 | 40.00 | **100.00** |
| `00402f70` | `aitask_entity_target_command_queue` | 53 | 78.57 | **100.00** |
| `00402fb0` | `aitask_cell_command_type11_issue` | 116 | 52.34 | **100.00** |
| `00403030` | `aitask_cell_command_type3_issue` | 115 | 30.51 | **100.00** |
| `004030b0` | `aitask_coordinate_pair_event_queue` | 44 | 35.90 | **100.00** |
| `004030e0` | `aitask_cmd_validated_record_queue` | 162 | 18.87 | **100.00** |
| `00403190` | `aitask_object_event_type2_queue` | 68 | 100.00 | **100.00** |
| `004031e0` | `aitask_active_entity_list_contains_code` | 143 | 56.34 | **100.00** |
| `00403270` | `aitask_selection_drag_area_command_create` | 176 | 55.12 | **100.00** |
| `00403320` | `aitask_object_command_task_create` | 148 | 45.93 | **100.00** |
| `004033c0` | `aitask_object_attached_record_allocate` | 46 | 86.36 | **100.00** |
| `004033f0` | `aitask_coordinate_pair_task_record_queue` | 60 | 100.00 | **100.00** |
| `00403430` | `aitask_task_registry_remove_type0_entry_by_pair` | 99 | 24.62 | **100.00** |
| `004034a0` | `aitask_scalar_task_record_queue` | 50 | 100.00 | **100.00** |
| `004034e0` | `aitask_task_pair_record_enqueue` | 50 | 100.00 | **100.00** |
| `00403520` | `aitask_task_record_kind10_queue` | 50 | 100.00 | **100.00** |
| `00403560` | `aitask_task_registry_remove_type10_entries_for_entity` | 84 | 31.11 | **100.00** |
| `004035c0` | `aitask_cmd_create_move` | 77 | 48.00 | **100.00** |
| `00403610` | `aitask_cmd_create_gated` | 64 | 38.10 | **100.00** |
| `00403650` | `aitask_entity_task_batch_append` | 102 | 43.30 | **100.00** |
| `004036c0` | `aitask_cmd_create_target` | 60 | 73.68 | **100.00** |
| `00403700` | `aitask_cmd_cancel_by_coords` | 71 | 96.55 | **100.00** |
| `00403750` | `aitask_cmd_command_kind_record_issue` | 188 | 51.69 | **100.00** |
| `00403810` | `aitask_terrain_cell_record_count_update` | 176 | 46.51 | 76.83 |
| `004038c0` | `aitask_timed_task_record_initialize_and_link` | 78 | 100.00 | **100.00** |
| `00403910` | `aitask_terrain_cell_record_counts_prune` | 132 | 84.48 | **100.00** |
| `004039a0` | `aitask_task_record_create_and_scan_grid` | 42 | 85.00 | 94.12 (effective 100) |
| `004039d0` | `aitask_task_record_kind11_timed_queue` | 52 | 100.00 | **100.00** |
| `00403a10` | `aitask_task_callback_dispatch` | 19 | 66.67 | **100.00** |
| `00403a30` | `aitask_callback_00403a30` | 27 | unmapped | **100.00** |
| `00403a50` | `aitask_entity_drain_command_chain` | 118 | 19.67 | **100.00** |
| `00403ad0` | `aitask_callback_00403ad0` | 20 | unmapped | **100.00** |
| `00403af0` | `aitask_entity_task_state_update_after_cleanup` | 67 | 77.78 | **100.00** |
| `00403b40` | `aitask_entity_pending_task_flag_clear` | 19 | 24.24 | **100.00** |
| `00403b60` | `aitask_entity_activity_records_refresh` | 153 | 80.99 | **100.00** |
| `00403c00` | `aitask_entity_activity_record_remove_by_type` | 260 | 100.00 | **100.00** |
| `00403d10` | `aitask_entity_command_unlink_by_kind` | 465 | 21.21 | 99.39 |
| `00403ef0` | `aitask_pending_record_callback_dispatch` | 19 | 10.87 | **100.00** |
| `00403f10` | `aitask_callback_00403f10` | 27 | unmapped | **100.00** |
| `00403f30` | `aitask_ai_idle_chain_record_attach` | 47 | 33.33 | **100.00** |
| `00403f60` | `aitask_task_record_kind14_remove_if_head` | 29 | 27.78 | **100.00** |
| `00403f80` | `aitask_callback_00403f80` | 169 | unmapped | **100.00** |
| `00404030` | `aitask_cmd_link_head` | 55 | 59.26 | **100.00** |
| `00404070` | `aitask_cmd_link_tail` | 35 | 55.56 | **100.00** |
| `004040a0` | `aitask_entity_task_queue_flush` | 81 | 32.79 | **100.00** |
| `00404100` | `aitask_command_queue_sweep_relink` | 131 | 16.92 | 58.82 |
| `00404190` | `aitask_cmd_chain_walk` | 31 | 68.97 | **100.00** |
| `004041b0` | `aitask_entity_task_chain_contains_kind` | 39 | 35.29 | 93.33 (effective 100) |
| `004041e0` | `aitask_entity_command_chain_head_remove` | 110 | 21.51 | **100.00** |
| `00404250` | `aitask_cmd_pool_timer_delta_apply` | 33 | 31.11 | **100.00** |
| `00404280` | `aitask_cmd_entity_command_chain_tick` | 2872 | 10.46 | 79.12 |
| `00404e00` | `aitask_object_path_submit_to_grid_target` | 193 | 17.65 | 95.95 |
| `00404ed0` | `aitask_check_object_candidate` | 56 | 86.36 | 94.74 |
| `00404f10` | `aitask_object_task_target_filter_find` | 49 | 18.37 | **100.00** |
| `00404f50` | `aitask_object_filter_matches_action_target` | 103 | 23.53 | **100.00** |
| `00404fc0` | `aitask_place_command_eligibility_check` | 2445 | 7.33 | 98.41 |
| `00405950` | `aitask_pending_command_timer_fire_scan` | 696 | 11.52 | 98.17 |
| `00405c10` | `aitask_pending_command_entity_assignment_tick` | 1868 | 13.37 | 61.82 |
| `00406360` | `aitask_entity_record_attach_and_relink` | 113 | 20.75 | **100.00** |
| `004063e0` | `aitask_pending_record_entity_reference_clear` | 31 | 18.75 | **100.00** |
| `00406400` | `aitask_cmd_pending_record_copy` | 51 | 24.00 | **100.00** |
| `00406440` | `aitask_cmd_alloc` | 64 | 49.06 | 60.87 |
| `00406480` | `aitask_cmd_free` | 62 | 75.00 | **100.00** |
| `004064c0` | `aitask_cmd_pool_grow` | 114 | 23.53 | **100.00** |
| `00406540` | `aitask_entity_pool_callback_search` | 131 | unmapped | **100.00** |
| `004065d0` | `renderer_global_dword_get` | 6 | 100.00 | **100.00** |
| `004065e0` | `render_surface_secondary_get` | 6 | 100.00 | **100.00** |

Effective matches: `004039a0` and `004041b0` differ only in operand naming
that `match_function.py` reports as an effective match.

### What is left

| Address | Score | What is left |
| --- | ---: | --- |
| `00403d10` | 99.39 | the original side does not name `0043f5b0` (`entity.cpp` carries its marker as a comment) |
| `00404fc0` | 98.41 | adapter call targets (below), the `0x800000` constant, one register in the `0x16` branch |
| `00405950` | 98.17 | two x87 ordering ties in the distance calculation; six source shapes and the permuter changed nothing |
| `00404e00`, `00404ed0` | 95.95, 94.74 | adapter call targets only |
| `00404280` | 79.12 | `case 0`'s arrived block is placed before the route block whatever the source order, so case code is not shared (about 30 instructions); `dx` is kept in a register where the original spills it; about 20 calls go through adapters. A 400-second permuter run found nothing |
| `00403810` | 76.83 | block order and a register swap; it keeps a `goto` (62.34% without it) |
| `00405c10` | 61.82 | register allocation (the original keeps `best` in a stack slot) and the placement of the "best found" block; about 20 variants and a 300-second permuter run |
| `00406440` | 60.87 | the original tests the free list in `eax` and reloads it into `esi`; ours keeps it in `esi` and forms `lea edx,[esi+0x64]`. Nine variants; the permuter had no candidates |
| `00404100` | 58.82 | register choice only (`ebx`/`edi`/`esi`); five shapes, no permuter candidates |
| `00402c20` | 31.37 | the original reloads the task's block position for the highlight clear; MSVC keeps it from the compare |

## How the conversion is done

### The port tail

The port's command subsystem is a different contract: the port's `CmdRec`
(declared in the frozen `cmd_for_aitask.hpp`) carries a port-only tail
(`grant_act`, `extra`, the GetTool route), its own pool with a live-slot
sidecar, hook tables and NULL guards that the tests pin. As in findings 556,
560, 564 and 567, it stays a port: `aitask.cpp` is renamed
`aitask_port.cpp`, every `// FUNCTION`/`// GLOBAL` annotation becomes a
`/* Port model of 0x... */` comment, and the file is a `tail` entry in the
link-order list. Native callers and tests keep calling it under the port's
names, so native behaviour is unchanged apart from the merge (below).

The unit's functions are named `aitask_` plus the port name. Seven keep the
port's name and serve native callers too, because their contract and state
are the same (the port definitions are deleted):
`priority_entry_order_value_set`, `priority_entry_state_set`,
`priority_entry_state_get` (`00402090`–`004020b0`), `engine_memory_allocate`,
`engine_memory_free` (`004021e0`, `00402210`), `renderer_global_dword_get`
and `render_surface_secondary_get` (`004065d0`, `004065e0`).
`priorities.c`'s exact `0045d4f0` calls the first three.

### State

AITask.c's `.bss` (`004b31c8`–`004b8064`) is one `static AITask_Globs
aiTaskGlobs`, laid out as the original: the pool blocks, free list and level,
the task-type and priority name tables, the priority values (`004b32e8`), the
two global list heads (`004b3354`, `004b3358`), the two 50-entry candidate
arrays and their counts, the 4,800-entry command-count table, the priority
states (`004b7ff4`) and the flags word (`004b8060`).

The priority values and states are shared live with the port: priorities.c
sets them through the shared accessors and the port's scheduler reads them.
The port tail and `cmd.cpp` reach them through two `// PORT-ONLY` accessors
at the end of `aitask.c`, behind `#define s_behaviour` /
`#define s_priority_state` placed after the frozen includes; the frozen
headers' declarations of those names are now never defined or used. A first
version kept the two arrays as separate C globals under the port's names. That
cost three exact matches (`00401cb0` ends its loop at the address after the
names, `00401cf0` let MSVC schedule the `rep stosd` fill elsewhere, and
`00403910` ends its loop at `&commandCounts[4800]`); the single struct
restores all three, with no other score moving. The port tail's naked-asm
registry initialiser, which named `s_behaviour` in `__asm`, is dropped for
its C body, which now also builds under MSVC.

`aitask.c` also holds `AITaskObject`, a view of the original object
(LegoObject) at its Win32 offsets. MSVC 5.0 uses a float field as an x87
memory operand in a compare only when it is read as a struct member, not
through an offset cast: `00405950` went from 93.4% to 97.7% on this alone.

### Callees and callers

- **`extern "C"` in place** on the callees' existing declarations in
  `ai.hpp`, `entity.hpp` (including three `unported_ledger_stubs` members),
  `level.hpp`, `cmd.hpp`, `cmd_shared.hpp`, `port_loop_ports.hpp`,
  `crt/crt.hpp` and `gods98/view.hpp` (under `#ifdef __cplusplus`).
- **Frozen headers are not edited.** `idle_register` (`00407450`) and
  `cmd_operation_group_append_object` (`00408ce0`) are declared in frozen
  `cmd_for_*.hpp` headers, so an `extern "C"` pre-declaration sits directly
  before each frozen include in `bubbles.cpp`, `lava_erosion.cpp`,
  `placement.cpp` (twice) and `tooltip.cpp`.
- **Adapters.** 26 thin unannotated `// PORT-ONLY:` original-ABI adapters at
  the end of `aitask_port.cpp`, for callees whose port definitions are
  namespaced ledger stubs in `legoobject.cpp` or have another contract. The
  call to an adapter is the usual remaining operand difference.
- **`00431570` (Lego_GetMap)** takes no arguments. Its definition, now in
  `lego_unit.c` (#583), keeps the port's two ignored cell arguments, and
  `lego_unit.h` prototypes it so. The C matching build calls it through a
  cast to an unprototyped function type, which still emits a direct call and
  pushes no arguments, as the original does; the native C++ compile goes
  through `lego.cpp`'s adapter. This keeps `00403c00` exact.
- **GAME-023 callees.** `aitask.c` includes #581's
  `legoobject_port_aliases.h` first, so its calls name `legoobject_unit.c`'s
  functions in the matching build. Three GAME-023 queries that header does
  not alias (`00438a60`, `00438ae0`, `00438bf0`, whose port entries are
  namespaced ledger stubs) get the same matching-build-only renaming at the
  top of `aitask_imports.h`.
- **Callers in other C units.** `aitask_port_aliases.h` is the same kind of
  matching-build-only alias header for this unit: 28 port names and three
  port-tail adapters (`terrain_cell_command_create_adapter`,
  `cmd_mark_matching_adapter`, `cell_command_type13_issue_original`) map to
  the unit's names. It is included first in `lego_unit.c`,
  `legoobject_unit.c` and `priorities.c`. Native builds are unchanged. It
  keeps four exact functions that call AITask.c: `004302b0` and `00430320`
  (`lego_unit.c`), `00431140` (`lego_unit.c`) and `0043a850`
  (`legoobject_unit.c`), and lifts about twenty more of their callers back
  to or above their baseline.
- **`interface_unit.c`** (GAME-020) calls twelve unit functions through
  `interface_callees.h`; the eight declarations there are renamed in place
  to the unit's names (the count is unchanged). Natively the interface unit
  is not the live interface (finding 564), so behaviour is unchanged. Without
  this, the exact `0041f640` and `0041f690` fell to 91.67% and 93.33%.

## The command record: one `+0x60` link

### Layout before and after

The original record (`AITask`, 0x68 bytes; finding 10) has one link at
`+0x60`. It serves the entity chain (entity `+0x2f0`) and both global lists
(`004b3354`, the head list, and `004b3358`, the second list). The free-list
link is `+0x64`.

| Offset | Original (`AITask`, `aitask.h`) | Port `CmdRec` before | Port `CmdRec` after |
| --- | --- | --- | --- |
| `+0x50` | `field_50`: `timeGetTime()` stored by the allocator `0x406440`; the purge comparator `0x403f80` sorts flag-`0x100` records on it | `global_next`: port-only global-list link | `global_next`: declared by the frozen header, no longer a link; read only by the port comparator as the `+0x50` sort key, never written |
| `+0x60` | `next`: entity chain **and** global lists | `next`: entity chain only | `next`: entity chain and global lists |
| `+0x64` | `nextFree`: pool free list | `grant_act` (port; carries the free link on x86) | unchanged |

`CmdRec` is declared in the frozen `cmd_for_aitask.hpp`, which is not edited.
The merge is done in the code that uses it:

- every port read and write of `global_next` as a link, in `aitask_port.cpp`
  (32) and `cmd.cpp` (6), now uses `next`;
- `cmd_raw_head_insert`/`cmd_raw_tail_insert` no longer write two links (they
  wrote `global_next = head; next = NULL`); they write `next = head`;
- `cmd_pending_record_copy` clears `next` only;
- `cmd_global_next` returns `next`.

The unit's own declaration of the original layout, `AITask` in `aitask.h`,
has the one link. The port's `CmdRec` keeps its port-only tail (`grant_act`,
`extra`, the GetTool route) and stays the port tail's type.

### Exclusivity proof

One link is safe only if no record is ever on an entity chain and on a global
list at the same time. Every writer and reader of the two links in the port
was traced (`aitask_port.cpp`, `cmd.cpp`, `ai.cpp`, `bubbles.cpp`,
`legoobject.cpp`, `entity.cpp` and the tests); no other file touches them.

**A record enters a global list** only through:

| Path | Record | Why it is on no entity chain |
| --- | --- | --- |
| `cmd_link_head` (`0x404030`), `cmd_link_tail` (`0x404070`) from the producers in `0x402170`–`0x403f60` | fresh from `cmd_alloc` | `cmd_alloc` pops a free record and zeroes it; nothing links it before the call |
| `cmd_link_head`/`cmd_link_tail` from `entity_drain_command_chain` (`0x403a50`) | a record taken from the entity's chain | `next` is read first, the record's link is cleared, and the loop ends by setting the chain head to NULL, so the whole chain is dissolved |
| `cmd_raw_head_insert`/`cmd_raw_tail_insert` from `command_queue_sweep_relink` (`0x404100`) | a record removed from the chain | the chain is rebuilt from the kept records only (each kept record's link is cleared before it is kept); the moved record is not among them |
| `cmd_raw_*_insert` from `entity_command_chain_head_remove` (`0x4041e0`) | the chain head | the head is unlinked first (`cmd_head = head->next; head->next = NULL`) |
| `cmd_link_head(copy)` in `cmd_validated_record_queue` (`0x4030e0`) | a fresh copy | fresh allocation |
| `level_no_gather_object_setup` (`0x402170`) | fresh | fresh allocation; it already prepended through `next`, as the original does |
| the purge relink in `cmd_global_list_purge` (`0x402310`) | records already on that list | it reorders one list |

**A record enters an entity chain** only through:

| Path | Record | Why it is on no global list |
| --- | --- | --- |
| the tail appends (`tail->next = rec` or `cmd_head = rec`) in `0x402f70`, `0x403270`, `0x403320`, `0x4033c0`, `0x4035c0`, `0x403610`, `0x403650`, `0x4036c0`, the `0x404280` move record and `cmd_chain_append` | fresh from `cmd_alloc` | fresh allocation |
| `ai_idle_chain_record_attach` (`0x403f30`) | fresh | fresh allocation |
| `entity_record_attach_and_relink` (`0x406360`) | a global-list record | it is unlinked from the list first (`previous->link = rec->link`, or the head moves), and its link is cleared before it becomes the chain head; or a fresh copy is attached instead (flag `0x40`) |
| `pending_command_timer_fire_scan` (`0x405950`) | a second-list record | the same: unlinked from `004b3358` and cleared first, or a fresh copy |
| `cmd_chain_purge` (port helper) | chain records | it reorders one chain |

**Removal and freeing.** Every `cmd_free` of a listed record unlinks it from
its one structure first (`0x402310`, `0x403430`, `0x403560`, `0x403d10`,
`0x403a50`, `0x404100`, `0x4041e0`, `cmd_chain_purge`, `ai.cpp`'s
`finish_head`). `cmd_global_unlink`, which `ai.cpp`, `cmd_chain_purge` and
`0x403d10` call on chain records, only walks the two global lists and finds
nothing there for a chain record.

**Readers.** The chain walkers (`cmd_chain_walk`, `0x4041b0`, `bubbles.cpp`,
`legoobject.cpp`, `ai.cpp`'s per-entity pass) start at an entity's head; the
global walkers start at `004b3354` or `004b3358`. Since membership is
exclusive, each walker sees only its own structure.

So no port path puts a record on both; the merge was made.

### Port behaviour that follows from the merge

These are not separate gameplay changes; each follows from there being one
link:

- **`ai_tick`'s global pass** (`ai.cpp`) walks `cmd_global_head()` with
  `cmd_rec_next`. Before the merge that accessor returned the chain link,
  which is NULL for a global-list record, so the pass saw only the head
  record. It now walks the whole list, which is what its comment describes.
  Its comment that move and target records sit on both structures was wrong
  and is corrected.
- **`level_no_gather_object_setup`** (`0x402170`) prepends its record to the
  second list through `+0x60`, as the original does. Before the merge every
  other walker of that list followed `global_next`, so the list behind that
  record was invisible to them. It is now one list.
- **The purge comparator** (`cmd_purge_compare_default`) reads the `+0x50`
  word, as `0x403f80` does. Before the merge that word was the port's global
  link, so flag-`0x100` records were ordered by link address. Nothing in the
  port writes `+0x50` now (the port's allocator does not store
  `timeGetTime()`), so they tie and fall through to the priority compare.

Native ctest passes (37/37, synthetic fixtures) before and after the merge.

## Entropy and drops

`make reccmp-check` against #584's baseline: 81 improved, 14 newly mapped,
18 below baseline. **No exact match is lost**, and no dropped function
changes instruction count.

Every drop is a C++ caller outside the unit (`placement.cpp`, `lego.cpp`,
`legoobject.cpp`) that still calls the port tail where the original calls
an AITask.c function. Its annotation moved to the unit, so the call operand
no longer maps (operand-only; these callers' declarations have C++ linkage
or another contract, so the alias header cannot rename them). The baseline is
refreshed for them; none is owner-reviewed:

| Address | Function | Before | After | Original call |
| --- | --- | ---: | ---: | --- |
| `00408e50` | `entity_repeated_resource_instances_create` | 35.29 | 33.33 | `00403750` |
| `00409150` | `simtick_selection_group_clear` | 51.20 | 48.00 | `00403700`, `00402720` |
| `00409600` | `ledger_stub_00409600_TerrainDescriptorTilesResetAndFree` | 16.87 | 15.26 | `00403700`, `00402720` |
| `00432c90` | `terrain_cell_state_update_current` | 20.55 | 19.18 | `00402b60` |
| `0043af30` | `creature_spawn_from_emergence` | 20.03 | 19.35 | `00403f30` |
| `0043bc40` | `objective_sidecar_append_gate` | 39.43 | 39.16 | `00403560` |
| `0043c6b0` | `entity_activity_counters_advance` | 44.73 | 43.88 | `004026c0` |
| `0043f6a0` | `entity_damage_apply` | 66.67 | 65.22 | `00402f30` |
| `0043f7d0` | `ledger_stub_0043f7d0_EntityEnergyLowTargetRouteCommandTick` | 28.83 | 28.47 | `00402f70`, `004041b0` |
| `00441940` | `entity_action_sequence_reset` | 45.21 | 44.52 | `00403c00`, `004040a0`, `00403af0` |
| `00441b30` | `entity_state_reset` | 41.70 | 41.26 | `00403d10`, `00403f60` |
| `00444620` | `entity_enter_event21_state_once` | 90.32 | 87.10 | `00403f30` |
| `00447780` | `entity_clear_linked_object_state` | 28.57 | 24.49 | `00403d10` |
| `00447b30` | `ledger_stub_00447b30_EntityActivityTick` | 26.21 | 25.24 | `00403f60` |
| `00447c40` | `activity_register_bookkeeping` | 19.80 | 17.82 | `00403f30` |
| `0044ae50` | `lego::random_kind_six_scatter_spawn` | 32.35 | 31.62 | `00402720` |
| `0044b250` | `ledger_stub_0044b250_PairingActivityRaiseAndMutualLink` | 73.61 | 72.22 | `00403f30` |
| `004526e0` | `selection_candidate_filter_active` | 77.12 | 76.27 | `00403b60` |

They recover as their units are converted and call the unit directly.

**`opt_ref` (finding 576).** In #584's `/OPT:REF` relink, 32 more annotated
functions and 40 more exact ones are removed as unreferenced. The unit's
functions are reached only from callers that are themselves unreferenced
there, or not at all, because native callers and most matching-build
callers still call the port tail; the port models they replace stay
referenced. This shrinks as callers move to the unit.

## Follow-ups

- Switch the callers above to the unit as their units are converted
  (`placement.cpp` with GAME-007, `legoobject.cpp` with GAME-023 part 2).
- The port's allocator does not store `timeGetTime()` at `+0x50`, so the
  port's purge comparator ties every flag-`0x100` record (above). A native
  gameplay follow-up, if wanted, belongs under the `port` label.

## Validation

Stacked on #584 (`1dddd451`):

- `make reccmp-check`: the 18 drops above (accepted, not owner-reviewed),
  no exact match lost.
- `make match-progress` (schema-4 `opt_ref` block), `make
  unit-data-offsets` (GAME-002: gaps-only, no unknowns) and `python3
  tools/translation_units.py build`, generated files committed;
  `tools/binary_progress_inputs.txt` rewritten with
  `tools/binary_progress_hash.py --write-manifest`.
- `make tools-test` (no `--allow-growth`): passes; the lint ledger shrinks
  from 1,476 to 1,445.
- `make winapi-link-check`: OK.
- Native: `ctest`: 37/37 pass (synthetic fixtures).
- C subset: `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only src/game/aitask.c` reports nothing.
