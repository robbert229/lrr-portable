// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-004 GAME-004: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-004.csv. */
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "view.hpp"
#include "record.hpp"
#include "scene_resources.hpp"
#include "scene_pool.hpp"
#include "surface.hpp"
#include "sound.hpp"
#include "texture.hpp"
#include "heap.hpp"
#include "string.hpp"
#include "resource.hpp"
#include "crt/crt.hpp"
#include "model.hpp"
#include "../game/ai.hpp"
#include "../game/entity.hpp"
#include "../game/level.hpp"
#include <float.h>
#include <stdint.h>
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
#include "terrain_grid.hpp"
#include "cmd.hpp"
#include "select.hpp"
#include "gods98/engine.hpp"
#include "gods98/message.hpp"
#include "gods98/block.hpp"
#include "upgrades.hpp"
#include "frontend.hpp"
#include "util.hpp"
#include "gods98/media.hpp"
#include "gods98/entity.hpp"
#include <limits.h>
#include "gods98/view_shared.hpp"
#include "entity_shared.hpp"
#include "entity_config_shared.hpp"
#include "gods98/record_shared.hpp"

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static void renderer_object_update_from_record(void* record, unsigned int list_offset, int enabled, void* update_value);

// FUNCTION: LEGORR 0x00406990
int entity_type_config_resource_row_load(void* destination,
                                        uint32_t row_index,
                                        void* scene_context,
                                        const char* asset_path,
                                        void* engine_config,
                                        uint32_t row_kind)
{
    TypeConfigTable* table = NULL;
    uint32_t row_slot = 0xffffffffu;
    uintptr_t row_address;
    uintptr_t table_address;
    (void)scene_context;
    (void)engine_config;
    if (destination == NULL) return 0;
    if (row_kind == 1u) table = &s_minifigure_type_config;
    else if (row_kind == 2u) table = &s_building_type_config;
    else if (row_kind == 3u) table = &s_rockmonster_type_config;
    else if (row_kind == 4u) table = &s_vehicle_type_config;
    if (row_kind == 2u) memset(destination, 0, 0x14cu);
    row_address = (uintptr_t)destination;
    table_address = table != NULL ? (uintptr_t)table->rows : 0u;
    if (table != NULL && table->rows != NULL && table->row_owned != NULL &&
        table->row_stride != 0u && row_address >= table_address &&
        row_address - table_address <
            (uintptr_t)table->capacity * table->row_stride) {
        size_t offset = (size_t)(row_address - table_address);
        if (offset % table->row_stride == 0u &&
            offset / table->row_stride < table->capacity)
            row_slot = (uint32_t)(offset / table->row_stride);
    }
    if (row_kind == 2u && row_slot == 0xffffffffu) return 0;
    if (!entity_type_config_row_parse_path(table, asset_path, destination,
                                           row_kind)) return 0;
    entity_type_config_word_set(destination, 0u, row_index);
    if (row_kind == 2u) {
        entity_type_config_word_set(destination, 0x148u, 0u);
        if (s_entity_type_runtime_ops.scene_apply_mode != NULL) {
            uint32_t scene = 0u;
            memcpy(&scene, (char*)destination + 4u, sizeof(scene));
            if (scene != 0u)
                s_entity_type_runtime_ops.scene_apply_mode(
                    s_entity_type_runtime_ops.context, scene, 1);
        }
    }
    return 1;
}

// FUNCTION: LEGORR 0x00406c00
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void renderer_object_update_from_record_field64(
    void* record, int enabled)
{
    __asm {
        mov edx, dword ptr [esp + 8]
        xor eax, eax
        test edx, edx
        sete al
        push 0
        push eax
        mov eax, dword ptr [esp + 0ch]
        mov ecx, dword ptr [eax + 4]
        mov edx, dword ptr [eax + 64h]
        push ecx
        push edx
        call resource_list_update_matching_key
        add esp, 10h
        ret
    }
}
#else
void renderer_object_update_from_record_field64(void* record, int enabled)
{
    renderer_object_update_from_record(record, 0x64u, enabled,
                                       (void*)(uintptr_t)(enabled == 0));
}
#endif

// FUNCTION: LEGORR 0x00406c30
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void renderer_object_update_from_record_field68(
    void* record, int enabled)
{
    __asm {
        mov edx, dword ptr [esp + 8]
        xor eax, eax
        test edx, edx
        sete al
        push 0
        push eax
        mov eax, dword ptr [esp + 0ch]
        mov ecx, dword ptr [eax + 4]
        mov edx, dword ptr [eax + 68h]
        push ecx
        push edx
        call resource_list_update_matching_key
        add esp, 10h
        ret
    }
}
#else
void renderer_object_update_from_record_field68(void* record, int enabled)
{
    renderer_object_update_from_record(record, 0x68u, enabled,
                                       (void*)(uintptr_t)(enabled == 0));
}
#endif

// FUNCTION: LEGORR 0x00406c60
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void renderer_object_update_from_record_field6c(
    void* record, int enabled, void* update_value)
{
    __asm {
        mov eax, dword ptr [esp + 0ch]
        xor ecx, ecx
        push eax
        mov eax, dword ptr [esp + 0ch]
        test eax, eax
        mov eax, dword ptr [esp + 8]
        sete cl
        mov edx, dword ptr [eax + 4]
        mov eax, dword ptr [eax + 6ch]
        push ecx
        push edx
        push eax
        call resource_list_update_matching_key
        add esp, 10h
        ret
    }
}
#else
void renderer_object_update_from_record_field6c(void* record, int enabled,
                                                void* update_value)
{
    renderer_object_update_from_record(record, 0x6cu, enabled, update_value);
}
#endif

/* AnimationObjectVisibilitySet (FUN_00406c90) — animation record +4. */
// FUNCTION: LEGORR 0x00406c90
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void animation_object_visibility_set(const void* anim,
                                                        uint32_t flag)
{
    __asm {
        mov ecx, dword ptr [esp + 4]
        mov eax, dword ptr [esp + 8]
        push eax
        mov edx, dword ptr [ecx + 4]
        push edx
        call scene_render_lock_toggle
        add esp, 8
        ret
    }
}
#else
void animation_object_visibility_set(const void* anim, uint32_t flag)
{
    scene_render_lock_toggle(rec_ptr(anim, 0x04), (int)flag);
}
#endif

// RendererRecordClone (FUN_00406cb0).
// FUNCTION: LEGORR 0x00406cb0
void renderer_record_clone(const RendererRecord* source,
                           RendererRecord* destination)
{
    unsigned int i;
    if (source == NULL || destination == NULL) return;
    destination->first_word = source->first_word;
    destination->visual_resource = source->visual_resource;
    for (i = 0; i < 23; ++i) destination->prefix[i] = source->prefix[i];
    for (i = 0; i < 3; ++i) {
        destination->resource_lists[i] = source->resource_lists[i];
    }
    destination->flags = source->flags & ~1u;
    if (s_renderer_record_clone_hooks.linked_records_clone != NULL) {
        for (i = 0; i < 3; ++i) {
            destination->resource_lists[i] =
                s_renderer_record_clone_hooks.linked_records_clone(
                    s_renderer_record_clone_hooks.context,
                    source->resource_lists[i]);
        }
    }
    if (s_renderer_record_clone_hooks.visual_resource_load != NULL) {
        destination->visual_resource =
            s_renderer_record_clone_hooks.visual_resource_load(
                s_renderer_record_clone_hooks.context,
                source->visual_resource);
    }
}

/* AnimationObjectTimeUpdate (FUN_00406d10): forward the supplied time to
 * the scene frame stored in the animation object's +4 slot. */
// FUNCTION: LEGORR 0x00406D10
void animation_object_time_update(void* record, float time)
{
    void* frame;
    if (record == NULL || s_animation_object_time_update_hook == NULL) return;
    frame = *(void**)((char*)record + 4);
    if (frame != NULL) {
        s_animation_object_time_update_hook(
            frame, time, s_animation_object_hooks_context);
    }
}
/* RendererObjectTimingValueAdjustByCount (FUN_00406d30): read the base
 * timing value from the frame interface and subtract one frame duration for
 * each additional instance when the value is nonzero. */
// FUNCTION: LEGORR 0x00406D30
float renderer_object_timing_value_adjust_by_count(void* record,
                                                   void* argument,
                                                   uint32_t count)
{
    void* frame;
    float value;
    int frame_duration;
    if (record == NULL || s_scene_timing_value_get_hook == NULL) {
        return 0.0f;
    }
    frame = *(void**)((char*)record + 4);
    if (frame == NULL) {
        return 0.0f;
    }
    value = s_scene_timing_value_get_hook(frame, argument,
                                         s_scene_timing_hooks_context);
    if (count > 1u && value != 0.0f &&
        s_scene_timing_count_get_hook != NULL) {
        frame_duration = s_scene_timing_count_get_hook(
            frame, s_scene_timing_hooks_context);
        value -= (float)frame_duration * (float)(count - 1u);
    }
    return value;
}

/* AnimationObjectTimeGet (FUN_00406da0): query the object's current scene
 * frame through the installed backend callback. */
// FUNCTION: LEGORR 0x00406DA0
void animation_object_time_get(void* record)
{
    void* frame;
    if (record == NULL || s_animation_object_time_get_hook == NULL) return;
    frame = *(void**)((char*)record + 4);
    if (frame != NULL) {
        s_animation_object_time_get_hook(frame,
                                         s_animation_object_hooks_context);
    }
}
// AnimationObjectTransformSet (FUN_00406dc0).
// FUNCTION: LEGORR 0x00406dc0
void animation_object_transform_set(void* record, float first, float second)
{
    static const float orientation[4] = { 0.0f, 0.0f, 0.0f, -1.0f };
    void* render_object;
    if (record == NULL || s_animation_orientation_hook == NULL) return;
    render_object = *(void**)((char*)record + 4);
    if (render_object == NULL) return;
    s_animation_orientation_hook(s_animation_transform_context,
                                 render_object, first, second, orientation);
}

// RenderNodeSetPositionFromCallback (FUN_00406df0).
// FUNCTION: LEGORR 0x00406df0
void render_node_position_from_callback(void* record, float x, float y,
                                        RenderNodePositionValueFn value_fn,
                                        void* context)
{
    void* render_object;
    float z;
    if (record == NULL || value_fn == NULL ||
        s_render_node_position_hook == NULL) return;
    render_object = *(void**)((char*)record + 4);
    if (render_object == NULL) return;
    z = value_fn(x, y, context);
    s_render_node_position_hook(s_animation_transform_context,
                                render_object, x, y, z);
}

// FUNCTION: LEGORR 0x00406e30
uint32_t animation_object_handle_get(const void* animation_object)
{
    uint32_t handle;
    __builtin_memcpy(&handle, (const char*)animation_object + 0x04,
                     sizeof(handle));
    return handle;
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

/* ActivityRunnerStateRead (FUN_00406f80): lazily resolve and cache the
 * selected subobject from the runner's +0x20 source into its +0x54 table. */
// FUNCTION: LEGORR 0x00406f80
void* activity_runner_state_read(void* runner, int index)
{
    void* source = NULL;
    void* cached = NULL;
    size_t slot_offset;

    if (runner == NULL) return NULL;
    __builtin_memcpy(&source, (char*)runner + 0x20u, sizeof(source));
    if (source == NULL) return NULL;
    slot_offset = 0x54u + (size_t)index * sizeof(void*);
    __builtin_memcpy(&cached, (char*)runner + slot_offset, sizeof(cached));
    if (cached == NULL && s_renderer_subobject_resolver != NULL) {
        cached = s_renderer_subobject_resolver(runner, source, index);
        __builtin_memcpy((char*)runner + slot_offset, &cached,
                         sizeof(cached));
    }
    __builtin_memcpy(&cached, (char*)runner + slot_offset, sizeof(cached));
    return cached;
}
/* RendererCachedHandle48GetOrCreate (FUN_00406fe0): lazily resolve the
 * resource at +0x34 and cache its scene subobject at +0x48. */
// FUNCTION: LEGORR 0x00406FE0
void* renderer_cached_handle_48_get_or_create(void* record)
{
    void** source;
    void** cached;
    if (record == NULL) return NULL;
    source = (void**)((char*)record + 0x34);
    cached = (void**)((char*)record + 0x48);
    if (*source == NULL) return NULL;
    if (*cached == NULL) {
        *cached = renderer_object_subobject_find_apply(record, *source);
    }
    return *cached;
}

/* RendererCachedHandle50GetOrCreate (FUN_00407010): sibling cache with
 * source at +0x3c and cached handle at +0x50. */
// FUNCTION: LEGORR 0x00407010
void* renderer_cached_handle_50_get_or_create(void* record)
{
    void** source;
    void** cached;
    if (record == NULL) return NULL;
    source = (void**)((char*)record + 0x3c);
    cached = (void**)((char*)record + 0x50);
    if (*source == NULL) return NULL;
    if (*cached == NULL) {
        *cached = renderer_object_subobject_find_apply(record, *source);
    }
    return *cached;
}

/* RendererCachedHandle4cIsNotOne (FUN_00407040): lazily resolve +0x38 into
 * +0x4c, then answer whether the handle's scalar property differs from 1. */
// FUNCTION: LEGORR 0x00407040
int renderer_cached_handle_4c_is_not_one(void* record)
{
    void** source;
    void** cached;
    float value;
    if (record == NULL) return 0;
    source = (void**)((char*)record + 0x38);
    cached = (void**)((char*)record + 0x4c);
    if (*cached == NULL && *source != NULL) {
        *cached = renderer_object_subobject_find_apply(record, *source);
    }
    if (*cached == NULL || s_renderer_handle_scalar_get_hook == NULL) {
        return 0;
    }
    value = s_renderer_handle_scalar_get_hook(
        *cached, s_renderer_handle_scalar_get_context);
    return value != 1.0f;
}
/* AnimationObjectValueGet (FUN_00407090): run the value-query method on
 * the scene frame stored at +4. */
// FUNCTION: LEGORR 0x00407090
void animation_object_value_get(void* record)
{
    void* frame;
    if (record == NULL || s_animation_object_value_get_hook == NULL) return;
    frame = *(void**)((char*)record + 4);
    if (frame != NULL) {
        s_animation_object_value_get_hook(
            frame, s_animation_object_hooks_context);
    }
}

/* RendererObjectUpdateFromRecordField64/68/6c (FUN_00406c00,
 * FUN_00406c30, FUN_00406c60): feed the selected list, renderer library,
 * update switch, and key to the resource-list adapter. The original passes
 * `param_2 == 0` as a dword-valued opaque pointer; retain that call shape so
 * installed native hooks observe the same zero/one value. */
static void renderer_object_update_from_record(void* record,
                                               unsigned int list_offset,
                                               int enabled,
                                               void* update_value)
{
    void* list;
    void* library;
    if (record == NULL) {
        return;
    }
    __builtin_memcpy(&list, (const char*)record + list_offset, sizeof(list));
    __builtin_memcpy(&library, (const char*)record + 0x04, sizeof(library));
    resource_list_update_matching_key(list, library, update_value, 0u);
}
