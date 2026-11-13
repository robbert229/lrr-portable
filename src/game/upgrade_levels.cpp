// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-047 GAME-047: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-047.csv. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "resource.hpp"
#include "heap.hpp"
#include "record.hpp"
#include "sound.hpp"
#include "string.hpp"
#include "texture.hpp"
#include "view.hpp"
#include "scene_pool.hpp"
#include "resource_for_upgrade_levels.hpp"
#include "crt/crt.hpp"
#include "game/util.hpp"
#include <math.h>
#include <limits.h>
#include "record_for_upgrade_levels.hpp"
#include "gods98/entity.h"

/* Upgrade.c's part functions (0046c030-0046c130), in original shape
 * (finding 597). The unit map gave them to ToolTip.c (GAME-046), but
 * 0046c110 loads its own 0.0f (.rdata 0049e7dc) where ToolTip_Initialise
 * reads another (0049e7cc). MSVC 5.0 pools a literal once an object, so they
 * are a separate object: Upgrade.c, which this file holds. A part
 * (Upgrade_PartModel, 0x10 bytes) is the port's RenderResourceBinding:
 * +0x00 the object type, +0x04 the container.
 *
 * The port's models with the hook tables stay in the port tail
 * tooltip_port.cpp. In the matching build (MSVC 5.0) these definitions take
 * four of the port's names, whose callers (lego_unit.c, legoobject_unit.c,
 * building_model.c) pass a part of the original's layout; the port tail
 * compiles its own definitions of them out there. Natively the port tail
 * serves those names. Upgrade_Part_MoveAnimation keeps the unit's name:
 * legoobject_unit.c calls it directly. A macro renames a definition one for
 * one, so MSVC's symbol numbering (finding 538) does not change. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define upgrade_part_load render_resource_binding_initialize /* 0046c030 */
#define upgrade_part_clone render_resource_binding_copy_and_convert /* 0046c0b0 */
#define upgrade_part_remove render_resource_binding_release /* 0046c0f0 */
#define upgrade_part_set_activity scene_object_set_animation_with_fallback /* 0046c130 */
#endif

extern "C" {

/* GAME-023 AI.c's activity names (004decc4); legoobject_port.cpp defines
 * them. */
extern const char* s_activity[79];
/* Container_Remove (00472a40), with C linkage in container_unit.cpp. */
void ledger_stub_00472a40_DDiSceneObjectDestroy(void* scene_object);

// FUNCTION: LEGORR 0x0046c030
void upgrade_part_load(RenderResourceBinding* upgradeModel, void* objID,
                       void* root, void* filename)
{
    memset(upgradeModel, 0, sizeof(*upgradeModel));

    upgradeModel->resource = container_load(root, (const char*)filename, "LWO", 1);
    if (upgradeModel->resource == NULL)
        upgradeModel->resource = container_load(root, (const char*)filename, "ACT", 1);
    if (upgradeModel->resource == NULL)
        upgradeModel->resource = container_load(root, (const char*)filename, "MESH", 1);

    upgradeModel->state = 0;
    upgradeModel->key = objID;
}

// FUNCTION: LEGORR 0x0046c0b0
void upgrade_part_clone(const RenderResourceBinding* srcUpgradeModel,
                        RenderResourceBinding* destUpgradeModel)
{
    *destUpgradeModel = *srcUpgradeModel;
    destUpgradeModel->resource = entity_visual_resource_load(srcUpgradeModel->resource);
}

// FUNCTION: LEGORR 0x0046c0f0
void upgrade_part_remove(RenderResourceBinding* upgradeModel)
{
    ledger_stub_00472a40_DDiSceneObjectDestroy(upgradeModel->resource);
}

// FUNCTION: LEGORR 0x0046c110
float upgrade_part_move_animation(RenderResourceBinding* upgradeModel,
                                  float elapsed, unsigned int repeat)
{
    ddiscene_advance_by_time_target(upgradeModel->resource, elapsed);
    return 0.0f;
}

// FUNCTION: LEGORR 0x0046c130
void upgrade_part_set_activity(RenderResourceBinding* upgradeModel,
                               void* activityName)
{
    if (!scene_action_name_set(upgradeModel->resource, (const char*)activityName)) {
        const char* standName = s_activity[0];
        scene_action_name_set(upgradeModel->resource, standName);
    }
}

} /* extern "C" */

/* IndexedRecordListRebuildForValue (FUN_0046c340): dispose the prior index
 * chain, then construct a linked node for each source record matching value. */
// FUNCTION: LEGORR 0x0046C340
void scene_indexed_record_list_rebuild_for_value(SceneIndexedRecordList* list,
                                                 int value)
{
    SceneIndexedRecordNode* node;
    SceneIndexedRecordNode* tail = NULL;
    uint32_t index;
    if (list == NULL) {
        return;
    }
    node = list->head;
    while (node != NULL) {
        SceneIndexedRecordNode* next = node->next;
        if (s_scene_indexed_record_release_hook != NULL) {
            s_scene_indexed_record_release_hook(node,
                                               s_scene_indexed_record_context);
        } else {
            free(node);
        }
        node = next;
    }
    list->selected_value = value;
    list->head = NULL;
    if (list->records == NULL) {
        return;
    }
    for (index = 0u; index < list->count; ++index) {
        if (list->records[index].value == value) {
            SceneIndexedRecordNode* fresh =
                (SceneIndexedRecordNode*)calloc(1u, sizeof(*fresh));
            if (fresh == NULL) {
                return;
            }
            fresh->record = &list->records[index];
            fresh->source_index = index;
            if (list->head == NULL) {
                list->head = fresh;
            } else {
                tail->next = fresh;
            }
            tail = fresh;
        }
    }
}

/* RecordFieldD8EqualsOne (FUN_0046c3d0): int == 1, not "nonzero". */
// FUNCTION: LEGORR 0x0046c3d0
int record_field_d8_equals_one(const void* record)
{
    int32_t v;
    __builtin_memcpy(&v, (const char*)record + 0xd8, sizeof(v));
    return v == 1;
}

// FUNCTION: LEGORR 0x0046C3F0
uint32_t scene_activity_paired_channel_update(
    ScenePairedAnimationState* state, uint32_t channel, int visible)
{
    uint32_t changed = 0u;
    uint32_t i;
    uint32_t previous_flags;
    if (state == NULL) {
        return 0u;
    }
    state->cached_b4 = NULL;
    state->cached_b8 = NULL;
    state->cached_c0 = NULL;
    for (i = 0u; i < state->activity_cache_count && i < 16u; ++i) {
        state->activity_cache[i] = NULL;
    }
    for (i = 0u; i < state->attached_count && i < 16u; ++i) {
        state->attached_cache[i] = NULL;
    }
    previous_flags = state->flags;
    if (s_animation_channel_update_hook != NULL) {
        changed = s_animation_channel_update_hook(state->primary_resource,
            channel, s_animation_channel_context);
    }
    state->flags &= ~4u;
    if (changed == 0u && (previous_flags & 8u) != 0u
        && s_animation_channel_query_hook != NULL
        && s_animation_channel_query_hook(state->primary_resource, channel,
                s_animation_channel_context) == 0) {
        state->flags |= 4u;
    }
    if (state->secondary_resource != NULL
        && s_animation_channel_update_hook != NULL) {
        changed |= s_animation_channel_update_hook(state->secondary_resource,
            channel, s_animation_channel_context);
    }
    if (s_animation_visibility_hook != NULL) {
        if ((state->flags & 4u) == 0u) {
            s_animation_visibility_hook(state->primary_resource, visible,
                                        s_animation_channel_context);
        }
        if (state->secondary_resource != NULL) {
            s_animation_visibility_hook(state->secondary_resource, visible,
                                        s_animation_channel_context);
        }
    }
    scene_resource_slots_rebuild(state, state->slot_base_name);
    return changed;
}

/* SceneChannelSlotsRebuild (FUN_0046c510): clear the fixed three-by-five
 * registration window and rebuild it by walking the scene's channel list. */
// FUNCTION: LEGORR 0x0046C510
void scene_channel_slots_rebuild(SceneChannelSlotsState* state, int mode)
{
    SceneChannelEntry* entry;
    uint32_t count = 0u;
    if (state == NULL) {
        return;
    }
    memset(state->slots, 0, sizeof(state->slots));
    state->registered_count = 0u;
    state->mode = mode == 0 ? 0u : 1u;
    for (entry = state->entries; entry != NULL; entry = entry->next) {
        uint32_t processed;
        uint32_t group;
        if (entry->record == NULL || s_scene_channel_entry_process_hook == NULL) {
            continue;
        }
        processed = s_scene_channel_entry_process_hook(entry->record,
            state->mode, &state->slots[0][0], 15u,
            s_scene_channel_entry_context);
        for (group = 0u; group < 3u; ++group) {
            uint32_t slot;
            for (slot = 0u; slot < 5u; ++slot) {
                if (state->slots[group][slot] != NULL) {
                    ++count;
                }
            }
        }
        if (processed == 0u) {
            continue;
        }
    }
    state->registered_count = count;
}
