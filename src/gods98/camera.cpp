// SPDX-License-Identifier: LGPL-3.0-or-later
#include "camera.hpp"
#include "message.hpp"

/* Reconstruction symbols for corpus globals. */
float g_camera_scale_14   = 1.0f;   /* DAT_00556f14 */
int   g_camera_grid_enabled = 1;    /* DAT_00556f18 */
float g_camera_countdown  = 0.0f;    /* DAT_00556ed8 */

/* --- helper stubs (fill from the corpus one at a time) ------------------- */

/* FUN_0046a700 — move pipeline step over the DAT_004aa458 entry list. */
static void move_pipeline_hook(EngineState* /*state*/, EngineContext* /*ctx*/)
{
    /* TODO(camera): FUN_0046a700 — walk the move list. */
}

/* FUN_004377b0 — post-step hook over the LAB_0046a5c0 label region. */
static void move_post_hook(int /*mode*/)
{
    /* TODO(camera): FUN_004377b0 — post-step hook. */
}

/* FUN_0046a4c0(id) — control-registry probe; nonzero = control present. */
static int control_probe(uint32_t /*id*/)
{
    /* TODO(camera): FUN_0046a4c0(0x40b) control-registry exist check. */
    return 0;
}

/* FUN_0040f190(&coords, hint) — grid cell spawn; nonzero = spawned. */
static int grid_spawn(uint32_t* /*coords*/, uint32_t /*hint*/)
{
    /* TODO(camera): FUN_0040f190 — instantiate at grid cell. */
    return 0;
}

/* FUN_00438960 — spawn registry probe; FUN_0043ad50 — issue a spawn. */
static int spawn_probe(int /*a*/, int /*b*/)
{
    return 1; /* corpus probe returns nonzero here in the shipped config */
}
static void spawn_issue(int /*a*/, uint32_t /*type*/)
{
    /* TODO(camera): FUN_0043ad50 — spawn entity of `type`. */
}

/* --- the six corpus functions -------------------------------------------- */

void camera_move_pipeline(EngineContext* ctx)
{
    if (ctx == NULL) {
        return;
    }
    move_pipeline_hook(ctx->state_block, ctx);
    if (g_move_hook_enabled()) {
        move_post_hook(0);
    }
}

void camera_registry_probe(EngineContext* ctx)
{
    if (ctx == NULL) {
        return;
    }
    if ((ctx->state & 0x8000000u) != 0) {
        if (control_probe(0x40b)) {
            ctx->debug |= 2u;
        }
    }
}

EngineState* camera_state_root(EngineContext* ctx)
{
    return ctx != NULL ? ctx->state_block : NULL;
}

void camera_grid_timers(EngineContext* ctx, float dt)
{
    EngineState* st = ctx != NULL ? ctx->state_block : NULL;
    if (st == NULL || st->move_list == NULL || st->move_list_count == 0) {
        return;
    }
    uint8_t* base = (uint8_t*)st->move_list;
    for (uint32_t i = 0; i < st->move_list_count; ++i) {
        float* f = (float*)(base + 0xc + i * 0x4c);
        if (*f > 0.0f) {
            *f -= dt;
        }
    }
}

void camera_grid_expiry(EngineContext* ctx, float dt)
{
    EngineState* st = ctx != NULL ? ctx->state_block : NULL;
    /* Prefer the bound level's voxel grid (width/height/voxels +0x14/0x18/0xb8). */
    const uint8_t* cells = NULL;
    uint32_t gw = 0, gh = 0;
    if (ctx != NULL && ctx->level_block != NULL) {
        gw = (uint32_t)engine_level_width(ctx);
        gh = (uint32_t)engine_level_height(ctx);
        cells = (const uint8_t*)engine_level_ptr(ctx, 0xb8);
    } else if (st != NULL) {
        gw = st->grid_w;
        gh = st->grid_h;
        cells = (const uint8_t*)st->grid_cells;
    }
    if (!g_camera_grid_enabled || gw == 0 || gh == 0 || cells == NULL) {
        return;
    }
    for (uint32_t y = 0; y < gh; ++y) {
        for (uint32_t x = 0; x < gw; ++x) {
            uint32_t idx = y * gw + x;
            uint8_t* cell = (uint8_t*)cells + idx * 0x48;
            uint32_t busy = *(uint32_t*)(cell + 0x3c);
            if (busy == 0) {
                continue;
            }
            float* timer = (float*)(cell + 0x40);
            *timer += dt;
            uint32_t spent = busy * (uint32_t)g_camera_scale_14 * 0x19;
            if ((float)spent < *timer) {
                *timer = 0.0f;
                uint32_t coords[2] = { x, y };
                uint32_t hint = *(uint32_t*)(cell + 0x38);
                if (grid_spawn(coords, hint)) {
                    message_dispatch(6, 0, 0, coords); /* corpus FUN_00419b80 */
                }
            }
        }
    }
}

void camera_spawn_countdown(EngineContext* ctx, float dt)
{
    EngineState* st = ctx != NULL ? ctx->state_block : NULL;
    if (st == NULL) {
        return;
    }
    if (st->spawn_type == 0x14) {
        return;
    }
    g_camera_countdown -= dt;
    if (g_camera_countdown < 0.0f) {
        if (spawn_probe(0, 0)) {
            spawn_issue(0, st->spawn_type);
        }
        g_camera_countdown = st->spawn_interval;
    }
}

void camera_cluster(EngineContext* ctx, float dt, float scale)
{
    (void)scale;
    /* corpus EngineTick step 4 (finding 03 §3). */
    camera_move_pipeline(ctx);
    camera_registry_probe(ctx);
    EngineState* st = camera_state_root(ctx);
    if (st != NULL) {
        camera_grid_timers(ctx, dt);
        camera_grid_expiry(ctx, dt);
        camera_spawn_countdown(ctx, dt);
    }
}

/* --- cross-module boundaries --------------------------------------------- */

int g_move_hook_enabled_flag = 0; /* DAT_004aa450 */

int g_move_hook_enabled(void)
{
    return g_move_hook_enabled_flag;
}

void camera_set_move_hook_enabled(int v)
{
    g_move_hook_enabled_flag = v != 0;
}