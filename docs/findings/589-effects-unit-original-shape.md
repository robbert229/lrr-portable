# Finding 589 — The effects unit (GAME-012) rebuilt in original shape, with a port tail

Issue #616, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-012 runs the game's one-shot scene effects: the rock-fall styles from
Lego.cfg's `RockFallStyles` block (four styles of three types, four instances
each), the boulder explosions and smash paths, the two electric fence beams
(30 instances each), up to four object explosions, and the ten `MiscObjects`
effects (lazer, pusher and freezer hits, path dust, four lava erosion smokes,
the bird scarer and the upgrade effect). Each effect clones a loaded
container, plays its animation once and then hides or removes it. Text
`0040bdc0`-`0040cdc0`, 27 functions. It was relocated verbatim into
`src/game/rock_fall_styles.cpp`. This change rebuilds it as the C unit
[`src/game/rock_fall_styles.c`](../../src/game/rock_fall_styles.c), with the
unit-private imports header
[`rock_fall_styles_imports.h`](../../src/game/rock_fall_styles_imports.h) and
the matching-build link names
[`rock_fall_styles_link_names.h`](../../src/game/rock_fall_styles_link_names.h).
The port's implementation moves, unannotated, to the port tail
[`rock_fall_styles_port.cpp`](../../src/game/rock_fall_styles_port.cpp).

The change is built on `main` `984a1213` (#614). "Before" is that commit, from
a full matching build (`make reccmp-check`). "After" is this change, from the
same build.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, 27 functions, 3,894 original code bytes) | 21.13% | **92.74%** |
| Exact functions in the unit | 1 | **21 of 27** |
| Mapped functions in the unit | 26 | 27 |
| Exact-function code bytes (`binary-progress.json`) | 148,281 (23.08%) | **151,279** (23.54%) |
| Accuracy, full / game-only (`reccmp-check` means) | 77.67% / 75.46% | 78.37% / 76.22% |
| `opt_ref` annotated (full): removed / exact removed | 585 / 275 | **574 / 269** |
| `opt_ref` annotated (game only): removed / exact removed | 568 / 264 | 557 / 258 |
| GAME-012 functions removed under `/OPT:REF` | 5 (0 exact) | 1 (1 exact) |
| Reccmp lint ledger | 1,314 | 1,312 |

Byte weights use the original sizes the single-function loop reports; `0040c3a0` was unmapped before and counts
as 0%. The names are OpenLRR-style `effect_*` names in snake case; where the
unit links under the port's name in the matching build (below), the report
shows the port's name.

| Address | Function (unit name) | Report name (matching build) | Bytes | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `0040bdc0` | `effect_remove_all` | `ledger_stub_0040bdc0_SetTimeoutTo10000` | 26 | 90.00 | **100.00** |
| `0040bde0` | `effect_spawn_boulder_explode` | same | 43 | 18.18 | **100.00** |
| `0040be10` | `effect_spawn_boulder_explode_at` | `scene_light_create_at_position` | 156 | 25.00 | 75.00 |
| `0040beb0` | `effect_spawn_smash_path` | same | 177 | 16.47 | **100.00** |
| `0040bf70` | `effect_get_rock_fall_style` | `rock_fall_style_name_lookup` | 79 | 46.51 | **100.00** |
| `0040bfc0` | `effect_set_rock_fall_style` | `rockfall_style_table_base_set` | 10 | 100.00 | **100.00** |
| `0040bfd0` | `effect_load_rock_fall_styles_all` | `rock_fall_initialise_adapter` | 246 | 19.92 | **100.00** |
| `0040c0d0` | `effect_load_rock_fall_style` | same | 211 | 18.52 | 97.62 |
| `0040c1b0` | `effect_load_electric_fence_beam` | `indexed_resource_handle_set_create` | 123 | 17.78 | 98.04 |
| `0040c230` | `effect_spawn_rock_fall` | `scene_slot_pose_scale_register` | 192 | 22.06 | **100.00** |
| `0040c2f0` | `effect_spawn_electric_fence_beam` | `transient_effect_render_slot_configure` | 168 | 34.74 | **100.00** |
| `0040c3a0` | `effect_update_all` | same | 304 | unmapped | 50.73 |
| `0040c4d0` | `effect_update_boulder_explode` | same | 71 | 13.33 | **100.00** |
| `0040c520` | `effect_update_smash_path` | same | 71 | 13.33 | **100.00** |
| `0040c570` | `effect_get_block_pos_rock_fall` | `scene_cell_table_pair_read` | 54 | 17.54 | **100.00** |
| `0040c5b0` | `effect_find_rock_fall_part` | `scene_resource_slot_register_name` | 212 | 30.06 | **100.00** |
| `0040c690` | `effect_remove_all_boulder_explode` | `scene_resource_singleton_pool_release` | 40 | 38.10 | **100.00** |
| `0040c6c0` | `effect_remove_all_rock_falls` | `rock_fall_style_scene_resources_release` | 89 | 23.81 | **100.00** |
| `0040c720` | `effect_load_explosion` | `global_resource_handle_load_and_enable_original` | 42 | 10.53 | 92.86 |
| `0040c750` | `effect_spawn_explosion` | same | 224 | 21.97 | **100.00** |
| `0040c830` | `effect_update_explosions` | same | 101 | 21.15 | **100.00** |
| `0040c8a0` | `effect_get_misc_effect_data` | same | 120 | 31.58 | **100.00** |
| `0040c920` | `effect_load_misc_effect` | same | 100 | 25.00 | 95.56 |
| `0040c990` | `effect_load_misc_objects` | `misc_objects_effect_slots_bind_original` | 479 | 12.50 | **100.00** |
| `0040cb70` | `effect_update_misc_effect` | same | 208 | 21.82 | **100.00** |
| `0040cc40` | `effect_update_misc_effects_all` | same | 147 | 32.00 | **100.00** |
| `0040cce0` | `effect_spawn_particle` | `lava_effect_select_and_place` | 201 | 23.53 | 60.69 |

No function is naked asm, and no `Deviation (scoring only)` is added.

### What is left

| Function | Score | Instructions (orig / rebuilt) | Remaining difference and what was tried |
| --- | ---: | ---: | --- |
| `0040c0d0`, `0040c1b0`, `0040c720`, `0040c920` | 92.86-98.04 | equal | operand only: `00472cd0` (Container_Load) is annotated as the port's `entity_assembly_parse` (another contract, C++ linkage); the unit calls `gods98/entity.cpp`'s `container_load`, the C adapter `lego_unit.c` and `advisor.c` already use |
| `0040be10` | 75.00 | 51 / 53 | the original passes the position's `y` and `z` to Container_SetPosition through `fld`/`push ecx`/`fstp [esp]` and `x` through `edx`; MSVC 5.0 copies all three through integer registers here. A scan of the original's `.text` for this `fld [reg+8]`/`fstp [esp]` argument copy finds it only here, in `0040cce0` and in GAME-011's `0040bb90`. Tried: `float*` and `const` pointers, `(*p).x`, casts, `+ 0.0f`, `* 1.0f`, union members (as `D3DVECTOR`), `#pragma pack`, `#pragma optimize("p")`, and 0-1,000 inert declarations (no rule B phase). The ternary that picks the ice or normal boulder became an `if`/`else`, which took it from 61.39% and gives the original's size; permuter (100 s): no candidates |
| `0040cce0` | 60.69 | 71 / 74 | the same x87 argument copies, twice (position and direction); the rest matches |
| `0040c3a0` | 50.73 | 102 / 103 | MSVC strength-reduces the completed-rock-fall stores (`types[count]`, `indexes[count]`) into a pointer and spills `count`; the original keeps `count` in `ebx` and indexes `[ebx*4 + base]`. Tried: `int` counters, declaration orders, `count++` in the index, local pointers to the two arrays, `continue`, a `do`/`while` inner loop, index loops for the fence beams, `#pragma optimize` variants (`g`, `a`, `w`, `y`, `p`), and 0-256 inert declarations. All give the same 50.73%; permuter (150 s): 2 candidates, none better |

## The unit's edges

The unit map gives `0040bdc0` and `0040bde0` low and `0040cce0` medium
confidence. All three belong to the unit, which is `0040bdc0`-`0040cdc0`:

- **Data.** The disassembly of `0040b7d0`-`0040cdc0` was scanned for
  references into `.rdata`, the string block and `.bss`. The functions before
  `0040bdc0` (GAME-011's low-confidence `0040b7d0`-`0040bd60`) reference only
  GAME-011's `.rdata` constant `0049e0d4`, and nothing of this unit. From
  `0040bdc0` on, every reference is to this unit's `.rdata`
  (`0049e104`-`0049e10c`: `1.0f`, `0.0f`, `12.5f`), its strings
  (`004a1dcc` `"RockFallStyles"` to `004a1e80` `"LazerHit"`, plus the pooled
  `"LWS"`, `","` and `"LWO"`), its `.bss` (`004c7180`-`004c7df8`) or Lego.c's
  commons.
- **`0040bdc0`** has no data of its own: it calls `0040c3a0` (Effect_UpdateAll)
  with an elapsed time of `10000.0f`, which ends every effect.
- **`0040bde0`** reads the boulder's container (LegoObject `+0x1c`) and calls
  `0040be10`, the boulder explosion.
- **`0040cce0`** calls `0040c8a0` and clones into the 0x38-byte MiscObjects
  record that `0040c920`, `0040c990` and `0040cb70` use.
- **The next function**, `0040cdc0`, starts GAME-013 and its `.bss`
  (`004c7df8`). The loop bound `004c7e5c` in `0040c3a0` is
  `&fenceBeams[2].finished`, the end of this unit's array, not a GAME-013
  item.

## How the conversion is done

### State

The unit's `.bss` is one `static Effect_Globs effectGlobs` at `004c7180`,
0xc78 bytes, laid out as the original:

- the completed rock falls' types and indexes (`004c7180`, `004c71b0`, twelve
  each), which Effect_UpdateAll returns;
- the rock falls, `[4 styles][3 types]` of 0xa4-byte `Effect_RockFall`
  (`004c71e0`): the loaded container, four clones, their block positions and
  finished flags, the part name and count, and a bit mask of used parts per
  instance (`+0x94`); `+0x4c`-`+0x93` is never touched here;
- the style names, count and current style (`004c7990`-`004c79a4`; the last is
  the port's `s_rockfall_base`, which loses its annotation);
- ten 0x38-byte `Effect_Misc` records (`004c79a8`), the four boulder
  explosion and four smash path containers (`004c7bd8`, `004c7be8`), two
  0xf4-byte fence beams (`004c7bf8`) and the explosion template, its four
  clones and their count (`004c7de0`-`004c7df4`).

The float constants are literals; MSVC pools them in this object. The unit's
strings are marked where they are used. `"LWS"` and `","` stay `advisor.c`'s.
`"LWO"` (`004a1d5c`) lies outside every unit's string range and had no marker;
it is marked here.

`make unit-data-offsets`, GAME-012 row: `.data` partial (13 strings, 0
unknown), one gap from `"UpgradeEffect"` (`004a1de4`, offset `0x18` against
`0x1c`); `.rdata` and `.bss` n/a (literals, one item).

### Source shapes that mattered

- A nested call as an argument (`f(g(x), …)`) makes MSVC push the outer
  call's other arguments first; the original calls the inner function first.
  A temporary (`cont = …; f(cont, …)` and `name = format(…); search(…, name,
  …)`) took `0040beb0` from 95.24% to 100%, `0040c0d0` from 86.90% to 97.62%
  and `0040c5b0` from 88.24% to 100%.
- `0040c5b0` compares `bx == rockFall->blockX[j]` (`cmp ebx, [eax]`), not the
  reverse, and reuses its `by` parameter as the part counter, whose address
  goes to Container_FormatPartName (the original's counter lives in that
  parameter's stack slot).
- `0040c750` compares the explosion count, an `int`, with an unsigned bound
  (`sizeof` of the array), as the original's `jae` shows; `0040c830` compares
  it signed.
- `0040bfd0`'s token loop counter is an `unsigned short` (`and eax, 0xffff`).
- A `// STRING` marker covers one literal per line, so `0040c990` puts each
  literal on its own line; with two markers on one line Reccmp loaded no
  annotation for the function.

### Callers: link names and three aliases

The port's effects are another contract: hook tables (`s_resource_child_create_hook`,
`s_scene_effect_pose_hook`, …), its own slot records in `rock_fall_styles_port.cpp`
and `gods98/resource.cpp`, and NULL and bound guards. `rock_fall_styles.cpp` is
renamed `rock_fall_styles_port.cpp`, a `tail` entry, and each `// FUNCTION`
becomes a `/* Port model of 0x… */` comment.

As in findings 581 and 585, `rock_fall_styles_link_names.h` renames 15 of the
unit's definitions to the port's names in the matching build only, where every
caller passes the original's arguments (style and type indices, block
coordinates, world positions and directions, the root container, the config
and the game name): C units (`lego_unit.c`, `legoobject_unit.c`, `weapons.c`,
`terrain_cell_objects.c`) and C++ port models (`lava_erosion.cpp`,
`lego_port.cpp`, `level.cpp`, `legoobject.cpp`, `weapons_port.cpp`,
`terrain_cell_objects_port.cpp`, `simtick.cpp`, `engine.cpp`) alike. Three of
the names are `PORT-ONLY` adapters that already had the original's contract for
`lego_unit.c`: `rock_fall_initialise_adapter` (`0040bfd0`, in `lego_port.cpp`),
`global_resource_handle_load_and_enable_original` (`0040c720`) and
`misc_objects_effect_slots_bind_original` (`0040c990`). They were no-ops or
path-only forwarders, so in the matching build the effects are now loaded at
all. The port tail compiles its definitions of the 15 names out under
`_MSC_VER < 1200`; natively it keeps serving them.

Three entries keep the unit's names, and `aitask_port_aliases.h`, which
`lego_unit.c` and `legoobject_unit.c` already include first, points the C
callers at them with matching-build `#define`s (no new `#include`, so no
symbol counter moves, finding 582):

- `0040bde0` and `0040beb0`: a C++ port model (`legoobject.cpp`'s
  `entity_type5_finalize_if_allowed`, and the port's own `0040bde0`) passes the
  port names a scene object, not a LegoObject;
- `0040c3a0`: its port model is defined in `gods98/resource.cpp`, outside this
  unit's files, and `simtick.cpp` runs it on the port's state.

The other nine functions are called only within the unit in the original, or
by `0040c750`'s caller below; their port names keep C++ linkage or another
contract and are untouched.

### Callees

All callees already had C declarations: `gods98/config.h`, `container.h`,
`entity.h` (Container_Load), `record.h`, `texture.h`, `utils.h`, `view.h` and
`terrain_map.h`. `rock_fall_styles_imports.h` declares the rest with the
port's types: the four Lego.c commons, Lego_GetLevel, Lego_GetMap (through an
unprototyped type in C, as `sfx_imports.h`), two LegoObject.c functions,
`0044a200` and Container_GetPosition. It is checked in
`test_c_boundaries.cpp`. No declaration is added to a shared header and no
`extern "C"` was needed.

## Entropy and drops

`make reccmp-check` against `main`'s baseline (`984a1213`): **none below
baseline, no exact match lost**, so no dropped function's instruction count
changed. Outside the unit four Lego.c functions rose, from call operands that
now map:

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `0041fb50` | `lego_game_config_init` | 93.67 | 93.71 |
| `00422850` | `lego_misc_object_resources_load` | 90.00 | 94.60 |
| `00424600` | `lego_terrain_cell_flag_cleanup` | 98.90 | **100.00** |
| `004519b0` | `model_entry_record_load` | 95.24 | 96.43 |

Instruction counts that differ from the original are listed in "What is left"
(`0040be10` +2, `0040c3a0` +1, `0040cce0` +3); every other unit function has
the original's count. No `0x800000` immediate is involved (plain-immediate
check OK).

## `opt_ref` (finding 579, `make opt-ref-callers`)

| | `main` `984a1213` | This change |
| --- | ---: | ---: |
| `opt_ref.annotated.full`: removed / exact removed | 585 / 275 | **574 / 269** |
| `opt_ref.annotated.game_only`: removed / exact removed | 568 / 264 | 557 / 258 |
| GAME-012's object | `rock_fall_styles.cpp.obj`: 5 removed (0 exact) | `rock_fall_styles.c.obj`: 1 removed (1 exact) |

Before, `/OPT:REF` removed `0040bfd0`, `0040c0d0`, `0040c920` and `0040c990`
because `lego_unit.c` called no-op adapters, and `0040c750`. Now 26 of the 27
unit functions are live. The one left is `0040c750` (Effect_Spawn_Explosion):
its only original caller, `00447ea0`, is a `legoobject.cpp` ledger stub (owned
by #615) that calls port models instead (finding 579 class (c)).

## Files outside the unit

| File | Change |
| --- | --- |
| `src/game/aitask_port_aliases.h` | three matching-build `#define`s (`0040bde0`, `0040beb0`, `0040c3a0`) |
| `src/game/lego_port.cpp` | `rock_fall_initialise_adapter` compiled out of the matching build |
| `src/game/test_c_boundaries.cpp` | includes `rock_fall_styles_imports.h` |
| `src/link_order.cmake`, `src/game/meson.build` | `unit GAME-012 game/rock_fall_styles.c`; `tail game/rock_fall_styles_port.cpp` |

**Files other threads own.** None is edited: `legoobject_unit.c`,
`legoobject.cpp`, `legoobject_port_aliases.h` (#615), `vehicle_model.cpp`
(#617) and `gods98/wad.cpp` (#618). `legoobject_unit.c` reaches the unit
through the link names and `aitask_port_aliases.h`; `legoobject.cpp`'s calls
keep their port names (`lava_effect_select_and_place` now reaches the unit in
the matching build through its link name, with the original's arguments).
`gods98/resource.cpp`, which holds the port's `0040c3a0` model next to WAD
code, is not edited.

## Native behaviour

Unchanged. Native callers reach the port tail under the same names; the unit
compiles natively as C++ under its own names, which nothing calls. No new
native gap, so no `port` issue is filed.

## Validation

On `main` `984a1213`:

- `make reccmp-check`: OK, none below baseline, no exact match lost;
  plain-immediate check, `winapi-link-check` and the link-order map check OK.
- `make match-progress` (baseline, listing digests, `binary-progress.json`),
  `make unit-data-offsets`, `python3 tools/translation_units.py build`,
  `make reccmp-lint-update` (ledger 1,314 to 1,312: the port tail's two
  static helpers) and `tools/binary_progress_hash.py --write-manifest`, then
  `make match-progress` again so all three files carry the final input hash.
- `make tools-test` (no `--allow-growth`), native build and `ctest` (37/37,
  synthetic fixtures).
- `gcc -std=gnu89 -Werror=declaration-after-statement -fsyntax-only` on
  `rock_fall_styles.c`.

## Reproducing

    git mv src/game/rock_fall_styles.cpp src/game/rock_fall_styles_port.cpp   # and make it a tail entry
    python3 tools/convert_unit.py GAME-012 --stem rock_fall_styles --force    # keep the old rock_fall_styles.h
    make match-function SOURCE=src/game/rock_fall_styles.c
    make reccmp-check
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object rock_fall_styles.c.obj"
    make match-progress
    make unit-data-offsets
