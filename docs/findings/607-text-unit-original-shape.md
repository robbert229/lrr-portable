# Finding 607 — Text.c (GAME-045) rebuilt in original shape, with a port tail

Issue #652, part of #388 under [ADR 0005](../adr/0005-original-shape-c-units.md).
GAME-045 is the game's `Text.c` (text `0046a7f0`-`0046b1d0`, 10 functions,
all high-confidence). It is the in-game message panel:

- each message type (`Text_Walk` to `Text_UnitUpgraded`, 26 types) has a
  text, an optional image and an optional sound, loaded from Lego.cfg's
  `TextMessages` and `TextMessagesWithImages`;
- Text_DisplayMessage selects a type, and Text_Update prints it into the
  text-only window, or into the image window beside its image;
- a NERPs message line (Text_SetNERPsMessage) takes precedence;
- when the text overflows the window, the panel grows a font line at a
  time, and it shrinks back once the text fits again.

#496 relocated the unit verbatim into `src/game/text.cpp`. This change
rebuilds it as the C unit [`src/game/text.c`](../../src/game/text.c), with:

- the C header [`text.h`](../../src/game/text.h);
- the unit-private imports header
  [`text_imports.h`](../../src/game/text_imports.h);
- the matching-build link names
  [`text_link_names.h`](../../src/game/text_link_names.h).

The port's implementation moves, unannotated, to the port tail
[`text_port.cpp`](../../src/game/text_port.cpp).

"Before" is the *match baseline* on `main` at `321fdbf1` (#653; its only
change from `18cc9483` is `README.md`). "After" is this change, from the
full matching build (`make reccmp-check`).

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (10 functions, weighted by function span, 2,528 bytes) | 21.61% | **99.73%** |
| Exact functions in the unit | 0 | **8** (no asm) |
| Mapped functions in the unit | 8 | **10** |
| Exact-function code bytes (`binary-progress.json`) | 183,830 | **184,977** |
| Accuracy, full / game-only | 84.26% / 82.73% | 84.47% / 82.95% |
| `opt_ref` annotated (full, by address): removed / exact removed | 500 / 243 | 497 / 243 |
| `opt_ref` annotated (game-only, by address) | 481 / 230 | 478 / 230 |
| Reccmp lint ledger (entries) | 1,239 | **1,236** |

The weights are the functions' spans up to the next function, padding
included. An unmapped function counts as 0%.

The unit's names follow OpenLRR's `Text_*` functions in the repository's
snake case. They are `inferred`: the binary attests only the 26 type names.
"Link name" is the name the unit's definition takes in the matching build
(see [Link names](#link-names)).

| Address | Function | Link name | Span | Before | After |
| --- | --- | --- | ---: | ---: | ---: |
| `0046a7f0` | `text_load` | `ui_message_strings_initialize` | 352 | 37.84 | **100.00** |
| `0046a950` | `text_initialise` | `text_message_panel_initialise_adapter` | 144 | unmapped | **100.00** |
| `0046a9e0` | `text_clear` | (its own) | 176 | unmapped | **100.00** |
| `0046aa90` | `text_restart` | `ui_message_overlay_state_initialize` | 64 | 25.00 | **100.00** |
| `0046aad0` | `text_get_type_from_name` | `ui_message_label_index_find` | 64 | 73.53 | **100.00** |
| `0046ab10` | `text_set_message` | `ui_message_text_slot_assign` | 160 | 39.39 | **100.00** |
| `0046abb0` | `text_set_message_with_image` | `ui_message_text_slot_assign_with_image` | 112 | 36.96 | 97.22 |
| `0046ac20` | `text_set_nerps_message` | `advisor_text_request_setup` | 64 | 40.91 | **100.00** |
| `0046ac60` | `text_display_message` | `advisor_text_animation_update` | 160 | 35.09 | **100.00** |
| `0046ad00` | `text_update` | `advisor_text_display_worker` | 1,232 | 13.25 | 99.70 |

- **The two functions with no source**, `0046a950` (Text_Initialise) and
  `0046a9e0` (Text_Clear), are written and mapped. `lego_unit.c` called a
  no-op `PORT-ONLY` adapter for Text_Initialise; the unit's definition now
  takes that adapter's name in the matching build.
- One caller outside the unit rose with no source change:
  `0041fb50` `lego_game_config_init` (93.84%, from 93.80%). It calls
  Text_Initialise, which is now mapped.

### What is left

Both are callee operands only. Every instruction matches.

- **`0046abb0`** (97.22%): the call to Image_LoadBMPScaled (`0047d9d0`). The
  unit calls `images.h`'s unannotated adapter `image_load_bmp_scaled`, as
  `bubbles.c`, `priorities.c` and `reward.c` do.
- **`0046ad00`** (99.70%): the call to Image_DisplayScaled (`0047dcb0`),
  `images.h`'s unannotated adapter `image_display_scaled`.

Converting Images.c (G98-011) closes both.

No naked asm, and no `Deviation (scoring only)`. The unit does not use the
flag `0x800000`, and the plain-immediate check (finding 584) passes.

## Unit extent

All ten functions were high-confidence, and the extent is confirmed:

- **Data.** The unit's state is one `.bss` block, `00503190`-`0050366c`
  (`textGlobs`, 0x4dc bytes). Every function reads or writes it. The 26
  type-name literals `004aa460`-`004aa640` are referenced only from
  `0046a7f0`. The unit's one `.rdata` operand, `0.0f` at `0049e7b4`, is a
  pooled literal.
- **Calls.** `0046a9e0` is called only by `0046a950`, `0046aa90` and
  `0046ad00`. The unit's callers are Lego.c, LegoObject.c, AITask.c,
  Advisor.c, NERPs.c, Objective.c and Priorities.c
  (`docs/coverage/call-graph/edges.csv`). `0046ad00` has no direct caller in
  the call graph. Lego.c's world update calls it (`lego_unit.c`).
- **The edges.** Outside the unit, only Lego.c's `004263c0` and NERPs.c's
  `004565f0` reference `00503190`-`0050366c`, by direct writes (see
  [Callers](#callers)). No function outside the unit references the type
  names. `0046b1d0` starts ToolTip.c (GAME-046, finding 597).

`python3 tools/translation_units.py build`: 0 conflicts. Only the
`current_source` column of GAME-045 changes.

## How the conversion is done

### State

`Text_Globs textGlobs` (`00503190`), laid out as the original `.bss`. It
has external linkage, because Lego.c and NERPs.c write it directly (see
[Callers](#callers)).

| Offset | Address | Field |
| ---: | --- | --- |
| `0x000` | `00503190` | `typeName[26]`: the type names |
| `0x068` | `005031f8` | `messages[26]`: the texts, `_` turned into spaces and `\n` into newlines |
| `0x0d0` | `00503260` | `images[26]` |
| `0x138` | `005032c8` | `soundNames[26][32]` |
| `0x478` | `00503608` | `currType`, `Text_Type_Count` for none |
| `0x47c` | `0050360c` | `typeCount` |
| `0x480` | `00503610` | `counter`: NERPs message frames left; `0x8000` keeps it, `0x10000` marks it set |
| `0x484` | `00503614` | `currText`: the NERPs message line |
| `0x488` | `00503618` | `timer`: the current type's display time |
| `0x48c` | `0050361c` | `textOnlyWindow` |
| `0x490` | `00503620` | `textImageWindow` |
| `0x494` | `00503624` | no code reference |
| `0x498` | `00503628` | `panelY`: the panel's resting y |
| `0x49c` | `0050362c` | `messageArea`: the text-only window's resting area |
| `0x4ac` | `0050363c` | `messageBottom`: the area's bottom less one font line |
| `0x4b0` | `00503640` | `panelTimer`: Text_Initialise's last argument (20.0); nothing reads it |
| `0x4b4` | `00503644` | `panelX` |
| `0x4b8` | `00503648` | `currPanelY` |
| `0x4bc` | `0050364c` | `currArea`: the text-only window's current area |
| `0x4cc` | `0050365c` | `imageX`, `imageY` (316, 425) |
| `0x4d4` | `00503664` | `flags`: 1 a new NERPs message, 4 flash with the interface |
| `0x4d8` | `00503668` | `pauseTime`: TextPauseTime × 25, 100 when unset |

The unit declares, as far as it reads them:

- Font.c's record (the height at `+0xbe4`, read inline, as in
  `textwindow.c`);
- TextWindow.c's record (the font at `+0`);
- Lego.c's level (the oxygen level at `+0x1fc`).

The port's four `// GLOBAL` annotations in `text.cpp` (`00503190`,
`005031f8`, `00503610`, `00503614`) are removed. The unit owns those
addresses.

`make unit-data-offsets`, GAME-045 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| GAME-045 | gaps-only (26, 0 unknown) | n/a (0, 0 unknown) fewer than two attributable items | n/a (0, 0 unknown) fewer than two attributable items | 00503190:_textGlobs(size=1244) |

Unit summary: full 0, gaps-only 8, partial 25, n/a 193.

The image's `.bss` grows by 1,280 bytes (`binary-progress.json`
`opt_ref.image.bss_bytes`, 1,607,024 to 1,608,304). That is `textGlobs`,
which now exists beside the port's own message state in the port tail. The
plain-immediate check still passes.

### The port tail

The port's message panel is another contract. Its display worker draws
through `AdvisorTextDisplayOps` hooks that `frontend.cpp` installs, with
NULL guards and a software blit of the slot image. `message.cpp`,
`engine.cpp` and `frontend.cpp` read its state.

As in findings 597-601, `text.cpp` is renamed `text_port.cpp` and becomes a
`tail` entry, also in Meson. Each `// FUNCTION` becomes a
`/* Port model of 0x… */` comment, and each `// GLOBAL` a
`/* Port state of 0x… */` comment.

### Link names

In the matching build, `text_link_names.h` gives nine of the ten unit
functions the port's names (table above). All nine port names have C
linkage (`message.hpp`, `frontend.hpp`, `lego_unit_imports.h`), and every
caller passes the original's arguments:

- the two text windows and the image position, with 316 and 425 passed as
  `void*` on a 32-bit build;
- a type and its text;
- a NERPs message line, which is the port's request word on Win32;
- the elapsed time.

So these callers reach the unit with no caller edit:

- C: `lego_unit.c`, `legoobject_unit.c`, `aitask.c`, `advisor.c`,
  `priorities.c` and `tutorial_script.c`;
- C++: `frontend.cpp`, `frameloop.cpp`, `lego_port.cpp`,
  `legoobject_port.cpp`, `entity.cpp`, `aitask_port.cpp` and
  `tutorial_script_port.cpp`.

What is compiled out of the matching build:

- `text_port.cpp`'s definitions of the nine names, behind
  `LRR_TEXT_PORT_NAMES`, set by an explicit `#if` (finding 590);
- `lego_port.cpp`'s `PORT-ONLY` no-op `text_message_panel_initialise_adapter`,
  with a plain `#if`.

Text_Clear keeps the unit's name `text_clear`, because only the unit calls
it.

Natively the unit keeps its own names and nothing calls it. The port tail
serves every port name, so native behaviour is unchanged.

### Callers

No caller's code changes. Two converted units write Text.c's state
directly, as the original does, and keep their exact matches:

- `004263c0` `lego_global_subsystem_state_reset` (Lego.c) writes the counter
  (`00503610`) and the timer (`00503618`);
- `004565f0` `nerpfunc_set_message_permit` (NERPs.c) writes the counter and
  the message line (`00503614`).

They named the port's words `s_overlay_word_03610`/`s_overlay_word_03614`
(`text.cpp`) and `textGlobs_f_503618` (defined and annotated in
`lego_unit.c`). In the matching build `lego_unit_imports.h` and
`tutorial_script_imports.h` now declare a partial `struct Text_Globs` (up to
the field each one writes) and `extern struct Text_Globs textGlobs`, and
`#define` the old names to its fields. `lego_unit.c`'s `textGlobs_f_503618`
definition is compiled out of the matching build. Natively both units still
write the port's words.

Each header trades two declarations for two, so neither unit's symbol
counter moves (finding 538). This is the pattern of finding 582 and finding
586, where `#define`s in headers were already included. The listings of
every function outside the unit are unchanged (see
[Entropy and drops](#entropy-and-drops)).

No file owned by #648 (`placement.cpp`), #649 (`gods98/avi_player.cpp`),
#650 (`gods98/bitmap_font.cpp`) or #651 (`map_view.cpp`) is edited. Text.c
makes no call into Font.c (`bitmap_font.cpp`). It reads the font height
inline.

Outside the unit and the port tail, this change edits:

- `lego_unit.c`: the `#if` around `textGlobs_f_503618`, and a comment;
- `lego_unit_imports.h` and `tutorial_script_imports.h`: the `textGlobs`
  views;
- `lego_port.cpp`: the `#if` around the Text_Initialise adapter;
- `engine.cpp`: a comment;
- `test_c_boundaries.cpp`: `text.h` and `text_imports.h`.

### Callees

All callees are reached by their names in the matching build:

| Original | Called as | Unit |
| --- | --- | --- |
| Interface_GetFlashingState `0041b9b0` | `sim_status_banner_word_get` | Interface.c (`interface_unit.c`) |
| NERPFunc_GetTutorialFlags `00456240`, NERPs_GetMessagePermit `00456640`, NERPs_ParseMessage `00456830` | `tutorial_script.c` names | NERPs.c |
| Panel_SetCurrentPosition `0045a6e0`, Panel_Update `0045a730`, Panel_AirMeter_DrawJuice `0045c3f0`, Panel_AirMeter_DrawOxygenLow `0045c4b0` | `panel_unit.c` names | Panel.c |
| SFX_GetType `00464c70`, SFX_Random_SetAndPlayGlobalSample `00464ef0` | `sfx_link_names.h`'s `sound_alias_lookup`, `sound_submit_with_global_cooldown` | SFX.c |
| Image_Remove `0047d470`, Image_SetupTrans `0047dbf0` | `image_surface_release`, `image_surface_blit_transformed` | Images.c (`images_unit.cpp`) |
| Image_LoadBMPScaled `0047d9d0`, Image_DisplayScaled `0047dcb0` | `images.h`'s adapters | not converted |
| TextWindow_ChangePosition, ChangeSize, PrintFOverlay, Update, Clear | `textwindow_link_names.h`'s port names | TextWindow.c (finding 600) |
| `_strcmpi`, `malloc`, `free`, `sprintf` | `crt/vc5names.h` | LIBC |

Image_SetupTrans is called through the annotated `0047dbf0`, which takes a
`TextureSurface*`, as `bubbles.c` does. No `extern "C"` was added to any
existing declaration.

### Source shapes that matched

- **Text_Load** (`0046a7f0`, 37.84%):
  - The 26 names come first, then the windows, the image position and the
    two counts. That gave 95.74%, the best of the 126 orders tried; the port's order
    gave 87.23%.
  - `if (0.0f == pauseTime)`: MSVC loads the literal and compares it with
    the parameter, as the original does. `pauseTime == 0.0f` loads the
    parameter first. This gave 100%.
- **Text_Initialise** (`0046a950`, unmapped): `messageArea.y +
  messageArea.height - (float)height`, without parentheses. The
  parenthesised sum schedules the load of `panelTimer` earlier (87.50%, and
  the same for all 60 statement orders). Copying the area member by member
  gives 18.92%.
- **Text_GetTypeFromName** (`0046aad0`): an `unsigned` index. MSVC turns
  the loop into a pointer walk, and an `int` index gives `jl` where the
  original has `jb` (96.55%).
- **Text_SetMessage** (`0046ab10`, 39.39%):
  `for (s = textGlobs.messages[textType], t = message; *t != '\0'; s++, t++)`,
  after the store of the `malloc` result. Taking `s` from the `malloc`
  result itself puts the two pointers in the other registers (69.03%).
  Advancing the parameter gives 59.05%.
- **Text_Update** (`0046ad00`, 13.25%): the original's `noImage` flag,
  initialised to 1 and cleared with the image, and three tails. The tails
  are the interface-flash case, the text-only case (with the grow and
  shrink) and the image case, each with its own calls to the four Panel
  functions. Every instruction matches.

`tools/msvc_permuter.py` was not needed: each near-miss closed with one of
the source changes above.

## Entropy and drops

`make reccmp-check` against the baseline at `321fdbf1`:

- 9 improved: 7 of the unit's addresses and `0041fb50`;
- 2 newly mapped: `0046a950`, `0046a9e0`;
- **none below baseline**, and no exact match lost.

Instruction counts changed only at the unit's addresses (listing digests):

| Address | before | after |
| --- | ---: | ---: |
| `0046a7f0` | 21 | 47 |
| `0046a950` | — | 32 |
| `0046a9e0` | — | 39 |
| `0046aa90` | 13 | 15 |
| `0046aad0` | 38 | 29 |
| `0046ab10` | 63 | 58 |
| `0046abb0` | 44 | 36 |
| `0046ac20` | 22 | 17 |
| `0046ac60` | 62 | 50 |
| `0046ad00` | 353 | 337 |

`004263c0` and `004565f0`, the two converted callers that write
`textGlobs`, keep their listings and their exact matches.

## `opt_ref` (finding 579, `make opt-ref-callers`)

| | before (`main`) | after |
| --- | ---: | ---: |
| `opt_ref.annotated.full` (by address): removed / exact removed | 500 / 243 | 497 / 243 |
| `opt_ref.annotated.game_only` (by address) | 481 / 230 | 478 / 230 |

`make opt-ref-callers OPT_REF_ARGS="--object text.c.obj"` lists no removed
function. All ten are kept.

`main` and this change were built in the same worktree, and their removed
annotated functions were compared by address:

- A fresh build of `main` (`321fdbf1`) counts 505 / 243, not the committed
  500 / 243. This is the layout noise of finding 591.
- **Kept now:** the eight old `text.cpp` addresses (`0046a7f0`, `0046aa90`,
  `0046aad0`, `0046ab10`, `0046abb0`, `0046ac20`, `0046ac60`, `0046ad00`).
  On `main` the original-mode relink dropped all eight. None was exact.
- **Removed now:** none. No other address changed between the two builds.

So the change against the same-worktree `main` build is −8, all in the unit.
The −3 against the committed figure includes the noise.

## Reccmp lint ratchet

1,239 → 1,236 entries (−3, no growth). The port's three static helpers
(`advisor_message_layout_result`, `advisor_text_output_copy` and
`advisor_text_output_draw`) are now in a tail file, which the lint exempts.
`make reccmp-lint-update` removed their entries.

## Native behaviour

Unchanged:

- the port tail serves all nine port names natively, and `lego_port.cpp`
  keeps its Text_Initialise adapter;
- `lego_unit.c` and `tutorial_script.c` still write the port's words;
- the unit compiles natively as C++ under its own names, and nothing native
  calls it.

There is no new native gap.

**Matching-build runtime** (`make reccmp-run` only). The unit's state and
the port's are separate. In the matching build the original callers use
`textGlobs`. The port's readers (`message.cpp`'s slot getters,
`engine.cpp`'s overlay words, `frontend.cpp`'s advisor panel) read the
port's own state. This is the same split that findings 590 and 592 accept
until the callers' units are converted.

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox): OK. None below
  baseline and no exact match lost. The plain-immediate check, the winapi
  link check and the link-order map check pass.
- `make match-progress`, `make unit-data-offsets`: run, and the generated
  files are committed.
- `make winapi-link-check`: OK.
- `make tools-test`: passes (no `--allow-growth`).
- Native: `cmake --build build` and `ctest --test-dir build`: 37 of 37
  pass.
- C subset: `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only src/game/text.c`.
