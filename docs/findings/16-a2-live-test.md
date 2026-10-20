# 16 — A.2 Live Test: The Game Launches Under GE-Proton

**Target:** `source/LegoRR.exe` (SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`)
**Host:** Bazzite 44 · RTX 3080 Ti (Vulkan 1.4) · gamescope 3.16 · GE-Proton10-25
(wine 10.0 Staging) via the Lutris `ge-proton` runner
**Run log (excerpt evidence):** `~/.local/share/rock-raider-a2/logs/run-20261020-144935.log`
(1,481,211 lines, 25 s automated session under `WINEDEBUG=+ddraw,+winmm`)

## Verdict

`python3 tools/compat/a2.py run` reaches real DDraw traffic: ~467 k trace lines,
6,324 `ddraw_surface_create` calls, texture-init activity — the shipped binary
renders through the translation layer exactly as finding 15's wrapper matrix
predicted. **The answer to "wrapper or port" is wrapper.**

The launch also resolved three static unknowns, by observation:

| Finding-15 item | Runtime answer | Evidence |
| --- | --- | --- |
| §5.1 COM GUID versions | The binary QIs `DirectDrawCreate`'s object to **`IID_IDirectDraw4`** (GUID `9c59509a-39bd-11d1-8c4a-00c04fd930c5`), after a first hop through `IID_IDirectDraw`; surfaces requested as v1/v3/v4/v7 variants | log: `ddraw1_QueryInterface … riid {9c59509a-…}` + `ddraw_surface_create … version 4.` |
| §5.2 D3DRM device flavor | Not directly traced, but rendering works through the bundled `D3DRM.DLL`'s DDraw path — no D3D device errors, no `wined3d` D3D calls in the log | absence of any `d3d*:` component output |
| §4.1 CDDA music | **Not yet exercised** — an unattended 25 s session never reaches level load, so the MCI call was never issued (`cdaudio`: 0 occurrences). Still a live question | `grep -ic cdaudio` → 0 |
| §4.2 Movies | Movies play (or attempt) via wine's **quartz** graph, not `avifil32`: `quartz:send_buffer … hr 0x80040211` ×2 = a decoder pin failing to deliver samples, consistent with the old-codecs friction predicted. `avifil`: 0 occurrences | `err:quartz:send_buffer` |

## Environment engineering the binary itself required

Three host-side facts, none about the game:

1. **The runner's bare `wine` binary cannot run this build at all** — this
   GE-Proton tree's ELF companion modules (`lib/wine/*-unix/*.so`) are absent
   or stubbed, so any PE that loads `kernel32` fails `c0000135` (`wine:` and
   `wine64:` both). The **`proton` script at the runner root is the only
   working entry point**; it wires sysroot/LD paths the bare binary lacks.
2. **The prefix must be Proton-native.** Proton's `copy_pfx` refuses to
   upgrade a prefix it didn't build (`FileExistsError` on
   `Program Files\Windows Media Player\wmplayer.exe` symlinks), so the
   harness keeps a `version`-marker check and moves seed prefixes aside.
3. **gamescope must not inherit wine's `LD_LIBRARY_PATH`** — with it set, the
   compositor fails its own Wayland/DRM init (`corrupted double-linked list`
   class of crash). The harness therefore exports wine vars *inside* the
   wrapped command, never around `gamescope` itself.

## Known-benign noise observed

- `waylanddrv … zwlr_data_control_manager_v1` — clipboard nicety, wine side.
- `keyboard_handle_keymap … Xkb compose table` — deadkeys only.
- `service:scmdatabase_autostart … failed to start: 2` — services.exe stubs.

## Open questions for the interactive session

1. Music: does `FUN_00489210`'s `mciSendStringA("play cdaudio…")` fail silently,
   show an error dialog, or hit the 4 s watchdog loop (finding 14)?
2. Title screen: do the intro AVIs show (quartz errors above may mean silent
   black frames with gameplay otherwise fine, or vice versa)?
3. Exclusive-mode behaviour at 640×480 under gamescope (vs windowed `--no-gamescope`).
4. Mouse grab: DInput was traced 62×; responsiveness/unclickability need eyes.
5. Palette-tile fidelity check on `World\WorldTextures\*Split*` (finding 15 §4.5).

### Confidence

| Claim | Status |
| --- | --- |
| Game launches and drives DDraw under GE-Proton10-25 | observed (user-confirmed launch; 1.48 M-line trace) |
| Interface climbing to IDirectDraw4 | observed (GUID literals in log) |
| Movies via quartz, one decoder sample error | observed |
| CDDA behaviour | **open** — code path not yet reached |
| Interactive correctness (menus, input, levels) | **open** — first manual session pending |
