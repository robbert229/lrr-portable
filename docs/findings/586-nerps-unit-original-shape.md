# Finding 586 — NERPs.c (GAME-028) rebuilt in original shape, with a port tail

Issue #606, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-028 is the original's `NERPs.c`: the tutorial's NERP script interpreter
and its function table (`.data` `004a5948`), the message file (lines, inline
`:image` and `$sound` tokens), the tutorial camera tracking, zoom and rotation,
message history and timers, the tutorial icon and the per-level block pointer
lists. Text `00452df0`-`004574e0`. It was relocated verbatim into
`src/game/tutorial_script.cpp` (#496). This change rebuilds it as the C unit
[`src/game/tutorial_script.c`](../../src/game/tutorial_script.c), with the
unit-private imports header
[`tutorial_script_imports.h`](../../src/game/tutorial_script_imports.h) and the
X-macro list
[`tutorial_script_port_functions.h`](../../src/game/tutorial_script_port_functions.h).
The port's implementation moves, unannotated, to the port tail
[`tutorial_script_port.cpp`](../../src/game/tutorial_script_port.cpp).

The top layer of stack #613: built on #611 (GAME-041 SFX.c, `197aabde`), which contains #610 (LegoObject.c part 2) and #609 (the camera) on `main` `31da2ab6` (#608). "Before" is #611's head: its match baseline and `binary-progress.json`. The per-function "before" column is the relocated file's scores, unchanged by the stack below.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, 31 functions, 5,555 original code bytes) | 18.75% | **81.92%** |
| Exact functions in the unit | 10 | **23 of 31** |
| Mapped functions in the unit | 29 | 31 |
| Exact functions, whole binary (full) | 1,340 | 1,356 |
| Exact-function code bytes (`binary-progress.json`) | 146,620 (22.82%) | **148,281** (23.08%) |
| Accuracy, full / game-only | 77.18% / 74.91% | 77.67% / 75.46% |
| `opt_ref` annotated (full): removed / exact removed | 589 / 276 | 585 / 275 |
| `opt_ref` annotated (game only): removed / exact removed | 572 / 265 | 568 / 264 |
| Reccmp lint ledger | 1,322 | 1,314 |

Byte weights are the original's code bytes without the trailing `0x90` fill
(from the binary, not the unit table, whose sizes run to the next annotated
function and so include the unannotated table functions); an unmapped
function counts as 0%. Two functions are newly mapped: `00454600` (the
tutorial icon over the tracked object, from Lego.c's engine tick) and
`00455300` (an object type's count by name, from Lego.c's pilot start and
Reward.c). Neither had an annotation. Every non-exact function's remaining
difference is register or x87 allocation, a call to an adapter, or a Reccmp
limitation (below); none differs in structure.

| Address | Function | Port name before | Bytes | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `00452df0` | `nerps_file_load_script_file` | `tutorial_script_file_load_target` | 121 | 34.62 | **100.00** |
| `00452e70` | `nerps_file_load_message_file` | `objective_advisor_token_file_load` | 909 | 12.66 | 78.61 |
| `00453200` | `nerps_file_get_message_line` | `objective_advisor_token_lookup` | 27 | 50.00 | **100.00** |
| `00453220` | `nerps_shutdown` | `tutorial_display_resources_release` | 187 | 11.11 | 90.60 |
| `004532e0` | `nerps_runtime_resolve_argument` | `tutorial_indirect_handle_resolve` | 62 | 11.11 | 95.00 |
| `00453320` | `nerps_runtime_execute` | `tutorial_script_command_dispatch` | 1405 | 6.13 | 67.49 |
| `004538a0` | `nerpfunc_set_game_speed` | `camera_focus_scroll_scale_commit` | 95 | 39.44 | **100.00** |
| `00453900` | `nerps_set_has_next_button` | `deferred_callback_set` | 10 | 100.00 | **100.00** |
| `00453910` | `nerps_replay_message_sound` | `objective_deferred_callback_invoke_if_set` | 15 | 25.00 | **100.00** |
| `00453920` | `nerps_message_previous` | `tutorial_advisor_scroll_step_advance` | 203 | 59.81 | **100.00** |
| `00453bb0` | `nerps_runtime_update_timers` | `tutorial_frame_timer_array_advance` | 240 | 50.00 | 96.97 |
| `00453da0` | `nerps_runtime_update_camera` | `tutorial_camera_focus_tween_update` | 569 | 11.90 | 92.62 |
| `00454600` | `nerps_draw_tutorial_icon` | - | 256 | unmapped | 96.05 |
| `00454880` | `nerps_monster_destroyed` | `global_counter_004ff990_increment` | 7 | 100.00 | **100.00** |
| `00454890` | `nerps_monsters_destroyed_reset` | `global_counter_004ff990_reset` | 11 | 100.00 | **100.00** |
| `00454b70` | `nerpfunc_set_level_completed` | `objective_voice_alias_raise_commit` | 16 | 71.43 | **100.00** |
| `00454b80` | `nerpfunc_set_level_fail` | `objective_voice_alias_raise_gate_bit27` | 28 | 40.00 | **100.00** |
| `00455300` | `nerps_object_count` | - | 51 | unmapped | **100.00** |
| `00456210` | `flh_magic_uu_noop` (shared) | `flh_magic_uu_noop` | 3 | 100.00 | **100.00** |
| `00456240` | `nerpfunc_get_tutorial_flags` | `system_state_flags_get` | 6 | 100.00 | **100.00** |
| `00456550` | `nerpfunc_set_tutorial_flags` | `system_state_flags_set_target` | 15 | 100.00 | **100.00** |
| `004565f0` | `nerpfunc_set_message_permit` | `engine_mode_set_target` | 65 | 81.82 | **100.00** |
| `00456640` | `nerps_get_message_permit` | `global_flag_004ff978_get` | 6 | 100.00 | **100.00** |
| `004567c0` | `nerpfunc_set_objective_switch` | `global_value_004ff9bc_store_and_return_target` | 15 | 100.00 | **100.00** |
| `004567d0` | `nerpfunc_get_objective_switch` | `objective_switch_state_get` (naked asm) | 28 | 100.00 | **100.00** |
| `00456810` | `nerps_play_message_sound` | `advisor_sound_alias_play_global` | 28 | 29.63 | **100.00** |
| `00456830` | `nerps_parse_message` | `advisor_inline_token_render` | 833 | 14.26 | 67.19 |
| `00456b80` | `nerps_init_block_pointers_table` | `tutorial_path_coordinate_index` | 174 | 17.52 | **100.00** |
| `00456c30` | `nerps_get_block_pointer` | `object_pool_list_head_get` (naked asm) | 34 | 100.00 | **100.00** |
| `00456c60` | `nerps_free_block_pointers` | `object_pool_lists_free_all` | 71 | 44.90 | **100.00** |
| `00456cb0` | `nerps_block_pointer_enumerate` | `ledger_stub_00456cb0_CoordinateListCallbackVisit` | 65 | 10.53 | **100.00** |

Both naked-asm port bodies (`004567d0`, `00456c30`) are plain C here.


## The unit's edges

`translation-units.csv` gave four functions low confidence
(`boundary:GAME-029`): `00456b80`, `00456c30`, `00456c60` and `00456cb0`. All
four belong to NERPs.c:

- The function table at `004a5948` (293 rows and an `**End Of List**`
  sentinel, 12 bytes each: name, function, kind) points at 285 functions in
  `00452df0`-`004574e0`, on both sides of the four, up to
  `GetMonsterAtTutorial` (`004574a0`). Objective.c starts at `004574e0`.
- `00456c30` (the block pointer list head) is called only by `00456c60`,
  `00456cb0` and the table's `00457170`. `00456cb0` (the block pointer
  enumerator) is called only by 17 table functions (`GetTutorialBlockIsGround`,
  `SetTutorialBlockClicks`, `GetMonsterAtTutorial` and others).
  `00456b80` and `00456c60` are called from Lego.c's level load (`0042b1de`)
  and level free (`0042f1e1`), as NERPs' block pointer setup and teardown.
- No function outside the unit reads or writes the unit's `.bss`
  (`004ff958`-`004ff9c0`), its `.data` or the common `nerpsGlobs`
  (`00555d40`-`00555df4`).

The unit is much larger than its 30 mapped functions: about 319 functions and
15,823 code bytes. Ghidra created none of the table's functions, because they
are reached only through the table. 276 distinct table functions (10,296
bytes, with 13 helpers they call) are not rebuilt here; see "The function
table" below. `004567d0` (`GetObjectiveSwitch`) was annotated in the port but
missing from the unit table; it is rebuilt here too, so the unit has 31
annotated functions.

## How the conversion is done

### The port tail

The port's tutorial script is another contract: hook tables
(`s_tutorial_script_ops`, `s_advisor_inline_token_ops`), the port's own
interpreter state and token tables, and globals that native tests pin.
`test_message.cpp` checks that the port's "has next button" word starts at 0;
the original's (`004a673c`) starts at 1. So, as in findings 570 and 582, it
stays a port: `tutorial_script.cpp` is renamed `tutorial_script_port.cpp`,
every `// FUNCTION` becomes a `/* Port model of 0x... */` comment, its seven
`// GLOBAL` markers are dropped (and `tutorial_hud.cpp`'s on the port's copy of
`00555d40`), and the file is a `tail` entry in the link-order list. Native
callers and tests keep calling it under the port's names. Native tutorial
behaviour does not change; #552's NEXT-arrow gap is untouched.

One entry keeps the port's name: `flh_magic_uu_noop` (`00456210`,
`NERPFunc__Stop`, `False`, `Null` and four more rows), which returns zero and
has no state. `flic_unit.cpp` and `crt.cpp` call it. It is shared only under
MSVC x86 (as finding 571's shared entries): natively the port tail keeps its
definition, because a gods98 test that calls it would otherwise pull
`tutorial_script.o` in after the link's last gods98 pass and leave
`file_load_binary` unresolved.

### State

- `nerpsruntimeGlobs` (`004ff958`-`004ff9c0`), one static struct as the
  recipe's step 3. Its first member, `state` (0x2c bytes), holds the registers
  R0-R7, the message permit, the train flags and the tutorial flags; the
  script loader clears just that member with one `memset` (`rep stosd`, 11
  dwords). Then the "next" acknowledgement, the message wait and timer, the
  monsters-destroyed counter, the debug flag, the four NERP timers, the arrow
  suppression and click-only flags, the tutorial icon's bounce phase and the
  objective switch. (As two statics, MSVC aligned the second to 8 bytes and
  moved it 4 bytes later than the original's `004ff984`.)
- `nerpsGlobs` (`00555d40`, 0xb4 bytes): a common (tentative definition), as
  the original. Camera tracking, zoom and rotation; the script and message
  buffers; the image and sound token lists; the message history, offset and
  acknowledgement count; the tutorial icon's object; the advisor sound flag.
  The loaders clear it with `memset`.
- `.data` in address order: the function table, the operators' debug names
  (`004a6710`), and the initialised scalars `004a673c`-`004a6768`. Two
  dwords there, `004a6744` and `004a6754`, are zero and never read or
  written; as zero-initialised scalars MSVC put them in `.bss` and every
  later item moved 4 bytes, so each is the zero tail of a small initialised
  object (`NERPs_Next` at `004a6740`, `NERPs_MessageTime` at `004a6748`),
  which keeps them in `.data` at the original's offsets. Field access compiles
  as the scalar did. (The objects need named types: the `// GLOBAL` parser
  takes the variable's name from the declaration line, and an anonymous
  `static struct {` left the original side unbound and cost `00453920` its
  exact match.) The
  pointer at `004a676c` (a callback that only unconverted table functions pass
  to `00456cb0`) is left to their conversion. Float constants are literals.

The unit declares its own views at the original's Win32 offsets: `NERPs_Level`
(Lego_Level: `map`, `width`, `height`, `blocks`, the 0x37 block pointer heads
at `+0x114`, `status` at `+0x27c`), `NERPs_Block` (the block pointer byte at
`+0x04`) and `NERPs_Object` (the tutorial icon flag at `+0x3f8`).
`tutorial_script.h` keeps its one declaration for the native build, because
`info_unit.c`, `panel_unit.c` and `interface_unit.c` include it (see below);
the unit's prototypes are in `tutorial_script.c`, inside `extern "C"` for the
native C++ compile.

### The function table

The table is defined in the unit as the original, with all 294 rows, their
names (each marked `// STRING`, except `"Null"`, which the original pools with
`frontend_unit.cpp`'s `004a23f4`) and kinds. The ten table functions rebuilt
here are in it directly, and `True` names `flic_unit.cpp`'s `00484b90`. Each of
the other 276 is named as `nerps_port_<name>`: a thin adapter in the port tail,
generated from `tutorial_script_port_functions.h`, that runs the port's NERP
implementation (`tutorial_builtin_command`) for that row with the original's
argument words. A later conversion of a table function deletes its row from
the list and defines it, annotated, in the unit.

### Callees and callers

- **`extern "C"` in place** on four existing declarations, each on its own
  line, so no line moves: `flh_magic_uu_noop` (`crt.hpp`),
  `sound_deferred_play_expire` and `sound_deferred_timer_set_if_idle`
  (`sound.hpp`; defined in `sfx.cpp`, which #605 owns and which is not
  edited) and `advisor_text_request_setup` (`frontend.hpp`).
  `s_overlay_word_03614` joins `s_overlay_word_03610` in the existing one-line
  `extern "C"` declarations in `text.cpp`, `engine.cpp` and the port tail.
- **LegoObject.c.** `LegoObject_GetBlockPos` (`004424f0`) takes the level as
  well in the port, so natively the unit calls `lego.cpp`'s original-ABI
  adapter. #610 rebuilt `004424f0` and `00442260` in `legoobject_unit.c`; in
  the C matching build `tutorial_script_imports.h` maps both calls to those
  (as `legoobject_port_aliases.h` does), which took `00453da0` from 89.93 to
  92.62 and `00454600` from 94.74 to 96.05 on the stack.
- **SFX.c.** #611 defines `00464e80` and `00464ec0` in `sfx.c` under the
  port's names (`sfx_link_names.h`), so the unit's calls reach the unit
  directly, with the C linkage of the `sound.hpp` declarations.
- **Adapters.** Text_SetMessage (`0046ac20`) takes the
  message line, which the port's request word carries on Win32.
- **C callers in the matching build.** `#define`s in headers the callers
  already include (finding 582: a new `#include` moves a unit's symbol counter,
  a `#define` in an included header does not):
  - `aitask_port_aliases.h`: 18 entries for `lego_unit.c` (script and message
    load, shutdown, execute, timers, previous message, replay, "has next"
    button, level complete and fail, object count, tutorial flags get and set,
    message permit, block pointer setup and free) and `legoobject_unit.c`
    (`00454890`, and `00456240` through `system_state_flags_get`).
  - `level.h`: `reward.c`'s `00455300` (an object type's count by name).
  - `weapons_port_aliases.h`: `aitask.c`'s `system_state_flags_get_original`
    (`00456240`).
  - `tutorial_script.h`: in the C matching build only,
    `system_state_flags_get()` becomes `nerpfunc_get_tutorial_flags(0)` for
    `info_unit.c`, `panel_unit.c` and `interface_unit.c`, which now pass the
    original's argument; the declaration it replaces is one for one (no named
    parameter), so their symbol numbering does not move.
- **Callers in files other threads own.** `legoobject_unit.c` (#588) is not
  edited; it reaches `00454890` through `aitask_port_aliases.h`, which it
  already includes. `sfx.cpp` (#605) is not edited. No file of #588, #604,
  #605 or #607 is edited.

### What is left

Each non-exact function and what was tried. "Rule B" is finding 538's counter
phase: inserting 1-64, 96, 128, 192 and 256 inert declarations before the
unit's first function changed none of these scores, so their register and
x87 choices do not depend on the phase near here. The permuter
(`tools/msvc_permuter.py`, 120-240 s each) found no improving rewrite for any
of them (0-33 candidates each).

| Function | Score | Remaining difference and what was tried |
| --- | ---: | --- |
| `00452e70` | 78.61 | register allocation: the original keeps `lineStart` in `edi` (reused around both `strlen`s) and the message buffer in `ebx`; ours swaps them, and reloads the list count into another register; declaration orders tried, no change |
| `00453220` | 90.60 | one register: the sound-list pointer in the release loop is `eax` in the original, `ecx` here (one byte longer); `soundCount > 0` (as the original's `jbe`) took it from 85.47 |
| `004532e0` | 95.00 | the table call `call [ecx + 0x4a594c]` (`nerpsFunctions[i].function`): Reccmp does not symbolize a table displacement in an indirect `call` on either side, so the raw addresses differ; the masked index in a local (as the original) took it from 75.00 |
| `00453320` | 67.49 | register roles: the original gives `negate` `ebx` and the argument count `ebp`; ours swaps them, which moves the prologue and every reset; the declaration order of the four register locals (24 orders, all equal), a `switch` on the kind (38.95), no argument-count variable (46.95) and resetting it inside the branch (35.33) were tried; the six table calls have the same Reccmp limitation as `004532e0` |
| `00453bb0` | 96.97 | `timers[i] + delta` is loaded in the other order (`fld [timer]; fadd [delta]`); source order, `+=`, a pointer loop and a separate local all compile the same; `(elapsed * 1000.0f) * 0.04f` keeps the original's two multiplies (MSVC folds them without the parentheses) |
| `00453da0` | 92.62 | the rotation's two range tests compare as `fld [current]; fcompp` in the original, `fcomp [current]` here, in either operand order; the block position at function scope (the original's frame, from 82.55), the rotation's `* (1/6)` in each branch (from 80.27) and `current > amount - 5.0f` (the original's `fst`/`fcompp`) were applied |
| `00454600` | 96.05 | `screen.y -= bounce * 20.0f + 2 * height` compiles to `fsubp` with a `-20.0f` constant MSVC creates, where the original has `faddp` with `20.0f`; five spellings of the expression compile the same; `(elapsed * 25.0f) * -0.013333334f` keeps the original's two multiplies |
| `00456830` | 67.19 | register allocation in the token loops (the original keeps the text pointer in `ebp` from entry and the output pointer in memory); `found` as a byte (the original's `bl`, from 58.32) and the per-branch `p`/`text` increments and indexed icon loop (from 63.87) were applied |


## Entropy and drops

`make reccmp-check` against #611's baseline (`197aabde`): 35 improved, 2
newly mapped (`00454600`, `00455300`), three below baseline, **no exact match
lost**, no instruction count changed in a dropped function. All three are
C++ port callers that still call the port's models, whose annotations moved
to the unit, so only their call operands no longer map (finding 579 class
(c)):

| Address | Function | File | Before | After | Original calls |
| --- | --- | --- | ---: | ---: | --- |
| `004119d0` | `has_active_resource` | `frontend_unit.cpp` | 41.38 | 34.48 | `00456240` |
| `004191a0` | `main_sim_state_admission_predicate` | `helpwindow.cpp` | 55.56 | 51.85 | `00456240` |
| `0046ac20` | `advisor_text_request_setup` | `text.cpp` | 45.45 | 40.91 | `00456830` |

The classifier labels all three "drift"; the listings differ only in the call
targets (and, in `0046ac20`, the message-timer store the port routes through
`engine_overlay_words_set`). They recover when their own units convert. The
baseline is refreshed for them; none is owner-reviewed. #608's plain-immediate
check (`reccmp-compare`) passes.

On `main` a fourth drop was `0043b6c0` in `legoobject.cpp`. #610 rebuilt it
in `legoobject_unit.c` (`legoobject_select_in_screen_rect`, 77.69%), where it
calls `00456240` with the original's argument through
`LegoObjH_NERPsTutorialFlags`, a cast of the port's `system_state_flags_get`.
A matching-build `#define system_state_flags_get nerpfunc_get_tutorial_flags`
in `aitask_port_aliases.h` (which `legoobject_unit.c` already includes; no C
file includes both it and `tutorial_script.h`) points that call at the unit,
so it keeps its 77.69% and `legoobject_unit.c` is not edited.

The C callers switched in the matching build gain: `info_send` (`00419b80`),
`cell_action_permission_get` (`0041e990`) and
`lego_terrain_cell_command_predicate` (`00428780`) become exact (they now push
the original's argument to `00456240`), `interface_is_item_disabled`
83.64 to 96.67, `interface_is_object_item_disabled` 90.38 to 98.18,
`reward_level_start` 94.59 to 97.30, `lego_start_level_pilots` 86.27 to 88.24,
and eight more Lego.c and panel functions by less than 2 points.


## `opt_ref` (finding 579, `make opt-ref-callers`)

On #611's head, `/OPT:REF` removed 589 annotated functions (276 exact).
With this change it removes 585 (275 exact) in the full report and 568 (264)
game only: the unit's
entries that Lego.c and LegoObject.c reach in the original are now reached
from `lego_unit.c`, `legoobject_unit.c`, `aitask.c`, `reward.c` and the three
C interface units through the aliases above. `tutorial_script.c.obj`: 5 of 31 removed, 3 exact (31 annotated functions; finding 579 counted the relocated file as 28 annotated).
The rest:

| Function | Original caller (live) | Why it stays removed |
| --- | --- | --- |
| `00454600` | Lego.c engine tick `004232e0` | the rebuilt tick calls `lego_port.cpp`'s `world_render_pass_b_original`, the port's render pass; class (c) |
| `00454880` | LegoObject.c `00437940` | `legoobject_unit.c` (#588) calls `legoobject.cpp`'s `legoobject_gb_tutorial_counter_increment_adapter(type)`, which folds the caller's type test into the callee; switching it means editing #588's file |
| `00456640`, `00456830` | Text.c `0046ad00`, `0046ac20` | `text.cpp` is a port model on the port's message slots and token tables; class (c) |
| `00456cb0` | the table's block functions (`004574a0` and 16 more) | they are port-tail adapters until the table functions are rebuilt |

`make reccmp-compare`'s plain-immediate check (#608) passes.


## Native behaviour

Unchanged. Native callers and tests use the port tail under the port's names;
the unit is compiled natively (as C++) but its functions are reached only
from the table's adapters' declarations and the shared `flh_magic_uu_noop`,
which natively stays the port's. The unit's state (`nerpsGlobs`,
`nerpsruntimeGlobs`, the table) adds to native `.bss` and
`.data`. No `port`-label gap is new: the matching build now runs the
original interpreter on the original's state, with the port's NERP functions
behind the adapters for the rows not yet rebuilt, while the port's C++ code
reads the port's own tutorial flags.

## Validation

On stack #613, over #611 (`197aabde`):

- `make reccmp-check` (on the one `match-progress` build: `reccmp-compare`
  with the plain-immediate check, link-order map, winapi link): OK after the
  baseline refresh; against #611's baseline, the three labelled drops
  above, no exact match lost, no instruction count changed.
- `make match-progress` (baseline, listing digests, `binary-progress.json`;
  `winapi-link-check` OK) and `make unit-data-offsets` (GAME-028: `.data`
  312 items, none unknown, offsets equal up to the pooled operator strings at
  `004a6770`; `.bss` one item at the original offset; common
  `nerpsGlobs` 180 bytes).
- `python3 tools/translation_units.py build`; `make reccmp-lint-update`
  (ledger 1,322 to 1,314); `tools/binary_progress_hash.py --write-manifest`.
- `make tools-test` (no `--allow-growth`), `make winapi-link-check`, native
  build and `ctest` (37/37, synthetic fixtures).
- `gcc -std=gnu89 -Werror=declaration-after-statement -fsyntax-only` on
  `tutorial_script.c`.


## Reproducing

    python3 tools/convert_unit.py GAME-028 --stem tutorial_script   # after moving tutorial_script.cpp to the tail
    make match-function SOURCE=src/game/tutorial_script.c
    make reccmp-check
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object tutorial_script.c.obj"
    make match-progress
    make unit-data-offsets
