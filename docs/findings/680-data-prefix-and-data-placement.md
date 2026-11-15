# 680. What decides where data lands, and the last two small tail roots

Issue #817, toward a byte-identical image. Finding 668 explained why the
raw identical prefix stops 11 bytes in. It stops on the absolute address of
`advisorGlobs`, because data does not sit at the original addresses. This
finding works out what decides that placement, section by section, and ranks
the fixes as finding 668 did for `.text`. It also adds data metrics to
`binary-progress.json`, and retargets two tail roots: FrontEnd's save buffer
and G98-028's streams (finding 674, section 5).

The analysis was measured on `main` `160f5efc` (one full matching build)
and checked again on this change's build. The change is stacked on #820
(`match/818-aitask-weapons-size` `46d95afa`, finding 681), and the before
and after figures below are measured against it. Every figure comes from a matching
build under the gate's link mode (`/DEBUG /OPT:REF /OPT:NOICF`). The
original-mode relink (`make opt-ref-link`, no `/DEBUG`) is used where
`.rdata` is compared, for the reason in section 2.

## Summary

1. **Every data address depends on two things.** These are the start of its
   region and its offset from that start. Today no region starts at the
   original's address: `.rdata` starts 80 KB late, `.data` 92 KB late. On
   offsets, `.data` agrees from its start through Bubbles.c: the CRT's
   pointers, Advisor.c, AITask.c, GAME-004 and Bubbles.c, 158 items. The
   first `.data` break is GAME-006's, the first `.bss` break AITask.c's, and
   the first commons break Text.c's (section 3).
2. **`advisorGlobs` needs three things to sit at `004b2db0`** (section 4):
   - `.text` must end in the original's page, so that `.rdata` starts at
     `0049e000`;
   - `.rdata` must fit in three pages, so that `.data` starts at `004a1000`;
   - the initialised `.data` must be exactly 73,136 bytes long.

   The tail's leaving the link gives the first two. It also takes 22,532
   bytes out of `.data`, which is 21,472 bytes long today, so `.data` would
   then be 1,060 bytes short. Unit definitions in the wrong section account
   for about 1,040 of those bytes, and FrontEnd.c's 1,024-byte key table is
   the largest.
3. **Most of the misplacement is the definition form, not the code.** In
   MSVC 5.0:
   - a `const` global goes to `.rdata`;
   - a zero-initialised external goes to the object's own `.bss` (finding
     554);
   - a tentative definition becomes a common.

   Unit files use the wrong form for 39 paired items, 53,562 bytes.
   Examples are FrontEnd.c's tables, Config.c's pooled literals, and
   `objectGlobs`, `textGlobs` and 26 of Lego.c's words. Stats.c's four
   ObjectStats records (1,344 bytes) are defined in LegoObject.c's file.
4. **The fixes are ranked in section 5**, and proposed as follow-ups in
   section 7, not filed. The tail is first because no address can agree
   before every region starts in place. The definition forms come second:
   they are small edits in unit files. In a model that shifts the rebuilt
   offsets, three of them take `.data` from 158 to 384 items (7,292 bytes)
   at their original offsets.
5. **`binary-progress.json` gains `data_identity` (schema 6),** from
   `tools/data_identity.py`, with tests. For each of `.rdata`, initialised
   `.data`, `.bss` and the commons, it records:
   - the region's start on both sides;
   - the raw identical prefix from the region's start;
   - the Reccmp-paired data items at their original offset and address;
   - the defining objects whose items all agree;
   - the first item that does not agree.
6. **Both tail roots are retargeted** (section 6). Three functions rise and
   none falls. `frontend_port.cpp` and `startup_config.cpp` are no longer
   referenced from units, and `startup_config.cpp` leaves the link.
   **Identity cost:** SizeOfImage grows by one page (4,096 bytes). G98-028
   now holds the original's 2,064-byte block (two 1,024-byte paths, against
   the port's 260), and the rest of the tail is still linked. The entry point
   moves 16 bytes further from the original's (`0048eb70` to `0048eb60`),
   because G98-028 shrinks
   toward its original span while the units before it stay short.

## Before and after

| Measure (`binary-progress.json`) | #820 `46d95afa` | This change |
| --- | ---: | ---: |
| **Exact-function code bytes** | **344,002 (53.54%)** | **344,002 (53.54%)** |
| Accuracy, full / game-only | 97.56% / 97.31% | 97.57% / 97.31% |
| Functions that rose / fell | | **3 / 0** |
| `.text` identical at original addresses | 6.78% | 6.79% |
| `.data` identical (section-relative) | 24.31% | 24.32% |
| Identical prefix, raw / masked | 11 B / 7,187 B (to `00402c13`) | same |
| Correct-size functions, full | 2,797 | 2,797 |
| Units with size agreement | 2 | 2 |
| Data-offset agreement (`layout`) | 20 of 57 units | 20 of 57 |
| Function-order inversions, full / game-only | 222 / 1 pairs | same |
| Entry point (original `0048f000`) | `0048eb70` | `0048eb60` (16 bytes further) |
| SizeOfImage (original 3,608,576) | 4,026,368 | 4,030,464 (one page further) |
| Image end (gate) | `0x7d7000` | `0x7d8000` |
| Tail `.text` | 76,128 | 76,080 |
| Tail objects referenced from units | 11 | **9** |
| `data_identity`: `.data` prefix / items at original offset | (new) 12 B / 158 of 2,455 | 12 B / 158 of 2,455 |
| `data_identity`: `.bss` items at original offset | (new) 1 of 169 | 1 of 169 |
| `data_identity`: commons at original offset | (new) 2 of 62 | 2 of 62 |
| `data_identity`: `.rdata` prefix (original-mode link) | (new) 0 B | 0 B |

The "(new)" figures are `tools/data_identity.py` run on `main`'s build; #820 does not change them.

Scores (Reccmp, full report):

| Function | Before | After |
| --- | ---: | ---: |
| `00462270` reward_advance | 90.82 | 91.84 |
| `0048b260` StartupStreamStateReset | 57.14 | 60.00 |
| `0048b2f0` startup_config_stream_load | 36.36 | 50.00 |

## 1. The regions and how they are measured

LINK 5.0 lays out each output section in object order (ADR 0006
decision 1). Within an object, each section keeps the compiler's order. The
data sections of both images hold these regions:

| Region | Original | Rebuilt (gate) | What fills it, in order |
| --- | --- | --- | --- |
| `.rdata` | `0049e000`, 11,232 B | `004b2000`, 22,344 B | the gate's 84-byte debug directory (rebuilt only); units' constants; LIBC; the DirectX GUID block (original) or the dxguid members (rebuilt); the tail's constants (rebuilt); `.xdata$x` |
| `.data` | `004a1000`, 73,136 B | `004b8000`, 94,608 B | `.CRT$X*` (48 B); each unit's data and string literals; LIBC; the late units; the tail and `level.cpp` (rebuilt) |
| `.bss` | `004b2db0`, 616,304 B | `004cf190`, 869,712 B | each object's own `.bss`, in object order |
| commons | `00549520`, 2,247,092 B | `005a36e0`, 2,296,932 B | the common symbols, which LINK allocates after every object's `.bss` |

`tools/data_identity.py` measures each region from its own start. The
rebuilt `.bss` start is the map's `.bss` group, and the rebuilt commons
start at the first `<common>` symbol. The original has no map. Its `.bss`
starts at GAME-001's `advisorGlobs` (the lowest `bss_range` in
`translation-unit-ranges.csv`), and its commons at `00549520` (finding
516). Items are Reccmp's paired data entities (globals, strings, floats,
vtables), and their objects come from the link map.

## 2. `.rdata`

The original-mode link's `.rdata` differs from the original's at its first
byte.

- **GAME-001's four constants are in another order.** The original has
  0.96f, 25.0f, 1.0f and 0.0f. The rebuild has 25.0f first. `advisor.c`
  defines 25.0f as the named static `s_advisor_frames_per_second`
  (`0049e004`). A named constant is emitted at its definition, before any
  function, while literals are emitted in first-use order. With the
  literal `25.0f` the order becomes the original's, but `0x401500` falls to
  84.62% and `0x4016f0` to 48.28%: the symbol-counter phase of finding 550.
  That try was reverted.
- **In the gate, the 84-byte debug directory comes first.** Every gate
  `.rdata` item therefore sits 80 or 88 bytes late (finding 576), so no
  `.rdata` operand in the gate's code can equal the original's. The gate's
  `.text` prefix will stop at the first `.rdata` operand, `0040135e` in
  `00401270`, until the identity metrics read the original-mode link
  (follow-up 6).
- **The unit objects' constants are too long.** By COFF size they hold
  3,736 bytes of `.rdata` against the original's 2,808. In the
  original-mode link, LIBC's first constant sits at offset `0xed8` against
  the original's `0xaf8`. FrontEnd.c accounts for 1,032 bytes: 1,224 against
  about 192. Its `static const` key table
  (`s_keyCharacters`, 1,024 B) and `"Null"` (`s_overlayNullName`) belong in
  `.data` (section 3). G98-002 is 84 bytes long, from GUIDs defined in the
  unit. Lws.c is 132 short. The rest are within ±60 bytes per unit.
- **The original's `.rdata` ends with a 6,784-byte GUID block**
  (`0049f160`–`004a0be0`, 424 GUIDs, after LIBC). The rebuild defines 26 of
  the GUIDs it uses inside unit objects (416 B), and takes four more from
  per-GUID `dxguid` members after LIBC. The original's block holds every
  GUID of the DirectX headers, used or not, so it comes from library members
  that each define a whole header's GUIDs.
- **The tail adds 16,464 bytes at the end.**

## 3. `.data`, `.bss` and the commons: the first divergences

### `.data`

Offsets agree for the first five objects after the CRT's table: Advisor.c,
AITask.c, GAME-004 and Bubbles.c (158 items, 3,037 B). The raw bytes agree
for only 12 of them, because `.CRT$XIC`'s pointer at `004a100c` is a code
address. After that, each step in the offset delta has one cause:

| At | Delta after | Cause |
| --- | ---: | --- |
| `004a1cd4` (GAME-006) | −4 | GAME-006's `":"` literal is Config.c's `const char s_config_rgb_separator[]` in `.rdata` |
| `004a1d5c`–`004a1de4` (GAME-009, GAME-012) | −8 to +128 | item order inside those objects |
| `004a1f60` (GAME-017) | −1,048 | FrontEnd.c's `static const unsigned int s_keyCharacters[256]` and `s_overlayNullName[5]` are in `.rdata`; the original has both in `.data` |
| `004a48b0` (GAME-023) | +296 | Stats.c's four `ObjectStats` records (`004a97a0`–`004a9ce0`, 1,344 B) are defined in `legoobject_unit.c` |
| `004a9ce0` (GAME-043) | −1,032 | the same 1,344 bytes, missing from Stats.c |
| `004aab18` (G98-002) | −1,036 | Container.c's `"::"` is Config.c's `s_config_separator` in `.rdata` |
| LIBC, late units | −1,104 to −1,204 | small LIBC differences |

Without the tail, the initialised `.data` would be 72,076 bytes, 1,060
short of the original's 73,136. FrontEnd.c's two tables and Config.c's two
literals make up about 1,040 of the 1,060, with alignment.

### `.bss`

Advisor.c's `advisorGlobs` is in place (1,040 B). Then:

- **`aiTaskGlobs` is 8 bytes early.** The original has 8 bytes at
  `004b31c0`–`004b31c8`, between `advisorGlobs` and `aiTaskGlobs`, and no
  code addresses them. Both structs have their original sizes.
- **2,480 bytes follow `aiTaskGlobs`** (`004b8064`–`004b8a14`), and no
  code operand addresses them either. Their owner is not known (follow-up 8).
- **35 paired items (52,528 B) are per-object `.bss` in the original but
  commons or `.rdata` in the rebuild:**

  | Item | Unit | Rebuilt |
  | --- | --- | --- |
  | `objectGlobs` (50,748 B), `objectGlobs_f_eadcc`, `s_entity_auxiliary_release_records` | GAME-023 | commons |
  | 26 Lego.c words (`004de208`–`004de58c`: `s_lego_toolTipBlockX`, `s_lego_mouseLastX`, `s_lego_dragging`, ...) | GAME-021 | commons |
  | `objectRecallGlobs` | GAME-030 | commons |
  | `textGlobs` (1,244 B) | GAME-045 | commons |
  | `s_frame_interval_override`, `s_start_level_name` | G98-005 (#816) | commons |
  | `fontGlobs` | G98-008 | commons |
  | the 4-byte word at `004b8a14` | GAME-004 | Config.c's `const char s_config_angle_empty[4]`, in `.rdata` |

  Each is a tentative definition (a common) where the original has a
  zero-initialised external or a static, so it sits in the unit's own
  `.bss` (finding 554). The fix is the form `advisorGlobs`, `panelGlobs`
  and `meshGlobs` already use: `= { 0 }` (findings 554, 669, 677).
  **Finding 662's "it is a common, as in the original" does not hold for
  `objectGlobs`.** `004de790` lies in the original's per-object `.bss`
  run, after Lego.c's `.bss` (ends `004de58c`) and before LightEffects.c's
  (`004eadd8`), far below the commons at `00549520`.
- **The tail's `.bss` (306,792 B) sits between the units' and LIBC's.** It
  holds port copies of state that the original keeps in units and commons.

### Commons

The original allocates the commons in four groups:

1. the game units in reverse unit order: Water.c `00549520`, ToolTip.c,
   SpiderWeb.c, Smoke.c, ScrollInfo.c, Roof.c, Reward.c, PTL.c, NERPs.c,
   Loader.c, LegoObject.c's word, Camera.c, Lego.c, FrontEnd.c,
   Steering.c, Bubbles.c `00557860`;
2. the late units: RNC's tables `00557d68`, Init.c's `initGlobs` `005580a0`;
3. gods98 in reverse unit order: Wad.c `00575500`, G98-028's block
   `00575ce0`, Sound.c, Keys.c, Files.c, Input.c's arrays `0076aa00`, the
   renderer words, DirectDraw.c, Viewports.c, Container.c `0076ad80`;
4. LIBC `0076cd98`.

The rebuild has the first group in the same order. The others differ:
gods98 comes in forward order (Container.c first), then LIBC, then the
late units. The order also differs within one object. The original's four
Smoke.c words are `textures, head, tail, count` at `00552100`, a 52-byte
block aligned to 32, so probably one struct. The rebuild has them as four
commons, `count, head, tail, textures`.

- **The first divergence is `textGlobs`.** It is a common in the rebuild
  and GAME-045's own `.bss` in the original. Without it, `spiderWebGlobs`
  (8 bytes, 8-aligned) should follow `toolTipGlobs` at offset `0x8bc8`, the
  original's. Smoke.c's internal order would then break next.
- Two more items sit between PTL.c's and NERPs.c's commons in the rebuild.
  `objectRecallGlobs` is GAME-030's own `.bss` (`004ffe50`) in the
  original. Objective.c's `s_window_mouse_word_48` is a common of the
  original's third group (`0076ab48`, among the renderer and window words).
- 13 original commons are defined in tail objects: G98-028's flag
  `005764f4` (`engine.cpp`), Input.c's key arrays and cursor deltas
  (`input.cpp`), and the renderer and window words (`port_state.cpp`). Before
  this change there were 14, with G98-028's streams. G98-028 now defines its
  streams and paths in its own `.bss` (section 6). G98-028 is C++, and MSVC
  gives a C++ global no common.

## 4. What `advisorGlobs` needs

The first instruction is `mov [004b2db0], 004a13cc` (`advisorGlobs.` and
`"Advisor_Objective"`). The string is already at its original offset in
`.data`. The struct is at its original offset in `.bss`. For both addresses
to equal the original's:

1. **`.rdata` must start at `0049e000`.** `.text` is 724,437 bytes long,
   and must be at most 643,072. The tail (76,080), `level.cpp` (3,392),
   LIBC's second pass (4,293; finding 658) and one thunk (16) leave with the
   tail. That makes 640,656 bytes, so `.text` ends at `0049d690`.
2. **`.data` must start at `004a1000`.** `.rdata` must be at most 12,288
   bytes. Without the tail (16,464) and `level.cpp` (120) it is 5,760
   bytes, including the gate's debug directory.
3. **The initialised `.data` must be 73,136 bytes long.** It is 94,608.
   The tail and `level.cpp` hold 22,532 of that. The rest is 1,060 bytes
   short, and FrontEnd.c's and Config.c's definitions account for 1,040 of
   them (section 3).

With all three, the first instruction is identical. The raw prefix then
runs toward the masked prefix's 7,187 bytes. In the gate it stops first at
`0040135e`, the 25.0f operand: Advisor.c's constant order and the debug
directory (section 2).

## 5. Ranking: what brings the most data to its original addresses soonest

No item is at its original address today in any region (`at_original_address`
is 0 everywhere). An item reaches its address only when its region starts in
place and its offset agrees.

| Rank | Fix | What it moves | Owner |
| ---: | --- | --- | --- |
| 1 | **The tail out of the link**: the last nine roots (#815's `lego_port`; #816's Main.c globals and key arrays; G98-028's flag in `engine.cpp`) | `.rdata` and `.data` start in place, so the 158 `.data` items already at their original offset (3,037 B) reach their original addresses; the image loses ~76 KB of `.text`, ~16 KB of `.rdata`, ~22 KB of `.data` and ~300 KB of `.bss` | #815, #816 |
| 2 | **`.data` definition forms**: FrontEnd.c's key table and `"Null"` as non-`const` `.data`; Config.c's `":"` and `"::"` as literals in their first users (GAME-006, G98-002), and the GAME-004 word into GAME-004's `.bss`; Stats.c's four ObjectStats records into `entity_stats.c` | modelled for GAME-006's literal, FrontEnd.c's tables and Stats.c's records: `.data` items at their original offset 158 → 384 (7,292 B); the initialised `.data` within ~20 bytes of its original size, so the `.bss` starts nearly in place; `.rdata` 1,032 bytes shorter | unit files (follow-up 1) |
| 3 | **Per-object `.bss`**: `= { 0 }` for the 35 items of section 3 (`objectGlobs`, Lego.c's 26 words, `textGlobs`, `objectRecallGlobs`, `fontGlobs`, Main.c's two) | 52,528 bytes back into their units' `.bss`; `textGlobs` out of the commons, so the commons agree up to Smoke.c | unit files (follow-up 2) |
| 4 | **The measures on the original-mode link**: the identical prefix and `data_identity` read the no-`/DEBUG` relink | `.rdata` operands can agree at all; the raw prefix can pass `0040135e` | tools (follow-up 6) |
| 5 | **The `.bss` gaps**: the 8 bytes before `aiTaskGlobs` and the 2,480 after it | every `.bss` item from AITask.c onward | follow-up 8 |
| 6 | **The commons' order**: Smoke.c's words as one struct; the gods98 and late units' order | the commons after Smoke.c, and every gods98 and LIBC common | follow-ups 3, 5 |
| 7 | **`.rdata` contents**: Advisor.c's constant order (named constant against literal, finding 550); the GUID block; per-unit constant counts | `.rdata` from its first byte | follow-ups 4, 7 |

Ranks 2 and 3 do not need rank 1 to show up. `data_identity` counts
offsets from each region's start, so they raise `at_original_offset` at
once. Their items reach their original addresses when rank 1 lands.

## 6. The two tail roots

Both switches apply only to the matching build (`_MSC_VER < 1200`).
Natively the port keeps its globals under the old names.

### FrontEnd's save buffer (`00557418`)

`00557418` is `frontGlobs.saveData[5]` (`00556fc0` + `0x0c0` + 5 × `0xb8`),
the current game's slot that Reward.c reads the save file into.
`frontend_api.h` now names it `&FRONT_GLOBS_FIELD(uint8_t, 0x458)`, the
form `g_front_save_tree` already uses. `frontend_port.cpp` compiles out its
own `g_front_save_slot_buffer[0xec]`. `reward_advance` (`00462270`) rises
from 90.82% to 91.84%. No unit references `frontend_port.cpp` any more.
Other tail objects still use it, so it stays linked until the tail goes.

### G98-028's streams (`00575ce0`)

The original's G98-028 block is 0x818 bytes from `00575ce0`:

| Offset | Address | Content | Code references |
| ---: | --- | --- | --- |
| `0x000` | `00575ce0` | four stream handles | `0048b260`, `0048b290`, `0048b2f0`; WinMain reads `00575ce4` (`0048bd42`) |
| `0x010` | `00575cf0` | the token-list path, 1,024 B | `0048b2f0` passes it to `004802d0` |
| `0x410` | `005760f0` | the output path, 1,024 B | `0048b2f0` opens it (`"w"`) |
| `0x810` | `005764f0` | a word | none |
| `0x814` | `005764f4` | the window-mode flag | `0048b260`, `0048b280` |

`file_list_streams.cpp` now defines the first three fields as one struct,
`fileListStreamsGlobs`, annotated at `00575ce0`. The port's names become
macros to its fields. The last two fields stay outside the struct:

- the unreferenced word at `0x810`, because no storage is added for
  scoring;
- the window-mode flag at `0x814`, which `engine.cpp` (#816's) still
  defines as `s_window_mode_flag`.

G98-028's two other references into `startup_config.cpp` also change, both
to the original's form:

- `0048b2f0` called `startup_config.cpp`'s `startup_streams_close`. It now
  calls its own unit's `0048b290`, as the original does (an alias).
- `0048b260` stored the port's `s_startup_window_mode` word, which the
  original does not have. The matching build drops that store.

`0048b260` rises from 57.14% to 60.00%, `0048b2f0` from 36.36% to 50.00%.
The rest of their difference is in the function bodies:

- `0048b260` calls `window_mode_flag_set` where the original stores the
  flag directly;
- `0048b2f0` tests both paths and keeps the handle in a local.

`startup_config.cpp` is no longer referenced and leaves the matching link
(48 bytes of tail code, 564 bytes of tail `.bss`).

**What this costs.** The unit's `.bss` grows by 2,064 bytes, the
original's size for this block. The tail's `.bss` shrinks by only 800
bytes, because the rest of the tail stays linked. The image grows by one
page (SizeOfImage 4,026,368 to 4,030,464), away from the original's. G98-028
was 32 bytes longer than its original span and is now 16 bytes longer: the
dropped store takes `0048b260` back to its original 32-byte slot. The unit
segment still ends 1,184 bytes early (on `main`), so the entry point moves
16 bytes further from `0048f000`: from `0048eb80` to `0048eb70` on `main`,
and from `0048eb70` to `0048eb60` on #820.

## 7. Proposed follow-ups (not filed)

1. **`.data` and `.rdata` definition forms in unit files.** Make FrontEnd.c's
   `s_keyCharacters` non-`const` `.data`, and `"Null"` a literal. Return
   Config.c's `":"` and `"::"` to literals in their first users (GAME-006,
   G98-002), and the `004b8a14` word to GAME-004's `.bss`. Move Stats.c's
   four ObjectStats records to `entity_stats.c`. Check each against finding
   550's counter phase with `make match-function`.
2. **Zero-initialised externals for per-object `.bss`.** Give the 35 items
   of section 3 `= { 0 }` (or `static`, where only the unit reads them).
   That is `objectGlobs` and its two neighbours, Lego.c's 26 words,
   `objectRecallGlobs`, `textGlobs`, `fontGlobs`, and Main.c's two (#816's
   file).
3. **Smoke.c's four words as one struct**, in the original's order
   (`textures[10]`, head, tail, count).
4. **The GUID block.** Rebuild the `cross/msvc5-importlibs` GUID members
   to define each header's GUIDs together, as the original's 6,784-byte block
   shows. The units should then declare their GUIDs `extern`, not define
   them.
5. **The commons' order beyond the game units.** In the original the late
   units come before gods98, and gods98 is in reverse unit order. The
   rebuild has gods98 forward, then LIBC, then the late units. This is ADR
   0006 decision 5.4's open question, now with the observed order. The
   likely cause is how LINK meets each common's first definition when it
   pulls library members (finding 658).
6. **The identity measures on the original-mode link.** The gate's
   `.rdata` starts with the 84-byte debug directory, so `.rdata` operands
   cannot agree there. Read the identical prefix (raw and masked) and
   `data_identity` from the original-mode link. Reccmp's `.rdata`
   addresses come from the gate, so `data_identity`'s `.rdata` items need
   the map of that link, or a translation.
7. **Advisor.c's constant order.** The named `25.0f` puts the unit's
   constants in another order. The literal costs three functions to the
   symbol-counter phase (finding 550). Find a form that keeps both.
8. **The `.bss` gaps.** Identify the 8 bytes at `004b31c0` and the 2,480
   bytes at `004b8064`–`004b8a14`. No code operand addresses either.
   Candidates are unreferenced statics of Advisor.c, AITask.c, GAME-003 or
   GAME-004.
9. **G98-028's remaining state and bodies.** Add the window-mode flag
   (`005764f4`) to G98-028's block once `engine.cpp`'s definition moves
   (#816). That leaves `engine.cpp` referenced only by Main.c's globals.
   Then write `0048b260` and `0048b2f0` in the original's form.

## Checks

- `make reccmp-check` passes: three functions rise and none falls.
  `make match-progress` rewrote the baseline, the listing digests and
  `binary-progress.json`.
- `make unit-data-offsets` passes. G98-028's `.bss` now attributes one item
  (`00575ce0`), and the unattributed annotations go from 86 to 85. The
  unit statuses are unchanged.
- `make pe-audit` passes. The image ends at `0x7d8000`, 163,840 bytes below
  `0x800000`.
- `make tools-test` passes without `--allow-growth`, and so does
  `make winapi-link-check`. `tools/tests/test_data_identity.py` tests the
  new module.
- The native build compiles, and its 39 ctest tests pass with the real
  WADs. `LegoRR` starts with the offscreen driver and runs until the timeout.

## Reproduce

```
make reccmp-check && make match-progress
python3 tools/data_identity.py            # the data_identity field alone
make unit-data-offsets && make pe-audit
python3 tools/port_tail_inventory.py
```
