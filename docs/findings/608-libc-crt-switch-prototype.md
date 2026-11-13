# 608. Taking the whole CRT from LIBC.LIB: inventory, prototype and blockers

Issue #447, stage S5 of the [#388 plan](../plans/388-original-link-order.md). It follows [finding 540](540-crt-from-libc-lib.md) (most of the CRT already comes from LIBC) and [finding 574](574-import-directory-and-library-order.md) (`libc.lib` is already listed explicitly, in the settled library order). Measured on `main` at `4862ceaa`.

## Summary

1. **Decision: the full switch is blocked.** Dropping `src/crt/` from the matching link loses exact matches. So this change lands only safe preparatory moves, and #447 stays open.
2. **What `src/crt/` still contributes** to the matching link, which already takes the CRT from `libc.lib` (`kernel32, libc, oldnames, ole32, d3drm, …`):
   - **`crt.obj`:** 112 `.text` symbols and 52 data symbols:
     - 4 annotated CRT stand-ins;
     - 7 annotated LATE-002 functions;
     - 2 annotated Main.c functions;
     - about 100 port accessors;
     - 115 `/include` directives (3 of them for the bridges below).
   - **`crt_heap.obj`:** `engine_calloc` only.
   - **`crt_runtime_aliases.obj` and `crt_runtime_calls.obj`:** six unreferenced `__allmul@16` bridge functions.
   - **`seh_runtime.obj`:** empty.
3. **The prototype**, two scratch relinks of the gate's objects scored with Reccmp:
   - LIBC supplies every CRT routine, and the link succeeds.
   - Three functions are lost: `string_prefix_reserve` (0x004907c0, 100%) and `_matherr` (0x004939d0, 92.31%) become unmapped, and so does `__strupr` (0x0049dcd0, 100%) once the `/include` list goes.
   - The first two are LIBC **statics** (`cvt.obj`'s `_shift`, `fpexcept.obj`'s `_get_fname`), and the rebuilt PDB has no symbol for them.
   - `__strupr` is a late member that nothing in the rebuild references.
4. **Landed here:** no score drops, and exact-function code bytes stay at 190,333.
   - `engine_calloc` now comes from LIBC's `operator new` (`new.obj`, 0x0048ead0, 100%).
   - The dead `__allmul@16` bridges and their `/include`s are gone.
   - `trnsctrl.obj`'s six annotations move from `game/seh_runtime.cpp` (now deleted) into `crt.cpp`'s CRT-029 block.
5. **LIBC member order:** the rebuild's order disagrees widely with the original's CRT units. The longest in-order run is 48 of 129 shared members, with 2,183 pair inversions. Retiring the whole `/include` list improves this to 64 of 129 and 1,474 inversions.

## Inventory

### What the matching link takes from `src/crt/` (before)

| Object | Link position | `.text` in the gate | Content |
| --- | --- | --- | --- |
| `crt.cpp.obj` | legacy `00494d10` | 112 symbols, 0x12d0 bytes | see below |
| `crt_heap.cpp.obj` | legacy `00491710` | 1 symbol, 16 bytes | `engine_calloc` (0x0048ead0) |
| `crt_runtime_aliases.cpp.obj` | tail | 3 symbols | `__allmul@16`, `__alldiv@16`, `__aulldiv@16` |
| `crt_runtime_calls.cpp.obj` | tail | 3 symbols | `_crt_runtime_{allmul,alldiv,aulldiv}_call` |
| `game/seh_runtime.cpp.obj` | legacy `0048ed00` | empty | everything is `#if !defined(_MSC_VER)` |

**Annotated functions in `crt.obj` under MSVC** (`translation-units.csv`, baseline scores):

| Address | Function | Unit | Score | What it really is |
| --- | --- | --- | --- | --- |
| 0x004907c0 | `string_prefix_reserve` | CRT-053 cvt.c | 100.00 | LIBC static `_shift` (`cvt.obj`) |
| 0x004939d0 | `_matherr` | CRT-066 fpexcept.c | 92.31 | LIBC static `_get_fname` (`fpexcept.obj`); the game's own `_matherr` is `flh_magic_uu_noop` at 0x00456210 (finding 540) |
| 0x00477f70 | `render_feature_flags_get` | G98-005 Main.c | 100.00 | game function |
| 0x00477bd0 | `low_texture_flag_set` | G98-005 Main.c | 100.00 | game function |
| 0x0049d5e0, 0x0049d630, 0x0049d780, 0x0049d870, 0x0049d8d0, 0x0049d950, 0x0049d9d0 | the display-mode dialog | LATE-002 | 5.56–84.00 | game functions, modelled on the port's mode lists (`s_display_modes`, `s_display_resolutions`, finding 602) |

`crt_heap.obj`'s `engine_calloc` (CRT-026 new.cpp, 100.00) is LIBC's `operator new`.

**Port-only code in `crt.obj`.**
- About 100 accessors (`*_peek`, `*_set`, `crt_float_*`, `request_record_*`, `panel_draw_*`, `selection_marker_*`, `crt_surface_*`, `string_cache_*`) and the display-mode list helpers.
- A scratch link without any `src/crt/` object reports **79 unresolved externals**. They are `crt.obj`'s accessors, data and display-mode helpers, plus `engine_calloc`. Units, legacy files and the port tail reference them, mostly `container_unit`, `frameloop`, `surface`, `directdraw_port`, `frontend`, `reward_port`, `info_port`, `lego_port` and `mesh_unit`. None is a LIBC symbol.

**`GLOBAL` markers in `crt.obj` under MSVC.**
- **22 on game data.** Examples: `0x00556f00`, the filter-tuple table `0x00568060`/`0x005754e0`, `0x00505924`/`28`, `0x0076cd78`–`94`, `0x0076ab24`–`58`, `0x0050608c` and `0x00505f84`.
- **4 on LIBC statics:** `s_math_handlers` (0x004b2170), the two time-zone name buffers (0x004b2900/0x004b2940) and `s_leap_month_day_offsets` (0x004b29a4). These are port copies. LIBC's own statics, also linked, have no symbol.

**`/include` directives.**
- 115 `#pragma comment(linker, "/include:…")` in `crt.cpp`. 3 of them are the `__allmul@16` bridges, retired here.
- `crt.cpp`, `crt_heap.cpp` and `seh_runtime.cpp` hold no other link directives.

## The prototype

**Method.**
- **Base** is `main` at `4862ceaa` (`make reccmp-check`, gate build).
- The scratch relinks reuse the gate's `link.txt` and objects. They change only:
  - `crt.obj`, recompiled from an edited copy at the same path so that Reccmp's line matching still applies;
  - the dropped objects.
- Each relink was scored with `tools/reccmp_full_diff.py` and `match_baseline.py check` against the committed baseline.
- **P1:** drop `crt_runtime_*`, `seh_runtime` and `crt_heap`. Compile out `string_prefix_reserve` and `_matherr`. Route `engine_calloc` to `::operator new`, and annotate 0x0048ead0 as LIBC's `??2@YAPAXI@Z`. Keep the 112 remaining `/include`s.
- **P2:** P1 without any `/include` directive.
- The port accessors stay in `crt.obj` in both. Where they go is a relocation (#446), not a CRT question.

**What fails to link:** nothing, in either variant. A link with all of `src/crt/` removed fails only on the 79 non-LIBC symbols above.

**What moves.**
- Both variants pull `matherr.obj` (LIBC's no-op `_matherr`) and `new.obj`.
- P2 drops `strupr.obj`. Every other `/include`d member is still pulled by a real reference, so their directives are redundant for membership. They still decide pull order (below).

| | Base | P1 | P2 |
| --- | --- | --- | --- |
| Exact-function code bytes | 190,333 | 190,386 | 190,204 |
| Full: exact / mapped | 1,654 / 2,583 | 1,653 / 2,581 | 1,652 / 2,580 |
| Game-only: exact / mapped | 1,446 / 2,339 | 1,442 / 2,336 | 1,442 / 2,336 |
| CRT units: mapped / exact / mean | 229 / 192 / 98.94 | 227 / 193 / 98.99 | 226 / 192 / 98.99 |
| LATE units: mapped / exact / mean | 22 / 1 / 41.45 | same | same |
| `.text` (virtual size) | 1,276,517 | 1,276,261 | 1,276,069 |
| Entry point (RVA) | 0x126790 | 0x126670 | 0x126670 |
| Imports | 10 DLLs, 163 names | same | same |
| LIBC members (map) | 170 | 172 | 171 |
| LIBC member order vs the original's CRT units: in-order run / pairs inverted | 48 of 129 / 2,183 | 48 of 130 / 2,261 | 64 of 129 / 1,474 |

**Score changes against the baseline.**
- **Unmapped in P1 and P2:**
  - `string_prefix_reserve` (0x004907c0, 100.00);
  - `_matherr` (0x004939d0, 92.31).
- **Unmapped in P2 only:** `__strupr` (0x0049dcd0, 100.00).
- **Improved:**
  - `__cftoe` (99.22 → 100.00) and `__cftof` (97.20 → 100.00). They call `_shift`, which the port name no longer shadows on the original side.
  - `__umatherr` (96.43 → 98.21).
- **Dropped in P1 and P2:** `AviMovie::AviMovie` (100 → 99.06) and `avi_load` (100 → 96.15). This is an artefact of the prototype's `engine_calloc` wrapper, and the landed change avoids it (below).
- **Dropped in P2:** `__setmbcp` (96.63 → 96.07), an operand-only drift from the reordered LIBC data. The owner has not reviewed it.
- `0x0048ead0` maps to LIBC's `operator new` at 100% in both.

**LIBC member order.** The map's first-appearance order of `libc:`/`oldnames:` members is compared with `translation-unit-ranges.csv`'s `crt` and `late` units, matched by source stem.
- 129 members are shared.
- 5 original units have no member of that name in this LIBC:
  - `new`, which P1 adds;
  - `crt0`, linked as `wincrt0`;
  - `remove`, linked as `unlink`;
  - `a_env`, linked as `aw_env`;
  - `mbsnbicl`, linked as `mbsnbico`.
- **29 members are linked that the original's CRT lacks.** The port tail pulls most of them: `acos`, `asin`, `pow`, `strtod`, `strtol`, `snprintf`, `fprintf`, `strncat`, `strcspn`, `strspn`, `memchr`, `strrchr`, `ldexp` and their helpers.
- **The late members.** `strnicmp`, `xtoa`, `strlwr` and `strupr` are LATE-003 to LATE-006. In the original they sit after the import thunks and after the two game objects LATE-001 (the WAD decoder) and LATE-002 (the display-mode dialog). In every variant here they land inside LIBC, before the thunks.
- **Hypothesis, not tested here.** LATE-001 and LATE-002 were library members pulled in LINK's second pass, and their references (`strnicmp`, `itoa`, `strlwr`, `strupr`, by their old names) resolved through `oldnames.lib`. That pass order would put the four LIBC members after them. Reproducing it needs those two objects linked from a library, not as explicit objects.

## Decision

The method's step 3 condition, keeping every exact match, fails:

| Blocker | Functions | Why | What would unblock it |
| --- | --- | --- | --- |
| LIBC statics have no symbol in the rebuilt PDB | `string_prefix_reserve` 0x004907c0 (exact), `_matherr` 0x004939d0 (92.31) | The port definitions shadow `cvt.obj`'s `_shift` and `fpexcept.obj`'s `_get_fname` so that Reccmp has a name to match. `LegoRR.pdb` has neither `__shift` nor `__get_fname`. The map lists both under "Static symbols". | Reccmp pairing by rebuilt address, or a data source generated from the map's static symbols (finding 540, blocker 1). Then `__cftoe` and `__cftof` become exact too. |
| The game's `_matherr` | 0x00456210 `flh_magic_uu_noop` | Without the port's `_matherr`, LIBC's no-op `matherr.obj` is pulled. The original links the game's `_matherr`, which `__umatherr` calls. | Give `flh_magic_uu_noop` the `_matherr` name in the matching build when 0x004939d0 can be mapped. |
| Late members | `__strupr` 0x0049dcd0 (exact), and LATE-003 to LATE-006 placement | Nothing in the rebuild references `strupr`. The `/include` keeps it, at the wrong position. | Link LATE-001 and LATE-002 as library members that call the old names through `oldnames.lib` (hypothesis above), then retire the `/include`s. |
| The `/include` list sets LIBC pull order | 111 redundant directives, plus `__setmbcp` drift | Retiring them all improves member order (in-order run 48 → 64 of 129) but drifts `__setmbcp`. | Owner review of the drift, or an `/include` list ordered as the original's member order. |
| Game functions in `crt.cpp` | LATE-002 ×7, Main.c ×2, 22 game `GLOBAL`s, about 100 port accessors | Relocation, not CRT work. | #446 and the G98-005 and LATE-002 relocations. The accessors go to the port tail. |

ADR 0006's library order (finding 574) needs no change. Every variant gives the original's descriptor order for the DLLs both binaries import, and the same thunk order.

## What lands

| Change | Effect |
| --- | --- |
| `gods98/avi_player.cpp`: the class-scope `operator new` forwarders call `::operator new` in the matching build and `engine_calloc` natively (`AVI_OPERATOR_NEW`) | LIBC's `new.obj` is pulled; `AviMovie::FrameToSurface` 59.81 → 60.75 |
| `crt/crt_heap.cpp`: 0x0048ead0 is a `LIBRARY` marker for `??2@YAPAXI@Z`; `engine_calloc` is native-only (`PORT-ONLY`) | 0x0048ead0 stays 100% as `operator new`; it leaves the game-only report (now a library function) |
| `crt/crt_runtime_aliases.cpp` and `crt/crt_runtime_calls.cpp` deleted, with their 3 `/include`s | `.text` −192 bytes; nothing referenced them |
| `game/seh_runtime.cpp` and `.hpp` deleted; their 6 trnsctrl `LIBRARY` markers move to `crt.cpp`'s CRT-029 block | no code change: the matching object was empty, and nothing called the native copies; `crt.cpp`'s key goes from 00494d10 to 00494c70 (no reorder) |
| CMake, Meson and the link-order list, `translation-units.csv` (`current_source` of the six trnsctrl rows), `binary_progress_inputs.txt` and the lint ledger (one stale entry) | bookkeeping |

**Before and after** (`make match-progress`):

| | Before (`4862ceaa`) | After |
| --- | --- | --- |
| Exact-function code bytes | 190,333 | 190,333 |
| Full: exact / mapped | 1,654 / 2,583 | 1,654 / 2,583 |
| Game-only: exact / mapped | 1,446 / 2,339 | 1,445 / 2,338 (0x0048ead0 is now a library function) |
| Accuracy, full / game-only | 86.18 / 84.81 | 86.18 / 84.80 |
| `.text` (gate) | 1,276,517 | 1,276,325 |
| Entry point (RVA) | 0x126790 | 0x1266c0 |
| Imports | 10 DLLs, 163 names; descriptor order unchanged | same |
| `opt_ref` annotated, full: removed / removed exact | 477 / 246 | 480 / 248 |
| `opt_ref` `.text` and entry | 993,872, 0xe4a20 | same |

**The `opt_ref` change** has two parts. The before side was measured by relinking `main`'s objects; it reproduced the gate's `.text` and entry point exactly.
- **0x0048ead0:** `engine_calloc` (legacy) and LIBC's `operator new` are both unreferenced under `/OPT:REF`. So the removal moves from the legacy category to `libc` and does not change the count.
- **+3 annotated, +2 exact:** LINK's identical-code folding keeps the other twin of three unit/port pairs (as in finding 599):
  - `legoobject_grid_record_count_tables_clear` / `grid_record_count_tables_clear`;
  - `lego_terrain_cell_palette_resource_at_grid_position` / `terrain_cell_palette_resource_at_grid_position_current`;
  - `noop_callback` / `info_panel_initialise`.

  Removing the bridge objects changes which twin LINK meets first. No code changes.

**Gate.**
- `make reccmp-check` passes.
- No function falls below the baseline, and no exact match is lost.
- The baseline changes in one score and one name, with no instruction-count changes:
  - `AviMovie::FrameToSurface` (0x0047e9e0) rises from 59.81 to 60.75; its `new` call now names `operator new` on both sides;
  - 0x0048ead0 is renamed from `engine_calloc` to `??2@YAPAXI@Z`, at 100.00 on both sides.

## Reproducing

```sh
make reccmp-check && make match-progress && make pe-audit
```

**A scratch relink** follows finding 574's recipe:
1. Copy `build-reccmp/CMakeFiles/program.dir/link.txt`, and point `@objects1.rsp`, `/out`, `/implib`, `/pdb` and `/MAP` at a scratch directory.
2. Edit the response file's object list.
3. To keep Reccmp's line matching, compile a variant of `crt.cpp` at its own path. Restore it before scoring, so the annotations are parsed from the committed source.
4. Score with `tools/reccmp_full_diff.py` and a temporary `reccmp-build.yml` pointing at the scratch executable and PDB. Compare with `tools/match_baseline.py check --report-dir`.
