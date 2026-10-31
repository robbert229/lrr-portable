// SPDX-License-Identifier: LGPL-3.0-or-later
#include "spawn.hpp"
#include "entity.hpp"
#include "level.hpp"
#include "cmd.hpp"
#include "ai.hpp"
#include "../crt/crt.hpp"

/* These two target entries already have live port implementations in the
 * ledger unit. Keep the calls target-shaped at ConfiguredMonsterSpawnTick. */
namespace unported_ledger_stubs {
void ledger_stub_0044afb0_ConfiguredTinyMonsterSpawnAndTeardown(
    EntitySlot* source);
void ledger_stub_0044af90_EntityAttachedSpawnRecordProcess(EntitySlot* owner);
}
float random_float_range(float minimum, float maximum);

namespace lego {
namespace {
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
{
    uint32_t flags = entity_state_flags_peek();
    if (enabled != 0) flags |= 0x20u;
    else flags &= ~0x20u;
    entity_state_flags_set(flags);
}
int scatter_spawn_gate_get()
{ return (entity_state_flags_peek() & 0x20u) != 0u; }
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
        offset_x = random_float_range(-5.0f, 5.0f);
        offset_y = random_float_range(-5.0f, 5.0f);
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
        LegacyInterfaceInvoke invoke =
            reinterpret_cast<LegacyInterfaceInvoke>(vtable[0]);
#if defined(_MSC_VER)
        invoke(object, 0, 1);
#else
        invoke(object, 1);
#endif
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
    entity_state_flags_set(entity_state_flags_peek() | 0x40u);
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

// FUNCTION: LEGORR 0x0044bfd0
int configured_monster_spawn_behavior_tick(EntitySlot* entity)
{
    if (entity == NULL || !(0.0f < entity->activity_timer)) return 0;
    entity->flags_3e0 &= ~0x00100000u;
    unported_ledger_stubs::
        ledger_stub_0044afb0_ConfiguredTinyMonsterSpawnAndTeardown(entity);
    unported_ledger_stubs::
        ledger_stub_0044af90_EntityAttachedSpawnRecordProcess(entity);
    entity->flags_3e8 |= 0x00800000u;
    global_counter_at_00555328_increment();
    return 1;
}
}
