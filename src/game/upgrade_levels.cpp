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
#include "resource_shared.hpp"
#include "crt/crt.hpp"
#include "game/util.hpp"
#include <math.h>
#include <limits.h>
#include "record_shared.hpp"

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
