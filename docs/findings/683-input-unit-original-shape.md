# 683. Input.c as a real unit: the input.cpp and winapi frameloop roots go

Issue #826 converts Input.c (G98-013, `0047ed90`-`0047f130`) from the
relocated C++ file `input_unit.cpp` to the original-shape C unit
`gods98/input_unit.c`. Input.c's port hooks were the last references from
any unit into `input.cpp` and `platform/winapi/frameloop.cpp` (finding
679, section 4). Measured with full matching builds under the gate's link
mode on `main` `ebc07595`.

## Summary

1. **All six functions are exact, and the unit is exactly its original
   size** (883 bytes). Before, they scored 9.70% to 45.45%, and none was
   exact. DirectInputCreateA's import thunk (`0049c7a8`) is now mapped and
   exact too.
2. **Input.c calls DirectInput and Win32 directly**, as the original does.
   It goes through the COM method tables of `IDirectInputA` and
   `IDirectInputDeviceA`, imports DirectInputCreateA, and links
   `c_dfDIKeyboard` (dinput.lib) and `GUID_SysKeyboard` (dxguid.lib). MSVC
   5.0's SDK has no `dinput.h`, so the unit declares the DirectInput 5
   layouts it uses, as `directdraw.c` does for DirectDraw.
3. **No unit references `input.cpp` or `platform/winapi/frameloop.cpp`
   now.** The only root left is `engine.cpp` (Panel.c, Info.c and G98-028,
   #827). The tail stays linked until that root goes (finding 668, section
   5).
4. **The state is in the commons, as separate items.** Input.c owns the
   two key maps, the DirectInput object, the keyboard device, its acquired
   flag and the mouse words. They move from the end of `main.c` into the
   unit. They stay separate commons, not the original's one `Input_Globs`,
   because nine other units read them by their port names (section 2).
5. **`GUID_SysKeyboard` joins the rebuilt dxguid.lib** as one more member
   (`cross/msvc5-importlibs/dxguid.c`). The original keeps it at
   `004a0148` in its GUID block.

## Before and after

| Measure | `main` `ebc07595` | This change |
| --- | ---: | ---: |
| **Exact-function code bytes** | **348,304 (54.21%)** | **349,193 (54.34%)** |
| Input.c exact bytes (unit identity) | 0 of 883 (0%) | 883 of 883 (100%) |
| Exact functions, full / game-only | 2,636 / 2,348 | 2,643 / 2,354 |
| Accuracy, full / game-only | 97.59% / 97.34% | 97.74% / 97.50% |
| `.text` identical at original addresses | 6.76% (43,434 B) | 6.78% (43,593 B) |
| `.data` / `.rdata` identical | 24.72% / 11.67% | 24.75% / 10.92% |
| Identical prefix, raw / masked | 11 B / 7,187 B (to `0x402c13`) | same |
| Correct-size functions, full / game-only | 2,801 / 2,510 | 2,808 / 2,516 |
| Units with size agreement (sizes, start, end) | 2 of 227 (168, 5, 4) | 2 of 227 (170, 5, 4) |
| Size-exact units | 48 | 50 |
| Unit-relative identity | 51.19% | 51.35% |
| Entry point (original `0048f000`) | `0048ebb0` | `0048eb30` |
| SizeOfImage (original 3,608,576) | 4,030,464 | same |
| Data-offset agreement | 20 of 57 units | same |
| Import bytes identical | 62.08% | 67.92% |
| Tail objects referenced from units | 3 | **1** (`engine.cpp`) |
| Tail `.text` / `.bss` / `.data` / `.rdata` | 79,408 / 284,421 / 19,629 / 14,777 | 78,512 / 284,421 / 19,629 / 14,777 |

| Function | Before | After |
| --- | ---: | ---: |
| `0047ed90` input_direct_initialize | 9.70 | **100** |
| `0047eef0` input_state_poll_retail | 31.45 | **100** |
| `0047efb0` input_keyboard_down_key_count | 45.45 | **100** |
| `0047efd0` input_direct_shutdown | 21.43 | **100** |
| `0047f010` input_cursor_position_update | 19.72 | **100** |
| `0047f0d0` input_cursor_client_position_set | 12.73 | **100** |
| `0049c7a8` `_DirectInputCreateA@16` (thunk) | unmapped | **100** |

No function falls.

**Two identity measures move away from the original:**

- **The entry point moves 128 bytes earlier.** The relocated Input.c was
  128 bytes longer than the original's, and that hid part of the unit
  segment's shortfall. Input.c is now exactly its original size, so the
  shortfall shows in full.
- **`.rdata` identical falls from 11.67% to 10.92%.** The new 16-byte
  `GUID_SysKeyboard` member moves the dxguid members after it. The original
  holds this GUID, so the bytes are its own.

`.data` grows by the 48-byte `__FILE__` string (`004aaed0`), which the
original has. `.bss` and the commons start 48 and 64 bytes later.

### data_identity

| Region | Original start | Rebuilt start (before → after) | Items at original offset | Paired items (before → after) |
| --- | --- | --- | ---: | ---: |
| `.rdata` | `0049e000` | `004b2000` | 0 | 95 → 96 |
| `.data` | `004a1000` | `004b8000` | 158 (3,037 B) | 2,455 → 2,456 |
| `.bss` | `004b2db0` | `004cf190` → `004cf1c0` | 1 (1,040 B) | 149 |
| commons | `00549520` | `005a3600` → `005a3640` | 2 (35,760 B) | 62 → 65 |

No region has an item at its original address. The `.data` identical
prefix stays at 12 bytes (to `004a100c`).

## 1. The functions

They follow the disassembly, with no hooks or guards:

- `input_direct_initialize` copies `GUID_SysKeyboard` to a local and checks
  each DirectInput call. On failure it calls the function a second time
  inside `directx_result_diagnostic_dispatch(..., 2, __FILE__, line)`, as
  the original's error macro does (lines 118, 121, 124 and 127). The last
  step is `r = Acquire(); acquired = r == 0;`. Through a local `HRESULT`,
  MSVC emits the original's `sete`. Without it, the code is
  `neg`/`sbb`/`inc`.
- `input_state_poll_retail` needs `if (flags & 2) active = 1; else active
  = mainGlobs.active;`. The form `active = 1; if (!(flags & 2)) ...` loads
  the flag byte into `cl`, not `al`. A ternary also matches.
- `input_keyboard_down_key_count` counts with a pointer and a count-down
  loop. An indexed `for` is 2 bytes longer.
- `input_cursor_client_position_set` stores `msx` before `msy`. In the
  other order, the score was an effective match only (registers swapped,
  69.23% in the linked report).

**Library names.** Reccmp left the original's references to
`c_dfDIKeyboard`, `GUID_SysKeyboard` and the DirectInputCreateA thunk
unnamed, so `input_direct_initialize` scored 93.10% with identical
instructions. Two changes fix this. `GLOBAL` markers sit on the two
`extern` declarations in `input_unit.c`; Reccmp's parser accepts them, and
`make reccmp-lint` passes. A `LIBRARY ... SYMBOL` marker for the thunk is
in `crt/crt.cpp`, next to DDRAW's.

**Native.** The file compiles to nothing natively, as `main.c` and
`directdraw.c` do. `input.cpp` gives the six functions their native bodies
under `LEGORRR_BACKEND_MINIWIN` (PORT-ONLY). These are the old bodies
without their WinAPI branches: the DirectInput hook table (never set, so
natively the device never opens) and the platform cursor. The MinGW
reference build compiles the original's code and now also links `dinput`
and `dxguid`. No MinGW toolchain was available here, so that link was not
tried.

## 2. The state

Input.c's state is in the commons, `0076aa00`-`0076ac80`. `input_unit.c`
makes these tentative definitions:

| Address | Item | Name |
| --- | --- | --- |
| `0076aa00` | prevKey_Map[256] | `s_global_previous_keys` |
| `0076ab00` | lpdi | `s_input_lpdi` |
| `0076ab04` | lpdiKeyboard | `s_input_lpdi_keyboard` |
| `0076ab0c` | fKeybdAcquired | `s_input_keyboard_acquired` |
| `0076ab24`, `0076ab28` | msx, msy | `s_renderer_word_a`, `_b` |
| `0076ab34`, `0076ab38` | diffx, diffy | `s_cursor_delta_x`, `_y` |
| `0076ab3c`-`0076ab58` | the mouse words | `g_renderer_word_c`, `s_window_mouse_word_*` |
| `0076ab80` | Key_Map[256] | `s_global_current_keys` |

`main.c` no longer defines the commons. Its block came after its last
function, so removing it moves no counter.

**Why not one `Input_Globs`.** A struct needs every reader to name
`inputGlobs.<field>`. A C name cannot alias an offset into another
symbol, and a struct annotated beside separate names scores readers
against `inputGlobs+0x124` (finding 679, section 1). The readers are
FrontEnd.c, Lego.c, LegoObject.c, Objective.c, Reward.c, Interface.c,
Message.c, ScreenEffectQuads.c and NERPs.c. Several of them are other open
issues' files. Each would need the symbol-counter placement work of
finding 679, section 3. So this is a follow-up: one `Input_Globs` with
`main_globs.h`-style field names for the matching build, done when those
units' owners can take the counter review.

## 3. What still references the tail

| Tail object | Referenced from | Owner |
| --- | --- | --- |
| `engine.cpp` | Panel.c `engine_shared_state_word` (7), Info.c (1), G98-028 `s_window_mode_flag` (1) | #827 |

No unit references `input.cpp` or `platform/winapi/frameloop.cpp` now,
but both are still in the link, which the tail's last root keeps there.
`input.cpp` keeps no code, only its 4,944 bytes of `.bss`. The inventory
shows no reference into it, so another tail object probably pulls it in
the archive pass; that was not traced. `platform/winapi/frameloop.cpp`
keeps 16 bytes of code, through `lwsc_winapi.cpp`. The tail's kept
`.text` falls by 896 bytes. Its `.bss`, `.data` and `.rdata` do not move.

## Files outside the issue's own

- `src/gods98/main.c`: the commons block and one header-comment sentence
  are removed (#829 owns the file).
- `src/crt/crt.cpp`: the thunk's `LIBRARY` marker.
- `cross/msvc5-importlibs/dxguid.c`, `importlibs.cmake`: the
  `GUID_SysKeyboard` member.
- `CMakeLists.txt`: `dinput` and `dxguid` in the MinGW reference link.
- `src/link_order.cmake`: the unit's entry and the `LRR_LINK_SYMBOLS`
  comment. The three `/INCLUDE`s stay: no object references
  `c_dfDIMouse`, and the order keeps the original's member order.
- `src/gods98/meson.build`, `src/game/test_c_boundaries.cpp`,
  `src/gods98/port_state.cpp` (one comment),
  `tools/binary_progress_inputs.txt` (regenerated; #830 owns `tools/`).

`input_for_input_unit.hpp`, the frozen header that only `input_unit.cpp`
used, is deleted with it.

## Checks

- `make reccmp-check` passes: six functions rise, the thunk is newly
  mapped, none falls, and the map check is OK.
- `make match-progress`, `make unit-data-offsets` and `make pe-audit`
  pass. The image end is `0x7d8000`, as before.
- `make tools-test` (no `--allow-growth`), `make winapi-link-check` and
  `make reccmp-lint` pass.
- `python3 tools/translation_units.py build` refreshes the unit map.
- The native build compiles, and its 39 ctest tests pass with the real
  WADs. `LegoRR` starts with the offscreen driver and runs for 20 s.

## Reproduce

```
make reccmp-check && make match-progress
python3 tools/port_tail_inventory.py --json /tmp/tail.json
make unit-data-offsets && make pe-audit
```
