# 18 — Movies Fixed: the Indeo Block was a Missing Library, not a Missing Codec

**Target:** `source/LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`)
**Supersedes** finding 17's "movies blocked" verdict. The game has always tried
to play `Data/AVI/*.avi`; the decoder it was reaching for existed all along —
its plugin just couldn't load.

## Root cause chain (each link evidenced)

1. Wine plays AVI video through its gstreamer bridge, whose ffmpeg codec
   provider is the sysroot's `libgstlibav.so` — the *only* element that can
   decode Indeo 5 (`ffprobe`: title movie `lmi640.avi` is `IV50`).
2. The registry wine actually built for this prefix
   (`compatdata/gstreamer-1.0/registry.i386.bin`) contains **zero** `libav` /
   `avdec_*` / `vpxdec` feature entries — the plugin never registered.
3. Why: `ldd` on that plugin, run against the sysroot's own library path,
   reports `libbz2.so.1.0 => not found` and `libvpx.so.6 => not found`.
   Proton-GE bundles the ffmpeg libraries (`libavcodec.so.58.91.100`, which
   contains `indeo5` — confirmed via `strings`) but *not* these two of its
   dependencies, so `dlopen` fails and gstreamer silently drops the whole
   plugin — codecs and all. The game then gets a flat `Missing decoder:`
   error (`×3` in the 14:58 session log) and skips the clip.
4. The host provides both: `/lib*/libbz2.so.1.0.8` and `libvpx.so.9.1.0`,
   with ABI-stable symbol tables for the needs of these old consumers.

## Fix (now automated)

Alias the two sonames inside the runner's `files/lib/{i386,x86_64}-linux-gnu/`
to correctly-classed host copies, and delete the stale gstreamer registry so
wine rescans on next launch. After that, **movies play** — verified in-session
(the user report "the videos loaded!") and by probe: `Missing decoder` drops
from ×3 to 0 across a 60 s run.

`a2.py` performs this in `setup`/`run` (`ensure_gst_libav_deps`): idempotent,
skips when links exist, reads the ELF header (byte 4) to pick the
32-bit-vs-64-bit host copy, clears registries only when it changes anything.

## Caveats kept honest

- The vpx alias jumps **two soname generations** (`libvpx.so.6` → `.so.9`).
  Indeo never touches vpx, so movie playback is unaffected; if any future
  run exercises VP8/VP9, judge it separately and replace the link with a
  real `libvpx.so.6` if symptoms appear.
- The fix lives in the Lutris runner tree, so a runner update (re-extracted
  `files/`) removes the links — `a2.py` re-creates them, that is why it is
  in the tool and not a one-off.
- The runner's `files/lib/x86_64-linux-gnu` registry (`registry.x86_64.bin`)
  matters for 64-bit helpers only; both arch directories get links so either
  way the plugin resolves.

### Confidence

| Claim | Status |
| --- | --- |
| Indeo 5 present in bundled ffmpeg | observed (`strings` on libavcodec) |
| Plugin dropped due to 2 missing deps | observed (`ldd`, registry feature counts) |
| Aliasing + registry rebuild restores movies | observed (×3 → 0; user-confirmed playback) |
| VP8/VP9 unaffected by vpx alias | **open** — untested, not needed by this game |
