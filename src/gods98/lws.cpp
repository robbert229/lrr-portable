// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-020 Lws.c: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/G98-020.csv. */
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
#include "view_for_com_object_tree.hpp"
#include <float.h>
#include <errno.h>
#include <limits.h>
#include "game/util.hpp"
#include "view_for_lws.hpp"
#include "model_for_lws.hpp"
#include "scene_resources_for_lws.hpp"

namespace unported_ledger_stubs {
typedef struct LedgerSceneGuid {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
} LedgerSceneGuid;
typedef int32_t (LEDGER_SCENE_COM_STDCALL *SceneInterfacePairGetMethod)(
    void* object, void** result, void** previous_result);
typedef int32_t (LEDGER_SCENE_COM_STDCALL *SceneQueryInterfaceMethod)(
    void* object, const LedgerSceneGuid* interface_id, void** result);
typedef uint32_t (LEDGER_SCENE_COM_STDCALL *SceneInterfaceReleaseMethod)(
    void* object);
typedef void* (LEDGER_SCENE_COM_STDCALL *SceneAnimationResourceGetMethod)(
    void* object);

// ResourceTreeCallbackReferenceChainRelease
typedef void (*ResourceReferenceReleaseHook)(void* resource, void* context);
typedef struct ResourceReferenceChain {
    void* resource;
    struct ResourceReferenceChain* next;
} ResourceReferenceChain;

// LoadRecordInterfacesReleaseFree
typedef struct LoadRecordInterfaceList {
    void** interfaces;
    uint32_t count;
} LoadRecordInterfaceList;
} // namespace unported_ledger_stubs

// GLOBAL: LEGORR 0x005446bc
int s_model_resource_primary_count;

// GLOBAL: LEGORR 0x005446c0
int s_model_resource_secondary_count;

static SharedObjectEntryResolveFn s_shared_object_entry_resolve;

namespace unported_ledger_stubs {
void ledger_stub_00487ba0_SharedObjectAnimationAndTimedEventUpdate(
    void* shared_object, float time);

void ledger_stub_00487d50_ComInterfaceQueryAndContextDispatch(
    void* owner, void* track);

// SharedObjectAnimationAndTimedEventUpdate
void ledger_stub_00487d50_ComInterfaceQueryAndContextDispatch(
    void* owner, void* track);
} // namespace unported_ledger_stubs

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static int scene_entry_comma_split(char* text, char** tokens, int token_capacity);
namespace unported_ledger_stubs { static void ledger_scene_interface_release(void* object); }
static void model_resource_path_join(char* output, size_t output_size, const char* prefix, const char* path);

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x004869f0
void* ledger_stub_004869f0_LoadLWSCScene(
    const char* path, void* context,
    void* (*scene_load)(const char* path, void* context))
{
    if (path == NULL || path[0] == '\0' || scene_load == NULL) return NULL;
    return scene_load(path, context);
}
} // namespace unported_ledger_stubs

/* SharedObjectDirectoryScan (FUN_004876c0) installs the directory prefix and
 * three callbacks consumed by the scene-entry and timed-sound paths. */
// FUNCTION: LEGORR 0x004876c0
void shared_object_directory_scan_configure(
    const char* directory, SharedObjectEntryResolveFn resolve,
    SharedObjectSoundEnabledFn sound_enabled,
    SharedObjectSoundSubmitFn sound_submit)
{
    if (directory != NULL) (void)shared_object_directory_root_set(directory);
    s_shared_object_entry_resolve = resolve;
    s_shared_object_sound_enabled = sound_enabled;
    s_shared_object_sound_submit = sound_submit;
}

namespace lego {
// FUNCTION: LEGORR 0x00487760
void scene_resource_string_tables_free()
{
    unsigned int i;
    /* The two retail tables freed at 0x87760 are the model-resource cache
     * tables owned by model.cpp. Clear them here because engine teardown
     * reaches this entrypoint; the portable cache also owns each parsed
     * Model value and releases it alongside its copied key. */
    model_resource_cache_clear();
    if (s_release_hook != 0) {
        for (i = 0; i < s_string_count_a; ++i)
            if (s_string_table_a != 0 && s_string_table_a[i] != 0) s_release_hook(s_string_table_a[i], s_release_context);
        for (i = 0; i < s_string_count_b; ++i)
            if (s_string_table_b != 0 && s_string_table_b[i] != 0) s_release_hook(s_string_table_b[i], s_release_context);
    }
    s_string_table_a = 0; s_string_table_b = 0; s_string_count_a = 0; s_string_count_b = 0;
}
} // namespace lego

// FUNCTION: LEGORR 0x004877d0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) uint32_t resource_entry_id_get(const void* entry)
{
    __asm {
        mov ecx, dword ptr [esp + 4]
        xor eax, eax
        mov ax, word ptr [ecx + 2]
        ret
    }
}
#else
uint32_t resource_entry_id_get(const void* entry)
{
    if (entry != NULL) {
        uint16_t v = 0;
        __builtin_memcpy(&v, (const char*)entry + 2, sizeof(v));
        return v;
    }
    return 0u;
}
#endif

/* SceneEntrySlotTableExpand (FUN_004877e0) consumes the directory callback
 * from 0x4876c0, resolves only records with entry flag bit 1, then records the
 * numeric prefix/suffix of each post-comma token in the 0xd0-byte output row. */
// FUNCTION: LEGORR 0x004877e0
void shared_object_scene_entry_slot_table_expand(void* scene)
{
    SceneEntryExpandOps* const ops = &s_scene_entry_expand_ops;
    uint16_t source_count = 0;
    uint16_t expanded_capacity = 0;
    SceneEntryExpandedRecord* table;
    uint16_t source_index;
    uint8_t output_index = 0;
    if (scene == NULL || ops->counts_get == NULL ||
        ops->expanded_capacity_set == NULL) return;
    if (s_shared_object_entry_resolve == NULL) {
        ops->expanded_capacity_set(scene, 0u, ops->context);
        return;
    }
    if (ops->entry_get == NULL || ops->entry_slot_set == NULL ||
        ops->expanded_table_set == NULL ||
        !ops->counts_get(scene, &source_count, &expanded_capacity,
                         ops->context)) return;
    if (expanded_capacity == 0u) return;
    table = (SceneEntryExpandedRecord*)engine_heap_alloc(
        (long)((size_t)expanded_capacity * sizeof(*table)));
    if (table == NULL) return;
    ops->expanded_table_set(scene, table, ops->context);
    for (source_index = 0u; source_index < source_count; ++source_index) {
        SceneEntryExpandInfo info;
        SceneEntryExpandedRecord* expanded;
        char name_copy[1024];
        char* tokens[1024];
        size_t name_length;
        int token_count;
        int token_index;
        int loaded;
        if (!ops->entry_get(scene, source_index, &info, ops->context) ||
            (info.flags & 2u) == 0u) continue;
        if ((uint16_t)output_index >= expanded_capacity) break;
        ops->entry_slot_set(scene, source_index, output_index, ops->context);
        expanded = &table[output_index++];
        if (info.name == NULL) {
            expanded->bytes[0xcc] = 0;
            expanded->bytes[0xcd] = 0;
            continue;
        }
        name_length = strlen(info.name);
        if (name_length >= sizeof(name_copy)) name_length = sizeof(name_copy) - 1u;
        memcpy(name_copy, info.name, name_length);
        name_copy[name_length] = '\0';
        token_count = scene_entry_comma_split(name_copy, tokens, 1024);
        if (token_count < 2) continue;
        *(uint16_t*)(expanded->bytes + 0xcc) = (uint16_t)(token_count - 2);
        loaded = s_shared_object_entry_resolve(tokens[1], expanded);
        if (ops->debug_flags_get != NULL &&
            (ops->debug_flags_get(ops->context) & 0x00100000u) != 0u &&
            loaded == 0) {
            memset(expanded->bytes, 0, sizeof(uint32_t));
        }
        for (token_index = 2; token_index < token_count; ++token_index) {
            char* dash = strstr(tokens[token_index], "-");
            int first = atoi(tokens[token_index]);
            int second = dash != NULL ? atoi(dash + 1) : first;
            size_t first_offset = 4u + (size_t)(token_index - 2) * 2u;
            size_t second_offset = 0x36u + (size_t)(token_index - 2) * 2u;
            uint16_t first_word = (uint16_t)first;
            uint16_t second_word = (uint16_t)second;
            if (first_offset + sizeof(first_word) <= sizeof(expanded->bytes))
                memcpy(expanded->bytes + first_offset, &first_word,
                       sizeof(first_word));
            if (second_offset + sizeof(second_word) <= sizeof(expanded->bytes))
                memcpy(expanded->bytes + second_offset, &second_word,
                       sizeof(second_word));
        }
    }
}

/* SharedObjectClone (FUN_00487a00): copy the 14-dword header, allocate a
 * fresh interface array, register the linked children, rebuild each eligible
 * interface component through its scene owner, link/refcount the source, and
 * finish by updating the clone at time zero. The interface callbacks preserve
 * the renderer-owned COM method sequence without byte-cloning interfaces. */
// FUNCTION: LEGORR 0x00487a00
void* shared_object_clone(void* source, void* caller_context)
{
    SharedObjectCloneOps* const ops = &s_shared_object_clone_ops;
    void* source_base;
    void* clone;
    void** component_table;
    uint16_t entry_count;
    uint16_t index;
    void* child;
    if (source == NULL || ops->source_base_get == NULL ||
        ops->child_first_get == NULL || ops->child_next_get == NULL ||
        ops->child_register == NULL || ops->entry_flags_get == NULL ||
        ops->entry_count_get == NULL || ops->component_get == NULL ||
        ops->component_table_set == NULL || ops->source_link_set == NULL ||
        ops->clone_reference_set == NULL ||
        ops->source_reference_increment == NULL ||
        ops->component_clone == NULL || ops->component_resource_create == NULL ||
        ops->time_update == NULL) return NULL;
    source_base = ops->source_base_get(source, ops->context);
    if (source_base == NULL) return NULL;
    entry_count = ops->entry_count_get(source_base, ops->context);
    clone = engine_heap_alloc(0x38L);
    if (clone == NULL) return NULL;
    memset(clone, 0, 0x38u);
    memcpy(clone, source_base, 0x38u);
    ops->clone_reference_set(clone, 1u, ops->context);
    component_table = NULL;
    if (entry_count != 0u) {
        component_table = (void**)engine_heap_alloc(
            (long)((size_t)entry_count * sizeof(void*)));
        if (component_table == NULL) {
            engine_heap_free(clone);
            return NULL;
        }
        memset(component_table, 0, (size_t)entry_count * sizeof(void*));
    }
    ops->component_table_set(clone, component_table, ops->context);
    child = ops->child_first_get(source_base, ops->context);
    while (child != NULL) {
        ops->child_register(clone, child, caller_context, ops->context);
        child = ops->child_next_get(child, ops->context);
    }
    for (index = 0u; index < entry_count; ++index) {
        void* source_interface = ops->component_get(source_base, index,
                                                     ops->context);
        void* clone_interface = ops->component_get(clone, index,
                                                   ops->context);
        uint32_t flags = ops->entry_flags_get(source_base, index,
                                              ops->context);
        ops->component_clone(source_interface, clone_interface, clone,
                             index, ops->context);
        if ((flags & 1u) == 0u)
            ops->component_resource_create(source_interface,
                                           clone_interface, clone, index,
                                           ops->context);
    }
    ops->source_link_set(clone, source_base, ops->context);
    ops->source_reference_increment(source_base, ops->context);
    ops->time_update(clone, 0.0f, ops->context);
    return clone;
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x00487ba0
void ledger_stub_00487ba0_SharedObjectAnimationAndTimedEventUpdate(
    void* shared_object, float time)
{
    unsigned char* owner = (unsigned char*)shared_object;
    uint16_t duration;
    uint16_t i;
    *(uint32_t*)(owner + 0x0c) = *(uint32_t*)(owner + 0x10);
    duration = *(uint16_t*)(owner + 2);
    if ((owner[0x34] & 1u) == 0u) {
        if (time > (float)duration) time = (float)duration;
        *(float*)(owner + 0x10) = time;
    }
    /* Repeat mode compares the duration with the input on the x87 stack,
     * calls the CRT intrinsic dispatcher, then stores the time operand.
     * The dispatcher is opaque here, so model it as leaving that operand
     * unchanged rather than inventing a remainder or clock-based update. */
    if ((owner[0x34] & 1u) != 0u) *(float*)(owner + 0x10) = time;
    for (i = 0; i < *(uint16_t*)(owner + 0x24); ++i) {
        unsigned char* tracks = *(unsigned char**)(owner + 0x20);
        unsigned char* track = tracks + (uint32_t)i * 0x34u;
        uint16_t vector_count;
        uint16_t scalar_count;
        uint16_t segment = 0;
        uint16_t scalar_segment = 0;
        float fraction;
        float scalar_fraction;
        vector_count = *(uint16_t*)(track + 0x24);
        if (vector_count > 1u) {
            fraction = animation_track_find_keyframe_and_fraction(
                track, *(float*)(owner + 0x10), &segment);
            lego::animation_track_keys_interpolate_apply(
                owner, track, segment, fraction);
        }
        scalar_count = *(uint16_t*)(track + 0x26);
        if (scalar_count > 1u) {
            scalar_fraction = lego::keyframe_time_segment_fraction(
                track, *(float*)(owner + 0x10), &scalar_segment);
            lego::shared_object_float_track_interpolate_apply(
                owner, track, scalar_segment, scalar_fraction);
        }
        ledger_stub_00487d50_ComInterfaceQueryAndContextDispatch(owner, track);
        shared_object_timed_sound_event_update(owner, track);
    }
}
} // namespace unported_ledger_stubs

// FUNCTION: LEGORR 0x00487cb0
/* AnimationTrackFindKeyframeAndFraction helper. The track contains
 * a count at +0x24 and 0x28-byte key records at +0x18; each key's time is at
 * +0x24. Keep the original lower-bound binary search and interpolation
 * denominator (the next key's timestamp) intact. */
float animation_track_find_keyframe_and_fraction(const void* track,
                                                 float time,
                                                 uint16_t* keyframe_out)
{
    const unsigned char* bytes =
        static_cast<const unsigned char*>(track);
    const unsigned char* keys;
    uint16_t count;
    uint32_t low = 0;
    uint32_t high;
    uint32_t middle;
    uint16_t key_time;
    uint16_t next_time;
    if (track == NULL || keyframe_out == NULL) return 0.0f;
    memcpy(&count, bytes + 0x24, sizeof(count));
    memcpy(&keys, bytes + 0x18, sizeof(keys));
    high = (uint32_t)count;
    do {
        middle = (high + low) / 2u;
        memcpy(&key_time, keys + middle * 0x28u + 0x24,
               sizeof(key_time));
        if (middle == low) break;
        if (time < (float)key_time) high = middle;
        else low = middle;
    } while (1);
    *keyframe_out = (uint16_t)middle;
    memcpy(&next_time, keys + middle * 0x28u + 0x4c,
           sizeof(next_time));
    return (time - (float)key_time) /
           ((float)next_time - (float)key_time);
}

namespace unported_ledger_stubs {
// FUNCTION: LEGORR 0x00487d50
void ledger_stub_00487d50_ComInterfaceQueryAndContextDispatch(
    void* owner, void* track)
{
    unsigned char* owner_bytes = (unsigned char*)owner;
    unsigned char* track_bytes = (unsigned char*)track;
    void** interfaces;
    void* object;
    void* pair = NULL;
    void* user_visual = NULL;
    void* user_visual_interface = NULL;
    void* animation_resource;
    void** vtable;
    uint16_t interface_index;
    int frame_offset;
    LedgerSceneGuid user_visual_id;
    SceneInterfacePairGetMethod interface_pair_get;
    SceneQueryInterfaceMethod query_interface;
    SceneAnimationResourceGetMethod animation_resource_get;
    if (owner_bytes == NULL || track_bytes == NULL ||
        (track_bytes[0x28] & 1u) != 0u) return;
    memcpy(&interface_index, track_bytes + 6, sizeof(interface_index));
    memcpy(&interfaces, owner_bytes + 0x14, sizeof(interfaces));
    if (interfaces == NULL) return;
    object = interfaces[interface_index];
    if (object == NULL) return;
    vtable = *(void***)object;
    if (vtable == NULL || vtable[0x80 / 4] == NULL) return;
    interface_pair_get = (SceneInterfacePairGetMethod)vtable[0x80 / 4];
    (void)interface_pair_get(object, &pair, NULL);
    (void)interface_pair_get(object, &user_visual, &pair);
    if (user_visual == NULL) return;
    user_visual_id.data1 = 0x59163de0u;
    user_visual_id.data2 = 0x6d43u;
    user_visual_id.data3 = 0x11cfu;
    user_visual_id.data4[0] = 0xacu;
    user_visual_id.data4[1] = 0x4au;
    user_visual_id.data4[2] = 0x00u;
    user_visual_id.data4[3] = 0x00u;
    user_visual_id.data4[4] = 0xc0u;
    user_visual_id.data4[5] = 0x38u;
    user_visual_id.data4[6] = 0x25u;
    user_visual_id.data4[7] = 0xa1u;
    vtable = *(void***)user_visual;
    if (vtable == NULL || vtable[0] == NULL) {
        ledger_scene_interface_release(user_visual);
        return;
    }
    query_interface = (SceneQueryInterfaceMethod)vtable[0];
    (void)query_interface(user_visual, &user_visual_id,
                          &user_visual_interface);
    ledger_scene_interface_release(user_visual);
    if (user_visual_interface == NULL) return;
    ledger_scene_interface_release(user_visual_interface);
    /* FUN_00487d50 calls slot +0x1c on the selected interface itself. The
     * queried UserVisual interface is only used for the IID check; it is not
     * the animation-track resource owner. */
    vtable = *(void***)object;
    if (vtable == NULL || vtable[0x1c / 4] == NULL) return;
    animation_resource_get =
        (SceneAnimationResourceGetMethod)vtable[0x1c / 4];
    animation_resource = animation_resource_get(object);
    memcpy(&frame_offset, owner_bytes + 0x10, sizeof(frame_offset));
    (void)ledger_stub_00482040_AnimationTrackApplyWrappedFrameOffset(
        animation_resource, frame_offset);
}
} // namespace unported_ledger_stubs

/* SharedObjectTimedSoundEventUpdate (FUN_00487de0) walks the event times and
 * saved sound handles in the expanded entry selected by track byte +8. */
// FUNCTION: LEGORR 0x00487de0
void shared_object_timed_sound_event_update(void* owner, void* track)
{
    unsigned char* owner_bytes = (unsigned char*)owner;
    unsigned char* track_bytes = (unsigned char*)track;
    void** sound_objects;
    unsigned char* entries;
    unsigned char* entry;
    uint16_t interface_index;
    uint16_t event_count;
    uint16_t i;
    uint8_t entry_index;
    SharedObjectSoundEnabledFn enabled_callback;
    SharedObjectSoundSubmitFn submit_callback;
    void* sound_object;
    if (owner_bytes == NULL || track_bytes == NULL) return;
    if ((track_bytes[0x28] & 2u) == 0u) return;
    memcpy(&interface_index, track_bytes + 6, sizeof(interface_index));
    memcpy(&sound_objects, owner_bytes + 0x14, sizeof(sound_objects));
    memcpy(&entries, owner_bytes + 0x18, sizeof(entries));
    if (sound_objects == NULL || entries == NULL) return;
    entry_index = track_bytes[8];
    entry = entries + (size_t)entry_index * 0xd0u;
    memcpy(&event_count, entry + 0xcc, sizeof(event_count));
    if (event_count == 0u) return;
    sound_object = sound_objects[interface_index];
    enabled_callback = s_shared_object_sound_enabled;
    submit_callback = s_shared_object_sound_submit;
    for (i = 0; i < event_count; ++i) {
        int16_t start_time;
        int16_t end_time;
        uint32_t handle;
        int changed;
        memcpy(&start_time, entry + 4u + (size_t)i * 2u,
               sizeof(start_time));
        memcpy(&end_time, entry + 0x36u + (size_t)i * 2u,
               sizeof(end_time));
        changed = start_time != end_time;
        if (lego::shared_object_sample_time_matches(owner,
                (uint32_t)(int32_t)start_time) != 0 &&
            enabled_callback != NULL && enabled_callback() != 0 &&
            submit_callback != NULL) {
            int alias;
            memcpy(&alias, entry, sizeof(alias));
            handle = (uint32_t)submit_callback(sound_object, alias,
                                                (uint32_t)changed, 1u, NULL);
            memcpy(entry + 0x68u + (size_t)i * 4u, &handle,
                   sizeof(handle));
        }
        if (changed != 0 && lego::shared_object_sample_time_matches(owner,
                (uint32_t)(int32_t)end_time) != 0) {
            memcpy(&handle, entry + 0x68u + (size_t)i * 4u,
                   sizeof(handle));
            sound_buffer_stop_handle((int)handle);
        }
    }
}

namespace lego {
// FUNCTION: LEGORR 0x00487ed0
int shared_object_sample_time_matches(const void* record, uint32_t time)
{
    const unsigned char* bytes = static_cast<const unsigned char*>(record);
    float a, b, low, high, sample_time;
    uint32_t duration;
    if (record == 0) return 0;
    memcpy(&a, bytes + 0x0c, sizeof(float));
    memcpy(&b, bytes + 0x10, sizeof(float));
    sample_time = (float)time;
    duration = resource_entry_id_get(record);
    high = b;
    if (b < a) high = a;
    low = b;
    if (a < b) low = a;
    if (0.5f <= (high - low) / (float)duration) {
        if (high <= sample_time || sample_time <= low) return 1;
    }
    else if (sample_time <= high && low <= sample_time) {
        return 1;
    }
    return 0;
}
// FUNCTION: LEGORR 0x00487fc0
float keyframe_time_segment_fraction(const void* track, float time, uint16_t* segment_out)
{
    const unsigned char* bytes = static_cast<const unsigned char*>(track);
    uint16_t count, i, *times;
    if (track == 0 || segment_out == 0) return 0.0f;
    memcpy(&count, bytes + 0x26, sizeof(count));
    if (count < 2) { *segment_out = (uint16_t)(count - 1); return 0.0f; }
    memcpy(&times, bytes + 0x20, sizeof(times));
    if (times == 0) { *segment_out = (uint16_t)(count - 1); return 0.0f; }
    for (i = 1; i < count; ++i) if (time <= (float)times[i]) {
        *segment_out = (uint16_t)(i - 1);
        return (time - (float)times[i - 1]) / ((float)times[i] - (float)times[i - 1]);
    }
    *segment_out = (uint16_t)(count - 1); return 0.0f;
}
// FUNCTION: LEGORR 0x00488070
/* SharedObjectFloatTrackInterpolateApply masks the key index to 16 bits,
 * uses the final value directly, or interpolates to the following key before
 * forwarding the result to the renderer's value-apply seam. */
void shared_object_float_track_interpolate_apply(
    void* owner, const void* track, uint32_t segment, float fraction)
{
    const unsigned char* bytes =
        static_cast<const unsigned char*>(track);
    const float* values;
    uint16_t count;
    float value;
    if (track == 0 || s_float_track_apply_hook == 0) return;
    memcpy(&count, bytes + 0x26, sizeof(count));
    if (count == 0) return;
    memcpy(&values, bytes + 0x1c, sizeof(values));
    if (values == 0) return;
    segment &= 0xffffu;
    value = values[segment];
    if (segment + 1u < count)
        value = (values[segment + 1u] - value) * fraction + value;
    s_float_track_apply_hook(owner, track, value,
                             s_float_track_apply_context);
}
} // namespace lego

// FUNCTION: LEGORR 0x00488170
/* AnimationTrackKeysInterpolate helper. */
void animation_track_keys_interpolate(void* owner, const void* track,
                                      uint32_t keyframe, float fraction)
{
    const unsigned char* bytes =
        static_cast<const unsigned char*>(track);
    const float* key;
    uint32_t index = keyframe & 0xffffu;
    float position[3];
    float rotation[3];
    float scale[3];
    unsigned int axis;
    if (owner == NULL || track == NULL) return;
    memcpy(&key, bytes + 0x18, sizeof(key));
    key = reinterpret_cast<const float*>(
        reinterpret_cast<const unsigned char*>(key) + index * 0x28u);
    for (axis = 0; axis < 3; ++axis) {
        position[axis] = key[axis] + (key[10 + axis] - key[axis]) * fraction;
        rotation[axis] = key[3 + axis] +
                         (key[13 + axis] - key[3 + axis]) * fraction;
        scale[axis] = key[6 + axis] + (key[16 + axis] - key[6 + axis]) * fraction;
    }
    scene_node_transform_matrix_apply(owner, track, position, rotation, scale);
}

/* SceneNodeTransformMatrixApply (FUN_004882e0). For flag bit 4, slot +0x5c
 * writes a transform interface to its out parameter; slot +0x74 reads the
 * current matrix through that interface into the local matrix; Release then
 * drops the temporary interface before scaling its diagonal and replacing
 * translation. */
// FUNCTION: LEGORR 0x004882e0
void scene_node_transform_matrix_apply(void* owner, const void* track,
                                       const float position[3],
                                       const float rotation[3],
                                       const float scale[3])
{
    const unsigned char* bytes =
        static_cast<const unsigned char*>(track);
    void** nodes;
    void* node;
    void** vtable;
    typedef void (VIEW_COM_CALL *MatrixSetMethod)(void*, uint32_t,
                                                  const float*);
    MatrixSetMethod set_matrix;
    uint16_t handle;
    float matrix[16];
    void* transform_interface;
    float sx, cx, sy, cy, sz, cz;
    float m3c, m40, m38, m44, m48, m10, m2c;
    float m30, m28, m1c, m20, m18;
    float offset[3];
    unsigned int i;
    if (owner == NULL || track == NULL || position == NULL ||
        rotation == NULL || scale == NULL) return;
    memcpy(&nodes, static_cast<unsigned char*>(owner) + 0x14,
           sizeof(nodes));
    memcpy(&handle, bytes + 6, sizeof(handle));
    if (nodes == NULL) return;
    node = nodes[handle];
    if (node == NULL) return;
    vtable = *static_cast<void***>(node);
    if (vtable == NULL || vtable[0x38 / 4] == NULL) return;
    set_matrix = reinterpret_cast<MatrixSetMethod>(vtable[0x38 / 4]);
    memset(matrix, 0, sizeof(matrix));
    if ((bytes[0x28] & 4u) != 0) {
        typedef void (VIEW_COM_CALL *TransformInterfaceGet)(void*, void**);
        typedef void (VIEW_COM_CALL *TransformMatrixGet)(void*, void*,
                                                         float*);
        typedef void (VIEW_COM_CALL *TransformInterfaceRelease)(void*);
        TransformInterfaceGet get_interface;
        TransformMatrixGet get_matrix;
        TransformInterfaceRelease release_interface;
        void** interface_vtable;
        if (vtable[0x5c / 4] == NULL || vtable[0x74 / 4] == NULL)
            return;
        transform_interface = NULL;
        get_interface = reinterpret_cast<TransformInterfaceGet>(
            vtable[0x5c / 4]);
        get_interface(node, &transform_interface);
        if (transform_interface == NULL) return;
        get_matrix = reinterpret_cast<TransformMatrixGet>(
            vtable[0x74 / 4]);
        get_matrix(node, transform_interface, matrix);
        interface_vtable = *static_cast<void***>(transform_interface);
        if (interface_vtable != NULL && interface_vtable[2] != NULL) {
            release_interface = reinterpret_cast<TransformInterfaceRelease>(
                interface_vtable[2]);
            release_interface(transform_interface);
        }
        matrix[0] *= scale[0];
        matrix[5] *= scale[1];
        matrix[10] *= scale[2];
        matrix[12] = position[0];
        matrix[13] = position[1];
        matrix[14] = position[2];
        matrix[15] = 1.0f;
    }
    else {
        memcpy(offset, bytes + 0x0c, sizeof(offset));
        sx = (float)sin((double)rotation[0]);
        cx = (float)cos((double)rotation[0]);
        sy = (float)sin((double)rotation[1]);
        cy = (float)cos((double)rotation[1]);
        sz = (float)sin((double)rotation[2]);
        cz = (float)cos((double)rotation[2]);
        m3c = scale[0] * sz;
        m40 = sx * (sy * m3c) + cx * (scale[0] * cz);
        m38 = cx * (sy * m3c) - sx * (scale[0] * cz);
        m3c = cy * m3c;
        m44 = scale[1] * cz;
        m48 = sy * m44;
        m10 = -(scale[1] * sz);
        m2c = cy * m44;
        m30 = sx * m48 + cx * m10;
        m28 = -(sx * m10) + cx * m48;
        m1c = -(scale[2] * sy);
        m20 = sx * (scale[2] * cy);
        m18 = cx * (scale[2] * cy);
        matrix[0] = m40; matrix[1] = m3c; matrix[2] = m38;
        matrix[4] = m30; matrix[5] = m2c; matrix[6] = m28;
        matrix[8] = m20; matrix[9] = m1c; matrix[10] = m18;
        matrix[12] = offset[0] * m40 + offset[1] * m30 +
                     offset[2] * m20 + position[0];
        matrix[13] = offset[0] * m3c + offset[1] * m2c +
                     offset[2] * m1c + position[1];
        matrix[14] = offset[0] * m38 + offset[1] * m28 +
                     offset[2] * m18 + position[2];
        matrix[15] = 1.0f;
    }
    set_matrix(node, 0, matrix);
}

/* ModelResourceResolveOrLoad (FUN_004885c0): concatenate prefix/path, use
 * the primary case-insensitive cache, load and cache on a miss, then retry
 * through the shared model root. The renderer instance operation remains a
 * backend seam; the portable cache owns parsed Model records. */
// FUNCTION: LEGORR 0x004885c0
Model* model_resource_resolve_or_load(const char* prefix, const char* path,
                                      void* flags, void* context)
{
    char resolved[260];
    Model* record;
    if (path == NULL) return NULL;
    model_resource_path_join(resolved, sizeof(resolved), prefix, path);
    record = (Model*)pair_table_find(s_model_resource_primary,
                                     (uint32_t)s_model_resource_primary_count,
                                     resolved);
    if (record != NULL) {
        if (s_model_resource_instance_hook != NULL)
            s_model_resource_instance_hook(record, flags);
        return record;
    }
    record = lwo_model_entry_load(resolved, flags, context);
    if (record != NULL) {
        /* Deviation: the corpus appends blindly past the 1000-entry
         * window; the port guards it. */
        if (s_model_resource_primary_count <
            (int)MODEL_RESOURCE_CACHE_CAPACITY)
            pair_table_append(s_model_resource_primary,
                              &s_model_resource_primary_count, resolved,
                              record);
        return record;
    }
    if (s_model_resource_fallback_prefix != NULL) {
        model_resource_path_join(resolved, sizeof(resolved),
                                 s_model_resource_fallback_prefix, path);
        record = (Model*)pair_table_find(
            s_model_resource_secondary,
            (uint32_t)s_model_resource_secondary_count, path);
        if (record != NULL) {
            if (s_model_resource_instance_hook != NULL)
                s_model_resource_instance_hook(record, flags);
            return record;
        }
        record = lwo_model_entry_load(resolved, flags, context);
        if (record != NULL) {
            if (s_model_resource_secondary_count <
                (int)MODEL_RESOURCE_CACHE_CAPACITY)
                pair_table_append(s_model_resource_secondary,
                                  &s_model_resource_secondary_count, path,
                                  record);
            return record;
        }
    }
    return NULL;
}

/* --- the pair-table window pair (see crt.hpp) ------------------------------
 * The find scan keeps the corpus's masked walk: the compare index is the
 * low 16 bits of the hop counter and the bound test reads that mask. It also
 * calls the shared strcasecmp_lite helper, as the corpus does.
 * Deviations: the port guards the table and the key; the corpus strides
 * them (and its append blindly stores past the caller's window). */
// FUNCTION: LEGORR 0x00488730
void* pair_table_find(const void* table, uint32_t count, const char* key)
{
    if (table == NULL || count == 0u || key == NULL) {
        return NULL; /* deviation: the corpus strides the table blindly */
    }
    const long field = (long)sizeof(void*);
    uint32_t hops = 0;
    for (;;) {
        const uint32_t idx = hops & 0xffffu; /* pinned: corpus walk mask */
        char* name = NULL;
        __builtin_memcpy(&name, (const char*)table + (long)idx * 2 * field,
                         sizeof name);
        void* value = NULL;
        __builtin_memcpy(&value,
                         (const char*)table + ((long)idx * 2 + 1) * field,
                         sizeof value);
        if (name != NULL && strcasecmp_lite(key, name) == 0) {
            return value;
        }
        ++hops;
        if ((hops & 0xffffu) >= count) {
            return NULL;
        }
    }
}

// FUNCTION: LEGORR 0x00488790
void pair_table_append(void* table, int* count, const char* key,
                       void* value)
{
    if (table == NULL || count == NULL) {
        return; /* deviation: the corpus strides both blindly */
    }
    const long field = (long)sizeof(void*);
    char* copy = string_duplicate(key); /* FUN_00477550 */
    char* entry = (char*)table + (long)*count * 2 * field;
    __builtin_memcpy(entry, &copy, sizeof copy);
    __builtin_memcpy(entry + field, &value, sizeof value);
    *count += 1;
}

namespace unported_ledger_stubs {
// SceneHierarchyInterfaceRegisterRecursive
// FUNCTION: LEGORR 0x004887c0
void ledger_stub_004887c0_SceneHierarchyInterfaceRegisterRecursive(
    unsigned short* next_index, void* root, void* node)
{
    scene_hierarchy_interface_register_recursive(next_index, root, node);
}

// FUNCTION: LEGORR 0x004889d0
uint32_t ledger_stub_004889d0_ResourceTreeCallbackReferenceChainRelease(
    ResourceReferenceChain* chain, ResourceReferenceReleaseHook release,
    void* context)
{
    uint32_t count = 0u;
    while (chain != NULL) {
        ResourceReferenceChain* next = chain->next;
        if (chain->resource != NULL && release != NULL)
            release(chain->resource, context);
        engine_heap_free(chain);
        chain = next;
        ++count;
    }
    return count;
}

// FUNCTION: LEGORR 0x00488a70
void ledger_stub_00488a70_LoadRecordInterfacesReleaseFree(
    LoadRecordInterfaceList* list, void (*release)(void* interface_ptr))
{
    uint32_t i;
    if (list == NULL) return;
    for (i = 0u; i < list->count; ++i)
        if (list->interfaces != NULL && list->interfaces[i] != NULL &&
            release != NULL) release(list->interfaces[i]);
    if (list->interfaces != NULL) engine_heap_free(list->interfaces);
    list->interfaces = NULL;
    list->count = 0u;
}
} // namespace unported_ledger_stubs

static int scene_entry_comma_split(char* text, char** tokens,
                                   int token_capacity)
{
    char* position;
    int count = 1;
    size_t separator_length = 1u;
    if (text == NULL || text[0] == '\0' || token_capacity <= 0) return 0;
    tokens[0] = text;
    for (position = text; position[1] != '\0'; ++position) {
        if (strncmp(",", position, separator_length) == 0) {
            if (count >= token_capacity) return count;
            *position = '\0';
            tokens[count++] = position + separator_length;
        }
    }
    return count;
}

namespace unported_ledger_stubs {
static void ledger_scene_interface_release(void* object)
{
    void** vtable;
    SceneInterfaceReleaseMethod release;
    if (object == NULL) return;
    vtable = *(void***)object;
    if (vtable == NULL || vtable[2] == NULL) return;
    release = (SceneInterfaceReleaseMethod)vtable[2];
    (void)release(object);
}
} // namespace unported_ledger_stubs

/* The reference's model resolver joins prefix and path with the raw
 * "%s%s" template (DAT_004a50d4) into a 260-byte stack path before its
 * case-insensitive pair-table lookup; no separator is inserted and a NULL
 * prefix copies the path verbatim.  Keep the same bounded contract without
 * snprintf, which is not part of the VC5 CRT. */
static void model_resource_path_join(char* output, size_t output_size,
                                     const char* prefix, const char* path)
{
    size_t position = 0u;
    const char* source;
    source = prefix;
    while (source != NULL && *source != '\0' &&
           position + 1u < output_size)
        output[position++] = *source++;
    source = path;
    while (*source != '\0' && position + 1u < output_size)
        output[position++] = *source++;
    output[position] = '\0';
}
