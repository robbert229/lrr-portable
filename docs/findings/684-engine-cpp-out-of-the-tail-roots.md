# 684. engine.cpp is no longer a tail root: Panel.c, Info.c and G98-028

Issue #827 does finding 679 section 4's `engine.cpp` row and finding 680's
follow-up 9. Three references from units kept `engine.cpp` as a tail root.
Each now goes to the original's definition, in the matching link's own
objects. Measured with full matching builds under the gate's link mode
(`/DEBUG /OPT:REF /OPT:NOICF`) on `main` `ebc07595`.

## Summary

1. **Panel.c reads `legoGlobs.flags1` (`00556ecc`).** The original reads
   that word in `0045a730` and `0045adb0`. The port read engine.cpp's
   `engine_shared_state_word()`, its EngineContext word. In the matching
   build, `panel_unit.c` defines `engine_shared_state_word()` as
   `(&legoGlobs_flags1)` after `lego_globs.h`. `lego.h`'s declaration
   stays, so no symbol counter moves (finding 538).
2. **Info.c calls Lego_GotoInfo (`0042de40`) itself.** That is
   `lego_viewport_target_position_clamp` in `lego_unit.c`. Before, it
   called engine.cpp's unannotated no-op adapter `lego_goto_info`. In the
   matching build `info_api.h` renames the declaration and its one use,
   as `panel_unit.c` already does for TransitionControl. Natively the
   adapter stays.
3. **G98-028 holds its whole original block.** `fileListStreamsGlobs`
   now runs the original's full 0x818 bytes from `00575ce0`, with two
   more fields:
   - `field_810` (`005764f0`), which no retail code references. It is
     part of the original's block, not padding. Without it the flag
     cannot sit at its original offset.
   - `windowMode` (`005764f4`), the window-mode flag. In the matching
     build `s_window_mode_flag` is a macro for this field, and engine.cpp
     no longer defines or annotates it. Natively engine.cpp keeps its
     variable.
4. **`0048b260` and `0048b2f0` are in the original's form.**
   - `0048b260` stores the flag directly and no longer calls `0048b280`.
   - `0048b2f0` assigns the open handle to `streams[3]`, tests it, and
     passes it to the close. It has no local and does not test the paths.
     The port's empty-path guards stay native-only.

   Both become exact.
5. **No unit references `engine.cpp` any more.** The tail now has two
   roots, `input.cpp` and `platform/winapi/frameloop.cpp`, both through
   Input.c's hooks (#826). `engine.cpp` stays linked, with 88 bytes of
   `.bss`, only because other tail objects reference it. On its own, this
   change leaves the tail's size almost unchanged. With #826 the
   inventory's model leaves no tail code (section 3).

## Before and after

| Measure | `main` `ebc07595` | This change |
| --- | ---: | ---: |
| **Exact-function code bytes** | **348,304 (54.21%)** | **349,089 (54.33%)** |
| Exact functions, full / game-only | 2,636 / 2,348 | 2,640 / 2,352 |
| Accuracy, full / game-only | 97.59% / 97.34% | 97.63% / 97.37% |
| `.text` identical at original addresses | 6.76% (43,434 B) | 6.83% (43,899 B) |
| `.data` identical | 24.72% (18,102 B) | 24.43% (17,885 B) |
| `.rdata` identical | 11.67% (1,311 B) | 11.52% (1,294 B) |
| Identical prefix, raw / masked | 11 B / 7,187 B (to `0x402c13`) | same |
| Correct-size functions, full / game-only | 2,801 / 2,510 | 2,804 / 2,513 |
| Units with all sizes correct | 168 of 227 | 169 (G98-028) |
| Units with size agreement (sizes, start, end) | 2 (168, 5, 4) | 2 (169, 5, 4) |
| Unit-relative identity | 51.19% | 51.32% |
| Entry point (original `0048f000`) | `0048ebb0` | `0048eb90` (32 B further) |
| SizeOfImage (original 3,608,576) | 4,030,464 | same |
| Data-offset agreement | 20 of 57 units | same |
| Function-order inversions, full / game-only | 222 / 1 pairs | same |
| Tail roots (objects referenced from units) | 3 | **2** |
| Tail `.text` / `.bss` / `.data` / `.rdata` | 79,408 / 284,421 / 19,629 / 14,777 | 79,360 / 284,417 / 19,629 / 14,777 |

`data_identity` (`binary-progress.json`):

| Region | Original start | Rebuilt start, before → after | Items paired | At original offset | At original address |
| --- | --- | --- | ---: | ---: | ---: |
| `.rdata` | `0049e000` | `004b2000` → same | 95 | 0 | 0 |
| `.data` | `004a1000` | `004b8000` → same | 2,455 | 158 (3,037 B) | 0 |
| `.bss` | `004b2db0` | `004cf190` → same | 149 | 1 (1,040 B) | 0 |
| commons | `00549520` | `005a3600` → `005a35e0` | 62 → 61 | 2 (35,760 B) | 0 |

The `.data` raw prefix is still 12 bytes (to `004a100c`, LIBC's
`crt0init.obj`), and the `.rdata` prefix is still 0. The paired commons
fall by one because `005764f4`'s own annotation is gone. The flag is now a
field of `fileListStreamsGlobs`, which is annotated at `00575ce0`.

Five functions rise and none falls:

| Function | Before | After |
| --- | ---: | ---: |
| `00419da0` incident_message_panel_refresh_if_active (Info.c) | 95.24 | **100** |
| `0045a730` simtick_manager_tick (Panel.c) | 94.92 | **100** |
| `0045adb0` panel_cursor_interaction_update (Panel.c) | 82.69 | 87.04 |
| `0048b260` StartupStreamStateReset (G98-028) | 60.00 | **100** |
| `0048b2f0` startup_config_stream_load (G98-028) | 50.00 | **100** |

`0045adb0`'s remaining difference is in its body, not in the flags word.
It is left for a near-miss pass.

### Identity cost

- **G98-028 is now at its original size**: all four functions are the
  original's length. It was 16 bytes longer than its span (finding 680).
  The units before it are still short, so the unit segment now ends
  32 bytes earlier (−1,152 against −1,120), and the entry point moves
  32 bytes further from `0048f000`.
- **`.data` and `.rdata` identical fall by 217 and 17 bytes** with no
  change to either section's contents. This was not traced byte by byte.
  The likely cause is pointer bytes. The code after G98-028 moves 32 bytes
  earlier (`.text` is 80 bytes shorter), and so does the commons region.
  Pointers in `.data`, `.rdata` and LIBC's tables to that code and those
  commons change value, and some of their bytes matched the original by
  chance before. `.text` identical at original addresses rises by
  465 bytes.

## 1. The three references

| Reference | Original | Matching build now | Native |
| --- | --- | --- | --- |
| Panel.c `*engine_shared_state_word()` (5 sites in `0045a730`, `0045adb0`) | `legoGlobs.flags1`, `00556ecc` | `(&legoGlobs_flags1)` | engine.cpp's EngineContext word, as before |
| Info.c `lego_goto_info` (`00419da0`) | `call 0042de40` | `lego_viewport_target_position_clamp` (lego_unit.c) | engine.cpp's no-op adapter, as before |
| G98-028 `s_window_mode_flag` (`0048b260`, `0048b280`) | `005764f4` | `fileListStreamsGlobs.windowMode` | engine.cpp's variable, as before |

Each switch applies only when `_MSC_VER < 1200`. The two unit-side macros
apply only to the C compile (`!defined(__cplusplus)`). The macros are
defined after the declarations they rename or replace, so no unit's symbol
count changes. `match_function.py --source` gave the same scores as `main`
for every other function in the three files.

## 2. G98-028's block

| Offset | Address | Field | Code references |
| ---: | --- | --- | --- |
| `0x000` | `00575ce0` | `streams[4]` | `0048b260`, `0048b290`, `0048b2f0`; WinMain |
| `0x010` | `00575cf0` | `tokenListPath[1024]` | `0048b2f0` |
| `0x410` | `005760f0` | `outputPath[1024]` | `0048b2f0` |
| `0x810` | `005764f0` | `field_810` | none in retail |
| `0x814` | `005764f4` | `windowMode` | `0048b260`, `0048b280` |

Finding 680 left the last two fields out while engine.cpp still defined
the flag. Both belong to the original's block: `0048b260`'s zeroing and the
0x818 span are the original's. So defining them adds no storage that the
original lacks (ADR 0005 entropy rule 4). The unit's `.bss` grows by
8 bytes, and engine.cpp's shrinks by 4.

## 3. What still references the tail

| Tail object | Referenced from | Owner |
| --- | --- | --- |
| `input.cpp` | Input.c (`input_unit.cpp`): the DirectInput hooks and state | #826 |
| `platform/winapi/frameloop.cpp` | Input.c: the cursor position and size | #826 |

`tools/port_tail_inventory.py` models the link with both retargeted:
`input.cpp` keeps 78,736 bytes alone, and retargeting both leaves 0 bytes
of tail code. So once #826 lands, `lrr_tail.lib` should contribute
nothing, and `.rdata` and `.data` can start at their original addresses
(finding 680 section 4). Two `.data` anchor sections, in
`terrain_grid.cpp` and `mesh_port.cpp`, sit inside the tail. Without
roots they keep nothing, because nothing pulls their objects.

## Checks

- `make reccmp-check` passes: five functions rise, none falls, and the
  map check is OK.
- `make match-progress` rewrote the baseline, the listing digests and
  `binary-progress.json`.
- `make unit-data-offsets` and `make pe-audit` pass. The image end is
  `0x7d8000`, as before.
- `make tools-test` (no `--allow-growth`) and `make winapi-link-check`
  pass.
- The native build compiles, and its 39 ctest tests pass with the real
  WADs. `LegoRR` starts with the offscreen driver and runs for 20 s.

## Reproduce

```
make reccmp-check && make match-progress
python3 tools/port_tail_inventory.py
make unit-data-offsets && make pe-audit
```
