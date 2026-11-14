# Finding 635 — Near-miss sweep 5 of FrontEnd.c

Issue #721, part of #388 and ADR 0005. FrontEnd.c (GAME-017,
`src/game/frontend_unit.c`) was converted in #705 and #715 (findings 623
and 628). On `main` it had 20.2 KB short of exact: 12 functions at 95–99%,
10 at 80–94% and 16 below 80%. This sweep follows findings 626, 629 and 630.
It takes both bands, the largest functions below 80%, and part 1's four
naked-asm functions.

Measured on `main` `7835ce86` with `make reccmp-check`.

| Measure | `main` | This change |
| --- | ---: | ---: |
| New exact functions | | **7** (3 at 95–99%, 3 at 80–94%, 1 below 80%) |
| Naked asm left in the unit | 4 | **1** (`004103f0`) |
| Functions that rose / fell | | 11 / **1** (`00437d00`, a layout coincidence, below) |
| Exact-function code bytes | 279,887 (43.56%) | **281,155** (43.76%) |
| Reccmp mean, full / game-only | 95.38 / 94.98 | 95.45 / 95.05 |
| `opt_ref` annotated (full): removed / exact removed | 164 / 127 | 164 / 127 |
| `/OPT:REF` relink `.bss` (`opt_ref.image.bss_bytes`) | 1,115,696 | 1,115,696 |
| Gate image end (`make pe-audit`) | `0x873000` | `0x873000` |

**Stacked on #724** (`5d9209b4`: #723, #726, #725 and #724 below it). The
same 11 functions rise by the same amounts, and nothing else moves.
`00437d00` stays exact there (see below). On that stack:

- exact-function code bytes go from 286,819 (44.64%) to **289,516**
  (45.06%);
- the Reccmp mean goes from 95.96 / 95.62 to 96.03 / 95.69;
- `opt_ref` goes from 125 / 103 to 124 / 102. That is a by-address change:
  the by-symbol caller listing (`make opt-ref-callers`) names no FrontEnd.c
  or Objective.c function;
- relink `.bss` stays 1,116,672 and the gate image end stays `0x875000`.

On the stack, `make opt-ref-callers` and `make pe-audit` were run as well.

`make match-progress` and `make unit-data-offsets` were run, and the three
progress files are committed. `unit-data-offsets` changed no tracked file.
`make tools-test`, `make winapi-link-check` and native ctest (37 of 37)
pass.

**On `main` alone, one exact match is lost, flagged for review.** On the
#724 stack it is not lost, because the layout differs there. `00437d00`
`legoobject_create` (`legoobject_unit.c`, unchanged), 100% to 99.76%. Its
`or ecx, 0x441190` sets entity flag bits. Reccmp reads an in-image constant
as a possible address. The original side has no entity there and prints
`<OFFSET10>`. On the rebuilt side `legoobject_action_sequence_reset` now
starts at exactly `0x441190`, so the same operand is named after it. The
code is unchanged. Layout moved it there: `004124f0` grew by `0x20`, to
exactly the original's `0x260` span, and every later function moved with
it. Before that change the function started at `0x441170`, and on `main` at
`0x441180`. A start anywhere else matches again. Finding 584's
`reccmp_plain_immediates.py` handles plain constants past the original
image end, not inside it. Extending it to in-image constants is tooling
work (#562's area), so this PR doesn't do it. The baseline records 99.76.

## What closed the gaps

The tallies from findings 626–630 point to callees and operands first.
FrontEnd.c has few of those left: its callees are converted and named.
Every gain here came from source shape, one real correction, or layout
evidence.

### 1. Statement and store order (5 exact, including the 3 asm functions)

- **Naked asm to C.** Finding 623 left `00410a10`, `00410d50` and
  `00413f90` as the port's naked asm: their C forms scored 92.98%, 93.90%
  and 90.70%, with the wrong registers. An exhaustive search of the store
  orders (720 orders for six stores, about 4 minutes in the single-function
  loop) found 100% for each:
  - `front_menu_item_create_cycle`: `valuePtr`, `nameCapacity`, `nameCount`,
    `x2`, `y2`, `callback`;
  - `front_menu_create_overlay`: `field_1c`, `next`, `x`, `y`, `sfxType`;
  - `front_menu_item_create_banner_item`: `banner`, then the struct's own
    order: `fontLo`, `fontHi`, `itemType`, `itemData`, `x1`, `y1`,
    `notInTuto`.

  The asm and its `#if` guards are deleted. Each C body declares one local
  that the asm did not, which moves the counter by one per function. Only
  `00411870` moved because of it, from 46.49% to 45.61%, before its
  correction (section 5).
- **`front_menu_item_create_image_item`** (`00410e20`, 92.00% to 100%):
  the same struct order, `fontLo` before `fontHi` and `itemType` before
  `itemData`.

### 2. Branch order and comparison forms (3 exact)

- `front_menu_item_add_select_item` (`00410740`, 80.74% to 100%): the
  `realloc` branch first, `if (select->selItemList != NULL)`. The original's
  `je` skips to the `malloc` branch at the end.
- `front_menu_load_menu_image` (`00410fb0`, 98.32% to 100%): `numParts >= 3`
  (`cmp 3; jb`), not `> 2`.
- `front_is_intros_enabled` (`00414ce0`, 95.00% to 100%): the programmer
  level compared unsigned (`jae`). Part 2 already had
  `FRONT_PROGRAMMER_LEVEL` for this. The macro moves to the top of the file,
  and a `#define` costs no counter.

### 3. Initialisers and block-scoped locals (2 exact, 1 near)

- `front_menu_set_create` (`00414070`, 84.06% to 100%): `memset(menuSet,
  0, sizeof(Front_MenuSet))`. The original's `xor eax, eax; mov [ecx], eax;
  mov [ecx+4], eax` is the inlined `memset`, not two stores.
- `front_load_menu_text_window` (`00415d00`, 97.73% to 100%): `float
  area[4] = { 0 }`. The original shares the `0` in `eax` with the
  `string_path_join` `NULL`. Four `0.0f` stores, or an initialiser with
  four `0.0f`, give immediates.
- `front_menu_load_slot_images` (`00413a60`, 72.19% to 97.59%). The
  original's frame is `0xb8` bytes: it holds either the `"%s\\%d.dat"`
  buffer or the save record, never both. So both are block-scoped, one
  in each loop. The record is `int saveData[0xb8 / 4] = { 0 }`, which gives
  `mov [x], 0; rep stosd` for the rest (Front_SaveData is defined with
  part 2, after this function). The reward count is `for (k = 8; k <
  count; k++)`, which gives the original's `jbe` with no zero test. What is
  left is two end-of-array operands (cause B).

### 4. Locals and argument order (1 exact, 2 rises)

- `front_menu_item_draw_select_item` (`00412450`, 69.81% to 100%):
  `select->selItemList[selIndex]` written out, with no `selItem` local, and
  `dest.x` before `dest.y`.
- `front_menu_item_draw_save_image` (`004124f0`, 45.73% to 79.43%): the
  config string goes into a `str` local before `sound_variant_split`. The
  original calls `block_get_str_copy` before it pushes `"|"` and the parts
  array. `frontGlobs_saveImageBigWidth`/`Height` are `unsigned int`: the
  original converts them with a zero high word (`mov [t+4], 0; fild
  qword`). `objective_unit_imports.h`'s two externs get the same type, and
  Objective.c's code is unchanged.
- `front_menu_item_draw_select_text_window` (`00412750`, 60.62% to 62.32%):
  the overwrite branch first (`!= 0`). `msvc_permuter.py` found it.

### 5. One real correction

`front_menu_item_slider_handle_input` (`00411870`, 46.49% to 89.92%). The
original clamps the click fraction to [0, 1] and scales it by the range. It
does **not** add `valueMin`:

```c
value = ((float)FRONT_MSX - x) * 0.01f;   /* fild msx; fisub x; fmul 0.01f */
if (value > 1.0f) value = 1.0f;           /* fcom 1.0f (0049e174) */
if (value < 0.0f) value = 0.0f;           /* fcom 0.0f (0049e170) */
*slider->valuePtr = (int)((slider->valueMax - slider->valueMin) * value);
```

The port added `valueMin` and did not clamp. The native build uses the same
C, so native slider clicks now behave like the original's. A slider with
a non-zero `valueMin` now gets the original's value, and clicks are
clamped to the range. What is left is rule B: the original's code appears at counter +31
(sections 6 and 7).

### 6. The counter

Every edit was checked over the whole unit with the single-function loop,
then in the linked build. Removing `selItem` from `00412450` lowered the
counter by one for all later functions. That alone moved
`front_level_link_callback_load_menu_item` (`004177e0`) from 69.33% to
100%. The `00413a60` change then raised the counter again and that gain
went away. `004177e0` is rule B with period 8: the original's code appears
at residues 1, 5 and 6. Getting it back would need a pad, which ADR 0005
rule 3 forbids, so it stays at its baseline.

### 7. Rule B and the permuter

A 0–63 inert-declaration sweep (diagnostic only, nothing committed) over 13
of the short functions found the original's code at another phase for one
only: `00411870`, at +31. The other 12 have one code at every residue:
`00410a10`, `00410d50` and `00413f90` (their C forms before the reorder),
`004103f0`, `00412450` (before its fix), `00411530`, `00411100`, `004129d0`,
`00412af0`, `00417f90`, `00417eb0`, `00416d70` and `004157c0`.

`tools/msvc_permuter.py` ran for 90 s on each of 13 functions: `00416d70`,
`004157c0`, `00415d00`, `00417bd0`, `00411360`, `00417090`, `00410e20`,
`00410c70`, `00411f10`, `00412750`, `004129d0`, `00411100` and `00412af0`.
It improved one, `00412750`, and most have no candidates.

## Per-function results

The 38 functions of the two bands and below 80%, and the three converted
asm functions. Sizes are the ledger's address gaps (`00417430` is a 39-byte
function followed by a gap). Letters are the causes tallied below.

| Address | Function | Size | `main` | After | Change, or what is left |
| --- | --- | ---: | ---: | ---: | --- |
| **95–99%** | | | | | |
| `00415d00` | front_load_menu_text_window | 1104 | 97.73 | **100.00** | `area[4] = { 0 }` |
| `00417bd0` | front_save_write_save_file | 544 | 99.38 | 99.38 | A: ObjectRecall.c's `004593e0` is reached through the `front_object_recall_save_adapter` port adapter |
| `00410fb0` | front_menu_load_menu_image | 336 | 98.32 | **100.00** | `numParts >= 3` |
| `004157c0` | front_play_intro_splash | 336 | 97.72 | 97.72 | E: the original loads `0.0f` before the `fstp` of `seconds`; the epilogue is shared |
| `00410320` | front_util_print_f | 176 | 98.21 | 98.21 | A: the original's `_vsprintf` (`0048e1e0`) is annotated as `formatted_text_buffer_build` (finding 505) |
| `00415220` | front_load_selected_level | 160 | 97.83 | 97.83 | A: ObjectRecall.c's `00459360` has no annotation; the call goes to a port adapter |
| `00410490` | front_cache_create | 144 | 98.21 | 98.21 | D: one store scheduled before the `s_cacheList` load (effective match) |
| `00417eb0` | front_save_set_level_completed | 112 | 95.45 | 95.45 | C: `lea` base/index order of `missionsTable[i]` (effective match) |
| `00416d70` | front_get_selected_level | 96 | 96.67 | 96.67 | D: the original jumps straight to `ret` with `eax` known to be zero |
| `00417df0` | sav_refresh_slots_target | 96 | 96.97 | 96.97 | B: the loop's end pointer `&frontGlobs_saveData[6]`; the rebuilt `<common>` puts `steeringTuning` there |
| `00414ce0` | front_is_intros_enabled | 80 | 95.00 | **100.00** | `FRONT_PROGRAMMER_LEVEL < 3` (unsigned) |
| `00417f40` | save_write_record_target | 80 | 96.43 | 96.43 | C: as `00417eb0` (effective match) |
| **80–94%** | | | | | |
| `00417430` | float_to_long_x87 | 1408 | 90.91 | 90.91 | C: as `00417eb0` (effective match) |
| `00410740` | front_menu_item_add_select_item | 720 | 80.74 | **100.00** | `realloc` branch first |
| `00411360` | front_menu_item_select_test_string_collision | 400 | 86.14 | 86.14 | C: `scrollStart + i` and the coordinate sums in other registers; `i + scrollStart` unchanged |
| `00417090` | front_level_set_load_level_links | 304 | 90.32 | 90.32 | C: `levelName` and `index` in swapped registers; the `setIndex` store moves with them |
| `00410e20` | front_menu_item_create_image_item | 272 | 92.00 | **100.00** | struct-order stores |
| `00410c70` | front_menu_item_free_menu_item | 224 | 82.61 | 82.61 | D: the original keeps four identical `optional_resource_destroy` cases apart; MSVC merges two of ours (case order tried) |
| `004103f0` | front_rock_wipe_play (asm) | 80 | 89.47 | 89.47 | B: needs `frontGlobs` as one struct (below) |
| `00414070` | front_menu_set_create | 80 | 84.06 | **100.00** | `memset` of the set |
| `00413b50` | front_menu_free_slot_images | 48 | 92.31 | 92.31 | B: the end pointer `0x4db8a4` has no symbol in the original; the rebuilt `.bss` puts `s_cacheList` there |
| `00417f90` | sav_record_ptr_target | 48 | 94.44 | 94.44 | C: as `00417eb0`; `a[i]`, `a + i` and `i + a` give one code (effective match) |
| **Below 80%** | | | | | |
| `00412c00` | front_menu_update | 3440 | 46.06 | 46.06 | D: classified from the diff |
| `004140c0` | front_load_menu_set | 3024 | 69.79 | 69.79 | D: stack slots and `esi`/`edi` swapped throughout; the rebuilt code is 49 instructions longer |
| `00411a00` | front_menu_update_menu_items_input | 1280 | 77.40 | 77.40 | D: classified from the diff |
| `004121b0` | front_menu_update_overlays | 672 | 75.93 | 75.93 | D: `index > 0` gives the original's `jbe` (kept); MSVC still cross-jumps the two `flic_setup` tails and keeps `1` in `ebx` |
| `00413b80` | front_run_screen_menu | 672 | 77.72 | 77.72 | D: classified from the diff |
| `00412750` | front_menu_item_draw_select_text_window | 640 | 60.62 | 62.32 | D: overwrite branch first (permuter); stack slots |
| `00411530` | front_menu_get_item_bounds | 608 | 59.50 | 59.50 | C: `eax`/`ecx` swapped through the default case |
| `00411f10` | front_menu_item_draw_slider | 608 | 47.67 | 47.67 | E: x87 copies into the parameter slots; a zero register |
| `004124f0` | front_menu_item_draw_save_image | 608 | 45.73 | 79.43 | `str` local, unsigned sizes; D: frame `0x34` against `0x20` |
| `00411100` | front_menu_create_menu | 352 | 73.53 | 73.53 | D: as `00416d70`, three inline returns with a known zero |
| `00411870` | front_menu_item_slider_handle_input | 352 | 46.49 | 89.92 | the clamp (section 5); R: original at counter +31 |
| `004129d0` | front_menu_draw_load_save_text | 288 | 78.92 | 78.92 | D: store scheduling and registers |
| `00412af0` | front_menu_draw_menu_image | 272 | 70.91 | 70.91 | D: the dark branch's tail is shared the other way; `!light` first and `if`/`else` for `return` change nothing |
| `00413a60` | front_menu_load_slot_images | 240 | 72.19 | 97.59 | block-scoped frame, `= { 0 }`, `k = 8`; B: two end pointers |
| `00414d30` | front_callback_slider_brightness | 176 | 77.11 | 77.11 | E: MSVC reuses `spot` for the last argument; the original computes it again |
| `00412450` | front_menu_item_draw_select_item | 160 | 69.81 | **100.00** | no `selItem` local; `dest.x` first |
| **Part 1's asm** | | | | | |
| `00410a10` | front_menu_item_create_cycle | 144 | 100.00 | 100.00 | now C (section 1) |
| `00410d50` | front_menu_item_create_banner_item | 208 | 100.00 | 100.00 | now C |
| `00413f90` | front_menu_create_overlay | 128 | 100.00 | 100.00 | now C |

## Why the other 31 are short

| Cause | Functions | Ledger bytes | What would close it |
| --- | ---: | ---: | --- |
| **A. Callee outside the unit**: ObjectRecall.c `00459360` and `004593e0` (`00415220`, `00417bd0`), the `_vsprintf` annotation (`00410320`) | 3 | 880 | GAME-030's conversion; a `LIBRARY` annotation for `0048e1e0` |
| **B. Data layout**: `frontGlobs` as separate globals (`004103f0`, `00417df0`), FrontEnd.c `.bss` end pointers (`00413b50`, `00413a60`) | 4 | 464 | `frontGlobs` as one struct; layout identity for the unit's `.bss` |
| **R. Rule B**: `00411870`, the original at +31 | 1 | 352 | the unit's real declaration set |
| **C. Register choice or integer operand order** | 7 | 2,960 | not found; phase-insensitive where swept |
| **D. Scheduling, stack slots, block layout or statement shape** | 13 | 11,712 | not found |
| **E. x87 scheduling and float copies** | 3 | 1,120 | not found |

`00417430`'s 1,408 is the ledger gap. The function is 39 bytes.

### `frontGlobs` as one struct

`front_rock_wipe_play`'s C form gives `or al, 3`, and the original keeps
`or al, 1; or al, 2`. Finding 623 tried every statement order. As a test, the
four rock-wipe fields went into one struct (`anim`, `flags`, `timer`,
`last`). The C form then gives the original's instructions exactly: the
timer store into the same object stops MSVC from folding the two ORs. Only
the operand names differ. The test is not committed: the struct would have
to be the real `frontGlobs` (`00556fc0`, about 0x890 bytes), with every
part-1 and part-2 field and Reward.c's and Objective.c's readers. That
would also fix `00417df0`'s end pointer. It is the largest lever left in
the unit, and it belongs in its own issue.

### Inline returns with a known zero

`00416d70` and `00411100` share one pattern. After `test reg, reg; je`,
the original returns with the register it knows is zero. It jumps straight
to `ret`, or emits `pop; ret` without `xor eax, eax`. Its epilogues are
therefore not identical, and MSVC doesn't merge them. Our build loads
`xor eax, eax` and merges every `return NULL` into one exit. `return 0`
in place of `NULL` and nesting with `else` change nothing, and the
counter sweep shows one code. This needs more investigation.

## Files

- `src/game/frontend_unit.c`: the shapes above, the slider correction, the
  three asm functions as C, `FRONT_PROGRAMMER_LEVEL` moved up, and
  `frontGlobs_saveImageBigWidth`/`Height` as `unsigned int`.
- `src/game/objective_unit_imports.h`: the two externs' type (declaration
  only; no other thread owns this file).
- `docs/coverage/match-baseline.txt`, `match-listing-digests.txt`,
  `binary-progress.json`: regenerated by `make match-progress`.
