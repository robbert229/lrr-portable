// SPDX-License-Identifier: LGPL-3.0-or-later
#include "camera.hpp"
#include "engine.hpp"
#include "mainsim.hpp"
#include "message.hpp"
#include "tickers.hpp"
#include "view.hpp"
#include "record.hpp"
#include "crt.hpp"
#include "sound.hpp"
#include "heap.hpp"

/* Frame tick — corpus EngineTick @ 0x4232e0. The numbered structure below
 * mirrors finding 03 §"Tick structure" and the first 130 lines of the corpus.
 * Every subsystem ticker is a registered stub in tickers.cpp whose function
 * body is filled in from the corpus one at a time; the *order* here is the
 * observed call order and must stay stable.
 */
int EngineTick(EngineContext* ctx, float dt)
{
    /* 1. Gate — loader/benchmark mode idles out of the frame. */
    if (ctx->app_mode == ENGINE_APP_LOADER && EngineKeepRunning(ctx) == 0) {
        return 0;
    }
    /* Frozen time zeroes dt (corpus: DAT_00556ecc & 0x100000). */
    if ((ctx->state & ENGINE_STATE_TIME_FROZEN) != 0) {
        dt = 0.0f;
    }

    /* 2. Per-frame housekeeping (EnginePreTick @ 0x4231f0). The text pump
     *    consumes the current frame's captured keys; the frame-loop drives
     *    the table lifecycle (capture before this, input_begin_frame at the
     *    frame end) — see frameloop.cpp. */
    EnginePreTick(ctx);

    /* 3. Scaled delta: subsystem tickers run scaled time, raw dt drives
     *    timers. Editor/preview (context_mode==0) runs raw dt. */
    float scale;
    if (ctx->context_mode == 0 && ctx->app_mode < 2) {
        scale = dt;
    } else {
        scale = EngineTimeScale(ctx) * dt;
    }
    /* One-shot "force scale 1.0 this frame" (corpus: DAT_00556ecc & 4). */
    if ((ctx->state & ENGINE_STATE_FORCE_SCALE_1) != 0) {
        ctx->state &= ~ENGINE_STATE_FORCE_SCALE_1;
        scale = 1.0f;
    }

    /* 4. Camera / move cluster (corpus lines 52-88): the camera
     *    interpolation block, screen-space → world projection, and the
     *    force-scale-1.0 one-shot are ported in camera.* (camera_cluster). */
    camera_cluster(ctx, dt, scale);

    /* 4b. The three view contexts (DAT_005560e4/e8/ec — ViewContextTick). */
    view_views_tick(ctx, dt, scale);

    /* 5. Main simulation step — 0 => quit frame (MainSimStep @ 0x4250c0,
     *    ported in mainsim.*). */
    uint32_t sim_out1 = 0, sim_out2 = 0, sim_out3 = 0;
    if (main_sim_step(ctx, scale, dt, &sim_out1, &sim_out2, &sim_out3) == 0) {
        return 0;
    }

    /* 6. The ordered subsystem-ticker sequence (finding 03; corpus lines
     *    93-119). Only the first entry is gated on !TIME_FROZEN at the call
     *    site; that gate lives in the table (see TICKERS[0].skip_when). */
    EngineSubsystemTickers(ctx, dt, scale);

    /* 7. Indexed manager dispatch (corpus ManagerTick FUN_0045a730) — enabled
     *    under ENGINE_STATE_INDEXED_SUBSYS; index list observed: 1,2,4,9,6,8,
     *    0xb,5,7,10 interleaved with the named manager tickers. Stub table. */
    if ((ctx->state & ENGINE_STATE_INDEXED_SUBSYS) != 0) {
        EngineManagerDispatch(ctx, dt);
    }

    /* 8. Render begin/end pairs, world-draw vs editor pass, and the
     *    &2/&0x2000-gated in-game render pass (corpus lines 130-215). Stub. */
    EngineRenderFrame(ctx, dt, scale, &sim_out1);

    /* 9. Air-supply beat mechanic (corpus tail): fires the air-low/empty
     *    notifications and loops SND_AirBeat; writes air_prev back. */
    EngineAirBeat(ctx, scale);

    /* Exit timer counts down in scaled time (corpus: DAT_00556efc -= scale). */
    ctx->exit_timer -= scale;

    return 1;
}

/* Per-frame housekeeping — corpus EnginePreTick @ 0x4231f0. The real body is
 * text/entity-name input handling (keyboard echo through DAT_00556fa0); the
 * input core (findings 04/05) supplies the text pump. cur is NOT cleared here
 * (the frame-loop clears it at the frame boundary; the corpus's in-place
 * clear is incompatible with per-frame device capture).
 */
void EnginePreTick(EngineContext* ctx)
{
    if (ctx->input == NULL) {
        return;
    }
    char*  target = ctx->input->text_target;
    int    cap    = ctx->input->text_cap;
    int    len    = ctx->input->text_len;
    if (target != NULL) {
        input_text_pump(ctx->input, target, cap, &len, 0);
        ctx->input->text_len = len;
    } else {
        input_mirror(ctx->input);
    }
}

/* Quit-poll probe — corpus KeepRunning @ 0x435940. The real body chains the
 * splash/quit checks and the app-state test (FUN_00431560, FUN_00416d00 …);
 * a quit request ends up here. For the scaffold the frame never asks to quit.
 */
int EngineKeepRunning(EngineContext* ctx)
{
    (void)ctx;
    /* TODO(engine): wire the corpus quit-poll chain here (finding 06). */
    return 1;
}

/* Time-scale factor — corpus FUN_0041fb40, read once per frame in the tick.
 * Multiplies dt into the *scaled* delta passed to subsystem tickers.
 */
float EngineTimeScale(EngineContext* ctx)
{
    /* Corpus FUN_0041fb40: the config-derived speed factor. Issue #85 wires
     * the Options Game_Speed slider through it — the frame loop writes
     * options_time_scale(&game.options) into ctx->time_scale every frame
     * (1.0 while no option has been raised). 0 keeps the scaffold default. */
    if (ctx == NULL || ctx->time_scale == 0.0f) {
        return 1.0f;
    }
    return ctx->time_scale;
}

/* Engine teardown — triplet slot DAT_00505934 (finding 20). The corpus
 * selected LAB_00424cf0 (normal play) or LAB_00424d00 (loader mode) at
 * registration time (FUN_0041fa20); that choice is reproduced here.
 */
void EngineShutdown(EngineContext* ctx)
{
    if (ctx->app_mode == ENGINE_APP_LOADER) {
        /* corpus EngineShutdown @ 0x424d00 — subsystem closes, entity-array
         * destructors, LoaderProfile load. TODO(engine). */
        return;
    }
    /* corpus LAB_00424cf0 — stub forwarding to FUN_004250a0. TODO(engine). */
}
/* RuntimeFlag40000000Get (FUN_0041f970): the faithful masked read of the
 * port's engine-state word. */
uint32_t runtime_flag40000000_get(const EngineContext* ctx)
{
    return ctx != NULL ? (ctx->state & 0x40000000u) : 0u;
}

/* ModeDependentField24Get (FUN_00423080): mode-selected follow-span read.
 * The corpus indexes views[ctx->context_mode == 1 ? 0 : 2]; other modes
 * return the mode word verbatim, and the port reads the ViewContext's
 * follow_span slot (the corpus's word 0x24) through the threaded view
 * table (deviation: NULL slots answer zero, corpus dereferences blindly).
 */
int mode_dependent_field24_get(const EngineContext* ctx)
{
    if (ctx == NULL) {
        return 0; /* deviation: the corpus dereferences its global blindly */
    }
    int which;
    if (ctx->context_mode == 1) {
        which = 0;
    } else if (ctx->context_mode == 0) {
        which = 2;
    } else {
        return ctx->context_mode;
    }
    const ViewContext* v = ctx->views[which];
    return v != NULL ? (int)v->follow_span : 0;
}

/* --- the 0x425-0x426 engine-state band (see engine.hpp) ---------------------
 * The two threaded one-liners are the corpus's own reads of the context-mode
 * and cheat-word globals; the two DAT-global shapes land as zero-initialized
 * module storage per the unowned-global convention (the triple's only corpus
 * writer is the unported renderer-side sim-interaction update). */
static int s_scroll_guard;        /* DAT_004de548 */
static uint32_t s_triple[3];      /* DAT_0055651c / DAT_00556520 / DAT_00556524 */

bool current_engine_mode_is_zero(const EngineContext* ctx)
{
    return ctx != NULL && ctx->context_mode == 0;
}

void global_state_flags_bit7_set(EngineContext* ctx, int value)
{
    if (ctx == NULL) {
        return; /* deviation: the corpus writes its global blindly */
    }
    if (value != 0) {
        ctx->debug |= 0x80u;
    } else {
        ctx->debug &= ~0x80u;
    }
}

void global_scroll_guard_set(int value)
{
    s_scroll_guard = value;
}

int global_scroll_guard_peek(void)
{
    return s_scroll_guard;
}

void global_triple_copy_out(uint32_t* out)
{
    if (out == NULL) {
        return; /* deviation: the corpus writes through blindly */
    }
    out[0] = s_triple[0];
    out[1] = s_triple[1];
    out[2] = s_triple[2];
}
