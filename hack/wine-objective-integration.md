# Objective/briefing integration checkpoint

Closed 2026-11-02. **PARTIAL**: genuine two-page briefing acceptance and the real scene-render path are demonstrated. Complete visible-Raider/gameplay acceptance and ordinary clean shutdown remain blocked. Sole shared integration/build/runtime/desktop ownership is retained pending explicit root transfer. No new phase begins here.

Evidence packet: `build-reccmp/runtime-evidence/20261102T231809Z-objective-integration/`.
Machine-readable disposition: `checkpoint-result.json`. All three run attempts below are separate evidence; their outcomes must not be combined into one passing run.

## Source and build identity

Parent HEAD `dc7f167b1a4b19efe65a35fcb5523fdf61e62fc1` remained dirty. Applied **only** the immutable named patch `wine-objective-ownership.patch`, SHA256 `1caaa0b8079f910d731c4187710f13f30e1dbe09bb46d99e72f264b42f7cc4cf`, from the author at `/var/home/johnrowl/.t3/worktrees/lrr-portable/t3-b5b3bdb5/hack/`. Complete handoff, manifest and patch are copied into the packet. Baseline hashes, read-only apply check, three-file application and final author-result hashes all match. No compile correction was required.

Only `src/gods98/frameloop.cpp` briefing hunks, `src/game/objective.cpp`, and `src/game/objective.hpp` changed relative to entry. All other 173 source hashes and the preexisting dirty miniwin diff are unchanged. Existing startup/Stats/input/movement/pause-quit changes remain. No pointer patch, pointer/pause composition, NERP callback delivery, renderer fix, harness/Makefile/assets/desktop setting change, commit/push/PR/merge, delegate, native64/SDL suite, or focused PE32 test was performed.

Serialized command: `distrobox enter developer -- make reccmp-report`, **exit 0**. `git diff --check` passed. `build-result.json` contains the exact command and times; `build.log` is complete. One passing production build only; no later rebuild. Frozen throughout runtime:

- EXE SHA256 **`50cc51f7d5b3fb326a66fbfed6c1f5ec214fe288c53883312cb10a52734e55c2`**.
- PDB SHA256 **`8ccb11237fa75c526924776dc7147448f170c76f89ded4cb74a8f4056c0defd1`**.
- Before EXE `a16f672d00622daa308054e3a4be03483bfa2b06b17890262b465c276ec723ed`, PDB `35f9d48a3e8ad28053432dcc5e8b4fa09ea7450184a70a400d521dfd086cb797`.

`before-build/` and `build/` contain complete EXE/PDB/cache and HTML/full/game JSON reports. `frozen-identity.json` and `final-frozen-identity.json` recheck all176 source hashes and artifacts. Raw before/after C++ snapshots and the PDB extractor are outside all source/annotation scan roots at `/var/tmp/20261102T231809Z-objective-integration/`; the executed extractor copy is `/tmp/lrr-objective-extract-pdb.py`. The first two attempted extraction paths failed (missing container executable/path), then the host lacked winepath; the successful developer-container extraction is in `pdb-extraction.log`. No production rebuild followed those tooling errors.

All PDB symbols/layouts were freshly resolved from this EXE/PDB. `pdb-runtime-map.json` includes globals, functions, members and line mappings; application size is5672, briefing page offset5628/held mask5632. Portable LevelState objective offset comes from its PDB, not the retail layout. No a16 runtime address was reused.

## Complete Reccmp comparison

`results.json-comparison.json` and `game-only.json-comparison.json` compare **every** row, with complete before/after changed rows and diffs. Full2332→2332 and game2113→2113 compared rows; **zero new/lost mappings**. Only two score changes, both decreases:

| Target | Function | Before | After | Source change |
|---|---|---:|---:|---|
| 0x458be0 | objective_tick_target | 15.7233% | 14.3791% | Edited pending/terminal presentation sequencing |
| 0x4512d0 | terrain_cell_normal_sample | 9.7297% | 9.5745% | Body/source unchanged |

The objective decrease follows the real entity gate moving into each terminal branch, pending remaining a presentation/audio state, following-tick terminal entry, and advisor expiry cleanup. Mapped void ABI remains intact. This is an accepted source-contract change, independently observed for pending acknowledgement, not proof of real terminal reward correctness. No terminal manipulation/probe or true terminal gameplay test was run.

For the unchanged terrain helper, `terrain-normal-code-diff.txt` and before/after objdumps show an added `lea edx,[ecx+0xc]`, equivalent `edx+4/8` versus `ecx+0x10/14` operands, commuted component sums, register scheduling and three-byte branch-distance shifts; normalization arithmetic/order after those component loads remains the same. `level.cpp` bytes are identical. This explains the specific static code-shape decrease; it neither proves terrain runtime parity nor identifies the visible scene defect. No flags or score-only edits were made.

Final full report **97.33% implemented /52.81% accuracy /51.40% progress** (2332/2396); game **98.88% /48.36% /47.82%** (2113/2137).

## Sequential reference attempt — failed comparison

Reference PID3956958 launched exclusively through `make reference-run` with `WINEDEBUG=-all`. Normal settings accepted via Enter, DDI intro appeared, then owned-window captures were black. A usable main menu and permitted NEXT progression were **not** established. No ordinary reference selection/button policy/destination success is claimed.

Focus diagnosis included repositioning the **owned** settings/game windows, and several ineffective activation/input attempts, retained in `events.jsonl`. No desktop setting changed and no user app was intentionally clicked. The later root instruction prohibited further equivalent cycles/repositioning; no subsequent reference attempt occurred. Repositioning might relate to the Wine region stack, but this is only a hypothesis.

`wmctrl -ic` removed the window but left the game busy. Read-only GDB attach/detach (`reference-close-debug.txt`) captured thread1 in Wine `REGION_IntersectRegion`/`NtGdiCombineRgn`/memcpy, not a captured application exception. Exact owned PID verification and **SIGTERM3956958** are in `reference-forced-cleanup.json`; make exited2. No normal-exit or full-reference-flow credit. The early `reference-pre-cleanup-*` archive was taken before writers had gone and is retained as intermediate evidence only. Authoritative `reference-final-*` capture was made after termination/writers gone, reviewed, restored and verified before candidate launch.

An events note initially said the stalled xdotool was terminated; the immediate correction records `kill: No such process` because xdotool had already completed exit0. No signal actually reached that xdotool or game at that point. Later reference SIGTERM above is the actual terminating signal.

## Instrumented candidate — ownership acceptance, then unexplained nonzero exit

Candidate PID3961908 launched through `make reccmp-run`, identical runner/preloader/prefix/cwd/relevant environment to reference (`*-effective-context.json`). Settings FullScreen640x480x16; candidate logical surface640x480 and existing2552x1406 client/32-bit presentation. No format or harness workaround. Initial menu attempts with lost focus/short input did not all advance; retained screenshots/events prevent crediting those attempts. Positioned normal Training/Moving clicks did reach the real briefing.

Owned captures `candidate-real-brief-page1.png`, `candidate-real-brief-page2-held.png`, and `candidate-real-brief-page2-released.png` show both shipped authored pages. Exact-PDB `candidate-brief-before.json`, `candidate-brief-between-held.json`, and `candidate-brief-between.json` show page lengths236/157, page offset0→237, private/mirror **0xb1**, pending1, result0, switch0, reward0 throughout. The held Space latch was1 then0 after release; page1 did not clear pending or enter rewards.

After a fresh delivered final Space, `candidate-final-ack-trace.json` directly records actual owner entry/return:

- Return1 at0x47fe10: private/mirror **0xb1→0xb0**, pending **1→0**, result **0 unchanged**, genuine switch **0→1**, reward **0 unchanged**. UI active is still1 at the acknowledgement return because the caller clears it next.
- Next scene-path breakpoint at **0x448ac2**, exact PDB `scene_render.cpp:947`, executes after the null/reward/depth fallback guards. Briefing active0, private/mirror0xb0, pending0, result0, reward0, switch already consumed0. Thus actual scene branch execution replaces the old source-only coarse-map inference.
- The following actual `ai_tick` entry receives0.076000005 seconds. No test-only actor, forced flag/outcome, state/register write, call injection or automatic acknowledgement was used.

`candidate-scene-real.png` shows textured scene geometry, with close tall walls dominating the view and only small colored fragments; **no complete recognizable real Raider/valid visible target is accepted**. `candidate-after-ack.json` records three actual OList Pilot actors (kind2,row0, RouteSpeed1.10000002) and one Toolstation(kind4,row0), valid owner and Stats. Attached authored scenes are all null and runtime type scene-ops bytes zero. This does not prove those absent authored bindings are the sole visual cause or that `render_raider` was executed. No invisible actor was clicked; no selection, route, scripted/user movement or arrival pass.

Fresh Escape opened parsed PausedMenu (`candidate-pause-released.png`). Two read-only snapshots at23:25:53.235667 and23:26:07.794915 show stable actors/mission/engine context/objectives/selection/queue/reward, tickers48290 unchanged for14.559248 seconds. Modal frame count advances by design. This proves idle loaded-state pause, not walking pause. Actor activity timers and tickers advanced before pause; no post-resume completed state delta was captured.

Continue returned the scene. `candidate-resume-trace.json` records the first rebase tick with outer dt3.0, then actual `ai_tick` argument **0.0575000048** with rebase0. This is the actual scaled argument after the local bound, unlike app.frame_dt. Debugger stops affect timing; this is not a pristine wall-clock performance test. The process disappeared at23:26:40.537376, about1.25 seconds after the first AI breakpoint/detach; make2/child **Error5**. No terminating cleanup signal was sent, but **fault identity/mechanism and debugger contribution are UNKNOWN**. No reliable sustained-resume, post-resume state-advance or Q credit from this run. Subsequent actions failed BadWindow/absent-PID checks and produced no new game evidence; the old /tmp view must not be treated as a new successful capture.

All GDB scripts/logs are retained. Ptrace attach stops and temporary code breakpoints are disclosed; no explicit SIGINT, application state/register writes or inferior calls. Both tracing GDBs detached and exited0. Instrumented results belong only to this run.

## ONE separately authorized debugger-free replay

Root explicitly authorized one further run after the first candidate's full afterstate archive, offline review, verified restoration and writers-gone check. **Same50cc EXE/PDB/source/cache/environment/harness/settings**, no new reference/build/fix. `ordinary-replay-authorization.json` and prelaunch identity checks retain authorization/scope. PID3967126, `candidate-ordinary-one-*`, `ordinary-effective-context.json`. **No GDB, ptrace, breakpoint, process memory inspection or signal** in this run.

Normal menu path and both pages were displayed/accepted again. Some early short key attempts/lost focus did not advance; no blind menu progression is credited. Packet-only `ordinary-input.py` verifies owned-window activation and logs each exact physical key/button action; a fresh release then sustained press with normal settling achieved both pages. No desktop permission/config/harness/source change. Screens `ordinary-brief-current.png`, `ordinary-space-held.png`, `ordinary-page2-released.png`, `ordinary-after-final-brief.png` are the accepted sequence.

- Fresh Escape→PausedMenu: `ordinary-paused.png`.
- Continue press/release→scene at23:32:12: `ordinary-continue-scene.png`.
- About11 seconds later, fresh Escape opens another pause: `ordinary-second-paused.png`. Process remained alive beyond the instrumented run's immediate-exit interval.
- A separate fresh Escape resumes at23:32:29: `ordinary-escape-resumed.png`.
- Ordinary WM close requested immediately afterward; window and process disappear, but launcher **make2 /child Error5**, end23:32:31.249329. `ordinary-normal-window-close` records WM close command exit0; it records completion after the launcher end, not an earlier precise send time. **Q fails**. No fault code/stack exists, so do not assert a definitive product crash or locate its owner from Error5 alone.

This replay verifies those ordinary pause/resume **UI transitions and continued responsiveness** without a debugger. It does not supply an observed post-resume simulation-state delta, walking pause, clean exit or menu-Quit parity. Keep the earlier instrumented abnormal exit and the debugger-free close failure separate; the first run's immediate post-Continue disappearance did not recur during this replay's observed resumed interval. No additional replay is authorized or performed.

## First remaining blocker and precise handoff

**First failed T/E gate: recognizable visible loaded Raiders / ordinary tutorial interaction.** Both candidate attempts reach the actual textured scene path, but walls dominate and only small colored parts are visible. No target was validated for ordinary actor interaction. Owned screenshots above plus actual OList/renderer-state snapshot are the evidence; exact geometric/occlusion/authored attachment cause remains unknown.

Source-only investigation pointers (no renderer edits): `scene_render.cpp` `scene_entity_origin_get`691, `scene_camera_configure`705, `render_terrain`751, `render_raider`863, actual entity loop958–966. Entity origins/focus/picking share the world→grid boundary, terrain heights/cache and camera/depth storage determine visibility, while authored type scene ops/attachments remain absent in snapshots. Next owner needs actual origin/camera/terrain/depth and actor-render evidence before choosing a production repair. The executed path breakpoint at947 does **not** establish the actor branch. Do not fabricate rays, hide terrain, force camera/flags, click invisible actors or substitute debug actors.

**Additional blocker:** ordinary candidate window close yields Error5 without debugger. Capture a precise fault/teardown boundary in a separately authorized phase; do not infer definitive product-crash identity from the shell's status. No such next phase started here.

Root separately reported a prepared isolated NERP leaf144/254 repair and unresolved200 AdvanceMessage owner, documented in `hack/wine-nerp-callback-audit.md`. It and the refreshed pointer delivery remain **UNINTEGRATED**; this checkpoint does not validate those future prerequisites. No real NEXT progression/permitted selection stage or reference mouse policy was established. Pause Quit still application exit rather than Training return; no mission lifecycle parity claim.

## Preservation and closure

Entry matched previous designated baseline0 differences, then this packet made its own O_NOATIME/full byte-verified immutable backup of all484reference/2218prefix entries. Every stopped run's afterstate was archived before reviewed restoration, and exact baseline verified before the next launch. Build-only directory atime effects were recorded/normalized before reference. Registry review: reference HID/WINEBUS ContainerIds, two monitor EDID→BAD_EDID and display-class memory/symbolic-link values; candidate/replay six HID/WINEBUS ContainerIds each, no user/game value changes. Offline diffs/reviews are saved. No blind live/shared registry restore, no unowned Wine termination.

`final-restoration.json` independently verifies **ZERO differences across484reference and2218prefix entries**, all bytes/type/target/mode/uid/gid/atime/mtime/xattrs. ctime/inode are nonrestorable and never claimed restored. Reference EXE/DLL/WAD remain immutable. Designated current `Data/Saves/5.sav` remains SHA256 `41ceff64d8d7d8590c7828cf72e76d32263c2ff929c6f831e179d19ad36ea69a`; unrecovered original `e33adbaa…` is not claimed restored.

Protected2821989/2822012 remain alive/untouched; no owned game/runner/debugger/wineserver remains. KDE XwaylandEisNoPrompt stayed observed true; no agent changed it, no stale portal approval was resurrected, and desktop settings are excluded from restoration. Final identities confirm no source/build mutation during runtime. Exclusive ownership is retained until explicit root transfer; stop at this bounded checkpoint.
