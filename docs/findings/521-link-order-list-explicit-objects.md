# Finding 521 — The link-order list: the matching build links game and gods98 objects explicitly in original order

Issue #437, stage S1b of the [#388 plan](../plans/388-original-link-order.md)
([ADR 0006](../adr/0006-original-link-order.md) decision 1). This is a
link-only change under ADR 0006 entropy rule 1: no file under `src/` other
than the new list changes. Measured on `main` at `4893b9bc` (after #459, #461
and #460).

## Summary

1. **One list.** `src/link_order.cmake` holds one line per source,
   `lrr_link(<kind> <key> <library> <source> [<platform>])`, in link order.
   It is the only place a game, gods98, platform or program source enters the
   CMake build. The native and reference builds build `game`, `gods98` and
   `platform_glue` from its `library` column. The matching build compiles
   every entry as its own CMake object library and links the objects
   explicitly, in list order, then `LRR_LINK_LIBRARIES` (from the same file).
   `tools/link_order.py` reads the file with the standard library only, so
   #438's static check needs no toolchain.
2. **The map follows the list.** In `build-reccmp/LegoRR.map` the explicit
   objects' contributions to `.text`, `.data` and `.bss` appear in list
   order, each object in one run, and every explicit object precedes the
   first LIBC member. Common symbols come after all of them, as before.
3. **Inversions** (`binary-progress.json`): pair inversions fall from
   855,122 to **633,298** (full) and from 841,215 to 619,397 (game-only), as
   finding 516 simulated (about 650,000). Adjacent inversions fall from 903
   to 895 (full) and 844 to 832 (game-only).
4. **Scores.** No function's score changes. The three objects that were never
   pulled now link without errors and add 19 newly mapped functions. 22
   rebuilt listings change, all in operand names only, with the same
   instruction count.
5. **One link fix.** Listing `libc.lib` before the import libraries made
   LINK pull the console start-up object `crt0.obj` (LNK2005 against
   `wincrt0.obj`, LNK2001 `_main`). The matching link now names its entry
   point, `/ENTRY:WinMainCRTStartup`.

## The list

| Kind | Key | Meaning |
| --- | --- | --- |
| `unit` | unit id | the file holds exactly one original unit |
| `legacy` | median original address of the file's functions | a mixed file that still holds original functions |
| `tail` | `-` | port-only code (the *port tail*) |

Order, as linked by the matching build (`PLATFORM=winapi`):

- **Game segment.** 2 converted units (GAME-003 `bezier.c`, GAME-034
  `priorities.c`) at their units' positions and 32 legacy files at their
  medians, from `game/cmd.cpp` (`00403a50`) to `game/weapon.cpp`
  (`0046f140`). Legacy files from `src/gods98` whose functions are mostly
  game code sit here too (`message.cpp`, `engine.cpp`, `record.cpp` and
  others), because the median decides.
- **gods98 segment.** 11 units and 17 legacy files, from G98-001 to
  `gods98/model.cpp` (`0048c210`). The units are the 10 converted gods98
  units (G98-004, 006, 007, 018, 019, 021, 024, 026, 027, 029) and G98-001,
  whose 11 functions are all of `game/video_graph.cpp` and nothing else
  (still in `src/game` and the `game` library; it moves when it is
  relocated). `platform/winapi/entry.cpp` (`004777a0`) and
  `platform/winapi/frameloop.cpp` (`004783f0`) hold G98-005 (Main.c)
  functions, so they are legacy entries at their medians, not tail entries.
  Their native counterparts are tail entries with platform `native`.
- **Port tail.** 21 files with no original function marker: 7 game (two of
  them winapi-only), 9 gods98 (one winapi-only), 2 `crt/` runtime files,
  `main.cpp` and the two native platform files.
- **CRT-range legacy files.** `game/seh_runtime.cpp` (`0048ed00`),
  `crt/crt.cpp` (`00491450`) and `crt/crt_heap.cpp` (`00491710`). Their
  medians are past the last gods98 unit, so they come after the tail and
  just before LIBC.
- **Libraries.** `libc.lib`, `oldnames.lib`, then the import libraries in the
  original's import-thunk order (finding 505 rule 7): `d3drm`, `ddraw`,
  `vfw32` (it holds the `avifil32` imports), `dsound`, `kernel32`. The
  toolchain has no `dinput.lib`; the rebuild imports nothing from DirectInput
  by thunk. The others follow in the order of the original's import
  directory: `ole32`, `winmm`, `user32`, `gdi32`, `advapi32`. CMake's
  standard libraries follow. In the map the library members come in the
  order `libc`, `d3drm`, `kernel32`, `winmm`, `user32`, `gdi32`, `advapi32`;
  the others contribute nothing.

Library membership in the native and reference builds is unchanged, entry for
entry. The three game files that the native program also compiles itself
(`seh_runtime.cpp`, `dependency_grid.cpp`, `screen_pick.cpp`) stay in its
sources there. A converted unit is a `.c` entry, so `LRR_ORIGINAL_SHAPE_SOURCES`
is now derived from the list.

### Compile commands do not change

Each object library gets the include path of its entry's library in the other
builds: `src`, `src/game`, `src/gods98` for `game` and `program`; `src`,
`src/gods98`, `src/game` for `gods98`; `src` for `platform_glue`. Only the
object paths change (`CMakeFiles/link_NNN_<stem>.dir/...`). A source-level
`INCLUDE_DIRECTORIES` property would not do this: CMake puts it before the
target's directories. No listing changes its instruction count, which agrees.

## Objects that were never pulled

Finding 507 named three: `gods98/ptrstack.cpp`, `game/video_graph.cpp` and
`engine_calloc` (`crt/crt_heap.cpp`). A comparison of the previous map with
the library sources found four more, all port-only: `game/opgroup.cpp`,
`gods98/draw_port.cpp`, `gods98/registry_port.cpp` and
`gods98/winmm_port.cpp`.

- **All seven link** as explicit objects. None duplicates a LIBC symbol or
  another object's symbol, so none had to move out of the matching build.
- `ptrstack.cpp` (GAME-016 ×6, GAME-037 ×1) is a legacy entry at `0040fe40`;
  `video_graph.cpp` is G98-001's unit entry; `crt_heap.cpp` is a CRT-range
  legacy entry. Their 19 functions are newly mapped (table below).
  `engine_calloc` at `0048ead0` scores 100%.
- `draw_port.cpp`, `registry_port.cpp` and `winmm_port.cpp` compile to empty
  objects under winapi (native-only code), so the map shows no
  contribution from them. `opgroup.cpp` is port-only code that nothing calls;
  it now sits in the tail.

## Verifying the order from the map

The map lists each public and static symbol with its `lib:object`; explicit
objects have no library prefix. Grouping symbols by section (from the map's
section table) and collapsing consecutive symbols of one object into runs:

| Section | Explicit-object runs | In list order | Explicit after the first library member |
| --- | ---: | --- | --- |
| `.text` | 80 | yes | none |
| `.rdata` | 0 | — | — |
| `.data` | 49 | yes | none |
| `.bss` | 24 | yes | `<common>` only |

Objects with no symbols in a section are skipped. The map names objects by
file name only. Two pairs share one: `game/entity.cpp` and
`gods98/entity.cpp`, and `gods98/frameloop.cpp` and
`platform/winapi/frameloop.cpp` (which are adjacent, so they show as one run).
The comparison therefore matches the sequence of runs, not a set of names.
#438's map check has to do the same, or tell the pairs apart by their
symbols.

## Metrics

From `binary-progress.json`, `main` (`4893b9bc`) against this change:

| Measure | Before | After |
| --- | ---: | ---: |
| Pair inversions, full | 855,122 | 633,298 |
| Pair inversions, game-only | 841,215 | 619,397 |
| Adjacent inversions, full | 903 | 895 |
| Adjacent inversions, game-only | 844 | 832 |
| Exact-function code bytes | 44,210 | 44,226 (+16, `engine_calloc`) |
| Mean score, full / game-only | 55.16% / 50.96% | 54.94% / 50.76% |
| `.text` identical at original addresses | 18,443 B (2.87%, chance 2.83%) | 17,957 B (2.79%) |
| `.rdata` identical at original addresses | 1,386 B (12.34%) | 1,696 B (15.10%) |
| `.data` identical at original addresses | 13,858 B (18.93%) | 13,823 B (18.88%) |
| Rebuilt `.text` | 932,700 B | 934,836 B |
| Functions identical at their original address | 0 | 0 |

- **The means fall** only because the 19 newly mapped functions enter them
  with low scores (mean 28.23%); no function's score falls.
- **`.text` at original addresses stays at chance**, as the plan expects
  for S1b: nearly every function still sits at a different address. The
  rebuilt `.text` grows by the newly linked objects (2,136 B).

## Baseline refresh (ADR 0006 entropy rule 1)

`make reccmp-check` against `main`'s baseline: **no function falls below
baseline, and no score changes**, in either scope. 19 functions are newly
mapped; 0 are newly unmapped.

Newly mapped (link cause: the object is now linked explicitly):

| Address | Function | Score | Scope |
| --- | --- | ---: | --- |
| `0x0040f6c0` | `scene_node_record_create` | 7.27 | game, full |
| `0x0040f7d0` | `callback_state_record_create` | 36.78 | game, full |
| `0x0040f850` | `callback_state_list_dispose` | 46.38 | game, full |
| `0x0040fe40` | `ptrstack_push` | 6.45 | game, full |
| `0x0040fe60` | `ptrstack_pop` | 28.00 | game, full |
| `0x0040fe90` | `ptrstack_for_each` | 54.55 | game, full |
| `0x004628e0` | `callback_record_allocate_initialize` | 50.98 | game, full |
| `0x004721e0` | `lego::directshow_sample_setup` | 8.26 | game, full |
| `0x004722b0` | `lego::configured_video_graph_setup` | 8.33 | game, full |
| `0x00472390` | `lego::configured_video_com_route_init` | 10.53 | game, full |
| `0x00472430` | `lego::directshow_interface_release` | 13.51 | game, full |
| `0x004724a0` | `lego::configured_video_sample_update` | 6.45 | game, full |
| `0x00472530` | `lego::video_duration_frame_bound_update` | 10.26 | game, full |
| `0x00472560` | `lego::directshow_render_surface_create` | 13.33 | game, full |
| `0x00472670` | `lego::directdraw_surface_description_get` | 12.50 | game, full |
| `0x004726c0` | `lego::video_frame_bound_refresh` | 0.00 | game, full |
| `0x004726d0` | `lego::directshow_sample_update_and_blit` | 58.82 | game, full |
| `0x004726f0` | `lego::directshow_wrapper_destroy` | 64.00 | game, full |
| `0x0048ead0` | `engine_calloc` | 100.00 | game, full |

### Changed listings

22 functions' rebuilt listing digests change (the same 22 in both scopes).
Every one keeps its score and its instruction count. The drift classifier
(`match_listings.classify`, listing format 3 after #461) labels all 22
`drift-review` with an instruction delta of +0.

The cause is the same in all 22. An immediate flag mask (in `test`, `or`,
`and`, `cmp` or `mov`) happens to equal an address inside a large rebuilt
array, and Reccmp renders it as an element of that array. The new
object order moves `.data` and `.bss`, so the constant now lands in a
different array:
`s_help_info_string_storage[2][104]+432 (offset)` becomes
`s_entity_path_motion_runtime[1000].samples+56 (offset)` in 19 of them; the
other three move between `s_entity_path_motion_runtime[1148]`,
`g_flic_windows[0]` and a raw address.

**Why the classifier still says `drift-review`.** #461 masks symbolic
operands, and these lines are masked on one side but not the other:
`name[i][j]+N (offset)` normalises to `<address>`, while
`name[i].field+N (offset)` normalises to `name[i].<address>` and keeps the
variable name. The two listings are otherwise identical in sequence after
normalisation, so a mask that also covers member-access operands would label
them `drift`. This is the only remaining gap seen in this change; the
anonymous-namespace names (build path) and function-pointer immediates that
also differ between the builds are masked by #461 and no longer show.

| Address | Function | Before | After | Scope | Classifier | Link cause |
| --- | --- | ---: | ---: | --- | --- | --- |
| `0x00404280` | `cmd_entity_command_chain_tick` | 10.71 | 10.71 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x00404fc0` | `ledger_stub_00404fc0_PlaceCommandEligibilityCheck` | 7.39 | 7.39 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x00405c10` | `pending_command_entity_assignment_tick` | 13.48 | 13.48 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x00407a10` | `pending_type_value_slot_select` | 50.97 | 50.97 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x00409600` | `ledger_stub_00409600_TerrainDescriptorTilesResetAndFree` | 12.85 | 12.85 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x00409a40` | `bobbin_delayed` | 19.35 | 19.35 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x0042f6f0` | `cell_init_pass` | 11.09 | 11.09 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x00430530` | `terrain_cell_mutate` | 12.41 | 12.41 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x004385b0` | `entity_shared_eligibility_check` | 57.69 | 57.69 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x0043bbc0` | `entity_detach_carried_object_and_reset_state` | 36.11 | 36.11 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x0043bc40` | `objective_sidecar_append_gate` | 36.03 | 36.03 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x0043f7d0` | `ledger_stub_0043f7d0_EntityEnergyLowTargetRouteCommandTick` | 26.69 | 26.69 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x00441ed0` | `entity_face_target_action_start` | 18.27 | 18.27 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x00444460` | `ledger_stub_00444460_FlaggedVectorStepCommandIssue` | 23.61 | 23.61 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x00447ea0` | `ledger_stub_00447ea0_EntityTypeBehaviorUpdate` | 11.38 | 11.38 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x0044bfd0` | `lego::configured_monster_spawn_behavior_tick` | 59.65 | 59.65 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x0044c110` | `ai_attached_entity_wake` | 10.53 | 10.53 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x00466750` | `grid_cell_invalidate_and_clear_flags` | 16.67 | 16.67 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x00477bf0` | `winmain_parse_options` | 79.91 | 79.91 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x00482b50` | `render_state_flags_apply` | 59.44 | 59.44 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x004893f0` | `level_map_slots_reset` | 87.50 | 87.50 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |
| `0x00489410` | `level_map_slot_allocate` | 95.65 | 95.65 | game, full | `drift-review`, +0 | flag immediate names a different rebuilt array element (`.data`/`.bss` order) |

No function has an instruction-count change, so nothing needs the owner's
review beyond the classifier gap above.

## Other observations

- **`/ENTRY`.** With `libc.lib` on the command line before the import
  libraries, LINK searches LIBC before it would infer the entry point from
  `/subsystem:windows`. LIBC.LIB's symbol index resolves the start-up data
  that `crt0.obj` and `wincrt0.obj` both define (`__amsg_exit`,
  `___app_type`, ...) to `crt0.obj`, which then needs `_main`. Naming
  `/ENTRY:WinMainCRTStartup` makes the entry symbol undefined from the
  start, so `wincrt0.obj` is pulled first. The entry point is the same
  function as before.
- **`match_function.py`** reads the compile commands of the per-object
  targets; it now also reconfigures when `src/link_order.cmake` changes.
  `make match-function-parity`: 2,360 of 2,374 functions score identically,
  with game (1,160), gods98 (897) and platform (3) in full, including the 19
  newly mapped functions. The 14 that differ are the CRT functions finding
  516 lists for S1a (`__isindst` and 13 LIBC functions), so the target exits
  non-zero as it does on `main`.
- **ReproBit.** `tools/reprobit.sh` imports the CMake build; its import of
  object libraries was not exercised here.
- **Meson** builds natively only and does not read the list. Its source
  lists are unchanged. On `main` it already fails: its lists lack the
  converted `.c` units, so test executables fail to link, and
  `test_record.cpp` does not compile. Whether to retire it is #452.
- **Inputs hash.** `tools/binary_progress_hash.py` reads the list's sources
  (all platforms; native ones are then excluded as before) and hashes
  `src/link_order.cmake` and `tools/link_order.py`.
- **Conversions in flight** rebase and replace their `CMakeLists.txt` lines
  with one `lrr_link(unit ...)` line, which `tools/convert_unit.py` now
  writes.

## Reproducing

    make reccmp-check && make match-progress
    python3 tools/link_order.py --platform winapi        # the linked entries

The map check groups the publics and statics of `build-reccmp/LegoRR.map`
by section, using the section table at the top of the map, and compares the
runs of explicit objects (no `lib:` prefix) with the entries' file names in
list order.
