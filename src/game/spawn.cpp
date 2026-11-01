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
int ledger_stub_0044a5d0_NearestOpenCellCoordSearch(
    EntitySlot* entity, int* output_x, int* output_y,
    int require_minimum_distance, int allow_maskc0, int allow_flag20);
int ledger_stub_00449880_BlockedCellApproachWeightWrite(
    EntitySlot* entity, unsigned int cell_x, unsigned int cell_y,
    float amount);
int ledger_stub_004473b0_RouteToCellCommandIssue(
    EntitySlot* entity, int target_x, int target_y, EntitySlot* target);
void entity_attached_object_list_detach(EntitySlot* entity, int reset_mode);
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

/* The blocked-cell approach helper chooses a connected neighbor and appends
 * its weighted waypoint. This entry then marks that waypoint and establishes
 * the reciprocal object link. */
// FUNCTION: LEGORR 0x004471b0
int entity_try_create_linked_at_position(EntitySlot* entity, float cell_x,
                                         float cell_y, EntitySlot* linked)
{
    if (entity == NULL || linked == NULL ||
        (entity->flags_3e0 & 0x400u) != 0u) return 0;
    if (!unported_ledger_stubs::
            ledger_stub_00449880_BlockedCellApproachWeightWrite(
                entity, (unsigned int)(int)cell_x,
                (unsigned int)(int)cell_y, 4.0f)) return 0;
    if (entity->wp_count <= 0) return 0;
    ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 3u;
    entity->link_2fc = (uintptr_t)linked;
    linked->attach_304 = entity;
    return 1;
}

/* Spawn the type-five object at the corpus's sentinel position, then let
 * the linked-position routine construct its approach route. */
// FUNCTION: LEGORR 0x004470d0
int entity_conditional_type_five_spawn(EntitySlot* entity)
{
    GameContext* context;
    int destination[2];
    int slot;
    EntitySlot* spawned;
    if (entity == NULL || (entity->flags_3e8 & 0x04000000u) == 0u ||
        entity->owner_ctx == NULL ||
        !(entity_stats_float_at90(entity) < entity->field_340)) return 0;
    context = entity->owner_ctx;
    if (context->level == NULL ||
        !unported_ledger_stubs::
            ledger_stub_0044a5d0_NearestOpenCellCoordSearch(
                entity, &destination[0], &destination[1], 1, 0, 0)) return 0;
    slot = entity_create(context, 5, NULL, 0.0f, 0.0f, 0.0f, 0.0f);
    if (slot < 0) return 0;
    spawned = (EntitySlot*)entity_at(context, 5, slot);
    if (spawned == NULL) return 0;
    entity_world_position_set(spawned, 10000.0f, 10000.0f, 0.0f, 1);
    if (entity_try_create_linked_at_position(
            entity, (float)destination[0], (float)destination[1], spawned)) {
        entity->field_33c = 0.0f;
        return 1;
    }
    entity_type5_finalize_if_allowed(spawned);
    return 0;
}

/* The two RNG draws select a starting neighbor and whether to walk the
 * twelve offsets forward or backward. The second pass drops the terrain-mask
 * filter; exhaustion detaches the entity from its attached-object list. */
// FUNCTION: LEGORR 0x00447210
int entity_adjacent_placement_cell_search(EntitySlot* entity,
                                          EntitySlot* target)
{
    static const int offsets[12][2] = {
        { 2, 0 }, { 0, 2 }, { -2, 0 }, { 0, -2 },
        { 2, -1 }, { 2, 1 }, { 1, 2 }, { -1, 2 },
        { -2, 1 }, { -2, -1 }, { -1, -2 }, { 1, -2 }
    };
    LevelState* level;
    int target_cell[2];
    int start;
    int direction;
    int pass;
    int i;
    if (entity == NULL || target == NULL || entity->type != 3 ||
        entity->owner_ctx == NULL || entity->owner_ctx->level == NULL)
        return 0;
    level = entity->owner_ctx->level;
    entity_position_to_terrain_cell(target, level, &target_cell[0],
                                    &target_cell[1]);
    start = (short)level_rand_u16() % 12;
    direction = (((short)level_rand_u16() & 1) != 0) ? -1 : 1;
    for (pass = 0; pass < 2; ++pass) {
        for (i = 0; i < 12; ++i) {
            int index = (start + i * direction) % 12;
            int x;
            int y;
            if (index < 0) index += 12;
            x = target_cell[0] + offsets[index][0];
            y = target_cell[1] + offsets[index][1];
            if (pass == 0 && level_cell_mask_180000(level, x, y)) continue;
            if (unported_ledger_stubs::
                    ledger_stub_004473b0_RouteToCellCommandIssue(
                        entity, x, y, target)) return 1;
        }
    }
    unported_ledger_stubs::entity_attached_object_list_detach(entity, 0);
    return 0;
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
