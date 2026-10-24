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
