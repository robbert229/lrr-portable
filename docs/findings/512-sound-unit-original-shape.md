# Finding 512 — gods98 sound layer (G98-021, formerly G98-021/022/023) rebuilt in original shape

Issues #409 and #410. This finding first settles a boundary: the map
([finding 505](505-original-translation-units-and-link-order.md)) had three
low-confidence units between Lws.c and the 2,000-slot handle table:

- G98-021, the sound front end and CD track state;
- G98-022, the RIFF/mmio wave readers;
- G98-023, the MCI `cdaudio` commands.

The evidence below makes them **one original object**, so the spec now has
one unit, G98-021, at `00488b50`–`004893f0` (16 functions). It was then
rebuilt under the [ADR 0005](../adr/0005-original-shape-c-units.md) recipe.

All scores come from the *matching build*. "Before" is the match baseline on
`main` at `6db0b3c7` (#421); "after" is this change. `make reccmp-check` and
`make match-function` give the same per-function scores.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, 2,087 bytes) | 18.43% | **100.00%** |
| Exact functions | 2 | **16** of 16 |
| Mapped functions | 16 | 16 |
| Game-only mean (2,121 functions mapped in both builds) | 49.29% | 49.81% |
| Full mean (2,340 functions mapped in both builds) | 53.66% | 54.13% |

## The boundary

### What the data shows

No data separates the three former units, and the alignment rule below
favours one object.

- **`.bss` is one run.** Lws.c's `.bss` ends with `00544860`. The next
  referenced slots are:
  - the CD state at `0054486c`, `00544a00`–`00544a08` and `00544a10`;
  - the MCI error code at `00544a1c`;
  - then G98-024's table from `00544a20`.

  Between them, `00544868`, `00544a0c`, `00544a14` and `00544a18`, and the
  400-byte block `00544870`–`005449ff`, are never referenced.
- **MSVC 5.0 aligns `.bss` by content.** Test objects compiled with the
  matching toolchain show three things:
  - an object holding an 8-byte or larger variable (or a `= {0}` globals
    struct) gets an 8-aligned `.bss` section, and one with only 4-byte
    variables gets a 4-aligned one;
  - statics are laid out by name hash, not declaration order;
  - unreferenced statics keep their storage.

  On `main`, 43 of the 48 game and gods98 units with `.bss` have an
  8-aligned first referenced slot. Two of the five exceptions were G98-021
  and G98-023. The CD state holds the 400-byte block, so its object
  starts at `00544868`. One object then spans `00544868`–`00544a20`, exactly
  0x1b8 bytes, with the MCI error code as its last slot. A separate MCI
  object (4-aligned) would also fit, but only if `00544a14` and `00544a18`
  were unused tail slots of the CD object.
- **`.data` fits either owner.** Two unreferenced dwords (`1`, `1`) and an
  unreferenced `char[8]` `"cdaudio"` (`004ab6d0`–`004ab6df`) sit directly
  before the MCI literals (`004ab6e0`–`004ab783`). Finding 511 puts G98-027's
  data at the next 8-byte boundary (`004ab788`), so G98-024 to G98-026 own no
  `.data`.
- **No `.rdata` and no padding signal.**
  - None of units 021–030 has x87 code or references `.rdata`, so the six
    `{1,2,4,-1}` header tables at `0049ea5c`–`0049eac4` cannot be matched to
    objects.
  - All 16 functions are 16-byte aligned with `0x90` fill.
- **The MCI return buffer is common.** `00576500` (200 bytes) is a
  tentative definition, so it lies in the common region in no link order.

### What decides it: behaviour

- The MCI commands (`00489210`–`004893a0`) are called only by the CD track
  code (`00488bb0`, `00488bf0` by a tail `jmp`, and `00488c00`).
- The wave readers have no data and are called only by 3DSound (G98-009), so
  call locality votes for neither neighbour. They sit between the CD code and
  the MCI code, so if those two are one object, the readers are part of it.

OpenLRR, an independent reconstruction of a different build of the game,
puts all 16 functions in one `Sound` module. It models one 0x1b8-byte
`soundGlobs` whose fields land on exactly these slots, with `mciErr` last.
That agrees with the merge, but it is someone else's reconstruction and was
not used as evidence.

The merged unit stays **low** confidence: the decision rests on behaviour,
as for utils.c (finding 509). Its name is left empty in the spec. "Sound.c"
is likely but not attested.

## The unit

[`src/gods98/soundsys.c`](../../src/gods98/soundsys.c) and its C header
[`soundsys.h`](../../src/gods98/soundsys.h) hold the 16 functions in
original order. The stem is `soundsys` because `gods98/sound.h` is already
the C boundary header of the port's `sound.cpp`. The functions keep the
port's names; the table gives the gods98 names.

| Address | Function (gods98) | Bytes | before | after |
| --- | --- | ---: | ---: | ---: |
| `00488b50` | `media_cd_audio_device_state_update` (Sound_Initialise) | 59 | 87.18 | **100.00** |
| `00488b90` | `media_cd_audio_playback_available` (Sound_IsInitialised) | 27 | 100.00 | **100.00** |
| `00488bb0` | `media_cd_audio_track_start` (Sound_PlayCDTrack) | 56 | 12.50 | **100.00** |
| `00488bf0` | `media_cd_audio_stop_and_close_if_active` (Sound_StopCD) | 15 | 100.00 | **100.00** |
| `00488c00` | `media_cd_audio_watchdog_tick` (Sound_Update) | 102 | 45.24 | **100.00** |
| `00488c70` | `sound_wave_memory_format_read` (WaveOpenFile, memory) | 501 | 7.79 | **100.00** |
| `00488e70` | `sound_wave_file_average_bytes_per_second_read` | 146 | 19.05 | **100.00** |
| `00488f10` | `sound_wave_file_open_read_chunks` (WaveOpenFile) | 425 | 11.76 | **100.00** |
| `004890c0` | `sound_wave_data_chunk_open` (WaveStartDataRead) | 54 | 11.04 | **100.00** |
| `00489100` | `sound_wave_data_read` (WaveReadFile) | 205 | 20.41 | **100.00** |
| `004891d0` | `sound_wave_handle_and_buffer_release` (WaveCloseReadFile) | 53 | 13.33 | **100.00** |
| `00489210` | `media_cd_audio_track_play_from` (Restart_CDTrack) | 80 | 22.22 | **100.00** |
| `00489260` | `media_cd_audio_error_text_get` (ReportCDError) | 23 | 17.14 | **100.00** |
| `00489280` | `media_cd_audio_track_completion_poll` (Status_CDTrack) | 163 | 8.82 | **100.00** |
| `00489330` | `media_cd_audio_device_open` (Play_CDTrack) | 100 | 16.33 | **100.00** |
| `004893a0` | `media_cd_audio_stop_and_close` (Stop_CDTrack) | 78 | 17.95 | **100.00** |

### State

- **`.bss`.** One `static SoundGlobs soundGlobs` at `00544868` (0x1b8 bytes)
  holds the sound flag, the CD player's track, loop flag, callback and
  last-poll time, and the last MCI error. The never-referenced slots keep
  the layout.
- **`.data`.** The two dwords and `"cdaudio"` are annotated file-scope
  definitions. Nothing reads them.
- **Common.** `char sound_mci_return[200]` at `00576500`.
- **Outside globals.** `mainGlobs.hWnd` (`00505800`, the port's
  `g_platform_cursor_window_handle`) and the 3D-sound flag (`00533878`,
  `s_bank_sound_on`).

### Shape notes

- **The wave readers** are the DirectX SDK's `wave.c` routines, kept as
  written: `goto` error exits, the `ER_*` codes (`0xe000`–`0xe103`), and
  WaveReadFile's precedence slip (`nError = mmioGetInfo(...) != 0`, the
  `neg/sbb/neg` at `00489119`).
  - The memory variant zeroes an `MMIOINFO` and opens with the `'MEM '` I/O
    procedure.
  - The byte-rate probe's second `if (hmmioIn)` (`00488eec`) sits after its
    outer `if`, not inside it.
- **Sound_PlayCDTrack and Sound_StopCD return `void`.** The original
  returns whatever `eax` holds (the zero flag on the sound-off path).
  Returning a value reloads or clears `eax`, and `void` compiles to the
  original's instructions.
- **One error path per MCI function.**
  - Play_CDTrack and Stop_CDTrack send each command only while the last one
    succeeded, then test `mciErr` once. Early returns, or one `||`
    condition, place the `return TRUE` block first.
  - Status_CDTrack jumps from its "stopped" test into the error block of
    its track test (`goto`), which is the original's block order.
- **Restart_CDTrack formats with `wsprintfA`** (USER32) and Status_CDTrack
  with LIBC's `sprintf`, as in the original. The status test uses
  `_strcmpi` and `atoi`.

## Win32 calls and the native port

The unit calls 15 imports: `GlobalAlloc`, `GlobalFree`, `wsprintfA`,
`timeGetTime`, nine `mmio*` functions, `mciSendStringA` and
`mciGetErrorStringA`.

- **Windows builds.** [`gods98/winmm.h`](../../src/gods98/winmm.h) includes
  `<windows.h>` and `<mmsystem.h>`, so every call goes through
  `call [__imp__...]` as in the original.
- **Native build.** The same header declares the names and the
  `pack(1)` layouts, and
  [`winmm_port.cpp`](../../src/gods98/winmm_port.cpp), compiled only off
  Windows, emulates them, like view.cpp's D3DRM stand-ins (finding 510):
  - mmio reads the whole file, or the caller's memory for `'MEM '`, into one
    buffer, so `mmioGetInfo` exposes it all and `mmioAdvance` reaches end
    of file;
  - `mmioDescend` follows `MMIO_FINDCHUNK`/`FINDRIFF`/`FINDLIST`, with
    padding to even sizes;
  - MCI has no `cdaudio` device, so every command fails with
    `MCIERR_INVALID_DEVICE_NAME` unless a test installs
    `winmm_port_mci_hook_set`;
  - `timeGetTime` is the platform's millisecond clock, or
    `winmm_port_time_hook_set`.

This replaces the port's previous designs:

- the CD code's hook table (`media_cd_audio_*_hook_set`, the open-flag
  setter), which ADR 0005 step 4 excludes;
- the wave readers' portable reimplementation over `GameFileHandle`, with
  its own handle and chunk types.

## Callers

- **The CD player** is declared in `media.hpp`, with C linkage in place,
  for its C++ callers:
  - the CD watchdog (`004297b0`, `media.cpp`);
  - the engine tick (`engine.cpp`);
  - startup (`frameloop.cpp`).

  The five functions only `soundsys.c` calls are no longer declared there.
  The contracts are now the original's:
  - Sound_Initialise takes `nosound` and sets the loop flag to -1, so an
    ended track is replayed before the callback runs.
  - The poll runs at most every 4,000 ms only while sound is on.
  - The track argument is 0-based, so MCI plays track + 1.
- **The wave readers** are declared only in `soundsys.h`, because their
  types are Win32's. The port's one caller, the loose-file sample loader,
  moved from `sound.cpp` to
  [`sound_wave.cpp`](../../src/gods98/sound_wave.cpp) (port-only), so that
  only that file includes the Win32 headers in the matching build. It keeps
  the port's rule that a mounted WAD entry wins over a loose file.
- **Outside callees and globals** gained `extern "C"` in place:
  - `audio_system_init` in `audio.hpp`;
  - `s_bank_sound_on` in `sound.hpp`;
  - `g_platform_cursor_window_handle` in `platform/platform.h`.

  They are declared for C in the new boundary headers
  [`gods98/audio.h`](../../src/gods98/audio.h) and
  [`platform/window.h`](../../src/platform/window.h).
  `test_c_boundaries.cpp` checks each pair.

**Caller scores.** The CD watchdog `media_cd_watchdog_update` (`004297b0`)
rose from 22.22% to 26.67%: Sound_PlayCDTrack and Sound_StopCD now have the
original's `void` contract.

## Entropy

This change refreshes the *match baseline* under ADR 0005 entropy rule 2.
Outside the unit, 8 functions rose (the CD watchdog and 7 by drift) and
4 fell.

The four drops are in files whose source did not change. Each includes
`sound.hpp`, which lost the old wave declarations and types; two also
include `media.hpp`. The listings below compare whole functions from
single-function objects on `main` and on this change:

| Address | Function | File | before | after | Listing difference | Class |
| --- | --- | --- | ---: | ---: | --- | --- |
| `00482620` | `mesh_segment_vertex_fill` | `record.cpp` | 31.30 | 26.09 | registers and stack slots only | drift, rule 2 |
| `00483420` | `screen_effect_item_opacity_set_target` | `record.cpp` | 33.90 | 28.25 | registers and instruction order (same instructions) | drift, rule 2 |
| `00411360` | `menu_mouse_coordinate_update` | `frontend.cpp` | 21.12 | 21.03 | operand order of commutative `a() + b()` sums, registers, branch offsets, one extra stack reload (`mov`) | drift, rule 2 (see below) |
| `00471370` | `cmd_place_ray_candidate_check` | `cmd.cpp` | 15.30 | 15.22 | one extra `fxch st(1)`, branch offsets +2 | drift, rule 2 (see below) |

**For the owner.** The last two each gain an instruction. In both, the
extra instruction comes from register allocation: a reload, or an x87 stack
exchange. Finding 510 classified x87 stack order and instruction order as
rule 2 drift, and these are classified the same way. Both drops are under
0.1 point.

**Net effect, both scopes:** 22 functions rose and 4 fell; none was newly
mapped or lost. Of the rises, 14 are in the unit, 1 is the CD watchdog, and
7 are drift in unchanged code.

`make reccmp-lint` fails on `main` (4,315 unannotated functions) and on this
change (4,321):
- 13 port helpers were removed;
- 19 functions were added: the native stand-ins for the imports and their
  test seams in `winmm_port.cpp`, which are not compiled in the matching
  build, and `sound_wave_file_load`.

## Native port

`cmake -S . -B build && cmake --build build && ctest --test-dir build
--output-on-failure` builds with no errors, and **all 32 tests pass**.
`soundsys.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
-fsyntax-only -Wall`.

- **`media` is new to CMake.** `test_media.cpp` existed but was not wired
  in. Its CD test now drives the original-shape player through the MCI and
  time hooks. It checks:
  - sound off;
  - open and play;
  - the 4,000 ms poll;
  - a track still playing;
  - an ended track being replayed (loop flag -1), stopped and replaced;
  - a failed stop, which does not close;
  - a failed open, which does not play.
- **`sound` gains a wave-reader test.** It runs the original readers on the
  emulation:
  - memory and file opens;
  - an odd-sized chunk before `fmt `;
  - reads that count down `cksize`;
  - a truncated data chunk (`ER_CORRUPTWAVEFILE`);
  - non-PCM extra format bytes;
  - a non-WAVE form, a short `fmt `, and a missing file.

  The existing variant-chain tests load loose `.wav` files through
  `sound_wave_file_load`.

Behaviour changes in the native port:

- CD audio: before, `platform_cd_audio_*` reported "unavailable". Now
  `mciSendStringA` reports an invalid device. Either way, no CD plays.
- Loose wave files are parsed by the original readers. Before, the port
  bounds-checked them with its own parser. The readers accept a RIFF form
  shorter than the file, as Windows mmio does.

The `platform_cd_audio_*` adapters in `platform.h` and the two
`frameloop.cpp` files are now unused. Removing their declarations would
renumber every file that includes `platform.h`, so that is left to a
separate change.

## Reproducing

    python3 tools/translation_units.py dossier 488b50 4894c0
    make reccmp-check
    make match-function SOURCE=src/gods98/soundsys.c
    python3 tools/translation_units.py build
