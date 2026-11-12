# Finding 565 — the entity link pair `+0x2fc`/`+0x300` merged to the original layout

Issue #555, part of #388 and ADR 0005, and a prerequisite for converting
GAME-023 (`LegoObject.c`, `src/game/legoobject.cpp`;
[finding 551](551-relocation-game-023-legoobject.md)).
[Finding 526](526-entity-command-pair-link-window-layout.md) placed `link_2fc`
at `+0x2fc`, but left retail `+0x300` unplaced because two port members
claimed it. This change merges those two members into one, at the original
offset. It records the original's accesses, the merge and the gate result,
and notes the command record's `+0x60` link as the follow-up.

## Summary

- **The original has one `+0x300` member.** It is a pointer to an entity: the
  reciprocal of `+0x2fc`. Six writers store it or clear it, and two callbacks
  read it. The port modelled it as two members:
  - `route_partner_300` (`void*`), which was written and never read;
  - `field_300` (`uintptr_t`), which was read by the route-marker search and
    had no producer.
- **The merge.** The two members become `EntitySlot* link_300` at `+0x300`.
  `link_2fc` changes from `uintptr_t` to `EntitySlot*`, the original's
  pointer type. 32-bit codegen is unchanged, and about 50 `(EntitySlot*)`
  and `(uintptr_t)` casts go.
- **Two port writes went to the wrong member.** Each is corrected to match
  the original:
  - `0x004471b0` stored the reciprocal in `attach_304`;
  - `0x00437940` stored its link in `route_partner_300`, where the original
    stores `+0x2fc`.
- **Gate:** two rises and one drop. **No exact match is lost**, and the
  exact counts are unchanged (game-only 751, full 951). The game-only mean
  stays 60.05%, and the full mean goes from 63.830% to 63.831%. The drop is `entity_at`, a port stand-in: its index
  multiply follows `sizeof(EntitySlot)`.
- **No frozen header is involved.** `EntitySlot` is in `game.hpp`. No
  `_for_` or `_from_` header names any of the members.
- **The `+0x60` follow-up is blocked on a frozen header.** `CmdRec` is
  declared in `cmd_for_aitask.hpp`; see "The command record's `+0x60` link".

## The original's accesses

Two sweeps of `reference/LegoRR.exe` found every memory operand with
displacement `0x2fc` or `0x300` and a non-`esp` base:
- a flow-following disassembly of each manifest function;
- a linear sweep of `.text`, which also covers callbacks between manifest
  entries.

Each hit was checked against the pseudocode in
`docs/decompiles/full/LEGORR.full.c`. `tools/unit_accesses.py` covers
absolute data addresses only, so it does not see member offsets. The scratch
scan is described here and not kept as a tool.

**`+0x300`: ten accesses. All of them name an entity, and all of them pair
with `+0x2fc`.**

| Instruction | Function | Access | Port before → after |
| --- | --- | --- | --- |
| `0x4390c9` | route-marker search callback (inside `0x438f60`'s range) | read: `mask & 0x80`, state bit 2, **`[esi+0x300] == 0`** | `entity_route_marker_query_match`: `field_300 == 0u` → `link_300 == NULL` |
| `0x439f39` | `EntityAttachedObjectListDetach` `0x439e70` | `record+0x304 = 0; record+0x300 = 0` | `record->field_300 = 0u` → `record->link_300 = NULL` |
| `0x43dcc1` | `0x43c6b0` (activity tick) | `(+0x2fc)->+0x300 = edi; +0x2fc = edi` | not ported: the port has no body for this branch |
| `0x43ec74` | `0x43c6b0` | write | not ported |
| `0x43ef22` | `SimulationEntityQueueFlush` `0x43eea0` | `+0x2fc = 0; +0x304 = 0; +0x300 = 0` | not in source |
| `0x4419e1` | `EntityActionSequenceReset` `0x441940` | `(+0x2fc)->+0x300 = 0; +0x2fc = 0` | `linked->route_partner_300 = NULL` → `linked->link_300 = NULL` |
| `0x441d82` | `EntityStateReset` `0x441b30` | `(+0x2fc)->+0x300 = 0; +0x2fc = 0` | `linked->route_partner_300 = NULL` → `linked->link_300 = NULL` |
| `0x445363` | collision callback inside `EntityPathStepVectorNormalize` `0x4451e0` | skip the candidate if it is the entity's `+0x304` or `+0x300` | not ported |
| `0x4471f4` | `TryCreateLinkedEntityAtPosition` `0x4471b0` | `+0x2fc = target; target+0x300 = source` | **`linked->attach_304 = entity` → `linked->link_300 = entity`** |
| `0x448e7d` | `CardinalStepCommandIssueAndLink` `0x448c90` | `+0x2fc = target; target+0x300 = source` | `target->route_partner_300 = entity` → `target->link_300 = entity` |

`0x4390c9` is the reader that the port's `field_300` modelled. It is the same
`mask & 0x80` predicate: candidate state bit 2 set and `+0x300` zero. The
writers are the ones that `route_partner_300` modelled. So the two port
members are one original field.

**`+0x2fc`: 97 accesses in 39 functions.** Type-only edits
(`uintptr_t` → `EntitySlot*`) apply everywhere the port already had the
access. The table lists each port function that touches the pair. "Type"
means the access already pointed at `+0x2fc`, and only the casts or the
`0u` → `NULL` literal change.

| Original | Port function | File | Before → after |
| --- | --- | --- | --- |
| `0x42e020` | `object_list_load_file` | `lego.cpp` | `link_2fc = (uintptr_t)target` → `= target` (type) |
| `0x437510` callback in `0x4374f0` | `entity_linked_route_clear_callback` | `legoobject.cpp` | type |
| `0x437540` | `entity_teardown` | `legoobject.cpp` | type (×5) |
| `0x437940` | `deferred_olist_entity_create_and_link` | `legoobject.cpp` | **`created[i]->route_partner_300 = target` → `created[i]->link_2fc = target`** (the original writes `+0x2fc`) |
| `0x439a20` | `entity_gated_step_command_issue` | `legoobject.cpp` | type |
| `0x439e70` | `entity_attached_object_list_detach` | `legoobject.cpp` | type (×2); `field_300` → `link_300` (above) |
| `0x43a300` | `entity_deposit_activity_gate` | `legoobject.cpp` | type |
| `0x43c4c0` | `entity_finish_state_1a` | `legoobject.cpp` | type (×2) |
| `0x43f100` | `entity_linked_route_flag_clear` | `legoobject.cpp` | type (×2) |
| `0x43f190` | `entity_spawn_request_queue_process` | `legoobject.cpp` | type |
| `0x43fc20` | `ledger_stub_0043fc20_RecordPairPoseCopyAndActivityRaise` | `legoobject.cpp` | type |
| `0x43fff0` | `entity_route_marker_setup` | `entity.cpp` | type |
| `0x440130` | `entity_update_and_unregister_at_tile` | `legoobject.cpp` | `= (uintptr_t)link` → `= (EntitySlot*)link` (the parameter stays `void*`) |
| `0x4401b0` | `ledger_stub_004401b0_TerrainCellToWorldPosition` | `legoobject.cpp` | type |
| `0x4403d0` | `ledger_stub_004403d0_LevelDebugCellRetargetDispatch` | `legoobject.cpp` | type |
| `0x441700` | `entity_route_setup_and_marker_associate` | `legoobject.cpp` | type |
| `0x441940` | `entity_action_sequence_reset` | `legoobject.cpp` | type (×7); `route_partner_300` → `link_300` |
| `0x441b30` | `entity_state_reset` | `legoobject.cpp` | type (×6); `route_partner_300` → `link_300` |
| `0x4438b0` | `entity_path_step_advance_and_arrival_check` | `legoobject.cpp` | type (×5) |
| `0x4440a0` | `linked_entity_flag_and_command_update` | `legoobject.cpp` | type |
| `0x4471b0` | `entity_try_create_linked_at_position` | `legoobject.cpp` | type; `attach_304` → `link_300` (above) |
| `0x4473b0` | `ledger_stub_004473b0_RouteToCellCommandIssue` | `legoobject.cpp` | type |
| `0x4474f0` | `ledger_stub_004474f0_LinkedEntityProximityResponse` | `legoobject.cpp` | type (×2) |
| `0x4475c0` | `ledger_stub_004475c0_EntityProgressAndNegativeDeltaHandler` | `legoobject.cpp` | type (×2) |
| `0x447730` | `entity_progress_advance_by_25` | `legoobject.cpp` | type |
| `0x4477d0` | `ledger_stub_004477d0_PathStepLookaheadEntryWrite` | `legoobject.cpp` | type (×2) |
| `0x448c90` | `ledger_stub_00448c90_CardinalStepCommandIssueAndLink` | `legoobject.cpp` | type; `route_partner_300` → `link_300` |
| `0x448eb0`, `0x4490a0`, `0x449430`, `0x449ac0`, `0x44b500` | the step-command issuers | `legoobject.cpp` | type (one store each) |
| `0x44ade0` | `entity_kind_one_random_spawn_trigger` | `legoobject.cpp` | type (×2) |
| — (port helpers) | `entity_type_four_linked_type_five_reset`, `entity_linked_reference_matches` | `legoobject.cpp` | type (×3) |
| — (port helpers) | `ai_action_route_frame_update` | `ai.cpp` | type (×2) |
| — (port helpers) | `cmd_place_target_nested_scene_pose_get`, `cmd_gettool_route_adopt` | `aitask.cpp` | type (×3) |
| — (port helpers) | `cmd_gettool_owner_forget`, `cmd_gettool_route_moving` | `cmd.cpp` | type (×3) |
| — (tests) | `layout_probe_window_2f0`, `progress_advance_test`; two `test_ai` cases | `test_entity.cpp`, `test_ai.cpp` | integer sentinels become entity pointers; the layout probe now also writes and checks `link_300` |

These original `+0x2fc` accessors have no port access yet. The port bodies
are partial or absent, and each is left to the conversion:
- `PlaceCommandEligibilityCheck` `0x404fc0`;
- `ObjectiveSidecarAppendGate` `0x43bc40`;
- the `0x43c6b0` activity tick (34 accesses);
- `SimulationEntityQueueFlush` `0x43eea0`;
- `EntityRouteToExplicitTarget` `0x43fe70`;
- `TypeFourEntityCallbackDispatch` `0x447900`.

## The change

`EntitySlot` (`src/game/game.hpp`). Only its 32-bit layout is constrained.

| Member | Before (32-bit) | After |
| --- | ---: | ---: |
| `link_2fc` | `+0x2fc`, `uintptr_t` | `+0x2fc`, `EntitySlot*` |
| `link_300` (new; merges the two below) | — | **`+0x300`** |
| `route_partner_300` | `+0xe8` | removed |
| `field_300` | `+0x134` | removed |
| `attached_children` (port-only) | `+0x300` | `+0xe8`, `route_partner_300`'s old slot |
| `activity_advance_overflow` (port-only, last member) | `+0x674` | `+0x134`, `field_300`'s old slot |

- Every other member keeps its offset. Only the last member moves, so the
  freed dword leaves the struct, which shrinks from `0x678` to `0x674`. No
  padding is added.
- `entity.cpp` asserts `link_300` at `+0x300` beside finding 526's members.
  The size assertion becomes `0x674`.

**Port behaviour changes caused by the merge.** These are not separate
gameplay work. Each one follows from the original having one field:
- `0x448c90` and `0x4471b0` now store the reciprocal in the same field that
  the route-marker search tests. Before the merge, `field_300` was always
  zero, so the `0x80` predicate matched every state-bit-2 candidate. Now an
  entity that another entity has linked to fails the predicate, as in the
  original.
- `0x4471b0` no longer writes `attach_304`. The original never stores
  `+0x304` there. `+0x304` is the attached object that
  `SelectionCandidateFilterActive` and `EntityDamageApply` follow.
- `0x437940` now stores the OList link in `link_2fc`, as `0x42e020` already
  did. Before, the link went to the write-only `route_partner_300`.

Native ctest passes: 37/37 with synthetic fixtures, and 39/39 with
`TEST_WADS_DIR`.

## Matching results

`make reccmp-check`, measured on #565's head `09594fe6` (finding 563, on
`main` `5fbba99a`). The result is the same as it was on `main` `63da6269`:
the same three functions move by the same amounts.

| Address | Function | Before | After | Note |
| --- | --- | ---: | ---: | --- |
| `0x441940` | `entity_action_sequence_reset` | 45.21% | 45.89% | its `+0x300` clear now uses the original displacement |
| `0x444fb0` | `ledger_stub_00444fb0_EntityTurnToVectorActivityIssue` | 25.56% | 26.04% | x87 drift (finding 538): no member it reads moved and its source is unchanged; the score returns to its value before finding 526 |
| `0x42ed70` | `entity_at` | 11.32% | **10.71%** | **drop**, `drift-review`, instructions +2. **Owner review** |

**The drop.** `entity_at` returns `&ctx->entity[tab][index]`, a port-only
table. The index multiply follows `sizeof(EntitySlot)`, and MSVC 5.0 builds
`* 0x674` with a longer `lea`/`shl`/`sub` chain than `* 0x678`. No source
changed. The original's `0x42ed70` does not index an entity table, so neither
size matches. The baseline is refreshed for this one drop.

**Why the writers barely move.**
- `0x4471b0` now emits the original's `mov [edi+0x300], esi`. The function
  stays at 41.98% because the port's null guards, its `__ftol` argument
  conversion and its `ENTITY_WAYPOINTS` select shift Reccmp's alignment, so
  the store is not credited.
- `0x448c90` (23.74%), `0x441b30` (42.15%), `0x439e70` (13.73%) and
  `0x437940` (17.87%) are unchanged for the same reason. The new
  displacement sits in code whose shape still differs.
- This is the expected order of work: with the layout right, the GAME-023
  conversion's original-shape bodies can match.

**Other measures:**
- 13 game listings change in `match-listing-digests.txt`; no `full`-only
  listing changes.
- `.text` identical bytes at original addresses go from 21,016 to 20,741.
  The rebuilt `.text` grows by 16 bytes, so later functions shift; that
  count is at chance level (ADR 0006).
- Run identity goes from 1,661 to 1,484 bytes. That is also at chance
  level: on `63da6269` the same shift raised it, from 1,684 to 1,780.

## The command record's `+0x60` link (follow-up, not changed here)

[Finding 10](10-entity-command-primitives.md) shows that the original
command record uses one link at `+0x60`:
- for the entity chain headed at entity `+0x2f0`;
- for the global lists `DAT_004b3354` and `DAT_004b3358`.

The port's `CmdRec` splits it:
- `next` at `+0x60` serves the entity chain;
- the port-only `global_next` at `+0x50` serves the global lists.

Finding 526's survey found `cmd_cancel_by_coords` strictly offset-only on
this (`0x50` → `0x60`), plus two other functions.
- `global_next` has 58 uses: 32 in `aitask.cpp`, 8 in `cmd.cpp`, 2 in
  `cmd.hpp` (`cmd_global_next`) and 16 in `test_cmd.cpp`.
- The original's `+0x50` is unrecovered. The port keeps no other member
  there.

**Blocker: a frozen header.** `CmdRec` is declared in
`src/game/cmd_for_aitask.hpp`. That header is a `_for_` header, written once
by the GAME-002 relocation (#496), and must not be edited. The merge needs
one of the following, decided by the owner:
1. move the `CmdRec` definition out of the frozen header into `cmd.hpp`
   (owner-approved exception to the frozen rule), and then merge
   `global_next` into `next`;
2. wait for the GAME-002 (`AITask.c`) or command-unit conversion, which
   rewrites the header's users anyway, and merge there.

**Semantic check needed before the merge.** One link field means a record
is never on an entity chain and a global list at the same time. The port's
two fields allow both. The follow-up must show that no port path does that:
`CmdLinkHead`/`CmdLinkTail` (`0x404030`/`0x404070`) against the chain
appenders. Otherwise one list can corrupt the other. The pool freelist link
(`+0x64`, `CMD_POOL_LINK_*`) is separate and not affected.
