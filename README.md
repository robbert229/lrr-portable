# lrr-portable

Portable reconstruction of **Lego Rock Raiders**, a real-time strategy game
published by LEGO Media International in 1999 for Windows 95/98.

The goal is a **native Linux port with no Wine layer** (ADR 0004), built from
the decompile corpus, by recompiling the game's logic against a
source-compatible shim (**miniwin**, LGPL, over SDL3) instead of the original
DirectX. A Win32/MinGW build is kept as a *reference build* for A/B
comparison against the shipped binary (ADR 0002, 0004).

## The game

Lego Rock Raiders cast the player as a team of LEGO minifigures mining a planet for
"power rocks." It shipped in 1999, is a 32-bit Windows binary using DirectX, and has been
largely forgotten by official channels ever since. It is of interest here for preservation
and curiosity; nothing in this repository is affiliated with or endorsed by the LEGO Group.

## Repository layout

```
README.md           This file
docs/findings/      Reverse-engineering findings through 479 (see index below)
tools/              WAD tooling (rrwad.py: unpack/pack/verify — see tools/README.md)
                    + model converter (tools/rrlwo.py: .lwo → glTF 2.0)
                    + compat harness (tools/compat/a2.py — see tools/compat/README.md)
docs/decompiles/    Canonical and legacy Ghidra pseudocode; see
                    docs/decompiles/README.md
docs/evidence/      Exported REA evidence bundle (address-level citations)
tools/              Analysis tooling: unwad.py, Ghidra function-naming script
docs/adr/           Architecture decision records (build/port decisions)
reference/          (gitignored) the original, unmodified game files
unpacked/           (gitignored) 5,845 assets extracted from the .wad archives
cross/              Meson cross files (i686.ini — MinGW-w64 Win32 toolchain)
meson.build         Reconstruction build (top-level; platform=native|winapi)
meson_options.txt   Build options (src_root, platform)
src/                Reconstruction source tree (stubs + gods98 / game / platform)
subprojects/        miniwin CMake bridge (sparse isle-portable submodule)
3rdparty/           Dependency submodules (isle-portable, sparse → miniwin/)
```

Everything under `reference/` is the original game as shipped. It is **not** committed to git
(copyright), and is expected to be provided by each person working in this repo from a
legally obtained copy of the game.

### What's in `reference/`

| Path | Description |
| --- | --- |
| `LegoRR.exe` | The main game executable — primary reverse engineering target |
| `LegoRR0.wad`, `LegoRR1.wad` | Game data archives (assets and possibly logic/data) |
| `LegoRR.icd` | Installation/configuration data |
| `D3DRM.DLL` | DirectX 5 render-mesh library dependency |
| `CLGen.exe`, `CLGen.dat` | Install-time generator component |
| `Data/` | Loose runtime data — streamed sounds (`Data/Sounds/Streamed/`), video (`Data/AVI/`) |
| `Readme.txt` | Original publisher readme |

## Goals

- [x] Map the high-level architecture of `LegoRR.exe` (engine, game logic, rendering)
      — findings 02–06, 10
- [x] Document the `.wad` archive container format(s) — finding 01
- [x] Document asset formats: 3D models, animations, sounds, videos
      — findings 11–14: models/scenes, `Lego*` configs, textures, audio, video.
      (Fine detail closed: `.flh` container — 21 (was 14 §5). `.uv` grammar
      closed — 19; `.x` identified as XOF-text leftovers the engine never
      loads — 11 §6.)
- [x] Understand mission / level scripting and data-driven behavior — findings 07, 09
- [x] Understand the savegame format — finding 08
- [x] Eventually: reimplementations or tooling that let the game run on modern systems
      — A.1 audit (finding 15) → A.2 live test (finding 16): **the shipped exe launches
      and renders under GE-Proton via `tools/compat/a2.py run`** (2026-10-20)

## Current status (2026-10-21)

Research is complete: all 29 findings shipped, every format shipped in the wads is
documented address-level — including the two that exist but are never loaded
(`.x` XOF-text leftovers, `.blx` editor-only) — and the `Lego*` block-text system that
`.ae` belongs to (finding 12). The game runs and is playable through the Proton wrapper,
and the toolchain (unpack/pack byte-exact, model+uv convert **and** write, live launcher)
is regression-verified against the shipped corpus.

On the reconstruction side, the engine scaffold, frame loop, input, audio and
now the **game layer** (level `.map` loading, objectives/advisor, savegames —
findings 07/08/22) build and pass their suites natively; see the
*Reconstruction build* section below. The **AVI movie path** (findings 14 §4,
23) has landed too: the boot films (`LegoAvi`/`RRAvi`) and the per-level
`Video` cinematics play through the port's own software decoder
(`src/gods98/movie.*` — FFmpeg runtime dlopen'd, no build dependency).

Finding 23 (2026-10-21) closed the **front-end**: the boot→main-menu machine
(`MenuBuild`, the modal `MenuRunLoop`, the `FrontEndActive` gate, menu-enter /
state-change, the level-chain + start-level entries, and the options/movie
callbacks) is decompiled, named (50 functions appended to
`docs/decompiles/legacy/engine-region.c`; REA evidence in
`docs/evidence/legorr-frontend.bundle`), and the shipped `Lego.cfg` menu/level
grammar documented. The port now supports the game's **`::` path separator**
and builds campaign/tutorial chains from the shipped `Levels::<name>` values.
The remaining question is shipped-data end-to-end validation; see the current
status below.

Finding 24 (2026-10-21) closed the menu **wipe/fade transition** left as a
named-but-not-decomposed gap in finding 23's `MenuRunLoop`: `MenuWipe` is a
config-keyed LWS (LightWave scene) full-screen rock slide
(`Interface\FrontEnd\Rock_Wipe\RockWipe`, 35 frames @ 25 fps), loaded by
`LoadMenuBuild @ 0x4161f0` through the `FUN_00472cd0` object factory (kind
`"LWS"`), triggered on every menu change by `MenuRunLoop`/`MenuTick`, with input
frozen while it plays (`DAT_005574f0 & 2`), released at half-scene, and gated
by the `-reduceanimation` feature bit / a literal `"Null"` disable. The port's
front-end currently renders an instant menu swap; finding 24 documents the
exact contract to reproduce it (LWS step or a `.flh` overlay stand-in). REA
evidence: `docs/evidence/legorr-menu-wipe.bundle` (18 records).

Finding 25 (2026-10-21) closed the runtime those LWS objects run on: the
game-wide **DDiScene scene-graph object model** — the 9-slot kind string table
(`NULL MESH FRAME ANIM ACT - - LWS LWO`) that the `FUN_00472cd0` factory
dispatches on, the pooled `0x2c`-byte object record (free list `DAT_0076add0`,
kind at `+0x10`, flags at `+0x14`), and the draw/step API every animated 3D
thing uses — `FUN_00475140` (draw at time), `FUN_00477010`/`FUN_00477150`
(begin/end scene), and the `FUN_004765c0` scene loader behind the
`DAT_00505914` vtable system (source string `gods98/dx6`). REA evidence:
`docs/evidence/legorr-ddscene.bundle` (64 records). Complements finding 24's
`MenuWipe` (the LWS rock-slide is one instance of this runtime).

Finding 26 (2026-10-21) decoded the two animation callbacks that draw step
fires: the action-entry **SFX sample hook** (`DAT_0076cd78/7c`, registered by the
45-slot SFX-name-hash bank init `FUN_00464740` — slot `0x27` =
`SFX_RockWipe`, the wipe's own start-rumble, confirmed), playing the selected
action's frame `SAMPLE` name as a positional D3D sound, and the scene-end
**entity-flag hook** (`DAT_0076cd80/84`, boot-registered) that ORs bit
`0x10000` into the owning entity's `+0x3e4` flag word (a new bit on the
finding-10 flag word). REA evidence:
`docs/evidence/legorr-ddscene-hooks.bundle` (46 records).

Finding 27 (2026-10-21) closed the loop on that scene-done bit: both readers
live inside the finding-10 flag-evaluator blob. Reader A (`0x43dd10`) plays
the **placement sounds by work-object type** — `7` (Ore) → `SFX_PlaceOre`,
`6` (PowerCrystal) → `SFX_PlaceCrystal`, else `SFX_Place`, via
`FUN_00465050` — gated on gated/placing mode (`+0x3e0 & 0x400`) and a target
handle (`+0x308`). Reader B (`0x43d903`) runs the `FUN_004475c0`
impact/wake probe (message 5, roll speed, shake; bits `0x100000` /
`0x40000000`) and clears the bit. New `+0x3e4` bit facts (`0x10000`,
`0x80000`, `0x100000`, `0x40000000`) appended to finding 10. REA evidence:
`docs/evidence/legorr-scene-done.bundle` (42 records).

Finding 28 (2026-10-21) closed finding 26's remaining owner-link question:
`FUN_00473da0` writes the DDiScene `+0x20` owner and `FUN_00473db0` reads it
for the scene-end callback. The entity spawner wires the new scene object
back to its entity; type-specific model paths propagate that pointer through
their referenced scene objects. This ties the scene-end flag writer to entity
construction while keeping unowned factory/menu objects in scope as possible.
REA evidence: `docs/evidence/legorr-ddscene-owner-link.bundle` (20 records).

Finding 29 (2026-10-21) traces DDiScene cleanup from the entity free path:
`FUN_00437540` passes scene-backed entities' `+0x1c` object to
`FUN_00472a40` before recycling the entity. The ordinary object path unlinks
its scene interfaces and enters the DDiScene free list without explicitly
clearing `+0x20`; the allocator zeroes the owner field on reuse. A kind-3/4
mode-0 path returns earlier, and static evidence does not establish callback
reachability after that unlink. REA evidence:
`docs/evidence/legorr-ddscene-lifecycle.bundle` (8 records).

Findings 30–32 (2026-10-21) extend that lifecycle trace. Kind-3/4 teardown
parks auxiliary clones for reuse; a later clone request can reattach one and
draw-step it at time zero. On the general entity-creation path, that draw
precedes assigning the new owner, while the positive-end scene callback cannot
pass its `end <= 0` threshold during that forced step. Pool shutdown clears the
sample-hook gate, while the scene-end callback uses a separate
pointer/time check. Post-shutdown reachability remains unproven. REA evidence:
`docs/evidence/legorr-ddscene-reuse.bundle` (5 focused records).

The fresh verify pass (finding 20, 2026-10-20) re-derived the critical path — PE entry
(`0x48f000`) → WinMain (`FUN_004777a0`) → init → frame loop → engine tick (`0x4232e0`) —
in a clean Ghidra session: no contradicting evidence, and the named corpus reproduces
byte-identical (7,653 lines, `diff` exit 0). It corrected one finding-02 reading:
`DAT_00505814` holds the exe basename minus `.EXE` (the `.wad` prefix), not the install
directory; the real data directory is `cwd\Data` (`DAT_005342a8`).

Open threads, in order of value:
1. **Winapi reference build** — cross-compile the same `src/` to a 32-bit
   `LegoRR.exe` (MinGW-w64, `-Dplatform=winapi`) for A/B against the shipped
   binary under `a2.py run --run-port` (ADRs 0002/0004). **PINNED for now
   (maintainer decision)** — the byte-faithfulness A/B is the remaining
   milestone but is not being worked at the moment; it needs its
   tooling/environment (installable in a distrobox/podman container — this
   box is immutable).
2. Deferred: dgVoodoo as a perf option; the `Steam.lwo` point-only sprites as
   glTF. (The live-mod proof is not planned — dropped by maintainer
   decision.)
3. Reconstruction notes: the `.ol` loader is decompile- + corpus-verified
   (grammar and the `ObjectTypeByName` code table, `entity_test --real`
   over all 44 shipped lists); the front-end/menu boot (finding 23, #41),
   the `.flh` overlay lane (finding 21 — `flic_test --real` decodes all 40
   shipped `.flh`), `.ae` entity assembly (#42), model-aware entity
   rendering (LWO wireframes), the campaign flow, command/AI, selection,
   reward save+advance, the advisor/incident bus wiring, the software level
   renderer, the engine↔level binding seam, and the AVI movie path
   (#47/#48) are all in.

## Findings index

| # | File | Subject |
| --- | --- | --- |
| 01 | [`findings/01-binary-and-loader-map.md`](docs/findings/01-binary-and-loader-map.md) | PE structure, WinMain, resource loader, WAD container format |
| 02 | [`findings/02-startup-and-main-loop.md`](docs/findings/02-startup-and-main-loop.md) | Startup sequence and frame loop, callback triplet |
| 03 | [`findings/03-engine-tick.md`](docs/findings/03-engine-tick.md) | Engine tick, state block, command queue, quit poll |
| 04 | [`findings/04-sim-input-and-events.md`](docs/findings/04-sim-input-and-events.md) | Input system, key table, edge detection, transitions |
| 05 | [`findings/05-exit-selection-and-text-input.md`](docs/findings/05-exit-selection-and-text-input.md) | Exit paths, selection, text input |
| 06 | [`findings/06-command-queue-and-selection.md`](docs/findings/06-command-queue-and-selection.md) | Command queue, selection manager, `KeepRunning` |
| 07 | [`findings/07-level-loading.md`](docs/findings/07-level-loading.md) | `.map` format, voxel grid, ten-layer loading, terrain |
| 08 | [`findings/08-save-and-checkpoints.md`](docs/findings/08-save-and-checkpoints.md) | `.sav`/`.osf` save format, checkpoint blob, no entity dump |
| 09 | [`findings/09-reward-task-manager.md`](docs/findings/09-reward-task-manager.md) | Mission task block, reward overlay, reward-zone save trigger |
| 10 | [`findings/10-entity-command-primitives.md`](docs/findings/10-entity-command-primitives.md) | Command records, AI task/priority registry, opcode map |
| 11 | [`findings/11-model-and-scene-formats.md`](docs/findings/11-model-and-scene-formats.md) | LightWave LWO loader, LWSC scene parser, surface/UV pipeline |
| 12 | [`findings/12-block-text-format.md`](docs/findings/12-block-text-format.md) | `Lego*` block-text config format: scanner, path-lookup API, `.ae` consumers |
| 13 | [`findings/13-texture-system.md`](docs/findings/13-texture-system.md) | BMP surface loader, name cache, LWO material binding, world tile sets, VRAM budget |
| 14 | [`findings/14-sound-and-video.md`](docs/findings/14-sound-and-video.md) | WAVE/mmio sound bank, CD-DA music via MCI, AVIFIL32 movies, logo splash |
| 15 | [`findings/15-compat-audit.md`](docs/findings/15-compat-audit.md) | Modern-compat API-surface audit: wrapper coverage matrix, launch-gate scan, friction list |
| 16 | [`findings/16-a2-live-test.md`](docs/findings/16-a2-live-test.md) | **A.2 live test passed**: game launches under GE-Proton10-25; DDraw interface climbing observed (→ `IDirectDraw4`); movies via quartz; CDDA question deferred (→ 17: resolved — digital release carries no CDDA media) |
| 17 | [`findings/17-interactive-a2-session.md`](docs/findings/17-interactive-a2-session.md) | **Playable**: first mission completes; music is streamed WAVs (no CDDA media on digital release); movies skip on missing Indeo 5 decoder; NVIDIA Xid driver-incident notes |
| 18 | [`findings/18-movies-libav-alias.md`](docs/findings/18-movies-libav-alias.md) | **Movies fixed**: Indeo block was two missing sonames (`libbz2.so.1.0`, `libvpx.so.6`) silently killing wine's ffmpeg plugin — aliases + registry rebuild, automated in `a2.py` |
| 19 | [`findings/19-uv-grammar.md`](docs/findings/19-uv-grammar.md) | **Last format closed**: `.uv` sidecar grammar in `FUN_00481c50` — surface texture-path overrides + per-corner UV (v negated, vertex duplication); names/keyframe tail structurally unread; validated 60/60 |
| 20 | [`findings/20-verify-pass.md`](docs/findings/20-verify-pass.md) | **Fresh verify pass** (2026-10-20): critical path re-derived in a clean session; corpus byte-identical; `DAT_00505814` = `.wad` prefix, not install dir; CRT entry `0x48f000` |
| 21 | [`findings/21-flh-container.md`](docs/findings/21-flh-container.md) | **`.flh` closed**: FLIC-family overlay container — 136 B header, `0xF1FA` chunk stream from `0x80`, RGB555 FLI16 keyframe (opcode 25) + word-diff deltas (opcode 27) + hold chunks, 555→565 on write; validated 18/18 files, 469/469 chunks, rendered frame check |
| 22 | [`findings/22-research-crumbs.md`](docs/findings/22-research-crumbs.md) | **Last open questions closed**: message registry = startup catalog (`FUN_00419230`, 40 `Info_*` names) + `InfoMessages` config population (`FUN_00434c10`); `DAT_004ffbc0` relabeled as the objective state word (not input devices); D3DRM is software-path-only (no HAL device referenced anywhere) |
| 23 | [`findings/23-front-end-and-menu.md`](docs/findings/23-front-end-and-menu.md) | **Front-end & menus closed**: the boot→main-menu machine — `MenuBuild @ 0x4140c0`, the modal `MenuRunLoop`/`MenuTick`, the `FrontEndActive` gate, `MenuEnter`/`FrontEndStateChange`, the level-chain + start-level entries, and the options/movie callbacks, with the config grammar. The port has since implemented `::` path lookup and `Levels::<name>` campaign/tutorial chains; shipped-data end-to-end level-load validation remains |
| 24 | [`findings/24-menu-wipe-and-front-end-transition.md`](docs/findings/24-menu-wipe-and-front-end-transition.md) | **Menu wipe closed**: the config-driven LWS front-end transition — `MenuWipe`/`!MenuWipe` keys (reduce-animation gate, `"Null"` disable) loaded by `LoadMenuBuild @ 0x4161f0` into the LWS scene factory (`FUN_00472cd0`, kind `"LWS"` → `FUN_004765c0`), stepping `RockWipe.lws` (35f @ 25 fps) as the menu-change wipe in `MenuRunLoop`/`MenuTick` with an input freeze — the wipe/fade gap left open in finding 23 |
| 25 | [`findings/25-ddscene-object-runtime.md`](docs/findings/25-ddscene-object-runtime.md) | **DDiScene object runtime closed**: the game-wide scene-graph object model the MenuWipe (finding 24) and every animated 3D thing runs on — the 9-slot kind string table (`NULL MESH FRAME ANIM ACT - - LWS LWO`), the pooled `0x2c`-byte object record (`DAT_0076add0` free list, kind at `+0x10`, flags at `+0x14`), the draw/step API (`FUN_00475140` draw-at-time, `FUN_00477010`/`FUN_00477150` begin/end scene, `gods98/dx6` source path), and the `FUN_004765c0` scene loader behind `DAT_00505914` |
| 26 | [`findings/26-ddscene-animation-hooks.md`](docs/findings/26-ddscene-animation-hooks.md) | **DDiScene animation hooks closed**: `FUN_00475140` can call the action-entry **SFX sample** hook (`DAT_0076cd78/7c`, registered by the `FUN_00464740` 45-slot SFX-name-hash bank init, `0x27` = `SFX_RockWipe` → `FUN_00465050`→`FUN_0047ad70` positional play) and the scene-**end entity flag** hook (`DAT_0076cd80/84`, boot-registered, ORs `0x10000` into `entity+0x3e4`) — plus the `FUN_00464f60` pending-SFX queue |
| 27 | [`findings/27-scene-done-flag-consumers.md`](docs/findings/27-scene-done-flag-consumers.md) | **Scene-done flag readers closed**: both consumers of `entity+0x3e4 bit 0x10000` (set by finding 26's scene-end hook) inside the finding-10 flag-evaluator blob — reader A (`0x43dd10`) plays **placement sounds by work-object type** (`7`→`SFX_PlaceOre`, `6`→`SFX_PlaceCrystal`, else `SFX_Place` via `FUN_00465050`), reader B (`0x43d903`) runs the `FUN_004475c0` impact/wake probe (message 5, roll speed, shake) and clears the bit — plus new `+0x3e4` bit facts (`0x80000`, `0x100000`, `0x40000000`) appended to finding 10 |
| 28 | [`findings/28-ddscene-entity-ownership.md`](docs/findings/28-ddscene-entity-ownership.md) | **DDiScene owner link closed**: `FUN_00473da0/0x473db0` set/read object `+0x20`; the entity spawner assigns the owner on general scene objects and propagates it through type-specific model scene nodes, explaining how the scene-end callback returns to `entity+0x3e4` |
| 29 | [`findings/29-ddscene-entity-teardown.md`](docs/findings/29-ddscene-entity-teardown.md) | **DDiScene teardown traced**: entity removal calls `FUN_00472a40(entity+0x1c)` before recycling the entity; ordinary pooled objects enter the DDiScene free list without an explicit `+0x20` clear (zeroed on reuse), while kind-3/4 has an early auxiliary-record return; post-unlink callback reachability remains unknown |
| 30 | [`findings/30-ddscene-clone-reuse.md`](docs/findings/30-ddscene-clone-reuse.md) | **DDiScene clone slots traced**: kind-3/4 mode-0 teardown marks an auxiliary instance inactive; `FUN_00473690` can find it later, reattach it, and call `FUN_00475140(clone, 0)` |
| 31 | [`findings/31-ddscene-reuse-callback-order.md`](docs/findings/31-ddscene-reuse-callback-order.md) | **Cached-clone callback ordering bounded**: entity creation assigns the owner after the clone factory's synchronous time-zero draw; positive scene-end frame values cannot pass the draw-step's `end <= 0` callback condition |
| 32 | [`findings/32-ddscene-shutdown-hook-gates.md`](docs/findings/32-ddscene-shutdown-hook-gates.md) | **DDiScene shutdown gates distinguished**: shutdown clears the sample-hook enable bit, while the scene-end callback has a separate pointer/time predicate; post-shutdown reachability remains unknown |
| 33 | [`findings/33-ddscene-init-failure-cleanup.md`](docs/findings/33-ddscene-init-failure-cleanup.md) | **DDiScene pool cleanup is on the initializer's failure exit**: the parsed config/resource failures converge on shutdown; success returns without this cleanup; one other direct callsite is outside a procedure |
| 34 | [`findings/34-ddscene-failed-init-retains-end-hook.md`](docs/findings/34-ddscene-failed-init-retains-end-hook.md) | **Failed initialization retains the scene-end hook pointer**: hook registration precedes later failure cleanup, which clears the sample gate but leaves the end-hook slots untouched; dispatch after failure is unproven |
| 35 | [`findings/35-ddscene-paired-channel-update.md`](docs/findings/35-ddscene-paired-channel-update.md) | **Paired-scene update can skip its primary channel**: a missing primary action under a wrapper flag suppresses its draw step while a present companion is still advanced |
| 36 | [`findings/36-ddscene-activity-name-cache.md`](docs/findings/36-ddscene-activity-name-cache.md) | **Activity-name pointer cache suppresses repeats after a miss**: the type-1 dispatcher caches the request pointer even when neither scene channel accepts it, so another call with that pointer short-circuits |
| 37 | [`findings/37-advisor-animation-controller.md`](docs/findings/37-advisor-animation-controller.md) | **Advisor animation controller setup**: map-pointing/talking action labels and a companion DDiScene are initialized by a dedicated controller |
| 38 | [`findings/38-advisor-request-admission.md`](docs/findings/38-advisor-request-admission.md) | **Advisor request admission**: slot availability, a shared gate, and active-state bit control admission; accepted requests encode two modes |
| 39 | [`findings/39-advisor-request-reset-sounds.md`](docs/findings/39-advisor-request-reset-sounds.md) | **Advisor request reset**: accepted requests reset the scene and submit a slot sound plus queued `SFX_CaptainSlide` |
| 40 | [`findings/40-advisor-transition-worker.md`](docs/findings/40-advisor-transition-worker.md) | **Advisor transition worker**: selected and companion scenes advance together with a per-slot transform and restart on completion |
| 41 | [`findings/41-advisor-animation-window.md`](docs/findings/41-advisor-animation-window.md) | **Advisor action windows**: one state bit selects completion or overflow-wrap behavior using slot boundaries and scene duration |
| 42 | [`findings/42-ddscene-sample-hook-consumption.md`](docs/findings/42-ddscene-sample-hook-consumption.md) | **DDiScene sample-hook consumption**: action selection sets a pending bit that the next qualifying kind-4 draw-step consumes |
| 43 | [`findings/43-ddscene-end-hook-repeatability.md`](docs/findings/43-ddscene-end-hook-repeatability.md) | **DDiScene end-hook repeatability**: callback conditions have no local one-shot latch; duplicate runtime dispatch remains unverified |
| 44 | [`findings/44-reward-buttons-use-separate-save-routes.md`](docs/findings/44-reward-buttons-use-separate-save-routes.md) | Reward Save enters the slot UI; Advance commits through a separate checkpoint route |
| 45 | [`findings/45-timer-objective-short-circuits-other-checks.md`](docs/findings/45-timer-objective-short-circuits-other-checks.md) | Timer expiry short-circuits later objective checks in the campaign evaluator |
| 46 | [`findings/46-objective-sidecar-append-gates.md`](docs/findings/46-objective-sidecar-append-gates.md) | Objective sidecar records have additional append gates and a fixed serialized field set |
| 47 | [`findings/47-entity-events-feed-incident-messages.md`](docs/findings/47-entity-events-feed-incident-messages.md) | Entity removal emits shared and type-specific event messages into the incident path |
| 48 | [`findings/48-directdraw-renderer-setup.md`](docs/findings/48-directdraw-renderer-setup.md) | DirectDraw mode setup and bridge into the D3DRM device/viewport path |
| 49 | [`findings/49-model-render-pass-dispatch.md`](docs/findings/49-model-render-pass-dispatch.md) | Model rendering walks ordered child passes through the shared renderer |
| 50 | [`findings/50-tile-texture-refresh-order.md`](docs/findings/50-tile-texture-refresh-order.md) | Tile texture refresh updates indexed slots in a defined pass order |
| 51 | [`findings/51-held-key-command-repeat.md`](docs/findings/51-held-key-command-repeat.md) | EnginePreFrame binds key bytes to command records; held keys enqueue again each tick |
| 52 | [`findings/52-advance-gates-campaign-completion-mark.md`](docs/findings/52-advance-gates-campaign-completion-mark.md) | Reward Advance gates campaign-row completion marks on the current level result |
| 53 | [`findings/53-nextlevel-builds-campaign-chain-arrays.md`](docs/findings/53-nextlevel-builds-campaign-chain-arrays.md) | Startup builds normal and tutorial level chains by following `NextLevel` references |
| 54 | [`findings/54-terrain-grid-emits-per-cell-quad-vertices.md`](docs/findings/54-terrain-grid-emits-per-cell-quad-vertices.md) | Terrain initialization builds and submits four vertex records for each grid cell |
| 55 | [`findings/55-terrain-cell-material-changes-use-bounded-transition-records.md`](docs/findings/55-terrain-cell-material-changes-use-bounded-transition-records.md) | Cell material changes update the surface and vertices, with a gated ten-record path |
| 56 | [`findings/56-incident-message-template-format.md`](docs/findings/56-incident-message-template-format.md) | Configured incident text compiles underscores, line breaks, and `$token$` spans |
| 57 | [`findings/57-incident-message-advisor-consumer.md`](docs/findings/57-incident-message-advisor-consumer.md) | Queued incident messages feed the scrolling text widget through a gated consumer |
| 58 | [`findings/58-win32-message-pump-filter.md`](docs/findings/58-win32-message-pump-filter.md) | Win32 message pump filters selected messages before translation and dispatch |
| 59 | [`findings/59-directinput-key-state-to-text.md`](docs/findings/59-directinput-key-state-to-text.md) | DirectInput snapshots feed key-edge handling and the text-entry path |
| 60 | [`findings/60-win32-cursor-sampling.md`](docs/findings/60-win32-cursor-sampling.md) | Win32 cursor sampling stores clamped client-relative coordinates and deltas |
| 61 | [`findings/61-cursor-screen-ray-camera-routing.md`](docs/findings/61-cursor-screen-ray-camera-routing.md) | Gameplay cursor coordinates feed a camera unprojection and ray path |
| 62 | [`findings/62-three-dimensional-sound-distance-controls.md`](docs/findings/62-three-dimensional-sound-distance-controls.md) | Configured 3D sound distance and rolloff values update DirectSound interfaces |
| 63 | [`findings/63-sound-instance-buffer-rotation-and-routing.md`](docs/findings/63-sound-instance-buffer-rotation-and-routing.md) | Sound submissions rotate duplicate-enabled buffers and separate streaming from in-memory playback |
| 64 | [`findings/64-wad-stream-handle-contract.md`](docs/findings/64-wad-stream-handle-contract.md) | WAD opens materialize per-open buffers behind the generic stream handle |
| 65 | [`findings/65-wad-end-relative-seek-boundary.md`](docs/findings/65-wad-end-relative-seek-boundary.md) | WAD end-relative seeks bypass the range clamp; out-of-range reads carry a derived risk |
| 66 | [`findings/66-front-end-options-round-trip-through-six-save-contexts.md`](docs/findings/66-front-end-options-round-trip-through-six-save-contexts.md) | Three front-end option globals round-trip through six per-slot contexts |
| 67 | [`findings/67-save-slot-write-includes-option-context.md`](docs/findings/67-save-slot-write-includes-option-context.md) | Save-slot headers serialize the selected option context along with level records |
| 68 | [`findings/68-avi-frame-timebase.md`](docs/findings/68-avi-frame-timebase.md) | AVI frame indices derive from elapsed time and stream rate |
| 69 | [`findings/69-avi-stream-open-and-teardown.md`](docs/findings/69-avi-stream-open-and-teardown.md) | AVI setup stages retain stream state across later failures before teardown |
| 70 | [`findings/70-activity-change-dispatch-and-consumption.md`](docs/findings/70-activity-change-dispatch-and-consumption.md) | Activity pointer changes select typed runners, then commit timer and state |
| 71 | [`findings/71-save-slot-read-restores-level-markers-not-the-live-task-block.md`](docs/findings/71-save-slot-read-restores-level-markers-not-the-live-task-block.md) | Slot reads feed campaign-list completion markers; live task restoration was not observed |
| 72 | [`findings/72-osf-reader-return-means-open-not-validated.md`](docs/findings/72-osf-reader-return-means-open-not-validated.md) | `.osf` reader's success result records file-open, even when validation rejects data |
| 73 | [`findings/73-flh-caller-gated-playback.md`](docs/findings/73-flh-caller-gated-playback.md) | `.flh` advancement is caller-gated while the current surface continues drawing |
| 74 | [`findings/74-reward-flh-context-lifetime.md`](docs/findings/74-reward-flh-context-lifetime.md) | Reward `.flh` contexts load lazily per task and close on transition or exit |
| 75 | [`findings/75-olistfile-entry-placement.md`](docs/findings/75-olistfile-entry-placement.md) | Level `.ol` loading creates typed entities from required entry fields |
| 76 | [`findings/76-entity-teardown-drains-command-chain-by-flags.md`](docs/findings/76-entity-teardown-drains-command-chain-by-flags.md) | Entity teardown drains command chains through flag-dependent disposition paths |
| 77 | [`findings/77-selection-clear-marker.md`](docs/findings/77-selection-clear-marker.md) | A zero-mode selection action clears active selection and transfers a per-entity marker |
| 78 | [`findings/78-energy-low-target-routing.md`](docs/findings/78-energy-low-target-routing.md) | Energy decay clamps a per-entity resource field using type stats |
| 79 | [`findings/79-terrain-voxel-mutation-stages.md`](docs/findings/79-terrain-voxel-mutation-stages.md) | Terrain mutation advances selected voxel states through a four-step progression |
| 80 | [`findings/80-kind-four-deposit-activity-gate.md`](docs/findings/80-kind-four-deposit-activity-gate.md) | Kind-4 entities enter `Activity_Deposit` through a gated state transition |
| 81 | [`findings/81-emergemap-seeds-delayed-lava-erosion-stages.md`](docs/findings/81-emergemap-seeds-delayed-lava-erosion-stages.md) | `EmergeMap` seeds a bounded table of delayed erosion cells |
| 82 | [`findings/82-healthbar-config-derives-clamped-border-shades.md`](docs/findings/82-healthbar-config-derives-clamped-border-shades.md) | Health-bar config derives lighter/darker border colors and clamps channels |
| 83 | [`findings/83-olistfile-driving-references.md`](docs/findings/83-olistfile-driving-references.md) | `.ol` loading resolves optional `driving` names in a second pass |
| 84 | [`findings/84-entity-teardown-dispatches-type-specific-payload-cleanup.md`](docs/findings/84-entity-teardown-dispatches-type-specific-payload-cleanup.md) | Entity teardown dispatches payload cleanup by raw type, with a type-3 gate |
| 85 | [`findings/85-selection-candidate-eligibility.md`](docs/findings/85-selection-candidate-eligibility.md) | Selection candidates pass flag, kind, and nonnegative-float eligibility gates |
| 86 | [`findings/86-low-resource-targets-queue-type-eleven-command.md`](docs/findings/86-low-resource-targets-queue-type-eleven-command.md) | Low-resource entities queue a type-0x11 target command after route checks |
| 87 | [`findings/87-other-voxel-states-refresh-neighborhood-flags.md`](docs/findings/87-other-voxel-states-refresh-neighborhood-flags.md) | Other voxel states run a separate neighborhood-flag refresh path |
| 88 | [`findings/88-neighbor-scan-consumes-terrain-work-flags.md`](docs/findings/88-neighbor-scan-consumes-terrain-work-flags.md) | A neighbor scan consumes terrain work flags through mutation mode 1 |
| 89 | [`findings/89-kind-four-deposit-monitor-uses-count-and-state-gates.md`](docs/findings/89-kind-four-deposit-monitor-uses-count-and-state-gates.md) | A kind-4 monitor gates deposit handoff on count and state |
| 90 | [`findings/90-lava-erosion-ticker-advances-pending-cells.md`](docs/findings/90-lava-erosion-ticker-advances-pending-cells.md) | Lava erosion ticker advances queued cells through timed stages |
| 91 | [`findings/91-olistfile-health-defaults-and-seeds-entity-field.md`](docs/findings/91-olistfile-health-defaults-and-seeds-entity-field.md) | Optional `.ol` health defaults zero to 100 before entity initialization |
| 93 | [`findings/93-objective-advisor-stream-token-tables.md`](docs/findings/93-objective-advisor-stream-token-tables.md) | Objective/advisor streams build separate resource and sound token tables |
| 94 | [`findings/94-objective-advisor-renderer-consumes-inline-tokens.md`](docs/findings/94-objective-advisor-renderer-consumes-inline-tokens.md) | Objective/advisor rendering consumes inline resource and sound tokens |
| 95 | [`findings/95-global-block-record-arena-is-released-at-shutdown.md`](docs/findings/95-global-block-record-arena-is-released-at-shutdown.md) | Block-text records accumulate in a process-wide arena until shutdown |
| 96 | [`findings/96-whole-file-loader-scopes-stream-and-buffer-lifetimes.md`](docs/findings/96-whole-file-loader-scopes-stream-and-buffer-lifetimes.md) | Whole-file loads close the stream and return a separate heap buffer |
| 97 | [`findings/97-explicit-entity-route-associates-target-pointer.md`](docs/findings/97-explicit-entity-route-associates-target-pointer.md) | Explicit entity routes bind a supplied pointer after route setup |
| 98 | [`findings/98-helper-returned-entity-route-marker.md`](docs/findings/98-helper-returned-entity-route-marker.md) | A helper-returned pointer follows a separate route-marker path |
| 99 | [`findings/99-upgrade-admission-tests-level-and-resource-choice.md`](docs/findings/99-upgrade-admission-tests-level-and-resource-choice.md) | Upgrade admission checks the next index and configured resource thresholds |
| 100 | [`findings/100-upgrade-command-advances-building-index.md`](docs/findings/100-upgrade-command-advances-building-index.md) | Upgrade command commits the next building index |
| 101 | [`findings/101-camera-key-hold-and-gates.md`](docs/findings/101-camera-key-hold-and-gates.md) | Camera key adjustments repeat while held behind gameplay and input gates |
| 102 | [`findings/102-camera-config-pan-and-follow-bounds.md`](docs/findings/102-camera-config-pan-and-follow-bounds.md) | Camera settings feed pointer-pan acceleration and follow bounds |
| 103 | [`findings/103-pointer-cursor-config-registry.md`](docs/findings/103-pointer-cursor-config-registry.md) | Pointer cursor config uses a fixed action-name registry |
| 104 | [`findings/104-pointer-overrides-expire-in-the-reward-pump.md`](docs/findings/104-pointer-overrides-expire-in-the-reward-pump.md) | Timed cursor overrides hold through reward-screen input polling |
| 105 | [`findings/105-sav-write-ignores-short-write-results.md`](docs/findings/105-sav-write-ignores-short-write-results.md) | Slot-save success ignores partial write results |
| 106 | [`findings/106-sav-reader-resets-slot-on-record-count-mismatch.md`](docs/findings/106-sav-reader-resets-slot-on-record-count-mismatch.md) | A save record-count mismatch reinitializes the slot |
| 107 | [`findings/107-samples-config-sound-alias-variants.md`](docs/findings/107-samples-config-sound-alias-variants.md) | Samples config builds duplicate/stream sound aliases and selects variants |
| 108 | [`findings/108-streamed-wav-format-extension.md`](docs/findings/108-streamed-wav-format-extension.md) | Streamed WAV parser preserves extensions before a PCM-only playback gate |
| 109 | [`findings/109-emergemap-layerl-records.md`](docs/findings/109-emergemap-layerl-records.md) | EmergeMap values populate six-slot LayerL records |
| 110 | [`findings/110-objective-entity-paired-field-predicate.md`](docs/findings/110-objective-entity-paired-field-predicate.md) | A delegated objective predicate searches entities for paired-field matches |
| 111 | [`findings/111-directdraw-windowed-exclusive-surfaces.md`](docs/findings/111-directdraw-windowed-exclusive-surfaces.md) | Windowed and exclusive modes build different DirectDraw surfaces |
| 112 | [`findings/112-registry-accessor-creates-path-keys.md`](docs/findings/112-registry-accessor-creates-path-keys.md) | The registry read accessor creates each subkey path component |
| 113 | [`findings/113-window-activation-key-sampling.md`](docs/findings/113-window-activation-key-sampling.md) | Focus loss suppresses keyboard sampling while the frame callback continues |
| 114 | [findings/114-terrain-cell-height-and-diagonal-selection.md](docs/findings/114-terrain-cell-height-and-diagonal-selection.md) | TerrainCreate separates map height from cell attributes and selects a diagonal |
| 115 | [findings/115-ptl-message-remap-table.md](docs/findings/115-ptl-message-remap-table.md) | PTLFile builds an ordered table of numeric Message-name pairs |
| 116 | [findings/116-reward-button-color-key-setup.md](docs/findings/116-reward-button-color-key-setup.md) | Reward button image handles receive a zero-to-zero color key |
| 117 | [findings/117-fallin-multiplier-config-source.md](docs/findings/117-fallin-multiplier-config-source.md) | FallinMultiplier supplies the block-pointer modulus factor |
| 118 | [findings/118-loader-progress-read-tap.md](docs/findings/118-loader-progress-read-tap.md) | Whole-file read hooks accumulate loader progress from measured file sizes |
| 119 | [findings/119-directsound-3d-listener-interface.md](docs/findings/119-directsound-3d-listener-interface.md) | Sound initialization queries DirectSound's 3D listener interface |
| 120 | [findings/120-sound-load-duplicates-directsound-buffer.md](docs/findings/120-sound-load-duplicates-directsound-buffer.md) | Sound loading can create two DirectSound duplicates for one sample buffer |
| 121 | [findings/121-per-source-sound-buffers-request-3d-control.md](docs/findings/121-per-source-sound-buffers-request-3d-control.md) | Sound buffers request 3D control and tracked buffers receive distance updates |
| 122 | [findings/122-wav-mmio-buffer-handoff.md](docs/findings/122-wav-mmio-buffer-handoff.md) | In-memory WAV loading copies the data chunk through the MMIO read window |
| 123 | [findings/123-avi-frame-dib-row-copy.md](docs/findings/123-avi-frame-dib-row-copy.md) | AVI frame helper copies DIB rows into the surface pixel buffer |
| 125 | [findings/125-screen-shake-flag-raiser.md](docs/findings/125-screen-shake-flag-raiser.md) | Negative entity health delta raises the screen-shake flag |
| 126 | [findings/126-screen-space-effect-lane-dispatch-and-opacity-update.md](docs/findings/126-screen-space-effect-lane-dispatch-and-opacity-update.md) | Ten-entry effect table dispatches timed opacity updates |
| 128 | [findings/128-permitted-activity-appender-capacity.md](docs/findings/128-permitted-activity-appender-capacity.md) | Activity appender reads capacity from the entity stats block |
| 129 | [findings/129-effect-item-flags-reach-model-render-dispatch.md](docs/findings/129-effect-item-flags-reach-model-render-dispatch.md) | Effect item flags can reach model render passes |
| 130 | [findings/130-activity-3f4-direct-reader-census.md](docs/findings/130-activity-3f4-direct-reader-census.md) | No other direct entity-field reference to the appender's `+0x3f4` store |
| 131 | [findings/131-tool-carry-stat-is-activity-capacity.md](docs/findings/131-tool-carry-stat-is-activity-capacity.md) | Stats `NumOfToolsCanCarry` values feed activity-list capacity |
| 133 | [findings/133-directsound-duplicate-buffer-selection.md](docs/findings/133-directsound-duplicate-buffer-selection.md) | Optional DirectSound duplicates rotate across three buffers at playback |
| 134 | [findings/134-samples-asterisk-enables-buffer-rotation.md](docs/findings/134-samples-asterisk-enables-buffer-rotation.md) | `Samples` asterisk marker requests duplicate buffers and three-buffer rotation |
| 135 | [findings/135-stream-marker-persists-across-sample-variants.md](docs/findings/135-stream-marker-persists-across-sample-variants.md) | `@` stream mode persists across later comma-delimited sample variants |
| 136 | [findings/136-streamed-playback-replaces-selected-stream-state.md](docs/findings/136-streamed-playback-replaces-selected-stream-state.md) | Starting a stream replaces the active stream in the selected state |
| 137 | [findings/137-stream-selector-caller-trace.md](docs/findings/137-stream-selector-caller-trace.md) | Level-name streams use selector zero; sound-instance submissions pass one |
| 138 | [findings/138-damage-hurt-sound-submits-with-selector-zero.md](docs/findings/138-damage-hurt-sound-submits-with-selector-zero.md) | Damage hurt-sound submission passes selector zero; the shipped alias is in-memory WAV |
| 139 | [findings/139-sound-submission-queue-preserves-selector-bits.md](docs/findings/139-sound-submission-queue-preserves-selector-bits.md) | Deferred sound submissions preserve selector and route bits |
| 140 | [findings/140-objective-outcome-streams-use-selector-zero.md](docs/findings/140-objective-outcome-streams-use-selector-zero.md) | Objective-outcome voice streams submit through selector zero |
| 141 | [findings/141-menu-overlay-streams-submit-on-state-zero.md](docs/findings/141-menu-overlay-streams-submit-on-state-zero.md) | Menu-overlay streams submit one queue step later through state zero |
| 142 | [findings/142-sample-hash-marker-sets-directsound-volume.md](docs/findings/142-sample-hash-marker-sets-directsound-volume.md) | `#...#` sets a per-sample DirectSound attenuation value |
| 143 | [findings/143-reducesamples-skips-bang-prefixed-sound-aliases.md](docs/findings/143-reducesamples-skips-bang-prefixed-sound-aliases.md) | `-reducesamples` skips `!`-prefixed aliases before registration |
| 144 | [findings/144-music-stream-aliases-use-state-one.md](docs/findings/144-music-stream-aliases-use-state-one.md) | Shipped ambient and stats music streams select stream state one |
| 145 | [findings/145-sample-hash-marker-parser-omits-termination-and-bounds.md](docs/findings/145-sample-hash-marker-parser-omits-termination-and-bounds.md) | `#...#` parser can read past its token or overwrite its stack buffer |
| 146 | [findings/146-streamed-samples-ignore-asterisk-duplicate-flag.md](docs/findings/146-streamed-samples-ignore-asterisk-duplicate-flag.md) | `@` streaming bypasses the `*` duplicate-buffer path |
| 147 | [findings/147-sample-variant-choice-uses-shared-seeded-lcg.md](docs/findings/147-sample-variant-choice-uses-shared-seeded-lcg.md) | Sample variant selection consumes the shared LCG state |
| 148 | [findings/148-global-wav-playback-applies-eight-db-attenuation.md](docs/findings/148-global-wav-playback-applies-eight-db-attenuation.md) | Global in-memory WAV playback subtracts 8 dB before SetVolume |
| 149 | [findings/149-deferred-sound-queue-drops-requests-after-ten.md](docs/findings/149-deferred-sound-queue-drops-requests-after-ten.md) | Deferred sound queue silently drops requests after ten entries |
| 150 | [findings/150-streamed-wav-playback-rejects-non-pcm.md](docs/findings/150-streamed-wav-playback-rejects-non-pcm.md) | Streamed playback rejects non-PCM WAV formats before buffer creation |
| 151 | [findings/151-streamed-pcm-buffer-uses-four-aligned-segments.md](docs/findings/151-streamed-pcm-buffer-uses-four-aligned-segments.md) | Streamed PCM buffer uses four aligned segments totaling about three seconds |
| 152 | [findings/152-streamed-playback-reuses-buffer-as-looping-ring.md](docs/findings/152-streamed-playback-reuses-buffer-as-looping-ring.md) | Streamed playback loops and refills its DirectSound ring buffer |
| 153 | [findings/153-stream-start-ignores-directsound-position-and-play-errors.md](docs/findings/153-stream-start-ignores-directsound-position-and-play-errors.md) | Stream start ignores position and Play HRESULTs before marking active |
| 154 | [findings/154-streamed-playback-stops-after-source-data-drains.md](docs/findings/154-streamed-playback-stops-after-source-data-drains.md) | Streamed playback pads the tail and stops after queued data drains |
| 155 | [findings/155-shipped-music-streams-exceed-ring-buffer.md](docs/findings/155-shipped-music-streams-exceed-ring-buffer.md) | Shipped music tracks exceed the three-second streamed ring buffer |
| 156 | [findings/156-options-sfx-volume-uses-nonlinear-directsound-table.md](docs/findings/156-options-sfx-volume-uses-nonlinear-directsound-table.md) | Options SFX slider maps to a nonlinear DirectSound attenuation table |
| 157 | [findings/157-music-volume-slider-collapses-top-two-values.md](docs/findings/157-music-volume-slider-collapses-top-two-values.md) | Music volume values 9 and 10 both request maximum mixer level |
| 158 | [findings/158-sfx-volume-slider-sets-primary-directsound-volume.md](docs/findings/158-sfx-volume-slider-sets-primary-directsound-volume.md) | SFX-labeled slider sets the shared primary DirectSound output volume |
| 159 | [findings/159-music-volume-targets-compact-disc-mixer-source.md](docs/findings/159-music-volume-targets-compact-disc-mixer-source.md) | Music volume callback targets the audio-CD mixer source |
| 160 | [findings/160-music-volume-defaults-to-zero-without-cd-mixer-control.md](docs/findings/160-music-volume-defaults-to-zero-without-cd-mixer-control.md) | Music slider starts at zero when the CD mixer source is unavailable |
| 161 | [findings/161-gameplay-uses-positional-and-object-attached-sfx.md](docs/findings/161-gameplay-uses-positional-and-object-attached-sfx.md) | Gameplay and shared-object events submit aliases through spatial sound paths |
| 162 | [findings/162-object-attached-sounds-refresh-spatial-state.md](docs/findings/162-object-attached-sounds-refresh-spatial-state.md) | Object-attached sounds refresh stored spatial values during sound updates |
| 163 | [findings/163-view-provider-feeds-directsound-listener.md](docs/findings/163-view-provider-feeds-directsound-listener.md) | The frame tick updates listener position and orientation from the stored view |
| 164 | [findings/164-explicit-sound-positions-apply-immediately.md](docs/findings/164-explicit-sound-positions-apply-immediately.md) | Explicit positions apply immediately; dispatcher also commits deferred settings |
| 165 | [findings/165-object-spatial-update-registry-has-twenty-slots.md](docs/findings/165-object-spatial-update-registry-has-twenty-slots.md) | Object spatial-update registry has 20 slots and does not deduplicate |
| 166 | [findings/166-spatial-playback-enters-through-shared-wrapper.md](docs/findings/166-spatial-playback-enters-through-shared-wrapper.md) | Direct spatial playback uses the shared positional/object wrapper |
| 167 | [findings/167-global-alias-playback-disables-3d-mode.md](docs/findings/167-global-alias-playback-disables-3d-mode.md) | Global in-memory aliases disable DirectSound 3D processing |
| 168 | [findings/168-in-memory-sound-start-ignores-hresults.md](docs/findings/168-in-memory-sound-start-ignores-hresults.md) | In-memory sound start returns a slot identifier without checking DirectSound HRESULTs |
| 169 | [findings/169-shared-object-sound-events-use-time-windows.md](docs/findings/169-shared-object-sound-events-use-time-windows.md) | Shared-object sound events submit object-attached aliases within time windows |
| 170 | [findings/170-shared-object-transition-stops-new-sound-handle.md](docs/findings/170-shared-object-transition-stops-new-sound-handle.md) | Shared-object transition may stop the newly returned sound handle |

| 171 | [findings/171-queued-shared-sound-records-zero-handle.md](docs/findings/171-queued-shared-sound-records-zero-handle.md) | Queued Shared-Object Sound Stores Handle Zero |
| 172 | [findings/172-avi-frames-blit-to-render-target.md](docs/findings/172-avi-frames-blit-to-render-target.md) | AVI Frames Blit to the Shared DirectDraw Render Target |
| 173 | [findings/173-avi-allocation-failure-reaches-frame-updater.md](docs/findings/173-avi-allocation-failure-reaches-frame-updater.md) | AVI Allocation Failure Reaches a Null Frame Updater |
| 174 | [findings/174-avi-frame-retriever-is-not-explicitly-closed.md](docs/findings/174-avi-frame-retriever-is-not-explicitly-closed.md) | AVI Frame Retriever Has No Explicit Close Path |
| 175 | [findings/175-null-avi-frame-reblits-previous-pixels.md](docs/findings/175-null-avi-frame-reblits-previous-pixels.md) | Null AVI Frame Fetch Leaves and Reblits Previous Pixels |
| 176 | [findings/176-avi-command-loop-restarts-at-stream-length.md](docs/findings/176-avi-command-loop-restarts-at-stream-length.md) | AVI Command Loop Restarts at Stream Length |
| 177 | [findings/177-avi-player-ignores-embedded-audio-streams.md](docs/findings/177-avi-player-ignores-embedded-audio-streams.md) | AVI Player Opens Video Streams but Not Embedded Audio |
| 178 | [findings/178-dynamic-sample-alias-table-has-no-capacity-check.md](docs/findings/178-dynamic-sample-alias-table-has-no-capacity-check.md) | Dynamic Sample Alias Registration Can Overrun Its Hash Table |
| 180 | [findings/180-cdda-watchdog-detects-track-overrun-at-four-second-intervals.md](docs/findings/180-cdda-watchdog-detects-track-overrun-at-four-second-intervals.md) | CD Audio Watchdog Detects Track Overrun at Four-Second Intervals |
| 181 | [findings/181-avi-command-loop-input-exit.md](docs/findings/181-avi-command-loop-input-exit.md) | AVI Command Loop Exits on Keyboard Input or Mouse Release |
| 182 | [findings/182-credits-scroll-uses-33ms-clock-steps.md](docs/findings/182-credits-scroll-uses-33ms-clock-steps.md) | Credits Scroll Uses 33 ms Clock Steps |
| 183 | [findings/183-shipped-credits-text-matches-crlf-split-count.md](docs/findings/183-shipped-credits-text-matches-crlf-split-count.md) | Shipped Credits Text Matches the CRLF Split Count |
| 184 | [findings/184-credits-text-prefers-cconv-sidecar.md](docs/findings/184-credits-text-prefers-cconv-sidecar.md) | Credits Text Lookup Prefers a `.cconv` Sidecar |
| 185 | [findings/185-successful-avi-loop-has-no-explicit-sleep.md](docs/findings/185-successful-avi-loop-has-no-explicit-sleep.md) | Successful AVI Playback Has No Explicit Loop Sleep |
| 186 | [findings/186-credits-background-bypasses-dontplayavis-gate.md](docs/findings/186-credits-background-bypasses-dontplayavis-gate.md) | `DontPlayAvis` Gates Boot Movies but Not Other Video Paths |
| 187 | [findings/187-video-commands-use-two-playback-routes.md](docs/findings/187-video-commands-use-two-playback-routes.md) | Configured Videos Use DirectShow; Credits Use AVIFIL32 |
| 188 | [findings/188-com-video-route-does-not-explicitly-uninitialize.md](docs/findings/188-com-video-route-does-not-explicitly-uninitialize.md) | COM Video Route Has No Explicit `CoUninitialize` Call |
| 189 | [findings/189-configured-videos-add-primary-audio-stream.md](docs/findings/189-configured-videos-add-primary-audio-stream.md) | Configured Videos Add Primary Video and Audio Streams |
| 190 | [findings/190-directshow-partial-init-cleanup-assumes-interfaces.md](docs/findings/190-directshow-partial-init-cleanup-assumes-interfaces.md) | DirectShow Partial-Setup Cleanup Assumes Every Interface Exists |
| 191 | [findings/191-level-video-stops-on-button-down.md](docs/findings/191-level-video-stops-on-button-down.md) | Level Video Stops on Mouse Button Down |
| 192 | [findings/192-shipped-boot-avis-have-audio-streams.md](docs/findings/192-shipped-boot-avis-have-audio-streams.md) | Shipped Boot AVIs Have Audio Streams; Credits Loop Does Not |
| 193 | [findings/193-media-class-creation-failure-releases-null.md](docs/findings/193-media-class-creation-failure-releases-null.md) | Failed Media-Class Creation Reaches `Release` Through Null |
| 194 | [findings/194-configured-video-graph-runs-on-separate-thread.md](docs/findings/194-configured-video-graph-runs-on-separate-thread.md) | Configured Video Graph Runs on a Separate Thread |
| 195 | [findings/195-cdda-watchdog-parses-current-track-without-checking-mci-result.md](docs/findings/195-cdda-watchdog-parses-current-track-without-checking-mci-result.md) | CD-Audio Watchdog Parses Track Status Without Checking MCI Success |
| 196 | [findings/196-listener-uses-scene-relative-camera-frame-vectors.md](docs/findings/196-listener-uses-scene-relative-camera-frame-vectors.md) | The Listener Uses Scene-Relative Frame Vectors |
| 197 | [findings/197-listener-caches-deferred-values-without-checking-results.md](docs/findings/197-listener-caches-deferred-values-without-checking-results.md) | Listener Caches Deferred Values Without Checking Results |
| 198 | [findings/198-attached-sound-caches-position-after-immediate-set.md](docs/findings/198-attached-sound-caches-position-after-immediate-set.md) | Attached Sound Caches Position After Immediate Set |
| 199 | [findings/199-directsound-keeps-default-distance-factor.md](docs/findings/199-directsound-keeps-default-distance-factor.md) | DirectSound Keeps Its Default Distance Factor |
| 200 | [findings/200-configured-video-polls-directshow-sample.md](docs/findings/200-configured-video-polls-directshow-sample.md) | Configured Video Pumps and Blits DirectShow Samples |
| 201 | [findings/201-configured-video-stops-after-update-failure.md](docs/findings/201-configured-video-stops-after-update-failure.md) | Configured Video Stops When Its Update Callback Fails |
| 202 | [findings/202-video-duration-only-bounds-render-count.md](docs/findings/202-video-duration-only-bounds-render-count.md) | Video Duration Bounds the Render Count, Not Frame Timing |
| 203 | [findings/203-spatial-sound-ignores-frame-query-failures.md](docs/findings/203-spatial-sound-ignores-frame-query-failures.md) | Spatial Sound Ignores Frame-Query Failures |
| 204 | [findings/204-stopping-one-attached-sound-clears-all-provider-entries.md](docs/findings/204-stopping-one-attached-sound-clears-all-provider-entries.md) | Stopping One Attached Sound Clears All Registry Entries for Its Frame |
| 205 | [findings/205-directshow-video-uses-shared-render-presentation.md](docs/findings/205-directshow-video-uses-shared-render-presentation.md) | DirectShow Video Uses the Shared Render Presentation Path |
| 206 | [findings/206-configured-video-blit-fills-render-surface.md](docs/findings/206-configured-video-blit-fills-render-surface.md) | Configured Video Blits Fill the Render Surface |
| 207 | [findings/207-configured-video-releases-without-explicit-stop.md](docs/findings/207-configured-video-releases-without-explicit-stop.md) | Configured Video Releases Its Stream Without an Explicit Stop Call |
| 208 | [findings/208-video-setup-ignores-audio-stream-add-failure.md](docs/findings/208-video-setup-ignores-audio-stream-add-failure.md) | Configured Video Continues if Adding Primary Audio Fails |
| 209 | [findings/209-batched-spatial-sound-queue-drops-requests-after-ten.md](docs/findings/209-batched-spatial-sound-queue-drops-requests-after-ten.md) | Batched Spatial Sound Queue Drops Requests After Ten |
| 210 | [findings/210-avi-frame-copy-proceeds-after-lock-failure.md](docs/findings/210-avi-frame-copy-proceeds-after-lock-failure.md) | AVI Frame Copy Proceeds After Surface Lock Failure |
| 211 | [findings/211-advisor-sound-token-is-latched-before-audio-gate.md](docs/findings/211-advisor-sound-token-is-latched-before-audio-gate.md) | Advisor Sound Token Is Latched Before the Audio Gate |
| 212 | [findings/212-frame-sound-tick-does-not-drain-packed-batch-queue.md](docs/findings/212-frame-sound-tick-does-not-drain-packed-batch-queue.md) | Frame Sound Tick Does Not Drain the Packed Batch Queue |
| 213 | [findings/213-disabling-sound-does-not-clear-packed-requests.md](docs/findings/213-disabling-sound-does-not-clear-packed-requests.md) | Disabling Sound Does Not Clear Packed Requests |
| 214 | [findings/214-objective-voice-timer-does-not-stop-stream.md](docs/findings/214-objective-voice-timer-does-not-stop-stream.md) | Objective Voice Timer Does Not Stop the Stream |
| 215 | [findings/215-in-memory-wav-failures-leave-mmio-handle-open.md](docs/findings/215-in-memory-wav-failures-leave-mmio-handle-open.md) | In-Memory WAV Failures Leave the MMIO Handle Open |
| 216 | [findings/216-avi-decoder-requests-best-display-format.md](docs/findings/216-avi-decoder-requests-best-display-format.md) | AVI Decoder Is Asked for the Best Display Format |
| 217 | [findings/217-streamed-wav-registration-ignores-format-rate-failure.md](docs/findings/217-streamed-wav-registration-ignores-format-rate-failure.md) | Streamed WAV Registration Ignores a Failed Format-Rate Probe |
| 218 | [findings/218-streamed-wav-metadata-probe-bypasses-game-file-layer.md](docs/findings/218-streamed-wav-metadata-probe-bypasses-game-file-layer.md) | Streamed WAV Metadata Probe Bypasses the Game File Layer |
| 219 | [findings/219-directshow-end-of-stream-takes-sample-cleanup-path.md](docs/findings/219-directshow-end-of-stream-takes-sample-cleanup-path.md) | DirectShow End of Stream Takes the Sample Cleanup Path |
| 220 | [findings/220-avi-unsupported-surface-depth-still-advances.md](docs/findings/220-avi-unsupported-surface-depth-still-advances.md) | AVI Timeline Advances When the Movie Surface Depth Is Unsupported |
| 221 | [findings/221-lava-effect-latches-zero-sound-handle.md](docs/findings/221-lava-effect-latches-zero-sound-handle.md) | Lava Effect Latches a Zero Sound Handle |
| 222 | [findings/222-wav-fmt-extension-read-is-not-chunk-bounded.md](docs/findings/222-wav-fmt-extension-read-is-not-chunk-bounded.md) | WAV Format Extension Read Is Not Bounded by the `fmt ` Chunk |
| 223 | [findings/223-cell-init-reads-outside-grid-at-top-and-left-edges.md](docs/findings/223-cell-init-reads-outside-grid-at-top-and-left-edges.md) | Conditional top/left edge read before the cell grid; Level14/17 tutorial spawns and two cell-flag writers are traced, while edge walkability and runtime ordering remain unknown |
| 224 | [findings/224-predug-cells-feed-a-capped-spawn-coordinate-pool.md](docs/findings/224-predug-cells-feed-a-capped-spawn-coordinate-pool.md) | Predug Cell Values Feed a Capped Coordinate Pool |
| 225 | [findings/225-terrain-objective-level-and-diagnostic-helpers.md](docs/findings/225-terrain-objective-level-and-diagnostic-helpers.md) | Newly Identified Terrain, Objective, Level, and Diagnostic Helpers |
| 226 | [findings/226-omitted-simulation-terrain-resource-text-procedures.md](docs/findings/226-omitted-simulation-terrain-resource-text-procedures.md) | Omitted Simulation, Terrain, Resource, and Text Procedures |
| 227 | [findings/227-newly-identified-engine-and-terrain-helpers.md](docs/findings/227-newly-identified-engine-and-terrain-helpers.md) | Newly Identified Engine and Terrain Helpers |
| 228 | [findings/228-entity-pool-terrain-and-timed-effect-helpers.md](docs/findings/228-entity-pool-terrain-and-timed-effect-helpers.md) | Entity-Pool, Terrain, and Timed-Effect Helpers |
| 229 | [findings/229-wwad-archive-and-render-resource-helpers.md](docs/findings/229-wwad-archive-and-render-resource-helpers.md) | WWAD Archive and Render-Resource Helpers |
| 230 | [findings/230-engine-input-selection-and-resource-helpers.md](docs/findings/230-engine-input-selection-and-resource-helpers.md) | Engine Input, Selection, and Resource Helpers |
| 231 | [findings/231-terrain-entity-ui-and-stream-helpers.md](docs/findings/231-terrain-entity-ui-and-stream-helpers.md) | Terrain, Entity, UI, and Stream Helpers |
| 232 | [findings/232-formatted-text-rendering-and-font-helpers.md](docs/findings/232-formatted-text-rendering-and-font-helpers.md) | Formatted Text Rendering and Font Helpers |
| 233 | [findings/233-terrain-rendering-and-stream-primitives.md](docs/findings/233-terrain-rendering-and-stream-primitives.md) | Terrain, Rendering, and Stream Primitives |
| 234 | [findings/234-game-file-streams-and-surface-pixels.md](docs/findings/234-game-file-streams-and-surface-pixels.md) | Game-File Streams and Surface Pixel Operations |
| 235 | [findings/235-camera-projection-lwo-layer-state-and-surface-normals.md](docs/findings/235-camera-projection-lwo-layer-state-and-surface-normals.md) | Camera Projection, LWO Layer State, and Surface Normals |
| 236 | [findings/236-terrain-labels-map-bounds-and-selection-predicate.md](docs/findings/236-terrain-labels-map-bounds-and-selection-predicate.md) | Terrain Labels, Map Bounds, and Selection Predicate |
| 237 | [findings/237-buffered-stream-and-win32-file-helpers.md](docs/findings/237-buffered-stream-and-win32-file-helpers.md) | Buffered Streams and Win32 File Helpers |
| 238 | [findings/238-reward-terrain-and-simulation-helpers.md](docs/findings/238-reward-terrain-and-simulation-helpers.md) | Reward, Terrain, and Simulation Helpers |
| 239 | [findings/239-mesh-segment-and-scan-runtime.md](docs/findings/239-mesh-segment-and-scan-runtime.md) | Mesh Segment Builder and Scan Runtime |
| 240 | [findings/240-cursor-interaction-overlay-menu-and-scene-light-helpers.md](docs/findings/240-cursor-interaction-overlay-menu-and-scene-light-helpers.md) | Cursor Interaction, Overlay, Menu, and Scene-Light Helpers |
| 241 | [findings/241-terrain-quad-run-upgrade-and-scene-slot-helpers.md](docs/findings/241-terrain-quad-run-upgrade-and-scene-slot-helpers.md) | Terrain/Quad-Run, Upgrade-Table, and Scene-Slot Helpers |
| 242 | [findings/242-path-step-terrain-and-pending-record-helpers.md](docs/findings/242-path-step-terrain-and-pending-record-helpers.md) | Path-Step, Terrain, and Pending-Record Helpers |
| 243 | [findings/243-config-loader-file-descriptor-and-crt-helpers.md](docs/findings/243-config-loader-file-descriptor-and-crt-helpers.md) | Config-Loader, File-Descriptor, and CRT Helpers |
| 244 | [findings/244-config-file-text-layout-and-record-emit-helpers.md](docs/findings/244-config-file-text-layout-and-record-emit-helpers.md) | Config-File, Text-Layout, and Record-Emit Helpers |
| 245 | [findings/245-region-route-light-config-and-surface-helpers.md](docs/findings/245-region-route-light-config-and-surface-helpers.md) | Region-Route, Light-Config, and Surface Helpers |
| 246 | [findings/246-entity-move-route-command-and-object-clone-helpers.md](docs/findings/246-entity-move-route-command-and-object-clone-helpers.md) | Entity Move/Route Commands, Cell Queue, and Object Clones |
| 247 | [findings/247-config-graphics-camera-tween-and-pool-spawn-helpers.md](docs/findings/247-config-graphics-camera-tween-and-pool-spawn-helpers.md) | Config/Graphics Quality, Camera Tween/Drag, Cell Search, and Pool/Spawn Helpers |
| 248 | [findings/248-config-band-loaders-display-mode-directional-activity-and-flight-record-helpers.md](docs/findings/248-config-band-loaders-display-mode-directional-activity-and-flight-record-helpers.md) | Config-Band Loaders, Display-Mode Setup, Directional Activity, and Flight-Record Helpers |
| 249 | [findings/249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md](docs/findings/249-config-band-loaders-poly-variant-build-panel-and-cell-registration-layout-and-activity-helpers.md) | Config-Band Loaders, Poly-Variant Build, Panel/Cell Registration, Layout, and Activity Helpers |
| 250 | [findings/250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md](docs/findings/250-config-band-loaders-cell-corner-scan-action-command-record-instantiation-and-render-state-helpers.md) | Config-Band Loaders, Cell-Corner Scan, Action/Command Issues, Record Instantiation, and Render-State Helpers |
| 251 | [findings/251-command-band-kind-dispatch-item-name-lookup-route-and-record-helpers.md](docs/findings/251-command-band-kind-dispatch-item-name-lookup-route-and-record-helpers.md) | Command-Band Kind Dispatch, Item Name Lookup, Route Commands, and Record/Geometry Helpers |
| 252 | [findings/252-config-band-texture-loaders-route-commands-and-surface-menu-helpers.md](docs/findings/252-config-band-texture-loaders-route-commands-and-surface-menu-helpers.md) | Config-Band Texture Loaders, Route Command Issues, and Surface/Menu Helpers |
| 253 | [findings/253-terrain-scatter-erosion-propagation-cell-queries-menu-records-and-buffer-helpers.md](docs/findings/253-terrain-scatter-erosion-propagation-cell-queries-menu-records-and-buffer-helpers.md) | Terrain Scatter, Erosion Propagation, Cell Queries, Menu Records, and Buffer Helpers |
| 254 | [findings/254-command-sweeps-cell-issuing-config-priorities-statistics-and-directdraw-create-helpers.md](docs/findings/254-command-sweeps-cell-issuing-config-priorities-statistics-and-directdraw-create-helpers.md) | Command Sweeps, Cell Issuing, Config Priorities, Statistics, and DirectDraw Create Helpers |
| 255 | [findings/255-visual-channel-relays-resource-tree-callback-walks-scene-pose-and-registry-helpers.md](docs/findings/255-visual-channel-relays-resource-tree-callback-walks-scene-pose-and-registry-helpers.md) | Visual Channel Relays, Resource-Tree Callback Walks, Scene Pose, and Registry Helpers |
| 256 | [findings/256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md](docs/findings/256-command-band-step-issuers-flag-behavior-timers-and-steering-helpers.md) | Command-Band Step Issuers, Flag-Behavior Timers, and Steering Helpers |
| 257 | [findings/257-configured-tiny-monster-spawning-and-mutual-activity-pairing.md](docs/findings/257-configured-tiny-monster-spawning-and-mutual-activity-pairing.md) | Configured Tiny-Monster Spawning and Mutual Activity Pairing |
| 258 | [findings/258-interface-method-wrappers-overlay-refresh-and-deferred-set-relays.md](docs/findings/258-interface-method-wrappers-overlay-refresh-and-deferred-set-relays.md) | Interface-Method Wrappers, Overlay Refresh, and Deferred-Set Relays |
| 259 | [findings/259-selected-entity-detach-callback-and-pair-record-commit.md](docs/findings/259-selected-entity-detach-callback-and-pair-record-commit.md) | Selected-Entity Detach Callback and Pair-Record Commit |
| 260 | [findings/260-crt-startup-locale-time-and-floating-point-helpers.md](docs/findings/260-crt-startup-locale-time-and-floating-point-helpers.md) | CRT Startup, Locale, Time-Zone, and Floating-Point Helpers |
| 261 | [findings/261-crt-argument-environment-and-error-helpers.md](docs/findings/261-crt-argument-environment-and-error-helpers.md) | CRT Argument, Environment, Error, and Conversion Helpers |
| 262 | [findings/262-scroll-panel-and-viewport-picking-helpers.md](docs/findings/262-scroll-panel-and-viewport-picking-helpers.md) | Scroll Panel and Viewport Picking Helpers |
| 263 | [findings/263-entity-effect-fields-and-nested-record-teardown.md](docs/findings/263-entity-effect-fields-and-nested-record-teardown.md) | Entity Effect Fields and Nested Record Teardown |
| 264 | [findings/264-crt-exception-and-extended-float-helpers.md](docs/findings/264-crt-exception-and-extended-float-helpers.md) | CRT Exception and Extended-Float Helpers |
| 265 | [findings/265-token-directory-scan-and-mesh-coordinate-helpers.md](docs/findings/265-token-directory-scan-and-mesh-coordinate-helpers.md) | Token Directory Scan and Mesh Coordinate Helpers |
| 266 | [findings/266-terrain-triangle-select-and-command-gate.md](docs/findings/266-terrain-triangle-select-and-command-gate.md) | Terrain Triangle Selection and Command Gate |
| 267 | [findings/267-entity-route-commit-and-terrain-activity-gate.md](docs/findings/267-entity-route-commit-and-terrain-activity-gate.md) | Entity Route Commit and Terrain Activity Gate |
| 268 | [findings/268-crt-conversion-scene-hierarchy-and-math-helpers.md](docs/findings/268-crt-conversion-scene-hierarchy-and-math-helpers.md) | CRT Conversion, Scene Hierarchy, and Math Helpers |
| 269 | [findings/269-crt-environment-and-multibyte-string-helpers.md](docs/findings/269-crt-environment-and-multibyte-string-helpers.md) | CRT Environment and Multibyte String Helpers |
| 270 | [findings/270-entity-route-markers-and-tickdown-spawn.md](docs/findings/270-entity-route-markers-and-tickdown-spawn.md) | Entity Route Markers and TickDown Spawn |
| 271 | [findings/271-panel-layout-and-advisor-render-helpers.md](docs/findings/271-panel-layout-and-advisor-render-helpers.md) | Panel Layout and Advisor Render Helpers |
| 272 | [findings/272-crt-file-resize-and-signal-helpers.md](docs/findings/272-crt-file-resize-and-signal-helpers.md) | CRT File Resize and Signal Helpers |
| 273 | [findings/273-terrain-object-sound-grid-and-activity-transition.md](docs/findings/273-terrain-object-sound-grid-and-activity-transition.md) | Terrain Object Sound Grid and Activity Transition |
| 274 | [findings/274-priority-image-selection-and-scene-registry-teardown.md](docs/findings/274-priority-image-selection-and-scene-registry-teardown.md) | Priority Image Selection and Scene Registry Teardown |
| 275 | [findings/275-scene-frame-record-clone-or-create.md](docs/findings/275-scene-frame-record-clone-or-create.md) | SceneAct Frame Record Clone or Create |
| 276 | [findings/276-additional-engine-ui-and-scene-helper-names.md](docs/findings/276-additional-engine-ui-and-scene-helper-names.md) | Additional Engine, UI, and Scene Helper Names |
| 277 | [findings/277-low-address-helper-names.md](docs/findings/277-low-address-helper-names.md) | Low-Address Helper Names |
| 278 | [findings/278-simulation-and-terrain-helper-names.md](docs/findings/278-simulation-and-terrain-helper-names.md) | Simulation and Terrain Helper Names |
| 279 | [findings/279-crt-and-display-helper-names.md](docs/findings/279-crt-and-display-helper-names.md) | CRT and Display Helper Names |
| 280 | [findings/280-vector-math-helper-names.md](docs/findings/280-vector-math-helper-names.md) | Vector Math Helper Names |
| 281 | [findings/281-advisor-terrain-task-helper-names.md](docs/findings/281-advisor-terrain-task-helper-names.md) | Advisor, Terrain, and Task Helper Names |
| 282 | [findings/282-display-resolution-helper-names.md](docs/findings/282-display-resolution-helper-names.md) | Display Resolution Helper Names |
| 283 | [findings/283-low-address-resource-and-geometry-helpers.md](docs/findings/283-low-address-resource-and-geometry-helpers.md) | Low-Address Resource and Geometry Helpers |
| 284 | [findings/284-movement-lighting-and-action-gates.md](docs/findings/284-movement-lighting-and-action-gates.md) | Movement, Lighting, and Action Gates |
| 285 | [findings/285-render-collision-fog-and-segment-helpers.md](docs/findings/285-render-collision-fog-and-segment-helpers.md) | Render, Collision, Fog, and Segment Helpers |
| 286 | [findings/286-task-registry-and-category-mask-helpers.md](docs/findings/286-task-registry-and-category-mask-helpers.md) | Task Registry and Category-Mask Helpers |
| 287 | [findings/287-terrain-cell-queued-work-transition.md](docs/findings/287-terrain-cell-queued-work-transition.md) | Terrain Cell Queued-Work Transition |
| 288 | [findings/288-render-object-and-mesh-helpers.md](docs/findings/288-render-object-and-mesh-helpers.md) | Render Object and Mesh Helpers |
| 289 | [findings/289-statistics-and-adjacent-cell-helpers.md](docs/findings/289-statistics-and-adjacent-cell-helpers.md) | Statistics and Adjacent-Cell Helpers |
| 290 | [findings/290-entity-velocity-state-accessors.md](docs/findings/290-entity-velocity-state-accessors.md) | Entity Velocity State Accessors |
| 291 | [findings/291-tracked-entity-pointer-array-removal.md](docs/findings/291-tracked-entity-pointer-array-removal.md) | Tracked Entity Pointer Array Removal |
| 292 | [findings/292-selection-and-pointer-registry-helpers.md](docs/findings/292-selection-and-pointer-registry-helpers.md) | Selection and Pointer-Registry Helpers |
| 293 | [findings/293-terrain-cell-predicates-and-spatial-lookup.md](docs/findings/293-terrain-cell-predicates-and-spatial-lookup.md) | Terrain Cell Predicates and Spatial Lookup |
| 294 | [findings/294-render-segment-and-object-state-helpers.md](docs/findings/294-render-segment-and-object-state-helpers.md) | Render Segment and Object-State Helpers |
| 295 | [findings/295-engine-mode-and-state-accessors.md](docs/findings/295-engine-mode-and-state-accessors.md) | Engine Mode and State Accessors |
| 296 | [findings/296-entity-effect-counter-record-application.md](docs/findings/296-entity-effect-counter-record-application.md) | Entity Effect-Counter Record Application |
| 297 | [findings/297-activity-record-and-message-table-helpers.md](docs/findings/297-activity-record-and-message-table-helpers.md) | Activity-Record and Message-Table Helpers |
| 298 | [findings/298-render-object-transform-and-string-helpers.md](docs/findings/298-render-object-transform-and-string-helpers.md) | Render-Object Transform and String Helpers |
| 299 | [findings/299-low-address-task-interface-helpers.md](docs/findings/299-low-address-task-interface-helpers.md) | Low-Address Task, Interface, and Grid Helpers |
| 300 | [findings/300-terrain-queue-and-particle-slot-helpers.md](docs/findings/300-terrain-queue-and-particle-slot-helpers.md) | Terrain-Queue and Particle-Slot Helpers |
| 301 | [findings/301-scene-geometry-and-resource-slot-helpers.md](docs/findings/301-scene-geometry-and-resource-slot-helpers.md) | Scene Geometry and Resource-Slot Helpers |
| 302 | [findings/302-crt-character-and-arithmetic-helpers.md](docs/findings/302-crt-character-and-arithmetic-helpers.md) | CRT Character and Arithmetic Helpers |
| 303 | [findings/303-bubble-action-and-string-cache-lookups.md](docs/findings/303-bubble-action-and-string-cache-lookups.md) | Bubble-Action and String-Cache Lookups |
| 304 | [findings/304-terrain-visited-cell-tracking.md](docs/findings/304-terrain-visited-cell-tracking.md) | Terrain Visited-Cell Tracking |
| 305 | [findings/305-entity-velocity-and-scene-position-accessors.md](docs/findings/305-entity-velocity-and-scene-position-accessors.md) | Entity Velocity and Scene-Position Accessors |
| 306 | [findings/306-string-cache-and-interface-message-loaders.md](docs/findings/306-string-cache-and-interface-message-loaders.md) | String-Cache and Interface-Message Loaders |
| 307 | [findings/307-terrain-cell-flag-operations-and-refresh.md](docs/findings/307-terrain-cell-flag-operations-and-refresh.md) | Terrain-Cell Flag Operations and Refresh |
| 308 | [findings/308-effect-mode-and-slot-helpers.md](docs/findings/308-effect-mode-and-slot-helpers.md) | Effect-Mode and Slot Helpers |
| 309 | [findings/309-geometry-and-directdraw-surface-helpers.md](docs/findings/309-geometry-and-directdraw-surface-helpers.md) | Geometry and DirectDraw Surface Helpers |
| 310 | [findings/310-terrain-cell-flag-and-state-tests.md](docs/findings/310-terrain-cell-flag-and-state-tests.md) | Terrain-Cell Flag and State Tests |
| 311 | [findings/311-renderer-fog-and-mesh-color-setters.md](docs/findings/311-renderer-fog-and-mesh-color-setters.md) | Renderer Fog and Mesh Color Setters |
| 312 | [findings/312-object-task-and-registry-helpers.md](docs/findings/312-object-task-and-registry-helpers.md) | Object Task and Registry Helpers |
| 313 | [findings/313-selection-resource-and-transform-helpers.md](docs/findings/313-selection-resource-and-transform-helpers.md) | Selection, Resource, and Transform Helpers |
| 314 | [findings/314-terrain-grid-record-and-flag-helpers.md](docs/findings/314-terrain-grid-record-and-flag-helpers.md) | Terrain Grid, Record, and Flag Helpers |
| 315 | [findings/315-pooled-record-and-display-mode-helpers.md](docs/findings/315-pooled-record-and-display-mode-helpers.md) | Pooled-Record and Display-Mode Helpers |
| 316 | [findings/316-entity-flag-and-fixed-record-pool-helpers.md](docs/findings/316-entity-flag-and-fixed-record-pool-helpers.md) | Entity-Flag and Fixed-Record-Pool Helpers |
| 317 | [findings/317-air-supply-amount-update.md](docs/findings/317-air-supply-amount-update.md) | Air-Supply Amount Update |
| 318 | [findings/318-entity-eligibility-and-dock-helpers.md](docs/findings/318-entity-eligibility-and-dock-helpers.md) | Entity Eligibility and Dock Helpers |
| 319 | [findings/319-entity-stats-and-mask-predicates.md](docs/findings/319-entity-stats-and-mask-predicates.md) | Entity-Stats and Mask Predicates |
| 320 | [findings/320-crt-descriptor-and-extended-float-helpers.md](docs/findings/320-crt-descriptor-and-extended-float-helpers.md) | CRT Descriptor and Extended-Float Helpers |
| 321 | [findings/321-transform-collection-and-surface-config-helpers.md](docs/findings/321-transform-collection-and-surface-config-helpers.md) | Transform-Collection and Surface-Config Helpers |
| 322 | [findings/322-resource-binding-and-map-row-span-helpers.md](docs/findings/322-resource-binding-and-map-row-span-helpers.md) | Resource-Binding and Map-Row-Span Helpers |
| 323 | [findings/323-camera-math-and-matrix-primitives.md](docs/findings/323-camera-math-and-matrix-primitives.md) | Camera Math and Matrix Primitives |
| 324 | [findings/324-global-state-and-record-accessors.md](docs/findings/324-global-state-and-record-accessors.md) | Global-State and Record Accessors |
| 325 | [findings/325-terrain-cell-refresh-and-grid-pick.md](docs/findings/325-terrain-cell-refresh-and-grid-pick.md) | Terrain-Cell Refresh and Grid Pick |
| 326 | [findings/326-render-config-and-grid-record-tables.md](docs/findings/326-render-config-and-grid-record-tables.md) | Render-Config and Grid-Record Tables |
| 327 | [findings/327-entity-threshold-and-flag-mask-predicates.md](docs/findings/327-entity-threshold-and-flag-mask-predicates.md) | Entity Threshold and Flag-Mask Predicates |
| 328 | [findings/328-crt-environment-and-descriptor-helpers.md](docs/findings/328-crt-environment-and-descriptor-helpers.md) | CRT Environment and Descriptor Helpers |
| 329 | [findings/329-viewport-and-global-refresh-helpers.md](docs/findings/329-viewport-and-global-refresh-helpers.md) | Viewport and Global-Refresh Helpers |
| 330 | [findings/330-camera-bound-and-transform-setters.md](docs/findings/330-camera-bound-and-transform-setters.md) | Camera-Bound and Transform Setters |
| 331 | [findings/331-weapon-type-record-accessors.md](docs/findings/331-weapon-type-record-accessors.md) | Weapon-Type Record Accessors |
| 332 | [findings/332-interpolated-geometry-and-entity-record-helpers.md](docs/findings/332-interpolated-geometry-and-entity-record-helpers.md) | Interpolated Geometry and Entity-Record Helpers |
| 333 | [findings/333-advisor-name-and-vector-math-helpers.md](docs/findings/333-advisor-name-and-vector-math-helpers.md) | Advisor-Name and Vector-Math Helpers |
| 334 | [findings/334-camera-bounds-and-global-refresh.md](docs/findings/334-camera-bounds-and-global-refresh.md) | Camera Bounds and Global Refresh |
| 335 | [findings/335-expansion-grid-and-timed-visual-helpers.md](docs/findings/335-expansion-grid-and-timed-visual-helpers.md) | Expansion-Grid and Timed-Visual Helpers |
| 336 | [findings/336-hunger-meter-images-and-entity-override-pop.md](docs/findings/336-hunger-meter-images-and-entity-override-pop.md) | Hunger-Meter Images and Entity Override Pop |
| 337 | [findings/337-scene-object-transform-and-pointer-wrappers.md](docs/findings/337-scene-object-transform-and-pointer-wrappers.md) | Scene-Object Transform and Pointer Wrappers |
| 338 | [findings/338-current-level-state-and-resource-helpers.md](docs/findings/338-current-level-state-and-resource-helpers.md) | Current-Level State and Resource Helpers |
| 339 | [findings/339-camera-distance-and-position-helpers.md](docs/findings/339-camera-distance-and-position-helpers.md) | Camera Distance and Position Helpers |
| 340 | [findings/340-entity-display-and-tool-name-resources.md](docs/findings/340-entity-display-and-tool-name-resources.md) | Entity Display and Tool-Name Resources |
| 341 | [findings/341-animation-and-input-state-helpers.md](docs/findings/341-animation-and-input-state-helpers.md) | Animation and Input-State Helpers |
| 342 | [findings/342-crt-descriptor-and-bounded-string-helpers.md](docs/findings/342-crt-descriptor-and-bounded-string-helpers.md) | CRT Descriptor and Bounded-String Helpers |
| 343 | [findings/343-advisor-action-and-animation-record-helpers.md](docs/findings/343-advisor-action-and-animation-record-helpers.md) | Advisor Action and Animation Record Helpers |
| 344 | [findings/344-indexed-surface-and-pair-copy-helpers.md](docs/findings/344-indexed-surface-and-pair-copy-helpers.md) | Indexed-Surface and Pair-Copy Helpers |
| 345 | [findings/345-sound-object-duration-query.md](docs/findings/345-sound-object-duration-query.md) | Sound-Object Duration Query |
| 346 | [findings/346-screen-surface-allocation-and-clip-distance.md](docs/findings/346-screen-surface-allocation-and-clip-distance.md) | Screen-Surface Allocation and Clip Distance |
| 347 | [findings/347-display-priority-and-allocator-wrappers.md](docs/findings/347-display-priority-and-allocator-wrappers.md) | Display, Priority, and Allocator Wrappers |
| 348 | [findings/348-entity-and-indexed-record-accessors.md](docs/findings/348-entity-and-indexed-record-accessors.md) | Entity and Indexed-Record Accessors |
| 349 | [findings/349-scene-property-and-render-resource-helpers.md](docs/findings/349-scene-property-and-render-resource-helpers.md) | Scene-Property and Render-Resource Helpers |
| 350 | [findings/350-low-address-string-and-global-field-helpers.md](docs/findings/350-low-address-string-and-global-field-helpers.md) | Low-Address String and Global-Field Helpers |
| 351 | [findings/351-text-output-task-table-and-indexed-record-helpers.md](docs/findings/351-text-output-task-table-and-indexed-record-helpers.md) | Text Output, Task Table, and Indexed Record Helpers |
| 352 | [findings/352-scene-interface-pair-and-property-value-helpers.md](docs/findings/352-scene-interface-pair-and-property-value-helpers.md) | Scene Interface Pair and Property Value Helpers |
| 353 | [findings/353-cell-commands-and-entity-activity-helpers.md](docs/findings/353-cell-commands-and-entity-activity-helpers.md) | Cell Commands and Entity Activity Helpers |
| 354 | [findings/354-record-vector-and-resource-state-helpers.md](docs/findings/354-record-vector-and-resource-state-helpers.md) | Record, Vector, and Resource State Helpers |
| 355 | [findings/355-bit-pointer-surface-and-window-wrappers.md](docs/findings/355-bit-pointer-surface-and-window-wrappers.md) | Bit, Pointer, Surface, and Window Wrappers |
| 356 | [findings/356-animation-and-low-address-state-helpers.md](docs/findings/356-animation-and-low-address-state-helpers.md) | Animation and Low-Address State Helpers |
| 357 | [findings/357-entity-fields-and-global-counter-helpers.md](docs/findings/357-entity-fields-and-global-counter-helpers.md) | Entity Fields and Global Counter Helpers |
| 358 | [findings/358-matrix-window-surface-and-heap-helpers.md](docs/findings/358-matrix-window-surface-and-heap-helpers.md) | Matrix, Window, Surface, and Heap Helpers |
| 359 | [findings/359-entity-command-records-and-neighbor-dispatch.md](docs/findings/359-entity-command-records-and-neighbor-dispatch.md) | Entity Command Records and Neighbor Dispatch |
| 360 | [findings/360-grid-animation-and-resource-cleanup-helpers.md](docs/findings/360-grid-animation-and-resource-cleanup-helpers.md) | Grid Animation and Resource Cleanup Helpers |
| 361 | [findings/361-sound-window-and-resource-interface-wrappers.md](docs/findings/361-sound-window-and-resource-interface-wrappers.md) | Sound, Window, and Resource Interface Wrappers |
| 362 | [findings/362-callback-event-and-attached-record-helpers.md](docs/findings/362-callback-event-and-attached-record-helpers.md) | Callback, Event, and Attached-Record Helpers |
| 363 | [findings/363-geometry-string-lookup-and-entity-state-helpers.md](docs/findings/363-geometry-string-lookup-and-entity-state-helpers.md) | Geometry, String Lookup, and Entity State Helpers |
| 364 | [findings/364-bit-count-cursor-and-directory-helpers.md](docs/findings/364-bit-count-cursor-and-directory-helpers.md) | Bit Count, Cursor, and Directory Helpers |
| 365 | [findings/365-renderer-record-and-cached-handle-helpers.md](docs/findings/365-renderer-record-and-cached-handle-helpers.md) | Renderer Record and Cached-Handle Helpers |
| 366 | [findings/366-task-records-callback-dispatch-and-pointer-stacks.md](docs/findings/366-task-records-callback-dispatch-and-pointer-stacks.md) | Task Records, Callback Dispatch, and Pointer Stacks |
| 367 | [findings/367-scene-resources-entity-predicates-and-offset-accessors.md](docs/findings/367-scene-resources-entity-predicates-and-offset-accessors.md) | Scene Resources, Entity Predicates, and Offset Accessors |
| 368 | [findings/368-record-pool-and-recursive-list-helpers.md](docs/findings/368-record-pool-and-recursive-list-helpers.md) | Record Pool and Recursive List Helpers |
| 369 | [findings/369-task-queue-and-entity-state-helpers.md](docs/findings/369-task-queue-and-entity-state-helpers.md) | Task Queue and Entity State Helpers |
| 370 | [findings/370-screen-effects-surface-and-ui-helpers.md](docs/findings/370-screen-effects-surface-and-ui-helpers.md) | Screen Effects, Surface, and UI Helpers |
| 371 | [findings/371-record-pools-pair-tables-and-buffered-streams.md](docs/findings/371-record-pools-pair-tables-and-buffered-streams.md) | Record Pools, Pair Tables, and Buffered Streams |
| 372 | [findings/372-incident-messages-options-and-resource-helpers.md](docs/findings/372-incident-messages-options-and-resource-helpers.md) | Incident Messages, Options, and Resource Helpers |
| 373 | [findings/373-cursor-simulation-renderer-and-file-helpers.md](docs/findings/373-cursor-simulation-renderer-and-file-helpers.md) | Cursor, Simulation, Renderer, and File Helpers |
| 374 | [findings/374-terrain-interface-and-media-helpers.md](docs/findings/374-terrain-interface-and-media-helpers.md) | Terrain, Interface, and Media Helpers |
| 375 | [findings/375-entity-ui-and-render-tree-helpers.md](docs/findings/375-entity-ui-and-render-tree-helpers.md) | Entity, UI, and Render Tree Helpers |
| 376 | [findings/376-selection-cell-and-overlay-state-helpers.md](docs/findings/376-selection-cell-and-overlay-state-helpers.md) | Selection, Cell, and Overlay State Helpers |
| 377 | [findings/377-callback-registry-and-runtime-helpers.md](docs/findings/377-callback-registry-and-runtime-helpers.md) | Callback, Registry, and Runtime Helpers |
| 378 | [findings/378-input-selection-and-action-state-helpers.md](docs/findings/378-input-selection-and-action-state-helpers.md) | Input Selection and Action-State Helpers |
| 379 | [findings/379-terrain-ui-and-crt-support-helpers.md](docs/findings/379-terrain-ui-and-crt-support-helpers.md) | Terrain, UI, and CRT Support Helpers |
| 380 | [findings/380-scene-objects-terrain-and-matrix-helpers.md](docs/findings/380-scene-objects-terrain-and-matrix-helpers.md) | Scene Objects, Terrain, and Matrix Helpers |
| 381 | [findings/381-selection-resource-runtime-and-grid-helpers.md](docs/findings/381-selection-resource-runtime-and-grid-helpers.md) | Selection, Resource, Runtime, and Grid Helpers |
| 382 | [findings/382-crt-extended-precision-and-entity-routing.md](docs/findings/382-crt-extended-precision-and-entity-routing.md) | CRT, Extended Precision, and Entity Routing |
| 383 | [findings/383-crt-global-accessors-and-light-effects.md](docs/findings/383-crt-global-accessors-and-light-effects.md) | CRT, Global Accessors, and Light Effects |
| 384 | [findings/384-crt-formatting-com-cleanup-and-spawn-helpers.md](docs/findings/384-crt-formatting-com-cleanup-and-spawn-helpers.md) | CRT Formatting, COM Cleanup, and Spawn Helpers |
| 385 | [findings/385-stream-surface-avi-and-route-state-helpers.md](docs/findings/385-stream-surface-avi-and-route-state-helpers.md) | Stream, Surface, AVI, and Route State Helpers |
| 386 | [findings/386-spatial-ui-com-and-crt-runtime.md](docs/findings/386-spatial-ui-com-and-crt-runtime.md) | Spatial, UI, COM, and CRT Runtime |
| 387 | [findings/387-record-and-resource-pool-lifecycles.md](docs/findings/387-record-and-resource-pool-lifecycles.md) | Record and Resource Pool Lifecycles |
| 388 | [findings/388-geometry-and-matrix-primitives.md](docs/findings/388-geometry-and-matrix-primitives.md) | Geometry, Projection, and Random Primitives |
| 389 | [findings/389-linked-records-and-state-helpers.md](docs/findings/389-linked-records-and-state-helpers.md) | Linked Records and State Helpers |
| 390 | [findings/390-motion-config-and-resource-utilities.md](docs/findings/390-motion-config-and-resource-utilities.md) | Motion, Configuration, and Resource Utilities |
| 391 | [findings/391-vector-geometry-and-pointer-state-primitives.md](docs/findings/391-vector-geometry-and-pointer-state-primitives.md) | Vector, Geometry, and Pointer-State Primitives |
| 392 | [findings/392-global-state-and-configuration-accessors.md](docs/findings/392-global-state-and-configuration-accessors.md) | Global State and Configuration Accessors |
| 393 | [findings/393-string-list-and-small-record-utilities.md](docs/findings/393-string-list-and-small-record-utilities.md) | String, List, and Small Record Utilities |
| 394 | [findings/394-runtime-engine-memory-management.md](docs/findings/394-runtime-engine-memory-management.md) | Runtime and Engine Memory Management |
| 395 | [findings/395-crt-and-os-file-stream-io.md](docs/findings/395-crt-and-os-file-stream-io.md) | CRT and OS File/Stream I/O |
| 396 | [findings/396-crt-process-startup-termination-and-exceptions.md](docs/findings/396-crt-process-startup-termination-and-exceptions.md) | CRT Process Startup, Termination, and Exceptions |
| 397 | [findings/397-crt-strings-locale-and-formatting.md](docs/findings/397-crt-strings-locale-and-formatting.md) | CRT Strings, Locale, and Formatting |
| 398 | [findings/398-application-startup-window-loop.md](docs/findings/398-application-startup-window-loop.md) | Application Startup, Window Loop, and Preferences |
| 399 | [findings/399-crt-numeric-and-floating-point-runtime.md](docs/findings/399-crt-numeric-and-floating-point-runtime.md) | CRT Numeric and Floating-Point Runtime |
| 400 | [findings/400-compiler-runtime-no-op-stubs.md](docs/findings/400-compiler-runtime-no-op-stubs.md) | Compiler/Runtime No-op Stubs |
| 401 | [findings/401-task-callback-record-allocator.md](docs/findings/401-task-callback-record-allocator.md) | Task Callback Record Allocator |
| 402 | [findings/402-registry-and-startup-streams.md](docs/findings/402-registry-and-startup-streams.md) | Registry and Startup Streams |
| 403 | [findings/403-config-and-text-parsing-helpers.md](docs/findings/403-config-and-text-parsing-helpers.md) | Configuration and Text Parsing Helpers |
| 404 | [findings/404-game-file-archive-and-pools.md](docs/findings/404-game-file-archive-and-pools.md) | Game Files, Archives, and Record Pools |
| 405 | [findings/405-level-map-loading-pipeline.md](docs/findings/405-level-map-loading-pipeline.md) | Level and Map Loading Pipeline |
| 406 | [findings/406-audio-playback-mixing-and-spatial-sound.md](docs/findings/406-audio-playback-mixing-and-spatial-sound.md) | Audio Playback, Mixing, and Spatial Sound |
| 407 | [findings/407-system-input-and-osf-state.md](docs/findings/407-system-input-and-osf-state.md) | System Input and OSF State |
| 408 | [findings/408-save-slot-context-and-level-record-lifecycle.md](docs/findings/408-save-slot-context-and-level-record-lifecycle.md) | Save Slot Context and Level Record Lifecycle |
| 409 | [findings/409-game-configuration-entity-type-and-object-name-tables.md](docs/findings/409-game-configuration-entity-type-and-object-name-tables.md) | Game Configuration Entity Type and Object-Name Tables |
| 410 | [findings/410-map-and-visual-resource-records.md](docs/findings/410-map-and-visual-resource-records.md) | Map and Visual Resource Records |
| 411 | [findings/411-interface-records-and-directx-diagnostics.md](docs/findings/411-interface-records-and-directx-diagnostics.md) | Interface Records and DirectX Diagnostics |
| 412 | [findings/412-platform-import-thunks.md](docs/findings/412-platform-import-thunks.md) | Platform Import Thunks |
| 413 | [findings/413-configured-directshow-video-path.md](docs/findings/413-configured-directshow-video-path.md) | Configured DirectShow Video Path |
| 414 | [findings/414-avi-and-flic-playback-routes.md](docs/findings/414-avi-and-flic-playback-routes.md) | AVI and FLIC Playback Routes |
| 415 | [findings/415-model-lightwave-and-animation-asset-pipeline.md](docs/findings/415-model-lightwave-and-animation-asset-pipeline.md) | Model, LightWave, and Animation Asset Pipeline |
| 416 | [findings/416-scene-and-render-resource-lifecycle.md](docs/findings/416-scene-and-render-resource-lifecycle.md) | Scene and Render Resource Lifecycle |
| 417 | [findings/417-sound-subsystem-aliases-queues-and-effects.md](docs/findings/417-sound-subsystem-aliases-queues-and-effects.md) | Sound aliases, submission queues, and transient effects |
| 418 | [findings/418-movie-stream-and-audio-playback.md](docs/findings/418-movie-stream-and-audio-playback.md) | Movie, Stream, and Audio Playback Boundaries |
| 419 | [findings/419-interface-and-presentation-config-loading.md](docs/findings/419-interface-and-presentation-config-loading.md) | Interface and Presentation Config Loading |
| 420 | [findings/420-renderer-and-scene-lifecycle.md](docs/findings/420-renderer-and-scene-lifecycle.md) | Renderer and Scene Lifecycle |
| 421 | [findings/421-ui-render-tree-traversal.md](docs/findings/421-ui-render-tree-traversal.md) | UI Render-Tree Traversal |
| 422 | [findings/422-viewport-picking-and-terrain-rendering.md](docs/findings/422-viewport-picking-and-terrain-rendering.md) | Viewport Picking and Terrain Rendering |
| 423 | [findings/423-input-keyboard-and-cursor-state.md](docs/findings/423-input-keyboard-and-cursor-state.md) | Input, Keyboard, and Cursor State |
| 424 | [findings/424-camera-viewport-and-view-context.md](docs/findings/424-camera-viewport-and-view-context.md) | Camera, Viewport, and View-Context State |
| 425 | [findings/425-display-mode-selection-state-and-controls.md](docs/findings/425-display-mode-selection-state-and-controls.md) | Display-Mode Selection State and Controls |
| 426 | [findings/426-camera-selection-point-and-pointer-pan.md](docs/findings/426-camera-selection-point-and-pointer-pan.md) | Camera Selection Point and Pointer Pan |
| 427 | [findings/427-screen-effects-and-ui-text-paths.md](docs/findings/427-screen-effects-and-ui-text-paths.md) | Screen Effects and UI Text Paths |
| 428 | [findings/428-font-and-text-buffer-rendering.md](docs/findings/428-font-and-text-buffer-rendering.md) | Font and Text-Buffer Rendering |
| 429 | [findings/429-advisor-and-message-ui-state.md](docs/findings/429-advisor-and-message-ui-state.md) | Advisor and Message UI State |
| 430 | [findings/430-scene-object-action-and-animation-lifecycle.md](docs/findings/430-scene-object-action-and-animation-lifecycle.md) | Scene Object, Action, and Animation Lifecycle |
| 431 | [findings/431-graphics-rendering-and-surface-paths.md](docs/findings/431-graphics-rendering-and-surface-paths.md) | Graphics, Rendering, and Pixel Surface Paths |
| 432 | [findings/432-scene-resource-animation-state-flow.md](docs/findings/432-scene-resource-animation-state-flow.md) | Scene Resource and Animation State Flow |
| 433 | [findings/433-scene-objects-effects-motion-and-light-state.md](docs/findings/433-scene-objects-effects-motion-and-light-state.md) | Scene Objects, Effects, Motion, and Light State |
| 434 | [findings/434-scene-resources-lights-markers-and-transient-effects.md](docs/findings/434-scene-resources-lights-markers-and-transient-effects.md) | Scene Resources, Lights, Markers, and Transient Effects |
| 435 | [findings/435-scene-objects-animation-and-render-records.md](docs/findings/435-scene-objects-animation-and-render-records.md) | Scene Objects, Animation, and Render Records |
| 436 | [findings/436-simulation-ui-and-player-controls.md](docs/findings/436-simulation-ui-and-player-controls.md) | Simulation UI and Player Controls |
| 437 | [findings/437-menu-widgets-and-menu-rendering.md](docs/findings/437-menu-widgets-and-menu-rendering.md) | Menu Widgets and Menu Rendering |
| 438 | [findings/438-front-end-flow-save-slot-and-option-state.md](docs/findings/438-front-end-flow-save-slot-and-option-state.md) | Front-End Flow, Save Slots, and Option State |
| 439 | [findings/439-hud-panel-render-and-interaction-state.md](docs/findings/439-hud-panel-render-and-interaction-state.md) | HUD Panel Rendering and Interaction State |
| 440 | [findings/440-tutorial-and-advisor-control.md](docs/findings/440-tutorial-and-advisor-control.md) | Tutorial and Advisor Control |
| 441 | [findings/441-incident-and-information-message-queue.md](docs/findings/441-incident-and-information-message-queue.md) | Incident and Information Message Queue |
| 442 | [findings/442-advisor-scene-and-action-state.md](docs/findings/442-advisor-scene-and-action-state.md) | Advisor Scene and Action State |
| 443 | [findings/443-help-tutorial-and-interface-resources.md](docs/findings/443-help-tutorial-and-interface-resources.md) | Help, Tutorial, and Interface Resources |
| 444 | [findings/444-input-selection-pointer-actions.md](docs/findings/444-input-selection-pointer-actions.md) | Input, Selection, and Pointer Actions |
| 445 | [findings/445-terrain-scene-geometry.md](docs/findings/445-terrain-scene-geometry.md) | Terrain Scene Geometry |
| 446 | [findings/446-terrain-and-overlay-rendering.md](docs/findings/446-terrain-and-overlay-rendering.md) | Terrain and Overlay Rendering |
| 447 | [findings/447-terrain-grid-path-search.md](docs/findings/447-terrain-grid-path-search.md) | Terrain Grid Path Search |
| 448 | [findings/448-routing-and-activity-transitions.md](docs/findings/448-routing-and-activity-transitions.md) | Routing and Activity Transitions |
| 449 | [findings/449-spatial-queries-and-candidate-search.md](docs/findings/449-spatial-queries-and-candidate-search.md) | Spatial Queries and Candidate Search |
| 450 | [findings/450-terrain-data-and-grid-operations.md](docs/findings/450-terrain-data-and-grid-operations.md) | Terrain Data and Grid Operations |
| 451 | [findings/451-terrain-cell-state-neighbors-occupancy-and-traversal.md](docs/findings/451-terrain-cell-state-neighbors-occupancy-and-traversal.md) | Terrain Cell State, Neighbors, Occupancy, and Traversal |
| 452 | [findings/452-lava-erosion-timing-and-neighbor-propagation.md](docs/findings/452-lava-erosion-timing-and-neighbor-propagation.md) | Lava Erosion Timing and Neighbor Propagation |
| 453 | [findings/453-object-search-and-placement-eligibility.md](docs/findings/453-object-search-and-placement-eligibility.md) | Object Search and Placement Eligibility |
| 454 | [findings/454-terrain-grid-and-cell-resource-tables.md](docs/findings/454-terrain-grid-and-cell-resource-tables.md) | Terrain Grid and Cell Resource Tables |
| 455 | [findings/455-terrain-command-feedback-and-cell-interaction.md](docs/findings/455-terrain-command-feedback-and-cell-interaction.md) | Terrain Command Feedback and Cell Interaction |
| 456 | [findings/456-world-entity-creation-and-pointer-registries.md](docs/findings/456-world-entity-creation-and-pointer-registries.md) | World Entity Creation and Pointer Registries |
| 457 | [findings/457-entity-stats-configuration-and-record-accessors.md](docs/findings/457-entity-stats-configuration-and-record-accessors.md) | Entity Stats Configuration and Record Accessors |
| 458 | [findings/458-combat-effects-projectiles.md](docs/findings/458-combat-effects-projectiles.md) | Combat Effects and Projectiles |
| 459 | [findings/459-timed-effects-and-cues.md](docs/findings/459-timed-effects-and-cues.md) | Timed Effects and Cues |
| 460 | [findings/460-velocity-and-collision-queries.md](docs/findings/460-velocity-and-collision-queries.md) | Velocity and Collision Queries |
| 461 | [findings/461-weapon-definitions-and-indexed-lookups.md](docs/findings/461-weapon-definitions-and-indexed-lookups.md) | Weapon Definitions and Indexed Lookups |
| 462 | [findings/462-objective-configuration-and-runtime-results.md](docs/findings/462-objective-configuration-and-runtime-results.md) | Objective Configuration and Runtime Results |
| 463 | [findings/463-particles-and-grid-expansion.md](docs/findings/463-particles-and-grid-expansion.md) | Particles and Grid Expansion |
| 464 | [findings/464-entity-lifecycle-and-pools.md](docs/findings/464-entity-lifecycle-and-pools.md) | Entity Lifecycle and Pools |
| 465 | [findings/465-task-scheduler-record-lifecycle.md](docs/findings/465-task-scheduler-record-lifecycle.md) | Task Scheduler Record Lifecycle |
| 466 | [findings/466-command-registry-global-queue-transitions.md](docs/findings/466-command-registry-global-queue-transitions.md) | Command Registry and Global Queue Transitions |
| 467 | [findings/467-command-creation-and-issue-routing.md](docs/findings/467-command-creation-and-issue-routing.md) | Command Creation and Issue Routing |
| 468 | [findings/468-command-queue-and-selection-state.md](docs/findings/468-command-queue-and-selection-state.md) | Command Queue and Selection State |
| 469 | [findings/469-entity-task-and-command-setup.md](docs/findings/469-entity-task-and-command-setup.md) | Entity Task and Command Setup |
| 470 | [findings/470-entity-placement-and-spawning.md](docs/findings/470-entity-placement-and-spawning.md) | Entity Placement and Spawning |
| 471 | [findings/471-entity-activities-and-action-completion.md](docs/findings/471-entity-activities-and-action-completion.md) | Entity Activities and Action Completion |
| 472 | [findings/472-entity-motion-and-action-execution.md](docs/findings/472-entity-motion-and-action-execution.md) | Entity Motion and Action Execution |
| 473 | [findings/473-simulation-dispatch-and-damage.md](docs/findings/473-simulation-dispatch-and-damage.md) | Simulation Dispatch and Damage |
| 474 | [findings/474-simulation-tick-and-engine-state-lifecycle.md](docs/findings/474-simulation-tick-and-engine-state-lifecycle.md) | Simulation Tick and Engine State Lifecycle |
| 475 | [findings/475-objectives-upgrades-and-resource-counters.md](docs/findings/475-objectives-upgrades-and-resource-counters.md) | Objectives, Upgrades, and Resource Counters |
| 476 | [findings/476-entity-activity-and-runtime-records.md](docs/findings/476-entity-activity-and-runtime-records.md) | Entity Activity and Runtime Records |
| 477 | [findings/477-tasks-rewards-and-checkpoints.md](docs/findings/477-tasks-rewards-and-checkpoints.md) | Tasks, Rewards, and Checkpoints |
| 478 | [findings/478-selection-groups-and-selection-state.md](docs/findings/478-selection-groups-and-selection-state.md) | Selection Groups and Selection State |
| 479 | [findings/479-renderer-com-interface-vtable-slots.md](docs/findings/479-renderer-com-interface-vtable-slots.md) | Renderer D3D/D3DRM interface vtable slots (issues 253/254) |

Canonical full pseudocode corpus: [`docs/decompiles/full/LEGORR.full.c`](docs/decompiles/full/LEGORR.full.c)
(2564 manifest functions; see its [README](docs/decompiles/full/README.md)). The
legacy [`engine-region.c`](docs/decompiles/legacy/engine-region.c) retains named body
variants that have not yet been reconciled; its 11 entries missing from the
full manifest are preserved in `docs/decompiles/full/LEGORR.supplement.c`.
Machine-readable evidence: [`docs/evidence/legorr-rev.bundle`](docs/evidence/legorr-rev.bundle)
(236 recorded REA observations against `LegoRR.exe`,
SHA-256 `c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`).
The latest helper expansion has its own supporting bundle at
[legorr-engine-helpers-expansion.bundle](docs/evidence/legorr-engine-helpers-expansion.bundle)
and the follow-up terrain/resource/text expansion is recorded in
[legorr-engine-helpers-expansion-2.bundle](docs/evidence/legorr-engine-helpers-expansion-2.bundle).
The latest engine and terrain helper decompiles are recorded in
[legorr-engine-helpers-expansion-3.bundle](docs/evidence/legorr-engine-helpers-expansion-3.bundle).
The current direct-decompile batch is in
[legorr-engine-helpers-expansion-4.bundle](docs/evidence/legorr-engine-helpers-expansion-4.bundle).
The reward, terrain-view, and simulation helpers are recorded in
[legorr-engine-helpers-expansion-10.bundle](docs/evidence/legorr-engine-helpers-expansion-10.bundle), with caller audits in the
[render](docs/evidence/legorr-engine-helper-audit-render-expansion-10.bundle) and
[simulation](docs/evidence/legorr-engine-helper-audit-simulation-expansion-10.bundle) bundles.
The segment mesh and formatted-scan implementations are in
[legorr-engine-helpers-expansion-11.bundle](docs/evidence/legorr-engine-helpers-expansion-11.bundle), with supporting
[text/runtime audit](docs/evidence/legorr-engine-helper-audit-text-io-expansion-10.bundle).
The cursor-interaction, overlay, menu, and scene-light helpers are recorded in
[legorr-engine-helpers-expansion-12.bundle](docs/evidence/legorr-engine-helpers-expansion-12.bundle).
The terrain quad-run, upgrade-table, and scene-slot helpers are recorded in
[legorr-engine-helpers-expansion-13.bundle](docs/evidence/legorr-engine-helpers-expansion-13.bundle).
The path-step, terrain, and pending-record helpers are recorded in
[legorr-engine-helpers-expansion-14.bundle](docs/evidence/legorr-engine-helpers-expansion-14.bundle).
The config-loader, file-descriptor, and CRT number helpers are recorded in
[legorr-engine-helpers-expansion-15.bundle](docs/evidence/legorr-engine-helpers-expansion-15.bundle).
The config-file, text-layout, and record-emit helpers are recorded in
[legorr-engine-helpers-expansion-16.bundle](docs/evidence/legorr-engine-helpers-expansion-16.bundle).
The region-route, light-config, and surface helpers are recorded in
[legorr-engine-helpers-expansion-17.bundle](docs/evidence/legorr-engine-helpers-expansion-17.bundle).
The entity move/route command, cell-queue, and object-clone helpers are recorded in
[legorr-engine-helpers-expansion-18.bundle](docs/evidence/legorr-engine-helpers-expansion-18.bundle).
The config graphics-quality, camera tween/drag, cell-search, and pool/spawn helpers are recorded in
[legorr-engine-helpers-expansion-19.bundle](docs/evidence/legorr-engine-helpers-expansion-19.bundle).
The config-band loaders, display-mode setup, directional-activity, and flight-record helpers are recorded in
[legorr-engine-helpers-expansion-20.bundle](docs/evidence/legorr-engine-helpers-expansion-20.bundle).
The config-band loaders, poly-variant build, panel/cell registration, layout, and activity helpers are recorded in
[legorr-engine-helpers-expansion-21.bundle](docs/evidence/legorr-engine-helpers-expansion-21.bundle).
The config-band loaders, cell-corner scan, action/command issues, record instantiation, and render-state helpers are recorded in
[legorr-engine-helpers-expansion-22.bundle](docs/evidence/legorr-engine-helpers-expansion-22.bundle).
The command-band kind dispatch, item name lookup, route command, drift/timer, terrain normal, and record/geometry helpers are recorded in
[legorr-engine-helpers-expansion-23.bundle](docs/evidence/legorr-engine-helpers-expansion-23.bundle).
The config-band texture loaders, route command issues, and surface/menu helpers are recorded in
[legorr-engine-helpers-expansion-24.bundle](docs/evidence/legorr-engine-helpers-expansion-24.bundle).
The terrain scatter, erosion propagation, cell query, menu record, and buffer helpers are recorded in
[legorr-engine-helpers-expansion-25.bundle](docs/evidence/legorr-engine-helpers-expansion-25.bundle).
The command sweeps, cell issuing, config priorities, statistics, and DirectDraw create helpers are recorded in
[legorr-engine-helpers-expansion-26.bundle](docs/evidence/legorr-engine-helpers-expansion-26.bundle).
The visual channel relays, resource-tree callback walks, scene pose helpers, and string/config registry helpers are recorded in
[legorr-engine-helpers-expansion-27.bundle](docs/evidence/legorr-engine-helpers-expansion-27.bundle).
The command-band step issuers, flag-behavior timers, and steering/mesh helpers are recorded in
[legorr-engine-helpers-expansion-28.bundle](docs/evidence/legorr-engine-helpers-expansion-28.bundle).
The configured tiny-monster spawning and mutual activity pairing helpers are recorded in
[legorr-engine-helpers-expansion-29.bundle](docs/evidence/legorr-engine-helpers-expansion-29.bundle).
The interface-method wrappers, frame overlay refresh, and former deferred-set relays are
recorded in [legorr-engine-helpers-expansion-30.bundle](docs/evidence/legorr-engine-helpers-expansion-30.bundle).
The selected-entity detach callback and pair-record commit helpers are recorded in
[legorr-engine-helpers-expansion-31.bundle](docs/evidence/legorr-engine-helpers-expansion-31.bundle).
The latest input, selection, and resource helper batch is in
[legorr-engine-helpers-expansion-5.bundle](docs/evidence/legorr-engine-helpers-expansion-5.bundle);
its address-range audits are in the [low](docs/evidence/legorr-engine-helper-audit-low-expansion-5.bundle),
[simulation/input](docs/evidence/legorr-engine-helper-audit-mid-expansion-5.bundle), and
[resource/rendering](docs/evidence/legorr-engine-helper-audit-high-expansion-5.bundle)
evidence bundles.
The latest terrain, entity, UI, and stream expansion is in
[legorr-engine-helpers-expansion-6.bundle](docs/evidence/legorr-engine-helpers-expansion-6.bundle),
with its [low](docs/evidence/legorr-engine-helper-audit-low-expansion-6.bundle),
[simulation](docs/evidence/legorr-engine-helper-audit-mid-expansion-6.bundle), and
[resource](docs/evidence/legorr-engine-helper-audit-high-expansion-6.bundle)
caller-audit bundles.
The text renderer and font helper call chain is in
[legorr-engine-helpers-expansion-7.bundle](docs/evidence/legorr-engine-helpers-expansion-7.bundle).
The terrain, rendering, and stream helper batch is in
[legorr-engine-helpers-expansion-8.bundle](docs/evidence/legorr-engine-helpers-expansion-8.bundle),
with [terrain](docs/evidence/legorr-engine-helper-audit-terrain-expansion-8.bundle),
[rendering](docs/evidence/legorr-engine-helper-audit-render-expansion-8.bundle), and
[text/IO](docs/evidence/legorr-engine-helper-audit-text-io-expansion-8.bundle) caller audits.
Game-file stream and DirectDraw pixel helpers were added in
[legorr-engine-helpers-expansion-9.bundle](docs/evidence/legorr-engine-helpers-expansion-9.bundle).
The camera, LWO layer, and normal helpers are in
[legorr-engine-helper-audit-render-expansion-9.bundle](docs/evidence/legorr-engine-helper-audit-render-expansion-9.bundle).
The terrain labels, grid-bound, and selection helpers are documented in
[legorr-engine-helper-audit-terrain-expansion-9.bundle](docs/evidence/legorr-engine-helper-audit-terrain-expansion-9.bundle).
The buffered-stream and Win32 file helpers are in
[legorr-engine-helper-audit-text-io-expansion-9.bundle](docs/evidence/legorr-engine-helper-audit-text-io-expansion-9.bundle).
The latest three-range helper audits are preserved in the
[low](docs/evidence/legorr-engine-helper-audit-low.bundle),
[simulation/terrain](docs/evidence/legorr-engine-helper-audit-mid.bundle), and
[resource/rendering](docs/evidence/legorr-engine-helper-audit-high.bundle) evidence bundles.

## Reproducibility

- **Unpacking**: `python3 tools/unwad.py reference/LegoRR1.wad unpacked/` extracts both archives.
- **Function naming**: `tools/ghidra/CreateEngineFunctions.java` is a Ghidra headless
  post-script that creates/renames the analyzed functions and emits the decompile corpus;
  batch runs use `support/analyzeHeadless` with `-postScript`.
- **Interactive analysis**: REA (Ghidra provider). Findings cite addresses, not guesses:
  every claim traces to a pseudocode block or an evidence record.

## Reconstruction build

Meson build with one source tree and two platform backends (ADR 0004). The
`native` build is green and the driver (`src/main.cpp`) now exercises the
**engine scaffold** plus the **game layer** (levels, objectives, savegames —
findings 07/08/22): `EngineTick`/`EnginePreTick`/`KeepRunning`/
`MainSimStep`/`EngineShutdown` in `src/gods98/engine.*` mirror the corpus's
critical path (`0x4232e0`, `0x4231f0`, `0x435940`, `0x4250c0`, `0x424cf0/0x424d00`)
over the `DAT_005560d4` state block, and `src/gods98/tickers.cpp` reproduces the
tick's ordered subsystem dispatch (finding 03 §4) as fill-in stubs
(`ticker_<FUN-hex>` ↔ corpus `FUN_<hex>`).

**Resource loader** (`src/gods98/resource.*`, finding 01): mounts
`<dir>/<prefix>N.wad` for N = 0..9 (the corrected exe-basename prefix), parses
the WWAD header + TOC, and opens entries by case-insensitive name with
later-mounts-shadow-earlier resolution (STD/loose-file branch + dormant-codec
stub included). Validated by `test_resource`: synthetic WWAD round trip always,
plus real-wad checks (counts 5631/214, payload tags, `.flh` via finding 21) when
`-Dtest_wads_dir=<dir>` is set:

```sh
meson setup build -Dtest_wads_dir=/path/to/shipped/game
meson test -C build --suite resource
```

**Texture loader** (`src/gods98/texture.*`, finding 13): decodes Windows BMPs —
BITMAPINFOHEADER (>= 40) and OS/2 core (12) headers, BI_RGB, 1/4/8-bit paletted
and 16/24/32-bit — loaded through the resource layer into ARGB8888 surfaces,
plus the engine's case-insensitive name-keyed get-or-load cache (finding 13 §2).
`texture_test` (suite `texture`) covers synthetic BMP round trips (all depths,
palette clamping, RLE/truncation rejection) and, with `test_wads_dir` set,
decodes five shipped entries and matches their pixel hashes (reference hashes
derived pixel-exact from the corpus via Pillow).

**Sound loader** (`src/gods98/sound.*`, finding 14 §1–2): order-independent
RIFF/WAVE chunk walk (`fmt` → `data`; `LIST`/`cue`/`smpl`/`acid`/`fact`
tolerated), PCM only, through the resource layer, plus the 600-slot sound bank
with the corpus's loop/stream flag bits. `sound_test` (suite `sound`) covers
synthetic WAVs (8/16-bit, stereo, odd-size padding, chunk-order variants,
junk rejection) and, with `test_wads_dir` set, parses **all 387 shipped
.wav files** and byte-matches five reference entries against `wave`-module-
derived PCM hashes.

**Sound alias registry** (2026-10-24, finding 137; port issues #313/#321/#314):
`sound_alias_registry_init` (`SoundAliasRegistryInit @ 0x464740`) builds the
corpus's index-to-alias mapping — 45 hard-coded `SFX_*` built-ins in slot
order — `sound_alias_lookup` (`SoundAliasLookup @ 0x464c70`) resolves names to
indices through built-ins and config-registered entries with the corpus's
case fold, and `sound_alias_registration_set` (`SoundDynamicAliasRegistration
Set @ 0x464c50`) arms/disarms appending unknown names (the port stores alias
*names* where the corpus stores uppercase string hashes, and bounds the 495-
slot table it overflowed). `sound_test` pins the whole 45-entry table, the
indices pinned by findings 137/144, the case-folded/NULL/no-append paths, the
flag surviving re-init, and the port's bound.

**Sound subsystem enable state** (2026-10-24; port issues #315/#322/#323/#316):
`sound_subsystem_enable_set` (`SoundSubsystemEnableSet @ 0x465260`) raises
`DAT_00502a2c`'s value-1 bit and, on disable, clears it, runs the stop seam,
and zeroes the global-cooldown word `_DAT_00502b2c`;
`sound_subsystem_enable_set_and_stop` (`@ 0x4652a0`) is the stop-armed form;
`sound_subsystem_enabled` (`SoundSubsystemEnabledGet @ 0x465370`) and
`sound_batch_mode_get` (`SoundBatchModeGet @ 0x4652b0`, returning the masked
word) read it back. Deviations: the corpus's stop seam (SoundSubsystemStopAll,
unported) and the queue-side submission suppression land with their own
milestones, and the port shares the corpus flag word with the alias
registration bit. `sound_test` covers the enable/disable/and-stop
transitions, the cooldown-word zero, batch-mode readback, and flag-bit
independence.

**Deferred request queue** (2026-10-24; port issue #317, aggregate #296
write side): `sound_request_queue_submit` (`SoundRequestQueueSubmit @
0x464f60`) appends alias + loop-flag requests to the corpus's two parallel
10-entry tables (`DAT_00502b30`/`DAT_00502b58`, count `DAT_00502bd0`) and
drops silently past capacity, exactly as shipped. The pending set is exposed
through count/get accessors; the per-frame consumer that rotates it into the
active playback copies (SoundSubsystemFrameTick) and the deferred/batch
drains land with their own milestones. `sound_test` covers ordered appends,
the drop-on-full path, bounded reads, and the count-only clear.

**Deferred-play cooldown** (2026-10-24; port issues #326/#327):
`sound_deferred_timer_set_if_idle` (`DeferredSoundTimerSetIfIdle @
0x464ec0`) arms the corpus's global cooldown timer (`_DAT_00502b24`) only
while idle, forgetting the recorded handle word (`DAT_00502b28`), and
`sound_deferred_play_expire` (`DeferredSoundPlayExpire @ 0x464e80`) stops +
resets when a recorded handle meets a running timer. Deviations: the expiry
stop seam is a no-op until the instance layer lands, and the handle word is
written through a port seam mirroring the submission route (whose body is
blocked behind the alias-play chain); blocked sound chains are catalogued in
`docs/coverage/blocked.md`. `sound_test` covers the idle gate, handle
forgetting, both expire skip-paths, and re-arming.

**Bank record lifecycle** (2026-10-24; finding 14 §1): the bank's 600 slots
are now corpus-shaped records — busy/loaded bits plus the loop (`*`
duplicate-buffer, finding 134) and stream bits — driven by
`sound_bank_entry_load` (`SoundBankEntryLoad @ 0x47a870`, claims a free slot
from `sound_bank_free_slot_find @ 0x47aad0`, loads `<name>.wav`, validates the
`#N#` millibel volume with `SoundVolumeValueValid @ 0x47a850`, drops the slot
whole on load failure) and `sound_bank_entry_release` (`@ 0x47ab00`). Deviations:
the loader seam reuses the port's resource-layer WAV loader for the corpus's
platform mmio path, the release seam's DirectSound interface drops land with
the instance layer, and the sound-system switch (DAT_00533878) is a port seam
defaulting on. `sound_bank_load` keeps its port-side dedupe. `sound_test`
covers slot claiming/reuse, flag bits, volume validation, the failure and
sound-off paths, and record accessors. This is the root of the chain recorded
in `docs/coverage/blocked.md`; the marker-parse/configured-variant entries
follow it.

**Configured sound-variant chains** (2026-10-24; port issues #318 / #324,
findings 107/134/135/142): `sound_sample_marker_parse`
(`SoundSampleMarkerParse @ 0x464d00`) turns a `Samples` config value —
`"*path,#-400#path,@path,path"` — into the alias's variant chain, loading
each comma-separated path into a bank record: the `*` prefix raises the
record's duplicate-buffer (loop) bit, `#N#` carries its millibel volume,
and `@` switches this and every later path to streamed playback
(finding 135). `sound_variant_select` (`SoundSampleVariantSelect @
0x464e20`) re-draws the chain with the corpus's 15-bit PRNG
(`random_value % variant_count`) and swaps the pick to the head. The port
reproduces the corpus's build-time quirk — the second load overwrites the
head slot, each later token lands in the previously allocated node, and the
last node keeps stale pool data. Deviations: the split works on a copy of
the config string (the corpus splits it destructively), the pool is capped
with further links dropped (the corpus scribbles past its region), empty
`#…#` parses as 0 (the corpus reads stack), and the PRNG is modelled
module-locally like `level.cpp`/`frontend.cpp`. `sound_test` covers the
head-overwrite quirk, marker semantics, the stream persistence rule, the
deterministic first draw and swaps, failure links staying linked as `-1`,
and the split quirks.

**Configured variant properties** (2026-10-24; port issues #319 / #320):
`sound_variant_property_set` (`ConfiguredVariantPropertySet @ 0x465010`)
draws the alias's variant chain and stores a millibel volume into the
picked record through `sound_bank_entry_volume_set`
(`SoundBankEntryVolumeSet @ 0x47b050`, record +0x110 word, validated —
the corpus's buffer-interface push waits on the instance layer), while
`sound_variant_property_read` (`ConfiguredVariantPropertyRead @ 0x465030`)
performs the draw only: the corpus reads the word back through the
stride-134 getter (`IndexedRecordStride134WordGet @ 0x47b0d0`, modelled by
the record accessor) but discards it, so the rotation is the whole visible
effect. Deviations: the corpus's invisible second argument to the setter is
taken explicitly (issue #319), and slot writes are bounds-guarded (the
corpus writes past the table). `sound_test` covers the draw-only read,
head-record volume writes, invalid-value zeroing, validator and slot
bounds, and the silent-alias no-op.

**Sound submission route** (2026-10-24; port issues #325 / #328 / #329 /
#330 / #332, findings 137–149): `sound_alias_play` (`SoundAliasPlay @
0x464fa0`) and `sound_positional_sound_submit` (`PositionalSoundSubmit @
0x465090`) run cues through the subsystem — with batch mode off the
alias's variant chain is drawn and the picked record goes to the
instance-layer play seam; with batch mode on (bit 3) submissions defer
into the corpus's ten-record batch table (six words per record: alias,
position word, coordinate words, and a mode word whose bit 0 marks the
positional form, bit 1 the loop and bit 2 the position-selector handed
back at replay). `sound_batch_queue_drain` (`SoundBatchQueueDrain @
0x4652d0`) raises the mode with `batch_set` (returning untouched) or
clears it and, with `replay`, re-runs every deferred record through the
route before clearing the table; `sound_deferred_queue_drain`
(`SoundDeferredQueueDrain @ 0x4652c0`) is the replay-1 wrapper.
`sound_subsystem_frame_tick` (`SoundSubsystemFrameTick @ 0x4651a0`) steps
the global cooldown timer, replays last frame's rotated request queue
through the alias-play route, restores armed batch mode afterwards,
rotates the pending queue (#317) into the active replay copies, and runs
the instance pump. The corpus quirks are reproduced: an eleventh deferred
submission is dropped, the deferred alias form never writes the loop bit
so a replay inherits whatever an earlier occupant of the record slot
left, slot value 0 counts as "no sound", and the silent alias defers
like any other. Deviations: the play/pump calls land at instance-layer
seams that wait on `TODO(corpus)` (the corpus routes through
`SoundBufferPlayWithPosition @ 0x47ad70` and the handle chain), so the
observable state is the chains, queues, records and PRNG draws.
`sound_test` covers the deferral cap and drop, the mode-word packing for
both submit forms, replay routing with the multi-link draw, the
stale-loop quirk, the frame tick's timer step / queue rotation / batch
restore, and the disabled-subsystem gate that runs before the draw.

**Media-playback state machines** (2026-10-24; port issues #301 / #302):
`src/gods98/media.*` models the two simulation-driven state machines of
the media area plus the one-shot scheduler #301 introduces (the port
analog of the corpus's scheduled-callback table, driven by the engine
clock). `media_cd_watchdog_update` (`CDAudioWatchdogModeUpdate @
0x4297b0`): enable raises the bit-value-8 flag in the audio state word, draws the
time-derived CD-track slot (random % window + start, per the CDTracks /
CDStartTrack config) and schedules the watchdog re-arm for that many
seconds; the callback re-arms with a fresh draw, as shipped. Disable
clears the bit, cancels, and runs one watchdog tick with 0 — which
touches nothing, as shipped. The stalled-track check exposes the
port-side analog of the corpus's CD-media status query, so the state
machine and scheduler are the observable core. Real CD-track playback
does not exist on native hardware under this architecture — the
track-start/stop helpers are documented no-op seams and the winapi
reference build keeps the real behaviour (the deviation is this note,
per the issue). `media_min_distance_update_mode`
(`SoundMinimumDistanceUpdateMode @ 0x4295f0`): mode 0 stores the supplied
entity/context and applies the caller-selected distance scalar (0.9 /
0.6 listener scale, clip = min-distance base × cell distance scale, 5.0
tracked reference distance); mode 1 clears the context and restores the
config-derived previous values (0.5 scale, the config clip and reference
distances); other modes update only the mode global. Finding 418 flags
the state's higher-level purpose as uncertain, so the port models
exactly this observable state machine — the render/audio distance
pushes land at inert seams. `media_test` covers the scheduler
(one-shot, overwrite, cancel), the watchdog's bit/schedule/cancel
behaviour, deterministic draws via an LCG mirror, the stalled re-arm,
the stale loop flag, and config zero-to-default rules, plus both modes,
both scalars, repeated entry, unknown modes, and restore of the
min-distance state. The corpus's simulation call paths (the music
toggle in MainSimStep, level reset, objective tick, level setup and
selection changes) land with their corresponding sim ports.

**Media playback toggle + cue dispatch** (2026-10-24; port issues
#298 / #300): `media_audio_playback_toggle` (`AudioPlaybackToggle @
0x429810`) raises or clears the sound-on bit (value `0x10` in the same
state word as the watchdog flag) and forwards enable/mute through the
ported `SoundSubsystemEnableSetAndStop @ 0x4652a0` gate — submissions
are gated while muted, repeats are idempotent. The cue dispatch pair
runs over the ported submission route: `media_action_feedback_sound_dispatch`
(`ActionFeedbackSoundDispatch @ 0x428800`) saves the batch mode, forces
it off with no replay, maps events 0–5 to a cursor-override slot
(`0x14`/`0x15`, value 12.5) plus cue (`0x13`, `0x14`, `0x1f`, `0x1e`,
`0x20`, `0x21`) through `SoundAliasPlay`, and restores the saved mode —
unknown events submit nothing but still restore, and deferred batch
records survive the dispatch untouched (the dispatch's drain never
replays). `media_action_code_to_sound_alias` (`ActionCodeToSoundAlias @
0x4351a0`) maps action-record codes 1–5/8/10/11 to cues `0x2b`–`0x2f`,
`0x32`, `0x34`, `0x35` through the cursor select. The select/override
pair itself (`PointerCursorSelect @ 0x45cc20`,
`PointerCursorOverrideSet @ 0x45cc40`) is held module-locally as the
observable half until the cursor draw lands: the select writes the slot
only while the override value is non-positive, the override setter
writes both. `media_test` covers both toggle directions with the
subsystem gate, batch save/force/restore including the no-replay quirk
and the unknown-event path, slot/cue mapping, select blocking, and the
unmapped-code no-ops.

**Rect-aware video pump** (2026-10-24; port issue #303):
`movie_video_playback_loop` (in `src/gods98/movie.*`) is one
shape-compatible entry point for `DirectShowVideoPlaybackLoop @
0x42efc0`'s two branches — a null destination rectangle pumps decoded
frames to the full window, and a supplied `MovieRect` scale-fills each
frame into it (`movie_scale_rgba_rect`, the rect twin of the existing
scaler) after the setup-time render-surface restore seam runs once
first, like the corpus. Frames stream while the decoder keeps
delivering, with the quit poll between frames. The frameloop movie
driver's frame loop now routes through this pump, so the level-intro
path — the port's replacement for the corpus's in-`LevelLoad @
0x429890` video call — runs through the same seam. Deviation (documented
in the header): the corpus's own DirectDraw blit passes null rectangles
and never applies the computed rect (findings 200/206), so the shipped
rect branch still fills the window; this port's replacement honors the
rect per the issue's contract, and that is the intentional difference.
`movie_test` drives both branches with a fake decoder and a fake sink:
frame counts, restore-before-first-frame ordering (rect branch only),
rect plumbing, quit leaving the branch mid-stream, failed-present
resumption, and the pixel-exact rect blit with clipping.

**Streamed-voice duration + stream triggers** (2026-10-24; port issues
#299 / #340): `sound_streamed_wave_buffer_step_rate`
(`StreamedWaveBufferStepRate @ 0x47b8e0`) divides a bank record's
stored WAV size by its format's average bytes per second — no zero
guard as shipped (finding 217: unloaded records divide to NaN, and a
NaN cooldown would never expire), slot −1 (the variant select's
no-sound result) reads 0. `sound_object_duration_get`
(`SoundObjectDurationGet @ 0x465160`) asks the variant chain what the
alias selects, so each duration consumes one PRNG draw.
`sound_submit_with_global_cooldown` (`SoundSubmitWithGlobalCooldown @
0x464ef0`) plays the alias through the submission route while the
cooldown timer is down, then records the last-alias word, the handle
word (the routed bank slot — deviation: the corpus's handle is
instance-layer, TODO(corpus)), and a timer of duration × 25. The
stream triggers `sound_stream_play_level` /
`sound_stream_play_tutorial` (`StreamPlayLevel @ 0x416150` /
`StreamPlayTutorial @ 0x4161a0`) format the `Stream_LevelName_Level_%d`
/ `Stream_LevelName_TLevel_%d` stream name, resolve it through the
alias registry, and submit through the same gate. `sound_test` covers
the slot −1 / unloaded-NaN / loaded-record step-rate cases, the
chain-backed duration draw, idle-timer submission with the recorded
words and derived timer, running-timer gating, the disabled-subsystem
and batch-mode declines with their deferral quirks, expiry, and the
stream-name triggers across the registration on/off paths.

**Terrain-cell flag primitives** (2026-10-24; terrain wave): sixteen
leaf accessors in `src/game/level.*` form the corpus's voxel-record
flag family that the walk, wave and occupancy paths read through —
`level_cell_walkable_flag` (`0x432bd0`: unsigned dim-1 gate, traversable
bit 0x10 with the blocked gate bit 3 clear), `level_cell_flag_wave`
(the wave flag, `0x432c20`), `level_cell_speed_class` (`0x432c50`: 0
under the terrain block bit, else the low terrain bits + 1),
`level_cell_mask_c0` (`0x432e60`, a byte read of the flag word),
`level_cell_flag_edge_wall` (the predug edge-wall bit, `0x432e90`), the
state-byte equality tests `level_cell_state_is_1/6` (`0x432ec0` /
`0x432f00`), `level_cell_mask18_clear` (`0x432f40`, outside the gate
counts as clear), `level_cell_flag_4` (`0x432f90`), the single-bit tests
`level_cell_flag_2000/80/8000` (`0x432fd0` / `0x433000` / `0x433030`),
`level_cell_secondary_bit0` (`0x433060`, the record+12 word),
`level_cell_occupied_at` (`0x433090`, signed in-bounds check on the
occupied-class bit), `level_cell_mask_180000` (`0x4330e0`), and the
write-side `level_cell_flag_10000000_set` (`0x433120`) raising or
clearing the flag. Every accessor reads through `level_voxel`, so an
unaddressable cell reports zero flags / no state — a deviation from the
corpus, which computes the record address with no bounds check at all
and reads or writes the voxel heap past the grid for bad coords
(documented in `level.hpp`). The corpus's own bounds gates are
reproduced exactly, including the unsigned wrap that lets every
coordinate through when a grid dimension is zero. `level_test` drives
the flag word of a hand-picked record on the loaded fixture and covers
every mask, speed class, state equality, secondary word, raise/clear
word pair, gate and off-map behavior.

**Terrain-cell secondary flags + gated bits** (2026-10-24; terrain
wave): the second leaf cluster of the same voxel family — the
secondary flag word (record+12) gains `level_cell_secondary_flag_100`,
`level_cell_visited_flag`, the bit-0 set/clear pair, the bit-3
set/test pair, and `level_cell_secondary_bit0_set/clear`
(`0x432170` / `0x4322d0` / `0x4324d0` / `0x432520` / `0x4323c0` /
`0x4325d0`); `level_cell_occupied_or_flag` (`0x432aa0`) answers the
flag-0x400-or-secondary-byte-bit-4 OR-test its two modes define; the
gated single-bit reads `level_cell_flag_800/8/20_bounded` (`0x432b00` /
`0x432b50` / `0x432d90`) reproduce the dim-1 gate; `level_cell_flag8_check`
(`0x4329d0` built on `level_coord_pair_within_bounds`, `0x4502c0`) uses
the corpus's *plain* unsigned bounds; `level_cell_type_is_8_or_10_or_11`
(`0x432b90`) tests the blocky state-byte set; the reservation bit reads
back through `level_cell_flag_reserved` (`0x432dd0`) as the read twin of
`level_tile_mark`; `level_cell_flag_80000_set` (`0x432710`) raises or
clears the flag without the refresh companion (the 0x40000000 twin that
does is blocked); `level_cell_record_byte3` (`0x4332c0`) reads the +3
byte; and `level_cell_work_flag_set` (`0x4332f0`) raises the gated work
bit. Same bounds deviation as the first cluster. The corpus's
Set-and-Track / Set-and-Refresh variants stay blocked (cell track queues
and the per-cell CellInitPass, `docs/coverage/blocked.md`).
`level_test` extends the synthetic-record section with the secondary
word, mode test, gated reads, state set, word pairs and coordinate
bounds.

**Map-file slot registry + terrain state leaves** (2026-10-24; terrain
wave): the corpus's 2000-entry map-image slot table —
`level_map_slots_reset` (`0x4893f0`, leak-clearing as shipped),
`level_map_slot_allocate` (`0x489410`, first-free claim, -1 when full),
`level_map_slot_release` (`0x489460`) and `level_map_slot_get`
(`0x4894a0`) — plus their accessors `level_map_dimensions_get`
(`0x4334c0`) / `level_map_cell_get` (`0x4319e0`) that resolve the slot
and read its `MAP ` image (dims at +8/+12, u16 cells row-major at
+0x10), and the raw-pointer twins `level_map_image_header` (`0x4515a0`)
/ `level_map_image_cell` (`0x4515c0`). An unresolved slot reads zero —
the corpus dereferences address 0/8/12 through a failed resolve
(deviation documented). The opener that fills the table,
`MapOpen2 @ 0x480170`, rides the unported wad-read seam
(`docs/coverage/blocked.md`). Terrain leaves: `level_cell_type_get_at`
(`0x433330`, the dim-1-gated state byte through an out param, untouched
outside) and `level_cell_byte_state_classify` (`0x42c5b0`) — the +5
byte indexes the {1,3,5,10,25} table by (v−1)/4 while the low bits
route it to four outs through the corpus's b/c-swap dispatch (bytes
past the table read stack garbage in the corpus and report 0 here).
`level_test` covers the slot lifecycle (claim, release, re-claim, reset,
header/cell images, zero reads through the failed-resolve deviation),
the gated type read and every classify routing corner.

**Entity record accessors** (2026-10-24; finding 457): the corpus's
`+0x334` stats-record family — thirty-nine field readers spanning
float words (`+0x20` … `+0x138`, including the terrain-speed array at
`+24 + index*4`, the energy-decay rate, speed component and collision
radius), the stats/state/capability flag words (`+144` / `+148` / `+14c`
plus `+ac` and `+13c`), the `+0xc0`/`+0xc4` selector word and the
`+0xa0` field **address** getter (`entity_stats_field_at_a0`, which
returns the address itself) — land as offset-preserving leaves in
`src/game/entity.*`. The record is modeled on `EntitySlot.stats_record`
(NULL → a zero template): the Stats config loader that fills real
records is unported, so reads default to zero exactly as the shipped
static templates do (`docs/coverage/blocked.md` records the config
layer, the indexed-table readers and the 0x400-toggle that needs the
type-4 post-create resolver). `entity_test` drives the whole family
over a synthetic record image, checks each read lands on its corpus
offset, and covers the zero-template fallback, the selector pair and
the address identity.

**Corpus text primitives** (2026-10-24; findings 397/12): four string
helpers land in `src/gods98/string.*` with a `string` test suite —
`string_duplicate` (`StringDuplicate @ 0x477550`, NUL copy through the
heap seam; malloc stands in for the engine-heap wrapper, TODO(corpus)),
`string_find_substring` (`StringFindSubstring @ 0x48da50`; the corpus's
aligned-word/SWAR scan is byte-equivalent strstr behavior, and the
empty-needle-yields-the-haystack early-outs are kept),
`string_length_bounded` (`BoundedStringLength @ 0x493ec0`; length up to
the bound, bound when no NUL turns up in time), and `string_path_join`
(`ConfigEntryFind @ 0x478f50`) building `root::comp1::comp2…` into the
static builder buffer that the already-ported `block_find` half reads
(the corpus's other half performs the record lookup; join-only is the
documented split). Appending past the static buffer's 256-byte bound
is bounded (the corpus runs through the static); `string_test` covers
duplicates, the scan corners, bound edges and join/truncation.

**Engine heap seam** (2026-10-24; findings 397/243): the corpus's
allocation entry points land in `src/gods98/heap.*` with a `heap` test
suite — `engine_heap_alloc` (`EngineHeapAlloc @ 0x48dbd0`, the
retry-flag entry), `engine_heap_alloc_retry`
(`EngineHeapAllocateWithRetry @ 0x48dbf0`; zero-size allocates one
byte, sizes at/above `0xffffffe1` hit the corpus overflow gate and
answer NULL, an unregistered retry handler makes a single attempt),
`engine_heap_alloc_aligned` (`EngineHeapAllocateAligned @ 0x48dc40`,
16-byte round-up), and the free pair (`EngineHeapFree @ 0x48db80` /
its resource-layer wrapper `@ 0x47ff40`, NULL no-op). The seam maps
onto the host allocator — the corpus's pool allocator, allocation-page
bookkeeping and new-handler registry stay blocked (see
`docs/coverage/blocked.md`), so every allocation is a single aligned
attempt; `heap_test` covers alignment, the zero-size and overflow-gate
quirks, the retry path and free/re-alloc flow.

**Selection record accessors** (2026-10-24; finding 06): the corpus's
selection-state leaves land in `src/game/select.*` —
`selection_has_any` (`SelectedEntitySetHasAny @ 0x4525b0`, the
count-nonzero gate), `selection_record_array`
(`SelectionRecordArrayGet @ 0x4525e0`, the `DAT_004fef2c` array
itself), the count getter (now citing `SelectionRecordCountGet @
0x4525f0`) and `selection_count_reset` (`SelectionRecordCountReset @
0x452600`), which zeroes only the count — the corpus leaves the entries
and their selected flags in place for the snapshot paths, so the
flag-clean variant stays `selection_clear`. `select_test` drives the
gate, array identity/order, the count and the count-only reset.

**CRT / compiler-runtime leaves** (2026-10-24; findings 260/381/393/394):
three runtime thunks land in a new `src/gods98/crt.*` module with a
`crt` suite — `crt_memory_move` (`CrtMemoryMove @ 0x498000`; the
corpus's word-loop/byte-tail copy with backward traversal for the
overlapping case is byte-equivalent memmove), `float_to_long`
(`FloatToLong @ 0x417430`; the compiler's `__ftol` helper under the
truncate-toward-zero control word, mapped to the C++ cast with the
corpus's 32-bit result width), and `noop_callback` (`NoOpCallback @
0x41f8e0`; the callable no-op several engine paths install — its return
value is undefined per finding 381). The two x87 control-word stub
thunks (`CrtNoOpStub @ 0x491ff5`, `CrtNoOpStubVariant @ 0x497870`) are
blocked — their call sites consume FP state the empty Ghidra bodies
cannot prove (`docs/coverage/blocked.md`). `crt_test` covers overlap
both directions, word/zero-count sizes, truncation corners and the
no-op's call identity.

**Pure vector helpers** (2026-10-24; finding 280): the eleven corpus
vector leaves land in `src/gods98/view.*` — scale/add/sub/cross/
normalize, 3D distance/dot/length, the two 2D distance variants and
the exact-equality test (`FUN_00401720/00401500/004016c0/00401470/
00401750/00401420/004016f0/004013a0/00401240/00406730/0047c0c0`). The
five earlier bodies already shipped; their abbreviated citation
(`FUN_00401720/1500/1420/...`) never flipped those ledger rows, so the
mapping comment now cites each address in full (the recurring
abbreviated-citation hazard, now closed). The corpus computes in x87
long double; the port's single-precision results are byte-equivalent,
and `vec_normalize` keeps the corpus's unguarded zero length.
`view_test` covers every helper including the cross convention and
the 2D variants' agreement.

**Pure geometry and numeric leaves** (2026-10-24; findings 262/323/363
and 355/364):
five geometry helpers join `src/gods98/view.*` —
`point_distance_within_radius` (`PointDistanceWithinRadius @
0x45b8a0`, strictly-under test), `rect_contains_point_half_open`
(`RectContainsPointHalfOpen @ 0x45e660`, the record's float {x,y,w,h}
quad, half-open both axes), `ray_plane_intersection_parameter`
(`RayPlaneIntersectionParameter @ 0x479a30`, `t = dot(q−p, n) /
dot(d, n)` with the parallel answer 0), `line_intersection_2d_compute`
(`LineIntersection2DCompute @ 0x479af0`, writes the hit point or
returns NULL when parallel), and
`sphere_ray_intersection_discriminant_positive` (`SphereRay… @
0x479c10`, the corpus's discriminant expression verbatim). Six numeric
runtime leaves join `src/gods98/crt.*` — `bit_count32` (`BitCount32 @
0x47d3f0`, clear-lowest loop), `pop_count32` (`PopCount32 @ 0x47d700`)
and `bit_count32_by_position` (`BitCount32ByPosition @ 0x47e7e0`, the
corpus's two identical position-loop bodies), `find_first_set_bit32`
(`FindFirstSetBit32 @ 0x47d720`, 0xffffffff for zero), `uint96_is_zero`
(`UInt96IsZero @ 0x497ab0`, exactly three dwords) and
`double_is_finite` (`DoubleIsFinite @ 0x48e870`, the exponent-not-all-ones
test). Corpus x87 math is byte-equivalent single precision here.
`view_test` drives all five geometry corners (edge exclusivity,
parallel answers, hit/miss spheres); `crt_test` the bit counts'
agreement, the scan corners and Inf/NaN.

**Matrices, polygon parity, and the LWO cursor** (2026-10-24; findings
309/388/415): the remaining pure-math leaves land in their corpus
module's homes — `matrix4_set_zero` (`Matrix4SetZero @ 0x479ea0`, 16
zero stores) and `matrix4_multiply` (`Matrix4Multiply @ 0x479ce0`,
which takes `(out, right, left)` and accumulates each cell as a
four-term dot product, i.e. `out = left × right`, row-major) next to
`point_in_polygon_2d` (`PointInPolygon2D @ 0x479b80`, the even-odd
horizontal-ray crossing parity over parallel edge-start/edge-end
arrays — the corpus reads the edge end through a lockstep pointer pair,
and the boundary/zero-span quirks carry over) in `src/gods98/view.*`;
`integer_pair_divide_to_float_pair` (`IntegerPairDivideToFloatPair @
0x45b900`) and `float_pair_to_integer_pair` (`FloatPairToIntegerPair @
0x45b930`, the two-shot `__ftol` pair truncation) in `src/gods98/crt.*`;
and `lwo_read_big_endian_float` (`LwoReadBigEndianFloat @ 0x48c1d0`) in
`src/gods98/model.*` — the stream-cursor reader that byte-reverses four
bytes and advances (the port's own model path parses via the `rrlwo`
converter, so the reader ships standalone). `view_test` pins the
matrix order and polygon parity, `crt_test` the pair conversions,
`model_test` the cursor walk.

**Terrain destination/placement predicates** (2026-10-24; findings
284/293): the two entity-gated cell predicates land in `src/game/level.*`
next to `level_walkable`, whose entity-gate TODO this closes —
`entity_move_destination_cell_usable` (`EntityMoveDestinationCellIsUsable
@ 0x432950`: entity `+1000` — decimal, the `flags_3e8` mirror — bit
0x8000, both destination coordinates nonzero and inside the 2-cell
margin, cell flag 0x10 set / 0x40 clear, type not in {1, 7, 11}) and
`terrain_cell_allows_placement` (`TerrainCellAllowsPlacement @ 0x432a20`:
the gate passes on a NULL record or the `+1000` low byte's bit 0x4, then
flag 0x10 set / 0x20 clear, type not in {8, 10, 11}). Both read the
already-modeled voxel grid through `level_voxel` (the corpus computes the
cell address unguarded past its gate; unreachable cells report unusable
— documented deviation). `level_test` drives both gates, the coordinate
margins, the flag and type exclusion sets and the differing
blockers/blocked-types of the pair.

**Offset-point rect family** (2026-10-24; findings 241/363/446): the
three hit-walk containment leaves land in `src/gods98/view.*` beside
`rect_contains_point_half_open` — `rect_contains_offset_point`
(`RectContainsOffsetPoint @ 0x45b590`, the category-cell walk's test),
`rect_contains_point_by_converted_coords`
(`RectContainsPointByConvertedCoords @ 0x463c00`, the region-route
walk's byte-identical copy) and `rect_contains_point_from_object_bounds`
(`RectContainsPointFromObjectBounds @ 0x463c90`, bounds read through the
record's +0x14 slot pointer). All three carry the same arithmetic: the
float offset pair truncates to int (the corpus's two `__ftol` calls —
the documented cast ≡ truncation), subtracts from the integer point, and
tests `x_min <= fx < x_max` / `y_min <= fy < y_max` over the bounds quad
`{x_min, y_min, x_max, y_max}` — half-open in both axes. The corpus
dereferences the +0x14 slot unguarded; the port answers 0 for a NULL
slot or rect (documented deviation). `view_test` drives the corners, the
truncated-offset shifts and the slot indirection.

**CRT math and wide-string leaves** (2026-10-24; findings 302/342/377/
397/399): four more runtime leaves join `src/gods98/crt.*` —
`crt_unsigned_add_with_carry` (`CrtUnsignedAddWithCarry @ 0x499fb0`, the
carry primitive the corpus's 96-bit helpers build on), `double_classify`
(`DoubleClassify @ 0x493a80`, the exponent+quiet-bit pattern classifier:
1/+Inf, 2/−Inf, 3/quiet-NaN-shaped, 4/signaling-NaN-shaped, 0/finite —
the corpus masks the high word's top 16 bits with `0x7ff8`, exponent
plus quiet bit, verbatim), and the two wide-string primitives
`wide_string_length` (`WideStringLength @ 0x499710`, the UTF-16 strlen)
and `bounded_wide_string_length_including_null` (`BoundedWideStringLength-
IncludingNull @ 0x49add0`, counting including the NUL when the terminator
falls inside the bound, the bound otherwise — and for a terminator at
exactly the bound position, since the corpus's remaining-count gate
answers 0 there). `crt_test` pins the carry corners, the classifier's
bit patterns and the bounded scan's boundary cases.

**Entity-state eligibility predicates** (2026-10-24; findings
318/319/471/472): three flag-word predicates join `src/game/entity.*`,
all pure reads over the `+0x3e0`/`+0x3e4`/`+0x3e8` mirrors and the
`+0x340` float that `EntitySlot` already models —
`entity_shared_eligibility_check` (`EntitySharedEligibilityCheck @
0x4385b0`: `+0x3e0` clear under the `0x700100` mask, `+0x3e4` bit
`0x100000` clear, the `+0x340` float not negative, `+0x3e0` bit `0x20000`
clear; the strict form additionally requires `+0x3e8` bit `0x20000000`
set with `0x80000000` clear, while the corpus's nonzero second argument
skips that clause), `entity_flags_mask107c33de_or_1c00203_any_test`
(`EntityFlagsMask107C33DEOr1C00203AnyTest @ 0x43baf0`, the literal-mask
any-test across both flag words) and `entity_type3_flag04000000_test`
(`EntityType3Flag04000000Test @ 0x4436f0`, kind 3 plus `+0x3e8` bit
`0x04000000`). `entity_test` drives each gate independently, the strict/
relaxed pair and the kind test.

**CRT format-argument cursor + exponent selector** (2026-10-24; findings
231/386/395/399): four runtime leaves join `src/gods98/crt.*` — the
varargs-cursor trio `format_argument_read_dword` /
`format_argument_read_word` / `format_argument_read_double`
(`FormatArgumentReadDword @ 0x491430`, `FormatArgumentReadWord @
0x491470`, `FormatArgumentReadDouble @ 0x491450`; each reads the value at
the cursor and advances it — four/two/eight bytes — the primitives
`EngineFormatStringToStream`'s width/precision/value walk builds on,
with the corpus's zero-extended word answer reproduced), and
`double_exponent_bits_or_special` (`DoubleExponentBitsGetOrSpecialValue
@ 0x492068`: the exponent field for ordinary values, the untouched high
word when the exponent is all ones, per finding 399's CrtAsin
pre-dispatch probe). `crt_test` pins the cursor arithmetic, the
zero-extended word read and the exponent selector's corners.

**Intrusive pointer stacks** (2026-10-24; findings 366/393): the corpus's
callback-state-list stack machine lands in a new `src/gods98/ptrstack.*`
module — `ptrstack_push` (`PointerStackPush @ 0x40fe40`),
`ptrstack_pop` (`PointerStackPop @ 0x40fe60`) and `ptrstack_for_each`
(`PointerStackForEach @ 0x40fe90`). Items chain through a link slot at
item `+0x9c` and the stack keeps head/count as a pair, exactly the
corpus's layout; the link is read with byte-safe access (the corpus's
dword slot is not pointer-aligned on 64-bit hosts), and the callback
walk reproduces the corpus's step order — the next pointer is read
*after* the callback returns, so a callback that re-links its own item
steers the walk. NULL-stack/NULL-item guards follow the corpus's push
skip; pop/foreach answer the empty case instead of dereferencing
(documented deviation). `ptrstack_test` drives LIFO order, the drain to
empty, the guards and the link-steering walk.

**Record access primitives** (2026-10-24; findings 305/332/354/356/367/
378/460): eight pure corpus readers join a new `src/gods98/record.*`
module — `record_field14_get` (`RecordField14Get @ 0x435f0b`),
`object_field10_get` (`ObjectField10Get @ 0x4850e0`),
`record_field_d8_equals_one` (`RecordFieldD8EqualsOne @ 0x46c3d0`, exact
equality with 1, not "nonzero"), `read_indexed_pair_slot_dword`
(`ReadIndexedPairSlotDword @ 0x471a20`, base + i·8 + j·4),
`velocity_slot_primary_object_get` / `velocity_slot_secondary_object_get`
(`VelocitySlotPrimaryObjectGet @ 0x471a40` /
`VelocitySlotSecondaryObjectGet @ 0x471a50`, indexed dword slots at
+0x18/+0x24 — returned as raw dwords so no corpus pointer is ever
truncated), `category_value_is_neither_zero_nor_fifteen`
(`CategoryValueIsNeitherZeroNorFifteen @ 0x41b470`) and
`interpolated_z_within_bounds` (`InterpolatedZWithinBounds @ 0x471330`,
the two-record +0x8 interpolation inside the target window). Field
*meanings* stay with their callers, as in the entity stats accessors;
the reads are byte-safe (the corpus's dword/float slots are not always
pointer-aligned on 64-bit hosts) and NULL records answer 0 (documented
deviation). `record_test` drives the offsets, the index arithmetic, the
equality corner and the window's edges.

**Active-engine state accessors** (2026-10-24; findings 295/324/392/475):
five level-block state readers land in `src/game/level.*` — the corpus
names its level block "active engine state", and `LevelState` already
models that block, so these read `level_int`/`level_float` words at new
`LEVEL_OFF_*` constants: `active_engine_field_b0_get`
(`ActiveEngineFieldB0Get @ 0x429850`, dword +0xb0),
`engine_float_field210_get_by_index` (`EngineFloatField210GetByIndex @
0x429860`, float +0x210 + 4·index), `engine_float_field260_get`
(`EngineFloatField260Get @ 0x429880`, float +0x260),
`global_state_field84_get` (`GlobalStateField84Get @ 0x42b500`, dword
+0x84 — the field meaning stays unresolved per finding 324), and
`upgrade_resource_check` (`UpgradeResourceCheck @ 0x41f900`, the
counter pair: +0xa0 for selector 0, +0xac for any nonzero selector).
The corpus dereferences its level global directly; a NULL level answers
0 (the corpus's own zero-when-no-level rule — documented deviation).
`level_test` drives each field through a store-and-read round trip, the
indexed float, the selector pair and the NULL guards.

**Config sibling iteration** (2026-10-24; findings 230/403): the one
missing piece of the config-lookup idiom lands in `src/gods98/block.*` —
`block_next_equal_key` (`SortedEntryNextEqualKey @ 0x479020`). In the
corpus a record's "key" is its *depth* word (+0xc) and the walk answers
the next entry that stays at that depth, which in the port's tree model
is exactly the next record under the same parent — next-sibling. The
loaders' `first; for (e = first; e; e = next_equal_key(e))` loop (the
three-call chain `ConfigEntryFind` + `ConfigNestedPathChildFind` + this
one, per findings 243/244) now composes fully. The corpus reads the
followed entry's depth before the bounds check, so a NULL next crashes
there; the port guards it (documented deviation). `block_test` drives
sibling order, subtree skipping, the end-of-chain corner and the
iteration loop.

**Final pure leaves: channel packer, FRNDINT, camera field** (2026-10-24;
findings 23/379/426/431): three stragglers from the pure-leaf sweep land
where they belong. `rgb_channels_pack` (`RGBChannelsPack @ 0x47db90`,
`surface.*`) packs the corpus's byte triple little-endian into the
0x00BBGGRR word the palette scan builds; a new `surface_test` drives the
corners. `x87_round_to_integral` (`X87RoundToIntegral @ 0x493150`,
`crt.*`) is FRNDINT — `rint()` matches the nearest-even integral
exactly, -0 kept (the x87 single/double deviation documented across
this band). `camera_rotation_get` (`CameraRotationGet @ 0x436b79`,
`record.*`) is the opaque-record float at camera +0x84: finding 426
establishes no axis/units for the field, so — like the stats accessors
— this is the faithful read and the meaning stays with callers.
`crt_test`/`record_test` pin the rounding corners and the field read.

**Documented last leaves: map center, widget test, progress, task bits**
(2026-10-24; findings 225/240/377/366/448/465): four more pure corpus
functions land where their semantics live. `map_cell_to_world_center`
(`MapCellToWorldCenter @ 0x44f640`, `record.*`) bounds-checks a cell
against the map descriptor and answers the cell's world X/Z center —
the corpus's five descriptor dwords (width +0, height +4, scale +16,
origin +24/+28) are ints converted to float at the use site and the
bounds test is the corpus's *unsigned* compare, both reproduced.
`menu_pointer_rectangle_hit_test` (`MenuPointerRectangleHitTest @
0x4112e0`, `view.*`) is the widget-rectangle test with all four edges
closed. `entity_progress_advance_by_25` (`EntityProgressAdvanceBy25 @
0x447730`, `entity.*`) adds 25 to the routing-progress float (new
`+0x344` mirror), clears the `+0x2fc` link field and clamps to the
entity's own `+0x340` bound, reporting when the bound is reached
(finding 448 keeps the field meanings with the routing layer).
`task_flag_bit_query` (`TaskFlagBitQuery @ 0x408970`, `record.*`) tests
`1 << (bit & 31)` against the task record's `+0x98` word, the corpus's
suppress-if-set form reproduced. `record_test`, `view_test` and
`entity_test` drive the arithmetic corners (also wiring up the
eligibility test that had been defined but never invoked).

**Bit-faithful trio: exponent builder, task pair, grid nibbles**
(2026-10-24; findings 366/373/383/399/451): the DAT-vein and pure-leaf
scan's next cluster. `double_build_with_exponent` (`DoubleBuildWithExponent @
0x493a40`, `crt.*`) rebuilds the floating value from two significand
words plus an exponent-bias input — Ghidra's CONCAT chains decompose to
the exact bit recipe (top word = `((high >> 16) & 0x800f) | ((exp +
0x3fe) << 4)`, halves of `high` over `low`), assembled with integer
arithmetic and bitcast (the x87 deviation as documented across the
band; `crt_test` pins the recipe as bit patterns).
`task_field84_get_and_field88_out` (`TaskField84GetAndField88Out @
0x408360`, `record.*`) is the faithful `+0x84` answer with the optional
`+0x88` copy, field meanings staying with the task layer.
`packed_grid_cell_value_get` (`PackedGridCellValueGet @ 0x40b850`,
`record.*`) decodes the packed cell byte into
`low nibble × width + high nibble` cell dwords, the corpus's own NULL
guard reproduced and its 32-bit cell-array slot read as a native
pointer (documented deviation).

**Handle sweeps: pool heads, render gates, overlay counts, cells**
(2026-10-24; findings 226/357/374/431/432): five more pure corpus leaves,
each in its semantic home. `object_pool_list_head_get`
(`ObjectPoolListHeadGet @ 0x456c30`, `record.*`) indexes the 55 list
heads at `+0x114` with the corpus's exact `-1 < idx < 55` bounds;
`model_child_render_eligibility_check` (`ModelChildRenderEligibilityCheck
@ 0x483a70`, `record.*`) is the two-bit render gate on the `+0x6c` word.
`entity_type_flag_bit1_get` (`EntityTypeFlagGetBit1 @ 0x46d330`,
`record.*`) reads bit 1 of the `+0x1e8` flag byte — meaning stays with
callers (finding 357). `menu_overlay_list_count` (`MenuOverlayListCount
@ 0x412170`, `view.*`) walks the overlay chain through the port's
native pointer slots. `terrain_cell_surface_type_get`
(`TerrainCellSurfaceTypeGet @ 0x44f540`, `level.*`) reads the surface
byte at block-table `+0x24` of the (now header-exposed) `terrain_create`
handle: the corpus's writer is the renderer-side appearance apply —
blocked with the other renderer seams — so a fresh terrain answers the
zero state, and the port adds the bounds guard the corpus lacks
(documented deviations). `level_test` drives the handle through the
fixture surface map.

**Reader sweep: sixteen faithful field reads** (2026-10-24; findings
305/337/356/361/372/373/376/381/393/411/430/432/435/451): the
pure-leaf scan's remaining field readers land in `src/gods98/record.*`
as the opaque-record pattern the stats accessors established — exact
offset, NULL answers zero, meaning stays with the callers. The object
and scene twins (`object_record_field11c_get` 0x46db60,
`object_field_be4_dword_get` 0x47a540, `entity_type_mask_get` 0x46cf80,
`scene_object_flags_get` 0x408850, `scene_object_owner_get` 0x473db0,
`scene_object_resource_get` 0x46d9e0,
`scene_object_wrapped_pointer_get` 0x473680,
`animation_object_handle_get` 0x406e30, `pointer_value_get` 0x482210)
plus the bit tests (`scene_object_flag_bit3_get` 0x473ba0,
`scene_object_mode_is_one` 0x406980), the 16-bit
`resource_entry_id_get` (0x4877d0) and the indexed readers —
`indexed_word_table_lookup` (0x4850c0, corpus (index, table) argument
order kept), `indexed_record_flag6c_bit0_get` (0x482780) and
`indexed_record_data_address_get` (0x483400), both walking the
record's +0x8 table slot at stride 0x70 — and the coordinate-pair
counter `count_matching_coordinates_in_list` (0x408e10), whose inline
entry slots hold native pointers (documented deviation).
`record_test` drives offsets, masks, bounds and the pair-list walk.

**Registry split and two straggler reads** (2026-10-24; findings
234/402/404/414): `registry_path_split` (`RegistryPathSplit @
0x48b330`, `block.*`) is the backslash component copier the registry
key builders loop over — copy up to (not including) the separator, skip
one, hand back the rest; the config paths' twin split lands where the
nested-path finder lives (the registry's 100-byte caller bound stays
with its callers, per finding 402). `avi_player_ready_get`
(`AviPlayerReadyGet @ 0x47ed80`, `record.*`) is the movie object's
byte-safe readiness byte at `+0x99` (the sleep-free AVI loop's gate,
finding 185), and `game_file_system_type_get` (`GameFileSystemTypeGet
@ 0x47fe20`, `record.*`) the system-kind word at the game-file handle's
start that picks WAD versus standard operations. `block_test` drives
the split corners; `record_test` pins byte isolation.

**Constant, stub and DAT-global band** (2026-10-24; findings 21/380/383/
386/392/399/406/440/455): the pure pool's fixed-return leaves land where
their roles live. In `src/gods98/crt.*`: the zero float (0x408930) and
the DirectSound minimum-volume `-10000` dword (0x47c160, corpus view
`0xffffd8f0`), the zero processor-feature fallback (0x490230) and the
FLH magic 'UU' silent leaf (0x456210), plus the two module-backed DAT
globals modeled on `message.cpp`'s `s_system_state` precedent — the
meaning-unresolved float field (0x4230b0, findings 295/392) and the
store-and-return global (0x4567c0) whose corpus writers are all
unported, so the storage answers its initial value until they land.
`always_accept_place_command_cell` (0x432550, `level.*`) is the kind-
0x16 command's unconditional cell-eligibility callback (finding 455),
and `flic_opcode25_success_stub` (0x484b90, `flic.*`) names the 8-bit
opcode-25 success stub (finding 21 — no pixel writes; the shipped 16-bit
coder's unsupported-coder note stands). Corpus-ignored arguments keep
the definitions `(void)`; x87 float10 stays a documented double
deviation. `crt_test`, `level_test` and `flic_test` drive the band.

**The indexed inline-block copy** (2026-10-24; findings 372/431):
`indexed_record_transform_copy_in` (`IndexedRecordTransformCopyIn @
0x483240`, `record.*`) is the copy-side twin of the data-address reader
above — 0x50 bytes (the corpus's twenty-dword loop, byte-equivalent to
the contiguous store) into the indexed record's inline block at
base + 0x10 + index*0x70, always answering success. That closes the
pure-leaf pool: the scan's ten remaining named rows are OS- and
x87-register-bound pseudocode and land in `blocked.md` as the
OS-bound/register-word residuals (MMIO and find-handle closes, the three
`IsBad*Ptr` probes, the four x87 control/status readers, and the
PE-unwind address resolver). `record_test` pins the block placement,
the header edge and the deviation corners.

**Tracked cell-flag set family** (2026-10-24; terrain wave): the eight
voxel-record setters the earlier cell-flag cluster left behind —
`terrain_cell_flag100_set_and_track` (0x432100), the visited mark's
setter and drainer (`terrain_cell_visited_flag_set_and_track` 0x432270,
`terrain_cell_visited_flags_clear_all` 0x432300), the mask-clear/
set-refresh leaves (0x432360/0x4323f0/0x432450/0x432490) and the
0x40000000 clear-or-set (0x432560) — land in `level.*` as faithful
word ops on the same `level_voxel` record the flag getters read. The
corpus's two tracking queues (the refresh-track and visited pair lists
with their counters) land as module statics bounded silently — the
corpus writes past the end of its DAT regions, and the queues'
consumers (the corpus's queued-work transition and refresh dispatch)
stay unported; the per-cell CellInitPass refresh call is a
TODO(corpus) seam, as `level.cpp` documents. `level_test` proves the
visited queue really queues (the drainer clears both listed cells),
the masks OR and clear exactly their bits, and out-of-cell setters
skip (bounds deviation).

**Descriptor coordinates and the neighbor compass** (2026-10-24): the
grid-pick forwarder's real body lands with it —
`world_to_terrain_cell_bounds_check` (`WorldToTerrainCellBoundsCheck @
0x44f700`, `record.*`) truncates a world point to cells, checks
`-1 < c < dim` and answers the subcell coordinate folded past one by
the lowest-pair rule; `grid_coordinate_clamp_and_store` (0x44f810) is
the clamped-store twin; `terrain_world_point_to_cell` (0x44f870) drives
both, with the corpus's own field-reading mismatch reproduced (scale
read as float by the offset path, converted as int by the center
formula). `view_grid_edge_get` (0x435f28, `view.*`) is the faithful
getter of the tile-edge clamp DAT global its ported writer already
owns. `cell_neighbor_direction_code` (`CellNeighborDirectionCode @
0x4508a0`, `level.*`) is the pure eight-neighbor compass (-1 for
non-adjacent). `record_test` hand-computes the subcell fold, the
clamps and the zero-width below-zero clamp (faithful arithmetic);
`level_test` walks all eight directions.

**The refresh-side terrain leaves and the record twins** (2026-10-24;
findings 354/373/450 wave): the companions that round out the cell-flag
families land in `level.*`. `terrain_neighbor_flag_refresh` (0x431450)
refreshes a cell's activity-build record — its +0x18 word, but the
record's owning release family is unported, so every port case reads
zero and the activity branches stay TODO(corpus) seams; this leaf had
only been quoted in earlier notes, so the ledger row is now truthful by
code. `terrain_cell_flag20_clear_and_refresh` (0x4310f0) runs that
refresh, drops primary bit 0x20 and hits the shared refresh seam;
`terrain_cell_edge_flag_set_and_update` (0x42f2e0) raises or clears the
0x200 edge flag with the same seam; `terrain_cell_work_flag_circle_set`
(0x435010) walks the filled circle of a radius about a center through
the ported in-gate work-flag setter, faithful to the corpus's signed
candidate bounds and wrapping radius math. The two pure record twins
`indexed_flag_value_set_one` (0x44e6b0 — dword 1 at +0x40b4 + (idx &
0xff)*4) and `scene_object_resource_handle_set` (0x44f220 — the handle
store at +0x2c) land in `record.*` beside their reader twins.
`level_test` proves the circle lights exactly its cross, the edge and
0x20 flags raise and drop exactly their bits, and the refresh leaf is
inert without build records; `record_test` pins the slot masking and
the handle placement.

**Timestamp dedup and the terrain dirty queue** (2026-10-24; findings
246/374/451): two DAT/context stores with unported owners land as
module statics in `level.*`. `cell_coordinate_timestamp_remember`
(0x40ee00) drops the coordinate pair and the current timestamp global
into the first free record of the corpus's 1000-entry dedup table —
the timestamp word has no ported writer yet, so stamps store zero —
and `cell_coordinate_timestamp_exists` (0x40efb0) answers exact-pair
matches; a full table stores nothing (faithful).
`terrain_cell_dirty_queue_push_and_clear` (0x450960) is the terrain's
dirty-cell queue over the physical block table: pushing raises byte
+0x19's 0x10 flag and appends the pair, clearing scans the queue and
swap-removes the found entry (the corpus's not-found and empty-queue
paths answer early with the flag left set — faithful). The queue
landed as a module static (the corpus's context offsets have no
ported owner) and `terrain_cell_dirty_flag` exposes the byte read the
contract needs. In the same audit pass the simtick marker callback
stopped quoting `FUN_00450f30`'s address — that geometry-builder leaf
was claimed ported by a TODO quote alone, so the quote is gone and the
row is honest (it lands with the terrain-data wave). `level_test`
drives the queue's push/swap-remove/flag corners and the dedup table's
fill and overflow.

**The tile-variant grid-record table** (2026-10-24; findings 356/377/380/
451): six leaves over the corpus's fixed 300-block record region land in
`level.*` as one static-backed table — the region's loaders are
unported, so its variant-pointer words read zero and every
record-dependent branch answers the default records (the timestamp-table
precedent). `grid_record_flag_mask_clear` (0x40ab30) sweeps the region
dropping mask 0x10c from each block's field words; `terrain_cell_record_get`
(0x40af90) resolves a coordinate through its block to the variant-pair
fields, reading the default records in port cases;
`cell_flag4_is_set`/`cell_flag100_set`/`cell_flag100_is_set`
(0x40b250/0x40b2a0/0x40b2e0) raise and test the per-field flag bits;
and `grid_cell_record_present_check` (0x40e460) tests the heap present
table, which stays NULL until the grid-initialize leaf lands
(documented deviation, answered as zero). All corpus blind indexing
becomes guarded, reporting the unaddressable cell as zero. `level_test`
proves the per-cell-per-field flag isolation, the sweep's in-band drop
and field-16 keep, and the NULL-safe record reads.

**Corner normals and surface-type config** (2026-10-24; findings
236/321/451): `terrain_cell_normal_sample` (0x4512d0) reads a cell's
corner float triple through the corpus's selectors — corner 0 from the
block's first triple, corner 2 from the +0x0c triple, corners 1/3 the
normalized sum, with the block byte +0x19 bit-0x1 parity raising the
selector first. The port's physical blocks carry no loader-fed corner
floats (the corpus's vertex builders are unported), so the axis corners
read zero and the diagonal selectors normalize the zero vector — the
faithful answers under the same loader-less conditions. The surface-type
config leaves `terrain_surface_type_name_get` (0x435170) and
`surface_type_configured_value_at_index` (0x435190) sit over the config
loader's tables, which land NULL/zero until that loader (unported) lands,
so names answer the missing-config literal and values read zero.
`level_test` pins the selector shapes, the zero-vector normalization, and
the guarded fallbacks.

**Neighbor-coordinate classification** (2026-10-24; findings 445/451
wave): `terrain_neighbor_coordinate_check` (0x42f350) — the corpus's
cardinal-neighbor probe — lands in `level.*`. It walks north, east,
south, west in corpus order, marking bit *i* when the neighbor's flag
word carries the occupied class `0x20000000` or the work flag `0x400`,
then reads a kind nibble and a return code from two sixteen-entry
tables (mode 0 answers `0x6x`/`99`/`100` codes, mode non-zero the
`0x7x` pair). The corpus's second probe dereferences out-of-bounds
cells, so off-map neighbors answer zero (the module's bounds
deviation). `level_test` pins the probe order, the flag-or-occupied
equivalence, the mask-15 and mode codes, and the corner's two-probe
mask.

**Unowned DAT-global accessor band** (2026-10-24; findings 357/383/386/
440): eleven single-address engine accessors land as static-backed
module leaves — none of their globals has a ported owner or feeder, so
each is a zero-initialized static (the `s_system_state` precedent), and
the write-only ones get tests/diagnostics peeks. The counter pair
`global_counter_004ff990_increment`/`_reset` (0x454880/0x454890) and the
three counter bumps `global_counter_at_00555324`/`880`/`890` live in
`crt.*`; so do the two `+1.0` floats (0x462830/0x462850) and the two
float accumulators `global_float_accumulator_00555330_add`/
`global_float_accumulate_00555120` (0x4628a0/0x4628c0, whose callers
findings leave unresolved). The system-state pair
`system_state_flags_get`/`system_state_flags_set` (0x456240/0x456550)
land in `message.*`, whose own `s_system_state` models `DAT_004ff980`:
the getter reads it directly, the setter installs a pointed-to value
and returns it (NULL answers zero, deviation). `crt_test` proves the
band's independent one-per-call bumps; `message_test` proves the
round-trip and the shared static.

**Grid-record count tables** (2026-10-24; findings 326/464): the corpus's
two adjacent count tables — 300 cells (the 15-column grid-record
convention) times sixteen slots, `row * 15 + column`, then `* 16 + slot` —
land in `level.*` as zero-initialized statics: the entity create/teardown
and stats-attach writers are modeled without feeding them, so every count
answers zero in port cases (the findings likewise leave the exact
meanings unestablished). `grid_record_count_tables_clear` (0x437440)
sweeps both tables, `grid_record_count_get` (0x4374a0) reads the first,
and `grid_record_count_difference_get` (0x437460) answers the second,
optionally minus the matching first-table slot; all three bound-guard
the corpus's blind indexing. A tests/diagnostics peek
(`grid_count_table_slot_peek`) keeps the statics observable.
`level_test` pins the zero band, the table-edge guards, and the sweep.

**Registry and selection-context accessors** (2026-10-24; findings
292/376/456): two unowned-global pairs land as faithful static-backed
leaves. The object-pointer registry — the corpus's ten-slot entity
pointer table and its count — lands in `entity.*`:
`object_pointer_registry_clear_match` (0x42dfc0) zeroes the matching
entries (a no-op while empty) and `object_pointer_registry_get_slot`
(0x42dff0) answers a slot only when it is below the count and non-zero.
The object-list parse that feeds it is modeled without pushing, so the
count stays zero and no lookup fires (documented deviation; a count peek
keeps it observable). The selection-context pair
`current_selection_context_get`/`_set` (0x417fe0/0x417ff0) lands in
`frontend.*`: its corpus writers leave it unset in port cases, so the
context answers NULL, and the two leaves round-trip. `entity_test` and
`frontend_test` pin the zero behavior and the round-trip.

**Message-queue flag leaves** (2026-10-24; findings 376/441): three
corpus set/clear leaves land in `message.*`, writing the module's own
storage — `selection_flag10_set_clear` (0x419450) raises or drops bit
0x10 of the flags word (`DAT_004dcd48`, so its neighbours survive),
while `info_message_always_play_flag_set` (0x419470) and
`indexed_selection_flag_20000_set_clear` (0x4194f0) raise or drop the
per-id config record's 0x10000 / 0x20000 flag bits on the config entry
the corpus keeps as the flag dword of its 0x14-stride record (finding
441). The corpus indexes its config records blindly; the port
bounds-checks, and flags peeks keep both storages observable.
`message_test` pins the bit isolation, idempotent raises, and guarded
ids.

**Input flag-8 leaves** (2026-10-24; findings 376/444): four set/clear/get
leaves land in `input.*` over three zero-initialized DAT tables with a
documented 256-entry window — the corpus sizes them through heap
allocations its allocator owner (unported) never hands over. The
input-entry leaf pair `input_entry_flag8_set_clear`/`_get`
(0x41c9f0/0x41ca60) raises, drops, and reads bit 0x8 of the indexed
entry flag word; the player pair `player_input_flag8_set_clear`/`_get`
(0x41ca20/0x41ca70) does the same through the kind-1 or kind-4 player
table, answering zero for other kinds (faithful — the corpus picks its
table by kind and ignores the rest). Writes outside the window skip,
reads answer zero (bounds deviation). `input_test` pins the one-bit
round-trips, table independence, and the guards.

**Entity record-field leaves** (2026-10-24; findings 354/476): three
plain record writers land in `record.*` with the corpus's offsets over
opaque records — `record_fields_4_and_14_set` (0x435ef4) stores the two
supplied dwords at +4/+0x14 (its read twin is the ported
`record_field14_get`), `entity_fields_a0_a4_store_a8_clear` (0x435f32)
stores +0xa0/+0xa4 and clears +0xa8 (the corpus's own clear), and
`entity_b8_bit3_set_or_clear` (0x435d91) raises or clears bit 3 of the
+0xb8 byte. The findings leave the field meanings unresolved; the port
keeps the write shape with NULL records skipping (deviation).
`record_test` proves the round-trips through the caller-owned buffer,
the bit isolation, and the NULL skips.

**Selection-state and context-selected accessors** (2026-10-24; findings
313/324/380/478): six selection leaves land across the two domains. The
help-window state pair lands in `frontend.*`:
`selection_state_flag_clear_bit0` (0x4181a0) drops bit 0 of the
selection-state word, and `selection_state_handle_get` (0x419150) walks
its mode/index gates over the strided handle tables and the extra
handle. No ported code feeds that band — the resource-positions apply
and button hit test are unported — so everything starts zero inside a
guarded 16-entry window (the corpus indexes blindly), with set/peek
views for tests. The context-selected pair lands in `entity.*`:
`context_selected_global_pointer_get` (0x4295a0) picks one of two
zero-init pointer statics by the context mode, and
`entity_matches_current_selection_or_owner` (0x4295c0) matches an
entity or its +0x36c owner against the current selection — both read
the mode and selection globals through the media layer's models, fed
by the ported sound-minimum-distance leaf. `frontend_test` pins the
gates and table switches; `entity_test` the mode selection and the
predicate's rejection paths.

**Command-queue adjacent leaves** (2026-10-24; findings 06/10/374/386/468):
three queue-side leaves land in `cmd.*`, reading the module's own
modeled state. `cmd_queue_cancel_match` (0x451f60) runs the corpus's
cancel sweep over the active bank — swap-with-last removals whose
forward cursor never rechecks a swapped-in match (kept faithfully; the
findings' "matching second field" is compared through the port's
pointer-sized store). `active_context_record_get` (0x452580) hands out
the active selection array and count through the cmd context, answering
zero/NULL while nothing is selected, and `cmd_array_find_value_index`
(0x452650) is the pure linear value search — matched index written,
NULL arrays skipped (deviation). `cmd_test` pins the sweep's swap-skip
count shape, the empty-context answer, and the search edges.
`caseinsensitive_name_table_lookup_at_offset8` (0x451fd0) stays open
with its table feeder `MessageStringTableInitialize` — both scan the
corpus's built-in `Message_*` name table (blocked: reference-binary
name tables).

**Selection-candidate leaves** (2026-10-24; findings 77/85/468): three
candidate leaves land on the port's own selection storage.
`selection_contains_entity` (0x452610, materializing the dedupe the
add-path used) scans the selection array below the count — NULL context
answers absent. `selection_candidate_already_selected_or_marked`
(0x452690) answers through that scan or the +0x3ec mark bit (the corpus
reads the word as a byte). `selection_candidate_eligibility_check`
(0x452870) runs the corpus's gate order in `entity.*`: the +0x3e8
state-bit-0x100 gate, the ported shared-eligibility predicate, the
+0x3e4 low-byte `& 10` band, the +0x3e0 0x24000000 band, and type 1's
extra +0x3e0 bit-0x40000 rejection. `select_test` pins the scan, the
mark path, and every eligibility band.

**Camera-selection leaves** (2026-10-24; findings 102/339/378/424/426):
four follow-view camera leaves land in `view.*`.
`camera_view_context_set` (0x435e0e) stores the four view-config globals
(the pan cap, follow scale, pan scale, edge clamp); the pan globals start
zero — the startup config seam is unported, so the pan is inert until
configured. `selection_record_point_set` (0x436b0b) writes the follow
camera's x/y (z zero) for follow-mode views, and
`camera_get_position_with_terrain_height` (0x436b33) copies that camera
and replaces its z with the terrain height — the corpus's world-handle
argument routes through the engine context's bound level block
(deviation), constant success. `camera_pointer_pan_update` (0x436b87)
normalizes the move, compounds the per-view accumulator (a modeled
+0x9c field) with the pan scale, caps it, and accumulates the scaled
move into the follow offsets — the zero-length case goes NaN through
the corpus's unguarded normalize, as shipped. `view_test` pins the
gates, the cap clamp, and the NaN shape.

**Selection snapshot/dispatch + the display filter tuple** (2026-10-24;
findings 06/10/282/425/468): `selection_snapshot_create` (0x452520) copies
the selection through the port's heap model — nonempty answers count plus
pointer copy, empty answers zero without writing (the corpus's dead
byte-copy tail is absent; allocations are pointer-sized).
`selection_command_dispatch` (0x452d60) gate-first dispatches the
sound-minimum-distance leaf: the current-selection context's flag bit
plus type-2/owner gate, else a selection scan that swaps through the
owner — the port models the owner as an EntitySlot +0x36c mirror field
(pointer-sized deviation). `display_resolution_filter_tuple_append`
(0x49d870) lands in `crt.*` over zero-initialized statics: corpus-shaped
unbounded counting, guarded 16-entry writes; the seeding DirectDraw
initialize and the match read stay blocked on the COM/display seams.
`cmd_test` pins the snapshot copy, the dispatch gates and indirection;
`crt_test` the seeded shape and overflow.

**Random-cell producer + entity remove** (2026-10-24; findings 06/10/468):
two queue producers land in `cmd.*`. `entity_random_valid_cell_command_create`
(0x452430) samples up to 1000 random cells through the port's module-local
15-bit LCG and queues operation 0x24 with the first walkable cell the
bounded bit-8 helper accepts — coordinates by value (the corpus threads
a stack pointer, deviation) and an absent level skips (the corpus would
modulo zero). `selection_entity_remove` (0x452c50) scans the selection
array, swaps the last entry into the vacated slot, and clears the
selected bit on both flag mirrors; the index-zero HUD post is the
unported HUD-chain seam, and it answers 1 only when it removed.
`cmd_test` pins the no-cell rejection, the queueing draw (hand-built
voxel block), the guard skip, and the swap-removal shape.

**Selection filter + state monitor** (2026-10-24; findings 06/10/468): two
selection-state leaves land in `cmd.*`. `selection_candidate_filter`
(0x4526e0) follows the +0x304 attach mirror (a new EntitySlot field),
runs the ported eligibility and already-selected leaves, and appends the
candidate below the corpus's hundred-entry cap with the +0x3e8 selected
bit — the marker/HUD reset, activity/action resets, type-2 turn issue, and
visual-resource route are documented unported seams (the insert is the
observable half). `selected_entity_state_monitor` (0x452cc0) runs the
corpus's counter-free loop — `count` passes over the FIRST selection entry
(kept faithfully) — clearing the attached owner's +0x3e4 bit 3 and owner
word when the stats word's bit 0x20000 says so; the per-type teardown is
the entity-layer seam. `cmd_test` pins eligibility, the append and cap,
the visual success half, and both monitor shapes.

**Band-mode transition + timed-effect/position leaves** (2026-10-24;
findings 250/228/05): four UI-band one-liners land in `frontend.*` over
the selection-state statics. `ui_mode_transition_request` (0x418fc0) is
the mode-word transition — the zero-second-argument arm goes to mode 4,
mode 4 hands over when the third argument is clear (raising the band's
bit 5), and a same-index request just raises the bit.
`selection_mode_set_by_index` (0x418f80) is the two-entry switch raising
the timed-effect update flags for entries 8/9.
`timed_effect_update_request_set` (0x46b770) raises an entry's bit-2 flag
inside a guarded 16-entry window — the 0x27c-stride band storage stays
with the unported update/timestamp family.
`ui_mode_resource_positions_apply` (0x419100) walks the two-entry help
layout table (zero-init statics; the layout loader is unported) and the
reposition hook per found handle is the simtick-cited surface seam.
`frontend_test` pins the arm/hand-over/same-index shapes, the bit-5 and
flag windows, and the positions pass.

**Surface-type writers** (2026-10-24; findings 226/450): the writer pair
over the per-cell surface byte lands in `level.*`.
`terrain_cell_surface_type_set` (0x44f4e0) returns the prior byte and
applies the type's appearance triplet from the shipped table — the
reference data's six real entries (white/half-gray/brown/green/blue/
yellow); larger types read undefined neighbours in the corpus, white
substituted (deviation). `terrain_cell_surface_type_reset` (0x44f570)
applies white and clears the byte, and
`terrain_cell_surface_appearance_apply` (0x44f5c0) is the leaf itself:
mode nonzero hands the triplet to the scene mesh color seam (no port
state), mode zero also clears the byte. All three guard the bounds the
corpus indexes blind; the setter's own write lands before the appearance
half, like the corpus. `level_test` pins the prior-value return, the
round-trip, and the guarded corners.

**Coordinate-callback dispatchers** (2026-10-24; findings 451/362/465):
three dispatch leaves land in `level.*`.
`terrain_cell_refresh_dispatch` (0x402ae0) resolves the current level's
terrain handle through the modeled level block and clears the cell's
surface type — the corpus's stack-thrown coordinate pair is read by
value, and the missing level/terrain skips (deviations); the coordinate
callback rides the entity-pool registry, an unported seam.
`coordinate_callback_dispatch` (0x402b30) and
`coordinate_callback_dispatch_by_mode` (0x402b60) are the pair-wrapping
dispatchers whose whole observable body is that callback machinery —
the port records the seams and changes no state. `level_test` pins the
refresh half's clear and guards and the seam no-ops.

**Record-queue validator + entity schedulers** (2026-10-24; findings
362/465/476): three entity-domain leaves land in `entity.*` / `cmd.*`.
`entity_flag40000_threshold_compare` (0x437cc0) is the validator — the
state flags (stats +0x148) carry bit 0x40000 and the nested float at
+0xe8 falls below the entity's own +0x340 bound.
`entity_command15_validated_record_queue` (0x402670) allocates a
type-0x1a record for the validated entity, links it at kind 0xf, stores
the entity at +0x10, and raises the +0x5c bits 6/14/18 after clearing
bit 4 — the corpus's opening CurrentLevelGet touch changes nothing (the
port's link lands it on the task list). `entity_deferred_work_schedule`
(0x4026c0) gates on the +0x304 attach mirror and the +0x3ec bit-0x100
flag, searches the pool through the unported callback seam (always
misses), and creates the fallback task. `cmd_test` pins the fallback
creation, both gates, the queue record's shape, and the validator
closure.

**Command-registry accessors** (2026-10-24; findings 10/466): five
registry leaves land in `cmd.*`. `cmd_mark_matching` (0x402000) walks
the task list raising bit 16 when the type matches plus either the
payload's low word (the corpus's 32-bit view) or the record's x/y dwords
equal the pair. `cmd_set_kind` (0x402060) records the kind at +0x4c and
binds the behaviour — pinning 99 when the record carries the 0x30000
marker bits. `priority_entry_order_value_set` (0x402090) writes the
behaviour/order table at the kind slot (guarded, deviation),
`priority_entry_state_set`/`_get` (0x4020a0/0x4020b0) land the 27-entry
per-priority state table (zero-init static; the registry initializer
seam) as guarded access. `cmd_test` pins the round-trips, the binding
branches, and both marking routes.

**Registry flush, heap wrappers, and batch issue** (2026-10-24; findings
10/393/466): six leaves land in `cmd.*`. `cmd_flush_all` (0x402100) takes
the disable mode on bit 1 of the registry word and the full flush at
zero — the entity callback sweep is the unported registry seam (inert),
and both list heads and the 27-entry priority-state table drop (the
corpus's cell-record counter block is unmodeled registry data,
documented). `engine_memory_allocate`/`engine_memory_free` (0x4021e0 /
0x402210) are the engine-heap wrapper pair, keeping the corpus's
discarded-return shape. `handle_array_release_and_reset` (0x4020c0)
walks the 12-dword handle array — no ported feeder, so a zero-init
static and an inert release — then zeroes the companion counter and the
registry word. `entity_command14_batch_issue` (0x402800) issues `count`
(or one for zero) kind-0x14 records, raising bit 6 in the zero case.
`level_no_gather_object_setup` (0x402170) chains the batch, the
kind-0x17 gather record with bit 6, and the kind-0 head record with bits
6/7/10 and bit 4 cleared. `cmd_test` pins the flush halves, the batch
flags, and the setup chain.

**Cancellation ref and conditional issuers** (2026-10-24; findings
10/466): four command leaves land in `cmd.*`. `cmd_cancel_ref` (0x402cd0)
is CmdFree's side effect — a type-0x13 record's cancel rides the
pool-callback seam, any other kind walks the task list raising bit 5 on
the type-0x13 record it references (the new +0x58 reference slot; the
shipped binary's spawn-dispatch switch populates it, the port stages it
in tests). `entity_conditional_task_schedule` (0x402ec0) gates on the
capability word's bit 0, the +0x3e0 bit-0x400 block and the +0x3ec
bit-0x10000 latch, then links the type-0x1e task at kind 0x11 raising
bits 4/20. `entity_threshold_progress_event_queue` (0x402f30) links the
type-10 record with the +0x3c word 0x10 at kind 7 — or kind 0x13 with
bit 21 when the flag is set. `entity_target_command_queue` (0x402f70)
chains the type-0x11 target record onto the entity chain through the
walk tail, touching no list head and no timestamp. `cmd_test` pins the
marking walk, both free paths, the three gates, both kinds, and the
chain placement.

**Selection-record array and the entity-kind issuer** (2026-10-24;
findings 06/10/362/466): four leaves land in `cmd.*`.
`entity_record_array_apply_zero_state` (0x402600) is gated by the
selected-entity presence test: it raises the record's bits 6/8, takes the
selection snapshot into the record's owned +0x40 buffer with the entry
count at +0x44 (so cmd_free releases it), and walks the entries — whose
per-entry action-sequence reset and attached-object detach belong to the
unported entity-teardown family (the seam half).
`entity_kind_command_issue` (0x402960) is the once-per-entity command:
the +0x3e4 bit-0x10 latch, the type-0x12 record, the +0x3c word chosen
from the capability bit 0x100 and the stats bits 0x1000000/0x2000000,
bit 20 under the capability bit 0x40 with the +0x3e0 bit-0x400 gate
clear, and the head-link at kind 0xc. `coordinate_pair_callback_dispatch`
(0x402a40) and `argument_callback_dispatch` (0x402aa0) are the pair- and
argument-wrapping dispatchers whose observable body is the pool-callback
machinery — recorded seams. `cmd_test` pins the guard, the snapshot
ownership, the five +0x3c arms, and both bit-20 shapes.

**Terrain cell-command issuers** (2026-10-24; findings 451/466): the
cell-command family lands in `cmd.*` over the ported terrain predicates,
with the pair stored in the record's +0x08/+0x0c slots as integers (the
corpus's union with the move records' floats — read through an int view)
and the action word / -1 target / zero count beside them.
`terrain_cell_command_create` (0x402510) chains the state-1 and state-6
and in-bounds flag-bit-4 guards, then the type-8 record, the caller's
bit 3, the record-array half, the kind-3 link, bit 7 (with bit 13
additionally), and — with the caller's flag clear — the surface-type
apply and the cell's bit-0x800000 mark. `kind_nine_cell_command_issue`
(0x402d50) requires the state byte to differ from 1, the walkable flag
and a clear reservation bit, issues the type-9 record with action 8 and
bit 10, and marks the cell occupied before the surface apply.
`cell_command_type13_issue` (0x402e40) gates on the bounded flag-bit-8
read; `cell_command_type11_issue` (0x402fb0) needs no guard and raises
the cell's secondary bit 3. The counter update after the link belongs to
the unported spatial-query family and the cell-record counter block, so
it is recorded as a seam in `docs/coverage/blocked.md`. `cmd_test` pins
the guard chains, the record shapes and kinds, and both terrain marks.

**Category-mask task gate and issuer** (2026-10-24; findings 327/449/465):
two leaves land. `entity_flags_contain_requested_single_bit` (0x438ed0,
`entity.*`) accepts the selector 1/2/4/8/0x10/0x20 exactly when the state
flag word's corresponding bit 4/8/0x10/0x20/0x40/0x80 is set — the
corpus's chain of word re-reads, collapsed to the equivalent switch.
`task_type23_create_for_category_mask` (0x402890, `cmd.*`) issues the
type-0x17 task record through that gate with the selector kept in the
+0x3c word, runs the record-array half on the caller's argument, and
links at the category kind the selector maps to — 1/2/4 to 0xc, 8 to 3,
0x10 to 7, 0x20 to 4 — clearing bit 6 after the link. `cmd_test` pins
the gate arms, the full selector-to-kind map, the out-of-map refusals,
and the record-array bit 8.

**The task-kind filtered search** (2026-10-24; findings 378/381/449):
three leaves land in `cmd.*`. `entity_action_kind_supported` (0x43a7c0)
gates on the stats word's bit 3, then answers the action kinds 9, 8,
0xb and 0xd unconditionally, kind 7 through the upgrade-resource
counters (selector zero reads counter A, anything else counter B), kind
6 through the callable no-op — whose return is corpus-undefined, so the
port treats that arm as always missing — and misses everywhere else.
`task_entity_flags_and_cell_task_match` (0x439240) is the same gate as
the pool-search callback, forwarding the kind. `task_kind_filtered_search`
(0x439200) skips selectors 1, 2 and 4, then walks the entity pool — table
order, ascending index, live slots — through the callback. `cmd_test`
pins the gates, the four unconditional arms, both counter selectors,
the no-op miss, the excluded selectors, and the pool walk's dead-slot
skip.

**The task-record queue producers** (2026-10-24; findings 366/369/465):
ten leaves land in `cmd.*`. The pair-queue family (`
coordinate_pair_event_queue` 0x4030b0, `coordinate_pair_task_record_queue`
0x4033f0, `task_pair_record_enqueue` 0x4034e0) stages the coordinate pair
as the corpus's integers; the scalars ride `scalar_task_record_queue`
(0x4034a0) and `task_record_kind10_queue` (0x403520) tail-appended with
bit 6 raised after the link; `object_event_type2_queue` (0x403190) runs
the record-array half with bit 4 up and bit 6 clear; and the timed pair
(`timed_task_record_initialize_and_link` 0x4038c0, which returns its
record, and `task_record_kind11_timed_queue` 0x4039d0) zeroes the
timestamp slot and raises the corpus's flag blocks. The registry walkers
remove the first kind-0 pair match and every kind-10 entity match,
unlinking and freeing through the cancel-ref path. `cmd_test` pins the
ten shapes, both removal walks, and the empty-chain guards.

**The entity command-chain housekeeping band** (2026-10-24; findings
369/465): seven leaves land in `cmd.*`. `entity_command_chain_head_remove`
(0x4041e0) pops the chain head — pending-undone heads ride raw onto the
global head list, bit-16 undone ones (the corpus's `test $0x20,%ch`, read
as 0x2000 by Ghidra) onto the tail list, everything else frees; the raw
moves stamp no timestamps and no flags. `entity_drain_command_chain`
(0x403a50) drains behind the enable word's bit-1 gate, head-linking the
pending, tail-linking the bit-16, freeing the rest; its pool-search tail
half is the registry seam. `command_queue_sweep_relink` (0x404100) sweeps
the chain, freeing bit-2 records, moving raw, and — the corpus quirk —
relinking every keeper through the head keeper's link slot, so
intermediate keepers orphan. The one-liners `entity_task_state_update_after_cleanup`
(0x403af0, with the kind-3 op-7 queue), `entity_pending_task_flag_clear`
(0x403b40), `task_record_kind14_remove_if_head` (0x403f60) and
`entity_task_chain_contains_kind` (0x4041b0) complete the band.
`cmd_test` pins the raw-move purity, the gate, the sweep's orphaning
relink, and the containment walk.

**The global-object field band** (2026-10-24; findings 295/324/392):
eight leaves land. The corpus's `DAT_005560d4` word-field arithmetic —
increment, subtract, add-with-argument, clear — comes over as seven
guarded mutators on the port's `LevelState` (`global_object_field8c_increment`
0x41f870, `global_object_field8c_subtract` 0x41f880,
`global_object_field98_add`/`clear` 0x41f8a0/0x41f8c0,
`level_state_field9c_add` 0x41f920, `global_object_field94_increment`
0x41f980, `global_object_fielda8_increment` 0x41f990 in `level.*`), the
`DAT_005560d4 != 0` guard becoming the NULL-level guard; and
`runtime_flag40000000_get` (0x41f970, `engine.*`) reads the
`DAT_00556ecc` state word's bit 30 through the threaded `EngineContext`.
`level_test` and `enginebind_test` pin the walks, the masked read, and
the guards.

**The mode-dependent field read** (2026-10-24; finding 03):
`mode_dependent_field24_get` (0x423080, `engine.*`) reads the corpus's
word field 0x24 — the follow-span slot — off the in-game view object in
mode 1 and the editor/preview view in mode 0, answering the mode word
itself otherwise; the read threads through the port's view table with
NULL slots answering zero. `enginebind_test` pins the selection, the
mode passthrough, and the guards.

**The engine-state band 0x425cd0-0x4263a0** (2026-10-24; findings
295/324/392): four leaves land in `engine.*`.
`current_engine_mode_is_zero` (0x425d80) reads the context-mode word
through the threaded `EngineContext`; `global_state_flags_bit7_set`
(0x4263a0) raises or clears cheat-word bit 7. The two DAT-global shapes
follow the unowned-global convention: `global_scroll_guard_set` (0x425cd0)
stores the scroll-scale guard global (the corpus's only reader is still
unported — a port-side peek stands in), and `global_triple_copy_out`
(0x426230) hands out the inert zero-init triple. `enginebind_test` pins
the mode reads, the bit-7 walk, the guard store, and the zero triple.

**The front-end flow words** (2026-10-24; findings 23/438): six rows land.
The corpus's five DAT menu-flow words come over as zero-initialized
module statics in `frontend.*` — `save_slot_open` 0x416d50 and `is_in_game`
0x416d60 test the `!= -1` sentinels (true at the load image's zero state,
faithfully, before the save/tutorial selections initialize), while
`quit_menu_probe` 0x416dd0, `level_start_flag` 0x416de0 and
`menu_options_flag` 0x416df0 read the flag words; a port-side setter
stands in for the unported callback bindings. `save_slot_get`
(SavGetSlot, 0x417e90, `savegame.*`) was already the port's model of the
slot word — cited here. `frontend_test` pins the boot state and the
setter walk, `savegame_test` the slot round-trip.

**The save-slot band** (2026-10-24; findings 21/408): three rows land in
`savegame.*`. `save_slot_selection_clear` (0x415cf0) drops the active slot
to the corpus's closed `-1` sentinel; `sav_slot_ctx` (0x417e50)
reproduces the corpus's 0..5 bounds test over the slot context block —
the port's one active-slot context stands in for the corpus's six
deviation, documented); and `save_slot_set` (SavSetSlot, 0x417ea0) was
already the port's model of the slot word — cited.
`savegame_test` pins the clear, the bounds, and the block answer.

**The message/sim one-liner band** (2026-10-24; findings 05/250/443/474):
five leaves land. The corpus's message-config DAT block — the stride-0x14
catalog read (`incident_message_text_catalog_get` 0x419420, finding 441:
the record's `+4` dword, not a template field, despite the corpus name)
and the secondary-sound store (`info_message_secondary_sound_set`
0x419680) — plus the incident-state word (`incident_message_state_get`
0x41a2f0) land as guarded zero-init storage in `message.*`: the block's
byte-read is all zeros in the shipped binary (finding 05), so the faithful
initial state is the zero block with the corpus's index math reproduced.
`ui_mode_resource_bit40_get` (0x4191f0, `frontend.*`) reads the
selection-state word's 0x40 bit through the band that models it, and
`simulation_global_word_set` (0x41ba90, `simtick.*`) is the corpus's blind
store onto the mute word. `message_test`, `frontend_test`, and
`simtick_test` pin the windows, the mask, and the store.

**The save-side accessor band** (2026-10-24; findings 08/214/250/383):
five leaves land across three modules. `osf_read_dirty_flag_get`
(0x4593d0, `savegame.*`) reads the OSF dirty word the save module already
models, and `checkpoint_blob_get` (0x4601f0) threads the checkpoint blob
pointer; `global_option_low_bits_enabled` (0x4589a0, `level.*`) gates on
the objective word's low three bits (the corpus's DAT_004ffbc0 →
LevelState.objective); and the two unresolved globals land as crt.*
zero-init storage — `global_flag_004ff978_get` (0x456640) and
`global_value_004ffe48_set` (0x458660), meanings unresolved per finding
383. `savegame_test`, `level_test`, and `crt_test` pin the reads and the
guards.

**The admission-predicate pair** (2026-10-24; findings 06/380/478): two
rows land. `main_sim_state_admission_predicate` (0x4191a0, `frontend.*`)
is the corpus's sim-admission gate — selection-word bits 1 and 2, the
0x40 bit through its leaf, and the message module's system-state word
clear — all three models already owned by the port, so no new storage
lands. `current_level_state_flags_contain_mask` (0x41f720, `select.*`)
ANDs the selected entity's `+0x3f0` level-state mask word through the
selection accessor; the word lands as a zero-init `EntitySlot` mirror
(both corpus readers documented — the ported gate and the unported
sim-interaction update) and an empty selection answers false where the
corpus dereferences NULL. `frontend_test` pins the gate's four walks and
`select_test` the mask, the empty-selection deviation, and the mirror.

**The unowned DAT word-store band** (2026-10-24; findings 37/256/392/408/419/440/478):
eleven corpus leaves land. Eleven DAT globals with no ported owner store,
clear, or toggle through blind one-liners; each lands as zero-initialized
module storage with port-side peeks standing in for the unported corpus
readers. `crt.*` takes eight — the advisor-word bit-0 read (0x401c20) and
its state-word hooks (findings 37/38, the advisor readers stay blocked),
the task-refresh flag pair `state_flag_toggle_when_zero` /
`global_task_refresh_flag_get` (0x407340/0x407360, finding 392), the
stored-value and config stores (0x409d40, 0x40f5e0 — the erosion-config
slot LevelLoad feeds, finding 419), the mode-zero flag (0x40ab10), the
rockfall table-base store (0x40bfc0), and the UI word's bit-0 clear
(0x40e7e0). `savegame.*` takes `save_state_event_context_set` (0x418110),
`frontend.*` the selection-mode word store (0x418190, beside the band
that models its neighbor), and `message.*` the deferred-callback presence
store (0x453900 — GameConfigInit seeds it from the message-resource
find). `crt_test`, `savegame_test`, `frontend_test`, and `message_test`
pin the stores, the toggle walk, and the guards.

**The app-mode + request-record band** (2026-10-24; findings 02/172):
eight rows land. `engine_app_mode_get` (0x401bf0, `engine.*`) reads the
app-mode word through the threaded `EngineContext`; the window-mode
extent pair (0x401c30/c40, `crt.*`) lands as zero-init statics with a
setter (the renderer-side store is unported), and `menu_wipe_end`
(0x410440, `frontend.*`) clears the wipe word's low two bits with the
corpus's start-side half unported. The three request-record predicates
(0x45ab30/60/90, `crt.*`) AND the flag word of the 18-record 0x30-stride
request block — the corpus's names read like setters but the bodies are
pure tests — a zero-init flag-word window guarded to 18 entries with a
port-side setter standing in for the unported panel-layer writers.
`enginebind_test`, `crt_test`, and `frontend_test` pin the reads, the
window guards, and the wipe clear.

**The record-field write band** (2026-10-24; findings 08/324/386): nine
rows land. Four record primitives in `record.*`:
`record_field18_set` (0x462ab0), `record_flags_24_or` (0x462ac0),
`object_field14_set` (0x45d8a0), and `linked_record_field4_clear`
(0x451c30) — the chain walker following each node's `+0x14` next link and
clearing every `+4`, the corpus's blind walk down to a guarded entry.
`checkpoint_blob_free` (0x4601e0, `savegame.*`) was already the port's
model of the corpus free — cited; the corpus quirk of freeing the global
and leaving the pointer dangling is replaced by the port's clear-on-free
(documented deviation). `level_objective_result_get` (0x458650, `level.*`)
threads the objective-result word LevelState already models. Three DAT
one-liners land as zero-init storage in `crt.*`: the unresolved store
(0x45c4a0) and the font-cursor table pair (0x45cc10/c60) whose feeder
(font slot loading) is unported — corpus index math reproduced, guarded
to a documented window. `record_test`, `crt_test`, `savegame_test`, and
`level_test` pin the writes, the walk, and the peeks.

**The selection-region band** (2026-10-24; findings 06/250/478): seven
rows land. Five selection/UI-state leaves go into `frontend.*` beside
the band that models the selection-state word: the coordinate quad
(0x41cce0 — the integer-to-float casts faithful), the five-slot region
store (0x41e9d0 — its 0.04 scale keeps the corpus's double-precision
multiply), the objective-selection reset (0x41a990 — clear, mirror the
region's +08 float, raise the 0x84 pair), the interaction-timer restock
(0x41ee60 — 25.0 and the bit-800 drop), and the mode-flag pair rule
(0x418800) applying to the selection word through the band's own setter.
The words land zero-initialized (deviation, no ported feeder).
`sav_build_ctx` (0x417e70, `savegame.*`) composes the port's slot
accessor and ctx probe the way the corpus's slot-open test does, and
`copy_two_dword_pair_to_outputs` (0x439930, `record.*`) splits a
two-dword field with entry guards. `frontend_test`, `savegame_test`, and
`record_test` pin the quads, the walks, and the rules.

**The camera-controller bounds band** (2026-10-24; findings 04/280/440):
eight rows land. Five camera-key leaves go into `view.*` as fixed-offset
record operations — the controller record's owners stay on the unported
scene seam, so only the field shape these leaves need is written:
`camera_tilt_bounds_set` (0x436831), `camera_scalar_bounds_set`
(0x436921), and `camera_distance_bounds_set` (0x436a18) raise the gate
bits in the `+0xb8` flag byte and store their bound pairs,
`camera_distance_set` (0x436a4a) is gated on bit 4 unless bit 8
overrides, and `camera_zoom_update` (0x436aa5) accumulates through the
gate. `cursor_screen_ray_context_reset` (0x435f16) drops the modeled
ViewContext pan slot when the cursor ray handler's dead-zone branch
fires. `incident_message_flag_set` (0x419ae0, `message.*`) walks bit 4
of the module's own flag word, and `cell_action_permission_get`
(0x41e990, `frontend.*`) permits every cell while the system-state word
keeps bit 4 clear and otherwise reads the 300-entry, stride-15 level
table — zero-init storage with port-side accessors (the corpus cell
writer and the level-start clear are unported). `view_test`,
`message_test`, and `frontend_test` pin the gates, the clamps, and the
window.

**The menu-tick wrapper band** (2026-10-24; findings 37/280/419/441/457):
five rows land. `menu_audio_tick_interval_reached` (0x412190,
`frontend.*`) asks the module-modeled shared draw whether the 15-bit word
lands on a multiple of 400 — the LCG constants and shift are the
corpus's. `sim_status_banner_word_get` (0x41b9b0, `frontend.*`) reads the
status-banner toggle word whose corpus feeder, the periodic status tick,
is unported — zero-init storage with a port setter.
`current_level_terrain_grid_pick` (0x433be0, `view.*`) is the per-level
pick wrapper: world root, x/y in, cell indices out, NULL outputs skipping
their stores. `entity_queue_for_rockfall_processing_if_eligible`
(0x43a9e0, `ai.**) is the corpus's tail call into the idle-registration
half — type gate in, deduped table land out, with the stats-flag and
tile probes documented deviations. The scene-list callback wrappers
(0x410070/0x4100b0) stay blocked: their callback bodies are bare code
thunks with no pseudocode. `view_test`, `ai_test`, and `frontend_test`
pin the pick, the table rule, and the draws.

**The stubs, wrappers, and word-clear band** (2026-10-24; findings 37/38/
155/393): nine rows land. Three pure-return stubs are ported faithful to
their emptiness — `flic_opcode27_fallback_noop` (0x484c90) for the
FLIC-internal fallback, `crt_no_op_stub` (0x491ff5) and
`crt_no_op_stub_variant` (0x497870) for the address-range variants.
`advisor_clear_active_state` (0x401b20, `crt.*`) drops the advisor word's
active bit through the band that owns the word. `ui_render_record_free`
(0x45daf0, `heap.*`) is the corpus's bare tail call into the engine heap
free; `scene_object_owner_set` (0x473da0, `record.*`) writes the owner
word at `+0x20`, entry-guarded. `grid_operation_argument_adapter`
(0x442540, `view.*`) rotates the grid-op arguments into the terrain-
height call and discards the float the way the corpus's void return
does, and `set_global_scalar_to_one` (0x40fec0, `view.*`) keeps the
corpus's discarded-ramp shape against the port's inert ramp model.
`objective_voice_alias_raise_commit` (0x454b70, `objective.*`) runs the
announcer with 1 and answers 1 — threaded through the port's block
(deviation). The WAD context-table reader (0x48bce0) stays blocked with
the mount layer. `crt_test`, `flic_test`, `heap_test`, `record_test`,
`view_test`, and `objective_test` pin the shapes.

**The pure math and record primitives band** (2026-10-24; findings
256/280/386/393): eleven rows land. The record module takes the fixed-
offset primitive pair — `indexed_record_field64_set` (0x482350) stores
through the record's `+8` base pointer at base + 100 + index * 0x70,
`object_vector3_set` (0x40ff70) writes the vector block's slot-0 base
into `+3c…44`, the two int/uint-to-float pair stores (0x4851b0 /
0x4851d0) keep the corpus's casts at `+4/+8` and `+0xc/+0x10`, and
`object_state_pair_get` (0x485a00) reads the `+0x820` pair; all four are
entry-guarded deviations against the corpus's blind dereferences. The
view module takes `float_negate_absolute_value` (0x44d250, both corpus
branches negate), `vector2_set_length` (0x406760, zero-length guarded),
and `camera_zoom_step_adjust` (0x45bf20, ±3x steps through the iter-85
zoom gate). `byte_range_swap` (0x48e030, `crt.*`) keeps the corpus's
same-pointer shortcut, `engine_heap_allocate` (0x47ff30, `heap.*`) is
the corpus's drop-the-pointer alloc wrapper, and
`ui_mode_resource_bit40_set_clear` (0x419200, `frontend.*`) runs the
bit-0x40 flip rule over the modeled selection word faithful to the
corpus. `EngineCalloc` (0x48ead0) stays blocked with the CRT memory-
pressure handler seam. `record_test`, `view_test`, `crt_test`,
`heap_test`, and `frontend_test` pin the strides, casts, and gates.

**The heap retry-handler pair** (2026-10-24; finding 394): two rows
land. `crt_new_handler_invoke` (0x491d90, `heap.*`) dispatches the
attempted size to the corpus's registered allocation-failure handler
and answers whether it asked for another attempt — the corpus registry
word lands as zero-init storage with a port-side setter (the
set_new_handler seam that registers it is unported), and unregistered
answers 0 like the corpus's unset case. `engine_calloc` (0x48ead0) is
the corpus's retry-enabled allocation — the port's alloc-retry loop
driven through that dispatch — and resolves the skip the earlier sweep
queued for it. `heap_test` pins the dispatch, the zero-size quirk, and
the overflow gate.

**The DAT window band** (2026-10-24; findings 250/252/357/394): fifteen
rows land. Six data-section windows get the unowned-global treatment —
zero-init module statics with guarded entries and port hooks where the
corpus writers are unported. `crt.*` takes them: `indexed_pair_set/get`
(0x45a6e0/0x45a700) and `indexed_float_pair_set` (0x463730, the corpus's
uint→float cast kept), the text-metrics pair read/write through its
pointer window (0x463ea0/0x463ed0, set raising the corpus's dirty flag),
`region_bounds_set` (0x45d370)'s dwords-plus-floats quad, the
panel-flag pair `panel_item_flag16_set`/`panel_item_flag32_set`
(0x45aac0/0x45abb0) over the draw-record block's stride-0x2c items,
`panel_state_flags_toggle` (0x45ab00) walking the request-record flags,
and `reward_score_add` (0x462800) — count +1.0, score += value, faithful
to the corpus's `x - -1.0`. `menu_wipe_start` (0x4103f0, `frontend.*`)
arms through the modeled wipe-flag word and stamps the tick hook into
the clock float before submitting the corpus's alias-0x27 request. The
save band takes its commit wrapper: `sav_record_ptr` (0x417f90) answers
record + index*0x3190 + 4 under the open-slot and count guards, and the
already-modeled commit path (0x417fc0) gets its corpus citation.
`cmd.*` takes `cmd_cancel_by_coords` (0x403700): global-list walk,
type-7 records on the modeled zone word (+0x2c, the corpus's kind-7
issuer unported — accessors stand in), completing on the flag-0x20
state. `crt_test`, `frontend_test`, `cmd_test`, and `savegame_test`
pin the windows, guards, and folds.

**Renderer command node prepend** (2026-10-25): one frontier row lands
in `crt.*`. `renderer_command_node_prepend` (0x482ce0) builds the head
node of the model-render list that the ported `model_render_list_clear`
(0x482d30 twin) already walks: zero the node window, store the command
kind word at `+0`, copy the sixteen payload dwords when a payload is
given (zeros otherwise — the corpus leaves untouched fields either way),
link the old list head into the corpus's `+0x44` slot, and raise the
head. Deviations: the node window rides the widened host-word layout
(the owned-stream-buffer convention, the corpus allocates `0x48`) with
the next link widened to `void*` at the corpus byte slot (the
`model_render_list_clear` precedent), and the alloc is guarded.
`crt_test` pins the kind word, the payload copy, the zero fields of the
null-payload node, the two-node head chain through `+0x44`, and the
clear freeing the whole chain.

**Options slot-context window copy** (2026-10-25): one frontier row lands
in `options.*`. `options_context_copy_and_replace_resource` (0x4180c0) is
the save-menu's options hand-off: with a save slot open it frees the slot
context's owned resource word (the corpus's `DAT_00557080 + slot*0xb8`
block's dword `0xd` — through the ported heap free), copies the full
46-dword source block over the context window, and re-lands the source's
dword `0xd` as the new owned word (the copy makes the store itself
redundant, as in the corpus). Closed-slot and NULL-source calls touch
nothing (the corpus's own context probe gates the whole body). Deviations:
the corpus strides its active-slot context blindly; the corpus block has
no ported owner, so the window and the widened (`void*`) resource ride
zero-init own storage behind `options_slot_window_install` / peek hooks
(the unowned-DAT convention, the fixed-record-slot precedent for
pointer-valued corpus words). `options_test` pins the closed-slot gate
with the owned word still held, the full-window copy with the freed
resource, the guarded no-op, and the no-resource fold skip.

**Indexed record entry read + quad vertices write** (2026-10-25): two
frontier rows land in `record.*`. `indexed_record_entry_read` (0x4821a0)
reads one stride-`0x70` entry of the indexed record table through the
record's `+0x08` pointer — the `+0x04` field word, the shape-kind answer
(`3`), the `+0x00` vertex count, and the count `ushort` indices striding
through the entry's own `+0x08` index pointer (widened to `uint32` answers
as the corpus folds them). `terrain_cell_quad_vertices_write` (0x4823e0)
is the exact mirror of the ported indexed geometry copy: eight dwords per
round (3+3+2) from `36`-byte source slots into the block's `32`-byte
vertex stride starting at the `first_vertex` quad; each slot's ninth
dword stays home. A zero-count write answers inert. Deviations: guarded
table, index pointer, vertex buffer, and source (the corpus strides all
of them blindly). `record_test` pins the entry band (field `0x5a`, kind
`3`, three widened indices, the count-bounded write, and the zero-count
and missing-table gates) and the mirror write (slot-to-quad dword map,
the ninth-dword skip, the untouched first quad, and the guard band).

**Cell-list min-distance scale apply** (2026-10-25): one frontier row lands
in `level.*`. `cell_list_min_distance_scale_apply` (0x450a80) is the sound
min-distance color sweep: raising the applied bit (2) paints every queued
cell through the scene object's mesh color setter — the block index folds
the entry's `y` through the scale word plus `x` (all unsigned) and stores
the `0.3` triple into the block's `+34` window; clearing drops the bit and
repaints the same cells with the zero triple. Already-applied (enable) and
not-yet-applied (disable) calls touch nothing (faithful fold gates). The
corpus strides its cell-list context (the level block's `+10` pointer —
scale word, scene-object slot, flag word, queue window), which has no
ported owner, so the scale/object/flag ride zero-init own storage behind
`cell_list_apply_state_install` / peek hooks (the unowned-DAT convention)
over the port's module-static dirty queue, entry-for-entry the same
capacity as the corpus's. Deviations: guarded state (the corpus dereferences
its context blindly). `level_test` pins the fold index (block 11 for the
(3,4) entry at scale 2), the applied-bit gates and their in-fold passes,
and the clear repaint. Triage: the cursor-name pair (0x45c830 + 0x45cbd0)
and the type-2 dispatch thunk (0x43b850) joined `blocked.md` — the exact
`.rdata` string bytes of the cursor table and the queue-dispatcher thunk
family.

**Path arc interpolation + screen projection** (2026-10-25): two pure
frontier rows land in `view.*`. `path_arc_length_interpolate` (0x4068c0) walks
the curve record's arc-length table (count at dword 0, point pairs at dwords
2i−1/2i, segment lengths at dword `0x65 + segment` — the window the already-
ported length builder stores through `+0x198`), finds the segment whose running
total passes the travelled distance, and lerps the endpoints; exhausted travel
rides the endpoint and answers the record count. Deviations: the corpus strides
the record and both outputs blindly — the port guards them, and the length
reads reinterpret the builder's stored float bits. `view_point_to_screen_geometry_transform`
(0x45e940) projects four floats in place through the projection window: the
`+1c`/`+20` screen offsets come off first, the `+14 / +18` scale ratio folds
x, y, and depth, x lands through the half-size + center pair, y mirrors through
its own, and w rides the ratio (the corpus recomputes the ratio five times —
one divide is bit-identical). Deviations: NULL guards on both inputs.
`view_test` pins the segment folds (2.0, 5.5, 14.0 on a 14-length record), the
zero-travel and single-point edges, the guards, and the four-part projection
fold.

**Entity query count + terrain choice select** (2026-10-25): two frontier
rows land in `entity.*` and `level.*`. `entity_query_flag_count` (0x41b3c0)
answers catalog-kind queries: `0x13` returns the second catalog count word,
`0x14` counts the first catalog's rows whose `+148` stats word raises bit
`0x1000` or `0x2000` while the `+14c` bit 8 stays clear, `0x15` counts rows
with bit `0x800` raised or the `+14c` bit 8 raised, and any other query
strides the eight-byte id-pair table. The count globals and pair window have
no ported corpus owner (their loader band is unported), so they ride
zero-init own storage behind `catalog_count_install` /
`catalog_id_pair_install` hooks (the unowned-DAT convention).
`terrain_choice_resource_select` (0x40e700) reads the record's dword pair
and lets the first word select the active resource word: cases 1..4 read a
flat choice window at `(kind − 1) + index`, cases 6/7 read the scalar pair,
and anything else clears the selection flag's bit 0. A successful select
raises the flag's bits 0+1, stores the record pointer, and releases the
panel's text-output holder (a new `panel_text_slot_set` hook in `frontend.*`
models the corpus's global slot). Deviations: the corpus strides the window
indices blindly — the port guards the edge and answers clear. `entity_test`
pins the fold rules, the row-range edge, and the pair guards; `level_test`
pins every case arm, the guarded stride, and the default-clear path.

**Command sample stats + selection marker reset** (2026-10-25): two frontier
rows land in `cmd.*` and `select.*`. `command_sample_stats_accumulate`
(0x46b660) keeps per-kind sliding-window statistics: a null sample clears the
window head and the running sum, and a sample whose `+8` word exceeds the head
rebuilds the total as `(old total − old head) + new head`, while every call
adds the sample's `+4` word to the sum, raises the high-water mark, and appends
the sample pointer to the kind's log. Deviations: the corpus strides the
stride-`0x27c` slot block blindly, so the port guards the kind index and the
window, and stops the log stores at the slot edge — where the corpus's
twenty-first store would stride onto the index word and corrupt it (the port
keeps counting while it stops storing). `selection_clear_marker_and_reset_hud`
(0x44c4a0) raises the engine state word's `0x10000000` bit and the entity's
`+0x3ec` bit 1 for a marked entity whose stats word carries `0x20000`, but only
when the state-flags word's same bit is clear or the `+0x36c` owner is present,
then always runs the selection-clear command post and HUD context post. The
engine state raise threads the engine handle (deviation: the corpus strides the
global word). `cmd_test` pins the slide fold, the null-sample clear split
(head+sum, not the total), the signed no-slide fold, and the log-edge stop;
`select_test` pins all four gate branches and the inert null paths.

**Token-pool init + sfx-grid config load** (2026-10-25): two frontier rows
land in `block.*` and `level.*`. `block_text_token_pool_init` (0x478df0)
clears the 32-slot window and the head/watermark pair and raises the resident
flag — the corpus window includes the head word, so the port's separate head
static takes its own store, and init clears the slots without freeing (the
corpus's startup contract). `terrain_object_sfx_grid_config_load` (0x44acc0)
clears the 300-cell terrain sfx grid, walks the config's `S_ObjTtSFXs` chain,
and stores each entry's resolved sound alias at `row + type*15`, the (type,
row) pair coming from the entity name resolver. Quirk pin: the corpus's
two dead preamble loops initialize the pair to 20/15 and carry it ACROSS
entries, so a name that writes only its half inherits the other from the
previous entry (the crystal-on-ore test pins the inheritance); unresolved
names and failed alias lookups store nothing, and negative or out-of-window
cells are guarded (the corpus strides blindly). `block_test` pins the
no-free clear and the flag raise; `level_test` pins the stores, the carry,
and the clear.

**Kind-six random sweep** (2026-10-25): one frontier row lands in
`cmd.*`. `kind_six_random_sweep_dispatch` (0x40f0e0) subtracts the frame delta
from the sweep timer; once it reaches zero it draws up to one hundred random
grid pairs modulo the current level's width and height — the grid-spawn route
is the quote-only seam precedent (never lands, so the message-6 dispatch with
the coordinate pair cannot fire) — and re-arms the timer at `(draw % 50) +
750.0` from one final draw, answering `draw / 50`. Deviations: the corpus
strides a missing current-level handle blindly and would take a draw modulo
zero, so the port skips the cell loop without the level (the re-arm draw still
runs, the corpus's re-arm is level-independent) and reads the dimensions
through the word model. Pinned: a positive timer counts down through the
three-comparison flag fold to zero, the NaN timer rides the 1024 bit, and the
seeded sweep answers identically across runs.

**Upgrade debit + name table** (2026-10-25): two frontier rows land in
`cmd.*`. `upgrade_resource_debit` (0x41f9e0) subtracts the amount from the
current level's `+0xa0` (selector 0) or `+0xac` (nonzero) under the level
gate, then steps the overlay animation outside the gate — pinned quirk: the
debit half steps with mode 0 (the add half is mode 1), and a nonzero selector
steps the constant five regardless of the amount while selector 0 steps the
amount. `upgrade_name_entry_find` (0x42cd50) scans the loader's upgrade-name
table case-insensitively and answers the matching 16-byte record's address,
zero on a cold table or miss. Deviation: the loader's globals (record block,
name-pointer array, count) have no ported owner (the config seam stays
apart), so the table rides zero-init own storage behind an install hook,
window-guarded to the 36 entries the corpus region's stride spans; NULL
names skip the compare and a dangling record base is guarded. `cmd_test`
pins the counter selectors, the five-step constant, the no-level path, the
fold, the window clamp, and the install guards.

**Scene-resource visibility setter** (2026-10-25): one frontier row lands in
`record.*`. `scene_resource_visibility_set` (0x46d2c0) raises or clears bit 1 of
the record's `+0x1e8` word and routes the raw flag through the visibility leaf:
unconditionally on `+8`, on `+0xc` when that field is set, and on every live
child of the `+0x10` window counted at `+0x40`. Deviations: the corpus's
adjacent four-byte-apart `+8`/`+0xc` object fields do not fit the port's
native-size pointer-slot model, so they route as raw 32-bit handles — exactly
what the corpus's leaf receives; the children keep the band's native-size stride
walked only to the six entries the window leaves before the count field (the
corpus strides blindly past it); the count is read once and NULL records are
guarded. The flag rides the leaf raw, unlike the overlay's folded call.
`record_test` pins the word fold both directions, the secondary's gate, the
NULL-slot skip, and the window guard.

**Entity pair predicate + active-list scan** (2026-10-25): two frontier rows
land in `ai.*`. `entity_state_pair_predicate` (0x43c200) reads the stats table
at `(type1, row1)` — a zero first read answers 1 with no walk (pinned);
otherwise the filtered pool walk (the entity-walker path with its liveness
mirror and raised `+0x3e8` bit-0x80000 gate) folds every live entity's `+0xDC`
stats float through the nested add, then the sum of the re-read `(type1, row1)`,
`(type2, row2)`, and the fold answers 0 when it is `<= 0` (pinned comparison).
Deviations: the corpus rides the row index and the float accumulator in one
register (the port keeps them apart, the accumulator resetting to 0.0 before the
walk, and the re-read using the saved index — pinned quirk); the corpus's float10
fold lands as a double fold; the walk composes the corpus's global type tables
over the port's typed entity blocks; NULL context skips the walk.
`active_entity_list_contains_code` (0x4031e0) scans the selection through the
permitted-activity check when a selection exists — first hit answers 1 and the
active-list branch is never consulted (branch isolation pinned) — else the
corpus's global active-entity list, skipping NULL entries without a test.
Deviation: the list globals have no ported owner (the activation band is a
hook-boundary seam), so they ride zero-init own storage behind install/count
hooks, window-guarded to the 50 entries the corpus region's stride suggests.
`ai_test` pins the short-circuit, the gate lift, the `<= 0` edge, branch
isolation, and the NULL-walk-past.

**Record-creation leaves** (2026-10-25): three frontier rows land in
`ptrstack.*`, the module already owning the adjacent callback-state list
disposal. `scene_node_record_create` (0x40f6c0) builds the 0xa0-byte scene-node
record over the engine heap: counter slots clear, dword 8 pinned to 1.0f, the
`param_2` triple at +0/+4/+8 and the `param_1` triple at +12/+16/+20 (zeros on
NULL), `param_3…param_6` duplicated into the paired dword slots
(0xf/0x13, 0x10/0x14, 0x11/0x15, 0xe/0x12), `param_7` at dword 0x26 — the whole
block zero-init, so the tail dwords the corpus leaves as heap garbage land
clean (deviation). `callback_state_record_create` (0x40f7d0) allocates the
0x28-byte state record, creates the node record through it into slot +0 (LP64:
native pointer), and records the two triples' non-nullness at dwords 3/4 —
pinned quirk: the node result is stored even when its own allocation fails
(slot +0 lands NULL while the flags still land). `callback_record_allocate_initialize`
(0x4628e0) builds the 0x28-byte record with dwords zeroed and payload
param_2…param_6 at dwords 1…5 — pinned: `param_3` rides **twice**, dwords 0
and 2 — with dword 8 holding a link slot whose front points at a zeroed
0x20-byte buffer; pinned quirk: the corpus writes the caller's out slot
*outside* the allocation-success branch, so the slot lands NULL when the record
allocation fails (the port guards the slot pointer itself). `ptrstack_test`
pins the layouts, the 1.0 slot, the duplicate pairs, the flag dwords, and the
slot/buffer ladder.

**CRT control-flags/signal leaves + menu-list find** (2026-10-25): four
frontier rows land across `crt.*` and `frontend.*`. `crt_control_flags_translate`
(0x4977d0) folds the file flags: bits 0/2/3/4/5 cross over to 0x10/8/4/2/1,
bit 1 pairs back as 0x80000, the bits 10/11 pair folds 0x100/0x200/0x300
(the pair-clear stays), the bits 8/9 pair answers 0x20000 when clear and
0x10000 on the lone 0x200, bit 12 answers 0x40000.
`crt_wide_char_to_multibyte` (0x498b70) lands the pinned-C-locale half of the
corpus's code-page conversion: chars below 0x100 write one byte and answer 1;
anything else stores the EILSEQ-shaped 0x2a into the modeled math-error slot
and answers -1 (the code-page branch rides the recorded locale-map chain).
`crt_signal_handler_record_find` (0x49ab90) hunts the corpus's signal records
(id at entry+4, count-bounded table); the writers sit in the unported CRT
startup band, so the records ride own storage behind install/peek hooks with
the LP64 stride widened. `menu_list_find_name` (0x414c90) walks the corpus's
list header {array @+0, count @+4} — deviation: explicit arguments (no ported
owner for the header) — answering the first non-null entry whose name
case-fold-equals the target; misses and non-positive counts answer -1.
`crt_test` and `frontend_test` pin the folds, the failure half, the bounded
hunt, and the name walk.

**String/CRT helper leaves** (2026-10-25): four frontier rows land across
`string.*`, `model.*` and `crt.*`. `string_case_substring_find` (0x48c040)
walks the corpus's `__strnicmp` shape — the same bounded ASCII fold compare
now exposed as `string_folded_compare` — answering the match pointer plus the
byte index and needle length through optional outputs, gated on non-empty
haystack and needle. `lwo_string_duplicate` (0x48c0c0) allocates `strlen + 1`
rounded UP TO THE NEXT EVEN size through the engine heap, zeroes the whole
block, then copies `strlen + 1` bytes — the corpus's own loop bounds,
assembly-pinned over Ghidra's tail-copy guess. `lwo_texture_projection_mode_parse`
(0x48c120) fold-compares against the binary's PLANAR / CYLINDRICAL / SPHERICAL
table; the first prefix match answers the table index, anything else answers
PLANAR. `crt_file_descriptor_flag40_get` (0x498b40) reads bit 0x40 of the byte
at entry+4 in the corpus's file-descriptor table — 32 eight-byte entries per
block, blocks indexed from an unowned CRT platform window, bounded by its count
word; the window rides own storage behind install/peek hooks with the block
table and descriptor count guarded. `string_test`, `model_test` and `crt_test`
pin the folds, the even-rounded dup blocks, the table fall-through, and the
flag-window bounds.

**Sav visited tables + slot refresh** (2026-10-25): two frontier rows land
across `frontend.*` and `savegame.*`. `sav_clear_visited` (0x416e70) walks the
corpus's two "visited" bands, out-of-game first (the modeled active-object
count words gate DAT_00556ff0/ff4 and DAT_00556fdc/e0 item/word tables); every
non-null entry record has its +0x10 flag word cleared and the table word
alongside. The installers sit in the unported sav-tree loaders, so the tables
ride own storage behind install/peek hooks (the unowned-global convention); the
entry windows and counts are guarded. `sav_refresh_slots` (0x417df0) walks the
six slots so every slot file exists: with the engine feature word's bit 0x400
raised each slot is written fresh from the live context before being read
back, otherwise it is read (and re-read once on failure). Deviation: the port's
single active-slot context stands in for the corpus's six context blocks (the
SavSlotCtx precedent). `savegame_test` pins both paths against distinct per-slot
record blobs; `frontend_test` pins the clear walk, the null-item skip, and the
guarded counts.

**UI/panel category-bounds band — category store, entry bounds, priority
state, render record** (2026-10-25): four frontier rows land in
`frontend.*`. `panel_category_bounds_set` (0x45a650) stores the panel
category rectangle over the corpus's stride-0x30 record band (ints cast to
floats, the record's word cleared) and routes the pair behind the record:
the first corner while the modeled flag word's bit 2 rides, the second
corner otherwise — the pair/flag windows already live in `crt.*`, the
rectangle fields are own storage. `ui_entry_bounds_get` (0x45bf70) reads the
draw-record block (stride 0x2c) and answers the position plus the derived
size (far minus near). `priority_image_selection_state_update` (0x45d300)
runs the sentinel-parked state pair (0x1b parked by a cleared gate; the
sentinel branch installs the candidate and raises flag bit 2 when the skip
word is clear; an equal state just raises the flag; the answer rides the
pre-call word). `ui_render_record_create` (0x45da90) builds the corpus's
record — the source, four payload words copied from the caller's array, the
extra argument, and the source's float at byte offset +0x10 (the corpus's
float → float10 → float round trip is exact, so the port copies).
Deviations: blind strides guarded, the record fields widen LP64-safe.
`frontend_test` pins the flag-gated routing, the size derivation, the full
state-machine ladder, and the record's field-by-field copy.

**Resource-handle release band — global set + list disposer** (2026-10-25):
two frontier rows land in `resource.*`. `resource_handle_set_release`
(0x41a850) frees the corpus's global handle set: the DAT_004ddc04 stride-8
record band (the walker strides pairs and frees each non-zero first dword
until the DAT_004ddcc4 bound) followed by the twelve named globals in corpus
order — 734/73c/744, ad4, 99c/9a4, then 738/740/748, ad8, 9a0/9a8 — through
the engine-heap free seam. The band has no ported writer, so it lands as own
storage behind install/peek hooks, and the port clears each freed slot (the
corpus dangles). `resource_handle_list_dispose` (0x410ae0) frees a handle
list: each non-zero handle in the count-bounded array, then the array, then
the record — the record rides the ladder even when its array word is zero.
Deviations: the record fields widen LP64-safe and the NULL record is guarded.
`resource_test` pins the install/peek hooks, the release-and-clear sweep,
the empty-set rerun, the zero-entry free ladder, and the guarded NULL.

**Wad payload band — RNC dispatch, expand, Huffman clear, handle pair**
(2026-10-25): five frontier rows land in `resource.*`.
`wad_payload_decode_dispatch` (0x49c840) gates on the corpus's three-byte
"RNC" magic (the DAT_004b2d08 global), branches on the type byte at [3] —
type 0 is the raw pass-through with the big-endian size dword at +4 and the
payload from +0x12 (dword stages plus the trailing `size & 3` bytes; the
port's single-span memcpy is byte-identical), types 1/2 hand over to the
dormant codecs (the port answers the type marker itself), and anything else
answers 0xfffe; bad magic answers 0xffff. `wad_expand_payload` (0x49c7c0)
probes or materializes: the size alone answers a NULL out-pointer, else the
engine heap allocates, the dispatch decodes, and a failed dispatch frees.
`wad_huffman_table_clear` (0x49cf50) zeroes 16-byte decode-table entries —
dword 0, word 0xffff, dword 0, word 0 — a zero count clears nothing.
`game_file_handle_allocate` (0x47fed0) builds the file-layer pair: kind 1
rides the flagged record with no stream buffer, kind 0 carries the 12-byte
stream buffer behind and releases the record if that alloc fails.
`wad_entry_read_into_stream_slot` (0x47fea0) zeroes the slot pair behind the
handle, resolves the entry through the corpus's wad path opener with the
search-all index inlined, and answers whether the entry exists. Deviations:
the record/slot fields widen pointer-safe for LP64, the corpus's -1 sentinel
becomes NULL, and blind strides are guarded. `resource_test` pins the magic
verdicts, the dword+tail copy, the dormant type markers, the probe/decode/
failure paths of expand, the cleared-entry shape and the untouched tail,
the kind verdicts, and the slot open/miss behavior.

**Surface record pixel band — flip, palette expand, 24-bit convert**
(2026-10-25): three frontier rows land. `surface_pixel_rows_flip_with_scratch`
(0x47d740) reverses the row order through an engine-heap scratch, three stage
copies per pair (top→scratch, bottom→top, scratch→bottom); rows stride at the
EVEN-rounded row_bytes, so an odd row length swaps the even span and the
stray byte is the corpus's rounding. `surface_palette8_to_16bit_surface_expand`
(0x47d520) walks source bytes as palette indices and lands each entry's three
bytes through their mask fields as ((byte >> (8 − popcount)) <<
first-set) — byte 0 the +0x58 mask, byte 1 +0x5c, byte 2 +0x60, entries four
bytes apart — then flips; depths 8/24/32 do no work and answer 0, any other
depth answers 1. `surface_bitmap24_bit_surface_convert` (0x47d800) is the
24-bit sibling: three source bytes per pixel ride the masks REVERSED (byte 0
the high field, byte 2 the low) and rows advance by the stride over
three-byte pixels. Deviations: the record is modeled LP64-safe as named
fields (the surface's blind strides are guarded), and the corpus's scratch
alloc/free rides the engine-heap seam. `surface_test` pins the pair swap,
the middle row, the even-stride rounding, the mask-field words, the reversed
24-bit mapping, the stride pads, and every depth verdict.

**FLIC opcode-27 8-bit delta + the decode dispatchers** (2026-10-25): three
frontier rows land. `flic_decode8_bit_alternate_delta` (0x484ca0) is the
palette-indexed sibling of the 16-bit delta: the same chunk walk (kind-0
header doubles as the pair count, 0xC retires the row by the whole-word
subtract, kind-4 ends), but pixels land as PALETTE INDEX PAIRS expanded
through the +0x41c indexed-word table — each run word carries two byte
indices, the pair's low byte skips in WORD units, the bit-7 back-count
repeats the trailing pair word's bytes 0x100 − n times with the cursor
resuming four bytes on, and the 8-chunk remembers its header byte (NO flag
fold — the 16-bit delta's fold is its own) for a lone palette WORD store
after the walk. `flic_opcode25_decode_dispatch` (0x484b20) and
`flic_opcode27_decode_dispatch` (0x485030) route on depth +0x64c, op
+0x6d4, and the marker word +0x644: depth-8 op-4 picks mode-4 versus the
alternate delta, the marker −0x50bd picks the keyframe versus the delta,
and 0x1234 falls to the noop. Deviations pinned: the corpus's zero-argument
callee calls leave the record in a register (the port threads it through),
and the LP64 window-slot fix makes this band safe. `flic_test` pins the
word-unit skips, the palette expansion, both repeats, and every route.

**FLIC 16-bit keyframe + delta — the word-stream half** (2026-10-25): two
frontier rows land. `flic_decode16_bit_keyframe` (0x4849d0) walks the word
stream at stream+7 over rows +0x10: the literal run repeats one expanded
word (count byte + value word, three bytes), the copy run walks successive
words, and each row re-reads the row length +0xc past one straddle byte.
`flic_decode16_bit_delta` (0x484e50) walks the chunk stream at stream+8
with the pair words the corpus shape pins: the kind-0 header doubles as
the pair count, the 0xC retire retargets the row counter by the whole-word
subtract (the corpus counter is a ushort — the wrap is the quirk), the
8-chunk remembers its (flag-folded) header for a lone BYTE store after
the pair walk, 4 ends the record, the pair's low byte skips and its high
byte runs — and a high byte with bit 7 set repeats the one word it reads
0x100 − n times (the cursor resumes four bytes past the pair). The pixel
expansion `((w & 0xffe0) << 1) | (w & 0x1f)` shifts the green field one
lane up; the flag +0x6e4 keeps it where it is (`w & 0x7fe0`). LP64 fix
en route: the corpus record is 32-bit, so the stream link +0x6d0 and the
pixel buffer +0x6cc sit four bytes apart — two LP64 pointers cannot fit
(reading the mode-4 window raw made the pixel store clobber the stream).
The install hook now keeps both in side slots (the hook-boundary static
precedent); decoders resolve through them, falling back to the guarded
raw read. `record_test`/`flic_test` pin the pair strides, the wraps, and
every guard.

**FLIC row-record band — triplet walk + mode-4 byte stream** (2026-10-25):
two frontier rows land on the corpus frame-record window (the triplet-word
precedent extended with the stream link +0x6d0, pixel buffer +0x6cc, pitch
+0x638, row count +0x10, row length +0xc, and an install hook).
`flic_decode16_bit_row_records` (0x484c00) walks the chunked stream:
per-chunk triplet skip +1*3, the copy lands every source byte >>2, a
zero-count byte restarts the table for one 0x100-triplet lap and ends the
walk, and op word +0x6d4 = 4 hands off to the triplet fold.
`flic_decode8_bit_opcode25_mode4` (0x4848d0) walks the byte stream at
stream+7 over rows of length +0xc — the literal run repeats one palette
word, the copy run walks its own indices through the +0x41c window
(the ported indexed-word lookup shape), and each row pass straddles one
stream byte between rows. Deviation: both stride the record blindly; the
port guards only the record pointer. `flic_test` pins chunk strides, the
restart lap, op gating, runs, and the row straddle.

**Velocity state trio + FLIC triplet word build** (2026-10-25): four
frontier rows land. `entity_velocity_state_record_get` (0x4712c0, record.*)
answers the type-keyed velocity base — type 1 the +0x0c sub-record plus
0x138, type 4 the +0x18 sub-record plus 0xa0, else NULL (the corpus walks
the foreign type as a null base, deviation). `entity_velocity_slot_value_get`
/ `_set` (0x4712f0 / 0x471310) roundtrip the float word at +0x98 + slot*4
of that base — the corpus's x87 widening leaves the bits unchanged, pinned
by the bit-pattern roundtrip. `flic_packed_triplet_table_to_word_build`
(0x484ba0, flic.*) folds the record's 256 packed triplets at +0x11c into
16-bit words at +0x41c, `((b1 | ((b0&0x3e)<<5))<<5) | (b2>>1)`, then clears
the first word; the record rides the frame-record byte window (the
opcode-16 dispatch precedent). `record_test` pins both type walks, the
window strides, and every guard; `flic_test` folds the whole table.

**Panel layout band — store + gated position update** (2026-10-25): two
frontier rows land. `panel_resource_layout_store` (0x45c340, frontend.*)
performs the corpus's two surface loads through the load seam (the
hook-boundary NULL answers; the glyph's transformed blit rides the seam
only when a surface is present) and installs the size/extra/position
words — the DAT_005007f0..808 window has no ported corpus owner and rides
own storage (deviation). `panel_resource_position_update` (0x45c4b0) adds
the indexed pair entry (`indexed_pair_get`, signed `cvtsi2ss` shape) to
the stored offsets and rides them to the blit seam, gated by the modeled
0x50080c word and the status-banner word. The glyph word lands only
through the seam, so a test hook stands in (deviation). `frontend_test`
pins the seam counts, word windows, and both gates.

**Scene record leaves — mesh color mode 3 + scalar direction adjust**
(2026-10-25): two frontier rows land. `scene_mesh_color_set` (0x474ff0,
view.*) rides the object's +0xc mesh chain onto the indexed record and
hands mode three to the ported `render_object_color_vector_set` — the
clamped triple lands in the block's +0x34 window without touching the
+0x6c flags (the alpha sibling's mode stays separate).
`scene_object_scalar_adjust_by_direction` (0x4082e0, record.*) scales the
record's +0x80 float with the corpus's two step folds — the direction-zero
(x−0.1)·10·1/9 fold with the negative-step clamp at zero, and the
x + (1−x)·0.1 up step whose fixed point is 1.0 — and raises the +0x148
word's bit 2. `view_test`/`record_test` pin the window, clamp, fixed
point, and flag fold.

**Activity toggle, axis tolerance, message lookup, cursor-tile pair**
(2026-10-25): four frontier rows land. `entity_activity_toggle` (0x4373d0,
game/ai.*) ports the on/off fold on the entity's +0x3ec word: off with bit
13 raised drops it and raises bit 14 while requesting activity 0x36, on
with bit 13 clear raises bit 12 and requests 0x35; the corpus's
activity-reregistration tail folds into the same call because the port's
last-name mirror is synced by `ai_set_activity`.
`simtick_vector_within_axis_tolerance` (0x44d9a0, simtick.*) compares every
axis of a vector against the config triple's window — the corpus's
tolerance triple rides own storage with an install hook (its corpus writer
is the recorded light-effects parameter band, deviation).
`incident_message_config_lookup` (0x4193e0, message.*) walks the config
name catalog with the corpus's C-locale ASCII fold and answers the id on a
hit (the port guards both blind strides; the catalog's install rides the
name model rather than the unported catalog-init DAT band).
`cursor_tile_selection_update_by_type` (0x41cc80, frontend.*) keys the
tile-selection pair by selection kind: 0x49 installs the caller's pair,
5 and 0xe install fixed shapes, others ride. blocked.md records the
entity-type dispatch/release pair (per-type object slots with no ported
owner) and the config-indirection timer getter. Tests green; 34/36.

**Curve band + persistence/queue leaves** (2026-10-25): six frontier rows
land. `cubic_bezier_point_evaluate` (0x4065f0, view.*) ports the corpus's
Horner-shaped cubic over four two-float control points with the half-term
order kept (the x half folds the linear term first, the y half the
t-squared term first); the endpoints are p(0)=P0, p(1)=P2 — the corpus
expansion's algebraic identity. `bezier_curve_samples_build` (0x406820)
builds the sample table — header {count, control pair}, then `count-1`
samples on 1/(count-1) with the 0x31 overflow clamp.
`polyline_segment_lengths_total` (0x4067b0) walks the packed point pairs
(floats at +4+8i / +8+8i), stores segment lengths through the +0x198
table, zeroes the +0x194 word, and answers the total (the vector
distance rides the ported `vec2_dist_alt`).
`timed_effect_persistence_set` (0x46b730, frontend.*) raises or drops the
entry's flag-word bit 4 in the guarded window. `message_queue_find_id`
(0x419b20, message.*) becomes the public index-by-id walk over the queue
(renamed from the port's internal helper). `text_buffer_create` (0x4850f0,
heap.*) allocates the 0x830 text-output record with mode/header dwords,
the widened buffer slot + length word, and the +0x82c bit 2.
`view_test`/`frontend_test`/`message_test`/`heap_test` pin the endpoints,
clamps, length layout, bit folds, and record headers.

**Handler-table + multiply-thunk band** (2026-10-25): four frontier rows
land. `crt_math_exception_handler_get` (0x4939d0, crt.*) scans the corpus's
27-entry math-handler table (own storage + install hook — the corpus
writer rides the unported CRT startup band). `crt_integer64_multiply`
(0x48ea90, crt.*) ports the compiler's signed 64×64 multiply helper (plain
word product under the double-zero high guard, else the wrapped cross-word
fold). `objective_score_text_origin_set` (0x4262e0, engine.*) stores the
score-text pair and rides the debug word's bit 14.
`ai_idle_table_pointer_remove` (0x43a930, ai.*) zeroes matching idle-table
slots through the count window — holes ride, dedup sees past them.
`crt_test`/`enginebind_test`/`ai_test` pin the handler scan, multiply
folds, origin pair, and hole-walking removal.

**Engine-mode + math/CRT band** (2026-10-25): four frontier rows land.
`engine_mode_set` (0x4565f0, engine.*) installs the mode into the DAT flag
word (crt.* own storage, install hook), raises the bits-6/9 + bit-7 global
state flags at mode 1, and clears the overlay-word pair (modeled own-storage
with peek hooks). `timed_effect_cue_request_set` (0x46b700, frontend.*)
stores the cue word in the guarded band window and raises flag bits 0/1.
`crt_math_error_code_set` (0x4939a0) models the math-error slot (param 1 →
0x21, 2/3 → 0x22) and `crt_math_exception_code_from_status` (0x493a00) the
status fold 0x20→5 / 8→1 / 4→2 / 1→3 / else bit-2 doubled. Recorded with
their blockers: the Win32 mixer volume band (0x4789c0 + wrappers 0x478980,
0x4789a0, 0x414df0, 0x414e10) and the engine callback triplet (0x477fd0,
consumers sit in the recorded WinMain pump). The overlay-anim decay test's
"steps down" assertion is corrected — 1.0 is the corpus formula's fixed
point (expression pin unchanged). `crt_test`/`enginebind_test`/
`frontend_test` pin the installs, flag folds, and cue window.

**Record-pool + command/reference band** (2026-10-25): four frontier rows land.
`pooled_record_allocate_and_reset` (0x4382c0, entity.*) pops the ported pool's
free list, clears 0x103 dwords, self-links the busy mark, and seeds the
−1.5 float pair at +0x2f4/+0x2f8, growing a chunk on the empty head.
`velocity_slot_vector_read` (0x471c70, record.*) strides three float words
from +0x44 + index·12. `entity_flag_80_set` (0x473b20, record.*) raises or
drops bit 7 of the +0x14 flag word. `pending_record_entity_reference_clear`
(0x4063e0, cmd.*) walks the global command list zeroing every record's new
+0x48 pending-reference slot (corpus writers of the slot ride the unported
spawn band — zero until one installs). Recorded with their blockers: the
object-layer/pose seams (`0x449cd0`, `0x471a60`, `0x471ca0`), the
reference-name-table remap family (`0x451fd0`, `0x45d7e0`, `0x45d870`), and
the wad context-table scan (`0x48be00`). `record_test`/`entity_test`/
`cmd_test` pin the vector stride, flag bit, pool reset, and reference walk.

**Erosion timing band** (2026-10-25): three frontier rows land in
`level.*`. `erosion_timing_state_init` (0x40e930) seeds the timing scales
(×25, second also ×0.2) and wipes the 2000/1000-dword scratch as guarded
own storage (the hook-stubbed tickers own the consumers); the timing
word lands in the timestamp tick model with its float bits riding.
`lava_erosion_delay_from_cell` (0x40e9c0) counts the cell's +6 byte down
from six over the delay scale (negative past six, faithful).
`lava_erosion_neighbor_check` (0x40f000) sweeps the cardinal ring off a
type-6 seed, answering on a busy-free untimestamped neighbour — the
uninitialized first x-offset in the corpus table is pinned to zero
(deviation). The `cell_coordinate_timestamp_remember` header note gains
its first writer. `level_test` pins the seeds, the delay gate, and the
neighbour sweep against a hand-built grid.

**CRT integer/string helper band** (2026-10-25): six frontier rows land in
`crt.*`. `uint96_copy` (0x497a80) walks three dwords forward, reproducing
the corpus's overlap word-duplication. `crt_string_duplicate` (0x49b660)
copies strlen+1 through the engine allocator. `crt_unsigned_format_radix`
(0x49dba0) folds digits through the corpus's `+ 'W'` lowercase letter
half, guarded on radix < 2 (the corpus's divide loop never ends), and
`crt_signed_to_string` (0x49db60) takes the corpus's sign half only for
radix 10 — INT_MIN rides the wrapping magnitude as −2147483648.
`crt_string_to_upper`/`crt_string_to_lower` (0x49dcd0 / 0x49dc10) fold the
whole string on the pinned C-locale half (the DAT_00549180 locale branch
rides the recorded locale-map chain). `crt_test` pins the overlap copy,
radix digits, sign wrapping, and ASCII folds.

**Config-seam + overlay/counter band** (2026-10-25): seven frontier rows
land. `text_buffer_reset` (0x4859b0, heap.*) zeroes the text-output
record's buffer sweep, +0x18 window, and flag words (the widened +0x14
slot is restored across the window zeroing, a documented deviation);
`ui_text_output_object_release` (0x45a620) runs it through the holder
slot. `config_rgb_vector_read` (0x479170) and `config_angle_radians_get`
(0x479110, block.*) land over the config model — the comma triplet
scaled 1/255 with FUN_00477440's split quirks pinned (trailing commas
ride the last token), and the degrees→radians gate with the missing /
unparseable strings answering zero; the shared config-seam blocker is
closed. `ui_overlay_animation_advance` (0x45c580, frontend.*) models the
DAT_00500820 animation float as own storage (the apply's object-word
halves stay blocked). `level_resource_balance_add_and_sound` (0x41f9a0)
and `simulation_mode_counter_add` (0x43a910, cmd.*) bump the level's
+0xa0/+0xac resource counters under the corpus's level gate while the
overlay advance half runs outside it. `heap_test`, `block_test`,
`frontend_test`, and `cmd_test` pin the sweeps, splits, convergence
halves, and counter gates.

**Stats-table stride triplets + feature/wad leaf band** (2026-10-25): five
frontier rows land. `stats_table_word_at_b4` (0x469af0),
`stats_table_word_at_d0` (0x469f60), and `stats_table_word_at_100`
(0x46a0f0), in `entity.*`, extend the `DAT_00502bd8` family with the
corpus's (type, row, stride-index) triplets — offset + index * 0x150
window reads over the installed template spans, guards answering zero
where the corpus strides blind. `low_texture_flag_set` (0x477bd0, crt.*)
turns on DAT_00505f84's bit 5 while bit 0x40000 is clear — modeled as an
install-hooked static (the EngineContext::feature half-model stays apart,
a documented deviation). `wad_memory_stream_buffer_get` (0x48c030,
resource.*) routes the slot through the fixed-record slot reader
(0x48bff0) and drops the result. `entity_test`, `crt_test`, and
`resource_test` pin the stride guards, the gate halves, and the slot
route.

**Indexed stats-type table band** (2026-10-25): nine frontier rows land in
`entity.*` over the `DAT_00502bd8` two-level type table. `stats_table_word_at_b8`
(0x469b20), `stats_instance_word_at_bc` (0x469b50), `stats_instance_word_at_e0`
(0x469b80), `stats_instance_word_at_ec` (0x469bb0 — the blocked.md entry is
resolved here), `stats_instance_word_at_f0` (0x469be0),
`stats_table_float_at_dc` (0x469e00), and `stats_table_word_at_144/148/14c`
(0x469ec0/ee0/f00) resolve template records through `*(*DAT_00502bd8 +
type*4) + row*4` and read fixed fields or stride-0x150 instance blocks. The
stats-config loader stays unported, so the table is guarded own storage with
install hooks: cells start empty, window/stride guards answer zero where the
corpus strides blind. The entity-indexed twins read (type, row) at +0x00/
+0x04 and the instance index at +0x330. `entity_test` pins the window reads,
the stride guards, and the install spans.

**HUD post chain band** (2026-10-25): three frontier rows land in `cmd.*`.
`menu_selection_resource_update` (0x41b300) gates on the corpus's kind set
{1,2,3,4,5,7}, applies the tracked surface word to the last tracked
coordinate's cell, and records the prior value when the parameter half
applies type 4. `command_context_post_to_hud` (0x41b2d0) runs the global-
state refresh, this update, and the map-coordinate store;
`selection_clear_request_and_hud_reset` (0x41e750) enqueues the clear
command and posts kind 0. The coordinate/kind words ride the frontend.*
map-store model, DAT_004de1f0 lands as a tracked static, and the packer's
lost register halves follow the cmd.cpp tail precedent. `cmd_test` pins the
track/apply/gate paths over a written surface map.

**Renderer-word getter band** (2026-10-25): seven frontier rows land on the
unowned-global convention. `renderer_word_a_get` / `renderer_word_b_get` /
`renderer_word_c_get` (0x410b30/b40/b50, crt.*),
`renderer_global_dword_get` (0x4065d0), `render_surface_secondary_get`
(0x4065e0), and `render_feature_flags_get` (0x477f70) read the corpus's
DAT_0076ab24/b28/b3c + DAT_0076ac84/ac8c + DAT_0050608c renderer band, each
modeled as a zero-init module static with install hooks (the corpus writers
are unported). `rng_thunk_draw15` (0x4798a0, level.*) forwards the seeded
15-bit draw through the module RNG model (the corpus's thunk copy inlines
the same DAT_004afcc8 LCG body). `crt_test` and `level_test` pin the zero
start, installs, and the shared draw. This band also follows the frontier
scan fix: the scanner's header regex required six digits and silently
skipped uppercase-hex corpus headers, so the portable-now frontier resizes
to 918 rows; triaged families stay recorded in blocked.md while the
portable clusters land cluster by cluster.

**Frontier closure band** (2026-10-25): two window-portable rows land and
the remaining portable-now frontier is recorded. `scroll_rect_resource_record_create`
(0x463770, frontend.*) allocates the 0x1c-float rect record, folds the quad
through the corpus's x/dx + y/dy add order, leaves the surface-load seam
dims zero (the panel-load NULL precedent), and stores into the view
window's rect half A/B by mode. `flic_opcode16_mode_dispatch` (0x484880,
flic.*) runs the mode-0x10 and opcode 0/4 fallback routes over the
frame-record window, answering 1 everywhere. The blocked.md frontier-closure
triage section records the residual portable-now frontier by family —
D3DRM/CDAudio/Win32/CRT-internal/platform seams, the queue-LAB thunk
closure behind the grid-lookup thunk, the renderer and DAT bands, the
name-table families, and the config-record chain. `flic_test` and
`frontend_test` pin the window routes and guards.

**Idle-tile update + terrain-eligibility band** (2026-10-25): four
portable-now rows land in the ai/level band. `entity_update_and_unregister_at_tile`
(0x440130, ai.*) strides the idle-registration table for entities standing
on the target cell: the link word lands through the +0x2fc mirror, the
linked-entity flag/command update runs, and the slot blanks (port-side
table scan where the corpus strides its global).
`entity_activity_terrain_eligibility_transition` (0x449980, ai.*) gates on
the +0x3e0 bit 0x40: an ineligible cell folds activity 11 in with argument
1 and moves the gate bit to the eligibility bit, a spent activity timer
returns untouched, and the handoff folds activity 0, queues the cell's
work transition, and blanks the +0x2f4/+0x2f8 pair (new EntitySlot fields)
to -1. `terrain_cell_queued_work_transition` (0x431140, level.*) raises the
work bit through the flag-0x10/busy-bit-clear gate and runs the refresh
dispatch, the coordinate-pair seam, and the 0x1c cell-work message.
`random_variant_cell_scatter` (0x435230, level.*) walks the (2r)² box with
the dedup-keyed draw pairs (`dy + dx*10`, key bounds pinned as a deviation
over the corpus stack overflow), the mode-0 quarter limit riding the round
cap, and the quote-only spawn route keeping every landing invisible.
`ai_idle_table_reset` joins the port hooks (the corpus table is unowned).
`ai_test` and `level_test` pin the gates, folds, guards, and draws.

**Cell-command / task-batch / pool window band** (2026-10-25): five
portable-now rows land across the command, path, and entity-pool windows.
`cell_command_type3_issue` (0x403030, cmd.*) issues while the cell is
walkable with the bounded flag-0x20 word clear: raw coordinate dwords,
the zero-state, and the kind-6 head link, riding the surface-type tail
the corpus's register-losing shape (the 0x402510/0x402d50 twins' pinned
deviation). `entity_task_batch_append` (0x403650, cmd.*) allocates a
type-1 record per entity, arms payload + zero time words, sweeps the
chain, and lands on the tail's +0x60 link (or the +0x2f0 head) — the
batch stays off the global list. `path_record_allocate_and_link`
(0x409350, level.*) stamps the serial counter store, the coordinate
pair, and the prev link across the corpus's 0xd4 window (widened link
slot). `pooled_record_chunk_allocate_and_link` (0x438310, entity.*)
grows the stride-0x410 record chunk onto the free head, tail-joining
the previous chunk. `resolved_entity_operation_dispatch` (0x460910,
entity.*) feeds the count-difference scan from the resolver's
(type, row) pair — table rows write it, the ore pair writes 0/1, the
rest ride zero (a new count-table poke hook exercises the read shape).
Deviations: guarded allocator/level/entity words where the corpus
strides blindly. Tests pin the gate pair, the raw-word staging, the
chain landing, the link join, and the scan mappings; 34/36 (flic +
frontend baseline). Ledger regenerates: 976 ported (+5).

**Advisor / indexed-list / scratch window band** (2026-10-25): six
portable-now rows land in the unowned-window model. The advisor pair
(objective.*): `advisor_action_position_and_mode_set` (0x401840) stores
row mode + pair dwords, raising flag bit 0x10000 when mode is 0xc and
clearing otherwise, and `advisor_action_position_get` (0x401890) reads
the row's separate read-out window — a faithful quirk: the set never
touches it. `objective_advisor_token_lookup` (0x453200) keeps the
corpus's bounds-guarded table read (an install hook replaces the
unported table writer). `indexed_linked_list_entry_get` (0x419780,
message.*) walks the installed chain through the node's +0x10 link with
the corpus's count/index guards. `rotated_pairs_scratch_fill` (0x464110,
view.*) fills the module scratch with the corpus's sign-fold mod-4
rotation classes (1 swaps both negated, 2 negates both, 3 lands
(y, -x), negative folds default to identity) plus the offset pair.
`can_spawn_expansion_entity_at_grid_cell` (0x4663f0, engine.*) scans
the descriptor's stride-0x48 voxel buffer: the cell's own flag bit 0x08
clears the answer, the raised self-pair lands the corpus's 0x3fc90fdb
word, and the half-clear branch decides on the up/down neighbor bits.
Deviations: bounded window tables (the corpus strides the unowned words
blindly). Tests pin the flag pair, the read-out quirk, the chain walk,
the rotation classes, and every can-spawn branch; 34/36 (flic +
frontend baseline). Ledger regenerates: 971 ported (+6).

**Field-grid + checkpoint fold band** (2026-10-25): three rows land in
the entity-cell field grid and the checkpoint blob. `set_expansion_entity_at_grid_cell`
(0x466380, engine.*) parks the entity on the cell slot, raises the
occupancy bit 0x100, and stores the corpus's fixed 100.0 timer;
`grid_cell_invalidate_and_clear_flags` (0x466750, engine.*) raises the
parked entity's `+0x3e8` bit 0x800000, clears `+0x3e0` bit 0x02000000
with the corpus's own mask (re-reading the entity from the slot before
each window), then drops the cell's occupancy bit. `checkpoint_blob_task_progress_recalculate`
(0x460950, savegame.*) folds the blob's eight stride-0x210 task windows
as percent = 100 / den * num — a zero denominator answers 100 and the
clamp lands over the store — then walks the count-up loops (each round
adds 100 to the `+0x2fac` sum and the last folded percent to the
`+0x2fb0` carry), the register-accumulator `0x2d90` loop, the clamp, and
the final fold plus the float-converted bonus word. Deviations: the
port-layout cell record widens the entity slot (stride 0x10 where the
corpus strides 0x0c), the guarded grid lookup answers silently where
the corpus strides missing words, and the fold band threads
GameContext where the corpus uses its two globals. Tests pin the park,
the mask, the fold shape, the zero-denominator answer, the register
loop's one-store, and the negative-carry clamp; 34/36 (flic + frontend
baseline). Ledger regenerates: 965 ported (+3).

**Color/opacity window band** (2026-10-25): three rows land on the
indexed-record window model. `render_object_color_vector_set` (0x483270,
record.*) writes a clamped RGB triple into one of the four color windows
(`+0x14/0x24/0x34` for slots 1–3); slot 0 doubles as the white-opacity
flag, raising `+0x6c` bit 0x100000 only when every channel reads exactly
1.0, and the corpus clamps before every branch so a NaN channel rides
through. `screen_effect_item_opacity_set` (0x483420, record.*) clamps
the opacity and composes mode 4's flag word from the item word `blk+0x64`
and the record feature word `+0x18` with the corpus's own precedence
(mask `0xeff7ffff` first; zero opacity raises bit 0x10000000 unless both
words read quiet; a mid opacity raises the fade bit 0x80000 only when the
`0x1c00` gate is not clear; the tail routes by which word is nonzero),
while mode 5 lands the plain `+0x44` dword and every other mode answers 1
without a trace. `scene_mesh_color_and_alpha_set` (0x475020, view.*) is
the scene wrapper: the object's `+0xc` inner pointer chains to the
indexed record, the RGB pair lands through the slot-0 color set, and the
alpha through the mode-4 composition. Deviations: guarded windows answer
1 silently where the corpus strides blindly. Tests pin the componentwise
clamps, the white-flag raise/clear pair, the mid-opacity gate, the
suppress-bit tail, and both link-chain NULL corners; 34/36 (flic +
frontend baseline). Ledger regenerates: 962 ported (+3).

**Indexed-record mesh window band** (2026-10-25): six rows land in
`record.*`, completing the stride-0x70 indexed-record window model the
flag-word getter and transform-copy readers established. The block window
rides the table slot at record +0x8: +0x4 vertex count (re-read every
round, as the corpus's aliasing reloads do), +0xc widened vertex-base
slot, vertices stride 0x20 as {position, normal, uv pair}.
`mesh_vertices_scale_apply` (0x482370) scales every block's vertices
componentwise with the count re-read per round and the zero-count block
skipped. `indexed_geometry_data_copy_to_output` (0x482470) packs the
32-byte vertex records into 36-byte output slots with the corpus's
untouched slot tail. The three fills pin the family's documented
quirks: `instance_quad_vertex_data_fill` (0x482500) recomputes each
normal as the triple minus the position, normalized in the corpus's
x²+z²+y² x87 order — a zero normal answers NaN/inf exactly as the
corpus's division does — while the same triple shapes every vertex;
`mesh_segment_vertex_fill` (0x482620) lands the SAME triple on every
vertex because the corpus's triple pointer never advances (pinned
quirk); `mesh_vertex_record_fill` (0x4826c0) is the advancing twin, each
vertex receiving its own triple record. `indexed_record_transform_initialize`
(0x483170) lays the identity block — size 0x50 plus four unit-float
diagonals — over the data-address window and answers 1. Deviations: NULL
table/base guards, window-threaded arguments, single-precision float
math where the corpus runs x87. Tests pin the componentwise scales, the
36-byte slot layout, both fill quirks, the zero-normal NaN, the pointer
advance, and the identity block; 34/36 (flic + frontend baseline).
Ledger regenerates: 959 ported (+6).

**Scroll-scale and window-release band** (2026-10-25): five rows land in
`options.*` / `engine.*` / `heap.*` / `cmd.*`. The corpus's scroll slider
pair joins `options.*`: `scroll_to_slider_index` (0x415030) bands the
engine's scroll-scale float into 0..5 — 0 below 0.6666667, 1 below 1.0, 2
below 1.3333334, 3 below 1.6666666, else 4 with 5 at 2.0 and above; the
corpus's float-promotion shape means a NaN scale answers 4 (pinned).
`scroll_slider_refresh` (0x415360) stores that band into the options speed
word the port models as `game_speed` (the same corpus dword). `engine.*`
gains the bit-6/bit-9 flag pair `global_state_flags_bits6_and_9_set`
(0x426360) over the debug word — bit 6 lands first through the value and
bit 9 raises or clears through the second flag, as the corpus composes
them. `heap.*` gains `dword_list_allocate_and_zero` (0x414070): the
{data, capacity} header over a zero-filled slot array, keeping the
corpus's masked zero-pass and dead second pass faithfully.
`cmd.*` gains `task_table_contents_release` (0x462960): the whole
window-tree free — count re-read every round through the record window,
sub-record slots through the widened entry edges, array and record last,
answering the corpus's 1. Deviations: pointer guards the corpus strides
blindly (NULL guards store nothing), corpus-owned globals thread through
port contexts, widened slots carry the corpus's dword strides. Tests pin
the bands, the pair masks, the window tree, the guards, and the
capacity/capacity-zero shapes; 34/36 (flic + frontend baseline). Ledger
regenerates: 953 ported (+5).

**Generic list window band** (2026-10-25): seven rows land in
`savegame.*` / `level.*`. The 0x416exx/0x4170xx list band rides
caller-supplied list-header windows as pure window operations: the fold
find (0x416e20) answers entry indices through the corpus's
case-insensitive compare, and the four wide-key halves (add/has
0x416ed0/0x416f40 over value array B; set/get 0x416fb0/0x417020 over
array A) compare keys as little-endian 16-bit units — the low byte
terminates, so a node key ending in the unit 0x1200 answers the probe
0x0000 equal (pinned quirk). `sav_tree_walk` (0x4171c0) is the
visit-each-once tree recursion with the corpus's plain indirect-call
callback over widened node slots. `cell_variant_entry_match_check`
(0x451180) joins `level.*` as a grid-window probe: both cells bounds-
checked, the first cell's inline variant list (count byte through the
window's +0x4034 table, entries at +0x34 + variant * 0x80) scanned for
the second cell's variant byte. Deviations: pointer guards everywhere
the corpus strides blindly; the corpus's list owners stay hook-bound,
so the windows ride their hooks. Tests pin the arrays, the terminator
quirk, the walk, the guards, and the variant lists.

**Pure string/geometry leaf band** (2026-10-25; findings 226/235/371/372/
388): seven rows land across `view.*`, `texture.*`, `crt.*`, and
`string.*`. `matrix4_identity_initialize` (0x483540) writes the corpus's
zero-then-diagonal order (the row-looping sibling stays cited as its own
row); `surface_triangle_normal_calculate` (0x479530) is the unit cross of
the triangle edges with no zero-length guard, faithful. The texture
cache pair (0x483030/0x483080) and the case-insensitive pair-table pair
(0x488730/0x488790) model their caller-supplied entry windows as widened
slots — append rides the string duplicate layer, find folds, and the
pair-table scan keeps the corpus's low-16-bit walk mask.
`case_insensitive_substring_find` (0x477610) scans every start position
behind the corpus's both-nonempty gate. Tests pin the diagonal, the
normalized crosses, the append/find windows, and the fold scans.

**Object registry windows band** (2026-10-25): five rows land in
`ai.*` / `cmd.*`. The 0x407xxx band registers entity records into four
pointer/timer pair windows (flagged 20 slots with seed 0x42160000, the
type-2 idle queue 20 — modeled by the cmd idle table, type-4 20, type-2
timed 50 with seed 0x4a095440 — riding module statics, no corpus owner).
`entity_pointer_registries_remove` (0x4073a0) sweeps all four for the
record (window B through a new `idle_queue_clear` hook); the inserts are
two-pass (full match pass, then first-empty append): a hit re-seeds, a
miss appends with the seed, and a full table drops (`type4_object_
registry_insert` 0x407410, `type2_timed_registry_insert` 0x4074b0,
`flagged_object_timer_registry_insert` 0x407540 — the last gated on the
capability word's bit 0x80 through the ported reader). `set_type2_
object_record_value` (0x407510) broadcasts into occupied timed-window
timers behind the type-2 gate. Deviation: the corpus strides the record's
type word blindly; the port guards and reads the mirrored type. The
corpus callers (teardown / post-create / damage-apply) stay unported, so
the windows ride their hooks. `ai_test` pins the gates, appends,
re-seeds, broadcasts, and resets.

**Entry cache get-or-load pair** (2026-10-25; finding 255): `crt.*` takes
the two cache-lookup wrappers over the string registry.
`string_cache_lookup_or_load` (0x410520) and
`resource_name_cache_get_or_load` (0x410560) intern the key, answer the
entry's bitmap/font cache slot, and run the loader once on a miss. The
corpus's surface and font loaders ride their seam cites; here the loader
call is a hook-boundary seam that answers NULL uninstalled
(load-answers-NULL precedent), and the loaded objects stay unowned — the
reset drops the slots with the entry. `crt_test` pins load-once, the hit
paths, and the reset reload.

**Renderer flag/hook + string registry band** (2026-10-25; findings
393/420): five rows land in `crt.*`. The renderer flag word (DAT_0076cd94,
zero-init module static) takes `renderer_flag_bit6_set` (0x4728e0) for its
bit-0x40 fold, and the scene hook registers store their callback pairs in
module statics (`scene_end_hook_register`, 0x472900;
`scene_sample_hook_register`, 0x472920 — the sample register also raises
bit 6). The corpus's locale-following entry compare rides the port's ASCII
fold. The DAT_004db8cc string registry has no corpus owner outside the
pair: `string_keyed_cache_entry_find` (0x410450) walks the widened-slot
chain case-insensitively, and `string_registry_intern` (0x410490) appends
misses at the head with the key copy through the heap seam (null or empty
keys answer null). `crt_test` pins the folds, hooks, chain, and reset.

**Write-band leaf trio** (2026-10-24; findings 465/101): three rows land
across `engine.*`, `view.*`, and `heap.*`. The objective/task text-origin
setter (`objective_task_text_origin_set`, 0x426320) stores the float pair
and raises the debug word's bit 0x8000 (the pair's corpus global rides
module storage — this setter is its only writer); a null argument clears
the bit. `view.*` takes the scene-node position store
(`scene_node_position_set`, 0x410020): the node's +0xc valid flag goes to
1 and the three floats land on the widened handle slot's inner object.
`heap.*` takes the text-output flag fold (`text_output_field82c_
bit2_set_clear`, 0x485160): the record's +0x82c word bit 2 set/clear.
`enginebind_test`, `view_test`, and `heap_test` pin the stores and guards.

**Scene-root argument normalization band** (2026-10-24; findings
25/349/420/430): three rows land across `view.*` and `record.*`. The
corpus's scene-object argument normalization (`scene_object_arguments_
normalize`, 0x475900) is the front-end of the ported camera-handle
primitives: the dispatch object rides out of the first argument's slot,
and the second argument resolves to its own slot or, when NULL, falls
back to the scene root object's first slot (the corpus's null-handle
dispatch). The root global is written only by the unported renderer-side
initializer, so it rides a module hook shared with the root read
(`scene_root_object_get`, 0x472940). `record.*` takes the current-object
field writer (`scene_current_object_set`, 0x477240): the interface
record's `+0x14` slot store whose consumer stays on the vtable seam.
`view_test` and `record_test` pin the hook, the fallback, and the slot.

**Air-supply accumulator** (2026-10-24; findings 317/462): `level.*`
ports `air_supply_amount_update` (0x424560). The entity's stats float
`+0xDC` is read before the gate; a kind-4 entity skips the update unless
its `+0x3e8` bit `0x20000000` is set. The delta folds through `0.04`, the
configured rate at block `+0x200`, and that stats float onto the
accumulator at `+0x1fc` — extended precision in the corpus, the port's
double — and is stored clamped high to `100.0`; at zero-or-below the
field is pinned to `+0.0` and the failed objective is announced (the
ported announcer, `who == 2`) unless the system-state word's bit
`0x08000000` suppresses it. `level_test` pins the gate, the fold, the
clamp, both depletion sides, and the guards.

**View-band matrix and ray math leaves** (2026-10-24; findings 309/363):
nine rows land, all in `view.*`. The ray pair lands first:
`point_along_ray` (0x479ab0, origin + t·dir with the corpus's component
order) and `view_ray_intersection_test` (0x4798e0), which normalizes both
direction vectors and solves the scaled ray through the ported plane
parameter before advancing. The matrix band follows the existing
zero/multiply pair: `matrix4_set_identity` (0x479e70), the
identity-plus-translation-row setter (0x479e40), `matrix4_copy`
(0x479eb0), the three rotation setters (0x479d50/0x479da0/0x479df0 —
each writes the identity and overwrites its two-row block; the corpus
reference names call two of them Z, the port names the blocks, and the
x87 `fcos`/`fsin` extended-precision evaluation lands as the documented
double-deviation cast), and the perspective projection quotients
(0x40bd60, including the corpus's roundabout m[1] product).
`view_test` pins the ray solve, the blocks at half-pi, and the zero
pattern.

**Terrain work-flag sweep, random activity attempt, and ray-plane grid
pick** (2026-10-24; findings 88/450/451/472): three rows land across the
terrain, ai, and view bands. `terrain_neighbor_work_flag_scan` (0x431580,
`level.*`) is the work-flag breadth-first sweep: a double-buffered
float-pair queue from one `(w + h) * 0x20` engine-heap block, each round
draining its page and handing flagged neighbours (work bit `0x20000`) to
the `TerrainCellMutation` fold before clearing them and requeueing; the
corpus quirks ride along faithfully — the enqueue stores the entry's
col-bound float (col+1) as x so the queue drifts one column per hop, and
the self-skip compares that bound float against the queue-buffer pointer
and never lands, so the seed requeues too. `entity_random_activity_attempt`
(0x443670, `ai.*`) draws the shared RNG word up to five times under the
bit-`0x400` and mask-word gates, folds each draw (`% 0x15 + 0x38`, the
code band) through the activity-set/registration pair, and answers 1 with
bit `0x8000` raised on the first draw the dispatch takes (the
early-return clause; the per-type runner return stays the scene seam).
`random_reachable_grid_point_select` (0x450560, `view.*`) iterates a ray
down through the z-plane at `(0, 0, -50)` `rounds` times, letting the
terrain height ride back into the plane's z so later rounds descend, and
grid-picks the final hit; the `!= 0` loop test makes a negative count run
until it wraps, and zero rounds grid-pick the caller's untouched buffer.
`level_test`, `ai_test`, and `view_test` pin the sweeps, the gates, and
the plane descent.

**Scene-pair set band** (2026-10-24; findings 25/435): `record.*` takes
the two remaining pair-set leaves. `scene_object_pair_set_render_parameters`
(0x4086c0) hands each handle's pose dispatch to the ported position and
facing primitives — the dispatch's argument block `(x, y, 0)` and the
constant `(0, 0, -1)` read as position plus facing.
`scene_object_pair_set_callback_scalar` (0x408710) runs its scalar probe
with the (x, y) pair and the third argument, then sets each handle's
position to (x, y, 0): the corpus consumes the probe's undefined float
return as the camera z, and the port pins it to zero (the sole call-site
probe, the ported height adapter, answers void). Both stride the +4
handle first, then the +0x7c partner when non-NULL, joining the already-
ported pair apply/visibility leaves. `record_test` pins both bands.

**Cell-command gate, system-word chain + voxel occupancy**
(2026-10-24):
`engine.*` ports `terrain_cell_command_predicate` (0x428780 — the
cell-command gate: an unconditional pass while the system-state word is
nonzero with bits 8 AND 4 both clear, else the occupancy branch under bit
1 where the target cell's surface byte (voxel +0x04, height-major index)
must be zero. The corpus re-reads its system word through the accessor
and strides the voxel blindly — the port reads once and answers through
the bounds-checking voxel accessor, deviations pinned). `enginebind_test`
pins all three gate shapes against the bound 2×1 grid: free-pass,
occupancy split, and the out-of-window answer.

**Token-pool grow, doubling free chunks**
(2026-10-24):
`block.*` ports `block_text_token_pool_grow` (0x479490 — the corpus's
watermark doubles the chunk size (1 << watermark words of 32), the chunk
lands on the engine heap, and its nodes chain into the free-list head:
block i links block i + 1, the last link is the old head, and the chunk
base becomes the new head. The watermark counter is parked slot-wise
through the 32-slot window (the corpus strides blindly — window guard as
deviation) and never resets, faithful through destroy. Port nodes are
widened to eight pointer slots (stride 0x40, link = field 7) so the
corpus's dword link rides a host-aligned slot — the stream-buffer
field-slot model. `block_test` pins the two-chunk chain, the slot parks,
and destroy leaving the watermark alone. The pool family completes with
this grow plus last iteration's destroy; init stays unported.

**uint96 bit-band clear, token-pool teardown + pressed-state reads**
(2026-10-24):
`crt.*` gains `crt_uint96_has_no_bits_at_or_above` (0x497900 — the
MSB-first limb scan above the named bit) and `crt_uint96_clear_from_bit`
(0x4979e0 — bumps bit − 1 only while some bit lives above bit + 1, keeps
the at-or-above half of the limb mask, zeroes the higher limbs; the
corpus's undefined increment return pins 0). `block.*` ports
`block_text_token_pool_destroy` (0x478e20 — frees the 32-slot pool words
through the engine heap and clears the head cursor and resident flag; the
init/grow entry points are unported, so the corpus storage lives
module-local behind install/peek hooks, and the head word's corpus
aliasing with the `crt.*` record-list head is documented, not modeled
across translation units). `input.*` adds `input_action_pressed_get`
(0x41c8f0) and `input_binding_pressed_get` (0x41c950) — same
0x10000000 engine-state gate, action 0x49 hard-wired not-pressed, and the
bit-2-complement inverted read under the system-state word's bit 0 (an
empty table word reads *pressed* there, corpus-truth pinned). `crt_test`,
`block_test`, and `input_test` pin the masks, the bump, the teardown, and
the gates.

**Linked-object clear, keyed entry removal + bit-indexed add**
(2026-10-24):
`entity.*` ports `entity_clear_linked_object_state` (0x447780 — behind the
flag word's 0x400 gate the carry link and state word drop, the carried
slot's flag bit 0x2000 raises, and the kind-4 command chain drops),
`record.*` ports `keyed_resource_entry_remove` (0x47b2a0 — the keyed sweep
of the association list through the sound-head hook, deep-first promotion
and unlink through the engine heap), and `crt.*` ports
`crt_uint96_increment_at_bit` (0x497970 — the bit-indexed single-mask add
at limb bit/32 with the mask 1<<(31-bit%32), the carry walking DOWN the
limb indices — this function's own order, opposite the uint96-add chain).
The fixtures pin the gate, the head promotion, and the downward carry.

**Carried-object detach, centering hit-test + selection code**
(2026-10-24):
`entity.*` ports `entity_detach_carried_object_and_reset_state` (0x43bbc0 —
the type-3 carrier's detach: a linked type-6 carry loses the link, state
word, attach back-pointer, and the flag word's 0x400 gate, both +0x340
floats drop to -1.0, and a linked reference has its dword folded into the
level's field 0x9c; the flag word's bit 0x800000 always rides the type-3
pass), `frontend.*` ports `text_layout_offset_for_centering` (0x411320 —
the centered-widget pointer test, the rect shifted left by half its width
before the closed-edge hit test) and `selection_entity_code_resolve`
(0x41b9c0 — the interaction word's bit-8-with-bit-7-clear state keeps the
stored code, otherwise the first selected record's high flag bit picks
0x22/0x23). `entity_test` and `frontend_test` pin the gates, the fold, and
both resolve routes.

**Special-state predicate, selection sweeps + nested list removal**
(findings 293/403/310/232/397):
`level.*` ports `terrain_cell_special_state_predicate` (0x40d3f0 — mode-1
occupancy, answered outright when the caller's gate word is zero, else the
secondary word's bit 0x100 must also hit), `cmd.*` ports the two
any-hit selection sweeps `selected_entities_any_predicate_match` (0x431a30 —
the walkability mode-1 entity gates stay the documented seam pass) and
`any_object_can_occupy_terrain_cell` (0x431ab0 — the placement gate reads
each record's own flag), `frontend.*` ports `store_selected_map_coordinate`
(0x41ea50 — the kind-switch branch raises the interaction word's bit 8 and
fires the alias-0x16 cue, the tracked branch refreshes the live pair, and
the kind word always re-arms), and `record.*` ports
`nested_record_list_remove_by_key_recursive` (0x47b2e0 — the deep-first key
sweep, matched child freed through the engine heap after its successor
promotion). The sweep fixtures pin the gates, stores, and chains.

**Panel highlight switch, action-target filter + pending record copy**
(findings 402/318/535/306):
`engine.*` ports `panel_selection_highlight_toggle` (0x435aa0 — the highlight
switch: ON raises the debug word's bit 0, arms the 250-tick exit timer and
kicks the alias-0x19 cue only when the timer was spent, and sets the panel-6
item-2 flag through the gated writer; OFF drops all three), `entity.*` ports
`object_filter_matches_action_target` (0x404f50 — kind 5 matches outright,
7/9/0xF gate the referenced handle through the shared eligibility check, and
the name pair +0x20/+0x24/-1-sentinel/+0x28 must agree with the target
record's identity words), and `cmd.*` ports `cmd_pending_record_copy`
(0x406400 — the same-type fresh allocation with the field window copied, the
chain link cleared, and the pending bit 0x200 raised; the corpus name's
"to entity" half never appears in its body). `engine_bind_test`,
`entity_test` and `cmd_test` pin the switch, gates, and copy shapes.

**Record factories, cardinal neighbors + a late citation** (findings 305/397/448/408):
`record.*` ports `bounded_float_record_create` (0x410b90 — the (min, max)
open-interval gate, kind + bounds + payload in the port record window) and
`menu_item_numeric_range_record_allocate` (0x410bf0 — the slider variant's
wider window in the corpus's late-bounds field order), `level.*` ports
`cell_cardinal_neighbor_apply` (0x409cd0 — the four refresh dispatches over
the north/east/south/west neighbor cells, the anchor cell left untouched),
and `save_write_record` gains its `SavWriteRecord @ 0x417f40` citation (the
implementation — the gated 0x318c blob copy with the record-table grow —
predated the ledger quote). `record_test` and `level_test` pin the windows,
gates, and sweeps.

**96-bit add, focus-scroll commit + pool free-all** (findings 375/102/437/398/374):
`crt.*` ports `crt_uint96_add` (0x499fe0 — the three-limb add through the
ported carry helper: the limb-0 carry bumps limb 1 with +1 and, if that
carries, limb 2; the mid-limb add bumps limb 2 again on its carry; the high
carry out is discarded), `engine.*` ports `camera_focus_scroll_scale_commit`
(0x4538a0 — the zero-focus gate against the state block's +0x27c word (new
EngineState field), the guard-word set/set pair around the focus * 0.01 scale
through the just-ported clamp setter) and `record.*` ports
`object_pool_lists_free_all` (0x456c60 — freeing the chains on the 0x37 pool
list heads, modeled as native link slots per the port's widened-link
convention, each chain walking the node's +0x08 next link, heads cleared
afterwards). `crt_test`, `engine_bind_test` and `record_test` pin the carry
chains, gates, and sweeps.

**Fixed-record slots + flag predicate** (findings 375/251/324/105):
`resource.*` ports `fixed_record_slot_resource_release` (0x48bfc0 — the
installed-slot heap free and flag clear) and `fixed_record_slot_resource_get`
(0x48bff0 — the installed slot's resource word), the corpus's stride-0x10
slot table (DAT_005756a0, whose only writer is the unported stream opener)
as sixteen zero-init slots with install hooks, and `entity.*` ports
`entity_type_and_state_flag_predicate` (0x45e400 — the excluded-kind set
{4, 5, 8, 9, 0xC..0xE, 0x10..0x13} answers zero; everything else answers the
inverse of the stats state word's bit 0x20000000). `resource_test` and
`entity_test` pin the slots and the predicate.

**String, stream-buffer, scale + attach band** (findings 375/93/437/398/251/270):
`crt.*` ports `string_prefix_reserve` (0x4907c0 — the strlen scan feeding the
overlapping shift that opens the prefix gap, tail NUL included) and
`crt_stream_buffer_allocate` (0x498ae0 — the counter tick, the 0x1000 engine-
heap buffer installed as the record's window with flag bit 3, the internal
buffer and flag bit 2 on failure, riding the port's stream-record layout),
`engine.*` ports `scroll_scale_set_and_clamp` (0x425ce0 — the guard-word gate,
the [0, 3] clamp, and the speed-slider band over the scale word as module
storage), and `cmd.*` ports `object_attached_record_allocate` (0x4033c0 — the
kind-0x1C record appended after the chain's last record, or hung on the
entity head when the chain is empty). `0x459240` is recorded in blocked.md
(stats-config layer, shared with `EntityAnimationStateSet`). `crt_test`,
`engine_bind_test`, and `cmd_test` pin the shifts, windows, bands, and chains.

**Counter + link band** (findings 251/469/435/434/438):
`ai.*` ports `simulation_update_counter_add` (0x43a8f0 — the global object's
+0x8C bump loop, then the scene-state pass), `entity_flag80000000_set_or_clear`
(0x438580 — the +0x3e8 high bit raised or cleared, then the scene-state pass),
`entity_nested_float_add_unless_blocked` (0x43c280 — a type-4 entity failing
the strict eligibility clause skips the +0xDC stats-float fold, which otherwise
lands on both corpus paths answering zero), and `linked_entity_flag_and_command_update`
(0x4440a0 — a type-4 link at +0x2fc raises +0x3ec bit 0x40 on both entities and
drops the parent's kind-0x1C records through the ported chain drop), while
`frontend.*` ports `screen_effect_flags_clear_and_release` (0x40a3d0 — the
ten-entry screen-effect table as hooked statics, active flags cleared with the
handles riding the visibility seam), widening the +0x2fc link mirror.
`ai_test` and `frontend_test` pin the loops, gates, folds, and walks.

**Activity-gate + selector + tree-release band** (findings 251/469/371/378/386/438/251/372):
`ai.*` ports `entity_dispatch_event31_for_pending_flag` (0x4437f0 — the
type-3/pending-bit gate raises the +0x3e4 marker and re-registers activity
0x31) and `entity_activity23_state_transition` (0x449a70 — the type-2
half-word, then the bit-0x400-gated activity-0x23 pass through the idle
register), `frontend.*` ports `active_object_list_word` (0x416e00 — the
in-game/out-of-game selector over the two unowned active-object words as
hooked statics), `crt.*` ports `owned_resource_tree_release` (0x48c180 —
the payload, payload, right-child, own-node release walk widened), and
`level.*` ports `entity_available_resource_spend` (0x438720 — the +0x1f8
gate zeroes the amount, the no-op-probe branch dispatches message 0xE, and
the accumulate branch lands in the global object's +0x98). `0x490280` and
the Queue* LAB-thunk band are recorded in blocked.md. `ai_test`,
`crt_test`, `level_test`, and `frontend_test` pin the gates, walks, and
branches.

**Handle-gate + activity chain + marker-release band** (findings 392/386/329/469/378/434):
`engine.*` ports `global_handle_position_apply_if_present` (0x435580 — the
unowned DAT_00556f3c handle gates the zero-position reposition seam, which
rides a recorded hook), `ai.*` ports `scene_object_has_task_type_4_5_7`
(0x4493e0 — the type-2 gate, then drill/teleport/reinforce through the
ported activity-list scan), and `crt.*` ports `selection_marker_resources_release`
(0x40ce30 — the 32-slot marker table frees, the count reset, and the
descriptor release; the corpus's dangling slots clear as a documented
deviation). `0x40ffe0` is recorded in blocked.md with the scene-object
thunk family. `engine_bind_test`, `ai_test`, and `crt_test` pin the gate,
the chain, and the release.

**Panel resource + scroll leaf band** (findings 367/432/251/469/386/439/436/444):
`record.*` ports `record_flag_bit_test` (0x46cf40 — the selected +0x130 bit,
optionally rejecting bits riding the type mask), `ai.*` ports
`entity_activity_scene_state_update` (0x439990 — the manager flag word's
gate pass, the cleared half threading GameContext to the ported level
mutator), `frontend.*` ports `panel_resource_position_apply_if_enabled`
(0x459fd0 — the enable-gated seam blit at position plus bubble offsets),
`panel_resource_handle_and_dimensions_store` (0x45c520 — the load seam answers
NULL with the dimensions stored as float words), and
`scroll_offset_from_cursor_update` (0x41a250 — the UI record's state-pair
difference clamps at zero into metrics slot 0, then the right-edge cache),
with the panel's unowned corpus words as hooked statics and both surface
seams recorded through the hook boundary. `record_test`, `ai_test`, and
`frontend_test` pin the bits, gates, stores, and the clamps.

**Texture-dir + forward + node-offset band** (findings 420/13/386/439/435/372):
`crt.*` ports `shared_texture_directory_set` (0x4728c0 — the DAT_0076cd88
directory word as a hooked static, the path copied through the ported string
duplicate and the corpus's leak of the prior copy reproduced faithfully),
`record.*` ports `ui_record_callback_forward` (0x45a630 — the UI record's +0
handle dereferenced through the ported state-pair read), and `view.*` ports
`renderer_node_relative_position_set` (0x436ac9 — the node's +0x28 camera
handle rides the ported mode-1 camera-offset side with a zero z).
`crt_test`, `record_test`, and `view_test` pin the copy, the forwarding, and
the offset.

**Global-state + six-slot refresh band** (findings 392/334/329/354/432/372):
`engine.*` ports `global_state_low_byte_clear_and_refresh` (0x435a20 — the
DAT_00556ed4 low byte clears, then DAT_005563b4 rides the ported surface
visibility set with mode 1; both unowned corpus words as hooked statics),
`record.*` ports `six_resource_slots_set_state` (0x46cfc0 — the six handle
slots from +0x10, each nonzero one calling the visibility leaf, widened-slot
stride), and `options.*` ports `options_context_six_field_set` (0x417f20 —
six dwords into the open slot context's +0x1c window, with the port threading
a GameContext argument). `engine_bind_test`, `record_test`, and
`options_test` pin the stores, gates, and the guarded closed-slot path.

**Allocate/add + slot-write leaf band** (findings 440/408/243/381): `heap.*`
ports `three_word_record_allocate_and_initialize` (0x410b60 — the three-word
record store, answered through the port for observability), `message.*` ports
`has_active_resource` (0x4119d0 — the +0x38 handle rides the system-state
bitmask), `options.*` ports `options_write_to_slot` (0x416910 — with a slot
open, the live speed/sfx/music words land in the slot context's +12/+8/+16
window), and `sound.*` ports `sound_buffer_association_add` (0x47b020 — the
four-word association record prepended onto the DAT_00506684 chain, modeled
here with hooks). Five seam-bound candidates (0x442260, 0x46c130, 0x40c720,
0x438f60, 0x46d350) are recorded in blocked.md with their next actions.
`heap_test`, `message_test`, `options_test`, and `sound_test` pin the stores,
gates, and chain.

**Scene-visibility wrapper band** (findings 446/370/435/356/373/476/28):
`record.*` ports six wrappers over the surface lock/unlock hook boundary —
`graphics_overlay_visibility_set` (0x4634b0 — the overlay word moves the
cache only on a change, the handle rides the folded 1/0 flag, both corpus
words as port statics), `surface_visibility_set` (0x464720 — the record +0
handle), `animation_object_visibility_set` (0x406c90 — the animation record's
+4 handle), `scene_object_pair_apply_mode` (0x408670 — the +4 object and the
nonzero +0x7c partner), `animation_object_visibility_get` (0x4086a0 — the +4
object's flag bit 3 through the ported getter), and `scene_assign_entity_owner`
(0x408380 — the owner store on the +4 object through the ported setter), each
routing FUN_00473b40 through a recorded hook so `record_test` pins the calls,
gates, and stores.

**Chain-release + view-transform leaves** (findings 376/478/431/13/395/379/
348/426): `simtick.*` ports `simtick_release_all_group_records` (0x4099d0 —
the bobbin chain walk dropping each record through the corpus drop leaf,
read-only through the hook boundary), `crt.*` ports `model_render_list_clear`
(0x482d30 — the unowned list head with hooks, nodes freed through the engine
heap along the +0x44 successor, head cleared) and
`crt_owned_stream_buffer_release` (0x494df0 — the flag-gated owned-buffer
free, bits 3 and 11 cleared and the pointer words zeroed in the host-word
field window), and `view.*` ports
`selected_global_point_transform_and_return_x` (0x46a420 — the mode-selected
view window read through the ported camera pair, answering the frame's x).
`simtick_test`, `crt_test`, and `view_test` pin the walks, the gates, and
the guards.

**Eligibility + release leaf band** (findings 332/460/251/435/471/393/377/
323/416/428): `entity.*` ports `entity_flag100_and_shared_eligibility_check`
(0x471ce0 — stats bit 0x100 rides the shared gate, strict only for type 4),
`entity_quad_mesh_render_record_allowed` (0x40aae0 — clear stats bit
0x08000000 and type neither 8 nor 13), and `task_type_and_condition_check`
(0x43c490 — the relaxed gate and the caller's condition; the corpus's first
argument is calling-convention residue), while `crt.*` ports
`linked_object_list_release` (0x479240 — the field-0 owner free, then the
field-5 chain recycled onto the global record list), `ptrstack.*` ports
`callback_state_list_dispose` (0x40f850 — the drain, the entry frees, and
the stack-record release, with the corpus record laid out host-widened),
and `frontend.*` ports `bitmap_font_glyph_measure_width` (0x47a520 — the
measure lane with the draw flag clear, the faithful void-effect wrapper).
`entity_test`, `crt_test`, `ptrstack_test`, and `frontend_test` pin the
gates, the chain, the drain, and the guards.

**Activity-request wrappers** (findings 448/223/377): `ai.*` ports the two
entity wrappers `entity_dispatch_event5_and_set_flag` (0x447b00 — activity
request through corpus index 5, folding the corpus setter's
dispatch-bookkeeping sibling and raising bit 0x200000 of the +0x3e0 word)
and `entity_set_activity_place` (0x448c50 — corpus activity index 30 against
bit 0x200 of +0x3e4, answering true; the corpus's "place" name is unproven
against the name table, so no semantic claim). `ai_test` pins the activity
indices, the flag bits, and the null guards.

**Config-fed cache + grid leaves** (findings 273/118/487/244/444/372):
`level.*` ports `terrain_object_type_sfx_at_cell_get` (0x44ad80 — the
position record's pair indexes the terrain-object SFX grid under the corpus's
second-word-first fold; the 300-cell grid lands as zero-init storage with
hooks behind the unported config-parsing loader), `frontend.*` ports
`screen_right_edge_cache_refresh` (0x41a290, the index-0 metrics pair folded
into the cached right-edge word), and `media.*` ports
`loader_progress_callback_set` (0x4802b0, the loader-progress callback pair
with set/peek hooks until the corpus read tap lands). Three register/thunk/
vtable leaves join blocked.md (`CallbackOperationRunAndReturnStatus`,
`CrtNlgFrameStateRecord`, `SoundPositionSetFromVector`). `level_test`,
`frontend_test`, and `media_test` pin the indexing, the fold, and the stores.

**Scene-motion + state-word leaves** (findings 433/385/464/08/477/459):
`simtick.*` ports `simtick_scene_position_reset_and_effect_motion_cancel`
(0x44cf70 — the owner-transform copy onto the configured triple, which lands
as module statics with hooks behind the unported transform initializer, plus
the corpus's bit-7 clear on the scene-transform word), `entity.*` ports
`entity_global_animation_state_set` (0x4383a0, the animation-state counter
word with a peek), `savegame.*` ports
`selected_level_start_object_update` (0x460200, the bound-level checkpoint
read whose result the corpus discards — the apparent argument is calling-
convention residue), and `frontend.*` extends its timed-effect window with
`timed_effect_timestamp_reset` (0x46b7a0, the global tick word — a zero-init
hook — stamped into the stride-0x27c table's time field). Three display/
renderer-word leaves join blocked.md. `simtick_test`, `entity_test`,
`savegame_test`, and `frontend_test` pin the copies, stores, and guards.

**Linked chain free + editor/timer word folds** (findings 389/390/433/250):
`crt.*` ports `linked_record_chain_free` (0x46a370, the recursive successor
chain through the widened +0x1c link then the record free) and
`global_linked_record_chain_clear` (0x46a390, the aux word — DAT_004aa450,
unowned, a zero-init hook — then the head chain), with a head setter hook
standing in for the corpus's list owners. `simtick.*` ports
`simtick_route_position_blocked_state_set` (0x44da20 — blocked folds bit 9 in
and bit 10 out, unblocked applies the corpus's 0xffffedff mask keeping bit
10) over the scene-transform word and `simtick_global_ui_flags_clear_bits12`
(0x45b610, clearing bits 1 and 2 of the index-9 timer word the shared-timer
dispatch reads), each with a peek hook. `crt_test` and `simtick_test` pin the
chain clear, the masks, and the guards.

**Grid-cell record buffer + small record leaves** (findings 463/373/378/386):
`engine.*` takes the entity-cell field-grid trio — `grid_cell_record_buffer_allocate`
(0x4661c0, the `width * height * 0x0c` buffer with the corpus's masked zero-word
loop, the descriptor landing even on an alloc failure), `entity_field_grid_buffer_release`
(0x466210), and `grid_cell_record_buffer_replace` (0x466230) — over the two
DAT words as module statics. `view.*` ports `selection_record_point_get`
(0x436ae9, the follow-mode camera readback), `heap.*` ports
`object_field_38_release` (0x489d70, the +0x38 slot free), and `crt.*` ports
the `crt_no_op_stub_variant2` empty stub (0x493c40). `enginebind_test`,
`view_test`, `heap_test`, and `crt_test` pin the zero pass, the gate, and the
slot free.

**Stats flag tests + availability seams + free wrappers** (findings 319/14/480/428):
`entity.*` takes the three stats-word bit tests (`entity_stats_flag10/20/40_test`,
0x439cf0/0x439d10/0x439d30) masking the already-ported stats reader's word,
faithful to the corpus's bare masked answers. `media.*` ports
`media_cd_audio_playback_available` (0x488b90, both corpus words in: the
device word — a zero-init hook while the MCI opener stays unported — and the
sound-on seam), `simtick.*` ports `simtick_directsound_listener_view_set`
(0x47ab80, the listener slot store gated on the sound word), and `heap.*`
ports the two free wrappers `optional_resource_destroy` (0x40f7b0, the null
gate) and `text_buffer_destroy` (0x485190, buffer slot then record).
`entity_test`, `media_test`, `heap_test`, and `simtick_test` pin the masks,
gates, and guards.

**Panel/view blit + small global primitives** (findings 250/252/256/245/388/417):
the panel/view blit band ports `panel_item_flag8_set_if_enabled` (0x45abf0,
the item word's bit 8 under the bit-0x20 enable gate), `view_blit_rect_positions_apply`
(0x463d20, the view origin from the float pair plus the region-indexed pair,
the two rect records' conditional size-pair blits, and the metrics finish),
and `panel_texture_slots_allocate` (0x45ac30, building the stride-0x2c draw-record
block with the three texture streams through the modeled surface seam —
monotonic load handles and a recording buffer in `crt.*`) into `crt.*`, and
`panel_texture_slots_blit` (0x45a9c0, the flag-bit picker with the status-banner
word as the corpus's alternate-look gate) into `frontend.*`. The small global
primitives band takes `uint96_zero` (0x497aa0), `global_value_clear_if_equal`
(0x40e910), the two global-list pushes (0x46a470/0x4792c0, link at +0x1c),
`scene_effect_registry_reset` (0x465380), and `crt_fatal_message_exit`
(0x498340, abort default with a testable observer hook) into `crt.*`,
`objective_voice_alias_raise_gate_bit27` (0x454b80, the failed-result raise
under the modeled CD-state gate) into `objective.*`, and `level_random_seed_set`
(the shared draw word reseed, modeled module-locally per precedent) into `level.*`.
`crt_test`, `frontend_test`, `objective_test`, and `level_test` pin the
picker, the guards, and the windows.

**Input core + CD/STD fallback** (findings 04/05, 22, 01): `src/gods98/input.*`
models the corpus's three 256-byte key tables (`DAT_0076ab80/aa00/4de430`),
edge semantics (`cur && !prev`), `KeyToAscii`/`KeyContextCheck`, the
`EnginePreTick` text pump + mirror/clear, and the `QuitPoll` corner-dwell
machine. The `_DAT_004ffbc0` word behind the dwell slots was reclassified by
finding 22 as the **objective/advisor flag word** (not an input device), so
the dwell slot is caller-supplied here; the OS-level device *writer* rides
with the frame-loop milestone. The resource layer gained the CD/STD fallback
chain (`resource_set_cd_drive/root/data_dir`, `%c:\%s\%s`, `cd.key`/`delme.dat`
probes) per finding 01. `input_test` (suite `input`, no game data needed) and a
`test_cd_fallback` unit in `test_resource` cover both.

**Block-text config parser** (`src/gods98/block.*`, finding 12): the single
`Lego*` scanner every data file reads through — comment stripping (`;` to end
of line) with ` \t\r\n`-only tokenizing, nested `name { value }` records with
container value `"{"`, and rooted case-insensitive path lookup (`block_find`,
`"LegoRR Activities Activity_Teleport"` — the query side is the exe basename,
which is why data spells its roots `Lego*` with a prefix-wildcard that the
corpus gates to the first segment). Typed getters cover string/int/float/bool
with the corpus vocabulary `{TRUE,YES,ON}→1`, `{FALSE,NO,OFF}→0`, else/missing
`2`. `block_test` (suite `block`) runs synthetic grammar cases and, with
`test_wads_dir` set, parses **all 138 shipped block-text entries** (56 `.ae`,
44 `.ol`, 37 `.ptl`, `Lego.cfg`): 12,876 records digest-identical to an
independent token model, plus value spot checks (`Barracks.ae`
`Activity_Teleport = Create`, `IceMonster.ae` `HIGHPOLY` bone map,
`Lego.cfg Main TextureUsage = 3145728`).

**LWO model loader + `.uv` sidecar** (`src/gods98/model.*`, findings 11 §2–3,
§5b + 19): the LightWave-4-era pipeline entity assembly reads through —
`FORM/LWOB` chunk walk with PNTS f32-BE points, POLS rings (origin
auto-detected per file: any zero index anywhere means 0-based, else 1-based;
negative indices flag reversed winding), SRFS names, SURF records anchored on
the SRFS names reading only the engine's vocabulary (COLR/TIMG/TSIZ), and the
`.uv` sidecar grammar: per-surface texture-path lines replace the LWO TIMG,
per-corner uv blocks attach with v negated, and the keyframe tail stops the
scan without invalidating what parsed — exactly the reference behaviour.
`model_test` (suite `model`) runs synthetic loader cases and, with
`test_wads_dir` set, parses **all 1,132 shipped `.lwo` entries**, applies
**all 60 `.uv` sidecars**, and digest-checks the corpus (36,506 vertices /
20,119 polygons / 3,396 surfaces) against the `rrlwo.py` ground truth, plus
spot checks (the four 0-based POLS outliers, `Arml.lwo`'s sidecar paths).

**Entity-visual assembly** (`src/gods98/entity.*`, finding 12 §4): the `.ae`
per-entity activity table the factory `FUN_00472cd0` builds — kind dispatch
(split `:` with the NOTEXTURE flag, 9-slot table `{NULL,MESH,FRAME,ANIM,ACT,-,-,
LWS,LWO}`), the `<name>\<basename>.ae` open, root `SCALE`, then the
`Activities` rows in document order: `FILE` gating, `TRANSCOEF`/`TRIGGER`
defaults, `LWSFILE`/`LOOPING` bools, `SAMPLE`, and the `!`-key
reduce-animation gate (feature word bit `0x400000`). Scene instantiation
(`FUN_00475d10`, `FUN_004765c0`) is a documented seam — the module ends at
the parsed table. `entity_ae_test` (suite `entity_ae`) runs synthetic
grammar cases and, with `test_wads_dir` set, assembles **all 56 shipped
`.ae` files** under both feature modes: 439 rows digest `e4e3630c` (default)
and 354 rows digest `fc217039` (`-reduceanimation`), cross-checked against
the `tools/rrae.py` ground-truth model.

- **`native`** (primary, end goal): compiles the reconstruction C++ against
  **miniwin** (SDL3) — no Wine layer. Host compiler, no cross file. miniwin
  comes in as a sparse git submodule — bootstrap with `tools/setup-miniwin.sh`
  (blob-none + cone sparse to `miniwin/` only; the sibling game's decompiled
  source is never written to the working tree).
- **`winapi`** (reference): cross-compiles the same source to a 32-bit Win32
  `LegoRR.exe` (MinGW-w64) for A/B against the shipped binary under the A.2
  harness — one environment, direct comparison (ADR 0002).

```sh
# native (primary)
meson setup build-native -Dplatform=native
tools/build-miniwin.sh                     # builds the real miniwin archive —
meson compile -C build-native              # meson's cmake bridge alone leaves
                                           # libminiwin.a empty (EXCLUDE_FROM_ALL)

# winapi reference build
meson setup build-winapi --cross-file cross/i686.ini -Dplatform=winapi
meson compile -C build-winapi
```

**Frame loop** (2026-10-21): the native build opens a 640×480 window, mounts
the shipped wads, renders real corpus textures (`ROCKROOF.BMP`,
`info_path.bmp`) through the texture loader onto a miniwin DirectDraw
surface, and flips at ~30 fps — the first visible proof of the port. Env
knobs: `LEGORRR_WAD_DIR`, `LEGORRR_AUTOSTOP_FRAMES` (headless/CI), and
`LEGORRR_SCREENSHOT=<path>` (PPM of the last frame). Run it from the game
directory (or set `LEGORRR_WAD_DIR`) — it exits cleanly on ESC/close.

**Front end — the main menu** (2026-10-21, finding 23): the native build now
**boots into the main menu before any level loads**. `src/game/frontend.*`
ports the corpus's modal front-end: `frontend_build` reads the four menu
groups (`MainMenuFull`/`SaveMenu`/`PausedMenu`/`OptionsMenu`) with the
shipped `Menu::<name>` query spelling (the `::` group separator,
`DAT_004aab18`, is now a `block_find` token boundary too), parses the
`Item%i` grammar (`Next`/`Trigger`/`Slider`/`Cycle`), and grafts the level
pickers — **`StartLevel`/`TutorialStartLevel` + `NextLevel` chains** at each
level's `FrontEndX/Y` scatter — onto the `LevelPick*` submenus.
`frontend_tick` runs the modal loop one frame: arrows select, Enter/Space
activates, Esc backs out (root Esc quits); a picker row returns
`FE_EVT_START_LEVEL` and the **frame loop loads that level afterwards**;
`FrontEnd FALSE` in `Lego.cfg` (or setting `LEGORRR_LEVEL`) skips the menu
straight into a level, like the classic demo. The menu now **looks like the
game**: the frame loop blits each submenu's `MenuImage` art behind the
items, and `frontend_render` draws the labels with the **shipped bitmap
fonts** — `LoFont` for unselected, `HiFont` for the selected row (without a
blue backing rectangle), plus the shipped `Pointer_Standard` bitmap cursor.
FLIC overlays honor the black transparency key and stay inside the framebuffer
at negative x offsets. The delta decoder reads packet skip/control bytes in
their on-disk order
(FUN_0047a470's 10×19 ASCII grid: glyph g = ch-0x20 at (g%10, g/10), cell =
width/10 × height/19, e.g. 380×817 → 38×43; each 8bpp face keys **both**
palette entries 0 and 255 transparent (FUN_0047d9d0→FUN_0047e050 — the
`Menu_Font_*` grid is black + pure red, `MenuFont*` (227,0,0) + (255,35,255);
surfaces without palette info keep the port's dominant-colour sampler as a
fallback); each glyph
blits as its measured **ink box**) — with the port's
own 3x5 typeface kept as a headless fallback. **In-game pause** (2026-10-21):
`Esc` in a level opens the shipped `PausedMenu` overlay (Continue / Options /
Restart / Quit, with mouse + keyboard like the main menu) instead of quitting
— Restart reloads the level in place, and `LEGORRR_AUTOPAUSE_FRAMES=N`
headlessly opens the pause at frame N for screenshots. The `!Overlay%i`
FLIC decorations (finding 21 — `avi\PB1.flh` etc.) animate on the menu
backdrops. **Load slots** (2026-10-21): the Load_A_Saved_Game screen lists
the on-disk slots (`SaveMenuBuild @ 0x415a10` rows fed by `save_slot_state`)
— picking one resumes the furthest completed record's campaign level and
restores its checkpoint (task + reward table) over the seeded mission. Each
level's checkpoint now lands in **its own record** (port of
`CheckpointStep`'s per-level `ListFindName` index:
`fe_record_for_level`/`fe_level_for_record` map levels↔records — tutorials
first, then the campaign chain — and the record array grows on demand), so
mid-campaign saves neither collide nor resume short.
**Options** (2026-10-21): the Options/Advanced `Slider`/`Cycle` items respond
to `←`/`→` (and render a live bar+knob / current option) and are **functional**:
the values live in a store on `GameContext` that the menu seeds from and writes
back into, so re-entering Options (any instance — the pause submenu or the
standalone OptionsMenu) shows the effective value, never a reset default.
**Game Speed** (`Game_Speed`, 0..5) scales the frame loop's simulation time
(`options_time_scale` → `EngineTimeScale` and the ai/objective tickers' dt) —
1 = normal. **Brightness** (`Brightness`, 0..10) multiplies the presented frame
by BrightnessSet's `value × 0.1` scalar. **Help Window** (`Help_Window`
Off/On) toggles the in-game help panel (the option gates the panel in
`frontend_help_overlay`, so it can never draw while Off). The volume families
(`SFX_Volume`, `Music_Volume`) keep driving the audio seam — the frame loop
opens audio with the store's SFX gain, so settings restored from a save take
effect immediately. The values load from the active save context when present
(`.sav` header option block; a fresh game and un-stamped legacy saves use the
defaults — speed 1, SFX 8, music 5 per corpus `OptionsLoadOrDefault`,
brightness 8 as the port's neutral default since the corpus assigns none)
and survive save/reload. Suite: `options` (menu-to-runtime for each control,
saved-value round trip, plus a `--real` probe asserting the shipped control
names + ranges).
**Pointer drag**
(2026-10-21): holding the left button inside a slider's corpus-style 100×20
bar rect (`FUN_00411870 @ 0x411870` semantics: value tracks pointer x as
`round(clamp((mx − x) × 0.01, 0, 1) × (max − min))`, re-evaluated every
frame the button is held) drags the value; release ends the grab.
**Picker scroll** (2026-10-21): `CanScroll` sub-menus pan their whole item
plane (corpus `FUN_00413970` / `DAT_005574e0` model — draw and hit-test
share the offset, clamped to the content height, reset on re-entry); the
grafted campaign/tutorial picker rows sit on one tall plane, so all 24
campaign levels are now reachable with the pointer. New key:
**`PLATFORM_KEY_ENTER`** at the seam. Suites: `block` (`::` paths) and
`frontend` (fixtures + a `--real` probe that builds the whole front end from
the shipped `Lego.cfg` — 24-level campaign chain, 8 tutorials, 32 picker
rows).

**Interactive input** (2026-10-21): the window now captures keyboard/mouse
into the engine's `input.*` tables (the finding-05 device-writer half):
platform events pump into a canonical-key vocabulary, `input_capture_events`
fills `cur` (edges vs `prev` per findings 04/05), and the **arrow keys pan
the blitted texture**. Unit coverage for the capture/mirror/two-frame flow
lives in `input_test`.

**Audio playback** (2026-10-21): the merged sound bank now plays. The seam
gains a DirectSound-analog audio surface (`platform.h` `PlatformAudio`, one
shot = mono 16-bit PCM over SDL), and `src/gods98/audio.*` converts any bank
sound (8/16-bit, mono/stereo, any rate) and submits it. The frame loop plays
a startup SFX and re-triggers it with **Space**. `audio_test` (suite
`audio`, `SDL_AUDIODRIVER=dummy`) covers the conversion (incl. 8-bit and
stereo-mixdown) and a real `.wav` play.

**Audio depth** (2026-10-21): volume, looping, and 3D-ish panning.
`platform.h` gains gain (`SDL_SetAudioStreamGain`), a continuous **loop
track** (stereo PCM re-seeded on change + per-frame refill), and stereo
submits; `audio.*` adds `audio_set_volume` / `audio_play_sound_3d` /
`audio_loop_named` / `audio_update`. The demo loops `Sounds\Iambloop.wav`
spatialized by the texture's pan position (the blit is the sound emitter),
with **`[` `]`** adjusting volume.

**Engine simulation fills** (2026-10-21, first tranche): the EngineTick
camera/move cluster (`FUN_0046a730`, `FUN_00435a50`, `FUN_00431560`,
`FUN_0042c440`, `FUN_0042cb70`, `FUN_004261c0` — finding 03 §3) is ported
into `src/gods98/camera.*` with its observed structure over the `EngineState`
grid/move-list fields; cross-cutting helpers (message bus `FUN_00419b80`,
spawn `FUN_0040f190`/`FUN_00438960`, move `FUN_0046a700`) remain stub
boundaries with corpus addresses. `camera_test` (suite `camera`) covers the
timer-decrement, grid-expiry, and spawn-countdown logic.

**Tranche 2 — the view contexts** (2026-10-21): `src/gods98/view.*` ports
`ViewContextTick` (`0x435f5c`, finding 04) — the ×3-per-frame camera state
machine (modes 1 follow/edge-clamped, 2 scripted-lag, 3 free/cruise with the
glide-to-stop tail). Camera handles are record-backed (`cam_*`), the pure
vector math is implemented, and world/object boundaries (grid pick
`FUN_0044f6d0`, terrain `FUN_0044f940`, EMA blend `FUN_0044a070`) stay stubs.
`EngineTick` now drives the three views (`DAT_005560e4/e8/ec`); `view_test`
(suite `view`) covers the vector/glide/edge/blend paths.

**Tranche 3 — the message bus** (2026-10-21): `src/gods98/message.*` ports
`MessageDispatch` (`0x419b80`) + `MessagePreCheck` (`0x4199f0`, finding 04)
— the 40-id dedupe-with-live-callback notification bus, seeded with the
finding-22 `Info*` catalog, with the auto-cooldown rule + linked family reset
({10,11,12,38}), the exception-id dedupe skip, and the 40-slot active queue.
It replaces the camera module's `message_dispatch` stub (the grid-expiry now
fires into the real bus) and the air-beat tick path dispatches `0x11`/`0x12`.
`message_test` (suite `message`) covers gates, dedupe/exceptions, cooldown and
the cap.

**Tranche 4 — air-beat sound hook** (2026-10-21): `EngineAirBeat` drives the
looped `SND_AirBeat` through the audio seam when the air supply runs low
(`airbeat_set_loop`), and the finding-03 notifications `0x11`/`0x12` dispatch
through the bus; the demo drains air so you can hear the cooler-to-air-beat
change (`airbeat_test`, suite `airbeat`).

**Tranche 5 — `MainSimStep`** (2026-10-21): `src/gods98/mainsim.*` ports the
input/app-state step (`0x4250c0`, finding 04) — quit gate (keyed to the named
quit key + `EngineKeepRunning`), the debug/cheat toggles (Frame Rate Monitor
`0x40`, Memory `0x80`, Sound Effects `0x10`, Music) gated by the debug word
`0x10`, the air cheat, camera keys (per-view step boundary), and the o1/o2/o3
edge outputs. The corpus's raw per-key scan slots aren't pinned by the
findings, so named canonical key bindings are used (documented). `mainsim_test`
(suite `mainsim`) covers the toggles, outs, air cheat and quit gate.

**Tranche 6 — first subsystem tickers** (2026-10-21):
`src/gods98/simtick.*` ports a first batch of the remaining dispatch-table
tickers with fresh decompiles: the mute timer (`FUN_0041ba10`), the
effect-table dispatch (`FUN_0040aa10`), the bobbin list flag machine
(`FUN_004091e0`), the post-step cluster (`FUN_0040d450`), and the grid marker
sweep (`FUN_0044eff0`). Their ticker-table entries now delegate to the
module; helpers stay hook boundaries. `simtick_test` (suite `simtick`)
covers all five.

**Tranche 7 — `ManagerTick`** (2026-10-21): simtick grows the 12 positional
trackers (`FUN_0045a730`, finding 04) — per-slot target selection (source A
floats / source B ints on flag bit 4), the exponential smoothing
`prev=(A/dt·prev+t)/(A/dt−B)` with the snap threshold, mode routing (reposition
vs screen-attach, with the `~state 0x40000` ingame bit), the **index-9
four-timer stop**, and the near-zero settle that clears flag bit 3. The
engine's manager dispatch drives it. `simtick_test` covers convergence,
snap, source-B, mode-1, timer-stop, and settle.

**Tranche 8 — tickers incl. the cooldown tick** (2026-10-21): simtick adds
the sound keep-alive (`FUN_0047aba0`), the editor step cluster
(`FUN_0044cfa0`), and the **message-bus cooldown tick** (`FUN_0041a2c0`,
which now decrements the merged bus's per-id cooldowns —
`message_cooldown_tick`), plus the bit-field tree helper (`FUN_0049cf90`);
their ticker-table entries are wired. `simtick_test` covers all four.

**Tranche 9 — five more tickers** (2026-10-21): the ordered post-step
(`FUN_004665c0`), the minion activity state machine over the state's `+0x88`
list (`FUN_004313b0`), the **grid-cell flag pass** over the shared camera
grid (`FUN_00424600`, stride 0x48 cells, bits `0x800/0x1000/0x2000000`), the
scene pass with its flag word + 125 s broadcast timer (`FUN_0043b270`), and
the effect-bank ticker (`FUN_0046f550`). `simtick_test` covers all five.

**Tranche 10 — the dispatch table is complete** (2026-10-21): the last eight
EngineTick subsystem stubs are ported — the world picker (`FUN_00425d90`,
mouse-distance gated), the task/frame counter (`FUN_00402220`), the ambient
spawn accumulator (`FUN_0040eab0`), the sound-bank lifetime ticker
(`FUN_00465b00`), the render-pass entry (`FUN_004291a0`), the render step
(`FUN_00449c00`), the vu/timer accumulators (`FUN_00426520`), and the fx-bank
ticker (`FUN_0046e390`) — leaving **zero `TODO` stubs** in `tickers.cpp`
(`FUN_004520d0` is owned by the game-layer command executor). All 25
dispatch entries are now live ports or forward to ported modules.

**Engine <-> level binding** (2026-10-21): `EngineContext.level_block` binds
a loaded `LevelState` (game/level.h) into the engine; `engine_level_*`
accessors read width/height/voxels at the corpus offsets
(`+0x14/+0x18/+0xb8` — the same offsets our `EngineState` grid model uses),
`view_tick` now takes the context and picks terrain from the voxel surface
byte (`+0x04`), and the camera grid-expiry prefers the bound level. The demo
binds `game.level` after load. `engine_bind_test` (suite `enginebind`) covers
the accessors + view terrain pick.
**Shipped `Lego.cfg` integration status:** the two issues root-caused in finding
23 have since been addressed in the port. `block_find` accepts `::` as a path
boundary, the front end builds campaign/tutorial chains from
`StartLevel`/`TutorialStartLevel`/`NextLevel`, and the level loader resolves
groups under the `Levels` container. Selecting a picker row reaches the frame
loop's level-load path. The `LEGORRR_LEVEL` fallback of `Level01` remains a
fixture/demo convenience and is not a level name in the shipped config; use a
picker row or a shipped level name such as `Tutorial01`. A real-data check of
the complete picker-to-map-load path remains useful; the optional WAD-backed
probes described below cover front-end construction and level-group lookup.

**Game layer — levels, objectives, savegames** (2026-10-21): the mission
plumbing that sits on top of the engine (findings 07/08/22) is now
reconstructed, on top of the merged block-text parser (`src/gods98/block.*`,
finding 12, #14). Three new modules in `src/game/`:

- **Level loading** (`src/game/level.*`, finding 07): `level_load` reads a
  level group's ~35 config keys from `Lego.cfg` into the 0x284-byte level
  block, then the 17-arg `MapLoadPipeline` port materialises the ten `.map`
  grids into the w×h×0x48 voxel array — TerrainCreate (grid + physical 0x28
  block table), the nine layer writers (erosion/cryo, surface, path/AI,
  emerge, predug, fallin flags, block pointers, layer L/M records) and the
  CellInitPass sweep, all in the corpus's positional key→loader order.
  `.map` files are read raw and cells accessed in place (`level_map_cell`,
  finding 07 §format).
- **Objective/advisor** (`src/game/objective.*`, finding 22): the
  `fun_00457d40` config loader (advisor-suppression keys → word bits
  0x10/0x20/0x80, crystal/ore/block/timer/construction targets at the
  corpus offsets), the checker/announcer/state-machine triplet with the
  full runtime bit flow (0x1 pending / 0x2 achieved / 0x4 failed /
  failed-with-crystals, 0x8 announced-once) and the seconds×25 timer rule.
- **Savegames** (`src/game/savegame.*`, finding 08): `Saves\\<slot>.sav`
  (0xB0 header + 5 × 0x3190 records) and `Saves\\<slot>.osf` (`'RO'` magic,
  count, 20-byte entries) with the slot context, mark/write/commit, the
  12,684-byte checkpoint blob (reward + 10-task table, Level03 tutorial
  byte), the `CheckpointStep` bootstrap-commit flow, and `OsfAdd`/snapshot/
  write.
- **Tasks + reward screen + object lists** (2026-10-21, finding 09/10):
  `src/game/mission.*` ports `TaskBlockInit` — the ten named task slots
  (Crystals…Score, stride 0x210) seeded from the level's `Reward` group with
  the six field loaders (Images/BoxImages/Flics/Fonts/Sounds config
  capture + flag bits), `TaskImportanceRefresh`/`TaskQuotaRefresh`,
  `CrystalQuotaSetup` (≤26), and the `RewardInit` display keys
  (Wallpaper/Display/CentreText/ScrollSpeed/VertSpacing/fonts/texts/TextPos
  plus the Save/Advance button hit-zones). `mission_update` is the
  live-progress bridge: it feeds crystal/ore counts into the objective
  checker's slots, advances task values and marks completions. `mission_test`
  (suite `mission`) covers names, importance/quota, the crystal cap, field
  loaders, reward keys and completion.
  `src/game/entity.*` models the corpus's entity tables (`EntityPointer`
  strides) and loads the `OListFile` `.ol` object list into them
  (`object_list_load`, **decompile- + corpus-verified**): the grammar
  `type <name>` / `xPos` / `yPos` / `heading` / `health` (default 100) /
  `driving` was pinned against **all 44 shipped `.ol` entries** (725 object
  records placed, 44 TVCameras → follow-camera, 90 unregistered dev `Test1`
  records skipped) by an independent parser in `entity_test --real`, and the
  `type`-name → code resolution is `FUN_0042e8b0` (`ObjectTypeByName`):
  codes 1–4 from the shipped `Lego.cfg` `VehicleTypes` / `MiniFigureTypes` /
  `RockMonsterTypes` / `BuildingTypes` groups, special kinds 5–19
  (PowerCrystal 6, Boulder 5, Ore/ProcessedOre 7, Dynamite 8, Barrier 9,
  ElectricFence 11, SpiderWeb 12, OohScary 13, Path 15, Pusher 16,
  Freezer 17, LaserShot 19), TVCamera −1 (follow camera), unregistered
  names skipped (the corpus aborts the whole load). The special kinds share
  a fifth table; the corpus keeps per-kind singletons. `entity_test` (suite
  `entity`) covers placement, stride/table caps, the camera/health/unknown
  paths and the corpus probe.
- **Command subsystem** (2026-10-21, finding 10): `src/game/cmd.*` ports the
  entity command primitives — the 0x68-byte record doubling **pool**
  (`CmdAlloc`/`CmdFree`/`CmdPoolGrow`), the **registry** (`CmdRegistryInit`:
  the 31 AI task-type names + 27 AI priorities in address order and the
  behaviour table, `CmdKindByName`), the **creators**
  (`CmdCreateMove`/Target/Gated/Task incl. the type→kind mapping and
  `MinionCompanionCmd`), the **chain/global-list** link ops, `CmdChainPurge`,
  `IdleQueueRegister`, and the **queue executor** (the 5-dword opcode switch
  of `FUN_004520d0`, run in the frame loop). `cmd_test` (suite `cmd`) covers
  pool doubling/reuse, the registry tables + behaviour defaults, creators,
  chains and queue dispatch.
- **Selection + pointer input** (2026-10-21, finding 06): `src/game/select.*`
  reconstructs the selection cluster (`DAT_004fef2c` array + `DAT_004ff298`
  count, `SelectedEntity`), add/dedupe/cap + the selected flag, a
  grid-cell **pick** (the "click the grid → select" path), and the
  pointer→command routes: opcode-5 (position → `CmdCreateMove`) and
  opcode-8 (add-to-selection) through the double-buffered queue. The queue
  executor gained its faithful producer `cmd_produce` (`FUN_00452060`,
  silent drop when full) and the **cheat-keybind pre-pass** (`cmd_bind_key`
  + the held-key probe). The demo maps the tracked mouse to a grid cell:
  `1`–`4` picks the nearest placed entity, `m` moves the selection to the
  cursor cell — the opcode-5 dispatch lands in `CmdCreateMove`, so the picked
  entity walks the **wave-built path** (see the AI block below) around
  blocked terrain instead of a straight hop — and `n` clears.
  `select_test` (suite `select`) covers add/dedupe, pick, pointer-move
  dispatch and the op-8 path.
- **AI execution receivers** (2026-10-21, finding 10 closed): the
  entity-manager tick (`FUN_0043b270`) is now decompile-covered — its walker
  runs the per-entity callback at `0x43c810`, which is a **flag-word
  behaviour evaluator, not a command-record switch** (`DAT_004b32e8` ids have
  xrefs only in the creation layer; no tick reads a record `+0x1c`).
  `src/game/ai.*` ports the verified branches in corpus order against entity
  mirrors: activity completion (`+0x3ec` bit `0x200000` + `FUN_00447c40`
  bookkeeping + idle-record attach), the wake timer (`+0x3e4` bit
  `0x8000000`, `+0x368` countdown early-return), target acquisition
  (`+0x3ec` `0x40000` → link `+0x354`), the cheat-word flag (`DAT_00556ed0`),
  and type-1 idle registration (`FUN_0043a960`, cap 100) — plus the
  77-entry `DAT_004decc4` activity-name table (`Activity_Stand`…
  `Activity_Waiting21`) with `FUN_004409e0` set semantics. Move commands
  flow through the **path-list mirror** (finding 10 §move branch): the
  entity waypoint buffer (`EntitySlot.wp`, corpus `+0x24`/`+0x28`/`+0x2c`,
  pop = entry pending-flag clear at `0x43cd90`) is seeded one hop by
  `cmd_create_move` together with the `+0x3e0` bit-8 mover flag; `ai_tick`
  walks the entries in order, popping each, and completes the command with
  the flag clear on exhaustion. Tile semantics are wired: `level_walkable`
  (`FUN_00432770`: margin, flag `0x10`/`0x40`, blocked types 1/7/11) gates
  the walker's current cell, `level_tile_mark` (`FUN_00432e00`) raises the
  `0x8000000` reservation while moving and drops it on arrival, and the
  idle registration probes the `'\t'` tile (`FUN_0043a960`) — mapless
  fallbacks keep the corpus-free unit tests unchanged. Move commands now
  install *built* paths: `cmd_create_move` resets the list per command
  (corpus dispatch install, `0x4418d0`) and `ai_path_build` appends one
  entry per walked cell (`0x44693b`'s per-cell appends), falling back to
  the one-hop seed when there is no grid or no path. The builder runs the
  ported corpus *wave-flood* pathfinder (`FUN_00430ee0` seed + spread and
  `FUN_00430530` traceback over the port's level-grid handle): a
  target-validated seed raises the wave flag `0x40000` over its northwest
  2x2 footprint and every spread cell, blocked (`flags & 8`) cells are
  refused by the ported walkability/occupancy queries (the mover's own
  cell exempt — it reserves its current cell every tick), and the
  traceback walks the wave back to the start cell, stamping it `5` along
  the way. The flood work state is
  scoped per path build — created and released inside one `ai_path_build`
  — so it never leaks between commands. Two flag branches that were
  approximated are now real: wake-timer expiry mirrors `FUN_0044c110`'s
  writes onto the attached `+0x364` target (`+0x340 = 100`, `+0x3e8` bit
  `0x800000`, gated by the target's `+0x3e0` bit `0x200000`), and the
  cheat-word branch runs the `FUN_0044c530` can-do probe (type-2 with a
  drill/teleport/reinforce check `FUN_0043c440` over the corpus's real
  *permitted-activity list* — `+0x370` id slots / `+0x384` count,
  `FUN_0044b480`'s scan, the FIFO append at capacity (`0x44b44a`/`0x44b417`)
  and the special-value in-place replace (`0x44b463`) via the install
  appender `FUN_0044b370`; vehicle spawns seed `Activity_Stand` (`0x437eb6`)
  and the deferred-install branch `0x43da62` installs the chain-head
  record's `+0x38` grant (then drops the type-0x13 grant records,
  `FUN_00403d10(ent, 0x13, 0, 1)` — `cmd_chain_drop`). The minion
  task-state granters close out the same story (2026-10-21): the spawn
  dispatch's `0x438150` switch (<code>type−8</code> → selector table
  `0x4382ac` → jump table `0x438298`) points the minion types' `+0x334`
  stats block at the static templates `DAT_004a98f0` (type 8),
  `DAT_004a9a40` (type 9) and `DAT_004a9b90` (types `0xc..0xe`/`0x10..0x13`)
  — `ai_minion_state_for` — and the state's `+0xcc` capacity
  (`FUN_0046a060`, `EntitySlot.activity_cap` via `ai_activity_cap`) now
  sizes the same appender per entity. `ai_test` pins the selector, the
  minion spawn state/cap assignment, granted-task probes against a
  cleared-list failure, the per-entity cap gate, and the type-0x13 grant
  record cleanup. The demo key `g`
  issues a task for the selection. `ai_test` (suite `ai`) covers activity
  set/complete bookkeeping with the idle-record attach, wake-timer freezing
  (plus the wake mirror and its attached-state gate), link acquisition, the
  cheat flag driven off the permitted-activity list (spawn seed, special
  replace, list clear), the FIFO install at capacity, and the deferred
  `0x43da62` command grant, idle registration (with the `'\t'` probe and its off-tile
  block), single- and multi-waypoint walking with the arrival pop,
  blocked-tile freezes with the occupancy mark/clear, and the
  queue→executor→move path plus a dispatch rerouting around a blocked
  column. With `test_wads_dir` set the suite gains a `--real` probe: the
  builder runs against all 42 shipped `.map` levels — sampled pairs
  checked for gate validity (paths never land on blocked, unwalkable, or
  reserved cells), agreement with an embedded reference oracle mirroring
  the pre-port BFS stand-in (reachability and shortest length, reroutes
  included), and ticked-to-arrival command walkers. It passed green and
  surfaced one loader gap: the predug edge-wall update is the load-time
  source of the traversable bit (ported as `predug_wall_cell`, see
  finding 07), without which shipped maps have no walkable cells. An
  explicit `--real` run fails if the wads or a level cannot load, if a
  level has too few walkable cells or walkers, or if the shipped corpus
  totals differ from 42 levels / 496 pairs / 350 paths / 84 arrivals.
- **Per-terrain movement speed** (2026-10-21, closing the finding 10
  per-terrain placeholder): the `FUN_00469cc0` speed probe — the corpus's
  per-terrain movement rates, read from the `+0x334` stats block's table
  at `+0x24 + terrain*4` (seconds to cross one cell; the move branch
  accumulates `dt / entry` per cell, `FUN_00434570`) — is ported as
  `ai_stats_speed`, the table modelled as `GameContext.stats_speed_sec`
  indexed by the cell's terrain type through the level tile accessors.
  `step_path` now steps the mover at the *terrain-under-foot* rate (cell
  via `level_cell_of`, type via `level_tile_type`) instead of the flat
  `AI_MOVE_SPEED_CPS` constant, with the tick's overflow carrying into
  the following waypoints, so slow terrain visibly slows a walker and
  fast terrain quickens it; unset entries, out-of-range types, and a
  mapless context fall back to the port default. The corpus's zero-entry
  walkability gate and the runner-bit scale stay behind the unported
  `+0x334` state objects / unrecorded constants (TODO). `ai_test` pins
  the probe reads and fallbacks and a slow/fast terrain band against a
  plain-row walker, arrival pops and the occupancy clear unchanged.
- **Advisor + message-bus runtime** (2026-10-21, finding 22): the announcer
  now builds the `Stream Objective_…` advisor text (`DAT_004ffe38` analog)
  on every outcome, and `objective_advisor_shown` applies the 0x10/0x20/0x80
  suppression rule; the frame loop displays the hint (with a sound when a
  suppression bit is clear). `mission_incident` is the game layer's path
  into the merged 40-id `Info` bus (#18) — AI gather deposits now dispatch
  the matching incident ids (`CrystalFound`/`OreSeamFound`/`Constructed`).
  Per-id sounds come from the **`InfoMessages` config section**
  (`message_load_infomessages`, finding 22 §1 — entries by Info name,
  `Info_`-prefixed handled, `AlwaysPlaySFX`/short values set the play-always
  flag), and the demo sound hook plays the configured wav with a stand-in
  fallback. `objective_test`/`ai_test`/`message_test` cover the stream, the
  suppression rule, the bus dispatch and the config population.
- **Engine ↔ level binding** (2026-10-22, seam #35 completion): the engine
  reads the loaded `LevelState` — the *resolved* voxel/terrain pointers are
  bound at the seam (`EngineContext.level_voxels`, since the 32-bit corpus
  slot truncates a 64-bit pointer — the raw `+0xb8` word fallback stays for
  winapi). `engine_level_blocksize` exposes the world scale; the demo seeds
  the follow camera at the grid centre and enables the camera grid sweep
  over the real voxel grid. `bind_test` (suite `bind`) loads a fixture
  level, binds it, and asserts the engine reads its dims/blocksize/voxel
  bytes, clamps the follow camera inside the level, and walks the real grid
  in `camera_grid_expiry`.
- **Reward screen + save-zone clicks** (2026-10-21, finding 09): closes the
  reward overlay's button story. `mission_reward_zone` ports
  `RewardZoneCheck @ 0x461dd0` — the `SaveButton` zone (live when the level
  result index is "achieved") and `AdvanceButton` zone (otherwise) hit-tested
  against the tracked mouse, and `mission_reward_click` runs the
  `FUN_00462780` handler → `CheckpointStep`. The overlay shows
  automatically when an objective resolves (or press `y`), the renderer draws
  the two button rects, and a **real mouse press** (SDL button forwarded
  through `PlatformEvent.button` into the input tables — `input_mouse_pressed`)
  or `b` commits the checkpoint. **Advance** additionally runs
  `mission_reward_advance` — the next-level progression: it checkpoints the
  current progress, tears the level down and loads + re-seeds the parsed
  `nextlevel` chain (finding 07), so the reward screen now *drives a
  campaign*. `mission_test` covers the zone hit tests, the click path and
  the Level01→Level02→campaign-end advance chain.
- **Save thumbnails** (2026-10-22, finding 08): the corpus's save-confirm
  dumps a framebuffer thumbnail to `Saves\<slot>.dat` (`FUN_00413b80` →
  `FUN_0047e440` → `FUN_0047c8f0` — a plain uncompressed 24bpp BMP, bottom-up
  dword-padded BGR rows). `save_write_thumbnail` reproduces that layout
  byte-for-byte, and `LegorrrRun` writes the `.dat` for the frame it presents
  when the reward screen's Save click commits the checkpoint — the port's
  stand-in trigger for the corpus's save-menu confirm (the port's save menu
  loads slots; it has no save-to-slot confirm yet). The `savegame` suite
  checks the exact header fields, the bottom-up row order, and that the
  dump round-trips through the port's own BMP loader.
- **Software level renderer** (2026-10-21, finding 22 §3): `src/game/render.*`
  is the port's own rasterizer — D3DRM is confirmed software-only, so the
  reconstruction draws the level itself. `level_render_map` paints the
  terrain grid from the voxel array (material colours by the documented
  cell semantics: dirt/crystal/ore/rock + surface-byte shading), overlays
  the placed entities as live type-coloured markers (yellow ring when
  selected) at their current `(x,y)` — so AI movement is visible — and
  outlines the tracked mouse cell underneath the selection cursor. The
  palette is ours, not a clone of the shipped one. When a level is loaded
  the frame loop draws the map instead of the placeholder texture blit.
  The **reward overlay** (shown when a mission resolves) dims the level,
  blits the `Reward Wallpaper` texture stretched to the frame, draws a
  state banner (achieved/failed/quit + centre-text strip) and the
  Save/Advance buttons with a bright hover fill on the one under the mouse.
  `render_test` (suite `render`) asserts the cell colours, the entity
  marker, the selection ring, the cursor outline, and the overlay's dim/
  banner/hover/wallpaper pixels.
  **Model-aware entities** (2026-10-22, finding 30): the `.ol` loader
  parses an optional `Model` key per object and resolves it through the LWO
  loader (`entity_set_model`, one cached `Model*` per path, process-lifetime
  for the demo); the renderer projects each model's vertices top-down
  (x,z → screen) and draws its poly wireframe in the entity's colour,
  replacing the placeholder dot (the dot remains as the no-model fallback).
  `render_test` builds a synthetic LWO diamond and asserts its projected
  corner; `entity_test` checks the `Model` key lands in `model_path`.
- **AVI movies** (2026-10-21, findings 14 §4 + 23): `src/gods98/movie.*`
  reconstructs the corpus's movie path — `AvisEnabled @ 0x414ce0` (the
  `DontPlayAvis` gate), `MoviePlayNamed @ 0x415910` and
  `MoviePlayLevelVideo @ 0x415990` (config-key and level `Video` paths through
  the DirectShow frame-wait helper). `MoviePlayLoop @ 0x4157c0` is a separate
  timed bitmap-surface display helper. See [Finding 418](docs/findings/418-movie-stream-and-audio-playback.md)
  for the native call paths. The
  shipped `Data/AVI/*.avi` files (Indeo5/msvideo1 video, adpcm_ms/pcm audio)
  are decoded in-process by **FFmpeg's runtime libraries dlopen'd at
  first use** — no header, no link dependency, so the build stays
  dependency-free and LGPL-clean; where the libs are absent (or the API
  version < FFmpeg 7) playback reports unavailable and is skipped. The frame
  loop plays the boot films before the front-end end (LMI logo `LegoAvi`,
  then the intro `RRAvi`), gated by `AvisEnabled` / `LEGORRR_NO_MOVIES`,
  skipped under `LEGORRR_AUTOSTOP_FRAMES`, and plays each level's `Video`
  cinematic after `level_load`; ESC/Space/Enter/click skips, the audio track
  feeds the PCM seam, and frames stretch-fill the 640×480 surface.
  `movie_test` (suite `movie`) covers the config surface + scaler always,
  decodes a synthetic DIB/rawvideo AVI end-to-end when FFmpeg is present,
  and with `--real <dir>` pins `Data/AVI/intro.avi` (320×200 @ 15 fps,
  22050 Hz, 3821 frames). The dependency is a **meson feature option**:
  `-Dmovies=auto|enabled|disabled` (default `auto`) probes the FFmpeg runtime
  at configure time and reports the result (`enabled` fails the configure if
  the libs are absent; `disabled` compiles the module out and drops the movie
  suite). Meson owns the soname lists and passes them to the module and the
  probe as defines, so the build and the runtime never drift apart, and
  `libdl` is linked where old glibc needs it. On macOS, the probe also checks
  Homebrew's FFmpeg formula prefix and its `.dylib` names. Install it with
  `brew install ffmpeg`, then rerun `meson setup --reconfigure build` so Meson
  detects the libraries. The Homebrew FFmpeg formula currently includes GPL
  components; review FFmpeg's [license guidance](https://ffmpeg.org/legal.html)
  before bundling those libraries in a distribution.
- **`.flh` FLIC overlays** (2026-10-21, finding 21): `src/gods98/flic.*` is
  the port's runtime player for the FLIC-family overlay animations the
  shipped game shows on its menus — the 18 wad `.flh` files
  (`Interface\Pointers\*`, `Interface\InfoPanel\HOLO.FLH`,
  `Interface\Reward\CAPTAIN/CAPTY.FLH`) plus the 11 loose `Data/AVI` copies
  the front-end `!Overlay%i` entries name (`avi\PB1.flh` etc.). It
  implements finding 21's coder model exactly: 128-byte header, chunk
  stream from 0x80, one opcode-25 keyframe (FLI16 byte-RLE) + nFrames
  opcode-27 delta records (word-based row jumps + skip/control), size-16
  hold chunks repeating frames, RGB555 storage expanded to RGBA32.
  `flic_test` (suite `flic`) builds a synthetic 4×4 FLIC and pins
  keyframe/delta/hold/loop/RGBA pixels end-to-end, and `--real` decodes all
  **29 wad + 11 loose `.flh` files to EOF (1,802 frames)** with content
  spot checks (HOLO 158×226, nFrames 81; animated last-frame≠first). The
  front end now parses and plays the `!Overlay%i` decorations on each
  submenu (`PlayRandom` picks one), ticking each open flic per frame and
  blitting its current frame at the configured (x,y) over the menu
  backdrop.

`GameContext` (`src/game/game.hpp`) carries the corpus globals these
modules share (`DAT_005560c4` config, `DAT_005560d4` level block,
`DAT_00556ed0` cheat word, `DAT_004ffbc0` objective word, the save/task/
reward/entity tables) so tests run isolated. The build boots into the
**main menu** first (finding 23); once a level starts — from the menu's
picker rows or via `LEGORRR_LEVEL` (default `Level01`, a fixture/demo name not
present in shipped `Lego.cfg`) — the frame loop
seeds tasks/reward and places the `.ol`, runs the mission bridge + objective
tick per frame, and exposes `S` (build blob + commit slot), `L`
(`CheckpointStep`),
`c`/`o`/`+` (live crystal/ore/score — the entity sim's stand-in), the
selection keys (`1`–`4` pick / `m` move / `g` gather-task / `n` clear),
`y` (toggle the reward overlay) and the **mouse-button click** (reward
save/advance zones, finding 09) — and the software level renderer draws the
grid + live entities (finding 22 §3), with the engine bound to the level
(seam #35). Eleven suites use synthetic fixtures
only:

```sh
meson test -C build --suite level
meson test -C build --suite objective
meson test -C build --suite savegame
meson test -C build --suite mission
meson test -C build --suite entity
meson test -C build --suite cmd
meson test -C build --suite select
meson test -C build --suite ai
meson test -C build --suite render
meson test -C build --suite bind
meson test -C build --suite frontend
meson test -C build --suite movie
meson test -C build --suite flic
```
Passing `-Dtest_wads_dir=<game>` adds real-wad probing to `level_test` and
`frontend_test`, mirroring the resource/texture/sound/block suites: they
mount the shipped wads — `level_test` counts Lego.cfg groups resolving a
`SurfaceMap`, `frontend_test` builds the whole front end (menu count, level
chains, grafted picker rows). Best-effort — the fixtures are the guaranteed
coverage, and the probes are skipped when no game data is present.

```sh
cd path/to/legally-owned/game
LEGORRR_AUTOSTOP_FRAMES=600 ../lrr-portable/build/src/LegoRR
```

Toolchain requirements: `native` needs SDL3 devel headers and cmake
(miniwin), `winapi` needs `i686-w64-mingw32-gcc`/`windres`/`ar` (this box is
immutable — install inside a distrobox/podman Fedora container). Rebuilt
Win32 binaries run through `a2.py` only via the (not yet wired)
`run --run-port` interface — ADR 0002, ADR 0004.

As discoveries are made, they should be documented here in the repo (e.g. `docs/` per
format) with byte-level field tables where possible.

## Working in this repo

1. Place a legally owned copy of the game in `reference/` (see layout above).
2. Do not commit anything under `reference/`.
3. Analysis output — notes, decompiler dumps, parsers, experiments — lives in the tracked
   part of the repo.

## Legal note

Lego Rock Raiders is © 1999 LEGO Media International. All original game assets are the
property of their respective owners and remain copyright LEGO. This repository exists
solely for interoperability research, documentation, and preservation purposes and
contains no original game content. Do not upload copies of the game anywhere.

The reconstruction source, build system, tooling and documentation in this
repository are licensed **LGPL-3.0-or-later** (see `LICENSE`); the
miniwin adapter is similarly LGPL-3.0. That license covers *our code*, not the
game's assets or binary.
