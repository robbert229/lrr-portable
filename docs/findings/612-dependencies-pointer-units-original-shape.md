# Finding 612 — Dependencies.c (GAME-010) and Pointer.c (GAME-033) rebuilt in original shape, with port tails

Issue #663, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
Two small units are converted in one change. Each is its own object and has
its own link-order entry.

- **GAME-010, `Dependencies.c`** (text `0040ab10`-`0040b310`, 11 functions)
  is the object dependency table from Lego.cfg's `Dependencies` block. Each
  entry names an object type and index, optionally with `HitOnceStay` and a
  level, and lists the objects it requires, each optionally with a level.
  Dependencies_Update re-checks every entry against the live objects, and
  the Interface and HelpWindow units read the result.
- **GAME-033, `Pointer.c`** (text `0045c830`-`0045cdc0`, 9 functions) is the
  mouse pointer. Lego.cfg's `Pointers` block gives each of the 56 pointer
  types a bitmap, or a flic with a hotspot offset. A pointer set with a timer
  holds until the timer runs out.

#489 (GAME-010) and #494 (GAME-033) relocated them verbatim into `src/game/dependencies.cpp` and
`src/game/pointer.cpp`. This change rebuilds them as C units:

| Unit | Source | Headers | Port tail |
| --- | --- | --- | --- |
| GAME-010 | [`dependencies.c`](../../src/game/dependencies.c) | [`dependencies.h`](../../src/game/dependencies.h), [`dependencies_imports.h`](../../src/game/dependencies_imports.h), [`dependencies_link_names.h`](../../src/game/dependencies_link_names.h) | [`dependencies_port.cpp`](../../src/game/dependencies_port.cpp) |
| GAME-033 | [`pointer.c`](../../src/game/pointer.c) | [`pointer.h`](../../src/game/pointer.h), [`pointer_imports.h`](../../src/game/pointer_imports.h), [`pointer_link_names.h`](../../src/game/pointer_link_names.h) | [`pointer_port.cpp`](../../src/game/pointer_port.cpp) |

"Before" is the *match baseline* on `main` at `4862ceaa` (#659). "After" is
this change, measured on the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| GAME-010 similarity (11 functions, 2,048 bytes, span-weighted) | 28.21% | **100.00%** |
| GAME-033 similarity (9 functions, 1,424 bytes, span-weighted) | 57.98% | **98.35%** |
| Exact functions, GAME-010 | 5 (4 of them naked asm) | **11** (no asm) |
| Exact functions, GAME-033 | 4 | **7** |
| Exact-function code bytes (`binary-progress.json`) | 190,333 | **192,532** |
| Accuracy, full / game-only | 86.18% / 84.81% | 86.48% / 85.15% |
| `opt_ref` annotated (full, by address): removed / exact removed | 477 / 246 | 480 / 248 (see [`opt_ref`](#opt_ref-finding-579-make-opt-ref-callers)) |
| Reccmp lint ledger (entries) | 1,222 | **1,220** |

The names follow OpenLRR's `Dependencies_*` and `Pointer_*` functions,
spelled in the repository's snake case. They are `inferred`. "Link name" is
the name a unit definition takes in the matching build (see
[Link names](#link-names)).

### GAME-010 Dependencies.c

| Address | Function | Link name | Span | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `0040ab10` | `dependencies_set_enabled` | `global_mode_zero_input_set` | 32 | 100.00 | **100.00** |
| `0040ab30` | `dependencies_reset_clear_all_level_flags_10c` | `grid_record_flag_mask_clear` | 64 | 50.00 | **100.00** |
| `0040ab70` | `dependencies_initialise` | `dependencies_initialise_adapter` | 816 | 11.76 | **100.00** |
| `0040aea0` | `dependencies_object_requirements_met` | `ensure_cell_entries_loaded` | 160 | 26.58 | **100.00** |
| `0040af40` | `dependencies_live_object_callback` | `terrain_cell_resource_entry_matches` | 80 | 25.97 | **100.00** |
| `0040af90` | `dependencies_object_get_requirements` | `dependencies_get_requirements` | 112 | 100.00 (asm) | **100.00** |
| `0040b000` | `dependencies_object_find_dependents` | `help_window_dependencies_matches_adapter` | 432 | 8.12 | **100.00** |
| `0040b1b0` | `dependencies_update` | `help_window_dependencies_pass_adapter` | 160 | 9.62 | **100.00** |
| `0040b250` | `dependencies_object_is_unlocked` | `help_window_dependencies_flag4_adapter` | 80 | 100.00 (asm) | **100.00** |
| `0040b2a0` | `dependencies_object_add_level_flag_100` | `help_window_dependencies_flag100_set_adapter` | 64 | 100.00 (asm) | **100.00** |
| `0040b2e0` | `dependencies_object_get_level_flag_100` | `help_window_dependencies_flag100_adapter` | 48 | 100.00 (asm) | **100.00** |

### GAME-033 Pointer.c

| Address | Function | Link name | Span | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `0045c830` | `pointer_initialise` | `ui_pointer_cursor_name_registry_init` | 576 | 100.00 | **100.00** |
| `0045ca70` | `pointer_load_pointers` | `pointer_cursor_config_load_target` | 352 | 16.09 | 98.18 |
| `0045cbd0` | `pointer_get_type` | `pointer_cursor_config_parse_target` | 64 | 24.56 | **100.00** |
| `0045cc10` | `pointer_get_image` | `indexed_global_value_at_get` | 16 | 37.50 | **100.00** |
| `0045cc20` | `pointer_set_current_if_timer_finished` | `media_pointer_cursor_select` | 32 | 100.00 | **100.00** |
| `0045cc40` | `pointer_set_current` | `media_pointer_cursor_override_set` | 32 | 100.00 | **100.00** |
| `0045cc60` | `pointer_get_current_type` | `global_cursor_word_get` | 16 | 100.00 | **100.00** |
| `0045cc70` | `pointer_draw_pointer` | `pointer_cursor_draw_target` | 288 | 16.90 | 94.05 |
| `0045cd90` | `pointer_update` | `media_pointer_cursor_override_tick` | 48 | 88.89 | **100.00** |

Five callers outside the units rose with no source change:

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `00418830` | `building_upgrade_state_changed_dispatch` (HelpWindow.c) | 95.71 | **100.00** |
| `00418920` | `help_window_list_dependencies` (HelpWindow.c) | 96.00 | **100.00** |
| `0041eca0` | `interface_draw_dependencies` (Interface.c) | 50.20 | 51.00 |
| `0041ee80` | `interface_get_dependency_at_point` (Interface.c) | 63.11 | 63.57 |
| `0041fb50` | `lego_game_config_init` (Lego.c) | 93.84 | 93.87 |

### What is left

Every remaining difference in Pointer.c is a callee operand. Every
instruction matches.

- **`0045ca70`** (98.18%): Image_LoadBMPScaled (`0047d9d0`) and Flic_Setup
  (`00483c80`). The unit calls the unannotated adapters
  `image_load_bmp_scaled` (`images.h`) and `flic_setup` (`flic.h`), as
  `bubbles.c` and `helpwindow.c` do. The port's `flic_open`, annotated at
  `00483c80`, takes only a name.
- **`0045cc70`** (94.05%): Image_DisplayScaled (`0047dcb0`, called twice),
  the flic's width and height (`004897d0`, `004850e0`) and Flic_Animate
  (`00484070`). These are `images.h`'s and `flic.h`'s adapters. The port's
  `object_field10_get` at `004850e0` has C++ linkage in `gods98/record.hpp`.
  Adding `extern "C"` there would move the symbol counter of every unit that
  includes it (finding 538, rule B), so the unit leaves it alone.

Converting Images.c (G98-011) and Flic.c (G98-016) closes both.

No naked asm, and no `Deviation (scoring only)`. Neither unit uses the flag
`0x800000`, and the plain-immediate check (finding 584) passes.

## Unit extents

### GAME-010 starts at `0040ab10`, not `0040ab70`

The unit map put `0040ab10` and `0040ab30` at the end of GAME-009 (the
screen-effect quads, #660). They are Dependencies.c's:

- `0040ab10` (OpenLRR's Dependencies_SetEnabled) writes `!on` to
  `004c7178`;
- `0040ab30` (Dependencies_Reset_ClearAllLevelFlags_10c) clears `0x10c` in
  every level flag of the table, from `004b8c4c` with stride `0xc4`;
- both addresses lie inside the unit's `.bss` block (below). Nothing else
  in GAME-009 references the block.

`translation-units.spec.csv` now starts GAME-010 at `0040ab10`. GAME-009's
row note (its `.bss` up to `0x4b8be0`) belongs to #660, which agreed to end
GAME-009 at `0040ab10`, with its `.bss` at `0x4b8a58`-`0x4b8bc4`.
`python3 tools/translation_units.py build`: 0 conflicts.

### Data

- **GAME-010.** One `.bss` block, `004b8bc8`-`004c717c` (`dependencyGlobs`,
  `0xe5b4` bytes): 20 object types × 15 indices of `0xc4`-byte records,
  then the `disabled` word at `004c7178`. The next object's state,
  `effectGlobs` (Rock Fall, `rock_fall_styles.c`), starts 8-aligned at
  `004c7180`. The unit owns the literals `"HitOnceStay"` (`004a1d8c`) and
  `"Dependencies"` (`004a1d98`). `":"` (`004a1cd4`) is `config.c`'s
  `s_config_rgb_separator` (finding 581). `","` (`004a13e4`) is pooled and
  marked in `advisor.c`.
- **GAME-033.** One `.bss` block, `00500a98`-`00500f00` (`pointerGlobs`,
  `0x468` bytes), the 56 `Pointer_*` literals `004a8a14`-`004a8ed4` and the
  `.rdata` `0.0f` at `0049e64c`. A plain `0.0f` literal matches the `fcomp`
  operand in both comparisons, so no annotated static is needed. `"%s"`
  (`004a1d60`) is pooled and marked in `files.c`.

Dependencies_Object_FindDependents and the level-flag reset end their loops
at a pointer past the table (`004c71f8` and `004c71fc`), which falls inside
Rock Fall's `effectGlobs`. The rebuilt `.bss` places `effectGlobs` the same
way after `dependencyGlobs`, so both compares match exactly in the full link.
`make match-function` cannot show this because its synthetic image has no
neighbour.

`make unit-data-offsets`:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-010 | gaps-only (2, 0 unknown) | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | - |
| GAME-033 | n/a (0, 56 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | n/a (1, 0 unknown) fewer than two attributable items | - |

Unit summary: full 0, gaps-only 10, partial 22, n/a 195.

GAME-033's 56 literals show as unknown because the linker takes each
literal's COMDAT from the first object that references it.
`game/pointer_cursor.hpp`, the port's registry, carries the same 56
strings and is included earlier in the link, by `screen_effect_quads.cpp`.
Its map entry names that object. Reccmp still pairs every literal, and
`0045c830` is exact. This layout difference ends when the port's pointer
registry leaves the link.

The image's `.bss` grows by 59,904 bytes (1,608,544 to 1,668,448). That is
`dependencyGlobs` and `pointerGlobs` sitting beside the port's own state,
which stays in the port tails. The plain-immediate check still passes.

## How the conversion is done

### Port tails

As in findings 597-607, `dependencies.cpp` and `pointer.cpp` are renamed
`dependencies_port.cpp` and `pointer_port.cpp` and become `tail` entries,
also in Meson. Each `// FUNCTION` becomes a `/* Port model of 0x… */`
comment, and each `// GLOBAL` a `/* Port state of 0x… */` comment.
`dependencies_port.cpp`'s four naked-asm table accessors (`0040af90`,
`0040b250`, `0040b2a0`, `0040b2e0`) keep only their C bodies. Their C++
names serve `level.cpp`'s wrappers.

### Link names

In the matching build, each unit's `*_link_names.h` gives its functions the
port's names (tables above). All 20 port names have C linkage, and every
caller passes the original's arguments: an enable word; the config and game
name; an object type, index and level, with the requirement outputs or the
four-array dependents record; a live object and a requirement; a Pointers
config item; a pointer name or type, with a timer; the mouse position; or
the elapsed time. So these callers reach the units with no caller edit:

- GAME-010: `lego_unit.c`, `aitask.c`, `helpwindow.c` and
  `interface_unit.c`; `frameloop.cpp`, `aitask_port.cpp`,
  `interface_port.cpp` and `dependency_grid.cpp`;
- GAME-033: `lego_unit.c`, `info_unit.c` and `reward.c`; `frontend.cpp`,
  `ui_list.cpp`, `mission.cpp` and `reward_port.cpp`.

Compiled out of the matching build, each with a plain
`#if !(defined(_MSC_VER) && _MSC_VER < 1200)`:

- `dependencies_port.cpp`'s `ensure_cell_entries_loaded` and
  `terrain_cell_resource_entry_matches` (`LRR_DEPENDENCIES_PORT_NAMES`), and
  `pointer_port.cpp`'s nine definitions and their two static helpers
  (`LRR_POINTER_PORT_NAMES`);
- `screen_effect_quads.cpp`'s `global_mode_zero_input_set` and
  `grid_record_flag_mask_clear` (now `// PORT-ONLY`);
- the `PORT-ONLY` adapters `dependencies_initialise_adapter`
  (`lego_port.cpp`) and the five `help_window_dependencies_*_adapter`s
  (`helpwindow_port.cpp`);
- `interface_port.cpp`'s stub `dependencies_get_requirements`.

Natively the units keep their own names and nothing calls them. The port
tails and the PORT-ONLY adapters still serve every port name, so native
behaviour is unchanged.

Reccmp reads annotations without preprocessing them. So the annotations of
`0040ab10`, `0040ab30` and `004b8bc8` in `screen_effect_quads.cpp` become
comments, not annotations inside the `#if`.

### Callers and owned files

No caller's code changes. Outside the units and their port tails, this
change edits only `#if` blocks and comments:

- `screen_effect_quads.cpp` (#660's file): the three annotations and the
  `#if` around the two Dependencies.c functions, agreed with #660;
- `lego_port.cpp`, `helpwindow_port.cpp` and `interface_port.cpp`: the `#if`
  around the adapters and the stub;
- `test_c_boundaries.cpp`: the four new headers;
- `link_order.cmake`, `src/game/meson.build` and
  `translation-units.spec.csv`.

No file owned by #447 (`src/crt/`), #661 (`gods98/resource.cpp`) or #662
(`info_track_overlay.cpp`) is edited. `lego_unit.c`, `legoobject_unit.c`,
`construction.c` and the other converted units are unchanged. Their calls
reach the units through the link names.

### Callees

| Original | Called as | Unit |
| --- | --- | --- |
| Config_BuildStringID, Config_FindArray, Config_GetNextItem | `config.h` | Config.c (`config.c`) |
| Util_Tokenise `00477440` | `sound_variant_split` | Utils.c |
| Lego_GetObjectByName `0042e8b0` | `lego_entity_type_pair_lookup` | Lego.c (`lego_unit.c`) |
| LegoObject_RunThroughListsSkipUpgradeParts `004377b0`, `00439200` | `legoobject_unit.c` names | LegoObject.c |
| Stats_GetLevels `00469f40` | `building_upgrade_admission_check` (`entity_stats_link_names.h`) | Stats.c (`entity_stats.c`) |
| Image_SetupTrans `0047dbf0` | `image_surface_blit_transformed` | Images.c (`images_unit.cpp`) |
| Image_LoadBMPScaled, Image_DisplayScaled | `images.h`'s adapters | not converted |
| Flic_Setup, Flic_Animate, the flic's width and height | `flic.h`'s adapters | not converted |
| `_strcmpi`, `atoi`, `malloc`, `sprintf` | `crt/vc5names.h` | LIBC |
| Main.c's flags `00505f84`, legoGlobs.flags1 `00556ecc` | `s_engine_feature_word`, `legoGlobs_flags1` | Main.c, Lego.c |

Stats_GetLevels is declared by its matching-build name. The first link
failed on `_stats_get_levels` because `entity_stats.c` defines it as
`building_upgrade_admission_check` there. No `extern "C"` was added to any
existing declaration.

### Source shapes that matched

- **Dependencies_Reset_ClearAllLevelFlags_10c** (`0040ab30`): three nested
  `unsigned` loops over type, index and level. MSVC reduces the outer
  counter to the record pointer and compares it with the end of the table,
  as the original does.
- **Dependencies_Initialise** (`0040ab70`): requirement `i` is written to
  `list[numRequirements - i - 1]`, so the list is filled from its end. MSVC
  reduces this to the original's offset that falls by `0x10` each time. The
  full `dependencyGlobs.objects[objType][objID]…` expression at every store
  gives the original's recomputed index at each one. It matched on the first
  compile, apart from the literal annotation.
- **Dependencies_Object_FindDependents, Dependencies_Update** (`0040b000`,
  `0040b1b0`): `unsigned` loop counters. An `int` type counter gives `jl`
  where the original has `jb`.
- **Dependencies_Update**: the original's test is
  `if (objLevel == 1 && hasLevels) break;`, inverted from what the name
  suggests: an object with levels checks only level 0. It is kept as is.
- **Pointer_GetType** (`0045cbd0`): an `unsigned` index (`int` gives `jl`).
- **Pointer_LoadPointers** (`0045ca70`):
  `if (!(flags & REDUCEFLICS)) name++; else ok = 0;`. The other order puts
  the `xor esi, esi` first (95.45%).
- **Pointer_DrawPointer** (`0045cc70`): two float locals `x` and `y` copied
  into `pos`. The flic offsets add `x` and `y`, not `pos`. This reproduces
  the original's held `st(1)` and the reuse of the `mouseY` slot (62.42% →
  39.08% → 94.05%). The non-flic branch comes first
  (`isFlic[currType] != 1`).

`tools/msvc_permuter.py` was not needed: each near-miss closed with one of
the source changes above, or is a callee operand.

## Entropy and drops

`make reccmp-check` against the baseline at `4862ceaa`:

- 16 improved: the 11 unit addresses that were below 100% and the 5 callers
  above;
- **none below baseline**, and no exact match lost (`0040ab10` stays exact);
- the plain-immediate check, the winapi link check and the link-order map
  check pass.

Instruction counts changed only at the units' addresses (listing digests):

| Address | before | after |
| --- | ---: | ---: |
| `0040ab30` | 28 | 20 |
| `0040ab70` | 9 | 259 |
| `0040aea0` | 95 | 63 |
| `0040af40` | 41 | 35 |
| `0040b000` | 100 | 123 |
| `0040b1b0` | 54 | 64 |
| `0045ca70` | 149 | 110 |
| `0045cbd0` | 84 | 29 |
| `0045cc10` | 9 | 3 |
| `0045cc70` | 40 | 84 |

`0045c830` keeps 57 instructions; only its operand names changed. The
listings of the five callers that rose are unchanged.

## `opt_ref` (finding 579, `make opt-ref-callers`)

| | committed `main` | same-worktree `main` | this change |
| --- | ---: | ---: | ---: |
| `opt_ref.annotated.full` (by address): removed / exact removed | 477 / 246 | 500 / 257 | 480 / 248 |
| `opt_ref.annotated.game_only` (by address) | 452 / 227 | 475 / 238 | 455 / 229 |
| Removed annotated functions (by symbol, `callers --min 1`) | | 468 | 462 |

`make opt-ref-callers OPT_REF_ARGS="--object dependencies.c.obj"` and
`"--object pointer.c.obj"` list no removed function. All 20 are kept.

`main` and this change were built in the same worktree, and their removed
annotated functions were compared by symbol:

- **Kept now:** six old `dependencies.cpp` addresses, `0040af90`,
  `0040b000`, `0040b1b0`, `0040b250`, `0040b2a0` and `0040b2e0`. Four of
  them were exact. On `main` the original-mode relink dropped all six.
- **Removed now:** none.

The committed figure (477 / 246) is below a fresh build of the same commit
(500 / 257). This is the layout noise of finding 591. Against the
same-worktree build, this change is −20 / −9 by address.

## Reccmp lint ratchet

1,222 → 1,220 entries (−2, no growth). `pointer_port.cpp`'s two static
helpers (`pointer_cursor_slot_release`, `pointer_cursor_value_split`) are now
in a tail file, which the lint exempts. `make reccmp-lint-update` removed
their entries. The two `screen_effect_quads.cpp` definitions that lost their
annotations are marked `// PORT-ONLY`.

## Native behaviour

Unchanged:

- the port tails serve every port name natively, and the PORT-ONLY
  adapters and the Interface stub stay;
- the units compile natively as C++ under their own names, and nothing
  native calls them.

There is no new native gap.

**Matching-build runtime** (`make reccmp-run` only). The units' state and
the port's are separate. In the matching build, the original callers use
`dependencyGlobs` and `pointerGlobs`. The port's readers read the port's own
state: `dependency_grid.cpp`'s level sidecar, `level.cpp`'s table wrappers,
`media.cpp`'s cursor getters, `ui_list.cpp`'s name table and `crt.cpp`'s
test writers. This is the split that findings 590, 592 and 607 accept until
the callers' units are converted.

## Rebased onto #664 (`76c6d2e9`)

The stack puts this change on #664 (the LIBC.LIB CRT prototype, finding
608). That change takes `operator new` from LIBC and deletes the
`__allmul@16` bridges. Neither unit uses them. The rebase had no source
conflicts, and the generated files were regenerated with `make
match-progress`. Against #664's head:

| | #664 head | this change |
| --- | ---: | ---: |
| Exact-function code bytes | 190,333 | **192,532** |
| Accuracy, full / game-only | 86.18% / 84.80% | 86.48% / 85.14% |
| `opt_ref` annotated full: removed / exact removed | 480 / 248 | 480 / 248 |
| `opt_ref` annotated game-only | 454 / 228 | 454 / 228 |
| Reccmp lint ledger (entries) | 1,221 | 1,219 |

`make reccmp-check` reports the same 16 improvements, none below baseline,
and no exact match lost. `make unit-data-offsets` gives the same GAME-010
and GAME-033 rows. `make tools-test`, `make winapi-link-check` and native
ctest (37 of 37) pass.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK. None below
  baseline and no exact match lost.
- `make match-progress`, `make unit-data-offsets`: run, and the generated
  files are committed. `tools/binary_progress_inputs.txt` was rewritten for
  the new sources (`binary_progress_hash.py --write-manifest`).
- `make winapi-link-check`: OK.
- `make tools-test`: passes (no `--allow-growth`).
- Native: `cmake --build build` and `ctest --test-dir build`: 37 of 37
  pass.
- C subset: `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only` on `dependencies.c` and `pointer.c`.
