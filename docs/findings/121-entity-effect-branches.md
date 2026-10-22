# 121 — The entity effect branches: projectile arcs, screen shake, screen-space effect lane

Closes the three documented-but-unguessed TODO branches that finding 10
listed (its "Documented TODO branches (not guessed)" paragraph): projectile
arcs (`+0x3e0` bit `0x400000`, `FUN_004755e0`/`FUN_00475120`), screen shake
(`+0x3e4` bit `0x40000000`, `DAT_004a48b0`), and the screen-space effect
lane (the corpus effect bank `DAT_0050391c` and the callback branches that
dispatch and draw through it). All three become real, demoable behaviour in
the port (`src/game/effect.*`, plus the callback branches in `ai.cpp`).

**Observations from** REA/Ghidra 12.1.4 over the shipped `source/LegoRR.exe`
(SHA-256
`c3d8ce23f1aa21814950c77e713ec176599506d4e71758cf1cf709525e581e0f`),
2026-10-22. Evidence bundle:
[`legorr-entity-effect-branches.bundle`](../evidence/legorr-entity-effect-branches.bundle)
(74 records).

## 1. The screen-space effect lane (`DAT_0050391c`)

The corpus keeps a fixed effect bank at `DAT_0050391c` — 10 records, each
`0x2b0` bytes — holding the in-flight projectile effects of the entity
branches:

- **Allocator** `FUN_0046ff70`: scans the `+0x2a4` owner words
  (`DAT_00503bc0` onward, stride `0xac` dwords) for a zero slot, returns the
  first free index or -1.
- **Firers** allocate one record and fill it: `FUN_0046faf0` (the corpus
  lazer/pusher record — type 0, straight travel) and `FUN_0046fc70` the
  freezer variant (type 1) build the ballistic arc; both write start/current
  position, a unit direction, and the speed constant `0x40a00000` (5.0) or
  `0x40800000` (4.0).
- **Dispatch**: the entity-tick callback's tool firing path
  (`FUN_004420d0`) maps tool kinds to these firers — case 1 lazer, 2 pusher,
  3 freezer, then the arced variant at case 4.
- **Per-frame advance** (`FUN_0046f550` effect-bank ticker → per-record
  `FUN_0046f920`): straight records move along the unit direction at
  `speed*dt`; arc records accumulate `progress = speed*dt`, sample the
  prebuilt polyline at that arc length (`FUN_004068c0`), and place the
  record at `start + dir · (sample x)` plus the sampled height.
- The callback's arc branch (`0x43ca82`) is the per-entity *consumer*: it
  resolves the work object (`FUN_0044a200`) and the `+0x2e4` target handle,
  steps/draws through `FUN_004755e0` (a vtable +0x3c method) and
  `FUN_00475120` (advance the scene and draw), and when the scene's leftover
  time passes the last frame (`fcomps 0.0` + `test $0x41` idiom) it
  completes: `+0x3e8 | 0x800000`, `+0x3e0 &= ~0x400000`, `+0x340 = -1.0`,
  and the target is woken.

### The arc geometry (the "corpus-shaped ballistic curve")

The arc records' shape is a **cubic-bezier polyline** built by
`FUN_00406820` (50 points, count `0x32`) with `FUN_004065f0` as the point
evaluation:

- Control vectors are pre-normalized by the firer (`FUN_00406760`): the
  corpus constants are `{1.0, -1.3}` scaled to `dist*0.2` and
  `{0.3, 1.0}` scaled to `dist*0.33333334`.
- The polyline runs from `(0,0)` to `(dist, dz)` in (horizontal-distance,
  height) space.
- `FUN_004067b0` builds the cumulative segment-length table; the sampler
  `FUN_004068c0` walks that table to the requested distance and lerps
  between the bracketing points.

The port transcribes these four functions in `effect.cpp` as
`effect_arc_build` / `effect_arc_sample`, and the test suite pins them
against the corpus table (a fired 12.0-distance, 3.0-height arc totals
`18.613` and samples `(7.334, -4.756)` at half length, `(3.115, -3.446)`
at quarter length).

## 2. Screen shake (`+0x3e4` bit `0x40000000`, `DAT_004a48b0`)

- **Raiser**: the impact/wake probe `FUN_004475c0` (finding 27 reader B)
  runs when the scene-done bit fired; when the impact speed is negative it
  calls `FUN_0043f6a0(iVar1, -speed, 1, 1.0)` and then
  `+0x3e4 |= 0x40000000` — a landed boulder raises the shake bit on its
  linked entity.
- **Consumer** (callback `0x43ce88`): for a shaking entity with a live work
  object and `dt > 0`, the branch draws the work object at a random offset
  scaled by the **global amplitude** `DAT_004a48b0` (shipped value `1.0`),
  then decays that amplitude by the `0x49e3fc` constant:
  `DAT_004a48b0 -= dt · 0.16`. Once the amplitude passes below zero the
  branch resets `DAT_004a48b0 = 1.0` and clears the entity's `+0x3e4` shake
  bit.
- The port mirrors `DAT_004a48b0` as `GameContext.shake_amp` (init 1.0) and
  the current viewport offset as `shake_dx`/`shake_dy`; `ai_shake_raise`
  mirrors the `FUN_004475c0` raiser. The callback branch decays the
  amplitude exactly like the corpus; the software renderer converts the
  world-unit offset to pixels and shifts the frame (`level_render_map`), so
  triggering visibly shakes the viewport with the corpus decay.

## 3. Port shape

`src/game/effect.*`:

- `EffectSlot` lane records (`game.hpp`, mirroring the corpus `0x2b0`
  layout's essentials) and a bound `EFFECT_LANE_CAP`.
- `effect_lane_dispatch(ctx, owner, target, kind)` — allocator + firer
  (counts dispatches).
- `effect_lane_tick(ctx, dt)` — the effect-bank advance; arrivals run the
  corpus completion (wake target, clear arc bit, `+0x340 = -1.0`).
- `effect_lane_draw(...)` — the screen-space draw path (lane markers),
  called after the map render without touching the render path's state.
- `effect_arc_build` / `effect_arc_sample` — the transcribed geometry.

`ai.cpp` gains the two callback branches in corpus order — the arc consumer
(before the deferred-grant) and the shake branch (decays amplitude, resets
at zero). The demo (`frameloop.cpp`) wires `'f'` to fire an arc from the
selection toward the cursor and `'h'` to raise shake.

## Confirmed / unknown ledger

| Claim | State |
| --- | --- |
| Effect bank `DAT_0050391c`, `0x2b0`-stride records, allocator `FUN_0046ff70` (first free `+0x2a4`) | observed (decompiles) |
| Firers `FUN_0046faf0` (straight) / `FUN_0046fc70` (arc) write start/dir/speed constants 5.0 / 4.0 | observed |
| Arc = 50-point cubic-bezier polyline, control vectors `{1,-1.3}`·0.2·dist and `{0.3,1}`·dist/3, cumulative-length sampling | observed (decompiles + transcribed table) |
| Callback arc branch `0x43ca82`: consume `+0x3e0` bit `0x400000`, step scene, complete on leftover → wake `0x800000`, `+0x340=-1.0` | observed (assembly) |
| Shake raiser `FUN_004475c0` raises `+0x3e4` bit `0x40000000`; consumer `0x43ce88` decays `DAT_004a48b0` by `0.16/s`, resets to 1.0 and clears at end | observed (decompiles + constants) |
| The port's `draw_marker` mapping (world → pixel) is the port's own; corpus draws through vtable thunks (`FUN_00475470` etc.) | port-side choice, documented |
| Exact left-over-time threshold semantics of `FUN_00475120`'s return in the arc completion | machine-level x87 observed; not fully decoded (finding 41's type-mismatch note still applies) |