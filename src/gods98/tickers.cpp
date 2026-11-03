// SPDX-License-Identifier: LGPL-3.0-or-later
#include <cstddef>

#include "engine.hpp"
#include "message.hpp"
#include "record.hpp"
#include "simtick.hpp"
#include "surface.hpp"
#include "tickers.hpp"
#include "game/cmd.hpp"
#include "game/entity.hpp"
#include "game/effects.hpp"
#include "game/frontend.hpp"
#include "game/util.hpp"
#include "media.hpp"
#include "view.hpp"
#include "game/level.hpp"

/* ------------------------------------------------------------------ *
 * Per-ticker stubs. Each `ticker_<hex>` corresponds to the corpus's
 * `FUN_<hex>`; fill the bodies in from docs/decompiles/legacy/engine-region.c one at a time.
 * The table order below is the observed EngineTick call order and must
 * stay stable (finding 03 §4; corpus lines 93-119).
 * ------------------------------------------------------------------ */

static EngineRenderSceneListGetHook s_positioned_scene_list_get;
static void* s_positioned_scene_list_context;
static float s_positioned_medium_range;
static float s_positioned_high_range;

void engine_render_scene_list_get_hook_set(EngineRenderSceneListGetHook hook,
                                           void* context)
{
    s_positioned_scene_list_get = hook;
    s_positioned_scene_list_context = context;
}

void engine_render_poly_ranges_set(float medium_range, float high_range)
{
    s_positioned_medium_range = medium_range;
    s_positioned_high_range = high_range;
}

static void ticker_00425d90(EngineContext* c, float dt, float scale)
{
    (void)scale;
    simtick_mouse_world(c, dt);
}

static void ticker_00402220(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt;
    cmd_tick_global_delta(scale);
}

static void ticker_00424600(EngineContext* c, float dt, float scale)
{
    (void)dt;
    simtick_grid_cell_pass(c, scale);
}

static void ticker_0040aa10(EngineContext* c, float dt, float scale)
{
    simtick_table_dispatch(c, dt, scale);
}

static void ticker_0043b270(EngineContext* c, float dt, float scale)
{
    /* EngineTick 0x42358d passes its scaled ESI carrier to this owner. */
    (void)dt;
    simtick_scene_pass(c, scale);
}

static void ticker_0046f550(EngineContext* c, float dt, float scale)
{
    (void)scale;
    effects_simtick_sfx_hooks_install();
    simtick_sfx_bank(c, dt);
}

static void ticker_0040eab0(EngineContext* c, float dt, float scale)
{
    (void)c;
    (void)dt;
    simtick_ambient_spawn_tick(scale);
}

static void ticker_004313b0(EngineContext* c, float dt, float scale)
{
    simtick_minion_states(c, dt, scale);
}

static void ticker_004520d0(EngineContext* c, float dt, float scale)
{
    (void)c; (void)dt; (void)scale;
    /* SelectionRoot (FUN_004520d0): drain the command bank here, after the
     * minion-state ticker and before the three view ticks, matching the
     * EngineTick call order. */
    cmd_queue_tick();
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
    (void)c; (void)scale;
    motion_gameplay_tick(dt);
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
static int engine_subsystem_tickers_dispatch(EngineContext* ctx, float dt,
    float scale, int update_views, float camera_dt, float camera_scale)
{
    for (int i = 0; i < (int)(sizeof(TICKERS) / sizeof(TICKERS[0])); ++i) {
        if (update_views && TICKERS[i].fn == ticker_004091e0) {
            /*4235cc/e3/fa precede423603, then42361b collection. */
            view_views_tick(ctx, camera_dt, camera_scale);
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
            if (scene_root_object_get() != NULL &&
                !scene_runtime_cameras_sync()) return 0;
#endif
        }
        if ((TICKERS[i].skip_when != 0) && (ctx->state & TICKERS[i].skip_when)) {
            continue;
        }
        TICKERS[i].fn(ctx, dt, scale);
        ++ctx->tickers_ran;
    }
    return simtick_camera_detail_collection_ready();
}

void EngineSubsystemTickers(EngineContext* ctx, float dt, float scale)
{
    (void)engine_subsystem_tickers_dispatch(ctx, dt, scale, 0, 0.0f, 0.0f);
}

int EngineSubsystemTickersWithViews(EngineContext* ctx, float dt, float scale,
                                     float camera_dt, float camera_scale)
{
    return engine_subsystem_tickers_dispatch(ctx, dt, scale, 1,
                                             camera_dt, camera_scale);
}

/* Camera / move cluster at the top of EngineTick — ported into camera.*
 * (camera_cluster → camera_move_pipeline / camera_registry_probe /
 * camera_grid_timers / camera_grid_expiry / camera_spawn_countdown). The
 * cluster is no longer a ticker here. */

static void ManagerTick(EngineContext* ctx, int idx, float dt)
{
    /* ported in simtick.* (FUN_0045a730): positional trackers with the
     * exponential smoothing; ingame = ~state bit 0x40000 (corpus). */
    int ingame = ((ctx->state & 0x40000u) == 0) ? 1 : 0;
    simtick_manager_tick(idx, dt, ingame);
    /* ManagerTick's final operation draws this indexed panel's texture
     * slots after advancing its position and animation state. */
    panel_texture_slots_blit(idx);
}

void EngineManagerDispatch(EngineContext* ctx, float dt)
{
    /* Retail's indexed block is conditionally interleaved. In particular,
     * slots 1 and 2 have distinct engine-state gates and slot 0 is always
     * visited. The target calls are all under ENGINE_STATE_INDEXED_SUBSYS
     * in EngineTick. */
    if ((ctx->state & 2u) == 0u) {
        ManagerTick(ctx, 1, dt);
        ++ctx->tickers_ran;
    }
    ManagerTick(ctx, 0, dt);
    ++ctx->tickers_ran;
    if ((ctx->state & 2u) != 0u && (ctx->state & 0x2000u) != 0u &&
        (ctx->state & 0x20000u) == 0u) {
        ManagerTick(ctx, 2, dt);
        ++ctx->tickers_ran;
    }
    ManagerTick(ctx, 4, dt);
    ++ctx->tickers_ran;
    ManagerTick(ctx, 9, dt);
    ++ctx->tickers_ran;
    ManagerTick(ctx, 6, dt);
    ++ctx->tickers_ran;
    ManagerTick(ctx, 8, dt);
    ++ctx->tickers_ran;

    /* PositionTrackerUpdateAll (0x45d6d0), the encyclopedia text tick
     * (0x40e7f0), and AdvisorOverlayUpdate (0x41b490) still need their
     * owning tables. */
    ManagerTick(ctx, 0xb, dt);
    ++ctx->tickers_ran;
    timed_status_and_event_tick(dt);
    ManagerTick(ctx, 5, dt);
    ++ctx->tickers_ran;
    ManagerTick(ctx, 7, dt);
    ++ctx->tickers_ran;
    incident_message_text_consumer(dt);
    ManagerTick(ctx, 10, dt);
    ++ctx->tickers_ran;
    /* EngineTick's 0x800 in-game manager block calls the message text
     * consumer between ManagerTick(7) and ManagerTick(10), then advances
     * the info-message queue. */
    info_message_queue_tick(dt);
}

/* Render orchestration — corpus EngineTick lines 130-215. The port has the
 * second render-pass and render-step operations, indexed-surface restore,
 * and renderer begin-frame operation. The reference surface handles
 * (DAT_00556100/04, DAT_005560f0/f4/f8/fc and DAT_0055610c) and scene/render
 * objects (DAT_005560d8/dc/e0) are not present in EngineContext; do not cast
 * view/camera records or invent a renderer ABI to stand in for them. Thus the
 * corresponding lock/unlock, sprite, scene draw, world draw, and editor
 * camera/graph operations remain unavailable at these call sites. */
void EngineRenderFrame(EngineContext* ctx, float dt, float scale, uint32_t* sim_out1)
{
    const EntitySlot* positioned_source;
    (void)dt;
    (void)sim_out1;
    if (ctx == NULL) {
        return;
    }

    positioned_source = (const EntitySlot*)media_min_distance_context();
    if (ctx->context_mode == 1 && positioned_source != NULL &&
        s_positioned_scene_list_get != NULL) {
        void* scene_list = s_positioned_scene_list_get(
            s_positioned_scene_list_context, ctx);
        if (scene_list != NULL) {
            /* EngineTick 0x4237c6: DAT_556400/404 are Med/HighPolyRange. */
            entity_type_two_positioned_callback_dispatch(
                positioned_source, scene_list, s_positioned_medium_range,
                s_positioned_high_range, 1);
        }
    }

    if (ctx->context_mode == 0) {
        /* The original editor path restores indexed surface visibility after
         * its camera/scene pass. Those preceding operations require the
         * absent editor scene and render-surface handles. */
        indexed_surface_entries_restore();
    }

    /* The corpus draws the extra camera pass only when both bits are set and
     * the 0x20000 suppression bit is clear. FUN_004291a0 takes scaled time
     * and pass selector 1 here; its existing port seam models that selector
     * as the nonzero scale argument. */
    if ((ctx->state & 0x0002u) != 0 &&
        (ctx->state & 0x2000u) != 0 &&
        (ctx->state & 0x20000u) == 0) {
        simtick_render_pass(ctx, scale, 1.0f, 0);
        simtick_render_step(ctx);

        /* The intervening target lock, scene begin/draw/end, and target
         * unlock need DAT_005560e0, DAT_005560d8 and DAT_005560f4, none of
         * which has a corresponding EngineContext/API handle. */
        /* Native423a1e restores the shared terrain queue after the extra
         * pass, before the primary world-arrow capture at423a64. */
        terrain_camera_detail_restore((LevelState*)ctx->level_block);
    }

    /* World renderers (FUN_00435300/FUN_00454600), viewport update, primary
     * surface change-queue flush, and the preceding regular/editor surface
     * passes need renderer-owned roots/targets absent from the engine API. */

    if (ctx->context_mode == 0 && positioned_source != NULL) {
        /* EngineTick 0x4238d5: editor pass clears all positioned state. */
        entity_type_two_positioned_callback_dispatch(
            positioned_source, NULL, 0.0f, 0.0f, 0);
    }
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
