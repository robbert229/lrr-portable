# Finding 682 — The 95–99% near-miss pool beyond `0x40a000`, size first; LegoObject.c and terrain leftovers

Issue #819, part of #388 and ADR 0005. This pass worked the 95–99% near
misses after `0x40a000`, outside Lego.c, AITask.c, Weapons.c, Main.c and
the tail files. It took the functions with the wrong size first, then
finding 676 §4's leftovers. Base: `main` `160f5efc`. Both sides were
measured with one full matching build each: `make reccmp-check`, then
`make match-progress` and `tools/port_tail_inventory.py`.

**Result: six functions rise and none falls. One wrong-size function
reaches the original's size.** No new function is exact, so exact-function
code bytes stay at 344,002. Every change is a source form. There is no
pad, no `#pragma`, no `Deviation (scoring only)` and no edit to a header
shared with another unit.

- **`objective_load_objective_text` (`004574e0`)** goes from 96.23% to
  99.69% and reaches the original's size (2,122 → 2,129 bytes, padded −16
  → 0). Objective.c's unit deficit halves, from −32 to −16.
- **`terrain_map_uv_wobble_update` (`00450b60`)** goes from 36.48% to
  45.08%. Its padded deficit shrinks from −112 to −96, and Map3D.c's unit
  deficit from −128 to −112.
- The entry point moves 32 bytes closer to the original's: `0048eb80` →
  `0048eba0`, against the original's `0048f000`.

## Before and after

| Measure (`binary-progress.json`) | `main` `160f5efc` | This change |
| --- | ---: | ---: |
| **Exact-function code bytes** | 344,002 (53.54%) | 344,002 (53.54%) |
| Functions that rose / fell | | **6 / 0** |
| Accuracy, full / game-only | 97.56% / 97.30% | 97.56% / **97.31%** |
| Correct-size functions, full: count / original bytes | 2,796 / 460,177 | **2,797 / 462,306** |
| Correct-size functions, game-only: count / original bytes | 2,505 / 409,350 | **2,506 / 411,479** |
| Identical prefix (raw / address-masked) | 11 B / 7,187 B (to `0x402c13`) | 11 B / 7,187 B (to `0x402c13`) |
| Unit size agreement (whole / correct sizes / start / end) | 2 / 168 / 5 / 4 | 2 / 168 / 5 / 4 |
| `.text` identical at original addresses | 43,833 B (6.82%) | 43,024 B (6.70%) |
| `.data` identical | 24.31% | 24.31% |
| Fully identical functions at original address | 42 (2,307 B) | 42 (2,307 B) |
| Entry point (original `0048f000`) | `0048eb80` | **`0048eba0`** |
| SizeOfImage (original 3,608,576) | 4,026,368 | 4,026,368 |
| Data-offset agreement | 20 of 57 units | 20 of 57 units |
| Function-order inversions, full / game-only (pairs) | 222 / 1 | 222 / 1 |
| GAME-029 Objective.c unit span | −32 | **−16** |
| GAME-026 Map3D.c unit span | −128 | **−112** |
| GAME-023 LegoObject.c unit span | −160 | −160 |

**`.text` identical at original addresses falls by 809 bytes.** Every unit
after Objective.c now starts 16 bytes later than on `main`, and every unit
after Map3D.c 32 bytes later. Both moves bring the units closer to their
original addresses, but not onto them. Some runs that matched by chance at
the old offsets are lost. This is the effect findings 669 and 676 describe.
No function's code changed for this reason.

`opt_ref.by_category` moves one function from `original_annotated` to
`port_unit` (2,718 to 2,717). This is the `/OPT:NOREF` relink flip that
findings 654, 663, 671 and 676 also saw.

`make reccmp-check` reports no function lower than the baseline.
`make match-progress`, `make unit-data-offsets` and `make pe-audit` were
run on the final tree. The three generated files are committed;
`unit-data-offsets` changes no tracked file. `make tools-test` (no
`--allow-growth`), `make winapi-link-check` and native ctest (37 of 37)
pass. The native build starts with the shipped WADs.

## 1. Per-function results

Sizes are `code_extent` (finding 668): the original's, then the rebuilt
size before → after. Scores are the full report's.

| Address | Function | Unit | Original | Rebuilt | `main` | After | Cause |
| --- | --- | --- | ---: | ---: | ---: | ---: | --- |
| `004574e0` | objective_load_objective_text | Objective.c | 2,129 | 2,122 → **2,129** | 96.23 | **99.69** | a `left` local for the shared x |
| `00450b60` | terrain_map_uv_wobble_update | Map3D.c | 964 | 856 → 867 | 36.48 | **45.08** | block index written inline, `uvWobble` pointer |
| `0043eea0` | legoobject_simulation_queue_flush | LegoObject.c | 598 | 583 → 585 | 89.26 | **90.96** | indexed loop, `first` flag |
| `00419530` | info_compile_text | Info.c | 281 | 279 → 279 | 95.02 | **96.83** | `d++` before `text++`; `count++` after the `'b'` store |
| `004589c0` | lego_quit_poll_adapter | Objective.c | 534 | 534 → 534 | 97.50 | **98.75** | `>= 315` (the original's `jl`) |
| `0048c820` | lwo_surface_settings_read | lwo.c | 2,716 | 2,716 → 2,716 | 97.65 | **98.38** | sub-chunk length as an unsigned short sum |

## 2. The shapes

- **`objective_load_objective_text`: the shared x goes through a local.**
  The original computes `imagePos[0] + 60` once, stores it in a frame
  slot, and copies it into `beginArea.x` and `textArea.x` with two integer
  moves (`mov eax, [esp+0x10]; mov edx, eax`). The port assigned
  `textArea.x` and later copied it with `beginArea.x = textArea.x`. With a
  `float left` assigned to `beginArea.x` and then `textArea.x`, both right
  after it is computed, the function has the original's size and its
  prologue matches to the byte.
  - What is left: two of the three `buff[i] = (str[i] == '_') ? ' ' :
    str[i]` loops store through `[edx + ebx]`, and the original stores
    through `[ebx + edx]`. The first loop, which has the same text,
    already matches. `phase_probe.py` shows one code over 64 shifts.
    These were tried without effect: other index spellings (`*(i + buff)`,
    `i[buff]`, `*(str + i)`), `'_' == str[i]`, another counter variable
    (`len`, `width`, `colons`), and other loop bounds. Two bound forms
    (`<= strlen`) and a cast on the result changed the size.
- **`terrain_map_uv_wobble_update`: no `index` local.** In the original,
  the `else` branch recomputes `(y + blockOffsets[j].y) * blockWidth + x +
  blockOffsets[j].x` from registers it kept from the first read
  (`imul esi, [edi+8]; add esi, ebx; add esi, [bx]`). So the block index
  is written out at both sites, not held in a variable. In the `else`
  branch, one `TerrainMapPoint2* uvw = &...uvWobble` is read twice and
  written once, as the original's single `lea eax, [eax + ecx*8 + 0x1c]`
  shows. That raises the function 8.6 points and adds 11 bytes.
  - What is left is finding 676's: the original strength-reduces the `j`
    loop into two pointers (`&uvOffsets[j]`, `&blockOffsets[j]`) and a
    `j * 8` byte offset in `ebp`, with `j` itself in memory. That gives a
    frame 12 bytes larger. These did not change it: 2-D `[4][2]` arrays
    for either table, and `k = (k + 1) & 3`, `k++; k &= 3`, `% 4` or a
    ternary for the rotated vertex.
- **`legoobject_simulation_queue_flush`: an indexed loop.** The original
  loads `numCarriedObjects` and tests it before it forms the
  carried-object pointer (`cmp; jbe; lea eax, [ebx+0x308]`). The `0` it
  stores to the parameter slot first is the induction variable that MSVC
  strength-reduced away. With `obj->carriedObjects[i]` (read and cleared)
  instead of a `slot` pointer, the loop head matches. The unused `slot`
  local is removed; no LegoObject.c function's score changes with it.
  - The `(i == 0) == 1` test is now `first = (i == 0); if (first == 1)`,
    which raises the single-function score but is still folded to
    `test ebp, ebp; jne`. The original materialises the flag
    (`xor ecx, ecx; sete cl; cmp ecx, 1`) and pushes immediate zeros,
    because its compiler no longer knows `i == 0` there. These were tried
    without effect: a ternary, `!i`, `(unsigned)` and `1u` casts, an
    if/else assignment, and the flag set earlier (which costs more). `(i
    == 0) == deposit` produces the `sete`/`cmp` pair and comes within 3
    bytes of the size, but scores lower. That is the function's remaining
    16 padded bytes.
- **`info_compile_text`: step order.** The loop ends `d++; text++;`, which
  is the original's `inc edi; inc esi`. The image escape stores the `'b'`
  before it stores the incremented count. What is left: the original
  reloads `*text` at the top of every iteration (`mov al, [esi]`), the
  same two bytes `font_v_print_f2` (`0047a1e0`) misses. These did not
  produce it: `for` forms with either step order, and `'\0' != *s`.
- **`lego_quit_poll_adapter`: `>= 315`.** The original tests the back
  button's top edge with `cmp ecx, 0x13b; jl`, which is `y >= 315`. The
  left edge is also `cmp 0x82; jl` in the original. Writing it `>= 130`
  swaps `eax` and `ecx` for the whole test and shortens the function by
  2 bytes, so that edge stays `> 129`.
- **`lwo_surface_settings_read`: the sub-chunk length.**
  `(unsigned short)((v << 8) + (v >> 8))` gives the original's argument
  scheduling for the following `sprintf` and keeps the function's size.
  The original's exact sequence (`xor edx, edx; mov dl, ah; and eax, 0xff;
  shl eax, 8; add eax, edx`) needs a form not found. Every variant that
  moved closer to it was 4 bytes too long.

## 3. Left, with what was seen

- **`0044a200` legoobject_type_component_get (−16 padded, 91.49%).** The
  original's last `return obj` reloads the parameter (`mov eax, [esp+4]`)
  although `eax` still holds it, and ends in its own `ret`. One code over
  64 shifts. These were tried without effect: `default:` inside the
  switch, `if`/`else`, a result local, a ternary, an inverted test, a
  copy of the parameter, and an `if` chain (which costs 28 bytes).
- **`00440c70` legoobject_terrain_grid_path_search (−32 padded, 51.91%).**
  The structure matches. The differences are register and frame
  assignment: the original keeps zero in `ebp` (`cmp edi, ebp` against
  `test edi, edi`) and puts `width` before `height` in the frame. A
  600-second `decls` search (576 orders) found nothing.
- **`0043c2f0` legoobject_periodic_timer25_callback (+16 padded, 96.86%).**
  This is the one pool function above its size. The original tests the
  ability flag in memory (`test byte ptr [esi+0x3f0], 0x20`) and the port
  loads it first (`mov al; test al`), which costs one byte past the slot.
  Its `fimul` is also scheduled later in the original. One code over 64
  shifts. These were tried without effect: nested `if`s, a separate
  radius statement, a scale local, and a declaration initialiser.
- **`0043b8d0` legoobject_lod_meshes_update_callback (−16 padded,
  96.55%).** It is only 2 code bytes short (527 of 529), but that crosses
  a 16-byte boundary. The original compares the distance with `fld st(0);
  fcomp`, and the port with `fcom`. One code over 64 shifts.
- **The other wrong-size functions** (`004157c0`, `00462ad0`, `00465c50`,
  `0047a1e0`) are ±1 or ±2 code bytes off, with the padded size already
  correct. They differ in x87 or store scheduling: `fxch` placement, and a
  store through a pointer register against a direct store.
  - In `reward_scroll_update`, swapping the operands of `length +
    area.height` changes nothing: MSVC orders the two loads itself.
  - In `terrain_map_block_texture_fade_set` (`0044e6d0`), `by *
    gridWidth` against `gridWidth * by` changes nothing, as an initialiser
    or as a statement in any position.
- **The rest of the pool.** A 45-second `operands` search over 18 pool
  functions (FrontEnd.c, Info.c, Reward.c, SFX.c, Font.c, HelpWindow.c,
  Interface.c, LegoObject.c) found no gain, so the other two batches were
  not run. What the diffs show for the remaining functions:
  - most differences are register choice, x87 or store scheduling, a
    swapped base and index in one addressing mode (`Container_Frame_SearchCallback`,
    `wad_file_read`), or two stack slots in the other order
    (`mesh_get_surf_info`: a 145-candidate `decls` search changed nothing);
  - several are link-resolved names or array-end constants that score
    100% in the single-function loop (`wad_get_free_wad`,
    `level_map_slot_allocate`, `named_option_index_find_case_insensitive`);
  - `G98CMovie::Update` compares with `cmp eax, edi` (finding 618).

## Reproduce

```
make reccmp-check && make match-progress
python3 tools/port_tail_inventory.py --json /tmp/tail.json   # unit_spans, entry_point
make match-function ADDR=0x4574e0
python3 tools/phase_probe.py probe 0x0044a200 0x0043c2f0 0x004574e0 --range 0 64 1
```
