// SPDX-License-Identifier: LGPL-3.0-or-later
/* Matching build: Lego.c callers call lego_unit.c (#587). */
#include "lego_port_aliases.h"
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "ai.hpp"
#include "entity.hpp"
#include "building_scene.hpp"
#include "actor_scene.hpp"
#include "type_config_ports.hpp"
#include "entity_pose_delta.hpp"
#include "entity_motion_ports.hpp"
#include "activity_timeout.hpp"
#include "entity_motion_probe.hpp"
#include "port_loop_ports.hpp"
#include "simulation_ui_ports.hpp"
#include "type_motion_scale.hpp"
#include "entity_quad_collision.hpp"
#include "spawn.hpp"
#include "weapon.hpp"
#include "effects.hpp"
#include "level.hpp"
#include "terrain_grid.hpp"
#include "cmd.hpp"

#include "select.hpp"
#if defined(LEGORRR_BACKEND_MINIWIN)
#include "creature_scene.hpp"
#endif
#include "gods98/model.hpp"
#include "gods98/view.hpp"
#include "gods98/engine.hpp"
#include "gods98/resource.hpp"
#include "gods98/sound.hpp"
#include "gods98/record.hpp"
#include "gods98/heap.hpp"
#include "gods98/message.hpp"
#include "gods98/string.hpp"
#include "gods98/block.hpp"
#include "upgrades.hpp"
#include "gods98/message.hpp"
#include "crt/crt.hpp"
#include "frontend.hpp"
#include "entity_shared.hpp"
/* C linkage for legoobject_unit.c (the frozen header below redeclares it). */
extern "C" int entity_building_index_set(EntitySlot* entity, uint32_t state);
extern "C" void* entity_stats_field_at_a0_corpus(const EntitySlot* e);
#include "entity_from_entity_stats.hpp"
#include "entity_from_lego.hpp"
/* C linkage for legoGlobs' object-name tables, which legoobject_unit.c
 * reads (declared again, with the same types, by the frozen header below). */
extern "C" const char* s_object_name_special[9];
extern "C" const char* s_object_the_name_special[9];
extern "C" const char** s_object_name_kind1;
extern "C" const char** s_object_name_kind2;
extern "C" const char** s_object_name_kind3;
extern "C" const char** s_object_name_kind4;
extern "C" const char** s_object_name_kind10;
extern "C" const char** s_object_the_name_kind1;
extern "C" const char** s_object_the_name_kind2;
extern "C" const char** s_object_the_name_kind3;
extern "C" const char** s_object_the_name_kind4;
extern "C" const char** s_object_the_name_kind10;
/* The same for legoGlobs' type counts and object registry, which
 * lego_unit.c reads (declared again by entity_for_lego.hpp, through the
 * header below). */
extern "C" uint32_t s_catalog_counts[5];
extern "C" uint32_t s_object_pointer_registry_words[10];
extern "C" uint32_t s_object_pointer_registry_count;
#include "entity_for_legoobject.hpp"

static uint32_t s_next_ddiscene_owner_token = 1u;
EntityActorActivityAdvanceHook s_actor_activity_advance_hook;
void* s_actor_activity_advance_context;

char* s_entity_kind_names[16];
uint32_t s_entity_kind_name_counts[16];
static char* s_entity_localized_names[16];
static char* s_entity_localized_the_names[16];
static uint32_t s_entity_localized_name_counts[16];
static const char** s_entity_localized_name_entries[16];
static const char** s_entity_localized_the_name_entries[16];
int s_stats_cell_span[STATS_TYPE_TABLE_TYPES][STATS_TYPE_TABLE_ROWS];

void entity_slot_type_component_set(EntitySlot* entity, void* component)
{
    if (entity != NULL) entity->type_component_handle = component;
}

// GLOBAL: LEGORR 0x00556254
const char** s_object_name_kind1;
// GLOBAL: LEGORR 0x00556258
const char** s_object_name_kind2;
// GLOBAL: LEGORR 0x0055625c
const char** s_object_name_kind3;
// GLOBAL: LEGORR 0x00556260
const char** s_object_name_kind4;
// GLOBAL: LEGORR 0x00556264
const char** s_object_name_kind10;
// GLOBAL: LEGORR 0x00556268
const char** s_object_the_name_kind1;
// GLOBAL: LEGORR 0x0055626c
const char** s_object_the_name_kind2;
// GLOBAL: LEGORR 0x00556270
const char** s_object_the_name_kind3;
// GLOBAL: LEGORR 0x00556274
const char** s_object_the_name_kind4;
// GLOBAL: LEGORR 0x00556278
const char** s_object_the_name_kind10;
// GLOBAL: LEGORR 0x0055618c
const char* s_object_name_special[9];
// GLOBAL: LEGORR 0x005561b0
const char* s_object_the_name_special[9];
EntityHealthThresholdReadHook s_entity_health_threshold_read_hook;
int s_entity_pair_activity_sentinel;
EntityVisualResourcePresentHook s_entity_visual_resource_present_hook;
EntityVisualResourceCreateHook s_entity_visual_resource_create_hook;
EntityTeardownHooks s_entity_teardown_hooks;
EntityCommandStatusFeedbackHooks s_entity_command_feedback_hooks;
EntitySlot* s_entity_command_feedback_last_entity;
char s_entity_command_feedback_last_text[256];
void* s_entity_visual_resource_context;
EntitySceneSlotRuntimeOps s_entity_scene_slot_runtime_ops;
static EntitySurfaceScaleReadHook s_entity_surface_scale_read_hook;
static void* s_entity_surface_scale_context;
EntityIndexedFlagPublishHook s_entity_indexed_flag_publish_hook;
void* s_entity_indexed_flag_context;
EntityToolNameLoadHook s_entity_tool_name_load_hook;
void* s_entity_tool_name_context;
void* s_entity_tool_name_handles[11];
EntityTypeTwoEventHook s_entity_type_two_event_hook;
void* s_entity_type_two_event_context;
EntityTypeTwoPositionedHooks s_entity_type_two_positioned_hooks;
GameContext* s_entity_task_count_context;
EntityTaskCoordinateMatchHook s_entity_task_coordinate_match_hook;
void* s_entity_task_coordinate_match_user;
EntityBuildingDepositHook s_entity_building_deposit_hook;
void* s_entity_building_deposit_context;
EntityDepositActivityHooks s_entity_deposit_activity_hooks;
EntityBuildPlacementHooks s_entity_build_placement_hooks;
GameContext* s_entity_build_placement_context;
EntityActionIssueHooks s_entity_action_issue_hooks;
EntityActionConsumeHooks s_entity_action_consume_hooks;
EntityActionConsumeCounterSidecar* s_entity_action_counter_sidecars;
EntityClippedRectangleHooks s_entity_clipped_rectangle_hooks;
static EntityPairRayHitCheckHook s_entity_pair_ray_hit_check_hook;
static void* s_entity_pair_ray_hit_check_context;
void* s_entity_health_threshold_context;
EntityStateOverrideEntry s_entity_state_override_entries[64];
uint32_t s_entity_state_override_count;

uint32_t* entity_action_consume_counter_sidecar_get(EntitySlot* owner,
                                                           int create)
{
    EntityActionConsumeCounterSidecar* entry =
        s_entity_action_counter_sidecars;
    while (entry != NULL) {
        if (entry->owner == owner) return entry->values;
        entry = entry->next;
    }
    if (!create || owner == NULL) return NULL;
    entry = (EntityActionConsumeCounterSidecar*)malloc(sizeof(*entry));
    if (entry == NULL) return NULL;
    entry->values = (uint32_t*)engine_heap_alloc(0x18);
    if (entry->values == NULL) {
        free(entry);
        return NULL;
    }
    memset(entry->values, 0, 0x18);
    entry->owner = owner;
    entry->next = s_entity_action_counter_sidecars;
    s_entity_action_counter_sidecars = entry;
    return entry->values;
}

/* One path-motion slot per possible entity (ENTITY_TYPES * ENTITY_CAP),
 * allocated whole on the first request that creates a slot (#641): a static
 * table put 824,320 bytes in the matching build's .bss. NULL until then. */
EntityPathMotionRuntime* s_entity_path_motion_runtime;
void entity_path_motion_runtime_clear(EntitySlot* entity);
namespace unported_ledger_stubs {
void entity_path_motion_runtime_clear_impl(EntitySlot* entity);
}
EntityStateOverrideValue330ApplyHook
    s_entity_state_override_value_330_apply;
EntityStateOverrideRenameHook s_entity_state_override_rename;
void* s_entity_state_override_context;

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
EntityPeriodicTickHook s_entity_periodic_tick_hook;
void* s_entity_periodic_tick_context;
float s_periodic_timer25;

void entity_kind_name_tables_install(int kind, const char* const* names,
                                     uint32_t count)
{
    uint32_t i;
    char* storage = NULL;
    size_t storage_size;
    if (kind < 1 || (kind > 4 && kind != 10)) {
        return;
    }
    if (names != NULL && count != 0u) {
        if ((size_t)count > ((size_t)-1) / 128u) return;
        storage_size = (size_t)count * 128u;
        storage = (char*)calloc(1u, storage_size);
        if (storage == NULL) return;
        for (i = 0u; i < count; ++i) {
            if (names[i] != NULL) {
                strncpy(storage + (size_t)i * 128u, names[i], 127u);
            }
        }
    }
    free(s_entity_kind_names[kind]);
    s_entity_kind_names[kind] = storage;
    s_entity_kind_name_counts[kind] = 0u;
    if (names == NULL) {
        return;
    }
    s_entity_kind_name_counts[kind] = count;
}

void entity_localized_name_tables_install(int kind,
                                          const char* const* names,
                                          const char* const* the_names,
                                          uint32_t count)
{
    uint32_t i;
    size_t entries_size;
    size_t text_size;
    char* name_storage = NULL;
    char* the_name_storage = NULL;
    const char** name_entries = NULL;
    const char** the_name_entries = NULL;
    char* old_name_storage;
    char* old_the_name_storage;
    const char** old_name_entries;
    const char** old_the_name_entries;
    const char* first_name;
    const char* first_the_name;
    if (kind < 1 || kind > 15) {
        return;
    }
    if (count != 0u) {
        if ((size_t)count > ((size_t)-1) / sizeof(char*) ||
            (size_t)count > ((size_t)-1) / 128u) return;
        entries_size = (size_t)count * sizeof(char*);
        text_size = (size_t)count * 128u;
        name_entries = (const char**)calloc(1u, entries_size);
        the_name_entries = (const char**)calloc(1u, entries_size);
        if (names != NULL) name_storage = (char*)calloc(1u, text_size);
        if (the_names != NULL)
            the_name_storage = (char*)calloc(1u, text_size);
        if (name_entries == NULL || the_name_entries == NULL ||
            (names != NULL && name_storage == NULL) ||
            (the_names != NULL && the_name_storage == NULL)) {
            free(name_entries);
            free(the_name_entries);
            free(name_storage);
            free(the_name_storage);
            return;
        }
    }
    for (i = 0u; i < count; ++i) {
        if (names != NULL && names[i] != NULL) {
            char* slot = name_storage + (size_t)i * 128u;
            strncpy(slot, names[i], 127u);
            name_entries[i] = slot;
        }
        if (the_names != NULL && the_names[i] != NULL) {
            char* slot = the_name_storage + (size_t)i * 128u;
            strncpy(slot, the_names[i], 127u);
            the_name_entries[i] = slot;
        }
    }
    old_name_storage = s_entity_localized_names[kind];
    old_the_name_storage = s_entity_localized_the_names[kind];
    old_name_entries = s_entity_localized_name_entries[kind];
    old_the_name_entries = s_entity_localized_the_name_entries[kind];
    s_entity_localized_names[kind] = name_storage;
    s_entity_localized_the_names[kind] = the_name_storage;
    s_entity_localized_name_entries[kind] = name_entries;
    s_entity_localized_the_name_entries[kind] = the_name_entries;
    s_entity_localized_name_counts[kind] = count;
    first_name = count != 0u && name_entries != NULL
        ? name_entries[0] : NULL;
    first_the_name = count != 0u && the_name_entries != NULL
        ? the_name_entries[0] : NULL;

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
        s_object_name_special[0] = first_name;
        s_object_the_name_special[0] = first_the_name;
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
        s_object_name_special[2] = first_name;
        s_object_the_name_special[2] = first_the_name;
        break;
    case 9:
        s_object_name_special[3] = first_name;
        s_object_the_name_special[3] = first_the_name;
        break;
    case 10:
        s_object_name_kind10 = s_entity_localized_name_entries[kind];
        s_object_the_name_kind10 = s_entity_localized_the_name_entries[kind];
        break;
    case 11:
        s_object_name_special[5] = first_name;
        s_object_the_name_special[5] = first_the_name;
        break;
    case 12:
        s_object_name_special[6] = first_name;
        s_object_the_name_special[6] = first_the_name;
        break;
    case 13:
        s_object_name_special[7] = first_name;
        s_object_the_name_special[7] = first_the_name;
        break;
    case 15:
        s_object_name_special[8] = first_name;
        s_object_the_name_special[8] = first_the_name;
        break;
    }
    free(old_name_storage);
    free(old_the_name_storage);
    free(old_name_entries);
    free(old_the_name_entries);
}

static const char* entity_localized_name_pick(uint32_t kind, int index,
                                              int the_name)
{
    if (kind >= 1u && kind <= 10u && index >= 0
        && (uint32_t)index < s_entity_localized_name_counts[kind]) {
        const char* value = the_name != 0
            ? s_entity_localized_the_names[kind] != NULL
                ? s_entity_localized_the_names[kind] + (size_t)index * 128u
                : NULL
            : s_entity_localized_names[kind] != NULL
                ? s_entity_localized_names[kind] + (size_t)index * 128u
                : NULL;
        if (value != NULL && value[0] != '\0') return value;
    }
    if (the_name != 0) {
        return NULL;
    }
    return item_name_by_kind_lookup(kind, index);
}

void entity_health_threshold_read_hook_install(
    EntityHealthThresholdReadHook hook, void* context)
{
    s_entity_health_threshold_read_hook = hook;
    s_entity_health_threshold_context = context;
}

/* EntityCreateByType calls EntityBuildingIndexSet(entity, 0) after assigning
 * the config row. The entity-name resolver supplies that row for the port's
 * name-based creation paths. */
int entity_stats_record_bind(EntitySlot* entity)
{
    int type;
    int row;
    uint32_t state;
    int resolved = 0;
    const int had_record = entity != NULL && entity->stats_record != NULL;
    if (entity == NULL) return 0;
    type = entity->type;
    row = entity->etype;
    /* FUN_00437d00 leaves the static task-state templates attached for the
     * selector's special arms. The retail templates have zero at +0xcc; this
     * port binds the same-name Stats field there when available, without
     * replacing the task-state image with an instance Stats record. */
    if (entity->task_state != AI_TASK_STATE_NONE) {
        entity->stats_record = NULL;
        return entity_task_state_capacity_bind(entity);
    }
    if (entity->type_name[0] != '\0' &&
        entity_type_row_for_name(entity->type_name, &type, &row) &&
        type == entity->type && row >= 0) {
        resolved = 1;
    }
    /* A live entity can have an explicit row from a creation path whose
     * name is unavailable (for example the queued spawn kind). Keep that row
     * when rebinding its existing Stats pointer after a table reload. */
    else if ((had_record || entity->type_name[0] != '\0' ||
              entity->etype != entity->type) && entity->etype >= 0) {
        type = entity->type;
        row = entity->etype;
        resolved = 1;
    }
    entity->stats_record = NULL;
    if (!resolved) return 0;
    entity->etype = row;
    state = entity->indexed_record_slot_330 >= 0
        ? (uint32_t)entity->indexed_record_slot_330 : 0u;
    if (entity_building_index_set(entity, state)) return 1;
    if (state != 0u && entity_building_index_set(entity, 0u)) return 1;
    return 0;
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

float periodic_timer25_peek(void)
{
    return s_periodic_timer25;
}
#include "util.hpp"
#include "gods98/media.hpp"
#include "gods98/resource.hpp"
#include "frontend.hpp"
#include "crt/crt.hpp"
#include "entity_for_reward.hpp"
#include "entity_for_entity_stats.hpp"
#include "entity_for_lego.hpp"
#include "entity_for_weapons.hpp"
#include "entity_for_interface.hpp"
/* C linkage for LegoObject.c state that legoobject_unit.c also reads
 * (defined in legoobject_port.cpp; the frozen header below redeclares it). */
extern "C" EntityAuxiliaryReleaseRecord* s_entity_auxiliary_release_records;
extern "C" uint32_t s_global_animation_state;
extern "C" uint32_t s_pool_chunk_count;
extern "C" void* s_pool_chunks[POOL_CHUNK_COUNT_SLOTS];
extern "C" void* s_pool_record_head;
#include "entity_from_legoobject.hpp"
#include "entity_stats_port.hpp"

uint32_t s_entity_finalize_mode_flags;
EntityFinalizeHook s_entity_type5_timeout_hook;
EntityFinalizeHook s_entity_type5_finalize_hook;
EntitySurfaceVectorGetHook s_entity_surface_vector_get_hook;
EntityFlagDispatchHook s_entity_flag_dispatch_first;
EntityFlagDispatchHook s_entity_flag_dispatch_second;
EntitySegmentRadiusGetHook s_entity_segment_radius_hook;
EntityVelocityApplyHook s_entity_velocity_hooks[5];
EntityRayHitSweepOps s_entity_ray_hit_sweep_ops;
EntitySteerVelocityApplyFn s_entity_steer_velocity_apply;
void* s_entity_steer_velocity_context;
EntityVelocitySlotDispatchOps s_entity_velocity_slot_dispatch_ops;
static EntityVelocitySteerDispatchOps s_entity_velocity_steer_dispatch_ops;
EntityRouteSegmentAcceptHook s_entity_route_segment_accept_hook;
EntityScenePoseHooks s_entity_scene_pose_hooks;

void entity_surface_vector_hook_set(EntitySurfaceVectorGetHook hook)
{
    s_entity_surface_vector_get_hook = hook;
}

int entity_surface_vector_normalize_if_available(void* surface,
                                                  float out_xy[2])
{
    float vector[2];
    float length;
    if (surface == NULL || out_xy == NULL ||
        s_entity_surface_vector_get_hook == NULL) return 0;
    vector[0] = 0.0f;
    vector[1] = 0.0f;
    if (s_entity_surface_vector_get_hook(surface, vector) == 0) return 0;
    length = (float)sqrt((double)(vector[0] * vector[0] +
                                  vector[1] * vector[1]));
    if (length <= 0.0f) return 0;
    out_xy[0] = vector[0] / length;
    out_xy[1] = vector[1] / length;
    return 1;
}

void entity_flag_dispatch_hooks_set(EntityFlagDispatchHook first,
                                    EntityFlagDispatchHook second)
{
    s_entity_flag_dispatch_first = first;
    s_entity_flag_dispatch_second = second;
}

void entity_segment_radius_hook_set(EntitySegmentRadiusGetHook hook)
{
    s_entity_segment_radius_hook = hook;
}

void ray_hit_cell_sweep_ops_set(const EntityRayHitSweepOps* ops)
{
    if (ops != NULL) s_entity_ray_hit_sweep_ops = *ops;
    else memset(&s_entity_ray_hit_sweep_ops, 0,
                sizeof(s_entity_ray_hit_sweep_ops));
}

void entity_steer_velocity_apply_hook_set(EntitySteerVelocityApplyFn hook,
                                          void* context)
{
    s_entity_steer_velocity_apply = hook;
    s_entity_steer_velocity_context = context;
}

void entity_velocity_slot_dispatch_ops_set(
    const EntityVelocitySlotDispatchOps* ops)
{
    if (ops != NULL) s_entity_velocity_slot_dispatch_ops = *ops;
    else memset(&s_entity_velocity_slot_dispatch_ops, 0,
                sizeof(s_entity_velocity_slot_dispatch_ops));
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

void entity_route_segment_accept_hook_set(EntityRouteSegmentAcceptHook hook)
{
    s_entity_route_segment_accept_hook = hook;
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

void entity_scene_pose_hooks_set(const EntityScenePoseHooks* hooks)
{
    if (hooks == NULL) {
        s_entity_scene_pose_hooks.context = NULL;
        s_entity_scene_pose_hooks.poses_get = NULL;
    }
    else s_entity_scene_pose_hooks = *hooks;
}

void entity_table_maps_release(EntityTableMaps* maps)
{
    if (maps == NULL) return;
    engine_heap_free(maps->primary_a);
    engine_heap_free(maps->flags_a);
    engine_heap_free(maps->secondary_a);
    engine_heap_free(maps->primary_b);
    engine_heap_free(maps->flags_b);
    engine_heap_free(maps->secondary_b);
    memset(maps, 0, sizeof(*maps));
}

EntityTableMaps s_entity_table_maps;

void entity_table_maps_allocate_for_counts(EntityTableMaps* maps,
                                           unsigned int count_a,
                                           unsigned int count_b)
{
    if (maps == NULL) return;
    if (count_a > 0x1fffffffu || count_b > 0x1fffffffu ||
        (size_t)count_a > ((size_t)-1) / sizeof(uint32_t) ||
        (size_t)count_b > ((size_t)-1) / sizeof(uint32_t)) {
        entity_table_maps_release(maps);
        return;
    }
    entity_table_maps_release(maps);
    if (count_a != 0u) {
        const size_t dword_bytes = (size_t)count_a * sizeof(uint32_t);
        maps->primary_a = (uint32_t*)engine_heap_alloc((long)dword_bytes);
        maps->flags_a = (uint8_t*)engine_heap_alloc((long)count_a);
        maps->secondary_a = (uint32_t*)engine_heap_alloc((long)dword_bytes);
        if (maps->primary_a != NULL) memset(maps->primary_a, 0, dword_bytes);
        if (maps->flags_a != NULL) memset(maps->flags_a, 0, (size_t)count_a);
        if (maps->secondary_a != NULL)
            memset(maps->secondary_a, 0, dword_bytes);
    }
    if (count_b != 0u) {
        const size_t dword_bytes = (size_t)count_b * sizeof(uint32_t);
        maps->primary_b = (uint32_t*)engine_heap_alloc((long)dword_bytes);
        maps->flags_b = (uint8_t*)engine_heap_alloc((long)count_b);
        maps->secondary_b = (uint32_t*)engine_heap_alloc((long)dword_bytes);
        if (maps->primary_b != NULL) memset(maps->primary_b, 0, dword_bytes);
        if (maps->flags_b != NULL) memset(maps->flags_b, 0, (size_t)count_b);
        if (maps->secondary_b != NULL)
            memset(maps->secondary_b, 0, dword_bytes);
    }
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

void entity_table_maps_current_release(void)
{
    EntityTableMaps* maps = &s_entity_table_maps;
    if (maps->flags_a != NULL) {
        if (resource_handle_set_slot_peek(30) == maps->flags_a)
            resource_handle_set_slot_install(30, NULL);
        else if (resource_handle_set_slot_peek(30) == NULL) {
            maps->flags_a = NULL;
        }
        engine_heap_free(maps->flags_a);
    }
    if (maps->flags_b != NULL) {
        if (resource_handle_set_slot_peek(31) == maps->flags_b)
            resource_handle_set_slot_install(31, NULL);
        else if (resource_handle_set_slot_peek(31) == NULL) {
            maps->flags_b = NULL;
        }
        engine_heap_free(maps->flags_b);
    }
    if (maps->secondary_a != NULL) {
        if (resource_handle_set_slot_peek(32) == maps->secondary_a)
            resource_handle_set_slot_install(32, NULL);
        else if (resource_handle_set_slot_peek(32) == NULL) {
            maps->secondary_a = NULL;
        }
        engine_heap_free(maps->secondary_a);
    }
    if (maps->secondary_b != NULL) {
        if (resource_handle_set_slot_peek(33) == maps->secondary_b)
            resource_handle_set_slot_install(33, NULL);
        else if (resource_handle_set_slot_peek(33) == NULL) {
            maps->secondary_b = NULL;
        }
        engine_heap_free(maps->secondary_b);
    }
    if (maps->primary_a != NULL) {
        if (resource_handle_set_slot_peek(34) == maps->primary_a)
            resource_handle_set_slot_install(34, NULL);
        else if (resource_handle_set_slot_peek(34) == NULL) {
            maps->primary_a = NULL;
        }
        engine_heap_free(maps->primary_a);
    }
    if (maps->primary_b != NULL) {
        if (resource_handle_set_slot_peek(35) == maps->primary_b)
            resource_handle_set_slot_install(35, NULL);
        else if (resource_handle_set_slot_peek(35) == NULL) {
            maps->primary_b = NULL;
        }
        engine_heap_free(maps->primary_b);
    }
    memset(maps, 0, sizeof(*maps));
}
EntityRouteStateHook s_entity_route_state_hook;

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
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
#endif /* !LEGORRR_BACKEND_WINAPI */

/* Table index for a corpus type code (FUN_0042e8b0):
 * 1..4 map to the four typed tables, 5..19 map to the special table. */
int entity_tab(int type)
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

/* EntityPoolFindByCallback (FUN_004377d0): the corpus walks each allocated
 * entity chunk, accepts only records whose +0x408 word points to themselves,
 * optionally filters the +0x3e8 0x80000 bit, and returns on the first
 * callback match. pool_link_408 is the native-width mirror of that dword. */
GameContext* s_entity_pool_callback_context;
// GLOBAL: LEGORR 0x005563b8
uint32_t s_object_pointer_registry_words[
    OBJECT_POINTER_REGISTRY_CAPACITY];
// GLOBAL: LEGORR 0x005563e0
uint32_t s_object_pointer_registry_count;
/* Native pointer sidecar follows the retail-shaped slots and count. */
EntitySlot* s_object_pointer_registry_entities[
    OBJECT_POINTER_REGISTRY_CAPACITY];

void object_pointer_registry_reset(void)
{
    memset(s_object_pointer_registry_words, 0,
           sizeof(s_object_pointer_registry_words));
    memset(s_object_pointer_registry_entities, 0,
           sizeof(s_object_pointer_registry_entities));
    s_object_pointer_registry_count = 0u;
}

void object_pointer_registry_append(EntitySlot* entity)
{
    uint32_t index = s_object_pointer_registry_count;
    if (entity == NULL || index >= OBJECT_POINTER_REGISTRY_CAPACITY) return;
    /* Retail stores a 32-bit pointer word at DAT_005563b8; the sidecar keeps
     * the complete native pointer for portable readers. */
    s_object_pointer_registry_words[index] =
        (uint32_t)(uintptr_t)entity;
    s_object_pointer_registry_entities[index] = entity;
    s_object_pointer_registry_count = index + 1u;
}

void object_pointer_registry_entity_remove(EntitySlot* entity)
{
    uint32_t index;
    if (entity == NULL) return;
    for (index = 0u; index < s_object_pointer_registry_count; ++index) {
        if (s_object_pointer_registry_entities[index] == entity) {
            s_object_pointer_registry_words[index] = 0u;
            s_object_pointer_registry_entities[index] = NULL;
        }
    }
}

uint32_t object_pointer_registry_count_get(void)
{
    return s_object_pointer_registry_count;
}

int32_t object_pointer_registry_word_get(uint32_t index)
{
    return index < s_object_pointer_registry_count
        ? (int32_t)s_object_pointer_registry_words[index] : 0;
}

EntitySlot* object_pointer_registry_entity_get(uint32_t index)
{
    return index < s_object_pointer_registry_count
        ? s_object_pointer_registry_entities[index] : NULL;
}

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
            if (entity->pool_link_408 != (uintptr_t)entity) {
                continue; /* retail +0x408 self-pointer / free-list distinction */
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

int entity_pool_find_by_callback_filtered(GameContext* ctx,
                                          int (*callback)(EntitySlot*, void*),
                                          void* payload)
{
    return entity_pool_find_by_callback(ctx, callback, payload, 1);
}

EntityCallbackDispatchHook s_entity_callback_dispatch_hook;
void* s_entity_callback_dispatch_context;
EntityActionPrerequisiteHooks s_entity_action_prerequisite_hooks;
EntityRouteMarkerCandidateHook s_entity_route_marker_candidate_hook;
void* s_entity_route_marker_candidate_context;
EntityKindRecordUpdateHooks s_entity_kind_record_update_hooks;
static EntitySpawnSpecialFinalizeHook s_entity_spawn_special_finalize_hook;
static void* s_entity_spawn_special_finalize_context;
/* Natively only (#700, finding 627). In the matching build
 * LegoObject.c's objectGlobs.recordObjs (legoobject_unit.c) is the table:
 * nothing calls grid_object_record_state_install there, so the count stays
 * 0 and the release below finds no record. */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
GridObjectRecord s_grid_object_records[200];
#endif
unsigned int s_grid_object_record_count;
GridObjectRecordsReleaseHook s_grid_object_records_release_tail;
void* s_grid_object_records_release_context;
QueuedGridAnimationHooks s_queued_grid_animation_hooks;
float s_queued_grid_animation_fraction;
RoverWorkQueueHooks s_rover_work_queue_hooks;
BuildingUpgradeAdmissionHooks s_building_upgrade_admission_hooks;
BuildingUpgradeCommitHooks s_building_upgrade_commit_hooks;
DeferredOListEntityRecord s_deferred_olist_records[200];
unsigned int s_deferred_olist_record_count;
DeferredOListEntityHooks s_deferred_olist_hooks;
static LevelState* s_deferred_olist_level;
PeriodicEntityCallback s_periodic_entity_callback;
void* s_periodic_entity_callback_context;

#define ENTITY_INDEXED_RECORD_TABLES 320
struct EntityIndexedRecordTable {
    const void* const* records;
    uint32_t count;
};
static EntityIndexedRecordTable
    s_entity_indexed_record_tables[ENTITY_INDEXED_RECORD_TABLES];

void* entity_spatial_object_query_hook(
    void* context, const SpatialObjectQueryRequest* query);

/* FUN_0040cf50 obtains the type-0xb object's owning grid cell before it
 * links the object into that level's cell-kind registry. OList entries hold
 * world coordinates; resolve them through the entity's owning context and
 * reject out-of-map cells so one level cannot insert into another level's
 * registry. */
static int entity_grid_registry_cell_get(void* object, int* cell_x,
                                         int* cell_y, void* context)
{
    EntitySlot* entity = (EntitySlot*)object;
    GameContext* game = entity != NULL && entity->owner_ctx != NULL
        ? entity->owner_ctx : (GameContext*)context;
    float position[2];
    if (entity == NULL || cell_x == NULL || cell_y == NULL || game == NULL ||
        game->level == NULL || game->level->ptr_terrain == NULL) return 0;
    entity_transform_position_get(entity, position);
    if (!terrain_world_xy_to_cell(game->level->ptr_terrain, position[0],
                                  position[1], cell_x, cell_y) ||
        !level_coord_pair_within_bounds(game->level, *cell_x, *cell_y))
        return 0;
    return 1;
}

void entity_pool_callback_context_set(GameContext* ctx)
{
    int type;
    s_entity_pool_callback_context = ctx;
    grid_cell_object_hooks_install(
        ctx != NULL ? entity_grid_registry_cell_get : NULL, NULL, ctx);
    /* SpatialObjectQuery wrappers funnel through the engine query hook.
     * Bind the recovered LAB_00438fb0 predicate to the active entity pool. */
    spatial_object_query_hook_install(
        ctx != NULL ? entity_spatial_object_query_hook : NULL, ctx);
    active_entity_list_clear();
    if (ctx == NULL) return;
    /* DAT_004b335c is the live active-entity view consumed by command
     * assignment. Seed it from the typed pools, then keep create/teardown
     * events synchronized below. Types 5..19 share one pool. */
    for (type = 1; type <= 5; ++type) {
        int index;
        const int count = entity_count(ctx, type);
        for (index = 0; index < count; ++index) {
            const EntitySlot* entity = entity_at(ctx, type, index);
            if (entity != NULL && entity->type != 0 &&
                entity->ddiscene_owner_token != 0u)
                active_entity_list_entity_add(entity);
        }
    }
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

void entity_teardown_hooks_set(const EntityTeardownHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_entity_teardown_hooks, 0, sizeof(s_entity_teardown_hooks));
    }
    else {
        s_entity_teardown_hooks = *hooks;
    }
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

void entity_route_marker_candidate_hook_set(
    EntityRouteMarkerCandidateHook hook, void* context)
{
    s_entity_route_marker_candidate_hook = hook;
    s_entity_route_marker_candidate_context = context;
}

void* entity_spatial_object_query_hook(
    void* context, const SpatialObjectQueryRequest* query)
{
    EntityRouteMarkerSearchRequest search;
    if (context == NULL || query == NULL) return NULL;
    search.position = query->center;
    search.flags = query->mask;
    search.best_distance = query->radius;
    search.cell_x = (int)query->filters[0];
    search.cell_y = (int)query->filters[1];
    search.action_argument = (int)query->filters[2];
    search.requested_state = (int)query->filters[3];
    search.match = NULL;
    (void)entity_pool_find_by_callback_filtered((GameContext*)context,
        entity_route_marker_candidate_visit, &search);
    return search.match;
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

void grid_object_record_state_install(const GridObjectRecord* records,
                                      unsigned int count,
                                      GridObjectRecordsReleaseHook tail,
                                      void* context)
{
    unsigned int i;
    if (count > 200u) count = 200u;
    s_grid_object_record_count = 0u;
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
    if (records != NULL) {
        for (i = 0; i < count; ++i) {
            s_grid_object_records[i] = records[i];
        }
        s_grid_object_record_count = count;
    }
#else
    (void)records; (void)i; /* no port table in the matching build (#700) */
#endif
    s_grid_object_records_release_tail = tail;
    s_grid_object_records_release_context = context;
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

void entity_indexed_value_table_clear(void)
{
    memset(entity_stats_port_globs()->toolTaskTypes, 0,
           sizeof(entity_stats_port_globs()->toolTaskTypes));
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

uint32_t entity_indexed_record_word_get(int group, int index,
                                               int slot, uint32_t offset)
{
    const EntityIndexedRecordTable* table;
    const uint8_t* record;
    uint32_t value = 0u;
    if (group < 0 || index < 0 || slot < 0 || offset > 0x14cu) {
        return 0u;
    }

    /* The corpus readers index DAT_00502bd8[type][row], then walk one
     * 0x150-byte record per third-argument slot. The explicit table is kept
     * as a portable adapter, but ordinary Stats table installs feed the
     * same address path used by EntityBuildingIndexSet. */
    record = NULL;
    if (group < ENTITY_INDEXED_RECORD_TABLES) {
        table = &s_entity_indexed_record_tables[group];
        if (table->records != NULL && (uint32_t)index < table->count) {
            record = (const uint8_t*)table->records[index];
        }
    }
    if (record == NULL && group < STATS_TYPE_TABLE_TYPES &&
        index < STATS_TYPE_TABLE_ROWS &&
        slot < s_stats_cell_span[group][index]) {
        record = (const uint8_t*)entity_stats_port_row_table(group)[index];
    }
    if (record == NULL) {
        return 0u;
    }
    record += (uint32_t)slot * 0x150u + offset;
    memcpy(&value, record, sizeof(value));
    return value;
}

/* IndexedRecordWordAtOffsetB8Get (FUN_00469b20) is the third adjacent
 * configuration-word reader used by the terrain descriptor transition. */
uint32_t entity_indexed_record_word_at_b8_get(int group, int index, int slot)
{
    return entity_indexed_record_word_get(group, index, slot, 0xb8u);
}

/* EntityIndexedRecordWordAtOffsetF0Get (FUN_00469be0): the per-entity
 * record selector used to choose an attached scene-resource slot. */
uint32_t entity_indexed_record_word_at_f0_get(int group, int index, int slot)
{
    return entity_indexed_record_word_get(group, index, slot, 0xf0u);
}

/* Not 0x00406540: that original (aitask.c's
 * aitask_entity_pool_callback_search) walks the AITask pool, not the entity
 * pool (finding 570). Pool storage is represented by the most recently
 * active portable game context; live slots are walked in the same
 * callback-until-match style. */
int entity_pool_callback_search(int (*callback)(EntitySlot*, void*),
                                void* payload)
{
    return entity_pool_find_by_callback_current(callback, payload, 0);
}

void periodic_entity_callback_install(PeriodicEntityCallback callback,
                                      void* context)
{
    s_periodic_entity_callback = callback;
    s_periodic_entity_callback_context = context;
}

static ActivityRecordClearFn s_activity_record_clear;
static ActivityRunnerChangeFn s_activity_runner_change;
static void* s_activity_runner_user;
static EntityActivityRunnerResourceOps s_activity_runner_resource_ops;

void activity_runner_hooks_set(ActivityRecordClearFn clear_records,
                               ActivityRunnerChangeFn change, void* user)
{
    s_activity_record_clear = clear_records;
    s_activity_runner_change = change;
    s_activity_runner_user = user;
}

/* Port model of FUN_00406e40, superseded by game/animation_object.c
 * (GAME-004, finding 549). Kept verbatim, renamed and uncalled: deleting it
 * re-rolls later functions in this file (ADR 0005 entropy rule 1). */
int activity_runner_update_superseded(void* runner, void* state, void* value)
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

void entity_activity_runner_resource_ops_set(
    const EntityActivityRunnerResourceOps* ops)
{
    if (ops == NULL)
        memset(&s_activity_runner_resource_ops, 0,
               sizeof(s_activity_runner_resource_ops));
    else
        s_activity_runner_resource_ops = *ops;
}

static EntityActivityRunnerSidecar* entity_activity_runner_sidecar_get(
    EntitySlot* entity, int type)
{
    if (entity == NULL) return NULL;
    if (type == 2) return &entity->activity_runner_type2;
    if (type == 3) return &entity->activity_runner_type3;
    return NULL;
}

static void entity_activity_runner_sidecar_release(
    EntitySlot* entity, EntityActivityRunnerSidecar* runner)
{
    uint32_t i;
    if (runner == NULL) return;
    memset(runner->indexed_cache, 0, sizeof(runner->indexed_cache));
    for (i = 0u; i < 3u; ++i) {
        if (runner->resource_chains[i] != NULL &&
            s_activity_runner_resource_ops.resource_chain_release != NULL) {
            s_activity_runner_resource_ops.resource_chain_release(
                s_activity_runner_resource_ops.context,
                runner->resource_chains[i]);
        }
    }
    if (runner->child_scene != NULL) {
        if (s_activity_runner_resource_ops.scene_owner_bind != NULL) {
            s_activity_runner_resource_ops.scene_owner_bind(
                s_activity_runner_resource_ops.context, runner->child_scene,
                NULL);
        }
        if (s_activity_runner_resource_ops.scene_release != NULL) {
            s_activity_runner_resource_ops.scene_release(
                s_activity_runner_resource_ops.context, runner->child_scene);
        }
    }
    (void)entity;
    memset(runner, 0, sizeof(*runner));
}

int entity_activity_runner_type_config_bind(EntitySlot* entity)
{
    const TypeConfigTable* table;
    const uint8_t* row_data;
    EntityActivityRunnerSidecar* runner;
    uint32_t scene_token, source_token, slot_count, component_gate, row;
    uint32_t chain_tokens[3];
    uint32_t activity_values[5];
    int config_type;
    int config_row;
    uint32_t i;
    if (entity == NULL || (entity->type != 2 && entity->type != 3) ||
        entity->type_name[0] == '\0' ||
        !entity_type_row_for_name(entity->type_name, &config_type,
                                  &config_row) ||
        config_type != entity->type || config_row < 0) return 0;
    table = entity_type_config_table_for_entity_type(entity->type);
    row = (uint32_t)config_row;
    if (table == NULL || table->rows == NULL || table->row_stride < 0x74u ||
        row >= table->count) return 0;
    row_data = (const uint8_t*)table->rows + (size_t)row * table->row_stride;
    memcpy(&scene_token, row_data + 4u, sizeof(scene_token));
    memcpy(&source_token, row_data + 0x20u, sizeof(source_token));
    memcpy(&slot_count, row_data + 0x24u, sizeof(slot_count));
    memcpy(&component_gate, row_data + 0x28u, sizeof(component_gate));
    for (i = 0u; i < 3u; ++i)
        memcpy(&chain_tokens[i], row_data + 0x64u + i * 4u,
               sizeof(chain_tokens[i]));
    for (i = 0u; i < 5u; ++i)
        memcpy(&activity_values[i], row_data + 0x40u + i * 4u,
               sizeof(activity_values[i]));
    runner = entity_activity_runner_sidecar_get(entity, entity->type);
    if (runner == NULL) return 0;
    if (runner->bound != 0u && runner->template_row == row &&
        runner->scene_token == scene_token &&
        runner->source_token == source_token &&
        runner->slot_count == slot_count &&
        runner->component_gate_value == component_gate) return 1;
    entity_activity_runner_sidecar_release(entity, runner);
    runner->scene_token = scene_token;
    runner->source_token = source_token;
    runner->slot_count = slot_count;
    runner->template_row = row;
    runner->component_gate_value = component_gate;
    memcpy(runner->chain_tokens, chain_tokens, sizeof(chain_tokens));
    memcpy(runner->activity_values, activity_values,
           sizeof(activity_values));
    if (scene_token != 0u &&
        s_activity_runner_resource_ops.scene_clone != NULL) {
        runner->child_scene = s_activity_runner_resource_ops.scene_clone(
            s_activity_runner_resource_ops.context, scene_token);
        if (runner->child_scene != NULL &&
            s_activity_runner_resource_ops.scene_owner_bind != NULL) {
            s_activity_runner_resource_ops.scene_owner_bind(
                s_activity_runner_resource_ops.context, runner->child_scene,
                entity);
        }
    }
    for (i = 0u; i < 3u; ++i) {
        if (chain_tokens[i] != 0u &&
            s_activity_runner_resource_ops.resource_chain_clone != NULL) {
            runner->resource_chains[i] =
                s_activity_runner_resource_ops.resource_chain_clone(
                    s_activity_runner_resource_ops.context,
                    chain_tokens[i]);
        }
    }
    runner->bound = 1u;
    return 1;
}

void* entity_activity_runner_indexed_get(EntitySlot* entity, int type,
                                         uint32_t index)
{
    EntityActivityRunnerSidecar* runner;
    void* handle;
    if (!entity_activity_runner_type_config_bind(entity)) return NULL;
    runner = entity_activity_runner_sidecar_get(entity, type);
    if (runner == NULL || entity->type != type || runner->bound == 0u ||
        index >= runner->slot_count ||
        index >= ENTITY_ACTIVITY_RUNNER_CACHE_MAX) return NULL;
    handle = runner->indexed_cache[index];
    if (handle == NULL && runner->child_scene != NULL &&
        runner->source_token != 0u &&
        s_activity_runner_resource_ops.indexed_resource_resolve != NULL) {
        handle = s_activity_runner_resource_ops.indexed_resource_resolve(
            s_activity_runner_resource_ops.context, runner->child_scene,
            runner->source_token, index);
        runner->indexed_cache[index] = handle;
    }
    return handle;
}

int entity_type1_scene_resource_bind(EntitySlot* entity)
{
    const TypeConfigTable* table;
    const uint8_t* row_data;
    EntitySceneResourceSidecar* resource;
    uint32_t scene_token;
    int config_type;
    int config_row;
    if (entity == NULL || entity->type != 1 || entity->type_name[0] == '\0' ||
        !entity_type_row_for_name(entity->type_name, &config_type,
                                  &config_row) ||
        config_type != 1 || config_row < 0) return 0;
    table = entity_type_config_table_for_entity_type(1);
    if (table == NULL || table->rows == NULL || table->row_stride < 0x74u ||
        (uint32_t)config_row >= table->count) return 0;
    row_data = (const uint8_t*)table->rows +
               (size_t)(uint32_t)config_row * table->row_stride;
    memcpy(&scene_token, row_data + 4u, sizeof(scene_token));
    resource = &entity->scene_resource_type1;
    if (resource->bound != 0u &&
        resource->template_row == (uint32_t)config_row &&
        resource->scene_token == scene_token) return 1;
    memset(resource->indexed_cache, 0, sizeof(resource->indexed_cache));
    if (resource->scene_resource != NULL) {
        if (s_activity_runner_resource_ops.scene_owner_bind != NULL) {
            s_activity_runner_resource_ops.scene_owner_bind(
                s_activity_runner_resource_ops.context,
                resource->scene_resource, NULL);
        }
        if (s_activity_runner_resource_ops.scene_release != NULL) {
            s_activity_runner_resource_ops.scene_release(
                s_activity_runner_resource_ops.context,
                resource->scene_resource);
        }
    }
    memset(resource, 0, sizeof(*resource));
    resource->scene_token = scene_token;
    resource->template_row = (uint32_t)config_row;
    if (scene_token != 0u &&
        s_activity_runner_resource_ops.scene_clone != NULL) {
        resource->scene_resource = s_activity_runner_resource_ops.scene_clone(
            s_activity_runner_resource_ops.context, scene_token);
        if (resource->scene_resource != NULL &&
            s_activity_runner_resource_ops.scene_owner_bind != NULL) {
            s_activity_runner_resource_ops.scene_owner_bind(
                s_activity_runner_resource_ops.context,
                resource->scene_resource, entity);
        }
    }
    resource->bound = 1u;
    return 1;
}

void* entity_type1_scene_resource_indexed_get(
    EntitySlot* entity, uint32_t index, uint32_t* component_present)
{
    EntitySceneResourceSidecar* resource;
    void* handle;
    uint32_t gate_value = 0u;
    if (component_present != NULL) *component_present = 0u;
    if (entity == NULL || entity->type != 1 ||
        !entity_type1_scene_resource_bind(entity)) return NULL;
    resource = &entity->scene_resource_type1;
    if (resource->bound == 0u || resource->scene_resource == NULL ||
        index >= ENTITY_SCENE_RESOURCE_INDEX_CACHE_MAX) return NULL;
    handle = resource->indexed_cache[index];
    if (handle == NULL && resource->scene_token != 0u &&
        s_activity_runner_resource_ops.indexed_resource_resolve != NULL) {
        handle = s_activity_runner_resource_ops.indexed_resource_resolve(
            s_activity_runner_resource_ops.context, resource->scene_resource,
            resource->scene_token, index);
        resource->indexed_cache[index] = handle;
    }
    if (component_present != NULL &&
        s_activity_runner_resource_ops.scene_component_gate_read != NULL &&
        s_activity_runner_resource_ops.scene_component_gate_read(
            s_activity_runner_resource_ops.context, resource->scene_resource,
            &gate_value)) {
        *component_present = gate_value == 1u ? 1u : 0u;
    }
    return handle;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void entity_activity_runner_cache_clear(EntitySlot* entity, int type)
{
    EntityActivityRunnerSidecar* runner;
    if (type == 0) {
        entity_activity_runner_cache_clear(entity, 2);
        entity_activity_runner_cache_clear(entity, 3);
        return;
    }
    runner = entity_activity_runner_sidecar_get(entity, type);
    if (runner != NULL)
        memset(runner->indexed_cache, 0, sizeof(runner->indexed_cache));
}

int entity_activity_runner_activity_update(EntitySlot* entity, int type,
                                           void* activity, void* value)
{
    EntityActivityRunnerSidecar* runner;
    int result = 0;
    uint32_t i;
    if (entity == NULL || entity->type != type ||
        !entity_activity_runner_type_config_bind(entity)) return 0;
    runner = entity_activity_runner_sidecar_get(entity, type);
    if (runner == NULL || runner->bound == 0u) return 0;
    memset(runner->activity_values, 0, sizeof(runner->activity_values));
    entity_activity_runner_cache_clear(entity, type);
    for (i = 0u; i < 3u; ++i) {
        if (runner->resource_chains[i] != NULL &&
            s_activity_runner_resource_ops.resource_chain_release != NULL) {
            s_activity_runner_resource_ops.resource_chain_release(
                s_activity_runner_resource_ops.context,
                runner->resource_chains[i]);
        }
        runner->resource_chains[i] = NULL;
        runner->chain_tokens[i] = 0u;
    }
    if (runner->child_scene != NULL &&
        s_activity_runner_change != NULL) {
        result = s_activity_runner_change(runner->child_scene, activity,
                                           value, s_activity_runner_user);
    }
    if (runner->child_scene != NULL &&
        s_activity_runner_resource_ops.scene_time_update != NULL) {
        s_activity_runner_resource_ops.scene_time_update(
            s_activity_runner_resource_ops.context, runner->child_scene,
            value);
    }
    return result;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

void entity_activity_runner_release(EntitySlot* entity)
{
    if (entity == NULL) return;
    entity_activity_runner_sidecar_release(entity,
                                           &entity->activity_runner_type2);
    entity_activity_runner_sidecar_release(entity,
                                           &entity->activity_runner_type3);
    if (entity->scene_resource_type1.scene_resource != NULL) {
        if (s_activity_runner_resource_ops.scene_owner_bind != NULL) {
            s_activity_runner_resource_ops.scene_owner_bind(
                s_activity_runner_resource_ops.context,
                entity->scene_resource_type1.scene_resource, NULL);
        }
        if (s_activity_runner_resource_ops.scene_release != NULL) {
            s_activity_runner_resource_ops.scene_release(
                s_activity_runner_resource_ops.context,
                entity->scene_resource_type1.scene_resource);
        }
    }
    memset(&entity->scene_resource_type1, 0,
           sizeof(entity->scene_resource_type1));
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

/* The port's model of 0x00409ff0. The three global pointer replacements
 * and teardown pair are exposed as operations because their owning
 * registries are not reconstructed in the entity module. The matching build
 * has the original's shape in entity_cell_effects.c (findings 642 and 652). */
#if !(defined(_MSC_VER) && _MSC_VER < 1200)
void entity_destroy_tracked_pointer_cleanup(void* entity)
{
    EntitySlot* slot;
    GameContext* context;
    if (entity == NULL) return;
    slot = (EntitySlot*)entity;
    context = slot->owner_ctx;
    object_pointer_registry_entity_remove(slot);
    if (s_tracked_cleanup_callback != NULL) {
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
    /* Retail always tears down the entity and refreshes the activity scene
     * state after replacing any matching tracked pointers. */
    (void)entity_teardown(slot);
    entity_activity_scene_state_update(context);
}
#endif

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
    float target_position[2];
    unsigned int count = 1u;
    x_values[0] = target->x;
    y_values[0] = target->y;
    entity_transform_position_get(target, target_position);
    if (s_entity_explicit_route != NULL
        && s_entity_explicit_route(actor, target, x_values, y_values,
                                   &count, s_entity_explicit_route_user) == 0) {
        return 0;
    }
    if (count == 0u || count > AI_WAYPOINTS_MAX
        || !entity_route_setup_and_marker_associate(actor, count,
                                                    x_values, y_values,
                                                    target_position)) {
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
    float target_position[2];
    unsigned int count = 1u;
    x_values[0] = target->x;
    y_values[0] = target->y;
    entity_transform_position_get(target, target_position);
    if (s_entity_explicit_route != NULL
        && s_entity_explicit_route(actor, target, x_values, y_values,
                                   &count, s_entity_explicit_route_user) == 0) {
        return 0;
    }
    if (count == 0u || count > AI_WAYPOINTS_MAX
        || !entity_route_setup_and_marker_associate(actor, count,
                                                    x_values, y_values,
                                                    target_position)) {
        return 0;
    }
    actor->link_2fc = target;
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
    int sound_alias = -1;
    int sound_available;
    if (entity == NULL) return 1;
    if (entity->type != 2 || damage == 0.0f) return 1;
    sound_available = sound_alias_lookup("SND_Hurt", &sound_alias);
    if (s_entity_hurt_sound_available != NULL) {
        sound_available = s_entity_hurt_sound_available(
            s_entity_hurt_sound_user);
    }
    if (sound_available == 0) return 1;
    EntityHurtSoundState* state = entity_hurt_sound_state_get(entity);
    if (state == NULL) return 1;
    if (state->timer == 0.0f) {
        if (s_entity_hurt_sound_submit != NULL) {
            s_entity_hurt_sound_submit(entity, s_entity_hurt_sound_user);
        }
        else if (sound_alias >= 0) {
            /* FUN_0043fb40 resolves SND_Hurt then submits it through the
             * entity's type component. The live scene wrapper owns the same
             * positional dispatch; a missing scene object becomes position 0. */
            sound_submission_route_wrapper(entity->attached_scene_object,
                                           sound_alias, 0, 1, NULL);
        }
        state->timer = time;
        state->duration = s_entity_hurt_sound_duration != NULL
            ? s_entity_hurt_sound_duration(s_entity_hurt_sound_user)
            : (sound_alias >= 0
                ? (float)sound_object_duration_get(sound_alias) : 0.0f);
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

struct FlagMaskPredicateContext {
    uint32_t bit;
    EntityFlagMaskPredicate predicate;
    void* user;
    int all_pass;
};

static int flag_mask_predicate_callback(EntitySlot* entity, void* payload)
{
    FlagMaskPredicateContext* context = (FlagMaskPredicateContext*)payload;
    if (context->predicate != NULL &&
        context->predicate(entity, context->bit, context->user) == 0) {
        context->all_pass = 0;
    }
    return 0;
}

uint32_t s_entity_iteration_guard_flags;
uint32_t s_scene_dispatch_point_low;
uint32_t s_scene_dispatch_point_high;
uint32_t s_scene_dispatch_word_c;
uint32_t s_scene_dispatch_word14;

uint32_t entity_iteration_guard_flags_peek(void)
{
    return s_entity_iteration_guard_flags;
}

void entity_iteration_guard_flags_set(uint32_t flags)
{
    s_entity_iteration_guard_flags = flags;
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

/* Camera/control registry callers need their supplied per-entity predicate;
 * keep that adapter separate from the one-argument retail entry point above. */
int entity_flag_mask_callbacks_predicate_all_pass(
    GameContext* ctx, uint32_t mask, EntityFlagMaskPredicate predicate,
    void* user)
{
    FlagMaskPredicateContext context;
    if (ctx == NULL) return 0;
    if (mask != 0u && predicate == NULL) return 0;
    for (uint32_t index = 0; index < 31u; ++index) {
        uint32_t bit = 1u << index;
        context.bit = bit;
        context.predicate = predicate;
        context.user = user;
        context.all_pass = 1;
        if ((mask & bit) != 0u) {
            entity_pool_find_by_callback_filtered_current(
                flag_mask_predicate_callback, &context);
            if (context.all_pass == 0) return 0;
        }
    }
    return 1;
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
    entity_path_motion_runtime_clear(slot);
    __builtin_memset(slot, 0, sizeof(*slot));
    entity_motion_inputs_initialize(slot);
    slot->type = type;
    slot->etype = type;
    slot->pool_link_408 = (uintptr_t)slot;
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
    /* Corpus spawn dispatch default arm: the 0x438150 switch assigns the
     * static task-state (+0x334 stats block) to the template-backed types —
     * type 8 -> DAT_004a98f0, 9 -> DAT_004a9a40, 0xc..0xe and 0x10..0x13
     * -> DAT_004a9b90; every other type falls through to the instance
     * stats attacher (FUN_00469c10 at 0x43818a). The task state carries the
     * permitted-activity capacity (state +0xcc, FUN_0046a060) that the
     * appender reads per entity. The port binds a same-name Stats row's
     * NumOfToolsCanCarry into this sidecar when present; an absent row leaves
     * the shipped zero-template capacity at zero. The checked-in decompile
     * does not show a config copy into the static template itself. */
    slot->task_state = ai_minion_state_for(type);
    if (type == 4) {
        /* EntityCreateByType's type-4 arm initializes +0x3e8 with 0x40100
         * before its final KindFourPostCreateResolve call. */
        slot->flags_3e8 |= 0x00040100u;
    }
    if (name != NULL) {
        size_t copy_length = strlen(name);
        if (copy_length > 31u) copy_length = 31u;
        strncpy(slot->name, name, copy_length);
        slot->name[copy_length] = '\0';
        int configured_type = 0;
        int configured_row = 0;
        if (entity_type_row_for_name(name, &configured_type,
                                     &configured_row) &&
            configured_type == type && configured_row >= 0) {
            size_t type_name_length = strlen(name);
            if (type_name_length >= sizeof(slot->type_name))
                type_name_length = sizeof(slot->type_name) - 1u;
            memcpy(slot->type_name, name, type_name_length);
            slot->type_name[type_name_length] = '\0';
            slot->etype = configured_row;
            (void)entity_stats_record_bind(slot);
        }
    }
    if (type == 2) {
        const TypeConfigTable* table =
            entity_type_config_table_for_entity_type(type);
        uint32_t component_gate = 0u;
        if (table != NULL && table->rows != NULL && slot->etype >= 0 &&
            (uint32_t)slot->etype < table->count && table->row_stride >= 0x30u)
            memcpy(&component_gate, (const char*)table->rows +
                (size_t)slot->etype * table->row_stride + 0x2cu, 4);
        /* EntityCreateByType's Pilot arm installs the scheduler mask,
         * binds Stats, then enables tools and grants the first tool group. */
        slot->flags_3e8 |= component_gate != 0u ? 0x004481f7u : 0x004481f5u;
        ai_activity_grant(ctx, slot, AI_ACT_STAND);
    }
    {
        int created_index = (*cnt)++;
        if (ctx == entity_pool_callback_context_get())
            active_entity_list_entity_add(slot);
        if (type == 4) {
            /* Retail EntityCreateByType calls FUN_0043c570 after the type
             * record and its stats image have been attached. The host pool
             * count must include the new slot before its neighbor lookup. */
            entity_kind_four_post_create_resolve(slot);
        }
        return created_index;
    }
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

EntitySlot* entity_create_with_position_subtype(GameContext* context, int type,
                                                int subtype, const char* name,
                                                int value, float x, float y,
                                                float heading)
{
    int index = entity_create(context, type, name, x, y, 0.0f, heading);
    EntitySlot* created;
    if (index < 0) return NULL;
    created = (EntitySlot*)entity_at(context, type, index);
    if (created == NULL) return NULL;
    if (name != NULL) {
        int configured_type = 0;
        int configured_row = 0;
        if (entity_type_row_for_name(name, &configured_type,
                                     &configured_row) &&
            configured_type == type && configured_row >= 0) {
            EntitySlot* mutable_created = created;
            size_t type_name_length = strlen(name);
            if (type_name_length >= sizeof(mutable_created->type_name))
                type_name_length = sizeof(mutable_created->type_name) - 1u;
            memcpy(mutable_created->type_name, name, type_name_length);
            mutable_created->type_name[type_name_length] = '\0';
            (void)entity_activity_runner_type_config_bind(mutable_created);
            (void)entity_type_config_scene_slot_nodes_load(mutable_created);
        }
    }
    /* FUN_00439370/004394b0 pass subtype zero, not the entity kind, to
     * EntityCreateByType. The fourth argument selects the later animation
     * state and must not substitute for that row (ProcessedOre uses row 1).
     * Named creation already resolves its row through the config registry. */
    if (name == NULL && (type == 6 || type == 7))
        created->etype = subtype;
    (void)entity_stats_record_bind(created);
    entity_world_position_set(created, x, y, heading, 1);
#if defined(LEGORRR_BACKEND_WINAPI) && defined(_WIN32) && !defined(_WIN64)
    if (type == 4 && !building_scene_entity_bind(created)) {
        (void)entity_teardown(created);
        return NULL;
    }
    if ((type == 2 || type == 3) && !actor_scene_entity_bind(created)) {
        (void)entity_teardown(created);
        return NULL;
    }
#elif defined(LEGORRR_BACKEND_MINIWIN)
    /* EntityCreateByType (0x437d00) clones the type row's AnimationObject. A
     * type without a loadable scene keeps the port's drawn stand-in. */
    if ((type == 2 || type == 3) && creature_scene_entity_create(created))
        creature_scene_pose_apply(created);
#endif
    entity_scene_slot_table_rebuild(created, 0);
    if (type == 6 || type == 7) {
        if (s_entity_spawn_special_finalize_hook != NULL) {
            s_entity_spawn_special_finalize_hook(created, value,
                s_entity_spawn_special_finalize_context);
        }
        else {
            (void)entity_animation_state_set(created, (uint32_t)value);
        }
    }
    return created;
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

/* The level owns this production binding. The mapped queue and the reveal
 * consumer share records; no initial OList registry entry is added here. */
static const char* deferred_olist_type_name(
    const DeferredOListEntityRecord* record)
{
    const TypeConfigTable* table =
        entity_type_config_table_for_entity_type(record->entity_type);
    if (table == NULL || table->names == NULL || record->variant < 0 ||
        (uint32_t)record->variant >= table->count) return NULL;
    return table->names[record->variant];
}

static EntitySlot* deferred_olist_live_create(
    const DeferredOListEntityRecord* record, void* context)
{
    const char* name = deferred_olist_type_name(record);
    EntitySlot* entity = entity_create_with_position((GameContext*)context,
        record->entity_type, name, record->variant,
        record->x, record->y, record->heading);
    if (entity != NULL && record->name != NULL) {
        strncpy(entity->name, record->name, sizeof(entity->name) - 1u);
        entity->name[sizeof(entity->name) - 1u] = '\0';
    }
    return entity;
}

struct OListBuildingCreateContext {
    GameContext* game;
    const DeferredOListEntityRecord* record;
    EntitySlot* created;
};

static EntitySlot* olist_building_entity_create(void* context,
    const WorldObjectSpawnRequest* request, float orientation)
{
    OListBuildingCreateContext* binding = (OListBuildingCreateContext*)context;
    float center[2];
    if (!terrain_cell_world_center_xy_transform(
            binding->game->level->ptr_terrain,
            request->origin_x, request->origin_y, center)) return NULL;
    binding->created = entity_create_with_position(binding->game, 4,
        deferred_olist_type_name(binding->record),
        (int)request->object_kind, center[0], center[1], orientation);
    return binding->created;
}

static void olist_building_terrain_prepare(void* context,
    const int32_t* pairs, uint32_t count, float extent)
{
    OListBuildingCreateContext* binding = (OListBuildingCreateContext*)context;
    TerrainTileMeshGrid* mesh = binding->game->level->ptr_tile_mesh_grid;
    if (mesh != NULL)
        terrain_cell_surface_height_clamp(mesh->surface_grid,
                                          (const int*)pairs, count, extent);
}

static void olist_building_matching_prepare(void* context, int32_t x, int32_t y)
{
    OListBuildingCreateContext* binding = (OListBuildingCreateContext*)context;
    terrain_cell_flags_set_mask20108000(binding->game->level, x, y);
}

static void olist_building_matching_attach(void*, EntitySlot* entity,
                                          const int32_t pair[2])
{
    entity_update_and_unregister_at_tile(pair[0], pair[1], entity);
}

static void olist_building_boundary_unlink(void*, const int32_t pair[2])
{
    cancel_action_for_matching_coordinates((const uint32_t*)pair);
}

static void olist_building_boundary_apply(void* context, int32_t x, int32_t y)
{
    OListBuildingCreateContext* binding = (OListBuildingCreateContext*)context;
    terrain_cell_flags_set_mask100400(binding->game->level, x, y);
}

static void olist_building_boundary_secondary(void* context, const int32_t pair[2])
{
    OListBuildingCreateContext* binding = (OListBuildingCreateContext*)context;
    level_cell_secondary_bit0_set(binding->game->level, pair[0], pair[1]);
}

static void olist_building_pair_refresh(void*, int32_t x, int32_t y)
{
    terrain_cell_and_neighbors_update(x, y);
}

EntitySlot* deferred_olist_live_building_create(
    const DeferredOListEntityRecord* record, unsigned int, void* context)
{
    const TypeConfigTable* table = entity_type_config_table_for_entity_type(4);
    const uint32_t* shape;
    uint32_t token, count, i;
    int source[20], origin[2], rotation;
    const int32_t* pairs;
    const char* row;
    OListBuildingCreateContext binding;
    WorldObjectCreateHooks hooks;
    if (table == NULL || table->rows == NULL || record->variant < 0 ||
        (uint32_t)record->variant >= table->count) return NULL;
    row = (const char*)table->rows + record->variant * table->row_stride;
    memcpy(&token, row + 0x84, 4);
    memcpy(&count, row + 0x88, 4);
    shape = type_config_shape_table_resolve(table, token);
    if (shape == NULL || count > 10u) return NULL;
    for (i = 0u; i < count * 2u; ++i) {
        const char* text = type_config_shape_string_resolve(table, token, shape[i]);
        source[i] = text != NULL ? atoi(text) : 0;
    }
    origin[0] = record->cell_x;
    origin[1] = record->cell_y;
    /* OList rounds heading to the nearest quadrant via ftol, then halves
     * the rounded half-quadrant index, as in 0x437940/0x42e020. */
    rotation = crt_float_to_long(record->heading / 0.7853982f);
    rotation = ((rotation & 1) + rotation) >> 1;
    pairs = (const int32_t*)rotated_pairs_scratch_fill(origin, source,
                                                     (int)count, rotation);
    memset(&binding, 0, sizeof(binding));
    binding.game = (GameContext*)context;
    binding.record = record;
    memset(&hooks, 0, sizeof(hooks));
    hooks.context = &binding;
    hooks.entity_create_type4 = olist_building_entity_create;
    hooks.terrain_pairs_prepare = olist_building_terrain_prepare;
    hooks.matching_pair_prepare = olist_building_matching_prepare;
    hooks.matching_pair_attach = olist_building_matching_attach;
    hooks.boundary_pair_unlink = olist_building_boundary_unlink;
    hooks.boundary_pair_apply = olist_building_boundary_apply;
    hooks.boundary_pair_secondary = olist_building_boundary_secondary;
    hooks.pair_refresh = olist_building_pair_refresh;
    world_object_create_with_transform(&hooks, (uint32_t)record->variant,
        (const int32_t*)origin, (uint32_t)rotation, pairs, count, 0);
    if (binding.created != NULL && record->name != NULL) {
        strncpy(binding.created->name, record->name,
                sizeof(binding.created->name) - 1u);
        binding.created->name[sizeof(binding.created->name) - 1u] = '\0';
    }
    return binding.created;
}

static void deferred_olist_live_control_initialize(EntitySlot* entity, void*)
{
    cmd_create_task(entity);
}

static void deferred_olist_live_link_finalize(EntitySlot* entity,
    EntitySlot*, unsigned int mode, void* context)
{
    entity_linked_activity_event_dispatch(entity, mode);
    entity_linked_route_flag_clear((GameContext*)context, entity);
}

static void deferred_olist_live_record_release(DeferredOListEntityRecord* record,
                                              void*)
{
    engine_heap_free(record->name);
    engine_heap_free(record->link_name);
}

void deferred_olist_entity_level_end(LevelState* level)
{
    if (s_deferred_olist_level != level) return;
    deferred_olist_reset();
    if (s_deferred_olist_hooks.record_release != NULL)
        for (unsigned int i = 0u; i < s_deferred_olist_record_count; ++i)
            s_deferred_olist_hooks.record_release(&s_deferred_olist_records[i],
                                                  s_deferred_olist_hooks.context);
    deferred_olist_entity_queue_install(NULL, 0u);
    deferred_olist_entity_hooks_set(NULL);
    s_deferred_olist_level = NULL;
}

void deferred_olist_entity_level_begin(GameContext* context)
{
    DeferredOListEntityHooks hooks;
    deferred_olist_entity_level_end(s_deferred_olist_level);
    memset(&hooks, 0, sizeof(hooks));
    if (context == NULL || context->level == NULL) return;
    s_deferred_olist_level = context->level;
    hooks.context = context;
    hooks.entity_create = deferred_olist_live_create;
    hooks.special_building_create = deferred_olist_live_building_create;
    hooks.control_initialize = deferred_olist_live_control_initialize;
    hooks.link_finalize = deferred_olist_live_link_finalize;
    hooks.record_release = deferred_olist_live_record_release;
    deferred_olist_entity_hooks_set(&hooks);
}

/* The entity renderer consumes the portable parsed-model representation.
 * Share the engine model path/cache so entity loads and scene resource
 * lookups do not parse and own duplicate models for the same path. */
static const void* model_for_path(const char* path)
{
    if (path == NULL || path[0] == '\0') {
        return NULL;
    }
    return model_resource_resolve_or_load(NULL, path, NULL, NULL);
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
 * template; the Stats config loader installs records from the live config.
 * Names are offset-preserving: the decompile proves the offsets, not the
 * field semantics. */

static const uint8_t s_stats_zero_record[ENTITY_STATS_RECORD_SIZE] = { 0 };

const uint8_t* stats_rec(const EntitySlot* e)
{
    const uint8_t* rec = (const uint8_t*)e->stats_record;
    return rec != NULL ? rec : s_stats_zero_record;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
float stats_rec_float(const EntitySlot* e, int off)
{
    float f = 0.0f;
    __builtin_memcpy(&f, stats_rec(e) + off, sizeof(f));
    return f;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

static void stats_rec_float_set(EntitySlot* e, int off, float value)
{
    if (e == NULL || e->stats_record == NULL) return;
    __builtin_memcpy((uint8_t*)e->stats_record + off, &value, sizeof(value));
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
uint32_t stats_rec_word(const EntitySlot* e, int off)
{
    uint32_t w = 0;
    __builtin_memcpy(&w, stats_rec(e) + off, sizeof(w));
    return w;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* The retail x86 readers dereference the selected record at +0x334 directly.
 * Keep the null-template adapter for other hosts, but preserve the unchecked
 * field load in the MSVC 5.0 target build. The 32-bit EntitySlot keeps the
 * record pointer at that original offset (finding 520).
 *
 * Every EntitySlot member placed at its original 32-bit offset is asserted
 * here: the Stats record pointer (finding 520), the +0x33c/+0x340 floats,
 * the +0x36c owner and the +0x3e0..+0x3f0 flag words (finding 522), and
 * the +0x28 path count, the +0x2f0..+0x308 command, pair and link words,
 * the +0x330 selector, the +0x344 progress float and the +0x3c0 pair link
 * (finding 526), and the +0x300 reciprocal link (finding 565). */
#if defined(_MSC_VER) && defined(_M_IX86)
#define ENTITY_SLOT_RETAIL_OFFSET(member, off) \
    static_assert((size_t)&((EntitySlot*)0)->member == (off), \
                  "EntitySlot." #member " must sit at its retail offset")
#elif defined(__i386__)
#define ENTITY_SLOT_RETAIL_OFFSET(member, off) \
    static_assert(__builtin_offsetof(EntitySlot, member) == (off), \
                  "EntitySlot." #member " must sit at its retail offset")
#endif
#ifdef ENTITY_SLOT_RETAIL_OFFSET
ENTITY_SLOT_RETAIL_OFFSET(wp_count, 0x28);
ENTITY_SLOT_RETAIL_OFFSET(cmd_head, 0x2f0);
ENTITY_SLOT_RETAIL_OFFSET(pos_pair_2f4, 0x2f4);
ENTITY_SLOT_RETAIL_OFFSET(pos_pair_2f8, 0x2f8);
ENTITY_SLOT_RETAIL_OFFSET(link_2fc, 0x2fc);
ENTITY_SLOT_RETAIL_OFFSET(link_300, 0x300);
ENTITY_SLOT_RETAIL_OFFSET(attach_304, 0x304);
ENTITY_SLOT_RETAIL_OFFSET(carry_link_308, 0x308);
ENTITY_SLOT_RETAIL_OFFSET(indexed_record_slot_330, 0x330);
ENTITY_SLOT_RETAIL_OFFSET(stats_record, 0x334);
ENTITY_SLOT_RETAIL_OFFSET(field_33c, 0x33c);
ENTITY_SLOT_RETAIL_OFFSET(field_340, 0x340);
ENTITY_SLOT_RETAIL_OFFSET(progress_344, 0x344);
ENTITY_SLOT_RETAIL_OFFSET(owner_36c, 0x36c);
ENTITY_SLOT_RETAIL_OFFSET(pair_link_3c0, 0x3c0);
ENTITY_SLOT_RETAIL_OFFSET(flags_3e0, 0x3e0);
ENTITY_SLOT_RETAIL_OFFSET(flags_3e4, 0x3e4);
ENTITY_SLOT_RETAIL_OFFSET(flags_3e8, 0x3e8);
ENTITY_SLOT_RETAIL_OFFSET(flags_3ec, 0x3ec);
ENTITY_SLOT_RETAIL_OFFSET(flags_3f0, 0x3f0);
static_assert(sizeof(EntitySlot) == 0x674,
              "EntitySlot 32-bit size (0x678 until finding 565 merged +0x300)");
#undef ENTITY_SLOT_RETAIL_OFFSET
#endif

void entity_stats_float_ate8_set(EntitySlot* e, float value)
{
    stats_rec_float_set(e, 0xe8, value);
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
uint32_t entity_stats_word_at_ec(const EntitySlot* e) { return ENTITY_STATS_WORD_READ(e, 0xec); }

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

#undef ENTITY_STATS_FLOAT_READ
#undef ENTITY_STATS_WORD_READ

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

/* EntitySharedEligibilityCheck (FUN_004385b0, finding 471/472 context):
 * the command and activity layers' shared gate. Corpus order: +0x3e0 bits
 * 0x700100 clear, +0x3e4 bit 0x100000 clear, +0x340 float not negative,
 * +0x3e0 bit 0x20000 clear; a nonzero `relaxed` skips the +0x3e8 clause,
 * otherwise bit 0x20000000 must be set and bit 0x80000000 clear.
 * (+1000 decimal is +0x3e8 — the flags_3e8 mirror.) */

void entity_pair_interaction_activity_sentinel_set(int activity)
{
    s_entity_pair_activity_sentinel = activity;
}

void entity_visual_resource_hooks_set(EntityVisualResourcePresentHook present,
                                      EntityVisualResourceCreateHook create,
                                      void* context)
{
    s_entity_visual_resource_present_hook = present;
    s_entity_visual_resource_create_hook = create;
    s_entity_visual_resource_context = context;
}

void entity_indexed_flag_publish_hook_set(EntityIndexedFlagPublishHook hook,
                                          void* context)
{
    s_entity_indexed_flag_publish_hook = hook;
    s_entity_indexed_flag_context = context;
}

void entity_tool_name_load_hook_set(EntityToolNameLoadHook hook,
                                    void* context)
{
    s_entity_tool_name_load_hook = hook;
    s_entity_tool_name_context = context;
}

void* entity_tool_name_string_get(int slot)
{
    return slot >= 0 && slot < 11 ? s_entity_tool_name_handles[slot] : NULL;
}

void entity_type_two_event_hook_set(EntityTypeTwoEventHook hook,
                                    void* context)
{
    s_entity_type_two_event_hook = hook;
    s_entity_type_two_event_context = context;
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

void entity_task_count_context_set(GameContext* context,
                                   EntityTaskCoordinateMatchHook match,
                                   void* user)
{
    s_entity_task_count_context = context;
    s_entity_task_coordinate_match_hook = match;
    s_entity_task_coordinate_match_user = user;
}

void entity_building_deposit_hook_set(EntityBuildingDepositHook hook,
                                      void* context)
{
    s_entity_building_deposit_hook = hook;
    s_entity_building_deposit_context = context;
}

void entity_deposit_activity_hooks_set(const EntityDepositActivityHooks* hooks)
{
    if (hooks == NULL) memset(&s_entity_deposit_activity_hooks, 0,
                              sizeof(s_entity_deposit_activity_hooks));
    else s_entity_deposit_activity_hooks = *hooks;
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
    GameContext* context = s_entity_build_placement_context != NULL
        ? s_entity_build_placement_context
        : entity_pool_callback_context_get();
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
        TerrainSurfaceGrid* grid = path_record_surface_grid_get();
        float vertices[12];
        if (context != NULL && context->level != NULL &&
            context->level->ptr_terrain != NULL && grid != NULL &&
            cell_xy[0] >= 0 && cell_xy[1] >= 0 &&
            (uint32_t)cell_xy[0] < grid->width &&
            (uint32_t)cell_xy[1] < grid->height &&
            terrain_cell_surface_vertices_get(grid, (uint32_t)cell_xy[0],
                                               (uint32_t)cell_xy[1],
                                               vertices) >= 0) {
            const float scale_a = terrain_context_scale_get(
                context->level->ptr_terrain);
            const float scale_b = terrain_context_scale_get(
                context->level->ptr_terrain);
            const float inset = scale_a * 0.5f - scale_b * 0.25f;
            /* FUN_004500d0 supplies the four cell corners. The reference
             * samples X and Y independently inside the same inset bands. */
            position[0] = random_float_range(vertices[0] + inset,
                                              vertices[6] - inset);
            position[1] = random_float_range(vertices[1] - inset,
                                              vertices[7] + inset);
        }
        else {
            if (!terrain_cell_world_position_get(cell_xy[0], cell_xy[1],
                                                 cell_position)) return 0;
            position[0] = cell_position[0];
            position[1] = cell_position[1];
        }
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

/* EntityActionKindSupported (FUN_0043a7c0): the kind-6 call is a no-op after
 * the stats flags have been loaded, so its x86 return residue is that
 * nonzero flags word. The kind-7 call reads the selected live level balance. */
static int entity_action_kind_supported_const_unused(const EntitySlot* source,
                                                     int kind, int selector)
{
    uint32_t flags;
    const LevelState* level;
    GameContext* context;
    if (source == NULL) return 0;
    flags = entity_stats_word_get(source);
    if ((flags & 8u) == 0u) return 0;
    if (kind == 6) return 1;
    if (kind == 7) {
        context = source->owner_ctx;
        if (context == NULL) context = entity_pool_callback_context_get();
        level = context != NULL ? context->level : NULL;
        if (level == NULL) return 0;
        return upgrade_resource_check(level, selector) != 0u;
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

uint32_t entity_action_consume_counter_get(const EntitySlot* entity,
                                          uint32_t slot)
{
    uint32_t* values;
    if (entity == NULL || slot >= 6u) return 0u;
    values = entity_action_consume_counter_sidecar_get(
        (EntitySlot*)entity, 0);
    return values != NULL ? values[slot]
                          : entity->action_consume_counters[slot];
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

void entity_pair_ray_hit_check_hook_set(EntityPairRayHitCheckHook hook,
                                        void* context)
{
    s_entity_pair_ray_hit_check_hook = hook;
    s_entity_pair_ray_hit_check_context = context;
}

int entity_pair_ray_hit_check(const EntitySlot* first,
                              const EntitySlot* second)
{
    if (first == NULL || second == NULL) return 1;
    if (s_entity_pair_ray_hit_check_hook != NULL) {
        return s_entity_pair_ray_hit_check_hook(
            first, second, s_entity_pair_ray_hit_check_context) != 0;
    }
    /* cmd_context_set installs the live position + entity-pool/terrain ray
     * sweep ops. Keep the absent-provider result conservative for callers
     * that require a clear line of sight. */
    if (s_entity_ray_hit_sweep_ops.object_position_get != NULL &&
        s_entity_ray_hit_sweep_ops.scene_trace_prepare != NULL &&
        s_entity_ray_hit_sweep_ops.grid_dimensions_get != NULL &&
        s_entity_ray_hit_sweep_ops.world_to_cell != NULL &&
        s_entity_ray_hit_sweep_ops.cell_intersection != NULL) {
        return object_pair_ray_hit_check((void*)first, (void*)second);
    }
    return 1;
}

EntityOneShotAttachedEffectHooks s_one_shot_attached_effect_hooks;

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
    if (level == g_active_level_retail) {
        return entity_cell_type_speed_multiplier_resolve_current(
            entity, x, y, scale, out_scale);
    }
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
        if (out_scale != NULL) {
            uint32_t rock_fall_type =
                lego::object_type_name_index_find("RockFallIn");
            float motion_scale =
                lego::entity_indexed_float_lookup_with_minus_one_fallback(
                    (int)rock_fall_type, entity);
            *out_scale = motion_scale * scale;
        }
        return 1;
    }
    return 0;
}

void entity_transform_position_get(const EntitySlot* entity,
                                   float out_xy[2])
{
    if (out_xy == NULL) return;
    entity_transform_position_get_outputs(entity, &out_xy[0], &out_xy[1]);
}

int entity_position_grid_coordinates_get(const EntitySlot* entity,
                                          const LevelState* level,
                                          float out_xy[2])
{
    float world[2];
    float center[2];
    float blocksize;
    int cell_x, cell_y;
    if (entity == NULL || level == NULL || level->ptr_terrain == NULL ||
        out_xy == NULL) return 0;
    blocksize = terrain_blocksize_get(level->ptr_terrain);
    if (blocksize <= 0.0f) return 0;
    entity_transform_position_get(entity, world);
    if (!terrain_world_xy_to_cell(level->ptr_terrain, world[0], world[1],
                                  &cell_x, &cell_y) ||
        !terrain_cell_world_center_xy_get(level->ptr_terrain,
                                           cell_x, cell_y, center)) return 0;
    out_xy[0] = (float)cell_x + 0.5f + (world[0] - center[0]) / blocksize;
    out_xy[1] = (float)cell_y + 0.5f + (center[1] - world[1]) / blocksize;
    return 1;
}

void entity_type5_finalize_environment_set(uint32_t mode_flags,
                                          EntityFinalizeHook timeout_hook,
                                          EntityFinalizeHook finalize_hook)
{
    s_entity_finalize_mode_flags = mode_flags;
    s_entity_type5_timeout_hook = timeout_hook;
    s_entity_type5_finalize_hook = finalize_hook;
}

void entity_route_state_hook_set(EntityRouteStateHook hook)
{
    s_entity_route_state_hook = hook;
}

#define PENDING_TYPE_VALUE_BASE 0x00557864u
#define PENDING_TYPE_VALUE_COUNT 39u
static EntityCursorTargetHooks s_entity_cursor_target_hooks;

void pending_type_value_set(uint32_t original_address, uint32_t value)
{
    if (original_address < PENDING_TYPE_VALUE_BASE
        || ((original_address - PENDING_TYPE_VALUE_BASE) & 3u) != 0u) {
        return;
    }
    uint32_t index = (original_address - PENDING_TYPE_VALUE_BASE) / 4u;
    if (index < PENDING_TYPE_VALUE_COUNT) {
        bubble_texture_handle_set((int)index, value);
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
        ? bubble_texture_handle_peek((int)index) : 0u;
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
        point[2] = entity->z;
    }
    /* FUN_004078c0 applies the stats +0x74 offset after reading the scene
     * position, so renderer-provided positions need the same adjustment. */
    point[2] -= entity_stats_float_at74(entity);
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

EntityPayloadReleaseFn s_payload_scene_release_hook;
EntityPayloadReleaseFn s_payload_chain_release_hook;

void entity_payload_release_hooks_set(EntityPayloadReleaseFn scene_release,
                                      EntityPayloadReleaseFn chain_release)
{
    s_payload_scene_release_hook = scene_release;
    s_payload_chain_release_hook = chain_release;
}

void entity_scene_slot_runtime_ops_set(const EntitySceneSlotRuntimeOps* ops)
{
    if (ops == NULL) {
        memset(&s_entity_scene_slot_runtime_ops, 0,
               sizeof(s_entity_scene_slot_runtime_ops));
    }
    else {
        s_entity_scene_slot_runtime_ops = *ops;
    }
}

void entity_scene_slot_owner_keys_set(EntitySlot* entity, uint32_t key_1c,
                                      uint32_t key_20, uint32_t key_24)
{
    if (entity == NULL) return;
    entity->scene_slot_owner_keys[0] = key_1c;
    entity->scene_slot_owner_keys[1] = key_20;
    entity->scene_slot_owner_keys[2] = key_24;
}

EntitySceneSlotNode* entity_scene_slot_node_append(EntitySlot* entity,
                                                   void* renderer_record,
                                                   void* payload)
{
    EntitySceneSlotNode* node;
    EntitySceneSlotNode** tail;
    if (entity == NULL) return NULL;
    node = (EntitySceneSlotNode*)calloc(1u, sizeof(*node));
    if (node == NULL) return NULL;
    node->renderer_record = renderer_record;
    node->payload = payload;
    tail = &entity->scene_slot_nodes;
    while (*tail != NULL) tail = &(*tail)->next;
    *tail = node;
    ++entity->scene_slot_node_count;
    return node;
}

void entity_scene_slot_nodes_clear(EntitySlot* entity)
{
    EntitySceneSlotNode* node;
    if (entity == NULL) return;
    node = entity->scene_slot_nodes;
    while (node != NULL) {
        EntitySceneSlotNode* next = node->next;
        free(node);
        node = next;
    }
    entity->scene_slot_nodes = NULL;
    entity->scene_slot_node_count = 0u;
    entity->scene_slot_registration_count = 0u;
    entity->scene_slot_cache_count = 0u;
    memset(entity->scene_slot_cache, 0, sizeof(entity->scene_slot_cache));
    memset(entity->scene_slot_owner_1c_handles, 0,
           sizeof(entity->scene_slot_owner_1c_handles));
    memset(entity->scene_slot_owner_24_handles, 0,
           sizeof(entity->scene_slot_owner_24_handles));
    memset(entity->scene_slot_owner_20_handles, 0,
           sizeof(entity->scene_slot_owner_20_handles));
    memset(entity->scene_slot_owner_1c_counts, 0,
           sizeof(entity->scene_slot_owner_1c_counts));
    memset(entity->scene_slot_payloads, 0,
           sizeof(entity->scene_slot_payloads));
    memset(entity->scene_slot_owner_keys, 0,
           sizeof(entity->scene_slot_owner_keys));
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
    static const char* fields[ENTITY_STATS_CONFIG_FIELD_COUNT] = {
        "RouteSpeed", "SoilDrillTime", "LooseDrillTime", "MedDrillTime",
        "HardDrillTime", "SeamDrillTime", "BuildingBase", "CostCrystal",
        "CostOre", "CostRefinedOre", "TrackDist", "AlertRadius", "CollHeight",
        "HealthDecayRate", "EnergyDecayRate", "PickSphere", "DrillSound",
        "DrillFadeSound", "EngineSound", "RestPercent", "CarryMinHealth",
        "StampRadius", "AttackRadius", "WaterEntrances", "RubbleCoef", "PathCoef",
        "OxygenCoef", "Capacity", "CollRadius", "CollBox", "SingleWidthDig",
        "RepairValue", "SurveyRadius", "MaxCarry", "CarryStart", "CrystalDrain",
        "NumOfToolsCanCarry", "UpgradeTime", "FunctionCoef", "UpgradeCostOre",
        "UpgradeCostStuds", "ProcessOre", "ProcessCrystal", "StoreObjects",
        "ManTeleporter", "SmallTeleporter", "BigTeleporter", "WaterTeleporter",
        "SnaxULike", "ToolStore", "CauseSlip", "RandomMove", "CanScare",
        "CanScareScorpion", "RandomEnterWall", "ScaredByPlayer", "GrabMinifigure",
        "CanBeDriven", "CanSteal", "CanClearRubble", "RouteAvoidance", "BumpDamage",
        "Flocks", "Flocks_Debug", "Flocks_Smooth", "CrossWater", "CrossLand",
        "Flocks_OnGround", "Flocks_QuickDestroy", "Flocks_AnimateByPitch",
        "ScaredByBigBangs", "UpgradeBuilding", "CanBeShotAt", "UseBigTeleporter",
        "UseSmallTeleporter", "UseWaterTeleporter", "UseLegoManTeleporter",
        "TrainPilot", "TrainSailor", "TrainDriver", "TrainDynamite", "TrainRepair",
        "TrainScanner", "Tracker", "PowerBuilding", "SelfPowered", "AttackPaths",
        "SplitOnZeroHealth", "CanBeHitByFence", "CanDoubleSelect", "DrainPower",
        "UseHoles", "CrossLava", "DamageCausesCallToArms", "CanFreeze", "CanLaser",
        "CanPush", "DontShowDamage", "RemoveReinforcement", "DontShowOnRadar",
        "InvisibleDriver", "Unselectable", "CarryVehicles", "VehicleCanBeCarried",
        "CanStrafe", "ClassAsLarge", "GetOutAtLand", "GetInAtLand",
        "TakeCarryingDrivers", "ShowHealthBar", "NeedsPilot", "EnterToolStore",
        "Flocks_Turn", "Flocks_Speed", "Flocks_Tightness", "Flocks_GoalUpdate",
        "Flocks_Height", "Flocks_Randomness", "Flocks_Size", "Flocks_AttackTime",
        "AwarenessRange", "PainThreshold", "FreezerTime", "FreezerDamage",
        "PusherDist", "PusherDamage", "LaserDamage", "WakeRadius", "RandomMoveTime"
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
        for (field = 0; field < ENTITY_STATS_CONFIG_FIELD_COUNT; ++field)
        {
            size_t field_length = strlen(fields[field]);
            if (field_length >= sizeof(record.field_names[field]))
                field_length = sizeof(record.field_names[field]) - 1u;
            memcpy(record.field_names[field], fields[field], field_length);
            record.field_names[field][field_length] = '\0';
            for (level = 0; level < capacity; ++level) {
                hooks->float_value_get(hooks->context, record.name,
                    fields[field], level,
                    &record.level_fields[level][field]);
            }
            record.fields[field] = capacity != 0u
                ? record.level_fields[0][field] : 0.0f;
        }
        (void)hooks->float_value_get(hooks->context, record.name, "Levels",
                                     0u, &record.level_limit);
        for (level = 0; level < capacity; ++level)
            hooks->float_value_get(hooks->context, record.name, "Levels",
                                   level, &record.levels[level]);
        if (hooks->record_commit != NULL)
            hooks->record_commit(hooks->context, &record);
        ++committed;
    }
    /* EntityStatsConfigLoad seeds these four grouped lookup records after
     * walking Stats. Reinitialization replaces the same four entries rather
     * than appending duplicates to the prior load. */
    if (committed != 0u) {
        entity_indexed_value_table_clear();
        entity_indexed_value_append(0, 8u);
        entity_indexed_value_append(1, 0xcu);
        entity_indexed_value_append(2, 0xbu);
        entity_indexed_value_append(3, 10u);
    }
    return committed;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
uint32_t object_pointer_registry_count_peek(void)
{
    return s_object_pointer_registry_count;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* --- context-selected globals (see entity.hpp) -----------------------------
 * The corpus reads the context-mode global (DAT_00556ec8) and the
 * current-selection pointer (DAT_00556160); both are modeled in the media
 * layer (the port's own models, fed by SoundMinimumDistanceUpdateMode).
 * The DAT_005560f8 / DAT_00556104 pair has no ported feeder — two
 * zero-initialized statics (documented deviation). */
uint32_t s_ctx_sel_ptr_first  = 0u; /* DAT_005560f8 */
uint32_t s_ctx_sel_ptr_second = 0u; /* DAT_00556104 */

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void context_selected_pointer_pair_set(uint32_t first, uint32_t second)
{
    s_ctx_sel_ptr_first = first;
    s_ctx_sel_ptr_second = second;
}

uint32_t context_selected_pointer_pair_peek(int second)
{
    return (second != 0) ? s_ctx_sel_ptr_second : s_ctx_sel_ptr_first;
}

uint32_t entity_global_animation_state_peek(void)
{
    return s_global_animation_state;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* --- the pooled-record chunk window + resolved dispatch band (2026-10-25) -- */

/* PooledRecordChunkAllocateAndLink (FUN_00438310) — see entity.hpp. The
 * corpus grows a power-of-two chunk of 0x40c-stride entity records, threads
 * them as a free list whose last link joins the previous head, and tracks
 * the chunk table with a byte counter. Deviations: the port records widen
 * the link slot (stride 0x410 where the corpus strides 0x40c, the
 * port-pointer convention) and the chunk table is bounded 32 entries where
 * the corpus strides it blindly. */

static void pooled_record_storage_clear(void)
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

void pooled_record_pool_initialize(void)
{
    /* ActivityNameTableInit zeroes the pool chunk table, generation, and
     * free-list head as part of its wide globals clear; it does not run the
     * entity teardown callback used by EntityPoolResourcesRelease. */
    pooled_record_storage_clear();
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void* pooled_record_head_peek(void) { return s_pool_record_head; }
uint32_t pooled_record_chunk_count_peek(void) { return s_pool_chunk_count; }
#endif /* !LEGORRR_BACKEND_WINAPI */

/* --- DAT_00502bd8 indexed stats-type tables (2026-10-25) ------------------
 * The corpus reads `*(row_table[type][row])` through DAT_00502bd8's pointer
 * tables and strides each template record by 0x150. The Stats config loader
 * installs those row bases and both allocation/raw level limits; guarded storage keeps
 * invalid types, rows, and stride indices from escaping the installed span.
 * The corpus float fields read as x87 float10 of the stored float; the port
 * hands back the plain float. */

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
    entity_stats_port_row_table(type)[row] =
        stride_count > 0 ? (StatsData*)record : NULL;
    s_stats_cell_span[type][row] = stride_count;
    s_stats_cell_level_limit[type][row] = (float)stride_count;
    /* The original's level count, which the unit's getters read. */
    entity_stats_port_globs()->objectLevels[type][row] =
        (unsigned int)stride_count;
}

void stats_type_table_level_limit_install(int type, int row, float level_limit)
{
    if (type < 0 || type >= STATS_TYPE_TABLE_TYPES || row < 0 ||
        row >= STATS_TYPE_TABLE_ROWS) return;
    s_stats_cell_level_limit[type][row] = level_limit;
}

const void* stats_type_table_cell(int type, int row)
{
    if (type < 0 || type >= STATS_TYPE_TABLE_TYPES
        || row < 0 || row >= STATS_TYPE_TABLE_ROWS) {
        return NULL; /* deviation: the corpus strides blindly */
    }
    return entity_stats_port_row_table(type)[row];
}

uint32_t stats_table_read(int type, int row, int off, int stride_index)
{
    if (type < 0 || type >= STATS_TYPE_TABLE_TYPES
        || row < 0 || row >= STATS_TYPE_TABLE_ROWS || stride_index < 0) {
        return 0u; /* deviation: the corpus strides the index blindly */
    }
    const void* base = entity_stats_port_row_table(type)[row];
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

uint32_t stats_table_word_at_cc(int type, int row, int stride_index)
{
    return stats_table_read(type, row, 0xcc, stride_index);
}

int entity_task_state_capacity_bind(EntitySlot* entity)
{
    int type;
    int row;
    if (entity == NULL || entity->task_state == AI_TASK_STATE_NONE ||
        entity->type_name[0] == '\0') {
        if (entity != NULL && entity->task_state != AI_TASK_STATE_NONE)
            entity->activity_cap = 0;
        return 0;
    }
    type = 0;
    row = 0;
    if (!entity_type_row_for_name(entity->type_name, &type, &row) ||
        type != entity->type || row < 0 ||
        stats_type_table_cell(type, row) == NULL) {
        entity->activity_cap = 0;
        return 0;
    }
    /* The static template is one record, unlike per-level instance Stats
     * tables. Its loaded +0xcc word is the only config-backed capacity. */
    entity->activity_cap = (int)stats_table_word_at_cc(type, row, 0);
    return 1;
}

/* FUN_00469b20 */

/* Port stats-table convenience reader at record offset +0xb4. The retail
 * 0x469af0 annotation belongs to entity_indexed_record_word_at_b4_get. */
uint32_t stats_table_word_at_b4(int type, int row, int stride_index)
{
    return stats_table_read(type, row, 0xb4, stride_index);
}
/* Port stats-table convenience reader at record offset +0xb0. The retail
 * 0x469ac0 annotation belongs to entity_indexed_record_word_at_b0_get. */
uint32_t stats_table_word_at_b0(int type, int row, int stride_index)
{
    return stats_table_read(type, row, 0xb0, stride_index);
}

/* FUN_00469f00 */

/* FUN_00469be0 */

/* --- entity query flag count (2026-10-25) ----------------------------------
 * EntityQueryFlagCount (FUN_0041b3c0): catalog-kind query. 0x13 answers the
 * second catalog count word; 0x14 counts the first catalog's rows whose
 * +148 word raises bit 0x1000 or 0x2000 while the +14c bit 8 stays clear;
 * 0x15 counts rows with bit 0x800 raised or the +14c bit 8 raised; any
 * other query strides the 8-byte id-pair table and answers that pair's
 * size. The five count globals are populated from the live type config;
 * the id table remains own storage behind install/peek hooks. Rows
 * past the guarded stats table read the zero template through the ported
 * stats getters, and the pair window is guarded (the corpus strides
 * blindly). */

/* The catalog window band: the five count words and four name tables the
 * encyclopedia loader indexes. Counts and names are installed from the
 * current config load; the five slots map DAT_0055627c/80/84/88/8c. */
// GLOBAL: LEGORR 0x0055627c
uint32_t s_catalog_counts[5]; /* DAT_0055627c/80/84/88/8c roles */
static const void* s_catalog_name_tables[4]; /* DAT_005561e8/ec/f0/f4 roles */

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

const EntityTableMaps* entity_table_maps_current_peek(void)
{
    return &s_entity_table_maps;
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

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
void catalog_id_pair_install(int entry, uint32_t size)
{
    /* The pair word rides the merged stride-8 band window (see above),
     * guarded to the corpus's strideable range. */
    if (entry >= 0 && (unsigned)entry < CATALOG_ID_PAIRS) {
        resource_handle_set_band_word_install(entry, size);
    }
}
#endif /* !LEGORRR_BACKEND_WINAPI */

namespace unported_ledger_stubs {

CellStepPathResolveHook s_cell_step_path_hook;
CellStepRandomRangeHook s_cell_step_random_hook;
void* s_cell_step_hook_user;

void entity_cell_step_hooks_set(CellStepPathResolveHook path_hook,
                                CellStepRandomRangeHook random_hook,
                                void* user)
{
    s_cell_step_path_hook = path_hook;
    s_cell_step_random_hook = random_hook;
    s_cell_step_hook_user = user;
}

int entity_cell_step_action_peek(const EntitySlot* entity, int* type_out,
                                 float out_xy[2], uint32_t* flags_out)
{
    if (type_out != NULL) *type_out = 0;
    if (out_xy != NULL) out_xy[0] = out_xy[1] = 0.0f;
    if (flags_out != NULL) *flags_out = 0u;
    if (entity == NULL || entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    const AiWaypoint* step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    if (step->step_kind != 8 && step->step_kind != 9) return 0;
    if (type_out != NULL) *type_out = step->step_kind;
    if (out_xy != NULL) {
        out_xy[0] = step->weight_y;
        out_xy[1] = step->weight_x;
    }
    if (flags_out != NULL) *flags_out = step->pending;
    return 1;
}

float entity_cell_step_random_range(float minimum, float maximum)
{
    if (s_cell_step_random_hook != NULL)
        return s_cell_step_random_hook(minimum, maximum,
                                       s_cell_step_hook_user);
    return random_float_range(minimum, maximum);
}

} // namespace unported_ledger_stubs

float entity_activity_tick(EntitySlot* entity, float delta)
{
    return unported_ledger_stubs::ledger_stub_00447b30_EntityActivityTick(
        entity, delta);
}

namespace unported_ledger_stubs {

// EntityCallbackDispatchUnlessTypeFive
void ledger_stub_004374f0_EntityCallbackDispatchUnlessTypeFive(
    EntitySlot* entity)
{
    if (entity->type != 5)
        entity_callback_dispatch_unless_type_five(entity);
}

// EntityTeardown
int ledger_stub_00437540_EntityTeardown(EntitySlot* entity)
{
    return entity_teardown(entity);
}

// GridObjectRecordsRelease
void ledger_stub_004378e0_GridObjectRecordsRelease()
{
    grid_object_records_release();
}

// DeferredOListEntityCreateAndLink
void ledger_stub_00437940_DeferredOListEntityCreateAndLink(
    const int cell_xy[2])
{
    deferred_olist_entity_create_and_link(cell_xy);
}

// RoverWorkQueueUpdate
void ledger_stub_004383b0_RoverWorkQueueUpdate(EntitySlot* entity, int mode)
{
    rover_work_queue_update(entity, mode);
}

// EntityKindRecordUpdateDispatch
void ledger_stub_00438460_EntityKindRecordUpdateDispatch(EntitySlot* entity)
{
    entity_kind_record_update_dispatch(entity);
}

// EntityCreateWithPosition
EntitySlot* ledger_stub_00438610_EntityCreateWithPosition(
    GameContext* context, int type, const char* name, int value,
    float x, float y, float heading)
{
    return entity_create_with_position(context, type, name, value, x, y,
                                       heading);
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

void* ledger_stub_00438df0_EntityLookupAtWorldPosition(
    const float center[2])
{
    return entity_lookup_at_world_position(center);
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// ClippedRectangleCallbackDispatch
void ledger_stub_0043b6c0_ClippedRectangleCallbackDispatch(
    void* payload, const float rect_a[2], const float rect_b[2])
{
    entity_clipped_rectangle_callback_dispatch(payload, rect_a, rect_b);
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// TypeTwoEntityEventDispatch
void ledger_stub_0043b820_TypeTwoEntityEventDispatch(
    const EntitySlot* entity, uint32_t argument_a, uint32_t argument_b)
{
    entity_type_two_event_dispatch(entity, argument_a, argument_b);
}

// TypeTwoEntityPositionedCallbackDispatch
void ledger_stub_0043b850_TypeTwoEntityPositionedCallbackDispatch(
    const EntitySlot* entity, void* scene_object, uint32_t argument_a,
    uint32_t argument_b, int mode)
{
    entity_type_two_positioned_callback_dispatch(
        entity, scene_object, argument_a, argument_b, mode);
}

// BuildingDepositStateMonitor
void ledger_stub_0043bb20_BuildingDepositStateMonitor(EntitySlot* entity)
{
    entity_building_deposit_state_monitor(entity);
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// EntityType26CommandCompletionGate
int ledger_stub_0043c3e0_EntityType26CommandCompletionGate(
    const EntitySlot* entity)
{
    return entity_type26_command_completion_gate(entity);
}

// KindFourPostCreateResolve
void ledger_stub_0043c570_KindFourPostCreateResolve(EntitySlot* entity)
{
    entity_kind_four_post_create_resolve(entity);
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// RecordPairPoseCopyAndActivityRaise
EntityPairPoseHooks s_entity_pair_pose_hooks;

void entity_pair_pose_hooks_set(const EntityPairPoseHooks* hooks)
{
    if (hooks == NULL) memset(&s_entity_pair_pose_hooks, 0,
                              sizeof(s_entity_pair_pose_hooks));
    else s_entity_pair_pose_hooks = *hooks;
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// TerrainGridPathSearch
int ledger_stub_00440c70_TerrainGridPathSearch(
    void* level, uint32_t start_x, uint32_t start_y,
    uint32_t goal_x, uint32_t goal_y, int** out_x, int** out_y,
    uint32_t* out_count, TerrainPathStopHook stop, void* user)
{
    return terrain_grid_path_search(level, start_x, start_y, goal_x, goal_y,
                                    out_x, out_y, out_count, stop, user);
}

// TerrainNeighborPathCheck
int ledger_stub_004410f0_TerrainNeighborPathCheck(
    void* entity, uint32_t start_x, uint32_t start_y,
    uint32_t goal_x, uint32_t goal_y, int** out_x, int** out_y,
    float* out_count, TerrainPathStopHook stop, void* user)
{
    uint32_t route_count = 0;
    int found = terrain_neighbor_path_check(
        entity, start_x, start_y, goal_x, goal_y, out_x, out_y,
        &route_count, stop, user);
    if (found && out_count != NULL) *out_count = (float)route_count;
    return found;
}

void entity_path_motion_runtime_clear_impl(EntitySlot* entity)
{
    unsigned int i;
    if (entity == NULL || s_entity_path_motion_runtime == NULL) return;
    for (i = 0u; i < ENTITY_TYPES * ENTITY_CAP; ++i) {
        if (s_entity_path_motion_runtime[i].owner == entity) {
            memset(&s_entity_path_motion_runtime[i], 0,
                   sizeof(s_entity_path_motion_runtime[i]));
            return;
        }
    }
}

int entity_path_waypoint_world_get(const EntitySlot* entity,
                                          unsigned int index,
                                          float position[2],
                                          int project_surface)
{
    LevelState* level;
    const AiWaypoint* step;
    float blocksize;
    if (entity == NULL || position == NULL || index >= (unsigned int)entity->wp_count ||
        (entity->wp_dynamic == NULL && entity->wp_count > AI_WAYPOINTS_MAX) ||
        (entity->wp_dynamic != NULL && index >= entity->wp_capacity)) return 0;
    level = entity->owner_ctx != NULL ? entity->owner_ctx->level : NULL;
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    step = &ENTITY_WAYPOINTS(entity)[index];
    if (!terrain_cell_world_center_xy_get(level->ptr_terrain,
            crt_float_to_long(step->x), crt_float_to_long(step->y), position))
        return 0;
    blocksize = terrain_blocksize_get(level->ptr_terrain);
    position[0] += (step->weight_y - 0.5f) * blocksize;
    position[1] += (0.5f - step->weight_x) * blocksize;
    /* Surface-edge steering belongs to the legacy specialized mover. A
     * plain cell/subcell destination must remain at its requested point. */
    if (project_surface && step->step_kind != 1u) {
        float projected_x;
        float projected_y;
        if (terrain_state_surface_point_project_offset(level->ptr_terrain,
                position[0], position[1], 0, 0.0f,
                &projected_x, &projected_y)) {
            position[0] = projected_x;
            position[1] = projected_y;
        }
    }
    return 1;
}

float ledger_stub_004438b0_PathStepAdvanceAndArrivalCheck(
    EntitySlot* entity, float delta)
{
    return unported_ledger_stubs::entity_path_step_advance_and_arrival_check(entity, delta);
}

int ledger_stub_00444680_StepMovementVectorResolve(
    EntitySlot* entity, int advance, int use_saved_vector,
    int steering_mode)
{
    return unported_ledger_stubs::entity_step_movement_vector_resolve(entity, advance,
                                               use_saved_vector,
                                               steering_mode);
}

} // namespace unported_ledger_stubs

/* Export the existing route owner through the declared production API. */
float entity_path_step_advance_and_arrival_check(EntitySlot* entity,
                                                 float delta)
{
    return unported_ledger_stubs::entity_path_step_advance_and_arrival_check(
        entity, delta);
}

/* Pure rate query: reuse the ordinary 0x446610 terrain/state adjustments,
 * with delta 1 reference tick. No cursor, pose, or animation is advanced. */
float entity_route_speed(EntitySlot* entity)
{
    float distance = 0.0f;
    float animation_delta = 0.0f;
    if (entity == NULL || entity->stats_record == NULL) return 0.0f;
    unported_ledger_stubs::ledger_stub_00446610_EntityMovementTickScaleAndDeltaGet(
        entity, 1.0f, &distance, &animation_delta);
    return distance;
}

int entity_route_waypoint_world_get(const EntitySlot* entity,
                                    unsigned int index, float position[2])
{
    return unported_ledger_stubs::entity_path_waypoint_world_get(
        entity, index, position, 0);
}

void entity_path_motion_runtime_clear(EntitySlot* entity)
{
    unported_ledger_stubs::entity_path_motion_runtime_clear_impl(entity);
}

namespace unported_ledger_stubs {

// EntityMovementTickScaleAndDeltaGet
static float entity_movement_component_float_get(
    const EntitySlot* entity, int offset, float stats_fallback)
{
    float value;
    if (entity == NULL || entity->type_component_handle == NULL)
        return stats_fallback;
    memcpy(&value, (const char*)entity->type_component_handle + offset,
           sizeof(value));
    return value;
}

float entity_movement_default_base_speed_get(void* context,
                                                     void* entity,
                                                     int kind)
{
    const EntitySlot* slot = (const EntitySlot*)entity;
    (void)context;
    (void)kind;
    /* 0x446610's displacement numerator is 0x469cb0: selected Stats
     * +0x20 (RouteSpeed). The type component is an animation-rate owner,
     * not a replacement source for physical route velocity. */
    return entity_stats_float_at20(slot);
}

int entity_movement_default_terrain_data_get(
    void* context, void* entity, int* cell_cost, int* terrain_type,
    int* occupiable, int* entity_scale_gate)
{
    EntitySlot* slot = (EntitySlot*)entity;
    LevelState* level;
    float position[2];
    int cell_x;
    int cell_y;
    uint8_t* cell;
    uint32_t cell_flags = 0u;
    (void)context;
    if (slot == NULL || slot->owner_ctx == NULL ||
        slot->owner_ctx->level == NULL) return 0;
    level = slot->owner_ctx->level;
    if (level->ptr_terrain == NULL) return 0;
    entity_transform_position_get(slot, position);
    if (!terrain_world_xy_to_cell(level->ptr_terrain, position[0],
                                  position[1], &cell_x, &cell_y)) return 0;
    cell = level_voxel(level, cell_x, cell_y);
    if (cell == NULL) return 0;
    memcpy(&cell_flags, cell + 8, sizeof(cell_flags));
    if (cell_cost != NULL)
        *cell_cost = level_cell_speed_class(level, cell_x, cell_y);
    if (terrain_type != NULL) *terrain_type = (int)(signed char)cell[2];
    if (occupiable != NULL)
        *occupiable = level_cell_flag_8_bounded(level, cell_x, cell_y) != 0u;
    if (entity_scale_gate != NULL)
        *entity_scale_gate = (cell_flags & 0x20000000u) != 0u;
    return 1;
}

float entity_movement_default_surface_scale_get(void* context,
                                                        void* entity)
{
    (void)context;
    /* 0x469ce0 also reads selected Stats, not the animation component. */
    return entity_stats_float_atd4((const EntitySlot*)entity);
}

float entity_movement_default_entity_scale_get(void* context,
                                                       void* entity)
{
    (void)context;
    /* 0x469d20: selected Stats surface/path modifier. */
    return entity_stats_float_atd8((const EntitySlot*)entity);
}

float entity_movement_default_animation_scale_get(void* context,
                                                          void* entity)
{
    (void)context;
    return entity_movement_component_float_get(
        (const EntitySlot*)entity, 0xf8,
        entity_speed_component_get((const EntitySlot*)entity));
}

// EntityConditionalTypeFiveSpawn
int ledger_stub_004470d0_EntityConditionalTypeFiveSpawn(EntitySlot* entity)
{
    return lego::entity_conditional_type_five_spawn(entity);
}

// TryCreateLinkedEntityAtPosition
int ledger_stub_004471b0_TryCreateLinkedEntityAtPosition(
    EntitySlot* entity, float cell_x, float cell_y, EntitySlot* linked)
{
    return lego::entity_try_create_linked_at_position(
        entity, cell_x, cell_y, linked);
}

int ledger_stub_00447210_EntityAdjacentPlacementCellSearch(
    EntitySlot* entity, EntitySlot* target)
{
    return lego::entity_adjacent_placement_cell_search(entity, target);
}

// LinkedEntityProximityResponse
void ledger_stub_00447950_EntityDirectionalActivityStart(
    EntitySlot* entity, const float direction[2], int mode);

// EntityDirectionalActivityStart
void ledger_stub_00447950_EntityDirectionalActivityStart(
    EntitySlot* entity, const float direction[2], int mode)
{
    entity_directional_activity_start(entity, direction, mode);
}

// EntityTypeOperationRelease
uint32_t ledger_stub_00449ff0_EntityTypeOperationRelease(EntitySlot* entity)
{
    return ::entity_type_operation_release(entity);
}

} // namespace unported_ledger_stubs

void entity_type_operation_fallback_handle_set(EntitySlot* entity,
                                               void* handle_1c)
{
    if (entity != NULL) entity->type_operation_fallback_1c = handle_1c;
}

namespace unported_ledger_stubs {

int ledger_stub_0044a2a0_EntityWorkObjectPositionGet(
    const EntitySlot* entity, float position[2])
{
    return ::entity_work_object_position_get(entity, position);
}

// BuildingPlacementPreviewUpdate
static BuildingPlacementPreviewHooks s_building_placement_preview_hooks;
static int s_building_placement_preview_owner_bound;

void building_placement_preview_owner_bind(
    const BuildingPlacementPreviewHooks* hooks)
{
    if (hooks == NULL) {
        memset(&s_building_placement_preview_hooks, 0,
               sizeof(s_building_placement_preview_hooks));
        s_building_placement_preview_owner_bound = 0;
        return;
    }
    s_building_placement_preview_hooks = *hooks;
    s_building_placement_preview_owner_bound = 1;
}

int building_placement_preview_owner_update(void)
{
    if (!s_building_placement_preview_owner_bound) return 0;
    return ledger_stub_0044a910_BuildingPlacementPreviewUpdate(
        &s_building_placement_preview_hooks);
}

/* Entry point retained for entity-layer callers; the scored implementation
 * and transition-pool owner live in level.cpp. */
void ledger_stub_0044e6d0_TerrainCellMaterialTransitionBegin(
    TerrainSurfaceGrid* grid, int x, int y, unsigned int material,
    unsigned int rotation)
{
    if (x < 0 || y < 0) return;
    terrain_cell_material_transition_begin(
        grid, (uint32_t)x, (uint32_t)y, (int)material, (int)rotation);
}

// ScatterSpawnGateSet is implemented in spawn.cpp.

EntityObjectPositionSceneResolveHook
    s_entity_object_position_scene_resolve_hook;
void* s_entity_object_position_scene_resolve_context;

void entity_object_position_scene_resolve_hook_set(
    EntityObjectPositionSceneResolveHook hook, void* context)
{
    s_entity_object_position_scene_resolve_hook = hook;
    s_entity_object_position_scene_resolve_context = context;
}

/* Adapter only: EntityMotionToSceneNodeSync (FUN_0044bc30) takes an entity
 * and delta, and drives its registered scene node. The additional hooks
 * object has no installed production owner in this port. */
void ledger_stub_0044bc30_EntityMotionToSceneNodeSync(
    EntitySlot* entity, float delta,
    const EntityMotionSceneSyncHooks* hooks)
{
    entity_motion_to_scene_node_sync(entity, delta, hooks);
}

/* Adapter only: EntityAuxiliaryPayloadRelease (FUN_0044be30) takes one
 * entity and operates on its attached node plus a global deferred-release
 * list. The explicit release hooks are not its ABI or installed owner. */
void ledger_stub_0044be30_EntityAuxiliaryPayloadRelease(
    EntitySlot* entity, const EntityAuxiliaryReleaseHooks* hooks)
{
    entity_auxiliary_payload_release(entity, hooks);
}

// DriftOffsetStepApply
void ledger_stub_0044d0d0_DriftOffsetStepApply(float delta)
{
    motion_drift_offset_step(delta);
}

uint32_t motion_config_profile_entries_release(
    void* config, MotionConfigProfileEntry* entries, uint32_t count,
    const MotionConfigProfileReleaseHooks* hooks)
{
    void* profile = NULL;
    uint32_t i;
    uint32_t released = 0u;
    if (entries == NULL) return 0u;
    if (hooks != NULL && hooks->profile_open != NULL)
        profile = hooks->profile_open(hooks->context, config);
    for (i = 0; i < count; ++i) {
        MotionConfigProfileEntry* entry = &entries[i];
        if (entry->key == NULL) continue;
        if (profile != NULL && hooks != NULL && hooks->profile_write != NULL) {
            hooks->profile_write(hooks->context, profile, entry->key, "=",
                entry->direct_value != 0 ? entry->direct_value
                                         : entry->fallback_value);
        }
        if (hooks != NULL && hooks->key_release != NULL)
            hooks->key_release(hooks->context, entry->key);
        else
            engine_heap_free(entry->key);
        entry->key = NULL;
        entry->direct_value = 0;
        entry->fallback_value = 0;
        ++released;
    }
    if (profile != NULL && hooks != NULL && hooks->profile_close != NULL)
        hooks->profile_close(hooks->context, profile);
    return released;
}

// SurfaceVertexSpanValidate
int ledger_stub_0044e8c0_SurfaceVertexSpanValidate(
    TerrainSurfaceGrid* grid, const int* cell_xy, unsigned int cell_count,
    float maximum_height_span, float maximum_edge_delta)
{
    return terrain_surface_vertex_span_validate(grid, cell_xy, cell_count,
                                                maximum_height_span,
                                                maximum_edge_delta);
}

// TerrainCellListSurfaceHeightClampMutate
void ledger_stub_0044ead0_TerrainCellListSurfaceHeightClampMutate(
    TerrainSurfaceGrid* grid, const int* cell_xy, unsigned int cell_count,
    float maximum_offset)
{
    terrain_cell_surface_height_clamp(grid, cell_xy, cell_count,
                                      maximum_offset);
}

// TerrainCellSurfaceTriangleVerticesSelect
void ledger_stub_0044fab0_TerrainCellSurfaceTriangleVerticesSelect(
    TerrainSurfaceGrid* grid, float x, float z, unsigned int out_vertices[3])
{
    terrain_cell_surface_triangle_vertices_select(grid, x, z, out_vertices);
}

// TerrainSurfacePointProjectOffset

} // namespace unported_ledger_stubs

EntityQuadCollisionHooks s_entity_quad_collision_hooks;

void entity_quad_collision_hooks_set(
    const EntityQuadCollisionHooks* hooks)
{
    if (hooks != NULL) s_entity_quad_collision_hooks = *hooks;
    else memset(&s_entity_quad_collision_hooks, 0,
                sizeof(s_entity_quad_collision_hooks));
}

namespace unported_ledger_stubs {

// EntityIndexedFloatLookupWithMinusOneFallback
float ledger_stub_0046f1d0_EntityIndexedFloatLookupWithMinusOneFallback(
    int weapon_type, const void* entity_record)
{
    return lego::entity_indexed_float_lookup_with_minus_one_fallback(
        weapon_type, entity_record);
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// BuildingUpgradeResourceAdmissionCheck
int ledger_stub_00438770_BuildingUpgradeResourceAdmissionCheck(
    EntitySlot* building, uint32_t* out_secondary_cost)
{
    return building_upgrade_resource_admission_check(
        building, out_secondary_cost);
}

// BuildingUpgradeCommit
void ledger_stub_004387f0_BuildingUpgradeCommit(EntitySlot* building)
{
    building_upgrade_commit(building);
}

// EntityActionKindPrerequisiteCheck
int ledger_stub_004388b0_EntityActionKindPrerequisiteCheck(
    EntitySlot* entity, int action)
{
    return entity_action_kind_prerequisite_check(entity, action);
}

// EntityRouteMarkerCandidateFind
EntitySlot* ledger_stub_004389e0_EntityRouteMarkerCandidateFind(
    EntitySlot* source, int include_secondary)
{
    return entity_route_marker_candidate_find(source, include_secondary);
}

// EntityBuildPlacementCommandBatchExpand
void ledger_stub_00439280_EntityBuildPlacementCommandBatchExpand(
    const int cell_xy[2])
{
    entity_build_placement_command_batch_expand(cell_xy);
}

// EntityBuildPlacementCommandPairCreate
void ledger_stub_00439340_EntityBuildPlacementCommandPairCreate(
    const int cell_xy[2], int entity_id)
{
    entity_build_placement_command_pair_create(cell_xy, entity_id);
}

// ToolNameStringResourcesLoad
void ledger_stub_00439950_ToolNameStringResourcesLoad(void* string_loader,
                                                     void* config)
{
    entity_tool_name_strings_load(string_loader, config);
}

// GatedStepCommandIssueWithTargetRecord
int ledger_stub_00439a20_GatedStepCommandIssueWithTargetRecord(
    EntitySlot* entity, EntitySlot* target)
{
    return entity_gated_step_command_issue(entity, target);
}

// TaskTargetCellEligibilityCheck
int ledger_stub_00439b80_TaskTargetCellEligibilityCheck(
    const EntitySlot* entity, const EntitySlot* target)
{
    return entity_task_target_cell_eligibility_check(entity, target);
}

// EntityAcceptsTargetForTaskCheck
int ledger_stub_00439bd0_EntityAcceptsTargetForTaskCheck(
    const EntitySlot* entity, const EntitySlot* target, int apply_extra_gate)
{
    return entity_accepts_target_for_task_check(entity, target,
                                                apply_extra_gate);
}

// EntityIndexedFlagSetAndRefresh
void ledger_stub_00439c80_EntityIndexedFlagSetAndRefresh(EntitySlot* entity)
{
    entity_indexed_flag_set_and_refresh(entity);
}

// VehicleUnloadAdjacentCellFind
int ledger_stub_00439d50_VehicleUnloadAdjacentCellFind(
    const EntitySlot* entity, int out_xy[2])
{
    return entity_vehicle_unload_adjacent_cell_find(entity, out_xy);
}

// VehicleUnloadCellFindIfEligible
int ledger_stub_00439e10_VehicleUnloadCellFindIfEligible(
    const EntitySlot* entity, int out_xy[2])
{
    return entity_vehicle_unload_cell_find_if_eligible(entity, out_xy);
}

// EntityDepositActivityGate
int ledger_stub_0043a300_EntityDepositActivityGate(EntitySlot* entity,
                                                   EntitySlot* target)
{
    return entity_deposit_activity_gate(entity, target);
}

// EntityVisualResourceGetOrCreate
void ledger_stub_0043a5f0_EntityVisualResourceGetOrCreate(EntitySlot* entity)
{
    entity_visual_resource_get_or_create(entity);
}

// ActionKindEntityCreateAndCommandIssue
EntitySlot* ledger_stub_0043a650_ActionKindEntityCreateAndCommandIssue(
    EntitySlot* source, int kind, int selector, int argument)
{
    return entity_action_kind_entity_create_and_command_issue(
        source, kind, selector, argument);
}

// EntityActionRecordConsumeAndCleanup
void ledger_stub_0043a850_EntityActionRecordConsumeAndCleanup(
    EntitySlot* entity, EntitySlot* record)
{
    entity_action_record_consume_and_cleanup(entity, record);
}

// EntityPairInteractionGate
void ledger_stub_0043a9f0_EntityPairInteractionGate(
    EntitySlot* entity, const EntitySlot* partner)
{
    entity_pair_interaction_gate(entity, partner);
}

// ProximityActivityRaise
void ledger_stub_0043aab0_ProximityActivityRaise(EntitySlot* entity)
{
    entity_proximity_activity_raise(entity);
}

/* The sound owner registers the SFX sample callback. This owner registers
 * only the entity scene-end callback; registering a second sound resolver
 * here would submit every entry sample twice. */
static EntityDDiSceneOwnerResolveHook s_ddiscene_owner_resolve;
static void* s_ddiscene_owner_resolve_context;

static void entity_ddiscene_end_callback(void* scene_object, void* context)
{
    EntitySlot* entity;
    uint32_t owner_token;
    (void)context;
    if (scene_object == NULL || s_ddiscene_owner_resolve == NULL) return;
    owner_token = scene_object_owner_get(scene_object);
    if (owner_token == 0u) return;
    entity = s_ddiscene_owner_resolve(owner_token,
                                     s_ddiscene_owner_resolve_context);
    if (entity != NULL) entity->flags_3e4 |= 0x00010000u;
}

void entity_ddiscene_callbacks_install(
    EntityDDiSceneOwnerResolveHook owner_resolve, void* context)
{
    s_ddiscene_owner_resolve = owner_resolve;
    s_ddiscene_owner_resolve_context = context;
    ddiscene_frame_callbacks_set(NULL, NULL, 1,
                                 entity_ddiscene_end_callback, NULL);
}

static EntitySpatialQueryHook s_spatial_entity_query_hook;
static void* s_spatial_entity_query_context;

void entity_spatial_query_hook_set(EntitySpatialQueryHook hook, void* context)
{
    s_spatial_entity_query_hook = hook;
    s_spatial_entity_query_context = context;
}

static int spatial_entity_query_adapter(EntitySlot* candidate, void* payload)
{
    SpatialEntityQueryRequest* request = (SpatialEntityQueryRequest*)payload;
    if (request == NULL) return 0;
    if (s_spatial_entity_query_hook != NULL) {
        return s_spatial_entity_query_hook(s_spatial_entity_query_context,
            request->subject, candidate, request->position, request->values);
    }
    return request->callback_kind == 0
        ? spatial_entity_damage_radius_callback(candidate, request)
        : spatial_entity_vector_step_callback(candidate, request);
}

int entity_spatial_query_dispatch_kind(EntitySlot* subject,
                                              const float position[2],
                                              const float values[3],
                                              int callback_kind)
{
    SpatialEntityQueryRequest request;
    GameContext* context;
    if (subject == NULL || position == NULL || values == NULL) return 0;
    request.subject = subject;
    request.position[0] = position[0];
    request.position[1] = position[1];
    request.values[0] = values[0];
    request.values[1] = values[1];
    request.values[2] = values[2];
    request.callback_kind = callback_kind;
    context = entity_pool_callback_context_get();
    return entity_pool_find_by_callback_filtered(context,
        spatial_entity_query_adapter, &request);
}

int entity_spatial_query_dispatch(EntitySlot* subject,
                                  const float position[2],
                                  const float values[3])
{
    return entity_spatial_query_dispatch_kind(subject, position, values, 0);
}

// EntityCommandStatusFeedbackDispatch
void ledger_stub_00428020_EntityCommandStatusFeedbackDispatch(EntitySlot* entity)
{
    entity_command_status_feedback_dispatch(entity);
}

// PopEntityStateOverride
int ledger_stub_00459240_PopEntityStateOverride(EntitySlot* entity)
{
    return entity_state_override_pop(entity);
}

// EntityVisualResourceLoad
void* ledger_stub_00473690_EntityVisualResourceLoad(void* scene_object)
{
    return entity_visual_resource_load(scene_object);
}

} // namespace unported_ledger_stubs

int entity_motion_scale_checked(EntitySlot* entity, float delta,
                                 float* movement_delta, float* scale)
{
    float actual_delta = 0.0f, actual_scale = 0.0f;
    if (entity == NULL || entity->stats_record == NULL ||
        movement_delta == NULL || scale == NULL) return 0;
    if (!unported_ledger_stubs::entity_movement_scale_impl(
            entity, delta, &actual_delta, &actual_scale)) return 0;
    if (actual_scale != actual_scale || fabs(actual_scale) > FLT_MAX ||
        actual_delta != actual_delta || fabs(actual_delta) > FLT_MAX) return 0;
    if ((entity->flags_3e8 & 0x20000u) == 0u && entity->type != 10)
        *movement_delta = actual_delta;
    *scale = actual_scale;
    return 1;
}

void entity_actor_activity_advance_hook_set(EntityActorActivityAdvanceHook hook,
                                            void* context)
{
    s_actor_activity_advance_hook = hook;
    s_actor_activity_advance_context = context;
}

/* MeshLOD_Load (FUN_0044a7d0) with the original's arguments, for
 * game/animation_object.c (game/entity.h). The port's 0x0044a7d0 is a
 * ledger stub with another contract and there is no mesh-list loader yet:
 * the adapter loads nothing (finding 549). */
extern "C" ModelEntryRecord* mesh_lod_load(BlockFile* config,
                                           const char* gameName,
                                           const char* filename, int lod,
                                           unsigned int count)
{
    (void)config;
    (void)gameName;
    (void)filename;
    (void)lod;
    (void)count;
    return NULL;
}
