# Finding 526 — the entity's command head, pair, link and path-count members sit at their original offsets

Issue #468. [Finding 522](522-entity-flag-words-and-owner-layout.md) placed
the entity flag words, `+0x33c`/`+0x340` and the `+0x36c` owner. Its survey
rerun ranked the `EntitySlot` members that the 32-bit matching build still
reads at the wrong offset. This finding moves the ten members that issue #468
names, as one planned set. It records the evidence, the new layout and the
effect on the matching build.

## Summary

- In the 32-bit layout, ten `EntitySlot` members now sit at their original
  offsets: `wp_count` (`+0x28`), `cmd_head` (`+0x2f0`), `pos_pair_2f4`,
  `pos_pair_2f8`, `link_2fc`, `attach_304`, `carry_link_308`,
  `indexed_record_slot_330`, `progress_344` and `pair_link_3c0`.
- The struct size (`0x678`) is unchanged, and so are `stats_record`
  (`+0x334`) and finding 522's members. No storage, padding or alignment is
  added (ADR 0006 decision 5). Twelve port-only members move to the freed
  slots or to slots next to them. None of them was at an original offset
  before.
- **No function becomes exact.** 53 functions improve and 5 drop. The
  game-only mean goes from 50.96% to 51.03%, and the full mean from 55.13% to
  55.19%. All five drops are `drift-review`. Two of them change instruction
  count by −1: one lost `fxch`, and one alignment `mov edi, edi`.
- After the move, finding 520's survey still finds **one** strictly
  offset-only function, `cmd_cancel_by_coords` (the command record's `+0x60`
  link). None of the ten members appears in the survey any longer.

## Evidence

The original offsets come from two independent sources.

1. **Paired listings.** These come from the Reccmp report on `main`
   (`a776838e`) and from the report after the move. After the rebase onto
   `68e9526c`, the move changes the same 58 scores by the same amounts. The table counts the functions where at
   least one aligned (`both`) instruction uses the retail displacement after
   the move and did not before. In those positions the rebuild had read the
   member at its port offset.
2. **Original pseudocode** ([full corpus](../decompiles/full/LEGORR.full.c)).
   Readers and writers use the same entity pointer that the function's other
   entity fields come from.

| Member | Before → after (32-bit) | Aligned functions | Original readers and writers (examples) |
| --- | --- | ---: | --- |
| `wp_count` | `+0x2b8` → `+0x28` | 12 | `EntityRouteAdjacentStepAppend` `0x4468c0`: `*(int *)(param_1 + 0x28) = *(int *)(param_1 + 0x28) + 1`, indexing the `+0x24` buffer by `+0x28 * 0x14`; `0x4477d0`, `0x448800`, `0x449240`: `*(param_1 + 0x24) + -3 + *(int *)(param_1 + 0x28) * 0x14` |
| `cmd_head` | `+0xe4` → `+0x2f0` | 9 | `CmdCreateTarget` `0x4036c0`: `*(int *)(param_1 + 0x2f0) = iVar1`; `0x403af0`: `piVar1 = (int *)param_1[0xbc]`; `0x403c00`: `*(int *)(param_1 + 0x2f0) = piVar5[0x18]` (unlink through the record's `+0x60`) |
| `pos_pair_2f4` | `+0x168` → `+0x2f4` | 5 | `0x449010`: `param_1[0xbd] = (int)(float)local_8`; `CreatureSpawnFromEmergence` `0x43af30`: `piVar4[0xbd] = …` |
| `pos_pair_2f8` | `+0x16c` → `+0x2f8` | 3 | `0x449010`: `param_1[0xbe] = (int)(float)local_4`; `0x449240`: `*(float *)(param_1 + 0x2f8) = (float)param_2[1]` |
| `link_2fc` | `+0x160` → `+0x2fc` | 14 | `EntityProgressAdvanceBy25` `0x447730`: `*(undefined4 *)(param_1 + 0x2fc) = 0`; `LinkedEntityRouteFlagClear` `0x43f100`: `*(int *)(param_2 + 0x2fc) + 0x3ec`; `EntityActionSequenceReset` `0x441940`, `EntityTeardown` `0x437540` |
| `attach_304` | `+0x138` → `+0x304` | 6 | `SelectionCandidateFilterActive` `0x4526e0`: `if ((int *)param_1[0xc1] != 0) param_1 = (int *)param_1[0xc1]`; `EntityDamageApply` `0x43f6a0`; `0x43bbc0`: `piVar1[0xc1] = 0` |
| `carry_link_308` | `+0x140` → `+0x308` | 4 | `EntityDetachCarriedObjectAndResetState` `0x43bbc0`: `piVar1 = (int *)param_1[0xc2]; … param_1[0xc2] = 0`; `0x448800`: `if (**(int **)(param_1 + 0x308) == 8)`; `PendingTypeValueSlotSelect` `0x407a10` |
| `indexed_record_slot_330` | `+0x314` → `+0x330` | 3 | `EntityBuildingIndexSet` `0x469c10`: `if (param_2 != param_1[0xcc]) … param_1[0xcc] = param_2`; `0x404ed0`: `FUN_00404f10(*param_1, param_1[1], param_1[0xcc])`; `0x43a850` |
| `progress_344` | `+0x15c` → `+0x344` | 2 | `0x447730`: `*(float *)(param_1 + 0x344) = fVar1`, then `= *(param_1 + 0x340)`; `0x43f7d0`: `if ((float)piVar1[0xd0] < (float)piVar1[0xd1]) piVar1[0xd1] = piVar1[0xd0]`; `EntityCreateByType` `0x437d00`: `piVar1[0xd1] = 0x42c80000` |
| `pair_link_3c0` | `+0x418` → `+0x3c0` | 2 | `0x44b250`: `*(int *)(param_1 + 0x3c0) = param_2; *(int *)(param_2 + 0x3c0) = param_1`; `EntityDirectionalActivityStart` `0x447950`: `param_1 = (int *)param_1[0xf0]`; `EntityStateReset` `0x441b30`: `*(int *)(param_1 + 0x3c0) + 0x3e4` |

The aligned counts come from a difflib-aligned version of finding 520's
pairing, so they are larger than the issue's table, which counted only
equal-length changed groups. Every member has aligned accesses in at least
two functions, and at least three readers or writers in the pseudocode.

**Not placed.**

- **Retail `+0x300`** is claimed by two port members:
  `route_partner_300`, the reciprocal step-command partner, and `field_300`,
  the spatial filter's pointer. Placing either would leave the other at a
  wrong offset. Like the command record's `+0x60` link, they need merging,
  not a layout move. `attached_children` keeps the slot.
- **`wp_index` (`+0x2c`) and the `+0x24` route buffer** are next to
  `wp_count` but outside the issue. `+0x2c` holds `y`, and the port keeps an
  inline array (`wp`) plus `wp_dynamic` where the original has one buffer
  pointer. They are listed in the survey below.

## Change

`EntitySlot` (`src/game/game.hpp`) is a port-native struct, so only its
32-bit layout is constrained. Each target slot was held by a port-only
member, which moves into the slot the target left, or close to it:

| Member | Before | After | Note |
| --- | ---: | ---: | --- |
| `wp_count` | `+0x2b8` | **`+0x28`** | original offset |
| `x` | `+0x28` | `+0x2b8` | into `wp_count`'s old slot |
| `cmd_head` | `+0xe4` | **`+0x2f0`** | original offset |
| `auxiliary_state_388` | `+0x2f0` | `+0xe4` | into `cmd_head`'s old slot |
| `pos_pair_2f4` | `+0x168` | **`+0x2f4`** | original offset |
| `auxiliary_slot_38c` | `+0x2f4` | `+0x168` | into `pos_pair_2f4`'s old slot |
| `pos_pair_2f8` | `+0x16c` | **`+0x2f8`** | original offset |
| `indexed_record_flags` | `+0x2f8` | `+0x16c` | into `pos_pair_2f8`'s old slot |
| `link_2fc` | `+0x160` | **`+0x2fc`** | original offset |
| `indexed_record_flag_index` | `+0x2fc` | `+0x160` | into `link_2fc`'s old slot |
| `attach_304` | `+0x138` | **`+0x304`** | original offset |
| `attached_child_count` | `+0x304` | `+0x138` | into `attach_304`'s old slot |
| `carry_link_308` | `+0x140` | **`+0x308`** | original offset |
| `attached_scene_object` | `+0x308` | `+0x140` | into `carry_link_308`'s old slot |
| `attached_direction[3]` | `+0x328` | `+0x30c` | displaced, port-only |
| `ddiscene_owner_token` | `+0x30c` | `+0x328` | displaced, port-only |
| `attached_scene_slot_index` | `+0x310` | `+0x32c` | displaced, port-only |
| `indexed_record_slot_330` | `+0x314` | **`+0x330`** | original offset |
| `progress_344` | `+0x15c` | **`+0x344`** | original offset |
| `action_consume_counters[6]` | `+0x344` | `+0x348` | displaced by 4 |
| `payload_handle_04` | `+0x35c` | `+0x15c` | into `progress_344`'s old slot |
| `scene_slot_cache[12]` | `+0x39c` | `+0x384` | displaced, port-only |
| `scene_slot_owner_24_handles[3]` | `+0x3d0` | `+0x3b4` | displaced, port-only |
| `pair_link_3c0` | `+0x418` | **`+0x3c0`** | original offset |
| `payload_owned_blocks[5]` | `+0x384` | `+0x3c4` | displaced, port-only |
| `scene_slot_generation` | `+0x3cc` | `+0x3d8` | displaced, port-only |
| `payload_flags_70` | `+0x398` | `+0x418` | into `pair_link_3c0`'s old slot |

Why these choices:

- **`+0x28`.** The only member there was `x`, and the original reads no
  position at `+0x28`. Nothing treats `x`, `y` and `z` as one vector, so `x`
  swaps with `wp_count` and `y`/`z` stay put.
- **`+0x330`.** It lay inside the 12-byte `attached_direction`
  (`+0x328`–`+0x333`). Moving the vector into the run that the two scene
  words and `indexed_record_slot_330`'s old slot leave (`+0x30c`–`+0x317`)
  takes three moves. Shifting the four `attached_direction_record_type*`
  pointers would take five. Two other placements were tried with
  `make match-function`. One kept the four pointers together; the other put
  the vector 8-byte aligned at `+0x318`. Both gave the same scores for the
  three readers that drop, so the three-move plan is kept.
- **`+0x344`.** `action_consume_counters` (24 bytes) starts there. It moves
  up 4 bytes, and the one dword it pushes out (`payload_handle_04`) takes
  `progress_344`'s old slot.
- **`+0x3c0`.** It lay inside the 48-byte `scene_slot_cache`. No arrangement
  of the `+0x370`–`+0x3cb` members alone leaves `+0x3c0` free, so the window
  runs to `+0x3df`. `visual_resource_handles`, `type_component_handle` and
  `flags_390` keep their offsets. The cache and the `+0x24` owner handles
  fill `+0x384`–`+0x3bf`, and the owned blocks and generation fill
  `+0x3c4`–`+0x3db`. One dword (`payload_flags_70`) takes
  `pair_link_3c0`'s old slot.
- Each displaced member was at a non-original offset before and is at a
  non-original offset now, so none is worse off. Some displaced members
  have names that cite an original offset (`auxiliary_state_388`, for
  example). Those offsets are still not where the members sit, before or
  after.

`src/game/entity.cpp` asserts all ten offsets on 32-bit builds (MSVC 5.0 and
i386 GCC), next to finding 520's and 522's. It also asserts
`sizeof(EntitySlot) == 0x678`.

**Raw-offset sites.** Some code addresses the entity as a retail byte image:

- the naked `entity_activity_record_remove_by_type` (`0x403c00`:
  `[ebp + 2f0h]`);
- the naked `stats_instance_word_at_bc` (`0x469b50`: `[eax + 330h]`);
- the Windows-only `pending_type_value_slot_select` (`0x407a10`: `+0x2f0`,
  `+0x308`);
- `pooled_record_allocate_and_reset`, which writes `-1.0` to a pool record's
  `+0x2f4`/`+0x2f8`.

On the 32-bit build, these now reach `cmd_head`, `indexed_record_slot_330`,
`carry_link_308` and the position pair. Before the move they reached
`auxiliary_state_388`, `attached_direction[2]`, `attached_scene_object` and
`auxiliary_slot_38c`/`indexed_record_flags`. The two naked bodies are
byte-exact and keep their scores. Native builds do not compile them.

## Matching results

Measured after rebasing on `main` at `68e9526c`, which includes #469's
Advisor relocation. `make reccmp-check` reports
**53 improved and 5 below baseline**. This change refreshes the baseline for
the 5 drops.

| | Before | After |
| --- | ---: | ---: |
| game-only mean | 50.96% | 51.03% |
| game-only exact | 568 | 568 |
| full mean | 55.13% | 55.19% |
| full exact | 672 | 672 |
| `.text` bytes identical at original addresses | 17,801 | 17,932 |
| rebuilt `.text` bytes | 934,836 | 934,676 |

The 58 functions whose score changed move by +2.65 points on average. The
largest gains are:

| Function | Before | After |
| --- | ---: | ---: |
| `entity_progress_advance_by_25` | 48.48% | 72.73% |
| `entity_placement_commands_build_once` | 24.37% | 35.53% |
| `entity_detach_carried_object_and_reset_state` | 52.78% | 61.11% |
| `entity_linked_route_flag_clear` | 61.54% | 69.23% |
| `entity_update_and_unregister_at_tile` | 54.32% | 61.90% |
| `cmd_chain_walk` | 62.07% | 68.97% |

No function becomes exact. Finding 520's survey found no strictly
offset-only function among these members, so this was expected. What
remains in their readers is code shape, not layout. For example,
`entity_progress_advance_by_25` now differs from the original only in x87
compare order (`fcom [ecx + 0x340]` against `fld; fcomp st(1)`) and the
matching branch test.

**At-address bytes.** The rebuilt `.text` shrinks by 160 bytes. The
`wp_count` accesses now use 8-bit displacements, and the `x` accesses now use
32-bit ones. Functions after the first change therefore move. The 131-byte
rise in at-address identity comes from this shift. Before and after, that
code is at chance level (ADR 0006). Before the rebase onto #469's Advisor
relocation, the same shift lowered the count by 453 bytes, from 18,260 to
17,807. Unit-relative identity (#464) is unchanged.

### Entropy review

| Address | Function | Before | After | Classifier | Instructions |
| --- | --- | ---: | ---: | --- | ---: |
| `0x4424f0` | `entity_position_to_terrain_cell` | 26.09% | 25.53% | `drift-review` | 25 → 25 (+0) |
| `0x4428a0` | `entity_world_position_set` | 14.04% | 13.95% | `drift-review` | 81 → 81 (+0) |
| `0x444680` | `entity_step_movement_vector_resolve` | 5.23% | 4.88% | `drift-review` | 276 → 276 (+0) |
| `0x444fb0` | `ledger_stub_00444fb0_EntityTurnToVectorActivityIssue` | 26.04% | 25.56% | `drift-review` | 179 → 178 (−1) |
| `0x449880` | `ledger_stub_00449880_BlockedCellApproachWeightWrite` | 25.12% | 24.27% | `drift-review` | 121 → 120 (−1) |

No function's source changed. Instruction counts are for the rebuilt
listing.

- **`0x4424f0`, `0x4428a0`.** The rebuilt listings differ only in member
  displacements and the branch targets that move with them. `x` goes from
  `+0x28` to `+0x2b8`. In `0x4428a0`, `attached_scene_object` goes from
  `+0x308` to `+0x140` and `attached_direction` from `+0x328` to `+0x30c`.
  The longer `x` encoding shifts the branch targets, and Reccmp's diff then
  pairs one more original instruction differently. The instruction
  multisets are identical.
- **`0x444680`.** Only displacements differ. `wp_count` now reads
  `[edi + 0x28]`, where the original also reads `+0x28`, and
  `attached_direction` reads `+0x30c`/`+0x310`. The instruction multiset is
  identical. The score falls because the diff realigns around the new
  matches.
- **`0x444fb0`.** It reads `attached_direction` throughout. Besides the
  displacements, MSVC 5.0 drops one `fxch st(1)` before a `faddp`. The
  original has that `fxch`, so this is x87 register-stack drift. It is the
  same effect finding 520 recorded for this function when the vector moved
  the other way (+1 `fxch`). The two alternative vector placements above
  give the same score.
- **`0x449880`.** Only displacements differ (`wp_count`, twice). The two
  shorter encodings shrink the function by 6 bytes. Its trailing jump table
  then needs one fewer alignment `mov edi, edi`, and that is the −1.

**Owner review:** all five drops are `drift-review`, because displacement
changes are not among the differences the classifier masks. Two change the
instruction count by −1: `0x444fb0` (an x87 `fxch`) and `0x449880` (padding).
They are listed here for the owner.

**Listings without a score change.** In `match-listing-digests.txt`, 106
game listings change: the 58 scored ones and 48 others. No `full`-only
(gods98 or library) listing changes.

## Native check

Native builds lay `EntitySlot` out differently anyway (64-bit pointers), and
nothing serialises it. Native ctest passes 35/35, and 37/37 with
`TEST_WADS_DIR`.

`entity_layout_real` gains `layout_probe_window_2f0`. It uses the
Toolstation, RockMonster and Pilot created from the shipped `Lego.cfg`, and
drives each moved member through a reader that the move aligned:

- `+0x28` through `entity_route_adjacent_step_append` (`0x4468c0`): an
  adjacent step appends and bumps the count, and a non-adjacent one does not;
- `+0x2f0` through `cmd_create_target` (`0x4036c0`) and `cmd_chain_walk`;
- `+0x344`, `+0x340` and `+0x2fc` through `entity_progress_advance_by_25`
  (`0x447730`): it advances by 25, clears the link, then clamps to the bound;
- `+0x2fc` through `entity_linked_route_flag_clear` (`0x43f100`);
- `+0x304` through `selection_candidate_filter_active` (`0x4526e0`): an
  ineligible attached object rejects the otherwise eligible Toolstation;
- `+0x308`, `+0x2f4` and `+0x2f8` through
  `entity_clear_nested_state_8_fields` (`0x4487c0`), with and without the
  `+0x3e0` bit `0x400`;
- `+0x330` through `entity_animation_state_set` (`0x439cd0` →
  `0x469c10`): level 1 selects the Toolstation's next `0x150`-byte Stats row,
  and level 0 restores the first;
- `+0x3c0` through `entity_directional_activity_start` (`0x447950`): with
  `+0x3e4` bit 2 set, the activity lands on the paired RockMonster;
- writing all ten members leaves the Pilot's Stats binding (RouteSpeed 1.1)
  and each other's values intact.

The probe passes on both the old and the new layout.

## Survey rerun: remaining offset-only differences

This uses finding 520's method on the new report. Changed groups of equal
length are paired row by row, and stack slots are excluded. **One** function
remains strictly offset-only:

| Record | Recomp → orig | Functions |
| --- | --- | --- |
| command record chain link | `0x50` → `0x60` | `cmd_cancel_by_coords` |

None of the ten members appears in the wider count any more. The remaining
wrong displacements, by number of functions:

| Recomp → orig | Functions | Record |
| --- | ---: | --- |
| `0x50` → `0x60` | 3 | command record: the port-only `global_next` occupies `+0x50` |
| `0x82c` → `0x4a84` | 3 | `TerrainSurfaceGrid` change count |
| `0x198` → `0x34`, `0x4` → `0xc` | 2 each | save record (`sav_record_ptr_target`), (unattributed) |
| `0x154` → `0x348`, `0x144` → `0x324`, `0x14c` → `0x348`, `0x13c` → `0x7c` | 1 each | `action_consume_counters_initialized`/`carry_ref_348`, `carry_state_324`, `entity_payload_teardown` |

When rows are aligned with difflib, which also catches displacements inside
changed groups of unequal length, the next entity members are:

| Recomp → orig | Functions | Member |
| --- | ---: | --- |
| `0x2b0` → `0x24` | 6 | `wp_dynamic`: the original's route buffer pointer at `+0x24` |
| `0x28` → `0x24` | 4 | the same buffer, where the port reads `wp_count` before indexing |
| `0x144` → `0x324`, `0x14c` → `0x348`, `0x124` → `0x2d4`, `0x130` → `0x364`, `0x2e4` → `0x3dc` | 2 each | `carry_state_324`, `carry_ref_348`, `activity_timer`, `wake_target`, `activity_counter_3dc` |

The route buffer is the largest remaining entity group. It needs the inline
`wp` array and `wp_dynamic` merged into one pointer at `+0x24`, with
`wp_index` at `+0x2c`. That is a data-model change, not a layout move, and
it is left to a follow-up. The command record's `+0x60` link and the
`TerrainSurfaceGrid` count stay open, as finding 522 recorded.
