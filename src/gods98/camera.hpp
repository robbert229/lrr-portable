// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — camera/move cluster (finds the corpus calls at the top of
 * EngineTick, finding 03 §3: FUN_0046a730, FUN_00435a50, FUN_00431560,
 * FUN_0042c440, FUN_0042cb70, FUN_004261c0).
 *
 * First tranche of the "engine simulation fills": the six functions are
 * ported with their observed structure. Helpers that belong to subsystems
 * not yet reconstructed (message bus FUN_00419b80 / grid-spawn FUN_0040f190 /
 * registry FUN_00438960) are exposed here as stubs with their corpus
 * addresses, so each cell's logic stays visible and fillable one at a time.
 *
 * State-model notes (offsets into EngineState, i.e. DAT_005560d4 + off):
 *   +0x14/+0x18  grid width / height (u32)
 *   +0xb8        grid cell buffer (stride 0x48; +0x38 cell ptr, +0x3c busy,
 *                +0x40 a per-cell float timer decremented by dt)
 *   +0x268       spawn-reset interval (float)      (+0x26c spawn type)
 * Module globals mirror DAT_00556f14 (integer multiplier),
 * DAT_00556f18 (grid enabled), and DAT_00556ed8 (chip-spawn countdown).
 */
#ifndef LEGORRR_CAMERA_HPP
#define LEGORRR_CAMERA_HPP

#include "engine.hpp"

struct EntitySlot;

/* The corpus callback bodies at LAB_0046a5c0 and LAB_0046a490 are not
 * recovered. Callers may supply their behavior while this module preserves
 * the observed filtered-pool traversal. */
typedef int (*CameraMovePostCallback)(EntitySlot* entity, void* user);
typedef int (*CameraControlRegistryPredicate)(EntitySlot* entity,
                                             uint32_t bit, void* user);
void camera_entity_callback_hooks_set(CameraMovePostCallback move_callback,
                                      void* move_user,
                                      CameraControlRegistryPredicate
                                          control_predicate,
                                      void* control_user);

/* EngineTick step 4: the observed camera/move cluster (corpus lines 52-57).
 * `dt`/`scale` are supplied by the caller (the corpus's call sites pass them
 * implicitly by register; here they are explicit). */
void camera_cluster(EngineContext* ctx, float dt, float scale);

/* --- the six corpus functions, one-to-one ------------------------------- */

/* FUN_0046a730 — advance the DAT_004aa458 chain by delta, then optionally
 * traverse the filtered entity pool when DAT_004aa450 is nonzero. The
 * callback behavior is supplied separately because LAB_0046a5c0 is unknown. */
void camera_move_pipeline(EngineContext* ctx, float delta);

/* FUN_00435a50 — when the engine state bit 0x8000000 is set, probe the
 * control registry (FUN_0046a4c0(0x40b)) and raise the debug flag 0x2 if all
 * selected per-bit filtered-pool callbacks pass. LAB_0046a490 is unknown. */
void camera_registry_probe(EngineContext* ctx);

/* FUN_00431560 — the level/game state root (DAT_005560d4). */
EngineState* camera_state_root(EngineContext* ctx);

/* FUN_0042c440(state, dt) — decrement every per-cell float (+0x40) in the
 * cell list (count at +0xc, base at +8, stride 0x4c) while > 0. */
void camera_grid_timers(EngineContext* ctx, float dt);

/* FUN_0042cb70(state, dt) — grid-wide scan (w×h at +0x14/+0x18 over the
 * +0xb8 buffer, stride 0x48): for busy cells (+0x3c) advance +0x40 by dt and
 * fire a message (id 6) when a cell's floor timer (cell+0x38) is spent.
 * g_camera_scale_14 gates the spent check. */
void camera_grid_expiry(EngineContext* ctx, float dt);

/* FUN_004261c0(dt) — countdown reads spawn type +0x26c and re-arms from
 * +0x268 on the current level block; this target has one float argument. */
void camera_spawn_countdown(float dt);
void camera_spawn_level_block_bind(const void* level_block);

/* --- reconstruction-owned globals ---------------------------------------- */
extern int   g_camera_scale_14;   /* DAT_00556f14 */
extern int   g_camera_grid_enabled;/* DAT_00556f18 */
extern float g_camera_countdown;  /* DAT_00556ed8 */

/* DAT_004aa450 movement-hook enable (reconstruction-owned). */
int g_move_hook_enabled(void);
void camera_set_move_hook_enabled(int v);

typedef int (*CameraSpawnRandomIndexHook)(int limit, void* context);
typedef float (*CameraSpawnRandomAngleHook)(void* context);
typedef int (*CameraSpawnObjectCellHook)(int source_object, int* cell_x,
                                         int* cell_y, void* context);
typedef int (*CameraSpawnCoordinateHook)(int cell_x, int cell_y,
                                         float* world_x, float* world_y,
                                         void* context);
typedef void* (*CameraSpawnCreateHook)(int source_object, int type,
                                       float world_x, float world_y,
                                       float angle, void* context);
typedef void (*CameraSpawnInitializeHook)(void* entity, void* context);
typedef int (*CameraGridSpawnHook)(const uint32_t cell_xy[2], uint32_t hint,
                                   void* context);
void camera_grid_spawn_hook_set(CameraGridSpawnHook hook, void* context);
typedef int (*CameraSpawnRegistryProbeHook)(int first, int second,
                                            void* context);
void camera_spawn_registry_probe_hook_set(CameraSpawnRegistryProbeHook hook,
                                          void* context);
void camera_predug_spawn_state_install(const int* cell_xy, uint32_t count,
                                      int enabled);
void camera_predug_spawn_state_clear(void);
void camera_spawn_coordinate_hook_set(CameraSpawnCoordinateHook coordinate,
                                      void* context);
void camera_spawn_hooks_install(CameraSpawnRandomIndexHook random_index,
                                CameraSpawnRandomAngleHook random_angle,
                                CameraSpawnObjectCellHook object_cell,
                                CameraSpawnCoordinateHook coordinate,
                                CameraSpawnCreateHook create,
                                CameraSpawnInitializeHook initialize,
                                void* context);
void* predug_spawn_coordinate_select_and_create(int source_object,
                                                int type); /* FUN_0043ad50 */

/* Stub boundaries (corpus addresses; fill one at a time):
 *  FUN_0040f190 /
 *  FUN_00438960 / FUN_0043ad50 / FUN_00419b80 (message.hpp) */

#endif /* LEGORRR_CAMERA_HPP */
