# Finding 602 — Right-sizing the port-only `.bss` in the matching build

Issue #641. This follows finding 584 (#608), which found that the rebuilt
image covers the original's flag constant `0x800000`, and finding 595 (#633),
which cut HelpWindow's port `.bss` from 742,472 bytes to 11,500. Measured on
#645's head (`0d4a9a3a`, G98-017 TextWindow.c, stacked on #644 and #643 over
`main` `8601d8ab`), before and after this change, from full matching builds.
The change was first measured on `main` `8601d8ab`; every figure moved by
under 5 KB on the rebase, and the savings are byte-for-byte the same.

## Summary

1. **Five port-only buffers leave `.bss`.** Together they took 1,583,648 bytes.
   - `entity.cpp`: the path-motion table (824,320 bytes) is allocated on first use.
   - `frontend.cpp`: the two zeroed sentinels (431,660 bytes) become one menu,
     allocated on first use.
   - `files_port.cpp`: an unreferenced static (131,076 bytes) leaves the
     matching build.
   - `sound.cpp`: the variant-chain pool (131,072 bytes) is allocated on first use.
   - `texture.cpp`: the two name caches (65,536 bytes) are allocated on first use.

   No capacity changes, and the native behaviour is the same (see
   [Behaviour](#behaviour)).
2. **The gate image now ends at `0x8c6000`, down from `0xa48000`.** Its `.bss`
   fell from 5,174,704 bytes to 3,591,056. The `/OPT:REF` relink's `.bss`
   (the "about 3.10 MB" in #641) fell from 3,126,672 bytes to 1,543,024. That
   image now ends at `0x68b000`, below the original's `0x771000`.
3. **The gate image can't get below `0x800000` yet. It is 811,008 bytes too
   long.** See [The `0x800000` budget](#the-0x800000-budget). About half of the
   port-only `.bss` that is left is read by annotated functions or by converted
   units. Port `.text` also moves `.data` 634,880 bytes later than in the
   original.
4. **No score moved.** All 2,562 mapped functions keep their scores, and the
   baseline changes only in its input hash. Exact-function code bytes stay at
   181,261. The plain-immediate check (#608) passes.
5. **The stale comment on `s_main_device` is fixed** (`viewports_port.cpp`).
   Since #635, the winapi builds publish the port renderer's
   IDirect3DRMDevice3 into `00505918` (`directdraw_port_publish`). Only the
   native builds leave it NULL.

## Before and after

"Gate" is the `/DEBUG` link that Reccmp scores. "Relink" is the
`/OPT:REF` link without `/DEBUG` that `make opt-ref-link` produces (finding
576).

| | Original | Gate before | Gate after | Relink before | Relink after |
| --- | ---: | ---: | ---: | ---: | ---: |
| `.bss` (map group) | about 2,863,396 | 5,174,704 | **3,591,056** | 3,126,672 | **1,543,024** |
| `.data` virtual size | 2,936,532 | 5,278,832 | **3,695,184** | 3,228,592 | **1,644,944** |
| `.data` start | `0x4a1000` | `0x53c000` | `0x53c000` | `0x4f7000` | `0x4f7000` |
| Image end | `0x771000` | `0xa48000` | **`0x8c6000`** | `0x80e000` | **`0x68b000`** |
| Below `0x800000` by | 585,728 | −2,392,064 | **−811,008** | −57,344 | **+1,527,808** |
| Exact-function code bytes | | 181,261 | 181,261 | | |

- The relink drops Files.c's `fileRedundantList` (2,048,000 bytes) because
  no live code references it. That is why its `.bss` is about 2 MB smaller
  than the gate's.
- The gate's `.bss` holds 2,688,715 bytes of annotated original globals
  (Reccmp's entity sizes, from the PDB), both before and after. The
  port-only part fell from 2,485,989 bytes to 902,341. LIBC's 1,132 bytes
  and the 21,164 unnamed bytes at the start of the group are counted in it.

## Per-object `.bss`, gate

Measurement:
- Each object's `.bss` is the sum of its COFF `.bss` sections, plus the
  commons that the object declares first.
- Objects are placed by the map's public symbols, or by `cvdump -s`'s
  module data symbols where they have no public `.bss` symbol.
- The annotated bytes are the annotated Reccmp data entities inside that
  range.

`make pe-audit` gives the per-category group totals. It attributes each gap
to the preceding public symbol, so it can't give per-object sizes: the map
lists no static data.

| Object | Kind | Annotated | Port-only before | Port-only after | Change |
| --- | --- | ---: | ---: | ---: | ---: |
| `files.c` | unit | 2,050,592 | 29 | 29 | |
| `entity.cpp` | tail | 176 | 850,420 | 26,108 | **−824,312** |
| `frontend.cpp` | tail | 60 | 489,977 | 58,321 | **−431,656** |
| `sound.cpp` | tail | 0 | 321,676 | 190,604 | **−131,072** |
| `crt.cpp` | legacy | 54,480 | 217,784 | 217,784 | |
| `sound3d.c` | unit | 185,164 | 0 | 0 | |
| `files_port.cpp` | tail | 0 | 133,144 | 2,064 | **−131,080** |
| `resource.cpp` | legacy | 790 | 97,775 | 97,775 | |
| `message_unit.cpp` | unit | 82,336 | 8 | 8 | |
| `tutorial_hud.cpp` | tail | 0 | 66,348 | 66,348 | |
| `texture.cpp` | tail | 0 | 65,552 | 20 | **−65,532** |
| `lava_erosion.cpp` | unit | 64,028 | 4 | 4 | |
| `screen_effect_quads.cpp` | unit | 58,848 | 400 | 400 | |
| `view.cpp` | tail | 16 | 52,972 | 52,972 | |
| `legoobject_unit.c` | unit | 50,464 | 180 | 180 | |
| `tooltip.c` | unit | 25,028 | 0 | 0 | |
| `aitask.c` | unit | 20,124 | 0 | 0 | |
| `aitask_port.cpp` | tail | 0 | 20,120 | 20,120 | |
| `lws.c` | unit | 16,428 | 0 | 0 | |
| `model.cpp` | tail | 0 | 16,008 | 16,008 | |
| `g98_024.c` | unit | 16,000 | 0 | 0 | |
| `frontend_unit.cpp` | unit | 264 | 15,821 | 15,821 | |
| `effects.cpp` | tail | 0 | 15,316 | 15,316 | |
| `reward.c` | unit | 12,880 | 17 | 17 | |
| `surface.cpp` | legacy | 0 | 11,477 | 11,477 | |
| `weapon.cpp` | tail | 0 | 9,748 | 9,748 | |
| `cmd.cpp` | tail | 4 | 9,712 | 9,712 | |
| `helpwindow_port.cpp` | tail | 0 | 8,064 | 8,064 | |
| `util.cpp` | tail | 0 | 7,725 | 7,725 | |
| `level.cpp` | legacy | 2,556 | 5,524 | 5,524 | |

The objects that the parallel conversions own hold no large port buffer:
- `lava_erosion.cpp` (#638) has 4 port-only bytes beside its 64,028 annotated.
- `bubbles.cpp` (#637) has 8.
- `gods98/text_window.cpp` (#639) and `gods98/lwo.cpp` (#640) have
  none worth listing.

No follow-up is needed for them.

## What changed

| Object | Buffer | Bytes | Change |
| --- | --- | ---: | --- |
| `entity.cpp` | `s_entity_path_motion_runtime[ENTITY_TYPES * ENTITY_CAP]`, 1,280 × 644 | 824,320 | A pointer. `entity_path_motion_runtime_get` (`legoobject_port.cpp`) `calloc`s the whole table on the first request that creates a slot. A lookup without `create`, and the clear, return when the table doesn't exist yet. |
| `frontend.cpp` | `frontend_menu`'s `static FeMenu s_empty` (383,704) and `frontend_active_submenu`'s `static FeSubmenu s_empty` (47,956) | 431,660 | One `calloc`ed zero `FeMenu`, made on the first out-of-range lookup. The empty submenu is that menu's `submenus[0]`, which is all zero too. |
| `files_port.cpp` | `static ResourceFileTokenList s_resource_file_tokens` (512 × 256 + 4) | 131,076 | Only the directory-tree scans read it, under `FILES_PORT_DEFINES_UNIT_NAMES`. The declaration is now under the same condition. |
| `sound.cpp` | `s_variant_pool[SOUND_VARIANT_POOL_NODES]`, 16,384 × 8 | 131,072 | A pointer. `s_variant_pool_alloc` (`sfx_port.cpp`) `calloc`s the pool for the first link. Node 0 is never a link, so no chain reaches the pool before then. |
| `texture.cpp` | `s_cache` and `s_shared_cache`, `TEX_CACHE_MAX` (4,096) × 8 each | 65,536 | Pointers. A table is `calloc`ed when its first texture is cached. |

The `files_port.cpp` buffer is worth noting. In the matching build no code
references `s_resource_file_tokens`, so the object file has no symbol for it.
MSVC 5.0 reserves the storage anyway, so the 128 KB showed up as an unnamed
gap after `s_loader_progress_callback` (finding 584 listed it as that
object's `.bss`).

A search of the gate's `.bss` for ranges that no PDB data symbol covers finds
only three more such gaps: `entity_config.cpp` (4,636), `files_port.cpp`
(1,028, `s_wad_mount_path_rewrite`) and `video_graph.cpp` (1,024).

No limit was lowered: each table keeps its capacity. A heap table is
allocated once and never freed, like the static it replaces.

## Behaviour

On the native build:
- Every path reads and writes the same zeroed storage as before, and at
  the same capacity.
- The only new case is an allocation failure:
  - the path-motion lookup returns no slot, which its callers already handle
    for a full table;
  - a variant link is dropped, as when the pool is full;
  - a texture stays uncached, as when the cache is full;
  - the front end's empty menu and submenu are NULL.

  The front end needs one 384 KB allocation, on the first out-of-range
  lookup.

None of the changed functions is annotated, and none of the changed files
has a `// FUNCTION:` annotation. The full Reccmp run confirms that no
annotated code changed: no score moved.

## The `0x800000` budget

The gate's `.data` starts at `0x53c000`. For its image to end below
`0x800000`, the `.data` virtual size must stay under about 2,891,776 bytes
(`.idata` and `.rsrc` take one page each after it). It is 3,695,184, so
about 803,408 bytes must still go. The gate image ends 811,008 bytes past
`0x800000`, after page rounding.

What fills `.data` now:

| Part | Bytes |
| --- | ---: |
| Initialised `.data` and the CRT initialiser tables | 104,124 |
| Annotated original `.bss` (with Files.c's `fileRedundantList`, 2,048,000) | 2,688,715 |
| Port-only `.bss` (LIBC and the unnamed head included) | 902,341 |

Without any port-only `.bss`, `.data` would be 2,792,839 bytes, about
100 KB under that limit. What is left, though, can't be removed by right-sizing:

- **Read by annotated functions:** 206,848 bytes. These are `crt.cpp`'s
  `s_display_modes` (139,264) and `s_display_resolutions` (67,584), the
  port's model of the DirectDraw mode lists. `0049d780`, `0049d8d0`
  and `0049d950` index them directly, so a pointer would change
  those functions' code. They go when the display dialog's unit is
  converted.
- **Read by converted units:** 266,976 bytes.
  - `sound.cpp`'s `s_bank` (184,800) is indexed by `sound3d.c`.
  - `view.cpp`'s `s_map_row_span_records` (51,456) is read by
    `screen_effect_quads.cpp`'s unit.
  - `resource.cpp`'s scene-availability and name tables (30,720) are read through the
    `rock_fall_styles` and vehicle-model headers.

  They can't change without editing the units.
- **By-value state shared across files:**
  - `tutorial_hud.cpp`'s `s_nerp_runtime` (49,316);
  - `frontend.cpp`'s `s_timed_effect_records` (25,440) and
    `s_interface_images` (25,160), declared in nine `frontend_for_*.hpp`
    headers;
  - `aitask_port.cpp`'s `cmd_pending_command_counts` (19,200, `cmd.hpp`).

  Each would need every reader rewritten, for 20 to 50 KB each.
- **Small tables (under 16 KB each):** about 250 KB in all, across
  `resource.cpp`'s WAD table, `model.cpp`, `effects.cpp`, `surface.cpp`,
  `weapon.cpp`, `cmd.cpp` and others.

The other half of the distance is port `.text`.
- The gate's `.text` is 1,258,152 bytes against the original's 642,549.
  With `.rdata`, that moves `.data` 634,880 bytes later than the original's
  `0x4a1000`.
- If `.data` started at `0x4a1000`, the current 3,695,184 bytes would end at
  `0x827250`. Only the two together, the port `.bss` and the port `.text`,
  push the image past `0x800000`.
- Both shrink as conversions retire port code and storage (finding 584).
- The `/OPT:REF` relink, which drops unreferenced code and `fileRedundantList`,
  already ends at `0x68b000`.

Until then, `tools/reccmp_plain_immediates.py` keeps the 21 + 2 + 18 original
plain constants plain (finding 584). It still reports OK.

## Checks

- `make reccmp-check` passes. No function fell below the baseline, and no
  score moved: the refreshed baseline differs only in its input hash.
  - The plain-immediate check reports OK.
  - The `winapi-link-check` reports OK.
  - `check-map` reports OK.
- `make match-progress` regenerated the baseline, the listing digests and
  `binary-progress.json`.
  - Accuracy is unchanged (83.30% full, 81.67% game-only).
  - Exact-function code bytes are unchanged, at 181,261.
- **`opt_ref`:**
  - By address, the count goes from 504/247 to 502/245 (full) and from
    485/234 to 483/232 (game-only). These are alias rows (finding 579's
    caveat): the layout moved.
  - By symbol (`make opt-ref-callers --min 1`), the count is 486/230 before
    and after, and the removed set is identical.
- `make unit-data-offsets` gives full 0, gaps-only 7, partial 27, n/a 191.
  Every row is identical to #645's.
- `make tools-test` passes (no `--allow-growth`).
- **Native:**
  - With the CI configuration, ctest passes 37/37.
  - Configured with `TEST_WADS_DIR` (real data), it passes 39/39.
  - The native game ran 900 frames of the front end and of Level01 with
    `SDL_VIDEODRIVER=offscreen` and `LEGORRR_AUTOSTOP_FRAMES`, and exited 0.
  - With `CMAKE_BUILD_TYPE=RelWithDebInfo`, the `message` test fails ("the
    catalog window answers zero, guarded"). It fails the same way on
    unmodified `main` `8601d8ab`, so it is not from this change.
- The source stays MSVC 5.0-compatible: plain `calloc`, C-style casts, and
  no new language features.
