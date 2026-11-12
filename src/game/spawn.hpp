// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GAME_SPAWN_HPP
#define LEGORRR_GAME_SPAWN_HPP
#include "gods98/texture.hpp"
struct EntitySlot;
namespace lego {
typedef void (*SpawnRandomTriggerHook)(void* entity, void* context);
typedef void (*SpawnScatterHook)(void* entity, int count, int mode, void* context);
typedef int (*SpawnScenePositionGetHook)(::EntitySlot* entity,
                                         float position_xy[2], void* context);
typedef void (*PairingActivityPrepareHook)(::EntitySlot* target,
                                            void* context);
void spawn_hooks_set(SpawnRandomTriggerHook trigger, SpawnScatterHook scatter, void* context);
void spawn_scene_position_get_hook_set(SpawnScenePositionGetHook hook,
                                       void* context);
void pairing_activity_prepare_hook_set(PairingActivityPrepareHook hook,
                                      void* context);
void pairing_activity_prepare(::EntitySlot* target);
int entity_conditional_type_five_spawn(::EntitySlot* entity); /* 0x004470d0 */
typedef struct SpawnTypeFiveFrameResourceOps {
    void* context;
    /* Assigns the opaque TextureSurfaceCreate handle selected for the cell
     * to one frame of the entity's +0x1c frame container. */
    void (*frame_surface_set)(void* context, void* frame_container,
                              unsigned int frame_index,
                              void* texture_resource);
} SpawnTypeFiveFrameResourceOps;
void spawn_type_five_frame_resource_ops_set(
    const SpawnTypeFiveFrameResourceOps* ops);
extern "C" void object_list_field_assign_all(::EntitySlot* entity,
                                             void* texture_resource); /* 0x00440bf0 */
int entity_try_create_linked_at_position(::EntitySlot* entity, float cell_x,
                                         float cell_y,
                                         ::EntitySlot* linked); /* 0x004471b0 */
int entity_adjacent_placement_cell_search(::EntitySlot* entity,
                                          ::EntitySlot* target); /* 0x00447210 */
extern "C" void scatter_spawn_gate_set(int enabled); /* 0x0044adc0 */
int scatter_spawn_gate_get();
void entity_kind_one_random_spawn_trigger(void* entity); /* 0x0044ade0 */
void random_kind_six_scatter_spawn(void* entity, int count, int mode); /* 0x0044ae50 */
#if defined(_MSC_VER)
/* MSVC 5 has no __thiscall keyword. __fastcall places the object in ECX,
 * a dummy in EDX, and leaves the explicit argument on the stack. */
typedef void (__fastcall *LegacyInterfaceInvoke)(void* object, void* edx,
                                                 int argument);
#elif defined(__i386__)
typedef void (__attribute__((thiscall)) *LegacyInterfaceInvoke)(
    void* object, int argument);
#else
typedef void (*LegacyInterfaceInvoke)(void* object, int argument);
#endif
void interface_invoke_one_if_nonnull(void* object); /* 0x0047ed70 */
typedef int (*LegacyResultCheckHook)(void* context, void* value);
void legacy_result_check_hook_set(LegacyResultCheckHook hook, void* context);
int zero_result_is_success(void* value); /* 0x00480780 */
typedef int (*GlobalScatterSpawnFindHook)(::EntitySlot* entity, int mode,
                                          void* context);
void global_scatter_spawn_find_hook_set(GlobalScatterSpawnFindHook hook,
                                        void* context);
void global_scatter_spawn_state_set(int flag_a, int flag_b);
extern "C" void global_scatter_spawn_enable_and_drain(void); /* FUN_0044c550 */
int configured_monster_spawn_behavior_tick(::EntitySlot* entity);
}

namespace unported_ledger_stubs {
void type11_cell_coordinate_append(const int* coordinate);
int type11_cell_coordinate_nearest_resolve(const ::EntitySlot* target,
                                           int cell_xy_out[2]);
}
#endif
