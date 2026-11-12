# Finding 557 — Backend region splits: G98-012 and G98-010 relocated

Issue #525, part of #490 and ADR 0006 (addendum: reshape commits). This
continues [finding 552](552-second-reshape-pass-if-blocked-units.md), which
left G98-012 and G98-010 behind their backend regions. This PR is stacked on
#541 (GAME-027 and GAME-002, [finding 559](559-relocation-game-027-game-002-static-renames.md)) at `7fba4d1e`,
which contains #537. Every metric below is this PR's delta on top of #541.

## Summary

- **Relocated:**
  - G98-012 (the AVI player) to `gods98/avi_player.cpp`: 25 functions;
  - G98-010 (DirectDraw) to `gods98/directdraw.cpp`: 15 functions.

  Each relocation is one generated commit. No split and no fix-up was
  needed.
- **Reshapes:** two zero-drift commits, one per file (`movie.cpp`,
  `surface.cpp`). Each closes and reopens `#if` groups with equivalent
  conditions. `tools/reshape_check.py` proves both (outputs below).
- **Tool commits:** two small ones, each with fixture tests
  (`make tools-test`: 112 relocation tests, up from 108):
  - finding 552's feature (b): a type that an included header declares
    binds to that declaration;
  - a guarded declaration for a function alone in a single-branch group.

  Finding 552's feature (a) was not needed. The reshape makes
  `movie_avi_file_initialize` one whole-alternatives item, which the tool
  already declares once, unguarded.
- **Still blocked by an `#if` block:** only G98-015 (`view.cpp`, out of
  scope).
- **Gate:**
  - No function fell below the baseline, so no exact match is lost.
  - One score rose, and no score fell.
  - Function-order pair inversions fall from **33,212 to 32,339**.

## Survey before and after

`python3 tools/relocate_unit.py --survey`. Before is on #541's head
`7fba4d1e`; after is on this branch.

| Unit | Before | After |
| --- | --- | --- |
| G98-012 | `#if` block ×10 | **relocated** → `gods98/avi_player.cpp` (25 functions) |
| G98-010 | `#if` block ×29 | **relocated** → `gods98/directdraw.cpp` (15 functions) |
| G98-015 | `#if` block ×1 | unchanged: `view.cpp:3122` (out of scope) |
| G98-002 | ok (102 functions) | unchanged (#521 owns it) |
| G98-005 Main.c | backend-only source ×2 | unchanged |

Totals: 5 → 3 units left. Units blocked by an `#if` block: 3 → 1.

## G98-012 (`movie.cpp`)

**Blocker.** The FFmpeg / VFW / disabled region at `movie.cpp:92`–`1469`
was one group holding `#undef MOVIE_VFW_LOAD`, so it could not move whole.
G98-012 needs two of its names:

- `movie_avi_file_initialize`, a `static` defined once in each branch;
- `Movie`, which the unit uses only by pointer.

A scratch probe (dropping only the `#undef`) shows why the group must also
be split. Without the `#undef`, the whole region becomes one cluster, and
the tool tries to export it; it then refuses on `movie_vfw`, a static in
branch 2.

**Reshape (`67a2c78c`).** The chain closes and reopens twice. All three
groups copy the original `#if`/`#elif`/`#else` lines verbatim:

1. each backend up to its `movie_available`;
2. `movie_avi_file_initialize`, as whole alternatives of one function;
3. the rest of each backend, with the `#undef`.

Exactly one branch of each group is active in any configuration, in the
original order. The scanner merges group 2 into one function item. A
remaining caller (VFW's `movie_open_path`) still uses it, so it stays in
`movie.cpp`, loses `static`, and gets one unguarded declaration. That is
correct, because the `#else` defines it in every configuration.

**First attempt, discarded.** I first cut the VFW branch into five
single-branch groups, so that `movie_vfw` and `movie_vfw_bind` could get
guarded declarations. That plan also succeeded. But because
`movie_avi_file_initialize` stays, none of the VFW items is involved. The
three-group shape is smaller and adds no new conditions.

**Tool (`ac0ca28f`).** `Movie` still pulled its in-file definitions and
their FFmpeg types (`ff_frame`, `FFFormatCtx`, …) into the closure.
`movie.hpp` declares `typedef struct Movie Movie;`. Now a use of a type
name binds to the header's declaration when both of these hold:

- an included header declares the name;
- the file defines it only as a type inside `#if` groups.

The definitions then stay where they are. The rule is per name, not per
cluster: after the reshape, the VFW `Movie` sits in group 1's merged
cluster beside functions. A use that needs the complete type would fail the
unit file's compile. Unconditional definitions keep the old rule (tested).

**Relocation (`1f4fca31`):**
- 25 functions, 2 owned globals and 8 closure items, from `movie.cpp`,
  `surface.cpp`, `crt.cpp`, `spawn.cpp` and `record.cpp`;
- `movie.cpp` holds no original function any more and moves to the port
  tail;
- the stem `avi_player` describes the unnamed unit; `movie.cpp` and
  `movie.hpp` exist.

## G98-010 (`surface.cpp`)

**Blocker.** `surface.cpp:3525`–`5236` is miniwin / winapi / else. The unit's
items sit in the winapi branch's nested `#if defined(_WIN32) &&
!defined(_WIN64)` group (`3789`–`4885`), which is plan #490's P5.

**Probe.** A scratch copy resolved the region to the winapi branch. The
dry run then planned 15 functions. I classified each item in the nested
group by what the relocation does with it:

- moves to the unit;
- moves to the private header (types);
- loses `static`, with a declaration (exported);
- stays.

The roles alternate in text order, and the token order must not change.
So the cuts follow the roles: one group per contiguous run.

**Reshape (`da962ab0`, B3):**
- The miniwin branch closes after itself.
- The nested group becomes 18 top-level single-branch groups. Each uses
  the equivalent condition `!defined(LEGORRR_BACKEND_MINIWIN) &&
  defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)`.
- The rest of the winapi branch and the else branch reopen as
  `#if !MINIWIN && WINAPI` / `#elif !MINIWIN`.

| Lines (before) | Items | Role |
| --- | --- | --- |
| `3790`–`3842` | DirectDraw structs, size checks | private header |
| `3844` | `s_iid_direct_draw4` | exported |
| `3848`–`3849` | `s_surface_ddraw_module`/`_create` | stays |
| `3851`–`3877` | COM method typedefs | private header |
| `3879`–`3903` | `surface_com_vtable`/`_release`, `surface_ddraw_api_get` | exported |
| `3905` | `WinapiRendererState` | private header |
| `3921` | `s_winapi_renderer_state` | exported |
| `3922` | `surface_winapi_immediate_device_peek` | stays |
| `3927` | `winapi_renderer_active` | moves |
| `3931`, `3933`, `3935` | `GLOBAL` 0x0076acbc/c0/c4, one group each, annotation inside | exported |
| `3938` | `windowed_surface_blit_impl` prototype | stays |
| `3941` | `winapi_renderer_state_release` | exported |
| `3973` | `0x0047cc50` (annotation inside) | moves |
| `4004` | `0x0047c8d0` (annotation inside) | moves |
| `4019`–`4272` | clipper, RM device, `directdraw_renderer_init_winapi` | moves |
| `4274`–`4884` | present, display mode, startup dialog | stays |

A group is one cluster, so mixing roles would drag items along:
- a type group mixing roles goes whole to the header (harmless);
- an exported or moving group must hold only its own role.

Each annotated function and global gets its own group, because a group
cannot hold an annotated function beside other items.

**Tool (`811b5a6f`).** `winapi_renderer_state_release` is alone in its
group, so the scanner merges it as whole alternatives of one function. Its
declaration was written unguarded. Its parameter type,
`WinapiRendererState`, exists only under the group's condition, so the
native build would have failed. A merged single-branch group now keeps its
`#if` and `#endif` lines, and its declaration takes that condition verbatim,
like a cgroup's. Whole alternatives with an `#else` stay unguarded.

**Relocation (`f8dbef71`):**
- 15 functions and 4 closure items, from `surface.cpp`, `frameloop.cpp`,
  `savegame.cpp` and `crt.cpp`. Two of the unit's functions (`0x0047cc20`
  and `0x0047d000`) are not in `src/`.
- The winapi types go to `surface_for_directdraw.hpp`, verbatim and
  guarded. The shared statics get guarded declarations.
- The annotated globals stay in `surface.cpp`.
- `savegame.cpp` moves to the port tail.

**Annotation attachment.** Finding 552 notes that `reshape_check` cannot see
annotation attachment. Here every annotation sits inside its `#if`, on the
line above its definition. These still score as before in a single-TU
compile:
- `0x0047cc50` (88.64%) and `0x0047c8d0` (100%), which read the three
  globals;
- `0x0047c170` (100%), `0x0047c610` (10.27%), `0x0047c8f0` (11.70%),
  `0x0047cdd0` (27.27%) and `0x0047d3f0` (100%).

The full gate agrees: no function fell.

## `reshape_check` output per commit

Each check compares the commit with its parent
(`python3 tools/reshape_check.py <file> --base <c>~1 --head <c>`, in the
`developer` distrobox). Both were rerun on the rebuilt stack (on #541).

`67a2c78c` Reshape `movie.cpp` (G98-012):

    src/gods98/movie.cpp:
      includes: identical
      native: identical
      matching: identical
      mingw: identical (emulated: host gcc + reference defines)
      object: identical (including .drectve when present)

`da962ab0` Reshape `surface.cpp` (G98-010):

    src/gods98/surface.cpp:
      includes: identical
      native: identical
      matching: identical
      mingw: identical (emulated: host gcc + reference defines)
      object: identical (including .drectve when present)

## Commits

| Kind | Commits |
| --- | --- |
| Reshape (zero drift) | `67a2c78c` movie.cpp; `da962ab0` surface.cpp |
| Tool | `ac0ca28f` header-declared types; `811b5a6f` guarded single-function declarations |
| Relocation (generated) | `1f4fca31` G98-012; `f8dbef71` G98-010 |
| Measurement | `4bc2a643` `make match-progress` and the binary-progress input manifest |

The first versions of these commits were made on `0a90abfa`, then on
`1fadd639` (#537). For the stack, the branch was rebuilt on #541's head
`7fba4d1e`. The reshape and tool commits were cherry-picked without
conflicts and re-proved. The relocations were regenerated with the tool,
not replayed:
- G98-010's patch has the same added and removed lines as before;
- G98-012's differs only in `crt.cpp`'s legacy link-order key
  (`00493c40` → `00493ca0`, where it was `00493c10` → `00493c40`), because
  #541 had already moved code out of `crt.cpp`.

No split or fix-up was needed. `make reccmp-lint-update` rewrote
`reccmp-lint-baseline.txt` with no change (4,117 entries).

## Gate (ADR 0006 rule 2)

| Check | Result |
| --- | --- |
| Native build + `ctest` | 37/37 (the miniwin build compiled the FFmpeg movie and SDL surface code; no movie was played) |
| MSVC 5.0 single-TU spot checks | both unit files compile. G98-012's `0x0047ec80`, `0x0047e800`, `0x0047e510` and `0x0047e7e0` (100%) are unchanged |
| `make tools-test` | OK. The lint ratchet is unchanged from #541 at 4,117 entries, 0 removable |
| `link_order.py check` / `check-map`, Meson parity | OK / OK / OK |
| `make reccmp-check` (distrobox) | no fall, **no exact match lost**; one rise |
| Means | game-only 56.27% (2,179 mapped) and full 60.47% (2,415), unchanged to two places by `match_baseline.py`; Reccmp's accuracy figures move 56.26 → 56.27% and 60.46 → 60.47% |
| `make match-progress` | function-order inversions: 174 → **155** adjacent, 33,212 → **32,339** pairs (game-only 18,152 → 17,285). Unit contiguity 89.50 → 90.41% (196 → 198 units). Unit-order inversion pairs 3,072 → 3,068. Identical `.text` at original addresses 21,250 → 21,349 bytes; identical `.data` 13,420 → 13,374 bytes |
| Listings | `match-listing-digests.txt` changes only its input hash: no normalised listing changed, including every earlier relocated or converted unit's, and `0x0047ec60`'s |
| `make unit-data-offsets` | full 0, gaps-only 2, partial 28, n/a 195 |

As in finding 552, `binary-progress-compare` ran with the host's `python3`,
because the distrobox's Python lacks `pefile`. The other `match-progress`
steps ran in the distrobox.

| Function | File (here) | Score | Recomp size | Cause |
| --- | --- | ---: | --- | --- |
| `0x0047ec60` AviPlaybackState::playback_frame_index_get | `avi_player.cpp` (moved from `movie.cpp`) | 11.11 → 23.53 | = (0x16) | the recorded `static` removal of `s_avi_playback_disabled` (0x00533998): `movie.cpp` still reads it. It reproduces in a single-TU compile (`match_function.py`: 23.53%). Its normalised listing digest is unchanged, so the rise comes from how Reccmp scores the now-external data reference, not from different instructions |

Nothing is flagged for the owner: no score fell, and no instruction count
changed.

## What is left

- **G98-015.** `view.cpp:3122`'s group needs the same treatment: hoist
  `scene_runtime_user_visual_manager` (`0x004827f0`) into its own
  single-branch group, with the annotation inside. That reshape is inside
  `view.cpp`, which is out of scope here. With `811b5a6f`, the tool should
  handle the result.
- **The private header's types.** The winapi DirectDraw ABI types now live
  in `surface_for_directdraw.hpp`. When #389 moves the DirectDraw loader to
  the platform layer, they belong there.

## Reproducing

    python3 tools/relocate_unit.py --survey
    python3 tools/reshape_check.py src/gods98/surface.cpp --base da962ab0~1 --head da962ab0
    python3 tools/relocate_unit.py G98-012 --stem avi_player --dry-run
    python3 tools/match_function.py 0x0047ec60
    make reccmp-check && make match-progress && make unit-data-offsets
