// SPDX-License-Identifier: LGPL-3.0-or-later
#include "spawn.hpp"
#include "entity.hpp"
#include "level.hpp"
#include "cmd.hpp"
#include <string.h>
namespace lego {
namespace {
unsigned int s_spawn_flags = 0;
SpawnRandomTriggerHook s_trigger_hook = 0;
SpawnScatterHook s_scatter_hook = 0;
void* s_hook_context = 0;
SpawnScenePositionGetHook s_scene_position_get;
void* s_scene_position_context;
PairingActivityPrepareHook s_pairing_activity_prepare_hook;
void* s_pairing_activity_prepare_context;
LegacyResultCheckHook s_result_check_hook = 0;
void* s_result_check_context = 0;
GlobalScatterSpawnFindHook s_global_scatter_find_hook = 0;
void* s_global_scatter_find_context = 0;
int s_global_scatter_flag_a = 0;
int s_global_scatter_flag_b = 0;
ConfiguredMonsterSpawnHooks s_configured_monster_spawn_hooks;

static int global_scatter_find_adapter(EntitySlot* entity, void* payload)
{
    int mode = payload != NULL ? *(int*)payload : 0;
    if (s_global_scatter_find_hook == NULL) return 0;
    return s_global_scatter_find_hook(entity, mode,
                                     s_global_scatter_find_context);
}

static int global_scatter_find(int mode)
{
    return entity_pool_callback_search(global_scatter_find_adapter, &mode);
}
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
}
// FUNCTION: LEGORR 0x0044adc0
void scatter_spawn_gate_set(int enabled)
{ if (enabled != 0) s_spawn_flags |= 0x20U; else s_spawn_flags &= ~0x20U; }
int scatter_spawn_gate_get() { return (s_spawn_flags & 0x20U) != 0; }
// FUNCTION: LEGORR 0x0044ade0
void entity_kind_one_random_spawn_trigger(void* entity)
{
    EntitySlot* source = (EntitySlot*)entity;
    EntitySlot* spawn_owner;
    uint32_t count;
    if (source == NULL || scatter_spawn_gate_get() || source->type != 1 ||
        (source->flags_3ec & 0x00000800u) != 0u) return;
    if (s_trigger_hook != NULL) {
        s_trigger_hook(entity, s_hook_context);
        source->flags_3ec |= 0x00000800u;
        return;
    }
    spawn_owner = source;
    if ((source->flags_3ec & 0x00000040u) != 0u &&
        source->link_2fc != 0u) {
        spawn_owner = (EntitySlot*)source->link_2fc;
    }
    count = stats_table_word_at_b4(1, source->etype, 0);
    random_kind_six_scatter_spawn(spawn_owner, (int)count, 0);
    source->flags_3ec |= 0x00000800u;
}
// FUNCTION: LEGORR 0x0044ae50
void random_kind_six_scatter_spawn(void* entity, int count, int mode)
{
    EntitySlot* source = (EntitySlot*)entity;
    int i;
    if (source == NULL || count <= 0) return;
    if (s_scatter_hook != NULL) {
        s_scatter_hook(entity, count, mode, s_hook_context);
        return;
    }
    if (source->owner_ctx == NULL || source->owner_ctx->level == NULL) return;
    for (i = 0; i < count; ++i) {
        float base_xy[2];
        float spawn_xy[2];
        float offset_x;
        float offset_y;
        int cell_x, cell_y;
        EntitySlot* spawned;
        entity_transform_position_get(source, base_xy);
        if (mode != 0 && source->type == 4 && s_scene_position_get != NULL) {
            float scene_xy[2];
            if (s_scene_position_get(source, scene_xy,
                                     s_scene_position_context) != 0) {
                base_xy[0] = scene_xy[0];
                base_xy[1] = scene_xy[1];
            }
        }
        offset_x = (float)((double)level_rand_u16() * 3.051851e-05 * 10.0
                           - 5.0);
        offset_y = (float)((double)level_rand_u16() * 3.051851e-05 * 10.0
                           - 5.0);
        spawn_xy[0] = base_xy[0];
        spawn_xy[1] = base_xy[1];
        if (terrain_world_xy_to_cell(source->owner_ctx->level->ptr_terrain,
                                     base_xy[0] + offset_x,
                                     base_xy[1] + offset_y,
                                     &cell_x, &cell_y) != 0 &&
            level_cell_flag_8_bounded(source->owner_ctx->level,
                                      cell_x, cell_y) != 0u) {
            spawn_xy[0] += offset_x;
            spawn_xy[1] += offset_y;
        }
        spawned = entity_create_with_position(source->owner_ctx, 6, NULL, 0,
                                               spawn_xy[0], spawn_xy[1], 0.0f);
        if (spawned != NULL) cmd_create_task(spawned);
    }
}
// FUNCTION: LEGORR 0x0047ed70
void interface_invoke_one_if_nonnull(void* object)
{
    if (object != 0) {
        void** vtable = *static_cast<void***>(object);
        LegacyInterfaceInvoke invoke = reinterpret_cast<LegacyInterfaceInvoke>(vtable[0]);
        invoke(object, 1);
    }
}
void legacy_result_check_hook_set(LegacyResultCheckHook hook, void* context)
{ s_result_check_hook = hook; s_result_check_context = context; }
// FUNCTION: LEGORR 0x00480780
int zero_result_is_success(void* value)
{ return s_result_check_hook != 0 && s_result_check_hook(s_result_check_context, value) == 0; }

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

// FUNCTION: LEGORR 0x0044c550
void global_scatter_spawn_enable_and_drain(void)
{
    s_spawn_flags |= 0x40u;
    if (global_scatter_find(0) != 0) return;
    if (s_global_scatter_flag_b == 0) {
        if (global_scatter_find(1) == 0 && s_global_scatter_flag_a != 0) {
            s_global_scatter_flag_a = 0;
            s_global_scatter_flag_b = 0;
            global_scatter_find(1);
            return;
        }
    }
    else {
        s_global_scatter_flag_a = 0;
        s_global_scatter_flag_b = 0;
        global_scatter_find(0);
    }
}

void configured_monster_spawn_hooks_set(
    const ConfiguredMonsterSpawnHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_configured_monster_spawn_hooks, 0,
               sizeof(s_configured_monster_spawn_hooks));
    }
    else {
        s_configured_monster_spawn_hooks = *hooks;
    }
}

// FUNCTION: LEGORR 0x0044bfd0
int configured_monster_spawn_behavior_tick(EntitySlot* entity)
{
    if (entity == NULL || entity->activity_timer <= 0.0f) return 0;
    entity->flags_3e0 &= ~0x00100000u;
    if (s_configured_monster_spawn_hooks.configured_spawn_teardown != NULL) {
        s_configured_monster_spawn_hooks.configured_spawn_teardown(
            entity, s_configured_monster_spawn_hooks.context);
    }
    if (s_configured_monster_spawn_hooks.attached_spawn_record_process != NULL) {
        s_configured_monster_spawn_hooks.attached_spawn_record_process(
            entity, s_configured_monster_spawn_hooks.context);
    }
    entity->flags_3e8 |= 0x00800000u;
    if (s_configured_monster_spawn_hooks.global_counter_increment != NULL) {
        s_configured_monster_spawn_hooks.global_counter_increment(
            s_configured_monster_spawn_hooks.context);
    }
    return 1;
}
}
