# Finding 672 — FrontEnd.c and Interface.c big functions: where the source shape still differed

Issue #797, part of #388 and ADR 0005. Follows finding 660 (which left all
four of its functions at a register or cross-jump choice) and finding 664
(the phase map). Base: `main` `1f5d8a1e`, measured with one full
`make reccmp-check` before and one after.

## Summary

Five of the seven functions looked at had a real source-shape difference
left, not only a register choice. Each fix is a form the original's code
shows directly (an argument computed before a call, a value kept in a
register across a store, a variable cleared on one path):

| Address | Function | Size | `main` | After | Size agrees |
| --- | --- | ---: | ---: | ---: | --- |
| `0041baa0` | interface_do_mouse | 1,834 | 76.56 | **94.76** | yes (was 10 bytes long) |
| `00411530` | front_menu_get_item_bounds | 608 | 59.92 | **86.85** | yes (was 8 bytes long) |
| `00411870` | front_menu_item_slider_handle_input | 337 | 90.70 | **96.90** | yes (phase only) |
| `00412c00` | front_menu_update | 3,500 | 49.24 | **59.59** | no: 0x24 bytes long (was 0x30) |
| `004140c0` | front_load_menu_set | 3,104 | 88.14 | **94.07** | yes (as before) |
| `0041dca0` | interface_do_action | 2,608 | 87.34 | 87.34 | yes |
| `0041d040` | interface_is_item_available | 3,160 | 94.92 | 94.92 | yes |

Scores are the full report's. The single-function loop gives 2–3 points
less for these functions (finding 660: callee names only); there the five
went 74.57 → 92.73, 57.85 → 85.59, 90.70 → 96.90, 46.64 → 56.27 and
88.04 → 93.97.

`0041dca0` and `0041d040` are unchanged; the experiments are below.
No function is lower than the baseline.

| Measure | `main` `1f5d8a1e` | This change |
| --- | ---: | ---: |
| Exact-function code bytes | 338,194 (52.63%) | 338,194 (52.63%) |
| Functions that rose / fell | | 5 / **0** |
| Reccmp accuracy, full / game-only | 97.10% / 96.76% | 97.12% / 96.79% |
| `.text` identical at original addresses | 34,303 (5.34%) | 34,304 (5.34%) |
| Image end (`make pe-audit`) | `0x7f5000` | `0x7f5000` |
| Function-order inversions, full / game-only | 222 / 1 pairs | 222 / 1 pairs |

None of the functions became exact, so exact-function bytes do not move.
FrontEnd.c's code is 0x10 bytes shorter and Interface.c's 0x20, so every
later object starts up to 0x30 bytes earlier in the gate link; no function
changes score from that. `make match-progress`, `make unit-data-offsets`
and `make pe-audit` were run and the generated files are committed.
`make tools-test` (no `--allow-growth`), `make winapi-link-check` and
native ctest (37 of 37) pass, and the native build starts with the shipped
WADs.

Only `src/game/frontend_unit.c` and `src/game/interface_unit.c` change
(issue #797's ownership). No header changes. FrontEnd.c loses two locals
(`plusLo`, `focusItem`) and gains one (`image`); Interface.c's counts are
unchanged. The symbol-counter effect on later functions was checked
function by function in the single-function loop for both units before
the full build (below).

## `0041baa0` interface_do_mouse

Three differences, all visible in the original's code:

1. **`tooltip` is cleared on the menu-item path only.** The original
   zeroes `ebx` and `ebp` at the top (`text` in `ebp`, the zero for
   `result` and `iconCount` in `ebx`), but calls
   `Interface_IsItemDisabled(type, 0)` with `push 0` and tests the disabled
   tooltip with `mov eax, [..]; test eax, eax`. Neither register is used
   as a known zero there. With `tooltip = NULL` at the top, MSVC keeps
   `tooltip` in `ebp`, reuses it as the zero (`push ebp`,
   `cmp [..], ebp`), and has no register left for `text`, which then
   shares a stack slot with `argc`. Clearing `tooltip` at the start of the
   non-object item branch gives the original's allocation of both: `text`
   in `ebp`, `tooltip` in `ebx`. This alone is 76.67 → 92.73 in the
   single-function loop.
2. **The sound after `Interface_DoAction` is an `if`/`else` of two calls**
   (`test eax, eax; je; push 0; jmp; push 1; call`). `== 0` as the
   argument compiles to `neg; sbb; inc`.
3. **The open/in test wraps the body.** The original's early exits jump to
   the shared `mov eax, [result]` epilogue, so the body is inside
   `if ((flags & OPEN) && (flags & IN)) { … } return result;`. The icon
   loop counts up (`for (i = 0; i < iconCount; i++)`), which MSVC reverses
   to the original's `test esi, esi; jbe`.

Left: `y` is in `ebx` and the image path's `count` in `esi`; the original
swaps them (`y` shares `esi` with the selected object). The original also
zeroes `ebx` once at the top, where ours zeroes it on the item path. `decls` (484 orders) and `stores` searches found nothing more.

## `00411530` front_menu_get_item_bounds (finding 664's lead)

Finding 664 named this function's shape as the one localized gap before
`00411870`. The `font_get_height` / `object_field_be4_dword_get`
difference is only a name: `font_link_names.h` maps one to the other, and
both are `0047a540`. The real differences:

1. **The slider's two image widths are one sum, computed before the
   bounds.** The original evaluates both ternaries into `esi` and `edi` and
   adds them with `lea edi, [edi + esi + 0x68]` before `*rcX`. One
   `capRight = (…) + (…) + 104;` does that; a second variable is not
   needed (`plusLo` is gone).
2. **The two heights are ternaries.** For the image item and the text item
   the original loads `menu->items[index]` once, before the branch, then
   reads `imageHi`/`imageLo` (or `fontHi`/`fontLo`) in each arm. An
   `if`/`else` of two assignments reloads it in each arm.

Left: the default branch loads `menu->xPos` first and our build `item->x1`
first. No operand order changes this (all six orders give identical code).

### `00411870` re-probed

After the rewrite `00411870` front_menu_item_slider_handle_input rises
from 90.70 to 96.90 in the single-function loop (phase only: its code is
unchanged in source). `make phase-probe` before its line (N = 0–64) now
puts 100 at +51 to +59, still a header-sized gap, so the missing
declarations are not in `00411530`.

The two ternaries move the symbol-counter phase of every later function,
although they declare nothing: either one alone takes `004177e0` (100%,
period 8) to 69.33. So control-flow rewrites, not only declarations, cost
counter numbers. Dropping `plusLo` restores `004177e0`, and
`front_menu_update`'s `image` local (+1) is paid for by dropping its
`focusItem` (below), so no later FrontEnd.c function moves.

## `004140c0` front_load_menu_set

**The item data is created inside the banner call.** For the Slider,
Trigger (5 parts) and RealSlider items the original pushes the banner's
trailing `0` before it calls `Front_MenuItem_Create*`, and pushes the
result directly (`push eax; push ebp; push type`). That is
`front_menu_add_menu_item(menu, front_menu_item_create_banner_item(…,
autoCenter, front_menu_item_create_slider(…), 0))`, not an `itemData`
statement. With that, finding 660's `args`/`menuSet` swap (`edi`/`esi`)
disappears: 88.04 → 93.97 in the single-function loop, at the same size.

Left: only stack-slot offsets of the first loop's locals (`menuTitle`,
`xPos`, `yPos`, `menuFont`, `centerOffset`, `displayTitle`, `anchored`)
differ. A `decls` search (729 orders) does not move them, so the slot
order follows use weight (finding 581), not declaration order.

## `00412c00` front_menu_update

1. **The title's position is computed before the second
   `Front_Util_ReplaceTextSpaces` call.** The original computes
   `s_display_w / 2 - (width >> 1)` into `esi` and `anchoredY + y` into
   `edi` before it calls ReplaceTextSpaces for the print, so both are
   statements (here into `itemX`/`itemY`).
2. **The image items keep the image pointer in a register.** The original
   tests and pushes the same register (`cmp eax, ebx; … push eax`); a
   field read reloads after the `dest` stores. An `image` local does that.
   It costs one counter number, so `focusItem` (only ever
   `menu->items[menu->itemFocus]`, which the switch keeps in `eax` either
   way) is written out instead; `004177e0` stays exact.
3. **`item` is declared first** (`decls` search, 46.64 → 50.40 before 1
   and 2).

Tried, not kept: the unfocused branch without its second `item =
menu->items[i]` (switch and item data read through `menu->items[i]`)
gives the original's size exactly (`0xd70`), but scores 53.91 against
56.27 (54.13 after a `decls` search), because `item` then takes `ebp`,
where the original keeps `y`. The original also has a second `Point2F`
for the unfocused image (`[esp+0x18]` against the focused one at
`[esp+0x34]`); block-scoped `Point2F` locals reproduce that but lower the
score (50.67).

The function is still register-bound: the original keeps `x` in `ebx`
and `y` in `ebp` through the item loop, the view in `esi` up to the rock
wipe, and `-1` in `edi`; ours keeps a zero in `esi` and `oldCamera` in
`ebp` and spills `x`.

## `0041dca0` interface_do_action and `0041d040` interface_is_item_available

No change. In `0041dca0`, cases 20, 21, 28 and 27 read the flag byte into
`al` first (`mov al, [flags]; mov ebx, mask; test bl, al`) while 29 and 30
use `test byte ptr [flags], bl`, so the original's source for the first
four differs from the last two. Every operand and cast form tried
(`s & mask`, `(unsigned char)(s & mask)`, `mask & s`, `!= 0`, byte
pointer, `s & (mask = N)`, 7 + 3 forms) compiles to the same code, which
all six cases then share. Case 0's 16/17/18 bodies stay merged into cases
17 and 18 (finding 660, rule 3).

`0041d040`'s one difference (which `return 0` keeps its own epilogue) is
finding 660's; nothing new was tried beyond the tail shapes above.

## Files

- `src/game/interface_unit.c`: `0041baa0`'s three shapes.
- `src/game/frontend_unit.c`: `00411530`, `00412c00` and `004140c0`.
- `docs/coverage/match-baseline.txt`, `match-listing-digests.txt` and
  `binary-progress.json`: regenerated.
