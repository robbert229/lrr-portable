// SPDX-License-Identifier: LGPL-3.0-or-later
#include <cstddef>

#include "engine.hpp"
#include "message.hpp"
#include "simtick.hpp"
#include "tickers.hpp"

/* ------------------------------------------------------------------ *
 * Per-ticker stubs. Each `ticker_<hex>` corresponds to the corpus's
 * `FUN_<hex>`; fill the bodies in from engine-region.c one at a time.
 * The table order below is the observed EngineTick call order and must
 * stay stable (finding 03 §4; corpus lines 93-119).
 * ------------------------------------------------------------------ */

static void ticker_00425d90(EngineContext* c, float dt, float scale)
{
    (void)scale;
    simtick_mouse_world(c, dt);
}

static void ticker_00402220(EngineContext* c, float dt, float scale)
{
    (void)dt; (void)scale;
    simtick_frame_counter(c);
}

static void ticker_00424600(EngineContext* c, float dt, float scale)
{
    (void)dt; (void)scale;
    simtick_grid_cell_pass(c);
}

static void ticker_0040aa10(EngineContext* c, float dt, float scale)
{
    simtick_table_dispatch(c, dt, scale);
}

static void ticker_0043b270(EngineContext* c, float dt, float scale)
{
    (void)scale;
    simtick_scene_pass(c, dt);
}

static void ticker_0046f550(EngineContext* c, float dt, float scale)
{
    (void)scale;
    simtick_sfx_bank(c, dt);
}

static void ticker_0040eab0(EngineContext* c, float dt, float scale)
{
    (void)scale;
    simtick_ambient_spawn(c, dt);
}

static void ticker_004313b0(EngineContext* c, float dt, float scale)
{
    simtick_minion_states(c, dt, scale);
}

static void ticker_004520d0(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* corpus FUN_004520d0 — the per-player command-queue executor
     * (finding 10 §"The queue executor"). The ported primitives live in
     * src/game/cmd.cpp and the executor runs from the game layer's per
     * frame hook (see frameloop.cpp) so gods98 stays linker-independent. */
}

static void ticker_004091e0(EngineContext* c, float dt, float scale)
{
    (void)dt; (void)scale;
    simtick_bobbins(c);
}

static void ticker_0041ba10(EngineContext* c, float dt, float scale)
{
    (void)scale;
    simtick_mute_timer(c, dt);
}

static void ticker_004291a0(EngineContext* c, float dt, float scale)
{
    (void)scale;
    simtick_render_pass(c, dt, 0.0f, 0); /* corpus calls FUN_004291a0(dt, 0) */
}

static void ticker_00449c00(EngineContext* c, float dt, float scale)
{
    (void)dt; (void)scale;
    simtick_render_step(c);
}

static void ticker_0040d450(EngineContext* c, float dt, float scale)
{
    (void)dt; (void)scale;
    simtick_post_cluster(c);
}

static void ticker_00426520(EngineContext* c, float dt, float scale)
{
    (void)scale;
    simtick_vu_meter(c, dt);
}

static void ticker_0044eff0(EngineContext* c, float dt, float scale)
{
    simtick_marker_sweep(c, dt, scale);
}

static void ticker_0046e390(EngineContext* c, float dt, float scale)
{
    (void)scale;
    simtick_fx_bank_tick(c, dt);
}

static void ticker_0047aba0(EngineContext* c, float dt, float scale)
{
    (void)dt; (void)scale;
    simtick_sound_keepalive(c);
}

static void ticker_00465b00(EngineContext* c, float dt, float scale)
{
    (void)scale;
    simtick_sound_bank_tick(c, dt);
}

static void ticker_004665c0(EngineContext* c, float dt, float scale)
{
    (void)dt; (void)scale;
    simtick_post_step(c);
}

static void ticker_0044cfa0(EngineContext* c, float dt, float scale)
{
    (void)dt; (void)scale;
    simtick_editor_steps(c);
}

static void ticker_0041a2c0(EngineContext* c, float dt, float scale)
{
    (void)scale;
    simtick_cooldown_tick(c, dt);
}

typedef struct TickerEntry {
    const char*  id;        /* corpus FUN_<hex> */
    void (*fn)(EngineContext*, float, float);
    uint32_t     skip_when; /* skip when (ctx->state & mask) — 0 = always run */
} TickerEntry;

static const TickerEntry TICKERS[] = {
    { "FUN_00425d90", ticker_00425d90, ENGINE_STATE_TIME_FROZEN },
    { "FUN_00402220", ticker_00402220, 0 },
    { "FUN_00424600", ticker_00424600, 0 },
    { "FUN_0040aa10", ticker_0040aa10, 0 },
    { "FUN_0043b270", ticker_0043b270, 0 },
    { "FUN_0046f550", ticker_0046f550, 0 },
    { "FUN_0040eab0", ticker_0040eab0, 0 },
    { "FUN_004313b0", ticker_004313b0, 0 },
    { "FUN_004520d0", ticker_004520d0, 0 },
    { "FUN_004091e0", ticker_004091e0, 0 },
    { "FUN_0041ba10", ticker_0041ba10, 0 },
    { "FUN_004291a0", ticker_004291a0, 0 },
    { "FUN_00449c00", ticker_00449c00, 0 },
    { "FUN_0040d450", ticker_0040d450, 0 },
    { "FUN_00426520", ticker_00426520, 0 },
    { "FUN_0044eff0", ticker_0044eff0, 0 },
    { "FUN_0046e390", ticker_0046e390, 0 },
    { "FUN_0047aba0", ticker_0047aba0, 0 },
    { "FUN_00465b00", ticker_00465b00, 0 },
    { "FUN_004665c0", ticker_004665c0, 0 },
    { "FUN_0044cfa0", ticker_0044cfa0, 0 },
    { "FUN_0041a2c0", ticker_0041a2c0, 0 },
};

/* The ordered EngineTick subsystem sequence (finding 03 §4; corpus 93-119). */
void EngineSubsystemTickers(EngineContext* ctx, float dt, float scale)
{
    for (int i = 0; i < (int)(sizeof(TICKERS) / sizeof(TICKERS[0])); ++i) {
        if ((TICKERS[i].skip_when != 0) && (ctx->state & TICKERS[i].skip_when)) {
            continue;
        }
        TICKERS[i].fn(ctx, dt, scale);
        ++ctx->tickers_ran;
    }
}

/* Camera / move cluster at the top of EngineTick — ported into camera.*
 * (camera_cluster → camera_move_pipeline / camera_registry_probe /
 * camera_grid_timers / camera_grid_expiry / camera_spawn_countdown). The
 * cluster is no longer a ticker here. */

/* Indexed manager dispatch — corpus ManagerTick FUN_0045a730. Under
 * ENGINE_STATE_INDEXED_SUBSYS the corpus calls it with idx
 *   1, 2, 4, 9, 6, 8, 0xb, 5, 7, 10
 * interleaved with the named manager tickers FUN_0045d6d0, FUN_0040e7f0,
 * FUN_0041b490, FUN_0041b930, FUN_0041a080, FUN_00419f30. Once an index is
 * identified (AI, particles, FX…), give it a name here.
 */
static const int MANAGER_ORDER[] = { 1, 2, 4, 9, 6, 8, 0xb, 5, 7, 10 };

static void ManagerTick(EngineContext* ctx, int idx, float dt)
{
    /* ported in simtick.* (FUN_0045a730): positional trackers with the
     * exponential smoothing; ingame = ~state bit 0x40000 (corpus). */
    int ingame = ((ctx->state & 0x40000u) == 0) ? 1 : 0;
    simtick_manager_tick(idx, dt, ingame);
}

void EngineManagerDispatch(EngineContext* ctx, float dt)
{
    for (size_t i = 0; i < sizeof(MANAGER_ORDER) / sizeof(MANAGER_ORDER[0]); ++i) {
        ManagerTick(ctx, MANAGER_ORDER[i], dt);
        ++ctx->tickers_ran;
    }
}

/* Render begin/end pairs + world draw vs editor pass — corpus lines 130-215:
 * surface lock/unlock (FUN_00473b40), sprite/animation draw, FUN_00477150
 * (render target, dt), the in-game world drawers FUN_00435300 / FUN_00454600,
 * and the &2/&0x2000-gated second render pass (FUN_004291a0(scale,1) etc.).
 */
void EngineRenderFrame(EngineContext* ctx, float dt, float scale, uint32_t* sim_out1)
{
    (void)ctx; (void)dt; (void)scale; (void)sim_out1;
    /* TODO(engine): begin/end pairs once surfaces exist (miniwin DDraw). */
}

/* Air-supply beat mechanic — corpus EngineTick tail. When the air supply
 * drains, the corpus fires notifications ids 0x10..0x13 (air-low / air-out)
 * through the message bus and loops SND_AirBeat whose pitch/position track
 * (finding 03 §7). The looping side rides the audio seam (airbeat_set_loop);
 * the engine has exactly one loop track, so engaging the air beat replaces
 * whatever was looping (a distinct audible cue for the demo).
 */

static AudioPlayer* s_air_player = NULL;
static const char* s_air_sound = NULL;
static int  s_air_loop_on = 0;
static int  s_air_prev_low = 0; /* last loop-engagement state */

void airbeat_set_loop(AudioPlayer* p, const char* sound)
{
    s_air_player = p;
    s_air_sound = sound;
    s_air_loop_on = 0;
    s_air_prev_low = 0;
}

int airbeat_loop_active(void)
{
    return s_air_loop_on;
}

void EngineAirBeat(EngineContext* ctx, float scale)
{
    (void)scale;
    EngineState* s = ctx->state_block;
    if (s == NULL) {
        return;
    }
    s->air_prev = s->air_beat;

    int low = s->air_beat <= 1;   /* draining / about to run out */
    int out = s->air_beat == 0;

    if (out) {
        message_dispatch(0x12, 0, 0, NULL); /* air out */
    } else if (s->air_prev != 0 && low) {
        message_dispatch(0x11, 0, 0, NULL); /* air low */
    }

    if (low != s_air_prev_low) {
        s_air_prev_low = low;
        if (low) {
            s_air_loop_on = 1;
            if (s_air_player != NULL && s_air_sound != NULL) {
                audio_loop_named(s_air_player, s_air_sound, 0.0f);
            }
        } else {
            s_air_loop_on = 0;
            if (s_air_player != NULL) {
                audio_loop_named(s_air_player, s_air_sound, 0.0f); /* repan */
            }
        }
    }
}