# Finding 620 — SpiderWeb.c (GAME-042) rebuilt in original shape, with a port tail

Issue #684, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
The unit map called GAME-042 "grid cell record buffers" and listed 9
low-confidence functions (`004661c0`-`00466750`, boundary evidence only).
**It is the original's `SpiderWeb.c`** (`inferred`): the spider webs. **The
extent holds** (`004661c0`-`004667e0`), and the unit has a 10th function,
the update callback `004665e0`, which Ghidra left as a label (see
[Unit extent](#unit-extent)).

What the unit does:

- The unit keeps one 0xc-byte record per map block (`webObj`, `health`,
  `flags`) in a buffer that SpiderWeb_Restart reallocates for each level.
  SpiderWeb_Add (`00466380`, from the object-list loader) records a web the
  level places, with health 100.
- SpiderWeb_SpawnAt (`00466250`, a Lego.c debug key) spins a new web. The
  block must be ground, with walls either east and west (the web faces at
  pi/2) or north and south (at 0). That test is `004663f0`.
- SpiderWeb_Update (`004665c0`, once a frame from Lego_MainLoop) runs
  `004665e0` over every object:
  - a web whose walls have gone is removed (`00466750`);
  - a caught unit (`flags1` `0x2000000`) loses the web's health at the
    SpiderWeb weapon's damage rate (`Weapon_GetWeaponIDByName("SpiderWeb")`,
    `Weapon_GetDamage`). Below zero the web is removed and the unit is
    released;
  - any other unit is tested against the web on its block (`00466490`,
    Weapon_LegoObject_Collision). A hit flags both the unit and the web.

This change:

- rebuilds the unit as the C unit [`src/game/spiderweb.c`](../../src/game/spiderweb.c),
  with [`spiderweb.h`](../../src/game/spiderweb.h) and
  [`spiderweb_imports.h`](../../src/game/spiderweb_imports.h);
- moves the port's models, unannotated, to the port tail
  [`spiderweb_port.cpp`](../../src/game/spiderweb_port.cpp), which is
  `grid_cell_records.cpp` under `git mv`;
- switches Lego.c's four call sites to the unit (see
  [Callers switched](#callers-switched)).

## Result

| | before (`main` `9043af89`) | after |
| --- | ---: | ---: |
| SpiderWeb.c similarity (10 functions, byte-weighted, 1,469 code bytes) | 23.62% | **100.00%** |
| Exact functions in SpiderWeb.c | 2 | **10** |
| Mapped functions in SpiderWeb.c | 9 | **10** |
| Exact-function code bytes (`binary-progress.json`) | 205,613 | **206,679** |
| Exact functions, full / game-only | 1,731 / 1,522 | **1,739 / 1,530** |
| `opt_ref` annotated (full, by address): removed / exact removed | 412 / 223 | **402 / 218** |
| Reccmp lint ledger (entries) | 1,052 | **1,051** |

Byte weights are the original sizes from the single-function loop. An
unmapped function counts as 0%. The exact-function code bytes rise by
1,066, not 1,421: `binary-progress.json` counts the ledger's functions, and
the ledger has no row for `004665e0` (355 bytes).

The names are OpenLRR's where known, in the repository's snake case, and
are `inferred`; the binary attests only the weapon name `SpiderWeb` and the
activity `Expand`.

| Address | Function | Port name before | Bytes | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `004661c0` | `spiderweb_initialise` | `grid_cell_record_buffer_allocate` | 78 | 25.93 | **100.00** |
| `00466210` | `spiderweb_shutdown` | `entity_field_grid_buffer_release` | 29 | 100.00 | **100.00** |
| `00466230` | `spiderweb_restart` | `grid_cell_record_buffer_replace` | 19 | 100.00 | **100.00** |
| `00466250` | `spiderweb_spawn_at` | `ledger_stub_00466250_ExpandCellEntitySpawn` | 290 | 19.12 | **100.00** |
| `00466380` | `spiderweb_add` | `set_expansion_entity_at_grid_cell` | 108 | 63.89 | **100.00** |
| `004663f0` | `spiderweb_get_angle` | `can_spawn_expansion_entity_at_grid_cell` | 145 | 64.96 | **100.00** |
| `00466490` | `spiderweb_check_collision` | `ledger_stub_00466490_CellSightRayCheckAndFlag` | 294 | 12.00 | **100.00** |
| `004665c0` | `spiderweb_update` | `simtick_post_step` | 19 | 14.63 | **100.00** |
| `004665e0` | `spiderweb_callback_update` | (no source) | 355 | unmapped | **100.00** |
| `00466750` | `spiderweb_remove_at` | `grid_cell_invalidate_and_clear_flags` | 132 | 16.67 | **100.00** |

Seven functions outside the unit rose, with no loss. Each one's listing is
unchanged; only an operand now resolves:

| Address | Function | Before | After | Why |
| --- | --- | ---: | ---: | --- |
| `00422850` | `lego_misc_object_resources_load` (Lego.c) | 94.60 | 94.76 | `"SpiderWeb"` is now a `STRING` (below) |
| `0042d600` | `lego_object_names_load` (Lego.c) | 86.38 | 86.69 | the same |
| `0042da20` | `lego_object_the_names_load` (Lego.c) | 86.38 | 86.69 | the same |
| `0042e8b0` | `lego_entity_type_pair_lookup` (Lego.c) | 82.43 | 82.67 | the same |
| `004396c0` | `legoobject_item_name_by_kind` (LegoObject.c) | 86.36 | 87.88 | the same |
| `004232e0` | `lego_engine_tick` (Lego.c) | 97.66 | 97.73 | calls `spiderweb_update` instead of an adapter |
| `004288e0` | `lego_level_debug_input_cell_mutator_dispatch` (Lego.c) | 97.35 | 97.51 | calls `spiderweb_spawn_at` instead of an adapter |

No naked asm, no `Deviation (scoring only)` and no filler declaration.

## Unit extent

All nine listed functions were low-confidence, with no data anchor, so the
extent was checked from the data, the literals and the call graph.

- **`.bss`: the unit's own commons.** `005520e8` (the block buffer) and
  `005520ec` (the level) are read and written only by `004661e2`-`004667bf`,
  which is inside `004661c0`-`004667e0`. No other code in the image
  references them. They become one common struct, `spiderWebGlobs`.
- **`.rdata`: one literal.** `004665e0` compares the web's health with the
  `0.0f` at `0049e780`, and nothing else reads it. It sits between SFX.c's
  pool (GAME-041, ending with the `0.3f` at `0049e77c` that `00465fe9`
  reads) and Stats.c's pool (GAME-043, from `0049e788`). So SpiderWeb.c
  follows SFX.c and precedes Stats.c, which is OpenLRR's alphabetical file
  order (`SFX.c`, `SpiderWeb.c`, `Stats.c`). The `1.0f` at `0049e784` is read
  by no code, like the unread `1.0f` of finding 617; it says nothing about
  the boundary.
- **Strings: both are pooled.** `"SpiderWeb"` (`004a3f40`, Lego.c's string
  range, also used by four Lego.c functions) and `"Expand"` (`004a5120`,
  LegoObject.c's range, marked in `legoobject_unit.c`). The unit owns no
  string.
- **The call graph is closed.** The unit's functions call each other, then
  only LegoObject.c (`004377b0`, `004422a0`, `004424f0`, `00438610`),
  Map3D (`0044f640`), Weapons.c (`0046f0d0`, `0046f1d0`, `00470260`),
  Container.c (`00473370`, `00475140`) and the heap. Every outside caller is
  Lego.c: `004288e0` (`00466250`), `00429890` and `0042b510` (`00466230`),
  `0042e020` (`00466380`) and `004232e0` (`004665c0`). `004667e0` is Stats.c's
  Stats_Initialise (finding 581).
- **The 10th function.** `004665c0` pushes `004665e0` as the callback of
  LegoObject_RunThroughListsSkipUpgradeParts. Ghidra made it a label, so
  the ledger and the unit map have no row for it, and the ledger gave
  `004665c0` 400 bytes. `004665e0` has its own prologue, and it is
  annotated now. It is not added to the ledger.

`translation-units.spec.csv`: GAME-042 becomes `SpiderWeb.c` (`inferred`),
with the evidence above. `python3 tools/translation_units.py build`: 0
conflicts.

## How the conversion is done

### Records

`spiderweb.h` declares `SpiderWeb_Block` (0xc bytes) and `SpiderWeb_Globs`
(8 bytes, `005520e8`). The unit reads Lego.c's `Lego_Level` and `Lego_Block`
(`lego_unit.h`) and LegoObject.c's `Lego_Object` (`legoobject_unit.h`):
`type`, `other` (the container), `flags1` (`+0x3e0`) and `flags3`
(`+0x3e8`).

### Callers switched

Every Lego.c caller passes the original's arguments, so it now calls the
unit directly. `lego_unit_imports.h` replaces each declaration one for one
(and its unused `struct EngineState;` with `struct Lego_Object;`), so
`lego_unit.c`'s symbol numbering does not move.

| Caller (`lego_unit.c`) | Before | After |
| --- | --- | --- |
| `004232e0` Lego_MainLoop's world update | `simtick_post_step_original` (`PORT-ONLY` adapter in `lego_port.cpp`) | `spiderweb_update` |
| `004288e0` debug keys | `expand_cell_entity_spawn_adapter` (`PORT-ONLY` adapter in `grid_cell_records.cpp`) | `spiderweb_spawn_at` |
| `00429890`, `0042b510` level load | `grid_cell_record_buffer_replace` (port model) | `spiderweb_restart` |
| `0042e020` object list | `set_expansion_entity_at_grid_cell` (port model) | `spiderweb_add` |

The two adapters are deleted. The port's models keep their names in the
port tail: natively the simulation ticker (`tickers.cpp`) and the tests
(`test_engine_bind.cpp`, `test_simtick.cpp`) still call them. The frozen
`level_for_*.hpp` headers still declare the ledger stub `00466250`, which
the port tail still defines; they are not edited.

### Callees

| Original | Called as | Unit |
| --- | --- | --- |
| LegoObject_GetBlockPos `004424f0` | `legoobject_position_to_terrain_cell` | LegoObject.c |
| LegoObject_GetFaceDirection `004422a0` | `legoobject_surface_vector_normalize` | LegoObject.c |
| LegoObject_CreateInWorld `00438610` | `legoobject_create_in_world` | LegoObject.c |
| LegoObject_RunThroughListsSkipUpgradeParts `004377b0` | `legoobject_run_through_lists_skip_upgrade_parts` | LegoObject.c |
| Map3D_BlockToWorldPos `0044f640` | `terrain_map_block_to_world` | GAME-026 |
| Weapon_GetWeaponIDByName `0046f0d0`, Weapon_GetDamage `0046f1d0`, Weapon_LegoObject_Collision `00470260` | `object_type_name_index_find`, `weapons_get_damage_for_object`, `weapons_object_collision_check` | Weapons.c |
| Container_SetActivity `00473370`, Container_SetAnimationTime `00475140` | `scene_action_name_set`, `ddiscene_frame_update_and_callbacks` | G98-002 |
| `malloc`, `free`, `memset` | `crt/vc5names.h`, `<stdlib.h>`, `<string.h>` | LIBC |

Two callees return a result that only SpiderWeb.c reads, and their
original-shape definitions returned `void`:

- `legoobject_position_to_terrain_cell` (`004424f0`) now returns
  Map3D_WorldToBlockPos's result. `00466490` tests it.
- `weapons_object_collision_check` (`00470260`) now returns its two
  callees' results. `00466490` tests it.

Both stay exact: their tail calls already left the result in `eax`.

### Source shapes that matched

Nine functions matched as first written from the pseudocode and the
disassembly. **`004663f0`** (65.67% at first) needed two changes:

- the `return 1` paths nested inside `if (block->flags1 & 0x8)`, with one
  `return 0` at the end: 100% "effective" (one register swap);
- the block indexed straight from `spiderWebGlobs.level` each time, with no
  `width`, `blocks` or index local: exact. Four shapes with locals (the
  `width`/`blocks` order, an `int` or `unsigned` index, `by * width`) all
  stay effective.

### The `"SpiderWeb"` literal

`004665e0` pushes Lego.c's literal `004a3f40`. `lego_port.cpp` annotated
that address as the port's `GLOBAL` `s_item_name_spider_web` (a C++ array),
so Reccmp read the push as a data reference, and the callback scored
99.11%. Following `"Expand"` (`004a5120`, a `STRING` marked at
LegoObject.c's use), the `GLOBAL` marker is removed and `lego_unit.c` marks
the literal at Lego.c's first use (`00422850`, the `MiscObjects` load). The
port's array stays, unannotated. The callback reaches 100%, and five
functions that use the string rise (table above).

## Entropy and drops

`make reccmp-check` against `main`'s baseline:

- 14 improved: 7 unit functions and the 7 functions above;
- 1 newly mapped: `004665e0`;
- **none below baseline**, and no exact match lost.

Instruction counts changed only in the unit (listing digests):

| Address | before | after |
| --- | ---: | ---: |
| `004661c0` | 29 | 29 (listing digest changed) |
| `00466250` | 141 | 96 |
| `00466380` | 36 | 32 |
| `004663f0` | 60 | 56 |
| `00466490` | 350 | 90 |
| `004665c0` | 15 | 6 |
| `004665e0` | — | 112 |
| `00466750` | 26 | 35 |

The seven functions that rose outside the unit keep their listings.

## `opt_ref` (finding 579, `make opt-ref-callers`)

| | before (`main`) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full` (by address): removed / exact removed | 412 / 223 | 402 / 218 |
| `opt_ref.annotated.game_only` (by address) | 405 / 216 | 395 / 211 |

`make opt-ref-callers OPT_REF_ARGS="--object spiderweb.c.obj --min 1"` lists
no removed function: Lego.c reaches every unit function, and the update
callback through its address push. Weapons.c's `00470260` is live too
(SpiderWeb.c's `00466490` calls it).

## Reccmp lint ratchet

1,052 → 1,051 entries (−1, no growth): the port's
`post_step_cell_sight_callback` is in the port tail now. The two deleted
adapters were already `PORT-ONLY`.

## Files outside the unit

The change edits no file owned by #682 (`video_graph.cpp`), #683
(`terrain_overlay_quads.cpp`), #685 (`record_chains.cpp`) or #686
(`gods98/sound.cpp`, `sound3d.c`). No frozen `_for_` or `_from_` header is
edited. Outside `spiderweb.c` and its headers, the change edits:

- `lego_unit.c`, `lego_unit_imports.h`: the four call sites, and the
  `"SpiderWeb"` `STRING` marker;
- `lego_port.cpp`: the `simtick_post_step_original` adapter deleted, and
  the `004a3f40` `GLOBAL` marker removed;
- `legoobject_unit.c`, `legoobject_unit.h`: `004424f0` returns its result;
- `weapons.c`: `00470260` returns its result;
- `gods98/engine.cpp`: the `005520e8`/`005520ec` `GLOBAL` markers removed
  from the port's `s_field_grid_buf`/`s_field_grid_desc`;
- `test_c_boundaries.cpp`: `spiderweb.h` and `spiderweb_imports.h`;
- `link_order.cmake` and `src/game/meson.build`: the unit and the tail;
- `translation-units.spec.csv` and its generated files;
- `tools/binary_progress_inputs.txt`: regenerated for the renamed and new
  files.

## Native behaviour

Native behaviour is unchanged, and there is no new native gap. The port
tail still serves every port name, and native callers (the ticker, the
tests) keep using it. `spiderweb.c` and `lego_unit.c` compile natively but
are not reached (#552 defers native gameplay).

**Matching-build runtime** (`make reccmp-run`, not run for this change):
Lego.c now reaches the unit's web model. Before, the main loop's update
went through an adapter to the port's model, which reads the port's
EngineState.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK. None below
  baseline, and no exact match lost. The plain-immediate check, the winapi
  link check and the link-order map check pass.
- `make match-progress`, `make unit-data-offsets`: run, and the generated
  files are committed. GAME-042 has one common (`005520e8`, 8 bytes) and no
  `.data`, so `unit-data-offsets` has fewer than two items to check.
- `make winapi-link-check`: OK.
- `make tools-test`: passes (no `--allow-growth`).
- Native: `cmake --build build` and `ctest --test-dir build`: 37 of 37 pass.
- C subset: `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only` on `spiderweb.c`, `lego_unit.c`, `legoobject_unit.c` and
  `weapons.c`.

## Reproduce

    python3 tools/convert_unit.py GAME-042 --stem spiderweb --out <scratch>   # with the GAME-042 link-order entry removed: the unit was already wired
    make match-function SOURCE=src/game/spiderweb.c
    make reccmp-check
    make opt-ref-callers OPT_REF_ARGS="--object spiderweb.c.obj --min 1"
