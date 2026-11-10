# Finding 508 — Config tree (G98-006) rebuilt in original shape

Issue #394. This is the first conversion under the [ADR 0005](../adr/0005-original-shape-c-units.md)
recipe after the [Priorities.c pilot](506-original-shape-pilot-priorities.md).
It rebuilds the gods98 config tree in the original's shape: original
translation unit G98-006, text `00478df0`–`00479500`, 16 functions
([finding 505](505-original-translation-units-and-link-order.md)). It also
removes the pilot's symbol-count pads. All scores are from the *matching
build*, comparing `main` with this change. The comparison was run twice:
first against `92857265`, then after rebasing onto `16b188e1` (#399, which
restored the native build). Both runs gave identical per-function results.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, 1,694 bytes) | 34.98% | **96.97%** |
| Exact functions | 2 | **13** of 16 |
| Mapped functions | 14 | 16 |
| Game-only mean (functions mapped in both builds) | 48.59% | 48.91% |
| Full mean (functions mapped in both builds) | 53.02% | 53.31% |

Every function now recompiles to exactly the original's length. The three
that are not exact are close:
- `block_find` (`004792e0`) is an effective match: one register differs at
  the loop advance.
- `block_open` (`00478e60`) swaps two registers: the comment flag and the
  loop counter.
- `block_record_create` (`00479270`) differs only in its store order.

## The unit

[`src/gods98/config.c`](../../src/gods98/config.c) and its C header
[`config.h`](../../src/gods98/config.h) hold the 16 functions in original
order. The original names in the gods98 codeline are not attested in the
binary, so the functions keep the port's names:

| Address | Function | Bytes | before | after |
| --- | --- | ---: | ---: | ---: |
| `00478df0` | `block_text_token_pool_init` (pool init) | 37 | 90.00 | **100.00** |
| `00478e20` | `block_text_token_pool_destroy` (pool shutdown) | 54 | 86.67 | **100.00** |
| `00478e60` | `block_open` (load and tokenise) | 231 | 21.62 | 91.18 |
| `00478f50` | `string_path_join` (build a `::` path) | 157 | 30.34 | **100.00** |
| `00478ff0` | `block_subgroup` (first item of a container) | 39 | 58.33 | **100.00** |
| `00479020` | `block_next_equal_key` (next item) | 34 | 30.77 | **100.00** |
| `00479050` | `block_get_str_copy` (heap copy of a value) | 95 | 85.71 | **100.00** |
| `004790b0` | `block_get_strptr` (value) | 29 | 76.19 | **100.00** |
| `004790d0` | `block_get_bool_path` (boolean value) | 56 | unmapped | **100.00** |
| `00479110` | `config_angle_radians_get` | 89 | 100.00 | **100.00** |
| `00479170` | `config_rgb_vector_read` | 195 | 25.00 | **100.00** |
| `00479240` | `block_close` (free a file) | 42 | 59.46 | **100.00** |
| `00479270` | `block_record_create` (take a record) | 77 | unmapped | 70.18 |
| `004792c0` | `block_record_remove` (return a record) | 19 | 100.00 | **100.00** |
| `004792e0` | `block_find` (path lookup) | 430 | 9.79 | 98.12 |
| `00479490` | `block_text_token_pool_grow` (add a pool block) | 110 | 49.32 | **100.00** |

How the original works, now reproduced as written:

- **Records.** A loaded file is a flat, doubly linked list of 32-byte
  records: `file_text`, `key`, `value`, `depth`, an unused word, `next`,
  `prev` and the pool link. The root record holds the file text. A
  container's value is `{` and its items follow at depth + 1.

  Tokenising always leaves one empty record (no key) at the end of the
  list. A stray `}` decrements the current record's depth, and a key
  with no value takes the next token as its value.
- **Pool.** Records come from a pool of blocks that double in size. Each
  block's pointer is kept in the unit's `.bss` (`00506498`), and freed
  records go back on a free list (`00506518`).
- **State.** The `.bss` range `00506098`–`00506524` is one `static`
  `ConfigGlobs`:
  - the 1,024-byte path buffer used by `string_path_join`;
  - 32 pool-block pointers;
  - the free list;
  - the block count;
  - a flags word.

  The `.rdata` floats at `0049e8f8`–`0049e904` are annotated statics. The
  shared literals `"::"` (`004aab18`) and `":"` (`004a1cd4`) are too.
- **Lookup.** `block_find` splits the path on `"::"` only, using
  `sound_variant_split` (`00477440`). It then walks the list for records
  whose depth equals the segment count less one, and checks each ancestor
  by walking back to the nearest shallower record.

  A root key ending in `*` matches any first segment that starts with the
  text before the `*`. After such a wildcard match the walk continues, so
  **the last match wins**. Otherwise the first match wins. With
  `Lego.cfg`'s two roots, `Lego*` and `LegoRR`, a `LegoRR::…` path
  therefore reads `LegoRR`'s value when it has one and `Lego*`'s
  otherwise.
- **`block_subgroup`** returns the container's first item, not the
  container.

`config.c` uses the VC5 LIBC names `malloc`, `free`, `_strcmpi` and
`_strnicmp`. For non-MSVC builds, [`src/crt/vc5names.h`](../../src/crt/vc5names.h)
now maps them onto the engine heap and the port's CRT compares. This is
needed because the native engine heap is not `malloc`-compatible.

`resource.hpp` gave `read_whole_file` C linkage in place. Its C boundary
header is the new [`src/gods98/resource.h`](../../src/gods98/resource.h),
and `test_c_boundaries.cpp` checks the pair.

### Port-only layer

[`block.hpp`](../../src/gods98/block.hpp) / `block.cpp` now holds only
port-only conveniences over the original records:
- `block_parse` (in `config.c`, after the original functions);
- the accessors `block_name`, `block_value`, `block_first_child`,
  `block_next_sibling` and `block_has_children`;
- the iterators `block_record_count` and `block_record_at`;
- the typed getters `block_get_str`, `block_get_int` and
  `block_get_float`.

`block_next_sibling` is `block_next_equal_key` without the original's
behaviour at a file's top level: there it returns the trailing empty
record, and then dereferences NULL.

`block.cpp` keeps two functions from other units: `block_get_bool`
(`00477710`) and `registry_path_split` (`0048b330`). The pool's port hooks
`slot_set` and `head_set` are gone, and the `ReleaseLinkedObjectList` and
`GlobalRecordListPushHead` versions in `crt.cpp` are now `block_close` and
`block_record_remove`.

The original parser reproduces the port's tree parser exactly on the
shipped data. All 138 block-text files give 12,876 keyed records with the
same FNV-1a digest (`1a0f0c04`) that `test_block --real` pins.

## Callers

The callee contract changed in three ways. Every caller was migrated:

1. **`::` is the only path separator.** The port built about 136 paths
   with spaces, such as `"Lego* Main CDTracks"` or `"%s Levels %s %s"`.
   All of them, and the helpers that build them, now use `::`:
   `util_level_cfg_path`, `rw_path`, `fe_cfg_str`, `fe_parse_submenu`,
   `entity_config_section_find`, `entity_assembly_parse`, `lwsc_assembly_*`,
   `movie_key_path` and others.
2. **`block_subgroup` returns the first item.**
   - `block_first_child(block_subgroup(…))` became `block_subgroup(…)`.
   - The port helpers that need the container (`entity_config_section_find`
     and `level_config_block_find`) now use `block_find` plus a has-items
     check.
3. **Single-segment lookups were root-qualified.** A one-segment path
   reaches the trailing empty record whenever it misses, and the original
   then dereferences its NULL key.

   Nine loaders looked sections up with a bare name: `Samples`,
   `Bubbles`, `Pointers`, `ToolTipIcons`, `UpgradeNames`, `UpgradeTypes`,
   `Dependencies` (two loaders) and `SeeThroughWalls`. On `Lego.cfg` those
   names never resolved, because they are sections under the root. Each
   now builds `string_path_join(root, "<name>", 0)`, exactly as the
   original does with `Config_BuildStringID(gameName, …)`.

   `panels_config_entry_apply` now uses the original's `Panels%ix%i`
   format, and `panel_button_layout_config_load` the root-qualified
   `Buttons%ix%i`. The bare fallbacks in `text_messages_config_load` were
   removed.

   **Behaviour change:** on the shipped `Lego.cfg` these sections now load
   for the first time:
   - Samples (446 items)
   - Pointers (52)
   - Bubbles (39)
   - ToolTipIcons (16)
   - UpgradeNames (6)
   - UpgradeTypes (57)
   - Dependencies (24)
   - Panels640x480 (12)
   - Buttons640x480 (38)
   - ObjTtSFXs (see Native port below)

   This is the original's behaviour, but these loaders have not yet been
   run against real data in the port.

The two-argument `block_get_bool(BlockFile*, path)` overload became the
unit's `block_get_bool_path` (`004790d0`). Paths that never resolved
before, because they have no root segment, were converted
mechanically and still do not resolve. Examples are `"Main::DrainTime"`
and `"HelpWindow::HelpWindowOn"`.

### Caller scores

`Priorities.c` (GAME-034) now uses the original contract: direct
`key`/`value` reads, and `block_subgroup` returning the first item. The
unit rose from 67.76% to 70.15% byte-weighted:

| Address | Function | before | after |
| --- | --- | ---: | ---: |
| `0045cdc0` | `priorities_load_images` | 65.69 | **91.84** |
| `0045cf50` | `priorities_load` | 66.27 | 57.69 |

The other twelve Priorities functions are unchanged. `priorities_load` is
the same 57.69% that finding 506 measured as its original-contract
ceiling, step (e). It now recompiles to exactly the original's 266 bytes,
where it was three instructions longer before. Its only source-level
difference from the original is the order value, `80 - count * 5`.

MSVC 5.0 factors that expression as `(16 - count) * 5`, where the original
kept `mov ebp, 0x50; sub ebp, count*5`. The register choices that follow
differ. Ten source forms of the expression, two declaration orders and
four include orders all compile to the same code. The earlier 66.27% came
from a longer listing whose registers happened to align. **This drop is
reported as an exception, not as entropy drift: the function's source
changed.**

Other callers whose source changed, before → after:

| Address | Function | before | after |
| --- | --- | ---: | ---: |
| `00434da0` | `advisor_tooltip_and_sound_config_load` | 28.33 | **67.84** |
| `00434a50` | `sound_samples_config_load` | 37.96 | 47.48 |
| `00407240` | `bubble_config_texture_load` | 17.18 | 26.19 |
| `00434af0` | `text_messages_config_load` | 26.52 | 32.50 |
| `00414ce0` | `movie_avis_enabled` | 19.35 | 25.81 |
| `00437150` | `tooltip_icons_config_load` | 18.32 | 22.90 |
| `0042d460` | `upgrade_types_load` | 16.62 | 18.68 |
| `00434710` | `panel_button_layout_config_load` | 31.29 | 33.07 |
| `004345f0` | `panels_config_entry_apply` | 31.22 | 32.69 |
| `0044acc0` | `terrain_object_sfx_grid_config_load` | 77.52 | 80.00 |
| `00472cd0` | `entity_assembly_parse` | 8.30 | 10.10 |
| `00427f80` | `level_upgrade_name_strings_load` | 16.16 | 17.17 |
| `0041ada0` | `interface_images_config_load` | 12.58 | 13.56 |
| `0041ab00` | `interface_build_images_config_load` | 9.62 | 10.22 |
| `00401530` | `advisor_position_config_load` | 28.38 | 28.67 |
| `00471370` | `cmd_place_ray_candidate_check` | 15.22 | 15.30 |

The `SeeThroughWalls` lookup is in a static helper inside
`cmd_place_ray_candidate_check`'s annotated range.

Two of these needed more than the contract change:
- `advisor_tooltip_and_sound_config_load` now has the original's loop
  shape: it iterates with `block_next_equal_key` and reads the fields
  directly. Its inline tool-tip text handling, unchanged in behaviour,
  moved to a static helper in place of the original's `ToolTip_SetContent`
  call.
- `level_upgrade_name_strings_load` matches its `Level` prefix with
  `_strnicmp`, as the original does.

A first pass without these two changes dropped both functions, to 23.08%
and 13.79%.

## Entropy

This change refreshes the *match baseline* under ADR 0005 entropy rule 2.
As rule 3 requires, it removes the pilot's pads, `lrr_symbol_pad_crt`
(`crt.hpp`) and `lrr_symbol_pad_frontend` (`frontend.hpp`). It also
edits the shared headers:
- `block.hpp`, `string.hpp` and `crt.hpp` lose declarations;
- `resource.hpp` gains `extern "C"`;
- the `BlockFile` forward declarations change.

Together, these moved **one** function whose source is unchanged:

| Address | Function | before | after | Listing difference |
| --- | --- | ---: | ---: | --- |
| `0044e0c0` | `terrain_create_retail` | 11.73 | 11.44 | registers, one spill slot, instruction order, branch offsets |

`level.cpp` changed elsewhere, so its symbol numbering shifted. The
listing has the same instructions apart from registers, stack slots and
branch layout. This is drift under rule 2.

Four functions with unchanged source rose slightly from the same cause:
- `surface_triangle_area_calculate`: 17.31 → 17.37
- `surface_palette8_to_16bit_surface_expand`: 32.65 → 33.82
- `text_buffer_draw_wrapped`: 5.96 → 5.99
- `parse_surf_chunk`: 6.34 → 6.94

**Net effect, both scopes:** 33 functions rose and 2 fell (the two
above). Two functions are newly mapped (`004790d0` and `00479270`).

## Native port

Native tests run on `main` again since #399, and this change was rebased
onto it. `config.c` compiles natively as C++ (CMake `LANGUAGE CXX`) and
passes `gcc -std=gnu89 -Werror=declaration-after-statement`, as does
`priorities.c`.

    cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure

This builds with no errors, and **all 31 tests pass**, including `block`,
`c_boundaries`, `level`, `mission`, `bind` and `frontend`.

The first run crashed `level`, `mission` and `bind`, and fixing it
uncovered a second port bug:

- **The crash.** It was the hazard described above:
  `terrain_texture_set_config_load_grid` looked up the level's
  `TextureSet` value (`Textures::Lava` in `Lego.cfg`, `TlfSet` in the
  fixture) as a bare one-segment path before the root-qualified one. It
  now uses only the root-qualified path, which is how the original
  resolves it.
- **The section name.** `terrain_object_sfx_grid_config_load` asked for
  `S_ObjTtSFXs`. That is the Ghidra label `s_ObjTtSFXs_004a5154`, not the
  original's string, which is `ObjTtSFXs`. The section now loads on the
  shipped data too. The `test_level` fixture had been shaped for the old
  container contract: its items were siblings after an empty group. It
  now nests them as `Lego.cfg` does.

New fixture coverage in `test_frontend` (`test_root_qualified_sections`)
loads the following from a `Lego* { … }` file shaped like `Lego.cfg`:
- `UpgradeNames`
- `UpgradeTypes`
- `Dependencies`, through the startup-config loader
- `ToolTipsSFXs`

It also checks that a file missing those sections loads nothing.
`test_level` covers `ObjTtSFXs`.

With the bare `"UpgradeTypes"` lookup restored, the test binary crashes
(SIGSEGV) on the trailing empty record. The other activated loaders
(Samples, Pointers, Bubbles, ToolTipIcons, Panels, Buttons) load images
or sounds, so the fixture tests cannot drive them without assets.

With `TEST_WADS_DIR` set to the shipped data:
- `block --real` passes, confirming the 12,876-record digest.
- `entity_ae` needed its digest constants updated: `LargeDigger.ae` has
  two `Drill` groups under `Lego*`, and the original's last-match rule
  picks `digdrill` over `digDRILL`. Only the case differs, and file
  lookups ignore case.
- `model` and `flic` fail on `main` with the shipped data as well, and
  this change does not touch them.

The Meson build does not list `config.c`. Like `priorities.c` before it,
the Meson build is not maintained for original-shape units.

## Reproducing

    make reccmp-check                       # matching build + baseline gate
    make match-function SOURCE=src/gods98/config.c
    make match-function SOURCE=src/game/priorities.c
    python3 tools/translation_units.py build
