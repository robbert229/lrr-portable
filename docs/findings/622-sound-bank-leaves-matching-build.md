# Finding 622 — The port's sound bank leaves the matching build

Issue #686. This follows finding 602 (the port-only `.bss` blockers) and the
`.bss` cuts in findings 613 and 614 (#679, #680). Measured on `main`
`9043af89` (GAME-049, Water.c), before and after this change, from full
matching builds in one worktree.

## Summary

1. **`sound.cpp`'s `BankSlot s_bank[600]` (600 × 0x134 = 184,800 bytes) is
   no longer defined in the Windows builds.** Those are the matching build and
   the MinGW reference build. Native and the tests keep it unchanged.
2. **In the Windows builds nothing ever wrote it.** G98-009 (`sound3d.c`)
   compiles its DirectSound branch there (`SOUND3D_DIRECTSOUND`). That branch
   loads every sample into the original's own table,
   `Sound3DGlobs.soundTable` (`s_sound3d` at `0x5065b8`, table at `+0xe0` =
   `0x506798`, finding 14 §1). Only the native branch of `sound3d.c` and
   `sound.cpp`'s native-only `sound_bank_clear` write `s_bank`. So the
   matching build already used one bank, the original's, and `s_bank` was
   184,800 bytes of zeroes that two port readers scanned.
3. **The two readers keep their exact Windows behaviour.**
   `sound_bank_find` returns `NULL` and `sound_bank_entry_play_mode` returns
   `-1`, which is what they returned for an all-zero bank (no slot has
   `SOUND_SLOT_BUSY`). Their native bodies are unchanged.
4. **The gate image ends at `0x890000`, down from `0x8bd000`.** It is still
   589,824 bytes past `0x800000`, so `tools/reccmp_plain_immediates.py` is
   still needed (finding 584).
5. **No score moved.** Exact-function code bytes stay at 205,613. The
   baseline changes only in its input hash.

## Before and after

"Gate" is the `/DEBUG` link that Reccmp scores (`make pe-audit` on
`build-reccmp/`). "Relink" is the `/OPT:REF` link from `make opt-ref`
(`binary-progress.json`).

| | before | after |
| --- | ---: | ---: |
| Gate `.bss` (map group) | 3,534,912 | **3,350,112** (−184,800) |
| — port tail objects | 708,032 | 523,232 (−184,800) |
| — legacy objects | 225,672 | 225,672 |
| — commons | 2,280,080 | 2,280,080 |
| Gate `.data` virtual size | 3,640,272 | 3,455,472 |
| Gate image end | `0x8bd000` | **`0x890000`** |
| Below `0x800000` by | −774,144 | **−589,824** |
| Relink `.bss` | 1,486,864 | **1,302,064** |
| Relink image end | `0x684000` | **`0x657000`** |
| Exact-function code bytes | 205,613 | 205,613 |
| `opt_ref` annotated (full): removed / exact removed | 412 / 223 | 411 / 222 |
| `opt_ref` annotated (game-only) | 405 / 216 | 404 / 215 |

### The `opt_ref` change is a folding swap

No annotated function changes state. The one difference is `0x43baf0`
(`legoobject_flags_mask107c33de_or_1c00203_any_test`, exact). The relink
folds its body with the identical port function
`entity_flags_mask107c33de_or_1c00203_any_test` (`legoobject_port.cpp`).
Before, it kept the port copy, so `make opt-ref` counted the unit's symbol as
removed. After, it keeps the unit's copy. The same code is in both images.
Finding 613 saw the same kind of identical-code-folding swap.

The relink also drops eight port functions that only
`sound_bank_entry_play_mode` reached in Windows builds:
`audio_system_player_get`, `audio_stream_start`, `audio_stream_submit_chunk`,
`audio_buffer_play_attenuated`, `audio_buffer_loop_attenuated` and the three
`platform_audio_*` functions they call. That is +1,568 removed
`port_legacy` bytes.

## G98-009's data placement

`make unit-data-offsets` gives the same result for G98-009 before and after:

- `.data`: partial, one gap from `0x4aae5c` (0x2c bytes in the original,
  0x30 rebuilt).
- `.bss`: partial, one gap from `0x533878` (0x2d2c0 in the original, 0x2d348
  rebuilt).

The change does not touch the unit. `s_bank` was never its state. The frozen
header `sound_for_sound3d.hpp` still declares `extern BankSlot s_bank[]`.
Only native code refers to it, so the Windows link needs no definition.

## Windows behaviour (unchanged, noted)

Three port paths in the Windows builds read the port bank, and they still
find nothing, as before:

- `audio_play_named` and `audio_loop_named` (`frameloop.cpp`, `tickers.cpp`,
  `frontend_unit.cpp`) call `sound_bank_load` and then `sound_bank_find`.
  The load reaches the original's `sound_bank_entry_load`, which fills
  `soundTable`, so the find returns `NULL` and they return failure.
- `advisor_sound_alias_play_global` (`tutorial_script_port.cpp`) passes an
  original `soundTable` slot to `sound_bank_entry_play_mode`, which returns
  `-1`.

They were already silent. Routing them through the original's play API
is port work, out of scope here.

## Other port `.bss`

No other port `.bss` can go without a conversion. `resource.cpp`'s tables
(about 98 KB) are shared state, not dead copies. Their readers are port code
that the matching build links:

- `s_wads` (41,360 bytes): `wad_port.cpp`, through `resource_for_wad.hpp`.
- `s_scene_resource_name_slots` (18,436), `s_scene_availability_styles`
  (12,288), `s_rock_fall_style_names` (8,192) and `s_scene_cell_table_pairs`
  (6,144): `rock_fall_styles_port.cpp`, plus the frozen headers of Vehicle
  model, upgrade levels and screen-effect quads.

These are the remaining port objects, attributed by the preceding public
symbol (`make pe-audit`). A private `static` counts with the public symbol
before it:

| Owner | Kind | `.bss` bytes | Largest |
| --- | --- | ---: | --- |
| `audio.cpp` | legacy | 186,356 | see the note below |
| `resource.cpp` | tail | 97,776 | `s_wads` 41,360 |
| `tutorial_hud.cpp` | tail | 69,752 | `s_nerp_runtime` 49,400 |
| `lava_erosion_port.cpp` | tail | 64,040 | two 16,000-byte cell lists |
| `screen_effect_quads_port.cpp` | tail | 59,248 | `s_grid_rec` |
| `frontend.cpp` | tail | 58,396 | `s_timed_effect_records` 25,444, `s_interface_images` 25,160 |
| `entity.cpp` | tail | 26,552 | `s_grid_object_records` 8,808 |
| `aitask_port.cpp` | tail | 20,120 | `cmd_pending_command_counts` 19,200 |
| `model.cpp` | tail | 16,008 | two 8,000-byte resource pairs |
| `effects.cpp` | tail | 15,320 | `s_active_effect_records` 7,124 |
| `surface.cpp` | legacy | 11,488 | |
| `crt.cpp` | legacy | 11,000 | |
| `weapon.cpp` | tail | 9,752 | `s_weapon_definitions` 9,728 |
| `cmd.cpp` | tail | 9,720 | `s_deferred_olist` 8,800 |
| everything else | | under 9 KB each | `sound.cpp` keeps 5,808 |

**Note on `audio.cpp`:** almost all of its 186,356 bytes are not
`audio.cpp`'s. They are G98-009's own `static Sound3DGlobs s_sound3d`
(0xe0 + 600 × 0x134 = 185,024 bytes, the original's bank). The map lists that
static under no symbol, so it is attributed to `audio.cpp`'s
`s_active_audio_player`, which comes just before it. The real port `.bss` is
therefore about 185 KB less than the 748,904 bytes of tail and legacy
objects: about 564 KB after this change, against about 749 KB before.

## Verification

- `make reccmp-check`: no function fell below the match baseline. The
  baseline changes only in `input_sha256`. The plain-immediate check, the
  winapi link check and the link-order check pass.
- `make match-progress`, `make unit-data-offsets` and `make opt-ref-callers`
  were run, and their generated files are committed.
- `make tools-test` (no `--allow-growth`), `make winapi-link-check` and
  `make pe-audit` pass.
- Native `cmake -S . -B build` and `ctest`: 37 of 37 pass, including `sound`.
  The native preprocessed source is unchanged. A `-DCMAKE_BUILD_TYPE=Release`
  configure fails `message` ("the catalog window answers zero, guarded").
  That build type is not CI's, and this change does not touch it.
- MSVC 5.0: the change only adds preprocessor guards and an early return.
