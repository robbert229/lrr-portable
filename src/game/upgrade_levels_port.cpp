// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-047 Upgrade.c port tail (finding 617): the port's model of
 * Upgrade_SetUpgradeLevel (0046c340), unannotated. It also keeps the port's
 * models of Vehicle.c's first three functions (0046c3d0-0046c510, GAME-048),
 * which the relocation placed here: their original-shape versions are in
 * vehicle_model.c. The original-shape Upgrade.c is upgrade.c; the port's
 * models of its part functions (0046c030-0046c130) are in tooltip_port.cpp.
 * These models keep the port's contracts (its own record list, channel
 * state and paired animation state, with hooks, bounds and NULL guards)
 * for the native port, which calls them by these names.
 *
 * In the matching build (MSVC 5.0) upgrade.c and vehicle_model.c define two
 * of these names themselves (upgrade_link_names.h,
 * vehicle_model_link_names.h), so their definitions here are compiled out
 * there (LRR_UPGRADE_PORT_NAMES). The other two have C++ linkage, and their
 * callers are port code with other contracts (record_field_d8_equals_one is
 * also test_record.cpp's), so they stay in every build.
 * Relocated from the verbatim upgrade_levels.cpp (body hashes:
 * docs/coverage/relocations/GAME-047.csv). */
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

/* The port's definitions of the names upgrade.c and vehicle_model.c take in
 * the matching build: every build but MSVC 5.0's. */
#if defined(_MSC_VER) && _MSC_VER < 1200
#define LRR_UPGRADE_PORT_NAMES 0
#else
#define LRR_UPGRADE_PORT_NAMES 1
#endif

#if LRR_UPGRADE_PORT_NAMES /* upgrade.c serves this name in the matching build */
/* Port model of 0x0046c340 (Upgrade_SetUpgradeLevel): dispose the prior
 * index chain, then construct a linked node for each source record matching
 * value. */
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

#endif /* LRR_UPGRADE_PORT_NAMES */

/* Port model of 0x0046c3d0 (Vehicle_IsCameraFlipDir): int == 1, not
 * "nonzero". */
int record_field_d8_equals_one(const void* record)
{
    int32_t v;
    __builtin_memcpy(&v, (const char*)record + 0xd8, sizeof(v));
    return v == 1;
}

/* Port model of 0x0046c3f0 (Vehicle_SetActivity). */
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

#if LRR_UPGRADE_PORT_NAMES /* vehicle_model.c serves this name in the matching build */
/* Port model of 0x0046c510 (Vehicle_SetUpgradeActivity): clear the fixed
 * three-by-five registration window and rebuild it by walking the scene's
 * channel list. */
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
#endif /* LRR_UPGRADE_PORT_NAMES */
