# 03 — Engine Tick (`EngineTick` @ `0x4232e0`)

**Method note:** this region had no function record (reached only via
`call [DAT_00505930]`), so auto-analysis skipped it and REA could not
decompile. We created the function offline with a Ghidra post-script
(`tools/ghidra/CreateEngineFunctions.java`, run via `analyzeHeadless`)
and decompiled there. Full pseudocode: `docs/decompiles/legacy/engine-region.c`.

## Signatures

| Function | Addr | Signature | Role |
| --- | --- | --- | --- |
| `EngineTick` | `0x4232e0` | `int f32->u32 tick(float dt)` | per-frame game logic; returns 0 → frame loop quits |
| `EngineShutdown` | `0x424d00` | `void()` | full teardown |
| `EngineShutdownAlt` | `0x424cf0` | `void()` | one-line stub → `FUN_004250a0` |

`dt` (`param_1`) is the seconds-delta produced by the frame loop (doc 02).
A second scaled delta, `fVar10 = dt * FUN_0041fb40()` (`DAT_...` = time-scale
factor, i.e. game-speed), is passed to most subsystem tickers — entity logic
runs scaled time, while raw `dt` drives timers.

## Global state (the game's core data block, `DAT_005560d4`+)

The tick is dominated by globals in the `0x5560d4`–`0x556f24` window:

| Global | Interpretation | Evidence |
| --- | --- | --- |
| `DAT_005560d4` | **level/game state block** (level time `+0x9c`, timers `+0x8c/+0x98`, air beats `+0x1fc/+0x204`) | every subsystem is passed it; air/warning logic reads `+0x1fc` |
| `DAT_00556ecc` | **engine state flag word** (bitfield) | set/cleared at transitions throughout tick |
| `DAT_00556ed0` | **debug/cheat flag word** | gates all debug overlay strings |
| `DAT_00556ec8` | **context mode**: 1 = in-game, 0 = editor/preview | the tick splits into two render+update branches on it |
| `DAT_00505f88` | app mode (10 = loader/benchmark) | see LoaderProfile below |
| `DAT_005560d8 / 0dc` | render target pair (`FUN_00477150(target, …, dt)` draws) | paired begin/end calls around draw sections |
| `DAT_005560e4 / e8 / ec` | three view/camera contexts | each ticked identically by `FUN_00435f5c(o, state, dt, scale)`; `0ec+0x24` = its render-list ref |
| `DAT_00556124 / 128 / 12c` | debug text-panel surfaces | target of `FUN_0047a180(surface, x, y, fmt, …)` = `printf`-style HUD text |
| `DAT_005561d4…28c` | entity/object arrays (strides `0x1ec`, `0x74`, `0x14c`, `0x10`) + counts | iterated in EngineShutdown |

Flag bits (observed behavior, partially inferred):

- `DAT_00556ecc & 0x100000` → time frozen: dt forced to 0.0 at entry.
- `DAT_00556ecc & 2` → "in-game render pass" companion of `& 0x2000` (enables
  `FUN_004291a0(scale,1)` + `FUN_00449c00()` + overlay draw + `FUN_00450780`).
- `DAT_00556ecc & 0x2000000` → fade/transition state machine (`FUN_00415370(dt, dir)`);
  entering clears `&0x800`-gated debug; on completion sets `*(state+0x27c)=3`.
- `DAT_00556ecc & 4` → one-shot "force scale 1.0" this frame.
- `DAT_00556ecc & 0x40000` → `FUN_0045cc70(u64)` screen-position commit (vertex
  vs block mode strings `s_Vertex`/`s_Block` + `%s mode: %s`).
- `DAT_00556ed0 & 0x10` → debug text layer (all `FUN_0047a180` strings below).

## Tick structure (first-pass reading)

1. **Gate:** `DAT_00505f88 == 10 && FUN_00435940() == 0` → return immediately
   (loader/benchmark mode idle). Bit `0x100000` zeroes dt.
2. `FUN_004231f0()` (per-frame housekeeping), scale computed.
3. **Camera / move** cluster: `FUN_0046a730`, `FUN_00435a50`, `FUN_00431560` →
   `FUN_0042c440`, `FUN_0042cb70`, `FUN_004261c0`; camera interpolation block
   (screen-space → world projection when `DAT_00556158` set: solves for
   `DAT_00556148/14c`, distance-test vs `_DAT_0049e2cc`).
4. **Main simulation:** `FUN_004250c0(scale, dt, &3 outs)` → **0 = quit frame**;
   then ~25 subsystem tickers in fixed order, several passed `(DAT_005560d4, scale)`:
   `FUN_00425d90`, `FUN_00402220`, `TerrainCellFlagCleanup` (`0x424600`), `FUN_0040aa10`, `FUN_0043b270`,
   `FUN_0046f550`, `FUN_0040eab0`, `FUN_004313b0`, `FUN_004520d0`,
   `FUN_00435f5c`×3 (the three view contexts), `FUN_004091e0`, `FUN_0041ba10`,
   `FUN_004291a0(dt,0)`, `FUN_00449c00`, `FUN_0040d450`, `FUN_00426520`,
   `TerrainDirtyCellUpdate` (`0x44eff0`), `FUN_0046e390`, `FUN_0047aba0`, `FUN_00465b00`, `FUN_004665c0`,
   `FUN_0044cfa0`, `FUN_0041a2c0`. `TerrainCellFlagCleanup` walks the selected terrain cells, clears per-cell processing bits, and queues command `0x14` for qualifying cells. `TerrainDirtyCellUpdate` processes cells marked dirty (`+0x19` flag `0x04`), clears the flag, then advances the terrain update helpers.
5. **Indexed subsystem table:** under bit `0x800`, `FUN_0045a730(idx, dt)` is
   called with idx `0,1,2,(3 skipped),4,6,7,8,9,10,11` interleaved with named
   tickers (`PositionTrackerUpdateAll` at `0x45d6d0`, `FUN_0040e7f0`, `FUN_0041b490`, `TimedStatusAndEventTick` at `0x41b930`,
   `FUN_0041a080`, `FUN_00419f30`). `PositionTrackerUpdateAll` applies each tracked offset to its entity's resolved scene handle; `TimedStatusAndEventTick` toggles a periodic status flag, advances a timer, and raises event bit `0x800` when its countdown expires. Looks like a numbered manager array
   (AI, particles, FX…) — worth naming once each index is identified.
6. **Render:** `FUN_0046b7c0(camL, camH, dt)` and begin/end pairs
   (`FUN_00473b40(surface, 0/1)` = lock/unlock; `FUN_00475580(surface, 2, …,
   1.0, …, speed)` = sprite/anim draw on `DAT_0055610c/fc` refs).
   In-game branch calls `FUN_00435300` / `FUN_00454600` (world draw); editor
   branch does the `0f` query scan `FUN_00437460(2, i)` loop.
7. **Air / "SND_AirBeat" mechanic** (`state+0x1fc/+0x204` vs global time
   `_DAT_0049e2b8`): fires `FUN_00419b80(msg_id, …)` with ids `0x10`–`0x13`
   (air-low / air-empty notifications) and drives a looped 3D sound
   `SND_AirBeat` whose pitch/position track via `FUN_0047c160`+`__ftol`.
   `FUN_00419b80` = central **message/notification dispatcher**.
8. **Debug overlays** (gated `DAT_00556ed0 & 0x10`): `Game Speed: %0.0f`,
   `3D sound fall off : %f`, `Fallin Mode : %s`, `Upgrade: %i %i %i %i`
   (bitfield of upgrade flags read from entity `+8 >> 0..3`), `%s mode: %s`
   (Vertex/Block), `NoNERPS` (transition debug).
9. Return 1.

The two big state globals give the frame its two shapes:

- `DAT_00556ec8 == 1` — **in-game**: draws via world renderers, no editor pass.
- `DAT_00556ec8 == 0` — **editor/preview**: `FUN_0043b850` scene binding of
  `DAT_00556160` + `FUN_00435f0b`, `FUN_00424730(0/1, scale)` editor camera
  pass, `FUN_004634b0`/`FUN_00463230/0463540` scene-graph bind/unbind around it.

## EngineShutdown (`0x424d00`)

Teardown order: subsystem closes (`FUN_0044e0a0`, `FUN_0045f750`, …), per-context
`FUN_00435dc8` closes for the three views, then iterates the entity arrays:

| Array | Stride | Count | Element destruct |
| --- | --- | --- | --- |
| `DAT_005561d4` | `0x1ec` | `DAT_0055627c` | `FUN_0046ce10` |
| `DAT_005561d8` | `0x74` | `DAT_00556280` | `FUN_00406ec0` |
| `DAT_005561dc` | `0x74` | `DAT_00556284` | `FUN_00406ec0` |
| `DAT_005561e4` | `0x10` | `DAT_0055628c` | `FUN_0046c0f0` |
| `DAT_005561e0` | `0x14c` | `DAT_00556288` | `FUN_00408940` |

Every element is followed by `FUN_0048db80(ptr)` (`free`) via its pointer
vector. In loader mode (`DAT_00505f88 == 10`) it loads
`LoaderProfile.txt` / `LoaderProfileNoSound.txt` — benchmark profile data,
confirming mode 10 = asset-loading benchmark. (`FUN_0044dd40(NULL)` in normal mode.)

## Confidence

- Observed: call graph, flag-bit effects on dt/pause, global roles marked,
  shutdown ownership. All directly from decompiled pseudocode.
- Inferred: names (`EngineTick` etc.), flag semantics where only one branch
  was exercised, "three view contexts" (they tick identically).

## Next targets

1. ~~`FUN_0045a730(idx, dt)`~~ resolved: 12-slot positional trackers, not
   subsystem managers — see [04 — Sim, Input & Events](04-sim-input-and-events.md).
2. ~~`FUN_004250c0`~~ resolved: input/app-state step (quit gate) — doc 04.
3. ~~`FUN_00435f5c`~~ resolved: camera state machines — doc 04.
4. ~~`FUN_00419b80`~~ resolved: config-driven message bus — doc 04.
5. Entity element layouts (`0x1ec`, `0x14c`…) — first field reads in the
   tick's entity loops will reveal position/health/type offsets.
