# Finding 621 — Teleporter.c (GAME-044) rebuilt in original shape

Issue #685, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-044 ("linked record chains", boundary `low`) is the original's
`Teleporter.c`: the teleporter services. Teleporter_Start marks every object
of the chosen types as teleporting. With the service mode it also adds a
service at the camera's position. A service is a ring that grows from that
point, or a one-in-N chance a frame. Teleporter_Update removes each
teleporting object once its service lets it go. Teleporter_ServiceAll
reports whether none of the chosen types is still teleporting. Lego.c starts
the level's pilots this way (`0041fa80`), and Objective.c teleports the
units out at the end of a level (`00458670`).

The unit map listed 9 functions. **Teleporter.c is 12 functions,
`0046a370`-`0046a7f0`.** The other three are the object callbacks
`0046a3c0`, `0046a490` and `0046a5c0`, which Ghidra left as labels because
they are reached only through `push imm32`.

This change:

- rebuilds the unit as the C unit [`src/game/teleporter.c`](../../src/game/teleporter.c),
  with [`teleporter.h`](../../src/game/teleporter.h),
  [`teleporter_imports.h`](../../src/game/teleporter_imports.h),
  [`teleporter_maths.h`](../../src/game/teleporter_maths.h) and the
  matching-build link names
  [`teleporter_link_names.h`](../../src/game/teleporter_link_names.h);
- moves the port's model, unannotated, to the port tail
  [`record_chains_port.cpp`](../../src/game/record_chains_port.cpp)
  (`record_chains.cpp` under `git mv`). The tail drops its naked-asm
  `linked_list_push_at_global_head`, which no longer carries an annotation.

Stacked on #689 (`fix/686-retire-sound-bank`, `75d6b613`). "Before" is
#689's committed match baseline and `binary-progress.json`; no GAME-044
score differs from `main` `9043af89`, where the change was first measured
(`opt_ref` there: 412 / 223). "After" is the full matching build
(`make reccmp-check`) of this change.

## Result

| | before | after |
| --- | ---: | ---: |
| Teleporter.c similarity (12 functions, byte-weighted, 1,079 code bytes) | 23.53% | **100.00%** |
| Exact functions in Teleporter.c | 3 | **12** |
| Mapped functions in Teleporter.c | 9 | **12** |
| Exact-function code bytes (`binary-progress.json`) | 205,613 | **206,155** |
| Exact functions, full / game-only | 1,731 / 1,522 | **1,740 / 1,531** |
| `opt_ref` annotated (full, by address): removed / exact removed | 411 / 222 | **409 / 221** |
| Reccmp lint ledger (entries) | 1,052 | **1,046** |

Byte weights are the original sizes from the single-function loop. An
unmapped function counts as 0%. No function uses naked asm, and there is no
`Deviation (scoring only)`.

The names are OpenLRR's, in the repository's snake case. They are
`inferred`: the binary has no `Teleporter_` string. "Link name" is the name
the definition takes in the matching build (see [Link names](#link-names)).

| Address | Function | Link name | Bytes | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `0046a370` | `teleporter_remove_all` | (its own) | 32 | 100.00 | **100.00** |
| `0046a390` | `teleporter_restart` | `global_linked_record_chain_clear` | 35 | 100.00 | **100.00** |
| `0046a3c0` | `teleporter_callback_start` (static) | (its own) | 89 | unmapped | **100.00** |
| `0046a420` | `teleporter_get_camera_position` | (its own) | 69 | 10.53 | **100.00** |
| `0046a470` | `teleporter_add_service` | (its own) | 19 | 100.00 (naked asm) | **100.00** (C) |
| `0046a490` | `teleporter_callback_check_type` (static) | (its own) | 44 | unmapped | **100.00** |
| `0046a4c0` | `teleporter_service_all` | `entity_flag_mask_callbacks_all_pass` | 71 | 96.88 | **100.00** |
| `0046a510` | `teleporter_start` | `scene_object_state_dispatch_original` | 170 | 19.67 | **100.00** |
| `0046a5c0` | `teleporter_callback_update` (static) | (its own) | 318 | unmapped | **100.00** |
| `0046a700` | `teleporter_update_services` | (its own) | 46 | 27.27 | **100.00** |
| `0046a730` | `teleporter_update` | (its own; Lego.c's imports point at it) | 45 | 23.53 | **100.00** |
| `0046a760` | `teleporter_get_service_object_type` | (its own) | 141 | 25.00 | **100.00** |

Three callers outside the unit rose, with no loss:

| Address | Function | Before | After | Why |
| --- | --- | ---: | ---: | --- |
| `0041fa80` | `lego_start_level_pilots` (Lego.c) | 88.24 | **96.08** | its four `0046a510` calls now reach the unit, not the tail's adapter |
| `004232e0` | `lego_engine_tick` (Lego.c) | 97.66 | 97.73 | calls `teleporter_update` instead of `lego_port.cpp`'s adapter |
| `00458670` | `objective_announce_current` (Objective.c) | 27.95 | 30.99 | the original's four Teleporter_Start calls (below) |

## Unit extent

The issue asked for both edges to be confirmed: `0046a370` was
`low` and `0046a760` bordered GAME-045.

- **The data.** The unit owns the 16 bytes of `.data` at `004aa450`:
  `{ 0, 40, NULL, 3.0f }`, which is teleporterGlobs (`teleportingCount`,
  `randomInterval`, `serviceList`, `ringSpeed`). They sit between Stats.c's
  last string (`Stats`, `004aa448`) and Text.c's first (`Text_UnitUpgraded`,
  `004aa460`). A scan of `.text` for each of the four words finds every
  reference inside `0046a370`-`0046a760`. The unit also owns the `.bss` word
  `00503188`, the "none teleporting" flag. It sits directly after Stats.c's
  statsGlobs (finding 581) and before Text.c's `.bss` (`00503190`). Its
  only reference outside the unit is Stats.c's `0046a14d`, which compares
  against its address to end a loop. The scan found no `.rdata` item: the
  one float the unit stores, `-1.0f`, is an immediate.
- **The start.** `0046a370` frees a service list through the services'
  `+0x1c` link, recursively. Only `0046a390` calls it (`0046a37d` is its own
  recursive call), on teleporterGlobs.serviceList. The functions before it,
  `0046a170`-`0046a330`, read the object's stats record (`+0x334`), so they
  are Stats.c's.
- **The end.** `0046a760` maps a TeleportObjectType bit to a
  LegoObject_Type. Only `0046a490` and `0046a519` call it, both in the unit.
  `0046a7f0` opens Text.c (GAME-045, `Text_*` strings).
- **The callbacks.** `0046a3c0` and `0046a490` are pushed by `0046a5a8` and
  `0046a4df`, and `0046a5c0` by `0046a74f`. Each is pushed only there, so
  they are the unit's static callbacks for
  LegoObject_RunThroughListsSkipUpgradeParts (`004377b0`).
- **The callers.** Outside the unit there are 13 call sites, and every one
  enters one of four functions. Lego.c calls `0046a390` (`004298b2`),
  `0046a510` (×4, `0041fad2`-`0041faff`), `0046a730` (`00423369`) and
  `0046a4c0` (`00435a61`). Objective.c calls `0046a510` (×4,
  `00458806`-`00458830`) and `0046a4c0` (×4, `0045475f`, `00458ac7`,
  `00458c6f`, `00458d14`).

`translation-units.spec.csv`: GAME-044 becomes `Teleporter.c`
(`inferred`), `0046a370`-`0046a7f0`. `python3 tools/translation_units.py
build`: 0 conflicts. The unit map still lists 9 functions, because it lists
Ghidra's functions, and the three callbacks have none (as Water.c's
comparator, finding 614).

## How the conversion is done

### Records

`teleporter.h` declares the original's records:

- TeleporterService, 0x20 bytes: `position`, `radius`, `speed`,
  `teleportingCount`, `randomInterval`, `flags` and `next`.
- Teleporter_Globs, 0x10 bytes.
- the Point2F that Teleporter_GetCameraPosition returns in `eax:edx`.

The unit keeps Teleporter_Start's search record (0x10 bytes) private.

The object's teleporter fields (`+0x390` mode, `+0x394` flags, `+0x398`
service) are read through `LEGOOBJ_FIELD`. `legoobject_unit.h` keeps them as
padding: naming them there would add declarations to every unit that
includes it (finding 538).

### Source shapes that matched

Eleven of the twelve functions matched as first written from the
disassembly. Teleporter_Callback_Update (`0046a5c0`) was 86.17% at first.
Two changes brought it to 100%:

- **`randomInterval` is signed.** The original divides Maths_Rand's `short`
  result with `idiv`, so the field is an `int`.
- **The distance is two inlines.** The ring test is gods98 Maths.h's
  Maths_Vector2DDistance, which calls Maths_Vector2DModulus on the
  difference. With the modulus written out in Distance, MSVC kept `c.x` in
  memory and squared it from there. With the separate Modulus inline, it
  keeps `c.y` in memory, as the original does. Both helpers are
  `static __inline` in `teleporter_maths.h`.

### Link names

In the matching build `teleporter_link_names.h` gives three functions the
port's names:

| Function | Link name | Matching-build callers |
| --- | --- | --- |
| `0046a390` | `global_linked_record_chain_clear` | `lego_unit.c` |
| `0046a4c0` | `entity_flag_mask_callbacks_all_pass` | `lego_unit.c`, `objective_unit.cpp`, `objective.cpp` |
| `0046a510` | `scene_object_state_dispatch_original` | `lego_unit.c`, `objective_unit.cpp` |

All three port names have C linkage, and every caller passes the original's
arguments. `record_chains_port.cpp` compiles its definitions of the three
out of the matching build (`LRR_TELEPORTER_PORT_NAMES`). Natively the tail
still serves them.

`lego_unit.c` calls Teleporter_Update as `camera_move_pipeline_original`.
That name's port definition is an adapter in `lego_port.cpp`, so
`lego_unit_imports.h` maps it to `teleporter_update` in the matching build,
as it does for Water.c (finding 614).

### Callers switched

| Caller | Before | After |
| --- | --- | --- |
| `objective_unit.cpp` `00458670` | 4× `scene_object_state_dispatch(ctx, type, 2, 4, NULL, NULL)` (the port's entry) | 4× `scene_object_state_dispatch_original(type, 2, flags)`, with `flags = DAT_004ffe48 ? 2 : 4` as in the original |

Before the switch, `make reccmp-check` dropped `00458670` from 27.95% to
25.75%. Its call target, the port's `scene_object_state_dispatch`, lost its
`0046a510` annotation, so the call operand no longer matched. After the
switch it scores 30.99%. The function reads `DAT_004ffe48` from its own
file's `s_value_004ffe48`. The port has no writer for it, so natively the
flags are still 4, as before. Natively `scene_object_state_dispatch_original`
is the tail's adapter, which passes the same game context the caller used.
The declaration is at file scope, before the function. No other function in
the file changed score.

### Callees

| Original | Called as | Unit |
| --- | --- | --- |
| LegoObject_RunThroughListsSkipUpgradeParts `004377b0`, LegoObject_GetPosition `00442260` | `legoobject_unit.h` | GAME-023 |
| Camera_GetTopdownWorldPos `00436b33` | `camera_topdown_position_get_adapter`, which is `camera_get_topdown_world_pos` in the matching build (as `lego_unit_imports.h`) | GAME-022 |
| Lego_GetMap `00431570` | an unprototyped call to `level_current_terrain_get` (as `camera_control_imports.h`); natively `level_current_terrain_get_original` | GAME-021 |
| legoGlobs.cameraMain, cameraFP, viewMode | `s_zoom_step_context`, `legoGlobs_cameraFP`, `s_context_mode` | GAME-021 |
| Maths_Rand `004798a0` | `rng_thunk_draw15` (`gods98/maths.h`) | G98 Maths.c |
| `malloc`, `free` | `crt/vc5names.h` | LIBC |

## Entropy and drops

`make reccmp-check` against #689's baseline:

- 9 improved: the unit's six and the three callers above;
- 3 newly mapped: `0046a3c0`, `0046a490` and `0046a5c0`;
- **none below baseline**, and no exact match lost.

Instruction counts changed only at these addresses (listing digests):

| Address | before | after |
| --- | ---: | ---: |
| `00458670` | 186 | 176 |
| `0046a3c0` | — | 27 |
| `0046a420` | 24 | 22 |
| `0046a490` | — | 13 |
| `0046a510` | 63 | 49 |
| `0046a5c0` | — | 91 |
| `0046a700` | 25 | 17 |
| `0046a730` | 9 | 14 |
| `0046a760` | 13 | 54 |

`0041fa80` and `004232e0` changed only their call operands, and their
listings are unchanged.

## `opt_ref` (finding 579, `make opt-ref-callers`)

| | before (#689, committed) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full` (by address): removed / exact removed | 411 / 222 | 409 / 221 |
| `opt_ref.annotated.game_only` (by address) | 404 / 215 | 402 / 214 |

`make opt-ref-callers OPT_REF_ARGS="--object teleporter.c.obj --object
record_chains_port.cpp.obj --min 1"` lists no removed annotated function in
either object. Every unit function is reached from Lego.c, Objective.c or
the unit itself.

## Reccmp lint ratchet

The ledger goes from 1,052 to 1,046 entries (−6, no growth). The tail's six
static helpers are exempt now that it is a port tail. The unit's two inline
maths helpers live in `teleporter_maths.h`, where they add no debt.

## Files outside the unit

The change edits no file owned by #682 (`video_graph.cpp`), #683
(`terrain_overlay_quads.cpp`), #684 (`grid_cell_records.cpp`) or #686
(`gods98/sound.cpp`, `sound3d.c`). It edits no frozen `_for_` or `_from_`
header. Outside the unit and its headers, the change edits:

- `objective_unit.cpp`: the four Teleporter_Start calls, and their
  declaration;
- `lego_unit_imports.h`: the `teleporter_update` mapping;
- `test_c_boundaries.cpp`: `teleporter.h` and `teleporter_imports.h`;
- `link_order.cmake` and `src/game/meson.build`: the unit and the tail;
- `translation-units.spec.csv` and its generated files, and
  `tools/binary_progress_inputs.txt`.

## Native behaviour

Native behaviour is unchanged. Natively the port tail serves every port
name, and `teleporter.c` compiles but nothing calls it. The port has never
teleported anything. Its `scene_object_state_dispatch` runs the per-object
step only with a callback, and every caller passes `NULL`. Its update walk
is gated on a flag that nothing sets. That gap is filed as #688 (`port`).

**Matching-build runtime** (`make reccmp-run` only): the level's pilots are
now marked teleporting at the start of a level, and Teleporter_Update
removes teleported objects. At the end of a level the units teleport out.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK. None below
  baseline, and no exact match lost. The plain-immediate check, the winapi
  link check and the link-order map check pass.
- `make match-progress`, `make unit-data-offsets`: run, and the generated
  files are committed. GAME-044 has one attributable `.data` item, so
  `unit-data-offsets` reports it as `n/a`.
- `make winapi-link-check`: OK.
- `make tools-test`: passes (no `--allow-growth`). `make reccmp-lint`:
  passes, 1,046 entries.
- Native: `cmake --build build` and `ctest --test-dir build`: 37 of 37
  pass.
- C subset: `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only` on `teleporter.c`.

## Reproduce

    python3 tools/convert_unit.py GAME-044 --stem teleporter --out <scratch>   # with the GAME-044 link-order entry removed: the unit was already wired
    make match-function SOURCE=src/game/teleporter.c
    make match-function ADDR=0x00458670
    make reccmp-check
    make opt-ref-callers OPT_REF_ARGS="--object teleporter.c.obj --object record_chains_port.cpp.obj --min 1"
