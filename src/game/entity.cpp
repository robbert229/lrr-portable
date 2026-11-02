// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "ai.hpp"
#include "entity.hpp"
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
extern float random_float_range(float minimum, float maximum);

static uint32_t s_next_ddiscene_owner_token = 1u;
static char* s_entity_kind_names[16];
static uint32_t s_entity_kind_name_counts[16];
static char* s_entity_localized_names[16];
static char* s_entity_localized_the_names[16];
static uint32_t s_entity_localized_name_counts[16];
static const char** s_entity_localized_name_entries[16];
static const char** s_entity_localized_the_name_entries[16];
static const void* s_stats_cell_ptr[STATS_TYPE_TABLE_TYPES][STATS_TYPE_TABLE_ROWS];
// GLOBAL: LEGORR 0x00502bd8
static const void** s_stats_type_table[STATS_TYPE_TABLE_TYPES] = {
    s_stats_cell_ptr[0], s_stats_cell_ptr[1], s_stats_cell_ptr[2],
    s_stats_cell_ptr[3], s_stats_cell_ptr[4], s_stats_cell_ptr[5],
    s_stats_cell_ptr[6], s_stats_cell_ptr[7], s_stats_cell_ptr[8],
    s_stats_cell_ptr[9], s_stats_cell_ptr[10], s_stats_cell_ptr[11],
    s_stats_cell_ptr[12], s_stats_cell_ptr[13], s_stats_cell_ptr[14],
    s_stats_cell_ptr[15], s_stats_cell_ptr[16], s_stats_cell_ptr[17],
    s_stats_cell_ptr[18], s_stats_cell_ptr[19]
};
static int s_stats_cell_span[STATS_TYPE_TABLE_TYPES][STATS_TYPE_TABLE_ROWS];
// GLOBAL: LEGORR 0x00502c28
static float s_stats_cell_level_limit[STATS_TYPE_TABLE_TYPES]
                                     [STATS_TYPE_TABLE_ROWS];

/* EntityTypeComponentGet (FUN_0044a200): dispatch through the legacy record's
 * payload slots. This mapped leaf accepts that corpus-layout record directly;
 * the higher-level EntitySlot sidecar is intentionally not treated as a
 * binary-compatible record. */
// FUNCTION: LEGORR 0x0044a200
void* entity_type_component_get(const void* entity_record)
{
    const uint32_t* words = (const uint32_t*)entity_record;
    switch (words[0]) {
    case 1:
        return (void*)(uintptr_t)scene_object_resource_get(
            (const void*)(uintptr_t)words[3]);
    case 2:
        return (void*)(uintptr_t)animation_object_handle_get(
            (const void*)(uintptr_t)words[4]);
    case 3:
        return (void*)(uintptr_t)animation_object_handle_get(
            (const void*)(uintptr_t)words[5]);
    case 4:
        return (void*)(uintptr_t)animation_object_handle_get(
            (const void*)(uintptr_t)words[6]);
    case 10:
        return (void*)(uintptr_t)animation_object_handle_get(
            (const void*)(uintptr_t)words[8]);
    default:
        if ((words[0xfa] & 0x00020000u) != 0u) {
            return (void*)(uintptr_t)words[7];
        }
        return (void*)entity_record;
    }
}

void entity_slot_type_component_set(EntitySlot* entity, void* component)
{
    if (entity != NULL) entity->type_component_handle = component;
}

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
static EntitySceneSlotRuntimeOps s_entity_scene_slot_runtime_ops;
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
typedef struct EntityActionConsumeCounterSidecar {
    EntitySlot* owner;
    uint32_t* values;
    struct EntityActionConsumeCounterSidecar* next;
} EntityActionConsumeCounterSidecar;
static EntityActionConsumeCounterSidecar* s_entity_action_counter_sidecars;
static EntityClippedRectangleHooks s_entity_clipped_rectangle_hooks;
static EntityPairRayHitCheckHook s_entity_pair_ray_hit_check_hook;
static void* s_entity_pair_ray_hit_check_context;
static void* s_entity_health_threshold_context;
static EntityStateOverrideEntry s_entity_state_override_entries[64];
static uint32_t s_entity_state_override_count;

static uint32_t* entity_action_consume_counter_sidecar_get(EntitySlot* owner,
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

static void entity_action_consume_counter_sidecar_release(EntitySlot* owner)
{
    EntityActionConsumeCounterSidecar** link =
        &s_entity_action_counter_sidecars;
    while (*link != NULL) {
        EntityActionConsumeCounterSidecar* entry = *link;
        if (entry->owner == owner) {
            *link = entry->next;
            engine_heap_free(entry->values);
            free(entry);
            break;
        }
        link = &entry->next;
    }
    if (owner != NULL && owner->type == 3) {
        memset(owner->action_consume_counters, 0,
               sizeof(owner->action_consume_counters));
        owner->action_consume_counters_initialized = 0u;
    }
}

typedef struct EntityPathMotionRuntime {
    EntitySlot* owner;
    int waypoint_index;
    float total_length;
    float progress;
    float carry_progress;
    float start[2];
    float target[2];
    float vector[2];
    uint32_t sample_count;
    float samples[50][2];
    float segment_lengths[49];
} EntityPathMotionRuntime;

static EntityPathMotionRuntime s_entity_path_motion_runtime[
    ENTITY_TYPES * ENTITY_CAP];
static void entity_path_motion_runtime_clear(EntitySlot* entity);
namespace unported_ledger_stubs {
void entity_path_motion_runtime_clear_impl(EntitySlot* entity);
}
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
static EntityPeriodicTickHook s_entity_periodic_tick_hook;
static void* s_entity_periodic_tick_context;
static float s_periodic_timer25;

/* CreateRepeatedResourceInstances (FUN_00408e50): the parameter is a cell
 * coordinate pair. The descriptor owns the sequence counter and supplies the
 * zone written into each newly linked kind-7 command. */
// FUNCTION: LEGORR 0x00408E50
void entity_repeated_resource_instances_create(
    const int cell_xy[2], int kind, int variant, uint32_t value, int count)
{
    uint8_t* record;
    uint32_t zone;
    if (cell_xy == NULL || count <= 0) return;
    record = (uint8_t*)terrain_descriptor_find_exact_current(
        (const uint32_t*)cell_xy, NULL);
    if (record == NULL) return;
    if (kind == 7 && variant == 1) {
        uint32_t flags;
        memcpy(&flags, record + 0xd0u, sizeof(flags));
        flags |= 8u;
        memcpy(record + 0xd0u, &flags, sizeof(flags));
    }
    memcpy(&zone, record + 8u, sizeof(zone));
    while (count-- > 0) {
        uint32_t sequence;
        memcpy(&sequence, record + 0x1cu, sizeof(sequence));
        ++sequence;
        memcpy(record + 0x1cu, &sequence, sizeof(sequence));
        cmd_command_kind_record_issue((uint32_t)kind, (uint32_t)variant,
                                      value, cell_xy, zone, NULL, 0);
    }
}

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
        return s_entity_kind_names[kind] != NULL
            ? s_entity_kind_names[kind] + (size_t)index * 128u : NULL;
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

static int entity_display_name_config_index(const EntitySlot* entity)
{
    uint32_t i;
    int kind;
    if (entity == NULL || entity->type < 1 ||
        (entity->type > 4 && entity->type != 10)) return -1;
    kind = entity->type;
    for (i = 0u; i < s_entity_kind_name_counts[kind]; ++i) {
        if (strcmp(entity->type_name,
                   s_entity_kind_names[kind] + (size_t)i * 128u) == 0)
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
            ? localized_object_name_get((uint32_t)entity->type, index)
            : NULL;
    }
    if (entity->type == 7) {
        /* The retail table base is DAT_00556190, which is special-name slot
         * 1; the config table's first row is therefore entity selector 0. */
        if (entity->etype < 0 || entity->etype >= 8) return NULL;
        return localized_object_name_get(7u, entity->etype);
    }
    switch (entity->type) {
    case 6:
    case 8:
    case 9:
    case 11:
    case 12:
    case 13:
    case 15:
        return localized_object_name_get((uint32_t)entity->type, 0);
    default:
        return NULL;
    }
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
    if (s_entity_health_threshold_read_hook != NULL) {
        if (s_entity_health_threshold_read_hook(
                entity, &state, s_entity_health_threshold_context) == 0)
            return 1;
    }
    else {
        /* Exact retail words: entity +0x328, Stats +0xec, entity +0x324,
         * and entity +0x3d4. These already have typed EntitySlot/Stats
         * mirrors; no external threshold provider is required in gameplay. */
        state.upper_limit = entity->field_328;
        /* FUN_00469bb0 indexes the type/row record by entity +0x330; the
         * scalar stats-record mirror omits that instance stride. */
        state.secondary_limit = stats_instance_word_at_ec(entity);
        state.required_value = (uint32_t)entity->carry_state_324;
        state.health_percent = entity->activity_counter_3d4;
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

/* EntityBuildingIndexSet (FUN_00469c10): select one of the entity type's
 * 0x150-byte records and update the two transition counters. */
// FUNCTION: LEGORR 0x00469c10
static int entity_building_index_set(EntitySlot* entity, uint32_t state)
{
    int type;
    int row;
    int current;
    int span;
    const uint8_t* record;
    if (entity == NULL) return 0;
    type = entity->type;
    row = entity->etype;
    if (type < 0 || type >= STATS_TYPE_TABLE_TYPES || row < 0
        || row >= STATS_TYPE_TABLE_ROWS) {
        return 0;
    }
    span = s_stats_cell_span[type][row];
    record = (const uint8_t*)s_stats_type_table[type][row];
    if (record == NULL || state >= (uint32_t)span) return 0;
    current = entity->indexed_record_slot_330;
    if (state != (uint32_t)current) {
        if (current >= 0 && current < 16) {
            grid_count_table_slot_poke(type, row, current, 0,
                (uint32_t)(grid_count_table_slot_peek(type, row, current, 0)
                    + 1));
        }
        grid_count_table_slot_poke(type, row, (int)state, 1,
            (uint32_t)(grid_count_table_slot_peek(type, row, (int)state, 1)
                + 1));
    }
    entity->indexed_record_slot_330 = (int32_t)state;
    entity->stats_record = record + (size_t)state * 0x150u;
    return 1;
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

// FUNCTION: LEGORR 0x00439CD0
int entity_animation_state_set(EntitySlot* entity, uint32_t state)
{
    return entity_building_index_set(entity, state);
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
extern float random_float_range(float minimum, float maximum);
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
    float entity_position[2];
    if (entity == NULL || point_xy == NULL || direction_xy == NULL) return 0;
    entity_transform_position_get(entity, entity_position);
    return ((entity_position[0] - point_xy[0]) * direction_xy[0] +
            (entity_position[1] - point_xy[1]) * direction_xy[1]) < 0.0f;
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
    float entity_position[2];
    float dx;
    float dy;
    float radius;
    if (entity == NULL || start_xy == NULL || end_xy == NULL) return 0;
    if (check_side && entity_directed_line_side_check(entity, start_xy, end_xy))
        return 0;
    entity_transform_position_get(entity, entity_position);
    line_x = (end_xy[0] + start_xy[0]) - start_xy[0];
    line_y = (end_xy[1] + start_xy[1]) - start_xy[1];
    inv_length = 1.0f / (float)sqrt(line_x * line_x + line_y * line_y);
    line_x *= inv_length;
    line_y *= inv_length;
    projection = (entity_position[0] - start_xy[0]) * line_x +
                 (entity_position[1] - start_xy[1]) * line_y;
    projected_x = start_xy[0] + projection * line_x;
    projected_y = start_xy[1] + projection * line_y;
    dx = projected_x - entity_position[0];
    dy = projected_y - entity_position[1];
    radius = s_entity_segment_radius_hook != NULL
        ? s_entity_segment_radius_hook(entity)
        : entity_stats_float_at6c(entity);
    if (radius <= (float)sqrt(dx * dx + dy * dy)) return 0;
    return 1;
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
                                   float* max_distance_io,
                                   void* ignored_object)
{
    EntityRayHitSweepState state;
    if (hit_object_out != NULL) *hit_object_out = NULL;
    if (start == NULL || direction == NULL || max_distance_io == NULL) return 0;
    state.hit_object = NULL;
    state.max_distance = *max_distance_io;
    state.start = start;
    state.direction = direction;
    state.hit = 0;
    state.ignored_object = ignored_object;
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
        start, direction, &hit_object, &max_distance, first);
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
    state.ignored_object = NULL;
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
                                            const float* final_marker_position)
{
    AiWaypoint* route;
    GameContext* context;
    LevelState* level;
    const void* terrain;
    unsigned int i;
    int previous_direction = -1;
    int route_marker_failed = 0;
    if (entity == NULL || count > 0x7fffffffu ||
        (count != 0 && (x_values == NULL || y_values == NULL))) return 0;

    /* FUN_0043baf0 is the exact busy/ineligible gate at the top of the
     * retail installer. The route is scratch-owned until every segment has
     * passed, so a failed marker leaves the existing route intact. */
    if (entity_flags_mask107c33de_or_1c00203_any_test(entity)) return 0;

    if ((entity->flags_3ec & 0x40u) != 0u) {
        entity->link_2fc = 0;
        entity->flags_3ec &= ~0x40u;
    }
    if (count == 0u) {
        route = NULL;
    }
    else {
        if ((size_t)count > ((size_t)-1) / sizeof(AiWaypoint)) return 0;
        route = (AiWaypoint*)malloc((size_t)count * sizeof(AiWaypoint));
        if (route == NULL) return 0;
    }

    context = entity->owner_ctx != NULL ? entity->owner_ctx
                                        : entity_pool_callback_context_get();
    level = context != NULL ? context->level : NULL;
    terrain = level != NULL ? level->ptr_terrain : NULL;
    for (i = 0; i < count; ++i) {
        route[i].x = x_values[i];
        route[i].y = y_values[i];
        route[i].weight_x = 0.5f;
        route[i].weight_y = 0.5f;
        route[i].pending = 0;
        route[i].step_kind = 0;
        route[i].reserved[0] = 0;
        route[i].reserved[1] = 0;
        if (i + 1u == count && final_marker_position != NULL &&
            terrain != NULL) {
            float cell_center[2];
            float blocksize = terrain_blocksize_get(terrain);
            if (blocksize != 0.0f &&
                terrain_cell_world_center_xy_get(terrain,
                    (int)x_values[i], (int)y_values[i], cell_center)) {
                route[i].weight_x =
                    (final_marker_position[0] - cell_center[0]) / blocksize
                    + 0.5f;
                route[i].weight_y =
                    (cell_center[1] - final_marker_position[1]) / blocksize
                    + 0.5f;
            }
        }
        if ((final_marker_position == NULL || i + 1u != count) &&
            (entity->flags_3e8 & 0x1000u) == 0u &&
            (entity_state_flags_get(entity) & 0x20000000u) != 0u) {
            route[i].weight_y = random_float_range(0.3f, 0.7f);
            route[i].weight_x = random_float_range(0.3f, 0.7f);
        }
        if (i + 1u < count) {
            const int direction = cell_neighbor_direction_code(
                (int)x_values[i + 1u], (int)y_values[i + 1u],
                (int)x_values[i], (int)y_values[i]);
            if (direction < 0) {
                route_marker_failed = 1;
                break;
            }
            if (i != 0u && previous_direction != direction && terrain != NULL &&
                terrain_cell_route_and_occupancy_check(
                    entity, level, (int)x_values[i + 1u],
                    (int)y_values[i + 1u], (int)x_values[i],
                    (int)y_values[i], 0) == 2) {
                route[i].pending |= 0x10u;
            }
            previous_direction = direction;
            if (s_entity_route_segment_accept_hook != NULL &&
                s_entity_route_segment_accept_hook(entity, x_values[i],
                    y_values[i], x_values[i + 1u], y_values[i + 1u]) == -1) {
                route_marker_failed = 1;
                break;
            }
        }
    }
    if (route_marker_failed) {
        free(route);
        return 0;
    }

    /* Retail resets/releases the old sequence only after the replacement
     * records have been built. Transfer a long route to the native sidecar;
     * shorter routes stay in EntitySlot's retail-sized inline span. */
    entity_action_sequence_reset(entity, 0);
    if (count > AI_WAYPOINTS_MAX) {
        entity->wp_dynamic = route;
        entity->wp_capacity = count;
        route = NULL;
    }
    else {
        for (i = 0; i < count; ++i) entity->wp[i] = route[i];
    }
    free(route);
    entity->wp_count = (int)count;
    entity->wp_index = 0;
    entity->flags_3e8 &= ~0x400u;
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
            else
                /* FUN_00409b30 calls FUN_004323f0 for this stats-gated
                 * branch. Use the mapped current-level entry when no host
                 * override supplies that operation. */
                terrain_cell_flags_set_and_refresh_current(
                    (const int*)pair);
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
            else
                /* FUN_00409cd0 runs the four cardinal cell refreshes here,
                 * immediately before FUN_0040cea0 refreshes this pair. The
                 * current-level entrypoint resolves the live LevelState
                 * through the same engine binding used by other retail ABI
                 * callers. */
                cell_cardinal_neighbor_apply_current((const int*)pair);
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
    else
        unported_ledger_stubs::terrain_cell_refresh_eight_neighbors_current(
            (const int*)position);
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
    int had_waypoints;
    if (entity == NULL) return;
    had_waypoints = entity->wp_count > 0 || entity->wp_dynamic != NULL;
    if (had_waypoints && preserve_mode == 0) {
        unsigned int index = (unsigned int)entity->wp_index;
        const unsigned int count = (unsigned int)entity->wp_count;
        AiWaypoint* waypoints = ENTITY_WAYPOINTS(entity);
        while (index < count) {
            if (waypoints[index].step_kind == 3u) {
                EntitySlot* linked = (EntitySlot*)entity->link_2fc;
                if (linked != NULL) {
                    if (linked->type == 5) {
                        entity_type5_finalize_if_allowed(linked);
                    }
                    else {
                        linked->route_partner_300 = NULL;
                    }
                    entity->link_2fc = 0;
                }
                break;
            }
            if (waypoints[index].step_kind == 7u) {
                if (entity->link_2fc != 0) entity->link_2fc = 0;
                break;
            }
            ++index;
        }

        if (entity->type == 3) {
            EntitySlot* linked = (EntitySlot*)entity->link_2fc;
            if ((entity->flags_3e8 & 0x400u) != 0u && linked != NULL &&
                linked->type == 5) {
                unported_ledger_stubs::entity_attached_object_list_detach(
                    entity, 0);
            }
            cmd_chain_drop(entity, 4);
            cmd_chain_drop(entity, 10);
        }
        if ((entity->flags_3e8 & 8u) != 0u && entity->owner_ctx != NULL &&
            entity->owner_ctx->level != NULL) {
            const int cell_x = crt_float_to_long(entity->x);
            const int cell_y = crt_float_to_long(entity->y);
            level_tile_mark(entity->owner_ctx->level, cell_x, cell_y, 0);
        }
        entity_task_queue_flush(entity);
    }
    /* The mode-1 type-2 branch restores a pose through the legacy component
     * record at +0x10 using saved values at +0x298/+0x29c. EntitySlot has no
     * owner for that record or those saved fields, so that renderer restore
     * cannot be represented by this module. */
    /* The retail path releases its route sequence allocation. The portable
     * route extension owns a separate allocation after the inline waypoints. */
    free(entity->wp_dynamic);
    entity->wp_dynamic = NULL;
    entity->wp_capacity = 0u;
    entity->wp_count = 0;
    entity->wp_index = 0;
    entity_path_motion_runtime_clear(entity);
    entity->flags_3e0 &= 0xff7ffffeu;
    entity->flags_3e8 &= 0xffffbbffu;
    if (entity->link_2fc != 0) {
        EntitySlot* partner = (EntitySlot*)entity->link_2fc;
        partner->flags_3ec &= ~0x10u;
    }
    if ((entity->flags_3e0 & 2u) != 0) {
        entity->attached_direction[0] = -entity->attached_direction[0];
        entity->attached_direction[1] = -entity->attached_direction[1];
        entity->attached_direction[2] = -entity->attached_direction[2];
        entity->flags_3e0 &= ~2u;
    }
    ai_set_activity(entity, 0, 0);
    if (had_waypoints)
        entity_task_state_update_after_cleanup(entity, preserve_mode);
}

// FUNCTION: LEGORR 0x00441b30
void entity_state_reset(EntitySlot* entity, int clear_mode, int reset_mode)
{
    uint32_t old_flags_3e0;
    uint32_t old_flags_3e4;
    EntitySlot* partner;
    int preserve_link_flags;
    if (entity == NULL) return;
    entity_action_sequence_reset(entity, 0);
    if (reset_mode != 0)
        unported_ledger_stubs::entity_attached_object_list_detach(entity, 0);

    partner = (EntitySlot*)entity->pair_link_3c0;
    if ((entity->flags_3e4 & 2u) != 0u) {
        entity->flags_3e4 &= ~2u;
        ai_set_activity(entity, 0x27, 0);
        entity->flags_3e4 |= 0x100u;
        if (partner != NULL && (partner->flags_3e4 & 1u) != 0u) {
            partner->flags_3e4 &= ~1u;
            ai_set_activity(entity, 0, 0);
        }
    }
    if ((entity->flags_3e4 & 1u) != 0u) {
        entity->flags_3e4 &= ~1u;
        ai_set_activity(entity, 0, 0);
        if (partner != NULL && (partner->flags_3e4 & 2u) != 0u) {
            partner->flags_3e4 &= ~2u;
            ai_set_activity(partner, 0x27, 0);
            partner->flags_3e4 |= 0x100u;
        }
    }

    old_flags_3e4 = entity->flags_3e4;
    entity->flags_3e4 &= 0xfe3fbfdfu;
    old_flags_3e0 = entity->flags_3e0;
    entity->flags_3e0 = old_flags_3e0 & 0x6ffff7ffu;
    if ((old_flags_3e0 & 0x2000u) != 0u) {
        entity->flags_3e0 = old_flags_3e0 & 0x6fffd7ffu;
        cmd_chain_drop(entity, 5);
        entity_simulation_queue_flush(entity);
        if (entity->link_2fc != 0) {
            EntitySlot* linked = (EntitySlot*)entity->link_2fc;
            linked->flags_3ec &= 0xfeffffffu;
        }
    }
    entity->flags_3e0 &= 0xdffffdffu;
    entity->flags_3e8 &= 0xffffbfffu;
    if ((old_flags_3e4 & 0x200000u) != 0u)
        entity->flags_3e4 &= 0xffdfffffu;
    old_flags_3e4 = entity->flags_3e4;
    entity->flags_3e4 = old_flags_3e4 & 0xfffbffffu;
    if ((old_flags_3e0 & 0x1000000u) != 0u) {
        entity->flags_3e4 = old_flags_3e4 & 0xfff3ffffu;
        cmd_chain_drop(entity, 10);
        entity->flags_3e0 &= 0xfeffffffu;
    }

    if ((entity->flags_3e0 & 0x40000000u) != 0u)
        entity->flags_3e0 &= 0xbfffffffu;
    if ((entity->flags_3e4 & 0x40u) != 0u)
        entity->flags_3e4 &= ~0x40u;
    if ((entity->flags_3e4 & 0x400u) != 0u)
        entity->flags_3e4 &= ~0x400u;

    if ((entity->flags_3e4 & 0x8000u) != 0u) {
        EntitySlot* linked = (EntitySlot*)entity->link_2fc;
        preserve_link_flags = 0;
        if (linked != NULL && (entity_stats_word_get(linked) & 2u) != 0u) {
            if ((linked->flags_3e4 & 0x8000u) != 0u) {
                preserve_link_flags = 1;
            }
            else {
                cmd_chain_drop(entity, 0x15);
                linked->route_partner_300 = NULL;
                entity->link_2fc = 0;
            }
        }
        if (!preserve_link_flags)
            entity->flags_3e4 &= ~0x8000u;
    }

    if ((entity->flags_3e0 & 0x80000u) != 0u) {
        entity->flags_3e0 &= 0xfff7ffffu;
        entity_type5_finalize_if_allowed(entity);
    }

    old_flags_3e0 = entity->flags_3e0;
    entity->flags_3e0 = old_flags_3e0 & 0xffffefffu;
    if ((old_flags_3e0 & 0x40u) != 0u) {
        int cell_x = level_cell_of(entity->x);
        int cell_y = level_cell_of(entity->y);
        entity->flags_3e0 = (old_flags_3e0 & 0xffffefbfu) | 0x80000000u;
        if (entity->owner_ctx != NULL && entity->owner_ctx->level != NULL)
            level_tile_mark(entity->owner_ctx->level, cell_x, cell_y, 0);
    }

    old_flags_3e0 = entity->flags_3e0;
    entity->flags_3e0 = old_flags_3e0 & 0xfbffffffu;
    if ((entity->flags_3e4 & 4u) != 0u) {
        entity->link_2fc = 0;
        entity->flags_3e4 &= ~4u;
    }
    entity->flags_3e0 = old_flags_3e0 & 0x7bfa7fffu;
    entity->flags_3e4 &= 0xfffcfcffu;

    if (clear_mode != 0) {
        task_record_kind14_remove_if_head(entity);
        ai_set_activity(entity, 0, 1);
    }
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

static EntityTableMaps s_entity_table_maps;

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
 * entity chunk, accepts only records whose +0x408 word points to themselves,
 * optionally filters the +0x3e8 0x80000 bit, and returns on the first
 * callback match. pool_link_408 is the native-width mirror of that dword. */
static GameContext* s_entity_pool_callback_context;
#define OBJECT_POINTER_REGISTRY_CAPACITY 10u
// GLOBAL: LEGORR 0x005563b8
static uint32_t s_object_pointer_registry_words[
    OBJECT_POINTER_REGISTRY_CAPACITY];
// GLOBAL: LEGORR 0x005563e0
static uint32_t s_object_pointer_registry_count;
/* Native pointer sidecar follows the retail-shaped slots and count. */
static EntitySlot* s_object_pointer_registry_entities[
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

/* The retail pool walkers use the active context instead of receiving it as
 * an argument. Keep context-taking forms above as portable caller adapters. */
// FUNCTION: LEGORR 0x004377D0
int entity_pool_find_by_callback_current(
    int (*callback)(EntitySlot*, void*), void* payload, int filter_flagged)
{
    GameContext* ctx = s_entity_pool_callback_context;
    int table;
    if (ctx == NULL) {
        return 0;
    }
    for (table = 0; table < ENTITY_TYPES; ++table) {
        int count = ctx->entity_count[table];
        int i;
        if (count < 0) {
            count = 0;
        }
        if (count > ENTITY_CAP) {
            count = ENTITY_CAP;
        }
        for (i = 0; i < count; ++i) {
            EntitySlot* entity = &ctx->entity[table][i];
            if (entity->pool_link_408 != (uintptr_t)entity) {
                continue;
            }
            if (filter_flagged != 0 &&
                (entity->flags_3e8 & 0x80000u) != 0u) {
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
int entity_pool_find_by_callback_filtered_current(
    int (*callback)(EntitySlot*, void*), void* payload)
{
    return entity_pool_find_by_callback_current(callback, payload, 1);
}

int entity_pool_find_by_callback_filtered(GameContext* ctx,
                                          int (*callback)(EntitySlot*, void*),
                                          void* payload)
{
    return entity_pool_find_by_callback(ctx, callback, payload, 1);
}

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
typedef struct EntityIndexedValueGroup {
    uint32_t values[ENTITY_INDEXED_VALUE_CAPACITY];
    uint32_t count;
} EntityIndexedValueGroup;
// GLOBAL: LEGORR 0x005030d8
static EntityIndexedValueGroup
    s_entity_indexed_value_groups[ENTITY_INDEXED_VALUE_GROUPS];
#define ENTITY_INDEXED_RECORD_TABLES 320
struct EntityIndexedRecordTable {
    const void* const* records;
    uint32_t count;
};
static EntityIndexedRecordTable
    s_entity_indexed_record_tables[ENTITY_INDEXED_RECORD_TABLES];

static void* entity_spatial_object_query_hook(
    void* context, const SpatialObjectQueryRequest* query);
static void* entity_spatial_query_execute_current(
    const SpatialObjectQueryRequest* request);

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

// FUNCTION: LEGORR 0x00437510
static int entity_linked_route_clear_callback(EntitySlot* candidate,
                                              void* payload)
{
    EntitySlot* source = (EntitySlot*)payload;
    if (candidate != NULL && source != NULL &&
        (EntitySlot*)candidate->link_2fc == source) {
        entity_action_sequence_reset(candidate, 0);
        candidate->link_2fc = 0;
    }
    /* LAB_00437510 always continues so every matching link is cleared. */
    return 0;
}

/* EntityCallbackDispatchUnlessTypeFive (FUN_004374f0): type five is a
 * non-dispatchable source; otherwise clear every live candidate route that
 * points back to it, then notify the optional host observer. */
// FUNCTION: LEGORR 0x004374F0
void entity_callback_dispatch_unless_type_five(EntitySlot* entity)
{
    if (entity == NULL || entity->type == 5) return;
    entity_pool_find_by_callback_filtered_current(
        entity_linked_route_clear_callback, entity);
    if (s_entity_callback_dispatch_hook != NULL) {
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
            s_entity_teardown_hooks.context) : entity_stats_word_get(entity);
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
        if (entity->link_2fc != 0) {
            EntitySlot* linked = (EntitySlot*)entity->link_2fc;
            linked->flags_3ec &= ~0x40u;
        }
    }
    if ((flags & 0x80U) != 0) {
        if (entity->link_2fc != 0) {
            EntitySlot* linked = (EntitySlot*)entity->link_2fc;
            linked->flags_3ec &= ~0x80u;
        }
        entity->link_2fc = 0;
        entity->flags_3ec = (int32_t)(flags & ~0x80U);
        flags &= ~0x80U;
    }
    /* EntityTeardown calls TrackedEntityPointerArrayRemove here, before it
     * releases the entity's other registries and payload. */
    ai_idle_table_pointer_remove(entity); /* FUN_0043a930 */
    entity_pointer_registries_remove(entity);
    entity_type_operation_fallback_handle_set(entity, NULL);
    entity_action_sequence_reset(entity, 0);
    entity_type_four_callback_dispatch(entity);
    if (entity->attach_304 != NULL) {
        unported_ledger_stubs::entity_attached_object_list_detach(
            (EntitySlot*)entity->attach_304, 0);
    }
    unported_ledger_stubs::entity_attached_object_list_detach(entity, 0);
    /* Remove the token from the scene before the release callback can return
     * that renderer record to its pool. Keep the entity token valid through
     * the release callback itself. */
    entity_scene_slot_nodes_clear(entity);
    entity_activity_runner_release(entity);
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
    entity_pool_find_with_local_predicate(entity->owner_ctx, entity);
    if ((state_flags & 4U) != 0 || (state_flags & 2U) != 0) {
        if (s_entity_teardown_hooks.operation != NULL)
            s_entity_teardown_hooks.operation(entity,
                ENTITY_TEARDOWN_RELEASE_TYPE_DATA,
                s_entity_teardown_hooks.context);
        else task_callback_dispatch(entity);
    }
    /* EntityTeardown calls ObjectPointerRegistryClearMatch before releasing
     * the type-specific payload. Preserve each OList slot index as a hole. */
    object_pointer_registry_entity_remove(entity);
    if (s_entity_teardown_hooks.type_resource_release != NULL)
        s_entity_teardown_hooks.type_resource_release(entity, type,
            s_entity_teardown_hooks.context);
    if (s_entity_teardown_hooks.operation != NULL)
        s_entity_teardown_hooks.operation(entity, ENTITY_TEARDOWN_FINALIZE,
            s_entity_teardown_hooks.context);
    entity_path_motion_runtime_clear(entity);
    entity_linked_list_entries_remove_by_key(
        (uintptr_t)entity);
    entity_callback_dispatch_unless_type_five(entity);
    entity_action_consume_counter_sidecar_release(entity);
    if (s_entity_teardown_hooks.free_list_link != NULL)
        s_entity_teardown_hooks.free_list_link(entity,
            s_entity_teardown_free_head,
            s_entity_teardown_hooks.context);
    free(entity->wp_dynamic);
    entity->wp_dynamic = NULL;
    entity->wp_capacity = 0;
    entity->wp_count = 0;
    entity->wp_index = 0;
    entity->stats_record = NULL;
    entity->ddiscene_owner_token = 0u;
    active_entity_list_entity_remove(entity);
    entity->pool_link_408 = (uintptr_t)s_entity_teardown_free_head;
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
    GameContext* context;
    EntityActionPrerequisiteHooks* const hooks =
        &s_entity_action_prerequisite_hooks;
    if (entity == NULL || entity->type != 1) return 0;
    context = entity->owner_ctx;
    if (hooks->action_ready != NULL) {
        if (!hooks->action_ready(entity, action, hooks->context)) return 0;
    }
    else if ((record_flag_bit_test(
                  entity->attached_direction_record_type1,
                  (uint32_t)action, 1) & (1u << ((uint32_t)action & 31u))) == 0u) {
        return 0;
    }
    if (hooks->action_values_get != NULL) {
        if (!hooks->action_values_get(entity, action, values,
                                      hooks->context)) return 0;
    }
    else {
        values[0] = entity_mapped_indexed_record_word_at_fc_get(
            entity->type, entity->etype, action);
        values[1] = entity_mapped_indexed_record_word_at_fc_get(
            entity->type, entity->etype, action);
        values[2] = entity_mapped_indexed_record_word_at_fc_get(
            entity->type, entity->etype, action);
    }
    first_resource = hooks->resource_get != NULL
        ? hooks->resource_get(0u, hooks->context)
        : (context != NULL
            ? upgrade_resource_check(context->level, 0) : 0u);
    if (first_resource < values[2]) {
        second_resource = hooks->resource_get != NULL
            ? hooks->resource_get(1u, hooks->context)
            : (context != NULL
                ? upgrade_resource_check(context->level, 1) : 0u);
        secondary_cost = values[1] / 5u +
                         (values[0] % 5u != 0u ? 1u : 0u);
        if (second_resource < secondary_cost) return 0;
    }
    return 1;
}

typedef struct EntityRouteMarkerSearchRequest {
    const float* position;
    uint32_t flags;
    float best_distance;
    int cell_x;
    int cell_y;
    int action_argument;
    int requested_state;
    EntitySlot* match;
} EntityRouteMarkerSearchRequest;

static int entity_route_marker_query_match(
    EntitySlot* candidate, const EntityRouteMarkerSearchRequest* request)
{
    uint32_t mask;
    uint32_t stats;
    uint32_t state;
    int matched = 0;
    int cell[2];
    if (candidate == NULL || request == NULL) return 0;
    mask = request->flags;
    if (!entity_shared_eligibility_check(candidate, (int)(mask & 0x400u)))
        return 0;
    /* The first shared-eligibility call consumes and then removes this bit
     * from the callback's later OR-of-predicates mask. */
    mask &= ~0x400u;
    if ((mask & 0x4000u) != 0u &&
        (candidate->flags_3ec & 0x10u) != 0u) return 0;
    stats = entity_stats_word_get(candidate);
    state = entity_state_flags_get(candidate);

    if (mask == 0u) {
        matched = candidate->type == 4;
    }
    else {
        if ((mask & 0x0001u) != 0u && (stats & 8u) != 0u &&
            entity_action_kind_supported(candidate, request->cell_y,
                                         request->action_argument))
            matched = 1;
        if ((mask & 0x1000u) != 0u) {
            cell[0] = request->cell_x;
            cell[1] = request->cell_y;
            if (dock_definition_matches_cell(cell)) matched = 1;
        }
        if ((mask & 0x2000u) != 0u && (state & 0x600u) == 0u &&
            entity_shared_eligibility_check(candidate, 0)) matched = 1;
        if ((mask & 0x0100u) != 0u &&
            entity_flags_contain_requested_single_bit(
                candidate, request->requested_state)) matched = 1;
        if ((mask & 0x0040u) != 0u && (stats & 8u) == 0u &&
            candidate->type == 4) matched = 1;
        if ((mask & 0x0080u) != 0u && (state & 2u) != 0u &&
            candidate->field_300 == 0u) matched = 1;
        if ((mask & 0x0800u) != 0u &&
            (stats & 0x80000000u) != 0u) matched = 1;
        if ((mask & 0x0010u) != 0u && (stats & 0x10u) != 0u)
            matched = 1;
        if ((mask & 0x0020u) != 0u && (stats & 0x20u) != 0u)
            matched = 1;
        if ((mask & 0x0200u) != 0u && (stats & 0x40u) != 0u)
            matched = 1;
        if ((mask & 0x0002u) != 0u &&
            (((stats & 2u) != 0u && request->cell_x == 7) ||
             ((stats & 4u) != 0u && request->cell_x == 6)))
            matched = 1;
        if ((mask & 0x0004u) != 0u && (stats & 0x4000u) != 0u)
            matched = 1;
        if ((mask & 0x0008u) != 0u && (stats & 0x100000u) != 0u)
            matched = 1;
    }
    return matched;
}

// FUNCTION: LEGORR 0x00438fb0
static int entity_route_marker_candidate_visit(EntitySlot* candidate,
                                                void* payload)
{
    EntityRouteMarkerSearchRequest* request =
        (EntityRouteMarkerSearchRequest*)payload;
    float candidate_position[2];
    float dx, dy, distance;
    if (request == NULL || candidate == NULL) return 0;
    if (!entity_route_marker_query_match(candidate, request)) return 0;
    if (s_entity_route_marker_candidate_hook != NULL &&
        !s_entity_route_marker_candidate_hook(
            candidate, request->position, request->flags,
            s_entity_route_marker_candidate_context)) return 0;
    entity_transform_position_get(candidate, candidate_position);
    dx = candidate_position[0] - request->position[0];
    dy = candidate_position[1] - request->position[1];
    distance = (float)sqrt((double)(dx * dx + dy * dy));
    if (distance < request->best_distance) {
        request->match = candidate;
        request->best_distance = distance;
    }
    /* LAB_00438fb0 always returns zero: retain the nearest match while the
     * pool walk visits every otherwise eligible entity. */
    return 0;
}

void entity_route_marker_candidate_hook_set(
    EntityRouteMarkerCandidateHook hook, void* context)
{
    s_entity_route_marker_candidate_hook = hook;
    s_entity_route_marker_candidate_context = context;
}

/* EntityRouteMarkerCandidateFind (FUN_004389e0): retain the nearest pooled
 * entity within the corpus's 10,000-unit radius that passes LAB_00438fb0's
 * mask-selected predicates. */
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
    request.best_distance = 10000.0f;
    request.cell_x = 0;
    request.cell_y = 0;
    request.action_argument = 0;
    request.requested_state = 0;
    context = entity_pool_callback_context_get();
    if (context != NULL) {
        entity_pool_find_by_callback_filtered_current(
            entity_route_marker_candidate_visit, &request);
    }
    return request.match;
}

static void* entity_spatial_object_query_hook(
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

static void* entity_spatial_query_execute_current(
    const SpatialObjectQueryRequest* request)
{
    return entity_spatial_object_query_hook(
        entity_pool_callback_context_get(), request);
}

// FUNCTION: LEGORR 0x00438df0
EntitySlot* entity_lookup_at_world_position(const float world_position[2])
{
    struct EntityLookupRequestRecord {
        uint32_t result_word;
        uint32_t center_x_bits;
        uint32_t center_y_bits;
        uint32_t radius_bits;
        uint32_t mask;
        uint32_t filters[4];
        EntitySlot* native_result;
    } record;
    SpatialObjectQueryRequest query;
    if (world_position == NULL) return NULL;

    /* FUN_00438df0's callback context is exactly nine retail dwords:
     * result, center x/y, radius, mask, and four zero filters. Keep that
     * record shape explicit; the native pointer result remains a sidecar. */
    memset(&record, 0, sizeof(record));
    memcpy(&record.center_x_bits, &world_position[0], sizeof(uint32_t));
    memcpy(&record.center_y_bits, &world_position[1], sizeof(uint32_t));
    record.radius_bits = 0x461c4000u; /* exact retail 10,000.0f */
    record.mask = 8u;
    memcpy(query.center, &record.center_x_bits, sizeof(query.center));
    memcpy(&query.radius, &record.radius_bits, sizeof(query.radius));
    query.mask = record.mask;
    memcpy(query.filters, record.filters, sizeof(query.filters));
    query.center_is_cell = 0;
    /* The reference calls EntityPoolFindByCallbackFiltered directly. Use the
     * same active-pool path as the mask wrappers, rather than depending on an
     * optional level-module spatial-query hook being installed. */
    record.native_result = (EntitySlot*)entity_spatial_query_execute_current(
        &query);
    if (sizeof(void*) == sizeof(uint32_t))
        record.result_word = (uint32_t)(uintptr_t)record.native_result;
    return record.native_result;
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
            /* FUN_00438460's flag-8 branch calls TaskRecordCreateAndScanGrid
             * before its shared refresh. That helper creates command kind 9
             * and performs the production 0x403910 count-prune sweep. */
            task_record_create_and_scan_grid(entity);
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

static int queued_grid_animation_anchor_cell_get(void* anchor,
                                                  float position[2])
{
    EntitySlot* entity = (EntitySlot*)anchor;
    int cell_x, cell_y;
    if (entity == NULL || entity->owner_ctx == NULL ||
        entity->owner_ctx->level == NULL || position == NULL) return 0;
    cell_x = level_cell_of(entity->x);
    cell_y = level_cell_of(entity->y);
    entity_position_to_terrain_cell(entity, entity->owner_ctx->level,
                                    &cell_x, &cell_y);
    position[0] = (float)cell_x;
    position[1] = (float)cell_y;
    return 1;
}

static EntitySlot* queued_grid_animation_spatial_find(
    const float position[2], int type, int filter_a, int filter_b)
{
    SpatialObjectQueryRequest request;
    float world_position[3];
    int cell_x, cell_y;
    if (position == NULL) return NULL;
    cell_x = (int)position[0];
    cell_y = (int)position[1];
    request.center[0] = 0.0f;
    request.center[1] = 0.0f;
    request.radius = 10000.0f;
    request.mask = 1u;
    request.filters[0] = (uint32_t)type;
    request.filters[1] = (uint32_t)filter_a;
    request.filters[2] = (uint32_t)filter_b;
    request.filters[3] = 0u;
    request.center_is_cell = 0;
    if (terrain_cell_world_position_get(cell_x, cell_y, world_position)) {
        request.center[0] = world_position[0];
        request.center[1] = world_position[1];
    }
    return (EntitySlot*)spatial_object_query_execute(&request);
}

/* ProcessQueuedGridAnimations (FUN_00437300): accumulate scaled fractional
 * work, then consume one queued type-six animation for each whole unit. */
// FUNCTION: LEGORR 0x00437300
void process_queued_grid_animations(void* anchor, int mode, float delta)
{
    QueuedGridAnimationHooks* const hooks = &s_queued_grid_animation_hooks;
    float position[2];
    float scaled_delta;
    if (mode == -1) {
        scaled_delta = delta;
    }
    else if (hooks->mode_scale_get != NULL) {
        scaled_delta = hooks->mode_scale_get(mode, hooks->context) * delta;
    }
    else {
        scaled_delta = lego::weapon_type_discharge_rate_get(mode) * delta;
    }
    s_queued_grid_animation_fraction += scaled_delta;
    if (hooks->anchor_position_get != NULL) {
        if (!hooks->anchor_position_get(anchor, position, hooks->context))
            return;
    }
    else if (!queued_grid_animation_anchor_cell_get(anchor, position)) return;
    while (s_queued_grid_animation_fraction >= 1.0f) {
        EntitySlot* candidate = hooks->entity_find != NULL
            ? hooks->entity_find(position, 6, 0, 0, hooks->context)
            : queued_grid_animation_spatial_find(position, 6, 0, 0);
        EntitySlot* animation_entity;
        if (candidate == NULL) break;
        animation_entity = hooks->animation_entity_get != NULL
            ? hooks->animation_entity_get(candidate, 6, 0, 0, hooks->context)
            : entity_action_kind_entity_create_and_command_issue(
                  candidate, 6, 0, 0);
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
    memset(s_entity_indexed_value_groups, 0,
           sizeof(s_entity_indexed_value_groups));
}

/* IndexedValueAppend (FUN_00469a90): append to one of the eleven
 * three-value groups; the count dword occupies the fourth word of each
 * 16-byte group in the reference table. */
// FUNCTION: LEGORR 0x00469A90
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
#endif
void entity_indexed_value_append(int group, uint32_t value)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        mov ecx, dword ptr [esp + 4]
        mov eax, ecx
        shl eax, 4
        mov edx, dword ptr [eax + s_entity_indexed_value_groups + 0ch]
        lea eax, [eax + s_entity_indexed_value_groups + 0ch]
        lea ecx, [edx + ecx*4]
        mov edx, dword ptr [esp + 8]
        mov dword ptr [ecx*4 + s_entity_indexed_value_groups], edx
        mov ecx, dword ptr [eax]
        inc ecx
        mov dword ptr [eax], ecx
        ret
    }
#else
    uint32_t count;
    if (group < 0 || group >= ENTITY_INDEXED_VALUE_GROUPS) {
        return;
    }
    count = s_entity_indexed_value_groups[group].count;
    if (count < ENTITY_INDEXED_VALUE_CAPACITY) {
        s_entity_indexed_value_groups[group].values[count] = value;
        s_entity_indexed_value_groups[group].count = count + 1u;
    }
#endif
}

/* GroupedCodeTableFindValueIndex (FUN_0046a120): scan the count-prefixed
 * fixed groups in order and return the first group containing the value. */
// FUNCTION: LEGORR 0x0046A120
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
#endif
int entity_grouped_code_table_find_value_index(uint32_t value,
                                               int* out_group)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        push ebx
        push esi
        push edi
        mov edi, dword ptr [esp + 10h]
        xor esi, esi
        mov eax, offset s_entity_indexed_value_groups
    grouped_value_group_loop:
        mov edx, dword ptr [eax + 0ch]
        xor ecx, ecx
        test edx, edx
        jbe grouped_value_next_group
        mov edx, eax
    grouped_value_item_loop:
        cmp edi, dword ptr [edx]
        je grouped_value_found
        mov ebx, dword ptr [eax + 0ch]
        inc ecx
        add edx, 4
        cmp ecx, ebx
        jb grouped_value_item_loop
    grouped_value_next_group:
        add eax, 10h
        inc esi
        cmp eax, offset s_entity_indexed_value_groups + 0b0h
        jb grouped_value_group_loop
        xor eax, eax
        pop edi
        pop esi
        pop ebx
        ret
    grouped_value_found:
        mov eax, dword ptr [esp + 14h]
        mov dword ptr [eax], esi
        mov eax, 1
        pop edi
        pop esi
        pop ebx
        ret
    }
#endif
#if !defined(_MSC_VER) || !defined(_M_IX86)
    int group;
    for (group = 0; group < ENTITY_INDEXED_VALUE_GROUPS; ++group) {
        uint32_t i;
        for (i = 0; i < s_entity_indexed_value_groups[group].count; ++i) {
            if (s_entity_indexed_value_groups[group].values[i] == value) {
                if (out_group != NULL) *out_group = group;
                return 1;
            }
        }
    }
    return 0;
#endif
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
        record = (const uint8_t*)s_stats_type_table[group][index];
    }
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
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
#endif
uint32_t entity_indexed_record_word_at_b0_get(int group, int index, int slot)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 0ch]
        mov edx, dword ptr [eax*4 + s_stats_type_table]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esp + 8]
        mov edx, dword ptr [edx + ecx*4]
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [edx + eax + 0b0h]
        ret
    }
#else
    return entity_indexed_record_word_get(group, index, slot, 0xb0u);
#endif
}

/* IndexedRecordWordAtOffsetB4Get (FUN_00469af0) is the adjacent dword
 * reader used by EntityKindOneRandomSpawnTrigger. */
// FUNCTION: LEGORR 0x00469AF0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
#endif
uint32_t entity_indexed_record_word_at_b4_get(int group, int index, int slot)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 0ch]
        mov edx, dword ptr [eax*4 + s_stats_type_table]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esp + 8]
        mov edx, dword ptr [edx + ecx*4]
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [edx + eax + 0b4h]
        ret
    }
#else
    return entity_indexed_record_word_get(group, index, slot, 0xb4u);
#endif
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

/* MappedIndexedRecordWordAtFCGet (FUN_0046a0c0) is the corresponding
 * +0xfc accessor over the same table. */
// FUNCTION: LEGORR 0x0046A0C0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
#endif
uint32_t entity_mapped_indexed_record_word_at_fc_get(int group, int index,
                                                     int slot)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 0ch]
        mov edx, dword ptr [eax*4 + s_stats_type_table]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esp + 8]
        mov edx, dword ptr [edx + ecx*4]
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [edx + eax + 0fch]
        ret
    }
#else
    return entity_indexed_record_word_get(group, index, slot, 0xfcu);
#endif
}

/* FUNCTION: LEGORR 0x00406540
 * Pool storage is represented by the most recently active portable game
 * context; live slots are walked in the same callback-until-match style. */
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
    entity_pool_find_by_callback_filtered_current(
        periodic_entity_callback_adapter, payload);
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
    LevelState* level;
    EntitySlot* found;
};

static int entity_grid_position_match(EntitySlot* entity, void* payload)
{
    EntityGridFindQuery* const query = (EntityGridFindQuery*)payload;
    int cell_x = -1;
    int cell_y = -1;
    if (entity == NULL || query == NULL || query->level == NULL)
        return 0;
    entity_position_to_terrain_cell(entity, query->level, &cell_x, &cell_y);
    if (cell_x == query->x && cell_y == query->y) {
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
    query.level = s_entity_pool_callback_context->level;
    query.found = NULL;
    entity_pool_find_by_callback_filtered_current(
        entity_grid_position_match, &query);
    return query.found;
}

static EntitySlot* entity_at_grid_position_find_in_context(
    GameContext* context, LevelState* level, const int cell_xy[2])
{
    EntityGridFindQuery query;
    if (context == NULL || level == NULL || cell_xy == NULL) return NULL;
    query.x = cell_xy[0];
    query.y = cell_xy[1];
    query.level = level;
    query.found = NULL;
    (void)entity_pool_find_by_callback_filtered(
        context, entity_grid_position_match, &query);
    return query.found;
}

/* The raw record model retains the corpus +0x2f0 head and +0x60 next-link
 * offsets. */
#if defined(_MSC_VER) && defined(_M_IX86)
// FUNCTION: LEGORR 0x00403c00
__declspec(naked) void entity_activity_record_remove_by_type(
    void* entity, int type, void* action_context)
{
    __asm {
        mov al, byte ptr [s_global_enabled]
        push ebx
        push ebp
        push esi
        test al, 2
        push edi
        jne removed_head_return
        mov ebp, dword ptr [esp + 14h]
        xor ebx, ebx
        mov esi, dword ptr [ebp + 2f0h]
        test esi, esi
        je removed_head_return
        mov eax, dword ptr [esp + 18h]
    find_record:
        cmp eax, dword ptr [esi]
        je found_record
        mov ebx, esi
        mov esi, dword ptr [esi + 60h]
        test esi, esi
        jne find_record
    not_found_return:
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    found_record:
        mov eax, dword ptr [esi]
        cmp eax, 5
        je task_kind_gate
        cmp eax, 7
        je issue_kind_seven
        cmp eax, 6
        je task_kind_gate
        cmp eax, 3
        je task_kind_gate
        cmp eax, 9
        je task_kind_gate
        cmp eax, 0fh
        je task_kind_gate
        cmp eax, 14h
        jne unlink_record
    task_kind_gate:
        cmp eax, 7
        jne after_kind_seven
    issue_kind_seven:
        mov eax, dword ptr [esi + 5ch]
        mov edx, dword ptr [esi + 2ch]
        and eax, 800h
        lea ecx, [esi + 30h]
        push eax
        push ecx
        mov ecx, dword ptr [esi + 28h]
        lea eax, [esi + 8]
        push edx
        mov edx, dword ptr [esi + 24h]
        push eax
        mov eax, dword ptr [esi + 20h]
        push ecx
        push edx
        push eax
        call cmd_command_kind_record_issue
        add esp, 1ch
    after_kind_seven:
        cmp dword ptr [esi], 9
        jne create_task
        lea edi, [esi + 8]
        push 0
        push edi
        call level_tile_mark_current_target
        mov ecx, dword ptr [esi + 0ch]
        mov edx, dword ptr [edi]
        add esp, 8
        push ecx
        push edx
        call level_current_terrain_get
        push eax
        call terrain_cell_surface_type_reset
        add esp, 0ch
    create_task:
        mov edi, dword ptr [esp + 1ch]
        push 43160000h
        push edi
        call cmd_create_task
        add esp, 8
        push edi
        call cmd_minion_companion
        add esp, 4
    unlink_record:
        test ebx, ebx
        je unlink_head
        mov eax, dword ptr [esi + 60h]
        push 0
        push esi
        mov dword ptr [ebx + 60h], eax
        call cmd_free
        add esp, 8
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    unlink_head:
        mov ecx, dword ptr [esi + 60h]
        push 0
        push esi
        mov dword ptr [ebp + 2f0h], ecx
        call cmd_free
        add esp, 8
    removed_head_return:
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    }
}
#else
void entity_activity_record_remove_by_type(void* entity, int type,
                                           void* action_context)
{
    if (entity == NULL) return;
    if ((s_global_enabled & 2) != 0) return;
    void* current = NULL;
    void* previous = NULL;
    __builtin_memcpy(&current, (char*)entity + 0x2f0u, sizeof(current));
    while (current != NULL) {
        uint32_t record_type = 0;
        void* next = NULL;
        __builtin_memcpy(&record_type, current, sizeof(record_type));
        __builtin_memcpy(&next, (char*)current + 0x60u, sizeof(next));
        if (record_type == (uint32_t)type) {
            if (record_type == 7u) {
                const uint32_t* words = (const uint32_t*)current;
                cmd_command_kind_record_issue(
                    words[8], words[9], words[10], (const int*)&words[2],
                    words[11], (const float*)&words[12],
                    (words[23] & 0x800u) != 0u);
            }
            if (record_type == 9u) {
                const int* cell_xy = (const int*)((char*)current + 0x08u);
                level_tile_mark_current_target(cell_xy, 0);
                terrain_cell_surface_type_reset(
                    level_current_terrain_get(cell_xy[0], cell_xy[1]),
                    cell_xy[0], cell_xy[1]);
            }
            if ((record_type == 3u || record_type == 5u
                 || record_type == 6u || record_type == 7u
                 || record_type == 9u || record_type == 15u
                 || record_type == 20u) && action_context != NULL) {
                cmd_create_task((EntitySlot*)action_context, 0x43160000);
                cmd_minion_companion((EntitySlot*)action_context);
            }
            if (previous == NULL) {
                __builtin_memcpy((char*)entity + 0x2f0u, &next,
                                 sizeof(next));
            } else {
                __builtin_memcpy((char*)previous + 0x60u, &next,
                                 sizeof(next));
            }
            cmd_free(current, 0);
            return;
        }
        previous = current;
        current = next;
    }
}
#endif

// FUNCTION: LEGORR 0x00403b60
void entity_activity_records_refresh(void* entity)
{
#if !defined(_MSC_VER) || !defined(_M_IX86)
    if (entity == NULL) return;
#endif
    uint32_t flags = 0;
    uint32_t count = 0;
    __builtin_memcpy(&flags, (char*)entity + 0x3e0u, sizeof(flags));
    __builtin_memcpy(&count, (char*)entity + 0x324u, sizeof(count));
    if ((flags & 0x400u) == 0u || (flags & 0x2000u) != 0u
        || count == 0u) return;
    {
        uint32_t i = 0;
        do {
            void* value = NULL;
            __builtin_memcpy(&value, (char*)entity + 0x308u
                             + (size_t)i * sizeof(void*), sizeof(value));
            entity_activity_record_remove_by_type(entity, 5u, value);
            entity_activity_record_remove_by_type(entity, 6u, value);
            entity_activity_record_remove_by_type(entity, 9u, value);
            entity_activity_record_remove_by_type(entity, 15u, value);
            entity_activity_record_remove_by_type(entity, 7u, value);
            entity_activity_record_remove_by_type(entity, 4u, NULL);
            entity_activity_record_remove_by_type(entity, 20u, value);
            ++i;
        } while (i < count);
    }
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

/* FUNCTION: LEGORR 0x00409ff0. The three global pointer replacements and
 * teardown pair are exposed as operations because their owning registries
 * are not reconstructed in the entity module. */
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

static uint32_t s_entity_flag_mask_all_pass;

static int entity_flag_mask_match_callback(EntitySlot* entity, void* payload)
{
    const uint32_t mask = (uint32_t)(uintptr_t)payload;
    const uint32_t type = first_set_flag_ordinal(mask);
    if (entity != NULL && (uint32_t)entity->type == type &&
        entity->flags_390 != 0u) {
        s_entity_flag_mask_all_pass = 0u;
    }
    return 0; /* retail visits every filtered entity for this mask bit */
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

static uint32_t s_entity_iteration_guard_flags;
static uint32_t s_scene_dispatch_point_low;
static uint32_t s_scene_dispatch_point_high;
static uint32_t s_scene_dispatch_word_c;
static uint32_t s_scene_dispatch_word14;

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
        entity_pool_find_by_callback_filtered_current(
            scene_state_dispatch_callback, &context);
    }
}

static int entity_pool_teardown_callback(EntitySlot* entity, void* unused)
{
    (void)unused;
    return entity_teardown(entity);
}

/* EntityIterationCallbackGuard (FUN_0043b320): the ABI takes no arguments;
 * LAB_0043b350 tears down each entity admitted by the filtered pool walk. */
// FUNCTION: LEGORR 0x0043B320
void entity_iteration_callback_guard(void)
{
    s_entity_iteration_guard_flags |= 2u;
    entity_pool_find_by_callback_filtered_current(
        entity_pool_teardown_callback, NULL);
    s_entity_iteration_guard_flags &= ~2u;
}

/* FlagMaskCallbacksAllPass (FUN_0046a4c0): for each set bit 0..30, visit
 * filtered entities and fail if LAB_0046a490 finds a matching type with a
 * nonzero +0x390 word. */
// FUNCTION: LEGORR 0x0046A4C0
int entity_flag_mask_callbacks_all_pass(uint32_t mask)
{
    for (uint32_t index = 0; index < 31u; ++index) {
        uint32_t bit = 1u << index;
        s_entity_flag_mask_all_pass = 1u;
        if ((mask & bit) != 0u) {
            entity_pool_find_by_callback_filtered_current(
                entity_flag_mask_match_callback, (void*)(uintptr_t)bit);
            if (s_entity_flag_mask_all_pass == 0u) return 0;
        }
    }
    return 1;
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

typedef struct EntitySpawnRequestNestedSum {
    float value;
} EntitySpawnRequestNestedSum;

static int entity_spawn_request_nested_sum_add(EntitySlot* candidate,
                                                void* payload)
{
    EntitySpawnRequestNestedSum* sum =
        (EntitySpawnRequestNestedSum*)payload;
    if (candidate != NULL && candidate->type == 4 &&
        entity_shared_eligibility_check(candidate, 0) != 0) {
        sum->value += entity_stats_float_atdc(candidate);
    }
    return 0;
}

/* EntityStatePairPredicate (FUN_0043c200): the selected type/row is
 * compatible when its +0xdc value, the Barracks +0xdc value, and the
 * eligible type-4 instance sum have a positive total. */
static int entity_spawn_request_state_pair_predicate(int type, int row,
                                                     int other_type,
                                                     int other_row,
                                                     GameContext* context)
{
    EntitySpawnRequestNestedSum sum;
    float candidate_value = stats_table_float_at_dc(type, row);
    float other_value;
    if (candidate_value == 0.0f) return 1;
    sum.value = 0.0f;
    if (context != NULL) {
        (void)entity_pool_find_by_callback_filtered(
            context, entity_spawn_request_nested_sum_add, &sum);
    }
    other_value = stats_table_float_at_dc(other_type, other_row);
    return candidate_value + other_value + sum.value > 0.0f;
}

static int entity_spawn_request_pair_matches(const EntitySpawnRequest* request,
                                              const EntitySlot* requester,
                                              GameContext* context)
{
    uint32_t candidate_flags;
    uint32_t requester_flags;
    int32_t available_slots;
    int barracks_type = 0;
    int barracks_row = 0;
    if (request == NULL || requester == NULL || request->type < 0 ||
        request->type >= STATS_TYPE_TABLE_TYPES || request->kind < 0 ||
        request->kind >= STATS_TYPE_TABLE_ROWS) return 0;
    candidate_flags = stats_table_word_at_148(request->type, request->kind);
    requester_flags = entity_stats_word_get(requester);
    if (!(((candidate_flags & 0x1000u) != 0u &&
           (requester_flags & 0x10u) != 0u) ||
          ((candidate_flags & 0x800u) != 0u &&
           (requester_flags & 0x20u) != 0u) ||
          ((candidate_flags & 0x2000u) != 0u &&
           (requester_flags & 0x40u) != 0u) ||
          ((candidate_flags & 0x400000u) != 0u &&
           (requester_flags & 0x80000000u) != 0u))) return 0;

    /* FUN_0041f8e0 is a void no-op whose caller consumes the unmodified
     * return register. The port pins that residue to zero, matching the
     * established behavior in EntityAvailableResourceSpend. */
    available_slots = (int32_t)stats_table_word_at_b4(
        request->type, request->kind, 0);
    if (available_slots > 0) return 0;

    (void)entity_type_row_for_name("Barracks", &barracks_type,
                                   &barracks_row);
    return entity_spawn_request_state_pair_predicate(
        request->type, request->kind, barracks_type, barracks_row, context);
}

/* EntitySpawnRequestQueueProcess (FUN_0043f190) applies the decompiled
 * eligibility, child-state, request-mask, capacity-residue, and pair-state
 * gates before consuming a request. EntitySlot carries the simulation child
 * link; creation of the retail +0x32c scene wrapper remains renderer-owned. */
// FUNCTION: LEGORR 0x0043f190
void entity_spawn_request_queue_process(EntitySlot* requester)
{
    EntitySlot* existing_child;
    GameContext* context;
    uint32_t requester_flags;
    if (requester == NULL || requester->owner_ctx == NULL
        || s_entity_spawn_request_count == 0u
        || entity_shared_eligibility_check(requester, 0) == 0
        || (requester->flags_3ec & 0xc0u) != 0u) {
        return;
    }
    context = requester->owner_ctx;
    existing_child = requester->spawn_child_3cc;
    if (existing_child != NULL &&
        (entity_stats_word_get(existing_child) & 0x200000u) != 0u) return;
    requester_flags = entity_stats_word_get(requester);
    for (uint32_t i = 0; i < s_entity_spawn_request_count; ++i) {
        const EntitySpawnRequest* request = &s_entity_spawn_requests[i];
        const int type = request->type;
        const char* type_name = NULL;
        uint32_t config_kind = 0u;
        float position[3];
        EntitySlot* spawned;
        if (type < 1 || type > 19 ||
            !entity_spawn_request_pair_matches(request, requester, context))
            continue;
        if (type == 2) config_kind = 1u;
        else if (type == 3) config_kind = 3u;
        else if (type == 4) config_kind = 2u;
        if (config_kind != 0u) {
            const TypeConfigTable* type_table =
                entity_type_config_table_get(config_kind);
            if (type_table != NULL && type_table->names != NULL &&
                (uint32_t)request->kind < type_table->count) {
                type_name = type_table->names[request->kind];
            }
        }
        spawned = entity_create_with_position(context, type, type_name, 0,
                                              0.0f, 0.0f, 0.0f);
        if (spawned == NULL) continue;
        spawned->etype = request->kind;
        (void)entity_building_index_set(spawned, 0u);
        requester->spawn_child_3cc = spawned;
        global_object_field8c_subtract_for_level(context->level,
            (int)stats_table_word_at_b4(type, request->kind, 0));
        entity_activity_scene_state_update(context);
        if ((requester_flags & 0x40u) != 0u) {
            requester->flags_3ec |= 0x40u;
            spawned->flags_3ec |= 0x40u;
        }
        else if ((entity_stats_word_get(spawned) & 0x20000u) != 0u) {
            requester->flags_3ec |= 0x80u;
            spawned->flags_3ec |= 0x80u;
            spawned->link_2fc = (uintptr_t)requester;
        }
        position[0] = requester->x;
        position[1] = requester->y;
        position[2] = requester->z;
        if (requester->attached_scene_object != NULL) {
            (void)ddiscene_object_position_get(
                requester->attached_scene_object, NULL, position);
        }
        entity_world_position_set(spawned, position[0], position[1],
                                  requester->rot, 1);
        spawned->z = position[2];
        (void)entity_state_override_pop(spawned);
        spawned->flags_3e0 |= 0x200000u;
        ai_set_activity(spawned, 0x4e, 1);
        activity_register_bookkeeping(spawned);
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
    entity_path_motion_runtime_clear(slot);
    __builtin_memset(slot, 0, sizeof(*slot));
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
    /* Corpus spawn dispatch (FUN_00437d00): the type-2 minifigure arm
     * (at 0x437eb6) seeds its permitted-activity list with Activity_Stand
     * through the FUN_0044b370 appender — the first grant is value 0. */
    if (type == 2) {
        ai_activity_grant(ctx, slot, AI_ACT_STAND);
    }
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
            (void)entity_stats_record_bind(slot);
        }
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

/* EntityCreateWithPosition (FUN_00438610): create through the pooled entity
 * allocator, apply the reference position update, then initialize the mode
 * word for pooled kinds six and seven. */
// FUNCTION: LEGORR 0x00438610
EntitySlot* entity_create_with_position(GameContext* context, int type,
                                        const char* name, int value,
                                        float x, float y, float heading)
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
    /* The raw create argument is the row selector when no type name is
     * available; named creation resolves and rebinds through the same Stats
     * table. Static task states only bind their separate +0xcc capacity. */
    (void)entity_stats_record_bind(created);
    entity_world_position_set(created, x, y, heading, 1);
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

static GameContext* building_upgrade_context_get(EntitySlot* building)
{
    if (building != NULL && building->owner_ctx != NULL)
        return building->owner_ctx;
    return entity_pool_callback_context_get();
}

static uint32_t building_upgrade_config_cost_get(GameContext* context,
                                                  int selector)
{
    const char* key = selector != 0
        ? "Main BuildingUpgradeCostStuds" : "Main BuildingUpgradeCostOre";
    const int default_cost = selector != 0 ? 1 : 5;
    if (context == NULL || context->cfg == NULL) return (uint32_t)default_cost;
    return (uint32_t)block_get_int(context->cfg, key, default_cost);
}

static int building_upgrade_live_payment_select(EntitySlot* building,
                                                uint32_t* resource_out,
                                                uint32_t* amount_out)
{
    GameContext* context = building_upgrade_context_get(building);
    uint32_t ore_cost, studs_cost;
    if (resource_out != NULL) *resource_out = 0u;
    if (amount_out != NULL) *amount_out = 0u;
    if (building == NULL || context == NULL || context->level == NULL)
        return 0;
    ore_cost = building_upgrade_config_cost_get(context, 0);
    studs_cost = building_upgrade_config_cost_get(context, 1);
    if (upgrade_resource_check(context->level, 0) >= ore_cost) {
        if (amount_out != NULL) *amount_out = ore_cost;
        return 1;
    }
    if (upgrade_resource_check(context->level, 1) < studs_cost) return 0;
    if (resource_out != NULL) *resource_out = 1u;
    if (amount_out != NULL) *amount_out = studs_cost;
    return 1;
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
    if (building == NULL) return 0;
    if (hooks->maximum_level_get == NULL || hooks->current_level_get == NULL ||
        hooks->cost_get == NULL || hooks->resource_get == NULL) {
        GameContext* context = building_upgrade_context_get(building);
        uint32_t secondary_resource = 0u;
        uint32_t amount = 0u;
        uint32_t maximum;
        if (context == NULL || context->level == NULL) return 0;
        maximum = building_upgrade_admission_check(
            building->type, building->etype);
        if (maximum <= (uint32_t)building->indexed_record_slot_330 + 1u)
            return 0;
        if (!building_upgrade_live_payment_select(
                building, &secondary_resource, &amount)) return 0;
        if (out_secondary_cost != NULL && secondary_resource != 0u)
            *out_secondary_cost = amount;
        return 1;
    }
    if (hooks->maximum_level_get(building, hooks->context) <=
        hooks->current_level_get(building, hooks->context) + 1u) return 0;
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
    int previous_level;
    GameContext* context;
    int hook_payment;
    if (building == NULL || building->type != 4 ||
        !building_upgrade_resource_admission_check(building,
                                                   &secondary_cost)) {
        return;
    }
    context = building_upgrade_context_get(building);
    hook_payment = s_building_upgrade_admission_hooks.cost_get != NULL &&
        s_building_upgrade_admission_hooks.resource_get != NULL &&
        s_building_upgrade_admission_hooks.current_level_get != NULL &&
        s_building_upgrade_admission_hooks.maximum_level_get != NULL;
    resource = 0u;
    amount = 0u;
    if (hook_payment) {
        amount = s_building_upgrade_admission_hooks.cost_get(
            0u, s_building_upgrade_admission_hooks.context);
        if (s_building_upgrade_admission_hooks.resource_get(
                0u, s_building_upgrade_admission_hooks.context) < amount) {
            resource = 1u;
            amount = s_building_upgrade_admission_hooks.cost_get(
                1u, s_building_upgrade_admission_hooks.context);
        }
    }
    else if (!building_upgrade_live_payment_select(building, &resource,
                                                    &amount)) return;
    previous_level = building->indexed_record_slot_330;
    next_level = (unsigned int)previous_level + 1u;
    if (hooks->resource_spend != NULL)
        hooks->resource_spend(building, resource, amount, hooks->context);
    else if (context != NULL && context->level != NULL)
        upgrade_resource_debit((int)resource, (int)amount);
    building->activity_counter_3d8 = 0.0f;
    if (hooks->upgrade_request_clear != NULL)
        hooks->upgrade_request_clear(building, hooks->context);
    if (hooks->payload_level_set != NULL)
        hooks->payload_level_set(building, next_level, hooks->context);
    else
        entity_payload_state_set(building, (int)next_level);
    if (hooks->stats_level_set != NULL)
        hooks->stats_level_set(building, next_level, hooks->context);
    else
        (void)entity_animation_state_set(building, next_level);
    if (hooks->upgrade_effect_dispatch != NULL)
        hooks->upgrade_effect_dispatch(building, next_level, hooks->context);
    else {
        float position[3];
        if (entity_world_center_position_read(building, position))
            (void)lava_effect_select_and_place(9, position, NULL);
    }
    lego::building_upgrade_state_changed_dispatch(
        building->type, building->etype,
        previous_level, 0);
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
                if (entity->type == 11) {
                    /* DeferredOListEntityCreateAndLink calls FUN_0040cf50
                     * for type 0xb after its type-specific initialization. */
                    register_object_at_grid_position(entity);
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

static void stats_rec_float_set(EntitySlot* e, int off, float value)
{
    if (e == NULL || e->stats_record == NULL) return;
    __builtin_memcpy((uint8_t*)e->stats_record + off, &value, sizeof(value));
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
void entity_stats_float_ate8_set(EntitySlot* e, float value)
{
    stats_rec_float_set(e, 0xe8, value);
}
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

/* ObjectListEntityTypeResolve scans the loaded RockMonster, Vehicle,
 * MiniFigure, and Building name tables in that order. Keep the fixed names
 * below as a pre-config fallback for callers that run before GameConfigInit;
 * once a table is installed, its rows and names are authoritative. */
static int entity_type_config_name_find(uint32_t table_kind,
                                       const char* name, int type,
                                       int* row_out)
{
    const TypeConfigTable* table = entity_type_config_table_get(table_kind);
    uint32_t row;
    if (table == NULL || table->names == NULL) return 0;
    for (row = 0u; row < table->count; ++row) {
        if (table->names[row] != NULL &&
            strcasecmp_lite(table->names[row], name) == 0) {
            if (row_out != NULL) *row_out = (int)row;
            return type;
        }
    }
    return -1;
}

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
        return 0; /* unregistered; ObjectListLoadFile aborts on this result. */
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
    code = entity_type_config_name_find(3u, name, 3, &row);
    if (code != 0) {
        if (code < 0) code = 0;
        else {
            if (type_out != NULL) *type_out = code;
            if (row_out != NULL) *row_out = row;
            return 1;
        }
    }
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
    code = entity_type_config_name_find(4u, name, 1, &row);
    if (code < 0) code = 0;
    if (code == 0 && (entity_type_config_table_get(4u) == NULL ||
        entity_type_config_table_get(4u)->names == NULL))
    for (i = 0; i < 12; ++i) {
        if (strcasecmp_lite(s_vehicle_names[i].name, name) == 0) {
            code = s_vehicle_names[i].code; row = (int)i; break;
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
    code = entity_type_config_name_find(1u, name, 2, &row);
    if (code < 0) code = 0;
    if (code == 0 && (entity_type_config_table_get(1u) == NULL ||
        entity_type_config_table_get(1u)->names == NULL))
    for (i = 0; i < 1; ++i) {
        if (strcasecmp_lite(s_minifig_names[i].name, name) == 0) {
            code = s_minifig_names[i].code; row = (int)i; break;
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
    if (entity_type_config_table_get(3u) == NULL ||
        entity_type_config_table_get(3u)->names == NULL) for (i = 0; i < 8; ++i) {
        if (strcasecmp_lite(s_creature_names[i].name, name) == 0) {
            code = s_creature_names[i].code; row = (int)i; break;
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
    code = entity_type_config_name_find(2u, name, 4, &row);
    if (code < 0) code = 0;
    if (code == 0 && (entity_type_config_table_get(2u) == NULL ||
        entity_type_config_table_get(2u)->names == NULL))
    for (i = 0; i < 10; ++i) {
        if (strcasecmp_lite(s_building_names[i].name, name) == 0) {
            code = s_building_names[i].code; row = (int)i; break;
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
        return 0;
    }

    BlockFile* ol = block_open(olist);
    if (ol == NULL) {
        return 0;
    }

    /* OListEntityRecordParse clears DAT_005563b8 only after the resource
     * opened successfully, then appends created entities in file order. */
    object_pointer_registry_reset();

    int placed = 0;
    int parse_ok = 1;
    size_t placed_slot_count = 0u;
    size_t placed_slot_capacity = block_record_count(ol);
    EntitySlot** placed_slots = (EntitySlot**)calloc(
        placed_slot_capacity, sizeof(EntitySlot*));
    if (placed_slot_capacity != 0u && placed_slots == NULL) {
        block_close(ol);
        return 0;
    }
    /* Root = the file's first record; objects are its children (document
     * order, like the entity list the corpus walks). */
    BlockRec* rootrec = block_record_at(ol, 0);
    const char* rname = rootrec ? block_name(rootrec) : NULL;
    if (rname == NULL) {
        free(placed_slots);
        block_close(ol);
        return 0;
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
        const char* x_value = obj_val(ol, rname, name, "xPos");
        const char* y_value = obj_val(ol, rname, name, "yPos");
        const char* heading_value = obj_val(ol, rname, name, "heading");
        const char* health_value = obj_val(ol, rname, name, "health");
        int type_row = 0;
        float grid_x;
        float grid_y;
        float center[2];
        int cell_x;
        int cell_y;
        float blocksize;
        if (t == NULL || x_value == NULL || y_value == NULL ||
            heading_value == NULL) {
            parse_ok = 0;
            break;
        }
        ol_strncpy(op.type_name, sizeof(op.type_name), t);
        if (!entity_type_row_for_name(op.type_name, &op.type, &type_row)) {
            parse_ok = 0;
            break;
        }
        grid_x = (float)atof(x_value) - 1.0f;
        grid_y = (float)atof(y_value) - 1.0f;
        cell_x = crt_float_to_long(grid_x);
        cell_y = crt_float_to_long(grid_y);
        if (ctx->level == NULL || ctx->level->ptr_terrain == NULL ||
            !terrain_cell_world_center_xy_transform(
                ctx->level->ptr_terrain,
                cell_x, cell_y, center)) {
            parse_ok = 0;
            break;
        }
        blocksize = terrain_blocksize_get(ctx->level->ptr_terrain);
        op.x = center[0] + (grid_x - ((float)cell_x + 0.5f)) * blocksize;
        op.y = center[1] - (grid_y - ((float)cell_y + 0.5f)) * blocksize;
        op.heading = (float)atof(heading_value) *
            0.0055555557f * 3.1415927f;
        op.z = 0.0f;
        op.rot = op.heading;
        {
            const ParsedEntityHealthValue* parsed_health =
                parse_entity_health_value(
                    health_value != NULL ? health_value : "", 0, 0, 0);
            op.health = parsed_health != NULL
                ? (float)parsed_health->value : 0.0f;
        }
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
        {
            EntitySlot* slot = entity_create_with_position(ctx, op.type,
                op.name, 0, op.x, op.y, op.heading);
            if (slot != NULL) {
                ol_strncpy(slot->type_name, sizeof(slot->type_name),
                           op.type_name);
                slot->etype = type_row;
                (void)entity_stats_record_bind(slot);
                (void)entity_activity_runner_type_config_bind(slot);
                (void)entity_type_config_scene_slot_nodes_load(slot);
                ol_strncpy(slot->driving, sizeof(slot->driving), op.driving);
                slot->rot = op.heading;
                slot->field_340 = op.health != 0.0f
                    ? op.health : 100.0f;
                entity_set_model(slot,
                    op.model[0] != '\0' ? op.model : NULL);
                entity_scene_slot_table_rebuild(slot, 0);
                if (op.type == 6 || op.type == 7 || op.type == 8 ||
                    op.type == 9) {
                    cmd_create_task(slot);
                }
                object_pointer_registry_append(slot);
                slot->flags_3ec |= 0x200u;
                lego::building_upgrade_state_changed_dispatch(
                    op.type, type_row, 0, 1);
                entity_queue_for_rockfall_processing_if_eligible(slot);
                if (op.type == 12) {
                    slot->flags_3e8 |= 0x10000u;
                    /* Retail also sets scene action "FlapInWind" and frame 0.
                     * The portable scene owner has no action/frame setter,
                     * and its expansion-cell table has no represented owner. */
                }
                if (slot->type == 11) {
                    /* The .ol loader's type-0xb branch installs its instance
                     * in the current level's per-cell object registry. */
                    register_object_at_grid_position(slot);
                }
                if (placed_slots != NULL &&
                    placed_slot_count < placed_slot_capacity) {
                    placed_slots[placed_slot_count++] = slot;
                }
                ++placed;
            }
        }
    }

    if (!parse_ok) {
        free(placed_slots);
        block_close(ol);
        ctx->entity_placed = placed;
        return 0;
    }

    /* The corpus resolves `driving` references after every object has been
     * created, so forward references work. It links the source entity to the
     * named object, raises +0x3e4 bit 2, dispatches the capability-derived
     * activity event, then clears route bit 0x40 on both linked entities. */
    if (placed_slots != NULL) {
        size_t source_index;
        for (source_index = 0u; source_index < placed_slot_count;
             ++source_index) {
            EntitySlot* source = placed_slots[source_index];
            size_t target_index;
            if (source == NULL || source->driving[0] == '\0') {
                continue;
            }
            for (target_index = 0u; target_index < placed_slot_count;
                 ++target_index) {
                EntitySlot* target = placed_slots[target_index];
                uint32_t events;
                uint32_t capabilities;
                if (target == NULL ||
                    strcasecmp_lite(source->driving, target->name) != 0) {
                    continue;
                }
                source->link_2fc = (uintptr_t)target;
                source->flags_3e4 |= 4u;
                capabilities = entity_capability_flags_get(target);
                if ((capabilities & 0x100u) != 0u) {
                    events = 1u;
                }
                else if ((target->flags_3e8 & 0x01000000u) == 0u) {
                    events = 4u;
                }
                else if ((target->flags_3e8 & 0x02000000u) == 0u) {
                    events = 2u;
                }
                else {
                    events = 1u;
                }
                entity_linked_activity_event_dispatch(source, events);
                entity_linked_route_flag_clear(ctx, source);
                break;
            }
        }
    }
    free(placed_slots);
    block_close(ol);
    ctx->entity_placed = placed;
    return 1;
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

/* GatedStepCommandIssueWithTargetRecord (FUN_00439a20): resolve both cells,
 * install a one-point route for same-cell targets or the computed path for a
 * different cell, mark the last route step as kind 3, and retain the target. */
// FUNCTION: LEGORR 0x00439A20
int entity_gated_step_command_issue(EntitySlot* entity, EntitySlot* target)
{
    GameContext* context;
    LevelState* level;
    float entity_position[2];
    float target_position[2];
    int* path_x = NULL;
    int* path_y = NULL;
    float* route_x = NULL;
    float* route_y = NULL;
    int entity_cell_x, entity_cell_y;
    int target_cell_x, target_cell_y;
    uint32_t route_count = 0u;
    unsigned int i;
    int route_result;

    if (entity == NULL || target == NULL
        || entity_flag400_and_health_threshold_check(entity) != 0
        || !entity_accepts_target_for_task_check(entity, target, 1)) {
        return 0;
    }
    context = entity->owner_ctx;
    if (context == NULL) context = entity_pool_callback_context_get();
    level = context != NULL ? context->level : NULL;
    if (level == NULL || level->ptr_terrain == NULL) return 0;

    entity_transform_position_get(entity, entity_position);
    entity_transform_position_get(target, target_position);
    if (!terrain_world_xy_to_cell(level->ptr_terrain,
            entity_position[0], entity_position[1],
            &entity_cell_x, &entity_cell_y) ||
        !terrain_world_xy_to_cell(level->ptr_terrain,
            target_position[0], target_position[1],
            &target_cell_x, &target_cell_y)) return 0;

    if (entity_cell_x == target_cell_x && entity_cell_y == target_cell_y) {
        path_x = (int*)malloc(sizeof(*path_x));
        path_y = (int*)malloc(sizeof(*path_y));
        if (path_x == NULL || path_y == NULL) {
            free(path_x);
            free(path_y);
            return 0;
        }
        path_x[0] = target_cell_x;
        path_y[0] = target_cell_y;
        route_count = 1u;
    }
    else {
        if (!terrain_neighbor_path_check(entity,
                (uint32_t)entity_cell_x, (uint32_t)entity_cell_y,
                (uint32_t)target_cell_x, (uint32_t)target_cell_y,
                &path_x, &path_y, &route_count, NULL, NULL)) {
            return 0;
        }
    }
    if (route_count == 0u ||
        (size_t)route_count > ((size_t)-1) / sizeof(*route_x)) {
        free(path_x);
        free(path_y);
        return 0;
    }
    route_x = (float*)malloc((size_t)route_count * sizeof(*route_x));
    route_y = (float*)malloc((size_t)route_count * sizeof(*route_y));
    if (route_x == NULL || route_y == NULL) {
        free(route_x);
        free(route_y);
        free(path_x);
        free(path_y);
        return 0;
    }
    for (i = 0u; i < route_count; ++i) {
        route_x[i] = (float)path_x[i];
        route_y[i] = (float)path_y[i];
    }
    route_result = entity_route_setup_and_marker_associate(
        entity, route_count, route_x, route_y, target_position);
    free(route_x);
    free(route_y);
    free(path_x);
    free(path_y);
    if (!route_result || entity->wp_count <= 0) return 0;
    ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 3u;
    entity->link_2fc = (uintptr_t)target;
    return 1;
}

// FUNCTION: LEGORR 0x00439b80
int entity_task_target_cell_eligibility_check(
    const EntitySlot* entity, const EntitySlot* target)
{
    SpatialObjectQueryRequest request;
    float position[2];
    void* candidate;
    if (entity == NULL) return 0;
    if (entity->type != 1) return 1;
    if (target == NULL || target->etype != 0) return 0;
    if ((entity_capability_flags_get(entity) & 0x200u) != 0u) return 1;

    /* The reference's 0x438ae0 call is the live mask-2 spatial query. Its
     * arguments are the actor, target type (+0), and target selector
     * (+0x330). Read the host fields directly; EntitySlot is compact and is
     * not a byte-for-byte 0x14c-byte retail record. */
    memset(&request, 0, sizeof(request));
    entity_transform_position_get(entity, position);
    request.center[0] = position[0];
    request.center[1] = position[1];
    request.radius = 10000.0f;
    request.mask = 2u;
    request.filters[0] = (uint32_t)target->type;
    request.filters[1] = (uint32_t)target->indexed_record_slot_330;
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
    (void)activity_register_bookkeeping(entity);
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
        if (entity->scene_slot_nodes == NULL &&
            entity_type_config_scene_slot_nodes_load(entity)) {
            entity_scene_slot_table_rebuild(entity, entity->activity);
        }
        return;
    }
    entity_scene_slot_nodes_clear(entity);
    if (s_entity_visual_resource_create_hook != NULL) {
        s_entity_visual_resource_create_hook(entity,
                                              s_entity_visual_resource_context);
    }
    (void)entity_type_config_scene_slot_nodes_load(entity);
    entity_scene_slot_table_rebuild(entity, entity->activity);
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
    (void)entity_animation_state_set(entity, entity->indexed_record_flags);
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
    BlockFile* cfg = (BlockFile*)string_loader;
    const char* config_root = (const char*)config;
    int slot;
    for (slot = 0; slot < 11; ++slot) {
        if (s_entity_tool_name_load_hook != NULL) {
            s_entity_tool_name_handles[slot] = s_entity_tool_name_load_hook(
                string_loader, config, "ToolNames", slot,
                s_entity_tool_name_context);
        }
        else {
            const char* tool_name = ai_tool_type_name_get(slot);
            if (cfg != NULL && config_root != NULL) {
                const char* path = string_path_join(config_root, "ToolNames",
                                                    tool_name, (char*)NULL);
                const char* value = block_get_strptr(cfg, path);
                s_entity_tool_name_handles[slot] = string_duplicate(value);
            }
            else {
                s_entity_tool_name_handles[slot] = NULL;
            }
        }
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
        (void)activity_register_bookkeeping(entity);
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

typedef struct EntityTypeTwoPositionedDispatch {
    const EntitySlot* source;
    float inner_range;
    float outer_range;
    int mode;
    float source_position[3];
    int source_position_valid;
} EntityTypeTwoPositionedDispatch;

static int entity_type_two_positioned_candidate(EntitySlot* candidate,
                                                void* payload)
{
    EntityTypeTwoPositionedDispatch* dispatch =
        (EntityTypeTwoPositionedDispatch*)payload;
    EntityTypeTwoPositionedHooks* hooks = &s_entity_type_two_positioned_hooks;
    int state;
    int next_state;
    int enabled;
    int old_state;

    if (candidate == NULL || candidate == dispatch->source) return 0;
    state = candidate->positioned_render_state_34c;
    old_state = state;
    if (dispatch->mode != 0) {
        float position[3];
        float dx;
        float dy;
        float dz;
        float distance;
        void* component;

        if (state != 0 || !dispatch->source_position_valid) return 0;
        component = candidate->type_component_handle != NULL
            ? candidate->type_component_handle
            : candidate->attached_scene_object;
        if (component == NULL) return 0;
        if (hooks->position_get != NULL) {
            if (hooks->position_get(hooks->context, component, position) == 0)
                return 0;
        }
        else if (ddiscene_object_position_get(component, NULL, position) == 0) {
            return 0;
        }
        dx = position[0] - dispatch->source_position[0];
        dy = position[1] - dispatch->source_position[1];
        dz = position[2] - dispatch->source_position[2];
        distance = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
        if (distance > dispatch->outer_range) next_state = 2;
        else if (distance <= dispatch->inner_range) next_state = 1;
        else return 0;
        enabled = 0;
    }
    else {
        if (state == 0) return 0;
        next_state = 0;
        enabled = 1;
    }

    if (candidate->type >= 1 && candidate->type <= 3 &&
        hooks->render_tracks_set != NULL) {
        hooks->render_tracks_set(hooks->context, candidate,
                                 old_state != 0 ? old_state : next_state,
                                 enabled);
    }
    candidate->positioned_render_state_34c = next_state;
    /* LAB_0043b8d0 always continues the filtered entity-pool walk. */
    return 0;
}

/* TypeTwoEntityPositionedCallbackDispatch (FUN_0043b850): select a type-2
 * source (or its linked type-2 owner), capture its scene position when mode
 * is nonzero, then update per-candidate renderer state through the filtered
 * live-entity pool. */
// FUNCTION: LEGORR 0x0043B850
void entity_type_two_positioned_callback_dispatch(
    const EntitySlot* entity, void* scene_object, float inner_range,
    float outer_range, int mode)
{
    const EntitySlot* selected = entity;
    EntityTypeTwoPositionedDispatch dispatch;
    if (selected == NULL) return;
    if (selected->type != 2) {
        const EntitySlot* owner = (const EntitySlot*)selected->owner_36c;
        if (owner != NULL && owner->type == 2) selected = owner;
    }

    memset(&dispatch, 0, sizeof(dispatch));
    dispatch.source = selected;
    dispatch.inner_range = inner_range;
    dispatch.outer_range = outer_range;
    dispatch.mode = mode;
    if (mode != 0) {
        EntityTypeTwoPositionedHooks* hooks =
            &s_entity_type_two_positioned_hooks;
        if (hooks->position_get != NULL) {
            dispatch.source_position_valid = hooks->position_get(
                hooks->context, scene_object, dispatch.source_position) != 0;
        }
        else {
            dispatch.source_position_valid = ddiscene_object_position_get(
                scene_object, NULL, dispatch.source_position) != 0;
        }
    }
    (void)entity_pool_find_by_callback_filtered_current(
        entity_type_two_positioned_candidate, &dispatch);
}

void entity_task_count_context_set(GameContext* context,
                                   EntityTaskCoordinateMatchHook match,
                                   void* user)
{
    s_entity_task_count_context = context;
    s_entity_task_coordinate_match_hook = match;
    s_entity_task_coordinate_match_user = user;
}

struct EntityTaskAtGridCoordinateRequest {
    uint32_t x;
    uint32_t y;
    EntitySlot** out_entity;
};

static int entity_task_at_grid_coordinate_candidate(
    EntitySlot* entity, void* payload)
{
    EntityTaskAtGridCoordinateRequest* request =
        (EntityTaskAtGridCoordinateRequest*)payload;
    if (request == NULL || entity == NULL ||
        !entity_shared_eligibility_check(entity, 1)) return 0;

    /* LAB_0043b3b0 has additional renderer/type-specific geometry tests after
     * this shared gate. The portable hook is the only current owner of that
     * scene query; don't substitute the unrelated +0x2f4/+0x2f8 route pair. */
    if (s_entity_task_coordinate_match_hook == NULL ||
        !s_entity_task_coordinate_match_hook(
            s_entity_task_coordinate_match_user, entity,
            request->x, request->y)) return 0;
    *request->out_entity = entity;
    return 1;
}

/* TaskCountAtGridCoordinateQuery (FUN_0043b360) actually returns whether the
 * callback found an entity and writes that EntitySlot pointer through its
 * output dword. LAB_0043b3b0's eligibility gate and synchronous pool walk are
 * live; its remaining renderer geometry predicate is supplied by the hook. */
// FUNCTION: LEGORR 0x0043B360
int entity_task_at_grid_coordinate_find(uint32_t x, uint32_t y,
                                        EntitySlot** out_entity)
{
    EntityTaskAtGridCoordinateRequest request;
    GameContext* context;
    if (out_entity == NULL) return 0;
    *out_entity = NULL;
    context = s_entity_task_count_context != NULL ?
        s_entity_task_count_context : entity_pool_callback_context_get();
    if (context == NULL) return 0;
    request.x = x;
    request.y = y;
    request.out_entity = out_entity;
    return entity_pool_find_by_callback_filtered(
        context, entity_task_at_grid_coordinate_candidate, &request);
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
    if ((capabilities & (4u | 2u)) != 0u) {
        /* FUN_0043bb20 calls EntityDepositActivityGate(entity, NULL)
         * directly once the carried-count and state gates pass. Preserve the
         * port's optional integration callback when one is explicitly set. */
        if (s_entity_building_deposit_hook != NULL)
            s_entity_building_deposit_hook(entity,
                                           s_entity_building_deposit_context);
        else
            (void)entity_deposit_activity_gate(entity, NULL);
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
        (void)activity_register_bookkeeping(entity);
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

static int entity_kind_four_candidate_search(GameContext* context,
                                            LevelState* level, int x, int y,
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
            found = entity_at_grid_position_find_in_context(
                context, level, cell_xy);
            if (found != NULL && entity_available_resource_spend(
                    level, amount, found)) return 1;
        }
        if (!level_cell_occupied_at(level, nx, ny) &&
            !level_cell_occupied_or_flag(level, nx, ny, 1)) continue;
        if (level_cell_visited_flag(level, nx, ny) != 0u) continue;
        terrain_cell_visited_flag_set_and_track(level, nx, ny);
        if (entity_kind_four_candidate_search(context, level, nx, ny,
                                              amount))
            return 1;
    }
    return 0;
}

static int entity_kind_four_candidate_find(GameContext* context,
                                            LevelState* level,
                                            const int start_xy[2],
                                            int amount)
{
    int found;
    if (level == NULL || start_xy == NULL) return 0;
    found = entity_kind_four_candidate_search(context, level, start_xy[0],
                                               start_xy[1], amount);
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
    GameContext* context;
    LevelState* level;
    uint32_t flags;
    int cell_xy[2];
    int have_cell;
    if (entity == NULL || entity->type != 4 ||
        (entity->flags_3e8 & 0x80000000u) != 0u) return;
    level = entity->owner_ctx != NULL ? entity->owner_ctx->level : NULL;
    if (level == NULL) {
        context = entity_pool_callback_context_get();
        level = context != NULL ? context->level : NULL;
    }
    else {
        context = entity->owner_ctx;
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
        !entity_kind_four_candidate_find(context, level, cell_xy,
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

namespace unported_ledger_stubs {
void ledger_stub_00439370_EntityBuildPlacementCommandSetup(
    const int cell_xy[2], int entity_id, const float world_xy[2],
    int update_state);
void ledger_stub_004394b0_EntityBuildPlacementAlternateCommandSetup(
    const int cell_xy[2], int entity_id, const float world_xy[2],
    int update_state);
}

// FUNCTION: LEGORR 0x00439340
void entity_build_placement_command_pair_create(const int cell_xy[2],
                                                int entity_id)
{
    unported_ledger_stubs::ledger_stub_00439370_EntityBuildPlacementCommandSetup(
        cell_xy, entity_id, NULL, 1);
    unported_ledger_stubs::ledger_stub_004394b0_EntityBuildPlacementAlternateCommandSetup(
        cell_xy, entity_id, NULL, 1);
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

/* EntityPlacementCommandsBuildOnce (FUN_00409f20): read the three indexed
 * stats words for this entity, enqueue the corresponding type-6/type-7
 * placement commands at its current terrain cell, and latch the per-entity
 * once bit. Empty/unloaded stats cells read as zero through the live table
 * seam, matching the other stats consumers in this reconstruction. */
// FUNCTION: LEGORR 0x00409f20
void entity_placement_commands_build_once(EntitySlot* entity)
{
    GameContext* context;
    LevelState* level;
    int cell_xy[2];
    int32_t type6_count;
    int32_t type7_count;
    int index;
    if (entity == NULL || (entity->flags_3ec & 0x800u) != 0u) return;
    context = entity_pool_callback_context_get();
    if (context == NULL) context = entity->owner_ctx;
    if (context == NULL || context->level == NULL) return;
    level = context->level;
    entity_position_to_terrain_cell(entity, level, &cell_xy[0], &cell_xy[1]);
    if (!level_coord_pair_within_bounds(level, cell_xy[0], cell_xy[1])) return;

    /* The reference getters index through entity +0 (stats type), +4
     * (stats row), and +0x330 (strided stats instance). */
    type6_count = (int32_t)stats_table_word_at_b4(
        4, entity->etype, entity->indexed_record_slot_330);
    type7_count = (int32_t)stats_table_word_at_b0(
        4, entity->etype, entity->indexed_record_slot_330);
    (void)stats_table_word_at_b8(
        4, entity->etype, entity->indexed_record_slot_330);

    for (index = type6_count; index != 0; --index)
        (void)entity_build_placement_command_setup(cell_xy, 0, NULL, 0, 0);
    for (index = type7_count; index != 0; --index)
        (void)entity_build_placement_command_setup(cell_xy, 0, NULL, 0, 1);
    entity->flags_3ec |= 0x800u;
}

/* EntityActionKindSupported (FUN_0043a7c0): the kind-6 call is a no-op after
 * the stats flags have been loaded, so its x86 return residue is that
 * nonzero flags word. The kind-7 call reads the selected live level balance. */
static int entity_action_kind_supported(const EntitySlot* source, int kind,
                                        int selector)
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
    if (source == NULL || hooks->create == NULL
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
    /* Kind 6 refreshes the activity scene and subtracts one from level +0x8c.
     * Kind 7 debits one unit from the selected resource. The adapter remains
     * an override for callers that own the corresponding presentation work. */
    if ((kind == 6 || kind == 7) && hooks->kind_event != NULL) {
        hooks->kind_event(hooks->context, kind, selector);
    }
    else if (kind == 6) {
        GameContext* context = source->owner_ctx;
        if (context == NULL) context = entity_pool_callback_context_get();
        if (context != NULL) {
            entity_activity_scene_state_update(context);
            global_object_field8c_subtract_for_level(context->level, 1);
        }
    }
    else if (kind == 7) {
        upgrade_resource_debit(selector, 1);
    }
    created = hooks->create(hooks->context, source, kind, selector, argument);
    if (created == NULL) return NULL;
    opcode = kind == 7 ? 0x1c : 0x16;
    if (hooks->queue != NULL)
        hooks->queue(hooks->context, opcode, created);
    else
        cmd_enqueue(opcode, created, 0, 0, 0);
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

/* EntityActionRecordConsumeAndCleanup (FUN_0043a850): the second argument is
 * a full entity record. Its +0x330 selector drives the type-6 owner counter;
 * +0x304 is cleared before the record is returned through EntityTeardown. */
// FUNCTION: LEGORR 0x0043A850
void entity_action_record_consume_and_cleanup(
    EntitySlot* entity, EntitySlot* record)
{
    EntityActionConsumeHooks* hooks = &s_entity_action_consume_hooks;
    GameContext* context;
    uint32_t* counters = NULL;
    if (entity == NULL || record == NULL) return;
    context = entity_pool_callback_context_get();
    if (context == NULL) context = entity->owner_ctx;
    if (entity->type == 3 && !entity->action_consume_counters_initialized) {
        memset(entity->action_consume_counters, 0,
               sizeof(entity->action_consume_counters));
        entity->action_consume_counters_initialized = 1;
    }
    if (entity->type == 3)
        counters = entity_action_consume_counter_sidecar_get(entity, 1);
    if (record->type == 6) {
        if (entity->type == 3) {
            uint32_t slot = record->indexed_record_slot_330 < 0 ? 6u :
                (uint32_t)record->indexed_record_slot_330;
            if (slot < 6u) {
                if (counters != NULL) {
                    ++counters[slot];
                    entity->action_consume_counters[slot] = counters[slot];
                } else {
                    ++entity->action_consume_counters[slot];
                }
            }
        }
        else if (hooks->event_dispatch != NULL) {
            hooks->event_dispatch(hooks->context, 6, 0);
        }
        else if (context != NULL) {
            global_object_field8c_increment_for_level(context->level);
        }
    }
    else if (record->type == 7 && entity->type != 3
             && hooks->event_dispatch != NULL) {
        hooks->event_dispatch(hooks->context, 7, record->etype);
    }
    else if (record->type == 7 && entity->type != 3) {
        level_resource_balance_add_and_sound(record->etype);
    }
    record->attach_304 = NULL;
    if (hooks->record_release != NULL) {
        hooks->record_release(hooks->context, record);
    }
    else {
        (void)entity_teardown(record);
    }
    if (entity->type != 3 && hooks->entity_records_cleanup != NULL) {
        hooks->entity_records_cleanup(hooks->context, entity);
    }
    else if (entity->type != 3) {
        /* FUN_00403910 is the concrete live implementation of the retail
         * non-creature record cleanup sweep used by this target. */
        terrain_cell_record_counts_prune(entity);
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
    else {
        /* FUN_00452060 is the existing portable command producer for the
         * fixed opcode emitted by the retail tail. */
        cmd_produce(0x2d, NULL, 0, NULL);
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
 * only once. Default pose and child creation use the live entity/context;
 * the host can still override those operations and supply the named scene
 * action that the portable view API does not expose. */
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
    /* The retail record stores the scaled duration through an integer
     * conversion before it reaches the +0x368 word. */
    entity->wake_timer = (float)(int)(duration * 25.0f);
    if (hooks->scene_pose_get != NULL) {
        if (!hooks->scene_pose_get(hooks->context, entity, position, &angle))
            return 0;
    }
    else {
        /* FUN_00442260 / FUN_00442480 read the owner's scene position and
         * heading. The entity accessors use the attached scene when present
         * and retain the slot's position/heading as the portable fallback. */
        entity_transform_position_get(entity, position);
        angle = entity_vector_direction_angle_radians_get(entity);
    }
    if (hooks->child_create != NULL) {
        child = hooks->child_create(hooks->context, entity, 0x12,
                                    position, angle);
    }
    else if (entity->owner_ctx != NULL) {
        /* The source creates an entity of kind 0x12 using its current level
         * context. This direct path keeps the owner/child linkage live even
         * when the host has not installed a renderer-specific creator. */
        child = entity_create_with_position(entity->owner_ctx, 0x12, NULL, 0,
                                            position[0], position[1], angle);
    }
    else {
        child = NULL;
    }
    if (child == NULL) return 0;
    if (hooks->child_action_start != NULL) {
        hooks->child_action_start(hooks->context, child, "Start");
    }
    /* The retail path immediately advances the new child's scene action at
     * time zero after selecting "Start". The portable scene updater exposes
     * that same operation when the created slot has a bound scene handle. */
    if (child->type_component_handle != NULL) {
        (void)ddiscene_frame_update_and_callbacks(
            child->type_component_handle, 0.0f);
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
 * by a clear ray check completes the gate. */
int entity_type26_command_completion_gate(const EntitySlot* entity)
{
    const void* command;
    const EntitySlot* target;
    if (entity == NULL || entity->type != 2) return 0;
    command = entity->cmd_head;
    if (command == NULL || cmd_rec_type(command) != 0x1a) return 0;
    target = (const EntitySlot*)cmd_rec_payload(command);
    if (target == NULL || !entity_centers_within_configured_radius(entity,
                                                                  target)) {
        return 0;
    }
    return entity_pair_ray_hit_check(entity, target) == 0;
}

static int entity_unload_adjacent_cell_find_on_level(
    const EntitySlot* entity, const LevelState* level, int out_xy[2])
{
    static const int offsets[4][2] = {
        { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }
    };
    int x = 0, y = 0, i;
    float position[2];
    if (entity == NULL || level == NULL) return 0;
    /* FUN_00439d50 starts from FUN_004424f0's scene-derived position. Use
     * the same live transform when a scene object exists, then apply the
     * terrain's own world-to-cell conversion. The logical EntitySlot x/y
     * values remain the transform fallback inside entity_transform_position_get. */
    entity_transform_position_get(entity, position);
    if (!terrain_world_xy_to_cell(level->ptr_terrain, position[0],
                                  position[1], &x, &y)) return 0;
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
    context = entity->owner_ctx != NULL ? entity->owner_ctx
                                        : entity_pool_callback_context_get();
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

static uint32_t entity_cell_speed_rock_fall_name_find(const char* name)
{
    return lego::object_type_name_index_find(name);
}

static float entity_cell_speed_indexed_scale_get(int type,
                                                  const void* entity_record)
{
    return lego::entity_indexed_float_lookup_with_minus_one_fallback(
        type, entity_record);
}

// FUNCTION: LEGORR 0x00433180
#if defined(_MSC_VER) && defined(_M_IX86)
static const char s_entity_rock_fall_in_name[] = "RockFallIn";
__declspec(naked) int entity_cell_type_speed_multiplier_resolve_current(
    const EntitySlot* entity, int x, int y, float scale, float* out_scale)
{
    __asm {
        push ebx
        push ebp
        mov ebp, dword ptr [esp + 0ch]
        push esi
        push edi
        mov edi, dword ptr [g_active_level_retail]
        mov eax, dword ptr [ebp]
        cmp eax, 6
        je entity_cell_type_speed_multiplier_failed
        cmp eax, 7
        je entity_cell_type_speed_multiplier_failed
        cmp eax, 5
        je entity_cell_type_speed_multiplier_failed
        cmp eax, 8
        je entity_cell_type_speed_multiplier_failed
        cmp eax, 0bh
        je entity_cell_type_speed_multiplier_failed
        cmp eax, 0dh
        je entity_cell_type_speed_multiplier_failed
        cmp eax, 0eh
        je entity_cell_type_speed_multiplier_failed
        cmp eax, 0fh
        je entity_cell_type_speed_multiplier_failed
        cmp eax, 10h
        je entity_cell_type_speed_multiplier_failed
        cmp eax, 11h
        je entity_cell_type_speed_multiplier_failed
        cmp eax, 12h
        je entity_cell_type_speed_multiplier_failed
        cmp eax, 13h
        je entity_cell_type_speed_multiplier_failed
        mov eax, dword ptr [ebp + 36ch]
        test eax, eax
        je entity_cell_type_speed_multiplier_cell_test
        push ebp
        call entity_stats_word_get
        add esp, 4
        test eax, 20000h
        je entity_cell_type_speed_multiplier_failed
    entity_cell_type_speed_multiplier_cell_test:
        mov esi, dword ptr [esp + 1ch]
        mov ebx, dword ptr [esp + 18h]
        mov eax, esi
        mov ecx, dword ptr [edi + 0b8h]
        imul eax, dword ptr [edi + 14h]
        add eax, ebx
        lea eax, dword ptr [eax + eax*8]
        lea eax, dword ptr [ecx + eax*8]
        cmp byte ptr [eax + 2], 6
        jne entity_cell_type_speed_multiplier_fallback
        mov ecx, dword ptr [eax + 8]
        test ch, 2
        jne entity_cell_type_speed_multiplier_fallback
        push ebp
        call entity_state_flags_get
        add esp, 4
        test eax, 200000h
        jne entity_cell_type_speed_multiplier_fallback
        mov eax, dword ptr [esp + 24h]
        test eax, eax
        je entity_cell_type_speed_multiplier_success
        mov edx, dword ptr [esp + 20h]
        mov dword ptr [eax], edx
        mov eax, 1
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    entity_cell_type_speed_multiplier_fallback:
        imul esi, dword ptr [edi + 14h]
        mov ecx, dword ptr [edi + 0b8h]
        add esi, ebx
        lea eax, dword ptr [esi + esi*8]
        mov edx, dword ptr [ecx + eax*8 + 8]
        test dh, 10h
        je entity_cell_type_speed_multiplier_failed
        mov esi, dword ptr [esp + 24h]
        test esi, esi
        je entity_cell_type_speed_multiplier_success
        push ebp
        push offset s_entity_rock_fall_in_name
        call entity_cell_speed_rock_fall_name_find
        add esp, 4
        push eax
        call entity_cell_speed_indexed_scale_get
        fmul dword ptr [esp + 28h]
        add esp, 8
        fstp dword ptr [esi]
    entity_cell_type_speed_multiplier_success:
        mov eax, 1
        pop edi
        pop esi
        pop ebp
        pop ebx
        ret
    entity_cell_type_speed_multiplier_failed:
        pop edi
        pop esi
        pop ebp
        xor eax, eax
        pop ebx
        ret
    }
}
#else
int entity_cell_type_speed_multiplier_resolve_current(
    const EntitySlot* entity, int x, int y, float scale, float* out_scale)
{
    LevelState* level = g_active_level_retail;
    if (entity->type == 6 || entity->type == 7 || entity->type == 5 ||
        entity->type == 8 || entity->type == 11 || entity->type == 13 ||
        entity->type == 14 || entity->type == 15 || entity->type == 16 ||
        entity->type == 17 || entity->type == 18 || entity->type == 19) {
        return 0;
    }
    if (entity->owner_36c != NULL &&
        (entity_stats_word_get(entity) & 0x20000u) == 0u) return 0;

    {
        int width = (int)level->word[LEVEL_OFF_WIDTH / 4];
#if defined(_WIN32) && defined(_M_IX86)
        uint8_t* voxels = (uint8_t*)(uintptr_t)
            level->word[LEVEL_OFF_VOXELS / 4];
#else
        uint8_t* voxels = (uint8_t*)level->ptr_voxels;
#endif
        int index = width * y + x;
        uint8_t* cell = voxels + index * 0x48;
        if (cell[2] == 6) {
            uint32_t cell_flags = *(uint32_t*)(cell + 8);
            if ((cell_flags & 0x200u) == 0u &&
                (entity_state_flags_get(entity) & 0x200000u) == 0u) {
                if (out_scale == NULL) return 1;
                *out_scale = scale;
                return 1;
            }
        }
        if ((*(uint32_t*)(cell + 8) & 0x1000u) != 0u) {
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
    }
    return 0;
}
#endif

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
void entity_transform_position_get_outputs(const EntitySlot* entity,
                                           float* out_x, float* out_y)
{
    float position[3];
    if (entity == NULL || out_x == NULL || out_y == NULL) return;
    /* The reference resolves the kind-specific transform, then reads its
     * slot-zero position.  The attached scene object is the port's live
     * transform for objects whose type component has reached the renderer;
     * the slot coordinates remain the simulation fallback. */
    if (entity->attached_scene_object != NULL &&
        ddiscene_object_position_get(entity->attached_scene_object, NULL,
                                     position)) {
        *out_x = position[0];
        *out_y = position[1];
        return;
    }
    *out_x = entity->x;
    *out_y = entity->y;
}

void entity_transform_position_get(const EntitySlot* entity,
                                   float out_xy[2])
{
    if (out_xy == NULL) return;
    entity_transform_position_get_outputs(entity, &out_xy[0], &out_xy[1]);
}

// FUNCTION: LEGORR 0x00471CA0
int entity_world_center_position_read(const EntitySlot* entity,
                                      float out_xyz[3])
{
    float position[3];
    if (entity == NULL || out_xyz == NULL) return 0;
    if (entity->attached_scene_object != NULL &&
        ddiscene_object_position_get(entity->attached_scene_object, NULL,
                                     position)) {
        out_xyz[0] = position[0];
        out_xyz[1] = position[1];
        out_xyz[2] = position[2];
    }
    else {
        out_xyz[0] = entity->x;
        out_xyz[1] = entity->y;
        out_xyz[2] = entity->z;
    }
    out_xyz[2] -= entity_stats_float_at74(entity) * 0.5f;
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
    /* The retail API passes x/y to the kind-specific pose setter. Those
     * setters obtain vertical height through FUN_00442540 -> 0x44f940; this
     * two-coordinate setter does not convert to a cell or clip to map bounds.
     * The host EntitySlot keeps the same horizontal pair directly. */
    entity->x = x;
    entity->y = y;
    if (update_heading) {
        float direction[3] = { 0.0f, 1.0f, 0.0f };
        float axis[3] = { 0.0f, 0.0f, -1.0f };
        /* FUN_00479510 rotates [0,1,0] around [0,0,-1] by the supplied
         * heading, then the retail body stores the result at +0x2ac..+0x2b4. */
        d3drm_vector_rotate(direction, direction, axis, heading);
        entity->attached_direction[0] = direction[0];
        entity->attached_direction[1] = direction[1];
        entity->attached_direction[2] = direction[2];
        entity->rot = heading;
    }
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
    if (s_entity_type5_timeout_hook != NULL)
        s_entity_type5_timeout_hook(entity);
    else
        scene_marker_create_at_object_position(
            entity->attached_scene_object);
    if (s_entity_type5_finalize_hook != NULL)
        s_entity_type5_finalize_hook(entity);
    else
        (void)entity_teardown(entity);
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
    (void)activity_register_bookkeeping(entity);
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

/* EntityClearNestedState8Fields (FUN_004487c0): clear the linked type-8
 * object's +0x2f4 position and run the entity's attached-object detach
 * pass. The separate nested +0xbe field has no EntitySlot mirror yet. */
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
    nested->pos_pair_2f8 = 0.0f;
    nested->pos_pair_2f4 = 0.0f;
    unported_ledger_stubs::entity_attached_object_list_detach(e, 1);
}

static int entity_type_four_linked_type_five_reset(EntitySlot* candidate,
                                                   void* payload)
{
    EntitySlot* source = (EntitySlot*)payload;
    if (candidate != NULL && candidate->type == 5 &&
        candidate->link_2fc == (uintptr_t)source &&
        candidate->attach_304 != NULL) {
        entity_action_sequence_reset(candidate, 0);
    }
    /* LAB_00447920 always continues the pool scan. */
    return 0;
}

/* TypeFourEntityCallbackDispatch (FUN_00447900): the source type-4 gate
 * selects a filtered pool walk; LAB_00447920 resets linked type-5 action
 * sequences when their attachment record is present. */
// FUNCTION: LEGORR 0x00447900
void entity_type_four_callback_dispatch(EntitySlot* source)
{
    if (source == NULL || source->type != 4) return;
    (void)entity_pool_find_by_callback_filtered_current(
        entity_type_four_linked_type_five_reset, source);
}

/* EntityEffectCountersApplyFromRecord (FUN_0042b2f0): when state bit 8 is
 * active, route the record words at +4/+8 to the global update and resource
 * counter helpers, in decompiled order. */
// FUNCTION: LEGORR 0x0042b2f0
void entity_effect_counters_apply_from_record(EntitySlot* e,
                                              const void* record)
{
    if (e == NULL || record == NULL
        || (entity_state_flags_get(e) & 8u) == 0u) {
        return;
    }
    uint32_t count_a_bits = 0u;
    uint32_t count_b_bits = 0u;
    int count_b;
    GameContext* context = e->owner_ctx;
    __builtin_memcpy(&count_a_bits, (const char*)record + 4,
                     sizeof(count_a_bits));
    __builtin_memcpy(&count_b_bits, (const char*)record + 8,
                     sizeof(count_b_bits));
    if (context == NULL) context = entity_pool_callback_context_get();

    /* FUN_0042b2f0 forwards +4 to SimulationUpdateCounterAdd, whose tail
     * also performs EntityActivitySceneStateUpdate. */
    simulation_update_counter_add(context, (int)count_a_bits);

    /* FUN_0043a910 calls LevelResourceBalanceAddAndSound(0) once per +8
     * count. Keep that operation on the command module's active context so
     * its global resource counter and overlay notification stay paired. */
    count_b = (int)count_b_bits;
    while (count_b != 0) {
        level_resource_balance_add_and_sound(0);
        --count_b;
    }
}

/* EntityDamageApply (FUN_0043f6a0): the corpus subtracts from the float at
 * +0x340, not the separate `.ol` integer health field. Preserve the visible
 * kind-4 progress event, kind-2 accumulator, and positive damage flags. */
// FUNCTION: LEGORR 0x0043f6a0
void entity_damage_apply(EntitySlot* e, float damage, int emit_effects)
{
    if (e == NULL || entity_shared_eligibility_check(e, 1) == 0
        || e->attach_304 != NULL) {
        return;
    }
    if (e->type == ENTITY_TYPE_BUILDING && damage != 0.0f &&
        e->field_340 == 100.0f) {
        entity_threshold_progress_event_queue(e, 0);
    }
    /* EntityDamageApply tests the kind-3 +0x3e8 bit before comparing the
     * integer conversions of HealthDecayRate (+0x80) and incoming damage.
     * The retail path adds the damage to DAT_00555330 when the latter is
     * larger; the typed stats accessor and CRT conversion preserve that
     * behavior without assuming retail structure offsets in EntitySlot. */
    if (entity_type3_flag04000000_test(e) != 0 &&
        crt_float_to_long(entity_stats_float_at80(e)) <
            crt_float_to_long(damage)) {
        global_float_accumulator_00555330_add(damage);
    }
    if (e->type == ENTITY_TYPE_MINIFIG) {
        global_float_accumulate_00555120(damage);
    }
    e->field_340 = e->field_340 - damage;
    if (emit_effects != 0 && damage > 0.0f) {
        e->flags_3e4 |= 0x3000u;
        flagged_object_timer_registry_insert(e);
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
    if (candidate->spawn_child_3cc == target) {
        candidate->spawn_child_3cc = NULL;
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

/* PendingTypeValueSlotSelect (FUN_00407a10): the config-fed DAT words are
 * zero-init model slots; this selection retains the exact flag precedence
 * and the command/activity families that choose among them. */
// FUNCTION: LEGORR 0x00407a10
void pending_type_value_slot_select(const EntitySlot* e, uint32_t* out)
{
#if defined(_WIN32) && defined(_M_IX86)
    int* record = *(int**)((uint8_t*)e + 0x2f0);
    int type;
    if ((*(uint8_t*)((uint8_t*)e + 0x3ec) & 0x20u) != 0u) {
        *out = s_bubble_texture_handles[30];
        return;
    }
    if ((*(uint32_t*)((uint8_t*)e + 0x3e0) & 0x800000u) != 0u) {
        *out = s_bubble_texture_handles[28];
        return;
    }
    if ((*(uint32_t*)((uint8_t*)e + 0x3e0) & 0x80000000u) != 0u) {
        *out = s_bubble_texture_handles[0];
        return;
    }
    if (record == NULL) return;
    type = record[0];
    while (type == 0xe &&
           (record = *(int**)((uint8_t*)record + 0x60)) != NULL) {
        type = record[0];
    }
    switch (type) {
    case 0:
        *out = s_bubble_texture_handles[22];
        return;
    case 3: {
        int* payload = *(int**)((uint8_t*)record + 0x10);
        if (payload[0] == 6) *out = s_bubble_texture_handles[2];
        else if (payload[0] == 7) {
            if (payload[1] != 0) *out = s_bubble_texture_handles[4];
            else *out = s_bubble_texture_handles[3];
            return;
        }
        else if (payload[0] == 8) *out = s_bubble_texture_handles[5];
        else if (payload[0] == 9) *out = s_bubble_texture_handles[6];
        else if (payload[0] == 0xb) *out = s_bubble_texture_handles[7];
        else break;
        return;
    }
    case 5: {
        int* linked = *(int**)((uint8_t*)e + 0x308);
        if (linked == NULL) break;
        type = linked[0];
        if (type == 6) *out = s_bubble_texture_handles[16];
        else if (type == 7) {
            if (linked[1] != 0) *out = s_bubble_texture_handles[18];
            else *out = s_bubble_texture_handles[17];
            return;
        }
        else if (type == 8) *out = s_bubble_texture_handles[19];
        else if (type == 9) *out = s_bubble_texture_handles[20];
        else if (type == 0xb) *out = s_bubble_texture_handles[21];
        else break;
        return;
    }
    case 7: *out = s_bubble_texture_handles[38]; return;
    case 8: *out = s_bubble_texture_handles[25]; return;
    case 9: *out = s_bubble_texture_handles[23]; return;
    case 10: *out = s_bubble_texture_handles[26]; return;
    case 0xb: *out = s_bubble_texture_handles[24]; return;
    case 0xc: *out = s_bubble_texture_handles[27]; return;
    case 0xf: *out = s_bubble_texture_handles[31]; return;
    case 0x10:
    case 0x11: *out = s_bubble_texture_handles[32]; return;
    case 0x12: *out = s_bubble_texture_handles[33]; return;
    case 0x13:
        type = record[0xe];
        if (type == 0) *out = s_bubble_texture_handles[8];
        else if (type == 1) *out = s_bubble_texture_handles[9];
        else if (type == 2) *out = s_bubble_texture_handles[10];
        else if (type == 3) *out = s_bubble_texture_handles[11];
        else if (type == 4) *out = s_bubble_texture_handles[12];
        else if (type == 5) *out = s_bubble_texture_handles[13];
        else if (type == 7) *out = s_bubble_texture_handles[14];
        else if (type == 6) *out = s_bubble_texture_handles[15];
        else break;
        return;
    case 0x15: *out = s_bubble_texture_handles[34]; return;
    case 0x16: *out = s_bubble_texture_handles[35]; return;
    case 0x17: *out = s_bubble_texture_handles[36]; return;
    case 0x1b: *out = s_bubble_texture_handles[37]; return;
    }
#else
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
            const int activity = (int)cmd_rec_value_38(record);
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
#endif
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

/* SceneEntitySlotTableRebuild (FUN_004083a0): clear the owner windows, walk
 * its +0x9c scene-node chain, prepare each node's renderer, select its
 * animation, then register up to two +0x1c handles followed by +0x24 and
 * +0x20 handles for payload nodes. Host pointers live in native-width
 * sidecars; renderer preparation, state, animation and resource lookup cross
 * the DDiScene seam. */
// FUNCTION: LEGORR 0x004083a0
void entity_scene_slot_table_rebuild(EntitySlot* e, int mode)
{
    EntitySceneSlotNode* node;
    uint32_t visited = 0u;
    uint32_t cached = 0u;
    if (e == NULL) return;

    memset(e->scene_slot_cache, 0, sizeof(e->scene_slot_cache));
    memset(e->scene_slot_owner_1c_handles, 0,
           sizeof(e->scene_slot_owner_1c_handles));
    memset(e->scene_slot_owner_24_handles, 0,
           sizeof(e->scene_slot_owner_24_handles));
    memset(e->scene_slot_owner_20_handles, 0,
           sizeof(e->scene_slot_owner_20_handles));
    memset(e->scene_slot_owner_1c_counts, 0,
           sizeof(e->scene_slot_owner_1c_counts));
    memset(e->scene_slot_payloads, 0, sizeof(e->scene_slot_payloads));
    e->scene_slot_registration_count = 0u;
    e->scene_slot_cache_count = 0u;
    ++e->scene_slot_generation;

    node = e->scene_slot_nodes;
    while (node != NULL && visited < e->scene_slot_node_count) {
        void* registered[4] = { NULL, NULL, NULL, NULL };
        uint32_t registered_count = 0u;
        uint32_t slot;
        ++visited;
        memset(node->slot_handles, 0, sizeof(node->slot_handles));
        node->slot_handle_count = 0u;
        node->animation_mode = mode;
        if (s_entity_scene_slot_runtime_ops.renderer_position_prepare != NULL)
            s_entity_scene_slot_runtime_ops.renderer_position_prepare(
                s_entity_scene_slot_runtime_ops.context, e, node);
        if (s_entity_scene_slot_runtime_ops.renderer_state_apply != NULL)
            s_entity_scene_slot_runtime_ops.renderer_state_apply(
                s_entity_scene_slot_runtime_ops.context, node);
        if (s_entity_scene_slot_runtime_ops.node_animation_select != NULL)
            s_entity_scene_slot_runtime_ops.node_animation_select(
                s_entity_scene_slot_runtime_ops.context, node, mode);

        if (node->payload != NULL) {
            uint32_t payload_index = e->scene_slot_registration_count;
            void* owner_1c[2] = { NULL, NULL };
            void* owner_24 = NULL;
            void* owner_20 = NULL;
            uint32_t owner_1c_count = 0u;
            if (s_entity_scene_slot_runtime_ops.resource_slot_register != NULL) {
                /* FUN_004083a0 stops the repeated +0x1c lookup at its first
                 * null handle, then always attempts +0x24 and +0x20 once.
                 * The native +0xd8 count includes that terminating lookup. */
                for (slot = 0u; slot < 2u; ++slot) {
                    void* handle =
                        s_entity_scene_slot_runtime_ops.resource_slot_register(
                            s_entity_scene_slot_runtime_ops.context, node,
                            e->scene_slot_owner_keys[0], slot);
                    ++owner_1c_count;
                    if (handle == NULL) break;
                    owner_1c[slot] = handle;
                    registered[registered_count++] = handle;
                }
                owner_24 =
                    s_entity_scene_slot_runtime_ops.resource_slot_register(
                        s_entity_scene_slot_runtime_ops.context, node,
                        e->scene_slot_owner_keys[2], 0u);
                registered[registered_count++] = owner_24;
                owner_20 =
                    s_entity_scene_slot_runtime_ops.resource_slot_register(
                        s_entity_scene_slot_runtime_ops.context, node,
                        e->scene_slot_owner_keys[1], 0u);
                registered[registered_count++] = owner_20;
            }
            if (payload_index < 12u)
                e->scene_slot_payloads[payload_index] = node->payload;
            if (payload_index < 3u) {
                e->scene_slot_owner_1c_counts[payload_index] =
                    owner_1c_count;
                e->scene_slot_owner_1c_handles[payload_index][0] =
                    owner_1c[0];
                e->scene_slot_owner_1c_handles[payload_index][1] =
                    owner_1c[1];
                e->scene_slot_owner_24_handles[payload_index] = owner_24;
                e->scene_slot_owner_20_handles[payload_index] = owner_20;
            }
            node->slot_handle_count = registered_count;
            memcpy(node->slot_handles, registered, sizeof(registered));
            for (slot = 0u; slot < registered_count && cached < 12u; ++slot)
                e->scene_slot_cache[cached++] = registered[slot];
            /* Retail publishes +0x144 only after all slot lookups finish. */
            e->scene_slot_registration_count = payload_index + 1u;
        }
        node = node->next;
    }
    e->scene_slot_cache_count = cached;
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

/* Retail-shaped accessors share the OList-fed registry and its host-pointer
 * sidecar above; do not keep a second table at DAT_005563b8. */
// FUNCTION: LEGORR 0x0042dfc0
void object_pointer_registry_clear_match(int32_t pointer)
{
    for (uint32_t i = 0; i < s_object_pointer_registry_count &&
         i < OBJECT_POINTER_REGISTRY_CAPACITY; ++i) {
        if ((int32_t)s_object_pointer_registry_words[i] == pointer) {
            s_object_pointer_registry_words[i] = 0u;
            s_object_pointer_registry_entities[i] = NULL;
        }
    }
}

// FUNCTION: LEGORR 0x0042dff0
int object_pointer_registry_get_slot(uint32_t index, int32_t* out)
{
    if (index >= s_object_pointer_registry_count) {
        return 0; /* deviation: the corpus reads blindly past the count */
    }
    const int32_t v = (int32_t)s_object_pointer_registry_words[index];
    if (out != NULL) {
        *out = v;
    }
    return v != 0 ? 1 : 0;
}

uint32_t object_pointer_registry_count_peek(void)
{
    return s_object_pointer_registry_count;
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
#if !defined(_MSC_VER) || !defined(_M_IX86)
    if (e == NULL) {
        return 0; /* deviation: the corpus dereferences blindly */
    }
#endif
    if ((e->flags_3e8 & 0x100u) == 0u) {
        return 0;
    }
    const int shared_eligible = entity_shared_eligibility_check(e, 1);
    if ((shared_eligible != 0)
        && ((uint8_t)e->flags_3e4 & 0x0Au) == 0u /* corpus: low byte & 10 */
        && (e->flags_3e0 & 0x24000000u) == 0u
        && ((e->type != 1)
            || (e->flags_3e0 & 0x00040000u) == 0u)) {
        return 1; /* type 1 has the additional 0x40000 rejection (finding 468). */
    }
    return 0;
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
        level_state_field9c_add_for_level(lvl, *(const int*)e->carry_ref_348);
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
    entity_command_unlink_by_kind(e, 4, carried, 1);
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

/* EntityPoolResourcesRelease (FUN_00437050) runs the reference's filtered
 * entity teardown walk before releasing tracked record chunks and clearing
 * the pool/lifecycle state. DAT_004dee00 is the terrain-grid cost map; retail
 * frees it without nulling the global pointer, while the portable owner
 * clears it to make repeated teardown safe. */
// FUNCTION: LEGORR 0x00437050
void pooled_record_pool_resources_release(void)
{
    uint32_t i;
    entity_iteration_callback_guard();
    for (i = 0; i < POOL_CHUNK_COUNT_SLOTS; ++i) {
        if (s_pool_chunks[i] != NULL) {
            engine_heap_free(s_pool_chunks[i]);
            s_pool_chunks[i] = NULL;
        }
    }
    s_pool_record_head = NULL;
    s_pool_chunk_count = 0;
    /* The retail body clears DAT_004dee14 before freeing the cost map, then
     * repeats its bit-0 clear afterward. */
    s_entity_iteration_guard_flags = 0u;
    terrain_neighbor_path_cost_release();
    s_entity_iteration_guard_flags &= ~1u;
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
    s_stats_type_table[type][row] = stride_count > 0 ? record : NULL;
    s_stats_cell_span[type][row] = stride_count;
    s_stats_cell_level_limit[type][row] = (float)stride_count;
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
    return s_stats_type_table[type][row];
}

/* BuildingUpgradeAdmissionCheck (FUN_00469f40) reads the raw word stored
 * beside the stats table. The loader writes the parsed integer as a float;
 * callers then consume that float's bits through an integer table read. */
// FUNCTION: LEGORR 0x00469f40
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
#endif
uint32_t building_upgrade_admission_check(int type, int row)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 8]
        lea eax, [eax + eax*2]
        lea edx, [ecx*4]
        lea eax, [eax + eax*4]
        mov eax, dword ptr [edx + eax*4 + s_stats_cell_level_limit]
        ret
    }
#else
    if (type < 0 || type >= STATS_TYPE_TABLE_TYPES
        || row < 0 || row >= STATS_TYPE_TABLE_ROWS) {
        return 0u; /* deviation: the corpus indexes the table blindly */
    }
    uint32_t raw_word = 0u;
    __builtin_memcpy(&raw_word, &s_stats_cell_level_limit[type][row],
                     sizeof(raw_word));
    return raw_word;
#endif
}

static uint32_t stats_table_read(int type, int row, int off, int stride_index)
{
    if (type < 0 || type >= STATS_TYPE_TABLE_TYPES
        || row < 0 || row >= STATS_TYPE_TABLE_ROWS || stride_index < 0) {
        return 0u; /* deviation: the corpus strides the index blindly */
    }
    const void* base = s_stats_type_table[type][row];
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

// FUNCTION: LEGORR 0x00469b20
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
#endif
uint32_t stats_table_word_at_b8(int type, int row, int stride_index)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 0ch]
        mov edx, dword ptr [eax*4 + s_stats_type_table]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esp + 8]
        mov edx, dword ptr [edx + ecx*4]
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [edx + eax + 0b8h]
        ret
    }
#else
    return stats_table_read(type, row, 0xb8, stride_index);
#endif
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
// FUNCTION: LEGORR 0x00469f60
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
#endif
uint32_t stats_table_word_at_d0(int type, int row, int stride_index)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 0ch]
        mov edx, dword ptr [eax*4 + s_stats_type_table]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esp + 8]
        mov edx, dword ptr [edx + ecx*4]
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [edx + eax + 0d0h]
        ret
    }
#else
    return stats_table_read(type, row, 0xd0, stride_index); /* FUN_00469f60 */
#endif
}
// FUNCTION: LEGORR 0x0046a0f0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
#endif
uint32_t stats_table_word_at_100(int type, int row, int stride_index)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        mov eax, dword ptr [esp + 4]
        mov ecx, dword ptr [esp + 0ch]
        mov edx, dword ptr [eax*4 + s_stats_type_table]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esp + 8]
        mov edx, dword ptr [edx + ecx*4]
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [edx + eax + 100h]
        ret
    }
#else
    return stats_table_read(type, row, 0x100, stride_index); /* FUN_0046a0f0 */
#endif
}

// FUNCTION: LEGORR 0x00469e00
float stats_table_float_at_dc(int type, int row)
{
    /* FUN_00469e00 */
#if defined(_MSC_VER) && defined(_M_IX86)
    return *(const float*)((const char*)s_stats_type_table[type][row] + 0xdc);
#else
    const uint32_t bits = stats_table_read(type, row, 0xdc, 0);
    float f = 0.0f;
    __builtin_memcpy(&f, &bits, sizeof(f));
    return f;
#endif
}

// FUNCTION: LEGORR 0x00469ec0
uint32_t stats_table_word_at_144(int type, int row)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    return *(const uint32_t*)((const char*)s_stats_type_table[type][row] +
                              0x144);
#else
    return stats_table_read(type, row, 0x144, 0);
#endif
}
/* FUN_00469ec0 */
// FUNCTION: LEGORR 0x00469ee0
uint32_t stats_table_word_at_148(int type, int row)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    return *(const uint32_t*)((const char*)s_stats_type_table[type][row] +
                              0x148);
#else
    return stats_table_read(type, row, 0x148, 0);
#endif
}
/* FUN_00469ee0 */
// FUNCTION: LEGORR 0x00469f00
uint32_t stats_table_word_at_14c(int type, int row)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    return *(const uint32_t*)((const char*)s_stats_type_table[type][row] +
                              0x14c);
#else
    return stats_table_read(type, row, 0x14c, 0);
#endif
}
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
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
#endif
uint32_t stats_instance_word_at_bc(const void* config_block)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        mov eax, dword ptr [esp + 4]
        push esi
        mov ecx, dword ptr [eax]
        mov edx, dword ptr [eax + 4]
        mov esi, dword ptr [ecx*4 + s_stats_type_table]
        mov ecx, dword ptr [eax + 330h]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esi + edx*4]
        pop esi
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [ecx + eax + 0bch]
        ret
    }
#else
    return stats_instance_read(config_block, 0xbc);
#endif
}
/* FUN_00469b50 */
// FUNCTION: LEGORR 0x00469b80
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
#endif
uint32_t stats_instance_word_at_e0(const void* config_block)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        mov eax, dword ptr [esp + 4]
        push esi
        mov ecx, dword ptr [eax]
        mov edx, dword ptr [eax + 4]
        mov esi, dword ptr [ecx*4 + s_stats_type_table]
        mov ecx, dword ptr [eax + 330h]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esi + edx*4]
        pop esi
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [ecx + eax + 0e0h]
        ret
    }
#else
    return stats_instance_read(config_block, 0xe0);
#endif
}
/* FUN_00469b80 */
// FUNCTION: LEGORR 0x00469bb0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
#endif
uint32_t stats_instance_word_at_ec(const void* config_block)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        mov eax, dword ptr [esp + 4]
        push esi
        mov ecx, dword ptr [eax]
        mov edx, dword ptr [eax + 4]
        mov esi, dword ptr [ecx*4 + s_stats_type_table]
        mov ecx, dword ptr [eax + 330h]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esi + edx*4]
        pop esi
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [ecx + eax + 0ech]
        ret
    }
#else
    return stats_instance_read(config_block, 0xec);
#endif
}
/* FUN_00469bb0 */
// FUNCTION: LEGORR 0x00469be0
 #if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked)
#endif
uint32_t stats_instance_word_at_f0(const void* config_block)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        mov eax, dword ptr [esp + 4]
        push esi
        mov ecx, dword ptr [eax]
        mov edx, dword ptr [eax + 4]
        mov esi, dword ptr [ecx*4 + s_stats_type_table]
        mov ecx, dword ptr [eax + 330h]
        mov eax, ecx
        shl eax, 3
        sub eax, ecx
        mov ecx, dword ptr [esi + edx*4]
        pop esi
        lea eax, [eax + eax*2]
        shl eax, 4
        mov eax, dword ptr [ecx + eax + 0f0h]
        ret
    }
#else
    return stats_instance_read(config_block, 0xf0);
#endif
}
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

/* EntityTableMapsAllocate (FUN_0041aa00): the retail entry has no arguments
 * and sizes its six zeroed maps from the loaded vehicle/building counts.
 * Native pointers remain in this owner; the table payloads are retail-width
 * dword/byte arrays and are never used to carry host pointers. */
// FUNCTION: LEGORR 0x0041AA00
void entity_table_maps_allocate(void)
{
    entity_table_maps_current_release();
    entity_table_maps_allocate_for_counts(&s_entity_table_maps,
                                          s_catalog_counts[0],
                                          s_catalog_counts[3]);
    resource_handle_set_slot_install(30, s_entity_table_maps.flags_a);
    resource_handle_set_slot_install(31, s_entity_table_maps.flags_b);
    resource_handle_set_slot_install(32, s_entity_table_maps.secondary_a);
    resource_handle_set_slot_install(33, s_entity_table_maps.secondary_b);
    resource_handle_set_slot_install(34, s_entity_table_maps.primary_a);
    resource_handle_set_slot_install(35, s_entity_table_maps.primary_b);
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

namespace unported_ledger_stubs {

static CellStepPathResolveHook s_cell_step_path_hook;
static CellStepRandomRangeHook s_cell_step_random_hook;
static void* s_cell_step_hook_user;
void ledger_stub_00446610_EntityMovementTickScaleAndDeltaGet(
    void* entity, float delta, float* movement_delta,
    float* movement_scale);
void ledger_stub_004477d0_PathStepLookaheadEntryWrite(EntitySlot* entity);

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

// FUNCTION: LEGORR 0x0043a120
int entity_cell_step_command_issue_with_jitter(EntitySlot* entity,
    const int target_xy[2], const float input_xy[2], int action_mode,
    int mark_pending)
{
    int start_x, start_y;
    int* path_x = NULL;
    int* path_y = NULL;
    uint32_t path_count = 0u;
    float* route_x;
    float* route_y;
    float jitter_x = 0.5f;
    float jitter_y = 0.5f;
    int mark_step = mark_pending;
    int route_result;
    uint32_t i;
    if (entity == NULL || target_xy == NULL ||
        (entity->flags_3e0 & 0x400u) == 0u) return 0;
    /* FUN_004424f0 resolves the entity's scene position through the active
     * terrain grid before the path finder is called. EntitySlot::x/y are
     * world coordinates, so rounding them directly only works when the
     * terrain origin and cell scale happen to be the identity transform. */
    if (entity->owner_ctx != NULL && entity->owner_ctx->level != NULL &&
        entity->owner_ctx->level->ptr_terrain != NULL) {
        if (!terrain_world_xy_to_cell(
                entity->owner_ctx->level->ptr_terrain, entity->x, entity->y,
                &start_x, &start_y)) return 0;
    }
    else {
        /* The path hook is the only available fallback when the entity has
         * no live terrain owner. */
        start_x = level_cell_of(entity->x);
        start_y = level_cell_of(entity->y);
    }
    if (start_x == target_xy[0] && start_y == target_xy[1]) {
        path_x = (int*)malloc(sizeof(int));
        path_y = (int*)malloc(sizeof(int));
        if (path_x == NULL || path_y == NULL) {
            free(path_x);
            free(path_y);
            return 0;
        }
        path_x[0] = target_xy[0];
        path_y[0] = target_xy[1];
        path_count = 1u;
    }
    else if (entity->owner_ctx != NULL && entity->owner_ctx->level != NULL) {
        if (!terrain_neighbor_path_check(entity->owner_ctx->level,
                (uint32_t)start_x, (uint32_t)start_y,
                (uint32_t)target_xy[0], (uint32_t)target_xy[1],
                &path_x, &path_y, &path_count, NULL, NULL)) return 0;
    }
    else if (s_cell_step_path_hook != NULL) {
        int fallback_xy[2] = { target_xy[0], target_xy[1] };
        if (!s_cell_step_path_hook(start_x, start_y, target_xy[0],
                                   target_xy[1], fallback_xy,
                                   s_cell_step_hook_user)) return 0;
        path_x = (int*)malloc(sizeof(int));
        path_y = (int*)malloc(sizeof(int));
        if (path_x == NULL || path_y == NULL) {
            free(path_x);
            free(path_y);
            return 0;
        }
        path_x[0] = fallback_xy[0];
        path_y[0] = fallback_xy[1];
        path_count = 1u;
    }
    else return 0;
    /* The retail path finder returns heap-owned coordinate arrays and the
     * route installer supports counts beyond EntitySlot's inline waypoint
     * span. Keep only the allocation-overflow guard here; the old inline
     * maximum rejected valid long paths before the dynamic route store saw
     * them. */
    if (path_count == 0u ||
        (size_t)path_count > ((size_t)-1) / sizeof(float)) {
        free(path_x);
        free(path_y);
        return 0;
    }
    route_x = (float*)malloc((size_t)path_count * sizeof(float));
    route_y = (float*)malloc((size_t)path_count * sizeof(float));
    if (route_x == NULL || route_y == NULL) {
        free(route_x);
        free(route_y);
        free(path_x);
        free(path_y);
        return 0;
    }
    for (i = 0u; i < path_count; ++i) {
        route_x[i] = (float)path_x[i];
        route_y[i] = (float)path_y[i];
    }
    route_result = entity_route_setup_and_marker_associate(
        entity, path_count, route_x, route_y, 0);
    free(route_x);
    free(route_y);
    free(path_x);
    free(path_y);
    if (!route_result || entity->wp_count <= 0) return 0;

    if (entity->type == 1) {
        float value = s_cell_step_random_hook != NULL
            ? s_cell_step_random_hook(0.55f, 0.75f, s_cell_step_hook_user)
            : (float)(int16_t)rng_thunk_draw15() * 3.051851e-05f * 0.20f
                + 0.55f;
        int direction;
        if (s_cell_step_random_hook != NULL) {
            direction = (int)s_cell_step_random_hook(
                0.0f, 3.999f, s_cell_step_hook_user);
        }
        else {
            int16_t draw = (int16_t)rng_thunk_draw15();
            direction = draw < 0 ? -(int)draw : (int)draw;
        }
        mark_step = 1;
        switch (direction & 3) {
        case 0: jitter_x = 0.5f; jitter_y = value; break;
        case 1: jitter_x = value; jitter_y = 0.5f; break;
        case 2: jitter_x = 0.5f; jitter_y = 1.0f - value; break;
        default: jitter_x = 1.0f - value; jitter_y = 0.5f; break;
        }
    } else if (input_xy != NULL) {
        jitter_x = input_xy[0];
        jitter_y = input_xy[1];
    }
    AiWaypoint* step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    step->step_kind = (uint8_t)(action_mode == 0 ? 8 : 9);
    if (input_xy != NULL) {
        step->weight_y = jitter_x;
        step->weight_x = jitter_y;
    }
    if (mark_step != 0) step->pending |= 8u;
    return 1;
}

} // namespace unported_ledger_stubs

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

static void spatial_query_request_init(SpatialObjectQueryRequest* request,
                                       uint32_t mask)
{
    memset(request, 0, sizeof(*request));
    request->radius = 10000.0f;
    request->mask = mask;
}

static void spatial_query_center_from_entity(
    SpatialObjectQueryRequest* request, const EntitySlot* entity)
{
    if (entity != NULL) {
        entity_transform_position_get(entity, request->center);
        request->center_is_cell = 0;
    }
}

static void spatial_query_center_from_cell(
    SpatialObjectQueryRequest* request, const int cell_xy[2])
{
    float world_position[3];
    if (request == NULL || cell_xy == NULL) return;
    /* The reference converts the cell to its world-space center. A failed
     * conversion leaves the initialized center at (0, 0), but still runs the
     * entity-pool query. */
    if (terrain_cell_world_position_get(cell_xy[0], cell_xy[1],
                                        world_position)) {
        request->center[0] = world_position[0];
        request->center[1] = world_position[1];
    }
    request->center_is_cell = 0;
}

// FUNCTION: LEGORR 0x00438a60
void* ledger_stub_00438a60_SpatialObjectQueryCenterMask01(
    const int cell_xy[2], uint32_t filter_a, uint32_t filter_b,
    uint32_t filter_c)
{
    SpatialObjectQueryRequest request;
    if (cell_xy == NULL) return NULL;
    spatial_query_request_init(&request, 1u);
    spatial_query_center_from_cell(&request, cell_xy);
    request.filters[0] = filter_a;
    request.filters[1] = filter_b;
    request.filters[2] = filter_c;
    return entity_spatial_query_execute_current(&request);
}

// FUNCTION: LEGORR 0x00438ae0
void* ledger_stub_00438ae0_SpatialObjectQueryCenterMask02(
    const EntitySlot* entity, const int cell_xy[2], uint32_t filter_a,
    uint32_t filter_b)
{
    SpatialObjectQueryRequest request;
    spatial_query_request_init(&request, 2u);
    if (entity != NULL) spatial_query_center_from_entity(&request, entity);
    else spatial_query_center_from_cell(&request, cell_xy);
    request.filters[0] = filter_a;
    /* Retail locals +0x10 and +0x8 map to context words 0 and 2; word 1
     * stays zero. */
    request.filters[2] = filter_b;
    return entity_spatial_query_execute_current(&request);
}

// FUNCTION: LEGORR 0x00438b80
void* ledger_stub_00438b80_SpatialObjectQueryCenterMaskFromArgument(
    const EntitySlot* entity, uint32_t mask)
{
    SpatialObjectQueryRequest request;
    spatial_query_request_init(&request, mask);
    spatial_query_center_from_entity(&request, entity);
    return entity_spatial_query_execute_current(&request);
}

// FUNCTION: LEGORR 0x00438bf0
void* ledger_stub_00438bf0_SpatialObjectQueryCenterMask80(
    const EntitySlot* entity)
{
    SpatialObjectQueryRequest request;
    spatial_query_request_init(&request, 0x80u);
    spatial_query_center_from_entity(&request, entity);
    return entity_spatial_query_execute_current(&request);
}

// FUNCTION: LEGORR 0x00438c60
void* ledger_stub_00438c60_EntityCandidateSearch(const EntitySlot* entity)
{
    SpatialObjectQueryRequest request;
    spatial_query_request_init(&request, 4u);
    spatial_query_center_from_entity(&request, entity);
    return entity_spatial_query_execute_current(&request);
}

// FUNCTION: LEGORR 0x00438cd0
void* ledger_stub_00438cd0_SpatialObjectQueryCenterMask20(
    const float center[2])
{
    EntityRouteMarkerSearchRequest request;
    float position[2];
    position[0] = center[0];
    position[1] = center[1];
    request.position = position;
    request.flags = 0x20u;
    request.best_distance = 10000.0f;
    request.cell_x = 0;
    request.cell_y = 0;
    request.action_argument = 0;
    request.requested_state = 0;
    request.match = NULL;
    entity_pool_find_by_callback_filtered_current(
        entity_route_marker_candidate_visit, &request);
    return request.match;
}

// FUNCTION: LEGORR 0x00438d30
void* ledger_stub_00438d30_SpatialObjectQueryCenterMask10(
    const float center[2])
{
    EntityRouteMarkerSearchRequest request;
    float position[2];
    position[0] = center[0];
    position[1] = center[1];
    request.position = position;
    request.flags = 0x10u;
    request.best_distance = 10000.0f;
    request.cell_x = 0;
    request.cell_y = 0;
    request.action_argument = 0;
    request.requested_state = 0;
    request.match = NULL;
    entity_pool_find_by_callback_filtered_current(
        entity_route_marker_candidate_visit, &request);
    return request.match;
}

// FUNCTION: LEGORR 0x00438d90
void* ledger_stub_00438d90_SpatialObjectQueryCenterMask200(
    const float center[2])
{
    EntityRouteMarkerSearchRequest request;
    float position[2];
    position[0] = center[0];
    position[1] = center[1];
    request.position = position;
    request.flags = 0x200u;
    request.best_distance = 10000.0f;
    request.cell_x = 0;
    request.cell_y = 0;
    request.action_argument = 0;
    request.requested_state = 0;
    request.match = NULL;
    entity_pool_find_by_callback_filtered_current(
        entity_route_marker_candidate_visit, &request);
    return request.match;
}

void* ledger_stub_00438df0_EntityLookupAtWorldPosition(
    const float center[2])
{
    return entity_lookup_at_world_position(center);
}

// FUNCTION: LEGORR 0x00438e50
void* ledger_stub_00438e50_SpatialObjectQueryCenterMask100(
    const EntitySlot* entity, const float center[2], uint32_t extra)
{
    SpatialObjectQueryRequest request;
    spatial_query_request_init(&request, 0x100u);
    if (center != NULL) {
        request.center[0] = center[0];
        request.center[1] = center[1];
    }
    spatial_query_center_from_entity(&request, entity);
    /* The extra argument initializes the final callback-context word. */
    request.filters[3] = extra;
    return entity_spatial_query_execute_current(&request);
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

/* EntityCapacityOrStatePredicate (FUN_0043c650): gate on stats bit 0x80000,
 * compare the indexed stats-table +0xe0 value against the referenced count,
 * then reject an entity carrying the +0x3e8 high-bit state. */
// FUNCTION: LEGORR 0x0043c650
int entity_capacity_or_state_predicate(const EntitySlot* entity)
{
    uint32_t stats_flags;
    uint32_t limit;
    uint32_t current = 0u;
    if (entity == NULL) return 0;
    stats_flags = entity_stats_word_get(entity);
    if ((stats_flags & 0x80000u) == 0u) return 0;
    limit = stats_instance_word_at_e0(entity);
    if (entity->carry_ref_348 != NULL)
        memcpy(&current, entity->carry_ref_348, sizeof(current));
    if (limit != 0u && entity->carry_ref_348 != NULL && limit <= current)
        return 0;
    return (entity->flags_3e8 & 0x80000000u) == 0u ? 1 : 0;
}


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// EntityEnergyLowTargetRouteCommandTick
// FUNCTION: LEGORR 0x0043f7d0
void ledger_stub_0043f7d0_EntityEnergyLowTargetRouteCommandTick(
    EntitySlot* entity, float delta)
{
    float damage = 0.0f;
    float terrain_damage = 0.0f;
    LevelState* level;
    int cell_x, cell_y;
    if (entity == NULL || entity->type == 10 || entity->type == 5) return;
    level = (entity->owner_ctx != NULL) ? entity->owner_ctx->level : NULL;
    if (entity->field_340 <= 0.0f) {
        entity->field_340 = 0.0f;
    }
    else if ((entity->flags_3e8 & 0x80000000u) == 0u) {
        damage = entity_energy_decay_rate_get(entity) * 0.04f * delta;
        if (damage > 0.0f) entity_damage_apply(entity, damage, 0);
    }
    if (entity->progress_344 <= 0.0f) {
        entity->progress_344 = 0.0f;
    }
    else if ((entity->flags_3e8 & 0x80000000u) == 0u) {
        const float decay = entity_stats_float_atdc(entity) * 0.04f * delta;
        entity->progress_344 -= decay;
        if (entity->progress_344 < 0.0f) entity->progress_344 = 0.0f;
    }

    if (level != NULL) {
        uint8_t* cell;
        entity_position_to_terrain_cell(entity, level, &cell_x, &cell_y);
        cell = level_voxel(level, cell_x, cell_y);
        if (entity->attach_304 == NULL && cell != NULL && cell[2] == 6 &&
            (entity->type == 6 || entity->type == 7)) {
            level_state_field9c_add_for_level(level, 1);
            entity->field_340 = -4.0f;
            entity->flags_3e8 |= 0x00800000u;
        }
        if (entity_cell_type_speed_multiplier_resolve(
                entity, level, cell_x, cell_y, delta, &terrain_damage)) {
            entity_damage_apply(entity, terrain_damage, 1);
        }
    }

    if ((entity_state_flags_get(entity) & 0x00010000u) != 0u &&
        (entity->flags_3e0 & 0x00100100u) == 0u &&
        (entity->flags_3e4 & 0x08000000u) == 0u) {
        (void)entity_fence_spark_terrain_interaction(entity);
    }

    /* FUN_0043f7d0: low-resource type-2 entities seek a nearby candidate,
     * then queue its pointer only after one of the four adjacent destination
     * cells passes the terrain route search. */
    if (entity->type == 2 && entity->owner_ctx != NULL &&
        entity->owner_ctx->cfg != NULL &&
        (entity->owner_ctx->cheat & 0x200000u) == 0u &&
        entity->progress_344 < entity->field_340 * 0.01f *
            block_get_float(entity->owner_ctx->cfg,
                            "Main MinEnergyForEat", 0.0f) &&
        entity->activity_counter_3d4 > 125.0f) {
        /* The source clears +0x3d4 as part of the timed gate before testing
         * the carried-object link and pending-command predicate. */
        entity->activity_counter_3d4 = 0.0f;
        if (entity->owner_36c == NULL &&
            !cmd_chain_contains_kind(entity, 0x11)) {
            EntitySlot* candidate = (EntitySlot*)
                ledger_stub_00438c60_EntityCandidateSearch(entity);
            if (candidate != NULL && level != NULL) {
                static const int neighbor_x[4] = { 0, 1, 0, -1 };
                static const int neighbor_y[4] = { -1, 0, 1, 0 };
                int start_x, start_y, target_x, target_y;
                unsigned int i;
                entity_position_to_terrain_cell(entity, level,
                                                 &start_x, &start_y);
                entity_position_to_terrain_cell(candidate, level,
                                                 &target_x, &target_y);
                for (i = 0u; i < 4u; ++i) {
                    int* route_x = NULL;
                    int* route_y = NULL;
                    float route_count = 0.0f;
                    if (terrain_neighbor_path_check_current(
                            entity, (uint32_t)start_x, (uint32_t)start_y,
                            (uint32_t)(target_x + neighbor_x[i]),
                            (uint32_t)(target_y + neighbor_y[i]),
                            &route_x, &route_y, &route_count, NULL, NULL)) {
                        free(route_x);
                        free(route_y);
                        entity_target_command_queue(entity, candidate);
                        break;
                    }
                    free(route_x);
                    free(route_y);
                }
            }
        }
    }
    if (entity->field_340 < entity->progress_344)
        entity->progress_344 = entity->field_340;
    (void)entity_damage_hurt_sound_submit(entity, delta, terrain_damage);
    if (entity->activity_param != 0 && entity->activity_timer >= 0.0f)
        entity->activity_timer += delta;
}


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// RecordPairPoseCopyAndActivityRaise
static EntityPairPoseHooks s_entity_pair_pose_hooks;

void entity_pair_pose_hooks_set(const EntityPairPoseHooks* hooks)
{
    if (hooks == NULL) memset(&s_entity_pair_pose_hooks, 0,
                              sizeof(s_entity_pair_pose_hooks));
    else s_entity_pair_pose_hooks = *hooks;
}

// FUNCTION: LEGORR 0x0043fc20
int ledger_stub_0043fc20_RecordPairPoseCopyAndActivityRaise(
    EntitySlot* entity)
{
    EntitySlot* attached;
    EntityScenePose source_pose;
    float position[3];
    LevelState* level;
    int cell_x, cell_y;
    if (entity == NULL || entity->owner_ctx == NULL ||
        s_entity_pair_pose_hooks.pose_read == NULL ||
        s_entity_pair_pose_hooks.pose_apply == NULL) return 0;
    attached = (EntitySlot*)entity->attach_304;
    if (attached == NULL ||
        !entity_world_center_position_read(attached, position)) return 0;
    if (s_entity_pair_pose_hooks.scene_present != NULL &&
        (!s_entity_pair_pose_hooks.scene_present(
             s_entity_pair_pose_hooks.context, entity) ||
         !s_entity_pair_pose_hooks.scene_present(
             s_entity_pair_pose_hooks.context, attached))) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    cell_x = level_cell_of(position[0]);
    cell_y = level_cell_of(position[1]);
    if (!terrain_cell_route_and_occupancy_check(entity, level,
            cell_x, cell_y, cell_x, cell_y, 0)
        || !s_entity_pair_pose_hooks.pose_read(
            s_entity_pair_pose_hooks.context, attached, &source_pose)) return 0;

    entity->attach_304 = NULL;
    attached->carry_link_308 = NULL;
    attached->flags_3e0 &= ~0x400u;
    attached->carry_state_324 = 0;
    s_entity_pair_pose_hooks.pose_apply(s_entity_pair_pose_hooks.context,
                                        entity, &source_pose);
    entity_activity_toggle(attached, 1);
    ai_idle_chain_record_attach(attached);
    ai_set_activity(entity, 0x34, 0);
    (void)activity_register_bookkeeping(entity);
    ai_idle_chain_record_attach(entity);
    entity->link_2fc = (uintptr_t)attached;
    entity->flags_3ec |= 0x8000u;
    if (s_entity_pair_pose_hooks.activity_raise != NULL)
        s_entity_pair_pose_hooks.activity_raise(
            s_entity_pair_pose_hooks.context, attached, 1);
    return 1;
}


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// TerrainCellToWorldPosition
// FUNCTION: LEGORR 0x004401b0
int ledger_stub_004401b0_TerrainCellToWorldPosition(EntitySlot* entity,
                                                     int force)
{
    EntitySlot* owner;
    EntitySlot* linked;
    LevelState* level;
    int cell_x, cell_y;
    int should_transfer = 0;
    int target_pose_valid = 0;
    int linked_pose_valid = 0;
    float target_position[3];
    EntityScenePose target_pose;
    EntityScenePose linked_pose;
    EntityScenePose entity_pose;
    if (entity == NULL || entity->owner_ctx == NULL ||
        (entity_state_flags_get(entity) & 0x20000u) == 0u) return 0;
    owner = (EntitySlot*)entity->owner_36c;
    if (owner == NULL || (owner->flags_3e0 & 1u) != 0u) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;

    /* FUN_004401b0 asks the scene object for its current position before
     * resolving the terrain cell. EntitySlot coordinates are only the
     * fallback when the renderer has no pose provider. */
    if (s_entity_pair_pose_hooks.pose_read != NULL &&
        s_entity_pair_pose_hooks.pose_read(s_entity_pair_pose_hooks.context,
                                           entity, &entity_pose)) {
        target_position[0] = entity_pose.position[0];
        target_position[1] = entity_pose.position[1];
        target_position[2] = entity_pose.position[2];
        cell_x = level_cell_of(target_position[0]);
        cell_y = level_cell_of(target_position[1]);
    }
    else {
        target_position[0] = entity->x;
        target_position[1] = entity->y;
        target_position[2] = entity->z;
        cell_x = level_cell_of(target_position[0]);
        cell_y = level_cell_of(target_position[1]);
    }
    if ((entity_state_flags_get(entity) & 0x02000000u) != 0u &&
        terrain_cell_route_and_occupancy_check(owner, level, cell_x, cell_y,
                                                cell_x, cell_y, 1))
        should_transfer = 1;
    if (!should_transfer && (entity->flags_3ec & 0x40u) == 0u) {
        int unload_cell[2];
        if (force == 0 &&
            entity_vehicle_unload_cell_find_if_eligible(entity, unload_cell)) {
            if (!terrain_cell_world_position_get(unload_cell[0], unload_cell[1],
                                                 target_position)) return 0;
            cell_x = unload_cell[0];
            cell_y = unload_cell[1];
            should_transfer = 1;
        }
        else if ((entity_state_flags_get(entity) & 0x02000000u) == 0u) {
            object_attached_record_allocate(entity);
        }
    }
    else if (!should_transfer && (entity->flags_3ec & 0x40u) != 0u) {
        linked = (EntitySlot*)entity->link_2fc;
        if (linked != NULL && linked->type == 4) {
            if (s_entity_pair_pose_hooks.scene_present != NULL &&
                !s_entity_pair_pose_hooks.scene_present(
                    s_entity_pair_pose_hooks.context, linked)) {
                linked = NULL;
            }
            if (linked != NULL &&
                s_entity_pair_pose_hooks.pose_read != NULL &&
                s_entity_pair_pose_hooks.pose_read(
                    s_entity_pair_pose_hooks.context, linked, &linked_pose)) {
                target_position[0] = linked_pose.position[0];
                target_position[1] = linked_pose.position[1];
                target_position[2] = linked_pose.position[2];
                cell_x = level_cell_of(target_position[0]);
                cell_y = level_cell_of(target_position[1]);
                linked_pose_valid = 1;
                should_transfer = 1;
            }
            else if (linked != NULL &&
                     entity_world_center_position_read(linked,
                                                       target_position)) {
                cell_x = level_cell_of(target_position[0]);
                cell_y = level_cell_of(target_position[1]);
                should_transfer = 1;
            }
        }
    }
    if (!should_transfer) return 0;

    target_position[2] = (float)terrain_surface_height_get(level->ptr_terrain,
                                                            cell_x, cell_y);
    entity_world_position_set(owner, target_position[0], target_position[1],
                              owner->rot, 0);
    owner->z = target_position[2];

    /* The linked-object branch explicitly copies its orientation. Other
     * branches only set the owner's position, preserving its current scene
     * orientation when the renderer can provide it. */
    if (s_entity_pair_pose_hooks.pose_apply != NULL) {
        if (linked_pose_valid) {
            target_pose = linked_pose;
            target_pose_valid = 1;
        }
        else if (s_entity_pair_pose_hooks.pose_read != NULL &&
                 s_entity_pair_pose_hooks.pose_read(
                     s_entity_pair_pose_hooks.context, owner, &target_pose)) {
            target_pose_valid = 1;
        }
        if (target_pose_valid) {
            target_pose.position[0] = target_position[0];
            target_pose.position[1] = target_position[1];
            target_pose.position[2] = target_position[2];
            s_entity_pair_pose_hooks.pose_apply(
                s_entity_pair_pose_hooks.context, owner, &target_pose);
        }
    }

    entity->owner_36c = NULL;
    owner->owner_36c = NULL;
    owner->flags_3e4 &= ~8u;
    ai_set_activity(owner, 0, 0);
    (void)selection_entity_remove(entity);
    return 1;
}


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// AttachedEntityDirectionVectorUpdate
// FUNCTION: LEGORR 0x00440a10
void ledger_stub_00440a10_AttachedEntityDirectionVectorUpdate(
    EntitySlot* entity)
{
    uint32_t i;
    if (entity == NULL || entity->attached_children == NULL) return;
    for (i = 0u; i < entity->attached_child_count; ++i) {
        EntitySlot* child = entity->attached_children[i];
        void* child_scene;
        void* record = NULL;
        void* parent_scene = NULL;
        int resolver_index = 0;
        int resolver_kind = 0;
        float old_position[3];
        float new_position[3];
        float dx, dy, dz, length, inverse_length;
        if (child == NULL || (child->flags_3e8 & 0x2000u) != 0u) continue;
        child_scene = child->attached_scene_object;
        if (child_scene == NULL ||
            !ddiscene_object_position_get(child_scene, NULL, old_position))
            continue;
        switch (entity->type) {
        case 1:
            record = entity->attached_direction_record_type1;
            resolver_kind = 2;
            resolver_index = (int)entity_indexed_record_word_at_f0_get(
                1, entity->etype, entity->indexed_record_slot_330) +
                child->attached_scene_slot_index;
            break;
        case 2:
            if (entity->owner_36c != NULL &&
                ((EntitySlot*)entity->owner_36c)->type == 1) {
                EntitySlot* linked = (EntitySlot*)entity->owner_36c;
                record = linked->attached_direction_record_type1;
                resolver_kind = 2;
                resolver_index = (int)entity_indexed_record_word_at_f0_get(
                    1, linked->etype, linked->indexed_record_slot_330) +
                    child->attached_scene_slot_index;
            }
            else {
                record = entity->attached_direction_record_type2;
                resolver_kind = 0;
            }
            break;
        case 3:
            record = entity->attached_direction_record_type3;
            resolver_kind = 0;
            break;
        case 4:
            record = entity->attached_direction_record_type4;
            resolver_kind = 1;
            resolver_index = child->attached_scene_slot_index;
            break;
        default:
            continue;
        }
        if (record == NULL) continue;
        switch (resolver_kind) {
        case 1:
            parent_scene = scene_object_indexed_handle_get_or_create(
                record, resolver_index);
            break;
        case 2:
            parent_scene = attached_entity_slot_object_get(
                (SceneResourceAnimationRecord*)record, resolver_index);
            break;
        default:
            parent_scene = renderer_cached_handle_48_get_or_create(record);
            break;
        }
        if (parent_scene == NULL) continue;
        (void)ddiscene_object_position_set(child_scene, parent_scene,
                                           0.0f, 0.0f, 0.0f);
        (void)ddiscene_object_pose_method_dispatch(
            child_scene, parent_scene, 0.0f, 0.0f, 1.0f,
            0.0f, 1.0f, 0.0f);
        if (!ddiscene_object_position_get(child_scene, NULL, new_position))
            continue;
        dx = new_position[0] - old_position[0];
        dy = new_position[1] - old_position[1];
        dz = new_position[2] - old_position[2];
        length = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));
        inverse_length = 1.0f / length;
        child->attached_direction_length = length;
        child->attached_direction[0] = inverse_length * dx;
        child->attached_direction[1] = inverse_length * dy;
        child->attached_direction[2] = inverse_length * dz;
    }
}


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// GridRouteSearchAdapter
// FUNCTION: LEGORR 0x00440c30
void ledger_stub_00440c30_GridRouteSearchAdapter(
    void* entity, uint32_t start_x, uint32_t start_y,
    uint32_t goal_x, uint32_t goal_y, int** out_x, int** out_y,
    uint32_t* out_count)
{
    /* FUN_00440c30 is a void eight-argument forwarder. The reference passes
     * its final count pointer unchanged and supplies null callback/context. */
    (void)terrain_neighbor_path_check(entity, start_x, start_y, goal_x,
                                      goal_y, out_x, out_y, out_count,
                                      NULL, NULL);
}

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

// SubobjectPoseDeltaNormalizeAndDispatch
// FUNCTION: LEGORR 0x004425f0
int ledger_stub_004425f0_SubobjectPoseDeltaNormalizeAndDispatch(
    EntityPoseDeltaState* state, const EntityPoseDeltaHooks* hooks,
    float target_x, float target_y)
{
    return entity_subobject_pose_delta_normalize_and_dispatch(
        state, hooks, target_x, target_y);
}

// EntityMovementAccumulatorApply
// FUNCTION: LEGORR 0x00442b10
int ledger_stub_00442b10_EntityMovementAccumulatorApply(
    EntityMovementAccumulatorState* state, void* entity, float delta,
    float* output_scale, const EntityMovementAccumulatorHooks* hooks)
{
    return entity_movement_accumulator_apply(state, entity, delta,
                                             output_scale, hooks);
}

// EntityPoseOrientationDispatch
// FUNCTION: LEGORR 0x00442f80
int ledger_stub_00442f80_EntityPoseOrientationDispatch(
    int kind, void* entity, const EntityPoseOrientationHooks* hooks)
{
    return entity_pose_orientation_dispatch(kind, entity, hooks);
}

// AdjacentPositionCandidateFind
typedef struct AdjacentRouteAcceptContext {
    const EntitySlot* entity;
    const LevelState* level;
    int route_cookie;
} AdjacentRouteAcceptContext;

static int adjacent_route_candidate_stop(void*, const uint32_t cell[2],
                                         void* context)
{
    AdjacentRouteAcceptContext* accept =
        (AdjacentRouteAcceptContext*)context;
    if (accept == NULL || cell == NULL) return 0;
    return entity_route_cell_candidate_accept(accept->entity, accept->level,
        (int)cell[0], (int)cell[1], accept->route_cookie);
}

// FUNCTION: LEGORR 0x00443710
int ledger_stub_00443710_AdjacentPositionCandidateFind(
    EntitySlot* entity, const int candidate[2], int output[2])
{
    GameContext* context;
    AdjacentRouteAcceptContext accept;
    int* route_x = NULL;
    int* route_y = NULL;
    uint32_t route_count = 0u;
    int found;
    if (entity == NULL || candidate == NULL || output == NULL ||
        candidate[0] < 0 || candidate[1] < 0) return 0;
    context = entity->owner_ctx != NULL ? entity->owner_ctx
                                       : entity_pool_callback_context_get();
    if (context == NULL || context->level == NULL) return 0;
    accept.entity = entity;
    accept.level = context->level;
    accept.route_cookie = 0;
    if (entity_route_cell_candidate_accept(entity, context->level,
            candidate[0], candidate[1], accept.route_cookie)) return 0;

    found = terrain_neighbor_path_check(entity,
        (uint32_t)candidate[0], (uint32_t)candidate[1], 0u, 0u,
        &route_x, &route_y, &route_count,
        adjacent_route_candidate_stop, &accept);
    /* The retail code repeats this route query after a failed attempt. */
    if (!found) {
        free(route_x);
        free(route_y);
        route_x = NULL;
        route_y = NULL;
        route_count = 0u;
        found = terrain_neighbor_path_check(entity,
            (uint32_t)candidate[0], (uint32_t)candidate[1], 0u, 0u,
            &route_x, &route_y, &route_count,
            adjacent_route_candidate_stop, &accept);
    }
    if (!found || route_count == 0u) {
        free(route_x);
        free(route_y);
        return 0;
    }
    output[0] = route_x[route_count - 1u];
    output[1] = route_y[route_count - 1u];
    free(route_x);
    free(route_y);
    return 1;
}

static EntityPathMotionRuntime* entity_path_motion_runtime_get(
    EntitySlot* entity, int create)
{
    EntityPathMotionRuntime* free_slot = NULL;
    unsigned int i;
    if (entity == NULL) return NULL;
    for (i = 0u; i < ENTITY_TYPES * ENTITY_CAP; ++i) {
        EntityPathMotionRuntime* state = &s_entity_path_motion_runtime[i];
        if (state->owner == entity) return state;
        if (free_slot == NULL && state->owner == NULL) free_slot = state;
    }
    if (!create || free_slot == NULL) return NULL;
    memset(free_slot, 0, sizeof(*free_slot));
    free_slot->owner = entity;
    free_slot->waypoint_index = -1;
    return free_slot;
}

void entity_path_motion_runtime_clear_impl(EntitySlot* entity)
{
    unsigned int i;
    if (entity == NULL) return;
    for (i = 0u; i < ENTITY_TYPES * ENTITY_CAP; ++i) {
        if (s_entity_path_motion_runtime[i].owner == entity) {
            memset(&s_entity_path_motion_runtime[i], 0,
                   sizeof(s_entity_path_motion_runtime[i]));
            return;
        }
    }
}

static int entity_path_waypoint_world_get(EntitySlot* entity,
                                          unsigned int index,
                                          float position[2])
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
    if (step->step_kind != 1u) {
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

// StepMovementVectorResolve
// FUNCTION: LEGORR 0x00444680
int entity_step_movement_vector_resolve(EntitySlot* entity, int advance,
                                       int use_saved_vector,
                                       int steering_mode)
{
    EntityPathMotionRuntime* state;
    const AiWaypoint* waypoint;
    float current[2];
    float target[2];
    float dx, dy, length;
    unsigned int index;
    if (entity == NULL || entity->wp_count <= 0) return 0;
    state = entity_path_motion_runtime_get(entity, 1);
    if (state == NULL) return 0;
    if (advance) ++entity->wp_index;
    if (entity->wp_index < 0 || entity->wp_index >= entity->wp_count)
        return 0;
    index = (unsigned int)entity->wp_index;
    waypoint = &ENTITY_WAYPOINTS(entity)[index];
    entity_transform_position_get(entity, current);
    if (!entity_path_waypoint_world_get(entity, index, target)) return 0;
    if (use_saved_vector && (waypoint->pending & 8u) != 0u) {
        const void* terrain = entity->owner_ctx != NULL &&
            entity->owner_ctx->level != NULL
            ? entity->owner_ctx->level->ptr_terrain : NULL;
        const float blocksize = terrain != NULL
            ? terrain_blocksize_get(terrain) : 0.0f;
        dx = (waypoint->weight_y - 0.5f) *
             blocksize;
        dy = (0.5f - waypoint->weight_x) *
             blocksize;
        if (dx == 0.0f && dy == 0.0f) {
            dx = target[0] - current[0];
            dy = target[1] - current[1];
        }
    }
    else if ((waypoint->pending & 4u) != 0u) {
        switch (waypoint->pending & 3u) {
        case 0u: dx = 0.0f; dy = 1.0f; break;
        case 1u: dx = 1.0f; dy = 0.0f; break;
        case 2u: dx = 0.0f; dy = -1.0f; break;
        default: dx = -1.0f; dy = 0.0f; break;
        }
    }
    else {
        dx = target[0] - current[0];
        dy = target[1] - current[1];
    }
    length = (float)sqrt(dx * dx + dy * dy);
    if (length == 0.0f) {
        state->vector[0] = 0.0f;
        state->vector[1] = 0.0f;
        state->total_length = 0.0f;
        state->progress = state->carry_progress;
        state->waypoint_index = entity->wp_index;
        state->start[0] = current[0];
        state->start[1] = current[1];
        state->target[0] = current[0];
        state->target[1] = current[1];
        state->sample_count = 1u;
        state->samples[0][0] = current[0];
        state->samples[0][1] = current[1];
        return 1;
    }
    state->vector[0] = dx / length;
    state->vector[1] = dy / length;
    if (steering_mode != 0) {
        float forward_x = entity->attached_direction[0];
        float forward_y = entity->attached_direction[1];
        float forward_length = (float)sqrt(
            forward_x * forward_x + forward_y * forward_y);
        if (forward_length > 0.0f) {
            float dot;
            float cross;
            forward_x /= forward_length;
            forward_y /= forward_length;
            dot = forward_x * state->vector[0] +
                  forward_y * state->vector[1];
            cross = forward_x * state->vector[1] -
                    forward_y * state->vector[0];
            if (dot < 0.70710678f) {
                unsigned int activity = cross < 0.0f ? 9u : 10u;
                if ((entity->flags_3e0 & 0x400u) != 0u) activity += 11u;
                entity->flags_3e0 |= 4u;
                ai_set_activity(entity, (int)activity, 0);
                (void)activity_register_bookkeeping(entity);
            }
        }
    }
    state->start[0] = current[0];
    state->start[1] = current[1];
    state->target[0] = target[0];
    state->target[1] = target[1];
    state->total_length = length;
    state->progress = state->carry_progress;
    state->sample_count = 2u;
    state->samples[0][0] = current[0];
    state->samples[0][1] = current[1];
    state->samples[1][0] = target[0];
    state->samples[1][1] = target[1];
    state->segment_lengths[0] = length;
    state->waypoint_index = entity->wp_index;
    return 1;
}

// PathStepAdvanceAndArrivalCheck
// FUNCTION: LEGORR 0x004438b0
float entity_path_step_advance_and_arrival_check(EntitySlot* entity,
                                                  float delta)
{
    EntityPathMotionRuntime* state;
    float movement_delta = 0.0f;
    float movement_scale = 1.0f;
    float position[2];
    float path_position[2];
    uint32_t path_flags;
    if (entity == NULL) return 0.0f;
    if (entity->wp_count <= 0) {
        entity_action_sequence_reset(entity, 0);
        return (float)entity->flags_3e0;
    }
    if (entity->wp_index < 0) entity->wp_index = 0;
    if (entity->wp_index >= entity->wp_count) {
        entity_action_sequence_reset(entity, 1);
        return (float)entity->flags_3e0;
    }
    state = entity_path_motion_runtime_get(entity, 1);
    if (state == NULL) return (float)entity->flags_3e0;
    if (state->waypoint_index != entity->wp_index ||
        state->total_length <= 0.0f) {
        state->carry_progress = 0.0f;
        (void)entity_step_movement_vector_resolve(entity, 0,
            (entity->flags_3e8 & 8u) != 0u,
            (entity->flags_3e0 & 0x400u) == 0u);
    }
    if (state->waypoint_index != entity->wp_index) return (float)entity->flags_3e0;

    unported_ledger_stubs::ledger_stub_00446610_EntityMovementTickScaleAndDeltaGet(
        entity, delta, &movement_delta, &movement_scale);
    (void)movement_scale;
    if ((entity->flags_3e8 & 0x4000u) == 0u)
        state->progress += movement_delta;
    if (state->progress > state->total_length) {
        state->carry_progress = state->progress - state->total_length;
        state->progress = state->total_length;
    }
    else {
        state->carry_progress = 0.0f;
    }
    path_position[0] = state->samples[0][0];
    path_position[1] = state->samples[0][1];
    if (state->sample_count > 1u && state->total_length > 0.0f) {
        float fraction = state->progress / state->total_length;
        path_position[0] += (state->samples[1][0] - state->samples[0][0]) * fraction;
        path_position[1] += (state->samples[1][1] - state->samples[0][1]) * fraction;
    }
    entity_world_position_set(entity, path_position[0], path_position[1],
                              entity->rot, 1);

    path_flags = ENTITY_WAYPOINTS(entity)[entity->wp_index].pending;
    entity_transform_position_get(entity, position);
    if ((entity->flags_3e0 & 8u) == 0u &&
        ENTITY_WAYPOINTS(entity)[entity->wp_index].step_kind == 3u &&
        entity->link_2fc != 0) {
        float work_position[2];
        float dx, dy, distance;
        if (entity_work_object_position_get((EntitySlot*)entity->link_2fc,
                                            work_position)) {
            dx = work_position[0] - position[0];
            dy = work_position[1] - position[1];
            distance = (float)sqrt(dx * dx + dy * dy);
            if (distance <= 6.6f) {
                entity->flags_3e0 |= 0x200u;
                entity_action_sequence_reset(entity, 1);
                return (float)entity->flags_3e0;
            }
        }
    }
    if (ENTITY_WAYPOINTS(entity)[entity->wp_index].step_kind == 12u &&
        entity->link_2fc != 0) {
        EntitySlot* linked = (EntitySlot*)entity->link_2fc;
        float work_position[2];
        float dx, dy;
        if (entity_work_object_position_get(linked, work_position)) {
            dx = work_position[0] - position[0];
            dy = work_position[1] - position[1];
            if ((float)sqrt(dx * dx + dy * dy) <=
                entity_stats_float_at6c(linked)) {
                entity->flags_3e4 |= 4u;
                entity_action_sequence_reset(entity, 1);
                return (float)entity->flags_3e4;
            }
        }
    }
    if (ENTITY_WAYPOINTS(entity)[entity->wp_index].step_kind == 9u &&
        (path_flags & 0x0cu) == 0u) {
        float dx = state->target[0] - position[0];
        float dy = state->target[1] - position[1];
        if ((float)sqrt(dx * dx + dy * dy) <= 6.0f) {
            entity->flags_3e0 |= 0x80000u;
            entity_action_sequence_reset(entity, 1);
            return (float)entity->flags_3e0;
        }
    }
    if (state->progress >= state->total_length) {
        if (entity->wp_index + 1 < entity->wp_count) {
            ++entity->wp_index;
            if ((path_flags & 0x10u) != 0u &&
                ENTITY_WAYPOINTS(entity)[entity->wp_index].step_kind != 1u)
                ++entity->wp_index;
            if (entity->wp_index < entity->wp_count &&
                ENTITY_WAYPOINTS(entity)[entity->wp_index].step_kind == 5u)
                unported_ledger_stubs::ledger_stub_004477d0_PathStepLookaheadEntryWrite(
                    entity);
            state->waypoint_index = -1;
            if (entity->wp_index >= entity->wp_count - 1)
                entity->flags_3e8 |= 0x400u;
        }
        else {
            entity->flags_3e8 |= 0x400u;
        }
    }

    if ((entity->flags_3e8 & 0x400u) != 0u) {
        const unsigned int terminal_index =
            entity->wp_index >= 0 && entity->wp_index < entity->wp_count
                ? (unsigned int)entity->wp_index
                : (unsigned int)(entity->wp_count - 1);
        const AiWaypoint* terminal = &ENTITY_WAYPOINTS(entity)[terminal_index];
        const unsigned int kind = terminal->step_kind;
        switch (kind) {
        case 2u: entity->flags_3e0 |= 0x40u; break;
        case 3u: entity->flags_3e0 |= 0x200u; break;
        case 4u: entity->flags_3e0 |= 0x40000u; break;
        case 5u: entity->flags_3e0 |= 0x800u; break;
        case 6u: entity->flags_3e0 |= 0x1000000u; break;
        case 7u: entity->flags_3e0 |= 0x2000u; break;
        case 8u: entity_action_sequence_reset(entity, 1); break;
        case 9u: entity->flags_3e0 |= 0x80000u; break;
        case 10u: entity->flags_3e0 |= 0x20000u; break;
        case 11u: entity->flags_3e0 |= 0x40000000u; break;
        case 12u: entity->flags_3e4 |= 4u; break;
        case 13u: entity->flags_3e4 |= 0x40u; break;
        case 14u: entity->flags_3e4 |= 0x400u; break;
        case 15u: entity->flags_3e4 |= 0x8000u; break;
        case 16u: entity->flags_3e4 |= 0x200000u; break;
        case 17u: entity->flags_3e4 |= 0x4000000u; break;
        case 18u: entity->flags_3e4 |= 0x10000000u; break;
        case 19u: {
            EntitySlot* linked = (EntitySlot*)entity->link_2fc;
            if (linked != NULL && linked->type == 4) {
                entity->flags_3ec |= 0x40u;
                linked->flags_3ec |= 0x40u;
                cmd_chain_drop(entity, 0x1c);
            }
            break;
        }
        case 20u: cmd_chain_drop(entity, 0x1d); break;
        case 21u: entity->flags_3e4 |= 0x80000000u; break;
        default: break;
        }
        if ((terminal->pending & 0x20u) != 0u)
            entity->flags_3e4 |= 0x80000u;
        entity_action_sequence_reset(entity, 1);
        return (float)entity->flags_3e0;
    }
    return (float)entity->flags_3e0;
}

float ledger_stub_004438b0_PathStepAdvanceAndArrivalCheck(
    EntitySlot* entity, float delta)
{
    return entity_path_step_advance_and_arrival_check(entity, delta);
}

// ActivityTimeoutAccumulateAndRaise
// FUNCTION: LEGORR 0x004440f0
void ledger_stub_004440f0_ActivityTimeoutAccumulateAndRaise(
    void* entity, ActivityTimeoutState* state,
    const ActivityTimeoutHooks* hooks, float delta)
{
    activity_timeout_accumulate_and_raise(entity, state, hooks, delta);
}

// EntityRandomCellStepRouteIssue
// FUNCTION: LEGORR 0x00444260
int ledger_stub_00444260_EntityRandomCellStepRouteIssue(EntitySlot* entity)
{
    static const int offsets[8][2] = {
        { -1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 },
        { 1, 0 }, { 1, -1 }, { 0, -1 }, { -1, -1 }
    };
    GameContext* context;
    LevelState* level;
    TerrainSurfaceGrid* surface;
    int start_x, start_y;
    float start_position[2];
    unsigned int attempt;
    if (entity == NULL ||
        entity_flags_mask107c33de_or_1c00203_any_test(entity)) return 0;
    context = entity->owner_ctx != NULL ? entity->owner_ctx
                                       : entity_pool_callback_context_get();
    if (context == NULL || context->level == NULL) return 0;
    level = context->level;
    surface = path_record_surface_grid_get();
    if (surface == NULL || surface->surface_vectors == NULL) return 0;
    entity_transform_position_get(entity, start_position);
    if (!terrain_world_xy_to_cell(level->ptr_terrain, start_position[0],
                                  start_position[1], &start_x, &start_y))
        return 0;
    for (attempt = 0u; attempt < 16u; ++attempt) {
        unsigned int direction =
            (unsigned int)level_random_draw15() % 8u;
        int cell_x = start_x + offsets[direction][0];
        int cell_y = start_y + offsets[direction][1];
        int* path_x = NULL;
        int* path_y = NULL;
        uint32_t path_count = 0u;
        uint32_t i;
        float vertices[12];
        float marker[2];
        float* route_x;
        float* route_y;
        int route_ok;
        if (cell_x < 0 || cell_y < 0 ||
            (uint32_t)cell_x >= surface->width ||
            (uint32_t)cell_y >= surface->height ||
            terrain_cell_route_and_occupancy_check(
                entity, level, cell_x, cell_y, cell_x, cell_y, 0) == 0 ||
            terrain_cell_surface_vertices_get(surface,
                (uint32_t)cell_x, (uint32_t)cell_y, vertices) < 0) {
            continue;
        }
        marker[0] = random_float_range(vertices[0] + 1.0f,
                                       vertices[6] - 1.0f);
        marker[1] = random_float_range(vertices[1] - 1.0f,
                                       vertices[7] + 1.0f);
        if (!terrain_neighbor_path_check(entity, (uint32_t)start_x,
                (uint32_t)start_y, (uint32_t)cell_x, (uint32_t)cell_y,
                &path_x, &path_y, &path_count, NULL, NULL) ||
            path_count == 0u) {
            free(path_x);
            free(path_y);
            continue;
        }
        route_x = (float*)malloc((size_t)path_count * sizeof(float));
        route_y = (float*)malloc((size_t)path_count * sizeof(float));
        if (route_x == NULL || route_y == NULL) {
            free(route_x);
            free(route_y);
            free(path_x);
            free(path_y);
            continue;
        }
        for (i = 0u; i < path_count; ++i) {
            route_x[i] = (float)path_x[i];
            route_y[i] = (float)path_y[i];
        }
        route_ok = entity_route_setup_and_marker_associate(
            entity, path_count, route_x, route_y, marker);
        free(route_x);
        free(route_y);
        free(path_x);
        free(path_y);
        if (route_ok) return 1;
    }
    return 0;
}

int ledger_stub_00444680_StepMovementVectorResolve(
    EntitySlot* entity, int advance, int use_saved_vector,
    int steering_mode)
{
    return entity_step_movement_vector_resolve(entity, advance,
                                               use_saved_vector,
                                               steering_mode);
}


} // namespace unported_ledger_stubs

static void entity_path_motion_runtime_clear(EntitySlot* entity)
{
    unported_ledger_stubs::entity_path_motion_runtime_clear_impl(entity);
}

namespace unported_ledger_stubs {

// EntityTurnToVectorActivityIssue
// FUNCTION: LEGORR 0x00444fb0
void ledger_stub_00444fb0_EntityTurnToVectorActivityIssue(
    EntitySlot* entity, const float target[2])
{
    float current_length;
    float target_length;
    float current_x;
    float current_y;
    float target_x;
    float target_y;
    float dot;
    float angle;
    float cross[3];
    unsigned int activity;
    if (entity == NULL || target == NULL || entity->type != 2) return;
    current_length = (float)sqrt((double)(
        entity->attached_direction[0] * entity->attached_direction[0] +
        entity->attached_direction[1] * entity->attached_direction[1] +
        entity->attached_direction[2] * entity->attached_direction[2]));
    target_length = (float)sqrt((double)(target[0] * target[0] +
                                          target[1] * target[1]));
    if (current_length == 0.0f || target_length == 0.0f) return;
    entity->attached_direction[0] /= current_length;
    entity->attached_direction[1] /= current_length;
    entity->attached_direction[2] /= current_length;
    current_x = entity->attached_direction[0];
    current_y = entity->attached_direction[1];
    target_x = target[0] / target_length;
    target_y = target[1] / target_length;
    dot = current_x * target_x + current_y * target_y;
    if (dot > 1.0f) dot = 1.0f;
    else if (dot < -1.0f) dot = -1.0f;
    angle = (float)acos((double)dot);
    entity_action_sequence_reset(entity, 0);
    if (angle <= 0.7853982f) return;
    if (angle <= 2.3561945f) {
        cross[0] = target_y * entity->attached_direction[2];
        cross[1] = -target_x * entity->attached_direction[2];
        cross[2] = target_x * entity->attached_direction[1] -
                   target_y * entity->attached_direction[0];
        d3drm_vector_rotate(entity->attached_direction,
                            entity->attached_direction, cross,
                            -1.57079637f);
        if (cross[2] < 0.0f) {
            activity = 9u;
        }
        else {
            activity = 10u;
        }
    }
    else {
        entity->attached_direction[0] = -entity->attached_direction[0];
        entity->attached_direction[1] = -entity->attached_direction[1];
        entity->flags_3e0 |= 0x80u;
        activity = 10u;
    }
    if ((entity->flags_3e0 & 0x400u) != 0u) activity += 11u;
    entity->flags_3e0 |= 4u;
    ai_set_activity(entity, (int)activity, 0);
    (void)activity_register_bookkeeping(entity);
}

// EntityPathStepVectorNormalize
// FUNCTION: LEGORR 0x004451e0
void ledger_stub_004451e0_EntityPathStepVectorNormalize(
    EntityPathStepState* state, void* entity,
    const EntityPathStepHooks* hooks)
{
    entity_path_step_vector_normalize(state, entity, hooks);
}

// EntityMoveVectorSurfaceProbe
// FUNCTION: LEGORR 0x004455a0
int ledger_stub_004455a0_EntityMoveVectorSurfaceProbe(
    EntitySlot* entity, const EntityMotionSurfaceProbeHooks* hooks)
{
    return entity_move_vector_surface_probe(entity, hooks);
}

// EntityBehaviorCallbackTick
// FUNCTION: LEGORR 0x004456e0
void ledger_stub_004456e0_EntityBehaviorCallbackTick(
    EntityBehaviorCallbackState* state,
    const EntityBehaviorCallbackHooks* hooks, void* entity)
{
    entity_behavior_callback_tick(state, hooks, entity);
}

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

static float entity_movement_default_base_speed_get(void* context,
                                                     void* entity,
                                                     int kind)
{
    const EntitySlot* slot = (const EntitySlot*)entity;
    (void)context;
    (void)kind;
    return entity_movement_component_float_get(
        slot, 0x20, entity_stats_float_at20(slot));
}

static int entity_movement_default_terrain_data_get(
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

static float entity_movement_default_surface_scale_get(void* context,
                                                        void* entity)
{
    (void)context;
    return entity_movement_component_float_get(
        (const EntitySlot*)entity, 0xd4,
        entity_stats_float_atd4((const EntitySlot*)entity));
}

static float entity_movement_default_entity_scale_get(void* context,
                                                       void* entity)
{
    (void)context;
    return entity_movement_component_float_get(
        (const EntitySlot*)entity, 0xd8,
        entity_stats_float_atd8((const EntitySlot*)entity));
}

static float entity_movement_default_animation_scale_get(void* context,
                                                          void* entity)
{
    (void)context;
    return entity_movement_component_float_get(
        (const EntitySlot*)entity, 0xf8,
        entity_speed_component_get((const EntitySlot*)entity));
}

// FUNCTION: LEGORR 0x00446610
void ledger_stub_00446610_EntityMovementTickScaleAndDeltaGet(
    void* entity, float delta, float* movement_delta,
    float* movement_scale)
{
    EntityMovementTickScaleState state;
    EntityMovementTickScaleHooks hooks;
    const EntitySlot* slot = (const EntitySlot*)entity;
    if (slot == NULL || movement_delta == NULL || movement_scale == NULL)
        return;
    memset(&state, 0, sizeof(state));
    state.kind = slot->type;
    state.flags_f8 = slot->flags_3e0;
    state.flags_f9 = slot->flags_3e4;
    state.flags_fa = slot->flags_3e8;
    state.stats_flags = entity_stats_word_get(slot);
    state.surface_scale = entity_stats_float_atd4(slot);
    hooks.base_speed_get = entity_movement_default_base_speed_get;
    hooks.terrain_data_get = entity_movement_default_terrain_data_get;
    hooks.surface_scale_get = entity_movement_default_surface_scale_get;
    hooks.entity_scale_get = entity_movement_default_entity_scale_get;
    hooks.animation_scale_get = entity_movement_default_animation_scale_get;
    hooks.context = NULL;
    entity_movement_tick_scale_and_delta_get(
        &state, entity, delta, movement_delta, movement_scale, &hooks);
}

// EntityNearestConnectedTerrainCellRouteSelect
static int entity_route_to_cell_commit(EntitySlot* entity,
                                       int target_x, int target_y,
                                       unsigned int marker);
// FUNCTION: LEGORR 0x004469c0
int ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
    EntitySlot* entity, unsigned int goal_x, unsigned int goal_y,
    int include_diagonals, unsigned int* out_candidate, int strict)
{
    static const int cardinal_offsets[4][2] = {
        { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 }
    };
    static const int diagonal_offsets[8][2] = {
        { -1, -2 }, { 0, -2 }, { 1, -1 }, { 1, 0 },
        { 0, 1 }, { -1, 1 }, { -2, 0 }, { -2, -1 }
    };
    int* path_x[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    int* path_y[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    uint32_t path_count[8] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    int candidate_x[8];
    int candidate_y[8];
    LevelState* level;
    GameContext* context;
    float goal_center[2];
    float entity_position[2];
    int terrain_start_x;
    int terrain_start_y;
    float blocksize;
    int direct_route_allowed = 1;
    int start_x, start_y;
    unsigned int best_index = 0xffffffffu;
    unsigned int candidate_count;
    unsigned int best_count = 0xffffffffu;
    unsigned int i;
    const int (*offsets)[2];
    if (entity == NULL) return 0;

    context = entity->owner_ctx != NULL ? entity->owner_ctx
                                        : entity_pool_callback_context_get();
    level = context != NULL ? context->level : NULL;
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    entity_transform_position_get(entity, entity_position);
    if (!terrain_world_xy_to_cell(level->ptr_terrain, entity_position[0],
                                  entity_position[1], &terrain_start_x,
                                  &terrain_start_y)) return 0;
    start_x = terrain_start_x;
    start_y = terrain_start_y;
    candidate_count = include_diagonals ? 8u : 4u;
    offsets = include_diagonals ? diagonal_offsets : cardinal_offsets;

    /* When param_6 is clear, the corpus suppresses the direct-current-cell
     * shortcut if the entity is already within one terrain block of the
     * goal cell center. */
    if (strict == 0) {
        if (terrain_cell_world_center_xy_get(level->ptr_terrain,
                (int)goal_x, (int)goal_y, goal_center)) {
            blocksize = terrain_blocksize_get(level->ptr_terrain);
            if (blocksize > 0.0f) {
                const float dx = goal_center[0] - entity_position[0];
                const float dy = goal_center[1] - entity_position[1];
                if ((float)sqrt((double)(dx * dx + dy * dy)) < blocksize)
                    direct_route_allowed = 0;
            }
        }
    }

    for (i = 0; i < candidate_count; ++i) {
        candidate_x[i] = (int)goal_x + offsets[i][0];
        candidate_y[i] = (int)goal_y + offsets[i][1];
        if (candidate_x[i] == start_x && candidate_y[i] == start_y &&
            direct_route_allowed &&
            (include_diagonals != 0 ||
             !level_cell_walkable_flag(level, start_x, start_y))) {
            const float direct_x = (float)start_x;
            const float direct_y = (float)start_y;
            if (!entity_route_setup_and_marker_associate(
                    entity, 1u, &direct_x, &direct_y, NULL)) return 0;
            if (out_candidate != NULL) *out_candidate = i;
            return 1;
        }
    }
    for (i = 0; i < candidate_count; ++i) {
        if (candidate_x[i] == start_x && candidate_y[i] == start_y) continue;
        if (!terrain_neighbor_path_check(entity, (uint32_t)start_x,
            (uint32_t)start_y, (uint32_t)candidate_x[i],
            (uint32_t)candidate_y[i], &path_x[i], &path_y[i],
            &path_count[i], NULL, NULL)) {
            free(path_x[i]);
            free(path_y[i]);
            path_x[i] = NULL;
            path_y[i] = NULL;
            path_count[i] = 0u;
        }
    }

    /* Retail ranks every successful non-walkable route first. If none has a
     * route, it makes a second pass over all successful candidates. Strict
     * less-than comparisons preserve the first candidate on equal lengths. */
    for (i = 0; i < candidate_count; ++i) {
        if (path_count[i] != 0u &&
            !level_cell_walkable_flag(level, candidate_x[i], candidate_y[i]) &&
            path_count[i] < best_count) {
            best_count = path_count[i];
            best_index = i;
        }
    }
    if (best_index == 0xffffffffu) {
        for (i = 0; i < candidate_count; ++i) {
            if (path_count[i] != 0u && path_count[i] < best_count) {
                best_count = path_count[i];
                best_index = i;
            }
        }
    }
    if (best_index == 0xffffffffu) goto cleanup;
    {
        float* route_x;
        float* route_y;
        unsigned int j;
        if ((size_t)best_count > ((size_t)-1) / sizeof(float)) {
            best_index = 0xffffffffu;
            goto cleanup;
        }
        route_x = (float*)malloc((size_t)best_count * sizeof(float));
        route_y = (float*)malloc((size_t)best_count * sizeof(float));
        if (route_x == NULL || route_y == NULL) {
            free(route_x);
            free(route_y);
            best_index = 0xffffffffu;
            goto cleanup;
        }
        for (j = 0; j < best_count; ++j) {
            route_x[j] = (float)path_x[best_index][j];
            route_y[j] = (float)path_y[best_index][j];
        }
        i = entity_route_setup_and_marker_associate(entity, best_count,
                                                     route_x, route_y, NULL);
        free(route_x);
        free(route_y);
        if (i == 0u) best_index = 0xffffffffu;
    }
cleanup:
    for (i = 0; i < candidate_count; ++i) {
        free(path_x[i]);
        free(path_y[i]);
    }
    if (best_index == 0xffffffffu) return 0;
    if (out_candidate != NULL) *out_candidate = best_index;
    return 1;
}

// EntityNearestCellRouteCommit
// FUNCTION: LEGORR 0x00446e40
int ledger_stub_00446e40_EntityNearestCellRouteCommit(
    EntitySlot* entity, unsigned int goal_x, unsigned int goal_y,
    int mode)
{
    static const int appended_offset_x[8] = {
        -1, 0, 0, 0, 0, -1, -1, -1
    };
    static const int appended_offset_y[8] = {
        -1, -1, -1, 0, 0, 0, 0, -1
    };
    static const float waypoint_weight_y[8] = {
        0.9f, 0.1f, 0.5f, 0.5f, 0.1f, 0.9f, 0.5f, 0.5f
    };
    static const float waypoint_weight_x[8] = {
        0.5f, 0.5f, 0.1f, 0.9f, 0.5f, 0.5f, 0.9f, 0.1f
    };
    unsigned int candidate;
    if (entity == NULL ||
        !terrain_cell_walkability_query_current(entity, goal_x, goal_y,
                                                mode, 1) ||
        !ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
            entity, goal_x, goal_y, mode, &candidate, 1)) return 0;
    if (candidate >= (mode != 0 ? 8u : 4u)) return 0;
    if (mode != 0) {
        AiWaypoint* final_step;
        if (!entity_route_adjacent_step_append(entity,
                (int)goal_x + appended_offset_x[candidate],
                (int)goal_y + appended_offset_y[candidate]) ||
            entity->wp_count <= 0 ||
            (entity->wp_dynamic == NULL
                ? entity->wp_count > AI_WAYPOINTS_MAX
                : (unsigned int)entity->wp_count > entity->wp_capacity))
            return 0;
        final_step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
        final_step->weight_y = waypoint_weight_y[candidate];
        final_step->weight_x = waypoint_weight_x[candidate];
    }
    else {
        AiWaypoint* final_step;
        if (!entity_route_adjacent_step_append(entity, (int)goal_x,
                                                (int)goal_y) ||
            entity->wp_count <= 0 ||
            (entity->wp_dynamic == NULL
                ? entity->wp_count > AI_WAYPOINTS_MAX
                : (unsigned int)entity->wp_count > entity->wp_capacity))
            return 0;
        final_step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
        final_step->weight_y = random_float_range(0.3f, 0.7f);
        final_step->weight_x = random_float_range(0.3f, 0.7f);
    }
    entity->pos_pair_2f4 = (float)goal_x;
    entity->pos_pair_2f8 = (float)goal_y;
    if (mode != 0) entity->flags_3e8 |= 0x02000000u;
    else entity->flags_3e8 &= ~0x02000000u;
    return 1;
}

// RegisterRelativeStateUpdateFragment
// FUNCTION: LEGORR 0x0044702d
int ledger_stub_0044702d_RegisterRelativeStateUpdateFragment(
    EntitySlot* entity, float target_x, float target_y)
{
    int result;
    AiWaypoint* step;
    if (entity == NULL) return 0;
    result = entity_route_to_cell_commit(entity, (int)target_x,
                                         (int)target_y, 3u);
    if (!result || entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity))
        return 0;
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    step->weight_x = random_float_range(0.3f, 0.7f);
    step->weight_y = random_float_range(0.3f, 0.7f);
    entity->flags_3e8 &= ~0x02000000u;
    return result;
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

// EntityAdjacentPlacementCellSearch
int ledger_stub_004473b0_RouteToCellCommandIssue(
    EntitySlot* entity, int target_x, int target_y, EntitySlot* target);
int ledger_stub_00447210_EntityAdjacentPlacementCellSearch(
    EntitySlot* entity, EntitySlot* target)
{
    return lego::entity_adjacent_placement_cell_search(entity, target);
}

// RouteToCellCommandIssue
// FUNCTION: LEGORR 0x004473b0
int ledger_stub_004473b0_RouteToCellCommandIssue(
    EntitySlot* entity, int target_x, int target_y, EntitySlot* target)
{
    LevelState* level;
    int target_cell[2];
    unsigned int candidate;
    unsigned int direction;
    float edge_weight;
    float step_fraction;
    AiWaypoint* step;
    if (entity == NULL || target == NULL || entity->owner_ctx == NULL ||
        (entity->flags_3e0 & 0x400u) == 0u ||
        entity_flags_mask107c33de_or_1c00203_any_test(entity)) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    entity_position_to_terrain_cell(target, level, &target_cell[0],
                                    &target_cell[1]);
    if (target_cell[0] == target_x && target_cell[1] == target_y) return 0;
    if (!entity_route_to_cell_commit(entity, target_x, target_y, 5u)) return 0;
    if (entity->carry_link_308 != NULL)
        ((EntitySlot*)entity->carry_link_308)->link_2fc = (uintptr_t)target;
    return 1;
}

// LinkedEntityProximityResponse
void ledger_stub_00447950_EntityDirectionalActivityStart(
    EntitySlot* entity, const float direction[2], int mode);
// FUNCTION: LEGORR 0x004474f0
void ledger_stub_004474f0_LinkedEntityProximityResponse(EntitySlot* entity)
{
    EntitySlot* linked;
    float entity_position[3];
    float linked_position[3];
    float dx, dy, distance_squared, radius;
    if (entity == NULL) return;
    linked = (EntitySlot*)entity->link_2fc;
    entity->link_2fc = 0;
    if (linked == NULL ||
        !entity_world_center_position_read(entity, entity_position) ||
        !entity_world_center_position_read(linked, linked_position)) return;
    dx = linked_position[0] - entity_position[0];
    dy = linked_position[1] - entity_position[1];
    distance_squared = dx * dx + dy * dy;
    radius = entity_collision_radius_get(entity) * 2.0f +
             entity_collision_radius_get(linked);
    if (distance_squared < radius * radius) {
        const float direction[2] = { dx, dy };
        ledger_stub_00447950_EntityDirectionalActivityStart(linked,
                                                            direction, 1);
        entity_damage_apply(linked, 10.0f, 1);
    }
}

// EntityProgressAndNegativeDeltaHandler
// FUNCTION: LEGORR 0x004475c0
int ledger_stub_004475c0_EntityProgressAndNegativeDeltaHandler(
    EntitySlot* entity)
{
    EntitySlot* target;
    float delta;
    int complete = 0;
    if (entity == NULL) return 1;
    target = (EntitySlot*)entity->link_2fc;
    if (target == NULL) return 1;
    if ((target->flags_3e4 & 0x00100000u) != 0u) return 0;

    if ((entity_state_flags_get(entity) & 0x00080000u) == 0u) {
        if ((entity->flags_3e8 & 0x00400000u) == 0u) {
            if (entity->type == 3) {
                message_dispatch(5, 0u, (uint32_t)(uintptr_t)target, NULL);
            }
            delta = entity_stats_float_at88(entity);
            if (delta >= 0.0f) {
                target->field_340 += delta;
            }
            else {
                entity_damage_apply(target, -delta, 1);
                target->flags_3e4 |= 0x40000000u;
            }
            if (target->field_340 < 100.0f) {
                entity_threshold_progress_event_queue(target, 0);
            }
            else {
                target->field_340 = 100.0f;
                complete = 1;
            }
        }
        else {
            building_upgrade_commit(target);
            entity->flags_3ec &= ~0x00400000u;
        }
    }
    else {
        if (!entity_shared_eligibility_check(target, 0)) return 1;
        {
            GameContext* context = entity->owner_ctx != NULL
                ? entity->owner_ctx : entity_pool_callback_context_get();
            float drain_time = context != NULL && context->cfg != NULL
                ? block_get_float(context->cfg, "Main DrainTime", 10.0f)
                : 10.0f;
            if (drain_time == 0.0f) drain_time = 10.0f;
            if (entity->activity_counter_3d8 >= drain_time * 25.0f) {
                process_queued_grid_animations(entity, -1, 1.0f);
                entity->activity_counter_3d8 = 0.0f;
            }
        }
    }
    if (target->field_340 <= 0.0f) complete = 1;
    if (complete) entity->link_2fc = 0;
    return complete;
}

// PathStepLookaheadEntryWrite
// FUNCTION: LEGORR 0x004477d0
void ledger_stub_004477d0_PathStepLookaheadEntryWrite(EntitySlot* entity)
{
    EntitySlot* linked;
    EntitySlot* target;
    LevelState* level;
    void* terrain;
    AiWaypoint* waypoint;
    float start_world[2];
    float target_world[2];
    float moved_world[2];
    float center[2];
    float blocksize;
    float dx;
    float dy;
    float length;
    int cell_x;
    int cell_y;
    GameContext* context;

    if (entity == NULL || entity->wp_count <= 0
        || (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) {
        return;
    }
    /* The reference reads DAT_005560d4 rather than an entity-owned level.
     * Use the entity's context when present and the active pool context for
     * entities constructed outside the normal pool owner path. */
    context = entity->owner_ctx != NULL ? entity->owner_ctx
                                       : entity_pool_callback_context_get();
    if (context == NULL || context->level == NULL) return;
    level = context->level;
    terrain = level->ptr_terrain;
    if (terrain == NULL) return;
    linked = (EntitySlot*)entity->carry_link_308;
    if (linked == NULL) {
        return;
    }
    target = (EntitySlot*)linked->link_2fc;
    if (target == NULL) {
        return;
    }

    waypoint = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    if (!terrain_cell_world_center_xy_get(terrain, (int)waypoint->x,
                                          (int)waypoint->y, start_world))
        return;
    entity_transform_position_get(target, target_world);
    dx = target_world[0] - start_world[0];
    dy = target_world[1] - start_world[1];
    length = (float)sqrt((double)(dx * dx + dy * dy));
    if (length == 0.0f) return;
    moved_world[0] = start_world[0] + (dx / length) * 0.1f;
    moved_world[1] = start_world[1] + (dy / length) * 0.1f;
    if (!terrain_world_xy_to_cell(terrain, moved_world[0], moved_world[1],
                                  &cell_x, &cell_y) ||
        !terrain_cell_world_center_xy_get(terrain, cell_x, cell_y, center))
        return;
    blocksize = terrain_blocksize_get(terrain);
    if (blocksize == 0.0f) return;
    waypoint->weight_x = (moved_world[0] - center[0]) / blocksize + 0.5f;
    waypoint->weight_y = (moved_world[1] - center[1]) / blocksize + 0.5f;
    waypoint->pending = (uint8_t)(waypoint->pending | 8u);
    linked->pos_pair_2f4 = target_world[0];
    linked->pos_pair_2f8 = target_world[1];
    linked->link_2fc = 0;
}

// EntityDirectionalActivityStart
void ledger_stub_00447950_EntityDirectionalActivityStart(
    EntitySlot* entity, const float direction[2], int mode)
{
    entity_directional_activity_start(entity, direction, mode);
}

// EntityActivityTick
// FUNCTION: LEGORR 0x00447b30
void ledger_stub_00447ea0_EntityTypeBehaviorUpdate(
    EntitySlot* entity, float delta, const EntityTypeBehaviorHooks* hooks);

float ledger_stub_00447b30_EntityActivityTick(
    EntitySlot* entity, float delta)
{
    const EntityActivityTickHooks* hooks = NULL;
    float scaled_delta;
    float movement_delta = 0.0f;
    EntityTypeActivityTickHook tick = NULL;
    float timer_result = 0.0f;
    if (entity == NULL) return delta;
    if (hooks != NULL && hooks->delta_scale != NULL) {
        scaled_delta = hooks->delta_scale(hooks->context, entity, delta);
    }
    else {
        scaled_delta = delta;
        /* FUN_00447b30 always derives the animation delta through
         * FUN_00446610. Use its live EntitySlot/level-backed adapter when no
         * host override supplies that result. */
        ledger_stub_00446610_EntityMovementTickScaleAndDeltaGet(
            entity, delta, &movement_delta, &scaled_delta);
    }
    timer_result = entity->activity_timer;
    if ((entity->flags_3e0 & 0x4000u) == 0u) {
        if (hooks != NULL) {
            switch (entity->type) {
            case 1: tick = hooks->type1_tick; break;
            case 2: tick = hooks->type2_tick; break;
            case 3: tick = hooks->type3_tick; break;
            case 4: tick = hooks->type4_tick; break;
            default: break;
            }
        }
        if (tick != NULL) {
            timer_result = tick(hooks->context, entity, scaled_delta);
            /* FUN_00447b30 writes each type-specific worker's returned
             * timer to entity +0x2d4 before testing the finish gate. */
            entity->activity_timer = timer_result;
        }
        else if ((entity->flags_3e8 & 0x20000u) != 0u) {
            if (hooks != NULL && hooks->behavior_update != NULL)
                hooks->behavior_update(hooks->context, entity, delta);
            else
                ledger_stub_00447ea0_EntityTypeBehaviorUpdate(
                    entity, delta, NULL);
        }
        else if (entity->type == 10 && hooks != NULL &&
                 hooks->type10_tick != NULL) {
            timer_result = hooks->type10_tick(hooks->context, entity,
                                               scaled_delta);
            entity->activity_timer = timer_result;
        }
    }
    if (timer_result > 0.0f) {
        if (hooks != NULL && hooks->activity_finish != NULL)
            hooks->activity_finish(hooks->context, entity);
        else
            task_record_kind14_remove_if_head(entity);
    }
    return scaled_delta;
}

static void entity_type_behavior_cell_flag_80000_set(EntitySlot* entity,
                                                      int set)
{
    LevelState* level;
    int cell_x = -1;
    int cell_y = -1;
    if (entity == NULL || entity->owner_ctx == NULL) return;
    level = entity->owner_ctx->level;
    if (level == NULL) return;
    entity_position_to_terrain_cell(entity, level, &cell_x, &cell_y);
    level_cell_flag_80000_set(level, cell_x, cell_y, set);
}

static float entity_type_behavior_animation_advance(
    EntitySlot* entity, float delta, const EntityTypeBehaviorHooks* hooks)
{
    if (hooks != NULL && hooks->animation_tick != NULL) {
        return (float)hooks->animation_tick(hooks->context, entity, delta);
    }
    /* FUN_00475120 operates on the scene object stored at entity +0x1c.
     * EntitySlot's attached_scene_object is that runtime owner, and the
     * view module already implements the corresponding advance/remainder
     * behavior. */
    if (entity != NULL && entity->attached_scene_object != NULL) {
        return ddiscene_advance_by_time(entity->attached_scene_object, delta);
    }
    return 0.0f;
}

static void entity_type_behavior_animation_play(
    EntitySlot* entity, const EntityTypeBehaviorHooks* hooks,
    const char* animation)
{
    if (entity == NULL || animation == NULL) return;
    if (hooks != NULL && hooks->animation_play != NULL) {
        hooks->animation_play(hooks->context, entity, animation);
    }
    else if (entity->attached_scene_object != NULL) {
        /* SceneActionNameSet already follows the retail scene object's
         * action interfaces and callback slots. Keep the optional host
         * override, but let the live scene object handle named transitions
         * when no override is installed. */
        (void)scene_action_name_set(entity->attached_scene_object,
                                    animation);
    }
}

// EntityTypeBehaviorUpdate
// FUNCTION: LEGORR 0x00447ea0
void ledger_stub_00447ea0_EntityTypeBehaviorUpdate(
    EntitySlot* entity, float delta, const EntityTypeBehaviorHooks* hooks)
{
    int animation_done;
    if (entity == NULL) return;
    switch (entity->type) {
    case 5:
        if ((entity->flags_3e8 & 0x2000u) != 0u &&
            hooks != NULL && hooks->boulder_behavior != NULL)
            hooks->boulder_behavior(hooks->context, entity, delta);
        return;
    case 6:
        if (entity->attach_304 == NULL &&
            entity->attached_scene_object != NULL) {
            render_object_rotate_axis(entity->attached_scene_object, 1u,
                                      0.0f, 1.0f, 0.0f,
                                      delta * 0.01f);
        }
        return;
    case 8:
    case 13:
        if ((entity->flags_3e8 & 0x10000u) == 0u &&
            (entity->flags_3e0 & 0x80000u) != 0u) {
            entity->flags_3e0 &= ~0x80000u;
            entity->flags_3e8 |= 0x10000u;
            entity_type_behavior_animation_play(entity, hooks, "TickDown");
            /* The reference starts the named action at frame zero before it
             * updates the cell and broadcasts message 0x0d. The portable
             * action-name setter is still renderer-owned, but the scene time
             * reset and the message dispatch have direct equivalents. */
            if (entity->attached_scene_object != NULL) {
                (void)ddiscene_frame_update_and_callbacks(
                    entity->attached_scene_object, 0.0f);
            }
            entity_type_behavior_cell_flag_80000_set(entity, 1);
            message_dispatch(0x0d, 0u,
                (uint32_t)(uintptr_t)entity, NULL);
            if (hooks != NULL && hooks->terrain_activity != NULL)
                hooks->terrain_activity(hooks->context, entity);
            return;
        }
        if ((entity->flags_3e8 & 0x10000u) != 0u) {
            animation_done = entity_type_behavior_animation_advance(
                entity, delta, hooks) != 0.0f;
            if (animation_done || entity->field_340 <= 0.0f) {
                entity->flags_3e8 &= ~0x10000u;
                entity->flags_3e8 |= 0x800000u;
                entity->field_340 = -4.0f;
                entity_type_behavior_cell_flag_80000_set(entity, 0);
                if (hooks != NULL && hooks->terrain_activity != NULL)
                    hooks->terrain_activity(hooks->context, entity);
            }
            return;
        }
        (void)entity_type_behavior_animation_advance(entity, delta, hooks);
        return;
    case 11:
        if ((entity->flags_3e0 & 0x80000u) != 0u) {
            LevelState* level = entity->owner_ctx != NULL
                ? entity->owner_ctx->level : NULL;
            int cell_x = -1;
            int cell_y = -1;
            float center[2];
            float heading;
            entity->flags_3e0 &= ~0x80000u;
            register_object_at_grid_position(entity);
            if (level != NULL && level->ptr_terrain != NULL) {
                entity_position_to_terrain_cell(entity, level,
                                                &cell_x, &cell_y);
                if (terrain_cell_world_center_xy_get(level->ptr_terrain,
                        cell_x, cell_y, center)) {
                    heading = entity_vector_direction_angle_radians_get(
                        entity) + 0.7853982f - random_float_range(0.0f, 1.0f);
                    entity_world_position_set(entity, center[0], center[1],
                                              heading, 1);
                    level_cell_secondary_bit8_set(level, cell_x, cell_y, 0);
                }
            }
        }
        return;
    case 9:
        /* The reference first advances the scene action; only a nonzero
         * result permits the expand/teleport state transitions below. */
        if (entity_type_behavior_animation_advance(entity, delta, hooks) ==
            0.0f)
            return;
        if ((entity->flags_3e0 & 0x80000u) != 0u) {
            entity->flags_3e0 &= ~0x80000u;
            entity->flags_3e0 |= 0x100u;
            entity_type_behavior_animation_play(entity, hooks, "Expand");
            return;
        }
        if ((entity->flags_3e0 & 0x100u) != 0u) {
            entity->flags_3e0 &= ~0x100u;
            entity->flags_3e8 |= 0x10000u;
            /* The retail action name at DAT_004a5118 is not recovered. */
            return;
        }
        if ((entity->flags_3e8 & 0x800000u) != 0u) {
            entity->field_340 = 100.0f;
            entity->flags_3e8 &= ~0x800000u;
            entity->flags_3e0 |= 0x200000u;
            entity_type_behavior_animation_play(entity, hooks, "Teleport");
            return;
        }
        if ((entity->flags_3e0 & 0x200000u) != 0u) {
            entity->flags_3e0 &= ~0x200000u;
            entity->field_340 = -4.0f;
            entity->flags_3e8 |= 0x800000u;
        }
        return;
    case 12:
        /* FUN_00447ea0 suppresses delta while this object is in its
         * disintegration state, unless the 0x100 phase bit is set. */
        if (entity_type_behavior_animation_advance(
                entity,
                ((entity->flags_3e0 & 0x2000000u) != 0u &&
                 (entity->flags_3e0 & 0x100u) == 0u) ? 0.0f : delta,
                hooks) == 0.0f) return;
        if ((entity->flags_3e0 & 0x100u) != 0u) {
            entity->flags_3e0 &= ~0x100u;
            entity->flags_3e8 |= 0x10000u;
            entity_type_behavior_animation_play(entity, hooks, "FlapInWind");
            return;
        }
        if ((entity->flags_3e8 & 0x800000u) != 0u) {
            entity->field_340 = 100.0f;
            entity->flags_3e8 &= ~0x800000u;
            entity->flags_3e0 |= 0x200000u;
            entity_type_behavior_animation_play(entity, hooks,
                                                "Disintegrate");
            return;
        }
        if ((entity->flags_3e0 & 0x200000u) != 0u) {
            EntitySlot* owner = (EntitySlot*)entity->wake_target;
            entity->flags_3e0 &= ~0x200000u;
            entity->field_340 = -4.0f;
            entity->flags_3e8 |= 0x800000u;
            if (owner != NULL) owner->flags_3e4 &= ~0x08000000u;
        }
        return;
    case 18:
        /* The type-0x12 sequence advances first. DAT_004a50f4 is the
         * configured "Melt" action token (docs/dat/globals.csv). */
        if (entity_type_behavior_animation_advance(entity, delta, hooks) ==
            0.0f)
            return;
        if ((entity->flags_3e0 & 0x100u) != 0u) {
            entity->flags_3e0 &= ~0x100u;
            entity->flags_3e8 |= 0x10000u;
            entity_type_behavior_animation_play(entity, hooks, "Normal");
            return;
        }
        if ((entity->flags_3e8 & 0x800000u) != 0u) {
            entity->field_340 = 100.0f;
            entity->flags_3e8 &= ~0x800000u;
            entity->flags_3e0 |= 0x200000u;
            entity_type_behavior_animation_play(entity, hooks, "Melt");
            return;
        }
        if ((entity->flags_3e0 & 0x200000u) != 0u) {
            EntitySlot* owner = (EntitySlot*)entity->wake_target;
            entity->flags_3e0 &= ~0x200000u;
            entity->field_340 = -4.0f;
            entity->flags_3e8 |= 0x800000u;
            if (owner != NULL) owner->flags_3e4 &= ~0x08000000u;
        }
        return;
    case 14:
    case 16:
    case 17:
    case 19:
        (void)entity_type_behavior_animation_advance(entity, delta, hooks);
        return;
    default:
        return;
    }
}

int ledger_stub_00449880_BlockedCellApproachWeightWrite(
    EntitySlot* entity, unsigned int cell_x, unsigned int cell_y,
    float amount);

// EntityMoveToCellCoordinates
// FUNCTION: LEGORR 0x00448800
int ledger_stub_00448800_EntityMoveToCellCoordinates(
    EntitySlot* entity, const int cell[2])
{
    const uint32_t* linked_record;
    LevelState* level;
    int x;
    int y;

    if (entity == NULL || cell == NULL || entity->carry_link_308 == NULL) {
        return 0;
    }
    linked_record = (const uint32_t*)entity->carry_link_308;
    if (linked_record[0] != 8u) {
        return 0;
    }
    level = (entity->owner_ctx != NULL) ? entity->owner_ctx->level : NULL;
    x = cell[0];
    y = cell[1];
    if (!entity_move_destination_cell_usable(entity, level, x, y)) {
        return 0;
    }
    if (!ledger_stub_00449880_BlockedCellApproachWeightWrite(
            entity, (unsigned int)x, (unsigned int)y, 7.0f)) {
        return 0;
    }
    if (entity->wp_count > 0 &&
        (entity->wp_dynamic == NULL
            ? entity->wp_count <= AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count <= entity->wp_capacity)) {
        ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 9u;
    }
    entity->pos_pair_2f4 = (float)x;
    entity->pos_pair_2f8 = (float)y;
    return 1;
}

// RecordGatedStepCommandIssue
// FUNCTION: LEGORR 0x00448880
int ledger_stub_00448880_RecordGatedStepCommandIssue(
    EntitySlot* entity, const int cell[2])
{
    const uint32_t* linked_record;
    LevelState* level;
    int x;
    int y;
    int source_x;
    int source_y;
    int* path_x;
    int* path_y;
    uint32_t path_count;
    float* route_x_values;
    float* route_y_values;
    uint32_t i;

    if (entity == NULL || cell == NULL || entity->carry_link_308 == NULL) {
        return 0;
    }
    linked_record = (const uint32_t*)entity->carry_link_308;
    if (linked_record[0] != 0x0du) {
        return 0;
    }
    level = (entity->owner_ctx != NULL) ? entity->owner_ctx->level : NULL;
    x = cell[0];
    y = cell[1];
    if (level == NULL || !level_cell_flag8_check(level, x, y)) {
        return 0;
    }
    source_x = 0;
    source_y = 0;
    route_x_values = NULL;
    route_y_values = NULL;
    entity_position_to_terrain_cell(entity, level, &source_x, &source_y);
    if (source_x == x && source_y == y) {
        float same_cell_x = (float)x;
        float same_cell_y = (float)y;
        /* FUN_00448880 issues a one-cell route on the same-cell branch, then
         * returns zero without changing the step opcode or paired target. */
        (void)entity_route_setup_and_marker_associate(entity, 1u,
            &same_cell_x, &same_cell_y, NULL);
        return 0;
    }
    else {
        path_x = NULL;
        path_y = NULL;
        path_count = 0;
        if (!terrain_neighbor_path_check(level, (uint32_t)source_x,
                (uint32_t)source_y, (uint32_t)x, (uint32_t)y,
                &path_x, &path_y, &path_count, NULL, NULL)) {
            free(path_x);
            free(path_y);
            return 0;
        }
        if (path_count == 0u ||
            (size_t)path_count > ((size_t)-1) / sizeof(float)) {
            free(path_x);
            free(path_y);
            return 0;
        }
        route_x_values = (float*)malloc((size_t)path_count * sizeof(float));
        route_y_values = (float*)malloc((size_t)path_count * sizeof(float));
        if (route_x_values == NULL || route_y_values == NULL) {
            free(route_x_values);
            free(route_y_values);
            free(path_x);
            free(path_y);
            return 0;
        }
        for (i = 0; i < path_count; ++i) {
            route_x_values[i] = (float)path_x[i];
            route_y_values[i] = (float)path_y[i];
        }
        free(path_x);
        free(path_y);
    }
    if (!entity_route_setup_and_marker_associate(entity, path_count,
            route_x_values, route_y_values, 0)) {
        free(route_x_values);
        free(route_y_values);
        return 0;
    }
    free(route_x_values);
    free(route_y_values);
    if (entity->wp_count > 0)
        ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 9u;
    entity->pos_pair_2f4 = (float)x;
    entity->pos_pair_2f8 = (float)y;
    return 1;
}

// EntityNeighborCellMoveCommandHandle
static unsigned int entity_route_cardinal_direction(int source_x, int source_y,
                                                    int target_x, int target_y);
static float entity_route_weight_random(float minimum, float maximum);

/* The retail route issuers all build a one-cell path for a same-cell target,
 * otherwise use TerrainNeighborPathCheck, then pass the returned cells to
 * EntityRouteSetupAndMarkerAssociate. Keep the temporary arrays sized to the
 * actual path instead of imposing the unrelated inline waypoint limit. */
static int entity_route_live_position_to_cell(const EntitySlot* entity,
                                              LevelState* level,
                                              int* cell_x, int* cell_y)
{
    float position[2];
    if (entity == NULL || level == NULL || level->ptr_terrain == NULL ||
        cell_x == NULL || cell_y == NULL) return 0;
    entity_transform_position_get(entity, position);
    return terrain_world_xy_to_cell(level->ptr_terrain, position[0],
                                     position[1], cell_x, cell_y);
}

static int entity_route_path_install(EntitySlot* entity, LevelState* level,
                                     int target_x, int target_y,
                                     const float* marker_position)
{
    int source_x = -1;
    int source_y = -1;
    int* path_x = NULL;
    int* path_y = NULL;
    float* route_x = NULL;
    float* route_y = NULL;
    uint32_t path_count = 0;
    uint32_t i;
    int installed;

    if (entity == NULL || level == NULL) return 0;
    if (!entity_route_live_position_to_cell(entity, level, &source_x,
                                             &source_y)) return 0;
    if (source_x == target_x && source_y == target_y) {
        path_x = (int*)malloc(sizeof(int));
        path_y = (int*)malloc(sizeof(int));
        if (path_x == NULL || path_y == NULL) {
            free(path_x);
            free(path_y);
            return 0;
        }
        path_x[0] = target_x;
        path_y[0] = target_y;
        path_count = 1;
    }
    else if (!terrain_neighbor_path_check(level, (uint32_t)source_x,
            (uint32_t)source_y, (uint32_t)target_x, (uint32_t)target_y,
            &path_x, &path_y, &path_count, NULL, NULL)) {
        free(path_x);
        free(path_y);
        return 0;
    }
    if (path_count == 0u ||
        (size_t)path_count > ((size_t)-1) / sizeof(float)) {
        free(path_x);
        free(path_y);
        return 0;
    }
    route_x = (float*)malloc((size_t)path_count * sizeof(float));
    route_y = (float*)malloc((size_t)path_count * sizeof(float));
    if (route_x == NULL || route_y == NULL) {
        free(route_x);
        free(route_y);
        free(path_x);
        free(path_y);
        return 0;
    }
    for (i = 0; i < path_count; ++i) {
        route_x[i] = (float)path_x[i];
        route_y[i] = (float)path_y[i];
    }
    free(path_x);
    free(path_y);
    installed = entity_route_setup_and_marker_associate(entity, path_count,
        route_x, route_y, marker_position);
    free(route_x);
    free(route_y);
    return installed;
}

// FUNCTION: LEGORR 0x00448a60
int ledger_stub_00448a60_EntityNeighborCellMoveCommandHandle(
    EntitySlot* entity, const int target_cell[2])
{
    LevelState* level;
    AiWaypoint* step;
    float weight;
    unsigned int direction;
    int32_t random_value;
    EntitySlot* linked_entity;
    if (entity == NULL || target_cell == NULL || entity->owner_ctx == NULL ||
        entity->carry_link_308 == NULL) return 0;
    linked_entity = (EntitySlot*)entity->carry_link_308;
    if (linked_entity->type != 11) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || !terrain_cell_has_occupied_or_flagged_neighbor(
            level, target_cell[0], target_cell[1], NULL))
        return 0;
    if (!entity_route_path_install(entity, level, target_cell[0],
                                   target_cell[1], NULL) || entity->wp_count <= 0)
        return 0;
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    step->step_kind = 9u;
    if (entity->type == 1) {
        random_value = (int16_t)rng_thunk_draw15();
        if (random_value < 0) random_value = -random_value;
        direction = (unsigned int)random_value & 3u;
        weight = unported_ledger_stubs::entity_cell_step_random_range(
            0.55f, 0.75f);
        switch (direction) {
        case 0u: step->weight_x = 0.5f; step->weight_y = weight; break;
        case 1u: step->weight_y = 0.5f; step->weight_x = weight; break;
        case 2u: step->weight_x = 0.5f;
                 step->weight_y = 1.0f - weight; break;
        default: step->weight_y = 0.5f;
                 step->weight_x = 1.0f - weight; break;
        }
        step->pending = (uint8_t)(step->pending | 8u);
    }
    entity->pos_pair_2f4 = (float)target_cell[0];
    entity->pos_pair_2f8 = (float)target_cell[1];
    return 1;
}

// CardinalStepCommandIssueAndLink
int ledger_stub_00448eb0_ProximityStepCommandIssueWithJitter(
    EntitySlot* entity, EntitySlot* target, int marker);
static unsigned int entity_route_cardinal_direction(int source_x, int source_y,
                                                    int target_x, int target_y)
{
    int dx = target_x - source_x;
    int dy = target_y - source_y;
    if (abs(dx) <= abs(dy)) return dy <= 0 ? 2u : 0u;
    return dx <= 0 ? 3u : 1u;
}

static float entity_route_weight_random(float minimum, float maximum)
{
    return unported_ledger_stubs::entity_cell_step_random_range(minimum,
                                                                 maximum);
}

// FUNCTION: LEGORR 0x00448c90
int ledger_stub_00448c90_CardinalStepCommandIssueAndLink(
    EntitySlot* entity, EntitySlot* target, int link_mode)
{
    LevelState* level;
    int target_cell[2];
    int source_cell[2];
    AiWaypoint* step;
    unsigned int direction;
    if (entity == NULL || target == NULL || target->type != 4) return 0;
    if (entity->type == 2)
        return ledger_stub_00448eb0_ProximityStepCommandIssueWithJitter(
            entity, target, 1);
    if (entity->type != 1 || entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    target_cell[0] = -1;
    target_cell[1] = -1;
    if (!entity_route_live_position_to_cell(target, level, &target_cell[0],
                                             &target_cell[1])) return 0;
    source_cell[0] = -1;
    source_cell[1] = -1;
    if (!entity_route_live_position_to_cell(entity, level, &source_cell[0],
                                             &source_cell[1])) return 0;
    if (source_cell[0] < 0 || source_cell[1] < 0 ||
        !entity_route_path_install(entity, level, target_cell[0],
                                   target_cell[1], NULL) || entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    direction = entity_route_cardinal_direction(source_cell[0], source_cell[1],
                                                 target_cell[0], target_cell[1]);
    step->step_kind = 15u;
    step->pending = (uint8_t)(step->pending | 4u | direction);
    entity->link_2fc = (uintptr_t)target;
    target->route_partner_300 = entity;
    entity->command_mode_404 = link_mode;
    return 1;
}

// ProximityStepCommandIssueWithJitter
// FUNCTION: LEGORR 0x00448eb0
int ledger_stub_00448eb0_ProximityStepCommandIssueWithJitter(
    EntitySlot* entity, EntitySlot* target, int marker)
{
    LevelState* level;
    int target_cell[2];
    unsigned int candidate;
    unsigned int direction;
    float edge_weight;
    float step_fraction;
    AiWaypoint* step;
    if (entity == NULL || target == NULL || entity->type != 2 ||
        target->type != 4 || entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    target_cell[0] = -1;
    target_cell[1] = -1;
    if (!entity_route_live_position_to_cell(target, level, &target_cell[0],
                                             &target_cell[1])) return 0;
    if (target_cell[0] < 0 || target_cell[1] < 0) return 0;
    if (!ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
            entity, (unsigned int)target_cell[0],
            (unsigned int)target_cell[1], 0, &candidate, 1) ||
        entity->wp_count <= 0) return 0;
    direction = (candidate - 2u) & 3u;
    edge_weight = unported_ledger_stubs::entity_cell_step_random_range(
        0.3f, 0.9f);
    step_fraction = 4.0f / terrain_context_scale_get(level->ptr_terrain);
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    switch (direction) {
    case 0u: step->weight_y = edge_weight;
            step->weight_x = 1.0f - step_fraction; break;
    case 1u: step->weight_y = 1.0f - step_fraction;
            step->weight_x = edge_weight; break;
    case 2u: step->weight_y = edge_weight;
            step->weight_x = step_fraction; break;
    default: step->weight_y = step_fraction;
            step->weight_x = edge_weight; break;
    }
    step->step_kind = marker ? 15u : 14u;
    step->pending = (uint8_t)(step->pending | 4u | direction);
    entity->link_2fc = (uintptr_t)target;
    return 1;
}

// EntityKind6RecordApproachRouteIssue
int type11_cell_coordinate_nearest_resolve(const EntitySlot* target,
                                                 int cell_xy_out[2]);
int ledger_stub_00449880_BlockedCellApproachWeightWrite(
    EntitySlot* entity, unsigned int cell_x, unsigned int cell_y, float amount);
// FUNCTION: LEGORR 0x00449010
int ledger_stub_00449010_EntityKind6RecordApproachRouteIssue(EntitySlot* entity)
{
    int cell[2];
    EntitySlot* kind_six_record;
    AiWaypoint* step;
    if (entity == NULL || entity->type != 2 ||
        (entity->flags_3e0 & 0x400u) == 0u) return 0;
    kind_six_record = (EntitySlot*)entity->carry_link_308;
    if (kind_six_record == NULL || kind_six_record->type != 6
        || (kind_six_record->flags_3e8 & 0x80000000u) == 0u) return 0;
    if (!unported_ledger_stubs::type11_cell_coordinate_nearest_resolve(entity, cell)) return 0;
    if (!ledger_stub_00449880_BlockedCellApproachWeightWrite(
            entity, (unsigned int)cell[0], (unsigned int)cell[1], 3.0f))
        return 0;
    if (entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    step->step_kind = 18u;
    entity->pos_pair_2f4 = (float)cell[0];
    entity->pos_pair_2f8 = (float)cell[1];
    return 1;
}

// ProximityStepCommandIssueWithLink
// FUNCTION: LEGORR 0x004490a0
int ledger_stub_004490a0_ProximityStepCommandIssueWithLink(
    EntitySlot* entity, EntitySlot* target, int mode, int set_global_gate)
{
    LevelState* level;
    int target_cell[2];
    unsigned int candidate;
    unsigned int direction;
    float edge_weight;
    float step_fraction;
    AiWaypoint* step;
    if (entity == NULL || target == NULL || target->type != 4 ||
        (entity->type != 2 && entity->type != 3) ||
        entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    target_cell[0] = -1;
    target_cell[1] = -1;
    if (!entity_route_live_position_to_cell(target, level, &target_cell[0],
                                             &target_cell[1])) return 0;
    if (target_cell[0] < 0 || target_cell[1] < 0) return 0;
    if (!ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
            entity, (unsigned int)target_cell[0],
            (unsigned int)target_cell[1], 0, &candidate, entity->type != 3) ||
        entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    direction = (candidate - 2u) & 3u;
    if (entity->type == 3 && (entity->flags_3e8 & 0x80000u) == 0u)
        edge_weight = 0.5f;
    else
        edge_weight = entity_route_weight_random(0.2f, 0.8f);
    step_fraction = 4.0f / terrain_context_scale_get(level->ptr_terrain);
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    switch (direction) {
    case 0u: step->weight_y = edge_weight;
            step->weight_x = 1.0f - step_fraction; break;
    case 1u: step->weight_y = 1.0f - step_fraction;
            step->weight_x = edge_weight; break;
    case 2u: step->weight_y = edge_weight;
            step->weight_x = step_fraction; break;
    default: step->weight_y = step_fraction;
            step->weight_x = edge_weight; break;
    }
    step->step_kind = 6u;
    step->pending = (uint8_t)(step->pending | 4u | direction |
                              (mode != 0 ? 0x20u : 0u));
    entity->link_2fc = (uintptr_t)target;
    if (set_global_gate) entity->flags_3ec |= 0x00400000u;
    return 1;
}

// EntityWalkableTargetCellRouteIssue
// FUNCTION: LEGORR 0x00449240
int ledger_stub_00449240_EntityWalkableTargetCellRouteIssue(
    EntitySlot* entity, const int target_xy[2])
{
    LevelState* level;
    if (entity == NULL || target_xy == NULL || entity->owner_ctx == NULL)
        return 0;
    level = entity->owner_ctx->level;
    if (!terrain_cell_allows_placement(entity, level, target_xy[0],
                                       target_xy[1])) return 0;
    if (!ledger_stub_00449880_BlockedCellApproachWeightWrite(entity,
            (unsigned int)target_xy[0], (unsigned int)target_xy[1], 3.0f) ||
        entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 2u;
    entity->pos_pair_2f4 = (float)target_xy[0];
    entity->pos_pair_2f8 = (float)target_xy[1];
    return 1;
}

// CellStepCommandIssueWithTargetMark
// FUNCTION: LEGORR 0x004492b0
int ledger_stub_004492b0_CellStepCommandIssueWithTargetMark(
    EntitySlot* entity, const int target_xy[2])
{
    LevelState* level;
    int result;
    if (entity == NULL || target_xy == NULL || entity->owner_ctx == NULL)
        return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || level_cell_speed_class(level, target_xy[0],
                                                target_xy[1]) == 0)
        return 1;
    if ((entity_state_flags_get(entity) & 0x10000u) == 0u) return 1;
    result = entity_route_path_install(entity, level, target_xy[0],
                                       target_xy[1], NULL);
    if (!result || entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 4u;
    if (entity->type == 1) {
        level_cell_flag_80000_set(level, target_xy[0], target_xy[1], 1);
        level_cell_flag_10000000_set(level, target_xy[0], target_xy[1], 1);
    }
    return 1;
}

// EntityTargetCellRouteAndAssociate
// FUNCTION: LEGORR 0x00449430
int ledger_stub_00449430_EntityTargetCellRouteAndAssociate(
    EntitySlot* entity, EntitySlot* target)
{
    int cell[2];
    LevelState* level;
    if (entity == NULL || target == NULL || entity->owner_ctx == NULL)
        return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    cell[0] = -1;
    cell[1] = -1;
    if (!entity_route_live_position_to_cell(target, level, &cell[0],
                                             &cell[1])) return 0;
    if (cell[0] < 0 || cell[1] < 0 ||
        !entity_route_path_install(entity, level, cell[0], cell[1], NULL) ||
        entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 0x11u;
    entity->link_2fc = (uintptr_t)target;
    return 1;
}

// EntityTargetCellRouteMarker14
// FUNCTION: LEGORR 0x00449520
int ledger_stub_00449520_EntityTargetCellRouteMarker14(
    EntitySlot* entity, EntitySlot* target)
{
    int cell[2];
    LevelState* level;
    if (entity == NULL || target == NULL || entity->owner_ctx == NULL)
        return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    entity_position_to_terrain_cell(target, level, &cell[0], &cell[1]);
    return entity_route_to_cell_commit(entity, cell[0], cell[1], 0x14u);
}

// EntityCoordinateRouteAndMark
static int entity_route_to_cell_commit(EntitySlot* entity,
                                       int target_x, int target_y,
                                       unsigned int marker)
{
    LevelState* level;
    int start_x, start_y;
    int* path_x = NULL;
    int* path_y = NULL;
    uint32_t path_count = 0;
    float route_x[AI_WAYPOINTS_MAX];
    float route_y[AI_WAYPOINTS_MAX];
    uint32_t i;

    if (entity == NULL || entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || !entity_route_cell_candidate_accept(
            entity, level, target_x, target_y, 1)) return 0;
    entity_position_to_terrain_cell(entity, level, &start_x, &start_y);
    if (start_x == target_x && start_y == target_y) {
        path_x = (int*)malloc(sizeof(int));
        path_y = (int*)malloc(sizeof(int));
        if (path_x == NULL || path_y == NULL) {
            free(path_x);
            free(path_y);
            return 0;
        }
        path_x[0] = target_x;
        path_y[0] = target_y;
        path_count = 1;
    }
    else if (!terrain_neighbor_path_check(level, (uint32_t)start_x,
                                           (uint32_t)start_y,
                                           (uint32_t)target_x,
                                           (uint32_t)target_y,
                                           &path_x, &path_y,
                                           &path_count, NULL, NULL)) {
        if (abs(target_x - start_x) + abs(target_y - start_y) != 1) return 0;
        path_x = (int*)malloc(sizeof(int));
        path_y = (int*)malloc(sizeof(int));
        if (path_x == NULL || path_y == NULL) {
            free(path_x);
            free(path_y);
            return 0;
        }
        path_x[0] = target_x;
        path_y[0] = target_y;
        path_count = 1;
    }
    if (path_count == 0 || path_count > AI_WAYPOINTS_MAX) {
        free(path_x);
        free(path_y);
        return 0;
    }
    for (i = 0; i < path_count; ++i) {
        route_x[i] = (float)path_x[i];
        route_y[i] = (float)path_y[i];
    }
    free(path_x);
    free(path_y);
    if (!entity_route_setup_and_marker_associate(entity, path_count,
                                                  route_x, route_y, 0))
        return 0;
    entity->activity = (int)marker;
    entity->activity_timer = 0.0f;
    entity->pos_pair_2f4 = (float)target_x;
    entity->pos_pair_2f8 = (float)target_y;
    return 1;
}

// FUNCTION: LEGORR 0x00449610
int ledger_stub_00449610_EntityCoordinateRouteAndMark(
    EntitySlot* entity, const int target_xy[2])
{
    if (entity == NULL || target_xy == NULL || entity->owner_ctx == NULL ||
        entity->owner_ctx->level == NULL) return 0;
    if (!entity_route_path_install(entity, entity->owner_ctx->level,
                                   target_xy[0], target_xy[1], NULL) ||
        entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 0x10u;
    entity->pos_pair_2f4 = (float)target_xy[0];
    entity->pos_pair_2f8 = (float)target_xy[1];
    return 1;
}

int ledger_stub_0044a5d0_NearestOpenCellCoordSearch(
    EntitySlot* entity, int* output_x, int* output_y,
    int require_minimum_distance, int allow_maskc0, int allow_flag20);
int ledger_stub_0044a4f0_NearestCellCoordinateWorldResolve(
    const EntitySlot* target, int cell_xy_out[2]);
int ledger_stub_00449880_BlockedCellApproachWeightWrite(
    EntitySlot* entity, unsigned int cell_x, unsigned int cell_y,
    float amount);

// RouteStepCommandIssue
// FUNCTION: LEGORR 0x00449700
int ledger_stub_00449700_RouteStepCommandIssue(EntitySlot* entity)
{
    LevelState* level;
    int destination[2];
    int found;
    if (entity == NULL || entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    if ((entity->flags_3e8 & 0x00100000u) != 0u) {
        found = ledger_stub_0044a4f0_NearestCellCoordinateWorldResolve(
            entity, destination);
        if (!found || !entity_route_path_install(entity, level,
                destination[0], destination[1], NULL) || entity->wp_count <= 0 ||
            (entity->wp_dynamic == NULL
                ? entity->wp_count > AI_WAYPOINTS_MAX
                : (unsigned int)entity->wp_count > entity->wp_capacity))
            return 0;
        ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 10u;
    }
    else {
        found = ledger_stub_0044a5d0_NearestOpenCellCoordSearch(
            entity, &destination[0], &destination[1], 1, 0, 1);
        if (!found || !ledger_stub_00449880_BlockedCellApproachWeightWrite(
                entity, (unsigned int)destination[0],
                (unsigned int)destination[1], 3.0f) ||
            entity->wp_count <= 0 ||
            (entity->wp_dynamic == NULL
                ? entity->wp_count > AI_WAYPOINTS_MAX
                : (unsigned int)entity->wp_count > entity->wp_capacity))
            return 0;
        ENTITY_WAYPOINTS(entity)[entity->wp_count - 1].step_kind = 10u;
    }
    entity->pos_pair_2f4 = (float)destination[0];
    entity->pos_pair_2f8 = (float)destination[1];
    argument_callback_dispatch(entity);
    return 1;
}

// BlockedCellApproachWeightWrite
// FUNCTION: LEGORR 0x00449880
int ledger_stub_00449880_BlockedCellApproachWeightWrite(
    EntitySlot* entity, unsigned int cell_x, unsigned int cell_y, float amount)
{
    LevelState* level;
    unsigned int direction;
    float scaled;
    AiWaypoint* step;
    if (entity == NULL || entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || level->ptr_terrain == NULL ||
        !level_cell_walkable_flag(level, (int)cell_x, (int)cell_y)) return 0;
    if (!ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
            entity, cell_x, cell_y, 0, &direction, 1)) return 0;
    if (entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    scaled = amount / terrain_context_scale_get(level->ptr_terrain);
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    step->weight_x = 0.5f;
    step->weight_y = 0.5f;
    switch (direction & 3u) {
    case 0u: step->weight_x = scaled; break;
    case 1u: step->weight_y = scaled; break;
    case 2u: step->weight_x = 1.0f - scaled; break;
    case 3u: step->weight_y = 1.0f - scaled; break;
    }
    step->pending = (uint8_t)(step->pending | 8u);
    return 1;
}

// ProximityStepCommandIssueClearingGate
// FUNCTION: LEGORR 0x00449ac0
int ledger_stub_00449ac0_ProximityStepCommandIssueClearingGate(
    EntitySlot* entity, EntitySlot* target)
{
    LevelState* level;
    int target_cell[2];
    unsigned int direction;
    float step_fraction;
    AiWaypoint* step;
    if (entity == NULL || target == NULL || entity->type != 2 ||
        target->type != 4 || entity->owner_ctx == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    target_cell[0] = -1;
    target_cell[1] = -1;
    if (!entity_route_live_position_to_cell(target, level, &target_cell[0],
                                             &target_cell[1])) return 0;
    if (!ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
            entity, (unsigned int)target_cell[0],
            (unsigned int)target_cell[1], 0, &direction, 1)) {
        entity->flags_3e4 &= ~0x00000800u;
        return 0;
    }
    if (entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    step_fraction = 4.0f / terrain_context_scale_get(level->ptr_terrain);
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    switch (direction & 3u) {
    case 0u: step->weight_y = 0.5f;
             step->weight_x = 1.0f - step_fraction; break;
    case 1u: step->weight_y = 1.0f - step_fraction;
             step->weight_x = 0.5f; break;
    case 2u: step->weight_y = 0.5f;
             step->weight_x = step_fraction; break;
    default: step->weight_y = step_fraction;
             step->weight_x = 0.5f; break;
    }
    step->step_kind = 0x0bu;
    step->pending = (uint8_t)(step->pending | 4u | (direction & 3u));
    entity->link_2fc = (uintptr_t)target;
    return 1;
}

// FUNCTION: LEGORR 0x00449cd0
void ledger_stub_00449cd0_SceneObjectStateHandleApplyValue(
    void* entity_record, int set)
{
    void* component = entity_type_component_get(entity_record);
    if (component != NULL) entity_flag_80_set(component, set);
}

// EntityTypeOperationDispatch
// FUNCTION: LEGORR 0x00449f50
void ledger_stub_00449f50_EntityTypeOperationDispatch(
    EntitySlot* entity, uint32_t operation)
{
    if (entity == NULL) return;
    switch (entity->type) {
    case 1:
        if (entity->attached_direction_record_type1 != NULL)
            scene_resource_visibility_set(
                entity->attached_direction_record_type1, operation);
        return;
    case 2:
        if (entity->attached_direction_record_type2 != NULL)
            animation_object_visibility_set(
                entity->attached_direction_record_type2, operation);
        return;
    case 3:
        if (entity->attached_direction_record_type3 != NULL)
            animation_object_visibility_set(
                entity->attached_direction_record_type3, operation);
        return;
    case 4:
        if (entity->attached_direction_record_type4 != NULL)
            scene_object_pair_apply_mode(
                entity->attached_direction_record_type4, operation);
        return;
    case 10:
        if (entity->attached_direction_record_type10 != NULL)
            animation_object_visibility_set(
                entity->attached_direction_record_type10, operation);
        return;
    default:
        if ((entity->flags_3e8 & 0x20000u) != 0u &&
            entity->type_operation_fallback_1c != NULL)
            scene_render_lock_toggle(entity->type_operation_fallback_1c,
                                     (int)operation);
        return;
    }
}

// EntityTypeOperationRelease
uint32_t ledger_stub_00449ff0_EntityTypeOperationRelease(EntitySlot* entity)
{
    return ::entity_type_operation_release(entity);
}

/* EntityWorkObjectPositionGet (FUN_0044a2a0) resolves the type-specific
 * work-object record before asking the scene layer for its position. The
 * sidecar record pointers correspond to the retail entity fields +0x0c,
 * +0x10, and +0x14 used by types 1, 2, and 3. */
// FUNCTION: LEGORR 0x0044a2a0
} // namespace unported_ledger_stubs

/* EntityDirectionalActivityStart (FUN_00447950) chooses an activity from the
 * facing vector's 45-degree cone, applies the activity transition and retry,
 * then runs a one-second tick. Retail reads the direction from entity +0x2ac
 * and +0x2b0 (the attached_direction sidecar). */
// FUNCTION: LEGORR 0x00447950
void entity_directional_activity_start(EntitySlot* entity,
                                       const float direction[2], int mode)
{
    EntitySlot* target;
    float forward_x, forward_y, length, dot, cross;
    int activity;
    int update_result;

    if (entity == NULL) return;
    target = entity;
    if ((entity->flags_3e4 & 2u) != 0u && entity->pair_link_3c0 != 0u)
        target = (EntitySlot*)entity->pair_link_3c0;
    if ((target->flags_3e0 & 0x1000u) != 0u ||
        !entity_shared_eligibility_check(target, 1)) return;

    if (direction == NULL) {
        /* The retail branch computes 0x24 for mode 0 and 0x1b for
         * nonzero mode. Finding 248's 0x25 label conflicts with the
         * shipped PE bytes (NEG/SBB, AND 0xfffffff7, ADD 0x24). */
        activity = mode != 0 ? 0x1b : 0x24;
    }
    else {
        forward_x = target->attached_direction[0];
        forward_y = target->attached_direction[1];
        length = (float)sqrt((double)(forward_x * forward_x +
                                      forward_y * forward_y));
        if (length == 0.0f) return;
        forward_x /= length;
        forward_y /= length;
        dot = forward_y * direction[1] + forward_x * direction[0];
        if (dot <= 0.2928932f) {
            if (dot >= -0.2928932f) {
                cross = forward_y * direction[0] -
                        forward_x * direction[1];
                activity = cross <= 0.0f ? 0x17 : 0x18;
            }
            else {
                activity = 0x19;
            }
        }
        else {
            activity = 0x1a;
        }
    }

    entity_state_reset(target, 0, 1);
    ai_set_activity(target, activity, 1);
    update_result = activity_register_bookkeeping(target);
    if (update_result == 0) {
        ai_set_activity(target, 0, 1);
        (void)activity_register_bookkeeping(target);
    }
    /* The retail tick dispatches through type-specific DDiScene methods.
     * The portable tick API needs those engine-owned callbacks; with no
     * installed tick ops it safely returns the supplied delta unchanged. */
    (void)unported_ledger_stubs::ledger_stub_00447b30_EntityActivityTick(
        target, 1.0f);
    target->flags_3e0 |= 0x1000u;
    if (target->type == 3) target->flags_3e8 &= 0x7fffffffu;
}

/* EntityTypeOperationRelease (FUN_00449ff0) returns the type-specific low
 * flag bit used by the outline caller despite its decompiler void label.
 * Kinds 2/3 own their renderer scene in the activity sidecar when the legacy
 * animation-record wrapper is unavailable. */
// FUNCTION: LEGORR 0x00449FF0
uint32_t entity_type_operation_release(const EntitySlot* entity)
{
    if (entity == NULL) return 0u;
    switch (entity->type) {
    case 1:
        return entity->attached_direction_record_type1 != NULL
            ? entity_type_flag_bit1_get(
                entity->attached_direction_record_type1) : 0u;
    case 2:
        if (entity->attached_direction_record_type2 != NULL)
            return animation_object_visibility_get(
                entity->attached_direction_record_type2);
        return entity->activity_runner_type2.child_scene != NULL
            ? scene_object_flag_bit3_get(
                entity->activity_runner_type2.child_scene) : 0u;
    case 3:
        if (entity->attached_direction_record_type3 != NULL)
            return animation_object_visibility_get(
                entity->attached_direction_record_type3);
        return entity->activity_runner_type3.child_scene != NULL
            ? scene_object_flag_bit3_get(
                entity->activity_runner_type3.child_scene) : 0u;
    case 4:
        return entity->attached_direction_record_type4 != NULL
            ? animation_object_visibility_get(
                entity->attached_direction_record_type4) : 0u;
    case 10:
        return entity->attached_direction_record_type10 != NULL
            ? animation_object_visibility_get(
                entity->attached_direction_record_type10) : 0u;
    default:
        if ((entity->flags_3e8 & 0x20000u) == 0u ||
            entity->type_operation_fallback_1c == NULL) return 0u;
        return scene_object_flag_bit3_get(
            entity->type_operation_fallback_1c);
    }
}

void entity_type_operation_fallback_handle_set(EntitySlot* entity,
                                               void* handle_1c)
{
    if (entity != NULL) entity->type_operation_fallback_1c = handle_1c;
}

/* EntityFollowTargetPoseGet (FUN_0044a070): resolve the indexed scene handle
 * for entity kinds 1-3, read its world position and facing, normalize the
 * facing vector, and reverse it when the type-specific component gate is 1. */
// FUNCTION: LEGORR 0x0044a070
void entity_follow_target_pose_get(EntitySlot* entity, int index,
                                   float position[3], float facing[3])
{
    void* scene_object = NULL;
    uint32_t component_present = 0u;
    float secondary_facing[3];
    float inverse_length;
    if (entity == NULL || position == NULL || facing == NULL) return;
    if (entity->type == 1) {
        scene_object = entity_type1_scene_resource_indexed_get(
            entity, (uint32_t)index, &component_present);
    }
    else if (entity->type == 2 || entity->type == 3) {
        EntityActivityRunnerSidecar* runner;
        scene_object = entity_activity_runner_indexed_get(
            entity, entity->type, (uint32_t)index);
        runner = entity->type == 2 ? &entity->activity_runner_type2
                                   : &entity->activity_runner_type3;
        component_present = runner->component_gate_value == 1u ? 1u : 0u;
    }
    else {
        return;
    }
    if (scene_object == NULL) return;
    (void)ddiscene_object_position_get(scene_object, NULL, position);
    (void)ddiscene_object_facing_pair_get(scene_object, NULL, facing,
                                         secondary_facing);
    inverse_length = 1.0f / sqrtf(facing[0] * facing[0] +
                                 facing[1] * facing[1] +
                                 facing[2] * facing[2]);
    facing[0] = inverse_length * facing[0];
    facing[1] = inverse_length * facing[1];
    facing[2] = inverse_length * facing[2];
    if (component_present != 0u) {
        facing[0] = -facing[0];
        facing[1] = -facing[1];
        facing[2] = -facing[2];
    }
}

int entity_work_object_position_get(const EntitySlot* entity,
                                    float out_xy[2])
{
    void* work_object = NULL;
    float position[3];
    if (entity == NULL || out_xy == NULL) return 0;
    switch (entity->type) {
    case 1:
        if (entity->attached_direction_record_type1 != NULL) {
            work_object = scene_resource_cached_slot_b4_get(
                (SceneResourceAnimationRecord*)
                    entity->attached_direction_record_type1);
        }
        break;
    case 2:
        if (entity->attached_direction_record_type2 != NULL) {
            work_object = renderer_cached_handle_40_get_or_create(
                entity->attached_direction_record_type2);
        }
        break;
    case 3:
        if (entity->attached_direction_record_type3 != NULL) {
            work_object = renderer_cached_handle_40_get_or_create(
                entity->attached_direction_record_type3);
        }
        break;
    default:
        return 0;
    }
    if (work_object == NULL ||
        !ddiscene_object_position_get(work_object, NULL, position)) return 0;
    out_xy[0] = position[0];
    out_xy[1] = position[1];
    return 1;
}

namespace unported_ledger_stubs {

int ledger_stub_0044a2a0_EntityWorkObjectPositionGet(
    const EntitySlot* entity, float position[2])
{
    return ::entity_work_object_position_get(entity, position);
}

// ObjectMovementInputAccumulate
// FUNCTION: LEGORR 0x0044a310
void ledger_stub_0044a310_ObjectMovementInputAccumulate(EntitySlot* object,
    int input_y, int input_x, float input_z)
{
    unsigned char* bytes = (unsigned char*)object;
    int type;
    uint32_t capability;
    unsigned char flags;
    int* accumulators;
    float* accumulator_z;
    void* linked;
    memcpy(&type, bytes, sizeof(type));
    if (type == ENTITY_TYPE_MINIFIG) {
        memcpy(&linked, bytes + 0x36c, sizeof(linked));
        if (linked != NULL) {
            bytes = (unsigned char*)linked;
        }
    }
    flags = bytes[0x3e0];
    if ((flags & 1u) != 0)
        entity_action_sequence_reset((EntitySlot*)bytes, 0);
    capability = entity_capability_flags_get((EntitySlot*)bytes);
    if ((capability & 4u) == 0) input_x = 0;
    accumulators = (int*)(bytes + 0x2bc);
    accumulator_z = (float*)(bytes + 0x2c4);
    *accumulator_z = input_z + *accumulator_z;
    accumulators[0] += input_x;
    accumulators[1] += input_y;
}

// NearestCellCoordinateWorldResolve
// FUNCTION: LEGORR 0x0044a4f0
int ledger_stub_0044a4f0_NearestCellCoordinateWorldResolve(
    const EntitySlot* target, int cell_xy_out[2])
{
    float target_xy[2];
    float best_distance = 10000.0f;
    const int* coordinate = &s_predug_coordinates[0][0];
    unsigned int i;
    unsigned int best_index = 0xffffffffu;
    unsigned int count = s_predug_coordinate_count;
    if (target == NULL) return 0;
    entity_transform_position_get_outputs(target, &target_xy[0],
                                          &target_xy[1]);
    if (count != 0u) {
        i = 0u;
        do {
            float world_x;
            float world_y;
            float dx;
            float dy;
            float distance;
            if (map_cell_to_world_center(level_current_terrain_get(
                    coordinate[0], coordinate[1]),
                    (uint32_t)coordinate[0], (uint32_t)coordinate[1],
                    &world_x, &world_y)) {
                dy = target_xy[1] - world_y;
                dx = target_xy[0] - world_x;
                distance = sqrtf(dy * dy + dx * dx);
                if (distance < best_distance) {
                    best_distance = distance;
                    best_index = i;
                }
            }
            ++i;
            coordinate += 2;
        } while (i < count);
    }
    if (best_index == 0xffffffffu) return 0;
    if (cell_xy_out != NULL) {
        cell_xy_out[0] = s_predug_coordinates[best_index][0];
        cell_xy_out[1] = s_predug_coordinates[best_index][1];
    }
    return 1;
}

typedef struct NearestOpenCellCandidate {
    int x;
    int y;
    float distance;
} NearestOpenCellCandidate;

static int nearest_open_cell_candidate_compare(const void* first,
                                               const void* second)
{
    const NearestOpenCellCandidate* a =
        (const NearestOpenCellCandidate*)first;
    const NearestOpenCellCandidate* b =
        (const NearestOpenCellCandidate*)second;
    if (a->distance < b->distance) return -1;
    if (a->distance > b->distance) return 1;
    return 0;
}

// NearestOpenCellCoordSearch
// FUNCTION: LEGORR 0x0044a5d0
int ledger_stub_0044a5d0_NearestOpenCellCoordSearch(
    EntitySlot* entity, int* output_x, int* output_y,
    int require_minimum_distance, int allow_maskc0, int allow_flag20)
{
    NearestOpenCellCandidate candidates[100];
    LevelState* level;
    float entity_position[2];
    int center_x = 0, center_y = 0;
    int x, y;
    unsigned int count = 0u;
    float minimum_distance;
    if (entity == NULL || entity->owner_ctx == NULL ||
        output_x == NULL || output_y == NULL) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || level->ptr_terrain == NULL) return 0;
    entity_transform_position_get(entity, entity_position);
    if (!entity_route_live_position_to_cell(entity, level, &center_x,
                                            &center_y)) return 0;
    minimum_distance = terrain_context_scale_get(level->ptr_terrain);
    for (y = center_y - 10; y < center_y + 10; ++y) {
        for (x = center_x - 10; x < center_x + 10; ++x) {
            float cell_position[2];
            float dx, dy, distance;
            if (!level_cell_walkable_flag(level, x, y) ||
                (allow_flag20 == 0 &&
                 level_cell_flag_20_bounded(level, x, y) != 0u) ||
                (allow_maskc0 == 0 && level_cell_mask_c0(level, x, y) != 0u)) {
                continue;
            }
            if (!terrain_cell_world_center_xy_get(level->ptr_terrain,
                                                  x, y, cell_position)) {
                continue;
            }
            dx = cell_position[0] - entity_position[0];
            dy = cell_position[1] - entity_position[1];
            distance = sqrtf(dx * dx + dy * dy);
            if (require_minimum_distance != 0 &&
                minimum_distance > distance) continue;
            if (count < 100u) {
                candidates[count].x = x;
                candidates[count].y = y;
                candidates[count].distance = distance;
                ++count;
            }
        }
    }
    if (count == 0u) return 0;
    crt_array_sort_with_comparator(candidates, (size_t)count,
                                  sizeof(candidates[0]),
                                  nearest_open_cell_candidate_compare);
    *output_x = candidates[0].x;
    *output_y = candidates[0].y;
    return 1;
}

// PolyVariantSectionModelBuild
typedef int (*PolyVariantSectionBuildHook)(void* context, void* section,
                                           uint32_t variant);
// FUNCTION: LEGORR 0x0044a7d0
int ledger_stub_0044a7d0_PolyVariantSectionModelBuild(
    void* section, uint32_t variant_count, PolyVariantSectionBuildHook build,
    void* context)
{
    uint32_t variant;
    if (section == NULL || build == NULL) return 0;
    for (variant = 0u; variant < variant_count; ++variant)
        if (!build(context, section, variant)) return 0;
    return 1;
}

// BuildingPlacementPreviewUpdate
static BuildingPlacementPreviewHooks s_building_placement_preview_hooks;
static int s_building_placement_preview_owner_bound;
// FUNCTION: LEGORR 0x0044a910
int ledger_stub_0044a910_BuildingPlacementPreviewUpdate(
    const BuildingPlacementPreviewHooks* hooks)
{
    float position[3];
    uint32_t record_key = 0u;
    uint32_t direction = 0u;
    uint32_t footprint_count = 0u;
    const int* footprint_xy = NULL;
    TerrainSurfaceGrid* surface_grid = NULL;
    int cell_xy[2] = { 0, 0 };
    int emit_boundary_commands = 0;
    int valid;
    if (hooks == NULL || hooks->position_get == NULL ||
        hooks->placement_check == NULL || hooks->preview_set == NULL ||
        !hooks->position_get(hooks->context, position)) return 0;
    valid = hooks->placement_check(hooks->context, position);
    if (valid && hooks->path_selection_get != NULL &&
        hooks->path_selection_get(hooks->context, position, &record_key,
                                  cell_xy, &direction, &footprint_xy,
                                  &footprint_count, &surface_grid,
                                  &emit_boundary_commands) &&
        (footprint_count == 0u || footprint_xy != NULL)) {
        path_record_surface_grid_bind(surface_grid);
        (void)ledger_stub_00409790_CellPathRecordRegisterApply(
            record_key, cell_xy, direction, footprint_xy, footprint_count);
        if (emit_boundary_commands) {
            (void)ledger_stub_00408ec0_KindNineNeighborEmitForFootprintCells(
                cell_xy, footprint_xy, footprint_count);
        }
    }
    hooks->preview_set(hooks->context, position, valid);
    return valid;
}

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

// EntityAttachedSpawnRecordProcess
// FUNCTION: LEGORR 0x0044af90
void ledger_stub_0044af90_EntityAttachedSpawnRecordProcess(EntitySlot* owner)
{
    if (owner != NULL && owner->carry_ref_348 != NULL) {
        const uint32_t count = *(const uint32_t*)owner->carry_ref_348;
        lego::random_kind_six_scatter_spawn(owner, (int)count, 0);
    }
}

// ConfiguredTinyMonsterSpawnAndTeardown
// FUNCTION: LEGORR 0x0044afb0
void ledger_stub_0044afb0_ConfiguredTinyMonsterSpawnAndTeardown(
    EntitySlot* source)
{
    static const char* const configured_names[3] = {
        "RockMonster", "Lavamonster", "Icemonster"
    };
    int resolved_type, resolved_row, source_type, source_row;
    int selected = -1, i;
    const char* replacement;
    float position[2];
    EntitySlot* monster = NULL;
    unsigned int successes = 0u;
    if (source == NULL || source->owner_ctx == NULL) return;
    if (source->type_name[0] == '\0' ||
        !entity_type_row_for_name(source->type_name, &source_type,
                                  &source_row)) return;
    for (i = 0; i < 3; ++i) {
        if (entity_type_row_for_name(configured_names[i], &resolved_type,
                                     &resolved_row) &&
            source_type == resolved_type && source_row == resolved_row) {
            selected = i;
            break;
        }
    }
    if (selected < 0) return;
    replacement = selected == 2 ? "TinyIM" : "TinyRM";
    if (!entity_type_row_for_name(replacement, &resolved_type, &resolved_row))
        return;
    entity_transform_position_get(source, position);
    while (successes < 3u) {
        monster = entity_create_with_position(
            source->owner_ctx, resolved_type, replacement, 0,
            position[0], position[1],
            (float)((int)(int16_t)level_rand_u16() % 7));
        if (monster == NULL) return;
        monster->field_340 = -1.0f;
        if (!ledger_stub_00449700_RouteStepCommandIssue(monster)) break;
        ++successes;
        source->flags_3e4 |= 0x20u;
    }
    if (successes == 3u) return;
    if (monster != NULL) (void)entity_teardown(monster);
}

// SmallSpiderCellSwarmSpawn
// FUNCTION: LEGORR 0x0044b140
void ledger_stub_0044b140_SmallSpiderCellSwarmSpawn(
    int cell_x, int cell_y, uint32_t variation)
{
    int type, row, i, count = (int)(variation & 3u) + 2;
    float vertices[12];
    GameContext* context = entity_pool_callback_context_get();
    TerrainSurfaceGrid* grid = path_record_surface_grid_get();
    if (context == NULL || context->level == NULL || grid == NULL ||
        cell_x < 0 || cell_y < 0 ||
        (uint32_t)cell_x >= grid->width ||
        (uint32_t)cell_y >= grid->height || grid->surface_vectors == NULL ||
        !entity_type_row_for_name("SmallSpider", &type, &row)) return;
    /* This target only scatters after the cell surface has been resolved. */
    (void)terrain_cell_surface_vertices_get(grid, (uint32_t)cell_x,
                                            (uint32_t)cell_y, vertices);
    (void)row;
    for (i = 0; i < count; ++i) {
        int rotation = (int)(int16_t)level_rand_u16() % 7;
        float x = random_float_range(vertices[0] + 1.0f,
                                     vertices[6] - 1.0f);
        float y = random_float_range(vertices[1] - 1.0f,
                                     vertices[7] + 1.0f);
        EntitySlot* spider = entity_create_with_position(
            context, type, "SmallSpider", 0, x, y, (float)rotation);
        if (spider != NULL) spider->flags_3e0 |= 0x10000u;
    }
}

// PairingActivityRaiseAndMutualLink
// FUNCTION: LEGORR 0x0044b250
void ledger_stub_0044b250_PairingActivityRaiseAndMutualLink(
    EntitySlot* first, EntitySlot* second)
{
    if (first == NULL || second == NULL ||
        !entity_shared_eligibility_check(second, 1) ||
        (second->flags_3e4 & 0x100u) != 0u ||
        (second->flags_3e4 & 2u) != 0u ||
        (first->flags_3e4 & 1u) != 0u ||
        entity_flags_mask107c33de_or_1c00203_any_test(first) ||
        !(entity_stats_float_ate8(first) < first->field_340)) return;
    lego::pairing_activity_prepare(second);
    entity_state_reset(second, 0, 1);
    entity_state_reset(first, 0, 1);
    ai_set_activity(first, 0x25, 1);
    ai_idle_chain_record_attach(first);
    ai_set_activity(second, 0x26, 1);
    ai_idle_chain_record_attach(second);
    first->flags_3e4 |= 1u;
    second->flags_3e4 |= 2u;
    first->pair_link_3c0 = (uintptr_t)second;
    second->pair_link_3c0 = (uintptr_t)first;
}

// TypeTwoEntityTargetResolveAndApply
// FUNCTION: LEGORR 0x0044b4c0
int ledger_stub_0044b4c0_TypeTwoEntityTargetResolveAndApply(
    EntitySlot* entity, uint32_t code)
{
    int group;
    /* The reference treats an unmapped code, and every non-minifigure,
     * permissively. A mapped code is a permitted-activity group id. */
    if (entity == NULL || entity->type != 2) return 1;
    if (!entity_grouped_code_table_find_value_index(code, &group)) return 1;
    return ai_activity_scan(entity, group);
}

static EntityObjectPositionSceneResolveHook
    s_entity_object_position_scene_resolve_hook;
static void* s_entity_object_position_scene_resolve_context;

// ObjectPositionStepCommandIssueAndLink
// FUNCTION: LEGORR 0x0044b500
int ledger_stub_0044b500_ObjectPositionStepCommandIssueAndLink(
    EntitySlot* entity, uint32_t selector)
{
    float entity_position[2];
    float target_position[3];
    int target_xy[2] = { -1, -1 };
    EntitySlot* spatial_candidate;
    void* scene_object;
    LevelState* level;
    AiWaypoint* step;
    if (entity == NULL || entity->type != 2 || entity->owner_ctx == NULL)
        return 0;
    level = entity->owner_ctx->level;
    if (level == NULL || level->ptr_terrain == NULL ||
        s_entity_object_position_scene_resolve_hook == NULL) return 0;
    entity_transform_position_get(entity, entity_position);
    spatial_candidate = entity_lookup_at_world_position(entity_position);
    if (spatial_candidate == NULL) return 0;
    scene_object = s_entity_object_position_scene_resolve_hook(
        s_entity_object_position_scene_resolve_context,
        spatial_candidate, selector);
    if (scene_object == NULL ||
        !ddiscene_object_position_get(scene_object, NULL, target_position) ||
        !terrain_world_xy_to_cell(level->ptr_terrain, target_position[0],
            target_position[1], &target_xy[0], &target_xy[1]) ||
        !entity_route_path_install(entity, level, target_xy[0], target_xy[1],
                                   target_position) || entity->wp_count <= 0 ||
        (entity->wp_dynamic == NULL
            ? entity->wp_count > AI_WAYPOINTS_MAX
            : (unsigned int)entity->wp_count > entity->wp_capacity)) return 0;
    entity->link_2fc = (uintptr_t)spatial_candidate;
    step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
    step->step_kind = 0x0du;
    return 1;
}

void entity_object_position_scene_resolve_hook_set(
    EntityObjectPositionSceneResolveHook hook, void* context)
{
    s_entity_object_position_scene_resolve_hook = hook;
    s_entity_object_position_scene_resolve_context = context;
}

// EntitySceneObjectTreeAttach
typedef int (*EntitySceneTreeAttachHook)(void* context, EntitySlot* entity,
                                        void* scene_object, void* parent);
// FUNCTION: LEGORR 0x0044b680
int ledger_stub_0044b680_EntitySceneObjectTreeAttach(
    EntitySlot* entity, void* scene_object, void* parent,
    EntitySceneTreeAttachHook attach, void* context)
{
    if (entity == NULL || scene_object == NULL || attach == NULL) return 0;
    return attach(context, entity, scene_object, parent);
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

// FUNCTION: LEGORR 0x0044c1b0
void ledger_stub_0044c1b0_EntityHeadingStepMove(EntitySlot* entity,
                                                 float delta)
{
    LevelState* level;
    float position[2];
    float target[2];
    float direction_x;
    float direction_y;
    float step_x;
    float step_y;
    float magnitude;
    int cell_x;
    int cell_y;
    int allowed;
    int transition;

    if (entity == NULL || entity->owner_ctx == NULL ||
        entity->owner_ctx->level == NULL ||
        entity->owner_ctx->level->ptr_terrain == NULL) return;
    level = entity->owner_ctx->level;
    entity_transform_position_get(entity, position);
    memcpy(&direction_x, &entity->task_pair_3b4[0], sizeof(direction_x));
    memcpy(&direction_y, &entity->task_pair_3b4[1], sizeof(direction_y));
    step_x = direction_x * delta;
    step_y = direction_y * delta;
    target[0] = position[0] + step_x;
    target[1] = position[1] + step_y;
    if (!terrain_world_xy_to_cell(level->ptr_terrain, target[0], target[1],
                                  &cell_x, &cell_y)) {
        allowed = 0;
    }
    else {
        allowed = terrain_cell_route_and_occupancy_check(entity, level,
            cell_x, cell_y, cell_x, cell_y, 1);
    }

    if (allowed) {
        transition = entity_flag_gated_callback_dispatch(entity,
            (uint32_t)(uintptr_t)target, 0u, 0u);
        if (transition == 0) {
            entity_world_position_set(entity, target[0], target[1], 0.0f, 0);
        }
        else {
            allowed = 0;
        }
    }
    if (!allowed) {
        magnitude = (float)sqrt((double)(direction_x * direction_x +
                                          direction_y * direction_y));
        if (magnitude != 0.0f) {
            float normalized_x = direction_x / magnitude;
            float normalized_y = direction_y / magnitude;
            if (normalized_x > 0.5f) {
                normalized_x = 1.0f;
                normalized_y = 0.0f;
            }
            else if (normalized_x < -0.5f) {
                normalized_x = -1.0f;
                normalized_y = 0.0f;
            }
            else if (normalized_y > 0.5f) {
                normalized_x = 0.0f;
                normalized_y = 1.0f;
            }
            else if (normalized_y < -0.5f) {
                normalized_x = 0.0f;
                normalized_y = -1.0f;
            }
            direction_x = normalized_x * magnitude;
            direction_y = normalized_y * magnitude;
            memcpy(&entity->task_pair_3b4[0], &direction_x,
                   sizeof(direction_x));
            memcpy(&entity->task_pair_3b4[1], &direction_y,
                   sizeof(direction_y));
        }
        step_x = direction_x * delta;
        step_y = direction_y * delta;
        target[0] = position[0] + step_x;
        target[1] = position[1] + step_y;
        if (terrain_world_xy_to_cell(level->ptr_terrain, target[0], target[1],
                                     &cell_x, &cell_y)) {
            allowed = terrain_cell_route_and_occupancy_check(entity, level,
                cell_x, cell_y, cell_x, cell_y, 1);
        }
        else {
            allowed = 0;
        }
        if (allowed) {
            transition = entity_flag_gated_callback_dispatch(entity,
                (uint32_t)(uintptr_t)target, 0u, 0u);
            if (transition == 0) {
                entity_world_position_set(entity, target[0], target[1],
                                          0.0f, 0);
            }
            else {
                entity->flags_3e4 &= ~0x4000u;
            }
        }
        else {
            entity->flags_3e4 &= ~0x4000u;
        }
    }
    {
        float remaining;
        memcpy(&remaining, &entity->task_scalar_3bc, sizeof(remaining));
        if (remaining <= 0.0f) entity->flags_3e4 &= ~0x4000u;
        remaining -= (float)sqrt((double)(step_x * step_x + step_y * step_y));
        memcpy(&entity->task_scalar_3bc, &remaining, sizeof(remaining));
    }
}

// DriftOffsetStepApply
void ledger_stub_0044d0d0_DriftOffsetStepApply(float delta)
{
    motion_drift_offset_step(delta);
}

// KeyValueEqualsFileLoad
// FUNCTION: LEGORR 0x0044db50
int ledger_stub_0044db50_KeyValueEqualsFileLoad(
    const MotionKeyValueLoadRequest* request, MotionKeyValueTable* table,
    const MotionKeyValueLoadHooks* hooks)
{
    char line[256];
    void* profile = NULL;
    uint32_t i;
    if (request == NULL || table == NULL || hooks == NULL) return 0;
    memset(table, 0, sizeof(*table));
    if (hooks->string_resolve != NULL) {
        table->name_a = hooks->string_resolve(hooks->context,
                                              request->name_a_id);
        table->name_b = hooks->string_resolve(hooks->context,
                                              request->name_b_id);
        if (request->optional_name_id != 0)
            table->optional_name = hooks->string_resolve(
                hooks->context, request->optional_name_id);
        if (request->text_resource_id != 0)
            table->text_resource = hooks->string_resolve(
                hooks->context, request->text_resource_id);
    }
    for (i = 0; i < 4u; ++i) table->values[i] = request->values[i];
    if (hooks->profile_open != NULL)
        profile = hooks->profile_open(hooks->context, request->config,
                                      request->profile_id);
    for (i = 0; i < MOTION_KEY_VALUE_ENTRY_CAPACITY; ++i) {
        char* key = NULL;
        char* value = NULL;
        if (profile == NULL || hooks->profile_line_get == NULL ||
            !hooks->profile_line_get(hooks->context, profile, i, line))
            continue;
        if (hooks->line_split != NULL) {
            if (hooks->line_split(hooks->context, line, &key, &value) != 2)
                continue;
        }
        else {
            char* separator = strchr(line, '=');
            if (separator == NULL) continue;
            *separator = '\0';
            key = line;
            value = separator + 1;
        }
        if (key == NULL || value == NULL) continue;
        table->entries[i].key = hooks->string_duplicate != NULL
            ? hooks->string_duplicate(hooks->context, key)
            : string_duplicate(key);
        if (table->entries[i].key == NULL) continue;
        table->entries[i].direct_value = 0;
        table->entries[i].fallback_value = hooks->integer_parse == NULL
            ? atoi(value) : hooks->integer_parse(hooks->context, value);
    }
    if (profile != NULL && hooks->profile_close != NULL)
        hooks->profile_close(hooks->context, profile);
    if (hooks->deferred_callback_register != NULL)
        hooks->deferred_callback_register(hooks->context, 0x0044dec0u);
    return 1;
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

// EngineLoaderProfileEntriesRelease
// FUNCTION: LEGORR 0x0044dd40
uint32_t ledger_stub_0044dd40_EngineLoaderProfileEntriesRelease(
    void* config, MotionConfigProfileEntry* entries, uint32_t count,
    const MotionConfigProfileReleaseHooks* hooks)
{
    return unported_ledger_stubs::motion_config_profile_entries_release(
        config, entries, count, hooks);
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

static EntityQuadCollisionHooks s_entity_quad_collision_hooks;

void entity_quad_collision_hooks_set(
    const EntityQuadCollisionHooks* hooks)
{
    if (hooks != NULL) s_entity_quad_collision_hooks = *hooks;
    else memset(&s_entity_quad_collision_hooks, 0,
                sizeof(s_entity_quad_collision_hooks));
}

namespace unported_ledger_stubs {

// FUNCTION: LEGORR 0x00466490
int ledger_stub_00466490_CellSightRayCheckAndFlag(
    const EntitySlot* observer)
{
    EntitySlot* cell_entity = NULL;
    LevelState* level;
    float observer_position[2];
    float start[2];
    float end[2];
    float direction_length;
    int cell_x = -1;
    int cell_y = -1;
    uint32_t cell_flags = 0u;
    int hit;
    if (observer == NULL || observer->owner_ctx == NULL ||
        observer->owner_ctx->level == NULL) return 0;
    level = observer->owner_ctx->level;
    if (level->ptr_terrain == NULL) return 0;
    entity_transform_position_get(observer, observer_position);
    if (!terrain_world_xy_to_cell(level->ptr_terrain, observer_position[0],
            observer_position[1], &cell_x, &cell_y) ||
        !entity_field_grid_cell_get(cell_x, cell_y, (void**)&cell_entity,
                                    &cell_flags) ||
        (cell_flags & 0x100u) == 0u || cell_entity == NULL ||
        !terrain_cell_world_center_xy_get(level->ptr_terrain, cell_x, cell_y,
                                          start)) return 0;

    /* Retail normalizes the occupied cell entity's surface vector before
     * using it as the ray endpoint. Prefer the installed renderer accessor;
     * EntitySlot's attached direction is the typed runtime representation
     * when that accessor has no provider. */
    if (!entity_surface_vector_normalize_if_available(cell_entity, end)) {
        end[0] = cell_entity->attached_direction[0];
        end[1] = cell_entity->attached_direction[1];
        direction_length = (float)sqrt((double)(end[0] * end[0] +
                                                 end[1] * end[1]));
        if (direction_length == 0.0f) return 0;
        end[0] /= direction_length;
        end[1] /= direction_length;
    }
    if ((entity_state_flags_get(observer) & 0x100u) != 0u) {
        hit = entity_surface_quad_segment_intersect((EntitySlot*)observer,
            start, end, NULL, 0, &s_entity_quad_collision_hooks);
    }
    else {
        hit = entity_segment_center_proximity_check(observer, start, end, 0);
    }
    if (!hit) return 0;
    ((EntitySlot*)observer)->flags_3e0 |= 0x02000000u;
    cell_entity->flags_3e0 |= 0x02000000u;
    return 1;
}


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// EntityIndexedFloatLookupWithMinusOneFallback
float ledger_stub_0046f1d0_EntityIndexedFloatLookupWithMinusOneFallback(
    int weapon_type, const void* entity_record)
{
    return lego::entity_indexed_float_lookup_with_minus_one_fallback(
        weapon_type, entity_record);
}

// TypeMotionScaleApplyDispatch
// FUNCTION: LEGORR 0x0046f270
void ledger_stub_0046f270_TypeMotionScaleApplyDispatch(
    EntitySlot* entity, int weapon_type, int apply_index, float scale,
    void* finish_argument)
{
    const TypeMotionScaleHooks* hooks = NULL;
    float base_value;
    float entry_scale = 0.0f;
    int entry_value = 0;
    int entry_present = 0;
    if (entity == NULL) return;
    base_value = lego::entity_indexed_float_lookup_with_minus_one_fallback(
        weapon_type, entity);
    if ((entity_state_flags_get(entity) & 0x00040000u) == 0u ||
        !entity_shared_eligibility_check(entity, 1)) return;
    if (hooks != NULL && hooks->indexed_entry_get != NULL) {
        entry_present = hooks->indexed_entry_get(hooks->context, weapon_type,
                                                 &entry_scale, &entry_value);
    }
    if (entry_present) {
        if (apply_index) {
            if (hooks != NULL && hooks->motion_apply != NULL) {
                hooks->motion_apply(hooks->context, entity,
                                    entry_scale * base_value, 1, scale);
            }
            else {
                entity_damage_apply(entity, entry_scale * base_value, 1);
            }
            if (hooks != NULL && hooks->entry_state_store != NULL)
                hooks->entry_state_store(hooks->context, entity, weapon_type,
                                         entry_value);
            if (hooks != NULL && hooks->finish_apply != NULL)
                hooks->finish_apply(hooks->context, entity, finish_argument, 1);
            else
                entity_directional_activity_start(entity,
                    (const float*)finish_argument, 1);
        }
        else {
            if (hooks != NULL && hooks->motion_apply != NULL) {
                hooks->motion_apply(hooks->context, entity,
                                    base_value * scale, 1, scale);
            }
            else {
                entity_damage_apply(entity, base_value * scale, 1);
            }
        }
        return;
    }
    if (entity->type == ENTITY_TYPE_OBJ) {
        if (hooks != NULL && hooks->type3_entry_miss != NULL) {
            hooks->type3_entry_miss(hooks->context, entity);
        }
        else {
            /* FUN_0046f270 calls FUN_00462890 on a type-3 table miss. */
            global_counter_at_0055532c_increment();
        }
    }
    if (hooks != NULL && hooks->motion_apply != NULL)
        hooks->motion_apply(hooks->context, entity, base_value, 1, scale);
    else
        entity_damage_apply(entity, base_value, 1);
    if (apply_index) {
        if (hooks != NULL && hooks->finish_apply != NULL)
            hooks->finish_apply(hooks->context, entity, finish_argument, 1);
        else
            entity_directional_activity_start(entity,
                (const float*)finish_argument, 1);
    }
    return;
}


} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

int ledger_stub_004702b0_EntitySurfaceQuadSegmentIntersect(
    EntitySlot* entity, const float start_xy[2], const float end_xy[2],
    float out_xy[2], int check_side);

// EntitySurfaceSegmentCollisionCheck
// FUNCTION: LEGORR 0x00470260
void ledger_stub_00470260_EntitySurfaceSegmentCollisionCheck(
    EntitySlot* entity, const float start_xy[2], const float end_xy[2],
    int check_side)
{
    if ((entity_state_flags_get(entity) & 0x100u) != 0u) {
        (void)ledger_stub_004702b0_EntitySurfaceQuadSegmentIntersect(
            entity, start_xy, end_xy, NULL, 0);
        return;
    }
    (void)entity_segment_center_proximity_check(entity, start_xy, end_xy,
                                               check_side);
}

// EntitySurfaceQuadSegmentIntersect
// FUNCTION: LEGORR 0x004702b0
int ledger_stub_004702b0_EntitySurfaceQuadSegmentIntersect(
    EntitySlot* entity, const float start_xy[2], const float end_xy[2],
    float out_xy[2], int check_side)
{
    return entity_surface_quad_segment_intersect(entity, start_xy, end_xy,
        out_xy, check_side, &s_entity_quad_collision_hooks);
}

// VelocitySlotSteerDispatchAndSound
// FUNCTION: LEGORR 0x00471d20
int ledger_stub_00471d20_VelocitySlotSteerDispatchAndSound(
    void* entity, void* argument, float delta)
{
    return entity_velocity_steer_dispatch_and_sound(entity, argument, delta);
}


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

// EntityBuildPlacementCommandSetup
// FUNCTION: LEGORR 0x00439370
void ledger_stub_00439370_EntityBuildPlacementCommandSetup(
    const int cell_xy[2], int entity_id, const float world_xy[2],
    int update_state)
{
    EntityBuildPlacementHooks* hooks = &s_entity_build_placement_hooks;
    GameContext* context = s_entity_build_placement_context != NULL
        ? s_entity_build_placement_context
        : entity_pool_callback_context_get();
    EntitySlot* created;
    uint8_t* cell_record;
    float position[2];
    float cell_position[3];

    if (cell_xy == NULL) return;
    if (world_xy != NULL) {
        position[0] = world_xy[0];
        position[1] = world_xy[1];
    }
    else if (hooks->position_choose != NULL) {
        if (!hooks->position_choose(hooks->context, cell_xy, position))
            return;
    }
    else {
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
            const float half_cell = terrain_context_scale_get(
                context->level->ptr_terrain);
            const float quarter_cell = terrain_context_scale_get(
                context->level->ptr_terrain);
            const float inset = half_cell * 0.5f - quarter_cell * 0.25f;
            position[0] = random_float_range(vertices[0] + inset,
                                              vertices[6] - inset);
            position[1] = random_float_range(vertices[1] - inset,
                                              vertices[7] + inset);
        }
        else {
            if (!terrain_cell_world_position_get(cell_xy[0], cell_xy[1],
                                                 cell_position)) return;
            position[0] = cell_position[0];
            position[1] = cell_position[1];
        }
    }
    if (hooks->entity_create != NULL)
        created = hooks->entity_create(hooks->context, 6, 0, entity_id,
                                       position[0], position[1]);
    else
        created = entity_create_with_position(context, 6, NULL, entity_id,
                                              position[0], position[1], 0.0f);
    if (created == NULL) return;
    if (hooks->queue_entity != NULL)
        hooks->queue_entity(hooks->context, 0x16, created);
    else
        cmd_enqueue(0x16, created, 0, 0, 0);

    cell_record = context != NULL && context->level != NULL
        ? level_voxel(context->level, cell_xy[0], cell_xy[1]) : NULL;
    if (update_state) {
        if (hooks->event_raise != NULL)
            hooks->event_raise(hooks->context, 0x11, cell_xy);
        else {
            message_dispatch(0, 0, 0, cell_xy);
            advisor_text_animation_update(0x11, 1, 0);
        }
        if (hooks->cell_counter_increment != NULL)
            hooks->cell_counter_increment(hooks->context, cell_xy, 0x2c);
        else if (cell_record != NULL)
            ++*(uint32_t*)(cell_record + 0x2c);
    }
}

// EntityBuildPlacementAlternateCommandSetup
// FUNCTION: LEGORR 0x004394b0
void ledger_stub_004394b0_EntityBuildPlacementAlternateCommandSetup(
    const int cell_xy[2], int entity_id, const float world_xy[2],
    int update_state)
{
    EntityBuildPlacementHooks* hooks = &s_entity_build_placement_hooks;
    GameContext* context = s_entity_build_placement_context != NULL
        ? s_entity_build_placement_context
        : entity_pool_callback_context_get();
    EntitySlot* created;
    uint8_t* cell_record;
    float position[2];
    float cell_position[3];

    if (cell_xy == NULL) return;
    if (world_xy != NULL) {
        position[0] = world_xy[0];
        position[1] = world_xy[1];
    }
    else if (hooks->position_choose != NULL) {
        if (!hooks->position_choose(hooks->context, cell_xy, position))
            return;
    }
    else {
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
            const float half_cell = terrain_context_scale_get(
                context->level->ptr_terrain);
            const float quarter_cell = terrain_context_scale_get(
                context->level->ptr_terrain);
            const float inset = half_cell * 0.5f - quarter_cell * 0.25f;
            position[0] = random_float_range(vertices[0] + inset,
                                              vertices[6] - inset);
            position[1] = random_float_range(vertices[1] - inset,
                                              vertices[7] + inset);
        }
        else {
            if (!terrain_cell_world_position_get(cell_xy[0], cell_xy[1],
                                                 cell_position)) return;
            position[0] = cell_position[0];
            position[1] = cell_position[1];
        }
    }
    if (hooks->entity_create != NULL)
        created = hooks->entity_create(hooks->context, 7, 0, entity_id,
                                       position[0], position[1]);
    else
        created = entity_create_with_position(context, 7, NULL, entity_id,
                                              position[0], position[1], 0.0f);
    if (created == NULL) return;
    if (hooks->queue_entity != NULL)
        hooks->queue_entity(hooks->context, 0x1c, created);
    else
        cmd_enqueue(0x1c, created, 0, 0, 0);

    if (hooks->event_raise != NULL)
        hooks->event_raise(hooks->context, 0x13, cell_xy);
    else
        advisor_text_animation_update(0x13, 1, 0);
    cell_record = context != NULL && context->level != NULL
        ? level_voxel(context->level, cell_xy[0], cell_xy[1]) : NULL;
    if (update_state) {
        if (hooks->cell_counter_increment != NULL)
            hooks->cell_counter_increment(hooks->context, cell_xy, 0x30);
        else if (cell_record != NULL)
            ++*(uint32_t*)(cell_record + 0x30);
    }
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

/* EntityAttachedObjectListDetach (FUN_00439e70): detach the carried-object
 * slot and clear its owner links. The port has one explicit carried object
 * instead of the corpus's variable-length attachment array. */
// FUNCTION: LEGORR 0x00439e70
void entity_attached_object_list_detach(EntitySlot* entity, int reset_mode)
{
    EntitySlot* carried;
    EntitySlot* linked_entity;
    uint32_t state_flags;
    if (entity == NULL) return;
    state_flags = entity->flags_3e0;
    if ((state_flags & 0x400u) == 0u ||
        ((state_flags & 0x2000u) != 0u && reset_mode == 0)) return;
    linked_entity = (EntitySlot*)(uintptr_t)entity->link_2fc;
    /* The retail consume-and-release branch walks the attached-record array
     * when the +0x2fc entity has stats bit 8 and reset mode is active. Those
     * records are full entity slots; clear their two owner links, unlink kind
     * 5, then let 0x43a850 apply counters and return each slot to its pool. */
    if (reset_mode != 0 && linked_entity != NULL &&
        (entity_stats_word_get(linked_entity) & 8u) != 0u &&
        entity->attached_children != NULL) {
        uint32_t i;
        GameContext* context = linked_entity->owner_ctx;
        if (context == NULL) context = entity_pool_callback_context_get();
        for (i = 0u; i < entity->attached_child_count; ++i) {
            EntitySlot* record = entity->attached_children[i];
            entity->attached_children[i] = NULL;
            if (record == NULL) continue;
            record->attach_304 = NULL;
            record->field_300 = 0u;
            entity_command_unlink_by_kind(entity, 5, record, 1);
            entity_action_record_consume_and_cleanup(linked_entity, record);
            entity_activity_scene_state_update(context);
        }
        entity->attached_child_count = 0u;
        entity->flags_3e0 &= ~0x400u;
        for (i = 0u; i < (uint32_t)entity->wp_count; ++i) {
            if (ENTITY_WAYPOINTS(entity)[i].step_kind == 5u ||
                ENTITY_WAYPOINTS(entity)[i].step_kind == 7u)
                ENTITY_WAYPOINTS(entity)[i].pending = 0u;
        }
        return;
    }
    carried = (EntitySlot*)entity->carry_link_308;
    if (reset_mode != 0 &&
        (entity_stats_word_get(entity) & 8u) == 0u) {
        (void)cmd_chain_drop(entity, 6);
        (void)cmd_chain_drop(entity, 9);
        (void)cmd_chain_drop(entity, 15);
        (void)cmd_chain_drop(entity, 20);
    }
    if (carried != NULL) {
        entity->carry_link_308 = NULL;
        entity->carry_state_324 = 0;
        carried->owner_36c = NULL;
        carried->link_2fc = 0u;
        if (reset_mode == 0) {
            if ((state_flags & 0x200u) == 0u) {
                (void)cmd_chain_drop(entity, 5);
                (void)cmd_chain_drop(entity, 6);
                (void)cmd_chain_drop(entity, 9);
                (void)cmd_chain_drop(entity, 15);
                (void)cmd_chain_drop(entity, 7);
                (void)cmd_chain_drop(entity, 4);
                (void)cmd_chain_drop(entity, 20);
            } else {
                (void)cmd_chain_drop(entity, 3);
            }
        } else {
            (void)cmd_chain_drop(entity, 5);
            if (carried->type != 7 && carried->type != 6)
                carried->flags_3ec |= 0x80000u;
        }
    }
    entity->carry_state_324 = 0;
    entity->flags_3e0 &= ~0x400u;
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


// RendererRecordClone
// FUNCTION: LEGORR 0x004078c0
void ledger_stub_004078c0_EntityCursorTargetStepResolveAndTimerAdvance(
    EntitySlot* entity, float delta, uint32_t* value_out, float out_xy[2])
{
    if (entity == NULL) return;
    if (out_xy != NULL)
        (void)entity_cursor_target_screen_position_get(entity, out_xy);

    entity->auxiliary_state_388 -= delta;
    if (entity->auxiliary_state_388 <= 0.0f) {
        if (value_out != NULL)
            pending_type_value_slot_select(entity, value_out);
    }
    else if (value_out != NULL) {
        *value_out = entity->auxiliary_slot_38c;
    }
    if (value_out != NULL && *value_out == 0u)
        *value_out = pending_type_value_get(0x00557868u);
}


/* The original boot path registers an SFX sample hook and an entity scene-end
 * hook. These adapters use the port's alias registry and EntitySlot flags;
 * owner-token resolution remains a renderer integration boundary. */
static EntityDDiSceneOwnerResolveHook s_ddiscene_owner_resolve;
static void* s_ddiscene_owner_resolve_context;

static void entity_ddiscene_sample_callback(const char* sample_name,
                                            void* scene_object,
                                            void* context)
{
    int alias;
    (void)context;
    if (sample_name != NULL && scene_object != NULL &&
        sound_alias_lookup(sample_name, &alias)) {
        sound_submission_route_wrapper(scene_object, alias, 0, 1, NULL);
    }
}

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
    ddiscene_frame_callbacks_set(entity_ddiscene_sample_callback, NULL, 1,
                                 entity_ddiscene_end_callback, NULL);
}


// LevelDebugCellRetargetDispatch
int ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
    EntitySlot* entity, unsigned int goal_x, unsigned int goal_y,
    int include_diagonals, unsigned int* out_candidate, int strict);
// FUNCTION: LEGORR 0x004403d0
int ledger_stub_004403d0_LevelDebugCellRetargetDispatch(
    EntitySlot* entity, EntitySlot* target,
    const EntityDebugCellRetargetHooks* hooks)
{
    LevelState* level;
    uint32_t target_state;
    uint32_t entity_flags;
    int target_cell[2];
    int target_flag20;
    unsigned int candidate;
    AiWaypoint* step;
    void* head;
    int simple_route_branch;
    if (entity == NULL || target == NULL || entity->type != 2 ||
        entity->owner_ctx == NULL || target->owner_36c != NULL) return 0;
    /* FUN_00403d10 only enters this path with an empty chain or a sole,
     * unlinked kind-0x12 record. The retail link is the record's +0x60
     * word, represented by cmd_rec_next in the portable record. */
    head = entity->cmd_head;
    if (head != NULL &&
        (cmd_rec_type(head) != 0x12 || cmd_rec_next(head) != NULL)) return 0;
    level = entity->owner_ctx->level;
    if (level == NULL) return 0;
    entity_flags = entity->flags_3f0;
    target_state = hooks != NULL && hooks->target_state_flags != NULL
        ? hooks->target_state_flags(hooks->context, target)
        : target->flags_3e8;
    target_flag20 = hooks != NULL && hooks->target_flag20_query != NULL
        ? hooks->target_flag20_query(hooks->context, target, 0u)
        : entity_record_flag20_query(target, NULL);
    /* The decompile's outer gate routes the fully flagged pair through its
     * special scene/record path. The terrain-edge route below is only in the
     * other branch. */
    simple_route_branch = (entity_flags & 5u) == 0u ||
        (target_state & 0x02000000u) == 0u ||
        (target_state & 0x00020000u) == 0u ||
        (target->flags_3ec & 0x40u) != 0u;
    if (simple_route_branch && (entity_flags & 2u) != 0u &&
        (target_state & 0x00020000u) != 0u &&
        (target->flags_3ec & 0x40u) == 0u &&
        target_flag20 != 0) {
        entity_position_to_terrain_cell(target, level, &target_cell[0],
                                        &target_cell[1]);
        if (target_cell[0] >= 0 && target_cell[1] >= 0 &&
            ledger_stub_004469c0_EntityNearestConnectedTerrainCellRouteSelect(
                entity, (unsigned int)target_cell[0],
                (unsigned int)target_cell[1], 0, &candidate, 1) &&
            entity->wp_count > 0) {
            step = &ENTITY_WAYPOINTS(entity)[entity->wp_count - 1];
            step->weight_x = 0.5f;
            step->weight_y = 0.5f;
            switch (candidate & 3u) {
            case 0u: step->weight_x = 0.1f; break;
            case 1u: step->weight_y = 0.1f; break;
            case 2u: step->weight_x = 0.9f; break;
            case 3u: step->weight_y = 0.9f; break;
            }
            step->pending = (uint8_t)(step->pending | 8u);
            step->step_kind = 12u;
            entity->link_2fc = (uintptr_t)target;
            return 1;
        }
    }
    return hooks != NULL && hooks->fallback_retarget != NULL
        ? hooks->fallback_retarget(hooks->context, entity, target, level) : 0;
}

// SpatialEntityCallbackQuery
typedef struct SpatialEntityQueryRequest {
    EntitySlot* subject;
    float position[2];
    float values[3];
    int callback_kind;
} SpatialEntityQueryRequest;
static EntitySpatialQueryHook s_spatial_entity_query_hook;
static void* s_spatial_entity_query_context;

namespace unported_ledger_stubs {

// FUNCTION: LEGORR 0x00444460
void ledger_stub_00444460_FlaggedVectorStepCommandIssue(
    EntitySlot* entity, const float vector[2])
{
    GameContext* context;
    LevelState* level;
    int start_cell[2] = { -1, -1 };
    int target_cell[2] = { -1, -1 };
    int* path_x = NULL;
    int* path_y = NULL;
    uint32_t path_count = 0u;
    float* route_x = NULL;
    float* route_y = NULL;
    uint32_t i;
    int route_installed;
    float position[2];
    float scale;
    if (entity == NULL || vector == NULL ||
        (entity->flags_3e0 & 0x800000u) != 0u ||
        !entity_shared_eligibility_check(entity, 1) ||
        entity_matches_current_selection_or_owner(entity) != 0 ||
        entity->owner_36c != NULL) return;
    context = entity->owner_ctx != NULL ? entity->owner_ctx
                                        : entity_pool_callback_context_get();
    if (context == NULL || context->level == NULL) return;
    level = context->level;
    entity_transform_position_get(entity, position);
    if (!terrain_world_xy_to_cell(level->ptr_terrain, position[0], position[1],
                                  &start_cell[0], &start_cell[1])) return;
    if (level_cell_walkable_flag(level, start_cell[0], start_cell[1]) != 0)
        return;

    entity_state_reset(entity, 1, 1);
    entity_transform_position_get(entity, position);
    scale = context->cfg != NULL
        ? block_get_float(context->cfg,
                          "Main MiniFigureRunAway", 0.0f)
        : 0.0f;
    if (scale == 0.0f) scale = 10.0f;
    position[0] += vector[0] * scale;
    position[1] += vector[1] * scale;
    if (!terrain_world_xy_to_cell(level->ptr_terrain, position[0], position[1],
                                  &target_cell[0], &target_cell[1])) return;

    if (start_cell[0] == target_cell[0] &&
        start_cell[1] == target_cell[1]) {
        path_x = (int*)malloc(sizeof(int));
        path_y = (int*)malloc(sizeof(int));
        if (path_x == NULL || path_y == NULL) {
            free(path_x);
            free(path_y);
            path_x = NULL;
            path_y = NULL;
            goto route_finish;
        }
        path_x[0] = target_cell[0];
        path_y[0] = target_cell[1];
        path_count = 1u;
    }
    else if (!terrain_neighbor_path_check(entity,
            (uint32_t)start_cell[0], (uint32_t)start_cell[1],
            (uint32_t)target_cell[0], (uint32_t)target_cell[1],
            &path_x, &path_y, &path_count, NULL, NULL)) {
        free(path_x);
        free(path_y);
        return;
    }
    if (path_count != 0u &&
        (size_t)path_count <= ((size_t)-1) / sizeof(float)) {
        route_x = (float*)malloc((size_t)path_count * sizeof(float));
        route_y = (float*)malloc((size_t)path_count * sizeof(float));
        if (route_x != NULL && route_y != NULL) {
            for (i = 0u; i < path_count; ++i) {
                route_x[i] = (float)path_x[i];
                route_y[i] = (float)path_y[i];
            }
            route_installed = entity_route_setup_and_marker_associate(
                entity, path_count, route_x, route_y, position);
            if (route_installed && entity->wp_count > 0 &&
                (entity->wp_dynamic == NULL
                    ? entity->wp_count <= AI_WAYPOINTS_MAX
                    : (unsigned int)entity->wp_count <= entity->wp_capacity)) {
                AiWaypoint* final_step = &ENTITY_WAYPOINTS(entity)[
                    entity->wp_count - 1];
                final_step->pending =
                    (uint8_t)(final_step->pending | 0x40u);
            }
        }
    }
    free(route_x);
    free(route_y);
    free(path_x);
    free(path_y);
route_finish:
    entity->flags_3e0 |= 0x800000u;
    idle_queue_register(entity);
}

} // namespace unported_ledger_stubs

void entity_spatial_query_hook_set(EntitySpatialQueryHook hook, void* context)
{
    s_spatial_entity_query_hook = hook;
    s_spatial_entity_query_context = context;
}

// FUNCTION: LEGORR 0x00440800
static int spatial_entity_damage_radius_callback(EntitySlot* candidate,
                                                 void* payload)
{
    SpatialEntityQueryRequest* request = (SpatialEntityQueryRequest*)payload;
    float candidate_position[2];
    float dx, dy, distance;
    if (request == NULL || candidate == NULL) return 0;
    if (candidate == request->subject ||
        (candidate->flags_3e8 & 0x40000u) == 0u) return 0;
    entity_transform_position_get(candidate, candidate_position);
    dx = candidate_position[0] - request->position[0];
    dy = candidate_position[1] - request->position[1];
    distance = (float)sqrt((double)(dx * dx + dy * dy));
    if (distance < request->values[0]) {
        float damage = (1.0f - distance / request->values[0]) *
                       request->values[1];
        entity_damage_apply(candidate, damage, 1);
        if (candidate->type == 3 && distance < request->values[2])
            entity_dispatch_event31_for_pending_flag(candidate);
    }
    return 0;
}

// FUNCTION: LEGORR 0x00440920
static int spatial_entity_vector_step_callback(EntitySlot* candidate,
                                               void* payload)
{
    SpatialEntityQueryRequest* request = (SpatialEntityQueryRequest*)payload;
    float candidate_position[2];
    float dx, dy, distance;
    if (request == NULL || candidate == NULL) return 0;
    if (candidate == request->subject ||
        (entity_state_flags_get(candidate) & 1u) == 0u) return 0;
    entity_transform_position_get(candidate, candidate_position);
    dx = candidate_position[0] - request->position[0];
    dy = candidate_position[1] - request->position[1];
    distance = (float)sqrt((double)(dx * dx + dy * dy));
    if (distance < request->values[0]) {
        if (candidate->type == 3)
            entity_dispatch_event31_for_pending_flag(candidate);
        if (distance != 0.0f) {
            float vector[2];
            vector[0] = (dx / distance) * request->values[1];
            vector[1] = (dy / distance) * request->values[2];
            unported_ledger_stubs::ledger_stub_00444460_FlaggedVectorStepCommandIssue(
                candidate, vector);
        }
    }
    return 0;
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

static int entity_spatial_query_dispatch_kind(EntitySlot* subject,
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

// FUNCTION: LEGORR 0x004407b0
void ledger_stub_004407b0_SpatialEntityCallbackQuery(
    EntitySlot* subject, float value_a, float value_b, float value_c)
{
    float position[2];
    float values[3];
    if (subject == NULL) return;
    entity_transform_position_get(subject, position);
    values[0] = value_a;
    values[1] = value_b;
    values[2] = value_c;
    (void)entity_spatial_query_dispatch_kind(subject, position, values, 0);
}

// SpatialEntityCallbackQueryAtOptionalCoordinate
// FUNCTION: LEGORR 0x004408c0
void ledger_stub_004408c0_SpatialEntityCallbackQueryAtOptionalCoordinate(
    EntitySlot* subject, const float* optional_position, float value)
{
    float position[2];
    float values[3];
    if (subject == NULL) return;
    if (optional_position == NULL)
        entity_transform_position_get(subject, position);
    else {
        position[0] = optional_position[0];
        position[1] = optional_position[1];
    }
    values[0] = value;
    /* The reference stack context leaves +0x10/+0x14 unwritten, but
     * LAB_00440920 reads them as vector scales. Use zero-initialized host
     * values for this otherwise undefined retail residue. */
    values[1] = 0.0f;
    values[2] = 0.0f;
    (void)entity_spatial_query_dispatch_kind(subject, position, values, 1);
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
