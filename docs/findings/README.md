# Findings index

[Project overview](../../README.md) · [Pseudocode corpus](../decompiles/README.md)

This catalog was moved from the main README. Finding numbers are identifiers,
not a completion measure; this directory also contains later findings that
have not yet been added to the catalog.


| # | File | Subject |
| --- | --- | --- |
| 01 | [`findings/01-binary-and-loader-map.md`](01-binary-and-loader-map.md) | PE structure, WinMain, resource loader, WAD container format |
| 02 | [`findings/02-startup-and-main-loop.md`](02-startup-and-main-loop.md) | Startup sequence and frame loop, callback triplet |
| 03 | [`findings/03-engine-tick.md`](03-engine-tick.md) | Engine tick, state block, command queue, quit poll |
| 04 | [`findings/04-sim-input-and-events.md`](04-sim-input-and-events.md) | Input system, key table, edge detection, transitions |
| 05 | [`findings/05-exit-selection-and-text-input.md`](05-exit-selection-and-text-input.md) | Exit paths, selection, text input |
| 06 | [`findings/06-command-queue-and-selection.md`](06-command-queue-and-selection.md) | Command queue, selection manager, `KeepRunning` |
| 07 | [`findings/07-level-loading.md`](07-level-loading.md) | `.map` format, voxel grid, ten-layer loading, terrain |
| 08 | [`findings/08-save-and-checkpoints.md`](08-save-and-checkpoints.md) | `.sav`/`.osf` save format, checkpoint blob, no entity dump |
| 09 | [`findings/09-reward-task-manager.md`](09-reward-task-manager.md) | Mission task block, reward overlay, reward-zone save trigger |
| 10 | [`findings/10-entity-command-primitives.md`](10-entity-command-primitives.md) | Command records, AI task/priority registry, opcode map |
| 11 | [`findings/11-model-and-scene-formats.md`](11-model-and-scene-formats.md) | LightWave LWO loader, LWSC scene parser, surface/UV pipeline |
| 12 | [`findings/12-block-text-format.md`](12-block-text-format.md) | `Lego*` block-text config format: scanner, path-lookup API, `.ae` consumers |
| 13 | [`findings/13-texture-system.md`](13-texture-system.md) | BMP surface loader, name cache, LWO material binding, world tile sets, VRAM budget |
| 14 | [`findings/14-sound-and-video.md`](14-sound-and-video.md) | WAVE/mmio sound bank, CD-DA music via MCI, AVIFIL32 movies, logo splash |
| 15 | [`findings/15-compat-audit.md`](15-compat-audit.md) | Modern-compat API-surface audit: wrapper coverage matrix, launch-gate scan, friction list |
| 16 | [`findings/16-a2-live-test.md`](16-a2-live-test.md) | **A.2 live test passed**: game launches under GE-Proton10-25; DDraw interface climbing observed (→ `IDirectDraw4`); movies via quartz; CDDA question deferred (→ 17: resolved — digital release carries no CDDA media) |
| 17 | [`findings/17-interactive-a2-session.md`](17-interactive-a2-session.md) | **Playable**: first mission completes; music is streamed WAVs (no CDDA media on digital release); movies skip on missing Indeo 5 decoder; NVIDIA Xid driver-incident notes |
| 18 | [`findings/18-movies-libav-alias.md`](18-movies-libav-alias.md) | **Movies fixed**: Indeo block was two missing sonames (`libbz2.so.1.0`, `libvpx.so.6`) silently killing wine's ffmpeg plugin — aliases + registry rebuild, automated in `a2.py` |
| 19 | [`findings/19-uv-grammar.md`](19-uv-grammar.md) | **Last format closed**: `.uv` sidecar grammar in `FUN_00481c50` — surface texture-path overrides + per-corner UV (v negated, vertex duplication); names/keyframe tail structurally unread; validated 60/60 |
| 20 | [`findings/20-verify-pass.md`](20-verify-pass.md) | **Fresh verify pass** (2026-10-20): critical path re-derived in a clean session; corpus byte-identical; `DAT_00505814` = `.wad` prefix, not install dir; CRT entry `0x48f000` |
| 21 | [`findings/21-flh-container.md`](21-flh-container.md) | **`.flh` closed**: FLIC-family overlay container — 136 B header, `0xF1FA` chunk stream from `0x80`, RGB555 FLI16 keyframe (opcode 25) + word-diff deltas (opcode 27) + hold chunks, 555→565 on write; validated 18/18 files, 469/469 chunks, rendered frame check |
| 22 | [`findings/22-research-crumbs.md`](22-research-crumbs.md) | **Last open questions closed**: message registry = startup catalog (`FUN_00419230`, 40 `Info_*` names) + `InfoMessages` config population (`FUN_00434c10`); `DAT_004ffbc0` relabeled as the objective state word (not input devices); D3DRM is software-path-only (no HAL device referenced anywhere) |
| 23 | [`findings/23-front-end-and-menu.md`](23-front-end-and-menu.md) | **Front-end & menus closed**: the boot→main-menu machine — `MenuBuild @ 0x4140c0`, the modal `MenuRunLoop`/`MenuTick`, the `FrontEndActive` gate, `MenuEnter`/`FrontEndStateChange`, the level-chain + start-level entries, and the options/movie callbacks, with the config grammar. The port has since implemented `::` path lookup and `Levels::<name>` campaign/tutorial chains; shipped-data end-to-end level-load validation remains |
| 24 | [`findings/24-menu-wipe-and-front-end-transition.md`](24-menu-wipe-and-front-end-transition.md) | **Menu wipe closed**: the config-driven LWS front-end transition — `MenuWipe`/`!MenuWipe` keys (reduce-animation gate, `"Null"` disable) loaded by `LoadMenuBuild @ 0x4161f0` into the LWS scene factory (`FUN_00472cd0`, kind `"LWS"` → `FUN_004765c0`), stepping `RockWipe.lws` (35f @ 25 fps) as the menu-change wipe in `MenuRunLoop`/`MenuTick` with an input freeze — the wipe/fade gap left open in finding 23 |
| 25 | [`findings/25-ddscene-object-runtime.md`](25-ddscene-object-runtime.md) | **DDiScene object runtime closed**: the game-wide scene-graph object model the MenuWipe (finding 24) and every animated 3D thing runs on — the 9-slot kind string table (`NULL MESH FRAME ANIM ACT - - LWS LWO`), the pooled `0x2c`-byte object record (`DAT_0076add0` free list, kind at `+0x10`, flags at `+0x14`), the draw/step API (`FUN_00475140` draw-at-time, `FUN_00477010`/`FUN_00477150` begin/end scene, `gods98/dx6` source path), and the `FUN_004765c0` scene loader behind `DAT_00505914` |
| 26 | [`findings/26-ddscene-animation-hooks.md`](26-ddscene-animation-hooks.md) | **DDiScene animation hooks closed**: `FUN_00475140` can call the action-entry **SFX sample** hook (`DAT_0076cd78/7c`, registered by the `FUN_00464740` 45-slot SFX-name-hash bank init, `0x27` = `SFX_RockWipe` → `FUN_00465050`→`FUN_0047ad70` positional play) and the scene-**end entity flag** hook (`DAT_0076cd80/84`, boot-registered, ORs `0x10000` into `entity+0x3e4`) — plus the `FUN_00464f60` pending-SFX queue |
| 27 | [`findings/27-scene-done-flag-consumers.md`](27-scene-done-flag-consumers.md) | **Scene-done flag readers closed**: both consumers of `entity+0x3e4 bit 0x10000` (set by finding 26's scene-end hook) inside the finding-10 flag-evaluator blob — reader A (`0x43dd10`) plays **placement sounds by work-object type** (`7`→`SFX_PlaceOre`, `6`→`SFX_PlaceCrystal`, else `SFX_Place` via `FUN_00465050`), reader B (`0x43d903`) runs the `FUN_004475c0` impact/wake probe (message 5, roll speed, shake) and clears the bit — plus new `+0x3e4` bit facts (`0x80000`, `0x100000`, `0x40000000`) appended to finding 10 |
| 28 | [`findings/28-ddscene-entity-ownership.md`](28-ddscene-entity-ownership.md) | **DDiScene owner link closed**: `FUN_00473da0/0x473db0` set/read object `+0x20`; the entity spawner assigns the owner on general scene objects and propagates it through type-specific model scene nodes, explaining how the scene-end callback returns to `entity+0x3e4` |
| 29 | [`findings/29-ddscene-entity-teardown.md`](29-ddscene-entity-teardown.md) | **DDiScene teardown traced**: entity removal calls `FUN_00472a40(entity+0x1c)` before recycling the entity; ordinary pooled objects enter the DDiScene free list without an explicit `+0x20` clear (zeroed on reuse), while kind-3/4 has an early auxiliary-record return; post-unlink callback reachability remains unknown |
| 30 | [`findings/30-ddscene-clone-reuse.md`](30-ddscene-clone-reuse.md) | **DDiScene clone slots traced**: kind-3/4 mode-0 teardown marks an auxiliary instance inactive; `FUN_00473690` can find it later, reattach it, and call `FUN_00475140(clone, 0)` |
| 31 | [`findings/31-ddscene-reuse-callback-order.md`](31-ddscene-reuse-callback-order.md) | **Cached-clone callback ordering bounded**: entity creation assigns the owner after the clone factory's synchronous time-zero draw; positive scene-end frame values cannot pass the draw-step's `end <= 0` callback condition |
| 32 | [`findings/32-ddscene-shutdown-hook-gates.md`](32-ddscene-shutdown-hook-gates.md) | **DDiScene shutdown gates distinguished**: shutdown clears the sample-hook enable bit, while the scene-end callback has a separate pointer/time predicate; post-shutdown reachability remains unknown |
| 33 | [`findings/33-ddscene-init-failure-cleanup.md`](33-ddscene-init-failure-cleanup.md) | **DDiScene pool cleanup is on the initializer's failure exit**: the parsed config/resource failures converge on shutdown; success returns without this cleanup; one other direct callsite is outside a procedure |
| 34 | [`findings/34-ddscene-failed-init-retains-end-hook.md`](34-ddscene-failed-init-retains-end-hook.md) | **Failed initialization retains the scene-end hook pointer**: hook registration precedes later failure cleanup, which clears the sample gate but leaves the end-hook slots untouched; dispatch after failure is unproven |
| 35 | [`findings/35-ddscene-paired-channel-update.md`](35-ddscene-paired-channel-update.md) | **Paired-scene update can skip its primary channel**: a missing primary action under a wrapper flag suppresses its draw step while a present companion is still advanced |
| 36 | [`findings/36-ddscene-activity-name-cache.md`](36-ddscene-activity-name-cache.md) | **Activity-name pointer cache suppresses repeats after a miss**: the type-1 dispatcher caches the request pointer even when neither scene channel accepts it, so another call with that pointer short-circuits |
| 37 | [`findings/37-advisor-animation-controller.md`](37-advisor-animation-controller.md) | **Advisor animation controller setup**: map-pointing/talking action labels and a companion DDiScene are initialized by a dedicated controller |
| 38 | [`findings/38-advisor-request-admission.md`](38-advisor-request-admission.md) | **Advisor request admission**: slot availability, a shared gate, and active-state bit control admission; accepted requests encode two modes |
| 39 | [`findings/39-advisor-request-reset-sounds.md`](39-advisor-request-reset-sounds.md) | **Advisor request reset**: accepted requests reset the scene and submit a slot sound plus queued `SFX_CaptainSlide` |
| 40 | [`findings/40-advisor-transition-worker.md`](40-advisor-transition-worker.md) | **Advisor transition worker**: selected and companion scenes advance together with a per-slot transform and restart on completion |
| 41 | [`findings/41-advisor-animation-window.md`](41-advisor-animation-window.md) | **Advisor action windows**: one state bit selects completion or overflow-wrap behavior using slot boundaries and scene duration |
| 42 | [`findings/42-ddscene-sample-hook-consumption.md`](42-ddscene-sample-hook-consumption.md) | **DDiScene sample-hook consumption**: action selection sets a pending bit that the next qualifying kind-4 draw-step consumes |
| 43 | [`findings/43-ddscene-end-hook-repeatability.md`](43-ddscene-end-hook-repeatability.md) | **DDiScene end-hook repeatability**: callback conditions have no local one-shot latch; duplicate runtime dispatch remains unverified |
| 44 | [`findings/44-reward-buttons-use-separate-save-routes.md`](44-reward-buttons-use-separate-save-routes.md) | Reward Save enters the slot UI; Advance commits through a separate checkpoint route |
| 45 | [`findings/45-timer-objective-short-circuits-other-checks.md`](45-timer-objective-short-circuits-other-checks.md) | Timer expiry short-circuits later objective checks in the campaign evaluator |
| 46 | [`findings/46-objective-sidecar-append-gates.md`](46-objective-sidecar-append-gates.md) | Objective sidecar records have additional append gates and a fixed serialized field set |
| 47 | [`findings/47-entity-events-feed-incident-messages.md`](47-entity-events-feed-incident-messages.md) | Entity removal emits shared and type-specific event messages into the incident path |
| 48 | [`findings/48-directdraw-renderer-setup.md`](48-directdraw-renderer-setup.md) | DirectDraw mode setup and bridge into the D3DRM device/viewport path |
| 49 | [`findings/49-model-render-pass-dispatch.md`](49-model-render-pass-dispatch.md) | Model rendering walks ordered child passes through the shared renderer |
| 50 | [`findings/50-tile-texture-refresh-order.md`](50-tile-texture-refresh-order.md) | Tile texture refresh updates indexed slots in a defined pass order |
| 51 | [`findings/51-held-key-command-repeat.md`](51-held-key-command-repeat.md) | EnginePreFrame binds key bytes to command records; held keys enqueue again each tick |
| 52 | [`findings/52-advance-gates-campaign-completion-mark.md`](52-advance-gates-campaign-completion-mark.md) | Reward Advance gates campaign-row completion marks on the current level result |
| 53 | [`findings/53-nextlevel-builds-campaign-chain-arrays.md`](53-nextlevel-builds-campaign-chain-arrays.md) | Startup builds normal and tutorial level chains by following `NextLevel` references |
| 54 | [`findings/54-terrain-grid-emits-per-cell-quad-vertices.md`](54-terrain-grid-emits-per-cell-quad-vertices.md) | Terrain initialization builds and submits four vertex records for each grid cell |
| 55 | [`findings/55-terrain-cell-material-changes-use-bounded-transition-records.md`](55-terrain-cell-material-changes-use-bounded-transition-records.md) | Cell material changes update the surface and vertices, with a gated ten-record path |
| 56 | [`findings/56-incident-message-template-format.md`](56-incident-message-template-format.md) | Configured incident text compiles underscores, line breaks, and `$token$` spans |
| 57 | [`findings/57-incident-message-advisor-consumer.md`](57-incident-message-advisor-consumer.md) | Queued incident messages feed the scrolling text widget through a gated consumer |
| 58 | [`findings/58-win32-message-pump-filter.md`](58-win32-message-pump-filter.md) | Win32 message pump filters selected messages before translation and dispatch |
| 59 | [`findings/59-directinput-key-state-to-text.md`](59-directinput-key-state-to-text.md) | DirectInput snapshots feed key-edge handling and the text-entry path |
| 60 | [`findings/60-win32-cursor-sampling.md`](60-win32-cursor-sampling.md) | Win32 cursor sampling stores clamped client-relative coordinates and deltas |
| 61 | [`findings/61-cursor-screen-ray-camera-routing.md`](61-cursor-screen-ray-camera-routing.md) | Gameplay cursor coordinates feed a camera unprojection and ray path |
| 62 | [`findings/62-three-dimensional-sound-distance-controls.md`](62-three-dimensional-sound-distance-controls.md) | Configured 3D sound distance and rolloff values update DirectSound interfaces |
| 63 | [`findings/63-sound-instance-buffer-rotation-and-routing.md`](63-sound-instance-buffer-rotation-and-routing.md) | Sound submissions rotate duplicate-enabled buffers and separate streaming from in-memory playback |
| 64 | [`findings/64-wad-stream-handle-contract.md`](64-wad-stream-handle-contract.md) | WAD opens materialize per-open buffers behind the generic stream handle |
| 65 | [`findings/65-wad-end-relative-seek-boundary.md`](65-wad-end-relative-seek-boundary.md) | WAD end-relative seeks bypass the range clamp; out-of-range reads carry a derived risk |
| 66 | [`findings/66-front-end-options-round-trip-through-six-save-contexts.md`](66-front-end-options-round-trip-through-six-save-contexts.md) | Three front-end option globals round-trip through six per-slot contexts |
| 67 | [`findings/67-save-slot-write-includes-option-context.md`](67-save-slot-write-includes-option-context.md) | Save-slot headers serialize the selected option context along with level records |
| 68 | [`findings/68-avi-frame-timebase.md`](68-avi-frame-timebase.md) | AVI frame indices derive from elapsed time and stream rate |
| 69 | [`findings/69-avi-stream-open-and-teardown.md`](69-avi-stream-open-and-teardown.md) | AVI setup stages retain stream state across later failures before teardown |
| 70 | [`findings/70-activity-change-dispatch-and-consumption.md`](70-activity-change-dispatch-and-consumption.md) | Activity pointer changes select typed runners, then commit timer and state |
| 71 | [`findings/71-save-slot-read-restores-level-markers-not-the-live-task-block.md`](71-save-slot-read-restores-level-markers-not-the-live-task-block.md) | Slot reads feed campaign-list completion markers; live task restoration was not observed |
| 72 | [`findings/72-osf-reader-return-means-open-not-validated.md`](72-osf-reader-return-means-open-not-validated.md) | `.osf` reader's success result records file-open, even when validation rejects data |
| 73 | [`findings/73-flh-caller-gated-playback.md`](73-flh-caller-gated-playback.md) | `.flh` advancement is caller-gated while the current surface continues drawing |
| 74 | [`findings/74-reward-flh-context-lifetime.md`](74-reward-flh-context-lifetime.md) | Reward `.flh` contexts load lazily per task and close on transition or exit |
| 75 | [`findings/75-olistfile-entry-placement.md`](75-olistfile-entry-placement.md) | Level `.ol` loading creates typed entities from required entry fields |
| 76 | [`findings/76-entity-teardown-drains-command-chain-by-flags.md`](76-entity-teardown-drains-command-chain-by-flags.md) | Entity teardown drains command chains through flag-dependent disposition paths |
| 77 | [`findings/77-selection-clear-marker.md`](77-selection-clear-marker.md) | A zero-mode selection action clears active selection and transfers a per-entity marker |
| 78 | [`findings/78-energy-low-target-routing.md`](78-energy-low-target-routing.md) | Energy decay clamps a per-entity resource field using type stats |
| 79 | [`findings/79-terrain-voxel-mutation-stages.md`](79-terrain-voxel-mutation-stages.md) | Terrain mutation advances selected voxel states through a four-step progression |
| 80 | [`findings/80-kind-four-deposit-activity-gate.md`](80-kind-four-deposit-activity-gate.md) | Kind-4 entities enter `Activity_Deposit` through a gated state transition |
| 81 | [`findings/81-emergemap-seeds-delayed-lava-erosion-stages.md`](81-emergemap-seeds-delayed-lava-erosion-stages.md) | `EmergeMap` seeds a bounded table of delayed erosion cells |
| 82 | [`findings/82-healthbar-config-derives-clamped-border-shades.md`](82-healthbar-config-derives-clamped-border-shades.md) | Health-bar config derives lighter/darker border colors and clamps channels |
| 83 | [`findings/83-olistfile-driving-references.md`](83-olistfile-driving-references.md) | `.ol` loading resolves optional `driving` names in a second pass |
| 84 | [`findings/84-entity-teardown-dispatches-type-specific-payload-cleanup.md`](84-entity-teardown-dispatches-type-specific-payload-cleanup.md) | Entity teardown dispatches payload cleanup by raw type, with a type-3 gate |
| 85 | [`findings/85-selection-candidate-eligibility.md`](85-selection-candidate-eligibility.md) | Selection candidates pass flag, kind, and nonnegative-float eligibility gates |
| 86 | [`findings/86-low-resource-targets-queue-type-eleven-command.md`](86-low-resource-targets-queue-type-eleven-command.md) | Low-resource entities queue a type-0x11 target command after route checks |
| 87 | [`findings/87-other-voxel-states-refresh-neighborhood-flags.md`](87-other-voxel-states-refresh-neighborhood-flags.md) | Other voxel states run a separate neighborhood-flag refresh path |
| 88 | [`findings/88-neighbor-scan-consumes-terrain-work-flags.md`](88-neighbor-scan-consumes-terrain-work-flags.md) | A neighbor scan consumes terrain work flags through mutation mode 1 |
| 89 | [`findings/89-kind-four-deposit-monitor-uses-count-and-state-gates.md`](89-kind-four-deposit-monitor-uses-count-and-state-gates.md) | A kind-4 monitor gates deposit handoff on count and state |
| 90 | [`findings/90-lava-erosion-ticker-advances-pending-cells.md`](90-lava-erosion-ticker-advances-pending-cells.md) | Lava erosion ticker advances queued cells through timed stages |
| 91 | [`findings/91-olistfile-health-defaults-and-seeds-entity-field.md`](91-olistfile-health-defaults-and-seeds-entity-field.md) | Optional `.ol` health defaults zero to 100 before entity initialization |
| 93 | [`findings/93-objective-advisor-stream-token-tables.md`](93-objective-advisor-stream-token-tables.md) | Objective/advisor streams build separate resource and sound token tables |
| 94 | [`findings/94-objective-advisor-renderer-consumes-inline-tokens.md`](94-objective-advisor-renderer-consumes-inline-tokens.md) | Objective/advisor rendering consumes inline resource and sound tokens |
| 95 | [`findings/95-global-block-record-arena-is-released-at-shutdown.md`](95-global-block-record-arena-is-released-at-shutdown.md) | Block-text records accumulate in a process-wide arena until shutdown |
| 96 | [`findings/96-whole-file-loader-scopes-stream-and-buffer-lifetimes.md`](96-whole-file-loader-scopes-stream-and-buffer-lifetimes.md) | Whole-file loads close the stream and return a separate heap buffer |
| 97 | [`findings/97-explicit-entity-route-associates-target-pointer.md`](97-explicit-entity-route-associates-target-pointer.md) | Explicit entity routes bind a supplied pointer after route setup |
| 98 | [`findings/98-helper-returned-entity-route-marker.md`](98-helper-returned-entity-route-marker.md) | A helper-returned pointer follows a separate route-marker path |
| 99 | [`findings/99-upgrade-admission-tests-level-and-resource-choice.md`](99-upgrade-admission-tests-level-and-resource-choice.md) | Upgrade admission checks the next index and configured resource thresholds |
| 100 | [`findings/100-upgrade-command-advances-building-index.md`](100-upgrade-command-advances-building-index.md) | Upgrade command commits the next building index |
| 101 | [`findings/101-camera-key-hold-and-gates.md`](101-camera-key-hold-and-gates.md) | Camera key adjustments repeat while held behind gameplay and input gates |
| 102 | [`findings/102-camera-config-pan-and-follow-bounds.md`](102-camera-config-pan-and-follow-bounds.md) | Camera settings feed pointer-pan acceleration and follow bounds |
| 103 | [`findings/103-pointer-cursor-config-registry.md`](103-pointer-cursor-config-registry.md) | Pointer cursor config uses a fixed action-name registry |
| 104 | [`findings/104-pointer-overrides-expire-in-the-reward-pump.md`](104-pointer-overrides-expire-in-the-reward-pump.md) | Timed cursor overrides hold through reward-screen input polling |
| 105 | [`findings/105-sav-write-ignores-short-write-results.md`](105-sav-write-ignores-short-write-results.md) | Slot-save success ignores partial write results |
| 106 | [`findings/106-sav-reader-resets-slot-on-record-count-mismatch.md`](106-sav-reader-resets-slot-on-record-count-mismatch.md) | A save record-count mismatch reinitializes the slot |
| 107 | [`findings/107-samples-config-sound-alias-variants.md`](107-samples-config-sound-alias-variants.md) | Samples config builds duplicate/stream sound aliases and selects variants |
| 108 | [`findings/108-streamed-wav-format-extension.md`](108-streamed-wav-format-extension.md) | Streamed WAV parser preserves extensions before a PCM-only playback gate |
| 109 | [`findings/109-emergemap-layerl-records.md`](109-emergemap-layerl-records.md) | EmergeMap values populate six-slot LayerL records |
| 110 | [`findings/110-objective-entity-paired-field-predicate.md`](110-objective-entity-paired-field-predicate.md) | A delegated objective predicate searches entities for paired-field matches |
| 111 | [`findings/111-directdraw-windowed-exclusive-surfaces.md`](111-directdraw-windowed-exclusive-surfaces.md) | Windowed and exclusive modes build different DirectDraw surfaces |
| 112 | [`findings/112-registry-accessor-creates-path-keys.md`](112-registry-accessor-creates-path-keys.md) | The registry read accessor creates each subkey path component |
| 113 | [`findings/113-window-activation-key-sampling.md`](113-window-activation-key-sampling.md) | Focus loss suppresses keyboard sampling while the frame callback continues |
| 114 | [findings/114-terrain-cell-height-and-diagonal-selection.md](114-terrain-cell-height-and-diagonal-selection.md) | TerrainCreate separates map height from cell attributes and selects a diagonal |
| 115 | [findings/115-ptl-message-remap-table.md](115-ptl-message-remap-table.md) | PTLFile builds an ordered table of numeric Message-name pairs |
| 116 | [findings/116-reward-button-color-key-setup.md](116-reward-button-color-key-setup.md) | Reward button image handles receive a zero-to-zero color key |
| 117 | [findings/117-fallin-multiplier-config-source.md](117-fallin-multiplier-config-source.md) | FallinMultiplier supplies the block-pointer modulus factor |
| 118 | [findings/118-loader-progress-read-tap.md](118-loader-progress-read-tap.md) | Whole-file read hooks accumulate loader progress from measured file sizes |
| 119 | [findings/119-directsound-3d-listener-interface.md](119-directsound-3d-listener-interface.md) | Sound initialization queries DirectSound's 3D listener interface |
| 120 | [findings/120-sound-load-duplicates-directsound-buffer.md](120-sound-load-duplicates-directsound-buffer.md) | Sound loading can create two DirectSound duplicates for one sample buffer |
| 121 | [findings/121-per-source-sound-buffers-request-3d-control.md](121-per-source-sound-buffers-request-3d-control.md) | Sound buffers request 3D control and tracked buffers receive distance updates |
| 122 | [findings/122-wav-mmio-buffer-handoff.md](122-wav-mmio-buffer-handoff.md) | In-memory WAV loading copies the data chunk through the MMIO read window |
| 123 | [findings/123-avi-frame-dib-row-copy.md](123-avi-frame-dib-row-copy.md) | AVI frame helper copies DIB rows into the surface pixel buffer |
| 125 | [findings/125-screen-shake-flag-raiser.md](125-screen-shake-flag-raiser.md) | Negative entity health delta raises the screen-shake flag |
| 126 | [findings/126-screen-space-effect-lane-dispatch-and-opacity-update.md](126-screen-space-effect-lane-dispatch-and-opacity-update.md) | Ten-entry effect table dispatches timed opacity updates |
| 128 | [findings/128-permitted-activity-appender-capacity.md](128-permitted-activity-appender-capacity.md) | Activity appender reads capacity from the entity stats block |
| 129 | [findings/129-effect-item-flags-reach-model-render-dispatch.md](129-effect-item-flags-reach-model-render-dispatch.md) | Effect item flags can reach model render passes |
| 130 | [findings/130-activity-3f4-direct-reader-census.md](130-activity-3f4-direct-reader-census.md) | No other direct entity-field reference to the appender's `+0x3f4` store |
| 131 | [findings/131-tool-carry-stat-is-activity-capacity.md](131-tool-carry-stat-is-activity-capacity.md) | Stats `NumOfToolsCanCarry` values feed activity-list capacity |
| 133 | [findings/133-directsound-duplicate-buffer-selection.md](133-directsound-duplicate-buffer-selection.md) | Optional DirectSound duplicates rotate across three buffers at playback |
| 134 | [findings/134-samples-asterisk-enables-buffer-rotation.md](134-samples-asterisk-enables-buffer-rotation.md) | `Samples` asterisk marker requests duplicate buffers and three-buffer rotation |
| 135 | [findings/135-stream-marker-persists-across-sample-variants.md](135-stream-marker-persists-across-sample-variants.md) | `@` stream mode persists across later comma-delimited sample variants |
| 136 | [findings/136-streamed-playback-replaces-selected-stream-state.md](136-streamed-playback-replaces-selected-stream-state.md) | Starting a stream replaces the active stream in the selected state |
| 137 | [findings/137-stream-selector-caller-trace.md](137-stream-selector-caller-trace.md) | Level-name streams use selector zero; sound-instance submissions pass one |
| 138 | [findings/138-damage-hurt-sound-submits-with-selector-zero.md](138-damage-hurt-sound-submits-with-selector-zero.md) | Damage hurt-sound submission passes selector zero; the shipped alias is in-memory WAV |
| 139 | [findings/139-sound-submission-queue-preserves-selector-bits.md](139-sound-submission-queue-preserves-selector-bits.md) | Deferred sound submissions preserve selector and route bits |
| 140 | [findings/140-objective-outcome-streams-use-selector-zero.md](140-objective-outcome-streams-use-selector-zero.md) | Objective-outcome voice streams submit through selector zero |
| 141 | [findings/141-menu-overlay-streams-submit-on-state-zero.md](141-menu-overlay-streams-submit-on-state-zero.md) | Menu-overlay streams submit one queue step later through state zero |
| 142 | [findings/142-sample-hash-marker-sets-directsound-volume.md](142-sample-hash-marker-sets-directsound-volume.md) | `#...#` sets a per-sample DirectSound attenuation value |
| 143 | [findings/143-reducesamples-skips-bang-prefixed-sound-aliases.md](143-reducesamples-skips-bang-prefixed-sound-aliases.md) | `-reducesamples` skips `!`-prefixed aliases before registration |
| 144 | [findings/144-music-stream-aliases-use-state-one.md](144-music-stream-aliases-use-state-one.md) | Shipped ambient and stats music streams select stream state one |
| 145 | [findings/145-sample-hash-marker-parser-omits-termination-and-bounds.md](145-sample-hash-marker-parser-omits-termination-and-bounds.md) | `#...#` parser can read past its token or overwrite its stack buffer |
| 146 | [findings/146-streamed-samples-ignore-asterisk-duplicate-flag.md](146-streamed-samples-ignore-asterisk-duplicate-flag.md) | `@` streaming bypasses the `*` duplicate-buffer path |
| 147 | [findings/147-sample-variant-choice-uses-shared-seeded-lcg.md](147-sample-variant-choice-uses-shared-seeded-lcg.md) | Sample variant selection consumes the shared LCG state |
| 148 | [findings/148-global-wav-playback-applies-eight-db-attenuation.md](148-global-wav-playback-applies-eight-db-attenuation.md) | Global in-memory WAV playback subtracts 8 dB before SetVolume |
| 149 | [findings/149-deferred-sound-queue-drops-requests-after-ten.md](149-deferred-sound-queue-drops-requests-after-ten.md) | Deferred sound queue silently drops requests after ten entries |
| 150 | [findings/150-streamed-wav-playback-rejects-non-pcm.md](150-streamed-wav-playback-rejects-non-pcm.md) | Streamed playback rejects non-PCM WAV formats before buffer creation |
| 151 | [findings/151-streamed-pcm-buffer-uses-four-aligned-segments.md](151-streamed-pcm-buffer-uses-four-aligned-segments.md) | Streamed PCM buffer uses four aligned segments totaling about three seconds |
| 152 | [findings/152-streamed-playback-reuses-buffer-as-looping-ring.md](152-streamed-playback-reuses-buffer-as-looping-ring.md) | Streamed playback loops and refills its DirectSound ring buffer |
| 153 | [findings/153-stream-start-ignores-directsound-position-and-play-errors.md](153-stream-start-ignores-directsound-position-and-play-errors.md) | Stream start ignores position and Play HRESULTs before marking active |
| 154 | [findings/154-streamed-playback-stops-after-source-data-drains.md](154-streamed-playback-stops-after-source-data-drains.md) | Streamed playback pads the tail and stops after queued data drains |
| 155 | [findings/155-shipped-music-streams-exceed-ring-buffer.md](155-shipped-music-streams-exceed-ring-buffer.md) | Shipped music tracks exceed the three-second streamed ring buffer |
| 156 | [findings/156-options-sfx-volume-uses-nonlinear-directsound-table.md](156-options-sfx-volume-uses-nonlinear-directsound-table.md) | Options SFX slider maps to a nonlinear DirectSound attenuation table |
| 157 | [findings/157-music-volume-slider-collapses-top-two-values.md](157-music-volume-slider-collapses-top-two-values.md) | Music volume values 9 and 10 both request maximum mixer level |
| 158 | [findings/158-sfx-volume-slider-sets-primary-directsound-volume.md](158-sfx-volume-slider-sets-primary-directsound-volume.md) | SFX-labeled slider sets the shared primary DirectSound output volume |
| 159 | [findings/159-music-volume-targets-compact-disc-mixer-source.md](159-music-volume-targets-compact-disc-mixer-source.md) | Music volume callback targets the audio-CD mixer source |
| 160 | [findings/160-music-volume-defaults-to-zero-without-cd-mixer-control.md](160-music-volume-defaults-to-zero-without-cd-mixer-control.md) | Music slider starts at zero when the CD mixer source is unavailable |
| 161 | [findings/161-gameplay-uses-positional-and-object-attached-sfx.md](161-gameplay-uses-positional-and-object-attached-sfx.md) | Gameplay and shared-object events submit aliases through spatial sound paths |
| 162 | [findings/162-object-attached-sounds-refresh-spatial-state.md](162-object-attached-sounds-refresh-spatial-state.md) | Object-attached sounds refresh stored spatial values during sound updates |
| 163 | [findings/163-view-provider-feeds-directsound-listener.md](163-view-provider-feeds-directsound-listener.md) | The frame tick updates listener position and orientation from the stored view |
| 164 | [findings/164-explicit-sound-positions-apply-immediately.md](164-explicit-sound-positions-apply-immediately.md) | Explicit positions apply immediately; dispatcher also commits deferred settings |
| 165 | [findings/165-object-spatial-update-registry-has-twenty-slots.md](165-object-spatial-update-registry-has-twenty-slots.md) | Object spatial-update registry has 20 slots and does not deduplicate |
| 166 | [findings/166-spatial-playback-enters-through-shared-wrapper.md](166-spatial-playback-enters-through-shared-wrapper.md) | Direct spatial playback uses the shared positional/object wrapper |
| 167 | [findings/167-global-alias-playback-disables-3d-mode.md](167-global-alias-playback-disables-3d-mode.md) | Global in-memory aliases disable DirectSound 3D processing |
| 168 | [findings/168-in-memory-sound-start-ignores-hresults.md](168-in-memory-sound-start-ignores-hresults.md) | In-memory sound start returns a slot identifier without checking DirectSound HRESULTs |
| 169 | [findings/169-shared-object-sound-events-use-time-windows.md](169-shared-object-sound-events-use-time-windows.md) | Shared-object sound events submit object-attached aliases within time windows |
| 170 | [findings/170-shared-object-transition-stops-new-sound-handle.md](170-shared-object-transition-stops-new-sound-handle.md) | Shared-object transition may stop the newly returned sound handle |

| 171 | [findings/171-queued-shared-sound-records-zero-handle.md](171-queued-shared-sound-records-zero-handle.md) | Queued Shared-Object Sound Stores Handle Zero |
| 172 | [findings/172-avi-frames-blit-to-render-target.md](172-avi-frames-blit-to-render-target.md) | AVI Frames Blit to the Shared DirectDraw Render Target |
| 173 | [findings/173-avi-allocation-failure-reaches-frame-updater.md](173-avi-allocation-failure-reaches-frame-updater.md) | AVI Allocation Failure Reaches a Null Frame Updater |
| 174 | [findings/174-avi-frame-retriever-is-not-explicitly-closed.md](174-avi-frame-retriever-is-not-explicitly-closed.md) | AVI Frame Retriever Has No Explicit Close Path |
| 175 | [findings/175-null-avi-frame-reblits-previous-pixels.md](175-null-avi-frame-reblits-previous-pixels.md) | Null AVI Frame Fetch Leaves and Reblits Previous Pixels |
| 176 | [findings/176-avi-command-loop-restarts-at-stream-length.md](176-avi-command-loop-restarts-at-stream-length.md) | AVI Command Loop Restarts at Stream Length |
| 177 | [findings/177-avi-player-ignores-embedded-audio-streams.md](177-avi-player-ignores-embedded-audio-streams.md) | AVI Player Opens Video Streams but Not Embedded Audio |
| 178 | [findings/178-dynamic-sample-alias-table-has-no-capacity-check.md](178-dynamic-sample-alias-table-has-no-capacity-check.md) | Dynamic Sample Alias Registration Can Overrun Its Hash Table |
| 180 | [findings/180-cdda-watchdog-detects-track-overrun-at-four-second-intervals.md](180-cdda-watchdog-detects-track-overrun-at-four-second-intervals.md) | CD Audio Watchdog Detects Track Overrun at Four-Second Intervals |
| 181 | [findings/181-avi-command-loop-input-exit.md](181-avi-command-loop-input-exit.md) | AVI Command Loop Exits on Keyboard Input or Mouse Release |
| 182 | [findings/182-credits-scroll-uses-33ms-clock-steps.md](182-credits-scroll-uses-33ms-clock-steps.md) | Credits Scroll Uses 33 ms Clock Steps |
| 183 | [findings/183-shipped-credits-text-matches-crlf-split-count.md](183-shipped-credits-text-matches-crlf-split-count.md) | Shipped Credits Text Matches the CRLF Split Count |
| 184 | [findings/184-credits-text-prefers-cconv-sidecar.md](184-credits-text-prefers-cconv-sidecar.md) | Credits Text Lookup Prefers a `.cconv` Sidecar |
| 185 | [findings/185-successful-avi-loop-has-no-explicit-sleep.md](185-successful-avi-loop-has-no-explicit-sleep.md) | Successful AVI Playback Has No Explicit Loop Sleep |
| 186 | [findings/186-credits-background-bypasses-dontplayavis-gate.md](186-credits-background-bypasses-dontplayavis-gate.md) | `DontPlayAvis` Gates Boot Movies but Not Other Video Paths |
| 187 | [findings/187-video-commands-use-two-playback-routes.md](187-video-commands-use-two-playback-routes.md) | Configured Videos Use DirectShow; Credits Use AVIFIL32 |
| 188 | [findings/188-com-video-route-does-not-explicitly-uninitialize.md](188-com-video-route-does-not-explicitly-uninitialize.md) | COM Video Route Has No Explicit `CoUninitialize` Call |
| 189 | [findings/189-configured-videos-add-primary-audio-stream.md](189-configured-videos-add-primary-audio-stream.md) | Configured Videos Add Primary Video and Audio Streams |
| 190 | [findings/190-directshow-partial-init-cleanup-assumes-interfaces.md](190-directshow-partial-init-cleanup-assumes-interfaces.md) | DirectShow Partial-Setup Cleanup Assumes Every Interface Exists |
| 191 | [findings/191-level-video-stops-on-button-down.md](191-level-video-stops-on-button-down.md) | Level Video Stops on Mouse Button Down |
| 192 | [findings/192-shipped-boot-avis-have-audio-streams.md](192-shipped-boot-avis-have-audio-streams.md) | Shipped Boot AVIs Have Audio Streams; Credits Loop Does Not |
| 193 | [findings/193-media-class-creation-failure-releases-null.md](193-media-class-creation-failure-releases-null.md) | Failed Media-Class Creation Reaches `Release` Through Null |
| 194 | [findings/194-configured-video-graph-runs-on-separate-thread.md](194-configured-video-graph-runs-on-separate-thread.md) | Configured Video Graph Runs on a Separate Thread |
| 195 | [findings/195-cdda-watchdog-parses-current-track-without-checking-mci-result.md](195-cdda-watchdog-parses-current-track-without-checking-mci-result.md) | CD-Audio Watchdog Parses Track Status Without Checking MCI Success |
| 196 | [findings/196-listener-uses-scene-relative-camera-frame-vectors.md](196-listener-uses-scene-relative-camera-frame-vectors.md) | The Listener Uses Scene-Relative Frame Vectors |
| 197 | [findings/197-listener-caches-deferred-values-without-checking-results.md](197-listener-caches-deferred-values-without-checking-results.md) | Listener Caches Deferred Values Without Checking Results |
| 198 | [findings/198-attached-sound-caches-position-after-immediate-set.md](198-attached-sound-caches-position-after-immediate-set.md) | Attached Sound Caches Position After Immediate Set |
| 199 | [findings/199-directsound-keeps-default-distance-factor.md](199-directsound-keeps-default-distance-factor.md) | DirectSound Keeps Its Default Distance Factor |
| 200 | [findings/200-configured-video-polls-directshow-sample.md](200-configured-video-polls-directshow-sample.md) | Configured Video Pumps and Blits DirectShow Samples |
| 201 | [findings/201-configured-video-stops-after-update-failure.md](201-configured-video-stops-after-update-failure.md) | Configured Video Stops When Its Update Callback Fails |
| 202 | [findings/202-video-duration-only-bounds-render-count.md](202-video-duration-only-bounds-render-count.md) | Video Duration Bounds the Render Count, Not Frame Timing |
| 203 | [findings/203-spatial-sound-ignores-frame-query-failures.md](203-spatial-sound-ignores-frame-query-failures.md) | Spatial Sound Ignores Frame-Query Failures |
| 204 | [findings/204-stopping-one-attached-sound-clears-all-provider-entries.md](204-stopping-one-attached-sound-clears-all-provider-entries.md) | Stopping One Attached Sound Clears All Registry Entries for Its Frame |
| 205 | [findings/205-directshow-video-uses-shared-render-presentation.md](205-directshow-video-uses-shared-render-presentation.md) | DirectShow Video Uses the Shared Render Presentation Path |
| 206 | [findings/206-configured-video-blit-fills-render-surface.md](206-configured-video-blit-fills-render-surface.md) | Configured Video Blits Fill the Render Surface |
| 207 | [findings/207-configured-video-releases-without-explicit-stop.md](207-configured-video-releases-without-explicit-stop.md) | Configured Video Releases Its Stream Without an Explicit Stop Call |
| 208 | [findings/208-video-setup-ignores-audio-stream-add-failure.md](208-video-setup-ignores-audio-stream-add-failure.md) | Configured Video Continues if Adding Primary Audio Fails |
| 209 | [findings/209-batched-spatial-sound-queue-drops-requests-after-ten.md](209-batched-spatial-sound-queue-drops-requests-after-ten.md) | Batched Spatial Sound Queue Drops Requests After Ten |
| 210 | [findings/210-avi-frame-copy-proceeds-after-lock-failure.md](210-avi-frame-copy-proceeds-after-lock-failure.md) | AVI Frame Copy Proceeds After Surface Lock Failure |
| 211 | [findings/211-advisor-sound-token-is-latched-before-audio-gate.md](211-advisor-sound-token-is-latched-before-audio-gate.md) | Advisor Sound Token Is Latched Before the Audio Gate |
| 212 | [findings/212-frame-sound-tick-does-not-drain-packed-batch-queue.md](212-frame-sound-tick-does-not-drain-packed-batch-queue.md) | Frame Sound Tick Does Not Drain the Packed Batch Queue |
| 213 | [findings/213-disabling-sound-does-not-clear-packed-requests.md](213-disabling-sound-does-not-clear-packed-requests.md) | Disabling Sound Does Not Clear Packed Requests |
| 214 | [findings/214-objective-voice-timer-does-not-stop-stream.md](214-objective-voice-timer-does-not-stop-stream.md) | Objective Voice Timer Does Not Stop the Stream |
| 215 | [findings/215-in-memory-wav-failures-leave-mmio-handle-open.md](215-in-memory-wav-failures-leave-mmio-handle-open.md) | In-Memory WAV Failures Leave the MMIO Handle Open |
| 216 | [findings/216-avi-decoder-requests-best-display-format.md](216-avi-decoder-requests-best-display-format.md) | AVI Decoder Is Asked for the Best Display Format |
| 217 | [findings/217-streamed-wav-registration-ignores-format-rate-failure.md](217-streamed-wav-registration-ignores-format-rate-failure.md) | Streamed WAV Registration Ignores a Failed Format-Rate Probe |
| 218 | [findings/218-streamed-wav-metadata-probe-bypasses-game-file-layer.md](218-streamed-wav-metadata-probe-bypasses-game-file-layer.md) | Streamed WAV Metadata Probe Bypasses the Game File Layer |
| 219 | [findings/219-directshow-end-of-stream-takes-sample-cleanup-path.md](219-directshow-end-of-stream-takes-sample-cleanup-path.md) | DirectShow End of Stream Takes the Sample Cleanup Path |
| 220 | [findings/220-avi-unsupported-surface-depth-still-advances.md](220-avi-unsupported-surface-depth-still-advances.md) | AVI Timeline Advances When the Movie Surface Depth Is Unsupported |
| 221 | [findings/221-lava-effect-latches-zero-sound-handle.md](221-lava-effect-latches-zero-sound-handle.md) | Lava Effect Latches a Zero Sound Handle |
| 222 | [findings/222-wav-fmt-extension-read-is-not-chunk-bounded.md](222-wav-fmt-extension-read-is-not-chunk-bounded.md) | WAV Format Extension Read Is Not Bounded by the `fmt ` Chunk |
| 223 | [findings/223-cell-init-reads-outside-grid-at-top-and-left-edges.md](223-cell-init-reads-outside-grid-at-top-and-left-edges.md) | Conditional top/left edge read before the cell grid; Level14/17 tutorial spawns and two cell-flag writers are traced, while edge walkability and runtime ordering remain unknown |
| 224 | [findings/224-predug-cells-feed-a-capped-spawn-coordinate-pool.md](224-predug-cells-feed-a-capped-spawn-coordinate-pool.md) | Predug Cell Values Feed a Capped Coordinate Pool |
| 225 | [findings/225-terrain-objective-level-and-diagnostic-helpers.md](225-terrain-objective-level-and-diagnostic-helpers.md) | Newly Identified Terrain, Objective, Level, and Diagnostic Helpers |
| 226 | [findings/226-omitted-simulation-terrain-resource-text-procedures.md](226-omitted-simulation-terrain-resource-text-procedures.md) | Omitted Simulation, Terrain, Resource, and Text Procedures |
| 227 | [findings/227-newly-identified-engine-and-terrain-helpers.md](227-newly-identified-engine-and-terrain-helpers.md) | Newly Identified Engine and Terrain Helpers |
| 228 | [findings/228-entity-pool-terrain-and-timed-effect-helpers.md](228-entity-pool-terrain-and-timed-effect-helpers.md) | Entity-Pool, Terrain, and Timed-Effect Helpers |
| 229 | [findings/229-wwad-archive-and-render-resource-helpers.md](229-wwad-archive-and-render-resource-helpers.md) | WWAD Archive and Render-Resource Helpers |
| 230 | [findings/230-engine-input-selection-and-resource-helpers.md](230-engine-input-selection-and-resource-helpers.md) | Engine Input, Selection, and Resource Helpers |
| 231 | [findings/231-terrain-entity-ui-and-stream-helpers.md](231-terrain-entity-ui-and-stream-helpers.md) | Terrain, Entity, UI, and Stream Helpers |
| 232 | [findings/232-formatted-text-rendering-and-font-helpers.md](232-formatted-text-rendering-and-font-helpers.md) | Formatted Text Rendering and Font Helpers |
| 233 | [findings/233-terrain-rendering-and-stream-primitives.md](233-terrain-rendering-and-stream-primitives.md) | Terrain, Rendering, and Stream Primitives |
| 234 | [findings/234-game-file-streams-and-surface-pixels.md](234-game-file-streams-and-surface-pixels.md) | Game-File Streams and Surface Pixel Operations |
| 235 | [findings/235-camera-projection-lwo-layer-state-and-surface-normals.md](235-camera-projection-lwo-layer-state-and-surface-normals.md) | Camera Projection, LWO Layer State, and Surface Normals |
| 236 | [findings/236-terrain-labels-map-bounds-and-selection-predicate.md](236-terrain-labels-map-bounds-and-selection-predicate.md) | Terrain Labels, Map Bounds, and Selection Predicate |
| 237 | [findings/237-buffered-stream-and-win32-file-helpers.md](237-buffered-stream-and-win32-file-helpers.md) | Buffered Streams and Win32 File Helpers |
| 238 | [findings/238-reward-terrain-and-simulation-helpers.md](238-reward-terrain-and-simulation-helpers.md) | Reward, Terrain, and Simulation Helpers |
| 239 | [findings/239-mesh-segment-and-scan-runtime.md](239-mesh-segment-and-scan-runtime.md) | Mesh Segment Builder and Scan Runtime |
| 240 | [findings/240-cursor-interaction-overlay-menu-and-scene-light-helpers.md](240-cursor-interaction-overlay-menu-and-scene-light-helpers.md) | Cursor Interaction, Overlay, Menu, and Scene-Light Helpers |
| 241 | [findings/241-terrain-quad-run-upgrade-and-scene-slot-helpers.md](241-terrain-quad-run-upgrade-and-scene-slot-helpers.md) | Terrain/Quad-Run, Upgrade-Table, and Scene-Slot Helpers |
| 242 | [findings/242-path-step-terrain-and-pending-record-helpers.md](242-path-step-terrain-and-pending-record-helpers.md) | Path-Step, Terrain, and Pending-Record Helpers |
| 243 | [findings/243-config-loader-file-descriptor-and-crt-helpers.md](243-config-loader-file-descriptor-and-crt-helpers.md) | Config-Loader, File-Descriptor, and CRT Helpers |
| 244 | [findings/244-config-file-text-layout-and-record-emit-helpers.md](244-config-file-text-layout-and-record-emit-helpers.md) | Config-File, Text-Layout, and Record-Emit Helpers |
| 245 | [findings/245-region-route-light-config-and-surface-helpers.md](245-region-route-light-config-and-surface-helpers.md) | Region-Route, Light-Config, and Surface Helpers |
| 246 | [findings/246-entity-move-route-command-and-object-clone-helpers.md](246-entity-move-route-command-and-object-clone-helpers.md) | Entity Move/Route Commands, Cell Queue, and Object Clones |
| 247 | [findings/247-config-graphics-camera-tween-and-pool-spawn-helpers.md](247-config-graphics-camera-tween-and-pool-spawn-helpers.md) | Config/Graphics Quality, Camera Tween/Drag, Cell Search, and Pool/Spawn Helpers |
| 248 | [findings/248-config-band-loaders-display-mode-directional-activity-and-flight-record-helpers.md](248-config-band-loaders-display-mode-directional-activity-and-flight-record-helpers.md) | Config-Band Loaders, Display-Mode Setup, Directional Activity, and Flight-Record Helpers |
| 249 | [findings/249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md](249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md) | Config-Band Loaders, Poly-Variant Build, Panel/Cell Registration, Layout, and Activity Helpers |
| 250 | [findings/250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md](250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md) | Config-Band Loaders, Cell-Corner Scan, Action/Command Issues, Record Instantiation, and Render-State Helpers |
| 251 | [findings/251-command-band-kind-dispatch-item-name-lookup-route-and-record-helpers.md](251-command-band-kind-dispatch-item-name-lookup-route-and-record-helpers.md) | Command-Band Kind Dispatch, Item Name Lookup, Route Commands, and Record/Geometry Helpers |
| 252 | [findings/252-config-band-texture-loaders-route-commands-and-surface-menu-helpers.md](252-config-band-texture-loaders-route-commands-and-surface-menu-helpers.md) | Config-Band Texture Loaders, Route Command Issues, and Surface/Menu Helpers |
| 253 | [findings/253-terrain-scatter-erosion-propagation-cell-queries-menu-records-and-buffer-helpers.md](253-terrain-scatter-erosion-propagation-cell-queries-menu-records-and-buffer-helpers.md) | Terrain Scatter, Erosion Propagation, Cell Queries, Menu Records, and Buffer Helpers |
| 254 | [findings/254-command-sweeps-cell-issuing-config-priorities-statistics-and-directdraw-create-helpers.md](254-command-sweeps-cell-issuing-config-priorities-statistics-and-directdraw-create-helpers.md) | Command Sweeps, Cell Issuing, Config Priorities, Statistics, and DirectDraw Create Helpers |
| 255 | [findings/255-visual-channel-relays-resource-tree-callback-walks-scene-pose-and-registry-helpers.md](255-visual-channel-relays-resource-tree-callback-walks-scene-pose-and-registry-helpers.md) | Visual Channel Relays, Resource-Tree Callback Walks, Scene Pose, and Registry Helpers |
| 256 | [findings/256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md](256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md) | Command-Band Step Issuers, Flag-Behavior Timers, and Steering Helpers |
| 257 | [findings/257-configured-tiny-monster-spawning-and-mutual-activity-pairing.md](257-configured-tiny-monster-spawning-and-mutual-activity-pairing.md) | Configured Tiny-Monster Spawning and Mutual Activity Pairing |
| 258 | [findings/258-interface-method-wrappers-overlay-refresh-and-deferred-set-relays.md](258-interface-method-wrappers-overlay-refresh-and-deferred-set-relays.md) | Interface-Method Wrappers, Overlay Refresh, and Deferred-Set Relays |
| 259 | [findings/259-selected-entity-detach-callback-and-pair-record-commit.md](259-selected-entity-detach-callback-and-pair-record-commit.md) | Selected-Entity Detach Callback and Pair-Record Commit |
| 260 | [findings/260-crt-startup-locale-time-and-floating-point-helpers.md](260-crt-startup-locale-time-and-floating-point-helpers.md) | CRT Startup, Locale, Time-Zone, and Floating-Point Helpers |
| 261 | [findings/261-crt-argument-environment-and-error-helpers.md](261-crt-argument-environment-and-error-helpers.md) | CRT Argument, Environment, Error, and Conversion Helpers |
| 262 | [findings/262-scroll-panel-and-viewport-picking-helpers.md](262-scroll-panel-and-viewport-picking-helpers.md) | Scroll Panel and Viewport Picking Helpers |
| 263 | [findings/263-entity-effect-fields-and-nested-record-teardown.md](263-entity-effect-fields-and-nested-record-teardown.md) | Entity Effect Fields and Nested Record Teardown |
| 264 | [findings/264-crt-exception-and-extended-float-helpers.md](264-crt-exception-and-extended-float-helpers.md) | CRT Exception and Extended-Float Helpers |
| 265 | [findings/265-token-directory-scan-and-mesh-coordinate-helpers.md](265-token-directory-scan-and-mesh-coordinate-helpers.md) | Token Directory Scan and Mesh Coordinate Helpers |
| 266 | [findings/266-terrain-triangle-select-and-command-gate.md](266-terrain-triangle-select-and-command-gate.md) | Terrain Triangle Selection and Command Gate |
| 267 | [findings/267-entity-route-commit-and-terrain-activity-gate.md](267-entity-route-commit-and-terrain-activity-gate.md) | Entity Route Commit and Terrain Activity Gate |
| 268 | [findings/268-crt-conversion-scene-hierarchy-and-math-helpers.md](268-crt-conversion-scene-hierarchy-and-math-helpers.md) | CRT Conversion, Scene Hierarchy, and Math Helpers |
| 269 | [findings/269-crt-environment-and-multibyte-string-helpers.md](269-crt-environment-and-multibyte-string-helpers.md) | CRT Environment and Multibyte String Helpers |
| 270 | [findings/270-entity-route-markers-and-tickdown-spawn.md](270-entity-route-markers-and-tickdown-spawn.md) | Entity Route Markers and TickDown Spawn |
| 271 | [findings/271-panel-layout-and-advisor-render-helpers.md](271-panel-layout-and-advisor-render-helpers.md) | Panel Layout and Advisor Render Helpers |
| 272 | [findings/272-crt-file-resize-and-signal-helpers.md](272-crt-file-resize-and-signal-helpers.md) | CRT File Resize and Signal Helpers |
| 273 | [findings/273-terrain-object-sound-grid-and-activity-transition.md](273-terrain-object-sound-grid-and-activity-transition.md) | Terrain Object Sound Grid and Activity Transition |
| 274 | [findings/274-priority-image-selection-and-scene-registry-teardown.md](274-priority-image-selection-and-scene-registry-teardown.md) | Priority Image Selection and Scene Registry Teardown |
| 275 | [findings/275-scene-frame-record-clone-or-create.md](275-scene-frame-record-clone-or-create.md) | SceneAct Frame Record Clone or Create |
| 276 | [findings/276-additional-engine-ui-and-scene-helper-names.md](276-additional-engine-ui-and-scene-helper-names.md) | Additional Engine, UI, and Scene Helper Names |
| 277 | [findings/277-low-address-helper-names.md](277-low-address-helper-names.md) | Low-Address Helper Names |
| 278 | [findings/278-simulation-and-terrain-helper-names.md](278-simulation-and-terrain-helper-names.md) | Simulation and Terrain Helper Names |
| 279 | [findings/279-crt-and-display-helper-names.md](279-crt-and-display-helper-names.md) | CRT and Display Helper Names |
| 280 | [findings/280-vector-math-helper-names.md](280-vector-math-helper-names.md) | Vector Math Helper Names |
| 281 | [findings/281-advisor-terrain-task-helper-names.md](281-advisor-terrain-task-helper-names.md) | Advisor, Terrain, and Task Helper Names |
| 282 | [findings/282-display-resolution-helper-names.md](282-display-resolution-helper-names.md) | Display Resolution Helper Names |
| 283 | [findings/283-low-address-resource-and-geometry-helpers.md](283-low-address-resource-and-geometry-helpers.md) | Low-Address Resource and Geometry Helpers |
| 284 | [findings/284-movement-lighting-and-action-gates.md](284-movement-lighting-and-action-gates.md) | Movement, Lighting, and Action Gates |
| 285 | [findings/285-render-collision-fog-and-segment-helpers.md](285-render-collision-fog-and-segment-helpers.md) | Render, Collision, Fog, and Segment Helpers |
| 286 | [findings/286-task-registry-and-category-mask-helpers.md](286-task-registry-and-category-mask-helpers.md) | Task Registry and Category-Mask Helpers |
| 287 | [findings/287-terrain-cell-queued-work-transition.md](287-terrain-cell-queued-work-transition.md) | Terrain Cell Queued-Work Transition |
| 288 | [findings/288-render-object-and-mesh-helpers.md](288-render-object-and-mesh-helpers.md) | Render Object and Mesh Helpers |
| 289 | [findings/289-statistics-and-adjacent-cell-helpers.md](289-statistics-and-adjacent-cell-helpers.md) | Statistics and Adjacent-Cell Helpers |
| 290 | [findings/290-entity-velocity-state-accessors.md](290-entity-velocity-state-accessors.md) | Entity Velocity State Accessors |
| 291 | [findings/291-tracked-entity-pointer-array-removal.md](291-tracked-entity-pointer-array-removal.md) | Tracked Entity Pointer Array Removal |
| 292 | [findings/292-selection-and-pointer-registry-helpers.md](292-selection-and-pointer-registry-helpers.md) | Selection and Pointer-Registry Helpers |
| 293 | [findings/293-terrain-cell-predicates-and-spatial-lookup.md](293-terrain-cell-predicates-and-spatial-lookup.md) | Terrain Cell Predicates and Spatial Lookup |
| 294 | [findings/294-render-segment-and-object-state-helpers.md](294-render-segment-and-object-state-helpers.md) | Render Segment and Object-State Helpers |
| 295 | [findings/295-engine-mode-and-state-accessors.md](295-engine-mode-and-state-accessors.md) | Engine Mode and State Accessors |
| 296 | [findings/296-entity-effect-counter-record-application.md](296-entity-effect-counter-record-application.md) | Entity Effect-Counter Record Application |
| 297 | [findings/297-activity-record-and-message-table-helpers.md](297-activity-record-and-message-table-helpers.md) | Activity-Record and Message-Table Helpers |
| 298 | [findings/298-render-object-transform-and-string-helpers.md](298-render-object-transform-and-string-helpers.md) | Render-Object Transform and String Helpers |
| 299 | [findings/299-low-address-task-interface-helpers.md](299-low-address-task-interface-helpers.md) | Low-Address Task, Interface, and Grid Helpers |
| 300 | [findings/300-terrain-queue-and-particle-slot-helpers.md](300-terrain-queue-and-particle-slot-helpers.md) | Terrain-Queue and Particle-Slot Helpers |
| 301 | [findings/301-scene-geometry-and-resource-slot-helpers.md](301-scene-geometry-and-resource-slot-helpers.md) | Scene Geometry and Resource-Slot Helpers |
| 302 | [findings/302-crt-character-and-arithmetic-helpers.md](302-crt-character-and-arithmetic-helpers.md) | CRT Character and Arithmetic Helpers |
| 303 | [findings/303-bubble-action-and-string-cache-lookups.md](303-bubble-action-and-string-cache-lookups.md) | Bubble-Action and String-Cache Lookups |
| 304 | [findings/304-terrain-visited-cell-tracking.md](304-terrain-visited-cell-tracking.md) | Terrain Visited-Cell Tracking |
| 305 | [findings/305-entity-velocity-and-scene-position-accessors.md](305-entity-velocity-and-scene-position-accessors.md) | Entity Velocity and Scene-Position Accessors |
| 306 | [findings/306-string-cache-and-interface-message-loaders.md](306-string-cache-and-interface-message-loaders.md) | String-Cache and Interface-Message Loaders |
| 307 | [findings/307-terrain-cell-flag-operations-and-refresh.md](307-terrain-cell-flag-operations-and-refresh.md) | Terrain-Cell Flag Operations and Refresh |
| 308 | [findings/308-effect-mode-and-slot-helpers.md](308-effect-mode-and-slot-helpers.md) | Effect-Mode and Slot Helpers |
| 309 | [findings/309-geometry-and-directdraw-surface-helpers.md](309-geometry-and-directdraw-surface-helpers.md) | Geometry and DirectDraw Surface Helpers |
| 310 | [findings/310-terrain-cell-flag-and-state-tests.md](310-terrain-cell-flag-and-state-tests.md) | Terrain-Cell Flag and State Tests |
| 311 | [findings/311-renderer-fog-and-mesh-color-setters.md](311-renderer-fog-and-mesh-color-setters.md) | Renderer Fog and Mesh Color Setters |
| 312 | [findings/312-object-task-and-registry-helpers.md](312-object-task-and-registry-helpers.md) | Object Task and Registry Helpers |
| 313 | [findings/313-selection-resource-and-transform-helpers.md](313-selection-resource-and-transform-helpers.md) | Selection, Resource, and Transform Helpers |
| 314 | [findings/314-terrain-grid-record-and-flag-helpers.md](314-terrain-grid-record-and-flag-helpers.md) | Terrain Grid, Record, and Flag Helpers |
| 315 | [findings/315-pooled-record-and-display-mode-helpers.md](315-pooled-record-and-display-mode-helpers.md) | Pooled-Record and Display-Mode Helpers |
| 316 | [findings/316-entity-flag-and-fixed-record-pool-helpers.md](316-entity-flag-and-fixed-record-pool-helpers.md) | Entity-Flag and Fixed-Record-Pool Helpers |
| 317 | [findings/317-air-supply-amount-update.md](317-air-supply-amount-update.md) | Air-Supply Amount Update |
| 318 | [findings/318-entity-eligibility-and-dock-helpers.md](318-entity-eligibility-and-dock-helpers.md) | Entity Eligibility and Dock Helpers |
| 319 | [findings/319-entity-stats-and-mask-predicates.md](319-entity-stats-and-mask-predicates.md) | Entity-Stats and Mask Predicates |
| 320 | [findings/320-crt-descriptor-and-extended-float-helpers.md](320-crt-descriptor-and-extended-float-helpers.md) | CRT Descriptor and Extended-Float Helpers |
| 321 | [findings/321-transform-collection-and-surface-config-helpers.md](321-transform-collection-and-surface-config-helpers.md) | Transform-Collection and Surface-Config Helpers |
| 322 | [findings/322-resource-binding-and-map-row-span-helpers.md](322-resource-binding-and-map-row-span-helpers.md) | Resource-Binding and Map-Row-Span Helpers |
| 323 | [findings/323-camera-math-and-matrix-primitives.md](323-camera-math-and-matrix-primitives.md) | Camera Math and Matrix Primitives |
| 324 | [findings/324-global-state-and-record-accessors.md](324-global-state-and-record-accessors.md) | Global-State and Record Accessors |
| 325 | [findings/325-terrain-cell-refresh-and-grid-pick.md](325-terrain-cell-refresh-and-grid-pick.md) | Terrain-Cell Refresh and Grid Pick |
| 326 | [findings/326-render-config-and-grid-record-tables.md](326-render-config-and-grid-record-tables.md) | Render-Config and Grid-Record Tables |
| 327 | [findings/327-entity-threshold-and-flag-mask-predicates.md](327-entity-threshold-and-flag-mask-predicates.md) | Entity Threshold and Flag-Mask Predicates |
| 328 | [findings/328-crt-environment-and-descriptor-helpers.md](328-crt-environment-and-descriptor-helpers.md) | CRT Environment and Descriptor Helpers |
| 329 | [findings/329-viewport-and-global-refresh-helpers.md](329-viewport-and-global-refresh-helpers.md) | Viewport and Global-Refresh Helpers |
| 330 | [findings/330-camera-bound-and-transform-setters.md](330-camera-bound-and-transform-setters.md) | Camera-Bound and Transform Setters |
| 331 | [findings/331-weapon-type-record-accessors.md](331-weapon-type-record-accessors.md) | Weapon-Type Record Accessors |
| 332 | [findings/332-interpolated-geometry-and-entity-record-helpers.md](332-interpolated-geometry-and-entity-record-helpers.md) | Interpolated Geometry and Entity-Record Helpers |
| 333 | [findings/333-advisor-name-and-vector-math-helpers.md](333-advisor-name-and-vector-math-helpers.md) | Advisor-Name and Vector-Math Helpers |
| 334 | [findings/334-camera-bounds-and-global-refresh.md](334-camera-bounds-and-global-refresh.md) | Camera Bounds and Global Refresh |
| 335 | [findings/335-expansion-grid-and-timed-visual-helpers.md](335-expansion-grid-and-timed-visual-helpers.md) | Expansion-Grid and Timed-Visual Helpers |
| 336 | [findings/336-hunger-meter-images-and-entity-override-pop.md](336-hunger-meter-images-and-entity-override-pop.md) | Hunger-Meter Images and Entity Override Pop |
| 337 | [findings/337-scene-object-transform-and-pointer-wrappers.md](337-scene-object-transform-and-pointer-wrappers.md) | Scene-Object Transform and Pointer Wrappers |
| 338 | [findings/338-current-level-state-and-resource-helpers.md](338-current-level-state-and-resource-helpers.md) | Current-Level State and Resource Helpers |
| 339 | [findings/339-camera-distance-and-position-helpers.md](339-camera-distance-and-position-helpers.md) | Camera Distance and Position Helpers |
| 340 | [findings/340-entity-display-and-tool-name-resources.md](340-entity-display-and-tool-name-resources.md) | Entity Display and Tool-Name Resources |
| 341 | [findings/341-animation-and-input-state-helpers.md](341-animation-and-input-state-helpers.md) | Animation and Input-State Helpers |
| 342 | [findings/342-crt-descriptor-and-bounded-string-helpers.md](342-crt-descriptor-and-bounded-string-helpers.md) | CRT Descriptor and Bounded-String Helpers |
| 343 | [findings/343-advisor-action-and-animation-record-helpers.md](343-advisor-action-and-animation-record-helpers.md) | Advisor Action and Animation Record Helpers |
| 344 | [findings/344-indexed-surface-and-pair-copy-helpers.md](344-indexed-surface-and-pair-copy-helpers.md) | Indexed-Surface and Pair-Copy Helpers |
| 345 | [findings/345-sound-object-duration-query.md](345-sound-object-duration-query.md) | Sound-Object Duration Query |
| 346 | [findings/346-screen-surface-allocation-and-clip-distance.md](346-screen-surface-allocation-and-clip-distance.md) | Screen-Surface Allocation and Clip Distance |
| 347 | [findings/347-display-priority-and-allocator-wrappers.md](347-display-priority-and-allocator-wrappers.md) | Display, Priority, and Allocator Wrappers |
| 348 | [findings/348-entity-and-indexed-record-accessors.md](348-entity-and-indexed-record-accessors.md) | Entity and Indexed-Record Accessors |
| 349 | [findings/349-scene-property-and-render-resource-helpers.md](349-scene-property-and-render-resource-helpers.md) | Scene-Property and Render-Resource Helpers |
| 350 | [findings/350-low-address-string-and-global-field-helpers.md](350-low-address-string-and-global-field-helpers.md) | Low-Address String and Global-Field Helpers |
| 351 | [findings/351-text-output-task-table-and-indexed-record-helpers.md](351-text-output-task-table-and-indexed-record-helpers.md) | Text Output, Task Table, and Indexed Record Helpers |
| 352 | [findings/352-scene-interface-pair-and-property-value-helpers.md](352-scene-interface-pair-and-property-value-helpers.md) | Scene Interface Pair and Property Value Helpers |
| 353 | [findings/353-cell-commands-and-entity-activity-helpers.md](353-cell-commands-and-entity-activity-helpers.md) | Cell Commands and Entity Activity Helpers |
| 354 | [findings/354-record-vector-and-resource-state-helpers.md](354-record-vector-and-resource-state-helpers.md) | Record, Vector, and Resource State Helpers |
| 355 | [findings/355-bit-pointer-surface-and-window-wrappers.md](355-bit-pointer-surface-and-window-wrappers.md) | Bit, Pointer, Surface, and Window Wrappers |
| 356 | [findings/356-animation-and-low-address-state-helpers.md](356-animation-and-low-address-state-helpers.md) | Animation and Low-Address State Helpers |
| 357 | [findings/357-entity-fields-and-global-counter-helpers.md](357-entity-fields-and-global-counter-helpers.md) | Entity Fields and Global Counter Helpers |
| 358 | [findings/358-matrix-window-surface-and-heap-helpers.md](358-matrix-window-surface-and-heap-helpers.md) | Matrix, Window, Surface, and Heap Helpers |
| 359 | [findings/359-entity-command-records-and-neighbor-dispatch.md](359-entity-command-records-and-neighbor-dispatch.md) | Entity Command Records and Neighbor Dispatch |
| 360 | [findings/360-grid-animation-and-resource-cleanup-helpers.md](360-grid-animation-and-resource-cleanup-helpers.md) | Grid Animation and Resource Cleanup Helpers |
| 361 | [findings/361-sound-window-and-resource-interface-wrappers.md](361-sound-window-and-resource-interface-wrappers.md) | Sound, Window, and Resource Interface Wrappers |
| 362 | [findings/362-callback-event-and-attached-record-helpers.md](362-callback-event-and-attached-record-helpers.md) | Callback, Event, and Attached-Record Helpers |
| 363 | [findings/363-geometry-string-lookup-and-entity-state-helpers.md](363-geometry-string-lookup-and-entity-state-helpers.md) | Geometry, String Lookup, and Entity State Helpers |
| 364 | [findings/364-bit-count-cursor-and-directory-helpers.md](364-bit-count-cursor-and-directory-helpers.md) | Bit Count, Cursor, and Directory Helpers |
| 365 | [findings/365-renderer-record-and-cached-handle-helpers.md](365-renderer-record-and-cached-handle-helpers.md) | Renderer Record and Cached-Handle Helpers |
| 366 | [findings/366-task-records-callback-dispatch-and-pointer-stacks.md](366-task-records-callback-dispatch-and-pointer-stacks.md) | Task Records, Callback Dispatch, and Pointer Stacks |
| 367 | [findings/367-scene-resources-entity-predicates-and-offset-accessors.md](367-scene-resources-entity-predicates-and-offset-accessors.md) | Scene Resources, Entity Predicates, and Offset Accessors |
| 368 | [findings/368-record-pool-and-recursive-list-helpers.md](368-record-pool-and-recursive-list-helpers.md) | Record Pool and Recursive List Helpers |
| 369 | [findings/369-task-queue-and-entity-state-helpers.md](369-task-queue-and-entity-state-helpers.md) | Task Queue and Entity State Helpers |
| 370 | [findings/370-screen-effects-surface-and-ui-helpers.md](370-screen-effects-surface-and-ui-helpers.md) | Screen Effects, Surface, and UI Helpers |
| 371 | [findings/371-record-pools-pair-tables-and-buffered-streams.md](371-record-pools-pair-tables-and-buffered-streams.md) | Record Pools, Pair Tables, and Buffered Streams |
| 372 | [findings/372-incident-messages-options-and-resource-helpers.md](372-incident-messages-options-and-resource-helpers.md) | Incident Messages, Options, and Resource Helpers |
| 373 | [findings/373-cursor-simulation-renderer-and-file-helpers.md](373-cursor-simulation-renderer-and-file-helpers.md) | Cursor, Simulation, Renderer, and File Helpers |
| 374 | [findings/374-terrain-interface-and-media-helpers.md](374-terrain-interface-and-media-helpers.md) | Terrain, Interface, and Media Helpers |
| 375 | [findings/375-entity-ui-and-render-tree-helpers.md](375-entity-ui-and-render-tree-helpers.md) | Entity, UI, and Render Tree Helpers |
| 376 | [findings/376-selection-cell-and-overlay-state-helpers.md](376-selection-cell-and-overlay-state-helpers.md) | Selection, Cell, and Overlay State Helpers |
| 377 | [findings/377-callback-registry-and-runtime-helpers.md](377-callback-registry-and-runtime-helpers.md) | Callback, Registry, and Runtime Helpers |
| 378 | [findings/378-input-selection-and-action-state-helpers.md](378-input-selection-and-action-state-helpers.md) | Input Selection and Action-State Helpers |
| 379 | [findings/379-terrain-ui-and-crt-support-helpers.md](379-terrain-ui-and-crt-support-helpers.md) | Terrain, UI, and CRT Support Helpers |
| 380 | [findings/380-scene-objects-terrain-and-matrix-helpers.md](380-scene-objects-terrain-and-matrix-helpers.md) | Scene Objects, Terrain, and Matrix Helpers |
| 381 | [findings/381-selection-resource-runtime-and-grid-helpers.md](381-selection-resource-runtime-and-grid-helpers.md) | Selection, Resource, Runtime, and Grid Helpers |
| 382 | [findings/382-crt-extended-precision-and-entity-routing.md](382-crt-extended-precision-and-entity-routing.md) | CRT, Extended Precision, and Entity Routing |
| 383 | [findings/383-crt-global-accessors-and-light-effects.md](383-crt-global-accessors-and-light-effects.md) | CRT, Global Accessors, and Light Effects |
| 384 | [findings/384-crt-formatting-com-cleanup-and-spawn-helpers.md](384-crt-formatting-com-cleanup-and-spawn-helpers.md) | CRT Formatting, COM Cleanup, and Spawn Helpers |
| 385 | [findings/385-stream-surface-avi-and-route-state-helpers.md](385-stream-surface-avi-and-route-state-helpers.md) | Stream, Surface, AVI, and Route State Helpers |
| 386 | [findings/386-spatial-ui-com-and-crt-runtime.md](386-spatial-ui-com-and-crt-runtime.md) | Spatial, UI, COM, and CRT Runtime |
| 387 | [findings/387-record-and-resource-pool-lifecycles.md](387-record-and-resource-pool-lifecycles.md) | Record and Resource Pool Lifecycles |
| 388 | [findings/388-geometry-and-matrix-primitives.md](388-geometry-and-matrix-primitives.md) | Geometry, Projection, and Random Primitives |
| 389 | [findings/389-linked-records-and-state-helpers.md](389-linked-records-and-state-helpers.md) | Linked Records and State Helpers |
| 390 | [findings/390-motion-config-and-resource-utilities.md](390-motion-config-and-resource-utilities.md) | Motion, Configuration, and Resource Utilities |
| 391 | [findings/391-vector-geometry-and-pointer-state-primitives.md](391-vector-geometry-and-pointer-state-primitives.md) | Vector, Geometry, and Pointer-State Primitives |
| 392 | [findings/392-global-state-and-configuration-accessors.md](392-global-state-and-configuration-accessors.md) | Global State and Configuration Accessors |
| 393 | [findings/393-string-list-and-small-record-utilities.md](393-string-list-and-small-record-utilities.md) | String, List, and Small Record Utilities |
| 394 | [findings/394-runtime-engine-memory-management.md](394-runtime-engine-memory-management.md) | Runtime and Engine Memory Management |
| 395 | [findings/395-crt-and-os-file-stream-io.md](395-crt-and-os-file-stream-io.md) | CRT and OS File/Stream I/O |
| 396 | [findings/396-crt-process-startup-termination-and-exceptions.md](396-crt-process-startup-termination-and-exceptions.md) | CRT Process Startup, Termination, and Exceptions |
| 397 | [findings/397-crt-strings-locale-and-formatting.md](397-crt-strings-locale-and-formatting.md) | CRT Strings, Locale, and Formatting |
| 398 | [findings/398-application-startup-window-loop.md](398-application-startup-window-loop.md) | Application Startup, Window Loop, and Preferences |
| 399 | [findings/399-crt-numeric-and-floating-point-runtime.md](399-crt-numeric-and-floating-point-runtime.md) | CRT Numeric and Floating-Point Runtime |
| 400 | [findings/400-compiler-runtime-no-op-stubs.md](400-compiler-runtime-no-op-stubs.md) | Compiler/Runtime No-op Stubs |
| 401 | [findings/401-task-callback-record-allocator.md](401-task-callback-record-allocator.md) | Task Callback Record Allocator |
| 402 | [findings/402-registry-and-startup-streams.md](402-registry-and-startup-streams.md) | Registry and Startup Streams |
| 403 | [findings/403-config-and-text-parsing-helpers.md](403-config-and-text-parsing-helpers.md) | Configuration and Text Parsing Helpers |
| 404 | [findings/404-game-file-archive-and-pools.md](404-game-file-archive-and-pools.md) | Game Files, Archives, and Record Pools |
| 405 | [findings/405-level-map-loading-pipeline.md](405-level-map-loading-pipeline.md) | Level and Map Loading Pipeline |
| 406 | [findings/406-audio-playback-mixing-and-spatial-sound.md](406-audio-playback-mixing-and-spatial-sound.md) | Audio Playback, Mixing, and Spatial Sound |
| 407 | [findings/407-system-input-and-osf-state.md](407-system-input-and-osf-state.md) | System Input and OSF State |
| 408 | [findings/408-save-slot-context-and-level-record-lifecycle.md](408-save-slot-context-and-level-record-lifecycle.md) | Save Slot Context and Level Record Lifecycle |
| 409 | [findings/409-game-configuration-entity-type-and-object-name-tables.md](409-game-configuration-entity-type-and-object-name-tables.md) | Game Configuration Entity Type and Object-Name Tables |
| 410 | [findings/410-map-and-visual-resource-records.md](410-map-and-visual-resource-records.md) | Map and Visual Resource Records |
| 411 | [findings/411-interface-records-and-directx-diagnostics.md](411-interface-records-and-directx-diagnostics.md) | Interface Records and DirectX Diagnostics |
| 412 | [findings/412-platform-import-thunks.md](412-platform-import-thunks.md) | Platform Import Thunks |
| 413 | [findings/413-configured-directshow-video-path.md](413-configured-directshow-video-path.md) | Configured DirectShow Video Path |
| 414 | [findings/414-avi-and-flic-playback-routes.md](414-avi-and-flic-playback-routes.md) | AVI and FLIC Playback Routes |
| 415 | [findings/415-model-lightwave-and-animation-asset-pipeline.md](415-model-lightwave-and-animation-asset-pipeline.md) | Model, LightWave, and Animation Asset Pipeline |
| 416 | [findings/416-scene-and-render-resource-lifecycle.md](416-scene-and-render-resource-lifecycle.md) | Scene and Render Resource Lifecycle |
| 417 | [findings/417-sound-subsystem-aliases-queues-and-effects.md](417-sound-subsystem-aliases-queues-and-effects.md) | Sound aliases, submission queues, and transient effects |
| 418 | [findings/418-movie-stream-and-audio-playback.md](418-movie-stream-and-audio-playback.md) | Movie, Stream, and Audio Playback Boundaries |
| 419 | [findings/419-interface-and-presentation-config-loading.md](419-interface-and-presentation-config-loading.md) | Interface and Presentation Config Loading |
| 420 | [findings/420-renderer-and-scene-lifecycle.md](420-renderer-and-scene-lifecycle.md) | Renderer and Scene Lifecycle |
| 421 | [findings/421-ui-render-tree-traversal.md](421-ui-render-tree-traversal.md) | UI Render-Tree Traversal |
| 422 | [findings/422-viewport-picking-and-terrain-rendering.md](422-viewport-picking-and-terrain-rendering.md) | Viewport Picking and Terrain Rendering |
| 423 | [findings/423-input-keyboard-and-cursor-state.md](423-input-keyboard-and-cursor-state.md) | Input, Keyboard, and Cursor State |
| 424 | [findings/424-camera-viewport-and-view-context.md](424-camera-viewport-and-view-context.md) | Camera, Viewport, and View-Context State |
| 425 | [findings/425-display-mode-selection-state-and-controls.md](425-display-mode-selection-state-and-controls.md) | Display-Mode Selection State and Controls |
| 426 | [findings/426-camera-selection-point-and-pointer-pan.md](426-camera-selection-point-and-pointer-pan.md) | Camera Selection Point and Pointer Pan |
| 427 | [findings/427-screen-effects-and-ui-text-paths.md](427-screen-effects-and-ui-text-paths.md) | Screen Effects and UI Text Paths |
| 428 | [findings/428-font-and-text-buffer-rendering.md](428-font-and-text-buffer-rendering.md) | Font and Text-Buffer Rendering |
| 429 | [findings/429-advisor-and-message-ui-state.md](429-advisor-and-message-ui-state.md) | Advisor and Message UI State |
| 430 | [findings/430-scene-object-action-and-animation-lifecycle.md](430-scene-object-action-and-animation-lifecycle.md) | Scene Object, Action, and Animation Lifecycle |
| 431 | [findings/431-graphics-rendering-and-surface-paths.md](431-graphics-rendering-and-surface-paths.md) | Graphics, Rendering, and Pixel Surface Paths |
| 432 | [findings/432-scene-resource-animation-state-flow.md](432-scene-resource-animation-state-flow.md) | Scene Resource and Animation State Flow |
| 433 | [findings/433-scene-objects-effects-motion-and-light-state.md](433-scene-objects-effects-motion-and-light-state.md) | Scene Objects, Effects, Motion, and Light State |
| 434 | [findings/434-scene-resources-lights-markers-and-transient-effects.md](434-scene-resources-lights-markers-and-transient-effects.md) | Scene Resources, Lights, Markers, and Transient Effects |
| 435 | [findings/435-scene-objects-animation-and-render-records.md](435-scene-objects-animation-and-render-records.md) | Scene Objects, Animation, and Render Records |
| 436 | [findings/436-simulation-ui-and-player-controls.md](436-simulation-ui-and-player-controls.md) | Simulation UI and Player Controls |
| 437 | [findings/437-menu-widgets-and-menu-rendering.md](437-menu-widgets-and-menu-rendering.md) | Menu Widgets and Menu Rendering |
| 438 | [findings/438-front-end-flow-save-slot-and-option-state.md](438-front-end-flow-save-slot-and-option-state.md) | Front-End Flow, Save Slots, and Option State |
| 439 | [findings/439-hud-panel-render-and-interaction-state.md](439-hud-panel-render-and-interaction-state.md) | HUD Panel Rendering and Interaction State |
| 440 | [findings/440-tutorial-and-advisor-control.md](440-tutorial-and-advisor-control.md) | Tutorial and Advisor Control |
| 441 | [findings/441-incident-and-information-message-queue.md](441-incident-and-information-message-queue.md) | Incident and Information Message Queue |
| 442 | [findings/442-advisor-scene-and-action-state.md](442-advisor-scene-and-action-state.md) | Advisor Scene and Action State |
| 443 | [findings/443-help-tutorial-and-interface-resources.md](443-help-tutorial-and-interface-resources.md) | Help, Tutorial, and Interface Resources |
| 444 | [findings/444-input-selection-pointer-actions.md](444-input-selection-pointer-actions.md) | Input, Selection, and Pointer Actions |
| 445 | [findings/445-terrain-scene-geometry.md](445-terrain-scene-geometry.md) | Terrain Scene Geometry |
| 446 | [findings/446-terrain-and-overlay-rendering.md](446-terrain-and-overlay-rendering.md) | Terrain and Overlay Rendering |
| 447 | [findings/447-terrain-grid-path-search.md](447-terrain-grid-path-search.md) | Terrain Grid Path Search |
| 448 | [findings/448-routing-and-activity-transitions.md](448-routing-and-activity-transitions.md) | Routing and Activity Transitions |
| 449 | [findings/449-spatial-queries-and-candidate-search.md](449-spatial-queries-and-candidate-search.md) | Spatial Queries and Candidate Search |
| 450 | [findings/450-terrain-data-and-grid-operations.md](450-terrain-data-and-grid-operations.md) | Terrain Data and Grid Operations |
| 451 | [findings/451-terrain-cell-state-neighbors-occupancy-and-traversal.md](451-terrain-cell-state-neighbors-occupancy-and-traversal.md) | Terrain Cell State, Neighbors, Occupancy, and Traversal |
| 452 | [findings/452-lava-erosion-timing-and-neighbor-propagation.md](452-lava-erosion-timing-and-neighbor-propagation.md) | Lava Erosion Timing and Neighbor Propagation |
| 453 | [findings/453-object-search-and-placement-eligibility.md](453-object-search-and-placement-eligibility.md) | Object Search and Placement Eligibility |
| 454 | [findings/454-terrain-grid-and-cell-resource-tables.md](454-terrain-grid-and-cell-resource-tables.md) | Terrain Grid and Cell Resource Tables |
| 455 | [findings/455-terrain-command-feedback-and-cell-interaction.md](455-terrain-command-feedback-and-cell-interaction.md) | Terrain Command Feedback and Cell Interaction |
| 456 | [findings/456-world-entity-creation-and-pointer-registries.md](456-world-entity-creation-and-pointer-registries.md) | World Entity Creation and Pointer Registries |
| 457 | [findings/457-entity-stats-configuration-and-record-accessors.md](457-entity-stats-configuration-and-record-accessors.md) | Entity Stats Configuration and Record Accessors |
| 458 | [findings/458-combat-effects-projectiles.md](458-combat-effects-projectiles.md) | Combat Effects and Projectiles |
| 459 | [findings/459-timed-effects-and-cues.md](459-timed-effects-and-cues.md) | Timed Effects and Cues |
| 460 | [findings/460-velocity-and-collision-queries.md](460-velocity-and-collision-queries.md) | Velocity and Collision Queries |
| 461 | [findings/461-weapon-definitions-and-indexed-lookups.md](461-weapon-definitions-and-indexed-lookups.md) | Weapon Definitions and Indexed Lookups |
| 462 | [findings/462-objective-configuration-and-runtime-results.md](462-objective-configuration-and-runtime-results.md) | Objective Configuration and Runtime Results |
| 463 | [findings/463-particles-and-grid-expansion.md](463-particles-and-grid-expansion.md) | Particles and Grid Expansion |
| 464 | [findings/464-entity-lifecycle-and-pools.md](464-entity-lifecycle-and-pools.md) | Entity Lifecycle and Pools |
| 465 | [findings/465-task-scheduler-record-lifecycle.md](465-task-scheduler-record-lifecycle.md) | Task Scheduler Record Lifecycle |
| 466 | [findings/466-command-registry-global-queue-transitions.md](466-command-registry-global-queue-transitions.md) | Command Registry and Global Queue Transitions |
| 467 | [findings/467-command-creation-and-issue-routing.md](467-command-creation-and-issue-routing.md) | Command Creation and Issue Routing |
| 468 | [findings/468-command-queue-and-selection-state.md](468-command-queue-and-selection-state.md) | Command Queue and Selection State |
| 469 | [findings/469-entity-task-and-command-setup.md](469-entity-task-and-command-setup.md) | Entity Task and Command Setup |
| 470 | [findings/470-entity-placement-and-spawning.md](470-entity-placement-and-spawning.md) | Entity Placement and Spawning |
| 471 | [findings/471-entity-activities-and-action-completion.md](471-entity-activities-and-action-completion.md) | Entity Activities and Action Completion |
| 472 | [findings/472-entity-motion-and-action-execution.md](472-entity-motion-and-action-execution.md) | Entity Motion and Action Execution |
| 473 | [findings/473-simulation-dispatch-and-damage.md](473-simulation-dispatch-and-damage.md) | Simulation Dispatch and Damage |
| 474 | [findings/474-simulation-tick-and-engine-state-lifecycle.md](474-simulation-tick-and-engine-state-lifecycle.md) | Simulation Tick and Engine State Lifecycle |
| 475 | [findings/475-objectives-upgrades-and-resource-counters.md](475-objectives-upgrades-and-resource-counters.md) | Objectives, Upgrades, and Resource Counters |
| 476 | [findings/476-entity-activity-and-runtime-records.md](476-entity-activity-and-runtime-records.md) | Entity Activity and Runtime Records |
| 477 | [findings/477-tasks-rewards-and-checkpoints.md](477-tasks-rewards-and-checkpoints.md) | Tasks, Rewards, and Checkpoints |
| 478 | [findings/478-selection-groups-and-selection-state.md](478-selection-groups-and-selection-state.md) | Selection Groups and Selection State |
| 479 | [findings/479-renderer-com-interface-vtable-slots.md](479-renderer-com-interface-vtable-slots.md) | Renderer D3D/D3DRM interface vtable slots (issues 253/254) |
| 505 | [findings/505-original-translation-units-and-link-order.md](505-original-translation-units-and-link-order.md) | Original translation units and link order (issue 383) |
| 506 | [findings/506-original-shape-pilot-priorities.md](506-original-shape-pilot-priorities.md) | Original-shape pilot: Priorities.c rebuilt as C, measured step by step (issue 384) |
| 507 | [findings/507-single-function-match-loop.md](507-single-function-match-loop.md) | Single-function match loop: parity with full Reccmp and timings (issue 381) |
| 508 | [findings/508-config-unit-original-shape.md](508-config-unit-original-shape.md) | Config tree (G98-006) rebuilt in original shape; pilot pads removed (issue 394) |
| 509 | [findings/509-string-helpers-unit-original-shape.md](509-string-helpers-unit-original-shape.md) | String and token helpers (G98-004) rebuilt in original shape; boundary confirmed on behaviour (issue 396) |
| 510 | [findings/510-maths-unit-original-shape.md](510-maths-unit-original-shape.md) | Maths unit (G98-007) rebuilt in original shape; asm leaves replaced by C (issue 395) |
| 511 | [findings/511-dxerror-unit-original-shape.md](511-dxerror-unit-original-shape.md) | DirectX error decoder (G98-027) rebuilt in original shape; 338 literals at their original addresses; boundary confirmed on data and behaviour (issue 411) |
| 512 | [findings/512-sound-unit-original-shape.md](512-sound-unit-original-shape.md) | gods98 sound layer: G98-021/022/023 merged into one unit on .bss alignment and behaviour, rebuilt in original shape with all 16 functions exact; native WinMM stand-ins (issues 409, 410) |
| 513 | [findings/513-keys-unit-original-shape.md](513-keys-unit-original-shape.md) | Key-name table (G98-018, Keys.c) rebuilt in original shape; both functions exact, literal block byte-identical, boundary confirmed; one spare table slot for Reccmp operand naming (issue 416) |
| 514 | [findings/514-bezier-unit-original-shape.md](514-bezier-unit-original-shape.md) | Bezier curve unit (GAME-003) rebuilt in original shape; asm leaves replaced by C, unit 68.64% to 78.74%; flight-curve effects now call it, so the object links; boundary confirmed (issue 417) |
| 515 | [findings/515-handle-table-original-shape.md](515-handle-table-original-shape.md) | gods98 handle table (G98-024) rebuilt in original shape with exactly 2,000 records; boundary and matching results (issue 413) |
| 516 | [findings/516-msvc5-link-order-and-layout-experiments.md](516-msvc5-link-order-and-layout-experiments.md) | How MSVC 5.0's linker orders objects, literals, statics and commons; incremental against non-incremental scores; what reordering and relocation do to inversions (issue 388) |
| 517 | [findings/517-bmp-preparation-original-shape.md](517-bmp-preparation-original-shape.md) | BMP palette and pixel-data preparation in original shape (issue 414) |
| 518 | [findings/518-registry-unit-original-shape.md](518-registry-unit-original-shape.md) | Registry unit in original shape (issue 415) |
| 519 | [findings/519-draw-unit-original-shape.md](519-draw-unit-original-shape.md) | 2D drawing unit (G98-019) rebuilt in original shape; 16 of 18 functions exact, unit 11.19% to 96.22%; six pixel writers missing from the manifest now mapped; boundary confirmed (issue 412) |
| 520 | [findings/520-entity-stats-record-pointer-layout.md](520-entity-stats-record-pointer-layout.md) | Entities reach their 0x150-byte Stats record through a pointer at +0x334; the 32-bit EntitySlot now puts it there (44 accessors exact); survey of other offset-only records (issue 457) |
| 521 | [findings/521-link-order-list-explicit-objects.md](521-link-order-list-explicit-objects.md) | The link-order list (`src/link_order.cmake`): the matching build links every game and gods98 source as an explicit object in original order, then LIBC, oldnames and the import libraries; pair inversions 855,122 to 633,298, no score changes, never-pulled objects link (issue 437) |
| 522 | [findings/522-entity-flag-words-and-owner-layout.md](522-entity-flag-words-and-owner-layout.md) | The 32-bit EntitySlot puts the +0x3e0..+0x3ec flag words, +0x3f0, +0x33c/+0x340 and the +0x36c owner at their original offsets (6 functions exact, 83 improved, 2 drift drops); survey rerun leaves one offset-only function (issue 462) |
| 523 | [findings/523-relocate-unit-tool-and-advisor-relocation.md](523-relocate-unit-tool-and-advisor-relocation.md) | `tools/relocate_unit.py` relocates a unit verbatim into its own file; Advisor.c (GAME-001) moves from five legacy files into `game/advisor.cpp`: pair inversions 633,221 to 593,010, unit-order inversions 3,816 to 3,736, unit contiguity 66.82% to 67.28%, 3 drift drops; survey: 21 of 65 units relocate as they are, named namespaces block 31 (issue 440) |
| 524 | [findings/524-d3drm-header-constants-in-rdata.md](524-d3drm-header-constants-in-rdata.md) | The 20 `{1,2,4,-1}` `.rdata` copies are per-unit `static const` Direct3DRM header constants (`D3DRMMAP_WRAPU/WRAPV/PERSPCORRECT`, `D3DRMGROUP_ALLGROUPS`) from `d3drmdef.h`, one copy per unit including `d3drm.h`; 14 copies attributed to units (issue 451) |
| 525 | [findings/525-msvc5-common-symbol-allocation.md](525-msvc5-common-symbol-allocation.md) | MSVC 5.0 common symbols follow reverse first-definition COFF order, with size-based alignment; original and rebuild validation limits (issue 449) |
| 526 | [findings/526-entity-command-pair-link-window-layout.md](526-entity-command-pair-link-window-layout.md) | The 32-bit EntitySlot puts the +0x28 path count, +0x2f0..+0x308 command/pair/link words, +0x330 selector, +0x344 progress and +0x3c0 pair link at their original offsets (53 improved, 5 drift drops, none exact); survey rerun points at the +0x24 route buffer (issue 468) |
| 527 | [findings/527-relocation-wave-r1-unblocked-units.md](527-relocation-wave-r1-unblocked-units.md) | Wave R1: GAME-004, 006, 009, 014, 015 and 016 relocate with `relocate_unit.py` into their own files: pair inversions 593,010 to 512,033, unit-order inversions 3,736 to 3,585, unit contiguity 67.28% to 70.05%; 3 drift drops (one exact match lost), 2 rises; three tool defects worked around in separate commits (issue 442) |
| 528 | [findings/528-reccmp-diagnostics.md](528-reccmp-diagnostics.md) | Matching-build diagnostics, annotation recovery, diagnostic gate and score effects (issue 379) |
| 529 | [findings/529-unit-relative-data-offsets.md](529-unit-relative-data-offsets.md) | `tools/unit_data_offsets.py` compares unit-relative `GLOBAL`/`STRING` offsets (COFF symbols vs original gaps, finding-524 anchors); post-R1: 0 full, 1 gaps-only, 6 partial, 218 n/a (mostly units without a dedicated object yet); GAME-003, G98-006/007/019 lack the 16-byte D3DRM constant prefix (issue 450) |
| 530 | [findings/530-relocating-named-namespace-members.md](530-relocating-named-namespace-members.md) | `relocate_unit.py` relocates named-namespace members (scoped names, reopened blocks, `using` carry-over) and fixes wave R1's three tool defects; GAME-007 relocated into `game/placement.cpp`: pair inversions 513,793 to 493,667, unit-order inversions 3,585 to 3,533, unit contiguity 70.05% to 70.51%, 1 drift drop; survey: 33 of 59 units relocate as they are, up from 18 (issue 471) |
| 531 | [findings/531-relocation-blocker-source-fixes.md](531-relocation-blocker-source-fixes.md) | One-off source shapes that blocked relocation: `s_motion_jitter_flags` and `s_cursor_slot` unified as one original global each (a shared-header declaration drifted, finding 514), annotations moved onto definitions, a one-line split and an `#if` group fix; no score or instruction-count change; survey: 15 to 17 of 59 units relocate as they are (issue 478) |
| 532 | [findings/532-config-direct3drm-prefix.md](532-config-direct3drm-prefix.md) | Config Direct3DRM prefix restored; three other units remain blocked by matching score drops (issue 481) |
| 533 | [findings/533-reccmp-lint-ratchet.md](533-reccmp-lint-ratchet.md) | `make reccmp-lint` is a ratchet: a relocation-stable baseline of existing unannotated definitions and ledger entries; fails only on new violations and reports removable ones; runs in `tools-test` (issue 487) |
| 534 | [findings/534-relocation-waves-r1-r2-r3-unblocked-units.md](534-relocation-waves-r1-r2-r3-unblocked-units.md) | Waves R1–R3: GAME-008, 010, 012, 013, 019 and G98-008, 013, 016, 017, 028 relocate with `relocate_unit.py`, no hand fix-ups: pair inversions 493,667 to 408,393, unit-order inversions 3,533 to 3,323, unit contiguity 70.51% to 74.65%; 5 drift drops, 10 rises, one exact match gained (`0x0048b290`), 7 instruction-count changes for owner review; unit-data-offsets partial 6 to 8; the `reccmp-lint` ratchet sees 20 keys change because shared heads lose `static` (issues 442, 443, 444) |
| 535 | [findings/535-lego-c-candidate-object-boundary.md](535-lego-c-candidate-object-boundary.md) | GAME-021 candidate boundary at `0042f350` is undecidable from section, call and source evidence; retain one provisional unit (issue 445) |
| 536 | [findings/536-relocation-wave-r5-unblocked-units.md](536-relocation-wave-r5-unblocked-units.md) | Wave R5: 21 game units relocate; pair inversions 395,256 to 319,385; 13 score drops, one exact match lost (66 B), 12 instruction-count changes flagged (issue 446) |
| 537 | [findings/537-overloads-declaration-splits-anonymous-namespaces.md](537-overloads-declaration-splits-anonymous-namespaces.md) | `relocate_unit.py` relocates overloads, splits several-declarator and type-plus-variable declarations in an opt-in, layout-checked step, and shares anonymous-namespace items; Bubbles.c (GAME-005) relocated into `game/bubbles.cpp`: pair inversions 408,393 to 395,256, unit-order inversions 3,323 to 3,303, unit contiguity 74.65% to 75.12%, 2 drift drops; survey: 23 of 48 units relocate as they are, up from 21, plus GAME-005 after the split (issue 485) |
| 538 | [findings/538-msvc5-symbol-counter-codegen-drift.md](538-msvc5-symbol-counter-codegen-drift.md) | Codegen drift in unchanged source follows MSVC 5.0's per-unit symbol counter: rule A, float-product memory operands in reverse first-declaration order; rule B, other ties (operand order, registers, `fxch`, spills) periodic in the counter (period 16–512, content-independent; 80/80 predicted for `draw.c`). `d3drm.h` bisects to 22 inert typedefs. `0x0040e9c0` restored 94.74 → 100 (issue 486) |
| 540 | [findings/540-crt-from-libc-lib.md](540-crt-from-libc-lib.md) | The matching build already links 214 of the 272 CRT-unit functions from LIBC.LIB; their near-misses were annotation names, not code. Naming LIBC data and helpers by their public symbols (`src/crt/libc_symbols.csv`, a Reccmp data source), retiring port markers at LIBC addresses and taking `__CIfmod`, `__ms_p5_test_fdiv`, `__fptrap`, `__seh_longjmp_unwind@4` and four `trnsctrl.obj` functions from the library: 110 functions up, 0 down, 94 newly exact; exact code bytes 44,636 to 64,777; blockers for #447 listed (issue 497) |
| 541 | [findings/541-per-relocation-private-headers.md](541-per-relocation-private-headers.md) | Per-relocation private headers: each relocation writes a frozen `<l>_for_<u>.hpp`/`<l>_from_<u>.hpp` pair, so a later relocation never changes an earlier unit's view; the 142 existing (unit, header) pairs become verbatim snapshots with no listing change; restoring two units' pre-R5 views returns `0x0040e9c0` to 100%; MSVC 5.0 codegen depends on the spelled include path; a GAME-011 replay changes no other unit's code (issue 496) |
| 542 | [findings/542-zero-drift-reshapes-b1-b2.md](542-zero-drift-reshapes-b1-b2.md) | Reshapes B1 and B2 (#490 plan): 15 split naked heads, 3 helper hoists, one annotation move and the `mission.cpp` macro rename, token- and object-identical in every file; no score or listing change. The annotation lint now accepts one annotation above an `#if` group (debt 4436 to 4139, none added). GAME-037 relocates; GAME-043, 021, 027, 017 and G98-012 move to the predicted next blocker; GAME-002 stops at P2/P3b macro groups rather than #485's overload (issue 502) |
| 543 | [findings/543-relocation-game-011-018-022-per-unit-headers.md](543-relocation-game-011-018-022-per-unit-headers.md) | GAME-011, GAME-018 (HelpWindow.c) and GAME-022 relocate under per-relocation private headers; pair inversions 320,874 to 292,291; no earlier unit's listing changes (726 functions); one drop (`0x004882e0`, +1 instruction); G98-009 still blocked by a conditional group in `sound.cpp` (issues 442–445) |
| 544 | [findings/544-steering-unit-original-shape.md](544-steering-unit-original-shape.md) | GAME-016 (steered groups) rebuilt as `game/steering.c`: unit 17.51% to 86.60% byte-weighted, 2 to 24 of 30 exact, 11 newly mapped (seven `ptrstack_for_each` callbacks the manifest missed); Maths.h inline helpers in `steering_maths.h` (macros compile differently); callbacks external, not static, for original link order; `.rdata` is `0049e148`–`0049e16c`; superseded declarations kept (rule 1), no drop or instruction-count change outside the unit (issue 491) |
| 545 | [findings/545-whole-conditional-groups-and-g98-025.md](545-whole-conditional-groups-and-g98-025.md) | The relocator moves a top-level `#if` group of one cluster whole and verbatim, writes guarded private-header declarations (the group's `#if`, verbatim) and exports statics a header item needs; two latent scanner/declaration defects fixed; survey 1 → 7 of 23 unrelocated units relocatable (with #507); G98-025 relocated with no function falling (issue 501) |
| 546 | [findings/546-relocation-game-032-panel-game-021-blocked.md](546-relocation-game-032-panel-game-021-blocked.md) | GAME-032 (Panel.c) relocated after its own declaration split (layout unchanged), with two tool-gap fix-ups (migration snapshots included together; a const object #516 exported moved without a prior extern); GAME-021 (Lego.c) blocked by anonymous-namespace helpers used before their definition; no earlier unit's listing changed; one legacy-file register-swap drop, no instruction-count change (issues 445, 446) |
| 547 | [findings/547-relocation-game-037-043-g98-003-020-030-031.md](547-relocation-game-037-043-g98-003-020-030-031.md) | GAME-037, GAME-043, G98-003 (Viewports.c), G98-020 (Lws.c), G98-030 (Wad.c) and G98-031 relocate as they are (176 functions, 21 new private headers); pair inversions 289,242 to 238,597; one fix-up moves the C4172 diagnostics allowance to `viewports.cpp`; earlier units' code unchanged (one label-only digest flip, `0x004078c0`); four drift drops flagged for the owner, one new exact match (`0x00476d50`); GAME-043's 13 naked heads unchanged |
| 548 | [findings/548-remaining-relocation-refusals.md](548-remaining-relocation-refusals.md) | The relocator handles nested named namespaces, shared unannotated inline functions, out-of-line class members, anonymous-namespace forward declarations and helper functions in whole `#if` groups; name-clash refusals say which source fix applies; GAME-023, G98-002 and GAME-002/027 need source fixes measured with reshape_check; the survey plans the relocation behind a declaration split; GAME-021, GAME-050 and G98-009 relocated (issue 514) |
| 549 | [findings/549-animation-object-unit-original-shape.md](549-animation-object-unit-original-shape.md) | GAME-004 (animation objects) rebuilt as `game/animation_object.c`: unit 24.94% to 97.65% byte-weighted, 2 to 15 of 21 exact, 3 newly mapped; seven functions keep port contracts for unconverted C++ callers; two unmarked adapters (Container_Load, MeshLOD_Load); a `// STRING` marker must sit directly above its literal or Reccmp drops the function; deleting `entity.cpp`'s old `00406e40` model drifted three listings, so it is restored renamed; no score or listing change outside the unit (issue 513) |
| 550 | [findings/550-advisor-unit-original-shape.md](550-advisor-unit-original-shape.md) | GAME-001 (Advisor) rebuilt as `game/advisor.c`: unit 50.66% to 94.85% byte-weighted, 13 to 24 of 31 exact, 8 functions byte-identical at their original addresses; one static globals struct replaces three port owners; Maths.h helpers return their destination and use repeated subexpressions, inline modulus/scale in `advisor_maths.h`; `vec_cross` keeps its asm body; `$S` probes keep every header edit count-neutral, no drop outside the unit (issue 512) |
| 551 | [findings/551-relocation-game-023-legoobject.md](551-relocation-game-023-legoobject.md) | GAME-023 (LegoObject.c) relocated to `game/legoobject.cpp`: 251 functions, 116 owned globals, 112 closure items; two zero-code-byte source fixes (self-nested `unported_ledger_stubs` in entity.cpp/view.cpp, dead `const` static renamed) proved with reshape_check; native-only bounds guard on the terrain SFX grid store (`0x0044acc0`) with the matching object identical; stacked on #528; two drift-review drops (`0x00444680`, `0x0044a070`) and two rises, no exact match lost; function-order inversions 77,707 to 52,084 pairs (issue 521) |
| 552 | [findings/552-second-reshape-pass-if-blocked-units.md](552-second-reshape-pass-if-blocked-units.md) | Second reshape pass: three zero-drift reshapes (`resource.cpp`, `frontend.cpp` ×2) and seven tool commits relocate G98-014, GAME-020, G98-011 and GAME-017 (249 functions); pair inversions 165,240 to 77,707; no exact match lost; a brace line that hides the next annotation from Reccmp's parser; `reshape_check` cannot see annotation attachment; G98-010 and G98-012 plan cleanly once their backend regions are split, G98-015 waits on `view.cpp` (issue 525) |
| 553 | [findings/553-sound3d-unit-original-shape.md](553-sound3d-unit-original-shape.md) | G98-009 (Sound3D) rebuilt as `gods98/sound3d.c`: unit 18.76% to 95.27% byte-weighted, 4 to 33 of 42 exact, 4 newly mapped; one `// FUNCTION` per `#if` group holds the original DirectSound body (Windows builds) and the port's SDL body (native); the unit declares the DirectX 5 COM tables itself; the SDK `WaveLoadFile` `nError`/`goto` shape and unsigned loop counters matter; one flagged drop outside the unit (`0x004295f0`, float contract) (issue 524) |
| 554 | [findings/554-panel-unit-original-shape.md](554-panel-unit-original-shape.md) | GAME-032 (Panel.c) rebuilt as `game/panel_unit.c`: unit 30.65% to 90.17% byte-weighted, 12 to 31 of 52 exact, 2 newly mapped; the state is one external `panelGlobs = { 0 }` (MSVC 5.0 keeps a zero-initialised external in `_BSS`), which other units and the native HUD read directly; float panel positions for Priorities.c and Advisor.c; `0x004369f4` written and annotated; header edits kept count-neutral after `vec_dot` and five other drifts; one flagged drop, `priorities_handle_cursor` (issue 523) |
| 555 | [findings/555-native-creature-scenes-seams.md](555-native-creature-scenes-seams.md) | Native creature scenes (#527 reworked): GAME-004's 15 Container calls reach native Containers through port-only forwarders that `container.h` routes to in MINIWIN builds, so no annotated Container body changes; `ddiscene_object_position_get` is one whole alternative group, matching branch first (Reccmp maps the line below the annotation; the other order unmapped `0x004754c0` and dropped 11 callers); call-site `#elif` arms beside the WinAPI `actor_scene` ones; the lint skips MINIWIN-only branches (ratchet 4,110 to 4,061); no score moved |
| 556 | [findings/556-terrain-map-unit-original-shape.md](556-terrain-map-unit-original-shape.md) | GAME-026 (Map3D.c + MeshLOD) rebuilt as `game/terrain_map.c` with the port's terrain model moved unannotated to the port tail `terrain_map_port.cpp`: unit 19.32% to 81.42% byte-weighted, 7 to 35 of 62 exact, 6 newly mapped; eight functions keep port names (callers already on the original contract), the rest listed for follow-up; four Container adapters; 15 call-target drops outside the unit incl. one exact (`0x00403c00`, flagged for #521); four `animation_object.c` functions to 100% (issue 522) |
| 557 | [findings/557-backend-region-splits-g98-012-g98-010.md](557-backend-region-splits-g98-012-g98-010.md) | Backend region splits: two zero-drift reshapes close and reopen `#if` groups with equivalent conditions (`movie.cpp`: `movie_avi_file_initialize` as one whole-alternatives group; `surface.cpp`: the nested winapi `_WIN32` group cut into 18 single-branch groups by relocation role, B3) and two tool commits (header-declared types bind to the header; guarded declarations for a lone function in a single-branch group) relocate G98-012 to `gods98/avi_player.cpp` and G98-010 to `gods98/directdraw.cpp` (40 functions); no score falls, one rise; pair inversions 52,363 to 51,490; only G98-015 is still `#if`-blocked (issue 525) |
| 558 | [findings/558-rule-a-sweep.md](558-rule-a-sweep.md) | Rule-A sweep: `tools/x87_operand_swaps.py` finds x87 operand swaps in Reccmp listings (it maps unannotated globals through aligned rows); 8 functions with arithmetic swaps and 10 more with comparison swaps only, on `0a90abfa`; one rule A case (`0x004075a0`: the timer as a function-scope static flips the operands, but the score does not move, so it is not applied); one expression-shape fix (`0x0045c3f0` 93.02 to 97.67, `air * (n * 0.01f)`); rule A does not cover literals or operands through pointers; G98-020 follow-ups (issue 534) |
| 559 | [findings/559-relocation-game-027-game-002-static-renames.md](559-relocation-game-027-game-002-static-renames.md) | GAME-027 (Message.c) → `game/message_unit.cpp` (24 functions) and GAME-002 (AITask.c) → `game/aitask.cpp` (95), after two static renames proved zero drift with the new `reshape_check.py --rename`; both obvious renames reorder MSVC 5.0's name-hashed `.bss` (finding 548 saw only `ai.cpp`'s), so names that keep every offset were found by search; no exact match lost, two flagged drops from the tool's `static` removals; game-only pair inversions 37,248 to 18,152 (issue 533) |
| 560 | [findings/560-reward-unit-original-shape.md](560-reward-unit-original-shape.md) | GAME-037 (Reward.c) rebuilt as `game/reward.c`, the port's reward screen moved unannotated to the port tail `reward_port.cpp`: unit 16.21% to 89.58% byte-weighted, 11 to 23 of 52 exact, 3 newly mapped; the state is one common `rewardGlobs` (00552980) whose embedded `RewardLevel` holds the counters other units bump; two annotated definitions renamed to C linkage behind forwarders for frozen-header callers; five adapters; one flagged operand-only drop, `0x0041fa80` (issue 535) |
| 561 | [findings/561-lws-unit-original-shape.md](561-lws-unit-original-shape.md) | G98-020 (Lws.c) rebuilt as `gods98/lws.c` with the port's models moved unannotated to the port tail `lws_port.cpp`: unit 16.09% to 87.75% byte-weighted, 1 to 14 of 25 exact, 4 newly mapped; six functions keep port names with the original's types; Mesh.c callees through `mesh_unit.h` and four adapters; the 1/180 and π factors as annotated statics (rule A order); finding 558's `0x00487ed0` compare swaps fixed; no drop outside the unit, `0x004897e0` rises; lint +13/−1; stacked on #547 (issue 536) |
| 562 | [findings/562-relocation-g98-002-g98-015.md](562-relocation-g98-002-g98-015.md) | The last `view.cpp` relocations: G98-002 to `gods98/container_unit.cpp` (102 functions; #526 had already closed its self-nested namespace) and G98-015 to `gods98/mesh_unit.cpp` (52 functions) after one zero-drift `view.cpp` reshape and a tool fix (forward declarations before a variable whose initializer needs them); pair inversions 32,339 to 16,588; no exact match lost; no unit left blocked by an `#if` block, only G98-005 (backend-only source) remains (issue 543) |
| 563 | [findings/563-whole-binary-audit.md](563-whole-binary-audit.md) | The whole-binary gap beyond function code, measured with the new `tools/pe_audit.py` (`make pe-audit`): size, not structure, is the gap (`.text` +331,717, of which the port tail is 307,616; `.rdata` +445,223, of which `frontend.cpp`'s two `s_empty` placeholders are 431,664; initialised `.data` +923,549, of which the named-section padding is 912,673); `/OPT:REF` would remove 312,258 bytes of code nothing references, but unmaps 1,102 annotated functions; the original was linked without `/DEBUG` (`LINE_NUMS_STRIPPED`; 14 thunks) and probably compiled without `/Gy`; `/DEBUG` puts 84 bytes at the start of `.rdata`; the import descriptors follow command-line library order; icon resources and stamps missing; follow-ups #560 to #563 (issue 554) |
| 564 | [findings/564-interface-unit-original-shape.md](564-interface-unit-original-shape.md) | GAME-020 (Interface.c) rebuilt as `game/interface_unit.c` with the port's interface moved unannotated to the port tail `interface_port.cpp`: unit 20.52% to 83.71% byte-weighted, 13 to 45 of 98 exact, 25 newly mapped (16 callbacks Ghidra merged); eleven entries keep port names and delegate natively to `*_port`; grouped `return 0` tests take `0041d040` from 56% to 93%; one flagged in-unit drop (`0x0041f230`), nothing falls outside the unit, `0x0045c0d0` rises; lint ledger 1,558 to 1,527 on top of #558 (issue 545) |
| 565 | [findings/565-entity-link-pair-2fc-300-merge.md](565-entity-link-pair-2fc-300-merge.md) | The entity link pair merged to the original layout: retail `+0x300` is one entity pointer (six writers, two callback readers) that the port split into `route_partner_300` and `field_300`; both become `link_300` at `+0x300` and `link_2fc` becomes `EntitySlot*`; two misattributed writes (`0x4471b0` into `attach_304`, `0x437940` into `+0x300`) corrected; two rises, one stride drop, no exact match lost; the command record `+0x60` follow-up is blocked on the frozen `cmd_for_aitask.hpp` (issue 555) |
| 566 | [findings/566-electric-fence-unit-original-shape.md](566-electric-fence-unit-original-shape.md) | GAME-013 (ElectricFence.c) rebuilt as `game/terrain_cell_objects.c`, the port's fence model moved unannotated to the port tail `terrain_cell_objects_port.cpp`: unit 15.74% to 91.49% byte-weighted, 0 to 19 of 27 exact, 4 newly mapped (three callbacks Ghidra folded into `0040d4f0`); seven per-module C boundary headers, `extern "C"` in place, two `PORT-ONLY` adapters; frame-kept coordinate pairs are structs and the direction tables two arrays, far first; `lego_unit.c`'s five calls switched to the unit; eight flagged operand-only caller drops, no exact match lost; lint ledger 1,503 to 1,490; stacked on #571 (issue 556) |
| 567 | [findings/567-info-unit-original-shape.md](567-info-unit-original-shape.md) | GAME-019 (Info.c) rebuilt as `game/info_unit.c` with the port's message queue moved unannotated to the port tail `info_port.cpp`: unit 33.93% to 97.38% byte-weighted, 5 to 32 of 39 exact, 4 newly mapped (three enumeration callbacks and `00419b00`); nine functions keep port names with `infoGlobs` (`004dc658`) as their state; `0041a2f0` reads #559's `interfaceGlobs.font`; one empty adapter (`lego_goto_info`); 13 flagged operand-only drops outside the unit from callers of the tail's `message_dispatch`; lint ledger 1,527 to 1,521 (issue 557) |
| 568 | [findings/568-lego-unit-part-1-original-shape.md](568-lego-unit-part-1-original-shape.md) | GAME-021 (Lego.c) part 1 (`0041f870`-`0042b500`, 78 functions incl. six Ghidra merged) rebuilt as `game/lego_unit.c`, stacked on #572; the port models stay unannotated (PORT-ONLY) in `lego.cpp` until part 3 retires it to the tail, so no part-2/3 function drifts; legoGlobs read through annotated port globals plus 161 unit stand-ins; range 8.98% to 89.29% byte-weighted, 22 to 46 exact, 15 newly mapped; 20 shared functions; `0x00429160` keeps a naked body under MSVC (as `vec_cross`); Detail_GetTexture takes a byte (GAME-026 callers rise); no exact lost, 13 drift drops with no instruction change; lint parser no longer reads `#if defined(...)` blocks as definitions, ledger 1,521 to 1,503 (issue 544) |
| 569 | [findings/569-string-literal-pooling.md](569-string-literal-pooling.md) | The original pools string literals across objects in writable `.data` (`/Gf`); MSVC 5.0's `/O2` already does the same, so no flag is adopted (`/GF` moves them to `.rdata`); GAME-037's gap was a `unit_data_offsets.py` artefact (discarded COMDATs, now skipped: 0 of 795 offsets disagree with the link, was 49) plus GAME-017's unconverted `"Text"`; the `// STRING` rule becomes one marker per address, in the owning unit: on top of PR #569, 1,022 to 1,026 exact, one operand-label drift; `reccmp_diagnostics.py` now sees Reccmp's coloured `[ERROR]` lines, and four duplicate markers on the stack it then caught are removed (issue 553) |
| 574 | [findings/574-import-directory-and-library-order.md](574-import-directory-and-library-order.md) | The import directory: `dinput.lib` (with the SDK's `c_dfDIMouse`/`c_dfDIKeyboard` `.text` data members), an ordinal-1 `dsound.lib` and the DirectInput `dxguid.lib` members rebuilt from `.def` and C sources in `cross/msvc5-importlibs/` (member bytes identical to the DirectX 5 SDK's); library order `kernel32, libc, oldnames, ole32, d3drm, ddraw, winmm, vfw32, dinput, dsound, user32, gdi32, advapi32` is the only one of six relinks that gives the original's descriptor order, LIBC before the thunks and `RtlUnwind` last under `/OPT:REF` (ADR 0006 decision 1 amended); the eight imported DLLs are now in original order; `reward_loop` calls Main.c's `0x00478000` (a tail call to `timeGetTime`), not the import: `main_time_get` added at 100%, `reward_loop` 97.25% to 98.28%; remaining missing imports are `GetProcAddress` or uncalled in port code (issue 563) |
| 575 | [findings/575-no-native-code-in-winapi-links.md](575-no-native-code-in-winapi-links.md) | No SDL3 or miniwin code reached a winapi link, but the native entry chain did (`main.cpp`'s `LegorrrMain`, `LegorrrRun`, the scaffold-name functions), plus 8,352 bytes of native-only and 19,584 bytes of test-only functions in shared files; `main.cpp` and `gods98/gods98.cpp` are tagged `native`, and 38 files guard unreferenced native and test code with `!LEGORRR_BACKEND_WINAPI`: matching `.text` 1,056,346 to 1,037,594, no score moved; 10,736 bytes stay (parallel PRs' files, code before annotated functions, code still referenced by unreachable callers); new `tools/winapi_link_audit.py` (`check-list` in `make tools-test`, `check-link` in `make reccmp-check` and `make match-progress`) fails on any native object, SDL/miniwin library or symbol, or native entry-chain symbol (issue 576) |

Canonical full pseudocode corpus: [`docs/decompiles/full/LEGORR.full.c`](../../docs/decompiles/full/LEGORR.full.c)
(2564 manifest functions; see its [README](../../docs/decompiles/full/README.md)). The
legacy [`engine-region.c`](../../docs/decompiles/legacy/engine-region.c) retains named body
variants that have not yet been reconciled; its 11 entries missing from the
full manifest are preserved in `docs/decompiles/full/LEGORR.supplement.c`.
Machine-readable evidence: [`docs/evidence/legorr-rev.bundle`](../../docs/evidence/legorr-rev.bundle)
(236 recorded REA observations against `LegoRR.exe`,
SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).
The latest helper expansion has its own supporting bundle at
[legorr-engine-helpers-expansion.bundle](../../docs/evidence/legorr-engine-helpers-expansion.bundle)
and the follow-up terrain/resource/text expansion is recorded in
[legorr-engine-helpers-expansion-2.bundle](../../docs/evidence/legorr-engine-helpers-expansion-2.bundle).
The latest engine and terrain helper decompiles are recorded in
[legorr-engine-helpers-expansion-3.bundle](../../docs/evidence/legorr-engine-helpers-expansion-3.bundle).
The current direct-decompile batch is in
[legorr-engine-helpers-expansion-4.bundle](../../docs/evidence/legorr-engine-helpers-expansion-4.bundle).
The reward, terrain-view, and simulation helpers are recorded in
[legorr-engine-helpers-expansion-10.bundle](../../docs/evidence/legorr-engine-helpers-expansion-10.bundle), with caller audits in the
[render](../../docs/evidence/legorr-engine-helper-audit-render-expansion-10.bundle) and
[simulation](../../docs/evidence/legorr-engine-helper-audit-simulation-expansion-10.bundle) bundles.
The segment mesh and formatted-scan implementations are in
[legorr-engine-helpers-expansion-11.bundle](../../docs/evidence/legorr-engine-helpers-expansion-11.bundle), with supporting
[text/runtime audit](../../docs/evidence/legorr-engine-helper-audit-text-io-expansion-10.bundle).
The cursor-interaction, overlay, menu, and scene-light helpers are recorded in
[legorr-engine-helpers-expansion-12.bundle](../../docs/evidence/legorr-engine-helpers-expansion-12.bundle).
The terrain quad-run, upgrade-table, and scene-slot helpers are recorded in
[legorr-engine-helpers-expansion-13.bundle](../../docs/evidence/legorr-engine-helpers-expansion-13.bundle).
The path-step, terrain, and pending-record helpers are recorded in
[legorr-engine-helpers-expansion-14.bundle](../../docs/evidence/legorr-engine-helpers-expansion-14.bundle).
The config-loader, file-descriptor, and CRT number helpers are recorded in
[legorr-engine-helpers-expansion-15.bundle](../../docs/evidence/legorr-engine-helpers-expansion-15.bundle).
The config-file, text-layout, and record-emit helpers are recorded in
[legorr-engine-helpers-expansion-16.bundle](../../docs/evidence/legorr-engine-helpers-expansion-16.bundle).
The region-route, light-config, and surface helpers are recorded in
[legorr-engine-helpers-expansion-17.bundle](../../docs/evidence/legorr-engine-helpers-expansion-17.bundle).
The entity move/route command, cell-queue, and object-clone helpers are recorded in
[legorr-engine-helpers-expansion-18.bundle](../../docs/evidence/legorr-engine-helpers-expansion-18.bundle).
The config graphics-quality, camera tween/drag, cell-search, and pool/spawn helpers are recorded in
[legorr-engine-helpers-expansion-19.bundle](../../docs/evidence/legorr-engine-helpers-expansion-19.bundle).
The config-band loaders, display-mode setup, directional-activity, and flight-record helpers are recorded in
[legorr-engine-helpers-expansion-20.bundle](../../docs/evidence/legorr-engine-helpers-expansion-20.bundle).
The config-band loaders, poly-variant build, panel/cell registration, layout, and activity helpers are recorded in
[legorr-engine-helpers-expansion-21.bundle](../../docs/evidence/legorr-engine-helpers-expansion-21.bundle).
The config-band loaders, cell-corner scan, action/command issues, record instantiation, and render-state helpers are recorded in
[legorr-engine-helpers-expansion-22.bundle](../../docs/evidence/legorr-engine-helpers-expansion-22.bundle).
The command-band kind dispatch, item name lookup, route command, drift/timer, terrain normal, and record/geometry helpers are recorded in
[legorr-engine-helpers-expansion-23.bundle](../../docs/evidence/legorr-engine-helpers-expansion-23.bundle).
The config-band texture loaders, route command issues, and surface/menu helpers are recorded in
[legorr-engine-helpers-expansion-24.bundle](../../docs/evidence/legorr-engine-helpers-expansion-24.bundle).
The terrain scatter, erosion propagation, cell query, menu record, and buffer helpers are recorded in
[legorr-engine-helpers-expansion-25.bundle](../../docs/evidence/legorr-engine-helpers-expansion-25.bundle).
The command sweeps, cell issuing, config priorities, statistics, and DirectDraw create helpers are recorded in
[legorr-engine-helpers-expansion-26.bundle](../../docs/evidence/legorr-engine-helpers-expansion-26.bundle).
The visual channel relays, resource-tree callback walks, scene pose helpers, and string/config registry helpers are recorded in
[legorr-engine-helpers-expansion-27.bundle](../../docs/evidence/legorr-engine-helpers-expansion-27.bundle).
The command-band step issuers, flag-behavior timers, and steering/mesh helpers are recorded in
[legorr-engine-helpers-expansion-28.bundle](../../docs/evidence/legorr-engine-helpers-expansion-28.bundle).
The configured tiny-monster spawning and mutual activity pairing helpers are recorded in
[legorr-engine-helpers-expansion-29.bundle](../../docs/evidence/legorr-engine-helpers-expansion-29.bundle).
The interface-method wrappers, frame overlay refresh, and former deferred-set relays are
recorded in [legorr-engine-helpers-expansion-30.bundle](../../docs/evidence/legorr-engine-helpers-expansion-30.bundle).
The selected-entity detach callback and pair-record commit helpers are recorded in
[legorr-engine-helpers-expansion-31.bundle](../../docs/evidence/legorr-engine-helpers-expansion-31.bundle).
The latest input, selection, and resource helper batch is in
[legorr-engine-helpers-expansion-5.bundle](../../docs/evidence/legorr-engine-helpers-expansion-5.bundle);
its address-range audits are in the [low](../../docs/evidence/legorr-engine-helper-audit-low-expansion-5.bundle),
[simulation/input](../../docs/evidence/legorr-engine-helper-audit-mid-expansion-5.bundle), and
[resource/rendering](../../docs/evidence/legorr-engine-helper-audit-high-expansion-5.bundle)
evidence bundles.
The latest terrain, entity, UI, and stream expansion is in
[legorr-engine-helpers-expansion-6.bundle](../../docs/evidence/legorr-engine-helpers-expansion-6.bundle),
with its [low](../../docs/evidence/legorr-engine-helper-audit-low-expansion-6.bundle),
[simulation](../../docs/evidence/legorr-engine-helper-audit-mid-expansion-6.bundle), and
[resource](../../docs/evidence/legorr-engine-helper-audit-high-expansion-6.bundle)
caller-audit bundles.
The text renderer and font helper call chain is in
[legorr-engine-helpers-expansion-7.bundle](../../docs/evidence/legorr-engine-helpers-expansion-7.bundle).
The terrain, rendering, and stream helper batch is in
[legorr-engine-helpers-expansion-8.bundle](../../docs/evidence/legorr-engine-helpers-expansion-8.bundle),
with [terrain](../../docs/evidence/legorr-engine-helper-audit-terrain-expansion-8.bundle),
[rendering](../../docs/evidence/legorr-engine-helper-audit-render-expansion-8.bundle), and
[text/IO](../../docs/evidence/legorr-engine-helper-audit-text-io-expansion-8.bundle) caller audits.
Game-file stream and DirectDraw pixel helpers were added in
[legorr-engine-helpers-expansion-9.bundle](../../docs/evidence/legorr-engine-helpers-expansion-9.bundle).
The camera, LWO layer, and normal helpers are in
[legorr-engine-helper-audit-render-expansion-9.bundle](../../docs/evidence/legorr-engine-helper-audit-render-expansion-9.bundle).
The terrain labels, grid-bound, and selection helpers are documented in
[legorr-engine-helper-audit-terrain-expansion-9.bundle](../../docs/evidence/legorr-engine-helper-audit-terrain-expansion-9.bundle).
The buffered-stream and Win32 file helpers are in
[legorr-engine-helper-audit-text-io-expansion-9.bundle](../../docs/evidence/legorr-engine-helper-audit-text-io-expansion-9.bundle).
The latest three-range helper audits are preserved in the
[low](../../docs/evidence/legorr-engine-helper-audit-low.bundle),
[simulation/terrain](../../docs/evidence/legorr-engine-helper-audit-mid.bundle), and
[resource/rendering](../../docs/evidence/legorr-engine-helper-audit-high.bundle) evidence bundles.
