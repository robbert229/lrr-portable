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
extern "C" float random_float_range(float minimum, float maximum);

namespace lego {
namespace {
SpawnRandomTriggerHook s_trigger_hook = 0;
SpawnScatterHook s_scatter_hook = 0;
void* s_hook_context = 0;
SpawnTypeFiveFrameResourceOps s_type_five_frame_resource_ops;
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

void spawn_type_five_frame_resource_ops_set(
    const SpawnTypeFiveFrameResourceOps* ops)
{
    if (ops == NULL)
        memset(&s_type_five_frame_resource_ops, 0,
               sizeof(s_type_five_frame_resource_ops));
    else
        s_type_five_frame_resource_ops = *ops;
}

/* ObjectListFieldAssignAll (FUN_00440bf0) walks the animated frame container
 * at the type-5 entity's +0x1c field and assigns the TextureSurfaceCreate
 * handle selected by TerrainCellPaletteResourceAtGridPosition. */
// FUNCTION: LEGORR 0x00440BF0
void object_list_field_assign_all(EntitySlot* entity, void* texture_resource)
{
    void* frame_container;
    int count;
    unsigned int frame;
    if (entity == NULL || entity->type != 5) return;
    frame_container = entity->type_operation_fallback_1c;
    count = lego::animated_texture_frame_count(frame_container);
    if (count <= 0 ||
        s_type_five_frame_resource_ops.frame_surface_set == NULL) return;
    for (frame = 0u; frame < (unsigned int)count; ++frame) {
        s_type_five_frame_resource_ops.frame_surface_set(
            s_type_five_frame_resource_ops.context, frame_container, frame,
            texture_resource);
    }
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
        void* texture_resource =
            terrain_cell_palette_resource_at_grid_position_current(
                destination);
        object_list_field_assign_all(spawned, texture_resource);
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
    int16_t random_start;
    int16_t random_direction;
    uint32_t start;
    int direction;
    int pass;
    int i;
    if (entity == NULL || target == NULL || entity->type != 3 ||
        entity->owner_ctx == NULL || entity->owner_ctx->level == NULL)
        return 0;
    level = entity->owner_ctx->level;
    random_start = (int16_t)level_rand_u16();
    start = (uint32_t)((int)random_start % 12);
    random_direction = (int16_t)level_rand_u16();
    /* The reference's sign-corrected expression selects reverse traversal
     * only for a nonnegative odd second draw. Negative odd values do not
     * satisfy its final comparison. */
    direction = (random_direction >= 0 &&
        (((uint16_t)random_direction & 1u) != 0u)) ? -1 : 1;
    entity_position_to_terrain_cell(target, level, &target_cell[0],
                                    &target_cell[1]);
    for (pass = 0; pass < 2; ++pass) {
        for (i = 0; i < 12; ++i) {
            uint32_t sequence = start + (uint32_t)i;
            uint32_t index = direction < 0
                ? (0x18u - sequence) % 12u : sequence % 12u;
            int x;
            int y;
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
    else if (target != NULL && !current_engine_mode_is_zero() &&
             selection_contains_entity_target(NULL, target, NULL))
        selection_clear_request_and_hud_reset();
}
// FUNCTION: LEGORR 0x0044adc0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void scatter_spawn_gate_set(int enabled)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        test eax, eax
        mov eax, dword ptr [s_entity_state_flags]
        je scatter_spawn_gate_clear
        or al, 20h
        mov dword ptr [s_entity_state_flags], eax
        ret
    scatter_spawn_gate_clear:
        and al, 0dfh
        mov dword ptr [s_entity_state_flags], eax
        ret
        nop
        nop
        nop
    }
}
#else
void scatter_spawn_gate_set(int enabled)
{
    if (enabled != 0) s_entity_state_flags |= 0x20u;
    else s_entity_state_flags &= ~0x20u;
}
#endif
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
        if (mode != 0 && source->type == 4) {
            float scene_position[3];
            int have_scene_position = 0;
            if (s_scene_position_get != NULL) {
                float scene_xy[2];
                if (s_scene_position_get(source, scene_xy,
                                         s_scene_position_context) != 0) {
                    base_xy[0] = scene_xy[0];
                    base_xy[1] = scene_xy[1];
                }
            }
            else {
                void* scene_object = scene_object_secondary_handle_get_or_create(
                    source->attached_direction_record_type4);
                have_scene_position = ddiscene_object_position_get(
                    scene_object, NULL, scene_position);
                if (have_scene_position) {
                    base_xy[0] = scene_position[0];
                    base_xy[1] = scene_position[1];
                }
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

namespace unported_ledger_stubs {

static EmergenceSpawnHooks s_emergence_spawn_hooks;

static GameContext* emergence_game_context_get(const void* base_entity)
{
    const EntitySlot* base = (const EntitySlot*)base_entity;
    if (base != NULL && base->owner_ctx != NULL) return base->owner_ctx;
    return entity_pool_callback_context_get();
}

static LevelState* emergence_level_get(const void* base_entity)
{
    GameContext* context;
    if (s_emergence_spawn_hooks.level != NULL)
        return s_emergence_spawn_hooks.level;
    context = emergence_game_context_get(base_entity);
    return context != NULL ? context->level : NULL;
}

const char* emergence_type_name_get(int owner_index)
{
    const TypeConfigTable* table = entity_type_config_table_for_entity_type(3);
    if (owner_index < 0 || table == NULL ||
        (uint32_t)owner_index >= table->count || table->names == NULL)
        return NULL;
    return table->names[owner_index];
}

static int emergence_cell_world_position_get(int cell_x, int cell_y,
                                             float out_xy[2])
{
    float position[3];
    if (s_emergence_spawn_hooks.cell_world_position != NULL)
        return s_emergence_spawn_hooks.cell_world_position(cell_x, cell_y,
            out_xy, s_emergence_spawn_hooks.user);
    if (!terrain_cell_world_position_get(cell_x, cell_y, position)) return 0;
    out_xy[0] = position[0];
    out_xy[1] = position[1];
    return 1;
}

static EntitySlot* emergence_entity_create(void* base_entity,
                                           int owner_index, float x,
                                           float y, float heading)
{
    EntitySlot* created;
    const EntitySlot* base = (const EntitySlot*)base_entity;
    GameContext* context = emergence_game_context_get(base_entity);
    const char* type_name;
    int configured_name;
    if (s_emergence_spawn_hooks.create != NULL)
        return s_emergence_spawn_hooks.create(base_entity, 3, owner_index,
            x, y, heading, s_emergence_spawn_hooks.user);
    if (context == NULL) return NULL;
    type_name = emergence_type_name_get(owner_index);
    configured_name = type_name != NULL;
    if (type_name == NULL && base != NULL) type_name = base->name;
    created = entity_create_with_position(context, 3,
        type_name, owner_index, x, y, heading);
    if (created == NULL) return NULL;
    if (configured_name) {
        /* FUN_00437d00 receives the configured type row and owner index as
         * separate arguments; the port's name-based constructor needs the
         * row index restored explicitly. */
        created->etype = owner_index;
        return created;
    }
    if (base == NULL) return created;

    /* EntityCreateWithPosition owns allocation and world positioning. The
     * emergence caller supplies the type-3 template record, so carry over
     * the row identity and stats image that select the creature variant. */
    created->etype = base->etype;
    memcpy(created->type_name, base->type_name, sizeof(created->type_name));
    created->type_name[sizeof(created->type_name) - 1u] = '\0';
    created->stats_record = base->stats_record;
    created->task_state = base->task_state;
    created->activity_cap = base->activity_cap;
    return created;
}

void emergence_spawn_hooks_set(const EmergenceSpawnHooks* hooks)
{
    if (hooks == NULL) memset(&s_emergence_spawn_hooks, 0,
                              sizeof(s_emergence_spawn_hooks));
    else s_emergence_spawn_hooks = *hooks;
}

// FUNCTION: LEGORR 0x0043aea0
EntitySlot* emergence_random_spawn_attempt(int entity_index, int owner_index,
                                           int grid_width, int grid_height)
{
    unsigned int attempt;
    GameContext* context = entity_pool_callback_context_get();
    LevelState* level = s_emergence_spawn_hooks.level != NULL
        ? s_emergence_spawn_hooks.level
        : (context != NULL ? context->level : NULL);
    EntitySlot* base_entity;
    if (grid_width <= 0 && level != NULL)
        grid_width = level_width(level);
    if (grid_height <= 0 && level != NULL)
        grid_height = level_height(level);
    if (grid_width <= 0 || grid_height <= 0 || grid_width > 65535 ||
        grid_height > 65535 ||
        (s_emergence_spawn_hooks.spawn_try == NULL &&
         (context == NULL || level == NULL)))
        return NULL;
    base_entity = context != NULL
        ? (EntitySlot*)entity_at(context, 3, entity_index) : NULL;
    if (s_emergence_spawn_hooks.spawn_try == NULL && base_entity == NULL &&
        emergence_type_name_get(owner_index) == NULL)
        return NULL;
    for (attempt = 0; attempt < 1000u; ++attempt) {
        int16_t rx = s_emergence_spawn_hooks.random_u16 != NULL
            ? (int16_t)s_emergence_spawn_hooks.random_u16(
                s_emergence_spawn_hooks.user)
            : (int16_t)rng_thunk_draw15();
        int16_t ry = s_emergence_spawn_hooks.random_u16 != NULL
            ? (int16_t)s_emergence_spawn_hooks.random_u16(
                s_emergence_spawn_hooks.user)
            : (int16_t)rng_thunk_draw15();
        EntitySlot* entity;
        int cell_x = (int)rx % grid_width;
        int cell_y = (int)ry % grid_height;
        if (s_emergence_spawn_hooks.spawn_try != NULL) {
            entity = s_emergence_spawn_hooks.spawn_try(entity_index,
                owner_index, cell_x, cell_y, s_emergence_spawn_hooks.user);
        }
        else {
            entity = creature_spawn_from_emergence(base_entity, 3,
                owner_index, cell_x, cell_y);
        }
        if (entity != NULL) return entity;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0043af30
EntitySlot* creature_spawn_from_emergence(void* base_entity, int mode_token,
                                          int owner_index, int cell_x,
                                          int cell_y)
{
    static const int offsets[4][2] = { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } };
    static const float headings[4] = { 0.0f, 1.570796327f,
                                       3.141592654f, 4.712388980f };
    int direction;
    int neighbor_x = 0;
    int neighbor_y = 0;
    float origin[2];
    float neighbor[2];
    EntitySlot* created;
    LevelState* level = emergence_level_get(base_entity);
    GameContext* context = emergence_game_context_get(base_entity);
    if (mode_token != 3 ||
        (level == NULL && s_emergence_spawn_hooks.cell_eligible == NULL) ||
        (s_emergence_spawn_hooks.create == NULL && context == NULL)) {
        return NULL;
    }
    if (level != NULL) {
        if (cell_x < 0 || cell_y < 0 || cell_x >= level_width(level) ||
            cell_y >= level_height(level) ||
            !level_cell_walkable_flag(level, cell_x, cell_y) ||
            level_cell_flag_20_bounded(level, cell_x, cell_y) != 0u ||
            level_cell_mask_c0(level, cell_x, cell_y) != 0u ||
            level_cell_state_is_1(level, cell_x, cell_y) ||
            level_cell_flag_reserved(level, cell_x, cell_y) != 0u) {
            return NULL;
        }
    }
    else if (!s_emergence_spawn_hooks.cell_eligible(
                 cell_x, cell_y, s_emergence_spawn_hooks.user)) {
        return NULL;
    }
    for (direction = 0; direction < 4; ++direction) {
        neighbor_x = cell_x + offsets[direction][0];
        neighbor_y = cell_y + offsets[direction][1];
        if (level != NULL) {
            if (level_cell_flag_8_bounded(level, neighbor_x, neighbor_y)
                    == 0u ||
                 level_cell_mask_180000(level, neighbor_x, neighbor_y) != 0 ||
                 !terrain_cell_route_and_occupancy_check(NULL, level,
                     neighbor_x, neighbor_y, neighbor_x, neighbor_y, 0)) {
                continue;
            }
        }
        else if (!s_emergence_spawn_hooks.cell_eligible(
                     neighbor_x, neighbor_y,
                     s_emergence_spawn_hooks.user)) {
            continue;
        }
        break;
    }
    if (direction >= 4) return NULL;
    if (!emergence_cell_world_position_get(cell_x, cell_y, origin) ||
        !emergence_cell_world_position_get(neighbor_x, neighbor_y, neighbor))
        return NULL;
    origin[0] = (origin[0] * 9.0f + neighbor[0] * 10.0f) / 19.0f;
    origin[1] = (origin[1] * 9.0f + neighbor[1] * 10.0f) / 19.0f;
    created = emergence_entity_create(base_entity, owner_index, origin[0],
                                      origin[1], headings[direction]);
    if (created == NULL) return NULL;
    created->flags_3e0 |= 0x100u;
    created->flags_3e8 &= 0x7fffffffu;
    entity_dispatch_event31_for_pending_flag(created);
    ai_set_activity(created, 0x0c, 0);
    ai_idle_chain_record_attach(created);
    message_dispatch(0x1f, 0u, (uint32_t)(uintptr_t)created, NULL);
    /* The reference does not attach a scene action here. It resolves the
     * type-3 entity's renderer component (legacy +0x14, then handle +4) and
     * submits positional sound alias 10 through 0x465050. EntitySlot only
     * retains the owner scene object, not that component wrapper, so this
     * sound call remains unavailable from the current EntitySlot state. */
    level_tile_mark(level, cell_x, cell_y, 1);
    created->pos_pair_2f4 = (float)cell_x;
    created->pos_pair_2f8 = (float)cell_y;
    {
        static const char* type_names[3] = {
            "RockMonster", "Lavamonster", "Icemonster"
        };
        unsigned int i;
        int type, row;
        for (i = 0u; i < 3u; ++i) {
            if (entity_type_row_for_name(type_names[i], &type, &row) &&
                created->type == type && created->etype == row) {
                message_dispatch((int)i + 2, 0u,
                                (uint32_t)(uintptr_t)created, NULL);
                break;
            }
        }
    }
    message_dispatch(0x27, 0u, (uint32_t)(uintptr_t)created, NULL);
    global_counter_at_00555324_increment();
    if (s_emergence_spawn_hooks.post_create != NULL)
        s_emergence_spawn_hooks.post_create(created, cell_x, cell_y,
                                            s_emergence_spawn_hooks.user);
    return created;
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// GLOBAL: LEGORR 0x004e84ec
static int s_type11_cell_coordinates[10][2];
// GLOBAL: LEGORR 0x004e8540
static unsigned int s_type11_cell_coordinate_count;

// Type11CellCoordinateAppend
// FUNCTION: LEGORR 0x0044a390
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void type11_cell_coordinate_append(const int* coordinate)
{
    __asm {
        mov ecx, dword ptr [s_type11_cell_coordinate_count]
        cmp ecx, 0ah
        jae type11_append_return
        mov eax, dword ptr [esp + 4]
        mov edx, dword ptr [eax]
        mov dword ptr [ecx*8 + s_type11_cell_coordinates], edx
        mov ecx, dword ptr [s_type11_cell_coordinate_count]
        mov eax, dword ptr [eax + 4]
        mov dword ptr [ecx*8 + s_type11_cell_coordinates + 4], eax
        mov eax, dword ptr [s_type11_cell_coordinate_count]
        inc eax
        mov dword ptr [s_type11_cell_coordinate_count], eax
    type11_append_return:
        ret
    }
}
#else
void type11_cell_coordinate_append(const int* coordinate)
{
    volatile unsigned int* count = &s_type11_cell_coordinate_count;
    volatile int* coordinates = (volatile int*)s_type11_cell_coordinates;
    volatile const int* input = coordinate;
    const unsigned int first = *count;
    unsigned int second;
    if (first >= 10u) return;
    coordinates[first * 2u] = input[0];
    second = *count;
    coordinates[second * 2u + 1u] = input[1];
    ++*count;
}
#endif

// PlaceCellNearestResolve
// FUNCTION: LEGORR 0x0044a3d0
int type11_cell_coordinate_nearest_resolve(const EntitySlot* target,
                                                 int cell_xy_out[2])
{
    float target_xy[2];
    float best_distance = 10000.0f;
    unsigned int i;
    unsigned int best_index = 0xffffffffu;
    if (target == NULL) return 0;
    entity_transform_position_get_outputs(target, &target_xy[0],
                                          &target_xy[1]);
    for (i = 0; i < s_type11_cell_coordinate_count; ++i) {
        float world[3];
        float dx;
        float dy;
        float distance;
        int x = s_type11_cell_coordinates[i][0];
        int y = s_type11_cell_coordinates[i][1];
        if (!terrain_cell_world_position_get(x, y, world)) continue;
        dy = target_xy[1] - world[1];
        dx = target_xy[0] - world[0];
        distance = sqrtf(dy * dy + dx * dx);
        if (distance < best_distance) {
            best_distance = distance;
            best_index = i;
        }
    }
    if (best_index == 0xffffffffu) return 0;
    if (cell_xy_out != NULL) {
        cell_xy_out[0] = s_type11_cell_coordinates[best_index][0];
        cell_xy_out[1] = s_type11_cell_coordinates[best_index][1];
    }
    return 1;
}

} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x004372a0
void spawn_coordinate_pool_clear(void)
{
    unsigned int i;
    entity_iteration_callback_guard();
    grid_record_count_tables_clear();
    grid_object_records_release();
    level_predug_coordinate_pool_clear();
    camera_predug_spawn_state_clear();
    camera_spawn_coordinate_hook_set(NULL, NULL);
    for (i = 0; i < 10u; ++i) {
        unported_ledger_stubs::s_type11_cell_coordinates[i][0] = 0;
        unported_ledger_stubs::s_type11_cell_coordinates[i][1] = 0;
    }
    unported_ledger_stubs::s_type11_cell_coordinate_count = 0u;
    queued_grid_animation_accumulator_clear();
    ai_idle_table_reset();
    entity_global_animation_state_set(0u);
}

namespace unported_ledger_stubs {

// GLOBAL: LEGORR 0x00555118
uint32_t s_start_level_pilot_grid_count;

} // namespace unported_ledger_stubs
