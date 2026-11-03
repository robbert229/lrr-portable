# Win32 startup scene integration — 2026-11-02

The first startup/menu repair is validated on PE32/MSVC5 candidate
`167cbe7d1d1ba5782e416c5ffa789fbbcb2dfb764bf706e4d4fe6f68c7ed87a7`.
Normal `make reccmp-run` accepted fullscreen 640×480×16, created the application
surface, initialized real root/camera/light owners, displayed the main menu,
and responded to mouse actions through Training Missions and the first tutorial
picker. Clicking **1. Moving Rock Raiders** then crashed in an entity Stats
reader. Tutorial load, selection/movement, pause/resume and gameplay quit remain
unverified. This is a startup/menu result, not complete gameplay acceptance.

Evidence packet: `build-reccmp/runtime-evidence/20261102T205420Z-startup-integration/manifest.json`.
All packet paths below are relative to that directory. No delegates, commits,
pushes or PRs. Native64 excluded. Pause/movement patches remain unintegrated.
The unrelated miniwin dirt and preexisting root evidence notes were preserved.

## Frozen input, corrections and review

Base `dc7f167b1a4b19efe65a35fcb5523fdf61e62fc1`. The patch and identity JSON were
copied before inspection into read-only `captured/`, with SHA256SUMS:

- Captured/final-author patch: `6a81d0689f7b47d95cb3065710c198698cce09892f30434bf06b10153c43544e`.
- Final integrated patch: `398fe15ce22872dfef910053db8cfad1283a3e71980a312a696370fd0695170b`.
- Complete source patch: `final-source.patch`; integrator correction only:
  `loader-resolution-correction.patch`. Eight startup source files only.
- Final EXE/PDB/reports/source hashes and build log: `attempt-4-build/`.
  Source hashes remained identical during candidate-4/5/6 and final verification.

`git apply --check` passed, then only the captured complete patch was applied.
The author's final handoff and final patch identity matched that capture.
The patch supplies real RM3 factory/frame/light allocation, scene pool ownership,
AppData records, the three-frame parent contract and actual init callback wiring
before LightEffects config. Pose wrappers now pass wrapper records rather than
addresses of local wrapper pointers. The annotated normalizer body is unchanged.
No dummy root, missing-root bypass, fake dialog, disabled renderer or no-op
initialization was installed. Existing normal `exit(0)` remains.

Local SDK review used `/var/home/johnrowl/guid_hdrs/d3drm.h`, `d3drmobj.h` and
`d3drmdef.h`, independently cross-checked against the installed Wine headers and
`docs/decompiles/full/LEGORR.full.c`: RM3 IID
4516EC83-8F20-11D0-9B6D-0000C0781BC3; factory CreateFrame +10/CreateLightRGB +30;
Frame3 AppData +18/+1c, AddLight +30, GetParent +5c, GetPosition +60,
GetOrientation +7c, DeleteChild +94, SetOrientation +ec, SetPosition +f0,
SetSortMode +f8; light color/range/umbra/penumbra +30/+38/+3c/+40.
Calls use x86 stdcall with explicit this. Creation/query/setters return HRESULT;
AppData getter returns DWORD. Pool releases payloads, AppData and frame references,
returns wrappers to its free list, and clears freed banks for init failure unwind.
A successful real init/menu quit exercised normal arena destruction; forced
allocation failures, loader-mode teardown and every pool reuse path were not tested.
Full retained-mode device/authored visual reconstruction is not claimed.

Candidate-1, using the captured patch, explicitly failed initialization; launcher
exit 0 was not accepted. Temporary tagged diagnostics (candidate-2) identified
**SetSortMode(1) -> 0x80004001 E_NOTIMPL**, before camera/light setup. Retail
0x472710 ignores this optional call's HRESULT, while the patch added a fatal gate.
The gate was retained: loader tracing established the deeper dependency mismatch.

- `reference-2-loader.log:159`: native DLL in the reference runtime directory.
- `candidate-3-loader.log:157,321–323`: Wine builtin system DLL; its Frame3
  SetSortMode stub returns E_NOTIMPL.
- Native shipped file: 437008 bytes, SHA256
  `ef5384bd48b78fe7bc93cb04191b79930ac2e456cfa6e22e8ba7ce9092945583`.

The one permanent integration correction resolves `D3DRM.DLL` with bounded
GetFullPathNameA from the established game working directory, checks failure and
truncation, then loads that absolute asset path. It does not change Make, Proton,
compatdata, environment, assets or any required init failure gate. The candidate
EXE lives outside `reference/`, so the previous bare lookup selected the system
builtin ahead of the working-directory native DLL. `candidate-4-native.log:324`
confirms the corrected native load. Another dependency still loads the builtin
at line 157; it is not the factory explicitly selected by this application owner.
All temporary `[scene-startup]` probes were removed before the final build.

## Builds and complete report comparison

`distrobox enter developer -- make reccmp-report` and `git diff --check` passed.
No native build/tests. Baseline reports/binaries were archived before source edits.
Final full report: **2332/2396, 97.33% implemented, 52.87% accuracy, 51.46% progress**.
Final game-only: **2113/2137, 98.88%, 48.43%, 47.88%**.
Baseline was full 52.85%/51.44%, game-only 48.40%/47.86%.
No compared address was added or removed in either report. Normalizer remains 100%.
Complete JSON comparisons exist per valid build in `comparison-to-baseline.json`.

Twenty matching scores changed (same set in both reports); ten decreased. Changes
in edited allocator/pose/tick/destructor bodies are assessed with the source
contract above. Changes in unedited bodies, including AVI index and mesh vertex
fill, are **unexplained code-generation/report shifts pending analysis**, not
established semantic regressions. No score-only semantic adjustment was made.

| Reference address | Function | Before | Final |
| --- | --- | ---: | ---: |
| 0x4089d0 | segment_2d_point_distance | 38.94% | 36.44% |
| 0x40e9c0 | lava_erosion_delay_from_cell | 84.21% | 100.00% |
| 0x4232e0 | EngineTick | 14.16% | 13.22% |
| 0x4388b0 | entity_action_kind_prerequisite_check | 14.49% | 18.12% |
| 0x444680 | entity_step_movement_vector_resolve | 6.74% | 6.39% |
| 0x44e0c0 | terrain_create_retail | 11.44% | 11.47% |
| 0x464110 | rotated_pairs_scratch_fill | 10.85% | 13.95% |
| 0x471370 | cmd_place_ray_candidate_check | 15.22% | 15.30% |
| 0x472950 | lego::ddiscene_object_allocate | 30.69% | 28.28% |
| 0x472a50 | ddiscene_object_destroy_with_mode | 23.94% | 24.54% |
| 0x475430 | ddiscene_object_position_set | 39.34% | 57.14% |
| 0x475470 | ddiscene_object_pose_method_dispatch | 40.54% | 55.07% |
| 0x4754c0 | ddiscene_object_position_get | 30.30% | 45.90% |
| 0x475500 | ddiscene_object_facing_pair_get | 21.21% | 20.59% |
| 0x479630 | surface_triangle_area_calculate | 17.37% | 17.31% |
| 0x47d520 | surface_palette8_to_16bit_surface_expand | 32.65% | 33.82% |
| 0x47d800 | surface_bitmap24_bit_surface_convert | 29.17% | 28.74% |
| 0x47e190 | font_device_color_convert | 19.92% | 18.33% |
| 0x47ec60 | AviPlaybackState::playback_frame_index_get | 23.53% | 11.11% |
| 0x482620 | mesh_segment_vertex_fill | 31.30% | 26.09% |

Build evidence correction: a temporary reconstructed source snapshot was placed
inside ignored evidence while removing diagnostics; Reccmp scanned its annotations
and produced invalid duplicate/missing metrics during an otherwise successful
unchanged build. No launch used that report. The snapshot was moved outside the
repository, the invalid output retained as `attempt-3-build/discarded-report/`,
and reports regenerated correctly in `attempt-4-build/`. Raw source copies must
stay outside the scanned repository; evidence here retains patches and hashes.

## Visible runtime observations

Both Make targets used the existing dw-proton runner, `runinprefix`, compatdata
`/home/johnrowl/.local/share/rock-raider-a2/compatdata`, client `/home/johnrowl`,
and the absolute `reference/` cwd. No startup arguments, autostop, direct level
or environment bypass. `launch-dry-run.txt`, launch/result JSON, process identities,
`events.jsonl`, logs and actual Spectacle screenshots record the runs. Candidate-5
has effective WINEDEBUG=-all and no game overrides. The reference-1 process env
was not separately captured this round; its Make recipe/runner/cwd/settings are
recorded, with the prior packet's process allowlist retained as history.

Reference-1: real fullscreen 640×480×16 settings, visible responsive menu,
Quit Game -> Yes -> exit 0. A fresh KDE permission prompt appeared initially;
agent did not approve/bypass it. Subsequent observed settings/menu input worked.
Reference-2, **diagnostic WINEDEBUG=+loaddll only**, confirmed native DLL loading,
menu and normal UI quit exit 0. Earlier complete tutorial/pause reference history
remains in the baseline packet; it was not replayed as current candidate evidence.

Candidate-1: settings accepted, surface rc=0, explicit scene init failure, exit 0.
Candidate-2: same failure with temporary source probes; exact sort HRESULT above.
Candidate-3: **diagnostic WINEDEBUG=+loaddll**, builtin module/stub confirmed.
Candidate-4: corrected final binary, **diagnostic WINEDEBUG=+loaddll**; native
module, nonzero initialized root, save/frontend setup, visible menu. Pointer
hover changed highlight; actual Quit Game click opened “AREYOUSURE?”, Yes click
exited 0. See `candidate-4-native-quit-hover.png`, `candidate-4-native-confirm.png`,
`candidate-4-native-result.json` and `candidate-4-native-stub-delta.log`.

Candidate-5: same final binary, **normal make reccmp-run / WINEDEBUG=-all**.
Menu screenshot `candidate-5-normal-menu.png`. First click opened the SaveGame
list; Escape returned to main. Training hover/click opened the real tutorial
picker (`candidate-5-normal-training-hover2.png`, `...training-picker.png`).
Hover selected Moving Rock Raiders (`...moving-hover.png`), then its held click
exited with child Error 5 / Make status 2. This proves real menu response and
reaches the tutorial selection boundary; it does not prove a loaded level.
All clicks were 200-ms held actions recorded in events. No direct level injection.

The enlarged candidate window is 2552×1406 even after fullscreen 640×480×16
selection; its framebuffer/menu coordinates remain 640×480. Pointer position
is visibly displaced/scaled, and overlapping text/hit regions require care.
Mouse control used actual desktop movement into those logical hit regions,
followed by observed highlight before clicking. This is a known input/presentation
parity defect, not a claim that general mouse aiming works. Down did not move
selection; source confirms printable VK handling precedes arrow VK mappings.
No input patch was integrated; these remain follow-ups to the next blocker.

## First remaining blocker: tutorial selection / Stats owner

Candidate-6 repeated the same final binary and menu path with **WINEDEBUG=+seh**
only. Game thread `0024` fault at candidate **0x004149BA**, read **0x00000148**,
ECX=0, EAX/EDI=0x0249c2b8. Mapped function entry candidate 0x004149B0, reference
0x469e80: `entity_state_flags_get`, `src/game/entity.cpp:5522`. Disassembly:
load entity Stats pointer `[eax+0x114]` into ECX, then fault reading `[ecx+0x148]`.
The entity argument is nonzero and its Stats pointer is null. The first exact
game fault is at log lines 305–313; earlier Wine service/syscall exceptions are
not substituted for that game crash. Wine then starts its automatic debugger,
which internally crashes; child Error 84 / Make status 2. No reliable stack,
return address, actor kind/id or current-level pointer was captured.

Artifacts: `tutorial-fault-extract.txt`, `tutorial-fault-symbol.json`,
`tutorial-fault-disassembly.txt`, `tutorial-fault-static-callers.txt`,
`tutorial-fault-static-caller-map.json`. The latter is a static candidate set,
not a captured dynamic caller. App stub log's last flushed startup line is
`frontend built=1 enabled=1 override=0`; buffered absence of later messages cannot
prove how far LevelLoad progressed. The selected UI item identifies the requested
tutorial; it does not establish the live level or faulting actor.

Narrow next patch request: inspect the return address and actor at the current
binary's getter entry 0x4149b0 (or an equivalent caller-side diagnostic), capture
actor kind/id/level and Stats binding provenance, then repair the real producer/
lifetime/order violation using existing production Stats load/rebind APIs in
`entity_config.cpp` and entity binding APIs. Do not assume all Stats are absent,
return zero from this getter, install a null template, or integrate movement/
pause to hide the failure. No further equivalent launch was attempted.

## Mutable data restoration and cleanup

Before the first launch: independently byte-verified, stable snapshots of all
**484 reference** and **2218 compatdata** entries, including every save/settings
file, symlinks and absent-path inventory. Backup is `immutable-backup/`; metadata
is `baseline-inventory.json` (uid/gid/mode, times, xattrs/ACLs, hashes and targets).
Complete changed state was preserved after every launch before restoration.
Atomic fsynced replacement was used only for verified owned differences; unrelated
Wine processes were never stopped. Prefix registry differences were reviewed as
Wine-generated device/display/DirectX/ActiveMovie/font/runtime state, then restored
only after game/wineserver exit and recheck against the archived post-run hashes.
No blanket live-registry import was performed. Attribution and key diffs are in
`prefix-restore-attribution.json`, registry key/diff files and per-run inventories.

The preservation helper was corrected after its first conservative restore stop:
readlink on Wine-renewed symlinks updated access times. It now distinguishes that
scanner effect from an intervening content/target writer, restores symlink access
times after target verification, and verifies metadata using lstat. ctime/inode
are recorded but cannot be restored by normal filesystem operations; restorable
metadata and bytes are verified exactly. This limitation is explicit.

**Final `final-restoration.json`: zero differences**, all reference and compatdata
hashes/targets/restorable metadata match baseline. Exact mutable hashes:
`mutable-restored-hashes.json`; every per-run restore verification is retained.
Current changed-state `reference/Data/Saves/5.sav` remains
`41ceff64d8d7d8590c7828cf72e76d32263c2ff929c6f831e179d19ad36ea69a`.
The unrecoverable earlier original bytes were not invented or claimed restored.
Registry baseline/final SHA256:

- system.reg `a8dbd66064474e40e20d4ec9694fe404caea8443373af5c217b2e23c9a9c28db`
- user.reg `1bee969ad7a30f40c2d6398e7d597d81863ec3bfe046d114a9caa0e6966130c1`
- userdef.reg `fd92ee20a84725f8f1b518bde8dedc11f245d39a00e04c673b06ec1a6189ce65`

Reference EXE/DLL/WAD bytes remain unchanged. Final source remains frozen at the
built patch. `final-processes.txt`/`final-windows.txt` show no owned game/debugger.
Protected preexisting Wine device PIDs 2821989/2822012 remain alive. No process
was signaled. Exclusive integration/build/desktop ownership is returned to root
with this bounded startup/menu handoff; future launches require the next explicit
owner assignment and the same preservation protocol.
