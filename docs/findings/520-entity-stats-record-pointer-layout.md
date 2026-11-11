# Finding 520 — the entity reaches its Stats record through a pointer at +0x334

Issue #457. The MSVC permuter run in #428 found that most of its
register/order-only cohort was the `entity_stats_*` accessor family
(`0x469cb0`–`0x46a330`). They differed in one field offset, which no source
permutation can change. This finding records the original layout and the
change that makes the matching build use it. [Finding 457](457-entity-stats-configuration-and-record-accessors.md)
describes the loader and accessor family; this one adds the entity-side link.

## Summary

- In the original, an entity holds a **pointer** to its Stats record at
  entity **`+0x334`**. The record is not embedded. The level (upgrade) index
  that selects the record is at `+0x330`.
- A Stats record is **0x150 bytes**. Each object type and row has one record
  per level, in one allocation.
- The reconstruction already used the original's **field offsets inside the
  record**. Only the entity's pointer slot was wrong: `EntitySlot.stats_record`
  was at `+0x114` in the 32-bit layout, where the original has `+0x334`.
  The issue text has the two numbers the wrong way round: in the Reccmp diff
  the original reads `[eax + 0x334]` and the rebuild reads `[eax + 0x114]`.
- Moving `stats_record` to `+0x334` makes **44 functions exact**. The only
  drop is x87 drift of 0.01 points in one function, which this change accepts
  and lists for the owner. The struct size is unchanged and every other member
  keeps its offset, except the two members that were swapped into place
  (below).

## Evidence

**Accessors.** All 45 readers from `0x469cb0` to `0x46a330` in the
[full pseudocode](../decompiles/full/LEGORR.full.c) load one dword from the
entity and use it as a base:

```c
// EntityComponentFloatAt20 @ 00469CB0
return (float10)*(float *)(*(int *)(param_1 + 0x334) + 0x20);
// 0x469da0 returns the field's address, not its value
return *(int *)(param_1 + 0x334) + 0xa0;
```

The listing shows the same: `mov ecx, dword ptr [eax + 0x334]` then
`fld dword ptr [ecx + off]`. A second load through the dword means a pointer,
not an inline record.

**The writer.** `EntityBuildingIndexSet` (`0x469c10`) keeps the level index at
`+0x330` (`param_1[0xcc]`). It then points `+0x334` at that level's record:

```c
param_1[0xcc] = param_2;
param_1[0xcd] = *(int *)(*(int *)(&DAT_00502bd8 + *param_1 * 4) + param_1[1] * 4)
              + param_2 * 0x150;
```

`DAT_00502bd8` is the per-type table of per-row record arrays. `+0`
holds the type and `+4` the row.

**Static templates.** `EntityCreateByType` stores the address of a `.data`
template in `+0x334` (`piVar1[0xcd]`) for the types with no Stats entry:
`0x4a97a0` (type 10), `0x4a98f0` (type 8), `0x4a9a40` (type 9) and
`0x4a9b90` (types `0xc`–`0xe` and `0x10`–`0x13`). The templates are exactly
`0x150` apart. Every other type calls `EntityBuildingIndexSet(entity, 0)`.

**The loader.** `EntityStatsConfigLoad` (`0x4667e0`) allocates
`Levels * 0x150` bytes per row and stores that pointer in the table. It writes
each key into each level's record at a fixed offset, stepping `0x150` per
level. `RouteSpeed` goes to `+0x20`. The `CollRadius` block stores
`+0x6c`, `+0x70`, `+0x74`, `+0x78`, `+0x7c`, `+0x80`, `+0x84`, `+0xa8`,
`+0xc0`, `+0xc4`, `+0xc8` and `+0x8c` in one pass. These match the port's
key table (`s_entity_stats_field_bindings` in `src/game/entity_config.cpp`).
This change leaves that table alone.

| Record offset | Key (shipped `Lego.cfg`) | Accessor |
| ---: | --- | --- |
| `+0x20` | `RouteSpeed` | `0x469cb0` |
| `+0x24`–`+0x4c` | drill times by rock type, ×25 | `0x469cc0` (indexed) |
| `+0x6c` | `CollRadius` | `0x469d40` |
| `+0x70` | `AlertRadius` | `0x469d90` |
| `+0x84` | `EnergyDecayRate` | `0x469de0` |
| `+0x88` | `RepairValue` | `0x469f20` |
| `+0x98` | `AttackRadius` | `0x46a020` |
| `+0xc0`/`+0xc4` | `DrillSound`/`DrillFadeSound` | `0x469f90` |
| `+0xcc` | `NumOfToolsCanCarry` | `0x46a060` |
| `+0xe4` | `AwarenessRange` | `0x46a270` |
| `+0x10c`–`+0x11c` | `FreezerTime`, `FreezerDamage`, `PusherDist`, `PusherDamage`, `LaserDamage` | `0x46a310`…`0x46a2d0` |
| `+0x120`–`+0x13c` | `Flocks_*` | `0x46a1b0`…`0x46a210` |
| `+0x144`/`+0x148`/`+0x14c` | three flag words | `0x469e60`/`0x469e80`/`0x469ea0` |

## Change

`EntitySlot` (`src/game/game.hpp`) is a port-native struct, not a retail
image. Before this change, no member with a retail-offset suffix sat at its
retail offset in the 32-bit build. In the 32-bit layout, `+0x334` fell inside
`attached_direction[3]` (`+0x32c`–`+0x337`). The smallest change that puts
`stats_record` at `+0x334` keeps the size (`0x678`) and every other
member's offset. It moves three 4-byte-aligned members:

| Member | Before | After | Retail offset |
| --- | ---: | ---: | ---: |
| `stats_record` | `+0x114` | **`+0x334`** | `+0x334` |
| `type_operation_fallback_1c` | `+0x328` | `+0x114` | `+0x1c` |
| `attached_direction[3]` | `+0x32c` | `+0x328` | `+0x2ac` |

The two members swapped into place were not at their retail offsets before
and are not now. This change adds no storage, padding or alignment, so it is
not a scoring-only deviation under ADR 0006 decision 5. It is the first
`EntitySlot` member placed at its original offset.

The measurement compiles `offsetof` for every member with the matching
compiler. `src/game/entity.cpp` asserts `+0x334` on 32-bit builds (MSVC 5.0
and i386 GCC). The naked `entity_record_flag400_toggle_and_refresh`
(`0x46a330`) hard-coded the old slot as `[edx + 114h]`. It now reads
`[edx + 334h]` and is exact. No other inline assembly addresses the three
moved members.

Native builds lay `EntitySlot` out differently anyway (64-bit pointers).
Nothing serialises it or indexes it by byte offset, so member order has no
native effect.

## Matching results

These are measured on `main` at `2635b3c3`: after #455's non-incremental link,
#459's 2D drawing conversion and #461's symbolic-operand normalisation. `make reccmp-check` reports **45 improved
and 1 below baseline**; the one drop is reviewed below, and this change
refreshes the baseline for it.

| | Before | After |
| --- | ---: | ---: |
| game-only mean | 50.46% | 50.96% |
| game-only exact | 517 | 561 |
| full mean | 54.71% | 55.16% |
| full exact | 621 | 665 |
| exact function code bytes | 43,412 (6.76%) | 44,210 (6.88%) |
| `.text` bytes identical at original addresses | 18,455 | 18,443 |

The last row falls by 12 bytes because two `entity.cpp` functions each grow
by 2 bytes (below), which moves every later function in the file by 4
bytes. Code at original addresses is at chance level on both sides (ADR
0006).

**The cohort.** #428's 64-function cohort
(`docs/coverage/msvc-permuter-cohort.csv`) has 44 entries from `0x469cb0`
to `0x46a330`, not the 37 the issue counted. They include `ai_activity_cap`
(`0x46a060`, `ai.cpp`) and the naked `0x46a330`. **All 44 are now 100%.**
The other 20 cohort functions keep their scores.

### Entropy review

Only two scores outside the 44 changed. Both functions read
`attached_direction`, one of the two members swapped into place. Their
source is unchanged:

| Address | Function | Before | After | Classifier | Instructions |
| --- | --- | ---: | ---: | --- | ---: |
| `0x444680` | `entity_step_movement_vector_resolve` | 5.24% | 5.23% | `drift-review` | 275 → 276 (**+1**) |
| `0x444fb0` | `ledger_stub_00444fb0_EntityTurnToVectorActivityIssue` | 25.56% | 26.04% | improved | 178 → 179 (**+1**) |

In both, the displacements of `attached_direction` change (`0x32c`–`0x334`
→ `0x328`–`0x330`). MSVC 5.0 then schedules the x87 loads and multiplies
differently, and one `fxch st(1)` is added. Nothing else changes, so this is
x87 register-stack drift, not a code change. **Owner review:** both
instruction-count changes are listed here. The drop is 0.01 points and
returns `0x444680` to its score before #459: #459's `view.hpp` edit had
taken it from 276 to 275 instructions (5.23% → 5.24%). Every layout that
puts `stats_record` at `+0x334` has to move `attached_direction`, because
`+0x334` lies inside it.

**Listings without a score change.** With #461's normalisation, this change
alters 56 game listings: the 44 accessors, the two functions above, and 10
functions that read a moved member (`+0x114`, `+0x328`–`+0x334`). Those 10 keep
their instruction count and score. Their displacements move from one
non-original value to another; examples are `entity_teardown`,
`entity_building_index_set`, `entity_world_position_set` and
`entity_type_operation_release`. The `full` section changes for the same
functions.

Neither known classifier gap arises:

- Symbolic operand names are masked since #461. Before #461, 20 more
  listings changed here. They were in files that include `game.hpp`, and
  differed only in the per-compilation numbers MSVC puts in
  anonymous-namespace static names.
- No `LIBRARY` listing changed, so the `source-unknown` label for `LIBRARY`
  functions does not arise.

## Native check

`entity_stats_real` (`entity_test --real-stats $TEST_WADS_DIR`) loads the
shipped `Lego.cfg` through the startup loader. It binds entities by name and
level and reads values back through the retail accessors:

- Pilot: `RouteSpeed` 1.1, `SoilDrillTime` 4.0 (×25),
  `EnergyDecayRate` 0.25 and `AwarenessRange` 1000. `RepairValue` is 10 at
  level 0 and 20 at level 2.
- SmallDigger at level 2: `RouteSpeed` 4.0 and `HardDrillTime` 180 (×25).
- RockMonster: `FreezerTime`, `PusherDist`, `LaserDamage`, `AttackRadius`,
  and the `CanFreeze`/`CanLaser`/`CanPush` flags.

It runs in its own process because the loader fills process-wide tables.

The game `entity_test` also has an older `--real` `.ol` probe that ctest
never runs. Run by hand, it already fails: all 44 `.ol` files place no
entities (`got=0`), because the synthetic run has no type-config rows. This
change does not wire it in, fix it or alter it.

## Survey: other records with offset-only differences

Method: in `game-only.json`, take every non-exact function. Pair the
orig/recomp rows of each changed diff group. Keep a function if every
differing pair differs only in a `[reg + disp]` displacement; stack slots are
excluded. Before this change, the 44 stats readers were the largest such
group (`+0x114` → `+0x334`). After it, **7 functions** remain strictly
offset-only:

| Record | Recomp → orig offset | Functions fixed | Functions |
| --- | --- | ---: | --- |
| `EntitySlot` flag words `flags_3e0`/`3e4`/`3e8`/`3ec` | `0xec/0xf4/0xf0/0xf8` → `0x3e0/0x3e4/0x3e8/0x3ec` | 5 | `entity_type3_flag04000000_test`, `selection_contains_entity_active_target`, `entity_flags_mask107c33de_or_1c00203_any_test`, `selection_active_candidate_already_selected_or_marked`, `selection_candidate_eligibility_check` |
| `EntitySlot.field_340` | `0x154` → `0x340` | 1 | `entity_flag40000_threshold_compare` |
| command record chain link | `0x50` → `0x60` | 1 | `cmd_cancel_by_coords` |

A wider count includes displacements that differ inside functions that also
differ in other ways. It ranks which wrong offsets are most widespread (not
functions fixed):

| Recomp → orig | Functions | Record |
| --- | ---: | --- |
| `0xec` → `0x3e0` | 25 | `EntitySlot.flags_3e0` |
| `0xf4` → `0x3e4` | 18 | `EntitySlot.flags_3e4` |
| `0xf0` → `0x3e8` | 18 | `EntitySlot.flags_3e8` |
| `0x154` → `0x340` | 14 | `EntitySlot.field_340` |
| `0xf8` → `0x3ec` | 11 | `EntitySlot.flags_3ec` |
| `0x13c` → `0x36c` | 7 | `EntitySlot.owner_36c` |
| `0xe4` → `0x2f0`, `0x160` → `0x2fc`, `0x138` → `0x304` | 5, 5, 4 | `EntitySlot.cmd_head`, `link_2fc`, `attach_304` |
| `0x50` → `0x60` | 4 | command record: the port-only `global_next` occupies `+0x50` |
| `0x16c`/`0x168` → `0x2f8`/`0x2f4` | 3 each | `EntitySlot.pos_pair_2f8`/`2f4` |
| `0x82c` → `0x4a84` | 3 | `TerrainSurfaceGrid` change-queue count |
| `0x314` → `0x330` | 2 | `EntitySlot.indexed_record_slot_330` |
| `0x194`/`0x198` → `0x18`/`0x34` | 2 | save record (`sav_record_ptr_target`) |

The next most valuable step is the same treatment for `EntitySlot`'s flag
words at `+0x3e0`–`+0x3ec`. Their reconstruction slots (`+0xec`–`+0xf8`) lie
below `+0x334`, and `+0x3e0` falls inside the `scene_slot_owner_*_handles`
arrays, so placing them needs more than one swap. This survey files nothing.
