# Finding 553 — 3D sound unit (G98-009) rebuilt in original shape

Issue #524, under [ADR 0005](../adr/0005-original-shape-c-units.md). #517
(finding 548) relocated G98-009 verbatim into `src/gods98/sound3d.cpp`. This
change rebuilds it as the C unit [`src/gods98/sound3d.c`](../../src/gods98/sound3d.c),
with the C boundary header [`sound3d.h`](../../src/gods98/sound3d.h), and deletes
the `.cpp`. The unit is text `0047a640`–`0047c170`: gods98's Sound3D.c, the
DirectSound layer. It owns:

- the DirectSound object, the primary buffer and its 3D listener;
- the 600-slot sample table (0x134-byte records, up to three buffers each);
- the list of playing 3D buffers and the frames whose sounds follow them;
- two streamed wave buffers, one plain and one looping, refilled from a ring.

"Before" is the *match baseline* at `1e6cf9ae`, the head of #526 (GAME-023
relocated, finding 551), which contains #528 (finding 552). This change is
stacked on #526. "After" is this change. `make reccmp-check` and `make match-function` give the same
per-function scores.

## Result

| | before | after |
| --- | ---: | ---: |
| Unit similarity (byte-weighted, all 42 functions, 6,641 bytes) | 18.76% | **95.27%** |
| Exact functions | 4 | **33 of 42** |
| Mapped functions | 38 | 42 |
| Accuracy, full (`binary-progress.json`) | 57.09% | 58.07% |
| Accuracy, game-only | 52.50% | 53.59% |
| Exact-function code bytes | 67,562 | 70,358 |
| Order inversions, full (pairs) | 52,084 | 52,261 |

Byte weights are the original's code bytes, without the `0x90` fill. An
unmapped function counts as 0%. Pair inversions rise only because four more
functions are mapped; all 42 link in original order.

| Address | Function | Bytes | before | after |
| --- | --- | ---: | ---: | ---: |
| `0047a640` | `audio_system_init` | 438 | 17.50 | 99.25 |
| `0047a800` | `audio_active_shutdown` | 77 | 18.60 | **100.00** |
| `0047a850` | `sound_volume_value_valid` | 24 | 100.00 | **100.00** |
| `0047a870` | `sound_bank_entry_load` | 597 | 26.39 | 98.34 |
| `0047aad0` | `sound_bank_free_slot_find` | 40 | 46.15 | 92.86 |
| `0047ab00` | `sound_bank_entry_release` | 128 | 23.53 | **100.00** |
| `0047ab80` | `simtick_directsound_listener_view_set` | 19 | 100.00 | **100.00** |
| `0047aba0` | `simtick_sound_keepalive` | 32 | 41.38 | **100.00** |
| `0047abc0` | `sound_listener_frame_update` | 324 | unmapped | **100.00** |
| `0047ad10` | `sound_position_set_from_vector` | 30 | 17.65 | **100.00** |
| `0047ad30` | `sound_attached_stop_by_entity` | 50 | 51.06 | **100.00** |
| `0047ad70` | `s_play_seam` | 687 | 12.42 | 96.73 |
| `0047b020` | `sound_buffer_association_add` | 45 | 38.71 | **100.00** |
| `0047b050` | `sound_bank_entry_volume_set` | 120 | 37.97 | **100.00** |
| `0047b0d0` | `sound_bank_entry_volume` | 21 | 83.33 | **100.00** |
| `0047b0f0` | `sound_handle_buffer_get` | 61 | 56.00 | **100.00** |
| `0047b130` | `sound_buffer_stop_handle` | 41 | 64.86 | **100.00** |
| `0047b160` | `sound_subsystem_stop_all` | 49 | 36.84 | **100.00** |
| `0047b1a0` | `sound_instance_track_register` | 113 | 8.96 | 94.23 |
| `0047b220` | `sound_instance_track_stop_and_remove` | 117 | 25.56 | **100.00** |
| `0047b2a0` | `keyed_resource_entry_remove` | 52 | 25.64 | **100.00** |
| `0047b2e0` | `nested_record_list_remove_by_key_recursive` | 71 | 29.51 | **100.00** |
| `0047b330` | `nested_record_list_remove_matching_key_impl` | 82 | 40.00 | **100.00** |
| `0047b390` | `sound_source_spatial_refresh_all` | 122 | 36.19 | **100.00** |
| `0047b410` | `sound_source_spatial_refresh` | 140 | 14.63 | **100.00** |
| `0047b4a0` | `sound_tracked_buffers_set_min_distance` | 45 | 44.44 | **100.00** |
| `0047b4d0` | `sound_tracked_buffers_set_max_distance` | 45 | 44.44 | **100.00** |
| `0047b500` | `audio_sound_volume_set` | 47 | 37.93 | **100.00** |
| `0047b530` | `audio_sound_volume_index_set` | 30 | 80.00 | **100.00** |
| `0047b550` | `audio_sound_volume_minus_300` | 36 | 36.36 | **100.00** |
| `0047b580` | `sound_wave_sample_load` | 317 | unmapped | 99.09 |
| `0047b6c0` | `sound_record_create_buffers` | 196 | 20.22 | **100.00** |
| `0047b790` | `sound_wave_sample_duplicate_buffer_create` | 336 | 11.63 | **100.00** |
| `0047b8e0` | `sound_streamed_wave_buffer_step_rate` | 131 | 24.66 | **100.00** |
| `0047b970` | `sound_streamed_wave_play` | 161 | unmapped | **100.00** |
| `0047ba20` | `sound_stream_stop_and_release` | 125 | 9.38 | **100.00** |
| `0047baa0` | `sound_streamed_wave_buffer_setup` | 395 | unmapped | 82.51 |
| `0047bc30` | `sound_stream_drain_check` | 383 | 12.77 | 84.77 |
| `0047bdb0` | `sound_stream_ring_buffer_tick` | 772 | 8.11 | 81.98 |
| `0047c0c0` | `vec_exact_equal` | 54 | 100.00 | **100.00** |
| `0047c100` | `ledger_stub_0047c100_DirectSoundListenerRolloffSet` | 82 | 68.18 | **100.00** |
| `0047c160` | `constant_negative_10000_get` | 6 | 100.00 | **100.00** |

29 functions are newly exact. Four functions are new: the listener callback
(`0047abc0`), the in-memory sample loader (`0047b580`), and the stream's play
(`0047b970`) and buffer setup (`0047baa0`). The port had no models of them.

## Two backends

The original calls DirectSound and Direct3DRM frames through their COM
method tables. The native port has neither: it plays through SDL
(`audio.cpp`'s player over `platform_audio_*`), keeps its sample bank in
`sound.cpp`, and its frames are not COM objects. The relocated file already
split the frame calls by compiler; this change keeps that split, one level
up (ADR 0006 step 4):

- **`SOUND3D_DIRECTSOUND`**, set unless `LEGORRR_BACKEND_MINIWIN`, compiles
  the original's code. That is the matching build and the MinGW `winapi`
  reference build.
- **Native** compiles the port's existing bodies, moved verbatim, in the
  `#else` branch of each function.

Each function has one `// FUNCTION` annotation over an `#if` group with both
bodies, Windows first. That is the shape `sound_volume_value_valid` already
had, and the annotation lint counts both bodies as annotated. Port-only
helpers sit at the end, inside `#if !defined(SOUND3D_DIRECTSOUND)`. The
matching build never sees the native branches; the native build never sees
the DirectSound code. `vec_exact_equal`, `sound_volume_value_valid`,
`audio_sound_volume_index_set` and `constant_negative_10000_get` have one
body for both.

The COM interfaces are declared in the unit: `IDirectSound`,
`IDirectSoundBuffer`, `IDirectSound3DListener`, `IDirectSound3DBuffer` and
the `IDirect3DRMFrame3` slots it calls (`+0x08`, `+0x1c`, `+0x60`, `+0x68`,
`+0x7c`). MSVC 5.0's `dsound.h` is DirectX 3's and has no 3D interfaces.
`DirectSoundCreate` is declared without `dllimport`, so the call goes
through the import thunk as in the original (`call 0049c7ae`). `GlobalAlloc`,
`GlobalFree` and `mmioClose` come from `winmm.h`, which includes
`<windows.h>` on Windows; they go through `call [__imp__...]`. The listener
and 3D-buffer IIDs are annotated statics at `004a04f8` and `004a0508`.

**Not verified:** the MinGW `winapi` build. No i686 MinGW toolchain is
installed here, and CI does not build it. The DirectSound branch is valid C++
by construction (no unprototyped calls, `malloc` results cast). It passes
`gcc -std=gnu89 -Werror=declaration-after-statement -fsyntax-only -Wall`
through `winmm.h`'s native declarations.

## State

- **`.bss`**: one static `Sound3DGlobs` at `005065b8`, 0x2d2c0 bytes. It
  holds the DirectSound objects, both stream buffers and their 0x5c-byte
  stream records, the playing-buffer list (`00506684`), the listener frame,
  the minimum and maximum distances, the volume and the sample table
  (`00506698`).
  - `s_bank_sound_on` (`00533878`, the initialised flag) stays a separate
    global for both backends, because `soundsys.c` (G98-021, all exact) and
    `sound.cpp` read it by name.
  - The rest, from `0053387c`, is a second static: the saved Windows volume,
    the 20 frames whose sounds are updated, and the listener's last
    position and orientation.
- **`.data`**: `s_sfx_attenuation` (`004aae30`, non-`const` like the
  original; `audio.cpp` reads it) and `"%s.wav"` (`004aae5c`, with a
  `// STRING` marker directly above the literal).
- **`.rdata`**: `0.0f` and `10.0f` (`0049e964`/`0049e968`) are literals; MSVC
  pools them.
- The `"r"` that `fopen` gets (`004a1eac`) is pooled with another unit, so it
  has no marker.

`make unit-data-offsets`, G98-009 row:

| Unit | .data | .rdata | .bss | Commons |
| --- | --- | --- | --- | --- |
| G98-009 | partial (2, 0 unknown) gap 1 first 004aae5c: 2c/30 | n/a (0, 0 unknown) fewer than two attributable items | partial (3, 0 unknown) gap 2 first 00533878: 2d2c0/2d348 | - |

MSVC lays out `.bss` statics by name hash (finding 512), so the 0x88-byte
second static lands before `s_bank_sound_on`. In `.data`, `"%s.wav"` lands 4
bytes after its original offset from the volume table. The unit summary is
full 0, gaps-only 2, partial 25, n/a 198 (G98-009 moves from n/a to partial).
The image grows by 188 KiB (192,512 bytes): the original's sample table is now in the
matching build, beside the port's own bank in `sound.cpp`.

## Contracts

`sound3d.h` and `audio.h` declare 20 of the unit's functions and two of its
globals for code outside the unit; all but `constant_negative_10000_get` have
callers. They keep the port's contracts; on Win32 the integer frame handle and the float
values are the original's arguments. Their C++ declarations gain
`extern "C"` in place (entropy rule 1). The unit's other functions are called
only from inside it, and the DirectSound bodies take the original's
arguments (frames, 3D buffers, sample records).

| Change | Where | Why |
| --- | --- | --- |
| `extern "C"` in place | `sound.hpp` (11), `audio.hpp` (3), `sound_shared.hpp`, `sound_for_sfx.hpp`, `simtick.hpp`, `view.hpp` (2), `crt.hpp`, `resource.hpp`, `resource_from_sound3d.hpp` (inside its namespace), `audio_from_sound3d.hpp` | C linkage for the unit and its callees |
| `sound_streamed_wave_buffer_step_rate` returns `float` | `sound.hpp` | the original loads a `float` 0.0 (`fld dword [0049e964]`) |
| `sound_tracked_buffers_set_min_distance` takes `float` | `sound.hpp`, `lego.cpp` | with the port's `uint32_t` bit pattern MSVC keeps the value in `edi` for the whole function, and `0047b4a0`/`0047b4d0` score 73.68% |
| `s_sfx_attenuation` is not `const` | `audio_from_sound3d.hpp` | the original's table is in `.data` |
| `0x00425c30` calls `sound_subsystem_stop_all` | `lego.cpp` | the original calls `0047b160` there; the port called `sound.cpp`'s association helper (38.71% → 40.86%) |

Callees:

- **New C boundary headers**, which only this unit and the boundary test
  include. `view.h`, `container.h` and `resource.h` are not extended, because
  other C units include them (finding 549):
  - [`gods98/files.h`](../../src/gods98/files.h): File_VerifyFilename
    (`resource_path_resolve_against_base`, `00480220`) and the adapter
    below;
  - [`gods98/container_frame.h`](../../src/gods98/container_frame.h):
    Container_Frame_SetAppData (`ddiscene_animation_hook_create`,
    `00475f70`).
- **One unannotated adapter** (ADR 0005 step 6), `resource_cd_file_path_get`
  at the end of `files.cpp` (G98-014, relocated by #528). File_GetCDFilePath
  (`0047f660`) takes `(out, name)`; the port's `ares_cd_fallback_path`
  takes `(name, out, size)`.
- `read_whole_file` (`004800a0`) was already in `resource.h`. The LIBC calls
  (`fopen`, `fclose`, `fseek`, `ftell`, `sprintf`, `malloc`, `free`) use their
  LIBC names (`crt/vc5names.h`).
- `sound.cpp`'s association-list helpers (`sound_buffer_association_*`) are
  now native-only (`#if defined(LEGORRR_BACKEND_MINIWIN)`): on Windows the
  list is the unit's. They sit after `sound.cpp`'s last annotated function,
  so no counter before an annotated function moves.

**For #521 (`view.cpp`):** the original pushes 11 arguments to `00475f70`;
the port's `ddiscene_animation_hook_create` takes 10. `0047b1a0` therefore
pushes one argument fewer (94.23%). This change does not edit `view.cpp`.

## Shape notes

- **Unsigned loop counters.** For the frame and sample loops MSVC turns the
  counter into a pointer compare and keeps the counter's signedness: `jl`
  with `int i`, `jb` with `unsigned int i`.
- **Locals in dead parameter slots.** MSVC 5.0 puts a local in a parameter's
  stack slot once the parameter is in a register: the scene in the two frame
  callbacks, `lpvAudio2` in `0047b790`, the stream buffer in `0047baa0`. No
  source shape is involved.
- **`0047abc0`** reuses one vector for the position and then the front
  direction. Three vectors make the frame 0x24 bytes, not 0x18 (78.90%).
- **`0047a640`** writes `WAVEFORMATEX` as tag, channels, bits, rate, then
  block align and byte rate derived from them, then `cbSize`. Constant
  fields in declaration order schedule the stores differently (96.99%).
- **`0047a870`, `0047ad70`**: `useName = fullName` comes before `fclose`. In
  `0047ad70`, `useName` is scoped to the streamed branch; at function scope
  its register swaps with `fullName` (93.06%).
- **`0047b580`** keeps the DirectX SDK's `WaveLoadFile` shape:
  `(nError = f(...)) != 0` and `goto ERROR_LOADING`. Only that form makes
  MSVC compare against a zero register (`cmp eax, edi`) rather than
  `test eax, eax` (96.36% → 99.09%). `nError` is a real variable of that
  code, not a declaration pad. `0047baa0` uses the same form for
  WaveStartDataRead (81.19% → 82.51%).
- **`0047b790`**: `dwBytes1` and `dwBytes2` start at 0 and `lpvAudio2` is not
  initialised (92.80% → 100%).
- **The silence fill** is `memset(p, (BYTE)(bits == 8 ? 128 : 0), n)`, as
  `FillMemory` would write it. Without the cast MSVC drops the
  `and ebx, 0xff`.
- **The stream buffer** is `(loop ? lpDSLoopStreamBuff : lpDSStreamBuff)`,
  written in full for both `this` and the method lookup. The original
  evaluates the selection twice per call, as a COM call macro does.
- **`0047bc30` (Sound3D_Stream_FillDataBuffer)** has no return statement on
  its success path. The original returns 0 when `Lock` fails and leaves
  `eax` as it is otherwise. For other compilers the function returns 1
  (`#if !defined(_MSC_VER)`), because flowing off a non-`void` function is
  undefined in C++.
- **`0047bdb0`** sets its first-pass flag to 0 right before the loop, not at
  its declaration (79.15% → 81.98%).

## Near misses

| Address | Score | What is left |
| --- | ---: | --- |
| `0047b580` | 99.09 | the call to `004800a0`: the port's `read_whole_file` has no annotation |
| `0047a640` | 99.25 | the DirectSoundCreate import thunk is not named on the original side |
| `0047a870` | 98.34 | the pooled `"r"` (two pushes) and the adapter call |
| `0047ad70` | 96.73 | the same two operands, and the return `slot * 3 + voice` loads its operands the other way round. That last part is rule B (finding 538): a sweep of 0–1,040 inert declarations has period 512, and the other phase (N ≡ 256–511) scores 95.51% without the swap but with the `"r"` and adapter residue. Inert declarations are not allowed (entropy rule 3) |
| `0047b1a0` | 94.23 | the 11th argument to `00475f70` (above) |
| `0047aad0` | 92.86 | the loop bound `005339a8`, one past the sample table, is not inside any annotated variable on the original side |
| `0047bc30` | 84.77 | register choice: the original loads `bufferSize` before choosing the buffer, keeps the selection in `edi`, and computes the wrap-around `offset - size` through `eax`/`ecx` moves |
| `0047baa0` | 82.51 | the original keeps `pwf` in `ecx` and re-reads it for `lpwfxFormat`; this build caches it in `ebx`. Five rounding shapes change nothing |
| `0047bdb0` | 81.98 | register choice. The original re-reads both progress counters at the loop's bottom test, where this build carries `writeProgress` across the back edge. An `if`/`do`–`while` (79.35%) and Ghidra's `while (!playEnd \|\| first)` with an inner return (78.64%) are worse, and so is computing the delta into a temporary first (73.50%) |

The rule B sweep (`tools/experiments/486/sweep.py`, N = 0–1,040, step 4)
found one code for each of `0047b580`, `0047b790`, `0047baa0`, `0047bc30`
and `0047bdb0`, so their residue is not the counter's phase.
`tools/msvc_permuter.py` refuses this unit: it cannot locate a function body
under an `#if` group ("cannot safely locate function body").

## Entropy

`make reccmp-check` against #526's head `1e6cf9ae`:

- **35 improved and 4 newly mapped**: all in the unit except `0x00425c30`
  (source changed, above);
- **one drop**, flagged below;
- **no exact match lost**: exact functions 814 → 843.

Every function outside the unit whose score or listing digest changed:

| Function | File | Score | Instructions | Label |
| --- | --- | --- | --- | --- |
| `0x00425c30` ledger_stub_00425c30_TransitionControl | `game/lego.cpp` | 38.71 → 40.86 | = | source changed: it calls `sound_subsystem_stop_all`, the original's target. The digest is unchanged, because its normalisation hides call targets |
| `0x004295f0` media_min_distance_update_mode | `game/lego.cpp` | **14.58 → 9.52** | −6 | `source-changed`: passes the minimum distance as a `float` |

**For the owner:**
- `0x004295f0` falls 5.06 points and loses 6 instructions. The port's model
  of it is far from the original (the original is 0xb5 bytes of calls the
  model does not make). Its old score came from the `media_float_bits` calls
  lining up with the original's. The `float` contract makes `0047b4a0` and
  `0047b4d0` exact (73.68% → 100% each). Byte-weighted, that outweighs the
  drop: +26.3 points on 45 bytes against −5.1 on 163. `media_float_bits` is
  now unused; its declaration stays, to keep `lego.cpp`'s counter.
- The only instruction-count change outside the unit is that −6.

All other header edits are `extern "C"` in place, a return type, a parameter
type and a `const`. None adds or removes a declaration or a parameter, so no
includer's counter moves, and no other score or listing digest changed.

## Lint ratchet

4,121 → 4,118 entries.

- Removed (4): `sound_frame_scene_get`, `sound_frame_position_get`,
  `sound_frame_release` and `sound_buffer_position_set`. These were the MSVC
  `__asm` copies of the port's frame helpers, which only native compiles
  now, and native is never MSVC. The GCC copies remain; the lint never
  detected them, after their `__attribute__((thiscall))` typedefs.
- Added (1): the `resource_cd_file_path_get` adapter, which ADR 0005 step 6
  requires unannotated. The update needed `--allow-growth` for this one key
  even though the total shrinks, as in findings 549 and 550.

## Native behaviour

Unchanged. The native branches are the relocated bodies, with three
contract edits:
- the step rate returns `float`;
- the minimum distance arrives as a `float` and is stored as its bit
  pattern, as before;
- the pause path in `0x00425c30` now also stops the two streams and the
  loop track, as the original's Sound3D_StopAllSounds does.

`ctest` passes 37/37.

## Tooling notes

- **`convert_unit.py` refuses a relocated unit**, as in findings 544, 549
  and 550. The scaffold was run in a scratch export with the `.cpp` entry
  removed from `src/link_order.cmake`.
- **The scaffold needs `reference/LegoRR.exe`**, which a fresh worktree
  lacks (symlink it).
- **`msvc_permuter.py` cannot parse an annotation over an `#if` group.**
  Every function in a two-backend unit has that shape.
- **`sweep.py` and `match_function.py --candidate`.** To score a sweep
  variant, put the inert declarations on one existing line; the tool keys
  annotations by line number.
- **`make match-progress`** needs `pefile`; a worktree-local `build-venv`
  (`--system-site-packages`) provides it. `translation_units.py build`
  rewrites `translation-units.csv`, which is a hashed input, so run it
  before `make match-progress`, or refresh the baseline and binary progress
  again afterwards (no rebuild needed).

## Validation

- `make reccmp-check` (MSVC 5.0, developer distrobox) against `1e6cf9ae`:
  fails only on the flagged `0x004295f0`. The baseline is refreshed with it; link-order map
  OK.
- `make match-progress` regenerated the baseline, listing digests and
  `binary-progress.json`. `pr_match_progress.py check --base
  origin/relocate/521-game-023 --head .` reports them fresh.
- `make unit-data-offsets` gives the G98-009 row above.
- `python3 tools/translation_units.py build`: 0 conflicts.
  `tools/binary_progress_inputs.txt` was regenerated.
- `make tools-test` passes: Meson parity, link order, and the lint (above).
- `sound3d.c` passes `gcc -std=gnu89 -Werror=declaration-after-statement
  -fsyntax-only -Wall`. Its one warning is the integer-to-pointer cast of
  the port's `int` frame handle on a 64-bit host.
- `cmake -S . -B build && cmake --build build && ctest --test-dir build
  --output-on-failure`: 37/37 pass, including `c_boundaries`, which now
  pairs `sound3d.h`, `files.h` and `container_frame.h` with their C++
  headers.

## Reproducing

    python3 tools/convert_unit.py G98-009 --stem sound3d --dry-run   # in a copy without the .cpp link-order entry
    make match-function SOURCE=src/gods98/sound3d.c
    make reccmp-check
    make match-progress BINARY_PROGRESS_PYTHON=build-venv/bin/python3
    make unit-data-offsets
