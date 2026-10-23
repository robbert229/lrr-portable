# Finding 159 — Music Volume Targets a Compact-Disc Mixer Source

The `Music_Volume` callback eventually calls `FUN_004789c0`. That helper enumerates WinMM mixer devices and connected source lines, but it proceeds only when a source line's `dwComponentType` equals `0x1005`. Windows defines `0x1005` as `MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC`, a source for audio from an internal audio CD. It then reads or writes that line's volume control.

This is distinct from the streamed WAV music path: the shipped `SFX_MusicLoop` and `SFX_AmbientMusicLoop` aliases create and play DirectSound buffers (findings 144 and 150). The code therefore routes the slider to an audio-CD mixer source, not to those streamed buffers. The digital release uses streamed music and carries no CDDA media (finding 17), so this callback does not directly control the music playback path used by that release. The separate `SFX_Volume` setting targets the primary DirectSound output (finding 158).

| Setting or playback path | Target |
| --- | --- |
| `Music_Volume` | WinMM mixer volume control on source lines with component type `0x1005` (audio CD) |
| Shipped streamed music aliases | DirectSound secondary buffers |
| `SFX_Volume` | Primary DirectSound buffer |

The static analysis establishes the target type selected by the callback. It does not establish which mixer devices or controls exist on a particular host, or how those devices route audio.

**Evidence.** Shipped `LegoRR.exe`, SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`. The callback and mixer-line enumeration are in `FUN_00414df0` and `FUN_004789c0` in `docs/decompiles/full/LEGORR.full.c`; the Options callback is recorded in `docs/evidence/legorr-options-context-expansion.bundle` (`ev_b90345d663c586434f1d1fb120cddc3733c5844841dfa6161c72a85d7cfa6ee5`). Microsoft defines [`MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC`](https://learn.microsoft.com/en-us/windows/win32/api/mmeapi/ns-mmeapi-mixerline) as an internal audio-CD source. The streamed alias mapping is documented in finding 144; the digital-release audio path is summarized in finding 17.
