# 584. Plain immediates past the original image (`0x800000`)

Issue #604. This is the follow-up to finding 580's "Cause A" and finding 581's
"Rebased onto stack #600". Measured on `main` `05cd8938` (stack #600 squashed,
Weapons.c included). The matching build's code and data are unchanged. Only
the comparison tooling changes.

## Summary

1. **The rebuilt image covers `0x800000`, and on this tree it can't be kept
   below it.**
   - The original image ends at `0x771000`, so its operand `0x800000` (an
     entity flag bit) is a plain constant.
   - The rebuilt image ends at `0xad0000`. Its `.data` section starts at
     `0x518000`, which is `0x7a000` later than the original's `0x49e000`
     because rebuilt `.text` is 1,141,388 bytes against 642,549. `.data`
     plus `.bss` take 5,958,352 bytes against the original's 2,936,532.
   - Reccmp's rebuilt-side address test is "a relocation target, or inside
     a known entity". The matching build has no relocations, so the test
     comes down to the PDB's globals. `0x800000` falls inside the port-tail
     array `s_entity_path_motion_runtime` (`entity.cpp`, at `0x786b10`), so
     the operand reads `s_entity_path_motion_runtime[771].samples+308 (OFFSET)`.
     Every later `<OFFSETn>` placeholder then shifts by one.
2. **The real fix (shrinking port-only `.bss` below `0x800000`) isn't
   possible yet.** See [Layout budget](#layout-budget). Annotated original
   globals already take 2,676,563 of the 3,047,424 bytes that fit between
   `.data`'s start and `0x800000`. That includes Files.c's own
   `fileRedundantList`, at 2,048,000 bytes. The port-only `.bss` would have
   to fall from about 3.2 MB to about 270 KB, and the margin would shrink
   with every conversion that adds original globals. Nothing was moved or
   shrunk, so `.bss` is unchanged. The real fix arrives with layout
   identity: once `.text` ends on the original's page and the port tail's
   storage is retired, `.data` ends where the original's does, at
   `0x76f000` or below.
3. **The safety net: `tools/reccmp_plain_immediates.py`**, tagged
   `Deviation (scoring only)` (ADR 0005 entropy rule 4).
   - The project's Reccmp wrapper (`reccmp_full_diff.py`) installs it at
     run time. It wraps each `FunctionComparator`'s inputs and doesn't
     edit Reccmp's package or its scoring.
   - For each compared function, it collects the hex values left plain in
     the original listing at or past the original image's end. While the
     same function's rebuilt side is sanitised, Reccmp's address test and
     name lookup report "not an address" for exactly those values.
   - Every other rebuilt operand is named as before. That includes a
     rebuilt pointer past `0x771000` whose original counterpart is a
     different, in-image address.
   - `match_function.py` attaches the same wrapper, so the single-function
     loop keeps parity. It scores `0x004667e0` and `0x00402510` at 100.00%,
     like the full run.
   - `RECCMP_PLAIN_IMMEDIATES=0` turns the wrapper off, for measuring what
     it changes.
4. **Guards.**
   - `make reccmp-compare`, and so `make reccmp-check`, runs
     `reccmp_plain_immediates.py check`. It fails when a rebuilt diff line
     is the paired original line with a plain constant past the original
     image replaced by a Reccmp name or placeholder. On this tree it
     reports OK. On a report produced with the wrapper off, it reports 17
     lines (Stats_Initialise, `winmain_parse_options` and the rest).
   - `make pe-audit` prints the image end on both sides and warns when the
     rebuilt image covers one of the original's plain constants. Those are
     `0x800000` (21 mapped functions), `0xff0000` (2) and `0x1000000` (18),
     the only values between `0x771000` and `0x2000000` in the original
     listings of mapped functions. Unmapped original code may use others,
     which the wrapper covers anyway.
   - `make tools-test` runs `test_reccmp_plain_immediates.py`. It covers the
     wrapper against Reccmp's real `ParseAsm` when Reccmp is installed, and
     the check. It also runs the image-end cases in `test_pe_audit.py`.

## Layout budget

Matching build at `05cd8938`. The figures are the same after this change,
since no source changed.

| | Original | Rebuilt |
| --- | ---: | ---: |
| `.text` virtual size | 642,549 | 1,141,388 |
| `.data` section start | `0x49e000` | `0x518000` |
| `.data` virtual size (initialised + `.bss`) | 2,936,532 | 5,958,352 |
| `.bss` group (map) | about 2,863,396 | 5,865,984 |
| `.bss` end | | `0xac6ad0` |
| Image end (`ImageBase + SizeOfImage`) | `0x771000` | `0xad0000` |
| Margin below `0x800000` | 585,728 | −2,949,120 |

Rebuilt `.bss` by category, from the map (`pe-audit`): tail 2,105,988;
common 2,076,464, of which 2,048,000 is the original's `fileRedundantList`;
unit 1,034,416; legacy 626,844; unnamed 21,164; LIBC 1,108. By Reccmp entity:
383 annotated `.bss` globals take 2,676,563 bytes and 1,662 unannotated ones
take 2,625,480. Unsized statics make up the rest.

The largest per-object `.bss`, from COFF section sizes (annotated globals
included where an object has them):

| Object | `.bss` | Kind |
| --- | ---: | --- |
| `entity.cpp` | 850,596 | port tail. `s_entity_path_motion_runtime` is 1,280 × 644 bytes |
| `helpwindow.cpp` | 742,472 | port tail. `s_help_info_string_storage[3][240][1024]` is 737,280 |
| `frontend.cpp` | 490,037 | port tail, with annotated menu words |
| `sound.cpp` | 321,676 | port legacy (`s_bank`, `s_variant_pool`) |
| `crt.cpp` | 272,264 | port tail |
| `sound3d.c` | 185,164 | original-shape unit |
| `files_port.cpp` | 133,208 | port tail |
| `resource.cpp` | 98,565 | port tail |
| `message_unit.cpp` | 82,344 | unit (`s_ping`) |

Ending `.data` below `0x800000` at `0x518000` means `.data` plus `.bss` must
fit in 3,047,424 bytes. That is 2,910,928 bytes less than now, and 110,892
bytes more than the original's whole `.data`. Even with every port-only
array moved to the heap, the margin would be under 300 KB on this tree. It
would shrink as conversions add the original's globals while their port
duplicates remain, and as `.text` grows. No margin could be guaranteed, so
this change leaves storage as it is. `helpwindow.cpp`'s 720 KB of 1 KB
info-string slots and `entity.cpp`'s path-motion pool are the obvious
candidates when port-tail storage is retired.

## Score changes

17 functions rose and none fell. These are exactly finding 580's Cause A list,
back to the scores they had before #595.

| Address | Function | Before | After |
| --- | --- | ---: | ---: |
| `0x00402510` | `aitask_terrain_cell_command_create` | 98.78 | **100.00** (exact restored) |
| `0x00404280` | `aitask_cmd_entity_command_chain_tick` | 79.01 | 79.23 |
| `0x00404fc0` | `aitask_place_command_eligibility_check` | 98.53 | 98.64 |
| `0x00407a10` | `pending_type_value_slot_select` | 50.97 | 51.95 |
| `0x00409600` | `ledger_stub_00409600_TerrainDescriptorTilesResetAndFree` | 14.46 | 15.26 |
| `0x0041fb50` | `lego_game_config_init` | 93.64 | 93.67 |
| `0x0042b330` | `lego_load_tile_meshes` | 98.17 | 99.08 |
| `0x0042f6f0` | `lego_cell_init_pass` | 76.86 | 77.84 |
| `0x00430530` | `lego_terrain_cell_mutate` | 95.03 | 95.31 |
| `0x0043bbc0` | `entity_detach_carried_object_and_reset_state` | 61.11 | 63.89 |
| `0x0043bc40` | `objective_sidecar_append_gate` | 38.64 | 39.16 |
| `0x0043f7d0` | `ledger_stub_0043f7d0_EntityEnergyLowTargetRouteCommandTick` | 27.76 | 28.47 |
| `0x00447ea0` | `ledger_stub_00447ea0_EntityTypeBehaviorUpdate` | 13.94 | 14.40 |
| `0x0044bfd0` | `lego::configured_monster_spawn_behavior_tick` | 73.68 | 77.19 |
| `0x004667e0` | `stats_initialise` (Stats_Initialise) | 99.92 | **100.00** (exact restored) |
| `0x00477bf0` | `winmain_parse_options` | 79.91 | 80.35 |
| `0x00482b50` | `render_state_flags_apply` | 59.44 | 60.14 |

`lego_game_config_init` reads 93.67 rather than finding 580's 93.64, because
its unit has changed since. No Weapons.c function uses the constant.

Two things are not restored, and neither belongs to this issue:
- `Key_Find` (`0x00485e30`) at 96.77% is finding 580's other drop, the spare
  slot. It waits for common order.
- In `entity_face_target_action_start` (`0x00441ed0`, 18.72%), one rebuilt
  line still reads `s_entity_path_motion_runtime[771].samples+308`. That
  port line has no `0x800000` in the paired original listing, so the rule
  correctly leaves it alone.

| | Before (`05cd8938`) | After |
| --- | ---: | ---: |
| Exact-function code bytes | 115,442 (17.97%) | **128,629** (20.02%) |
| Accuracy, full / game-only | 73.97% / 71.34% | 73.97% / 71.35% |
| `opt_ref` annotated exact, full / game-only | 1,238 / 1,038 | 1,240 / 1,040 |
| `opt_ref` removed / exact removed | 762 / 346 | 762 / 346 |
| `make unit-data-offsets` | full 0, gaps-only 3, partial 28 | unchanged |

The 13,187 extra exact-function bytes are `Stats_Initialise` (12,961) and
`0x00402510` (226).

## Checks

- `make reccmp-check` passes. The baseline is refreshed by
  `make match-progress`, which also regenerated the listing digests and
  `binary-progress.json`.
- `make tools-test` passes without `--allow-growth`, and
  `make winapi-link-check` passes.
- The native build passes ctest. No C or C++ source changed, so the source
  stays MSVC 5.0-compatible.
