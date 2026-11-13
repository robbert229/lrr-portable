# Finding 598 — Bubbles.c (GAME-005) rebuilt in original shape, with a port tail

Issue #637, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-005 is the original's `Bubbles.c`: the info overlays drawn over objects.
It covers three things:

- the thought bubble over a minifigure, showing its current task, or "Idle";
- the health bar of an object that was just damaged;
- the blinking power-off icon over an unpowered building.

Each kind is a table of objects with a timer. Bubble_DrawAllObjInfos draws the
tables and counts them down every frame. When the object UIs are always
visible, every minifigure shows its bubble and health bar.

#485 (finding 537) relocated the unit verbatim into `src/game/bubbles.cpp`. This
change rebuilds it as the C unit [`src/game/bubbles.c`](../../src/game/bubbles.c),
with:

- the unit's C header [`bubbles.h`](../../src/game/bubbles.h);
- the unit-private imports header
  [`bubbles_imports.h`](../../src/game/bubbles_imports.h);
- the link names [`bubbles_link_names.h`](../../src/game/bubbles_link_names.h).

The port's implementation moves, unannotated, to the port tail
[`bubbles_port.cpp`](../../src/game/bubbles_port.cpp).

The unit map gave GAME-005 15 functions, `004070b0`-`00407d60`, with the last
two at medium confidence. **The extent is confirmed, and one function is
added**: the LegoObject_RunThroughLists callback at `00407960` (see
[Unit extent](#unit-extent)).

"Before" is the *match baseline* on `main` at `8601d8ab` (#635). "After" is
this change, from the full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Bubbles.c similarity (byte-weighted, 16 functions, 3,248 bytes) | 38.82% | **98.80%** |
| Exact functions of the 16 | 3 | **10** |
| Mapped functions of the 16 | 15 | 16 |
| Exact functions, full / game-only (`binary-progress.json`) | 1,556 / 1,354 | **1,563 / 1,361** |
| Exact-function code bytes | 178,866 (27.84%) | **180,093 (28.03%)** |
| Accuracy, full / game-only | 83.30% / 81.67% | 83.61% / 82.01% |
| `opt_ref` annotated (full, by address): removed / exact removed | 498 / 239 | 498 / 242 (see [`opt_ref`](#opt_ref-finding-579-make-opt-ref-callers)) |
| `opt_ref` annotated (game-only, by address) | 479 / 226 | 479 / 229 |
| Reccmp lint ledger (entries) | 1,267 | **1,266** |

The weights are the functions' spans up to the next function, padding
included. An unmapped function counts as 0%.

The unit's names follow OpenLRR's `Bubble_*` functions, in the repository's
snake case. "Link name" is the name the unit's definition takes in the
matching build (see [Link names](#link-names)).

| Address | Function | Link name | Bytes | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `004070b0` | `bubble_initialise` | `bubble_action_strings_initialize` | 400 | 100.00 | **100.00** |
| `00407240` | `bubble_load_bubbles` | `bubble_config_texture_load` | 192 | 26.19 | 98.61 |
| `00407300` | `bubble_get_bubble_type` | (unit name) | 64 | 56.34 | **100.00** |
| `00407340` | `bubble_toggle_object_uis_always_visible` | `state_flag_toggle_when_zero` | 32 | 100.00 | **100.00** |
| `00407360` | `bubble_get_object_uis_always_visible` | `global_task_refresh_flag_get` | 16 | 100.00 | **100.00** |
| `00407370` | `bubble_reset_object_bubble_image` | `entity_reset_auxiliary_state` | 48 | 31.25 | **100.00** |
| `004073a0` | `bubble_remove_object` | `entity_pointer_registries_remove` | 112 | 33.96 | 96.67 |
| `00407410` | `bubble_show_power_off` | `type4_object_registry_insert` | 64 | 33.33 | **100.00** |
| `00407450` | `bubble_show_bubble` | `idle_register` | 96 | 42.42 | **100.00** |
| `004074b0` | `bubble_show_call_to_arms` | `type2_timed_registry_insert` | 96 | 21.43 | 94.29 |
| `00407510` | `bubble_set_call_to_arms_timer` | `set_type2_object_record_value` | 48 | 43.75 | 92.31 |
| `00407540` | `bubble_show_health_bar` | `flagged_object_timer_registry_insert` | 96 | 40.00 | **100.00** |
| `004075a0` | `bubble_draw_all_obj_infos` | `screen_effects_tick` | 800 | 9.32 | 97.90 |
| `004078c0` | `bubble_live_object_update_bubble_image` | (unit name) | 160 | 10.26 | **100.00** |
| `00407960` | `bubble_callback_draw_obj_info` | (unit name) | 176 | unmapped | 96.30 |
| `00407a10` | `bubble_live_object_get_bubble_image` | (unit name) | 848 | 51.95 | **100.00** |

`004078c0` was `bubbles.cpp`'s unreferenced ledger stub, whose annotated span
also covered `00407960`. `00407960` had no source.

No function outside the unit changed score.

## Unit extent

The unit is the 16 functions `004070b0`-`00407d60`. Both medium-confidence
functions are Bubbles.c's, and so is a callback that Ghidra left as a label.

- **The callback `00407960`** (Bubble_Callback_DrawObjInfo) has its own
  prologue after `004078c0`'s `ret` and padding. Its one reference in the
  image is the `push 0x407960` in `004075a0`, which passes it to
  LegoObject_RunThroughListsSkipUpgradeParts (`004377b0`). The corpus
  manifest, and so the unit map, do not list it; as with DirectDraw.c's
  callbacks (finding 596), Reccmp maps it through its annotation.
- **`004078c0`** (Bubble_LiveObject_UpdateBubbleImage) compares against the
  `0.0f` at `0049e060`. `004075a0` compares against the same literal, and
  MSVC 5.0 emits a float literal once per object, so the two are in one
  object.
- **`00407a10`** (Bubble_LiveObject_GetBubbleImage) reads only the Bubbles
  commons `00557864`-`005578fc` (the 39 images). Its jump table (`00407cf8`)
  and index table (`00407d40`) end at `00407d5c`, and `00407d60` starts
  GAME-006 (finding 592). Its callers are `00407370` and `004078c0`.
- **The commons.** A scan of `.text` for operands in `00557860`-`00557d58`
  finds only the unit's functions `004070b0`-`00407a10` (the one other hit,
  `0048e690`, is not an operand of an instruction that reads data). The next
  referenced item is Resource's `00557d68`.
- **The left edge.** `00407090`, GAME-004's last function, compares against
  `0049e058`, GAME-004's `.rdata`; the unit's `.rdata` is `0049e060`-`0049e06c`
  (`0.0f`, `12.5f`, `0.5f`, `25.0f`, all used by `004075a0`).
- **Callers.** Lego.c calls `004070b0` and `00407240` (`0041fb50`),
  `00407340` (`004250c0`) and `004075a0` (`0042396e`); `004589c0` (QuitPoll, not annotated)
  calls `00407340` and `00407360`; AITask.c calls `00407370` (`00404793`) and
  `00407450` (`004035c0`, `00406360`); LegoObject.c calls `004073a0`
  (`00437540`), `00407410` (`0043c570`), `00407450` (`00444460`),
  `004074b0` and `00407510` (`0043c900`) and `00407540` (`0043f6a0`).

`translation-units.spec.csv` records this evidence.
`python3 tools/translation_units.py build`: 0 conflicts. The generated map's
confidence comes from the evidence tokens, so `004078c0` and `00407a10` stay
`medium` there.

## How the conversion is done

### State

Bubbles.c's state is one `Bubble_Globs bubbleGlobs` (`00557860`, 0x4f4 bytes)
in the common region:

| Offset | Address | Field |
| --- | --- | --- |
| `+0x000` | `00557860` | `alwaysVisible` (the object UIs) |
| `+0x004` | `00557864` | `bubbleImage[39]` |
| `+0x0a0` | `00557900` | `bubbleName[39]` |
| `+0x13c` | `0055799c` | 72 bytes with no reference |
| `+0x184` | `005579e4` | `healthBarTable[20]` (object, timer) |
| `+0x224` | `00557a84` | `bubbleTable[20]` |
| `+0x2c4` | `00557b24` | `powerOffTable[20]` |
| `+0x364` | `00557bc4` | `callToArmsTable[50]` |

The `.bss` float at `004b8a18` is the power-off icon's blink timer, a
function-level `static` of Bubble_DrawAllObjInfos (see
[Source shapes](#source-shapes-that-matched)).

The LegoObject fields the unit reads at `+0x388` (the bubble timer) and
`+0x38c` (the current bubble image) are named in `legoobject_unit.h`'s
`Lego_Object`, in place of the padding that covered them. A struct counts once
on MSVC's symbol counter, whatever its fields (finding 538), and the layout is
unchanged. The AITask fields (`+0x00` type, `+0x10` target, `+0x38` tool
type, `+0x60` next) are a private view in `bubbles.c`.

The old storage annotations retire, and the variables stay as the port's own
copies:

- `crt.cpp`'s `s_task_refresh_flag` (`00557860`);
- `ai.cpp`'s `s_bubble_texture_handles` (`00557864`);
- `util.cpp`'s `s_bubble_action_strings` (`00557900`);
- `cmd.cpp`'s `s_idle` (`00557a84`).

The 39 `"Bubble_*"` names (`004a19cc`-`004a1ca4`) and `"Bubbles"`
(`004a1cb4`) are marked `// STRING` where the unit uses them; the port tail's
39 `// GLOBAL` string annotations retire.

`make unit-data-offsets`, GAME-005 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-005 | gaps-only (40, 0 unknown) | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 1 unknown) fewer than two attributable items | `00557860`:`_bubbleGlobs` (size 1268) |

### The port tail

The port's bubbles are another contract: name-registered textures with load
and initialise hooks, registries peeked through `ai.cpp`'s accessors, and a
screen-effect tick driven by hook tables. Native code and the tests
(`test_ai.cpp`, `test_crt.cpp`) use it. As in findings 590, 592 and 597,
`bubbles.cpp` is renamed `bubbles_port.cpp` and becomes a `tail` entry. Each
`// FUNCTION` becomes a `/* Port model of 0x… */` comment, and the unused
ledger stub for `004078c0` is deleted.

### Link names

In the matching build `bubbles_link_names.h` gives twelve unit functions the
port's names:

| Address | Unit name | Port name |
| --- | --- | --- |
| `004070b0` | `bubble_initialise` | `bubble_action_strings_initialize` |
| `00407240` | `bubble_load_bubbles` | `bubble_config_texture_load` |
| `00407340` | `bubble_toggle_object_uis_always_visible` | `state_flag_toggle_when_zero` |
| `00407360` | `bubble_get_object_uis_always_visible` | `global_task_refresh_flag_get` |
| `00407370` | `bubble_reset_object_bubble_image` | `entity_reset_auxiliary_state` |
| `004073a0` | `bubble_remove_object` | `entity_pointer_registries_remove` |
| `00407410` | `bubble_show_power_off` | `type4_object_registry_insert` |
| `00407450` | `bubble_show_bubble` | `idle_register` |
| `004074b0` | `bubble_show_call_to_arms` | `type2_timed_registry_insert` |
| `00407510` | `bubble_set_call_to_arms_timer` | `set_type2_object_record_value` |
| `00407540` | `bubble_show_health_bar` | `flagged_object_timer_registry_insert` |
| `004075a0` | `bubble_draw_all_obj_infos` | `screen_effects_tick` |

Every C caller of these names passes the original's arguments, so
`lego_unit.c`, `legoobject_unit.c` and `aitask.c` reach the unit with no
caller edit. `bubbles_port.cpp` compiles its definitions of these names out of
the matching build behind `LRR_BUBBLES_PORT_NAMES`, set by an explicit `#if`
(finding 590).

Eleven of the port names already had C linkage. `global_task_refresh_flag_get`
is given `extern "C"` in place in `crt.hpp`, which adds no declaration
(entropy rule 1). Its callers are C++ port code (`scene_render.cpp`,
`frameloop.cpp`, `test_crt.cpp`), so in the matching build the toggle and the
read now share the unit's flag.

Four functions keep the unit's names in every build:
`bubble_get_bubble_type` (`00407300`),
`bubble_live_object_update_bubble_image` (`004078c0`),
`bubble_callback_draw_obj_info` (`00407960`) and
`bubble_live_object_get_bubble_image` (`00407a10`). Only the unit calls them
in the original. Their port names (`bubble_action_name_to_index`,
`pending_type_value_slot_select`) have C++ linkage and the port's contracts,
and the port tail keeps serving them.

### Callees

- **ObjInfo.c** (`object_overlays.cpp`, GAME-031, not converted).
  ObjInfo_DrawHealthBar (`00459b00`) and ObjInfo_DrawBubbleImage (`00459fd0`)
  take the original's arguments (object or image, x, y), so their
  declarations get `extern "C"` in place in `tutorial_hud_ports.hpp` and
  `frontend.hpp`. ObjInfo_DrawHungerImage (`00459f50`) in the port takes a
  pointer to the energy float, not the object, so a `PORT-ONLY` adapter with
  the original's arguments, `objinfo_draw_hunger_image_adapter`, is added at
  the end of `object_overlays.cpp`. It passes the object's `+0x344`.
- **Images.c**. Image_LoadBMPScaled and Image_DisplayScaled go through the
  existing original-ABI adapters `image_load_bmp_scaled` and
  `image_display_scaled`; Image_SetupTrans is `image_surface_blit_transformed`.
- **Stats.c**, by its link names: StatsObject_GetCollHeight
  (`entity_stats_float_at74`) and StatsObject_GetStatsFlags3
  (`entity_capability_flags_get`).
- **Other exact callees**: Config_BuildStringID, Config_FindArray and
  Config_GetNextItem (`config.c`), Viewport_WorldToScreen (`viewports.c`),
  LegoObject_RunThroughListsSkipUpgradeParts and
  LegoObject_GetActivityContainer (`legoobject_unit.c`), Lego.c's
  `00425d80` (`current_engine_mode_is_zero`), Container_GetPosition
  (`ddiscene_object_position_get`) and `_strcmpi`.

### Caller switches

None were needed: every original caller already reaches the unit by its link
name. No Lego.c or LegoObject.c file is edited apart from naming the two
`Lego_Object` fields in `legoobject_unit.h` (above). No file owned by #638
(`lava_erosion.cpp`), #639 (`gods98/text_window.cpp`), #640 (`gods98/lwo.cpp`)
or #641 (port `.bss` sizing, `viewports_port.cpp`) is edited.
`lava_erosion.cpp` includes `tutorial_hud_ports.hpp`, whose one declaration
gains `extern "C"`; that does not change its declaration count.

### Source shapes that matched

- **The registry inserts** (`00407410`, `00407450`, `004074b0`,
  `00407540`): a `BubbleTimer*` set at the top of each loop iteration and
  used after the loops, an `unsigned` counter, and the object compared as
  `liveObj == entry->object`. Indexed stores kept a second counter and an
  `[ecx*8 + …]` store (46.67%).
- **Bubble_RemoveObject** and **Bubble_SetCallToArmsTimer**: an `unsigned`
  counter gives the original's `jb`; `int` gives `jl`.
- **Bubble_LoadBubbles**: `if (!(flags & REDUCEIMAGES)) name++; else use = 0;`
  orders the branches as the original (94.44% to 98.61%).
- **Bubble_DrawAllObjInfos**: the blink timer is a `static` inside the
  function, as finding 558 predicted. Declared at file scope, before the parameter, its
  `timer += elapsedGame` loads `elapsedGame` first (finding 538, rule A); as a
  block-scope static its symbol comes after the parameter, and MSVC loads the
  timer first as the original does. Separate `t -= e; if (t < 0.0f)`
  statements give the original's `fcom`/`fstp`; the compound form
  `if ((t -= e) < 0.0f)` gave `fld st(0)`/`fcomp`.
- **Bubble_LiveObject_GetBubbleImage**: the outer `switch` keeps the
  original's case order (Gather, Dig, Repair, Clear, Goto, Collect, Dynamite,
  GetTool, Reinforce, ElecFence, Eat, Drive, BuildPath, Train, Recharge,
  Upgrade, Request), and the inner choices are `if`/`else` chains with
  `id == 0` first. Inner `switch`es built jump tables (79.42%).

No naked asm, and no `Deviation (scoring only)`.

### What is left

Every remaining difference is a call operand or an operand Reccmp names
differently; the instructions are the original's.

| Function | Score | Remaining difference |
| --- | ---: | --- |
| `004073a0`, `004074b0`, `00407510`, `004075a0` | 96.67, 94.29, 92.31, 97.90 | **One-past-the-end loop bounds.** The pointer loops end at `00557d54` (`&callToArmsTable[50]`) and, in `00407510`, at `00557d58` (`&callToArmsTable[50].timer`). Nothing in the original image is named there, so Reccmp prints the plain number; in the rebuild the next common follows `bubbleGlobs`, so Reccmp names it `__crtheap`. Nothing in `.text` references `00557d54`-`00557d64`, so the original's size of `bubbleGlobs` (0x4f4 or more) is not known; padding the struct to cover the bound would be a storage guess (ADR 0005 entropy rule 4), so the bound stays. |
| `004075a0`, `00407960` | 97.90, 96.30 | **`test [esi+0x3e0], 0x600000`.** Reccmp reads the flag immediate as an address inside both images (`fileRedundantList` in the original, `s_nerp_runtime` in the rebuild), and they differ. The bytes are identical. `reccmp_plain_immediates.py` handles only values past the original image (finding 584), and this one is inside it. |
| `00407240`, `004075a0` | 98.61, 97.90 | Image_LoadBMPScaled and Image_DisplayScaled are called through the `image_load_bmp_scaled` and `image_display_scaled` adapters; Images.c's originals (`0047d9d0`, `0047dcb0`) are not mapped. |
| `00407960` | 96.30 | ObjInfo_DrawHungerImage goes through `objinfo_draw_hunger_image_adapter` (above). |

`match-function` scores `00407540` (97.37%) and `004078c0` (98.00%) lower
than the full build because its symbol table names the Stats.c callees by
their unit names. The full build maps them and scores both 100%.

## Entropy and drops

`make reccmp-check` against the baseline at `8601d8ab`:

- 12 improved and 1 newly mapped (`00407960`); `004070b0`, `00407340` and
  `00407360` stay at 100%;
- **none below baseline**, and no exact match lost;
- no other function's score changed (`match-baseline.txt` differs only at the
  unit's addresses).

Instruction counts changed only at the unit's addresses (listing digests):

| Address | before | after |
| --- | ---: | ---: |
| `004070b0` | 40 | 40 (operand names only) |
| `00407240` | 85 | 72 |
| `00407300` | 33 | 26 |
| `00407370` | 15 | 11 |
| `004073a0` | 28 | 30 |
| `00407410` | 22 | 22 |
| `00407450` | 29 | 35 |
| `004074b0` | 25 | 35 |
| `00407510` | 15 | 13 |
| `00407540` | 31 | 38 |
| `004075a0` | 173 | 238 |
| `004078c0` | 42 | 50 |
| `00407960` | — | 54 |
| `00407a10` | 308 | 308 |

The three edited shared headers add no declaration: `extern "C"` on an
existing declaration in `crt.hpp`, `frontend.hpp` and
`tutorial_hud_ports.hpp`, and two named fields in place of a padding array in
`legoobject_unit.h`. The listing digests confirm that no function outside the
unit changed.

## `opt_ref` (finding 579, `make opt-ref-callers`)

| | before (`main` `8601d8ab`) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full` (by address): removed / exact removed | 498 / 239 | 498 / 242 |
| `opt_ref.annotated.game_only` (by address) | 479 / 226 | 479 / 229 |

`make opt-ref-callers`: `bubbles.c.obj` and `bubbles_port.cpp.obj` have no
removed function (`bubbles.cpp.obj` had one, `004078c0`). All 16 unit
functions are kept.

Both trees were built and relinked in this worktree, and the removed sets
compared symbol by symbol. The real changes are three annotated functions
that are now kept:

- `004078c0`, the unit's own (before, an unreferenced ledger stub);
- `00459f50` and `00459fd0` (ObjInfo.c), which the unit now calls.

The other nine differences are not removals. LINK 5.0's `/OPT:REF` folds
identical functions, and the relink map lists every folded name under the
object whose body it keeps. `opt_ref_link.py` counts a gate symbol as removed
when its (name, object) pair is not in the relink map, so a folded function
counts as removed whenever another object's identical body is the one kept.
Changing the link's inputs changed which body LINK keeps for nine annotated
functions:

| Function | Kept body before | Kept body after | Counted |
| --- | --- | --- | --- |
| `0047fe20` `file_get_system` | `container_unit.cpp` | `files.c` | removed → kept |
| `00431560` `level_current_owner_get` | `terrain_grid.cpp` | `lego_unit.c` | removed → kept |
| `00472a40` `ledger_stub_00472a40_DDiSceneObjectDestroy` | `view.cpp` | `container_unit.cpp` | removed → kept |
| `00473680` `scene_object_wrapped_pointer_get` | `container_unit.cpp` | `files.c` | kept → removed |
| `00429160` `lego_terrain_cell_palette_resource_at_grid_position` | `lego_unit.c` | `lego_port.cpp` | kept → removed |
| `00437440` `legoobject_grid_record_count_tables_clear` | `legoobject_unit.c` | `legoobject_port.cpp` | kept → removed |
| `00438ed0` `legoobject_flags_contain_requested_single_bit` | `legoobject_unit.c` | `legoobject_port.cpp` | kept → removed |
| `00456210` `flh_magic_uu_noop` | `tutorial_script.c` | `entity.cpp` | kept → removed |
| `00484b90` `flic_opcode25_success_stub` | `flic_unit.cpp` | `lego_port.cpp` | kept → removed |

All nine are exact and present in both relinks, so exact-removed moves by
+3 although no exact function lost its callers. Crediting a folded function as
kept when its body is kept would remove this noise; that is a follow-up for
`opt_ref_link.py`.

## Reccmp lint ratchet

1,267 → 1,266 entries (−1, no growth). `bubbles_port.cpp`'s
`bubble_action_names_ensure_initialized` is in a `tail` file now, so
`make reccmp-lint-update` removed its entry.

## Native behaviour

Unchanged:

- The port tail serves every port name natively.
- The unit compiles natively as C++ under its own names. Nothing native calls
  them, and `objinfo_draw_hunger_image_adapter` has no native caller.
- `extern "C"` on three existing declarations changes only their link names.

There is no new native gap.

**Matching-build runtime** (`make reccmp-run` only). The unit's state and the
port's are separate. In the matching build, callers of the twelve link names
write `bubbleGlobs`, while the port tail's `bubble_action_name_to_index`,
`pending_type_value_slot_select` and `util.cpp`'s `bubble_action_string_get`
read the port's tables. This is the split findings 590, 592 and 597 accept
until the callers' units are converted.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK. None below
  baseline and no exact match lost. The plain-immediate check (finding 584),
  `winapi-link-check` and the link-order map check pass.
- `make match-progress` (baseline, listing digests, `binary-progress.json`)
  and `make unit-data-offsets`.
- `python3 tools/translation_units.py build`: 0 conflicts.
- `make tools-test`, without `--allow-growth`.
- `bubbles.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only`.
- Native: `cmake -S . -B build -DTEST_WADS_DIR=<shipped WADs> && cmake
  --build build && ctest --test-dir build`: 39/39.

## Follow-ups

- **Images.c (G98-011)**: its conversion replaces the `image_load_bmp_scaled`
  and `image_display_scaled` adapters (the call operands left in `00407240`
  and `004075a0`).
- **ObjInfo.c (GAME-031)**: its conversion gives ObjInfo_DrawHungerImage the
  original's arguments, so `00407960` can call it and the adapter goes.
- **The `0x600000` operand** (above) is a Reccmp display artifact; a fix
  belongs in `tools/reccmp_plain_immediates.py`'s family, not in the source.
- **`opt_ref_link.py`**: credit a folded function as kept when its body is kept (above).

## Rebased onto stack #643

This section gives the figures after rebasing onto #643 (`77c9ede5`,
`lwo.c`, finding 601). All the per-function scores above are unchanged on the
rebase.

`make reccmp-check` against #643's baseline improves the same 12 functions
and newly maps `00407960`. No function falls below baseline, and no exact
match is lost. The match baseline and listing digests differ from #643's only
at the unit's addresses.

| | #643 head | this change |
| --- | ---: | ---: |
| Exact-function code bytes | 179,821 (27.99%) | **181,048 (28.18%)** |
| Exact functions, full / game-only | 1,562 / 1,360 | 1,569 / 1,367 |
| `opt_ref` full: removed / exact removed | 507 / 247 | **503 / 246** |
| `opt_ref` game-only | 488 / 234 | 484 / 233 |
| Lint ledger | 1,250 | **1,249** |

`bubbles.c.obj` and `bubbles_port.cpp.obj` still have no removed function, and
`004078c0`, `00459f50` and `00459fd0` are kept. Those three account for −3
removed. The remaining −1 removed and −1 exact removed come from the folded
functions above: on this base `00473680`, `00437440`, `00456210` and
`00484b90` count as removed, and the other five as kept. #643's relink maps
were not kept, so this split is inferred, not compared symbol by symbol.

## Reproducing

    git mv src/game/bubbles.cpp src/game/bubbles_port.cpp   # and make it a tail entry
    python3 tools/convert_unit.py GAME-005 --stem bubbles
    make match-function SOURCE=src/game/bubbles.c
    make reccmp-check
    make opt-ref-link opt-ref
    make opt-ref-callers OPT_REF_ARGS="--object bubbles.c.obj --object bubbles_port.cpp.obj"
    make match-progress
    make unit-data-offsets
