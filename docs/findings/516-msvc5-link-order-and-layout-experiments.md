# Finding 516 — How MSVC 5.0's linker orders objects, and what that means for the matching build

Issue #388. These are the facts behind
[ADR 0006](../adr/0006-original-link-order.md) and the
[restructuring plan](../plans/388-original-link-order.md). There are three
parts:

- a small link experiment with the matching toolchain (MSVC 5.0 RTM `cl`,
  `link` and `lib` under Wine);
- measurements of an existing matching build that is source-identical to
  `main` at `041ab287`;
- an inventory of the unit graph and of the layout devices already in `src/`.

Nothing under `src/` or the build changed.

## Summary

1. LINK 5.0 places explicit objects in **command-line order**, in every
   section. That order is deterministic, so it is the link-order mechanism to
   use.
2. Library members are placed in **pull order**, and pull order follows the
   references. Today's matching build is laid out this way. It cannot
   reproduce the original's order unless every object references the same
   symbols in the same order.
3. `/ORDER` works only for external COMDAT functions. It cannot name `static`
   functions, and it does not order data.
4. `/O2` packages every function as a 16-byte-aligned COMDAT and every string
   literal as a `.data` COMDAT. When several objects define a literal, the
   **first object in link order** keeps it.
5. The compiler emits a `static` function **after its first caller**, not
   where the source defines it.
6. Common symbols are not allocated in object order. The order changed with
   the link inputs in a way three links could not explain.
7. Linking without incremental mode, which the binary-progress build already
   does, scores better than the gate's incremental link: 600 exact
   functions against 589, with 55 functions up and 5 down.
8. Reordering today's source files gains little: pair inversions fall from
   873,206 to about 650,000. Moving each function into its unit's file
   brings them to about zero.

## Link experiment

Three C files were compiled with the matching flags (`cl /TC /ML /O2 /Ob1
/DNDEBUG`). `link /INCREMENTAL:NO /MAP` linked them in several orders, from a
library, and with an order file. The build stayed in a scratch directory
inside the worktree and was deleted afterwards.

```c
/* a.c */
int a_common[4];
int mm_common;
static int a_bss[3];
int a_init = 5;
static int a_s(int x) { a_bss[x & 1] += x; return a_bss[0] * 3 + x; }
const char *a1(int x) { a_common[x & 3] = a_s(x); return "shared"; }
float a2(float x) { return x * 1.5f + (float)a_init; }

/* b.c */
int b_common;
int zz_common[100];
int mm_common;
static double b_bss;
const char *b1(int x) { b_common = x; zz_common[x] = 1; b_bss += x; return x ? "shared" : "b only"; }
float b2(float x) { return x * 1.5f; }

/* c.c: c_common[2], aa_common[50]; c1(); main() calls b1, a1, a2, b2, c1 */
```

| Link | Inputs | `.text` order (from the map) | Owner of `"shared"` | Common order |
| --- | --- | --- | --- | --- |
| L1 | `c.obj a.obj b.obj` | c1, main, a1, a_s, a2, b1, b2, then LIBC | `a.obj` | b_common, zz, a_common, c_common, aa |
| L1b | `b.obj a.obj c.obj` | b1, b2, a1, a_s, a2, c1, main, then LIBC | `b.obj` | c_common, aa, a_common, b_common, zz |
| L2 | `c.obj ab.lib` (a, b archived) | c1, main, **b1, b2, a1, a_s, a2** | `ab:b.obj` | c_common, aa, a_common, b_common, zz |
| L3 | `a.obj b.obj c.obj /ORDER:@{c1,b2,a_s,a1}` | **c1, b2, a1**, a_s, a2, b1, main | n/a | n/a |

What the runs show:

- **Explicit objects keep command-line order**, in `.text` and in `.data`.
  Each object's data block (its plain `.data` first, then its literals)
  follows the objects before it. L1 and L1b swap the blocks exactly as they
  swap the objects.
- **Library members are placed in the order they are pulled.** In L2 `c.obj`
  references `b1` before `a1`, so `b.obj` comes first, although `a.obj` comes
  first in the archive. Today's matching build behaves the same way (below).
- **Literal COMDATs go to the first object in link order.** `"shared"` lives
  in `a.obj`'s block in L1 and in `b.obj`'s block in L1b. This is the
  mechanism behind finding 505's rule 2, "a literal lives in the `.data`
  block of the first object in link order that uses it". With the original
  link order and the same literals per object, literal placement follows.
- **`/O2` makes every function a COMDAT.** In `a.obj`, each function is its
  own `.text` section with the `LNK_COMDAT` flag and 16-byte alignment, and
  each literal is a `.data` COMDAT. This matches finding 505's rule 5
  (16-byte-aligned functions with `0x90` fill, inside and between objects).
- **`/ORDER` places the external functions it names first, in the order
  given.** Everything else follows in object order. A `static` function
  cannot be named: `a_s` gave `LNK4037 "_a_s" does not exist; ignored`.
  LINK 5.0 adds the leading underscore itself, so the order file lists
  `c1`, not `_c1`. `/ORDER` has no effect on data.
- **Statics are emitted after their first caller.** `a.c` defines `a_s`
  before `a1`. The object's COMDAT order, and the link, are `a1`, `a_s`,
  `a2`. So "functions in original address order" (ADR 0005 recipe step 4)
  does not always mean "in that order in the source": a static helper that
  sits right after its first caller in the binary may be defined before it.
- **Common symbols** (`int x;` at file scope in C: storage class 2, section
  0, value = size) were allocated after all `.bss`. Within one object they
  kept their declaration order in every run. Across objects, the order was
  neither object order nor its reverse in all three links (L2 differs from
  L1's pattern), and it does not sort by size or name. `mm_common`, declared
  in both `a.c` and `b.c` and never referenced, does not appear in the map.
  The allocation rule is not settled; the plan has a ticket for it.

## Today's matching build

The measurements below use the matching build of #425's branch, which is
source-identical to `main` at `041ab287`. They read its `build-reccmp` and
`build-binary-progress` outputs; nothing was rebuilt.

### Link order is library pull order

The executable target links `main.cpp`, `entry.cpp`, `seh_runtime.cpp`,
`dependency_grid.cpp` and `screen_pick.cpp` directly, then
`game.lib gods98.lib game.lib platform_glue.lib`, then the import libraries
(`winmm.lib d3drm.lib … kernel32.lib user32.lib …`). LIBC.LIB comes in as a
default library.

Sorting the Reccmp report's functions by rebuilt address and grouping them by
the file that holds their marker gives the layout:

- **Every source file is one contiguous run**, as finding 505's rule 1
  predicts. 58 files give 68 runs. The extra runs are LIBC members
  interleaved with `crt.cpp`, whose `LIBRARY` markers name LIBC functions,
  and one `level.cpp` function placed elsewhere.
- **The order is executable objects first, then `game.lib` members, then
  `gods98.lib` members**, each in pull order. The executable objects run
  from `entry.cpp` at `0x401070` to `screen_pick.cpp`. The `game.lib`
  members start with `startup_config.cpp`, `terrain_grid.cpp`,
  `entity.cpp` and `util.cpp`, and end with `priorities.c` at `0x485500`.
  The `gods98.lib` members start with `frameloop.cpp` at `0x486080` and
  `resource.cpp`, and `crt.cpp` and LIBC are interleaved from `0x4972d0`.
- Function order inverts 908 times between neighbours (873,206 reversed
  pairs) in the full scope.

### Incremental and non-incremental links score differently

The gate (`make reccmp-check`) links with `/INCREMENTAL`. `make
binary-progress` builds the same sources with `/INCREMENTAL:NO`. Both
reports were compared, all 2,347 functions:

| | incremental (gate) | `/INCREMENTAL:NO` |
| --- | ---: | ---: |
| Mean score | 53.968% | 54.047% |
| Exact functions | 589 | 600 |
| Functions that differ | | 60: 55 up, 5 down |

The five drops:

| Address | Function | Incremental | `/INCREMENTAL:NO` |
| --- | --- | ---: | ---: |
| `0x404280` | `cmd_entity_command_chain_tick` | 11.99 | 10.71 |
| `0x4068c0` | `path_arc_length_interpolate` | 34.25 | 32.88 |
| `0x43f7d0` | `ledger_stub_0043f7d0_EntityEnergyLowTargetRouteCommandTick` | 27.40 | 26.69 |
| `0x46b8b0` | `rectangular_grid_overlay_geometry_draw` | 11.55 | 7.94 |
| `0x497ec0` | `__fltout` | 88.10 | 85.71 |

The code generation is the same, so these differences come from the link:
calls through incremental-link thunks, and names that Reccmp gives to
addresses and offsets. This is the effect finding 506's step (a) saw, when
listing a unit in the executable's sources dropped three call sites. Without
incremental linking there are no thunks, so explicit objects are safe.

### Reordering files is not enough

This simulation reuses the non-incremental report: it keeps each function's
rebuilt order within its file and changes only the order of objects.

| Layout | Adjacent inversions | Reversed pairs |
| --- | ---: | ---: |
| Today (pull order) | 908 | 873,206 |
| Today's 58 files, sorted by the median original address of their functions | 894 | 650,664 |
| Every function in its unit's object, units in original order | 17 | 25,903 |

In the last row, all 17 remaining adjacent inversions are caused by 23
report entries whose addresses are not function starts in the unit map. The
simulation could not place them, so it put them last. In a real build each
of them sits inside a unit. **The inversion count falls only as functions
move into unit-shaped files.** The link-order mechanism is a prerequisite,
not the gain.

## The source tree today

From `translation_units.py layout` on `0cfa8749`:

- 225 units: 50 game, 29 gods98 (after the G98-021 merge of finding 512),
  138 CRT, 6 import and 2 late. 70 units are spread over more than one
  source file, and 42 of the 61 marked source files mix units.
- 157 original game and gods98 functions have no reconstruction: 112 game
  and 45 gods98.
- Seven units are already in original shape and in their own files: G98-004
  `utils.c`, G98-006 `config.c`, G98-007 `maths.c`, G98-018 `keys.c`,
  G98-021 `soundsys.c`, G98-027 `dxerror.c` and GAME-034 `priorities.c`.
- File-static state in the largest mixed files. A function that moves to its
  unit file takes these with it, or they must become shared:

  | File | Lines | `static` lines | Function markers | Unit count |
  | --- | ---: | ---: | ---: | ---: |
  | `game/entity.cpp` | 15,737 | 309 | 297 | 21 |
  | `game/level.cpp` | 14,247 | 257 | 196 | 20 |
  | `game/frontend.cpp` | 12,848 | 414 | 160 | 20 |
  | `crt/crt.cpp` | 9,940 | 159 | 342 | 159 |
  | `gods98/view.cpp` | 9,483 | 197 | 202 | 28 |
  | `game/cmd.cpp` | 7,389 | 195 | 112 | 10 |
  | `gods98/record.cpp` | 3,582 | 53 | 115 | 31 |

### The unit call graph

Direct calls between game and gods98 units, from
`docs/coverage/call-graph/edges.csv` (79 units, 558 unit-to-unit edges):

- 11 units call no other unseen unit. The first wave has 9: GAME-003,
  G98-004, G98-007, G98-018, G98-019, G98-024, G98-026, G98-027 and G98-029.
  A second wave has 2: GAME-016 and G98-013.
- The other **68 units form one strongly connected component**: all 48
  remaining game units and 20 gods98 units.
- Only **6 unit pairs** are gods98 calling game: G98-001 calls GAME-002 and
  GAME-036; G98-005 calls GAME-021; G98-012 calls GAME-002 and GAME-036;
  G98-016 calls GAME-028. (The issue's figure of 8 predates the G98-021
  merge.) Without them the component splits into:
  - two gods98 cycles, `{G98-014, 016, 030}` (Files, Flic, Wad) and
    `{G98-002, 005, 009, 010, 015, 020, 021, 025}`;
  - six gods98 levels;
  - a **43-unit game cycle**. Five game units, GAME-011, 024, 026, 033 and
    038, sit below it.

This order matters for conversions, which change callee contracts (ADR
0005). It does not matter for moving code verbatim: a move changes no
contract, so it can go in any order.

### Layout devices already in `src/`

These were written to make Reccmp name addresses. Each one shifts data, so
each must go before data can land at original addresses:

- **Zero-filled padding in named `.data` sections**, which put tables at
  fixed offsets:
  - `game/frontend.cpp`: `s_menu_action_table_gap[0x3aad0]` and
    `s_reference_menu_data_pad0[0x79144]` in `.data$zzzzzz`;
  - `gods98/audio.cpp`: the 0x29c21-byte `MenuCompatData` in
    `.data$yyyyyy`;
  - `game/cmd.cpp`: the command pool in `.data$cmdpool`.

  These blocks are most of why the rebuilt `.data` is 988,160 bytes against
  the original's 73,216.
- **115 `#pragma comment(linker, "/include:…")` directives** in
  `crt/crt.cpp`. They force LIBC members into the link and fix part of their
  pull order.
- **Storage changed only for scoring.** `gods98/keys.c` declares
  `key_names[KEY_NAMES_COUNT + 1]` (257 slots against the original's 256), so
  that Reccmp names `Key_Find`'s past-the-end operand (finding 513). The
  table is a common symbol, so the spare slot moves every common allocated
  after it.

## S1a: the gate links without incremental mode (#436)

#436 switched the gate build to `/INCREMENTAL:NO` with `/MAP` and made
`make binary-progress` reuse it. Measured on `main` at `579737e7`, against an
incremental build of the same tree that reproduces the committed baseline:

| | incremental | `/INCREMENTAL:NO` |
| --- | ---: | ---: |
| Exact functions, full | 594 | 605 |
| Mean score, full | 54.165% | 54.245% |
| Exact functions, game-only | 495 | 501 |
| Mean score, game-only | 49.859% | 49.932% |
| Functions that differ, full | | 59: 55 up, 4 down |
| Functions that differ, game-only | | 47: 44 up, 3 down |

Finding 516 above counted 5 drops on #425's tree. `path_arc_length_interpolate`
no longer drops: the Bezier unit was converted since (#431), and its score
(75.76%) is the same under both links.

What the measurements show:

- **`/MAP` alone disables incremental linking.** LINK 5.0 prints `LNK4075:
  ignoring /INCREMENTAL due to /MAP specification`. The build passes
  `/INCREMENTAL:NO` explicitly anyway.
- **The causes are in the rendering, not the code.** Reccmp's listing of
  the original side is identical in both runs. On the rebuilt side every
  instruction is the same; only operand names differ.
- **The main cause is base relocations.** The incremental link writes a
  `.reloc` section. The non-incremental one leaves relocations stripped, as
  the original does (`IMAGE_FILE_RELOCS_STRIPPED`). Reccmp recognises float
  constants (`find_float_consts`), strings and import thunks only at
  relocated operands. Without relocations a rebuilt `fld [x]` is no longer
  named `1.0 (FLOAT)` but renders as `<OFFSETn>`, as the original's does.
  That accounts for 52 of the 59 changes. It also lowers 3 scores: in
  structurally different functions the `<OFFSETn>` numbers on the two sides
  are assigned in different orders. Likewise, an immediate holding the
  address of an unnamed rebuilt variable or string now renders as raw hex
  instead of a placeholder, and the later placeholders are renumbered. That
  is the only cause in `__fltout` (a drop), `__fprem_common`, `__setargv` and
  `__input`, and contributes in three more.
- **Incremental-link thunks are gone.** Calls into the executable's own
  objects (`dependency_grid.cpp`, `entry.cpp`) rendered as `Thunk of 'X'`;
  they now name `X`, as the original does. Three functions.
- **`.data` moves.** `.data` starts at `0x555000` instead of `0x5a5000`, so
  a flag that falls inside a large rebuilt array names a different element.
  It changes no score by itself.
- **Stripping the debug directory changes no score.** The same
  non-incremental executable scores identically before and after
  `strip_pe_debug.py`.

**`/OPT:REF` is not adopted.** It removes unreferenced functions: 990 of the
2,347 report entries disappear, because the port does not call them. Of
those left, 5 change score (2 down, 3 up). The link keeps LINK's `/DEBUG`
default, `/OPT:NOREF`.

**`match_function.py`.** Its synthetic image emulated incremental-link
thunks for the `program` target's objects and recorded every `DIR32` fixup
as a base relocation. Both are removed, the import thunks are named by their
public symbol as the linker PDB names them, and the assert fix-up override
is gone because the full run's image now has no debug directory. Against the
non-incremental report, 2,333 of 2,347 functions score identically. The 14
that differ are `__isindst` and 13 LIBC functions whose LIBC data operands
the tool names by public symbol; the same 14 differed with the incremental
build and the old emulation.

**Drift classifier (#454).** Checked against `main`'s listing digests,
`make reccmp-check` labels the three game drops `drift-review` and `__fltout`
`source-unknown`, each with an instruction delta of +0. The labels come from
what the classifier can see, not from a code change:

- `match_listings.normalize` masks registers, stack offsets, branch targets
  and raw addresses, but not Reccmp's symbolic operand names. A rebuilt
  `fsub [1.0 (FLOAT)]` that becomes `fsub [<OFFSET1>]` therefore changes
  the instruction multiset. With those names masked as well, the rebuilt
  listings of `rectangular_grid_overlay_geometry_draw` and `__fltout` are
  identical in sequence. The other two differ only in the element of
  `s_help_info_string_storage` that a flag constant lands in (the moved
  `.data` base).
- `__fltout` carries a `LIBRARY` marker, which the source index does not
  read, so its source is unknown rather than unchanged.

Across the whole report, 359 full-scope listing digests change and none
changes its instruction count. Every one of them differs only in operand
names: float, string and address operands without relocations, removed ILT
thunks, and import thunks named by their PDB public instead of
`DLL::name (IMPORT_THUNK)`.

Every changed function and its link cause:

| Address | Function | Before | After | Scope | Link cause |
| --- | --- | ---: | ---: | --- | --- |
| `0x00404280` | `cmd_entity_command_chain_tick` | 11.99 | 10.71 | game, full | float constant, .data base shift |
| `0x0043f7d0` | `ledger_stub_0043f7d0_EntityEnergyLowTargetRouteCommandTick` | 27.40 | 26.69 | game, full | float constant, unrelocated address immediate, .data base shift |
| `0x0046b8b0` | `rectangular_grid_overlay_geometry_draw` | 11.55 | 7.94 | game, full | float constant |
| `0x00497ec0` | `__fltout` | 88.10 | 85.71 | full | unrelocated address immediate, placeholder renumbering |
| `0x004089d0` | `segment_2d_point_distance` | 36.44 | 40.89 | game, full | float constant |
| `0x0040aa40` | `effect_helper` | 13.24 | 14.71 | game, full | float constant |
| `0x0040f0e0` | `kind_six_random_sweep_dispatch` | 33.33 | 34.52 | game, full | float constant |
| `0x00414e10` | `option_music_default` | 43.75 | 50.00 | game, full | float constant |
| `0x00418830` | `lego::building_upgrade_state_changed_dispatch` | 28.04 | 29.91 | game, full | ILT thunk |
| `0x00418920` | `dependency_grid_category_candidates_build` | 15.38 | 17.09 | game, full | ILT thunk |
| `0x00418da0` | `help_window_button_hit_test` | 34.81 | 35.49 | game, full | float constant |
| `0x00419030` | `help_window_button_feedback_update` | 67.31 | 71.15 | game, full | float constant |
| `0x00419b50` | `remaining_timer_duration_get` | 25.00 | 30.00 | game, full | float constant |
| `0x0041ba10` | `simtick_mute_timer` | 60.87 | 63.77 | game, full | float constant |
| `0x0041ee80` | `ledger_stub_0041ee80_CellGridPointHitTest` | 22.61 | 23.04 | game, full | float constant |
| `0x004230c0` | `debug_panel_text_draw_update` | 15.67 | 16.59 | game, full | float constant |
| `0x0043c2b0` | `periodic_timer25_advance` | 12.61 | 14.41 | game, full | float constant |
| `0x0043c6b0` | `entity_activity_counters_advance` | 43.88 | 45.57 | game, full | float constant |
| `0x00440800` | `spatial_entity_damage_radius_callback` | 61.19 | 62.69 | game, full | float constant |
| `0x00440a10` | `ledger_stub_00440a10_AttachedEntityDirectionVectorUpdate` | 22.16 | 22.71 | game, full | float constant |
| `0x00447730` | `entity_progress_advance_by_25` | 36.36 | 42.42 | game, full | float constant |
| `0x0044c030` | `entity_one_shot_attached_effect_start` | 28.27 | 29.32 | game, full | float constant |
| `0x0044c1b0` | `ledger_stub_0044c1b0_EntityHeadingStepMove` | 27.60 | 28.03 | game, full | float constant |
| `0x0044d0d0` | `motion_drift_offset_step` | 12.81 | 13.79 | game, full | float constant |
| `0x0044f700` | `world_to_terrain_cell_bounds_check` | 57.29 | 60.42 | game, full | float constant |
| `0x0044f870` | `terrain_world_point_to_cell` | 39.69 | 42.75 | game, full | float constant |
| `0x004512d0` | `terrain_cell_normal_sample` | 10.31 | 11.34 | game, full | float constant |
| `0x00453bb0` | `tutorial_frame_timer_array_advance` | 48.53 | 50.00 | game, full | float constant |
| `0x00453da0` | `tutorial_camera_focus_tween_update` | 11.33 | 11.90 | game, full | float constant |
| `0x0045bf20` | `camera_zoom_step_adjust` | 91.30 | 100.00 | game, full | float constant |
| `0x0045d550` | `priorities_move_cursor_to_up_button` | 73.10 | 75.86 | game, full | float constant |
| `0x0045e940` | `view_point_to_screen_geometry_transform` | 18.90 | 22.05 | game, full | float constant |
| `0x00460360` | `mission_checkpoint_task_progress_refresh_bound` | 3.68 | 10.31 | game, full | float constant |
| `0x00462700` | `pointer_override_expiry_tick` | 93.94 | 100.00 | game, full | float constant |
| `0x00463e40` | `float_round_to_nearest_int` | 96.88 | 100.00 | game, full | float constant |
| `0x00465f40` | `transient_effect_instance_quad_vertices_build` | 20.69 | 21.46 | game, full | float constant, unrelocated address immediate |
| `0x00470630` | `entity_directed_line_side_check` | 35.00 | 37.50 | game, full | float constant |
| `0x00471370` | `cmd_place_ray_candidate_check` | 15.30 | 15.85 | game, full | float constant |
| `0x00473e10` | `renderer_fog_color_set` | 23.88 | 29.85 | game, full | float constant |
| `0x004750b0` | `render_object_set_packed_rgba_from_float_channels` | 74.19 | 87.10 | game, full | float constant |
| `0x004757f0` | `typed_child_resource_scalar_get` | 95.45 | 100.00 | game, full | float constant |
| `0x004761a0` | `ddiscene_animation_duration_get` | 90.00 | 100.00 | game, full | float constant |
| `0x004761c0` | `scene_property_float_at14_get` | 90.00 | 100.00 | game, full | float constant |
| `0x00477150` | `ddiscene_draw_end` | 23.75 | 30.00 | game, full | float constant, string literal |
| `0x004789c0` | `audio_mixer_control_init` | 29.21 | 38.20 | game, full | float constant |
| `0x0047c100` | `ledger_stub_0047c100_DirectSoundListenerRolloffSet` | 59.09 | 68.18 | game, full | float constant |
| `0x00483420` | `screen_effect_item_opacity_set_target` | 28.25 | 30.51 | game, full | float constant |
| `0x00487fc0` | `lego::keyframe_time_segment_fraction` | 39.42 | 42.34 | game, full | float constant |
| `0x0048e280` | `_floor` | 99.06 | 100.00 | full | float constant |
| `0x0048e680` | `_fabs` | 98.98 | 100.00 | full | float constant |
| `0x0048e8e0` | `_ceil` | 99.06 | 100.00 | full | float constant |
| `0x0048f000` | `_WinMainCRTStartup` | 91.49 | 92.20 | full | unrelocated address immediate, ILT thunk |
| `0x0049200c` | `__convertTOStoQNaN` | 85.71 | 100.00 | full | float constant |
| `0x004921e4` | `__ctrandisp2` | 93.86 | 99.12 | full | float constant |
| `0x00492bc6` | `__fprem_common` | 97.18 | 99.30 | full | unrelocated address immediate, placeholder renumbering |
| `0x004935c0` | `__handle_exc` | 94.44 | 96.83 | full | float constant |
| `0x00493ae0` | `__decomp` | 97.65 | 100.00 | full | float constant |
| `0x00493ef0` | `__input` | 99.40 | 99.57 | full | placeholder renumbering, string literal |
| `0x00495ee0` | `__setargv` | 85.19 | 88.89 | full | unrelocated address immediate, placeholder renumbering |

## Reproducing

The experiment needs only the matching toolchain:

    T=$REPROBIT_MSVC_5_0_RTM_ROOT/wine/x86
    for f in a b c; do $T/cl /nologo /TC /ML /O2 /Ob1 /DNDEBUG /c $f.c; done
    $T/link /nologo /subsystem:console /INCREMENTAL:NO /MAP:l1.map /out:l1.exe c.obj a.obj b.obj
    $T/lib /nologo /out:ab.lib a.obj b.obj
    $T/link /nologo /subsystem:console /INCREMENTAL:NO /MAP:l2.map /out:l2.exe c.obj ab.lib
    printf 'c1\nb2\na_s\na1\n' > order.txt
    $T/link /nologo /subsystem:console /INCREMENTAL:NO /MAP:l3.map /ORDER:@order.txt /out:l3.exe a.obj b.obj c.obj

The build measurements need a `make reccmp-report` and a `make
binary-progress` of the same tree. The file runs come from joining each
report entry's `recomp` address with the file that holds its marker. The
inversions come from `binary_progress.inversions`.
