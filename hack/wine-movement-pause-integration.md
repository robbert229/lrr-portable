# Movement and pause/quit integration checkpoint

Evidence packet: `build-reccmp/runtime-evidence/20261102T223410Z-movement-integration`.

Closed 2026-11-02T23:03:06.560566+00:00. Status: **partial, blocked by objective/briefing ownership**. Sole shared integration/build/desktop ownership remains reserved until root transfers it. No owned runtime remains. No commit, push, PR, native64/SDL test, harness edit, desktop permission change, pointer integration, or objective/renderer flag workaround was performed.

## Exact sequence and identity

The input checkpoint closed separately in `../20261102T215407Z-input-integration/continuation-handoff.md`, with final zero differences for484 reference/2218 prefix entries. Its frozen EXE was `3d9aca4a30e35efe24b0b35c782c21db820b97827c51f7be756834a7793a6ca9`.

1. Read complete movement handoff/probe/patch; verified artifact SHA `0335ba351f723f72fac4d38c0952c14bee6cf2fd654cb8cd0be93e6331c6157d` and apply check against actual dirty input baseline. Applied its12 production files, keeping the opt-in raw C++ probe outside Reccmp source roots. No extra production fix was needed. Built `distrobox enter developer -- make reccmp-report`, exit0. Movement-only EXE `1dca5aaa2714fdef8af4114975f324c661e75f6af15c8ec316e811114d05bb8e`; no gameplay run on old quit code. Archive `movement-build/`, before archive `before-build/`, build log/result and `movement-report-comparison.json`.
2. Root reprioritized quit before runtime. Read complete pause handoff/delta, verified SHA `a93140e43c97bc92d1f34bb3005960cb9f33d5118c6f639abc2eaf6acd185c3d`, actual frameloop baseline and apply check. Applied only `src/gods98/frameloop.cpp` sequentially. Built the same MSVC5 report command, exit0. Archive `combined-build/`, `pause-build.log`, `pause-build-result.json`, `pause-report-comparison.json`.
3. Frozen combined EXE **`a16f672d00622daa308054e3a4be03483bfa2b06b17890262b465c276ec723ed`**, PDB **`35f9d48a3e8ad28053432dcc5e8b4fa09ea7450184a70a400d521dfd086cb797`**. `final-frozen-identity.json` rechecks all176 source hashes, EXE/PDB/cache/full+game JSON and complete tracked diff. No source edit followed freeze. Startup, Stats, input, dirty miniwin and unrelated changes retained.
4. Sequential ordinary reference and combined candidate, with exact restoration between them and after candidate. Same runner/prefix/cwd/env documented in `reference-effective-context.json` and `candidate-effective-context.json`. Environment and cwd equal. Reference-supported640x480x16; candidate surface640x480 with existing32-bit presentation, nonintegral2552x1406 client. No format workaround.

Movement implements actual loaded-actor world/grid conversion, integer-cell queue transport, real actor-aware route admission, waypoint continuity, incremental motion owned by existing AI and arrival through existing command owner. These are source/build facts, not accepted gameplay. No forced selection/scheduler flags, fabricated actors, camera/rays, straight-line routes, M/numeric selection, or direct-call E acceptance was used.

## Runtime acceptance and first failed gates

Reference PID3926951: settings/main Training/Moving/briefing/scene accepted. Real Raiders visible. At the **first instruction still asking for flashing NEXT**, a visible Raider hover showed a face and ordinary left press/release showed red X. This is tutorial gating, not button policy. Exact prerequisite images: `reference-scene-before-selection.png`, `reference-raider-left-released.png`. The reference had already exited before root requested bounded NEXT progression; it was not relaunched mid-candidate. Next reference acceptance must advance visible NEXT instructions until an explicit selection/movement request, then record permitted gesture and persistent feedback. Ordinary move/cancel policy is still unestablished. PausedMenu visible; later immediate resume image insufficient to prove resumed simulation. Ordinary WM close exited0, no signal.

Candidate PID3931168: normal settings accepted, main Training/Moving/real two-page briefing accepted. Use direct owned-window `candidate-brief-owned.png`, `candidate-brief-page2-owned.png`, `candidate-scene-owned.png`. Some earlier Spectacle active-window images captured the user's browser after focus changed: retained and excluded as game evidence in events.jsonl. Owned activation preceded subsequent actions; explicit `import -window0x04c00001` captures establish game identity.

Actual loaded OList state: three Pilot2/row0 and one Toolstation4/row0, nonzero Stats, Pilot RouteSpeed1.10000002, real loaded world positions, matching owner context. Attached authored scene objects are null and type runtime scene ops are zero. The procedural representation is compiled, but no recognizable actor appears after briefing. Authored-renderer gap remains disclosed. No invisible actor click was attempted and no candidate ordinary selection/move/arrival passed. Strict opt-in movement probe lacked live attached-actor prerequisites and driver; it was **MSVC5 compile-only**, not executed or credited.

Pause: fresh Escape eventually displayed real parsed PausedMenu. `candidate-paused-settled-owned.png` and the held-repeat image prove that menu; the earlier immediate/held game image does not. `paused-state-comparison.json` establishes about289 seconds with tickers79530 unchanged and identical actor, mission, engine context, objective, queue and selection data. This proves paused stability for the idle loaded state, not active walking. Modal shared frames continue by design and are not a gameplay clock.

Continue press/release and fresh Escape did not resume. `candidate-escape-held-in-pause.json` proves actual Escape delivery (`pause_escape_down=1`) while menu2/open1 remains. Read-only breakpoint `candidate-pause-gate-decoded.json` confirms controller **blocked=1** (other action/options/level flags0). Do not repeat equivalent actions. Resume timing, closing-gesture suppression, pause while walking and normal menu Quit remain unverified/blocked. `app.frame_dt` is stored before the local delta bound and cannot establish catch-up; no catch-up failure is attributed. A valid future capture must observe the actual post-bound gameplay/AI argument and simulation advance. Pause Quit still means application exit in the authorized patch, not return to Training; no mission lifecycle parity claim.

Ordinary `wmctrl -ic0x04c00001` at22:59:57 ended candidate through normal close handling with `make reccmp-run` exit0 (`candidate-movement-pause-result.json`); archived stub log ends `exit=closed`. No terminating cleanup signal was sent. This candidate run did contain an earlier debugger SIGINT interruption while cancelling a failed resume trace, followed by debugger SIGTERM/detach. The earlier note saying no signal reached game is corrected explicitly in events.jsonl. Thus do not call the whole run wholly signal-free. The trace never reached its intended resume/AI/renderer breakpoints and supplies **no** first-delta or executed-renderer proof. No state-changing debugger call or flag write occurred.

## Objective/render/pause diagnosis on this exact EXE/PDB

`candidate-objective-renderer-paused.json` uses complete exact-PDB member offsets (portable `LevelState::objective`+908, not a guessed retail+0xb0):

- `GameContext.reward_active=1`.
- `level.objective=0xb1`, objective_pending1, objective_result0; `s_objective_result_word=0xb1`; mission_brief_active0.
- `ptr_terrain=0x3edc870` non-null. Depth pointer0x47a0020 and capacity0x4b000=307200 floats match640x480; this state does not require depth reallocation.
- `level_render_scene` compiled unconditional. Source~918-920 takes coarse `level_render_map` when reward_active is nonzero before scene geometry. Snapshot+exact source imply this branch for these arguments, consistent with the dark map. It is not a direct recorded branch hit: pause could not resume for the breakpoint.
- `objective_tick_target`~975 makes the adapter return0 for any low-three objective bits, including pending bit1. Frameloop~3921 treats return0 as terminal and activates reward. Archived log repeatedly says objective resolved(word0xb1), then reward screen shown. This supports root's ownership-transition hypothesis; no geometry speculation or forced clear was applied.
- Independently, frameloop pause controller passes `global_option_low_bits_enabled(level)` as blocked; `level.cpp:10500` tests objective&7. `overlay_menu_gate` does not invoke menu_tick when blocked. Exact runtime blocked1 explains why received Escape and menu actions do not reach frontend resume/quit handling. Source matches root's more immediate objective/briefing prerequisite. Proper acknowledgement/transition repair is a separate authorized phase, not included here.

## Reccmp results and all tradeoffs

Both reports retain2332 full/2113 game rows, no new/missing mappings in either build. Movement-only changed12 rows,8 decreases; pause then changed2 rows,1 decrease. Combined versus frozen input:14 changed,9 decreases. Full:97.33% implemented(2332/2396),52.81% accuracy,51.40% progress. Game:98.88%(2113/2137),48.36%,47.82%. Complete before/movement/combined full+game JSON and per-row comparisons are archived; table is percentage exact match, not runtime quality.

| Address | Row | Input | Combined | Change |
| --- | --- | ---: | ---: | --- |
| 0x404280 | cmd_entity_command_chain_tick | 14.8936% | 14.4444% | decrease |
| 0x404e00 | object_path_submit_to_grid_target | 22.6277% | 17.6471% | decrease |
| 0x40a0c0 | frameloop_credits | 8.9153% | 11.7820% | increase |
| 0x40e9c0 | lava_erosion_delay_from_cell | 100.0000% | 84.2105% | decrease |
| 0x4410f0 | terrain_neighbor_path_check_current | 15.6313% | 13.4875% | decrease |
| 0x4428a0 | entity_world_position_set | 17.7778% | 12.3288% | decrease |
| 0x444680 | entity_step_movement_vector_resolve | 6.3910% | 5.2336% | decrease |
| 0x44e0c0 | terrain_create_retail | 11.4667% | 11.7333% | increase |
| 0x44fb90 | terrain_state_surface_point_project_offset | 22.4000% | 22.8916% | increase |
| 0x4512d0 | terrain_cell_normal_sample | 10.6383% | 9.7297% | decrease |
| 0x459470 | osf_read | 12.6214% | 21.1982% | increase |
| 0x471370 | cmd_place_ray_candidate_check | 15.3005% | 15.2174% | decrease |
| 0x478040 | window_message_dispatch_pump | 100.0000% | 62.9630% | decrease |
| 0x4882e0 | scene_node_transform_matrix_apply | 13.9963% | 16.1468% | increase |

Root's independent static code-shape review explains unedited erosion scheduling/commutative imul with identical level.cpp hash; terrain normal equivalent offsets/register choices/commutative FADD; placement ray FXCH before commutative FADDP and branch-distance changes. This is static review, not runtime proof. Pause raw message-pump decrease corresponds to added quit ingestion. All decreases retained; no score-only rewrite or extra build. Input phase's earlier three declines remain separately disclosed in its handoff; they are baseline here.

## Preservation, artifacts, ownership

Stopped reference and candidate states fully archived (`*-final-after/`) before restoring. Registry diffs reviewed key by key: candidate six HID/WINEBUS ContainerId changes only, no user/game values; reference also display device values documented in its offline review. No blind live-registry replacement. Verified owned game/runner/wineserver/debugger gone before exact archived-state restoration. Final independent `final-restoration.json`: **zero differences across484 reference and2218 prefix entries**, full bytes/type/target/mode/uid/gid/mtime/atime/xattrs. ctime/inode cannot be restored and are not claimed. Protected2821989/2822012 remained untouched/alive. Current designated5.sav SHA `41ceff64d8d7d8590c7828cf72e76d32263c2ff929c6f831e179d19ad36ea69a` intact; original e33ad… loss is unrecoverable and not claimed restored.

KDE XwaylandEisNoPrompt was observed true in both runtime contexts; no agent changed it, no permanent per-agent grant inferred, and restoration excludes desktop config. Runtime accepted settings/menu actions are input proof. No new portal prompt appeared.

`checkpoint-result.json`, `final-frozen-identity.json`, `final-restoration.json`, events, archived logs, owned screenshots, exact PDB map/read-only snapshots and both build archives are authoritative. Raw source snapshots and probe remain outside scan roots at `/tmp/20261102T223410Z-movement-integration/`.

Refreshed pointer artifact `bff2e80feb1242226013573f37db57203eba0cd67e810e276a2323f2aaf1d49c` and separate pause composition `5ba2a5191be68273dff9e012a051e79dc284a9c5b18b1d9308bf84f09b3319d7` are **unintegrated**, awaiting root authorization after this checkpoint. No next gameplay run or new code phase started. Immediate prerequisite: proper objective/briefing ownership transition; later permitted reference NEXT progression and visible actual-actor pointer/route acceptance. Sole shared ownership retained pending explicit transfer.
