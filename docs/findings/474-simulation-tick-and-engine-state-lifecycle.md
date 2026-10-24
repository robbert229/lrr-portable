# Finding 474 — Simulation Tick and Engine State Lifecycle

This finding maps the 15 procedures assigned to simulation-lifecycle. The behaviors below are direct observations from the canonical pseudocode. “Simulation,” “pause,” and “level transition” describe nearby call paths and state fields; they do not assign meaning to otherwise opaque global bits.

## Admission, frame update, and transitions

`MainSimStateAdmissionPredicate @ 0x004191a0` returns true only when `FUN_004191f0()` is nonzero, bits 1 and 2 are both set in `DAT_004dc650`, and `FUN_00456240(0)` returns zero. `FUN_004191f0` tests bit 0x40 of the same global, so the predicate's effective conditions include bits 0x40, 1, and 2. `SimulationGlobalWordSet @ 0x0041ba90` simply assigns its argument to `DAT_004de1f4`; `SimulationPointerActionDispatch @ 0x0041baa0` calls it with a table-selected word when dispatching a pointer action and the global gating conditions pass.

`MainSimStep @ 0x004250c0` is the large per-frame input/update coordinator. It handles global transition and focus conditions first, may call `ResetLevelAndExit`, and can return early when simulation is gated. It initializes three output flags, applies key-driven camera and entity movement, processes user-interface state changes and debug toggles, updates interaction/selection state, and routes pointer/cell actions to command handlers. Its many branches depend on global modes and UI controls; their full user-facing mapping remains a follow-up. `ResetLevelAndExit @ 0x004250a0` calls `FUN_004297b0(0)` and `FUN_0048e4b0(0)`.

`TransitionControl @ 0x00425c30` is disabled while global state bit 2 is set. Otherwise it enters a transition by setting `DAT_00556ecc` bit 0x100000 and `DAT_00556ed0` bit 0x1000 (calling a helper on the first transition), or exits by dispatching `PeriodicCallbackDispatch`, resetting option 7, and clearing those bits. `CurrentEngineModeIsZero @ 0x00425d80` returns whether `DAT_00556ec8` equals zero. `StartLevelPilots @ 0x0041fa80` always calls `FUN_00431560`, then once per latch bit initializes pilot/resource state, enables several feature values, calls `TransitionControl(0, 0)` and a UI update, and optionally initializes two panels. `TimeScaleGet @ 0x0041fb40` returns `_DAT_0055615c` as a floating-point value.

`EnginePreTick @ 0x004231f0` consumes pending input-state deltas into a shared buffer. On the observed input-release conditions it calls `TransitionControl(0, 0)`, appends changed input codes while capacity remains, then copies the current input-state block into a previous-state block and clears the pending block. `DebugPanelTextDrawUpdate @ 0x004230c0` reads the buffer for display, but the corpus does not establish whether it consumes the same update records or runs after `EnginePreTick`; call ordering remains unresolved.

## Simulation interaction and engine state

`MainSimInteractionAndCellUpdate @ 0x00426520` updates interaction state from pointer/cursor input, calls `FUN_00418da0` with current pointer and mode values, resolves a terrain cell, and routes through UI/pointer/cell handlers based on engine and UI flags. Its branches may update selection, issue terrain/entity commands, update camera state, play feedback, or set a pending interaction bit. If the global 0x100000 transition bit is set, it clears interaction state and exits early. The routine is a direct dispatcher; the meaning of each command/UI bit is not inferred here.

`GlobalSubsystemStateReset @ 0x004263c0` zeros two global fields, passes a one-valued local to `FUN_004565f0`, calls `TextBufferReset @ 0x004859b0`, and clears global state through `FUN_00426360(0,0)` and `FUN_004263a0(0)`. `SpawnCoordinatePoolTick @ 0x004261c0` decrements a global timer when the level field at +0x26c is not 0x14; on expiry it calls `FUN_00438960(0,0)` and, if that succeeds, `PredugSpawnCoordinateSelectAndCreate(0, levelField)`, then reloads the timer from level field +0x268.

`GlobalStateZeroTransitionDetect @ 0x00435550` detects a transition of `DAT_0076ab3c` from nonzero to zero, stores the current value, and returns 1 only on that edge. `KeepRunning @ 0x00435940` performs a set of refreshes and input/resource checks, polls the current level, updates a mode-dependent subsystem when active, and returns false when required resource/level predicates fail; if its result passes `FUN_00429890`, it passes that result to `FUN_0048db80` and returns true; the ownership/meaning of the result remains unresolved. `MainSimFeatureFlagLatch @ 0x00435a50` checks global bit 0x8000000 and feature 0x40b, setting `DAT_00556ed0` bit 2 when both are present.

## Caller and data flow

`MainSimStep` directly calls `MainSimStateAdmissionPredicate`, `KeepRunning`, `ResetLevelAndExit`, and `TransitionControl`; this ties admission, continued execution, cleanup, and transition flags into the same frame coordinator. `StartLevelPilots` also enters through `TransitionControl`, while `MainSimInteractionAndCellUpdate` has its own transition-bit early exit. `SpawnCoordinatePoolTick` directly hands expiry to the predug spawn creator. These calls and data flows are visible in the pseudocode; labels such as “frame coordinator” and “transition” summarize those direct relationships.

## Unresolved questions and follow-up slices

- Map the three output flags and key input transitions consumed by `MainSimStep`, including the call site that runs `EnginePreTick`.
- Resolve the semantics and ownership of globals `DAT_004dc650`, `DAT_004de1f4`, `DAT_00556ecc`, and `DAT_00556ed0`.
- Trace the reset helpers behind `ResetLevelAndExit` and `GlobalSubsystemStateReset` to establish which state survives a level transition.
- Trace the level-handle and final validation contract used by `KeepRunning`.
- Determine when and how `SpawnCoordinatePoolTick` is scheduled and whether its timer is reset elsewhere.

**Evidence.** Canonical pseudocode in [LEGORR.full.c](../decompiles/full/LEGORR.full.c), procedures 0x004191a0–0x00435a50. The assigned procedures are listed in [function-areas.csv](../coverage/function-areas.csv) under `simulation-lifecycle`.
