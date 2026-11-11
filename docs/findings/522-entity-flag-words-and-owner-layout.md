# Finding 522 — the entity's flag words, +0x340 float and +0x36c owner sit at their original offsets

Issue #462. [Finding 520](520-entity-stats-record-pointer-layout.md) placed
the Stats record pointer at `EntitySlot +0x334` and surveyed the members
that the 32-bit matching build still reads at the wrong offset. This finding
moves the six it ranked highest, plus two neighbours whose original offsets
fall inside the same windows. It records the evidence, the new layout and the
effect on the matching build.

## Summary

- In the 32-bit layout, eight `EntitySlot` members now sit at their original
  offsets: `flags_3e0`, `flags_3e4`, `flags_3e8`, `flags_3ec`, `field_340`
  and `owner_36c` (the issue's six), and `flags_3f0` and `field_33c`, whose
  original offsets lie in the windows the move rearranges.
- The struct size (`0x678`) and `stats_record` (`+0x334`) are unchanged. No
  storage, padding or alignment is added (ADR 0006 decision 5). Nine
  displaced port-only members move to the slots the moved members left or to
  neighbouring slots. None of them was at an original offset before.
- **6 functions become exact.** 83 functions improve and 2 drop. The game-only
  mean goes from 50.76% to 50.97%, and the full mean from 54.94% to 55.14%.
  No function changes its instruction count.
- After the move, finding 520's survey finds **one** strictly offset-only
  function (`cmd_cancel_by_coords`, the command record's `+0x60` link). It
  found 7 before.

## Evidence

The original offsets come from two independent sources.

1. **Paired listings.** In the Reccmp report on `main` (`44734133`; unchanged at `b842219a`), the
   rebuild read each member at its port offset where the original's
   instruction at the same diff position used the retail offset. After the
   move, the two instructions are identical. The table counts the functions
   where at least one aligned (`both`) instruction now uses the retail offset
   that the rebuild missed before.
2. **Original pseudocode** ([full corpus](../decompiles/full/LEGORR.full.c)).
   Readers and writers use the same entity pointer that the function's other
   entity fields come from.

| Member | Before → after (32-bit) | Aligned functions | Original readers and writers (examples) |
| --- | --- | ---: | --- |
| `flags_3e0` | `+0xec` → `+0x3e0` | 37 | `EntityFlagsMask107C33DEOr1C00203AnyTest` `0x43baf0`: `*(uint *)(param_1 + 0x3e0) & 0x107c33de`; `EntityAcceptsTargetForTaskCheck` `0x439bd0`; `EntitySharedEligibilityCheck` `0x4385b0` |
| `flags_3e4` | `+0xf4` → `+0x3e4` | 22 | `0x43baf0`: `*(uint *)(param_1 + 0x3e4) & 0x1c00203`; `EntityStateReset` `0x441b30` (16 aligned accesses, e.g. `param_1 + 0x3e4 & 0xfffffffd`) |
| `flags_3e8` | `+0xf0` → `+0x3e8` | 31 | `EntityType3Flag04000000Test` `0x4436f0`: `param_1[0xfa] & 0x4000000`; `EntityCreateByType` `0x437d00`: `piVar1[0xfa] = 0` then `\|= 0x441190`; `EntityKindRecordUpdateDispatch` `0x438460` |
| `flags_3ec` | `+0xf8` → `+0x3ec` | 18 | `SelectionCandidateAlreadySelectedOrMarked` `0x452690`: `*(byte *)(param_1 + 0x3ec) & 2`; `EntityTeardown` `0x437540`; `EntityActivityToggle` `0x4373d0` |
| `field_340` | `+0x154` → `+0x340` | 17 | `EntityFlag40000ThresholdCompare` `0x437cc0`: `fVar2 < *(float *)(param_1 + 0x340)`; `EntityCreateByType`: `piVar1[0xd0] = 0x42c80000` (100.0); `EntityDamageApply` `0x43f6a0` |
| `owner_36c` | `+0x13c` → `+0x36c` | 8 | `EntityMatchesCurrentSelectionOrOwner` `0x4295c0`: `*(int *)(param_1 + 0x36c) != DAT_00556160`; `SelectionDragAreaCommandCreate` `0x403270`; `EntityTypeTwoEventDispatch` `0x43b820` |
| `flags_3f0` | `+0x104` → `+0x3f0` | 1 | `CurrentLevelStateFlagsContainMask` `0x41f720` (aligned): `*(uint *)(iVar1 + 0x3f0) & param_1`; `EntityCreateByType`: `piVar1[0xfc] = 0x20` (type 1 and type 4 arms); `OsfAdd` `0x459190`: `*piVar1 = param_1[0xfc]` |
| `field_33c` | `+0x41c` → `+0x33c` | 1 | `EntityConditionalTypeFiveSpawn` `0x4470d0` (aligned): `*(undefined4 *)(param_1 + 0x33c) = 0`; `ActivityTimeoutAccumulateAndRaise` `0x4440f0`: `param_1[0xcf] = (int)(param_2 + (float)param_1[0xcf])` |

The issue's six each have at least 8 aligned functions. The two extra
members have one aligned function each and at least two more readers or
writers in the pseudocode. Their port names already carried the original
offset.

## Change

`EntitySlot` (`src/game/game.hpp`) is a port-native struct, so only its
32-bit layout is constrained. The plan rearranges three windows around the
targets (`+0x338`–`+0x35b`, `+0x35c`–`+0x36f` and `+0x3cc`–`+0x3f3`). It fills
the slots the moved members leave (`+0xec`–`+0x107`, `+0x13c`, `+0x154` and
`+0x41c`) with members of the same total size, so no other member moves. The
only member at an original offset before, `stats_record`, stays put:

| Member | Before | After | Note |
| --- | ---: | ---: | --- |
| `field_33c` | `+0x41c` | **`+0x33c`** | original offset |
| `field_340` | `+0x154` | **`+0x340`** | original offset |
| `action_consume_counters[6]` | `+0x33c` | `+0x344` | displaced, port-only |
| `action_consume_counters_initialized` | `+0x354` | `+0x154` | into `field_340`'s old slot |
| `payload_state_8c` | `+0x358` | `+0x41c` | into `field_33c`'s old slot |
| `payload_resource_chains[3]` | `+0x364` | `+0x360` | displaced, port-only |
| `owner_36c` | `+0x13c` | **`+0x36c`** | original offset |
| `payload_aux_handle_7c` | `+0x360` | `+0x13c` | into `owner_36c`'s old slot |
| `scene_slot_owner_24_handles[3]` | `+0x3e8` | `+0x3d0` | displaced, port-only |
| `flags_390` | `+0xfc` | `+0x3dc` | displaced |
| `flags_3e0` | `+0xec` | **`+0x3e0`** | original offset |
| `flags_3e4` | `+0xf4` | **`+0x3e4`** | original offset |
| `flags_3e8` | `+0xf0` | **`+0x3e8`** | original offset |
| `flags_3ec` | `+0xf8` | **`+0x3ec`** | original offset |
| `flags_3f0` | `+0x104` | **`+0x3f0`** | original offset |
| `scene_slot_owner_1c_handles[3][2]` | `+0x3d0` | `+0xec` | into the flag words' old slots |
| `command_mode_404` | `+0x100` | `+0x104` | displaced by 4 |

Why these choices:

- `+0x3e0` lay inside the 24-byte `scene_slot_owner_1c_handles`. The 20 bytes
  between `scene_slot_generation` (`+0x3cc`) and `+0x3e0` cannot be filled from
  that window's members, whose sizes are 4, 12 and 24. The 24-byte array
  therefore takes the flag words' old run (`+0xec`), and one dword
  (`flags_390`) comes the other way.
- `+0x3f0` was the only free slot after the flag words. The original has
  `flags_3f0` there, so it takes it. `flags_390` (original `+0x390`) goes
  to `+0x3dc`. The original keeps an activity counter at `+0x3dc`, but the
  port's counters are not moved here.
- `+0x33c` is likewise known (`field_33c`), so it moves from the struct's
  tail. That costs no extra move: `payload_state_8c` takes its old slot.
- Each displaced member was at a non-original offset before and is at a
  non-original offset now, so none is worse off.

`src/game/entity.cpp` asserts all eight offsets, and `stats_record`, on
32-bit builds (MSVC 5.0 and i386 GCC). No inline assembly addresses a moved
member. The `0fch`/`100h`/`0ech`/`0f0h` displacements in `entity.cpp`'s naked
functions index the Stats record, not the entity.

## Matching results

These are measured on `main` at `b842219a`: after #463's explicit-object
link and #464's layout metrics (the scores match `44734133`). `make
reccmp-check` reports **83 improved and 2 below baseline**. This
change refreshes the baseline for the 2 drops.

| | Before | After |
| --- | ---: | ---: |
| game-only mean | 50.76% | 50.97% |
| game-only exact | 562 | 568 |
| full mean | 54.94% | 55.14% |
| full exact | 666 | 672 |
| exact function code bytes | 44,226 (6.88%) | 44,517 (6.93%) |
| unit-relative identity (#464) | 36,283 (6.55%) | 36,574 (6.60%) |
| `.text` bytes identical at original addresses | 17,957 | 17,874 |

Unit-relative identity rises in two units: GAME-023 (815 → 939 bytes) and
GAME-027 (115 → 282 bytes).

The 85 functions whose score changed move by +5.33 points on average.

**Newly exact (6):** `entity_flag40000_threshold_compare` (`0x437cc0`),
`entity_flags_mask107c33de_or_1c00203_any_test` (`0x43baf0`),
`entity_type3_flag04000000_test` (`0x4436f0`),
`selection_contains_entity_active_target` (`0x452610`),
`selection_active_candidate_already_selected_or_marked` (`0x452690`) and
`selection_candidate_eligibility_check` (`0x452870`). These are the six
offset-only functions that finding 520's survey attributed to these members.

The largest gains are `ai_attached_entity_wake` (10.53% → 31.58%),
`entity_state_reset` (26.07% → 40.90%), `entity_activity23_state_transition`
(60.87% → 78.26%) and `entity_detach_carried_object_and_reset_state`
(36.11% → 52.78%).

**At-address bytes.** Every function keeps its address; the map's `.text`
contributions are identical. The 83-byte fall is in displacement bytes of
functions that are not at their original addresses. Before and after, that
code is at chance level (ADR 0006).

### Entropy review

| Address | Function | Before | After | Classifier | Instructions |
| --- | --- | ---: | ---: | --- | ---: |
| `0x444460` | `ledger_stub_00444460_FlaggedVectorStepCommandIssue` | 23.61% | 22.72% | `drift-review` | 291 → 291 (+0) |
| `0x449980` | `entity_activity_terrain_eligibility_transition` | 31.58% | 26.90% | `drift-review` | 95 → 95 (+0) |

Neither function's source changed.

- **`0x444460`.** It reads `flags_3e0` and `owner_36c`, and both now carry
  the original displacements (`0x3e0`, `0x36c`). Beyond that, MSVC 5.0
  schedules one x87 product differently: `fld st(0); fmul dword ptr [eax]`
  becomes `fld dword ptr [eax]; fmul st(1)`, with the same instruction
  count. This is x87 register-stack drift.
- **`0x449980`.** The rebuilt listing differs **only** in the
  displacements, five reads and writes of `flags_3e0` going from `0xec` to
  `0x3e0`. Each now equals the original's instruction. The score falls
  because Reccmp's diff realigns around the newly matching
  `test byte ptr [esi + 0x3e0], 0x40`. It then pairs the two `__ftol` calls and
  their pushes differently. The rebuilt code is strictly closer to the
  original.

The classifier labels both `drift-review` because displacement changes are
not among the differences it masks (registers, stack slots, branch targets).
This is expected for a layout move. **Owner review:** both drops are listed
here, and no function changes instruction count. One known classifier gap
remains: `name[i].field+N (offset)` operands keep the variable name. It does
not cause either label here. The only such operand,
`s_entity_path_motion_runtime[1000].samples+56` in `0x444460`, is identical
before and after.

**Listings without a score change.** With the per-compilation numbers in
anonymous-namespace symbols masked, 141 game listings change: the 85 scored
ones and 56 others. No `full`-only (gods98 or library) listing changes.
54 of the 56 differ only in moved members' displacements. In the other two,
`ledger_stub_00404fc0_PlaceCommandEligibilityCheck` and
`pending_command_entity_assignment_tick`, the two loads of a `test` swap
order. `flags_3f0` is one operand and a command record word the other. Both
keep their instruction count and score. Another 28 listings differ only in
those anonymous-namespace numbers, because every file that includes `game.hpp`
changed. #461's normalisation masks them in the gate.

## Native check

Native builds lay `EntitySlot` out differently anyway (64-bit pointers), and
nothing serialises it. Native ctest passes 35/35, and 37/37 with
`TEST_WADS_DIR`.

`entity_layout_real` (`entity_test --real-layout $TEST_WADS_DIR`) loads the
shipped `Lego.cfg`. It creates a Toolstation, a RockMonster and a Pilot
through `entity_create` and checks:

- each binds its Stats record;
- the type-4 (`0x40100`) and Pilot (`0x4481f5`) `+0x3e8` creation arms;
- each clause of `selection_candidate_eligibility_check`: the `+0x3e8` bit
  `0x100`, a negative `+0x340`, and the `+0x3e0`/`+0x3e4` reject bits;
- the `+0x3e8`, `+0x3e0`/`+0x3e4` and `+0x3ec` readers `0x4436f0`, `0x43baf0`
  and `0x452690`;
- `entity_flag40000_threshold_compare` against each type's shipped
  PainThreshold and Stats flag `0x40000`;
- that writing all eight moved members leaves the Pilot's Stats binding
  (RouteSpeed 1.1) and each other's values intact.

The probe passes on both the old and the new layout.

The probe does not check creation defaults of `+0x340` or `+0x3f0`. The
original's `EntityCreateByType` sets `+0x340` (and `+0x344`) to 100.0 and,
for types 1 and 4, `+0x3f0` to `0x20`; the port's `entity_create` does
neither. That is a behaviour gap outside this layout change, so the probe
sets `+0x340` itself rather than pin today's value.

**Raw-offset sites (not changed).** These functions address an entity as a
retail byte image, through `(char *)entity + 0x3e0`-style accesses, rather
than through the members:

- `entity_activity_records_refresh` (`0x403b60`: `+0x3e0`, `+0x324`);
- `entity_matches_current_selection_or_owner` (`0x4295c0`: `+0x36c`);
- gods98 `grid_cell_invalidate_and_clear_flags` (`0x466750`: `+0x3e0`,
  `+0x3e8`);
- `simtick.cpp`'s `bobbin_delayed` (`0x409a40`: `+0x340`, `+0x3e8`);
- `terrain_grid.cpp`'s unhooked paths of `0x40cf80`/`0x40d0a0` (`+0x3e4`).

The Windows-only `pending_type_value_slot_select` does too. On the 32-bit
build these now land on the intended members. In the native 64-bit layout
they hit unrelated members both before and after this change. For example,
`+0x3e0` was inside `payload_resource_chains` and is now inside
`action_consume_counters`. The move changes which unrelated bytes they reach.
Making them layout-independent changes native behaviour and the gods98/game
boundary, so it is left to a follow-up.

## Survey rerun: remaining offset-only differences

This uses finding 520's method on the new report. **One** function remains
strictly offset-only:

| Record | Recomp → orig | Functions |
| --- | --- | --- |
| command record chain link | `0x50` → `0x60` | `cmd_cancel_by_coords` |

The six members of this change no longer appear in the wider count. The
remaining wrong displacements, by number of functions:

| Recomp → orig | Functions | Record |
| --- | ---: | --- |
| `0x50` → `0x60` | 6 | command record: the port-only `global_next` occupies `+0x50` |
| `0xe4` → `0x2f0` | 6 | `EntitySlot.cmd_head` |
| `0x160` → `0x2fc` | 6 | `EntitySlot.link_2fc` |
| `0x138` → `0x304`, `0x168` → `0x2f4` | 4 each | `EntitySlot.attach_304`, `pos_pair_2f4` |
| `0x2b8` → `0x28` | 4 | `EntitySlot.wp_count` |
| `0x16c` → `0x2f8`, `0x82c` → `0x4a84`, `0x4` → `0xc` | 3 each | `pos_pair_2f8`, `TerrainSurfaceGrid` change count, (unattributed) |
| `0x314` → `0x330`, `0x140` → `0x308`, `0x15c` → `0x344`, `0x418` → `0x3c0` | 2 each | `indexed_record_slot_330`, `carry_link_308`, `progress_344`, `pair_link_3c0` |

The issue's optional items stay open:

- **Command record `+0x60`.** The original has one link word at `+0x60`.
  The port splits it into `global_next` (`+0x50`) and the entity-chain
  `next` (`+0x60`). Swapping them moves the wrong offset onto `next`'s
  readers, so it needs the two links merged, not a layout move.
- **`TerrainSurfaceGrid` change count at `+0x4a84`.** The port's record ends
  well before `+0x4a84`. Placing it needs a size change, which the issue
  excludes.

The `+0x2f0`–`+0x308` window (`cmd_head`, `link_2fc`, `attach_304`,
`pos_pair_2f4`/`2f8`, `carry_link_308`) is the next group of entity members.
It overlaps the port-only `indexed_record_*` and `attached_*` sidecars, so,
like the flag words, it needs a planned set of moves.
