# Early research milestones

These notes preserve the October 2026 research snapshot formerly in the
main README. Statements such as “complete” and “pinned” describe that
snapshot, not the current reconstruction status. See the
[project overview](../README.md) for current progress.

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

## Reproducibility

- **Unpacking**: `python3 tools/unwad.py reference/LegoRR1.wad unpacked/` extracts both archives.
- **Function naming**: `tools/ghidra/CreateEngineFunctions.java` is a Ghidra headless
  post-script that creates/renames the analyzed functions and emits the decompile corpus;
  batch runs use `support/analyzeHeadless` with `-postScript`.
- **Interactive analysis**: REA (Ghidra provider). Findings cite addresses, not guesses:
  every claim traces to a pseudocode block or an evidence record.

