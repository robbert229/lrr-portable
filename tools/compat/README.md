# `tools/compat/` — A.2 live-test harness

Turns the static compat audit ([finding 15](../../docs/findings/15-compat-audit.md))
into runtime evidence by driving the shipped `LegoRR.exe` under Proton/Wine
with automatic log capture and signature triage. One command per test run:

```sh
python3 tools/compat/a2.py doctor      # is the box ready? (runner, Vulkan, digests)
python3 tools/compat/a2.py setup       # stage the game (source/ is copied, never edited)
python3 tools/compat/a2.py run         # launch, stream + save log, triage on exit
python3 tools/compat/a2.py triage      # re-scan the newest (or a named) log
python3 tools/compat/a2.py patch-avi   # flip DontPlayAvis TRUE in the staged wad
python3 tools/compat/a2.py unpatch     # restore original wads from backups
python3 tools/compat/a2.py shell …     # arbitrary exe/command under the prefix
```

## How it runs the game

- **Runner** — discovery order: `$RR_A2_PROTON`, Steam/ProtonUp-Qt `proton`
  scripts, Lutris wine runners (this box: `ge-proton`, wine 10.0 Staging),
  `$UMU_RUN`/`umu-run`. First run bootstraps the prefix; `winecfg.exe` via
  `setup --winecfg` or `shell winecfg.exe` for manual tweaks.
- **Prefix/game state** — under `${XDG_DATA_HOME:-~/.local/share}/rock-raider-a2/`:
  `compatdata/pfx/` (prefix), `game/` (staged copy of `source/`, plus
  `.mods/L1/` as the repack working tree), `logs/`, `backups/`.
- **Display** — wrapped in `gamescope` at 640×480 @ 60 Hz when available
  (the game is hardcoded for that mode, finding 15 §4.3); `--no-gamescope`
  opts out.
- **Logs** — `WINEDEBUG=+ddraw,+winmm,-fixme-all` by default (the two
  channels that answer the finding-15 friction list); override the env var
  to widen (`+all` during crash triage).
- **Integrity gate** — staged `LegoRR.exe` must hash to the analysis digest
  `c3d8ce23…e0f`; a mismatch is reported loudly because all doc evidence
  presumes that build.

## Triage signatures

Every run's log is scanned for exactly the finding-15 predictions:
`cdaudio` (music is dead — expected, finding 14), `err:module:import_dll`
(import surface gap), `err:ddraw`/`fixme:ddraw` (translation misses —
candidate for a dgVoodoo `ddraw.dll` override), AVI decode chatter
(`patch-avi` escape hatch), `d3drm` (bundled DLL active), DInput, install
data checks, crashes. A run with *no* signature hits is a result too — the
table is a cross-check, not a checklist that must fill up.

## Verdicts we're collecting (map into finding 16 later)

| Question | Where answered |
| --- | --- |
| Does DDraw/D3DRM render through DXVK's translation? | first `run` + visual check |
| Which `IDirectDrawN` does the bundled d3drm query? | `+ddraw` log GUID chatter (§5.1) |
| HAL vs software d3drm device? | perf + `+d3d9`/DXVK log (§5.2) |
| CDDA failure mode (silent? error dialog spam?) | `+winmm` log, watchdog behaviour |
| Do the shipped AVIs decode, or need `patch-avi`? | `+avifil`/`vfw` lines + eyeball |
| Exception-handler reporting (`SetUnhandledExceptionFilter`) | any crash's dialog/log |

## Patches

`patch-avi` demonstrates the mod loop end-to-end: staged wad →
`.mods/L1/` tree (the one `rrwad.py` understands) → `Lego.cfg` value flip
(the shipped file contains `DontPlayAvis FALSE` verbatim) → byte-level
repack with backup under `backups/`. The same shape is the template for the
bigger CDDA patch when we decide which sound backend to swap in.
