// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — frame tick + core state scaffold.
 *
 * Reconstruction scaffold for the engine-core functions named in the decompile
 * corpus (docs/decompiles/engine-region.c; findings 02/03/20):
 *
 *     EngineTick     @ 0x4232e0   per-frame tick          (triplet slot DAT_00505930)
 *     EnginePreTick  @ 0x4231f0   per-frame housekeeping
 *     KeepRunning    @ 0x435940   quit-poll probe
 *     MainSimStep    @ 0x4250c0   input/app-state step; 0 => quit frame
 *     shutdown       @ 0x424cf0 / 0x424d00  normal-play stub / loader-mode teardown
 *
 * The state they operate on is the global block the corpus knows as
 * DAT_005560d4+ (finding 03). Discipline (ADR 0003/0004, GLOSSARY): plain
 * C-shaped code, no OS-facing headers below the seam (src/platform/), no heavy
 * STL. Field offsets in the struct comment refer to the decompiler offsets.
 */
#ifndef LEGORRR_ENGINE_HPP
#define LEGORRR_ENGINE_HPP

#include <cstdint>

#include "input.hpp"

/* --- App mode (corpus DAT_00505f88) ------------------------------------ */
#define ENGINE_APP_NORMAL 0
#define ENGINE_APP_LOADER 10 /* asset-load benchmark: tick idles, full teardown */

/* --- Engine state flags (corpus DAT_00556ecc), named from finding 03 ----- */
#define ENGINE_STATE_IN_GAME_RENDER 0x0002u    /* companion of 0x2000 */
#define ENGINE_STATE_FORCE_SCALE_1  0x0004u    /* one-shot "scale=1.0" this frame */
#define ENGINE_STATE_INDEXED_SUBSYS 0x0800u    /* ManagerTick dispatch enabled */
#define ENGINE_STATE_TIME_FROZEN    0x100000u  /* dt forced to 0.0 */
#define ENGINE_STATE_FADE_TRANSITION 0x2000000u /* fade/transition state machine */

/* --- Feature word (corpus DAT_00505f84), findings 02/20 ------------------ */
#define ENGINE_FEATURE_ALT_MODE   0x2u
#define ENGINE_FEATURE_SKIP_TIME  0x10u   /* dt := 0.0 */
#define ENGINE_FEATURE_FIXED_STEP 0x80u   /* dt := 0.8333333 */

/* --- Debug/cheat word (corpus DAT_00556ed0) — bits the tick reads --------- */
#define ENGINE_DEBUG_TEXT_LAYER 0x10u /* HUD printf text gated by this */

/* Per-level/game state block. The corpus reads these fields at the offsets
 * shown on the right (DAT_005560d4 + off). This struct lays out for
 * readability, not byte-identically — this is a reimplementation, not a
 * clone; remaining fields land here as they are mapped out of the corpus. */
typedef struct EngineState {
    uint32_t tick_a;     /* +0x8c  timer pair */
    uint32_t tick_b;     /* +0x98  timer pair */
    uint32_t level_time; /* +0x9c  level time (vs quota_278) */
    uint32_t field_a0;   /* +0xa0  (used with field_ac*5 in ticker 0x45a460) */
    uint32_t field_ac;   /* +0xac  */
    uint32_t air_beat;   /* +0x1fc air-supply beat counter */
    uint32_t air_prev;   /* +0x204 previous air value */
    uint32_t quota_278;  /* +0x278 compared against level_time (finding 03) */
} EngineState;

/* Mutable engine context — stands in for the corpus's global window
 * (DAT_00505f88 / 00505f84 / 00556ecc / 00556ed0 / 00556ec8 / 005560d4…). */
typedef struct EngineContext {
    int32_t      app_mode;     /* DAT_00505f88 — 0 normal, 10 loader */
    uint32_t     feature;      /* DAT_00505f84 — engine feature word */
    uint32_t     state;        /* DAT_00556ecc — engine state flag word */
    uint32_t     debug;        /* DAT_00556ed0 — debug/cheat flag word */
    int32_t      context_mode; /* DAT_00556ec8 — 1 in-game, 0 editor/preview */
    EngineState *state_block;  /* DAT_005560d4 */
    InputState  *input;        /* key tables / text pump / quit-dwell
                                * (findings 04/05; dwell slots per 22) */
    float        exit_timer;   /* DAT_00556efc — decremented by scaled dt */
    int          tickers_ran;  /* scaffolding: count of dispatched tickers */
} EngineContext;

/* Frame tick — the triplet slot DAT_00505930 (corpus EngineTick @ 0x4232e0).
 * Returns 0 when the frame loop must quit, 1 otherwise. dt is seconds. */
int EngineTick(EngineContext* ctx, float dt);

/* Per-frame housekeeping (corpus EnginePreTick @ 0x4231f0). */
void EnginePreTick(EngineContext* ctx);

/* Quit-poll probe (corpus KeepRunning @ 0x435940). 0 => quit requested. */
int EngineKeepRunning(EngineContext* ctx);

/* Time-scale factor (corpus FUN_0041fb40 call site). */
float EngineTimeScale(EngineContext* ctx);

/* Main simulation step / quit gate (corpus MainSimStep @ 0x4250c0).
 * Consumes input+app state for the frame; the three outs are the step's
 * results passed to the later subsystem tickers. Returns 0 => quit frame. */
int MainSimStep(EngineContext* ctx, float scale, float dt,
                uint32_t* out1, uint32_t* out2, uint32_t* out3);

/* Engine teardown (triplet slot DAT_00505934): corpus LAB_00424cf0 is the
 * normal-play stub forwarding to FUN_004250a0; LAB_00424d00 is the full
 * teardown used in loader mode (finding 20). */
void EngineShutdown(EngineContext* ctx);

#endif /* LEGORRR_ENGINE_HPP */