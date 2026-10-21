// SPDX-License-Identifier: LGPL-3.0-or-later
#include "engine.hpp"
#include "tickers.hpp"

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

    /* 2. Per-frame housekeeping (EnginePreTick @ 0x4231f0). The corpus runs
     *    the device pre-ticks (FUN_0047eef0 / FUN_0047f010) just before the
     *    tick; input_begin_frame is that model here. */
    if (ctx->input != NULL) {
        input_begin_frame(ctx->input);
    }
    EnginePreTick(ctx);

    /* 3. Scaled delta: subsystem tickers run scaled time, raw dt drives
     *    timers. Editor/preview (context_mode==0) runs raw dt. */
    float scale;
    if (ctx->context_mode == 0 && ctx->app_mode < 2) {
        scale = dt;
    } else {
        scale = EngineTimeScale(ctx) * dt;
    }

    /* 4. Camera / move cluster (corpus lines 52-88): the camera interpolation
     *    block, screen-space → world projection, and the force-scale-1.0
     *    one-shot. Left as a stub — see tickers.cpp EngineCameraCluster. */
    EngineCameraCluster(ctx, dt, scale);

    /* 5. Main simulation step — 0 => quit frame (MainSimStep @ 0x4250c0). */
    uint32_t sim_out1 = 0, sim_out2 = 0, sim_out3 = 0;
    if (MainSimStep(ctx, scale, dt, &sim_out1, &sim_out2, &sim_out3) == 0) {
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
 * input core (findings 04/05) supplies the text pump + mirror/clear here.
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
        input_text_pump(ctx->input, target, cap, &len);
        ctx->input->text_len = len;
    } else {
        input_mirror_and_clear(ctx->input);
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
    (void)ctx;
    /* TODO(engine): the real factor (a configuration-derived speed) lives
     * behind the corpus function; 1.0 == game speed. */
    return 1.0f;
}

/* Main simulation step / quit gate — corpus MainSimStep @ 0x4250c0.
 *
 * Consumes the frame's input + app state. Return 0 quits the frame loop.
 * The real body: quit-poll chain (FUN_004191a0 / FUN_004589a0 / KeepRunning),
 * selected-entity camera tracking, viewport-relative mouse deltas
 * (DAT_005560e4 camera * _DAT_0049e308 …), and the transition trigger
 * (DAT_00556ecc |= 0x2000000). The three outs are the step results the later
 * tickers consume (see corpus lines 731+ where they are zeroed and set).
 */
int MainSimStep(EngineContext* ctx, float scale, float dt,
                uint32_t* out1, uint32_t* out2, uint32_t* out3)
{
    (void)scale;
    (void)dt;

    *out1 = 0;
    *out2 = 0;
    *out3 = 0;

    if (EngineKeepRunning(ctx) == 0) {
        return 0;
    }

    /* TODO(engine): edge detection / camera input / transition state from
     * the corpus body; set the outs as the observed branches do. */
    return 1;
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