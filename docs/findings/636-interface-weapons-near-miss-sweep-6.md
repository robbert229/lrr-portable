# Finding 636 — Near-miss sweep 6 of Interface.c and Weapons.c

Issue #722, part of #388 and ADR 0005. It follows finding 630 (sweep 3 of
the same units), whose tally this sweep started from. That sweep took the
95–99% and 80–94% bands; this one went through them again and then through
the functions below 80%, which no sweep had looked at.

- Interface.c: GAME-020, `src/game/interface_unit.c`.
- Weapons.c: GAME-050, `src/game/weapons.c` and `src/game/weapons_maths.h`.

Measured on `main` `7835ce86` with `make reccmp-check`.

| Measure | `main` | This change |
| --- | ---: | ---: |
| New exact functions | | **10** (all Interface.c) |
| Functions that rose / fell | | 14 / **0** |
| Exact-function code bytes | 279,887 (43.56%) | **280,476** (43.65%) |
| `opt_ref` annotated (full): removed / exact removed | 164 / 127 | 166 / 131 |
| `/OPT:REF` relink `.bss` (`opt_ref.image.bss_bytes`) | 1,115,696 | 1,115,696 |

No exact match was lost and no function fell below the baseline.

**Stacked on #725** (`11621ba2`, on #726 and #723): the same 14 functions rise
by the same amounts and none falls. Exact-function code bytes go from
286,230 (44.55%) to 286,819 (44.64%); `opt_ref` from 125 / 101 to 125 / 103;
relink `.bss` stays 1,116,672; gate image end is `0x875000`. No function of
either unit moved with #726's newly annotated callees.
`make match-progress` and `make unit-data-offsets` were run, and the three
progress files are committed (`unit-data-offsets` changed no tracked file).
The gate image end (`make pe-audit`) stays `0x873000`. `make tools-test`,
`make winapi-link-check` and native ctest (37 of 37) pass.

None of the gains came from the 95–99% and 80–94% bands. Every function
there is short for a cause that finding 630 already named; see
[Why the others are short](#why-the-others-are-short). The gains are below
80%, where the source had the wrong shape.

## What closed the gaps

### 1. Aggregate initialisers for the object search (6 functions exact)

The six object-search helpers (`0041f100`, `0041f230` to `0041f340`) set
the six fields of `InterfaceObjectSearch` one by one. The original stores
the first field as an immediate `0`, the other five from a zeroed `eax`,
and then overwrites the fields it sets. That is MSVC's code for
`InterfaceObjectSearch search = { 0 };` followed by assignments. Where
the helper sets `flags`, the original does `or al, 1` on the zeroed
register, which only `search.flags |= 1` gives (`= 1` stores an
immediate after the call's pushes).

| Function | Before | After |
| --- | ---: | ---: |
| `interface_any_object_of_type2` `0041f230` | 29.63 | 100 |
| `interface_any_object_with_flags3` `0041f270` | 41.38 | 100 |
| `interface_any_object_with_flags2_100000` `0041f2b0` | 37.04 | 100 |
| `interface_any_object_of_type` `0041f2f0` (`flags \|= 1`) | 76.47 | 100 |
| `interface_any_object_with_ability` `0041f340` (`flags \|= 2`) | 64.52 | 100 |
| `interface_object_type_present` `0041f100` | 68.42 | 100 |

Finding 630 noted the same idiom for local tables (`0044e8c0`, `0041f740`);
a struct cleared field by field is a sign of it too.

### 2. Branch shapes (3 functions exact)

- `interface_set_object_item_pressed` (`0041ca20`) and
  `interface_set_object_item_flash` (`0041e7a0`): the port picked a flags
  array into a pointer and then set or cleared the bit once. The original
  repeats the `if (on)` in each type branch on
  `interfaceGlobs.vehicleItemFlags[objIndex]` and `buildingItemFlags`
  directly. The `flags` local is gone. 64.86% to 100% each.
- `interface_update_item_enabled` (`0041c7b0`): the availability call
  is a nested `if` with its own `else |= DISABLED`, not the last operand
  of the `&&` chain. MSVC then loads the flags once before the branch, as
  the original does. 75% to 100%.

### 3. Corrections (2 functions)

- **`interface_get_advisor_type` (`0041e8d0`, 30% to 100%)** returned 4 for
  types 0 to 4 and 5 for type 5. The original is a switch over types 0
  to `0x44` behind a byte index table and a 17-entry jump table: type 0
  gives 4, the 26 types 1, 2, 5, 6, 9, 11, 12, 14, 20, 22–24, 31, 34, 36,
  41–44, 46, 47, 49, 50, 58, 59, 67 and 68 give 5, and every other type
  gives 3. Each group of cases keeps its own `return 5;`, because the
  original's table has one entry per group (MSVC merges the identical
  bodies only afterwards). Nothing in the port calls it yet.
- **`interface_update_slide` (`0041eac0`, 72.73% to 75.27%)**, two faults:
  - The slide-in ran only while `INTERFACE_FLAG_IN` was set. Slide-in is
    what sets that flag, so it could never start. The original returns
    when `IN` is set, slides out when `OUT` is set, and else slides in
    when `OPEN` is set.
  - The menu-22 test read `panelGlobs.flags & 2`. The original reads
    `panelGlobs+0x1dc`, which is `panelTable[8].flags` (`PANEL_FLAG_OUT`),
    the panel the next line toggles.

  The two `currMenuPosition.x` stores now come before their flag
  updates, as in the original. What is left is block layout: the
  original loads `flags` once for both the menu-23 and default arms. A
  `switch` instead of the `if` chain scores 63%.

### 4. Weapons.c's ray end point and projectile shapes (3 functions rise)

- **`weapons_ray_end_point`** (`weapons_maths.h`) added `v->x * t` to each
  component of `p` in one statement. gods98's Maths_RayEndPoint scales
  the ray into a local vector and then adds the origin. The local gives
  the callers the original's larger frame and x87 order. Three equivalent
  forms of the scale compile to the same code.
  - `weapons_projectile_hit_check` (`0046f3b0`): 74.48% to 87.75%.
  - `weapons_projectile_update` (`0046f920`): 45.69% to 50.18%, together
    with the next change.
- **`weapons_projectile_update`**: the original calls
  Container_SetOrientation in each branch, with the direction or the
  difference as arguments, and MSVC tail-merges the two calls. The port's
  `dirX`/`dirY`/`dirZ` locals and the single call after the `if` are gone.
- **`weapons_projectile_create_curved`** (`0046fc70`): a `dist` local
  holds the distance that `p2.x` and both `vector2_set_length` calls use.
  That gives the original's frame (`sub esp, 0x2c`) and stack slots:
  57.22% to 59.89%.

Rescoring every function of the unit with `match_function.py` after each
change showed no other function move (the `weapons_ray_end_point` local
and the removed `flags`/`dir*` locals change the symbol counter for the
functions after them).

## Per-function results

All the unit functions below 100% on `main`, from the match baseline.
Letters in the last column are the causes tallied below. "n.e." means not
examined in this sweep.

| Address | Function | `main` | After | Change, or what is left |
| --- | --- | ---: | ---: | --- |
| `00471630` | weapons_ray_block_sweep | 97.93 | 97.93 | E: `fld`/`fadd` order (effective match) |
| `0041c4f0` | interface_update_advisor | 98.58 | 98.58 | E: `fsubr` with `-0.5f` (finding 630) |
| `0041f190` | interface_callback_object_search | 97.14 | 97.14 | n.e. |
| `0041b680` | interface_enumerate_menu_items | 96.20 | 96.20 | B: type counts |
| `00471d20` | weapons_model_fire_update | 95.60 | 95.60 | D: slots of `i` and `discharge`; 6 declaration orders and 4 block scopes move nothing |
| `0041d040` | interface_is_item_available | 94.92 | 94.92 | D: one inline epilogue (finding 630) |
| `0041ab00` | interface_load_object_items | 94.65 | 94.65 | B: type counts |
| `00471960` | weapons_see_through_walls_check | 94.74 | 94.74 | E |
| `0041b3c0` | interface_get_menu_item_count | 93.33 | 93.33 | B: type counts |
| `0041c470` | interface_callback_hotkey | 91.14 | 91.14 | n.e. |
| `00472080` | weapons_object_in_range | 90.16 | 90.16 | E: `add esp, 8` among the x87 ops; 4 shapes tried |
| `00472130` | weapons_object_in_touch_range | 90.16 | 90.16 | E: as `00472080` |
| `0041ec30` | interface_draw_object_count | 89.19 | 89.19 | B: request arrays outside one `legoGlobs` struct |
| `004718d0` | weapons_object_pair_ray_hit | 88.00 | 88.00 | E |
| `0046eb80` | weapons_initialise | 87.14 | 87.14 | C (finding 630) |
| `0041dca0` | interface_do_action | 86.90 | 86.90 | D (finding 630) |
| `0041f4d0` | interface_callback_can_pick_up | 86.87 | 86.87 | n.e. |
| `0041a920` | interface_reset | 85.19 | 85.19 | B: type counts |
| `0046faf0` | weapons_projectile_create | 84.50 | 84.50 | C: projectile pointer as `add`, not `lea`; indexed access scores 53%. E: one `fmul` order in the normalise |
| `00471370` | weapons_callback_ray_candidate | 84.38 | 84.38 | n.e. |
| `0041aa00` | interface_init_object_items | 77.78 | 77.78 | B: type counts |
| `0041f2f0` | interface_any_object_of_type | 76.47 | **100** | `= { 0 }`, `flags \|= 1` |
| `0041baa0` | interface_do_mouse | 75.84 | 75.84 | n.e. |
| `0041c7b0` | interface_update_item_enabled | 75.00 | **100** | nested availability test |
| `0041eac0` | interface_update_slide | 72.73 | 75.27 | two corrections; D: hoisted `flags` load |
| `0041a850` | interface_shutdown | 70.31 | 70.31 | C: original uses `eax` for every `free` argument; the same code at counter offsets 0–63 |
| `00471a60` | weapons_model_aim | 69.30 | 69.30 | C: pivot kept in `ebp`, not a slot |
| `0041f100` | interface_object_type_present | 68.42 | **100** | `= { 0 }` |
| `0041ca20` | interface_set_object_item_pressed | 64.86 | **100** | update in each type branch |
| `0041e7a0` | interface_set_object_item_flash | 64.86 | **100** | as `0041ca20` |
| `0041f340` | interface_any_object_with_ability | 64.52 | **100** | `= { 0 }`, `flags \|= 2` |
| `0041ee80` | interface_get_dependency_at_point | 63.57 | 63.57 | D: the original's `jbe` count guard and 8 more frame bytes; `count > 0` compiles as `!= 0` |
| `004702b0` | weapons_object_quad_intersect | 59.40 | 59.40 | D: 4 fewer frame bytes in the original |
| `0046fc70` | weapons_projectile_create_curved | 57.22 | 59.89 | `dist` local; C: struct-copy registers |
| `0046ffb0` | weapons_fire_lazer | 56.49 | 56.49 | D: `hitObject` in a dead parameter's slot; declaration orders give at most +0.9 |
| `0041eca0` | interface_draw_dependencies | 52.59 | 52.59 | C: one more callee-saved register; `count > 0` changes nothing |
| `00470760` | weapons_mesh_build_segment | 25.59 | 25.59 | D: one more 4-byte slot in the original shifts every local; the normalise inline does not add it |
| `0046f920` | weapons_projectile_update | 45.69 | 50.18 | ray end point, per-branch orientation calls; E: arguments passed through `fld`/`fstp` |
| `0041c1c0` | interface_get_item_at_point | 49.28 | 49.28 | C: `x`/`y` kept in `edi`/`ebx` and 8 more frame bytes in the original |
| `0041b800` | interface_callback_draw_item | 48.72 | 48.72 | C: register allocation; MSVC duplicates the pressed call the original keeps once |
| `0041f270` | interface_any_object_with_flags3 | 41.38 | **100** | `= { 0 }` |
| `0041f2b0` | interface_any_object_with_flags2_100000 | 37.04 | **100** | `= { 0 }` |
| `0041e8d0` | interface_get_advisor_type | 30.00 | **100** | the original's switch (a correction) |
| `0041f230` | interface_any_object_of_type2 | 29.63 | **100** | `= { 0 }` |
| `0046f3b0` | weapons_projectile_hit_check | 74.48 | 87.75 | ray end point; E: `fcom` against `fld st(0); fcomp` |
| `0041cd30` | position_tracker_box_draw | 26.60 | 26.60 | E: x87 scheduling; writing the ten points out gives the original's frame but scores 24.67% |

## Why the others are short

36 of the 46 functions are still short. Each is counted once, under its
main cause. Ledger bytes are address gaps in
`docs/coverage/translation-units.csv` where it has the function, and the
listing size otherwise.

| Cause | Functions | What would close it |
| --- | ---: | --- |
| **B. Data layout**: type counts (`0041b680`, `0041ab00`, `0041b3c0`, `0041a920`, `0041aa00`); request arrays (`0041ec30`) | 6 | see below |
| **C. Register choice**: `0046eb80`, `0046faf0`, `0041a850`, `00471a60`, `0046fc70`, `0041eca0`, `0041c1c0`, `0041b800` | 8 | not found |
| **D. Scheduling, stack slots or block layout**: `00471d20`, `0041d040`, `0041dca0`, `0041eac0`, `0041ee80`, `004702b0`, `0046ffb0`, `00470760` | 8 | not found |
| **E. x87 scheduling or operand order**: `00471630`, `0041c4f0`, `00471960`, `00472080`, `00472130`, `004718d0`, `0046f920`, `0046f3b0`, `0041cd30` | 9 | not found |
| Not examined: `0041f190`, `0041c470`, `0041f4d0`, `00471370`, `0041baa0` | 5 | a later sweep |

No residue is a callee outside the units (cause A) any more: #711 (Images.c,
Flic.c) and #707 (FrontEnd.c part 2) closed finding 630's 17.

**The type counts.** Five Interface.c functions read the vehicle and
building counts as `lego_vehicle_type_count` and `lego_building_type_count`
(`interface_callees.h`). The original reads `legoGlobs` at `0055627c` and
`00556288`. The port keeps that storage in entity.cpp's
`uint32_t s_catalog_counts[5]`, which has C++ linkage, so C cannot name
it, and the frozen `entity_for_lego.hpp` declares it without
`extern "C"`. The rename that would close the five (a `#define` to
`s_catalog_counts`, as finding 630 did for other `legoGlobs` fields) needs
that declaration to have C linkage first. The five functions' code is
otherwise identical: the remaining diff lines are these operands, plus
Stats.c calls that only `match_function.py` names differently (see below).

**Effects on scoring, not on code.** `match_function.py` names a call by
the callee's link name, while the full run maps it by the callee's
annotation. Calls to functions renamed by a `*_link_names.h` macro (Stats.c,
Message.c, Dependencies.c, Construction.c, SFX, Container.c) are therefore
diffs in the single-function tool and not in the full report. `0041d040`
scores 93.07% there and 94.92% in full, for example. End-of-array pointers
(`weapons_projectile_find_free`'s `0x5056a0`) differ the same way.

## What was tried and failed

So that a later sweep need not repeat it:

- `tools/msvc_permuter.py`, 90 s each on `00472080`, `0041a850`, `0041c7b0`,
  `0041ca20`, `0041eac0`, `0046f3b0` and `00471a60`: 0 to 4 candidates each,
  no improvement.
- A rule B probe (0–63 inert externs before the function) on `0041a850`:
  the same score at every offset, so its register choice is not counter
  phase.
- `00471d20`: six declaration orders, and `discharge`, `range`,
  `weaponType` and `fireNull` declared in the loop's block. The slots do
  not move.
- `00472080`: `range > dist`, swapped `distance` operands, the inline
  expanded by hand, and modulus over subtract. None is better.
- `0046f3b0`: `t * v->x` in the scale. The code is unchanged.
- `0041cd30`: the ten polyline points written out in order, with `+`,
  `- -` and parenthesised sums. That gives the original's `sub esp, 0x2c`,
  but the x87 order is further away (24.67%–30%).
- `0041eac0`: a `switch` on `nextMenuType` (63%).
- `00470760`: `weapons_vector3d_normalise(&dir)` in place of the
  hand-written `len` products. The frame stays 4 bytes short (26.03%).
- `0046faf0`: `weaponGlobs.projectileList[index].` in place of the
  `projectile` pointer (52.83%).
- `0041ee80`, `0041eca0`: `count > 0` in place of `count != 0`. The code
  is unchanged.

## Follow-ups

- **The port never writes `lego_vehicle_type_count` or
  `lego_building_type_count`.** Nothing in `src/` or the tests assigns
  them, so every Interface.c loop over the vehicle and building types
  runs zero times in any build that reaches it through these names. The
  storage the original reads is entity.cpp's `s_catalog_counts`. Giving
  that C linkage (a frozen-header change) would fix this and close the
  five B functions above.
- **`interface_get_advisor_type`** has no caller in the port. The
  Advisor.c conversion should call it.

## Files

- `src/game/interface_unit.c`: section 1 to 3 shapes and the two
  corrections.
- `src/game/weapons.c`, `src/game/weapons_maths.h`: section 4.
- `docs/coverage/match-baseline.txt`, `match-listing-digests.txt`,
  `binary-progress.json`: regenerated by `make match-progress`.
