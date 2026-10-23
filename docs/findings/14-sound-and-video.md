# 14 — Sound & Video: WAVE via `mmio`, CD-Audio via MCI, AVI via AVIFIL32

*Target: `LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`), Ghidra 12.1.4 via REA.*

## Summary

The game adds no custom containers for audio or video — it uses three
Win98-era OS facilities and plain files:

| Media | Format | OS API | Evidence |
| --- | --- | --- | --- |
| SFX / voices | `.wav` (RIFF) | `WINMM!mmio*` + DirectSound | import xrefs `0x488cd2–0x489113`, `%s.wav` `0x4aae5c` |
| Music | audio-CD tracks | `WINMM!mciSendStringA` | `"play cdaudio from %i"` `0x489210` |
| Movies / logos | `.avi` (AVI1) | `AVIFIL32!AVIStream*` | module `FUN_0047e800` |

`DSOUND.dll` is imported via ordinal 1 (`DirectSoundCreate`) at thunk
`0x49c7ae`, called once from `FUN_0047a640` during sound-system init
(`FUN_00488b50`). `WINMM.dll` additionally exposes `mmioGetInfo/mmioDescend/
mmioRead` (RIFF chunk walk) and `mciSendStringA/mciGetErrorStringA`.

## 1. The sound bank — 600 slots, `FUN_0047a870` (SoundLoad, 597 B)

`SoundLoad` (only xref of `%s.wav` at `0x4aae5c`) drives a fixed **sound
table**: records of stride `0x134`, from `DAT_00506698` with the busy-flag
word base `DAT_005067c8`, scanned to `DAT_005339a8` —
`(0x5339a8−0x5067c8)/0x134 = 600` slots (flag bit `+0x130 & 1` = used).
Slot fields seen: `+0x104` (state), `+0x108` (size), `+0x130` (flags: bit 2 =
**looping**, bit 3 = **memory/stream mode**).

Flow:

1. Refuse unless `DAT_00533878 != 0` (sound subsystem on; `SoundOn` config
   key `0x4a3858` feeds it — and benchmark profiles `LoaderProfileNoSound.txt`
   `0x4a3ab0` switch it off entirely).
2. `FUN_0047aad0` claims a free slot index.
3. Build `<name>.wav` and load:
   * `param_2 == 0`: in-memory load via `FUN_0047b580` — path check through
     the WAD layer (`FUN_0047f660`), open `FUN_0048eb90(…, "r" DAT_004a1eac)`,
     RIFF parse by `FUN_00488e70` (mmio-based, §2); loop mode sets flag bit 2.
   * `param_2 != 0`: register the file for *streamed* playback (size via
     `FUN_0048f2b0`, `FUN_00488e70` parse for format; flag bit 3 set) — used
     for the 387 `.wav`s in the shipped archives, including the voiced lines.

Consumers: `FUN_00464d00` (sound bank load at level init via `FUN_00434a50`)
and `FUN_00452e70` (gameplay trigger). Per-sound records carry a sound handle
+ cooldown float (finding 04 §…); `FUN_00464ef0(sound, v)` scales volume, and
3D placement uses config keys `RollOffFor3DSounds` / `MaxDistFor3DSounds` /
`MinDistFor3DSoundsOnTopView` (`0x4a393c–0x4a3964`). The air-lock warning
loop (`EngineSound` `0x4aa340`) is a persistent 3D instance from finding 03.

## 2. RIFF/WAVE parsing (`mmio` region `0x488c00–0x489330`)

* `mmioDescend` ×4 sites (`0x488cd2 / 0x488e8b / 0x488f47 / 0x4890ed`) and
  `mmioRead` ×3 (`0x488d2b / 0x488edd / 0x488fa0`) — the classic
  `RIFF → fmt → (fact) → data` chunk walk; `mmioGetInfo` binds the file buffer
  at `0x489113`. `FUN_00488e70` = "open WAV by mmio handle, return data
  length/params".
* Everything routes through the same disk-or-WAD open layer as models/config
  (findings 11/12) — `.wav` files inside `LegoRR*.wad` load transparently.

## 3. Music = audio CD, restarted by a watchdog

`FUN_00489210` (80 B) issues MCI `"play cdaudio from %i"` (`wsprintfA` +
`mciSendStringA`); errors are stringified via `mciGetErrorStringA`
(`FUN_00489260`). `FUN_00488c00` runs from the in-game tick (`FUN_004297b0`,
finding 03): every `timeGetTime() − DAT_00544a10 > 4000` ms it re-checks
track `DAT_00544a00` (`FUN_00489280` = MCI status) and re-issues playback if
the track stopped or playback advanced beyond the selected track, then fires
callback `DAT_00544a08`. The `play` command has no end-track argument, so MCI
plays toward the end of the disc; finding 180 records the conditional brief
spill into a following track before the watchdog notices. **Level music is a
CD-DA track number** with a 4-second restart check. No music files exist in
the archives; the "music/CD" panel strings (finding 04) match.

## 4. Video — `AVIFIL32` streaming module (`FUN_0047e800`, 376 B)

The single AVI module (its 5 OS calls sit in the `0x49b6d4–0x49b6ec` thunk
row, all owned by `FUN_0047e800`'s body region): `AVIFileInit`,
`AVIStreamOpenFromFileA`, `AVIStreamInfoA`, `AVIStreamGetFrameOpen`, then
frame pulls at `AVIStreamGetFrame` (`0x47eb52`) and `AVIStreamRelease`
(`0x47e9ba`); `timeGetTime` drives the clock. Wrapper `FUN_0047ec90` =
"open movie(path)" ← called from `FUN_0040a0c0` (message/dialog command
router region — AVIs are triggered by config-referenced commands).

Config surface (values from `Lego.cfg`):

| Key | Value / role | Ref |
| --- | --- | --- |
| `RRAvi` | `avi\intro.avi` — startup movie | key `0x4a3b38`, reg `FUN_00415910` (`0x420316`) |
| `LegoAvi` | `avi\lmi640.avi` — LMI logo movie | `0x4a3b5c`, `0x4202e4` |
| `DDIAvi` | `avi\DDI.avi` (optional) | `0x4a3b40` |
| `IntroAVI` | per-level movie at load | read in `LevelLoad` `FUN_00429890` `0x429fb1` |
| `CreditsBackAVI` | `AVI\Loop.avi`, credits backdrop | `0x4a3c0c` |
| `EndGameAVI1/2`, `ObjectiveAcheivedAVI` | endings / objective stingers | `0x4a4398/0x4a43a4/0x4a81ac` |
| `DontPlayAvis` | master kill switch | `0x4a24ec` |
| `DDILogo(Time)`, `LMILogo(Time)` | timed **BMP** splash, not AVI | `FUN_004157c0` uses `timeGetTime`+blit |

Startup sequence (in the big init `FUN_0041fb50`): play `LegoAvi` if the
device path checks pass, show `DDILogo` for `DDILogoTime`, then `DDIAvi` and
`RRAvi`; each gated by `FUN_00415910(key, enable)` → config lookup →
file-exists check (`FUN_0047fb60`) → registration.

## 5. Adjacent: FLIC/.flh overlays

Not part of AVIFIL32: the `Reward*Flics` config group and `.flh` overlay
entries (`avi\PB1.flh:SFX_NULL:488:0` style, `!Overlay*` dev switches) drive
the renderer lane documented in finding 09 (`FUN_00483c80`/`FUN_00484070`
playback region). The `.flh` container is documented in finding 21.

### Function map

| Address | Working name |
| --- | --- |
| 0x47a640 / 0x488b50 | DirectSound init and 3D listener QueryInterface (finding 119) |
| `0x47a870` | SoundLoad (`%s.wav`, slot alloc) |
| `0x47aad0` | SoundSlotClaim (600 × `0x134` records) |
| `0x47b580` | SoundLoadInMemory |
| `0x488e70` | RiffWavParse (mmio) |
| `0x488c00` | CddaWatchdog (4 s loop keep-alive) |
| `0x489210` / `0x489280` | MCI play / MCI status |
| `0x47e800` | AviStreamModule |
| `0x47ec90` | MovieOpen |
| `0x415910` / `0x4157c0` | RegisterMovie(key) / timed-logo splash |

## Notes and open items

1. DirectSound 3D listener acquisition is documented in finding 119. Finding 121 traces the
   per-source descriptor's 3D-control bit and configured distance setters; finding 161 maps named
   gameplay aliases submitted through positional and object-attached wrapper routes, finding 162
   traces spatial-state refresh for attached objects, finding 163 traces stored view vectors into
   listener position/orientation setters and their frame-tick call, finding 164 shows explicit
   source-position writes are deferred then committed in the same dispatch, and finding 166 inventories
   direct dispatcher callers. Findings 170–171 trace conditional shared-object transition handle issues across immediate and queued submission; finding 178 records the dynamic alias-table capacity during sample loading. Indirect 3D submission paths remain open. Finding 120 documents optional duplicate
   sample buffers, and finding 133 traces playback's single-buffer versus three-buffer rotation.
2. Finding 122 traces the in-memory WAV path from the `data` chunk through `mmioGetInfo`'s read
   window into a `GlobalAlloc` buffer before DirectSound setup. The shared file/WAD open layer is
   documented in finding 11; runtime WAD-backed loading and Windows' internal file access remain
   unverified.
3. Finding 123 traces `AVIStreamGetFrame` output into a row-copy helper and the surface pixel
   buffer. The helper walks source rows backward while advancing the destination; bottom-up to
   top-down conversion is an inference. Finding 172 traces each frame blit into the shared
   DirectDraw render target; final device presentation remains dependent on the surrounding renderer.
   Findings 173–177 record the unchecked null-allocation path, missing explicit frame-retriever close, stale-surface blit after a null frame fetch, caller-level restart at stream length, and the video-only AVI stream selection.
4. `.flh` container format — **closed** by finding 21 (2026-10-20): layout,
   both 16-bit coders, and the RGB555→565 write transform.
5. MCI: only `play cdaudio from N` and status queries were observed; no
   volume/`set cdaudio` commands in the binary.
