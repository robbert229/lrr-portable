// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "ai.hpp"
#include "entity.hpp"
#include "weapon.hpp"
#include "effects.hpp"
#include "level.hpp"
#include "terrain_grid.hpp"
#include "cmd.hpp"
#include "gods98/model.hpp"
#include "gods98/view.hpp"
#include "gods98/record.hpp"
#include "gods98/heap.hpp"
#include "gods98/string.hpp"
#include "gods98/message.hpp"
#include "frontend.hpp"

static EntityRepeatedResourceCreateHook s_repeated_resource_create_hook;
static uint32_t s_next_ddiscene_owner_token = 1u;
static void* s_repeated_resource_create_context;
static char s_entity_kind_names[16][32][128];
static uint32_t s_entity_kind_name_counts[16];
static char s_entity_localized_names[16][32][128];
static char s_entity_localized_the_names[16][32][128];
static uint32_t s_entity_localized_name_counts[16];
static const char* s_entity_localized_name_entries[16][32];
static const char* s_entity_localized_the_name_entries[16][32];
// GLOBAL: LEGORR 0x00556254
static const char** s_object_name_kind1;
// GLOBAL: LEGORR 0x00556258
static const char** s_object_name_kind2;
// GLOBAL: LEGORR 0x0055625c
static const char** s_object_name_kind3;
// GLOBAL: LEGORR 0x00556260
static const char** s_object_name_kind4;
// GLOBAL: LEGORR 0x00556264
static const char** s_object_name_kind10;
// GLOBAL: LEGORR 0x00556268
static const char** s_object_the_name_kind1;
// GLOBAL: LEGORR 0x0055626c
static const char** s_object_the_name_kind2;
// GLOBAL: LEGORR 0x00556270
static const char** s_object_the_name_kind3;
// GLOBAL: LEGORR 0x00556274
static const char** s_object_the_name_kind4;
// GLOBAL: LEGORR 0x00556278
static const char** s_object_the_name_kind10;
// GLOBAL: LEGORR 0x0055618c
static const char* s_object_name_special[9];
// GLOBAL: LEGORR 0x005561b0
static const char* s_object_the_name_special[9];
static EntityHealthThresholdReadHook s_entity_health_threshold_read_hook;
static int s_entity_pair_activity_sentinel;
static EntityVisualResourcePresentHook s_entity_visual_resource_present_hook;
static EntityVisualResourceCreateHook s_entity_visual_resource_create_hook;
static EntityTeardownHooks s_entity_teardown_hooks;
static EntitySlot* s_entity_teardown_free_head;
static EntityCommandStatusFeedbackHooks s_entity_command_feedback_hooks;
static EntitySlot* s_entity_command_feedback_last_entity;
static char s_entity_command_feedback_last_text[256];
static void* s_entity_visual_resource_context;
static EntitySurfaceScaleReadHook s_entity_surface_scale_read_hook;
static void* s_entity_surface_scale_context;
static EntityIndexedFlagPublishHook s_entity_indexed_flag_publish_hook;
static void* s_entity_indexed_flag_context;
static EntityToolNameLoadHook s_entity_tool_name_load_hook;
static void* s_entity_tool_name_context;
static void* s_entity_tool_name_handles[11];
static EntityTypeTwoEventHook s_entity_type_two_event_hook;
static void* s_entity_type_two_event_context;
static EntityTypeTwoPositionedHooks s_entity_type_two_positioned_hooks;
static GameContext* s_entity_task_count_context;
static EntityTaskCoordinateMatchHook s_entity_task_coordinate_match_hook;
static void* s_entity_task_coordinate_match_user;
static EntityBuildingDepositHook s_entity_building_deposit_hook;
static void* s_entity_building_deposit_context;
static EntityDepositActivityHooks s_entity_deposit_activity_hooks;
static EntityBuildPlacementHooks s_entity_build_placement_hooks;
static GameContext* s_entity_build_placement_context;
static EntityActionIssueHooks s_entity_action_issue_hooks;
static EntityActionConsumeHooks s_entity_action_consume_hooks;
static EntityClippedRectangleHooks s_entity_clipped_rectangle_hooks;
static EntityPairRayHitCheckHook s_entity_pair_ray_hit_check_hook;
static void* s_entity_pair_ray_hit_check_context;
static void* s_entity_health_threshold_context;
static EntityStateOverrideEntry s_entity_state_override_entries[64];
static uint32_t s_entity_state_override_count;
static EntityStateOverrideValue330ApplyHook
    s_entity_state_override_value_330_apply;
static EntityStateOverrideRenameHook s_entity_state_override_rename;
static void* s_entity_state_override_context;

typedef struct EntitySimulationQueueItem {
    EntitySlot* target;
    void* record;
    int kind;
    struct EntitySimulationQueueItem* next;
} EntitySimulationQueueItem;

typedef struct EntitySimulationQueue {
    EntitySlot* source;
    EntitySimulationQueueItem* head;
    EntitySimulationQueueItem* tail;
    struct EntitySimulationQueue* next;
} EntitySimulationQueue;

static EntitySimulationQueue* s_entity_simulation_queues;
static EntityQueuedRecordProcessHook s_entity_simulation_process_hook;
static EntityQueuedRecordReleaseHook s_entity_simulation_release_hook;
static EntitySimulationImmediateModeHook s_entity_simulation_mode_hook;

void entity_simulation_queue_hooks_set(EntityQueuedRecordProcessHook process,
                                      EntityQueuedRecordReleaseHook release)
{
    s_entity_simulation_process_hook = process;
    s_entity_simulation_release_hook = release;
}

void entity_simulation_queue_mode_hook_set(
    EntitySimulationImmediateModeHook immediate_mode)
{
    s_entity_simulation_mode_hook = immediate_mode;
}

int entity_simulation_queue_attach(EntitySlot* source, EntitySlot* target,
                                   void* record, int record_kind)
{
    EntitySimulationQueue* queue;
    EntitySimulationQueueItem* item;
    if (source == NULL || record == NULL) return 0;
    for (queue = s_entity_simulation_queues; queue != NULL; queue = queue->next)
        if (queue->source == source) break;
    if (queue == NULL) {
        queue = (EntitySimulationQueue*)calloc(1, sizeof(*queue));
        if (queue == NULL) return 0;
        queue->source = source;
        queue->next = s_entity_simulation_queues;
        s_entity_simulation_queues = queue;
    }
    item = (EntitySimulationQueueItem*)malloc(sizeof(*item));
    if (item == NULL) return 0;
    item->target = target;
    item->record = record;
    item->kind = record_kind;
    item->next = NULL;
    if (queue->tail != NULL) queue->tail->next = item;
    else queue->head = item;
    queue->tail = item;
    return 1;
}

void entity_simulation_queue_flush(EntitySlot* source)
{
    EntitySimulationQueue* queue;
    EntitySimulationQueue* previous;
    EntitySimulationQueueItem* item;
    int direct;
    if (source == NULL) return;
    previous = NULL;
    for (queue = s_entity_simulation_queues; queue != NULL; queue = queue->next) {
        if (queue->source == source) break;
        previous = queue;
    }
    if (queue == NULL) return;

    /* The original chooses between immediate consumption and transfer to a
     * target's deferred array according to global simulation flags. */
    direct = s_entity_simulation_mode_hook != NULL
        && s_entity_simulation_mode_hook(source) != 0;
    item = queue->head;
    while (item != NULL) {
        EntitySimulationQueueItem* next = item->next;
        if (direct) {
            if (item->kind == 7) {
                if (s_entity_simulation_release_hook != NULL)
                    s_entity_simulation_release_hook(item->record);
            }
            else if (s_entity_simulation_process_hook != NULL)
                s_entity_simulation_process_hook(source, item->target, item->record);
        }
        else if (item->target != NULL && s_entity_simulation_process_hook != NULL) {
            /* The callback owns target-side deferred insertion and index
             * bookkeeping, which depend on the engine's absent record pool. */
            s_entity_simulation_process_hook(source, item->target, item->record);
        }
        free(item);
        item = next;
    }
    if (previous != NULL) previous->next = queue->next;
    else s_entity_simulation_queues = queue->next;
    free(queue);
}
static EntityAnimationStateApplyHook s_entity_animation_state_apply_hook;
static void* s_entity_animation_state_context;
static EntityPeriodicTickHook s_entity_periodic_tick_hook;
static void* s_entity_periodic_tick_context;
static float s_periodic_timer25;

void entity_repeated_resource_create_hook_install(
    EntityRepeatedResourceCreateHook hook, void* context)
{
    s_repeated_resource_create_hook = hook;
    s_repeated_resource_create_context = context;
}

/* CreateRepeatedResourceInstances (FUN_00408e50): mark the kind-7 visual
 * variant, increment the per-entity sequence for each instance, and dispatch
 * the native creation seam with the current sequence. */
// FUNCTION: LEGORR 0x00408E50
void entity_repeated_resource_instances_create(
    EntityRepeatedResourceState* state, int kind, int variant,
    void* value, void* entity, int count)
{
    if (state == NULL || count <= 0) {
        return;
    }
    if (kind == 7 && variant == 1) {
        state->flags |= 8u;
    }
    while (count-- > 0) {
        ++state->sequence_count;
        if (s_repeated_resource_create_hook != NULL) {
            s_repeated_resource_create_hook(kind, variant, value, entity,
                state->sequence_count, s_repeated_resource_create_context);
        }
    }
}

void entity_kind_name_tables_install(int kind, const char* const* names,
                                     uint32_t count)
{
    uint32_t i;
    if (kind < 1 || (kind > 4 && kind != 10)) {
        return;
    }
    if (count > 32u) {
        count = 32u;
    }
    memset(s_entity_kind_names[kind], 0, sizeof(s_entity_kind_names[kind]));
    s_entity_kind_name_counts[kind] = 0u;
    if (names == NULL) {
        return;
    }
    for (i = 0u; i < count; ++i) {
        if (names[i] != NULL) {
            strncpy(s_entity_kind_names[kind][i], names[i],
                    sizeof(s_entity_kind_names[kind][i]) - 1u);
        }
    }
    s_entity_kind_name_counts[kind] = count;
}

/* ItemNameByKindLookup's fixed labels reside at the literal DAT addresses
 * used by its original switch. Keep separate named arrays so Reccmp can map
 * each string reference without relying on literal pooling. */
// GLOBAL: LEGORR 0x004a1e98
static const char s_item_name_ore[] = "Ore";
// GLOBAL: LEGORR 0x004a1e9c
static const char s_item_name_power_crystal[] = "PowerCrystal";
// GLOBAL: LEGORR 0x004a3f40
static const char s_item_name_spider_web[] = "SpiderWeb";
// GLOBAL: LEGORR 0x004a3f90
static const char s_item_name_electric_fence[] = "ElectricFence";
// GLOBAL: LEGORR 0x004a3fa0
static const char s_item_name_processed_ore[] = "ProcessedOre";
// GLOBAL: LEGORR 0x004a3fb0
static const char s_item_name_barrier[] = "Barrier";
// GLOBAL: LEGORR 0x004a3fb8
static const char s_item_name_ooh_scary[] = "OohScary";
// GLOBAL: LEGORR 0x004a3fc4
static const char s_item_name_dynamite[] = "Dynamite";
// GLOBAL: LEGORR 0x004a46dc
static const char s_item_name_tv_camera[] = "tvcamera";

/* ItemNameByKindLookup (FUN_004396c0): fixed item labels plus the four
 * runtime-configured vehicle/minifigure/creature/building name tables. */
// FUNCTION: LEGORR 0x004396C0
const char* item_name_by_kind_lookup(uint32_t kind, int index)
{
    static const char* const special_names[] = {
        "", "", "", "", "", "", s_item_name_power_crystal, "",
        s_item_name_dynamite, s_item_name_barrier, "",
        s_item_name_electric_fence, s_item_name_spider_web,
        s_item_name_ooh_scary
    };
    if (kind >= 1u && kind <= 4u) {
        if (index < 0 || (uint32_t)index >= s_entity_kind_name_counts[kind]) {
            return NULL;
        }
        return s_entity_kind_names[kind][index];
    }
    if (kind == 7u) {
        return index == 0 ? s_item_name_ore : s_item_name_processed_ore;
    }
    if (kind == 0xffffffffu) {
        return s_item_name_tv_camera;
    }
    if (kind < sizeof(special_names) / sizeof(special_names[0])
        && special_names[kind][0] != '\0') {
        return special_names[kind];
    }
    return NULL;
}

void entity_localized_name_tables_install(int kind,
                                          const char* const* names,
                                          const char* const* the_names,
                                          uint32_t count)
{
    uint32_t i;
    if (kind < 1 || kind > 15) {
        return;
    }
    if (count > 32u) count = 32u;
    memset(s_entity_localized_names[kind], 0,
           sizeof(s_entity_localized_names[kind]));
    memset(s_entity_localized_the_names[kind], 0,
           sizeof(s_entity_localized_the_names[kind]));
    memset(s_entity_localized_name_entries[kind], 0,
           sizeof(s_entity_localized_name_entries[kind]));
    memset(s_entity_localized_the_name_entries[kind], 0,
           sizeof(s_entity_localized_the_name_entries[kind]));
    s_entity_localized_name_counts[kind] = 0u;
    for (i = 0u; i < count; ++i) {
        if (names != NULL && names[i] != NULL) {
            strncpy(s_entity_localized_names[kind][i], names[i], 127u);
            s_entity_localized_name_entries[kind][i] =
                s_entity_localized_names[kind][i];
        }
        if (the_names != NULL && the_names[i] != NULL) {
            strncpy(s_entity_localized_the_names[kind][i], the_names[i], 127u);
            s_entity_localized_the_name_entries[kind][i] =
                s_entity_localized_the_names[kind][i];
        }
    }
    s_entity_localized_name_counts[kind] = count;

    switch (kind) {
    case 1:
        s_object_name_kind1 = s_entity_localized_name_entries[kind];
        s_object_the_name_kind1 = s_entity_localized_the_name_entries[kind];
        break;
    case 2:
        s_object_name_kind2 = s_entity_localized_name_entries[kind];
        s_object_the_name_kind2 = s_entity_localized_the_name_entries[kind];
        break;
    case 3:
        s_object_name_kind3 = s_entity_localized_name_entries[kind];
        s_object_the_name_kind3 = s_entity_localized_the_name_entries[kind];
        break;
    case 4:
        s_object_name_kind4 = s_entity_localized_name_entries[kind];
        s_object_the_name_kind4 = s_entity_localized_the_name_entries[kind];
        break;
    case 6:
        s_object_name_special[0] = s_entity_localized_name_entries[kind][0];
        s_object_the_name_special[0] =
            s_entity_localized_the_name_entries[kind][0];
        break;
    case 7:
        for (i = 1u; i < 9u; ++i) {
            s_object_name_special[i] = NULL;
            s_object_the_name_special[i] = NULL;
        }
        for (i = 0u; i < count && i < 8u; ++i) {
            s_object_name_special[i + 1u] =
                s_entity_localized_name_entries[kind][i];
            s_object_the_name_special[i + 1u] =
                s_entity_localized_the_name_entries[kind][i];
        }
        break;
    case 8:
        s_object_name_special[2] = s_entity_localized_name_entries[kind][0];
        s_object_the_name_special[2] =
            s_entity_localized_the_name_entries[kind][0];
        break;
    case 9:
        s_object_name_special[3] = s_entity_localized_name_entries[kind][0];
        s_object_the_name_special[3] =
            s_entity_localized_the_name_entries[kind][0];
        break;
    case 10:
        s_object_name_kind10 = s_entity_localized_name_entries[kind];
        s_object_the_name_kind10 = s_entity_localized_the_name_entries[kind];
        break;
    case 11:
        s_object_name_special[5] = s_entity_localized_name_entries[kind][0];
        s_object_the_name_special[5] =
            s_entity_localized_the_name_entries[kind][0];
        break;
    case 12:
        s_object_name_special[6] = s_entity_localized_name_entries[kind][0];
        s_object_the_name_special[6] =
            s_entity_localized_the_name_entries[kind][0];
        break;
    case 13:
        s_object_name_special[7] = s_entity_localized_name_entries[kind][0];
        s_object_the_name_special[7] =
            s_entity_localized_the_name_entries[kind][0];
        break;
    case 15:
        s_object_name_special[8] = s_entity_localized_name_entries[kind][0];
        s_object_the_name_special[8] =
            s_entity_localized_the_name_entries[kind][0];
        break;
    }
}

static const char* entity_localized_name_pick(uint32_t kind, int index,
                                              int the_name)
{
    if (kind >= 1u && kind <= 10u && index >= 0
        && (uint32_t)index < s_entity_localized_name_counts[kind]) {
        const char* value = the_name != 0
            ? s_entity_localized_the_names[kind][index]
            : s_entity_localized_names[kind][index];
        if (value[0] != '\0') return value;
    }
    if (the_name != 0) {
        return NULL;
    }
    return item_name_by_kind_lookup(kind, index);
}

static int entity_display_name_config_index(const EntitySlot* entity)
{
    uint32_t i;
    int kind;
    if (entity == NULL || entity->type < 1 ||
        (entity->type > 4 && entity->type != 10)) return -1;
    kind = entity->type;
    for (i = 0u; i < s_entity_kind_name_counts[kind]; ++i) {
        if (strcmp(entity->type_name, s_entity_kind_names[kind][i]) == 0)
            return (int)i;
    }
    return -1;
}

/* EntityDisplayNameLookup (FUN_004395e0) receives an EntitySlot*. Its
 * category-specific index is represented by the config type name in the
 * portable entity record; kind 7's second-word selector is retained as
 * etype, matching the original +4 read. */
// FUNCTION: LEGORR 0x004395E0
const char* entity_display_name_lookup(const EntitySlot* entity)
{
    int index;
    if (entity == NULL || entity->type < 0) return NULL;
    if ((entity->type >= 1 && entity->type <= 4) || entity->type == 10) {
        index = entity_display_name_config_index(entity);
        return index >= 0
            ? entity_localized_name_pick((uint32_t)entity->type, index, 0)
            : NULL;
    }
    if (entity->type == 7)
        return entity_localized_name_pick(7u, entity->etype, 0);
    return entity_localized_name_pick((uint32_t)entity->type, 0, 0);
}

// FUNCTION: LEGORR 0x00439860
const char* localized_object_name_get(uint32_t kind, int index)
{
    switch (kind) {
    case 1: return s_object_name_kind1[index];
    case 2: return s_object_name_kind2[index];
    case 3: return s_object_name_kind3[index];
    case 4: return s_object_name_kind4[index];
    case 6: return s_object_name_special[0];
    case 7: return s_object_name_special[index + 1];
    case 8: return s_object_name_special[2];
    case 9: return s_object_name_special[3];
    case 10: return s_object_name_kind10[index];
    case 11: return s_object_name_special[5];
    case 12: return s_object_name_special[6];
    case 13: return s_object_name_special[7];
    case 15: return s_object_name_special[8];
    default: return NULL;
    }
}

// FUNCTION: LEGORR 0x00439790
const char* localized_object_the_name_get(uint32_t kind, int index)
{
    switch (kind) {
    case 1: return s_object_the_name_kind1[index];
    case 2: return s_object_the_name_kind2[index];
    case 3: return s_object_the_name_kind3[index];
    case 4: return s_object_the_name_kind4[index];
    case 6: return s_object_the_name_special[0];
    case 7: return s_object_the_name_special[index + 1];
    case 8: return s_object_the_name_special[2];
    case 9: return s_object_the_name_special[3];
    case 10: return s_object_the_name_kind10[index];
    case 11: return s_object_the_name_special[5];
    case 12: return s_object_the_name_special[6];
    case 13: return s_object_the_name_special[7];
    case 15: return s_object_the_name_special[8];
    default: return NULL;
    }
}

void entity_health_threshold_read_hook_install(
    EntityHealthThresholdReadHook hook, void* context)
{
    s_entity_health_threshold_read_hook = hook;
    s_entity_health_threshold_context = context;
}

/* EntityFlag400AndHealthThresholdCheck (FUN_004399c0): require flag 0x400;
 * type-1 entities also pass the corpus's indexed-count and 75-percent gate. */
// FUNCTION: LEGORR 0x004399C0
int entity_flag400_and_health_threshold_check(const EntitySlot* entity)
{
    EntityHealthThresholdState state;
    uint32_t value;
    if (entity == NULL || (entity->flags_3e0 & 0x400u) == 0u) {
        return 0;
    }
    if (entity->type != 1) {
        return 1;
    }
    memset(&state, 0, sizeof(state));
    if (s_entity_health_threshold_read_hook == NULL
        || s_entity_health_threshold_read_hook(entity, &state,
                s_entity_health_threshold_context) == 0) {
        return 1;
    }
    value = state.secondary_limit;
    if (state.upper_limit < value) {
        value = state.upper_limit;
    }
    if (state.required_value < value && state.health_percent < 75.0f) {
        return 0;
    }
    return 1;
}

void entity_animation_state_apply_hook_install(
    EntityAnimationStateApplyHook hook, void* context)
{
    s_entity_animation_state_apply_hook = hook;
    s_entity_animation_state_context = context;
}

// FUNCTION: LEGORR 0x00439CD0
int entity_animation_state_set(EntitySlot* entity, uint32_t state)
{
    if (entity == NULL || s_entity_animation_state_apply_hook == NULL) {
        return 0;
    }
    return s_entity_animation_state_apply_hook(entity, state,
                                               s_entity_animation_state_context);
}

/* Apply the requested animation state only when the entity capability word
 * contains bit 0x20. */
int entity_record_flag20_query(EntitySlot* entity, uint32_t state)
{
    if (entity == NULL
        || (entity_capability_flags_get(entity) & 0x20u) == 0u) {
        return 0;
    }
    return entity_animation_state_set(entity, state);
}

void entity_periodic_tick_hook_install(EntityPeriodicTickHook hook,
                                       void* context)
{
    s_entity_periodic_tick_hook = hook;
    s_entity_periodic_tick_context = context;
}

/* PeriodicTimer25Advance (FUN_0043c2b0): add the elapsed time, reset after
 * 25 units, then dispatch the filtered live-entity tick callback. */
// FUNCTION: LEGORR 0x0043C2B0
void periodic_timer25_advance(GameContext* ctx, float delta)
{
    int table;
    s_periodic_timer25 += delta;
    if (s_periodic_timer25 > 25.0f) {
        s_periodic_timer25 = 0.0f;
    }
    if (ctx == NULL || s_entity_periodic_tick_hook == NULL) {
        return;
    }
    for (table = 0; table < ENTITY_TYPES; ++table) {
        int count = ctx->entity_count[table];
        int index;
        if (count < 0) count = 0;
        if (count > ENTITY_CAP) count = ENTITY_CAP;
        for (index = 0; index < count; ++index) {
            EntitySlot* entity = &ctx->entity[table][index];
            if (entity->type != 0 && (entity->flags_3e8 & 0x80000u) == 0u) {
                s_entity_periodic_tick_hook(entity, delta,
                                            s_entity_periodic_tick_context);
            }
        }
    }
}

float periodic_timer25_peek(void)
{
    return s_periodic_timer25;
}
#include "util.hpp"
#include "gods98/media.hpp"
#include "gods98/resource.hpp"
#include "frontend.hpp"
#include "crt/crt.hpp"

static uint32_t s_entity_finalize_mode_flags;
static EntityFinalizeHook s_entity_type5_timeout_hook;
static EntityFinalizeHook s_entity_type5_finalize_hook;
static EntitySurfaceVectorGetHook s_entity_surface_vector_get_hook;
static EntityFlagDispatchHook s_entity_flag_dispatch_first;
static EntityFlagDispatchHook s_entity_flag_dispatch_second;
static EntitySegmentRadiusGetHook s_entity_segment_radius_hook;
static EntityVelocityApplyHook s_entity_velocity_hooks[5];
static EntityRayHitSweepOps s_entity_ray_hit_sweep_ops;
static EntitySteerVelocityApplyFn s_entity_steer_velocity_apply;
static void* s_entity_steer_velocity_context;
static EntityVelocitySlotDispatchOps s_entity_velocity_slot_dispatch_ops;
static EntityVelocitySteerDispatchOps s_entity_velocity_steer_dispatch_ops;
static EntityRouteSegmentAcceptHook s_entity_route_segment_accept_hook;
static EntityScenePoseHooks s_entity_scene_pose_hooks;

void entity_surface_vector_hook_set(EntitySurfaceVectorGetHook hook)
{
    s_entity_surface_vector_get_hook = hook;
}

// FUNCTION: LEGORR 0x004422a0
void entity_surface_vector_normalize(void* surface, float out_xy[2])
{
    float vector[2];
    float inverse_length;
    vector[0] = 0.0f;
    vector[1] = 0.0f;
    if (s_entity_surface_vector_get_hook != NULL)
        s_entity_surface_vector_get_hook(surface, vector);
    inverse_length = 1.0f / (float)sqrt(vector[0] * vector[0] + vector[1] * vector[1]);
    out_xy[0] = inverse_length * vector[0];
    out_xy[1] = inverse_length * vector[1];
}

// FUNCTION: LEGORR 0x00442480
float entity_vector_direction_angle_radians_get(void* surface)
{
    float direction[2];
    float angle;
    entity_surface_vector_normalize(surface, direction);
    angle = (float)atan2(direction[1], direction[0]);
    if (angle < 0.0f) angle += 6.2831855f;
    return angle;
}

void entity_flag_dispatch_hooks_set(EntityFlagDispatchHook first,
                                    EntityFlagDispatchHook second)
{
    s_entity_flag_dispatch_first = first;
    s_entity_flag_dispatch_second = second;
}

// FUNCTION: LEGORR 0x00445d70
int entity_flag_gated_callback_dispatch(EntitySlot* entity,
                                        uint32_t argument_a,
                                        uint32_t argument_b,
                                        uint32_t argument_c)
{
    int result = 0;
    if (entity == NULL) return result;
    if ((entity->flags_3e8 & 0x100u) != 0 && s_entity_flag_dispatch_first != NULL)
        s_entity_flag_dispatch_first(entity, argument_a, argument_b,
                                     argument_c, &result);
    if ((entity->flags_3e8 & 0x80u) != 0 && s_entity_flag_dispatch_second != NULL)
        s_entity_flag_dispatch_second(entity, argument_a, argument_b,
                                      argument_c, &result);
    return result;
}

// FUNCTION: LEGORR 0x00470630
int entity_directed_line_side_check(const EntitySlot* entity,
                                    const float point_xy[2],
                                    const float direction_xy[2])
{
    if (entity == NULL || point_xy == NULL || direction_xy == NULL) return 0;
    return ((entity->x - point_xy[0]) * direction_xy[0] +
            (entity->y - point_xy[1]) * direction_xy[1]) < 0.0f;
}

void entity_segment_radius_hook_set(EntitySegmentRadiusGetHook hook)
{
    s_entity_segment_radius_hook = hook;
}

// FUNCTION: LEGORR 0x00470540
int entity_segment_center_proximity_check(const EntitySlot* entity,
                                          const float start_xy[2],
                                          const float end_xy[2],
                                          int check_side)
{
    float line_x;
    float line_y;
    float inv_length;
    float projection;
    float projected_x;
    float projected_y;
    float dx;
    float dy;
    float radius;
    if (entity == NULL || start_xy == NULL || end_xy == NULL) return 0;
    if (check_side && entity_directed_line_side_check(entity, start_xy, end_xy))
        return 0;
    line_x = (end_xy[0] + start_xy[0]) - start_xy[0];
    line_y = (end_xy[1] + start_xy[1]) - start_xy[1];
    inv_length = 1.0f / (float)sqrt(line_x * line_x + line_y * line_y);
    line_x *= inv_length;
    line_y *= inv_length;
    projection = (entity->x - start_xy[0]) * line_x +
                 (entity->y - start_xy[1]) * line_y;
    projected_x = start_xy[0] + projection * line_x;
    projected_y = start_xy[1] + projection * line_y;
    dx = projected_x - entity->x;
    dy = projected_y - entity->y;
    radius = s_entity_segment_radius_hook != NULL
        ? s_entity_segment_radius_hook(entity) : 0.0f;
    return radius > (float)sqrt(dx * dx + dy * dy);
}

void ray_hit_cell_sweep_ops_set(const EntityRayHitSweepOps* ops)
{
    if (ops != NULL) s_entity_ray_hit_sweep_ops = *ops;
    else memset(&s_entity_ray_hit_sweep_ops, 0,
                sizeof(s_entity_ray_hit_sweep_ops));
}

/* RayHitCellDistanceMinimizeSweep (FUN_00471630): enumerate the grid cells
 * touched by the start/end segment and retain the nearest reported hit. */
// FUNCTION: LEGORR 0x00471630
void ray_hit_cell_distance_minimize_sweep(EntityRayHitSweepState* state)
{
    uint32_t width = 0u;
    uint32_t height = 0u;
    uint32_t start_x = 0u;
    uint32_t start_y = 0u;
    uint32_t end_x = 0u;
    uint32_t end_y = 0u;
    uint32_t min_x;
    uint32_t min_y;
    uint32_t max_x;
    uint32_t max_y;
    uint32_t x;
    uint32_t y;
    float endpoint[3];
    float direction_length;
    if (state == NULL || state->start == NULL || state->direction == NULL ||
        s_entity_ray_hit_sweep_ops.grid_dimensions_get == NULL ||
        s_entity_ray_hit_sweep_ops.world_to_cell == NULL ||
        s_entity_ray_hit_sweep_ops.cell_intersection == NULL ||
        s_entity_ray_hit_sweep_ops.grid_dimensions_get(
            s_entity_ray_hit_sweep_ops.context, &width, &height) == 0 ||
        width == 0u || height == 0u) return;
    for (x = 0u; x < 3u; ++x) {
        endpoint[x] = state->start[x] + state->direction[x];
    }
    if (!s_entity_ray_hit_sweep_ops.world_to_cell(
            s_entity_ray_hit_sweep_ops.context, state->start,
            &start_x, &start_y)) {
        if (start_x >= width) start_x = width - 1u;
        if (start_y >= height) start_y = height - 1u;
    }
    if (!s_entity_ray_hit_sweep_ops.world_to_cell(
            s_entity_ray_hit_sweep_ops.context, endpoint, &end_x, &end_y)) {
        if (end_x >= width) end_x = width - 1u;
        if (end_y >= height) end_y = height - 1u;
    }
    if (start_x >= width) start_x = width - 1u;
    if (start_y >= height) start_y = height - 1u;
    if (end_x >= width) end_x = width - 1u;
    if (end_y >= height) end_y = height - 1u;
    min_x = (start_x < end_x) ? start_x : end_x;
    max_x = (start_x < end_x) ? end_x : start_x;
    min_y = (start_y < end_y) ? start_y : end_y;
    max_y = (start_y < end_y) ? end_y : start_y;
    direction_length = (float)sqrt(
        state->direction[0] * state->direction[0] +
        state->direction[1] * state->direction[1] +
        state->direction[2] * state->direction[2]);
    if (direction_length <= 0.0f) return;
    for (x = min_x; x <= max_x; ++x) {
        for (y = min_y; y <= max_y; ++y) {
            float hit_point[3];
            float dx;
            float dy;
            float dz;
            float distance;
            if (!s_entity_ray_hit_sweep_ops.cell_intersection(
                    s_entity_ray_hit_sweep_ops.context, x, y,
                    state->start, state->direction, hit_point)) continue;
            dx = hit_point[0] - state->start[0];
            dy = hit_point[1] - state->start[1];
            dz = hit_point[2] - state->start[2];
            distance = (float)sqrt(dx * dx + dy * dy + dz * dz) /
                       direction_length;
            if (distance < state->max_distance) {
                state->max_distance = distance;
                state->hit = 1;
                state->hit_object = NULL;
            }
        }
    }
}

/* SceneSegmentTraceNearestHit (FUN_00471860): initialize the trace record,
 * let the scene walk populate its candidate context, then minimize against
 * grid-cell ray hits and publish the nearest result. */
// FUNCTION: LEGORR 0x00471860
int scene_segment_trace_nearest_hit(const float start[3],
                                   const float direction[3],
                                   void** hit_object_out,
                                   float* max_distance_io)
{
    EntityRayHitSweepState state;
    if (hit_object_out != NULL) *hit_object_out = NULL;
    if (start == NULL || direction == NULL || max_distance_io == NULL) return 0;
    state.hit_object = NULL;
    state.max_distance = *max_distance_io;
    state.start = start;
    state.direction = direction;
    state.hit = 0;
    if (s_entity_ray_hit_sweep_ops.scene_trace_prepare != NULL) {
        s_entity_ray_hit_sweep_ops.scene_trace_prepare(
            s_entity_ray_hit_sweep_ops.context, &state);
    }
    ray_hit_cell_distance_minimize_sweep(&state);
    *max_distance_io = state.max_distance;
    if (hit_object_out != NULL) *hit_object_out = state.hit_object;
    return state.hit;
}

/* ObjectPairRayHitCheck (FUN_004718d0): trace the normalized segment from
 * the first object to the second and ignore a hit reported on the target. */
// FUNCTION: LEGORR 0x004718d0
int object_pair_ray_hit_check(void* first, void* second)
{
    float start[3];
    float target[3];
    float direction[3];
    float max_distance = 1.0f;
    void* hit_object = NULL;
    int index;
    int hit;
    if (first == NULL || second == NULL ||
        s_entity_ray_hit_sweep_ops.object_position_get == NULL ||
        !s_entity_ray_hit_sweep_ops.object_position_get(
            s_entity_ray_hit_sweep_ops.context, first, start) ||
        !s_entity_ray_hit_sweep_ops.object_position_get(
            s_entity_ray_hit_sweep_ops.context, second, target)) return 0;
    for (index = 0; index < 3; ++index) {
        direction[index] = target[index] - start[index];
    }
    hit = scene_segment_trace_nearest_hit(
        start, direction, &hit_object, &max_distance);
    return (hit != 0 && hit_object != second) ? 1 : 0;
}

/* SeeThroughWallsRayCheck (FUN_00471960): short-circuit when the setting is
 * enabled; otherwise sweep the object-to-object segment through grid cells. */
// FUNCTION: LEGORR 0x00471960
int see_through_walls_ray_check(void* first, void* second)
{
    float start[3];
    float target[3];
    float direction[3];
    EntityRayHitSweepState state;
    int index;
    if (s_entity_ray_hit_sweep_ops.see_through_walls_enabled != NULL &&
        s_entity_ray_hit_sweep_ops.see_through_walls_enabled(
            s_entity_ray_hit_sweep_ops.context) != 0) return 0;
    if (first == NULL || second == NULL ||
        s_entity_ray_hit_sweep_ops.object_position_get == NULL ||
        !s_entity_ray_hit_sweep_ops.object_position_get(
            s_entity_ray_hit_sweep_ops.context, first, start) ||
        !s_entity_ray_hit_sweep_ops.object_position_get(
            s_entity_ray_hit_sweep_ops.context, second, target)) return 0;
    for (index = 0; index < 3; ++index) {
        direction[index] = target[index] - start[index];
    }
    state.hit_object = NULL;
    state.max_distance = 1.0f;
    state.start = start;
    state.direction = direction;
    state.hit = 0;
    ray_hit_cell_distance_minimize_sweep(&state);
    return state.hit;
}

void entity_steer_velocity_apply_hook_set(EntitySteerVelocityApplyFn hook,
                                          void* context)
{
    s_entity_steer_velocity_apply = hook;
    s_entity_steer_velocity_context = context;
}

/* EntityVelocitySteerBlendUpdate (FUN_00471a60): move the target when asked,
 * blend the existing velocity toward it at the time-scaled rate, normalize,
 * cap upward speed, then apply to the secondary and primary objects. */
// FUNCTION: LEGORR 0x00471a60
void entity_velocity_steer_blend_update(EntityVelocitySteerState* state,
                                        const float target[3], float elapsed,
                                        int update_target)
{
    float position[3];
    float desired[3];
    float length;
    float blend;
    int axis;
    if (state == NULL) return;
    if (update_target != 0 && target != NULL) {
        for (axis = 0; axis < 3; ++axis) state->target[axis] = target[axis];
    }
    if (state->primary == NULL || elapsed <= 0.00001f ||
        s_entity_ray_hit_sweep_ops.object_position_get == NULL ||
        !s_entity_ray_hit_sweep_ops.object_position_get(
            s_entity_ray_hit_sweep_ops.context, state->primary, position)) {
        return;
    }
    for (axis = 0; axis < 3; ++axis) {
        desired[axis] = state->target[axis] - position[axis];
    }
    length = (float)sqrt(desired[0] * desired[0] +
                         desired[1] * desired[1] +
                         desired[2] * desired[2]);
    if (length <= 0.0f) return;
    blend = 5.0f / elapsed;
    for (axis = 0; axis < 3; ++axis) {
        state->velocity[axis] = state->velocity[axis] * blend +
                                desired[axis] / length;
    }
    length = (float)sqrt(state->velocity[0] * state->velocity[0] +
                         state->velocity[1] * state->velocity[1] +
                         state->velocity[2] * state->velocity[2]);
    if (length > 0.0f) {
        for (axis = 0; axis < 3; ++axis) state->velocity[axis] /= length;
    }
    if (state->max_vertical_velocity < state->velocity[2]) {
        state->velocity[2] = state->max_vertical_velocity;
    }
    if (s_entity_steer_velocity_apply != NULL) {
        if (state->secondary != NULL) {
            float secondary_velocity[3];
            secondary_velocity[0] = state->velocity[0];
            secondary_velocity[1] = state->velocity[1];
            secondary_velocity[2] = 0.0f;
            s_entity_steer_velocity_apply(
                s_entity_steer_velocity_context, state->secondary,
                secondary_velocity, state->heading, 1);
        }
        s_entity_steer_velocity_apply(
            s_entity_steer_velocity_context, state->primary,
            state->velocity, state->heading, 0);
    }
}

/* PoseOffsetVelocitySlotBlend (FUN_00471f00): convert the pose's local
 * offset to world scale, add the pose position, and feed that target through
 * the shared velocity steering blend. */
// FUNCTION: LEGORR 0x00471f00
int pose_offset_velocity_slot_blend_update(
    EntityVelocitySteerState* state, const float offset[3],
    const float translation[3], float elapsed)
{
    float target[3];
    int axis;
    if (state == NULL || offset == NULL || translation == NULL) return 1;
    for (axis = 0; axis < 3; ++axis) {
        target[axis] = offset[axis] * 10000.0f + translation[axis];
    }
    entity_velocity_steer_blend_update(state, target, elapsed, 1);
    return 1;
}

void entity_velocity_slot_dispatch_ops_set(
    const EntityVelocitySlotDispatchOps* ops)
{
    if (ops != NULL) s_entity_velocity_slot_dispatch_ops = *ops;
    else memset(&s_entity_velocity_slot_dispatch_ops, 0,
                sizeof(s_entity_velocity_slot_dispatch_ops));
}

/* EntityVelocitySlotDispatch (FUN_00471fc0): walk every velocity slot and
 * choose the pose-offset path or the steer/sound path from entity bit 0. */
// FUNCTION: LEGORR 0x00471fc0
int entity_velocity_slot_dispatch(void* entity, void* argument)
{
    uint32_t flags = 0u;
    uint32_t count = 0u;
    uint32_t slot;
    if (entity == NULL || s_entity_velocity_slot_dispatch_ops.state_get == NULL ||
        !s_entity_velocity_slot_dispatch_ops.state_get(
            s_entity_velocity_slot_dispatch_ops.context, entity,
            &flags, &count)) return 1;
    for (slot = 0u; slot < count; ++slot) {
        if ((flags & 1u) != 0u) {
            if (s_entity_velocity_slot_dispatch_ops.steer_sound_dispatch != NULL) {
                s_entity_velocity_slot_dispatch_ops.steer_sound_dispatch(
                    s_entity_velocity_slot_dispatch_ops.context, entity,
                    argument, slot);
            }
        }
        else if (s_entity_velocity_slot_dispatch_ops.pose_slot_get != NULL) {
            EntityVelocitySteerState* state = NULL;
            float offset[3] = { 0.0f, 0.0f, 0.0f };
            float translation[3] = { 0.0f, 0.0f, 0.0f };
            float elapsed = 0.0f;
            if (s_entity_velocity_slot_dispatch_ops.pose_slot_get(
                    s_entity_velocity_slot_dispatch_ops.context, entity,
                    argument, slot, &state, offset, translation,
                    &elapsed)) {
                pose_offset_velocity_slot_blend_update(
                    state, offset, translation, elapsed);
            }
        }
    }
    return 1;
}

void entity_velocity_steer_dispatch_ops_set(
    const EntityVelocitySteerDispatchOps* ops)
{
    if (ops != NULL) s_entity_velocity_steer_dispatch_ops = *ops;
    else memset(&s_entity_velocity_steer_dispatch_ops, 0,
                sizeof(s_entity_velocity_steer_dispatch_ops));
}

/* VelocitySlotSteerDispatchAndSound (FUN_00471d20): blend the target first,
 * then run the two-candidate sound/steering branch when its global flag is
 * set and the current speed is nonpositive. */
int entity_velocity_steer_dispatch_and_sound(void* entity, void* argument,
                                             float delta)
{
    EntityVelocitySteerDispatchOps* ops =
        &s_entity_velocity_steer_dispatch_ops;
    EntityVelocitySteerState* state = NULL;
    float target[3] = { 0.0f, 0.0f, 0.0f };
    float current_velocity = 0.0f;
    uint32_t global_flags = 0u;
    void* active_entity = NULL;
    int candidate;
    if (entity == NULL || ops->steer_input_get == NULL) return 1;
    if (ops->steer_input_get(ops->context, entity, argument, delta, &state,
                             target, &current_velocity) != 0) {
        entity_velocity_steer_blend_update(state, target, delta, 1);
    }
    if (ops->effect_state_get == NULL ||
        !ops->effect_state_get(ops->context, &global_flags) ||
        (global_flags & 0x400u) == 0u || current_velocity > 0.0f ||
        ops->candidate_ready == NULL) return 1;
    if (ops->active_entity_get != NULL &&
        ops->active_entity_get(ops->context, &active_entity) != 0 &&
        active_entity == entity) return 1;
    for (candidate = 0; candidate < 2; ++candidate) {
        float speed_scale = 1.0f;
        if (ops->candidate_ready(ops->context, entity, argument,
                                 (uint32_t)candidate) == 0) return 1;
        if (ops->candidate_speed_scale != NULL) {
            speed_scale = ops->candidate_speed_scale(
                ops->context, entity, (uint32_t)candidate);
        }
        if (candidate == 0 && ops->first_candidate_initialize != NULL) {
            ops->first_candidate_initialize(ops->context, entity, argument);
        }
        if (ops->candidate_update != NULL &&
            ops->candidate_update(ops->context, entity, argument,
                                  (uint32_t)candidate, speed_scale) == 0) {
            return 1;
        }
    }
    return 1;
}

void entity_velocity_apply_hooks_set(EntityVelocityApplyHook vehicle,
                                     EntityVelocityApplyHook minifig,
                                     EntityVelocityApplyHook object,
                                     EntityVelocityApplyHook building,
                                     EntityVelocityApplyHook special)
{
    s_entity_velocity_hooks[0] = vehicle;
    s_entity_velocity_hooks[1] = minifig;
    s_entity_velocity_hooks[2] = object;
    s_entity_velocity_hooks[3] = building;
    s_entity_velocity_hooks[4] = special;
}

// FUNCTION: LEGORR 0x00442300
void entity_velocity_vector_dispatch(EntitySlot* entity, float angle,
                                     const float vector_or_null[3])
{
    float x, y, z;
    EntityVelocityApplyHook hook = NULL;
    if (entity == NULL) return;
    if (vector_or_null != NULL) {
        x = vector_or_null[0];
        y = vector_or_null[1];
        z = vector_or_null[2];
    }
    else {
        x = (float)sin(angle);
        y = (float)cos(angle);
        z = 0.0f;
    }
    if (entity->type >= 1 && entity->type <= 4)
        hook = s_entity_velocity_hooks[entity->type - 1];
    else if ((entity->flags_3e8 & 0x20000u) != 0)
        hook = s_entity_velocity_hooks[4];
    if (hook != NULL) hook(entity, x, y, z);
    entity->velocity_x = x;
    entity->velocity_y = y;
    entity->velocity_z = z;
    entity->movement_tick_scale = 0.01f;
}

void entity_route_segment_accept_hook_set(EntityRouteSegmentAcceptHook hook)
{
    s_entity_route_segment_accept_hook = hook;
}

// FUNCTION: LEGORR 0x00441700
int entity_route_setup_and_marker_associate(EntitySlot* entity,
                                            unsigned int count,
                                            const float* x_values,
                                            const float* y_values,
                                            int final_marker)
{
    AiWaypoint route[AI_WAYPOINTS_MAX];
    unsigned int i;
    (void)final_marker;
    if (entity == NULL || count > AI_WAYPOINTS_MAX ||
        (count != 0 && (x_values == NULL || y_values == NULL))) return 0;
    for (i = 0; i < count; ++i) {
        route[i].x = x_values[i];
        route[i].y = y_values[i];
        route[i].weight_x = 0.5f;
        route[i].weight_y = 0.5f;
        route[i].pending = 1;
        route[i].step_kind = 0;
        route[i].reserved[0] = 0;
        route[i].reserved[1] = 0;
        if (i + 1 < count && s_entity_route_segment_accept_hook != NULL &&
            s_entity_route_segment_accept_hook(entity, x_values[i], y_values[i],
                                                x_values[i + 1], y_values[i + 1]) == -1)
            return 0;
    }
    entity_action_sequence_reset(entity, 0);
    for (i = 0; i < count; ++i) ENTITY_WAYPOINTS(entity)[i] = route[i];
    entity->wp_count = (int)count;
    entity->wp_index = 0;
    entity->flags_3ec &= ~0x400u;
    entity->flags_3e0 |= 1u;
    entity->pos_pair_2f4 = 0.0f;
    entity->pos_pair_2f8 = 0.0f;
    return 1;
}

int entity_waypoint_storage_reserve(EntitySlot* entity, unsigned int count)
{
    AiWaypoint* grown;
    unsigned int current_capacity;
    unsigned int copy_count;
    if (entity == NULL) return 0;
    current_capacity = entity->wp_dynamic != NULL
        ? entity->wp_capacity : AI_WAYPOINTS_MAX;
    if (count <= current_capacity) return 1;
    if ((size_t)count > ((size_t)-1) / sizeof(AiWaypoint)) return 0;
    grown = (AiWaypoint*)realloc(entity->wp_dynamic,
                                (size_t)count * sizeof(AiWaypoint));
    if (grown == NULL) return 0;
    if (entity->wp_dynamic == NULL) {
        copy_count = (unsigned int)entity->wp_count;
        if (copy_count > AI_WAYPOINTS_MAX) copy_count = AI_WAYPOINTS_MAX;
        if (copy_count != 0u) {
            memcpy(grown, entity->wp, copy_count * sizeof(AiWaypoint));
        }
    }
    entity->wp_dynamic = grown;
    entity->wp_capacity = count;
    return 1;
}

/* AdjacentRouteStepAppend (FUN_004468c0): append a half-cell waypoint only
 * while route-building is active and the requested cell touches the current
 * tail cardinally. Grow the port's inline waypoint span with realloc when the
 * route reaches it, matching the corpus's count-based buffer growth. */
// FUNCTION: LEGORR 0x004468c0
int entity_route_adjacent_step_append(EntitySlot* entity, int cell_x,
                                      int cell_y)
{
    AiWaypoint* waypoint;
    int last_x;
    int last_y;
    if (entity == NULL || (entity->flags_3e0 & 1u) == 0u
        || entity->wp_count <= 0
        || entity->wp_count == 0x7fffffff) {
        return 0;
    }
    waypoint = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    last_x = (int)waypoint->x;
    last_y = (int)waypoint->y;
    if (!((cell_x == last_x &&
           (cell_y == last_y - 1 || cell_y == last_y + 1)) ||
          (cell_y == last_y &&
           (cell_x == last_x - 1 || cell_x == last_x + 1)))) {
        return 0;
    }
    if (!entity_waypoint_storage_reserve(
            entity, (unsigned int)entity->wp_count + 1u)) return 0;
    waypoint = &ENTITY_WAYPOINTS(entity)[entity->wp_count];
    waypoint->x = (float)cell_x;
    waypoint->y = (float)cell_y;
    waypoint->weight_x = 0.5f;
    waypoint->weight_y = 0.5f;
    waypoint->pending = 0;
    waypoint->step_kind = 1;
    waypoint->reserved[0] = 0;
    waypoint->reserved[1] = 0;
    ++entity->wp_count;
    entity->flags_3e8 &= ~0x400u;
    return 1;
}

void entity_surface_scale_read_hook_set(EntitySurfaceScaleReadHook hook,
                                        void* context)
{
    s_entity_surface_scale_read_hook = hook;
    s_entity_surface_scale_context = context;
}

float entity_surface_scale_get(void)
{
    float scale = (s_entity_surface_scale_read_hook == NULL) ? 1.0f
        : s_entity_surface_scale_read_hook(s_entity_surface_scale_context);
    return scale > 0.0f ? scale : 1.0f;
}

// FUNCTION: LEGORR 0x0040e350
void cell_entity_kind_create_or_remove(CellEntityKindRegistry* registry,
                                       uint32_t x, uint32_t y,
                                       int create)
{
    uint32_t index;
    void* current;
    if (registry == NULL || registry->records == NULL ||
        x >= registry->width || y >= registry->height) return;
    index = y * registry->width + x;
    current = registry->records[index];
    if (!create) {
        if (current != NULL && registry->release_entity != NULL)
            registry->release_entity(registry->context, current);
        registry->records[index] = NULL;
        return;
    }
    if (current != NULL || registry->grid_to_world == NULL ||
        registry->create_kind14 == NULL) return;
    {
        float world_x, world_y;
        if (registry->grid_to_world(registry->context, x, y,
                                    &world_x, &world_y))
            registry->records[index] = registry->create_kind14(
                registry->context, world_x, world_y);
    }
}

// FUNCTION: LEGORR 0x00409b30
EntitySlot* world_object_spawn_request_create(
    const WorldObjectCreateHooks* hooks,
    const WorldObjectSpawnRequest* request,
    int notify)
{
    EntitySlot* entity;
    uint32_t i;
    float orientation;
    if (hooks == NULL || request == NULL ||
        hooks->entity_create_type4 == NULL) return NULL;
    orientation = (float)request->orientation_quadrant * 1.5707964f;
    entity = hooks->entity_create_type4(hooks->context, request, orientation);
    if (entity == NULL) return NULL;
    if (notify != 0 && hooks->event8_dispatch != NULL)
        hooks->event8_dispatch(hooks->context, entity);
    for (i = 0; request->coordinate_pairs != NULL &&
         i < request->coordinate_pair_count; ++i) {
        const int32_t* pair = request->coordinate_pairs + i * 2u;
        const int is_matching_pair =
            (i + 1u < request->coordinate_pair_count &&
             pair[0] == pair[2] && pair[1] == pair[3]);
        if (is_matching_pair) {
            if (hooks->matching_pair_prepare != NULL)
                hooks->matching_pair_prepare(hooks->context, pair[0], pair[1]);
            if (hooks->matching_pair_attach != NULL)
                hooks->matching_pair_attach(hooks->context, entity, pair);
            ++i;
        }
        else if ((entity_stats_word_get(entity) & 0x100000u) != 0u) {
            if (hooks->boundary_pair_special != NULL)
                hooks->boundary_pair_special(hooks->context, pair);
        }
        else {
            if (hooks->boundary_pair_unlink != NULL)
                hooks->boundary_pair_unlink(hooks->context, pair);
            if (hooks->boundary_pair_apply != NULL)
                hooks->boundary_pair_apply(hooks->context, pair[0], pair[1]);
            if ((entity_state_flags_get(entity) & 0x200u) != 0u &&
                hooks->boundary_pair_secondary != NULL) {
                hooks->boundary_pair_secondary(hooks->context, pair);
            }
            if (hooks->cardinal_neighbors_apply != NULL)
                hooks->cardinal_neighbors_apply(hooks->context,
                                                pair[0], pair[1]);
        }
        if (hooks->pair_refresh != NULL)
            hooks->pair_refresh(hooks->context, pair[0], pair[1]);
    }
    return entity;
}

// FUNCTION: LEGORR 0x00409a90
void world_object_create_with_transform(const WorldObjectCreateHooks* hooks,
                                        uint32_t object_kind,
                                        const int32_t position[2],
                                        uint32_t subtype,
                                        const int32_t* coordinate_pairs,
                                        uint32_t coordinate_pair_count,
                                        int notify)
{
    WorldObjectSpawnRequest request;
    if (hooks == NULL || position == NULL) return;
    memset(&request, 0, sizeof(request));
    request.object_kind = object_kind;
    request.orientation_quadrant = subtype;
    request.origin_x = position[0];
    request.origin_y = position[1];
    request.coordinate_pairs = coordinate_pairs;
    request.coordinate_pair_count = coordinate_pair_count;
    if (hooks->terrain_pairs_prepare != NULL)
        hooks->terrain_pairs_prepare(hooks->context, coordinate_pairs,
                                     coordinate_pair_count, 4.0f);
    if (hooks->neighbor_cells_refresh != NULL)
        hooks->neighbor_cells_refresh(hooks->context, position);
    world_object_spawn_request_create(hooks, &request, notify);
}

void entity_scene_pose_hooks_set(const EntityScenePoseHooks* hooks)
{
    if (hooks == NULL) {
        s_entity_scene_pose_hooks.context = NULL;
        s_entity_scene_pose_hooks.poses_get = NULL;
    }
    else s_entity_scene_pose_hooks = *hooks;
}

// FUNCTION: LEGORR 0x00443510
void scene_object_pose_position_sync(EntitySlot* entity)
{
    EntityScenePose* source = NULL;
    EntityScenePose* target = NULL;
    int changed = 0;
    unsigned int i;
    if (entity == NULL || s_entity_scene_pose_hooks.poses_get == NULL ||
        !s_entity_scene_pose_hooks.poses_get(s_entity_scene_pose_hooks.context,
                                             entity, &source, &target) ||
        source == NULL || target == NULL) return;
    for (i = 0; i < 3; ++i) {
        if (source->position[i] != target->position[i]) changed = 1;
    }
    if (changed) memcpy(target->position, source->position, sizeof(source->position));
    changed = 0;
    for (i = 0; i < 7; ++i) {
        if (source->orientation[i] != target->orientation[i]) changed = 1;
    }
    if (changed)
        memcpy(target->orientation, source->orientation, sizeof(source->orientation));
}

// FUNCTION: LEGORR 0x0040fde0
void set_vector_and_notify_listeners(float* state, const float* delta)
{
    steering_physics_step(NULL, state, delta);
}

// FUNCTION: LEGORR 0x00441ed0
int entity_face_target_action_start(EntitySlot* entity, EntitySlot* target,
                                    int mode)
{
    float vector[3];
    float length;
    if (entity == NULL || target == NULL || entity->wake_timer > 0.0f ||
        target->type != ENTITY_TYPE_OBJ) return 0;
    entity_action_sequence_reset(entity, 0);
    vector[0] = target->x - entity->x;
    vector[1] = target->y - entity->y;
    vector[2] = target->z - entity->z;
    length = (float)sqrt(vector[0] * vector[0] + vector[1] * vector[1] +
                         vector[2] * vector[2]);
    vector[0] /= length;
    vector[1] /= length;
    vector[2] /= length;
    entity_velocity_vector_dispatch(entity, 0.0f, vector);
    entity->facing_target_x = target->x;
    entity->facing_target_y = target->y;
    entity->facing_target_z = target->z;
    if (mode == 1) entity->flags_3e4 |= 0x00400000u;
    else if (mode == 2) entity->flags_3e4 |= 0x00800000u;
    else if (mode == 3) entity->flags_3e4 |= 0x01000000u;
    return 1;
}

/* EntityFinishState1A (FUN_0043c4c0): finish the linked target's face
 * action when the port can represent that target, clear the finishing bit,
 * and discard the entity's type-0x1a command record. */
// FUNCTION: LEGORR 0x0043c4c0
void entity_finish_state_1a(EntitySlot* entity)
{
    EntitySlot* target;
    if (entity == NULL) return;
    if (entity->link_2fc != 0u) {
        target = (EntitySlot*)entity->link_2fc;
        if (target->type == ENTITY_TYPE_OBJ && entity->wake_timer <= 0.0f)
            (void)entity_face_target_action_start(entity, target, 1);
    }
    entity->flags_3e4 &= ~0x04000000u;
    (void)cmd_chain_drop(entity, 0x1a);
}

/* EntityMarkAndFinishState1A (FUN_0043c530): the corpus only marks a type-2
 * entity when its command head is state 0x1a, then shares the finish path. */
// FUNCTION: LEGORR 0x0043c530
void entity_mark_and_finish_state_1a(EntitySlot* entity)
{
    if (entity == NULL || entity->type != 2 || entity->cmd_head == NULL ||
        cmd_rec_type(entity->cmd_head) != 0x1a) return;
    entity->flags_3e4 |= 0x04000000u;
    entity_finish_state_1a(entity);
}

// FUNCTION: LEGORR 0x00441940
void entity_action_sequence_reset(EntitySlot* entity, int preserve_mode)
{
    if (entity == NULL) return;
    entity->wp_count = 0;
    entity->wp_index = 0;
    entity->activity_last = 0;
    entity->flags_3e0 &= 0xff7ffffeu;
    entity->flags_3e4 &= 0xffffbbffu;
    if ((entity->flags_3e0 & 2u) != 0) {
        entity->rot = -entity->rot;
        entity->rot = -entity->rot;
        entity->flags_3e0 &= ~2u;
    }
    if (preserve_mode == 0) {
        entity->activity_timer = 0.0f;
        entity->activity = -1;
        entity->activity_param = 0;
    }
}

// FUNCTION: LEGORR 0x00441b30
void entity_state_reset(EntitySlot* entity, int clear_mode, int reset_mode)
{
    if (entity == NULL) return;
    entity_action_sequence_reset(entity, 0);
    entity->flags_3e4 &= 0xfffcfcffu;
    entity->flags_3e0 &= 0x7bfa7fffu;
    entity->activity_timer = 0.0f;
    entity->wake_timer = 0.0f;
    entity->field_340 = 0.0f;
    entity->progress_344 = 0.0f;
    if ((entity->flags_3e4 & 4u) != 0) entity->link_2fc = 0;
    if (clear_mode != 0) {
        entity->activity = -1;
        entity->activity_param = 0;
        entity->activity_last = -1;
    }
    if (reset_mode != 0) {
        entity->last_action_event = 0;
        entity->last_action_event_flags = 0;
    }
}

void entity_table_maps_release(EntityTableMaps* maps)
{
    if (maps == NULL) return;
    free(maps->primary_a);
    free(maps->flags_a);
    free(maps->secondary_a);
    free(maps->primary_b);
    free(maps->flags_b);
    free(maps->secondary_b);
    memset(maps, 0, sizeof(*maps));
}

// FUNCTION: LEGORR 0x0041aa00
void entity_table_maps_allocate(EntityTableMaps* maps, unsigned int count_a,
                                unsigned int count_b)
{
    if (maps == NULL) return;
    entity_table_maps_release(maps);
    maps->primary_a = (uint32_t*)calloc(count_a, sizeof(uint32_t));
    maps->flags_a = (uint8_t*)calloc(count_a, sizeof(uint8_t));
    maps->secondary_a = (uint32_t*)calloc(count_a, sizeof(uint32_t));
    maps->primary_b = (uint32_t*)calloc(count_b, sizeof(uint32_t));
    maps->flags_b = (uint8_t*)calloc(count_b, sizeof(uint8_t));
    maps->secondary_b = (uint32_t*)calloc(count_b, sizeof(uint32_t));
    if ((count_a != 0 && (maps->primary_a == NULL || maps->flags_a == NULL ||
                          maps->secondary_a == NULL)) ||
        (count_b != 0 && (maps->primary_b == NULL || maps->flags_b == NULL ||
                          maps->secondary_b == NULL))) {
        entity_table_maps_release(maps);
        return;
    }
    maps->count_a = count_a;
    maps->count_b = count_b;
}
static EntityRouteStateHook s_entity_route_state_hook;

// GLOBAL: LEGORR 0x004e2684
static uint32_t s_task_counter_nonzero;
// GLOBAL: LEGORR 0x004e7184
static uint32_t s_task_counter_zero;

/* TaskCounterIncrementByBranch (FUN_004374d0). */
// FUNCTION: LEGORR 0x004374d0
void entity_task_counter_increment_by_branch(int nonzero)
{
    if (nonzero != 0) {
        ++s_task_counter_nonzero;
    } else {
        ++s_task_counter_zero;
    }
}

int entity_stride(int type)
{
    switch (type) {
    case ENTITY_TYPE_VEHICLE:  return 0x1ec; /* corpus 005561d4 stride */
    case ENTITY_TYPE_MINIFIG:  return 0x74;  /* corpus 005561d8 stride */
    case ENTITY_TYPE_OBJ:      return 0x74;  /* corpus 005561dc stride */
    case ENTITY_TYPE_BUILDING: return 0x14c; /* corpus 005561e0 stride */
    default:
        /* Codes 5..19 are the special-object kinds (FUN_0042e8b0); the
         * corpus keeps one singleton slot each (DAT_00556368..b0), which the
         * port groups under the shared special table with the object
         * stride. */
        return (type >= 5 && type <= 19) ? 0x74 : 0;
    }
}

/* Table index for a corpus type code (FUN_0042e8b0):
 * 1..4 map to the four typed tables, 5..19 map to the special table. */
static int entity_tab(int type)
{
    return (type >= 1 && type <= ENTITY_TYPES - 1) ? type - 1
                                                   : ENTITY_TYPES - 1;
}

int entity_count(GameContext* ctx, int type)
{
    if (ctx == NULL || type < 1 || type > 19) {
        return 0;
    }
    return ctx->entity_count[entity_tab(type)];
}

/* EntityPointer's types 1..4 table lookup (FUN_0042ed70). The port adds
 * bounds/null guards and groups the original special singleton types into
 * one table, so only the regular typed-array branch is represented here. */
// FUNCTION: LEGORR 0x0042ed70
const EntitySlot* entity_at(GameContext* ctx, int type, int index)
{
    if (ctx == NULL || type < 1 || type > 19) {
        return NULL;
    }
    int tab = entity_tab(type);
    if (index < 0 || index >= ctx->entity_count[tab]) {
        return NULL;
    }
    return &ctx->entity[tab][index];
}

/* EntityPoolFindByCallback (FUN_004377d0): the corpus walks each allocated
 * entity chunk, accepts only self-linked live records, optionally filters
 * the +0x3e8 0x80000 bit, and returns on the first callback match. The port's
 * typed arrays and entity_count values are its live-record registry. */
// FUNCTION: LEGORR 0x004377D0
int entity_pool_find_by_callback(GameContext* ctx,
                                 int (*callback)(EntitySlot*, void*),
                                 void* payload, int filter_flagged)
{
    if (ctx == NULL || callback == NULL) {
        return 0;
    }
    for (int table = 0; table < ENTITY_TYPES; ++table) {
        int count = ctx->entity_count[table];
        if (count < 0) {
            count = 0;
        }
        if (count > ENTITY_CAP) {
            count = ENTITY_CAP;
        }
        for (int i = 0; i < count; ++i) {
            EntitySlot* entity = &ctx->entity[table][i];
            if (entity->type == 0) {
                continue; /* corpus +0x408 self-link liveness marker */
            }
            if (filter_flagged != 0 && (entity->flags_3e8 & 0x80000u) != 0u) {
                continue;
            }
            if (callback(entity, payload) != 0) {
                return 1;
            }
        }
    }
    return 0;
}

/* EntityPoolFindByCallbackFiltered (FUN_004377b0) fixes the filter on. */
// FUNCTION: LEGORR 0x004377B0
int entity_pool_find_by_callback_filtered(GameContext* ctx,
                                          int (*callback)(EntitySlot*, void*),
                                          void* payload)
{
    return entity_pool_find_by_callback(ctx, callback, payload, 1);
}

static GameContext* s_entity_pool_callback_context;
static EntityCallbackDispatchHook s_entity_callback_dispatch_hook;
static void* s_entity_callback_dispatch_context;
static EntityActionPrerequisiteHooks s_entity_action_prerequisite_hooks;
static EntityRouteMarkerCandidateHook s_entity_route_marker_candidate_hook;
static void* s_entity_route_marker_candidate_context;
static EntityKindRecordUpdateHooks s_entity_kind_record_update_hooks;
static EntitySpawnSpecialFinalizeHook s_entity_spawn_special_finalize_hook;
static void* s_entity_spawn_special_finalize_context;
static GridObjectRecord s_grid_object_records[200];
static unsigned int s_grid_object_record_count;
static uint32_t s_grid_object_record_words[200];
static GridObjectRecordsReleaseHook s_grid_object_records_release_tail;
static void* s_grid_object_records_release_context;
static QueuedGridAnimationHooks s_queued_grid_animation_hooks;
static float s_queued_grid_animation_fraction;
static RoverWorkQueueHooks s_rover_work_queue_hooks;
static BuildingUpgradeAdmissionHooks s_building_upgrade_admission_hooks;
static BuildingUpgradeCommitHooks s_building_upgrade_commit_hooks;
static DeferredOListEntityRecord s_deferred_olist_records[200];
static unsigned int s_deferred_olist_record_count;
static DeferredOListEntityHooks s_deferred_olist_hooks;
static PeriodicEntityCallback s_periodic_entity_callback;
static void* s_periodic_entity_callback_context;
static EntityDefaultTransformApplyHook s_default_transform_apply_hook;
static void* s_default_transform_apply_context;

#define ENTITY_INDEXED_VALUE_GROUPS 11
#define ENTITY_INDEXED_VALUE_CAPACITY 3
static uint32_t s_entity_indexed_values[ENTITY_INDEXED_VALUE_GROUPS]
                                      [ENTITY_INDEXED_VALUE_CAPACITY];
static uint32_t s_entity_indexed_value_count[ENTITY_INDEXED_VALUE_GROUPS];
#define ENTITY_INDEXED_RECORD_TABLES 320
struct EntityIndexedRecordTable {
    const void* const* records;
    uint32_t count;
};
static EntityIndexedRecordTable
    s_entity_indexed_record_tables[ENTITY_INDEXED_RECORD_TABLES];

void entity_pool_callback_context_set(GameContext* ctx)
{
    s_entity_pool_callback_context = ctx;
}

GameContext* entity_pool_callback_context_get(void)
{
    return s_entity_pool_callback_context;
}

void entity_callback_dispatch_hook_set(EntityCallbackDispatchHook hook,
                                       void* context)
{
    s_entity_callback_dispatch_hook = hook;
    s_entity_callback_dispatch_context = context;
}

/* EntityCallbackDispatchUnlessTypeFive (FUN_004374f0): type five is a
 * non-dispatchable pooled record; all other live records enter the callback. */
void entity_callback_dispatch_unless_type_five(EntitySlot* entity)
{
    if (entity != NULL && entity->type != 5 &&
        s_entity_callback_dispatch_hook != NULL) {
        s_entity_callback_dispatch_hook(entity,
                                        s_entity_callback_dispatch_context);
    }
}

void entity_teardown_hooks_set(const EntityTeardownHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_entity_teardown_hooks, 0, sizeof(s_entity_teardown_hooks));
    }
    else {
        s_entity_teardown_hooks = *hooks;
    }
}

/* EntityTeardown (FUN_00437540). The legacy routine's engine calls are
 * represented by typed stage callbacks; record flags, command-chain cleanup,
 * special back-reference clearing, and free-list ownership stay local. */
// FUNCTION: LEGORR 0x00437540
int entity_teardown(EntitySlot* entity)
{
    uint32_t flags;
    uint32_t state_flags;
    int type;
    if (entity == NULL) return 0;
    type = entity->type;
    flags = (uint32_t)entity->flags_3ec;
    state_flags = s_entity_teardown_hooks.state_flags_get != NULL ?
        s_entity_teardown_hooks.state_flags_get(entity,
            s_entity_teardown_hooks.context) : 0;
    if (s_entity_teardown_hooks.counter_increment != NULL) {
        unsigned int slot = ((unsigned int)entity->etype +
                             (unsigned int)type * 15U) * 16U;
        s_entity_teardown_hooks.counter_increment(slot,
            s_entity_teardown_hooks.context);
    }
    if ((flags & 0x100000U) != 0 &&
        s_entity_teardown_hooks.resource_release != NULL) {
        void* resource = s_entity_teardown_hooks.resource_get != NULL ?
            s_entity_teardown_hooks.resource_get(entity, 0xd4u,
                s_entity_teardown_hooks.context) : NULL;
        s_entity_teardown_hooks.resource_release(resource,
            s_entity_teardown_hooks.context);
    }
    if ((flags & 0x80000U) != 0 &&
        s_entity_teardown_hooks.resource_release != NULL) {
        void* resource = s_entity_teardown_hooks.resource_get != NULL ?
            s_entity_teardown_hooks.resource_get(entity, 0xd5u,
                s_entity_teardown_hooks.context) : NULL;
        s_entity_teardown_hooks.resource_release(resource,
            s_entity_teardown_hooks.context);
    }
    if (s_entity_teardown_hooks.operation != NULL)
        s_entity_teardown_hooks.operation(entity, ENTITY_TEARDOWN_CALLBACK,
            s_entity_teardown_hooks.context);
    if (type == 3 && s_entity_teardown_hooks.operation != NULL)
        s_entity_teardown_hooks.operation(entity,
            ENTITY_TEARDOWN_TYPE_THREE_PREPARE,
            s_entity_teardown_hooks.context);
    if ((flags & 0x40U) != 0) {
        entity->flags_3ec = (int32_t)(flags & ~0x40U);
        flags &= ~0x40U;
    }
    if ((flags & 0x80U) != 0) {
        entity->route_partner_300 = NULL;
        entity->flags_3ec = (int32_t)(flags & ~0x80U);
        flags &= ~0x80U;
    }
    /* Remove the token from the scene before the release callback can return
     * that renderer record to its pool. Keep the entity token valid through
     * the release callback itself. */
    entity_ddiscene_scene_owner_bind(entity, NULL);
    if (s_entity_teardown_hooks.operation != NULL) {
        s_entity_teardown_hooks.operation(entity, ENTITY_TEARDOWN_DETACH_PARENT,
            s_entity_teardown_hooks.context);
        s_entity_teardown_hooks.operation(entity, ENTITY_TEARDOWN_RELEASE_SCENE,
            s_entity_teardown_hooks.context);
        s_entity_teardown_hooks.operation(entity, ENTITY_TEARDOWN_RELEASE_LINK,
            s_entity_teardown_hooks.context);
        s_entity_teardown_hooks.operation(entity, ENTITY_TEARDOWN_RESET_REFERENCE,
            s_entity_teardown_hooks.context);
        s_entity_teardown_hooks.operation(entity, ENTITY_TEARDOWN_RESET_ACTION,
            s_entity_teardown_hooks.context);
        s_entity_teardown_hooks.operation(entity, ENTITY_TEARDOWN_RELEASE_ACTIVITY,
            s_entity_teardown_hooks.context);
        s_entity_teardown_hooks.operation(entity, ENTITY_TEARDOWN_RELEASE_AI,
            s_entity_teardown_hooks.context);
    }
    entity_drain_command_chain(entity);
    entity_task_state_update_after_cleanup(entity, 0);
    if ((state_flags & 4U) != 0 || (state_flags & 2U) != 0) {
        if (s_entity_teardown_hooks.operation != NULL)
            s_entity_teardown_hooks.operation(entity,
                ENTITY_TEARDOWN_RELEASE_TYPE_DATA,
                s_entity_teardown_hooks.context);
    }
    if (s_entity_teardown_hooks.type_resource_release != NULL)
        s_entity_teardown_hooks.type_resource_release(entity, type,
            s_entity_teardown_hooks.context);
    if (s_entity_teardown_hooks.operation != NULL)
        s_entity_teardown_hooks.operation(entity, ENTITY_TEARDOWN_FINALIZE,
            s_entity_teardown_hooks.context);
    entity_linked_list_entries_remove_by_key(
        (uint32_t)(uintptr_t)entity);
    if (s_entity_teardown_hooks.free_list_link != NULL)
        s_entity_teardown_hooks.free_list_link(entity,
            s_entity_teardown_free_head,
            s_entity_teardown_hooks.context);
    free(entity->wp_dynamic);
    entity->wp_dynamic = NULL;
    entity->wp_capacity = 0;
    entity->wp_count = 0;
    entity->wp_index = 0;
    entity->ddiscene_owner_token = 0u;
    s_entity_teardown_free_head = entity;
    return 0;
}

void entity_command_status_feedback_hooks_set(
    const EntityCommandStatusFeedbackHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_entity_command_feedback_hooks, 0,
               sizeof(s_entity_command_feedback_hooks));
        s_entity_command_feedback_last_entity = NULL;
        s_entity_command_feedback_last_text[0] = '\0';
    }
    else s_entity_command_feedback_hooks = *hooks;
}

/* EntityCommandStatusFeedbackDispatch (FUN_00428020): suppresses the first
 * observed entity after a selection switch, clears stale empty command text,
 * and routes changed command/status text through the UI adapter. */
// FUNCTION: LEGORR 0x00428020
void entity_command_status_feedback_dispatch(EntitySlot* entity)
{
    const char* command;
    const char* status_text;
    char text[256];
    unsigned int i;
    int status;
    EntityCommandStatusFeedbackHooks* hooks =
        &s_entity_command_feedback_hooks;
    if (entity == NULL || hooks->command_text_get == NULL) return;
    command = hooks->command_text_get(entity, hooks->context);
    if (command == NULL) return;
    if (s_entity_command_feedback_last_entity != entity) {
        s_entity_command_feedback_last_entity = entity;
        return;
    }
    if (command[0] == '\0' && hooks->empty_command_clear != NULL) {
        hooks->empty_command_clear(entity, hooks->context);
        s_entity_command_feedback_last_text[0] = '\0';
        return;
    }
    if (command[0] == '\0') return;
    for (i = 0; i + 1 < sizeof(text) && command[i] != '\0'; ++i)
        text[i] = command[i];
    text[i] = '\0';
    status = hooks->status_get != NULL ?
        hooks->status_get(entity, hooks->context) : 0;
    status_text = hooks->status_text_get != NULL ?
        hooks->status_text_get(entity, status, hooks->context) : NULL;
    if (status_text != NULL && status_text[0] != '\0') {
        size_t j = strlen(text);
        if (j < sizeof(text) - 1) text[j++] = ' ';
        for (i = 0; j + 1 < sizeof(text) && status_text[i] != '\0'; ++i)
            text[j++] = status_text[i];
        text[j] = '\0';
    }
    if (strcmp(text, s_entity_command_feedback_last_text) == 0) return;
    strcpy(s_entity_command_feedback_last_text, text);
    if (hooks->feedback_dispatch != NULL)
        hooks->feedback_dispatch(entity, text, hooks->context);
}

void entity_action_prerequisite_hooks_set(
    const EntityActionPrerequisiteHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_entity_action_prerequisite_hooks, 0,
               sizeof(s_entity_action_prerequisite_hooks));
    }
    else {
        s_entity_action_prerequisite_hooks = *hooks;
    }
}

/* EntityActionKindPrerequisiteCheck (FUN_004388b0): require a type-one
 * entity and a valid action, then gate the two resource balances against
 * the corpus's packed primary/secondary action costs. */
// FUNCTION: LEGORR 0x004388B0
int entity_action_kind_prerequisite_check(EntitySlot* entity, int action)
{
    uint32_t values[3] = { 0u, 0u, 0u };
    uint32_t first_resource;
    uint32_t second_resource;
    uint32_t secondary_cost;
    EntityActionPrerequisiteHooks* const hooks =
        &s_entity_action_prerequisite_hooks;
    if (entity == NULL || entity->type != 1 ||
        hooks->action_ready == NULL || hooks->action_values_get == NULL ||
        hooks->resource_get == NULL ||
        !hooks->action_ready(entity, action, hooks->context) ||
        !hooks->action_values_get(entity, action, values, hooks->context)) {
        return 0;
    }
    first_resource = hooks->resource_get(0u, hooks->context);
    second_resource = hooks->resource_get(1u, hooks->context);
    secondary_cost = values[1] / 5u + (values[0] % 5u != 0u ? 1u : 0u);
    if (first_resource < values[2] && second_resource < secondary_cost) {
        return 0;
    }
    return 1;
}

typedef struct EntityRouteMarkerSearchRequest {
    const float* position;
    uint32_t flags;
    EntitySlot* match;
} EntityRouteMarkerSearchRequest;

static int entity_route_marker_candidate_visit(EntitySlot* candidate,
                                                void* payload)
{
    EntityRouteMarkerSearchRequest* request =
        (EntityRouteMarkerSearchRequest*)payload;
    float candidate_position[2];
    float dx;
    float dy;
    if (request == NULL || candidate == NULL) return 0;
    entity_transform_position_get(candidate, candidate_position);
    dx = candidate_position[0] - request->position[0];
    dy = candidate_position[1] - request->position[1];
    if (dx * dx + dy * dy > 100000000.0f) return 0;
    if (s_entity_route_marker_candidate_hook != NULL &&
        !s_entity_route_marker_candidate_hook(
            candidate, request->position, request->flags,
            s_entity_route_marker_candidate_context)) {
        return 0;
    }
    request->match = candidate;
    return 1;
}

void entity_route_marker_candidate_hook_set(
    EntityRouteMarkerCandidateHook hook, void* context)
{
    s_entity_route_marker_candidate_hook = hook;
    s_entity_route_marker_candidate_context = context;
}

/* EntityRouteMarkerCandidateFind (FUN_004389e0): find the first pooled
 * entity within the corpus's 10,000-unit radius that passes the optional
 * route-marker predicate. */
// FUNCTION: LEGORR 0x004389E0
EntitySlot* entity_route_marker_candidate_find(EntitySlot* source,
                                               int include_secondary)
{
    EntityRouteMarkerSearchRequest request;
    GameContext* context;
    float position[2];
    if (source == NULL) return NULL;
    entity_transform_position_get(source, position);
    request.match = NULL;
    request.flags = 0x1000u | (include_secondary != 0 ? 0x400u : 0u);
    request.position = position;
    context = entity_pool_callback_context_get();
    entity_pool_find_by_callback_filtered(
        context, entity_route_marker_candidate_visit, &request);
    return request.match;
}

void entity_kind_record_update_hooks_set(
    const EntityKindRecordUpdateHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_entity_kind_record_update_hooks, 0,
               sizeof(s_entity_kind_record_update_hooks));
    }
    else {
        s_entity_kind_record_update_hooks = *hooks;
    }
}

/* EntityKindRecordUpdateDispatch (FUN_00438460): preserve type-specific
 * update order, action transitions, event flags, and the two shared flag
 * updates while leaving engine-owned side effects behind typed callbacks. */
// FUNCTION: LEGORR 0x00438460
void entity_kind_record_update_dispatch(EntitySlot* entity)
{
    EntityKindRecordUpdateHooks* const hooks =
        &s_entity_kind_record_update_hooks;
    int type;
    if (entity == NULL) return;
    type = entity->type;
    if (type == 4) {
        if (hooks->type_four_update != NULL)
            hooks->type_four_update(entity, hooks->context);
        entity->flags_3e8 |= 0x20u;
        if ((entity->flags_3e0 & 8u) != 0u) {
            if (hooks->type_four_global_action != NULL)
                hooks->type_four_global_action(hooks->context);
        }
        else if ((entity->flags_3e0 & 2u) != 0u) {
            if (hooks->action_dispatch != NULL)
                hooks->action_dispatch(entity, 7, hooks->context);
        }
        else if ((entity->flags_3e0 & 4u) != 0u) {
            if (hooks->action_dispatch != NULL)
                hooks->action_dispatch(entity, 6, hooks->context);
        }
        else {
            entity->flags_3e8 &= ~0x20u;
        }
        if (hooks->global_refresh != NULL)
            hooks->global_refresh(hooks->context);
        entity->flags_3ec |= 0x200000u;
    }
    else if (type == 1 && hooks->type_one_record_update != NULL) {
        hooks->type_one_record_update(entity, hooks->context);
    }
    if (type == 1 || type == 2) {
        (void)entity_pool_find_with_local_predicate(entity->owner_ctx, entity);
    }
    if (hooks->event_emit != NULL) {
        hooks->event_emit(entity, type, entity->flags_3ec & 0x200u,
                          hooks->context);
    }
    if (type == 1 || type == 2) {
        scalar_task_record_queue(entity);
    }
    if (type == 4 && hooks->type_four_counter_increment != NULL) {
        hooks->type_four_counter_increment(hooks->context);
    }
    if ((type == 1 || type == 4)) entity->flags_3ec |= 0x40000u;
}

void grid_object_record_state_install(const GridObjectRecord* records,
                                      unsigned int count,
                                      GridObjectRecordsReleaseHook tail,
                                      void* context)
{
    unsigned int i;
    if (count > 200u) count = 200u;
    s_grid_object_record_count = 0u;
    if (records != NULL) {
        for (i = 0; i < count; ++i) {
            s_grid_object_records[i] = records[i];
        }
        s_grid_object_record_count = count;
    }
    s_grid_object_records_release_tail = tail;
    s_grid_object_records_release_context = context;
}

/* GridObjectRecordsRelease (FUN_004378e0): release both owned handles for
 * each active record, clear the fixed 200-word table, then release its side
 * structure through the installed owner callback. */
// FUNCTION: LEGORR 0x004378e0
void grid_object_records_release(void)
{
    unsigned int i;
    for (i = 0; i < s_grid_object_record_count; ++i) {
        if (s_grid_object_records[i].second != NULL)
            engine_heap_free(s_grid_object_records[i].second);
        if (s_grid_object_records[i].first != NULL)
            engine_heap_free(s_grid_object_records[i].first);
        memset(&s_grid_object_records[i], 0,
               sizeof(s_grid_object_records[i]));
    }
    s_grid_object_record_count = 0u;
    memset(s_grid_object_record_words, 0, sizeof(s_grid_object_record_words));
    if (s_grid_object_records_release_tail != NULL) {
        s_grid_object_records_release_tail(
            s_grid_object_records_release_context);
    }
}

void queued_grid_animation_hooks_set(const QueuedGridAnimationHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_queued_grid_animation_hooks, 0,
               sizeof(s_queued_grid_animation_hooks));
        s_queued_grid_animation_fraction = 0.0f;
    }
    else {
        s_queued_grid_animation_hooks = *hooks;
    }
}

void queued_grid_animation_accumulator_clear(void)
{
    s_queued_grid_animation_fraction = 0.0f;
}

/* ProcessQueuedGridAnimations (FUN_00437300): accumulate scaled fractional
 * work, then consume one queued type-six animation for each whole unit. */
void process_queued_grid_animations(void* anchor, int mode, float delta)
{
    QueuedGridAnimationHooks* const hooks = &s_queued_grid_animation_hooks;
    float position[2];
    float scaled_delta;
    if (hooks->anchor_position_get == NULL || hooks->entity_find == NULL ||
        hooks->animation_entity_get == NULL) return;
    if (mode == -1) {
        scaled_delta = delta;
    }
    else if (hooks->mode_scale_get != NULL) {
        scaled_delta = hooks->mode_scale_get(mode, hooks->context) * delta;
    }
    else {
        scaled_delta = 0.0f;
    }
    s_queued_grid_animation_fraction += scaled_delta;
    if (!hooks->anchor_position_get(anchor, position, hooks->context)) return;
    while (s_queued_grid_animation_fraction >= 1.0f) {
        EntitySlot* candidate = hooks->entity_find(position, 6, 0, 0,
                                                   hooks->context);
        EntitySlot* animation_entity;
        if (candidate == NULL) break;
        animation_entity = hooks->animation_entity_get(candidate, 6, 0, 0,
                                                       hooks->context);
        if (animation_entity == NULL) break;
        animation_entity->flags_3e8 |= 0x80000000u;
        if (hooks->work_queue_update != NULL) {
            hooks->work_queue_update(animation_entity, 0, hooks->context);
        }
        else {
            rover_work_queue_update(animation_entity, 0);
        }
        s_queued_grid_animation_fraction -= 1.0f;
    }
}

void rover_work_queue_hooks_set(const RoverWorkQueueHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_rover_work_queue_hooks, 0,
               sizeof(s_rover_work_queue_hooks));
    }
    else {
        s_rover_work_queue_hooks = *hooks;
    }
}

/* RoverWorkQueueUpdate (FUN_004383b0): apply the selected global work
 * position to every animation component in the rover's scene model. */
// FUNCTION: LEGORR 0x004383B0
void rover_work_queue_update(EntitySlot* entity, int mode)
{
    RoverWorkQueueHooks* const hooks = &s_rover_work_queue_hooks;
    void* model;
    float position[3];
    unsigned int count;
    unsigned int i;
    if (entity == NULL || entity->type != 6 || hooks->model_get == NULL ||
        hooks->animation_count_get == NULL || hooks->position_get == NULL ||
        hooks->animation_position_set == NULL ||
        hooks->animation_update == NULL) return;
    model = hooks->model_get(entity, hooks->context);
    if (model == NULL) return;
    hooks->position_get(mode, position, hooks->context);
    count = hooks->animation_count_get(model, hooks->context);
    if (count > 4096u) count = 4096u;
    for (i = 0; i < count; ++i) {
        hooks->animation_position_set(model, i, position, 1.0f,
                                     hooks->context);
        hooks->animation_update(model, i, position, hooks->context);
    }
}

void entity_indexed_value_table_clear(void)
{
    memset(s_entity_indexed_values, 0, sizeof(s_entity_indexed_values));
    memset(s_entity_indexed_value_count, 0,
           sizeof(s_entity_indexed_value_count));
}

/* IndexedValueAppend (FUN_00469a90): append to one of the eleven
 * three-value groups; the count dword occupies the fourth word of each
 * 16-byte group in the reference table. */
// FUNCTION: LEGORR 0x00469A90
void entity_indexed_value_append(int group, uint32_t value)
{
    uint32_t count;
    if (group < 0 || group >= ENTITY_INDEXED_VALUE_GROUPS) {
        return;
    }
    count = s_entity_indexed_value_count[group];
    if (count < ENTITY_INDEXED_VALUE_CAPACITY) {
        s_entity_indexed_values[group][count] = value;
        s_entity_indexed_value_count[group] = count + 1u;
    }
}

/* GroupedCodeTableFindValueIndex (FUN_0046a120): scan the count-prefixed
 * fixed groups in order and return the first group containing the value. */
// FUNCTION: LEGORR 0x0046A120
int entity_grouped_code_table_find_value_index(uint32_t value,
                                               int* out_group)
{
    int group;
    for (group = 0; group < ENTITY_INDEXED_VALUE_GROUPS; ++group) {
        uint32_t i;
        for (i = 0; i < s_entity_indexed_value_count[group]; ++i) {
            if (s_entity_indexed_values[group][i] == value) {
                if (out_group != NULL) *out_group = group;
                return 1;
            }
        }
    }
    return 0;
}

void entity_indexed_record_table_set(int group, const void* const* records,
                                     uint32_t count)
{
    if (group < 0 || group >= ENTITY_INDEXED_RECORD_TABLES) {
        return;
    }
    s_entity_indexed_record_tables[group].records = records;
    s_entity_indexed_record_tables[group].count = count;
}

static uint32_t entity_indexed_record_word_get(int group, int index,
                                               int slot, uint32_t offset)
{
    const EntityIndexedRecordTable* table;
    const uint8_t* record;
    uint32_t value = 0u;
    if (group < 0 || group >= ENTITY_INDEXED_RECORD_TABLES || index < 0 ||
        slot < 0 || offset > 0x14cu) {
        return 0u;
    }
    table = &s_entity_indexed_record_tables[group];
    if (table->records == NULL || (uint32_t)index >= table->count) {
        return 0u;
    }
    record = (const uint8_t*)table->records[index];
    if (record == NULL) {
        return 0u;
    }
    record += (uint32_t)slot * 0x150u + offset;
    memcpy(&value, record, sizeof(value));
    return value;
}

/* IndexedRecordWordAtOffsetB0Get (FUN_00469ac0) reads record +0xb0 at a
 * 0x150-byte stride from an installed group/index table. */
// FUNCTION: LEGORR 0x00469AC0
uint32_t entity_indexed_record_word_at_b0_get(int group, int index, int slot)
{
    return entity_indexed_record_word_get(group, index, slot, 0xb0u);
}

/* IndexedRecordWordAtOffsetB4Get (FUN_00469af0) is the adjacent dword
 * reader used by EntityKindOneRandomSpawnTrigger. */
uint32_t entity_indexed_record_word_at_b4_get(int group, int index, int slot)
{
    return entity_indexed_record_word_get(group, index, slot, 0xb4u);
}

/* EntityIndexedRecordWordAtOffsetF0Get (FUN_00469be0): the per-entity
 * record selector used to choose an attached scene-resource slot. */
uint32_t entity_indexed_record_word_at_f0_get(int group, int index, int slot)
{
    return entity_indexed_record_word_get(group, index, slot, 0xf0u);
}

/* MappedIndexedRecordWordAtFCGet (FUN_0046a0c0) is the corresponding
 * +0xfc accessor over the same table. */
// FUNCTION: LEGORR 0x0046A0C0
uint32_t entity_mapped_indexed_record_word_at_fc_get(int group, int index,
                                                     int slot)
{
    return entity_indexed_record_word_get(group, index, slot, 0xfcu);
}

/* FUNCTION: LEGORR 0x00406540
 * Pool storage is represented by the most recently active portable game
 * context; live slots are walked in the same callback-until-match style. */
int entity_pool_callback_search(int (*callback)(EntitySlot*, void*),
                                void* payload)
{
    return entity_pool_find_by_callback(s_entity_pool_callback_context,
                                        callback, payload, 0);
}

void periodic_entity_callback_install(PeriodicEntityCallback callback,
                                      void* context)
{
    s_periodic_entity_callback = callback;
    s_periodic_entity_callback_context = context;
}

static int periodic_entity_callback_adapter(EntitySlot* entity,
                                            void* payload)
{
    if (s_periodic_entity_callback == NULL) {
        return 0;
    }
    return s_periodic_entity_callback(entity, payload,
                                      s_periodic_entity_callback_context);
}

/* PeriodicCallbackDispatch (FUN_00425b40): run the installed periodic
 * callback over live entity records, skipping the corpus's +0x3e8 bit
 * 0x80000 flagged entries and stopping when a callback reports a match. */
// FUNCTION: LEGORR 0x00425B40
void periodic_entity_callback_dispatch(void* payload)
{
    entity_pool_find_by_callback_filtered(
        s_entity_pool_callback_context, periodic_entity_callback_adapter,
        payload);
}

void entity_default_transform_apply_hook_set(
    EntityDefaultTransformApplyHook hook, void* context)
{
    s_default_transform_apply_hook = hook;
    s_default_transform_apply_context = context;
}

// FUNCTION: LEGORR 0x004247d0
int entity_default_transform_apply_if_flagged(EntitySlot* entity,
                                              void* argument)
{
    if (entity != NULL && (entity->flags_3ec & 1u) != 0u &&
        s_default_transform_apply_hook != NULL) {
        s_default_transform_apply_hook(entity, argument, 1.0f, 0, 0,
                                       s_default_transform_apply_context);
    }
    return 0;
}

static int entity_default_transform_apply_adapter(EntitySlot* entity,
                                                  void* argument)
{
    (void)entity_default_transform_apply_if_flagged(entity, argument);
    return 0;
}

// FUNCTION: LEGORR 0x00424810
void entity_default_transform_apply_all(void* argument)
{
    entity_pool_callback_search(entity_default_transform_apply_adapter,
                                argument);
}

struct EntityGridFindQuery {
    int x;
    int y;
    EntitySlot* found;
};

static int entity_grid_position_match(EntitySlot* entity, void* payload)
{
    EntityGridFindQuery* const query = (EntityGridFindQuery*)payload;
    GameContext* const ctx = s_entity_pool_callback_context;
    if (entity == NULL || query == NULL || ctx == NULL || ctx->level == NULL)
        return 0;
    if (level_cell_of(entity->x) == query->x &&
        level_cell_of(entity->y) == query->y) {
        query->found = entity;
        return 1;
    }
    return 0;
}

/* EntityAtGridPositionLookup (FUN_00438670): the callback pool walk returns
 * the first eligible entity whose world position rounds to the requested
 * grid cell. The portable entity tables replace the corpus's raw chunk scan. */
// FUNCTION: LEGORR 0x00438670
EntitySlot* entity_at_grid_position_find(const int cell_xy[2])
{
    EntityGridFindQuery query;
    if (cell_xy == NULL || s_entity_pool_callback_context == NULL ||
        s_entity_pool_callback_context->level == NULL) {
        return NULL;
    }
    query.x = cell_xy[0];
    query.y = cell_xy[1];
    query.found = NULL;
    entity_pool_find_by_callback_filtered(s_entity_pool_callback_context,
                                          entity_grid_position_match, &query);
    return query.found;
}

static EntityActivityRecordRemoveFn s_activity_record_remove_hook;
static void* s_activity_record_remove_user;
static uint32_t s_activity_record_removal_flags;

void entity_activity_record_remove_hook_set(EntityActivityRecordRemoveFn hook,
                                            void* user)
{
    s_activity_record_remove_hook = hook;
    s_activity_record_remove_user = user;
}

void entity_activity_record_removal_suppression_set(uint32_t flags)
{
    s_activity_record_removal_flags = flags;
}

/* FUNCTION: LEGORR 0x00403c00. The raw record model retains the corpus
 * +0x2f0 head and +0x60 next-link offsets; opaque type-specific destruction
 * is delegated through the removal hook. */
void entity_activity_record_remove_by_type(void* entity, uint32_t type,
                                           void* action_context)
{
    if (entity == NULL || (s_activity_record_removal_flags & 2u) != 0u) return;
    void* current = NULL;
    void* previous = NULL;
    __builtin_memcpy(&current, (char*)entity + 0x2f0u, sizeof(current));
    while (current != NULL) {
        uint32_t record_type = 0;
        void* next = NULL;
        __builtin_memcpy(&record_type, current, sizeof(record_type));
        __builtin_memcpy(&next, (char*)current + 0x60u, sizeof(next));
        if (record_type == type) {
            if (previous == NULL) {
                __builtin_memcpy((char*)entity + 0x2f0u, &next,
                                 sizeof(next));
            } else {
                __builtin_memcpy((char*)previous + 0x60u, &next,
                                 sizeof(next));
            }
            if (s_activity_record_remove_hook != NULL) {
                s_activity_record_remove_hook(current, action_context,
                                              s_activity_record_remove_user);
            }
            return;
        }
        previous = current;
        current = next;
    }
}

/* FUNCTION: LEGORR 0x00403b60 */
void entity_activity_records_refresh(void* entity)
{
    if (entity == NULL) return;
    uint32_t flags = 0;
    uint32_t count = 0;
    __builtin_memcpy(&flags, (char*)entity + 0x3e0u, sizeof(flags));
    __builtin_memcpy(&count, (char*)entity + 0x324u, sizeof(count));
    if ((flags & 0x400u) == 0u || (flags & 0x2000u) != 0u) return;
    if (count > 256u) count = 256u;
    for (uint32_t i = 0; i < count; ++i) {
        void* value = NULL;
        __builtin_memcpy(&value, (char*)entity + 0x308u
                         + (size_t)i * sizeof(void*), sizeof(value));
        const uint32_t remove_types[] = { 5u, 6u, 9u, 15u, 7u, 4u, 20u };
        for (unsigned int j = 0; j < sizeof(remove_types) / sizeof(remove_types[0]); ++j) {
            entity_activity_record_remove_by_type(entity, remove_types[j],
                                                  j == 5u ? NULL : value);
        }
    }
}

static ActivityRecordClearFn s_activity_record_clear;
static ActivityRunnerChangeFn s_activity_runner_change;
static void* s_activity_runner_user;

void activity_runner_hooks_set(ActivityRecordClearFn clear_records,
                               ActivityRunnerChangeFn change, void* user)
{
    s_activity_record_clear = clear_records;
    s_activity_runner_change = change;
    s_activity_runner_user = user;
}

/* FUNCTION: LEGORR 0x00406e40 */
int activity_runner_update(void* runner, void* state, void* value)
{
    unsigned int i;
    if (runner == NULL) return 0;
    const size_t chain_offsets[3] = { 0x64u, 0x68u, 0x6cu };
    for (i = 0; i < 3u; ++i) {
        void* head = NULL;
        __builtin_memcpy(&head, (char*)runner + chain_offsets[i], sizeof(head));
        if (s_activity_record_clear != NULL) {
            s_activity_record_clear(head, s_activity_runner_user);
        }
    }
    const uint32_t zero = 0u;
    for (size_t offset = 0x40u; offset <= 0x50u; offset += 4u) {
        __builtin_memcpy((char*)runner + offset, &zero, sizeof(zero));
    }
    uint32_t count = 0u;
    __builtin_memcpy(&count, (char*)runner + 0x24u, sizeof(count));
    if (count > 64u) count = 64u;
    for (i = 0; i < count; ++i) {
        __builtin_memcpy((char*)runner + 0x54u + (size_t)i * 4u,
                         &zero, sizeof(zero));
    }
    void* scene_owner = NULL;
    __builtin_memcpy(&scene_owner, (char*)runner + 4u, sizeof(scene_owner));
    return s_activity_runner_change != NULL
        ? s_activity_runner_change(scene_owner, state, value,
                                   s_activity_runner_user) : 0;
}

static void** s_tracked_entity_slots;
static EntityTrackedCleanupFn s_tracked_cleanup_callback;
static void* s_tracked_cleanup_user;

void entity_tracked_cleanup_context_set(void** tracked_entities,
                                       EntityTrackedCleanupFn callback,
                                       void* user)
{
    s_tracked_entity_slots = tracked_entities;
    s_tracked_cleanup_callback = callback;
    s_tracked_cleanup_user = user;
}

/* FUNCTION: LEGORR 0x00409ff0. The three global pointer replacements and
 * teardown pair are exposed as operations because their owning registries
 * are not reconstructed in the entity module. */
void entity_destroy_tracked_pointer_cleanup(void* entity)
{
    if (entity == NULL || s_tracked_cleanup_callback == NULL) return;
    uint32_t flags = 0u;
    float position[3] = { 0.0f, 0.0f, 0.0f };
    __builtin_memcpy(&flags, (char*)entity + 0x148u, sizeof(flags));
    __builtin_memcpy(position, (char*)entity + 0x14u, sizeof(position));
    const uint32_t masks[3] = { 0x10u, 0x20u, 0x40u };
    for (int i = 0; i < 3; ++i) {
        if ((flags & masks[i]) != 0u && s_tracked_entity_slots != NULL
            && s_tracked_entity_slots[i] == entity) {
            s_tracked_entity_slots[i] =
                s_tracked_cleanup_callback(i, entity, flags, position,
                                           s_tracked_cleanup_user);
        }
    }
    s_tracked_cleanup_callback(3, entity, flags, position,
                               s_tracked_cleanup_user);
    s_tracked_cleanup_callback(4, entity, flags, position,
                               s_tracked_cleanup_user);
}

static EntityPairRecord* s_entity_pair_records;
static uint32_t s_entity_pair_record_count;
static EntityPairCountUpdateFn s_entity_pair_count_update;
static void* s_entity_pair_count_user;

void entity_pair_count_records_install(EntityPairRecord* records,
                                       uint32_t count,
                                       EntityPairCountUpdateFn update,
                                       void* user)
{
    s_entity_pair_records = records;
    s_entity_pair_record_count = count > 65536u ? 65536u : count;
    s_entity_pair_count_update = update;
    s_entity_pair_count_user = user;
}

/* FUNCTION: LEGORR 0x0041ec30 */
void entity_pair_count_refresh(void* first, void* second)
{
    uint32_t matches = 0u;
    for (uint32_t i = 0; s_entity_pair_records != NULL
         && i < s_entity_pair_record_count; ++i) {
        if (s_entity_pair_records[i].first == first
            && s_entity_pair_records[i].second == second) {
            ++matches;
        }
    }
    if (matches != 0u && s_entity_pair_count_update != NULL) {
        s_entity_pair_count_update(matches + 3u, s_entity_pair_count_user);
    }
}

static EntityActionTargetCellValidateFn s_action_target_cell_validator;
static void* s_action_target_cell_user;

void entity_action_target_cell_validator_set(
    EntityActionTargetCellValidateFn validator, void* user)
{
    s_action_target_cell_validator = validator;
    s_action_target_cell_user = user;
}

/* FUNCTION: LEGORR 0x0043fdc0 */
int entity_action_target_cell_is_valid(EntitySlot* entity)
{
    if (entity == NULL
        || (entity_capability_flags_get(entity) & 1u) == 0u
        || (entity->flags_3e0 & 0x400u) == 0u
        || entity->cmd_head == NULL || cmd_rec_type(entity->cmd_head) != 1) {
        return 0;
    }
    return s_action_target_cell_validator != NULL
        ? s_action_target_cell_validator(entity, s_action_target_cell_user) : 0;
}

static EntityExplicitRouteFn s_entity_explicit_route;
static void* s_entity_explicit_route_user;

void entity_explicit_route_hook_set(EntityExplicitRouteFn hook, void* user)
{
    s_entity_explicit_route = hook;
    s_entity_explicit_route_user = user;
}

/* FUNCTION: LEGORR 0x0043fe70 */
int entity_route_to_explicit_target(EntitySlot* actor, EntitySlot* target)
{
    if (actor == NULL || target == NULL
        || (entity_capability_flags_get(actor) & 2u) == 0u
        || (entity_capability_flags_get(target) & 1u) == 0u
        || actor->attach_304 != NULL) return 0;
    float x_values[AI_WAYPOINTS_MAX];
    float y_values[AI_WAYPOINTS_MAX];
    unsigned int count = 1u;
    x_values[0] = target->x;
    y_values[0] = target->y;
    if (s_entity_explicit_route != NULL
        && s_entity_explicit_route(actor, target, x_values, y_values,
                                   &count, s_entity_explicit_route_user) == 0) {
        return 0;
    }
    if (count == 0u || count > AI_WAYPOINTS_MAX
        || !entity_route_setup_and_marker_associate(actor, count,
                                                    x_values, y_values, 1)) {
        return 0;
    }
    actor->link_target = target;
    ENTITY_WAYPOINTS(actor)[count - 1u].pending = 0x15u;
    actor->flags_3ec |= 0x8000u;
    entity_activity_toggle(target, 1);
    return 1;
}

/* FUNCTION: LEGORR 0x0043fff0 */
int entity_route_marker_setup(EntitySlot* actor)
{
    if (actor == NULL || actor->link_target == NULL) return 0;
    EntitySlot* target = (EntitySlot*)actor->link_target;
    float x_values[AI_WAYPOINTS_MAX];
    float y_values[AI_WAYPOINTS_MAX];
    unsigned int count = 1u;
    x_values[0] = target->x;
    y_values[0] = target->y;
    if (s_entity_explicit_route != NULL
        && s_entity_explicit_route(actor, target, x_values, y_values,
                                   &count, s_entity_explicit_route_user) == 0) {
        return 0;
    }
    if (count == 0u || count > AI_WAYPOINTS_MAX
        || !entity_route_setup_and_marker_associate(actor, count,
                                                    x_values, y_values, 1)) {
        return 0;
    }
    actor->link_2fc = (uintptr_t)target;
    ENTITY_WAYPOINTS(actor)[count - 1u].pending = 0x13u;
    return 1;
}

struct EntityHurtSoundState {
    EntitySlot* entity;
    float timer;
    float duration;
};
static EntityHurtSoundState s_entity_hurt_sound_states[128];
static EntityHurtSoundAvailableFn s_entity_hurt_sound_available;
static EntityHurtSoundSubmitFn s_entity_hurt_sound_submit;
static EntityHurtSoundDurationFn s_entity_hurt_sound_duration;
static void* s_entity_hurt_sound_user;

void entity_hurt_sound_hooks_set(EntityHurtSoundAvailableFn available,
                                 EntityHurtSoundSubmitFn submit,
                                 EntityHurtSoundDurationFn duration,
                                 void* user)
{
    s_entity_hurt_sound_available = available;
    s_entity_hurt_sound_submit = submit;
    s_entity_hurt_sound_duration = duration;
    s_entity_hurt_sound_user = user;
}

static EntityHurtSoundState* entity_hurt_sound_state_get(EntitySlot* entity)
{
    int free_slot = -1;
    for (int i = 0; i < 128; ++i) {
        if (s_entity_hurt_sound_states[i].entity == entity) {
            return &s_entity_hurt_sound_states[i];
        }
        if (s_entity_hurt_sound_states[i].entity == NULL && free_slot < 0) {
            free_slot = i;
        }
    }
    if (free_slot < 0) return NULL;
    s_entity_hurt_sound_states[free_slot].entity = entity;
    s_entity_hurt_sound_states[free_slot].timer = 0.0f;
    s_entity_hurt_sound_states[free_slot].duration = 0.0f;
    return &s_entity_hurt_sound_states[free_slot];
}

/* FUNCTION: LEGORR 0x0043fb40 */
int entity_damage_hurt_sound_submit(EntitySlot* entity, float time,
                                    float damage)
{
    if (entity == NULL) return 1;
    if (entity->type != 2 || damage == 0.0f
        || s_entity_hurt_sound_available == NULL
        || s_entity_hurt_sound_available(s_entity_hurt_sound_user) == 0) {
        return 1;
    }
    EntityHurtSoundState* state = entity_hurt_sound_state_get(entity);
    if (state == NULL) return 1;
    if (state->timer == 0.0f) {
        if (s_entity_hurt_sound_submit != NULL) {
            s_entity_hurt_sound_submit(entity, s_entity_hurt_sound_user);
        }
        state->timer = time;
        state->duration = s_entity_hurt_sound_duration != NULL
            ? s_entity_hurt_sound_duration(s_entity_hurt_sound_user) : 0.0f;
        return 1;
    }
    state->timer += time * 0.04f;
    if (state->duration * 2.0f < state->timer) {
        state->timer = 0.0f;
        state->duration = 0.0f;
    }
    return 1;
}

static EntityLinkedEventSubmitFn s_entity_linked_event_submit;
static void* s_entity_linked_event_user;
static uint32_t s_entity_linked_event_global_flags;

void entity_linked_event_submit_hook_set(EntityLinkedEventSubmitFn hook,
                                         void* user)
{
    s_entity_linked_event_submit = hook;
    s_entity_linked_event_user = user;
}

/* FUNCTION: LEGORR 0x0043f5b0 */
void entity_linked_activity_event_dispatch(EntitySlot* entity,
                                           uint32_t events)
{
    if (entity == NULL || entity->type != 2) return;
    entity->flags_3f0 |= events;
    s_entity_linked_event_global_flags |= events;
    static const uint32_t masks[6] = { 1u, 2u, 4u, 8u, 0x10u, 0x20u };
    static const uint32_t codes[6] = { 0x17u, 0x18u, 0x14u, 0x15u,
                                       0x16u, 0x19u };
    for (int i = 0; i < 6; ++i) {
        if ((events & masks[i]) != 0u) {
            if (s_entity_linked_event_submit != NULL) {
                s_entity_linked_event_submit(codes[i], entity,
                                             s_entity_linked_event_user);
            }
            if (i != 5) break;
        }
    }
    entity->last_action_event_flags = 0u;
}

/* FUNCTION: LEGORR 0x0040f890 */
void direction_driven_heading_angle_step(void* state, const float* direction)
{
    if (state == NULL || direction == NULL) return;
    float error = direction[1];
    float current = 0.0f;
    float step = 0.0f;
    __builtin_memcpy(&current, (char*)state + 0x28u, sizeof(current));
    __builtin_memcpy(&step, (char*)state + 0x3cu, sizeof(step));
    if (error <= 1.0f) {
        if (error >= -1.0f) {
            current *= 0.95f;
        } else {
            current -= step;
            if (current < -0.8f) current = -0.8f;
        }
    } else {
        current += step;
        if (current > 0.8f) current = 0.8f;
    }
    __builtin_memcpy((char*)state + 0x28u, &current, sizeof(current));
}

/* FUNCTION: LEGORR 0x0040f920 */
void kind_two_steering_torque_apply(void* state, float direction[3])
{
    if (state == NULL || direction == NULL) return;
    float* object_axis = (float*)((char*)state + 0x18u);
    direction[1] = 0.0f;
    object_axis[1] = 0.0f;
    float axis_length = sqrtf(object_axis[0] * object_axis[0]
        + object_axis[1] * object_axis[1] + object_axis[2] * object_axis[2]);
    float inv_axis_length = 1.0f / axis_length;
    object_axis[0] *= inv_axis_length;
    object_axis[1] *= inv_axis_length;
    object_axis[2] *= inv_axis_length;
    float direction_length = sqrtf(direction[0] * direction[0]
        + direction[1] * direction[1] + direction[2] * direction[2]);
    float inv_direction_length = 1.0f / direction_length;
    float dx = direction[0] * inv_direction_length;
    float dy = direction[1] * inv_direction_length;
    float dz = direction[2] * inv_direction_length;
    direction[0] = dy * object_axis[2] - dz * object_axis[1];
    direction[1] = object_axis[0] * dz - dx * object_axis[2];
    direction[2] = dx * object_axis[1] - object_axis[0] * dy;
    float speed = 0.0f;
    float torque = 0.0f;
    __builtin_memcpy(&speed, (char*)state + 0x3cu, sizeof(speed));
    __builtin_memcpy(&torque, (char*)state + 0x30u, sizeof(torque));
    float correction = (1.0f - (object_axis[0] * dx
        + object_axis[2] * dz + object_axis[1] * dy)) * speed * 5.0f;
    if (direction[1] > 0.01f) {
        correction = 9.0f * torque + correction;
    } else if (direction[1] < 0.01f) {
        correction = 9.0f * torque - correction;
    }
    torque = correction * 0.1f;
    __builtin_memcpy((char*)state + 0x30u, &torque, sizeof(torque));
    float value = 0.0f;
    __builtin_memcpy(&value, (char*)state + 0x24u, sizeof(value));
    value += torque;
    __builtin_memcpy((char*)state + 0x24u, &value, sizeof(value));
    value = -9.0f * torque;
    __builtin_memcpy((char*)state + 0x2cu, &value, sizeof(value));
}

/* FUNCTION: LEGORR 0x0040fb90. D3DRM's matrix and random-vector helpers
 * are replaced by the existing portable math seams; the motion state uses
 * the corpus's documented float offsets. */
void steering_physics_step(void* unused, float* state, const float* delta)
{
    (void)unused;
    if (state == NULL || delta == NULL) return;
    float direction[3];
    direction[0] = state[0] - state[3];
    direction[1] = state[1] - state[4];
    direction[2] = state[2] - state[5];
    const float length = sqrtf(direction[0] * direction[0]
        + direction[1] * direction[1] + direction[2] * direction[2]);
    if (length < 0.00001f) d3drm_vector_random(direction);
    direction_driven_heading_angle_step(state, direction);
    kind_two_steering_torque_apply(state, direction);
    matrix_rotation_compose_xyz(state);
    state[6] = state[0x1e];
    state[7] = state[0x1f];
    state[8] = state[0x20];
    const float step = state[0x10] * delta[0];
    state[3] += state[6] * step;
    state[4] += state[7] * step;
    state[5] += state[8] * step;
    float translation[16];
    float previous[16];
    matrix4_identity_translation_set(translation, state + 3);
    __builtin_memcpy(previous, state + 0x16, sizeof(previous));
    matrix4_multiply(state + 0x16, translation, previous);
}

struct FlagMaskPredicateContext {
    uint32_t bit;
    EntityFlagMaskPredicate predicate;
    void* user;
    int all_pass;
};

static int flag_mask_predicate_callback(EntitySlot* entity, void* payload)
{
    FlagMaskPredicateContext* context = (FlagMaskPredicateContext*)payload;
    if (context->predicate != NULL
        && context->predicate(entity, context->bit, context->user) == 0) {
        context->all_pass = 0;
    }
    return 0; /* the corpus visits every matching entity for this mask bit */
}

static uint32_t s_entity_iteration_guard_flags;
static uint32_t s_scene_dispatch_point_low;
static uint32_t s_scene_dispatch_point_high;
static uint32_t s_scene_dispatch_word_c;
static uint32_t s_scene_dispatch_word14;

uint32_t entity_iteration_guard_flags_peek(void)
{
    return s_entity_iteration_guard_flags;
}

void scene_object_state_dispatch_words_install(uint32_t point_low,
                                               uint32_t point_high,
                                               uint32_t word_c,
                                               uint32_t word14)
{
    s_scene_dispatch_point_low = point_low;
    s_scene_dispatch_point_high = point_high;
    s_scene_dispatch_word_c = word_c;
    s_scene_dispatch_word14 = word14;
}

struct SceneStateDispatchContext {
    uint32_t ordinal;
    uint32_t mode;
    uint32_t flags;
    SceneObjectStateCallback callback;
    void* user;
};

static int scene_state_dispatch_callback(EntitySlot* entity, void* payload)
{
    SceneStateDispatchContext* context = (SceneStateDispatchContext*)payload;
    if (context->callback == NULL) {
        return 0;
    }
    return context->callback(entity, context->ordinal, context->mode,
                             context->flags, context->user);
}

/* SceneObjectStateDispatch (FUN_0046a510): compute the selector ordinal,
 * optionally allocate/link its 0x20-byte record, then visit filtered entities
 * with the state tuple. The per-caller LAB_0046a3c0 effect and four global
 * words are represented by explicit install/callback hooks. Native records
 * reserve four extra bytes so the widened pointer at +0x1c fits. */
// FUNCTION: LEGORR 0x0046A510
void scene_object_state_dispatch(GameContext* ctx, uint32_t selector,
                                 uint32_t mode, uint32_t flags,
                                 SceneObjectStateCallback callback,
                                 void* user)
{
    SceneStateDispatchContext context;
    context.ordinal = first_set_flag_ordinal(selector);
    context.mode = mode;
    context.flags = flags;
    context.callback = callback;
    context.user = user;
    if ((mode & 2u) != 0u) {
        unsigned char* record = (unsigned char*)engine_heap_alloc(0x24);
        if (record != NULL) {
            uint32_t zero = 0u;
            memset(record, 0, 0x24);
            linked_list_push_at_global_head(record);
            memcpy(record + 0x10, &zero, sizeof(zero));
            if ((flags & 2u) != 0u) {
                memcpy(record, &s_scene_dispatch_point_low, sizeof(uint32_t));
                memcpy(record + 4, &s_scene_dispatch_point_high, sizeof(uint32_t));
                memcpy(record + 0x0c, &s_scene_dispatch_word_c, sizeof(uint32_t));
            }
            if ((flags & 1u) != 0u) {
                memcpy(record + 0x14, &s_scene_dispatch_word14, sizeof(uint32_t));
            }
            memcpy(record + 0x18, &flags, sizeof(flags));
        }
    }
    if (ctx != NULL && callback != NULL) {
        entity_pool_find_by_callback_filtered(
            ctx, scene_state_dispatch_callback, &context);
    }
}

/* EntityIterationCallbackGuard (FUN_0043b320): raise guard bit 1, walk the
 * filtered entity pool through the caller-supplied port callback, then clear
 * the bit. The corpus's LAB_0043b350 body is unresolved; the callback is an
 * explicit seam instead of an invented per-entity effect. */
// FUNCTION: LEGORR 0x0043B320
void entity_iteration_callback_guard(GameContext* ctx,
                                     int (*callback)(EntitySlot*, void*),
                                     void* payload)
{
    s_entity_iteration_guard_flags |= 2u;
    if (ctx != NULL && callback != NULL) {
        entity_pool_find_by_callback_filtered(ctx, callback, payload);
    }
    s_entity_iteration_guard_flags &= ~2u;
}

/* FlagMaskCallbacksAllPass (FUN_0046a4c0): for each set bit 0..30, visit
 * filtered entities and fail if any callback clears the shared pass flag.
 * The caller-specific LAB_0046a490 callback is supplied by the port caller;
 * it is unresolved in the corpus and therefore has no fabricated default. */
// FUNCTION: LEGORR 0x0046A4C0
int entity_flag_mask_callbacks_all_pass(GameContext* ctx, uint32_t mask,
                                        EntityFlagMaskPredicate predicate,
                                        void* user)
{
    FlagMaskPredicateContext context;
    if (ctx == NULL) {
        return 0; /* deviation: the corpus's entity registry is global */
    }
    if (mask != 0u && predicate == NULL) {
        return 0; /* cannot infer the caller-specific LAB predicate */
    }
    for (uint32_t index = 0; index < 31u; ++index) {
        uint32_t bit = 1u << index;
        context.bit = bit;
        context.predicate = predicate;
        context.user = user;
        context.all_pass = 1;
        if ((mask & bit) != 0u) {
            entity_pool_find_by_callback_filtered(
                ctx, flag_mask_predicate_callback, &context);
            if (context.all_pass == 0) {
                return 0;
            }
        }
    }
    return 1;
}

typedef struct EntitySpawnRequest {
    int type;
    int kind;
} EntitySpawnRequest;

static EntitySpawnRequest s_entity_spawn_requests[9];
static uint32_t s_entity_spawn_request_count;

/* EntitySpawnRequestQueueAdd (FUN_0043f150): append the two request words
 * while the corpus's nine-slot queue has room. The first argument is an
 * unused context-shaped parameter in the original. */
// FUNCTION: LEGORR 0x0043f150
int entity_spawn_request_queue_add(GameContext* ctx, int type,
                                  int request_kind)
{
    (void)ctx;
    if (s_entity_spawn_request_count >= 9u) {
        return 0;
    }
    s_entity_spawn_requests[s_entity_spawn_request_count].type = type;
    s_entity_spawn_requests[s_entity_spawn_request_count].kind = request_kind;
    s_entity_spawn_request_count++;
    return 1;
}

/* EntitySpawnRequestQueueProcess (FUN_0043f190): the original performs a
 * multi-table stats/availability probe, creates the configured object,
 * and binds it to the requester. Those resource tables are not modeled,
 * so this rough port consumes the first type-valid request once the
 * existing shared eligibility gate passes, spawning at the requester's
 * pose and installing the observed reverse link/route-bit relationship. */
// FUNCTION: LEGORR 0x0043f190
void entity_spawn_request_queue_process(EntitySlot* requester)
{
    if (requester == NULL || requester->owner_ctx == NULL
        || s_entity_spawn_request_count == 0u
        || entity_shared_eligibility_check(requester, 0) == 0
        || (requester->flags_3ec & 0xc0u) != 0u) {
        return;
    }
    for (uint32_t i = 0; i < s_entity_spawn_request_count; ++i) {
        const int type = s_entity_spawn_requests[i].type;
        if (type < 1 || type > 19) {
            continue;
        }
        const int index = entity_create(requester->owner_ctx, type,
                                        "Spawned", requester->x,
                                        requester->y, requester->z,
                                        requester->rot);
        if (index < 0) {
            continue;
        }
        EntitySlot* spawned = (EntitySlot*)entity_at(requester->owner_ctx,
                                                      type, index);
        if (spawned != NULL) {
            spawned->link_2fc = (uintptr_t)requester;
            if ((requester->flags_3ec & 0x40u) != 0u) {
                requester->flags_3ec |= 0x40u;
                spawned->flags_3ec |= 0x40u;
            }
            else {
                requester->flags_3ec |= 0x80u;
                spawned->flags_3ec |= 0x80u;
            }
        }
        s_entity_spawn_request_count--;
        while (i < s_entity_spawn_request_count) {
            s_entity_spawn_requests[i] = s_entity_spawn_requests[i + 1u];
            ++i;
        }
        return;
    }
}

int entity_create(GameContext* ctx, int type, const char* name,
                  float x, float y, float z, float rot)
{
    if (ctx == NULL || type < 1 || type > 19) {
        return -1;
    }
    /* Codes 1..4 map to the four typed tables; the special kinds (5..19)
     * share the last table (the corpus's per-kind singletons). */
    int ti = entity_tab(type);
    int* cnt = &ctx->entity_count[ti];
    if (*cnt >= ENTITY_CAP) {
        return -1;
    }
    EntitySlot* slot = &ctx->entity[ti][*cnt];
    __builtin_memset(slot, 0, sizeof(*slot));
    slot->type = type;
    slot->etype = type;
    slot->activity = -1;       /* corpus +0x2e8 NULL pointer = no activity */
    slot->activity_last = -1;
    slot->owner_ctx = ctx;
    slot->ddiscene_owner_token = s_next_ddiscene_owner_token++;
    if (slot->ddiscene_owner_token == 0u) {
        slot->ddiscene_owner_token = s_next_ddiscene_owner_token++;
    }
    slot->x = x;
    slot->y = y;
    slot->z = z;
    slot->rot = rot;
    /* Corpus spawn dispatch (FUN_00437d00): the type-2 minifigure arm
     * (at 0x437eb6) seeds its permitted-activity list with Activity_Stand
     * through the FUN_0044b370 appender — the first grant is value 0. */
    if (type == 2) {
        ai_activity_grant(ctx, slot, AI_ACT_STAND);
    }
    /* Corpus spawn dispatch default arm: the 0x438150 switch assigns the
     * minion task-state (+0x334 stats block) to the static-template types —
     * type 8 -> DAT_004a98f0, 9 -> DAT_004a9a40, 0xc..0xe and 0x10..0x13
     * -> DAT_004a9b90; every other type falls through to the instance
     * stats attacher (FUN_00469c10 at 0x43818a). The task state carries the
     * permitted-activity capacity (state +0xcc, FUN_0046a060) that the
     * appender reads per entity, so minion spawns set their own cap here
     * (the shipped templates are zero — the real capacity is filled from
     * config at runtime, TODO(corpus); the port defaults it to the span). */
    slot->task_state = ai_minion_state_for(type);
    if (slot->task_state != AI_TASK_STATE_NONE) {
        slot->activity_cap = AI_ACTIVITY_SLOTS;
    }
    if (name != NULL) {
        size_t copy_length = strlen(name);
        if (copy_length > 31u) copy_length = 31u;
        strncpy(slot->name, name, copy_length);
        slot->name[copy_length] = '\0';
    }
    return (*cnt)++;
}

uint32_t entity_ddiscene_owner_token_get(const EntitySlot* entity)
{
    return entity != NULL ? entity->ddiscene_owner_token : 0u;
}

/* EntityVisualLoadACT/general model creation ultimately supplies the scene
 * object to this seam. Keep the scene pointer as a port sidecar and store the
 * stable token in the renderer's legacy +0x20 owner word. */
void entity_ddiscene_scene_owner_bind(EntitySlot* entity, void* scene_object)
{
    if (entity == NULL) return;
    if (entity->attached_scene_object != NULL &&
        entity->attached_scene_object != scene_object) {
        scene_object_owner_set(entity->attached_scene_object, 0u);
    }
    entity->attached_scene_object = scene_object;
    if (scene_object != NULL) {
        scene_object_owner_set(scene_object, entity->ddiscene_owner_token);
    }
}

void entity_spawn_special_finalize_hook_set(
    EntitySpawnSpecialFinalizeHook hook, void* context)
{
    s_entity_spawn_special_finalize_hook = hook;
    s_entity_spawn_special_finalize_context = context;
}

/* EntityCreateWithPosition (FUN_00438610): create through the pooled entity
 * allocator, apply its world position at construction, then run the extra
 * mode setup used by pooled kinds six and seven. */
EntitySlot* entity_create_with_position(GameContext* context, int type,
                                        const char* name, int value,
                                        float x, float y, float z)
{
    int index = entity_create(context, type, name, x, y, z, 0.0f);
    const EntitySlot* created;
    if (index < 0) return NULL;
    created = entity_at(context, type, index);
    if (created == NULL) return NULL;
    if ((type == 6 || type == 7) &&
        s_entity_spawn_special_finalize_hook != NULL) {
        s_entity_spawn_special_finalize_hook((EntitySlot*)created, value,
                                             s_entity_spawn_special_finalize_context);
    }
    return (EntitySlot*)created;
}

void building_upgrade_admission_hooks_set(
    const BuildingUpgradeAdmissionHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_building_upgrade_admission_hooks, 0,
               sizeof(s_building_upgrade_admission_hooks));
    }
    else {
        s_building_upgrade_admission_hooks = *hooks;
    }
}

/* BuildingUpgradeResourceAdmissionCheck (FUN_00438770): reject the terminal
 * upgrade level, then apply the two-resource threshold and return the
 * secondary debit amount when that path is selected. */
// FUNCTION: LEGORR 0x00438770
int building_upgrade_resource_admission_check(EntitySlot* building,
                                               uint32_t* out_secondary_cost)
{
    BuildingUpgradeAdmissionHooks* const hooks =
        &s_building_upgrade_admission_hooks;
    uint32_t first_cost;
    uint32_t second_cost;
    if (out_secondary_cost != NULL) *out_secondary_cost = 0u;
    if (building == NULL || hooks->maximum_level_get == NULL ||
        hooks->current_level_get == NULL || hooks->cost_get == NULL ||
        hooks->resource_get == NULL) return 0;
    if (hooks->maximum_level_get(building, hooks->context) <=
        hooks->current_level_get(building, hooks->context) + 1u) {
        return 0;
    }
    first_cost = hooks->cost_get(0u, hooks->context);
    second_cost = hooks->cost_get(1u, hooks->context);
    if (hooks->resource_get(0u, hooks->context) < first_cost) {
        if (hooks->resource_get(1u, hooks->context) < second_cost) return 0;
        if (out_secondary_cost != NULL) *out_secondary_cost = second_cost;
        return 1;
    }
    return 1;
}

void building_upgrade_commit_hooks_set(
    const BuildingUpgradeCommitHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_building_upgrade_commit_hooks, 0,
               sizeof(s_building_upgrade_commit_hooks));
    }
    else {
        s_building_upgrade_commit_hooks = *hooks;
    }
}

/* BuildingUpgradeCommit (FUN_004387f0): spend the selected resource, clear
 * the pending request, advance the level in payload/stats, and dispatch the
 * upgrade's world effect. */
// FUNCTION: LEGORR 0x004387f0
void building_upgrade_commit(EntitySlot* building)
{
    BuildingUpgradeCommitHooks* const hooks =
        &s_building_upgrade_commit_hooks;
    uint32_t secondary_cost = 0u;
    unsigned int next_level;
    unsigned int resource;
    uint32_t amount;
    if (building == NULL || building->type != 4 ||
        s_building_upgrade_admission_hooks.current_level_get == NULL ||
        !building_upgrade_resource_admission_check(building,
                                                   &secondary_cost)) {
        return;
    }
    resource = secondary_cost != 0u ? 1u : 0u;
    amount = secondary_cost;
    if (amount == 0u && s_building_upgrade_admission_hooks.cost_get != NULL) {
        amount = s_building_upgrade_admission_hooks.cost_get(
            0u, s_building_upgrade_admission_hooks.context);
    }
    next_level = s_building_upgrade_admission_hooks.current_level_get(
        building, s_building_upgrade_admission_hooks.context) + 1u;
    if (hooks->resource_spend != NULL)
        hooks->resource_spend(building, resource, amount, hooks->context);
    building->activity_counter_3d8 = 0.0f;
    if (hooks->upgrade_request_clear != NULL)
        hooks->upgrade_request_clear(building, hooks->context);
    if (hooks->payload_level_set != NULL)
        hooks->payload_level_set(building, next_level, hooks->context);
    if (hooks->stats_level_set != NULL)
        hooks->stats_level_set(building, next_level, hooks->context);
    if (hooks->upgrade_effect_dispatch != NULL)
        hooks->upgrade_effect_dispatch(building, next_level, hooks->context);
}

void deferred_olist_entity_hooks_set(const DeferredOListEntityHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_deferred_olist_hooks, 0, sizeof(s_deferred_olist_hooks));
    }
    else {
        s_deferred_olist_hooks = *hooks;
    }
}

void deferred_olist_entity_queue_install(
    const DeferredOListEntityRecord* records, unsigned int count)
{
    if (count > 200u) count = 200u;
    s_deferred_olist_record_count = 0u;
    if (records != NULL) {
        unsigned int i;
        for (i = 0; i < count; ++i) s_deferred_olist_records[i] = records[i];
        s_deferred_olist_record_count = count;
    }
}

static int deferred_olist_string_equal(const char* first, const char* second)
{
    if (first == NULL || second == NULL) return 0;
    while (*first != '\0' && *second != '\0') {
        if (tolower((unsigned char)*first) !=
            tolower((unsigned char)*second)) return 0;
        ++first;
        ++second;
    }
    return *first == '\0' && *second == '\0';
}

/* DeferredOListEntityCreateAndLink (FUN_00437940): realize all queued object
 * records at a cell, initialize each created entity, resolve named links
 * after creation, then release the consumed queue records. */
// FUNCTION: LEGORR 0x00437940
void deferred_olist_entity_create_and_link(const int cell_xy[2])
{
    EntitySlot* created[200];
    DeferredOListEntityRecord active[200];
    DeferredOListEntityRecord pending[200];
    DeferredOListEntityRecord consumed[200];
    unsigned int created_count = 0u;
    unsigned int consumed_count = 0u;
    unsigned int write_index = 0u;
    unsigned int i;
    DeferredOListEntityHooks* const hooks = &s_deferred_olist_hooks;
    if (cell_xy == NULL) return;
    for (i = 0; i < s_deferred_olist_record_count; ++i)
        pending[i] = s_deferred_olist_records[i];
    for (i = 0; i < s_deferred_olist_record_count; ++i) {
        DeferredOListEntityRecord* record = &pending[i];
        if (record->cell_x == cell_xy[0] && record->cell_y == cell_xy[1]) {
            EntitySlot* entity = NULL;
            if (record->entity_type == 4 &&
                hooks->special_building_create != NULL) {
                entity = hooks->special_building_create(
                    record, (unsigned int)record->variant, hooks->context);
            }
            else if (hooks->entity_create != NULL) {
                entity = hooks->entity_create(record, hooks->context);
            }
            if (entity != NULL) {
                if (record->entity_type != 4) {
                    entity->x = record->x;
                    entity->y = record->y;
                    entity->z = record->z;
                }
                if ((record->entity_type == 6 || record->entity_type == 7 ||
                     record->entity_type == 8 || record->entity_type == 9) &&
                    hooks->control_initialize != NULL) {
                    hooks->control_initialize(entity, hooks->context);
                }
                entity->flags_3ec |= 0x200u;
                entity_kind_record_update_dispatch(entity);
                if (entity->type == 2 && hooks->type_two_event != NULL)
                    hooks->type_two_event(entity, hooks->context);
                if (entity->type == 11 &&
                    hooks->type_eleven_finalize != NULL) {
                    hooks->type_eleven_finalize(entity, hooks->context);
                }
                created[created_count] = entity;
                active[created_count] = *record;
                ++created_count;
            }
            consumed[consumed_count++] = *record;
            continue;
        }
        s_deferred_olist_records[write_index] = *record;
        ++write_index;
    }
    s_deferred_olist_record_count = write_index;
    for (i = 0; i < created_count; ++i) {
        unsigned int j;
        const char* target_name = active[i].link_name;
        if (target_name == NULL || target_name[0] == '\0') continue;
        for (j = 0; j < created_count; ++j) {
            unsigned int mode;
            EntitySlot* target;
            if (i == j || !deferred_olist_string_equal(
                    target_name, active[j].name)) continue;
            target = created[j];
            created[i]->route_partner_300 = target;
            created[i]->flags_3e4 |= 4u;
            if ((target->flags_3e0 & 0x100u) != 0u) mode = 1u;
            else if ((target->flags_3e0 & 0x1000000u) == 0u) mode = 4u;
            else if ((target->flags_3e0 & 0x2000000u) == 0u) mode = 2u;
            else mode = 1u;
            if (hooks->link_finalize != NULL) {
                hooks->link_finalize(created[i], target, mode,
                                     hooks->context);
            }
            break;
        }
    }
    if (hooks->record_release != NULL) {
        for (i = 0; i < consumed_count; ++i)
            hooks->record_release(&consumed[i], hooks->context);
    }
}

/* Parse an optional child value under "<root> <obj> <key>". */
static const char* obj_val(BlockFile* bf, const char* root, const char* obj,
                           const char* key)
{
    char path[512];
    snprintf(path, sizeof(path), "%s %s %s", root, obj, key);
    return block_get_strptr(bf, path);
}

/* --- model cache (finding 30) --------------------------------------------
 * One parsed LWO model per resource path, process-lifetime for the demo —
 * models are never freed (documented; the renderer only reads them). */
#define MODEL_CACHE_MAX 16
static struct {
    const char* path;
    const void* model;
} s_model_cache[MODEL_CACHE_MAX];
static int s_model_cache_n = 0;

static char* model_dup(const char* s)
{
#if defined(_MSC_VER)
    return _strdup(s);
#else
    size_t n = strlen(s);
    char* d = (char*)malloc(n + 1);
    if (d != NULL) {
        memcpy(d, s, n);
        d[n] = '\0';
    }
    return d;
#endif
}

static const void* model_for_path(const char* path)
{
    if (path == NULL || path[0] == '\0') {
        return NULL;
    }
    for (int i = 0; i < s_model_cache_n; ++i) {
        if (s_model_cache[i].path != NULL
            && strcmp(s_model_cache[i].path, path) == 0) {
            return s_model_cache[i].model;
        }
    }
    const void* m = NULL;
    if (s_model_cache_n < MODEL_CACHE_MAX) {
        m = model_open(path);
        char* kept = m != NULL ? model_dup(path) : NULL;
        if (kept != NULL) {
            s_model_cache[s_model_cache_n].path = kept;
            s_model_cache[s_model_cache_n].model = m;
            ++s_model_cache_n;
        }
    }
    return m;
}

void entity_set_model(EntitySlot* e, const char* path)
{
    if (e == NULL) {
        return;
    }
    e->model_path[0] = '\0';
    e->model = NULL;
    if (path != NULL && path[0] != '\0') {
        size_t path_length = strlen(path);
        if (path_length > 63u) path_length = 63u;
        strncpy(e->model_path, path, path_length);
        e->model_path[path_length] = '\0';
        e->model = model_for_path(path);
    }
}

const void* entity_model(const EntitySlot* e)
{
    return e != NULL ? e->model : NULL;
}

/* --- entity record accessors (finding 457) --------------------------------
 * Every corpus reader resolves the entity's selected stats record and reads
 * a fixed field. The port mirrors the record on the slot (NULL = the zero
 * template, the Stats config loader being unported — see entity.hpp).
 * Names are offset-preserving: the decompile proves the offsets, not the
 * field semantics. */

static const uint8_t s_stats_zero_record[ENTITY_STATS_RECORD_SIZE] = { 0 };

static const uint8_t* stats_rec(const EntitySlot* e)
{
    const uint8_t* rec = (const uint8_t*)e->stats_record;
    return rec != NULL ? rec : s_stats_zero_record;
}

static float stats_rec_float(const EntitySlot* e, int off)
{
    float f = 0.0f;
    __builtin_memcpy(&f, stats_rec(e) + off, sizeof(f));
    return f;
}

static uint32_t stats_rec_word(const EntitySlot* e, int off)
{
    uint32_t w = 0;
    __builtin_memcpy(&w, stats_rec(e) + off, sizeof(w));
    return w;
}

// FUNCTION: LEGORR 0x00469cb0
float entity_stats_float_at20(const EntitySlot* e) { return stats_rec_float(e, 0x20); }
// FUNCTION: LEGORR 0x00469cc0
float entity_stats_float_at24_indexed(const EntitySlot* e, int index)
{
    return stats_rec_float(e, 0x24 + index * 4);
}
// FUNCTION: LEGORR 0x00469ce0
float entity_stats_float_atd4(const EntitySlot* e) { return stats_rec_float(e, 0xd4); }
// FUNCTION: LEGORR 0x00469d00
float entity_stats_float_at9c(const EntitySlot* e) { return stats_rec_float(e, 0x9c); }
// FUNCTION: LEGORR 0x00469d20
float entity_stats_float_atd8(const EntitySlot* e) { return stats_rec_float(e, 0xd8); }
// FUNCTION: LEGORR 0x00469d40
float entity_stats_float_at6c(const EntitySlot* e) { return stats_rec_float(e, 0x6c); }
// FUNCTION: LEGORR 0x00469d50
float entity_stats_float_at74(const EntitySlot* e) { return stats_rec_float(e, 0x74); }
// FUNCTION: LEGORR 0x00469d60
float entity_stats_float_at78(const EntitySlot* e) { return stats_rec_float(e, 0x78); }
// FUNCTION: LEGORR 0x00469d70
float entity_stats_float_ate8(const EntitySlot* e) { return stats_rec_float(e, 0xe8); }
// FUNCTION: LEGORR 0x00469d90
float entity_stats_float_at70(const EntitySlot* e) { return stats_rec_float(e, 0x70); }
// FUNCTION: LEGORR 0x00469da0
static void* entity_stats_field_at_a0_corpus(const EntitySlot* e)
{
    /* the corpus returns the ADDRESS record+0xa0, not its contents */
    return (void*)((const uint8_t*)e->stats_record + 0xa0);
}

void* entity_stats_field_at_a0(const EntitySlot* e)
{
    /* Portable callers may leave the stats record unset; keep their tested
     * zero-template address while the mapped leaf models a real record. */
    if (e->stats_record != NULL) {
        return entity_stats_field_at_a0_corpus(e);
    }
    return (void*)(stats_rec(e) + 0xa0);
}
// FUNCTION: LEGORR 0x00469db0
float entity_stats_float_at7c(const EntitySlot* e) { return stats_rec_float(e, 0x7c); }
// FUNCTION: LEGORR 0x00469dc0
float entity_stats_float_at80(const EntitySlot* e) { return stats_rec_float(e, 0x80); }
// FUNCTION: LEGORR 0x00469de0
float entity_energy_decay_rate_get(const EntitySlot* e) { return stats_rec_float(e, 0x84); }
// FUNCTION: LEGORR 0x00469e20
float entity_stats_float_atdc(const EntitySlot* e) { return stats_rec_float(e, 0xdc); }
// FUNCTION: LEGORR 0x00469e40
uint32_t entity_stats_word_atac(const EntitySlot* e) { return stats_rec_word(e, 0xac); }
uint32_t entity_stats_word_at_ec(const EntitySlot* e) { return stats_rec_word(e, 0xec); }
// FUNCTION: LEGORR 0x00469e60
uint32_t entity_stats_word_get(const EntitySlot* e) { return stats_rec_word(e, 0x144); }
// FUNCTION: LEGORR 0x00469e80
uint32_t entity_state_flags_get(const EntitySlot* e) { return stats_rec_word(e, 0x148); }

void entity_state_override_hooks_set(
    EntityStateOverrideValue330ApplyHook value_330_apply,
    EntityStateOverrideRenameHook rename, void* context)
{
    s_entity_state_override_value_330_apply = value_330_apply;
    s_entity_state_override_rename = rename;
    s_entity_state_override_context = context;
}

int entity_state_override_stack_set(const EntityStateOverrideEntry* entries,
                                    uint32_t count)
{
    if (count > sizeof(s_entity_state_override_entries) /
                    sizeof(s_entity_state_override_entries[0]) ||
        (count != 0u && entries == NULL)) {
        return 0;
    }
    if (count != 0u) {
        memcpy(s_entity_state_override_entries, entries,
               count * sizeof(s_entity_state_override_entries[0]));
    }
    s_entity_state_override_count = count;
    return 1;
}

int entity_state_override_stack_push(const EntityStateOverrideEntry* entry)
{
    if (entry == NULL || s_entity_state_override_count >=
            sizeof(s_entity_state_override_entries) /
                sizeof(s_entity_state_override_entries[0])) {
        return 0;
    }
    s_entity_state_override_entries[s_entity_state_override_count++] = *entry;
    return 1;
}

/* PopEntityStateOverride (FUN_00459240): only type-2 entities consume the
 * global LIFO; restore +0x3f0 and the saved name, then apply +0x330. */
// FUNCTION: LEGORR 0x00459240
int entity_state_override_pop(EntitySlot* entity)
{
    EntityStateOverrideEntry entry;
    if (entity == NULL || entity->type != 2 ||
        s_entity_state_override_count == 0u) {
        return 0;
    }
    entry = s_entity_state_override_entries[--s_entity_state_override_count];
    entity->flags_3f0 = entry.flags_3f0;
    if (s_entity_state_override_rename != NULL) {
        s_entity_state_override_rename(entity, entry.name,
                                       s_entity_state_override_context);
    }
    else {
        size_t length = 0;
        while (length < sizeof(entry.name) && entry.name[length] != '\0') {
            entity->name[length] = entry.name[length];
            ++length;
        }
        entity->name[length] = '\0';
    }
    if (s_entity_state_override_value_330_apply != NULL) {
        s_entity_state_override_value_330_apply(
            entity, entry.value_330, s_entity_state_override_context);
    }
    else {
        entity->indexed_record_slot_330 = (int32_t)entry.value_330;
    }
    return 1;
}

/* EntityRecordFlag400ToggleAndRefresh (FUN_0046a330): toggle stats-record
 * flag 0x400 and run the kind-four post-create resolver. */
// FUNCTION: LEGORR 0x0046A330
void entity_record_flag400_toggle_and_refresh(EntitySlot* e)
{
    uint32_t* flags;
    if (e == NULL) return;
    flags = (uint32_t*)(stats_rec(e) + 0x148);
    *flags ^= 0x400u;
    entity_kind_four_post_create_resolve(e);
}
// FUNCTION: LEGORR 0x00469ea0
uint32_t entity_capability_flags_get(const EntitySlot* e) { return stats_rec_word(e, 0x14c); }
// FUNCTION: LEGORR 0x00469f20
float entity_stats_float_at88(const EntitySlot* e) { return stats_rec_float(e, 0x88); }
// FUNCTION: LEGORR 0x00469f90
uint32_t entity_stats_word_at_c0_or_c4(const EntitySlot* e, int selector)
{
    return stats_rec_word(e, selector != 0 ? 0xc4 : 0xc0);
}
// FUNCTION: LEGORR 0x00469fc0
uint32_t entity_stats_word_atc8(const EntitySlot* e) { return stats_rec_word(e, 0xc8); }
// FUNCTION: LEGORR 0x00469fe0
float entity_stats_float_at8c(const EntitySlot* e) { return stats_rec_float(e, 0x8c); }
// FUNCTION: LEGORR 0x0046a000
float entity_stats_float_at90(const EntitySlot* e) { return stats_rec_float(e, 0x90); }
// FUNCTION: LEGORR 0x0046a020
float entity_stats_float_at98(const EntitySlot* e) { return stats_rec_float(e, 0x98); }
// FUNCTION: LEGORR 0x0046a040
float entity_stats_float_at94(const EntitySlot* e) { return stats_rec_float(e, 0x94); }
// FUNCTION: LEGORR 0x0046a080
float entity_stats_float_atf4(const EntitySlot* e) { return stats_rec_float(e, 0xf4); }
// FUNCTION: LEGORR 0x0046a0a0
float entity_speed_component_get(const EntitySlot* e) { return stats_rec_float(e, 0xf8); }
// FUNCTION: LEGORR 0x0046a170
float entity_stats_float_at130(const EntitySlot* e) { return stats_rec_float(e, 0x130); }
// FUNCTION: LEGORR 0x0046a190
float entity_stats_float_at134(const EntitySlot* e) { return stats_rec_float(e, 0x134); }
// FUNCTION: LEGORR 0x0046a1b0
float entity_stats_float_at120(const EntitySlot* e) { return stats_rec_float(e, 0x120); }
// FUNCTION: LEGORR 0x0046a1d0
float entity_stats_float_at128(const EntitySlot* e) { return stats_rec_float(e, 0x128); }
// FUNCTION: LEGORR 0x0046a1f0
float entity_stats_float_at124(const EntitySlot* e) { return stats_rec_float(e, 0x124); }
// FUNCTION: LEGORR 0x0046a210
uint32_t entity_stats_word_at13c(const EntitySlot* e) { return stats_rec_word(e, 0x13c); }
// FUNCTION: LEGORR 0x0046a230
float entity_stats_float_at12c(const EntitySlot* e) { return stats_rec_float(e, 0x12c); }
// FUNCTION: LEGORR 0x0046a250
float entity_stats_float_at138(const EntitySlot* e) { return stats_rec_float(e, 0x138); }
// FUNCTION: LEGORR 0x0046a270
float entity_collision_radius_get(const EntitySlot* e) { return stats_rec_float(e, 0xe4); }
// FUNCTION: LEGORR 0x0046a290
float entity_stats_float_at114(const EntitySlot* e) { return stats_rec_float(e, 0x114); }
// FUNCTION: LEGORR 0x0046a2b0
float entity_stats_float_at118(const EntitySlot* e) { return stats_rec_float(e, 0x118); }
// FUNCTION: LEGORR 0x0046a2d0
float entity_stats_float_at11c(const EntitySlot* e) { return stats_rec_float(e, 0x11c); }
// FUNCTION: LEGORR 0x0046a2f0
float entity_stats_float_at110(const EntitySlot* e) { return stats_rec_float(e, 0x110); }
// FUNCTION: LEGORR 0x0046a310
float entity_stats_float_at10c(const EntitySlot* e) { return stats_rec_float(e, 0x10c); }


/* `.ol` `type` name -> entity table code — DECOMPILE-VERIFIED (finding 07/10).
 *
 * The corpus resolver `FUN_0042e8b0` (ObjectTypeByName) matches the name
 * case-insensitively (__strcmpi) against the four runtime name registries —
 * populated from the shipped `Lego.cfg` `VehicleTypes` (FUN_0042cda0),
 * `RockMonsterTypes` (FUN_0042d100, the type-3 general/creature table),
 * `MiniFigureTypes` and `BuildingTypes` (parallel inits) — and then against
 * a hardcoded special-object table:
 *
 *   1 = VehicleTypes      (Hoverboard, SmallDigger, … 12)
 *   2 = MiniFigureTypes   (Pilot)
 *   3 = RockMonsterTypes  (RockMonster, TinyRM, TinyIM, SmallSpider, Bat,
 *                          Slug, LavaMonster, IceMonster)
 *   4 = BuildingTypes     (Toolstation, TeleportPad, … Geo-dome, … 10)
 *   5..19 = special objects (hardcoded in FUN_0042e8b0):
 *          Boulder=5, PowerCrystal=6, Ore=7(idx 0), ProcessedOre=7(idx 1),
 *          Dynamite=8, Barrier=9, ElectricFence=11, SpiderWeb=12, OohScary=13,
 *          Path=15, Pusher=16, Freezer=17, LaserShot=19
 *   -1 = TVCamera (the .ol loader's camera branch, no entity)
 *   0  = not registered (the corpus aborts the load; the port skips, see
 *        object_list_load_file)
 *
 * The special-entity codes are stored in the port's shared table index 4
 * (ENTITY_TYPE_SPECIAL); the corpus keeps one singleton slot each
 * (DAT_00556368..b0). */
typedef struct NameCode { const char* name; int code; } NameCode;

static const NameCode s_vehicle_names[] = {
    { "hoverboard", 1 }, { "smalldigger", 1 }, { "smalltruck", 1 },
    { "smallcat", 1 },   { "smallmlp", 1 },    { "smallheli", 1 },
    { "bulldozer", 1 },  { "walkerdigger", 1 }, { "largemlp", 1 },
    { "largedigger", 1 }, { "largecat", 1 },   { "largeheli", 1 },
};
static const NameCode s_minifig_names[] = {
    { "pilot", 2 },
};
static const NameCode s_creature_names[] = {
    { "rockmonster", 3 }, { "tinyrm", 3 }, { "tinyim", 3 },
    { "smallspider", 3 }, { "bat", 3 },    { "slug", 3 },
    { "lavamonster", 3 }, { "icemonster", 3 },
};
static const NameCode s_building_names[] = {
    { "toolstation", 4 }, { "teleportpad", 4 }, { "docks", 4 },
    { "powerstation", 4 }, { "barracks", 4 },   { "upgrade", 4 },
    { "geo-dome", 4 },    { "orerefinery", 4 }, { "gunstation", 4 },
    { "teleportbig", 4 },
};
static const NameCode s_special_names[] = {
    { "boulder", 5 },      { "powercrystal", 6 },  { "ore", 7 },
    { "processedore", 7 }, { "dynamite", 8 },      { "barrier", 9 },
    { "electricfence", 11 }, { "spiderweb", 12 },  { "oohscary", 13 },
    { "path", 15 },        { "pusher", 16 },       { "freezer", 17 },
    { "lasershot", 19 },
};

/* Resolver pair (FUN_0042e8b0 shape): answers known-1 with the corpus's
 * (type, registry-row) outputs. The corpus zeroes the row slot at entry
 * (`mov ebx,[esp+0x10]` / `mov [ebx],0` at 0x42e8b8, before the name load
 * and on every path, even the failed ones); registry matches then write
 * both, the special names write the dedicated ore-pair rows (0/1) only,
 * so everything else answers a zero row (the dispatch aliases that slot
 * onto its first argument's word — a corpus quirk, pinned in the tests). */
// FUNCTION: LEGORR 0x0042e8b0
static int entity_type_pair_for_name(const char* name, int* type_out,
                                     int* row_out, int* zero_out)
{
    /* The corpus's entry zeroing, blind (mov ebx,[esp+0x10] / mov [ebx],0
     * at 0x42e8b8): every in-tree caller threads a live row slot
     * (entity_type_for_name carries a sink; the wrapper's two callers pass
     * &barracks_row / &obj_y; the dispatch aliases its name slot). */
    *row_out = 0;
    if (name == NULL || name[0] == '\0') {
        return 0; /* unregistered — the corpus aborts, the port skips */
    }
    if (strcasecmp_lite(name, "tvcamera") == 0) {
        if (type_out != NULL) {
            *type_out = -1; /* follow-camera branch in FUN_0042e020 */
        }
        /* corpus: the special-name answer also zeroes the fourth word
         * (test a3 / store) — every port caller threads NULL. */
        if (zero_out != NULL) {
            *zero_out = 0;
        }
        return 1;
    }
    int code;
    int row = 0;
    size_t i;
    code = 0;
    for (i = 0; i < 13; ++i) {
        if (strcasecmp_lite(s_special_names[i].name, name) == 0) {
            code = s_special_names[i].code;
            row = (int)i;
            break;
        }
    }
    if (code != 0) {
        if (type_out != NULL) {
            *type_out = code;
        }
        if (row_out != NULL) {
            if (code == 7) {
                *row_out = row - 2; /* Ore and ProcessedOre occupy rows 2 and 3. */
            }
        }
        return 1;
    }
    for (i = 0; i < 12; ++i) {
        if (strcasecmp_lite(s_vehicle_names[i].name, name) == 0) {
            code = s_vehicle_names[i].code;
            row = (int)i;
            break;
        }
    }
    if (code != 0) {
        if (type_out != NULL) {
            *type_out = code;
        }
        if (row_out != NULL) {
            *row_out = row;
        }
        return 1;
    }
    for (i = 0; i < 1; ++i) {
        if (strcasecmp_lite(s_minifig_names[i].name, name) == 0) {
            code = s_minifig_names[i].code;
            row = (int)i;
            break;
        }
    }
    if (code != 0) {
        if (type_out != NULL) {
            *type_out = code;
        }
        if (row_out != NULL) {
            *row_out = row;
        }
        return 1;
    }
    for (i = 0; i < 8; ++i) {
        if (strcasecmp_lite(s_creature_names[i].name, name) == 0) {
            code = s_creature_names[i].code;
            row = (int)i;
            break;
        }
    }
    if (code != 0) {
        if (type_out != NULL) {
            *type_out = code;
        }
        if (row_out != NULL) {
            *row_out = row;
        }
        return 1;
    }
    for (i = 0; i < 10; ++i) {
        if (strcasecmp_lite(s_building_names[i].name, name) == 0) {
            code = s_building_names[i].code;
            row = (int)i;
            break;
        }
    }
    if (code != 0) {
        if (type_out != NULL) {
            *type_out = code;
        }
        if (row_out != NULL) {
            *row_out = row;
        }
        return 1;
    }
    return 0;
}

static int entity_type_for_name(const char* name)
{
    int type = 0;
    int row_sink = 0; /* the entry zeroing lands here (corpus-blind form) */
    if (!entity_type_pair_for_name(name, &type, &row_sink, NULL)) {
        return 0;
    }
    return type;
}

/* Public (type, row) shape of the resolver — the corpus call form with a
 * NULL fourth output (FUN_0042e8b0(name, &type, &row, 0)): the corpus's
 * entry zeroing clears the row slot first (guarded here), table matches
 * then write both, the special names write only their dedicated rows,
 * so everything else answers a zero row. */
int entity_type_row_for_name(const char* name, int* type_out, int* row_out)
{
    return entity_type_pair_for_name(name, type_out, row_out, NULL);
}

static void ol_strncpy(char* dst, size_t n, const char* src)
{
    if (n == 0) {
        dst[0] = '\0';
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, n - 1);
    dst[n - 1] = '\0';
}

int object_list_load(GameContext* ctx, BlockFile* cfg, const char* level_name)
{
    if (ctx == NULL || cfg == NULL || level_name == NULL) {
        return -1;
    }
    char olist[256] = { 0 };
    char path[512];
    if (util_level_cfg_path(cfg, level_name, "OListFile", path, sizeof(path))
        == NULL) {
        return -1; /* no object list for this level */
    }
    const char* v = block_get_strptr(cfg, path);
    if (v == NULL) {
        return -1; /* no object list for this level */
    }
    snprintf(olist, sizeof(olist), "%s", v);
    return object_list_load_file(ctx, olist);
}

// FUNCTION: LEGORR 0x0042e020
int object_list_load_file(GameContext* ctx, const char* olist)
{
    if (ctx == NULL || olist == NULL || olist[0] == '\0') {
        return -1;
    }

    BlockFile* ol = block_open(olist);
    if (ol == NULL) {
        return -1; /* missing/unparsable .ol resource */
    }

    int placed = 0;
    /* Root = the file's first record; objects are its children (document
     * order, like the entity list the corpus walks). */
    BlockRec* rootrec = block_record_at(ol, 0);
    const char* rname = rootrec ? block_name(rootrec) : NULL;
    if (rname == NULL) {
        block_close(ol);
        return -1;
    }
    for (BlockRec* rec = block_first_child(rootrec); rec != NULL;
         rec = block_next_sibling(rec)) {
        const char* name = block_name(rec);
        if (name == NULL) {
            continue;
        }
        ObjectPlacement op;
        __builtin_memset(&op, 0, sizeof(op));
        snprintf(op.name, sizeof(op.name), "%s", name);

        /* Shipped grammar (decompile-verified, FUN_0042e020): `type <name>`,
         * `xpos` / `ypos`, `heading`, optional `health` (default 100) and
         * `driving <ref>`. `Model` is the port's extension key (absent from
         * all 44 shipped lists). */
        const char* t = obj_val(ol, rname, name, "type");
        ol_strncpy(op.type_name, sizeof(op.type_name), t);
        op.type = entity_type_for_name(op.type_name);
        op.x = (float)atof(obj_val(ol, rname, name, "xPos"));
        op.y = (float)atof(obj_val(ol, rname, name, "yPos"));
        op.heading = (float)atof(obj_val(ol, rname, name, "heading"));
        op.z = 0.0f;
        op.rot = op.heading;
        op.health = (int)atof(obj_val(ol, rname, name, "health"));
        ol_strncpy(op.driving, sizeof(op.driving),
                   obj_val(ol, rname, name, "driving"));
        ol_strncpy(op.model, sizeof(op.model), obj_val(ol, rname, name, "Model"));

        if (op.type == -1) {
            /* TVCamera — FUN_0042e020's type -1 branch drives the follow
             * camera instead of creating an entity (last one wins). */
            ctx->ol_camera_set = 1;
            ctx->ol_camera_x = op.x;
            ctx->ol_camera_y = op.y;
            ctx->ol_camera_heading = op.heading;
            ++placed;
            continue;
        }
        if (op.type == 0) {
            /* Unregistered type name — the corpus aborts the whole load
             * (FUN_0042e020 breaks and returns 0); the port skips the
             * record and keeps the rest (dev leftovers like Paul.ol's
             * Test1 would otherwise kill every load). */
            continue;
        }

        int idx = entity_create(ctx, op.type, op.name, op.x, op.y, op.z,
                                op.heading);
        if (idx >= 0) {
            EntitySlot* slot = (EntitySlot*)entity_at(ctx, op.type, idx);
            ol_strncpy(slot->type_name, sizeof(slot->type_name), op.type_name);
            ol_strncpy(slot->driving, sizeof(slot->driving), op.driving);
            slot->rot = op.heading;
            slot->health = op.health > 0 ? op.health : 100;
            entity_set_model(slot, op.model[0] != '\0' ? op.model : NULL);
            ++placed;
        }
    }
    block_close(ol);
    ctx->entity_placed = placed;
    return placed;
}

/* EntityFlagsMask107C33DEOr1C00203AnyTest (FUN_0043baf0, finding 319):
 * either flag word intersects its literal mask. */
// FUNCTION: LEGORR 0x0043baf0
int entity_flags_mask107c33de_or_1c00203_any_test(const EntitySlot* e)
{
    if ((e->flags_3e0 & 0x107c33deu) != 0u) {
        return 1;
    }
    return (e->flags_3e4 & 0x01c00203u) != 0u;
}

/* EntitySharedEligibilityCheck (FUN_004385b0, finding 471/472 context):
 * the command and activity layers' shared gate. Corpus order: +0x3e0 bits
 * 0x700100 clear, +0x3e4 bit 0x100000 clear, +0x340 float not negative,
 * +0x3e0 bit 0x20000 clear; a nonzero `relaxed` skips the +0x3e8 clause,
 * otherwise bit 0x20000000 must be set and bit 0x80000000 clear.
 * (+1000 decimal is +0x3e8 — the flags_3e8 mirror.) */

/* The corpus's eligibility gate compares the +0x340 bound against this
 * .rdata zero dword (objdump-verified fcomp operand of
 * EntitySharedEligibilityCheck); a named const static keeps the fcomp
 * operand resolvable by Reccmp. */
// GLOBAL: LEGORR 0x0049e3a4
static const float s_eligibility_floor = 0.0f;   /* DAT_0049e3a4, verified */

// FUNCTION: LEGORR 0x004385b0
int entity_shared_eligibility_check(const EntitySlot* e, int relaxed)
{
    if ((e->flags_3e0 & 0x00700100u) != 0u) {
        return 0;
    }
    if ((e->flags_3e4 & 0x00100000u) != 0u) {
        return 0;
    }
    if (!(e->field_340 >= s_eligibility_floor)) {
        return 0;
    }
    if ((e->flags_3e0 & 0x00020000u) != 0u) {
        return 0;
    }
    if (relaxed != 0) {
        return 1;
    }
    if ((e->flags_3e8 & 0x20000000u) != 0u
        && (e->flags_3e8 & 0x80000000u) == 0u) {
        return 1;
    }
    return 0;
}

/* EntityType3Flag04000000Test (FUN_004436f0, finding 318): kind 3 with
 * +0x3e8 bit 0x04000000. The corpus reads the flag word as
 * entity[0xfa] (dword-indexed +0x3e8). */
// FUNCTION: LEGORR 0x004436f0
int entity_type3_flag04000000_test(const EntitySlot* e)
{
    return e->type == 3 && (e->flags_3e8 & 0x04000000u) != 0u;
}

/* EntityAcceptsTargetForTaskCheck (FUN_00439bd0, finding 471). */
// FUNCTION: LEGORR 0x00439bd0
int entity_accepts_target_for_task_check(const EntitySlot* entity,
                                         const EntitySlot* target,
                                         int apply_extra_gate)
{
    uint32_t capabilities;
    if (entity == NULL || target == NULL) return 0;
    if (entity->type == 3) {
        if ((entity->flags_3e8 & 0x08000000u) == 0u) return 0;
    }
    else if ((entity->flags_3e8 & 0x20u) == 0u) {
        return 0;
    }
    if ((entity->flags_3e0 & 1u) != 0u) return 0;
    if (apply_extra_gate != 0
        && entity_flags_mask107c33de_or_1c00203_any_test(entity)) return 0;
    capabilities = entity_capability_flags_get(entity);
    if ((capabilities & 4u) != 0u && target->type != 6) return 0;
    if ((capabilities & 2u) != 0u && target->type != 7) return 0;
    if (entity->carry_state_324 != 0) {
        const EntitySlot* carried = (const EntitySlot*)entity->carry_link_308;
        if (carried == NULL || carried->type != target->type) return 0;
    }
    return 1;
}

/* GatedStepCommandIssueWithTargetRecord (FUN_00439a20): when the flag/health
 * gate and target gate pass, install a move command toward the target and
 * retain the target link. The original's bespoke action-record/path writer
 * is represented by the port's shared move-command path builder. */
// FUNCTION: LEGORR 0x00439A20
int entity_gated_step_command_issue(EntitySlot* entity, EntitySlot* target)
{
    if (entity == NULL || target == NULL
        || entity_flag400_and_health_threshold_check(entity) != 0
        || !entity_accepts_target_for_task_check(entity, target, 1)) {
        return 0;
    }
    cmd_create_move(entity, target->x, target->y, 0);
    entity->link_2fc = (uintptr_t)target;
    return 1;
}

// FUNCTION: LEGORR 0x00439b80
int entity_task_target_cell_eligibility_check(
    const EntitySlot* entity, const uint32_t* task_record)
{
    SpatialObjectQueryRequest request;
    float position[2];
    void* candidate;
    if (entity == NULL) return 0;
    if (entity->type != 1) return 1;
    if (task_record == NULL || task_record[1] != 0u) return 0;
    if ((entity_capability_flags_get(entity) & 0x200u) != 0u) return 1;

    /* The reference's 0x438ae0 call is the live mask-2 spatial query. Its
     * arguments are the actor, task_record[0], and task_record[0xcc]. */
    memset(&request, 0, sizeof(request));
    entity_transform_position_get(entity, position);
    request.center[0] = position[0];
    request.center[1] = position[1];
    request.radius = 10000.0f;
    request.mask = 2u;
    request.filters[0] = task_record[0];
    request.filters[1] = task_record[0xcc];
    candidate = spatial_object_query_execute(&request);
    return candidate != NULL;
}

void entity_pair_interaction_activity_sentinel_set(int activity)
{
    s_entity_pair_activity_sentinel = activity;
}

/* EntityPairInteractionGate (FUN_0043a9f0): a qualifying carrier pair
 * requests activity 0x24 once per current sentinel and raises the partner
 * interaction bit in the entity's +0x3e4 flags. */
// FUNCTION: LEGORR 0x0043A9F0
void entity_pair_interaction_gate(EntitySlot* entity,
                                  const EntitySlot* partner)
{
    const int* carried_kind;
    if (entity == NULL || partner == NULL) return;
    if ((entity->flags_3e0 & 0x400u) != 0u
        || (entity->flags_3e4 & 1u) != 0u
        || (entity->flags_3e0 & 0x01000100u) != 0u
        || entity->field_340 <= 0.0f
        || (partner->flags_3e0 & 0x400u) == 0u
        || partner->carry_link_308 == NULL) return;
    carried_kind = (const int*)partner->carry_link_308;
    if (*carried_kind != 6 || s_entity_pair_activity_sentinel
        == entity->activity) return;
    ai_set_activity(entity, 0x24, 0);
    entity->flags_3e4 |= 0x00200000u;
}

void entity_visual_resource_hooks_set(EntityVisualResourcePresentHook present,
                                      EntityVisualResourceCreateHook create,
                                      void* context)
{
    s_entity_visual_resource_present_hook = present;
    s_entity_visual_resource_create_hook = create;
    s_entity_visual_resource_context = context;
}

/* EntityVisualResourceGetOrCreate (FUN_0043a5f0): choose one of the four
 * type-specific cached visual handles and create the model when absent. */
// FUNCTION: LEGORR 0x0043a5f0
void entity_visual_resource_get_or_create(EntitySlot* entity)
{
    int slot;
    void* resource;
    if (entity == NULL || entity->type < 1 || entity->type > 4) return;
    slot = entity->type - 1;
    resource = entity->visual_resource_handles[slot];
    if (s_entity_visual_resource_present_hook != NULL
        && s_entity_visual_resource_present_hook(entity, resource,
                                                 s_entity_visual_resource_context)) {
        return;
    }
    if (s_entity_visual_resource_create_hook != NULL) {
        s_entity_visual_resource_create_hook(entity,
                                              s_entity_visual_resource_context);
    }
}

void entity_indexed_flag_publish_hook_set(EntityIndexedFlagPublishHook hook,
                                          void* context)
{
    s_entity_indexed_flag_publish_hook = hook;
    s_entity_indexed_flag_context = context;
}

/* EntityIndexedFlagSetAndRefresh (FUN_00439c80): type-1 records publish
 * their indexed bit, refresh the owning entity's flags, and raise event 0x19. */
// FUNCTION: LEGORR 0x00439C80
void entity_indexed_flag_set_and_refresh(EntitySlot* entity)
{
    uint32_t bit;
    if (entity == NULL || entity->type != 1) return;
    bit = 1u << (entity->indexed_record_flag_index & 0x1fu);
    entity->indexed_record_flags |= bit;
    if (s_entity_indexed_flag_publish_hook != NULL) {
        s_entity_indexed_flag_publish_hook(entity,
                                           entity->indexed_record_flags,
                                           0x19u,
                                           s_entity_indexed_flag_context);
    }
}

void entity_tool_name_load_hook_set(EntityToolNameLoadHook hook,
                                    void* context)
{
    s_entity_tool_name_load_hook = hook;
    s_entity_tool_name_context = context;
}

/* ToolNameStringResourcesLoad (FUN_00439950): load the eleven configured
 * entries from the ToolNames category into the global indexed handle table. */
// FUNCTION: LEGORR 0x00439950
void entity_tool_name_strings_load(void* string_loader, void* config)
{
    int slot;
    for (slot = 0; slot < 11; ++slot) {
        s_entity_tool_name_handles[slot] =
            s_entity_tool_name_load_hook != NULL
            ? s_entity_tool_name_load_hook(string_loader, config, "ToolNames",
                                           slot, s_entity_tool_name_context)
            : NULL;
    }
}

void* entity_tool_name_string_get(int slot)
{
    return slot >= 0 && slot < 11 ? s_entity_tool_name_handles[slot] : NULL;
}

/* ProximityActivityRaise (FUN_0043aab0, finding 471): activate activity
 * 0x24 when a type-3 entity is near a qualifying terrain cell. */
// FUNCTION: LEGORR 0x0043AAB0
void entity_proximity_activity_raise(EntitySlot* entity)
{
    const GameContext* context;
    const LevelState* level;
    float cell_xy[2];
    float block_size;
    float dx, dy;
    int x = 0, y = 0;
    if (entity == NULL || entity->type != 3
        || (entity_capability_flags_get(entity) & 0x4000u) == 0u
        || s_entity_pair_activity_sentinel == entity->activity
        || (entity->flags_3e0 & 0x400u) != 0u
        || !(entity_stats_float_ate8(entity) < entity->field_340)) return;
    context = entity->owner_ctx;
    level = context != NULL ? context->level : NULL;
    if (level == NULL || level->ptr_terrain == NULL) return;
    entity_position_to_terrain_cell(entity, level, &x, &y);
    if (!level_coord_pair_within_bounds(level, x, y)
        || !level_cell_occupied_at(level, x, y)
        || level_cell_flag_8000(level, x, y) != 0u
        || !terrain_cell_world_center_xy_get(level->ptr_terrain, x, y,
                                             cell_xy)) return;
    block_size = terrain_blocksize_get(level->ptr_terrain);
    dx = entity->x - cell_xy[0];
    dy = entity->y - cell_xy[1];
    if (sqrtf(dx * dx + dy * dy) < block_size * 0.33333334f) {
        ai_set_activity(entity, 0x24, 0);
        entity->flags_3e4 |= 0x00200000u;
    }
}

void entity_type_two_event_hook_set(EntityTypeTwoEventHook hook,
                                    void* context)
{
    s_entity_type_two_event_hook = hook;
    s_entity_type_two_event_context = context;
}

/* TypeTwoEntityEventDispatch (FUN_0043b820): resolve the supplied entity or
 * its attached owner to a type-2 record, then dispatch through its event hook. */
// FUNCTION: LEGORR 0x0043B820
void entity_type_two_event_dispatch(const EntitySlot* entity,
                                    uint32_t argument_a,
                                    uint32_t argument_b)
{
    const EntitySlot* selected = entity;
    if (selected == NULL) return;
    if (selected->type != 2) {
        selected = (const EntitySlot*)selected->owner_36c;
    }
    if (selected != NULL && selected->type == 2
        && s_entity_type_two_event_hook != NULL) {
        s_entity_type_two_event_hook(selected, argument_b, argument_a,
                                     s_entity_type_two_event_context);
    }
}

void entity_type_two_positioned_hooks_set(
    const EntityTypeTwoPositionedHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_entity_type_two_positioned_hooks, 0,
               sizeof(s_entity_type_two_positioned_hooks));
    }
    else {
        s_entity_type_two_positioned_hooks = *hooks;
    }
}

/* TypeTwoEntityPositionedCallbackDispatch (FUN_0043b850): resolve the type-2
 * record, optionally capture a scene-object position, and delegate the
 * record-set walk to the registered callback. */
// FUNCTION: LEGORR 0x0043B850
void entity_type_two_positioned_callback_dispatch(
    const EntitySlot* entity, void* scene_object, uint32_t argument_a,
    uint32_t argument_b, int mode)
{
    const EntitySlot* selected = entity;
    float position[3] = { 0.0f, 0.0f, 0.0f };
    EntityTypeTwoPositionedHooks* hooks = &s_entity_type_two_positioned_hooks;
    if (selected == NULL || hooks->dispatch == NULL) return;
    if (selected->type != 2) {
        selected = (const EntitySlot*)selected->owner_36c;
    }
    if (mode != 0 && hooks->position_get != NULL) {
        (void)hooks->position_get(hooks->context, scene_object, position);
    }
    hooks->dispatch(hooks->context, selected, argument_a, argument_b,
                    mode, position);
}

void entity_task_count_context_set(GameContext* context,
                                   EntityTaskCoordinateMatchHook match,
                                   void* user)
{
    s_entity_task_count_context = context;
    s_entity_task_coordinate_match_hook = match;
    s_entity_task_coordinate_match_user = user;
}

/* TaskCountAtGridCoordinateQuery (FUN_0043b360): clear the result and scan
 * the live entity tables for task records associated with the requested cell. */
// FUNCTION: LEGORR 0x0043B360
void entity_task_count_at_grid_coordinate_query(uint32_t x, uint32_t y,
                                                uint32_t* out_count)
{
    int type, index;
    if (out_count == NULL) return;
    *out_count = 0;
    if (s_entity_task_count_context == NULL) return;
    for (type = 0; type < ENTITY_TYPES; ++type) {
        int count = s_entity_task_count_context->entity_count[type];
        if (count < 0) count = 0;
        if (count > ENTITY_CAP) count = ENTITY_CAP;
        for (index = 0; index < count; ++index) {
            const EntitySlot* entity =
                &s_entity_task_count_context->entity[type][index];
            int matches;
            if (entity->type == 0) continue;
            if (s_entity_task_coordinate_match_hook != NULL) {
                matches = s_entity_task_coordinate_match_hook(
                    s_entity_task_coordinate_match_user, entity, x, y);
            }
            else {
                matches = entity->activity >= 0
                    && entity->pos_pair_2f4 == (float)x
                    && entity->pos_pair_2f8 == (float)y;
            }
            if (matches) ++*out_count;
        }
    }
}

void entity_building_deposit_hook_set(EntityBuildingDepositHook hook,
                                      void* context)
{
    s_entity_building_deposit_hook = hook;
    s_entity_building_deposit_context = context;
}

/* BuildingDepositStateMonitor (FUN_0043bb20): clear stale deposit state
 * when the carried count, capability threshold, or state flags no longer
 * permit it; otherwise request the configured deposit action when supported. */
// FUNCTION: LEGORR 0x0043BB20
void entity_building_deposit_state_monitor(EntitySlot* entity)
{
    uint32_t capabilities;
    uint32_t threshold;
    if (entity == NULL || entity->type != 4) return;
    capabilities = entity_capability_flags_get(entity);
    if ((capabilities & 2u) == 0u) {
        threshold = (uint32_t)entity->carry_state_324;
    }
    else {
        EntityHealthThresholdState state;
        memset(&state, 0, sizeof(state));
        threshold = (uint32_t)entity->field_328;
        if (s_entity_health_threshold_read_hook != NULL
            && s_entity_health_threshold_read_hook(
                entity, &state, s_entity_health_threshold_context)
            && state.upper_limit < threshold) {
            threshold = state.upper_limit;
        }
    }
    if ((uint32_t)entity->carry_state_324 < threshold
        || (entity->flags_3e0 & 0x2000u) != 0u
        || (entity->flags_3e8 & 0x01000000u) != 0u) {
        entity->flags_3e8 &= ~0x01000000u;
        return;
    }
    if ((capabilities & (4u | 2u)) != 0u
        && s_entity_building_deposit_hook != NULL) {
        s_entity_building_deposit_hook(entity,
                                       s_entity_building_deposit_context);
    }
}

void entity_deposit_activity_hooks_set(const EntityDepositActivityHooks* hooks)
{
    if (hooks == NULL) memset(&s_entity_deposit_activity_hooks, 0,
                              sizeof(s_entity_deposit_activity_hooks));
    else s_entity_deposit_activity_hooks = *hooks;
}

/* EntityDepositActivityGate (FUN_0043a300): activated deposits enter activity
 * 0x1d through the live activity setter; otherwise the accepted target and
 * route hooks control the approach/attachment branch. */
// FUNCTION: LEGORR 0x0043A300
int entity_deposit_activity_gate(EntitySlot* entity, EntitySlot* target)
{
    uint32_t flags;
    int direction = 0;
    if (entity == NULL || (entity->flags_3e0 & 0x400u) == 0u) return 0;
    if (s_entity_deposit_activity_hooks.activity_flags != NULL)
        flags = s_entity_deposit_activity_hooks.activity_flags(
            s_entity_deposit_activity_hooks.context, entity);
    else flags = entity_stats_word_get(entity);
    if ((flags & (4u | 2u)) != 0u) {
        entity->flags_3e0 |= 0x2000u;
        ai_set_activity(entity, 0x1d, 1);
        entity->activity_timer = 1.0f;
        if (s_entity_deposit_activity_hooks.deposit_cancel != NULL)
            s_entity_deposit_activity_hooks.deposit_cancel(
                s_entity_deposit_activity_hooks.context, entity);
        return 1;
    }
    if (target == NULL) return 0;
    if (!entity_accepts_target_for_task_check(
            target, (const EntitySlot*)entity->carry_link_308, 0)
        && (entity_stats_word_get(target) & 8u) == 0u) return 0;
    flags = entity_stats_word_get(target);
    if (s_entity_deposit_activity_hooks.target_busy != NULL
        && s_entity_deposit_activity_hooks.target_busy(
            s_entity_deposit_activity_hooks.context, target, 0)) flags |= 8u;
    if ((flags & 8u) == 0u && s_entity_deposit_activity_hooks.route_to_target != NULL
        && !s_entity_deposit_activity_hooks.route_to_target(
            s_entity_deposit_activity_hooks.context, entity, target, &direction))
        return 0;
    if (s_entity_deposit_activity_hooks.deposit_start != NULL)
        s_entity_deposit_activity_hooks.deposit_start(
            s_entity_deposit_activity_hooks.context, entity, target, direction);
    return 1;
}

static int entity_kind_four_candidate_search(LevelState* level, int x, int y,
                                              int amount)
{
    static const int offsets[4][2] = {
        { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }
    };
    int direction;
    for (direction = 0; direction < 4; ++direction) {
        int cell_xy[2];
        int nx = x + offsets[direction][0];
        int ny = y + offsets[direction][1];
        if (level_cell_secondary_bit0(level, nx, ny) != 0u) {
            EntitySlot* found;
            cell_xy[0] = nx;
            cell_xy[1] = ny;
            found = entity_at_grid_position_find(cell_xy);
            if (found != NULL && entity_available_resource_spend(
                    level, amount, found)) return 1;
        }
        if (!level_cell_occupied_at(level, nx, ny) &&
            !level_cell_occupied_or_flag(level, nx, ny, 1)) continue;
        if (level_cell_visited_flag(level, nx, ny) != 0u) continue;
        terrain_cell_visited_flag_set_and_track(level, nx, ny);
        if (entity_kind_four_candidate_search(level, nx, ny, amount))
            return 1;
    }
    return 0;
}

static int entity_kind_four_candidate_find(LevelState* level,
                                            const int start_xy[2],
                                            int amount)
{
    int found;
    if (level == NULL || start_xy == NULL) return 0;
    found = entity_kind_four_candidate_search(level, start_xy[0], start_xy[1],
                                               amount);
    terrain_cell_visited_flags_clear_all(level);
    return found;
}

static void entity_kind_four_adjacent_expand(LevelState* level, int x, int y)
{
    static const int offsets[4][2] = {
        { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }
    };
    int direction;
    for (direction = 0; direction < 4; ++direction) {
        int nx = x + offsets[direction][0];
        int ny = y + offsets[direction][1];
        if (!level_cell_occupied_at(level, nx, ny) &&
            !level_cell_occupied_or_flag(level, nx, ny, 1)) continue;
        if (level_cell_secondary_flag_100(level, nx, ny) != 0u) continue;
        terrain_cell_flag100_set_and_track(level, nx, ny);
        entity_kind_four_adjacent_expand(level, nx, ny);
    }
}

/* KindFourPostCreateResolve (FUN_0043c570): clear the pending marker and
 * register the kind-four object when its nearby resource search fails;
 * otherwise keep the marker and expand the adjacent-cell state when the
 * observed stats and entity-eligibility gates pass. */
// FUNCTION: LEGORR 0x0043c570
void entity_kind_four_post_create_resolve(EntitySlot* entity)
{
    LevelState* level;
    uint32_t flags;
    int cell_xy[2];
    int have_cell;
    if (entity == NULL || entity->type != 4 ||
        (entity->flags_3e8 & 0x80000000u) != 0u) return;
    level = entity->owner_ctx != NULL ? entity->owner_ctx->level : NULL;
    if (level == NULL) {
        GameContext* context = entity_pool_callback_context_get();
        level = context != NULL ? context->level : NULL;
    }
    have_cell = 0;
    cell_xy[0] = -1;
    cell_xy[1] = -1;
    if (level != NULL) {
        entity_position_to_terrain_cell(entity, level, &cell_xy[0],
                                        &cell_xy[1]);
        have_cell = level_coord_pair_within_bounds(
            level, cell_xy[0], cell_xy[1]);
    }
    flags = entity_state_flags_get(entity);
    if ((flags & (0x200u | 0x400u)) == 0u && have_cell &&
        !entity_kind_four_candidate_find(level, cell_xy,
            (int)stats_instance_word_at_bc(entity))) {
        entity->flags_3e8 &= ~0x20000000u;
        type4_object_registry_insert(entity);
        return;
    }
    entity->flags_3e8 |= 0x20000000u;
    if ((flags & 0x200u) != 0u && have_cell &&
        entity_shared_eligibility_check(entity, 0)) {
        /* FUN_0041f8e0 is empty, so its return-register residue preserves
         * the preceding nonzero eligibility result in the original call. */
        entity_kind_four_adjacent_expand(level, cell_xy[0], cell_xy[1]);
    }
}

void entity_build_placement_hooks_set(const EntityBuildPlacementHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_entity_build_placement_hooks, 0,
               sizeof(s_entity_build_placement_hooks));
    }
    else {
        s_entity_build_placement_hooks = *hooks;
    }
}

void entity_build_placement_context_bind(GameContext* context)
{
    s_entity_build_placement_context = context;
}

/* EntityBuildPlacementCommandSetup and its alternate (FUN_00439370 / 4b0).
 * World-cell sampling, model creation, command enqueueing, and mission
 * counters cross the installed game/scene boundary hooks. */
int entity_build_placement_command_setup(const int cell_xy[2], int entity_id,
                                         const float world_xy[2],
                                         int update_state, int alternate)
{
    EntityBuildPlacementHooks* hooks = &s_entity_build_placement_hooks;
    float position[2];
    float cell_position[3];
    EntitySlot* created;
    GameContext* context = entity_pool_callback_context_get();
    uint8_t* cell_record;
    int kind = alternate ? 7 : 6;
    int opcode = alternate ? 0x1c : 0x16;
    if (cell_xy == NULL) return 0;
    if (world_xy != NULL) {
        position[0] = world_xy[0];
        position[1] = world_xy[1];
    } else if (hooks->position_choose != NULL) {
        if (!hooks->position_choose(hooks->context, cell_xy, position))
            return 0;
    } else {
        if (!terrain_cell_world_position_get(cell_xy[0], cell_xy[1],
                                             cell_position)) return 0;
        position[0] = cell_position[0];
        position[1] = cell_position[1];
    }
    if (hooks->entity_create != NULL) {
        created = hooks->entity_create(hooks->context, kind, 0, entity_id,
                                       position[0], position[1]);
    } else {
        created = entity_create_with_position(context, kind, NULL, entity_id,
                                              position[0], position[1], 0.0f);
    }
    if (created == NULL) return 0;
    if (hooks->queue_entity != NULL)
        hooks->queue_entity(hooks->context, opcode, created);
    else
        cmd_enqueue(opcode, created, 0, 0, 0);
    cell_record = context != NULL && context->level != NULL
        ? level_voxel(context->level, cell_xy[0], cell_xy[1]) : NULL;
    if (alternate) {
        if (hooks->event_raise != NULL) {
            hooks->event_raise(hooks->context, 0x13, cell_xy);
        } else {
            advisor_text_animation_update(0x13, 1, 0);
        }
        if (update_state && hooks->cell_counter_increment != NULL) {
            hooks->cell_counter_increment(hooks->context, cell_xy, 0x30);
        } else if (update_state && cell_record != NULL) {
            ++*(uint32_t*)(cell_record + 0x30);
        }
    }
    else if (update_state) {
        if (hooks->event_raise != NULL) {
            hooks->event_raise(hooks->context, 0x11, cell_xy);
        } else {
            message_dispatch(0, 0, 0, cell_xy);
            advisor_text_animation_update(0x11, 1, 0);
        }
        if (hooks->cell_counter_increment != NULL) {
            hooks->cell_counter_increment(hooks->context, cell_xy, 0x2c);
        } else if (cell_record != NULL) {
            ++*(uint32_t*)(cell_record + 0x2c);
        }
    }
    return 1;
}

// FUNCTION: LEGORR 0x00439340
void entity_build_placement_command_pair_create(const int cell_xy[2],
                                                int entity_id)
{
    (void)entity_build_placement_command_setup(cell_xy, entity_id, NULL, 1, 0);
    (void)entity_build_placement_command_setup(cell_xy, entity_id, NULL, 1, 1);
}

/* EntityBuildPlacementCommandBatchExpand (FUN_00439280): parse four counts,
 * then expand selectors 0/1 into the two command-setup families. The four
 * values are the outputs of the level's byte-state classifier at 0x42c5b0,
 * so this path uses the live level record instead of requiring a separately
 * installed count callback. */
// FUNCTION: LEGORR 0x00439280
void entity_build_placement_command_batch_expand(const int cell_xy[2])
{
    uint32_t counts[4];
    uint32_t i;
    GameContext* context = entity_pool_callback_context_get();
    if (cell_xy == NULL || context == NULL || context->level == NULL ||
        !level_cell_byte_state_classify(context->level, cell_xy[0],
            cell_xy[1], &counts[0], &counts[1], &counts[2], &counts[3])) {
        return;
    }
    for (i = 0; i < counts[0]; ++i) {
        (void)entity_build_placement_command_setup(cell_xy, 0, NULL, 1, 0);
    }
    for (i = 0; i < counts[1]; ++i) {
        (void)entity_build_placement_command_setup(cell_xy, 1, NULL, 1, 0);
    }
    for (i = 0; i < counts[2]; ++i) {
        (void)entity_build_placement_command_setup(cell_xy, 0, NULL, 1, 1);
    }
    for (i = 0; i < counts[3]; ++i) {
        (void)entity_build_placement_command_setup(cell_xy, 1, NULL, 1, 1);
    }
}

/* EntityActionKindSupported (FUN_0043a7c0): the no-op callback used by the
 * kind-6 branch has no defined nonzero return in the port, while kind 7
 * checks the selected live level resource. */
static int entity_action_kind_supported(const EntitySlot* source, int kind,
                                        int selector)
{
    uint32_t flags;
    const LevelState* level;
    if (source == NULL) return 0;
    flags = entity_stats_word_get(source);
    if ((flags & 8u) == 0u) return 0;
    if (kind == 6) return 0;
    if (kind == 7) {
        level = source->owner_ctx != NULL ? source->owner_ctx->level : NULL;
        if (level == NULL) return 0;
        return level_int(level, selector != 0 ? 0xac : 0xa0) != 0;
    }
    return kind == 8 || kind == 9 || kind == 0xb || kind == 0xd;
}

void entity_action_issue_hooks_set(const EntityActionIssueHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_entity_action_issue_hooks, 0,
               sizeof(s_entity_action_issue_hooks));
    }
    else {
        s_entity_action_issue_hooks = *hooks;
    }
}

/* ActionKindEntityCreateAndCommandIssue (FUN_0043a650). The installed
 * constructor owns resource/template selection; this routine keeps the
 * verified action gate, event order, queue opcode, and kind labels. */
// FUNCTION: LEGORR 0x0043A650
EntitySlot* entity_action_kind_entity_create_and_command_issue(
    EntitySlot* source, int kind, int selector, int argument)
{
    EntityActionIssueHooks* hooks = &s_entity_action_issue_hooks;
    EntitySlot* created;
    int opcode;
    if (source == NULL || hooks->create == NULL || hooks->queue == NULL
        || !entity_action_kind_supported(source, kind, selector)) return NULL;
    switch (kind) {
    case 6:
    case 7:
    case 8:
    case 9:
    case 0xb:
    case 0xd:
        break;
    default:
        return NULL;
    }
    created = hooks->create(hooks->context, source, kind, selector, argument);
    if (created == NULL) return NULL;
    if (kind == 6 || kind == 7) {
        if (hooks->kind_event != NULL) {
            hooks->kind_event(hooks->context, kind, selector);
        }
    }
    opcode = kind == 7 ? 0x1c : 0x16;
    hooks->queue(hooks->context, opcode, created);
    if (kind == 8 && hooks->label_set != NULL) {
        hooks->label_set(hooks->context, created, "Normal");
    }
    else if (kind == 9 && hooks->label_set != NULL) {
        hooks->label_set(hooks->context, created, "Short");
    }
    return created;
}

void entity_action_consume_hooks_set(const EntityActionConsumeHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_entity_action_consume_hooks, 0,
               sizeof(s_entity_action_consume_hooks));
    }
    else {
        s_entity_action_consume_hooks = *hooks;
    }
}

/* EntityActionRecordConsumeAndCleanup (FUN_0043a850): preserve the type-6
 * count accumulation, type-7 external notification, record release, and
 * non-creature activity-list cleanup. */
// FUNCTION: LEGORR 0x0043A850
void entity_action_record_consume_and_cleanup(
    EntitySlot* entity, EntityActionConsumeRecord* record)
{
    EntityActionConsumeHooks* hooks = &s_entity_action_consume_hooks;
    if (entity == NULL || record == NULL) return;
    if (entity->type == 3 && !entity->action_consume_counters_initialized) {
        memset(entity->action_consume_counters, 0,
               sizeof(entity->action_consume_counters));
        entity->action_consume_counters_initialized = 1;
    }
    if (record->type == 6) {
        if (entity->type == 3) {
            uint32_t slot = record->counter_index;
            if (slot < 6u) ++entity->action_consume_counters[slot];
        }
        else if (hooks->event_dispatch != NULL) {
            hooks->event_dispatch(hooks->context, 6, record->reference);
        }
        else if (entity->owner_ctx != NULL) {
            global_object_field8c_increment(entity->owner_ctx->level);
        }
    }
    else if (record->type == 7 && entity->type != 3
             && hooks->event_dispatch != NULL) {
        hooks->event_dispatch(hooks->context, 7, record->reference);
    }
    else if (record->type == 7 && entity->type != 3
             && entity->owner_ctx != NULL) {
        level_resource_balance_add_and_sound(record->reference);
    }
    record->active = 0;
    if (hooks->record_release != NULL) {
        hooks->record_release(hooks->context, record);
    }
    if (entity->type != 3 && hooks->entity_records_cleanup != NULL) {
        hooks->entity_records_cleanup(hooks->context, entity);
    }
}

void entity_clipped_rectangle_hooks_set(
    const EntityClippedRectangleHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_entity_clipped_rectangle_hooks, 0,
               sizeof(s_entity_clipped_rectangle_hooks));
    }
    else {
        s_entity_clipped_rectangle_hooks = *hooks;
    }
}

/* ClippedRectangleCallbackDispatch (FUN_0043b6c0): form the componentwise
 * rectangle union, visit entities, then enqueue opcode 0x2d unless renderer
 * state bit 8 suppresses the path. */
// FUNCTION: LEGORR 0x0043b6c0
void entity_clipped_rectangle_callback_dispatch(
    void* payload, const float rect_a[2], const float rect_b[2])
{
    float bounds[4];
    EntityClippedRectangleHooks* hooks = &s_entity_clipped_rectangle_hooks;
    if (rect_a == NULL || rect_b == NULL
        || (system_state_flags_get() & 8u) != 0u) return;
    bounds[0] = rect_a[0] < rect_b[0] ? rect_a[0] : rect_b[0];
    bounds[1] = rect_a[1] < rect_b[1] ? rect_a[1] : rect_b[1];
    bounds[2] = rect_a[0] > rect_b[0] ? rect_a[0] : rect_b[0];
    bounds[3] = rect_a[1] > rect_b[1] ? rect_a[1] : rect_b[1];
    if (hooks->visit_entities != NULL) {
        hooks->visit_entities(hooks->context, payload, bounds);
    }
    if (hooks->queue_command != NULL) {
        hooks->queue_command(hooks->context, 0x2d);
    }
}

void entity_pair_ray_hit_check_hook_set(EntityPairRayHitCheckHook hook,
                                        void* context)
{
    s_entity_pair_ray_hit_check_hook = hook;
    s_entity_pair_ray_hit_check_context = context;
}

int entity_pair_ray_hit_check(const EntitySlot* first,
                              const EntitySlot* second)
{
    if (first == NULL || second == NULL
        || s_entity_pair_ray_hit_check_hook == NULL) return 1;
    return s_entity_pair_ray_hit_check_hook(first, second,
                                            s_entity_pair_ray_hit_check_context)
        != 0;
}

/* EntityActivityCountersAdvance (FUN_0043c6b0): accumulate the frame delta
 * into the three per-entity counters. Type 2 emits the two configured
 * threshold messages once on each upward crossing; kinds 6, 7, 9, and 0xb
 * schedule deferred work each time the first counter reaches 750. */
// FUNCTION: LEGORR 0x0043c6b0
void entity_activity_counters_advance(EntitySlot* entity, float elapsed)
{
    float old_3d8;
    float old_3d4;
    float threshold;
    const LevelState* level;
    if (entity == NULL) return;
    old_3d8 = entity->activity_counter_3d8;
    old_3d4 = entity->activity_counter_3d4;
    entity->activity_counter_3d8 += elapsed;
    entity->activity_counter_3d4 += elapsed;
    entity->activity_counter_3dc += elapsed;
    level = entity->owner_ctx != NULL ? entity->owner_ctx->level : NULL;
    if (entity->type == 2) {
        threshold = engine_float_field210_get_by_index(level, 2);
        if (threshold != 0.0f
            && threshold <= entity->activity_counter_3d8
            && old_3d8 < threshold) {
            message_dispatch(0x22, 0,
                (uint32_t)(uintptr_t)entity, NULL);
        }
        threshold = engine_float_field260_get(level);
        if (threshold != 0.0f
            && threshold <= entity->activity_counter_3d4
            && old_3d4 < threshold) {
            message_dispatch(0x23, 0,
                (uint32_t)(uintptr_t)entity, NULL);
            return;
        }
    }
    else if ((entity->type == 6 || entity->type == 7 || entity->type == 9
              || (entity->type == 0xb
                  && (entity->flags_3e4 & 0x02000000u) == 0u))
             && 750.0f <= old_3d8 + elapsed && old_3d8 < 750.0f) {
        entity->activity_counter_3d8 = 0.0f;
        entity_deferred_work_schedule(entity);
    }
}

static EntityOneShotAttachedEffectHooks s_one_shot_attached_effect_hooks;

void entity_one_shot_attached_effect_hooks_set(
    const EntityOneShotAttachedEffectHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_one_shot_attached_effect_hooks, 0,
               sizeof(s_one_shot_attached_effect_hooks));
    }
    else {
        s_one_shot_attached_effect_hooks = *hooks;
    }
}

/* OneShotAttachedEffectStart (FUN_0044c030): a type-3 owner accepts this
 * only once. The scene adapter supplies the pose and child/action operations;
 * the owner timer, child kind, flags, and reciprocal +0x364 link stay here. */
// FUNCTION: LEGORR 0x0044c030
int entity_one_shot_attached_effect_start(EntitySlot* entity,
                                          float duration)
{
    EntityOneShotAttachedEffectHooks* const hooks =
        &s_one_shot_attached_effect_hooks;
    EntitySlot* child;
    float position[2];
    float angle;
    if (entity == NULL || entity->type != 3
        || (entity->flags_3e4 & 0x08000000u) != 0u) {
        return 0;
    }
    entity->flags_3e4 |= 0x08000000u;
    entity->wake_timer = duration * 25.0f;
    if (hooks->scene_pose_get == NULL || hooks->child_create == NULL) {
        return 0;
    }
    if (!hooks->scene_pose_get(hooks->context, entity, position, &angle)) {
        return 0;
    }
    child = hooks->child_create(hooks->context, entity, 0x12,
                                position, angle);
    if (child == NULL) return 0;
    if (hooks->child_action_start != NULL) {
        hooks->child_action_start(hooks->context, child, "Start");
    }
    child->flags_3e0 |= 0x100u;
    entity->wake_target = child;
    child->wake_target = entity;
    return 1;
}

/* EntityPairScalarFieldsSet (FUN_0044c150): accept kinds one through three,
 * reset their action state, then install the pair/scalar and raise the
 * corresponding +0x3e4 state bit. */
// FUNCTION: LEGORR 0x0044c150
int entity_pair_scalar_fields_set(EntitySlot* entity, const int32_t pair[2],
                                  int32_t scalar)
{
    if (entity == NULL || pair == NULL
        || (entity->type != 1 && entity->type != 2 && entity->type != 3)) {
        return 0;
    }
    entity_state_reset(entity, 0, 1);
    entity->task_pair_3b4[0] = pair[0];
    entity->task_pair_3b4[1] = pair[1];
    entity->task_scalar_3bc = scalar;
    entity->flags_3e4 |= 0x00004000u;
    return 1;
}

// FUNCTION: LEGORR 0x0043c3e0
/* EntityType26CommandCompletionGate (FUN_0043c3e0): only type-2 entities
 * carrying a kind-0x1a command are considered; distance acceptance followed
 * by a clear ray check completes the gate. The renderer-owned ray provider
 * must be installed before this gate can report completion. */
int entity_type26_command_completion_gate(const EntitySlot* entity)
{
    const void* command;
    const EntitySlot* target;
    int ray_hit;
    if (entity == NULL || entity->type != 2) return 0;
    command = entity->cmd_head;
    if (command == NULL || cmd_rec_type(command) != 0x1a) return 0;
    target = (const EntitySlot*)cmd_rec_payload(command);
    if (target == NULL || !entity_centers_within_configured_radius(entity,
                                                                  target)) {
        return 0;
    }
    if (s_entity_pair_ray_hit_check_hook != NULL) {
        ray_hit = s_entity_pair_ray_hit_check_hook(
            entity, target, s_entity_pair_ray_hit_check_context);
    }
    else {
        /* object_pair_ray_hit_check needs renderer object positions and the
         * scene trace owner. Do not treat its no-provider result as a clear
         * ray; no production installer for these ops exists in this tree. */
        if (s_entity_ray_hit_sweep_ops.object_position_get == NULL ||
            s_entity_ray_hit_sweep_ops.scene_trace_prepare == NULL) {
            return 0;
        }
        ray_hit = object_pair_ray_hit_check((void*)entity, (void*)target);
    }
    return ray_hit == 0;
}

static int entity_unload_adjacent_cell_find_on_level(
    const EntitySlot* entity, const LevelState* level, int out_xy[2])
{
    static const int offsets[4][2] = {
        { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }
    };
    int x = 0, y = 0, i;
    if (entity == NULL || level == NULL) return 0;
    entity_position_to_terrain_cell(entity, level, &x, &y);
    if (level_coord_pair_within_bounds(level, x, y) == 0) return 0;
    for (i = 0; i < 4; ++i) {
        int candidate_x = x + offsets[i][0];
        int candidate_y = y + offsets[i][1];
        if (level_cell_flag_8_bounded(level, candidate_x, candidate_y) != 0u
            && terrain_cell_route_and_occupancy_check(
                NULL, level, candidate_x, candidate_y,
                candidate_x, candidate_y, 0) != 0) {
            if (out_xy != NULL) {
                out_xy[0] = candidate_x;
                out_xy[1] = candidate_y;
            }
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00439d50
int entity_vehicle_unload_adjacent_cell_find(const EntitySlot* entity,
                                             int out_xy[2])
{
    const GameContext* context;
    if (entity == NULL) return 0;
    context = entity->owner_ctx;
    return entity_unload_adjacent_cell_find_on_level(
        entity, context != NULL ? context->level : NULL, out_xy);
}

// FUNCTION: LEGORR 0x00439e10
int entity_vehicle_unload_cell_find_if_eligible(const EntitySlot* entity,
                                                int out_xy[2])
{
    if (entity == NULL
        || (entity_capability_flags_get(entity) & 0x10u) == 0u) {
        return 0;
    }
    return entity_vehicle_unload_adjacent_cell_find(entity, out_xy);
}

// FUNCTION: LEGORR 0x00439e40
int entity_record_flag20_query(const EntitySlot* entity, int out_xy[2])
{
    if (entity == NULL
        || (entity_capability_flags_get(entity) & 0x20u) == 0u) {
        return 0;
    }
    return entity_vehicle_unload_adjacent_cell_find(entity, out_xy);
}

// FUNCTION: LEGORR 0x00433180
int entity_cell_type_speed_multiplier_resolve(const EntitySlot* entity,
                                              const LevelState* level,
                                              int x, int y, float scale,
                                              float* out_scale)
{
    static const int excluded_types[] = { 6, 7, 5, 8, 11, 13, 14, 15, 16, 17, 18, 19 };
    uint8_t* cell;
    uint32_t cell_flags = 0;
    uint32_t i;
    int type;
    if (entity == NULL || level == NULL) return 0;
    for (i = 0; i < sizeof(excluded_types) / sizeof(excluded_types[0]); ++i) {
        if (entity->type == excluded_types[i]) return 0;
    }
    if (entity->owner_36c != NULL &&
        (entity_stats_word_get(entity) & 0x20000u) == 0u) return 0;
    cell = level_voxel(level, x, y);
    if (cell == NULL) return 0;
    type = cell[2];
    memcpy(&cell_flags, cell + 8, sizeof(cell_flags));
    if (type == 6 && (cell_flags & 0x200u) == 0u &&
        (entity_state_flags_get(entity) & 0x200000u) == 0u) {
        if (out_scale != NULL) *out_scale = scale;
        return 1;
    }
    if ((cell_flags & 0x1000u) != 0u) {
        if (out_scale != NULL) *out_scale = scale;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00443840
int entity_route_cell_candidate_accept(const EntitySlot* entity,
                                       const LevelState* level,
                                       int x, int y, int mode)
{
    if (level == NULL) return 0;
    if (mode == 0 && level_cell_walkable_flag(level, x, y)) return 0;
    if (!terrain_cell_route_and_occupancy_check(entity, level, x, y, x, y, 1))
        return 0;
    if (entity_cell_type_speed_multiplier_resolve(entity, level, x, y,
                                                   0.0f, NULL)) return 0;
    if (level_cell_mask_180000(level, x, y)) return 0;
    return 1;
}

// FUNCTION: LEGORR 0x00442260
void entity_transform_position_get(const EntitySlot* entity, float out_xy[2])
{
    float position[3];
    if (entity == NULL || out_xy == NULL) return;
    /* The reference resolves the kind-specific transform, then reads its
     * slot-zero position.  The attached scene object is the port's live
     * transform for objects whose type component has reached the renderer;
     * the slot coordinates remain the simulation fallback. */
    if (entity->attached_scene_object != NULL &&
        ddiscene_object_position_get(entity->attached_scene_object, NULL,
                                     position)) {
        out_xy[0] = position[0];
        out_xy[1] = position[1];
        return;
    }
    out_xy[0] = entity->x;
    out_xy[1] = entity->y;
}

// FUNCTION: LEGORR 0x00471CA0
int entity_world_center_position_read(const EntitySlot* entity,
                                      float out_xyz[3])
{
    if (entity == NULL || out_xyz == NULL) return 0;
    out_xyz[0] = entity->x;
    out_xyz[1] = entity->y;
    out_xyz[2] = entity->z - entity_stats_float_at74(entity) * 0.5f;
    return 1;
}

static int entity_centers_within_radius(const EntitySlot* first,
                                       const EntitySlot* second,
                                       float radius)
{
    float a[3];
    float b[3];
    float dx, dy, dz;
    if (first == NULL || second == NULL || radius <= 0.0f
        || !entity_world_center_position_read(first, a)
        || !entity_world_center_position_read(second, b)) return 0;
    dx = a[0] - b[0];
    dy = a[1] - b[1];
    dz = a[2] - b[2];
    return sqrtf(dx * dx + dy * dy + dz * dz) < radius;
}

// FUNCTION: LEGORR 0x00472080
int entity_centers_within_configured_radius(const EntitySlot* first,
                                            const EntitySlot* second)
{
    int activity_class;
    if (first == NULL || second == NULL) return 0;
    activity_class = ai_activity_probe(first);
    if (activity_class == 0) return 0;
    return entity_centers_within_radius(first, second,
        lego::weapon_type_range_get(activity_class));
}

// FUNCTION: LEGORR 0x00472130
int entity_centers_within_object_radius(const EntitySlot* first,
                                        const EntitySlot* second)
{
    if (first == NULL || second == NULL) return 0;
    return entity_centers_within_radius(first, second,
                                        entity_collision_radius_get(first));
}

// FUNCTION: LEGORR 0x004428a0
void entity_world_position_set(EntitySlot* entity, float x, float y,
                               float heading, int update_heading)
{
    if (entity == NULL) return;
    /* Keep an already-created scene transform in step with the simulation
     * coordinates.  Kind-specific pose components are not represented in
     * EntitySlot yet, so the renderer handle is the available live endpoint. */
    if (entity->attached_scene_object != NULL)
        (void)ddiscene_object_position_set(entity->attached_scene_object, NULL,
                                           x, y, entity->z);
    entity->x = x;
    entity->y = y;
    if (update_heading) entity->rot = heading;
    /* Corpus writes 0.01 to the +0xae tick scale after every update. */
    entity->movement_tick_scale = 0.01f;
}

void entity_type5_finalize_environment_set(uint32_t mode_flags,
                                          EntityFinalizeHook timeout_hook,
                                          EntityFinalizeHook finalize_hook)
{
    s_entity_finalize_mode_flags = mode_flags;
    s_entity_type5_timeout_hook = timeout_hook;
    s_entity_type5_finalize_hook = finalize_hook;
}

// FUNCTION: LEGORR 0x00441ea0
void entity_type5_finalize_if_allowed(EntitySlot* entity)
{
    if (entity == NULL || (s_entity_finalize_mode_flags & 2u) != 0u ||
        entity->type != 5) return;
    entity->type5_finalize_timeout = 10000.0f;
    if (s_entity_type5_timeout_hook != NULL)
        s_entity_type5_timeout_hook(entity);
    if (s_entity_type5_finalize_hook != NULL)
        s_entity_type5_finalize_hook(entity);
}

void entity_route_state_hook_set(EntityRouteStateHook hook)
{
    s_entity_route_state_hook = hook;
}

// FUNCTION: LEGORR 0x00442210
void entity_action_route_state_initialize(EntitySlot* entity)
{
    if (entity == NULL) return;
    entity->activity_timer = 0.0f;
    entity->last_action_event = 0x4du;
    entity->last_action_event_flags = 1u;
    if (s_entity_route_state_hook != NULL &&
        s_entity_route_state_hook(entity) != 0) {
        entity->flags_3e0 |= 0x100000u;
    }
    entity->field_340 = -1.0f;
}

// FUNCTION: LEGORR 0x00444620
void entity_enter_event21_state_once(EntitySlot* entity)
{
    if (entity == NULL || (entity->flags_3e0 & 0x04000000u) != 0u ||
        entity->owner_36c != NULL) return;
    entity_state_reset(entity, 0, 1);
    entity->flags_3e0 |= 0x04000000u;
    ai_set_activity(entity, 0x21, 0);
    ai_idle_chain_record_attach(entity);
}

/* EntityProgressAdvanceBy25 (FUN_00447730, findings 377/448): the corpus
 * routing-progress step — add 25 (the corpus writes `- -25.0`), clear
 * the link field, clamp to the entity's own +0x340 bound, and report
 * when the bound was reached. Field meanings stay with the routing
 * layer per finding 448. */
// FUNCTION: LEGORR 0x00447730
int entity_progress_advance_by_25(EntitySlot* e)
{
    const float advanced = e->progress_344 + 25.0f;
    e->link_2fc = 0u;
    e->progress_344 = advanced;
    if (e->field_340 < advanced) {
        e->progress_344 = e->field_340;
        return 1;
    }
    return 0;
}

/* LinkedEntityRouteFlagClear (FUN_0043f100, finding 428): the corpus
 * clears bit 0x40 in +0x3ec on the entity and on its optional +0x2fc
 * linked entity. The port stores those words in EntitySlot mirrors; a
 * null-safe guard documents the port's deviation from the unchecked body. */
// FUNCTION: LEGORR 0x0043f100
void entity_linked_route_flag_clear(GameContext* ctx, EntitySlot* e)
{
    (void)ctx; /* the corpus's first parameter is not read */
    if (e == NULL) {
        return;
    }
    e->flags_3ec &= ~0x40u;
    if (e->link_2fc != 0u) {
        EntitySlot* linked = (EntitySlot*)e->link_2fc;
        linked->flags_3ec &= ~0x40u;
    }
}

/* EntityClearNestedState8Fields (FUN_004487c0): clears the +0x2f4/+0x2f8
 * pair on the linked type-8 object when the entity's 0x400 mode bit is
 * raised. The corpus then performs a wider attached-object detach pass;
 * this port keeps the coordinate/state writes and leaves that registry
 * cleanup behind its still-unmodeled owner. */
// FUNCTION: LEGORR 0x004487c0
void entity_clear_nested_state_8_fields(EntitySlot* e)
{
    if (e == NULL || (e->flags_3e0 & 0x400u) == 0u
        || e->carry_link_308 == NULL) {
        return;
    }
    EntitySlot* nested = (EntitySlot*)e->carry_link_308;
    if (nested->type != 8) {
        return;
    }
    nested->pos_pair_2f4 = 0.0f;
    nested->pos_pair_2f8 = 0.0f;
}

typedef struct EntityTypeFourDispatch {
    int (*callback)(EntitySlot*, void*);
    void* payload;
} EntityTypeFourDispatch;

static int entity_type_four_dispatch_match(EntitySlot* e, void* payload)
{
    EntityTypeFourDispatch* dispatch = (EntityTypeFourDispatch*)payload;
    if (e == NULL || e->type != 4) {
        return 0;
    }
    return (dispatch->callback != NULL)
        ? dispatch->callback(e, dispatch->payload) : 1;
}

/* TypeFourEntityCallbackDispatch (FUN_00447900): the port replaces the
 * corpus's opaque stack callback thunk with an explicit predicate. It
 * retains the filtered entity-pool walk and applies the recovered type-4
 * gate before forwarding the caller payload. */
// FUNCTION: LEGORR 0x00447900
int entity_type_four_callback_dispatch(GameContext* ctx,
                                      int (*callback)(EntitySlot*, void*),
                                      void* payload)
{
    EntityTypeFourDispatch dispatch;
    dispatch.callback = callback;
    dispatch.payload = payload;
    return entity_pool_find_by_callback_filtered(
        ctx, entity_type_four_dispatch_match, &dispatch);
}

/* EntityEffectCountersApplyFromRecord (FUN_0042b2f0): the original's
 * two helper calls accumulate the record words at +4/+8 when state bit 8
 * is active. The port keeps those totals as explicit per-entity mirrors. */
// FUNCTION: LEGORR 0x0042b2f0
void entity_effect_counters_apply_from_record(EntitySlot* e,
                                              const void* record)
{
    if (e == NULL || record == NULL
        || (entity_state_flags_get(e) & 8u) == 0u) {
        return;
    }
    uint32_t add_a = 0u;
    uint32_t add_b = 0u;
    __builtin_memcpy(&add_a, (const char*)record + 4, sizeof(add_a));
    __builtin_memcpy(&add_b, (const char*)record + 8, sizeof(add_b));
    e->effect_counter_a += add_a;
    e->effect_counter_b += add_b;
}

/* EntityDamageApply (FUN_0043f6a0): preserve the recovered shared gate,
 * attached-object exclusion, x87 float-to-int health write, and positive
 * damage response bits. Sound, particle, and stats side effects remain
 * owned by their unported systems. */
// FUNCTION: LEGORR 0x0043f6a0
void entity_damage_apply(EntitySlot* e, float damage, int emit_effects)
{
    if (e == NULL || entity_shared_eligibility_check(e, 1) == 0
        || e->attach_304 != NULL) {
        return;
    }
    e->health = crt_float_to_long((float)e->health - damage);
    if (emit_effects != 0 && damage > 0.0f) {
        e->flags_3e4 |= 0x3000u;
    }
}

static int entity_linked_reference_matches(EntitySlot* candidate,
                                           void* target)
{
    if (candidate == NULL || target == NULL) {
        return 0;
    }
    if (candidate->link_2fc == (uintptr_t)target) {
        candidate->link_2fc = 0u;
        candidate->flags_3ec &= ~0x40u;
        return 1;
    }
    if (candidate->carry_link_308 == target) {
        candidate->carry_link_308 = NULL;
        return 1;
    }
    return 0;
}

/* EntityPoolFindWithLocalPredicate (FUN_0043f560): the original dispatches
 * a local predicate over the filtered entity pool during teardown and
 * state changes. Its LAB body is not included in the decompile corpus; the
 * port models the common reverse-link cleanup for both known entity links. */
// FUNCTION: LEGORR 0x0043f560
int entity_pool_find_with_local_predicate(GameContext* ctx,
                                          EntitySlot* target)
{
    if (target == NULL) {
        return 0;
    }
    return entity_pool_find_by_callback_filtered(
        ctx, entity_linked_reference_matches, target);
}

#define PENDING_TYPE_VALUE_BASE 0x00557864u
#define PENDING_TYPE_VALUE_COUNT 39u
static uint32_t s_pending_type_values[PENDING_TYPE_VALUE_COUNT];
static EntityCursorTargetHooks s_entity_cursor_target_hooks;

void pending_type_value_set(uint32_t original_address, uint32_t value)
{
    if (original_address < PENDING_TYPE_VALUE_BASE
        || ((original_address - PENDING_TYPE_VALUE_BASE) & 3u) != 0u) {
        return;
    }
    uint32_t index = (original_address - PENDING_TYPE_VALUE_BASE) / 4u;
    if (index < PENDING_TYPE_VALUE_COUNT) {
        s_pending_type_values[index] = value;
    }
}

uint32_t pending_type_value_get(uint32_t original_address)
{
    if (original_address < PENDING_TYPE_VALUE_BASE
        || ((original_address - PENDING_TYPE_VALUE_BASE) & 3u) != 0u) {
        return 0u;
    }
    uint32_t index = (original_address - PENDING_TYPE_VALUE_BASE) / 4u;
    return index < PENDING_TYPE_VALUE_COUNT
        ? s_pending_type_values[index] : 0u;
}

void entity_cursor_target_hooks_set(const EntityCursorTargetHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_entity_cursor_target_hooks, 0,
               sizeof(s_entity_cursor_target_hooks));
    }
    else {
        s_entity_cursor_target_hooks = *hooks;
    }
}

int entity_cursor_target_screen_position_get(const EntitySlot* entity,
                                             float out_xy[2])
{
    float point[3];
    if (entity == NULL || out_xy == NULL) return 0;
    if (s_entity_cursor_target_hooks.scene_position_get != NULL) {
        if (!s_entity_cursor_target_hooks.scene_position_get(
                s_entity_cursor_target_hooks.context, entity, point)) return 0;
    }
    else {
        point[0] = entity->x;
        point[1] = entity->y;
        point[2] = entity->z - entity_stats_float_at74(entity);
    }
    if (s_entity_cursor_target_hooks.point_project != NULL) {
        return s_entity_cursor_target_hooks.point_project(
            s_entity_cursor_target_hooks.context, point, out_xy);
    }
    if (s_entity_cursor_target_hooks.projection_adapter == NULL) return 0;
    {
        const ViewProjectionAdapter* adapter =
            (const ViewProjectionAdapter*)
                s_entity_cursor_target_hooks.projection_adapter;
        if (adapter->transform_point == NULL) return 0;
        camera_project_point_to_screen(adapter, out_xy, point);
    }
    return 1;
}

/* PendingTypeValueSlotSelect (FUN_00407a10): the config-fed DAT words are
 * zero-init model slots; this selection retains the exact flag precedence
 * and the command/activity families that choose among them. */
// FUNCTION: LEGORR 0x00407a10
void pending_type_value_slot_select(const EntitySlot* e, uint32_t* out)
{
    if (e == NULL || out == NULL) {
        return;
    }
    uint32_t address = 0u;
    if ((e->flags_3ec & 0x20u) != 0u) {
        address = 0x005578dcu;
    }
    else if ((e->flags_3e0 & 0x800000u) != 0u) {
        address = 0x005578d4u;
    }
    else if ((e->flags_3e0 & 0x80000000u) != 0u) {
        address = 0x00557864u;
    }
    else {
        const void* record = e->cmd_head;
        while (record != NULL && cmd_rec_type(record) == 0xe) {
            record = cmd_rec_next(record);
        }
        if (record == NULL) {
            return;
        }
        const int type = cmd_rec_type(record);
        switch (type) {
        case 0: address = 0x005578bcu; break;
        case 3: {
            const uint32_t* payload = (const uint32_t*)cmd_rec_payload(record);
            if (payload == NULL) break;
            switch (payload[0]) {
            case 6: address = 0x0055786cu; break;
            case 7: address = payload[1] != 0u
                ? 0x00557874u : 0x00557870u; break;
            case 8: address = 0x00557878u; break;
            case 9: address = 0x0055787cu; break;
            case 11: address = 0x00557880u; break;
            default: break;
            }
            break;
        }
        case 5: {
            const EntitySlot* linked = (const EntitySlot*)e->carry_link_308;
            if (linked == NULL) break;
            switch (linked->type) {
            case 6: address = 0x005578a4u; break;
            case 7: address = linked->etype != 0
                ? 0x005578acu : 0x005578a8u; break;
            case 8: address = 0x005578b0u; break;
            case 9: address = 0x005578b4u; break;
            case 11: address = 0x005578b8u; break;
            default: break;
            }
            break;
        }
        case 7: address = 0x005578fcu; break;
        case 8: address = 0x005578c8u; break;
        case 9: address = 0x005578c0u; break;
        case 10: address = 0x005578ccu; break;
        case 11: address = 0x005578c4u; break;
        case 12: address = 0x005578d0u; break;
        case 15: address = 0x005578e0u; break;
        case 16: case 17: address = 0x005578e4u; break;
        case 18: address = 0x005578e8u; break;
        case 19: {
            const int activity = cmd_rec_activity(record);
            if (activity >= 0 && activity <= 7) {
                address = 0x00557884u + (uint32_t)activity * 4u;
                if (activity == 6) address = 0x005578a0u;
                if (activity == 7) address = 0x0055789cu;
            }
            break;
        }
        case 21: address = 0x005578ecu; break;
        case 22: address = 0x005578f0u; break;
        case 23: address = 0x005578f4u; break;
        case 27: address = 0x005578f8u; break;
        default: break;
        }
    }
    if (address != 0u) {
        *out = pending_type_value_get(address);
    }
}

/* ResetObjectAuxiliaryState (FUN_00407370): the global refresh gate clears
 * the auxiliary word then refreshes its selected per-entity value slot. */
// FUNCTION: LEGORR 0x00407370
void entity_reset_auxiliary_state(EntitySlot* e)
{
    if (e == NULL || global_task_refresh_flag_get() == 0u) {
        return;
    }
    e->auxiliary_state_388 = 0.0f;
    pending_type_value_slot_select(e, &e->auxiliary_slot_38c);
}

static EntityPayloadReleaseFn s_payload_scene_release_hook;
static EntityPayloadReleaseFn s_payload_chain_release_hook;

void entity_payload_release_hooks_set(EntityPayloadReleaseFn scene_release,
                                      EntityPayloadReleaseFn chain_release)
{
    s_payload_scene_release_hook = scene_release;
    s_payload_chain_release_hook = chain_release;
}

/* SceneEntitySlotTableRebuild (FUN_004083a0): the original clears three
 * cache bands and rebuilds them from an attached scene-object chain. The
 * port models the cache invalidation/generation change; the scene chain
 * owner remains behind the resolver/release hooks. */
// FUNCTION: LEGORR 0x004083a0
void entity_scene_slot_table_rebuild(EntitySlot* e, int mode)
{
    (void)mode;
    if (e == NULL) {
        return;
    }
    for (int i = 0; i < 12; ++i) {
        e->scene_slot_cache[i] = NULL;
    }
    e->scene_slot_generation++;
}

/* EntityPayloadStateSet (FUN_004089a0): store the payload state and rebuild
 * its scene slot cache. */
// FUNCTION: LEGORR 0x004089a0
void entity_payload_state_set(EntitySlot* e, int state)
{
    if (e == NULL) {
        return;
    }
    e->payload_state_8c = state;
    entity_scene_slot_table_rebuild(e, 0);
}

/* EntityPayloadReleaseSubslots (FUN_00406ec0): release the three owned
 * reference chains, conditionally free the five auxiliary blocks under
 * flag bit 0, then release the primary scene object. */
// FUNCTION: LEGORR 0x00406ec0
void entity_payload_release_subslots(EntitySlot* e)
{
    if (e == NULL) {
        return;
    }
    if (s_payload_chain_release_hook != NULL) {
        for (int i = 0; i < 3; ++i) {
            if (e->payload_resource_chains[i] != NULL) {
                s_payload_chain_release_hook(e->payload_resource_chains[i]);
            }
        }
    }
    if ((e->payload_flags_70 & 1u) != 0u) {
        for (int i = 0; i < 5; ++i) {
            if (e->payload_owned_blocks[i] != NULL) {
                engine_heap_free(e->payload_owned_blocks[i]);
                e->payload_owned_blocks[i] = NULL;
            }
        }
    }
    if (e->payload_handle_04 != NULL && s_payload_scene_release_hook != NULL) {
        s_payload_scene_release_hook(e->payload_handle_04);
    }
    e->payload_handle_04 = NULL;
}

/* EntityPayloadTeardown (FUN_00408940): set state to -1, then release the
 * optional secondary and primary scene handles through the owner hook. */
// FUNCTION: LEGORR 0x00408940
void entity_payload_teardown(EntitySlot* e)
{
    if (e == NULL) {
        return;
    }
    entity_payload_state_set(e, -1);
    if (e->payload_aux_handle_7c != NULL
        && s_payload_scene_release_hook != NULL) {
        s_payload_scene_release_hook(e->payload_aux_handle_7c);
    }
    e->payload_aux_handle_7c = NULL;
    entity_payload_release_subslots(e);
}

void entity_heading_step_move(EntityHeadingStepState* state, void* entity,
                              float delta,
                              const EntityHeadingStepHooks* hooks)
{
    float step_x, step_y, target[2], length;
    int cell_x, cell_y;
    int can_step = 0;
    int moved = 0;
    if (state == NULL || hooks == NULL || hooks->position_to_cell == NULL ||
        hooks->cell_step_allowed == NULL) return;
    step_x = state->direction[0] * delta;
    step_y = state->direction[1] * delta;
    target[0] = state->position[0] + step_x;
    target[1] = state->position[1] + step_y;
    if (hooks->position_to_cell(hooks->context, target[0], target[1],
                                &cell_x, &cell_y))
        can_step = hooks->cell_step_allowed(hooks->context, entity,
                                           cell_x, cell_y);
    if (can_step) {
        if (hooks->position_transition == NULL ||
            !hooks->position_transition(hooks->context, entity, target)) {
            moved = 1;
        }
    }
    if (!moved && (!can_step || hooks->position_transition != NULL)) {
        length = (float)sqrt((double)(state->direction[0] * state->direction[0] +
                                      state->direction[1] * state->direction[1]));
        if (length > 0.0f) {
            float magnitude = length;
            float nx = state->direction[0] / length;
            float ny = state->direction[1] / length;
            if (nx > 0.5f) { nx = 1.0f; ny = 0.0f; }
            else if (nx < -0.5f) { nx = -1.0f; ny = 0.0f; }
            else if (ny > 0.5f) { nx = 0.0f; ny = 1.0f; }
            else { nx = 0.0f; ny = -1.0f; }
            state->direction[0] = nx * magnitude;
            state->direction[1] = ny * magnitude;
        }
        step_x = state->direction[0] * delta;
        step_y = state->direction[1] * delta;
        target[0] = state->position[0] + step_x;
        target[1] = state->position[1] + step_y;
        can_step = 0;
        if (hooks->position_to_cell(hooks->context, target[0], target[1],
                                    &cell_x, &cell_y))
            can_step = hooks->cell_step_allowed(hooks->context, entity,
                                                cell_x, cell_y);
        if (can_step && (hooks->position_transition == NULL ||
            !hooks->position_transition(hooks->context, entity, target)))
            moved = 1;
        else if (!can_step || hooks->position_transition != NULL)
            state->flags &= ~0x4000u;
    }
    if (moved) {
        state->position[0] = target[0];
        state->position[1] = target[1];
        if (hooks->position_set != NULL)
            hooks->position_set(hooks->context, entity, target);
    }
    if (state->remaining <= 0.0f) state->flags &= ~0x4000u;
    state->remaining -= (float)sqrt((double)(step_x * step_x + step_y * step_y));
}

// GLOBAL: LEGORR 0x004eadd0
static EntityAuxiliaryReleaseRecord* s_entity_auxiliary_release_records;

void entity_auxiliary_payload_release(
    EntitySlot* entity, const EntityAuxiliaryReleaseHooks* hooks)
{
    EntityAuxiliaryReleaseRecord* record;
    void* scene_object;
    uint32_t flags;
    float point[3] = { 0.0f, 0.0f, 0.0f };
    if (entity == NULL || hooks == NULL || hooks->scene_object_get == NULL)
        return;
    scene_object = hooks->scene_object_get(hooks->context, entity);
    if (scene_object == NULL) return;
    flags = entity->flags_3e4;
    if ((flags & 0x08000000u) != 0u) {
        if (hooks->scene_object_dispose != NULL)
            hooks->scene_object_dispose(hooks->context, scene_object);
        return;
    }
    if (hooks->record_allocate != NULL)
        record = hooks->record_allocate(hooks->context);
    else
        record = (EntityAuxiliaryReleaseRecord*)engine_heap_alloc(
            (long)sizeof(*record));
    if (record == NULL) {
        if (hooks->scene_object_dispose != NULL)
            hooks->scene_object_dispose(hooks->context, scene_object);
        return;
    }
    memset(record, 0, sizeof(*record));
    record->scene_object = scene_object;
    record->next = s_entity_auxiliary_release_records;
    record->flags = flags;
    record->value = hooks->value_get != NULL
        ? hooks->value_get(hooks->context, entity) : 0.0f;
    if (hooks->scene_point_get != NULL)
        hooks->scene_point_get(hooks->context, scene_object, point);
    s_entity_auxiliary_release_records = record;
    if (hooks->scene_callback_apply != NULL) {
        hooks->scene_callback_apply(hooks->context, scene_object,
                                    0x00410020u, point);
        hooks->scene_callback_apply(hooks->context, scene_object,
                                    0x004100b0u, point);
    }
}

void entity_auxiliary_release_records_flush(
    const EntityAuxiliaryReleaseHooks* hooks)
{
    EntityAuxiliaryReleaseRecord** link = &s_entity_auxiliary_release_records;
    while (*link != NULL) {
        EntityAuxiliaryReleaseRecord* record = *link;
        int ready = hooks == NULL || hooks->scene_callback_ready == NULL ||
            hooks->scene_callback_ready(hooks->context, record->scene_object);
        if (!ready) {
            link = &record->next;
            continue;
        }
        if (hooks != NULL && hooks->scene_object_dispose != NULL)
            hooks->scene_object_dispose(hooks->context, record->scene_object);
        *link = record->next;
        if (hooks != NULL && hooks->record_release != NULL)
            hooks->record_release(hooks->context, record);
        else
            engine_heap_free(record);
    }
}

void entity_motion_to_scene_node_sync(
    EntitySlot* entity, float delta, const EntityMotionSceneSyncHooks* hooks)
{
    void* record;
    void* scene_object;
    float position[3] = { 0.0f, 0.0f, 0.0f };
    float callback_values[4];
    uint32_t flags, modes;
    int doubled;
    if (entity == NULL || hooks == NULL) return;
    flags = entity->flags_3e0;
    if ((flags & 0x200000u) == 0u) return;
    record = hooks->scene_record_get != NULL
        ? hooks->scene_record_get(hooks->context, entity) : NULL;
    scene_object = hooks->scene_object_get != NULL
        ? hooks->scene_object_get(hooks->context, entity)
        : entity->attached_scene_object;
    if ((flags & 0x400000u) == 0u &&
        (hooks->scene_should_wake == NULL ||
         !hooks->scene_should_wake(hooks->context, scene_object)) &&
        hooks->wake_transition != NULL)
        hooks->wake_transition(hooks->context, entity);
    if (record != NULL && hooks->record_timer_active != NULL &&
        hooks->record_timer_active(hooks->context, record) &&
        hooks->record_timer_advance != NULL)
        hooks->record_timer_advance(hooks->context, record, delta);
    if (record != NULL && hooks->entity_timer_get != NULL &&
        hooks->entity_timer_get(hooks->context, entity) <= delta &&
        (entity->flags_3e4 & 0x80u) == 0u) {
        if (hooks->entity_action != NULL)
            hooks->entity_action(hooks->context, entity, 1, 0, 1);
        if (hooks->record_timer_clear != NULL)
            hooks->record_timer_clear(hooks->context, record);
    }
    if (hooks->scene_position_get != NULL) {
        hooks->scene_position_get(hooks->context, entity, position);
    }
    else if (scene_object != NULL &&
             ddiscene_object_position_get(scene_object, NULL, position)) {
        /* The renderer owns this transform. Keep the simulation mirror in
         * sync when no type-specific pose adapter was installed. */
        entity->x = position[0];
        entity->y = position[1];
        entity->z = position[2];
    }
    else {
        position[0] = entity->x;
        position[1] = entity->y;
        position[2] = entity->z;
    }
    callback_values[0] = position[0];
    callback_values[1] = position[1];
    callback_values[2] = -position[2];
    callback_values[3] = delta;
    if (record != NULL && (hooks->scene_object_registered == NULL ||
        !hooks->scene_object_registered(hooks->context, record)) &&
        hooks->scene_callback_apply != NULL) {
        hooks->scene_callback_apply(hooks->context, scene_object,
                                    0x00410020u, callback_values);
        hooks->scene_callback_apply(hooks->context, scene_object,
                                    0x00410070u, callback_values);
        hooks->scene_callback_apply(hooks->context, scene_object,
                                    0x004100b0u, callback_values);
    }
    doubled = (entity->flags_3e4 & 0x80u) != 0u;
    entity_node_scale_update(entity, doubled);
    if (doubled) entity->flags_3e4 &= ~0x80u;
    if (hooks->record_transform_apply != NULL && record != NULL) {
        hooks->record_transform_apply(hooks->context, record, 0x0040fdb0u,
                                      position);
        hooks->record_transform_apply(hooks->context, record, 0x0040ff50u,
                                      entity->velocity_x == 0.0f ? position :
                                      &entity->velocity_x);
        hooks->record_transform_apply(hooks->context, record, 0x0040fe00u,
                                      &delta);
    }
    modes = ((flags >> 23) & 1u) | (((flags >> 26) & 1u) << 1) |
            (((flags >> 28) & 1u) << 2);
    if (hooks->periodic_callback != NULL)
        hooks->periodic_callback(hooks->context, entity, delta,
                                 entity->velocity_z, modes & 1u,
                                 (modes >> 1) & 1u, (modes >> 2) & 1u);
}

unsigned int entity_stats_config_load(
    void* config, void* section, const EntityStatsConfigHooks* hooks)
{
    static const char* fields[65] = {
        "AnimateByPitch", "CanBeShotAt", "ScaredByBigBangs", "UseWaterTeleporter",
        "UseSmallTeleporter", "UseBigTeleporter", "UpgradeBuilding", "Smooth",
        "OnGround", "CrossLand", "CrossWater", "BumpDamage", "Debug", "Flocks",
        "RouteAvoidance", "CanSteal", "CanBeDriven", "GrabMinifigure",
        "ScaredByPlayer", "RandomEnterWall", "CanScareScorpion", "CanScare",
        "RandomMove", "CauseSlip", "SnaxULike", "ToolStore", "WaterTeleporter",
        "SmallTeleporter", "BigTeleporter", "ManTeleporter", "ProcessCrystal",
        "ProcessOre", "StoreObjects", "NumOfToolsCanCarry", "FunctionCoef",
        "UpgradeCostOre", "UpgradeCostStuds", "CrystalDrain", "CarryStart",
        "MaxCarry", "SurveyRadius", "RepairValue", "SingleWidthDig", "CollBox",
        "CollRadius", "Capacity", "OxygenCoef", "PathCoef", "RubbleCoef",
        "WaterEntrances", "AttackRadius", "StampRadius", "CarryMinHealth",
        "RestPercent", "AlertRadius", "CollHeight", "HealthDecayRate",
        "EnergyDecayRate", "PickSphere", "DrillSound", "DrillFadeSound",
        "EngineSound", "RouteSpeed", "SeamDrillTime", "HardDrillTime"
    };
    unsigned int count, i, committed = 0;
    if (hooks == NULL || hooks->entry_count == NULL ||
        hooks->entry_name == NULL || hooks->type_row_resolve == NULL ||
        hooks->float_value_get == NULL) return 0;
    count = hooks->entry_count(hooks->context, config, section);
    for (i = 0; i < count; ++i) {
        EntityStatsConfigRecord record;
        const char* name = hooks->entry_name(hooks->context, config, section, i);
        unsigned int field, level, capacity;
        size_t length = 0;
        if (name == NULL) continue;
        memset(&record, 0, sizeof(record));
        while (length < sizeof(record.name) - 1u && name[length] != '\0') {
            record.name[length] = name[length];
            ++length;
        }
        if (!hooks->type_row_resolve(hooks->context, record.name,
                                     &record.type, &record.row)) continue;
        capacity = hooks->type_capacity != NULL
            ? hooks->type_capacity(hooks->context, record.type) : 16u;
        if (capacity > 16u) capacity = 16u;
        record.level_count = capacity;
        for (field = 0; field < 65u; ++field)
            hooks->float_value_get(hooks->context, record.name, fields[field],
                                   0, &record.fields[field]);
        for (level = 0; level < capacity; ++level)
            hooks->float_value_get(hooks->context, record.name, "Levels",
                                   level, &record.levels[level]);
        if (hooks->record_commit != NULL)
            hooks->record_commit(hooks->context, &record);
        ++committed;
    }
    return committed;
}

/* --- object pointer registry (see entity.hpp) ------------------------------
 * The corpus's ten-slot table (DAT pair at 0x5563b8 / 0x5563e0 as the
 * count): zero-initialized statics, no ported feeder. */
// GLOBAL: LEGORR 0x005563b8
static int32_t  s_obj_ptr_slots[10];
// GLOBAL: LEGORR 0x005563e0
static uint32_t s_obj_ptr_count = 0u;

// FUNCTION: LEGORR 0x0042dfc0
void object_pointer_registry_clear_match(int32_t pointer)
{
    for (uint32_t i = 0; i < s_obj_ptr_count && i < 10u; ++i) {
        if (s_obj_ptr_slots[i] == pointer) {
            s_obj_ptr_slots[i] = 0;
        }
    }
}

// FUNCTION: LEGORR 0x0042dff0
int object_pointer_registry_get_slot(uint32_t index, int32_t* out)
{
    if (index >= s_obj_ptr_count) {
        return 0; /* deviation: the corpus reads blindly past the count */
    }
    const int32_t v = s_obj_ptr_slots[index];
    if (out != NULL) {
        *out = v;
    }
    return v != 0 ? 1 : 0;
}

uint32_t object_pointer_registry_count_peek(void)
{
    return s_obj_ptr_count;
}

/* --- context-selected globals (see entity.hpp) -----------------------------
 * The corpus reads the context-mode global (DAT_00556ec8) and the
 * current-selection pointer (DAT_00556160); both are modeled in the media
 * layer (the port's own models, fed by SoundMinimumDistanceUpdateMode).
 * The DAT_005560f8 / DAT_00556104 pair has no ported feeder — two
 * zero-initialized statics (documented deviation). */
static uint32_t s_ctx_sel_ptr_first  = 0u; /* DAT_005560f8 */
static uint32_t s_ctx_sel_ptr_second = 0u; /* DAT_00556104 */

void context_selected_pointer_pair_set(uint32_t first, uint32_t second)
{
    s_ctx_sel_ptr_first = first;
    s_ctx_sel_ptr_second = second;
}

uint32_t context_selected_pointer_pair_peek(int second)
{
    return (second != 0) ? s_ctx_sel_ptr_second : s_ctx_sel_ptr_first;
}

// FUNCTION: LEGORR 0x004295a0
uint32_t context_selected_global_pointer_get(void)
{
    return (media_min_distance_mode() != 0) ? s_ctx_sel_ptr_second
                                           : s_ctx_sel_ptr_first;
}

// FUNCTION: LEGORR 0x004295c0
int entity_matches_current_selection_or_owner(const void* entity)
{
    if (entity == NULL) {
        return 0;
    }
    const void* selected = media_min_distance_context();
    const int mode = media_min_distance_mode();
    void* owner = NULL;
    __builtin_memcpy(&owner, (const char*)entity + 0x36c, sizeof(owner));
    /* corpus shape: match (mode-clear && entity == selection) or
     * (owner != NULL && owner == selection); reject otherwise. */
    const int direct_ok = (mode == 0) && (entity == selected);
    const int owner_ok = (owner != NULL) && (owner == selected);
    return (direct_ok || owner_ok) ? 1 : 0;
}

/* SelectionCandidateEligibilityCheck (FUN_00452870, findings 77/85/468):
 * corpus order kept — state gate, shared predicate, then the clear-bit
 * band; the byte-read of +0x3e4's low byte (& 10 decimal) maps to the
 * flag-word mask 0xA. */
// FUNCTION: LEGORR 0x00452870
int selection_candidate_eligibility_check(const EntitySlot* e)
{
    if (e == NULL) {
        return 0; /* deviation: the corpus dereferences blindly */
    }
    if ((e->flags_3e8 & 0x100u) == 0u) {
        return 0;
    }
    if (entity_shared_eligibility_check(e, 1) == 0) {
        return 0;
    }
    if ((e->flags_3e4 & 0xAu) != 0u) { /* corpus: low byte & 10 (decimal) */
        return 0;
    }
    if ((e->flags_3e0 & 0x24000000u) != 0u) {
        return 0;
    }
    if (e->type == 1 && (e->flags_3e0 & 0x00040000u) != 0u) {
        return 0; /* corpus: `*param_1 != 1 || (word & 0x40000)` — type 1
                   * has the extra +0x3e0 bit-0x40000 rejection (finding
                   * 468); every other type ignores the branch */
    }
    return 1;
}

/* EntityFlag40000ThresholdCompare (see entity.hpp). */
// FUNCTION: LEGORR 0x00437cc0
int entity_flag40000_threshold_compare(const EntitySlot* e)
{
    if ((entity_state_flags_get(e) & 0x40000u) != 0u) {
        if (entity_stats_float_ate8(e) < e->field_340) {
            return 1;
        }
    }
    return 0;
}

/* EntityFlagsContainRequestedSingleBit (FUN_00438ed0, findings 327/449).
 * The corpus re-reads the word at each chain rung; the selector arms and
 * the flag bits pair one-to-one, so one read through the switch is the
 * identical answer. */
// FUNCTION: LEGORR 0x00438ed0
int entity_flags_contain_requested_single_bit(const EntitySlot* e,
                                              int selector)
{
    if ((entity_state_flags_get(e) & 0x04u) == 0u || selector != 1) {
        if ((entity_state_flags_get(e) & 0x08u) == 0u || selector != 2) {
            if ((entity_state_flags_get(e) & 0x10u) == 0u || selector != 4) {
                if ((entity_state_flags_get(e) & 0x20u) == 0u || selector != 8) {
                    if ((entity_state_flags_get(e) & 0x40u) == 0u
                        || selector != 0x10) {
                        if ((entity_state_flags_get(e) & 0x80u) == 0u
                            || selector != 0x20) {
                            return 0;
                        }
                    }
                }
            }
        }
    }
    return 1;
}

// FUNCTION: LEGORR 0x00439cf0
uint32_t entity_stats_flag10_test(const EntitySlot* e)
{
    return entity_stats_word_get(e) & 0x10u;
}

// FUNCTION: LEGORR 0x00439d10
uint32_t entity_stats_flag20_test(const EntitySlot* e)
{
    return entity_stats_word_get(e) & 0x20u;
}

// FUNCTION: LEGORR 0x00439d30
uint32_t entity_stats_flag40_test(const EntitySlot* e)
{
    return entity_stats_word_get(e) & 0x40u;
}

/* GlobalAnimationStateSet (FUN_004383a0) — see entity.hpp. */
// GLOBAL: LEGORR 0x004eadc4
static uint32_t s_global_animation_state; /* DAT_004eadc4 */

// FUNCTION: LEGORR 0x004383a0
void entity_global_animation_state_set(uint32_t value)
{
    s_global_animation_state = value;
}

uint32_t entity_global_animation_state_peek(void)
{
    return s_global_animation_state;
}

/* EntityFlag100AndSharedEligibilityCheck (FUN_00471ce0) — see entity.hpp. */
// FUNCTION: LEGORR 0x00471ce0
int entity_flag100_and_shared_eligibility_check(const EntitySlot* e)
{
    /* Corpus shape: both gates jump to the shared zero tail, only the
     * double pass answers 1. */
    if ((entity_state_flags_get(e) & 0x100u) != 0u) {
        if (entity_shared_eligibility_check(e, e->type != 4) != 0) {
            return 1;
        }
    }
    return 0;
}

/* EntityQuadMeshRenderRecordAllowed (FUN_0040aae0) — see entity.hpp. */
// FUNCTION: LEGORR 0x0040aae0
int entity_quad_mesh_render_record_allowed(const EntitySlot* e)
{
    if ((entity_state_flags_get(e) & 0x08000000u) == 0u && e->type != 8
        && e->type != 13) {
        return 1;
    }
    return 0;
}

/* TaskTypeAndConditionCheck (FUN_0043c490) — see entity.hpp. */
// FUNCTION: LEGORR 0x0043c490
int task_type_and_condition_check(const void* unused,
                                  const EntitySlot* target, int condition)
{
    (void)unused;
    if (entity_shared_eligibility_check(target, 1) == 0 || condition == 0) {
        return 0;
    }
    return 1;
}

/* EntityTypeAndStateFlagPredicate (FUN_0045e400) — see entity.hpp. */
// FUNCTION: LEGORR 0x0045e400
int entity_type_and_state_flag_predicate(const EntitySlot* e)
{
    if (e == NULL) {
        return 0; /* deviation: the corpus strides blindly */
    }
    const int type = e->type;
    if (type != 0xe && type != 5 && type != 8 && type != 0xd
        && type != 0x10 && type != 0x11 && type != 0x13
        && type != 9 && type != 0x12 && type != 4 && type != 0xc) {
        return ((~entity_state_flags_get(e)) >> 29) & 1u;
    }
    return 0;
}

/* ObjectFilterMatchesActionTarget (FUN_00404f50) — see entity.hpp. The
 * identity pair (+0x20/+0x24/+0x28) stays dword; only the +0x10 entity
 * handle widens for the host pointer it dereferences. */
static uint32_t filt_dword(const void* rec, size_t off)
{
    uint32_t v = 0;
    if (rec != NULL) {
        __builtin_memcpy(&v, (const char*)rec + off, sizeof v);
    }
    return v;
}

// FUNCTION: LEGORR 0x00404f50
int object_filter_matches_action_target(const void* filter_rec,
                                        const void* target_rec)
{
    if (filter_rec == NULL || target_rec == NULL) {
        return 0; /* deviation: the corpus strides both records blindly */
    }
    const uint32_t type = filt_dword(filter_rec, 0x00);
    if (type != 5u) {
        if (type != 7u && type != 9u && type != 0xfu) {
            return 0;
        }
        uintptr_t handle = 0;
        __builtin_memcpy(&handle, (const char*)filter_rec + 0x10,
                         sizeof handle);
        if (handle != 0
            && entity_shared_eligibility_check((const EntitySlot*)handle, 0)
                   == 0) {
            return 0;
        }
    }
    if (filt_dword(filter_rec, 0x20) != 0u) {
        const uint32_t selector = filt_dword(filter_rec, 0x24);
        if (filt_dword(target_rec, 0x00) != filt_dword(filter_rec, 0x20)
            || (selector != 0xFFFFFFFFu
                && filt_dword(target_rec, 0x04) != selector)
            || filt_dword(target_rec, 0x08) != filt_dword(filter_rec, 0x28)) {
            return 0;
        }
    }
    return 1;
}

/* EntityDetachCarriedObjectAndResetState (FUN_0043bbc0) — see entity.hpp.
 * The carried half is read through the link as a native slot pointer; the
 * corpus strides its dword words blindly. */
// FUNCTION: LEGORR 0x0043bbc0
void entity_detach_carried_object_and_reset_state(EntitySlot* e,
                                                  LevelState* lvl)
{
    if (e == NULL) {
        return; /* deviation: the corpus dereferences the slot blindly */
    }
    if (e->type != 3) {
        return;
    }
    EntitySlot* carried = (EntitySlot*)e->carry_link_308;
    if (carried != NULL && carried->type == 6) {
        e->carry_link_308 = NULL;
        e->carry_state_324 = 0;
        carried->attach_304 = NULL;
        e->flags_3e0 &= ~0x400u;
        carried->field_340 = -1.0f;
    }
    if (e->carry_ref_348 != NULL) {
        level_state_field9c_add(lvl, *(const int*)e->carry_ref_348);
    }
    e->field_340 = -1.0f;
    e->flags_3e8 |= 0x800000u;
}

/* EntityClearLinkedObjectState (FUN_00447780) — see entity.hpp. */
// FUNCTION: LEGORR 0x00447780
void entity_clear_linked_object_state(EntitySlot* e)
{
    if (e == NULL) {
        return; /* deviation: the corpus strides the slot blindly */
    }
    if ((e->flags_3e0 & 0x400u) == 0u) {
        return;
    }
    EntitySlot* carried = (EntitySlot*)e->carry_link_308;
    e->flags_3e0 &= ~0x400u;
    e->carry_state_324 = 0;
    e->carry_link_308 = NULL;
    if (carried != NULL) {
        carried->flags_3e8 |= 0x2000u;
    }
    cmd_chain_drop(e, 4);
}

/* --- the pooled-record chunk window + resolved dispatch band (2026-10-25) -- */

/* PooledRecordChunkAllocateAndLink (FUN_00438310) — see entity.hpp. The
 * corpus grows a power-of-two chunk of 0x40c-stride entity records, threads
 * them as a free list whose last link joins the previous head, and tracks
 * the chunk table with a byte counter. Deviations: the port records widen
 * the link slot (stride 0x410 where the corpus strides 0x40c, the
 * port-pointer convention) and the chunk table is bounded 32 entries where
 * the corpus strides it blindly. */

#define POOL_CHUNK_COUNT_SLOTS 32 /* deviation: bounded chunk table */
#define POOL_RECORD_STRIDE 0x410  /* corpus 0x40c widened for the link */
#define POOL_RECORD_LINK_OFF 0x408

// GLOBAL: LEGORR 0x004dee10
static uint32_t s_pool_chunk_count;
// GLOBAL: LEGORR 0x004de790
static void* s_pool_chunks[POOL_CHUNK_COUNT_SLOTS];
// GLOBAL: LEGORR 0x004de810
static void* s_pool_record_head;

// FUNCTION: LEGORR 0x00438310
void pooled_record_chunk_allocate_and_link(void)
{
    const uint32_t n =
        1u << ((uint32_t)(unsigned char)s_pool_chunk_count & 0x1fu);
    void* base = engine_heap_alloc((long)n * POOL_RECORD_STRIDE);
    if (base == NULL) {
        return; /* deviation: the corpus strides the allocator's word */
    }
    if (s_pool_chunk_count < POOL_CHUNK_COUNT_SLOTS) {
        s_pool_chunks[s_pool_chunk_count] = base;
    }
    s_pool_chunk_count = s_pool_chunk_count + 1u;
    char* cur = (char*)base;
    for (uint32_t k = 1; k < n; ++k) {
        void* next = cur + POOL_RECORD_STRIDE;
        __builtin_memcpy(cur + POOL_RECORD_LINK_OFF, &next, sizeof next);
        cur += POOL_RECORD_STRIDE;
    }
    __builtin_memcpy(cur + POOL_RECORD_LINK_OFF, &s_pool_record_head,
                     sizeof(s_pool_record_head));
    s_pool_record_head = base;
}

void* pooled_record_head_peek(void) { return s_pool_record_head; }
uint32_t pooled_record_chunk_count_peek(void) { return s_pool_chunk_count; }

/* PooledRecordAllocateAndReset (FUN_004382c0) — see entity.hpp. The
 * corpus pops the free-list head, clears 0x103 dwords, self-links the
 * link slot as its busy mark, and seeds the two 0xbf800000 float words
 * (the corpus's -1.5 pair). Deviations: the port's link slot is
 * native-size (the pool-stride convention), and the grow half answers
 * NULL when the allocator is empty where the corpus strides the word. */
// FUNCTION: LEGORR 0x004382c0
void* pooled_record_allocate_and_reset(void)
{
    if (s_pool_record_head == NULL) {
        pooled_record_chunk_allocate_and_link();
    }
    char* record = (char*)s_pool_record_head;
    if (record == NULL) {
        return NULL; /* deviation: empty allocator (corpus strides) */
    }
    void* link = NULL;
    __builtin_memcpy(&link, record + POOL_RECORD_LINK_OFF, sizeof link);
    s_pool_record_head = link;
    memset(record, 0, 0x40cu); /* the corpus's 0x103-dword clear */
    __builtin_memcpy(record + POOL_RECORD_LINK_OFF, &record,
                     sizeof record); /* the busy self-link */
    const uint32_t seeded = 0xbf800000u;
    __builtin_memcpy(record + 0x2f4, &seeded, 4);
    __builtin_memcpy(record + 0x2f8, &seeded, 4);
    return record;
}

/* EntityPoolResourcesRelease (FUN_00437050) frees the tracked entity-record
 * chunks and resets the pool's chunk table, head, and generation count. The
 * reference also walks entity records before freeing an adjacent resource
 * array; the portable entity walk is owned by the game context, so this
 * module releases only the allocator-owned blocks it tracks here. */
// FUNCTION: LEGORR 0x00437050
void pooled_record_pool_resources_release(void)
{
    uint32_t i;
    for (i = 0; i < POOL_CHUNK_COUNT_SLOTS; ++i) {
        if (s_pool_chunks[i] != NULL) {
            engine_heap_free(s_pool_chunks[i]);
            s_pool_chunks[i] = NULL;
        }
    }
    s_pool_record_head = NULL;
    s_pool_chunk_count = 0;
}

/* ResolvedEntityOperationDispatch (FUN_00460910) — see entity.hpp. */
// FUNCTION: LEGORR 0x00460910
int resolved_entity_operation_dispatch(const char* name, int slot,
                                       int subtract)
{
    /* Corpus shape: one local (the type slot, `push ecx`) and the resolver's
     * row output aliased onto this function's first-parameter stack slot
     * (`lea eax,[esp+0x8]` at 0x460915), so the resolver's entry zeroing
     * clears it for special names while table/ore matches write the row
     * there, and the post-resolver count scan reads its column word back
     * out of the name parameter (`mov edx,[esp+0x8]` at 0x460936). The
     * corpus's fourth resolver word is the immediate 0 (its NULL-guarded
     * tvcamera store never fires from here). */
    int type;
    if (entity_type_pair_for_name(name, &type, (int*)&name, NULL)) {
        return grid_record_count_difference_get(type, *(int*)&name, slot,
                                                subtract);
    }
    return 0;
}

/* --- DAT_00502bd8 indexed stats-type tables (2026-10-25) ------------------
 * The corpus reads `*(row_table[type][row])` through DAT_00502bd8's pointer
 * tables and strides the template record blindly; the config loader that
 * fills the tables is unported, so the port keeps guarded own storage and
 * guards every stride (deviations documented per the DAT-window convention).
 * The corpus float fields read as x87 float10 of the stored float; the port
 * hands back the plain float. */

static const void* s_stats_cell_ptr[STATS_TYPE_TABLE_TYPES][STATS_TYPE_TABLE_ROWS];
static int s_stats_cell_span[STATS_TYPE_TABLE_TYPES][STATS_TYPE_TABLE_ROWS];

void stats_type_table_install(int type, int row, const void* record,
                              int stride_count)
{
    if (type < 0 || type >= STATS_TYPE_TABLE_TYPES
        || row < 0 || row >= STATS_TYPE_TABLE_ROWS) {
        return; /* deviation: the corpus strides its tables blindly */
    }
    if (stride_count < 0) {
        stride_count = 0;
    }
    s_stats_cell_ptr[type][row] = stride_count > 0 ? record : NULL;
    s_stats_cell_span[type][row] = stride_count;
}

const void* stats_type_table_cell(int type, int row)
{
    if (type < 0 || type >= STATS_TYPE_TABLE_TYPES
        || row < 0 || row >= STATS_TYPE_TABLE_ROWS) {
        return NULL; /* deviation: the corpus strides blindly */
    }
    return s_stats_cell_ptr[type][row];
}

/* BuildingUpgradeAdmissionCheck (FUN_00469f40) reads the raw word stored
 * beside the stats table. The loader writes the parsed integer as a float;
 * callers then consume that float's bits through an integer table read. */
// FUNCTION: LEGORR 0x00469f40
uint32_t building_upgrade_admission_check(int type, int row)
{
    if (type < 0 || type >= STATS_TYPE_TABLE_TYPES
        || row < 0 || row >= STATS_TYPE_TABLE_ROWS) {
        return 0u; /* deviation: the corpus indexes the table blindly */
    }
    const float level_count = (float)s_stats_cell_span[type][row];
    uint32_t raw_word = 0u;
    __builtin_memcpy(&raw_word, &level_count, sizeof(raw_word));
    return raw_word;
}

static uint32_t stats_table_read(int type, int row, int off, int stride_index)
{
    if (type < 0 || type >= STATS_TYPE_TABLE_TYPES
        || row < 0 || row >= STATS_TYPE_TABLE_ROWS || stride_index < 0) {
        return 0u; /* deviation: the corpus strides the index blindly */
    }
    const void* base = s_stats_cell_ptr[type][row];
    if (base == NULL) {
        return 0u; /* empty cell = the zero template */
    }
    const int64_t bound = (int64_t)s_stats_cell_span[type][row] * 0x150;
    const int64_t off64 = (int64_t)off + (int64_t)stride_index * 0x150;
    if (off64 + 4 > bound) {
        return 0u; /* deviation: the corpus strides past the template */
    }
    uint32_t word = 0u;
    __builtin_memcpy(&word, (const uint8_t*)base + off64, sizeof(word));
    return word;
}

// FUNCTION: LEGORR 0x00469b20
uint32_t stats_table_word_at_b8(int type, int row, int stride_index)
{
    return stats_table_read(type, row, 0xb8, stride_index);
}
/* FUN_00469b20 */

// FUNCTION: LEGORR 0x00469af0
uint32_t stats_table_word_at_b4(int type, int row, int stride_index)
{
    return stats_table_read(type, row, 0xb4, stride_index); /* FUN_00469af0 */
}
// FUNCTION: LEGORR 0x00469f60
uint32_t stats_table_word_at_d0(int type, int row, int stride_index)
{
    return stats_table_read(type, row, 0xd0, stride_index); /* FUN_00469f60 */
}
// FUNCTION: LEGORR 0x0046a0f0
uint32_t stats_table_word_at_100(int type, int row, int stride_index)
{
    return stats_table_read(type, row, 0x100, stride_index); /* FUN_0046a0f0 */
}

// FUNCTION: LEGORR 0x00469e00
float stats_table_float_at_dc(int type, int row)
{
    /* FUN_00469e00 */
    const uint32_t bits = stats_table_read(type, row, 0xdc, 0);
    float f = 0.0f;
    __builtin_memcpy(&f, &bits, sizeof(f));
    return f;
}

// FUNCTION: LEGORR 0x00469ec0
uint32_t stats_table_word_at_144(int type, int row) { return stats_table_read(type, row, 0x144, 0); }
/* FUN_00469ec0 */
// FUNCTION: LEGORR 0x00469ee0
uint32_t stats_table_word_at_148(int type, int row) { return stats_table_read(type, row, 0x148, 0); }
/* FUN_00469ee0 */
// FUNCTION: LEGORR 0x00469f00
uint32_t stats_table_word_at_14c(int type, int row) { return stats_table_read(type, row, 0x14c, 0); }
/* FUN_00469f00 */

static uint32_t stats_instance_read(const void* config_block, int off)
{
    if (config_block == NULL) {
        return 0u; /* deviation: the corpus dereferences blindly */
    }
    int type = 0, row = 0, index = 0;
    __builtin_memcpy(&type, (const uint8_t*)config_block + 0x0, sizeof(type));
    __builtin_memcpy(&row, (const uint8_t*)config_block + 0x4, sizeof(row));
    __builtin_memcpy(&index, (const uint8_t*)config_block + 0x330, sizeof(index));
    return stats_table_read(type, row, off, index);
}

// FUNCTION: LEGORR 0x00469b50
uint32_t stats_instance_word_at_bc(const void* config_block) { return stats_instance_read(config_block, 0xbc); }
/* FUN_00469b50 */
// FUNCTION: LEGORR 0x00469b80
uint32_t stats_instance_word_at_e0(const void* config_block) { return stats_instance_read(config_block, 0xe0); }
/* FUN_00469b80 */
// FUNCTION: LEGORR 0x00469bb0
uint32_t stats_instance_word_at_ec(const void* config_block) { return stats_instance_read(config_block, 0xec); }
/* FUN_00469bb0 */
// FUNCTION: LEGORR 0x00469be0
uint32_t stats_instance_word_at_f0(const void* config_block) { return stats_instance_read(config_block, 0xf0); }
/* FUN_00469be0 */

/* --- entity query flag count (2026-10-25) ----------------------------------
 * EntityQueryFlagCount (FUN_0041b3c0): catalog-kind query. 0x13 answers the
 * second catalog count word; 0x14 counts the first catalog's rows whose
 * +148 word raises bit 0x1000 or 0x2000 while the +14c bit 8 stays clear;
 * 0x15 counts rows with bit 0x800 raised or the +14c bit 8 raised; any
 * other query strides the 8-byte id-pair table and answers that pair's
 * size. Deviations: the count globals and the id table have no ported
 * corpus owner (their loader band is unported), so they ride own storage
 * zero-init behind install/peek hooks (the unowned-DAT convention); rows
 * past the guarded stats table read the zero template through the ported
 * stats getters, and the pair window is guarded (the corpus strides
 * blindly). */

/* The catalog window band: the five count words and the four name tables
 * the encyclopedia loader indexes (the corpus's loader band is unported,
 * so the window rides own storage behind the hooks below; the five count
 * slots map DAT_0055627c/80/84/88/8c. */
// GLOBAL: LEGORR 0x0055627c
static uint32_t s_catalog_counts[5]; /* DAT_0055627c/80/84/88/8c roles */
static const void* s_catalog_name_tables[4]; /* DAT_005561e8/ec/f0/f4 roles */
#define CATALOG_ID_PAIRS 8u

// FUNCTION: LEGORR 0x0042ef40
int stats_level_record_count_for_stat_type(int stat_type)
{
    switch (stat_type) {
    case 1:
        return (int)s_catalog_counts[0];
    case 2:
        return (int)s_catalog_counts[1];
    case 3:
        return (int)s_catalog_counts[2];
    case 4:
        return (int)s_catalog_counts[3];
    case 5:
    case 6:
    case 8:
    case 9:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
        return 1;
    case 7:
        return 2;
    case 10:
        return (int)s_catalog_counts[4];
    default:
        return stat_type - 1;
    }
}

// FUNCTION: LEGORR 0x0041b3c0
int entity_query_flag_count(int query)
{
    if (query == 0x13) {
        return (int)s_catalog_counts[1];
    }
    if (query == 0x14 || query == 0x15) {
        int count = 0;
        for (uint32_t row = 0; row < s_catalog_counts[0]; ++row) {
            const uint32_t w148 = stats_table_word_at_148(1, (int)row);
            const uint32_t w14c = stats_table_word_at_14c(1, (int)row);
            if (query == 0x14) {
                if (((w148 & 0x1000u) != 0u || (w148 & 0x2000u) != 0u)
                    && (w14c & 8u) == 0u) {
                    count = count + 1;
                }
            } else if ((w148 & 0x800u) != 0u || (w14c & 8u) != 0u) {
                count = count + 1;
            }
        }
        return count;
    }
    if (query < 0 || (unsigned)query >= CATALOG_ID_PAIRS) {
        return 0; /* deviation: the corpus strides blindly */
    }
    /* The pair word is the stride-8 band's count word (the corpus reads
     * one and the same window for both roles); the merged model lives
     * behind the resource band accessor. */
    return (int)resource_handle_set_band_word_peek(query);
}

void catalog_count_install(int which, uint32_t value)
{
    if (which >= 0 && which < 5) {
        s_catalog_counts[which] = value;
    }
}

uint32_t catalog_count_peek(int which)
{
    return (which >= 0 && which < 5) ? s_catalog_counts[which] : 0u;
}

/* Port hooks over the encyclopedia loader's name-table windows: kind 0..3
 * ride the DAT_005561e8/ec/f0/f4 arrays in address order. */
void catalog_name_table_install(int kind, const void* names)
{
    if (kind >= 0 && kind < 4) {
        s_catalog_name_tables[kind] = names;
    }
}

const void* catalog_name_table_peek(int kind)
{
    return (kind >= 0 && kind < 4) ? s_catalog_name_tables[kind] : NULL;
}

/* CatalogCallbackByIdDispatch (FUN_0041b680): dispatch over the shared
 * stride-8 band. Query 0x13 walks the second catalog count, handing the
 * callback the row lane; 0x14 walks the first catalog's rows and calls
 * the callback on the rows whose +148 word raises bit 0x1000 or 0x2000
 * while the +14c bit 8 stays clear (the first callback that answers
 * nonzero wins); 0x15 hands over rows raising bit 0x800 or the +14c bit
 * 8; any other query needs a nonzero pair count and walks the group's
 * code array through the callback — 0x22/0x23 codes ride through the
 * selection-code resolve, which rewrites the array element in place
 * (the corpus's cached pick). Deviations: the query lanes thread the
 * context block (the resolve's state window is not a corpus argument),
 * NULL callback/pair arrays answer zero where the corpus strides
 * blindly, and the guarded reads fold the corpus's blind strides. */
// FUNCTION: LEGORR 0x0041b680
int catalog_callback_by_id_dispatch(const GameContext* ctx, int query,
                                    int (*callback)(int, int, int, void*),
                                    void* payload)
{
    if (callback == NULL) {
        return 0; /* deviation: the corpus calls the pointer blindly */
    }
    if (query == 0x13) {
        for (uint32_t row = 0; row < s_catalog_counts[1]; ++row) {
            if (callback(0x49, 4, (int)row, payload) != 0) {
                return 1;
            }
        }
        return 0;
    }
    if (query == 0x14 || query == 0x15) {
        for (uint32_t row = 0; row < s_catalog_counts[0]; ++row) {
            int answer = 0;
            if (query == 0x14) {
                if ((stats_table_word_at_148(1, (int)row) & 0x1000u) != 0u
                    || (stats_table_word_at_148(1, (int)row) & 0x2000u) != 0u) {
                    if ((stats_table_word_at_14c(1, (int)row) & 8u) == 0u) {
                        answer = callback(0x49, 1, (int)row, payload);
                    }
                }
            } else {
                if ((stats_table_word_at_148(1, (int)row) & 0x800u) != 0u
                    || (stats_table_word_at_14c(1, (int)row) & 8u) != 0u) {
                    answer = callback(0x49, 1, (int)row, payload);
                }
            }
            if (answer != 0) {
                return 1;
            }
        }
        return 0;
    }
    if (resource_handle_set_band_word_peek(query) == 0u) {
        return 0;
    }
    int* codes = (int*)resource_handle_set_slot_peek(query);
    if (codes == NULL) {
        return 0; /* deviation: the corpus strides the pair blindly */
    }
    for (uint32_t i = 0;;) {
        if (codes[i] == 0x22 || codes[i] == 0x23) {
            /* the corpus rewrites the cached element through the resolve */
            selection_entity_code_resolve(ctx, &codes[i]);
        }
        if (callback(codes[i], 0, 0, payload) != 0) {
            return 1;
        }
        /* corpus order: the index increments and the band word re-reads
         * every hop (a callback that re-registers the group retargets the
         * walk mid-sweep), and the walk stops before the first index the
         * count no longer covers */
        if (++i == 0u
                || resource_handle_set_band_word_peek(query) <= i) {
            return 0;
        }
    }
}

void catalog_id_pair_install(int entry, uint32_t size)
{
    /* The pair word rides the merged stride-8 band window (see above),
     * guarded to the corpus's strideable range. */
    if (entry >= 0 && (unsigned)entry < CATALOG_ID_PAIRS) {
        resource_handle_set_band_word_install(entry, size);
    }
}
