# Finding 552 — Second reshape pass: four `#if`-blocked units relocated; G98-010, G98-012 and G98-015 left

Issue #525, part of #490 and ADR 0006 (addendum: reshape commits). The
first pass was #502 → PR #507 ([finding 542](542-zero-drift-reshapes-b1-b2.md)).
The base is `main` at `b5d7be57`, after #517
([finding 548](548-remaining-relocation-refusals.md)).

## Summary

- **Relocated:** G98-014 (Files.c), GAME-020 (Interface.c), G98-011
  (Images.c) and GAME-017 (FrontEnd.c), in that order (smallest first). That
  is 249 functions. Each split and each relocation is its own generated
  commit. Each fix-up is separate.
- **Reshapes:** three zero-drift commits, one for `resource.cpp` and two for
  `frontend.cpp`. Each was proved with `tools/reshape_check.py` against its
  parent (outputs below).
- **Fixes and fix-ups:** three hand edits, none of them a reshape:
  - one source fix, a struct tag. Its tokens differ, its object is identical;
  - two fix-ups of generated output. One is a header declaration; the other
    is whitespace, token- and object-identical.
- **Tool commits:** seven small ones, each with a fixture test
  (`make tools-test`: 108 relocation tests, up from 101). Six fix defects
  these relocations found; one adds a rule (annotated datum groups).
- **Still blocked by `#if` blocks:**
  - G98-010 and G98-012: both plan cleanly once their backend region is
    resolved (a probe, below). The region split is the follow-up.
  - G98-015: out of scope, because the reshape is inside `view.cpp`.
- **Gate:**
  - No exact match is lost (614 game-only, 814 full, unchanged). Two
    functions are newly mapped.
  - Five scores fall and three rise, and every change reproduces in a
    single-TU compile.
  - Function-order pair inversions fall from **165,240 to 77,707**.

## Survey before and after

`python3 tools/relocate_unit.py --survey`. Before is on `b5d7be57`; after is
on this branch.

| Unit | Before | After |
| --- | --- | --- |
| GAME-017 FrontEnd.c | `#if` block ×27, macro redefined or `#undef`'d ×14 | **relocated** → `game/frontend_unit.cpp` (123 functions) |
| GAME-020 Interface.c | `#if` block ×2, macro ×2, needs `--split-declarations` ×1 | **relocated** → `game/interface.cpp` (73 functions) |
| G98-011 Images.c | `#if` block ×6 | **relocated** → `gods98/images_unit.cpp` (19 functions) |
| G98-014 Files.c | `#if` block ×1 | **relocated** → `gods98/files.cpp` (34 functions) |
| G98-010 | `#if` block ×29 | unchanged: `surface.cpp` backend region (P5) |
| G98-012 | `#if` block ×10 | unchanged: `movie.cpp` backend region |
| G98-015 | `#if` block ×1 | unchanged: `view.cpp:3122` (out of scope) |
| GAME-002, GAME-027 | name clash (source fix) | unchanged (out of scope) |
| GAME-023, G98-002 | namespace nested in itself (source fix) | unchanged (#521) |
| G98-005 Main.c | backend-only source ×2 | unchanged |

Totals: 12 → 8 units left. Units blocked by an `#if` block: 7 → 3.

## Per unit

### G98-014 (Files.c) → `gods98/files.cpp`

- **Blocker:** `resource.cpp:7`'s include group held a nested
  `#ifndef INVALID_FILE_ATTRIBUTES`.
- **Reshape:** the nested define moves into its own group,
  `#if defined(_WIN32) && !defined(INVALID_FILE_ATTRIBUTES)`, after the
  include group. This closes and reopens with an equivalent condition, and
  the `windows.h` include still precedes the test.
- **Tool defect:** the relocation dropped the platform include group
  (`<windows.h>` / `<dirent.h>`). The moved directory scan needs `DIR` and
  `getcwd`, so the native build failed. Fixed in
  "copy include-only `#if` groups into the unit file".
- **Relocation:** 34 functions, 14 owned globals and 19 closure items.

### GAME-020 (Interface.c) → `game/interface.cpp`

- **Blocker 1:** `frontend.cpp:63`, a P6 group. Its winapi branch held the
  `#pragma data_seg` padding and `MENU_ACTION_TABLE_ALLOC`.
  - *Reshape:* the padding closes its group, and the macro alternatives
    reopen under the same condition.
  - *Effect:* the padding stays in `frontend.cpp`, so the plan's worry about
    P6 (the padding moving to another object) does not arise.
- **Blocker 2:** `resource.cpp`'s
  `static struct { … } s_handle_set_band[24];` (0x004ddc04) needs a split,
  and the type had no tag.
  - *Fix commit:* add the tag `ResourceHandleSetBandSlot`. The tokens differ,
    so this is not a reshape. `reshape_check` reports the object identical.
  - *Split:* `relocate_unit.py GAME-020 --split-declarations --check-layout`
    (layout unchanged: 463 data symbols, identical section sizes).
- **Tool defects:**
  - The split left the variable's `GLOBAL` above the `struct T {…};` line,
    and the relocation then refused: "annotates a type". It now moves onto
    the first variable. The first split commit was regenerated after this
    fix.
  - The shared label table `s_interface_menu_action_labels[]` was declared
    `extern … [];`, an incomplete type, so the unit's `sizeof` failed. The
    bound now comes from the brace initializer (`[74]`).
- **Fix-up:** `frontend_for_interface.hpp` declares the label table without
  `MENU_ACTION_TABLE_ALLOC`. MSVC 5.0 rejects `__declspec(allocate(…))` on an
  `extern` declaration whose segment is not yet declared (C2341). The tool
  copies the specifier verbatim; a follow-up could strip `allocate` from
  generated declarations.
- **Relocation:** 73 functions, 43 owned globals and 38 closure items.

### G98-011 (Images.c) → `gods98/images_unit.cpp`

- **Blocker:** six region types (`SurfaceLockFn`, `SurfaceDdrawDesc2`, …)
  in `surface.cpp`'s backend region. No G98-011 code uses them: the helper
  `font_device_pixel_format_query` declares its own block-scope
  `SurfaceLockFn` and `SurfaceUnlockFn`.
- **Tool defect:** the scanner counted block-scope `typedef` names as uses
  of the file-scope names, which pulled the region in. Fixed in
  "a block-scope typedef is not a use of the file-scope name". No reshape
  was needed.
- **Relocation:** 19 functions, 2 owned globals and 7 closure items. The stem
  is `images_unit` because `gods98/images.h` exists.

### GAME-017 (FrontEnd.c) → `game/frontend_unit.cpp`

- **Blocker:** all 27 `#if` entries and the 14 macro entries were one group,
  `frontend.cpp:154`. A scratch probe, with only that group resolved to its
  winapi branch, planned cleanly.
- **Reshape:** the group is split into:
  1. the `#pragma data_seg` padding group;
  2. the `DAT_0055613c`/`…6f50`/`…6f54` trio, made unconditional. Both
     branches declared them with identical tokens, and in the same place
     relative to the rest of each branch;
  3. `#define s_fe_flow`, winapi-only;
  4. one single-branch group per annotated flow word (`s_fe_flow_quit`, …),
     with the annotation on the declaration;
  5. the `#else` array, as `#if !defined(LEGORRR_BACKEND_WINAPI)`;
  6. the `FE_FLOW_*` macro alternatives, which are header-safe.
- **Tool changes:**
  - *New rule:* a single-branch group around exactly one `GLOBAL`-annotated
    variable moves whole, as that global.
  - *Defect:* the winapi `#define s_fe_flow` and the non-winapi
    `int s_fe_flow[5]` share a name and form one cluster. Only the
    cluster's first `#if` group (the macro) was declared, so the array lost
    `static` without a declaration. Every group that defines something is
    now declared.
  - *Defect:* GAME-017 owns `s_map_open_binary_mode` (0x004a2770), a
    `const` object in `level.cpp`. G98-014's frozen `level_for_files.hpp`
    declares it for `files.cpp`. The unit file dropped that header (it
    defines the name rather than using it), the definition got internal
    linkage, and the native link failed. The unit now keeps an earlier
    private header that declares a `const` object it defines.
- **Fix-up:** `frontend_enabled` (0x00416d00) has a brace line
  `{    if (cfg == NULL) {`. Reccmp's parser then skips the next
  annotation:
  - in `frontend.cpp` that was `0x00410740`, unmapped on `main`;
  - after the move it was `save_slot_open` (`0x00416d50`, 100%), which the
    first gate run reported as *newly unmapped*: a lost exact match.

  Putting the brace on its own line is token- and object-identical. All
  three functions now map. GAME-017.csv's body hash for `0x00416d00`
  records the text before this fix-up.
- **Relocation:** 123 functions, 16 owned globals and 41 closure items, from
  18 legacy files. The stem is `frontend_unit`.

## A blind spot in `reshape_check`

Before the final GAME-017 reshape, I tried putting each flow word's
annotation *above* its `#if`. The reccmp lint accepts that, and
`reshape_check` reported all checks identical. But Reccmp reads the line
after `// GLOBAL:` as the declaration. It named `0x0055701c` `defined`,
and `match_function.py` scored `0x00416dd0` 50% instead of 100%.

`reshape_check` cannot see annotation attachment, because annotations are
comments. A reshape that moves annotations should also run
`match_function.py` on the functions that use them, as this one did. A
follow-up could add an annotation-attachment check to `reshape_check`,
using Reccmp's parser on both versions.

## `reshape_check` output per commit

Each check compares the commit with its parent
(`python3 tools/reshape_check.py <file>`, in the `developer` distrobox).

`cbb92968` Reshape `resource.cpp` (G98-014):

    src/gods98/resource.cpp:
      includes: identical
      native: identical
      matching: identical
      mingw: identical (emulated: host gcc + reference defines)
      object: identical (including .drectve when present)

`a915085e` Reshape `frontend.cpp` (GAME-020):

    src/game/frontend.cpp:
      includes: identical
      native: identical
      matching: identical
      mingw: identical (emulated: host gcc + reference defines)
      object: identical (including .drectve when present)

`7f969077` Reshape `frontend.cpp` (GAME-017). The flow-word users
`0x00416d50`/`d60`/`dd0`/`de0`/`df0` keep 100%, and `0x004103d0` keeps
88.89% (`match_function.py`):

    src/game/frontend.cpp:
      includes: identical
      native: identical
      matching: identical
      mingw: identical (emulated: host gcc + reference defines)
      object: identical (including .drectve when present)

`18b419a1` is a source fix, not a reshape: the tag on `resource.cpp`'s band
struct.

    src/gods98/resource.cpp:
      includes: identical
      native: different at token 14050: ['{'] -> ['ResourceHandleSetBandSlot']
      matching: different at token 13773: ['{'] -> ['ResourceHandleSetBandSlot']
      mingw: different at token 13820: ['{'] -> ['ResourceHandleSetBandSlot'] (emulated: host gcc + reference defines)
      object: identical (including .drectve when present)

`fbea9ab4` is a fix-up: `frontend_unit.cpp`'s brace line.

    src/game/frontend_unit.cpp:
      includes: identical
      native: identical
      matching: identical
      mingw: identical (emulated: host gcc + reference defines)
      object: identical (including .drectve when present)

The split `e503d794` is generated; `--check-layout` reports the layout of
`src/gods98/resource.cpp` unchanged (463 data symbols, identical section
sizes).

## Commits

| Kind | Commits |
| --- | --- |
| Reshape (zero drift) | `cbb92968` resource.cpp; `a915085e`, `7f969077` frontend.cpp |
| Source fix | `18b419a1` band struct tag (object identical) |
| Tool | `88237a2c` include-only groups; `72dae427` split annotation; `0e6117c3` array bound; `afe52d8b` block-scope typedefs; `e77eb0a8` annotated datum groups; `713cbedd` every defining group; `81038dd7` const object's header |
| Split (generated) | `e503d794` GAME-020 |
| Relocation (generated) | `a4f01da8` G98-014; `bfa0c46f` GAME-020; `400f31a1` G98-011; `f8ef400f` GAME-017 |
| Fix-up | `56889ca3` `frontend_for_interface.hpp`; `fbea9ab4` `frontend_unit.cpp` |
| Measurement | `make match-progress` and the binary-progress input manifest |

## Gate (ADR 0006 rule 2)

| Check | Result |
| --- | --- |
| Native build + `ctest` | 37/37 |
| `make tools-test` | OK. The lint ratchet is unchanged at 4,122 entries |
| `link_order.py check` / `check-map`, Meson parity | OK / OK / OK |
| `make reccmp-check` (distrobox) | 5 falls, 3 rises, 2 newly mapped, **no exact match lost** (614 / 814) |
| Means | game-only 52.525 → 52.485% (2,164 → 2,166 mapped); full 57.119 → 57.079% (2,400 → 2,402). Over functions present in both baselines: 52.5253 → 52.5240% and 57.1195 → 57.1184%. The plain means fall mostly because the two newly mapped functions score about 10% |
| `make match-progress` | function-order inversions: 430 → **326** adjacent, 165,240 → **77,707** pairs (game-only 149,803 → 62,635). Unit contiguity 88.13 → 89.04% (193 → 195 units). Unit-order inversion pairs 3,155 → 3,080. Link-order entries 107 → 103. Identical `.text` at original addresses 20,551 → 20,396 bytes; identical `.data` 10,791 → 13,462 bytes |
| `make unit-data-offsets` | full 0, gaps-only 2, partial 24, n/a 199 |

`binary_progress.py` needs `pefile`, which the distrobox's Python lacks, so
`binary-progress-compare` ran with the host's `python3`. The other
`match-progress` steps ran in the distrobox.

Each changed score was compiled as a single TU on `b5d7be57` and on this
branch with `match_function.py`. Every base score equals the baseline, and
every head score equals the gate's:

| Function | File (here) | Score | Recomp size | Label | Cause |
| --- | --- | ---: | ---: | --- | --- |
| `0x0041c4f0` advisor_animation_position_and_request_update_internal | `interface.cpp` (moved from `objective.cpp`) | 13.23 → **13.10** | 778 → 792 (+4 instructions) | `source-changed` | the recorded `static` removal: `objective.cpp` still calls it |
| `0x00444fb0` ledger_stub_00444fb0_EntityTurnToVectorActivityIssue | `entity.cpp` | 26.04 → **25.56** | 606 → 604 (−1) | `drift-review` | compile drift (`entity.cpp` lost GAME-017/020 items). It returns to its score before #517, where finding 548 measured 25.56 → 26.04 |
| `0x0047d800` surface_bitmap24_bit_surface_convert | `images_unit.cpp` (moved) | 29.34 → **29.17** | 462 → 463 (+1) | `drift-review` | compile drift after the move |
| `0x00482620` mesh_segment_vertex_fill | `record.cpp` | 31.30 → **26.09** | = | `drift` | register re-roll. It returns to its score before #517 (finding 548: 26.09 → 31.30) |
| `0x00483420` screen_effect_item_opacity_set_target | `record.cpp` | 36.16 → **30.51** | = | `drift-reordered` | register choice. It returns to its score before #517 (finding 548: 30.51 → 36.16) |
| `0x00417f90` sav_record_ptr_target | `frontend_unit.cpp` (moved from `savegame.cpp`) | 70.27 → 75.68 | = | — | compile drift |
| `0x0041a850` resource_handle_set_release | `interface.cpp` (moved) | 35.09 → 38.60 | = | — | compile drift |
| `0x00444680` entity_step_movement_vector_resolve | `entity.cpp` | 4.88 → 4.89 | 885 → 883 | — | compile drift |
| `0x00410740` fe_save_menu_list_entry_append | `frontend_unit.cpp` | unmapped → 10.07 | — | — | the brace fix-up (above) |
| `0x00416d00` frontend_enabled | `frontend_unit.cpp` | unmapped → 10.00 | — | — | the brace fix-up (above) |

**Flagged for the owner (rule 2):**
- the instruction-count changes: `0x0041c4f0` (+4, the `static` removal),
  `0x00444fb0` (−1) and `0x0047d800` (+1);
- all five falls. Three of them (`0x00444fb0`, `0x00482620`,
  `0x00483420`) undo #517's rises exactly, so these listings flip between
  two states as `entity.cpp`'s and `record.cpp`'s declaration sequences
  change.

## What is left

### G98-012 and G98-010: backend regions (P5, B3)

A scratch probe resolved each region to the matching build's branch
(`/tmp`-only; it removes native code). Then:

- **G98-012** plans cleanly with 25 functions. Its `movie.cpp:92`–`1469`
  region (FFmpeg / VFW / disabled) holds no annotated item, but the unit
  uses two of its names:
  - `movie_avi_file_initialize`: a `static` defined once in each of the
    three branches, with one signature;
  - `Movie`: defined differently in two branches. The unit uses it only by
    pointer, and `movie.hpp` already declares `typedef struct Movie Movie;`.

  Neither is a reshape. The tool needs two features:
  - **(a)** a `static` defined with one head in every branch of a group,
    including `#else`, is exported with one unguarded declaration;
  - **(b)** a type that an included header already declares resolves to
    that declaration when the file's own definitions sit in an unmergeable
    group. The compile catches a complete-type use.
- **G98-010** plans cleanly with 15 functions once `surface.cpp:3540`'s
  miniwin/winapi/else region and its nested `#if _WIN32 && !_WIN64` are
  resolved. Its annotated functions sit inside the nested winapi group.
  The plan's B3 would split them into top-level single-branch groups, for
  example
  `#if !defined(LEGORRR_BACKEND_MINIWIN) && defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)`.

  That needs the function analogue of this PR's annotated-datum rule:
  - an annotated function in a single-branch group, with the annotation on
    the definition;
  - guarded declarations for the closure (`WinapiRendererState`, the COM
    helpers) that remaining port code shares.

  The region is about 1,100 lines. This is the next ticket.

### G98-015

`view.cpp:3122`'s group (`LEGORRR_BACKEND_WINAPI && _WIN32 && !_WIN64`)
holds `#include "lwsc_winapi.hpp"`, the annotated
`scene_runtime_user_visual_manager` (`0x004827f0`) and port helpers. The fix
would hoist the annotated function into its own group, a reshape inside
`view.cpp`, which is out of scope for #525 (#521 owns `view.cpp`'s
namespace fix). With the rule above for annotated functions in
single-branch groups, it would then relocate.

## Reproducing

    python3 tools/relocate_unit.py --survey
    python3 tools/reshape_check.py src/game/frontend.cpp --base 7f969077~1 --head 7f969077
    python3 tools/match_function.py 0x00416d50
    make reccmp-check && make match-progress && make unit-data-offsets
