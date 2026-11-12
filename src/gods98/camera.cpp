// SPDX-License-Identifier: LGPL-3.0-or-later
#include "camera.hpp"
#include "../game/entity.hpp"
#include "message.hpp"
#include "crt/crt.hpp"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "camera_shared.hpp"
#include "camera_for_lego.hpp"
#include "camera_for_legoobject.hpp"

/* Reconstruction symbols for corpus globals. */
int   g_camera_scale_14   = 1;      /* DAT_00556f14, integer multiplier */
int   g_camera_grid_enabled = 1;    /* DAT_00556f18 */
// GLOBAL: LEGORR 0x00556ed8
float g_camera_countdown  = 0.0f;    /* DAT_00556ed8 */
int s_predug_spawn_cells[256][2];
uint32_t s_predug_spawn_count;
int s_predug_spawn_enabled;
const uint8_t* s_spawn_level_block;
CameraSpawnRandomIndexHook s_spawn_random_index_hook;
CameraSpawnRandomAngleHook s_spawn_random_angle_hook;
CameraSpawnObjectCellHook s_spawn_object_cell_hook;
CameraSpawnCoordinateHook s_spawn_coordinate_hook;
CameraSpawnCreateHook s_spawn_create_hook;
CameraSpawnInitializeHook s_spawn_initialize_hook;
void* s_spawn_hook_context;
void* s_spawn_coordinate_context;
CameraGridSpawnHook s_grid_spawn_hook;
void* s_grid_spawn_context;
CameraSpawnRegistryProbeHook s_spawn_registry_probe_hook;
void* s_spawn_registry_probe_context;
CameraMovePostCallback s_move_post_callback;
void* s_move_post_user;
CameraControlRegistryPredicate s_control_registry_predicate;
void* s_control_registry_user;

/* --- helper stubs (fill from the corpus one at a time) ------------------- */

void camera_entity_callback_hooks_set(CameraMovePostCallback move_callback,
                                      void* move_user,
                                      CameraControlRegistryPredicate
                                          control_predicate,
                                      void* control_user)
{
    s_move_post_callback = move_callback;
    s_move_post_user = move_user;
    s_control_registry_predicate = control_predicate;
    s_control_registry_user = control_user;
}

void camera_grid_spawn_hook_set(CameraGridSpawnHook hook, void* context)
{
    s_grid_spawn_hook = hook;
    s_grid_spawn_context = context;
}

void camera_spawn_registry_probe_hook_set(CameraSpawnRegistryProbeHook hook,
                                          void* context)
{
    s_spawn_registry_probe_hook = hook;
    s_spawn_registry_probe_context = context;
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

/* Clear the camera module's reconstruction-side copy of predug coordinates. */
void camera_predug_spawn_state_clear(void)
{
    uint32_t i;
    for (i = 0; i < 256u; ++i) {
        s_predug_spawn_cells[i][0] = 0;
        s_predug_spawn_cells[i][1] = 0;
    }
    s_predug_spawn_count = 0u;
    s_predug_spawn_enabled = 0;
    s_spawn_level_block = NULL;
}

void camera_spawn_level_block_bind(const void* level_block)
{
    s_spawn_level_block = (const uint8_t*)level_block;
}

void camera_spawn_coordinate_hook_set(CameraSpawnCoordinateHook coordinate,
                                      void* context)
{
    s_spawn_coordinate_hook = coordinate;
    s_spawn_coordinate_context = context;
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
    s_spawn_coordinate_context = context;
}

/* --- the six corpus functions -------------------------------------------- */

/* Camera-context state root used by the reconstructed camera helpers. */
EngineState* camera_state_root(EngineContext* ctx)
{
    return ctx != NULL ? ctx->state_block : NULL;
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
        camera_spawn_countdown(dt);
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
