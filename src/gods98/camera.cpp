// SPDX-License-Identifier: LGPL-3.0-or-later
#include "camera.hpp"
#include "message.hpp"
#include "crt/crt.hpp"
#include <math.h>

/* Reconstruction symbols for corpus globals. */
int   g_camera_scale_14   = 1;      /* DAT_00556f14, integer multiplier */
int   g_camera_grid_enabled = 1;    /* DAT_00556f18 */
// GLOBAL: LEGORR 0x00556ed8
float g_camera_countdown  = 0.0f;    /* DAT_00556ed8 */
static int s_predug_spawn_cells[256][2];
static uint32_t s_predug_spawn_count;
static int s_predug_spawn_enabled;
static CameraSpawnRandomIndexHook s_spawn_random_index_hook;
static CameraSpawnRandomAngleHook s_spawn_random_angle_hook;
static CameraSpawnObjectCellHook s_spawn_object_cell_hook;
static CameraSpawnCoordinateHook s_spawn_coordinate_hook;
static CameraSpawnCreateHook s_spawn_create_hook;
static CameraSpawnInitializeHook s_spawn_initialize_hook;
static void* s_spawn_hook_context;
static CameraGridSpawnHook s_grid_spawn_hook;
static void* s_grid_spawn_context;
static CameraSpawnRegistryProbeHook s_spawn_registry_probe_hook;
static void* s_spawn_registry_probe_context;

/* --- helper stubs (fill from the corpus one at a time) ------------------- */

/* The linked-record float advance has its original global list owner in
 * crt.cpp; the entity callback after it remains a separate seam. */
static void move_pipeline_hook(float delta)
{
    linked_list_conditional_float_advance(delta);
}

/* The corpus post-step callback is LAB_0046a5c0, dispatched through
 * EntityPoolFindByCallbackFiltered (FUN_004377b0). The generic entity walk
 * is ported; the callback's behavior remains a camera-specific seam. */
static void move_post_hook(int /*mode*/)
{
    /* TODO(camera): port LAB_0046a5c0 and dispatch it through the walker. */
}

/* Caller-specific control-registry predicate is exposed through
 * entity_flag_mask_callbacks_all_pass; this camera-only shim has no game
 * entity context to supply yet. */
static int control_probe(uint32_t /*id*/)
{
    /* TODO(camera): FUN_0046a4c0(0x40b) control-registry exist check. */
    return 0;
}

/* Spawn one pending grid cell through the installed game entity factory. */
static int grid_spawn(uint32_t* coords, uint32_t hint)
{
    if (coords == NULL || s_grid_spawn_hook == NULL) return 0;
    return s_grid_spawn_hook(coords, hint, s_grid_spawn_context);
}

void camera_grid_spawn_hook_set(CameraGridSpawnHook hook, void* context)
{
    s_grid_spawn_hook = hook;
    s_grid_spawn_context = context;
}

/* Query the game-owned registry before issuing a delayed spawn. */
static int spawn_probe(int first, int second)
{
    return s_spawn_registry_probe_hook != NULL
        ? s_spawn_registry_probe_hook(first, second,
                                      s_spawn_registry_probe_context) : 0;
}

void camera_spawn_registry_probe_hook_set(CameraSpawnRegistryProbeHook hook,
                                          void* context)
{
    s_spawn_registry_probe_hook = hook;
    s_spawn_registry_probe_context = context;
}
static void spawn_issue(int source_object, uint32_t type)
{
    predug_spawn_coordinate_select_and_create(source_object, (int)type);
}

void camera_predug_spawn_state_install(const int* cell_xy, uint32_t count,
                                      int enabled)
{
    uint32_t i;
    if (count > 256u) {
        count = 256u;
    }
    s_predug_spawn_count = 0u;
    s_predug_spawn_enabled = enabled != 0;
    if (cell_xy == NULL) {
        return;
    }
    for (i = 0u; i < count; ++i) {
        s_predug_spawn_cells[i][0] = cell_xy[i * 2u];
        s_predug_spawn_cells[i][1] = cell_xy[i * 2u + 1u];
    }
    s_predug_spawn_count = count;
}

/* SpawnCoordinatePoolClear (FUN_004372a0) resets the predug coordinate
 * array as part of its broader level-side cleanup. This module owns the
 * coordinate slice and keeps the reset available to that lifecycle seam. */
// FUNCTION: LEGORR 0x004372A0
void camera_predug_spawn_state_clear(void)
{
    uint32_t i;
    for (i = 0; i < 256u; ++i) {
        s_predug_spawn_cells[i][0] = 0;
        s_predug_spawn_cells[i][1] = 0;
    }
    s_predug_spawn_count = 0u;
    s_predug_spawn_enabled = 0;
}

void camera_spawn_hooks_install(CameraSpawnRandomIndexHook random_index,
                                CameraSpawnRandomAngleHook random_angle,
                                CameraSpawnObjectCellHook object_cell,
                                CameraSpawnCoordinateHook coordinate,
                                CameraSpawnCreateHook create,
                                CameraSpawnInitializeHook initialize,
                                void* context)
{
    s_spawn_random_index_hook = random_index;
    s_spawn_random_angle_hook = random_angle;
    s_spawn_object_cell_hook = object_cell;
    s_spawn_coordinate_hook = coordinate;
    s_spawn_create_hook = create;
    s_spawn_initialize_hook = initialize;
    s_spawn_hook_context = context;
}

/* PredugSpawnCoordinateSelectAndCreate (FUN_0043ad50): choose a configured
 * predug cell or resolve the source entity's cell, add the corpus 12.75-unit
 * radial offset, create the requested entity, and run its setup callback. */
// FUNCTION: LEGORR 0x0043AD50
void* predug_spawn_coordinate_select_and_create(int source_object, int type)
{
    int cell_x;
    int cell_y;
    int selected;
    float angle = 0.0f;
    float world_x;
    float world_y;
    void* entity;
    if (s_predug_spawn_enabled == 0 || s_predug_spawn_count == 0u
        || s_spawn_coordinate_hook == NULL || s_spawn_create_hook == NULL) {
        return NULL;
    }
    if (source_object == 0) {
        selected = s_spawn_random_index_hook != NULL
            ? s_spawn_random_index_hook((int)s_predug_spawn_count,
                                        s_spawn_hook_context)
            : 0;
        if (selected < 0 || (uint32_t)selected >= s_predug_spawn_count) {
            selected = 0;
        }
        cell_x = s_predug_spawn_cells[selected][0];
        cell_y = s_predug_spawn_cells[selected][1];
    } else if (s_spawn_object_cell_hook == NULL
               || s_spawn_object_cell_hook(source_object, &cell_x, &cell_y,
                                            s_spawn_hook_context) == 0) {
        return NULL;
    }
    if (s_spawn_random_angle_hook != NULL) {
        angle = s_spawn_random_angle_hook(s_spawn_hook_context);
    }
    if (s_spawn_coordinate_hook(cell_x, cell_y, &world_x, &world_y,
                                s_spawn_hook_context) == 0) {
        return NULL;
    }
    world_x += (float)sin((double)angle) * 12.75f;
    world_y += (float)cos((double)angle) * 12.75f;
    entity = s_spawn_create_hook(source_object, type, world_x, world_y, angle,
                                 s_spawn_hook_context);
    if (entity != NULL && s_spawn_initialize_hook != NULL) {
        s_spawn_initialize_hook(entity, s_spawn_hook_context);
    }
    return entity;
}

/* --- the six corpus functions -------------------------------------------- */

/* CameraMoveCommandRoute preserves the corpus's move-list call followed by
 * the conditional post-step walk; the list advance is implemented, while
 * the LAB_0046a5c0 entity callback remains unresolved. */
// FUNCTION: LEGORR 0x0046a730
void camera_move_pipeline(EngineContext* ctx, float delta)
{
    if (ctx == NULL) {
        return;
    }
    move_pipeline_hook(delta);
    if (g_move_hook_enabled()) {
        move_post_hook(0);
    }
}

/* MainSimFeatureFlagLatch (FUN_00435a50): probe control 0x40b when state
 * bit 0x08000000 is set, then latch debug bit 0x2. Context-owned state is
 * the port adaptation of the original globals. */
// FUNCTION: LEGORR 0x00435a50
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

/* CurrentLevelGet (FUN_00431560): return the active level-state root.
 * The port carries the original global through EngineContext. */
// FUNCTION: LEGORR 0x00431560
EngineState* camera_state_root(EngineContext* ctx)
{
    return ctx != NULL ? ctx->state_block : NULL;
}

// FUNCTION: LEGORR 0x0042c440
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

// FUNCTION: LEGORR 0x0042cb70
void camera_grid_expiry(EngineContext* ctx, float dt)
{
    EngineState* st = ctx != NULL ? ctx->state_block : NULL;
    /* Prefer the bound level's voxel grid (width/height +0x14/0x18, voxels
     * via the resolved pointer — the raw +0xb8 word truncates on 64-bit). */
    const uint8_t* cells = NULL;
    uint32_t gw = 0, gh = 0;
    if (ctx != NULL && ctx->level_block != NULL) {
        gw = (uint32_t)engine_level_width(ctx);
        gh = (uint32_t)engine_level_height(ctx);
        cells = ctx->level_voxels;
        if (cells == NULL) {
            cells = (const uint8_t*)engine_level_ptr(ctx, 0xb8);
        }
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

// FUNCTION: LEGORR 0x004261c0
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
    camera_move_pipeline(ctx, dt);
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
