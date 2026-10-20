# 17 — Interactive A.2 Session: Playable, Music Works, Movies Blocked on Indeo 5

**Target:** `source/LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`)
**Session:** manual play through the first mission, `a2.py run` under GE-Proton10-25
(wine 10.0 Staging), gamescope 640×480 @ 60, exclusive mode
**Session log:** `~/.local/share/rock-raider-a2/logs/run-20261020-145826.log`
(83,249,722 lines under `+ddraw,+winmm`)

## Verdict

**The game is playable start-of-mission-1:** menus click, mouse responds, the
first mission completes, exclusive 640×480 works. Finding 15 §3's mouse-grab
question resolves to working. The remaining friction is codec, not API: movies.

## Music (answers finding 14's open item, finding 15 §4.1)

The soundtrack is audible and behaves like rippled CD audio — fades and loops.
But the CDDA code path is **not what plays it**, on this install:

| Evidence | Observation |
| --- | --- |
| 83 M-line session log | `cdaudio` / `mci`: **0** occurrences |
| Prefix drive map (`compatdata/pfx/dosdevices`) | `c:`, `z:` only — no CD-ROM drive |
| No `.iso`/`.cue`/`.bin` in the staged tree or prefix | no emulated CD media |
| Staged audio tree `Data/Sounds/Streamed/` | 590 WAV files, incl. **`Atmosdel.wav`, 16 MB / 91 s** — a real music track; `InGame/` holds ~9 s ambient loops |

The digital release replaced CD-audio tracks with streamed WAV files played
through DirectSound/`waveOut` (traced: `waveOutGetNumDevs count: 4`). The
CD-era `FUN_00489210` "play cdaudio from %i" music call has no media to
address and evidently isn't taken (or fails the media check silently) —
**no audio patch is needed for this distribution**. A CDDA patch would only
matter for the physical-CD retail media.

## Movies (answers finding 15 §4.2 more precisely than finding 16)

Title movie (`Data/AVI/lmi640.avi`, staged correctly — user-identified) does
not appear; the game drops straight to the loading screen. The log names the
reason outright, ×3:

```
Missing decoder: Intel Indeo 5 (video/x-indeo, indeoversion=(int)5, ...)
```

That string is wine's **gstreamer** layer reporting a missing plugin — i.e.
the AVI container opens and the decoder negotiation fails, so the game skips
the clip. `avifil`: 0 occurrences, `quartz`: 7 — the decode attempt never
reaches wine's vfw/avifil code. `ffprobe` on the staged file confirms
`codec_tag_string=IV50` (Indeo 5, 640×480). Indeo 5 was never redistributed
for 64-bit/modern Windows, so there is no system decoder for wine to borrow.

Options, by honesty about effort:

1. **Accept the skip.** Gameplay is unaffected. `patch-avi` (flip
   `DontPlayAvis` in the staged wad) would silence the attempts entirely.
   It is currently *not* needed for playability.
2. **Get an IV50 decoder into the container** — install
   `gstreamer1-libav`/`gst-plugins-ugly` class packages (or equivalent
   inside the Proton runtime sysroot) so the missing-plugin message goes
   away. Untested here; the failure point is host-side, not the game.
3. **Transcode** the AVIs to a codec wine ships (e.g. Microsoft RLE or
   uncompressed RGB) with ffmpeg, in `game/Data/AVI/`. Loose files, easy to
   revert, fits the staging model. What wine's pipeline would accept needs a
   probe (`gst-inspect-1.0`/playbin), so this is a tooling step, not a
   15-minute job.

## Host-driver incident (record so a future "won't launch" isn't misdiagnosed)

At 15:06 the NVIDIA driver entered a recovery state independent of the game:
kernel `Xid 51` (BAD_TSG) then `Xid 154` requesting **PF FLR**, after which
*every* Vulkan client failed `vkCreateDevice` with `VK_ERROR_INITIALIZATION_FAILED`
(`-3`) — gamescope alone with `-- true` reproduced it. An earlier `Xid 31`
(MMU fault, attributed to a `gamescope` pid) had logged at 13:54. A reboot
resolved everything; the game was never the cause. If `run` ever exits
`-6` in gamescope's init again, check `journalctl -k | grep Xid` before
suspecting the harness. **Note:** this state likely overlapped the 14:58
session's slow-performance report; the verdict was re-judged after the clean
reboot (good enough), with dgVoodoo kept as a documented future avenue only.

## Findings-index updates implied here

- 15 §3 mouse: **works**
- 15 §4.1 CDDA: **moot on digital media** (no CDDA media present; streamed WAV path instead)
- 15 §4.2 movies: **blocked on Indeo 5 decoder**, container path is gstreamer, game skips gracefully — **resolved in 18** (plugin deps, not codec availability)
- 16 §open-1 music: answered above
- 16 §open-2 title movie: answered above (file is `lmi640.avi`, IV50)
- 16 §open-4 mouse + §open-3 exclusive mode: answered (works)

### Confidence

| Claim | Status |
| --- | --- |
| First mission completable, menus/mouse work | observed |
| Music audible; not via CDDA (no CD media; streamed WAVs) | observed (log + filesystem) |
| Movies skipped due to missing Indeo 5 decoder | observed (`Missing decoder:` ×3, ffprobe IV50) |
| Perf verdict | **good enough as observed** (clean-boot sessions, 2026-10-20). Deferred future work: dgVoodoo as an alternate translation path if ever wanted |
| CDDA behaviour on physical-CD media | open — would need retail media to test; no evidence required for this distribution |
