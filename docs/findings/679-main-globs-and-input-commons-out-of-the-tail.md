# 679. Main.c's mainGlobs and Input.c's commons out of the port tail

Issue #816 implements finding 668's follow-up 3. The units read Main.c's
state and Input.c's key maps and mouse words from port-tail objects. This
change defines that state in the original's shape, in the matching link's
own objects. Measured with full matching builds under the gate's link mode
(`/DEBUG /OPT:REF /OPT:NOICF`) on `main` `160f5efc`.

## Summary

1. **Main.c's `.bss` is one external struct, `mainGlobs` (`00505800`,
   0x890 bytes, 21 fields).** `main.c` defines it with `= { 0 }`, so it
   stays in the unit's `.bss` (finding 669). The eight separate statics and
   the two external commons main.c had before are its fields. A test
   object shows that MSVC 5.0 does not keep separate definitions in source
   order: it puts statics and externals in separate runs. Only one struct
   gives the original's offsets.
2. **In the matching build, the units read the 13 fields that the port
   shares as `mainGlobs` fields.** `src/gods98/main_globs.h` maps each port
   name (`s_display_w`, `s_main_device`, `s_engine_feature_word` and the
   rest) to its field, for `_MSC_VER < 1200` only. The native build and the
   MinGW reference build keep the port's variables (ADR 0006 decision 4).
   No unit references `port_state.cpp`, `lws_port.cpp`,
   `viewports_port.cpp`, `mesh_port.cpp`, `view.cpp` or
   `gods98/frameloop.cpp` any more.
3. **Input.c's key maps, pointer deltas and mouse words are commons that
   main.c defines after its last function.** These are `0076aa00`,
   `0076ab24` to `0076ab58` and `0076ab80`. They are tentative definitions,
   as `objective_unit.c`'s lClicked (`0076ab48`) already is. Natively,
   `input.cpp` and `port_state.cpp` keep their definitions, under
   `#if !defined(LEGORRR_BACKEND_WINAPI)`.
4. **The symbol counter decides where each unit includes the new header.**
   A naive retarget re-rolled about 20 functions across six units
   (finding 538). Each reader now keeps its counter at every function
   (section 3). No function falls. Two Main.c functions become exact.
5. **The tail stays linked.** It has 11 root objects before this change and
   6 after. Of the eight objects this issue names, `input.cpp` and
   `platform/winapi/frameloop.cpp` are still roots through Input.c's own
   port hooks (`input_unit.cpp`), and `engine.cpp` through three
   references that do not belong to Main.c. The tail's code and `.bss`
   cannot leave until the last root goes (finding 668, section 5).

## Before and after

| Measure | `main` `160f5efc` | This change |
| --- | ---: | ---: |
| **Exact-function code bytes** | **344,002 (53.54%)** | **344,876 (53.67%)** |
| Exact functions, full / game-only | 2,632 / 2,344 | 2,634 / 2,346 |
| Accuracy, full / game-only | 97.56% / 97.30% | 97.56% / 97.30% |
| `.text` identical at original addresses | 6.82% (43,833 B) | 6.81% (43,772 B) |
| `.data` identical | 24.31% | 24.31% (+1 B) |
| `.rdata` identical | 11.06% | 11.37% |
| Identical prefix, raw / masked | 11 B / 7,187 B (to `0x402c13`) | same |
| Correct-size functions, full | 2,796 | 2,796 |
| Units with size agreement (sizes, start, end) | 2 of 227 (168, 5, 4) | same |
| Entry point (original `0048f000`) | `0048eb80` | same |
| SizeOfImage (original 3,608,576) | 4,026,368 | same |
| Data-offset agreement | 20 of 57 units | same |
| Function-order inversions, full / game-only | 222 / 1 pairs | same |
| G98-005 unit-relative identity | 43.39% | 59.06% |
| Tail `.text` / `.bss` / `.data` / `.rdata` | 79,520 / 285,781 / 19,629 / 14,777 | 79,520 / 285,221 / 19,629 / 14,777 |
| Tail objects referenced from units | 11 | 6 |

Two functions rise and none falls:

| Function | Before | After |
| --- | ---: | ---: |
| `00477bf0` winmain_parse_options | 99.57 | **100** |
| `00478940` d3d_material_state_flush | 95.24 | **100** |

`.text` identical at original addresses falls by 61 bytes, and no
instruction changes. The cause was not traced byte by byte. The likely
cause is operand bytes: the 13 fields' rebuilt addresses moved from the
tail's `.bss` into Main.c's (`mainGlobs` is at `005131a8`; the original
has it at `00505800`), and a few address bytes matched the original by
chance before. No unit's size moves, so the identity metrics that
measure layout (prefix, correct sizes, unit agreement, entry point) do
not change.

G98-005's data-offset row stays "no agreement": its `.data` and `.rdata`
annotations still cannot all be attributed. The `.bss` row now has
one attributable item (the struct), so the tool no longer reports it as
"partial".

## 1. mainGlobs

| Offset | Field | Port name (shared fields) |
| --- | --- | --- |
| `0x000` | hWnd | `g_platform_cursor_window_handle` |
| `0x004` | hInst | `g_main_instance_handle` |
| `0x008` | active | `s_window_application_active` |
| `0x00c` | exit | `s_window_close_requested` |
| `0x010` | className | |
| `0x014` | programName[256] | |
| `0x114` | lpD3DRM | `s_main_d3drm` |
| `0x118` | device | `s_main_device` |
| `0x11c` | imDevice | `s_main_im_device` |
| `0x120` | fogMethod | `s_scene_fog_mode` |
| `0x124` | appWidth | `s_display_w` |
| `0x128` | appHeight | `s_display_h` |
| `0x12c` | currState (Main_State) | |
| `0x138` | stateSet | |
| `0x13c` | fixedFrameTiming | (Reward.c reads it) |
| `0x140` | renderStates[200] | |
| `0x780` | style | |
| `0x784` | flags | `s_engine_feature_word` |
| `0x788` | programmerLevel | `s_application_mode` |
| `0x78c` | startLevel[256] | (Advisor.c reads it) |
| `0x88c` | clFlags | `s_render_feature_flags` |

`main.c` declares the original's types: `Main_State`,
`Main_StateChangeData` and the `Main_Globs` typedef. It reads its own
fields as `mainGlobs.className` and so on. It reads the shared fields
by their port names, so the MinGW reference build, where those names are
the port's variables, keeps working.

The tail's 13 `GLOBAL` annotations at these addresses are removed. Reccmp
names the original's addresses by annotation, so while they stayed, the
original side read `s_engine_feature_word (DATA)` and the rebuilt side
read `mainGlobs+1924`. That cost every reader up to half its score. The
tail's variables stay, unannotated, for the native and reference builds.

## 2. The commons

Input.c is not converted (`input_unit.cpp` is still relocated C++), and
only a C file makes tentative definitions. So the shared Input.c state
gets common definitions at the end of `main.c`:

- `s_global_previous_keys` (`0076aa00`) and `s_global_current_keys`
  (`0076ab80`);
- `s_renderer_word_a` and `s_renderer_word_b` (msx and msy, `0076ab24`
  and `0076ab28`);
- `s_cursor_delta_x` and `s_cursor_delta_y` (`0076ab34` and `0076ab38`);
- `g_renderer_word_c` and the mouse words `0076ab40` to `0076ab58`.

After the last function, they move no counter in main.c. Their
annotations move from the tail into main.c.

These are separate commons, not the original's Input_Globs struct. The
order of the common region is still deferred (ADR 0006 decision 5,
finding 525). Defining Input_Globs belongs to Input.c's conversion.

## 3. The symbol counter

A unit that reads the shared fields has to see the struct. Every
declaration before a function moves that function's counter (finding 538).
Measured with the matching `cl`:

- A struct tag costs 1 symbol, a typedef 1, a variable 1, and members none.
  An anonymous struct still costs a tag.
- A header costs symbols the first time it is included and nothing after
  that.
- With the struct defined inside the `extern` declaration, the first
  include of `main_globs.h` costs 3 symbols. A probe that puts a static
  before every function (scratch only, not committed) confirmed this in
  `lego_unit.c`.

So the header gives readers a reduced view: the same layout, with
`currState` and `renderStates` as plain storage. The original's two
member typedefs and the `Main_Globs` typedef would cost each reader 5
more symbols. This is tagged `Deviation (scoring only)`, declaration-only.
`main.c` keeps the original's typed declaration (`LEGORRR_MAIN_GLOBS_OWNER`).

Each reader has one of three placements. Each was checked against the
`main` scores of every function in the unit (`match_function.py
--source`), and the full gate confirmed them:

| Placement | Units | Counter |
| --- | --- | --- |
| Three port declarations dropped in the matching build; the header included at the end of the import header | Lego.c (`lego_unit_imports.h`), FrontEnd.c (`frontend_unit_imports.h`), Mesh.c (`mesh_imports.h`) | unchanged at every function |
| `crt/crt.h` includes the header with `LEGORRR_MAIN_GLOBS_NO_NAMES` in place of its three declarations; the unit includes it again, for free, where the names are needed | Advisor.c, Interface.c, Reward.c, Lws.c; Draw.c (struct only) | unchanged, except after Advisor.c's and Reward.c's removed declarations (−1, no score moves) |
| Included before the first function that uses the names (+3 after it) | AnimClone.c, Bubbles.c, Container.c, DirectDraw.c, Init.c, LegoObject.c, Objective.c, Pointer.c, ScreenEffectQuads.c, SoundSys.c, Viewports.c, Input.c (`input_unit.cpp`) | +3 after the include; no score moves |

The first attempt included the full original header before each unit's
first use (+7). It re-rolled 20 functions in Advisor.c, FrontEnd.c,
Lego.c, Lws.c, Mesh.c and ScreenEffectQuads.c, for example
`0042c330` lego_emerge_map_trigger_spawn (100 to 55.79) and `004016f0`
vec_dot (100 to 48.28). Every one was a commutative operand or register
swap, the finding 538 phase effect. Reader views of 2 to 6 symbols moved
the drops around without removing them. Advisor.c's `vec_add` and
`vec_dot` are wrong at every shift from +4 to +10, so only a placement
that keeps the counter fixes them. Unused port declarations stay where
removing them would move a counter, and are tagged as declaration-only
deviations in `main_globs.h`.

## 4. What still references the tail

| Tail object | Referenced from | Owner |
| --- | --- | --- |
| `input.cpp` | Input.c (`input_unit.cpp`): the DirectInput hooks and state, `input_reset`, `input_global_current_keys_copy` | Input.c's conversion |
| `platform/winapi/frameloop.cpp` | Input.c: `platform_cursor_position_get`/`_set`, the cursor width and height | Input.c's conversion |
| `engine.cpp` | Panel.c `engine_shared_state_word`, Info.c `lego_goto_info`, G98-028 `s_window_mode_flag` (`005764f4`) | Panel.c, Info.c, G98-028 (#819, #817) |
| `startup_config.cpp` | G98-028's streams | #817 |
| `lego_port.cpp` | Lego.c's light setters | #815 |
| `frontend_port.cpp` | Reward.c's save buffer `00557418` | outside this issue |

Main.c's fields and Input.c's commons no longer hold any root. The two
Input.c roots go when Input.c is rebuilt in original shape: its six
functions call DirectInput directly and own the hooks' state. With #815's
and #817's roots also gone, Input.c, Panel.c, Info.c and the save
buffer are what keep `lrr_tail.lib` in the link.

## Checks

- `make reccmp-check` passes: no function falls, and the map check is OK.
- `make match-progress` rewrites the baseline with the two rises above.
- `make unit-data-offsets` and `make pe-audit` pass. The image end is
  `0x7d7000`, as before.
- `make tools-test` (no `--allow-growth`) and `make winapi-link-check`
  pass.
- The native build compiles, and its 39 ctest tests pass with the real
  WADs. `LegoRR` starts with the offscreen driver and runs for 20 s.

## Reproduce

```
make reccmp-check && make match-progress
python3 tools/port_tail_inventory.py --json /tmp/tail.json
make unit-data-offsets && make pe-audit
```
