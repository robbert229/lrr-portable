# Finding 653 — legoGlobs is one struct

Issue #759, part of #388 and ADR 0005. Data layout (cause B) was a recorded
mismatch cause in every recent Lego.c and LegoObject.c sweep (findings 626,
644, 648, 656). The cause was that Lego.c's state was 180 separate globals
in 12 files. In the original it is one `Lego_Globs` struct.
`frontGlobs` became one struct in #756 (finding 649). This change does the
same for `legoGlobs`.

Measured on `main` `85050ccd` (#766, on #765 and #764), with one full
matching build of `main` and one full `make reccmp-check` of this change.
An earlier run on `c42dc2cf` gave the same 10 rises.

| Measure | `main` | This change |
| --- | ---: | ---: |
| New exact functions | | **5** (Lego.c 1, LegoObject.c 2, HelpWindow.c 1, Encyclopedia.c 1) |
| Functions that rose / fell | | 10 / **0** |
| Exact functions | 2,401 | **2,406** |
| Exact-function code bytes | 330,196 (51.39%) | **332,454 (51.74%)** |
| Reccmp accuracy, full / game-only | 97.01 / 96.67 | **97.02 / 96.68** |
| `unit-data-offsets`: unit statuses | | unchanged |
| `unit-data-offsets`: unattributed annotations | 126 | **79** |
| `opt_ref` annotated removed | 0 | 0 |
| Image end (`make pe-audit`) | `0x7f5000` | `0x7f5000` |
| Naked asm in Lego.c | 12 | 12 |

No exact match was lost, and no function fell below the baseline, so no
drops need a label. `make match-progress`, `make unit-data-offsets`,
`make opt-ref-callers OPT_REF_ARGS=--summary` and `make pe-audit` were run
on the final tree, and the generated files are committed. `make
tools-test` (no `--allow-growth`), `make winapi-link-check` and native ctest
(37 of 37) pass. `tools/binary_progress_inputs.txt` lists the new header.

## 1. Inventory

The units converted so far keep their state in one `*Globs` struct at its
original address, except two:

| Struct | Unit | Address, size | Before | This change |
| --- | --- | --- | --- | --- |
| `legoGlobs` | GAME-021 Lego.c | `005560c0`, `0xef8` | 180 annotated globals: 131 stand-ins in `lego_unit.c`, 2 in `legoobject_unit.c`, and 47 port globals in 10 C++ files (`engine.cpp` 15, `entity.cpp` 15, `level.cpp` 8, `frontend.cpp` 3, and one each in `ai.cpp`, `cmd.cpp`, `terrain_grid.cpp`, `camera.cpp`, `view.cpp` and `port_state.cpp`) | **one struct** |
| `objectGlobs` | GAME-023 LegoObject.c | `004de790`–`004eadd0` | 9 `objectGlobs_*` stand-ins in `legoobject_unit.c`, plus port globals (`s_pool_chunks`, `s_terrain_sfx_grid`, `s_grid_count_second`, …) | not done (follow-up below) |

Every other converted unit already defines a single struct: FrontEnd.c
(#756), ObjectRecall.c and PTL.c (#764), and the rest from their
conversions. `legoGlobs_f_556fb8` (`00556fb8`, the pointer's block) and
`legoGlobs_f_556090` (`00556090`) keep their own definitions. Both lie
outside legoGlobs. Lego_Gods_Go zeroes exactly `0xef8` bytes, which ends
the struct at `00556fb8`, and `frontGlobs` begins at `00556fc0`.

## 2. The struct

`src/game/lego_globs.h` defines `struct Lego_Globs`:

- Every field sits at its original offset. The four unknown gaps are named
  fields: `field_8`, `field_c`, `field_58[3]` and `field_3a0`.
- `lego_unit.c` defines it under one annotation,
  `// GLOBAL: LEGORR 0x005560c0`.
- It is a common in the matching build, as in the original. The GAME-021
  row of `unit-data-offsets` now lists `005560c0:_legoGlobs(size=3832)`.

The units' code is unchanged. Each old name is a `#define` to its field:

- **Fields no port module owns** (the stand-ins of `lego_unit.c` and
  `legoobject_unit.c`). They are the struct's in every build, natively too.
- **Fields a C++ port module keeps** (`g_active_level_retail`,
  `s_engine_config_root`, `s_catalog_counts`, the block lists, ...). They
  are the struct's in the matching build only:
  - the port modules' definitions are compiled out there;
  - the 17 C++ files that use them include `lego_globs.h` after their
    frozen headers.

  Natively the port keeps its own globals with their initial values, and
  the units read them by the port's names, as before. Native behaviour does
  not change.

Pointer members are `void*` (C) and the macros cast them to their types.
This way the struct names no struct tag that a unit has not declared
(section 3). `currLevel` is a plain member access, because the inline asm
in `lego_unit.c` and `lego_port.cpp` reads it. Its type is `void*` in C and
the port's `struct LevelState*` in C++.

Three units read fields under types of their own. They redefine those
names after the include (matching build):

- `interface_unit.c`: `LegoCamera*`, `LegoObject*`, `LegoObjectRequests`
  and `unsigned int` costs;
- `entity_cell_effects.c`: the building data as `char*`, the three tracked
  objects as `Lego_Object*`.

The only `.data` the struct retires is two port initialisers, which are now
`.bss` in the matching build: `ai.cpp`'s tool-name table and
`engine.cpp`'s `s_scroll_scale = 1.0f`. The original zero-fills both and
sets them at run time. The matching build is scored, not played (finding
628).

## 3. Symbol counter: a new cost, and how each unit stays neutral

**An `#include` of a file not yet included costs one symbol.** An empty
header moves a following static's `$S` number by 1, and a header holding
`int e1;` moves it by 2. A guarded re-include costs 0. Finding 538's table
had no row for the directive itself. It is consistent with that table's
"0–1" for SDK headers. Measured with the matching flags:

| Construct | Cost |
| --- | ---: |
| `#include "empty.h"` (new file) | 1 |
| re-include of a guarded header | 0 |
| `struct Lego_Globs { … };` (184 members of basic types) | 1 |
| the same with members naming two undeclared struct tags | 3 |
| `extern struct Lego_Globs legoGlobs;` | 1 |
| `lego_globs.h` as committed | **3** |

Unbalanced, the change re-rolled finding 538's phases. Removing the 124
stand-in declarations from `lego_unit.h` moved Lego.c's counter by −124,
and every other including unit moved by +3 minus the declarations it lost.
In the single-function loop, 34 functions moved: 14 rose and 20 fell, 10
of them exact (for example `00401500` 100 → 69, `0040e9c0` 100 → 84 and
`00433030` 100 → 77). Each unit is now counter-neutral, or its changed
counter starts only at a function that reads legoGlobs:

- **Lego.c's header keeps its declarations.** `lego_unit.h` keeps its
  declarations of the old names, unreferenced. Three of them (`flags1`,
  `cameraFP`, `005560f8`) are dropped, which pays for `lego_globs.h`'s
  three symbols. The units that include `lego_unit.h` (Lego.c, AITask.c,
  Construction.c, Erode.c, FallIn.c, Message.c, Objective.c, RadarMap.c,
  SelectPlace.c, SpiderWeb.c, Water.c, Weapons.c) include `lego_globs.h`
  right after their imports, so their counter is unchanged throughout.
  Water.c reads no field, so it includes the header only to stay neutral.
- **LegoObject.c** keeps its imports header's declarations, and its six
  mid-file declarations as `#undef`/declaration/`#define` triples. One
  declaration, `s_tool_type_names`, is dropped in the matching build to
  pay for the include's third symbol. `counter_probe.py` reports 0 at all
  286 functions.
- **Lego.c group 3B** keeps one declaration in the same way,
  `legoGlobs_quality`. With it, `00433380` stays exact. Without it, the
  group moved by −1, and the function fell to 90.91 (a phase: a 0–15 pad
  sweep makes it exact again at some offsets only). Group 3C (`00434570`
  on) is still −5; none of its functions moves.
- **The other units** include `lego_globs.h` immediately before their first
  function that reads a field. The functions above it keep their counter.

Each kept declaration carries `Deviation (scoring only)`. None changes
storage. The single-function loop over all 76 C units shows 9 rises and no
fall, and the full report shows 10 rises and no fall.

## 4. Per-function results (full report)

| Address | Function | `main` | After | Why |
| --- | --- | ---: | ---: | --- |
| `0040e490` | encyclopedia_config_files_open | 91.79 | **100** | the type counts are `legoGlobs+0x1bc` itself, not an offset from `legoGlobs_config` (finding 633) |
| `00418450` | help_window_load_infos | 96.08 | **100** | the same |
| `004350a0` | lego_surface_type_descriptions_config_load | 98.55 | **100** | B closed: the end-of-array pointer is the next field |
| `00439950` | legoobject_tool_name_strings_load | 92.00 | **100** | `toolName[11]` and `langToolName[11]` are adjacent fields |
| `0043f190` | legoobject_spawn_request_queue_process | 99.67 | **100** | B closed: the count reload falls where a store may alias the struct |
| `0042f0c0` | lego_level_free | 86.01 | 90.03 | B partly closed: the flag words' `&=` are no longer folded across fields; what is left is scheduling (D) |
| `00437150` | legoobject_tool_tip_icons_load | 98.33 | 99.17 | the end pointer of the names |
| `004250c0` | lego_main_sim_step | 91.25 | 91.51 | field operands |
| `0042e020` | lego_object_list_load_file | 73.91 | 74.24 | field operands |
| `00429890` | lego_level_load | 95.04 | 95.08 | the `dragBoxRGB` end pointer (finding 648) |

`0042dfc0` lego_object_pointer_registry_clear_match stays exact without the
`volatile` read of finding 644. The count and the array are members of one
struct, so the reload after the store happens without help. The deviation
`legoGlobs_recordObjectCountReloaded` is deleted.

### Cause B re-check (findings 644, 648 and 656)

| Address | Function | `main` | After | Now |
| --- | --- | ---: | ---: | --- |
| `004350a0` | lego_surface_type_descriptions_config_load | 98.55 | **100** | closed |
| `0043f190` | legoobject_spawn_request_queue_process | 99.67 | **100** | closed |
| `00439950` | legoobject_tool_name_strings_load | 92.00 | **100** | closed |
| `0042f0c0` | lego_level_free | 86.01 | 90.03 | D: `flags1`/`flags2` stores scheduled before the `s_entity_state_flags` read; the original's callees there are still port adapters (A) |
| `00437150` | legoobject_tool_tip_icons_load | 98.33 | 99.17 | D |
| `00429890` | lego_level_load | 95.04 | 95.08 | D: stack slots (finding 652) |
| `00428020` | lego_entity_command_status_feedback_dispatch | 98.89 | 98.89 | C: `ecx`/`eax`; no legoGlobs operand differs |
| `0042cda0` `0042cf50` `0042d100` `0042d2b0` `0042d460` | the five type-table loaders | 96.91–96.93, 96.75 | same | C: `ecx`/`eax` at the shared `free`; with one struct the original's register still differs, so it was not the cause |
| `00430530` | lego_terrain_cell_mutate | 95.45 | 95.45 | D: the block reload for the third work-flag test is not data layout |
| `00424730` | lego_sine_phase_midpoint_blend_dispatch | 95.45 | 95.45 | E: `fadd` order; both sides now read `legoGlobs+0x348` indexed, so the end pointer is gone |
| `00438310` | legoobject_pool_chunk_allocate | 97.22 | 97.22 | objectGlobs (not done) |
| `00437050`, `004385b0`, `00437940`, `0044bf00` | LegoObject.c | | same | objectGlobs (not done) |

So of finding 656's nine cause-B Lego.c functions, `004350a0` is closed,
and six are now C or D, because their operands match the original's and
only registers or scheduling differ. `00424730` is E, and `00430530` is D.

### `00432200` stays asm

Finding 538's note on `lego_terrain_cell_previous_state_track` expected one
struct to give the original's loop in C. A C form with a Point2I copy
(`*(LegoPoint2I2A*)prev[i] = *(LegoPoint2I2A*)update[i]`) gives every
instruction except the order of one `imul`'s operands: 93.75%, an
effective match. The order is the same at every phase tried (N = 0–56) and
in five shapes of the index expression. The element-wise copy scores
81.25%. The naked asm, with its operands now `legoGlobs + 78ch` and
`legoGlobs.updateBlockCount`, stays exact. The C form stays for other
compilers. `lego_port.cpp`'s two port asm copies take the same offsets.

## Files touched in other threads' files (all mechanical)

- `src/game/lego_unit.c` (#762): the stand-in definitions become the one
  struct; one kept declaration (group 3B); `00432200`'s asm operands.
- `src/game/frontend_unit.c`, `interface_unit.c`, `weapons.c` (#761): one
  include each; in FrontEnd.c two mid-file port declarations guarded and
  three mid-file declarations of old names removed; Interface.c's field
  types (section 2).
- `src/game/aitask.c` (#760): one include.
- `src/game/entity_cell_effects.c` (#764): four declarations become the
  include and its typed names.

No function body changes outside `lego_unit.c`'s `00432200`.

## Follow-up

- **objectGlobs** (`004de790`–`004eadd0`, `0xc640` bytes), LegoObject.c's
  state, as one struct. Nine `objectGlobs_*` stand-ins in
  `legoobject_unit.c` and the port's pool, grid and path globals in
  `ai.cpp`, `spawn.cpp` and others. Recorded B in findings 626 and 648:
  `00438310`, `00437050`, `004385b0`, `00437940` and `0044bf00`. The same
  method applies: one header, port names as matching-only macros, and a
  counter-neutral include.
- Native port gaps are unchanged. Natively the port-owned fields are still
  the port's own globals, so lego_unit.c reads the port's copies (as
  before).
