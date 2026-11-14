# Finding 652 — ObjectRecall.c, PTL.c and GAME-008 converted to original shape

Issue #758, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
Three units had been relocated verbatim and partly rebuilt in their `.cpp`
files under `#if` blocks for the matching build (findings
[642](642-gate-takes-the-original-link-mode.md) and
[648](648-lego-legoobject-near-miss-sweep-9.md)). They were never
converted. This change:

- rebuilds GAME-030 as **ObjectRecall.c** ([`src/game/objectrecall.c`](../../src/game/objectrecall.c));
- rebuilds GAME-035 as **PTL.c** ([`src/game/ptl.c`](../../src/game/ptl.c));
- rebuilds GAME-008, which is still unnamed, in
  [`src/game/entity_cell_effects.c`](../../src/game/entity_cell_effects.c),
  with
  [`entity_cell_effects_link_names.h`](../../src/game/entity_cell_effects_link_names.h).

Each unit has a C header. The relocated files move, unannotated, to the
port tails `object_state_file_port.cpp`, `level_id_remap_port.cpp` and
`entity_cell_effects_port.cpp` (`git mv`, `tail` entries in
`src/link_order.cmake`, renamed in `src/game/meson.build`). Step 0
(`tools/convert_unit.py`) scaffolded the three units after the old unit
entries were taken out of the link-order list.

"Before" is `main` `f88df193` (#763, finding 656), whose figures this change
was rebased onto. "After" is this change, from the full matching build
(`make reccmp-check`). The per-function scores of these units and their
callers are the same on `1e84f9da`.

## Result

| Measure | `main` `f88df193` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 329,992 (51.36%) | **330,196 (51.39%)** |
| Reccmp accuracy, full / game-only | 96.99% / 96.64% | **97.00% / 96.65%** |
| Exact / mapped functions (full) | 2,399 / 2,833 | **2,401** / 2,833 |
| Function-order inversions, full: adjacent / pairs | 43 / 12,327 | **39 / 12,261** |
| Function-order inversions, game-only: adjacent / pairs | 12 / 65 | **11 / 63** |
| `opt_ref` annotated removed (full) | 0 | 0 |
| Image end (`make pe-audit`) | `0x7f5000` | `0x7f5000` |

`make reccmp-check`: two functions rise and **none falls**. No exact match
was lost, so there is no drop to label. The units use no naked asm and no
`Deviation (scoring only)`. The one naked-asm definition the units had,
`0045d8a0`, is now C.

### Extents and names

- **GAME-030 is ObjectRecall.c** (`00459190`–`00459530`, `.bss`
  `004ffe50`–`004ffe68`). The extent is unchanged. OpenLRR's name, already
  used by findings 635 and 648. The six `.bss` words are one
  `ObjectRecall_Globs` (the live table, its count and capacity, the shadow
  table, its count and the loaded flag). No code outside the range reads
  them.
- **GAME-035 is PTL.c** (`0045d7e0`–`0045d8b0`, `config-key`).
  `0045d7e0` loads the file that the level's `PTLFile` key names
  (`lego_port.cpp` reads that key for the native loader). Lego_LoadLevel
  passes its name and the game name. The state is the common `ptlGlobs`
  (`00555be0`–`00555d24`: 40 message-type pairs and the count). Only
  `0045d7e0` and `0045d870` read it. Before, it was `util.cpp`'s annotated
  port table. `0045d8a0` (RadarMap_SetZoom) reads no data. It stays at the
  end of this object, as finding 606 decided.
- **GAME-008 stays unnamed** (`00409d40`–`0040a0c0`). The binary names
  neither the file nor its functions, so the names describe what the
  functions do: `building_set_cost_drop_disabled`,
  `building_remove_from_terrain`, `building_drop_cost`,
  `object_tracked_replace_and_remove`. Its `.bss` word `004b8a54` is the
  level's switch for the building cost drop.

`translation-units.spec.csv` records the two names. `translation_units.py
build` regenerated the derived CSVs.

## Per function

"Link name" is the name the definition takes in the matching build.

### ObjectRecall.c (GAME-030)

| Address | Function | Link name | Before | After |
| --- | --- | --- | ---: | ---: |
| `00459190` | `object_recall_store_minifigure` | (own) | 100.00 | 100.00 |
| `00459240` | `object_recall_recall_minifigure` | (own) | 100.00 | 100.00 |
| `004592a0` | `object_recall_reset` | (own) | 100.00 | 100.00 |
| `004592e0` | `object_recall_snapshot` | (own) | 100.00 | 100.00 |
| `00459360` | `object_recall_restore` | (own) | 100.00 | 100.00 |
| `004593d0` | `object_recall_is_loaded` | (own) | 100.00 | 100.00 |
| `004593e0` | `object_recall_save` | (own) | 100.00 | 100.00 |
| `00459470` | `object_recall_load_rrosfile` | (own) | 100.00 | 100.00 |

The original's name for `004592a0` (free the live table) is not known.
Every function matched at the first source shape, with the struct and with
Files.c's and Stats.c's original names (`file_*`,
`stats_object_set_object_level`, through `gods98/files.h` and
`entity_stats_link_names.h`).

### PTL.c (GAME-035)

| Address | Function | Link name | Before | After |
| --- | --- | --- | ---: | ---: |
| `0045d7e0` | `ptl_initialise` | (own) | 100.00 | 100.00 |
| `0045d870` | `ptl_translate_event` | (own) | 75.00 | **100.00** |
| `0045d8a0` | `radarmap_set_zoom` | (own) | 100.00 (naked asm) | 100.00 (C) |

- `0045d7e0` needs a pointer to the entry, as in finding 642's version:
  `property = &ptlGlobs.table[ptlGlobs.count++]`. Indexing twice reloads
  the count (56.07%).
- `0045d870` takes the entry's address inside the loop
  (`property = &ptlGlobs.table[i]`). The port's `NULL` check had held it at
  75%. With the pointer in the `for` initialiser, MSVC loads the table
  address before the count test (91.30%).

### GAME-008

| Address | Function | Link name | Before | After |
| --- | --- | --- | ---: | ---: |
| `00409d40` | `building_set_cost_drop_disabled` | `level_state_stored_value_set` | 100.00 | 100.00 |
| `00409d50` | `building_remove_from_terrain` | `entity_cell_path_effects_apply_current` | 100.00 | 100.00 |
| `00409f20` | `building_drop_cost` | (own) | 100.00 | 100.00 |
| `00409ff0` | `object_tracked_replace_and_remove` | `entity_destroy_tracked_pointer_cleanup` | 100.00 | 100.00 |

The unit reads `Lego_Object` (`legoobject_unit.h`) and calls its callees by
their original names, through their units' link-name headers (Building.c,
Stats.c, the effects unit, SelectPlace.c). Lego_GetCurrentMapTerrain
(`00431570`) is declared with no argument, as the original calls it.

`00409f20` keeps its own name. Its port callers (`level.cpp`) are C++, so
the port tail gives the matching build a `PORT-ONLY` C++ forwarder,
`entity_placement_commands_build_once`. The other three take the port names
that the C callers and the port's `extern "C"` callers already use, and the
port tail compiles its own `level_state_stored_value_set` out of that
build.

`match-function` scores four ObjectRecall.c and GAME-008 functions at
90–98% because, without a link, it compares callee names (`file_read`
against `wad_file_read`). The linked build scores them all at 100%.

## Callers

| Address | Caller | Callee | Before | After |
| --- | --- | --- | ---: | ---: |
| `00415220` | front_load_selected_level (FrontEnd.c) | `00459360` | 97.83 | **100.00** |
| `00429890` | lego_level_load (Lego.c) | `004593d0`, `00459470`, `0045d7e0`, `00409d40` | 95.04 | 95.04 |
| `00426250` | lego_update_render_viewport_state (Lego.c) | `0045d8a0` | 100.00 | 100.00 |
| `0042e020` | lego_object_list_load_file (Lego.c) | `00459240` | 73.91 | 73.91 |
| `004520d0` | cmd_queue_tick (Message.c) | `0045d870` | 100.00 | 100.00 |
| `0043bc40` | LegoObject.c | `00459190`, `00409d50` | 100.00 | 100.00 |
| `00458be0` | objective_tick_target (Objective.c) | `004592e0`, `00459360` | 60.56 | 60.56 |

Lego.c's two cause-A functions in finding 644 are `00429890` (6,752 bytes)
and `004250c0` (the spot setters, not these units). Findings 642 and 648
had already switched `00429890`'s OSF and `0045d7e0` calls to the
original-shape definitions, at 95.04. This change keeps those contracts
(the targets are now the C units) and moves no score. What is left in
`00429890` is cause D (stack slots), with the `dragBoxRGB - 4` end pointer
(cause B). Cause A no longer applies to it.

The callers switch by object-like aliases. No caller body changes, and
the symbol counter does not move:

- `lego_unit_imports.h`: `level_identifier_remap_table_load_original` →
  `ptl_initialise`, `radar_map_zoom_set_adapter` → `radarmap_set_zoom` (the
  aliases were `level_remap_table_load` and `object_field14_set`). The
  edits are comments and the two targets, and the line count is
  unchanged. #762 owns `lego_unit.c`'s bodies, which this change does not
  touch.
- `message_unit_imports.h`: `command_entity_reference_remap` →
  `ptl_translate_event` (matching build, C only).
- `frontend_unit_link_names.h`: `front_object_recall_restore_adapter` →
  `object_recall_restore`. This is finding 648's follow-up.
  `frontend_port.cpp`'s no-op adapter is now native-only. #761 owns
  `frontend_unit.c`'s bodies: the only edit there is one comment word.
- `legoobject_unit_imports.h`, `objective_unit_imports.h`: comments only.

## Native

The port's models stay the live ones natively, unannotated, in the three
port tails. Each original-shape function that was compiled out natively
is now in a C unit, so none of the tails keeps a matching-build block.
`util.cpp`'s port table loses its two `GLOBAL` markers, which now belong
to `ptlGlobs`. `cmake --build build` succeeds and ctest passes 37/37.
Native gameplay is unchanged (#552).

## Gate

- `make reccmp-check`: OK. Two rise, none fall. The image-end guard, the
  winapi link check and the link-order map check pass.
- `make match-progress` (after `git add` and
  `binary_progress_hash.py --write-manifest` for the new sources),
  `make unit-data-offsets` (no change),
  `make opt-ref-callers OPT_REF_ARGS=--summary` (0 removed) and
  `make pe-audit` were run. The match baseline, the listing digests and
  `binary-progress.json` are committed.
- `make tools-test` (without `--allow-growth`) and
  `make winapi-link-check`: OK.
- The three units pass `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only`.

## Files

- New: `src/game/objectrecall.{c,h}`, `src/game/ptl.{c,h}`,
  `src/game/entity_cell_effects.{c,h}`,
  `src/game/entity_cell_effects_link_names.h`.
- Port tails (renamed, unannotated): `object_state_file_port.cpp`,
  `level_id_remap_port.cpp`, `entity_cell_effects_port.cpp`.
- `src/game/util.cpp`: the port table's markers.
- Alias headers: `lego_unit_imports.h`, `message_unit_imports.h`,
  `frontend_unit_link_names.h` (and comments in
  `legoobject_unit_imports.h` and `objective_unit_imports.h`).
- `src/game/frontend_port.cpp`: `front_object_recall_restore_adapter` is
  native-only.
- `src/link_order.cmake`, `src/game/meson.build`,
  `src/game/test_c_boundaries.cpp`, `tools/binary_progress_inputs.txt`.
- `docs/coverage/`: the unit spec and the derived CSVs, the match baseline,
  the listing digests and binary progress.
