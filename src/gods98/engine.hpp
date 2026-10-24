// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — frame tick + core state scaffold.
 *
 * Reconstruction scaffold for the engine-core functions named in the decompile
 * corpus (docs/decompiles/legacy/engine-region.c; findings 02/03/20):
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

/* Forward: the view/camera objects (view.hpp behind a cycle-breaking decl). */
typedef struct ViewContext ViewContext;

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

    /* Move-list / grid fields used by the camera cluster (camera.hpp). */
    void*    move_list;      /* +0x08 ptr, stride 0x4c (cell +0xc float) */
    uint32_t move_list_count;/* +0x0c */
    uint32_t grid_w;         /* +0x14 grid width */
    uint32_t grid_h;         /* +0x18 grid height */
    void*    grid_cells;     /* +0xb8 grid buffer, cell stride 0x48 */
    float    spawn_interval; /* +0x268 chip-spawn re-arm interval */
    uint32_t spawn_type;     /* +0x26c chip/spawn type (0x14 skips) */

    /* per-subsystem fields used by the ticker ports (simtick.hpp):
     * marker-sweep grid (+0x8/+0xc/+0x28/+0x73d0 in the level block view). */
    uint32_t sim_w;           /* +0x08 grid width  */
    uint32_t sim_h;           /* +0x0c grid height */
    void*    sim_grid;        /* +0x28 cell buffer, stride 0x28 */
    uint8_t  sim_marker_flags;/* +0x73d0 flag byte bit 2 enables the sweep */

    /* minion activity list (tranche 9, FUN_004313b0 at +0x88). */
    void* minion_list;
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
    ViewContext *views[3];     /* DAT_005560e4 / e8 / ec (view.hpp) */
    void*       level_block;  /* LevelState block when bound (game level) */
    /* Resolved native pointers the raw +0xb8/+0x04 words cannot carry on a
     * 64-bit host (the game layer keeps the full pointers in its registry —
     * engine_level_ptr would read a truncated word). The frame loop fills
     * these at bind time via level_load_ptr. */
    const uint8_t* level_voxels;   /* the w*h*0x48 voxel array */
    const void*    level_terrain;  /* TerrainState (reserved for heights) */

    float        exit_timer;   /* DAT_00556efc — decremented by scaled dt */
    float        time_scale;   /* game-speed dt factor (issue #85). 0 = 1.0:
                                * options_time_scale writes the live value
                                * every frame; zero-initialized scaffolds and
                                * tests keep the pre-#85 1.0 behaviour. */
    int          tickers_ran;  /* scaffolding: count of dispatched tickers */
} EngineContext;

/* --- engine-level binding ------------------------------------------------
 * The corpus uses ONE 0x284-byte block (DAT_005560d4) for both engine state
 * and the loaded level (game/level.h). These accessors read the bound level
 * block at the corpus offsets (width +0x14, height +0x18, voxels +0xb8) so
 * the engine's camera/view/ticker code sees the real terrain. */
static inline uint32_t engine_level_u32(const EngineContext* ctx, int off)
{
    const uint8_t* b = (const uint8_t*)ctx->level_block;
    uint32_t v = 0;
    __builtin_memcpy(&v, b + off, 4);
    return v;
}

static inline uintptr_t engine_level_ptr(const EngineContext* ctx, int off)
{
    uintptr_t v = 0;
    __builtin_memcpy(&v, (const uint8_t*)ctx->level_block + off,
                     sizeof(uintptr_t));
    return v;
}

static inline int engine_level_width(const EngineContext* ctx)
{
    return (ctx != NULL && ctx->level_block != NULL)
               ? (int)engine_level_u32(ctx, 0x14) : 0;
}

static inline int engine_level_height(const EngineContext* ctx)
{
    return (ctx != NULL && ctx->level_block != NULL)
               ? (int)engine_level_u32(ctx, 0x18) : 0;
}

/* Blocksize (level +0x1c float) — the world scale the camera/renderer uses
 * to convert cells to world units. 1.0 when unbound/absent. */
static inline float engine_level_blocksize(const EngineContext* ctx)
{
    if (ctx == NULL || ctx->level_block == NULL) {
        return 1.0f;
    }
    uint32_t raw = engine_level_u32(ctx, 0x1c);
    float bs;
    __builtin_memcpy(&bs, &raw, 4);
    return bs > 0.0f ? bs : 1.0f;
}

static inline const uint8_t* engine_level_voxel(const EngineContext* ctx,
                                                int x, int y)
{
    if (ctx == NULL || ctx->level_block == NULL) {
        return NULL;
    }
    int w = engine_level_width(ctx);
    if (w <= 0 || x < 0 || y < 0) {
        return NULL;
    }
    int h = engine_level_height(ctx);
    if (x >= w || y >= h || h <= 0) {
        return NULL;
    }
    /* Prefer the resolved pointer (set at bind); fall back to the raw word
     * (valid on 32-bit builds where the pointer fits the corpus slot). */
    const uint8_t* voxels = ctx->level_voxels;
    if (voxels == NULL) {
        voxels = (const uint8_t*)engine_level_ptr(ctx, 0xb8);
    }
    if (voxels == NULL) {
        return NULL;
    }
    return voxels + ((y * w) + x) * 0x48;
}

/* byte `off` within a voxel cell (surface byte = +0x04, finding 07). */
static inline uint8_t engine_voxel_byte(const EngineContext* ctx,
                                        int x, int y, int off)
{
    const uint8_t* v = engine_level_voxel(ctx, x, y);
    return v != NULL ? v[off] : 0;
}

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
 * results passed to the later subsystem tickers. Returns 0 => quit frame.
 * (Ported into mainsim.* — see main_sim_step.) */

/* Engine teardown (triplet slot DAT_00505934): corpus LAB_00424cf0 is the
 * normal-play stub forwarding to FUN_004250a0; LAB_00424d00 is the full
 * teardown used in loader mode (finding 20). */
void EngineShutdown(EngineContext* ctx);

/* RuntimeFlag40000000Get (FUN_0041f970, finding 03): the DAT_00556ecc
 * engine-state word's bit 30 — the corpus's no-argument global read,
 * threaded through the port's EngineContext (NULL answers zero). */
uint32_t runtime_flag40000000_get(const EngineContext* ctx);

/* ModeDependentField24Get (FUN_00423080, finding 03): the corpus reads
 * word field 0x24 — the follow-span slot — off DAT_005560e4 in-game and
 * DAT_005560ec in editor/preview mode, and returns the mode word itself
 * for any other mode. Threaded through the port's EngineContext (NULL
 * views answer zero). */
int mode_dependent_field24_get(const EngineContext* ctx);

/* --- the 0x425-0x426 engine-state band (2026-10-24; findings 295/324/392) ---
 * CurrentEngineModeIsZero (FUN_00425d80): the context-mode word reads zero.
 * GlobalStateFlagsBit7Set (FUN_004263a0): cheat-word bit 7 on/off. Both
 * thread through EngineContext (the corpus's DAT_00556ec8 / DAT_00556ed0).
 * GlobalScrollGuardSet (FUN_00425cd0) stores the scroll-scale guard global
 * (corpus DAT_004de548) — zero-init module storage, the corpus's writer
 * side being this setter itself. GlobalTripleCopyOut (FUN_00426230) hands
 * out the inert triple (corpus DAT_0055651c/520/524) — zero-init statics,
 * documented: the corpus's only writer is the unported sim-interaction
 * renderer update. */
bool   current_engine_mode_is_zero(const EngineContext* ctx);
void   global_state_flags_bit7_set(EngineContext* ctx, int value);
void   global_scroll_guard_set(int value);
int    global_scroll_guard_peek(void); /* port-side hook: the corpus reader
                                        * ScrollScaleSetAndClamp is unported */
void   global_triple_copy_out(uint32_t* out);

/* GlobalField505f88Get (FUN_00401bf0, finding 02): the corpus's app-mode
 * word read — the EngineContext's own field. NULL answers 0 (deviation:
 * the corpus reads its global blindly). */
int engine_app_mode_get(const EngineContext* ctx);

#endif /* LEGORRR_ENGINE_HPP */