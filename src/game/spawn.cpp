// SPDX-License-Identifier: LGPL-3.0-or-later
#include "spawn.hpp"
#include "port_loop_ports.hpp"
#include "entity.hpp"
#include "level.hpp"
#include "cmd.hpp"
#include "select.hpp"
#include "terrain_grid.hpp"
#include "type_config_ports.hpp"
#include "ai.hpp"
#include "gods98/engine.hpp"
#include "gods98/view.hpp"
#include "gods98/record.hpp"
#include "gods98/scene_resources.hpp"
#include "../crt/crt.hpp"
#include "../gods98/message.hpp"
#include "../gods98/camera.hpp"
#include <math.h>
#include <string.h>
#include "spawn_for_lego.hpp"
#include "spawn_for_legoobject.hpp"

/* These two target entries already have live port implementations in the
 * ledger unit. Keep the calls target-shaped at ConfiguredMonsterSpawnTick. */

namespace lego {
SpawnRandomTriggerHook s_trigger_hook = 0;
SpawnScatterHook s_scatter_hook = 0;
void* s_hook_context = 0;
SpawnTypeFiveFrameResourceOps s_type_five_frame_resource_ops;
SpawnScenePositionGetHook s_scene_position_get;
void* s_scene_position_context;
namespace {
PairingActivityPrepareHook s_pairing_activity_prepare_hook;
void* s_pairing_activity_prepare_context;
LegacyResultCheckHook s_result_check_hook = 0;
void* s_result_check_context = 0;
} // namespace
GlobalScatterSpawnFindHook s_global_scatter_find_hook = 0;
void* s_global_scatter_find_context = 0;
int s_global_scatter_flag_a = 0;
int s_global_scatter_flag_b = 0;

void spawn_type_five_frame_resource_ops_set(
    const SpawnTypeFiveFrameResourceOps* ops)
{
    if (ops == NULL)
        memset(&s_type_five_frame_resource_ops, 0,
               sizeof(s_type_five_frame_resource_ops));
    else
        s_type_five_frame_resource_ops = *ops;
}

void spawn_hooks_set(SpawnRandomTriggerHook trigger, SpawnScatterHook scatter, void* context)
{ s_trigger_hook = trigger; s_scatter_hook = scatter; s_hook_context = context; }
void spawn_scene_position_get_hook_set(SpawnScenePositionGetHook hook,
                                       void* context)
{
    s_scene_position_get = hook;
    s_scene_position_context = context;
}
void pairing_activity_prepare_hook_set(PairingActivityPrepareHook hook,
                                      void* context)
{
    s_pairing_activity_prepare_hook = hook;
    s_pairing_activity_prepare_context = context;
}
void pairing_activity_prepare(EntitySlot* target)
{
    if (s_pairing_activity_prepare_hook != NULL)
        s_pairing_activity_prepare_hook(target,
                                        s_pairing_activity_prepare_context);
    else if (target != NULL && !current_engine_mode_is_zero() &&
             selection_contains_entity_target(NULL, target, NULL))
        selection_clear_request_and_hud_reset();
}
int scatter_spawn_gate_get()
{ return (entity_state_flags_peek() & 0x20u) != 0u; }
void legacy_result_check_hook_set(LegacyResultCheckHook hook, void* context)
{ s_result_check_hook = hook; s_result_check_context = context; }
// FUNCTION: LEGORR 0x00480780
int zero_result_is_success(void* value)
{
    return s_result_check_hook != 0 && s_result_check_hook(s_result_check_context, value) == 0;
}

void global_scatter_spawn_find_hook_set(GlobalScatterSpawnFindHook hook,
                                        void* context)
{
    s_global_scatter_find_hook = hook;
    s_global_scatter_find_context = context;
}

void global_scatter_spawn_state_set(int flag_a, int flag_b)
{
    s_global_scatter_flag_a = flag_a != 0;
    s_global_scatter_flag_b = flag_b != 0;
}

}

namespace unported_ledger_stubs {

EmergenceSpawnHooks s_emergence_spawn_hooks;

const char* emergence_type_name_get(int owner_index)
{
    const TypeConfigTable* table = entity_type_config_table_for_entity_type(3);
    if (owner_index < 0 || table == NULL ||
        (uint32_t)owner_index >= table->count || table->names == NULL)
        return NULL;
    return table->names[owner_index];
}

void emergence_spawn_hooks_set(const EmergenceSpawnHooks* hooks)
{
    if (hooks == NULL) memset(&s_emergence_spawn_hooks, 0,
                              sizeof(s_emergence_spawn_hooks));
    else s_emergence_spawn_hooks = *hooks;
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// GLOBAL: LEGORR 0x00555118
uint32_t s_start_level_pilot_grid_count;

} // namespace unported_ledger_stubs
