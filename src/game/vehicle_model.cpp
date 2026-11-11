// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-048 GAME-048: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-048.csv. */
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

/* EntityPayloadType1Release (FUN_0046ce10): clear indexed registrations,
 * free optional owned descriptor buffers, release six paired scene slots,
 * then release the primary/secondary scene resources. */
// FUNCTION: LEGORR 0x0046CE10
void entity_payload_type1_release(ScenePairedAnimationState* state)
{
    uint32_t i;
    if (state == NULL) {
        return;
    }
    for (i = 0u; i < state->activity_cache_count && i < 16u; ++i) {
        state->activity_cache[i] = NULL;
    }
    for (i = 0u; i < state->attached_count && i < 16u; ++i) {
        state->attached_cache[i] = NULL;
    }
    if ((state->flags & 1u) != 0u && s_scene_owned_memory_release_hook != NULL) {
        for (i = 0u; i < 4u; ++i) {
            if (state->owned_memory[i] != NULL) {
                s_scene_owned_memory_release_hook(state->owned_memory[i],
                                                  s_scene_owned_memory_context);
                state->owned_memory[i] = NULL;
            }
        }
    }
    for (i = 0u; i < 6u; ++i) {
        if (state->scene_slots[i] != NULL && s_scene_release_hook != NULL) {
            s_scene_release_hook(state->scene_slots[i], s_scene_lifecycle_context);
        }
        state->scene_slots[i] = NULL;
    }
    state->scene_slot_count = 0u;
    if (state->primary_resource != NULL && s_scene_release_hook != NULL) {
        s_scene_release_hook(state->primary_resource, s_scene_lifecycle_context);
    }
    if (state->secondary_resource != NULL && s_scene_release_hook != NULL) {
        s_scene_release_hook(state->secondary_resource, s_scene_lifecycle_context);
    }
    state->primary_resource = NULL;
    state->secondary_resource = NULL;
}
// FUNCTION: LEGORR 0x0046CED0
void paired_resource_state_set(ScenePairedAnimationState* state, int enabled)
{
    if (state == NULL || s_animation_track_apply_hook == NULL) {
        return;
    }
    s_animation_track_apply_hook(state->primary_track,
        state->primary_resource, enabled == 0, s_animation_track_context);
    if (state->secondary_resource != NULL && state->secondary_track != NULL) {
        s_animation_track_apply_hook(state->secondary_track,
            state->secondary_resource, enabled == 0,
            s_animation_track_context);
    }
}

// FUNCTION: LEGORR 0x0046CF20
void scene_entity_animation_tracks_set_enabled(
    ScenePairedAnimationState* state, int enabled)
{
    paired_resource_state_set(state, enabled);
}

/* RecordFlagBitTest (FUN_0046cf40) — see record.hpp. */
// FUNCTION: LEGORR 0x0046cf40
uint32_t record_flag_bit_test(const void* record, uint32_t bit_index,
                              int reject_masked)
{
    /* The corpus shifts blind; the x86 shift masks the count to 5 bits. */
    const uint32_t bit = 1u << bit_index;
    if (reject_masked != 0 && (entity_type_mask_get(record) & bit) != 0u) {
        return 0u;
    }
    uint32_t flags = 0u;
    __builtin_memcpy(&flags, (const char*)record + 0x130, sizeof(flags));
    return flags & bit;
}
// FUNCTION: LEGORR 0x0046cf80
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t entity_type_mask_get(const void* record)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov eax, dword ptr [eax + 12ch]
        ret
    }
}
#else
uint32_t entity_type_mask_get(const void* record)
{
    if (record == NULL) {
        return 0u; /* pinned deviation: the corpus loads blindly */
    }
    return *(const uint32_t*)((const char*)record + 300);
}
#endif

/* RecordIndexedEntriesRebuildAndRefresh (FUN_0046cf90): rebuild the selected
 * value's linked entries, then reset and refresh the associated channels. */
// FUNCTION: LEGORR 0x0046CF90
void scene_record_indexed_entries_rebuild_and_refresh(
    SceneIndexedRecordList* list, SceneChannelSlotsState* channels, int value)
{
    scene_indexed_record_list_rebuild_for_value(list, value);
    scene_channel_slots_rebuild(channels, 0);
}

/* SixResourceSlotsSetState (FUN_0046cfc0) — see record.hpp. The corpus
 * walks record+0x10..0x24 with a 6-count dec/jne loop from a lea'd cursor
 * and loads each slot directly; mirror that shape here. */
// FUNCTION: LEGORR 0x0046cfc0
void six_resource_slots_set_state(const void* record, uint32_t flag)
{
    const void* const* slot =
        (const void* const*)((const char*)record + 0x10);
    for (int remaining = 6; remaining != 0; --remaining, ++slot) {
        const void* handle = *slot;
        if (handle != NULL) {
            scene_visibility_hook_call((void*)handle, (int)flag);
        }
    }
}

/* SceneRecordTemplateInstantiate (FUN_0046cff0): copy template state,
 * clone the primary/secondary resources and attached scene handles, clear
 * per-instance caches, then enable the new resource graph. */
// FUNCTION: LEGORR 0x0046CFF0
int scene_record_template_instantiate(
    const ScenePairedAnimationState* source,
    ScenePairedAnimationState* destination)
{
    uint32_t i;
    if (source == NULL || destination == NULL
        || s_resource_child_create_hook == NULL
        || source->primary_resource == NULL) {
        return 0;
    }
    *destination = *source;
    memset(destination->activity_cache, 0, sizeof(destination->activity_cache));
    memset(destination->attached_cache, 0, sizeof(destination->attached_cache));
    memset(destination->scene_slots, 0, sizeof(destination->scene_slots));
    destination->primary_resource = s_resource_child_create_hook(
        source->primary_resource, s_resource_instance_context);
    if (destination->primary_resource == NULL) {
        return 0;
    }
    if (source->secondary_resource != NULL) {
        destination->secondary_resource = s_resource_child_create_hook(
            source->secondary_resource, s_resource_instance_context);
        if (destination->secondary_resource != NULL && s_resource_lock_hook != NULL) {
            s_resource_lock_hook(destination->secondary_resource, 1,
                                 s_resource_instance_context);
        }
    }
    if (s_resource_lock_hook != NULL) {
        s_resource_lock_hook(destination->primary_resource, 1,
                             s_resource_instance_context);
    }
    if (s_scene_resource_owner_apply_hook != NULL
        && destination->secondary_resource != NULL) {
        s_scene_resource_owner_apply_hook(destination->secondary_resource,
            destination->primary_resource, s_scene_resource_owner_context);
    }
    if (destination->attached_count > 16u) {
        destination->attached_count = 16u;
    }
    for (i = 0u; i < destination->attached_count; ++i) {
        if (source->attached_models[i] != NULL) {
            destination->attached_models[i] = s_resource_child_create_hook(
                source->attached_models[i], s_resource_instance_context);
        }
    }
    destination->cached_b4 = NULL;
    destination->cached_b8 = NULL;
    destination->cached_c0 = NULL;
    scene_resource_slots_rebuild(destination, destination->slot_base_name);
    return 1;
}
// FUNCTION: LEGORR 0x0046D140
void scene_assign_owner_from_model(ScenePairedAnimationState* state,
                                   void* owner)
{
    uint32_t i;
    if (state == NULL || s_scene_resource_owner_apply_hook == NULL) {
        return;
    }
    s_scene_resource_owner_apply_hook(state->primary_resource, owner,
                                      s_scene_resource_owner_context);
    if (state->secondary_resource != NULL) {
        s_scene_resource_owner_apply_hook(state->secondary_resource, owner,
                                          s_scene_resource_owner_context);
    }
    if (state->attached_count > 16u) {
        state->attached_count = 16u;
    }
    for (i = 0u; i < state->attached_count; ++i) {
        if (state->attached_models[i] != NULL) {
            s_scene_resource_owner_apply_hook(state->attached_models[i],
                owner, s_scene_resource_owner_context);
        }
    }
}

// FUNCTION: LEGORR 0x0046D1A0
void scene_object_attached_resource_release(void* record)
{
    /* FUN_0046d1a0 reads its scene object from argument +8 and queries the
     * object's duration. The length query has no result use at this callsite;
     * this entry point does not release or clear an attached resource. */
    if (record != NULL) {
        void* scene_object = *(void**)((char*)record + 8);
        (void)ddiscene_object_length_get(scene_object);
    }
}
/* AnimationDurationForLoopCount (FUN_0046d1c0): select the primary duration
 * unless flag 4 suppresses it, let a secondary animation override it, then
 * subtract repeated primary loop frames for multi-loop requests. */
// FUNCTION: LEGORR 0x0046D1C0
float animation_duration_for_loop_count(
    const SceneAnimationDurationState* state, uint32_t primary_mode,
    uint32_t secondary_mode, uint32_t loop_count)
{
    float duration = 0.0f;
    if (state == NULL) {
        return 0.0f;
    }
    if ((state->flags & 4u) == 0u && state->primary != NULL
        && s_animation_duration_hook != NULL) {
        duration = s_animation_duration_hook(state->primary, primary_mode,
                                             s_animation_duration_context);
    }
    if (state->secondary != NULL && s_animation_duration_hook != NULL) {
        duration = s_animation_duration_hook(state->secondary, secondary_mode,
                                             s_animation_duration_context);
    }
    if (loop_count > 1u && duration != 0.0f
        && state->primary != NULL && s_animation_last_frame_hook != NULL) {
        duration -= (float)(s_animation_last_frame_hook(state->primary,
            s_animation_duration_context) * (loop_count - 1u));
    }
    return duration;
}

// FUNCTION: LEGORR 0x0046D260
void scene_resource_slots_rebuild(ScenePairedAnimationState* state,
                                  const char* slot_base_name)
{
    uint32_t index;
    uint32_t count = 0u;
    if (state == NULL) {
        return;
    }
    for (index = 0u; index < 6u; ++index) {
        void* handle = NULL;
        if (state->primary_resource != NULL && slot_base_name != NULL
            && s_scene_resource_named_resolve_hook != NULL) {
            handle = s_scene_resource_named_resolve_hook(
                state->primary_resource, slot_base_name, index,
                s_scene_resource_record_context);
        }
        state->scene_slots[index] = handle;
        if (handle != NULL) {
            ++count;
        }
    }
    state->scene_slot_count = count;
}

/* SceneResourceVisibilitySet (FUN_0046d2c0): raise (+0x1e8 |= 2) or clear
 * (&= ~2) the record's visibility word, then route the raw flag through the
 * visibility operation — unconditionally on the +8 object, on +0xc when it
 * is set, and on every live child in the +0x10 window counted at +0x40.
 * Deviations: the corpus's +8/+0xc object fields are adjacent 32-bit
 * dwords, which the port's native-size pointer-slot model cannot carry
 * four bytes apart — the port routes the field values as raw handles,
 * exactly as the corpus does (adapter calls are recorded for tests).
 * The children keep the band's native-size stride (the
 * slot-window precedent) walked only to the window's six entries before
 * +0x40 (the corpus strides its dword slots blindly past the count field);
 * the count is read once (the visibility helper never touches the record) and NULL
 * records are guarded. The flag rides the leaf raw, unlike the overlay's
 * folded call. */
// FUNCTION: LEGORR 0x0046d2c0
void scene_resource_visibility_set(void* record, uint32_t flag)
{
    if (record == NULL) {
        return; /* deviation: the corpus strides blindly */
    }
    const uint32_t word = rec_dword(record, 0x1e8);
    rec_dword_put(record, 0x1e8, flag == 0u ? word & ~2u : word | 2u);
    scene_visibility_hook_call(
        (void*)(uintptr_t)rec_dword(record, 0x08), (int)flag);
    const uint32_t secondary = rec_dword(record, 0x0c);
    if (secondary != 0u) {
        scene_visibility_hook_call((void*)(uintptr_t)secondary, (int)flag);
    }
    const uint32_t count = rec_dword(record, 0x40);
    const uint32_t window = (0x40u - 0x10u) / (uint32_t)sizeof(void*);
    for (uint32_t i = 0; i < count && i < window; ++i) {
        void* child = rec_ptr(record, (size_t)0x10 + (size_t)i * sizeof(void*));
        if (child != NULL) {
            scene_visibility_hook_call(child, (int)flag);
        }
    }
}
/* The mapped EntityTypeFlagBit1Get body follows the corpus's unchecked
 * +0x1e8 load; the public helper retains its null guard. */
// FUNCTION: LEGORR 0x0046d330
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t entity_type_flag_bit1_get_target(const void* record)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov eax, dword ptr [eax + 01e8h]
        and eax, 0ffh
        shr eax, 1
        and eax, 1
        ret
    }
}
#else
uint32_t entity_type_flag_bit1_get_target(const void* record)
{
    return (*(const uint32_t*)((const char*)record + 0x1e8) & 0xffu) >> 1 & 1u;
}
#endif

/* SceneResourceDrawAtOrigin (FUN_0046d350): forward the record's primary
 * scene handle and caller coordinates through the origin-pose seam. */
// FUNCTION: LEGORR 0x0046D350
void scene_resource_draw_at_origin(SceneResourceAnimationRecord* record,
                                   float x, float y, float z)
{
    if (record != NULL && record->primary_resource != NULL
        && s_scene_effect_pose_hook != NULL) {
        s_scene_effect_pose_hook(record->primary_resource, x, y, z,
            0.0f, 0.0f, -1.0f, s_scene_effect_pose_context);
    }
}

// FUNCTION: LEGORR 0x0046d9e0
uint32_t scene_object_resource_get(const void* scene_object)
{
    uint32_t resource;
    __builtin_memcpy(&resource, (const char*)scene_object + 0x08,
                     sizeof(resource));
    return resource;
}

/* SceneResourceNamedEntryResolveWithFallback (FUN_0046d9f0): query the
 * secondary resource first, then the primary resource if it has no match. */
// FUNCTION: LEGORR 0x0046D9F0
void* scene_resource_named_entry_resolve_with_fallback(
    SceneResourceAnimationRecord* record, const char* name, uint32_t index)
{
    void* result = NULL;
    if (record == NULL || s_scene_resource_named_resolve_hook == NULL) {
        return NULL;
    }
    if (record->secondary_resource != NULL) {
        result = s_scene_resource_named_resolve_hook(
            record->secondary_resource, name, index,
            s_scene_resource_record_context);
    }
    if (result == NULL && record->primary_resource != NULL) {
        result = s_scene_resource_named_resolve_hook(record->primary_resource,
            name, index, s_scene_resource_record_context);
    }
    return result;
}

// FUNCTION: LEGORR 0x0046DA50
int scene_activity_change_dispatch(SceneResourceAnimationRecord* record,
                                   int index)
{
    void* result;
    if (record == NULL || record->activity_resource == NULL
        || index < 0 || index >= 16) {
        return 0;
    }
    result = record->activity_cache[index];
    if (result == NULL) {
        result = s_scene_resource_named_resolve_hook != NULL
            ? s_scene_resource_named_resolve_hook(record->activity_resource,
                NULL, (uint32_t)index, s_scene_resource_record_context) : NULL;
        record->activity_cache[index] = result;
    }
    return (int)(size_t)result;
}

// FUNCTION: LEGORR 0x0046DA90
void* scene_resource_cached_slot_b4_get(SceneResourceAnimationRecord* record)
{
    if (record == NULL || record->slot_b4_name == NULL) return NULL;
    if (record->cached_b4 == NULL) {
        record->cached_b4 = scene_resource_named_entry_resolve_with_fallback(
            record, record->slot_b4_name, 0u);
    }
    return record->cached_b4;
}

// FUNCTION: LEGORR 0x0046DAC0
void* scene_resource_cached_slot_b8_get(SceneResourceAnimationRecord* record)
{
    if (record == NULL || record->slot_b8_name == NULL) return NULL;
    if (record->cached_b8 == NULL) {
        record->cached_b8 = scene_resource_named_entry_resolve_with_fallback(
            record, record->slot_b8_name, 0u);
    }
    return record->cached_b8;
}

// FUNCTION: LEGORR 0x0046DAF0
void* scene_resource_cached_slot_c0_get(SceneResourceAnimationRecord* record)
{
    if (record == NULL || record->slot_c0_name == NULL) return NULL;
    if (record->cached_c0 == NULL) {
        record->cached_c0 = scene_resource_named_entry_resolve_with_fallback(
            record, record->slot_c0_name, 0u);
    }
    return record->cached_c0;
}

// FUNCTION: LEGORR 0x0046DB20
void* attached_entity_slot_object_get(SceneResourceAnimationRecord* record,
                                      int index)
{
    void* result;
    if (record == NULL || record->attached_resource == NULL
        || index < 0 || index >= 16) return NULL;
    result = record->attached_cache[index];
    if (result == NULL && s_scene_resource_named_resolve_hook != NULL) {
        result = s_scene_resource_named_resolve_hook(record->attached_resource,
            NULL, (uint32_t)index, s_scene_resource_record_context);
        record->attached_cache[index] = result;
    }
    return result;
}

// FUNCTION: LEGORR 0x0046db60
uint32_t object_record_field11c_get(const void* record)
{
    uint32_t value;
    __builtin_memcpy(&value, (const char*)record + 0x11c, sizeof(value));
    return value;
}

/* FUN_0046db70 passes the pointer stored at +8 to the typed-child scalar
 * accessor and discards its result. */
// FUNCTION: LEGORR 0x0046DB70
void scene_object_resource_release(void* record)
{
    (void)typed_child_resource_scalar_get(*(void**)((char*)record + 8));
}
