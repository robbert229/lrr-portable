// SPDX-License-Identifier: LGPL-3.0-or-later
#include "scene_resources.hpp"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "record.hpp"
#include "heap.hpp"
#include "view.hpp"
#include "crt/crt.hpp"
namespace lego {
namespace {
// GLOBAL: LEGORR 0x0076ad68
int s_slots_live = 0;
SceneResourceReleaseHook s_release_hook = 0;
void* s_release_context = 0;
void** s_string_table_a = 0;
void** s_string_table_b = 0;
unsigned int s_string_count_a = 0;
unsigned int s_string_count_b = 0;
SceneFloatTrackApplyHook s_float_track_apply_hook = 0;
void* s_float_track_apply_context = 0;
SceneAnimationTrackApplyHook s_animation_track_apply_hook = 0;
void* s_animation_track_apply_context = 0;
SceneNodeMatrixApplyHook s_node_matrix_apply_hook = 0;
void* s_node_matrix_apply_context = 0;
AnimatedTextureFrameApplyHook s_texture_frame_apply_hook = 0;
void* s_texture_frame_apply_context = 0;
char s_shared_texture_root[1024];
void* s_shared_texture_resources[20];
unsigned int s_shared_texture_resource_count = 0;
int s_shared_texture_initialized = 0;
SharedTextureRootLoadHook s_shared_texture_load_hook = 0;
void* s_shared_texture_load_context = 0;
}
void scene_resource_release_hook_set(SceneResourceReleaseHook hook,
                                     void* context)
{ s_release_hook = hook; s_release_context = context; }
// FUNCTION: LEGORR 0x00476d50
void scene_resource_slots_initialize()
{
    memset(s_render_surface_pool_blocks, 0,
           sizeof(s_render_surface_pool_blocks));
    s_render_surface_pool_free_head = 0;
    s_render_surface_pool_level_count = 0;
    s_slots_live = 1;
}
// FUNCTION: LEGORR 0x00476d80
void scene_resource_slots_release_all()
{
    unsigned int i;
    scene_resource_tracked_references_release_all();
    for (i = 0; i < SCENE_RESOURCE_SLOT_COUNT; ++i) {
        if (s_render_surface_pool_blocks[i] != 0)
            engine_heap_free(s_render_surface_pool_blocks[i]);
    }
    s_render_surface_pool_free_head = 0;
    s_slots_live = 0;
}
void* scene_resource_slot_get(unsigned int index)
{ return s_slots_live && index < SCENE_RESOURCE_SLOT_COUNT ? s_render_surface_pool_blocks[index] : 0; }
int scene_resource_slot_set(unsigned int index, void* resource)
{
    if (!s_slots_live || index >= SCENE_RESOURCE_SLOT_COUNT) return 0;
    s_render_surface_pool_blocks[index] = resource;
    return 1;
}
int animated_texture_frame_count(const void* texture)
{
    const unsigned char* record = static_cast<const unsigned char*>(texture);
    void* owner;
    void* direct_count;
    void* interface_object;
    void** vtable;
    void* frame_container;
    typedef void* (*FrameContainerGet)(void*);
    typedef int (*FrameCountGet)(void*);
    if (texture == 0) return 0;
    memcpy(&owner, record + 0x0c, sizeof(owner));
    if (owner == 0) return 0;
    memcpy(&direct_count, static_cast<unsigned char*>(owner) + 0x0c,
           sizeof(direct_count));
    if (direct_count != 0) {
        int count;
        memcpy(&count, direct_count, sizeof(count));
        return count;
    }
    memcpy(&interface_object, static_cast<unsigned char*>(owner) + 8,
           sizeof(interface_object));
    if (interface_object == 0) return 0;
    vtable = *static_cast<void***>(interface_object);
    if (vtable == 0) return 0;
    if (vtable[0x1c / 4] != 0) {
        FrameContainerGet get_container =
            reinterpret_cast<FrameContainerGet>(vtable[0x1c / 4]);
        frame_container = get_container(interface_object);
        if (frame_container != 0) {
            int count;
            memcpy(&count, static_cast<unsigned char*>(frame_container) + 4,
                   sizeof(count));
            return count + 1;
        }
    }
    if (vtable[0x58 / 4] != 0) {
        FrameCountGet get_count = reinterpret_cast<FrameCountGet>(
            vtable[0x58 / 4]);
        return get_count(interface_object);
    }
    return 0;
}

void animated_texture_frame_bind(void* texture, void* frame, int mode)
{
    unsigned char* record = static_cast<unsigned char*>(texture);
    void* owner;
    void* direct_count;
    void* interface_object;
    void** vtable;
    unsigned int selector = 0x40;
    typedef void (*FrameBind)(void*, void*, unsigned int);
    if (texture == 0) return;
    memcpy(&owner, record + 0x0c, sizeof(owner));
    if (owner == 0) return;
    memcpy(&direct_count, static_cast<unsigned char*>(owner) + 0x0c,
           sizeof(direct_count));
    if (direct_count != 0) return;
    memcpy(&interface_object, static_cast<unsigned char*>(owner) + 8,
           sizeof(interface_object));
    if (interface_object == 0) return;
    if (mode != 0) selector = (unsigned int)(uintptr_t)frame;
    if (mode == 1) selector = 0x80;
    if (mode == 2) selector = 0x88;
    if (mode == 3) selector = 0x89;
    vtable = *static_cast<void***>(interface_object);
    if (vtable == 0 || vtable[0x4c / 4] == 0) return;
    {
        FrameBind bind = reinterpret_cast<FrameBind>(vtable[0x4c / 4]);
        bind(interface_object, frame, selector);
    }
}

int animated_texture_frame_resolve(void* interface_object, int frame,
                                   void** resolved_out)
{
    void** vtable;
    void* frame_container;
    typedef void* (*FrameContainerGet)(void*);
    if (resolved_out != 0) *resolved_out = 0;
    if (interface_object == 0) return 0;
    vtable = *static_cast<void***>(interface_object);
    if (vtable == 0 || vtable[0x1c / 4] == 0) return 0;
    {
        FrameContainerGet get_container =
            reinterpret_cast<FrameContainerGet>(vtable[0x1c / 4]);
        frame_container = get_container(interface_object);
    }
    if (frame_container == 0) return 0;
    if (resolved_out != 0 && frame != 0) {
        void* value;
        memcpy(&value, static_cast<unsigned char*>(frame_container) - 4 +
               (size_t)frame * sizeof(void*), sizeof(value));
        *resolved_out = value;
    }
    return 1;
}
void animated_texture_frame_apply_hook_set(AnimatedTextureFrameApplyHook hook,
                                           void* context)
{
    s_texture_frame_apply_hook = hook;
    s_texture_frame_apply_context = context;
}
/* AnimatedTextureFrameSurfaceSet (FUN_004830c0): the frame state's slot-zero
 * method is called with the record's shared-surface selector word and the
 * indexed frame record (records + 0x60 + frame * 0x70); the portable port
 * routes that call through the installed apply hook.  The state's third
 * dword then raises or clears the 0x00400000 shared-surface bit in the
 * frame record's flag word at +0x0c. */
// FUNCTION: LEGORR 0x004830c0
void animated_texture_frame_surface_set(void* texture, int frame_index,
                                        const void* frame_state)
{
    unsigned char* record = static_cast<unsigned char*>(texture);
    const unsigned char* state =
        static_cast<const unsigned char*>(frame_state);
    unsigned char* records;
    void* interface_object = 0;
    unsigned int flags;
    unsigned char* frame_record;
    if (record == 0 || state == 0 || frame_index < 0) return;
    memcpy(&records, record + 8, sizeof(records));
    if (records == 0) return;
    frame_record = records + 0x60 + (size_t)frame_index * 0x70;
    memcpy(&interface_object, state, sizeof(interface_object));
    if (interface_object != 0 && s_texture_frame_apply_hook != 0)
        s_texture_frame_apply_hook(interface_object, frame_record,
                                   s_texture_frame_apply_context);
    memcpy(&flags, frame_record + 0x0c, sizeof(flags));
    if (((const uint32_t*)state)[2] != 0u) flags |= 0x00400000u;
    else flags &= ~0x00400000u;
    memcpy(frame_record + 0x0c, &flags, sizeof(flags));
}
void shared_texture_root_load_hook_set(SharedTextureRootLoadHook hook,
                                       void* context)
{
    s_shared_texture_load_hook = hook;
    s_shared_texture_load_context = context;
}
// FUNCTION: LEGORR 0x004805b0
int shared_texture_root_register(const char* root)
{
    /* SharedTextureRootRegister drops the shared-pool link and releases the
     * model-render list (DAT_005343c4 + FUN_00482d30) before re-seeding;
     * crt.cpp owns that list head, so the walk-and-free runs there. */
    model_render_list_clear();
    if (root != 0) {
        size_t length = strlen(root);
        size_t written;
        if (length >= sizeof(s_shared_texture_root))
            length = sizeof(s_shared_texture_root) - 1u; /* port bound */
        written = length;
        /* Reference quirk: the final stored byte is always the appended
         * backslash, replacing whatever was copied there; when the byte
         * two before the end is already a backslash the root loses one
         * extra byte.  The reference also repoints the model-render list
         * head (DAT_00534558) at this freshly allocated string; this port
         * keeps the root in the module static instead. */
        if (length >= 2u && root[length - 2u] == '\\')
            written = length - 1u;
        if (length != 0u) memcpy(s_shared_texture_root, root, length);
        if (written != 0u) s_shared_texture_root[written - 1u] = '\\';
        s_shared_texture_root[written] = '\0';
    }
    /* The reference zeroes the twenty-slot table (DAT_00540584) and the two
     * trailing state words (DAT_005405d4/d8) the port models as the count
     * and initialized flag. */
    memset(s_shared_texture_resources, 0, sizeof(s_shared_texture_resources));
    s_shared_texture_resource_count = 20u;
    s_shared_texture_initialized = 0;
    (void)shared_texture_root_initialize();
    return 1;
}
/* Port adapter for SharedTextureRootInitialize. Retail takes no arguments
 * but consumes renderer-owned Direct3D resource arrays and COM interfaces;
 * the portable helper can only refill resources through an installed hook. */
int shared_texture_root_initialize()
{
    /* SharedTextureRootInitialize releases the resident slot-node chains
     * (FUN_00480780/FUN_004807a0 over the DAT_005405d8 and DAT_005343ac
     * windows) and refills the twenty slots under the shared texture root.
     * The portable port models those chains as loader-owned, so the slot
     * refill runs through the installed load hook instead. */
    unsigned int i;
    s_shared_texture_initialized = 0;
    memset(s_shared_texture_resources, 0, sizeof(s_shared_texture_resources));
    if (s_shared_texture_load_hook == 0) return 0;
    for (i = 0; i < 20u; ++i) {
        void* resource = 0;
        if (!s_shared_texture_load_hook(s_shared_texture_root, i, &resource,
                                        s_shared_texture_load_context)) {
            return 0;
        }
        s_shared_texture_resources[i] = resource;
    }
    s_shared_texture_initialized = 1;
    return 1;
}
const char* shared_texture_root_get()
{
    return s_shared_texture_root;
}
void* shared_texture_root_resource_get(unsigned int index)
{
    if (!s_shared_texture_initialized || index >= s_shared_texture_resource_count)
        return 0;
    return s_shared_texture_resources[index];
}
void scene_resource_tracked_references_release_all()
{
    unsigned int pool;
    typedef uint32_t (*ReleaseMethod)(void*);
    for (pool = 0; pool < ddiscene_render_surface_pool_block_count(); ++pool) {
        unsigned char* records = static_cast<unsigned char*>(
            ddiscene_render_surface_pool_block_get(pool));
        unsigned int index;
        unsigned int record_count = 1u << pool;
        if (records == 0) continue;
        for (index = 0; index < record_count; ++index) {
            size_t stride = (size_t)0x20u + sizeof(void*) - sizeof(uint32_t);
            unsigned char* record = records + (size_t)index * stride;
            void* self_link;
            void* interface_object;
            void** vtable;
            memcpy(&self_link, record + 0x1c, sizeof(self_link));
            if (self_link != record) continue;
            memcpy(&interface_object, record + 0x10,
                   sizeof(interface_object));
            if (interface_object == 0) continue;
            vtable = *static_cast<void***>(interface_object);
            if (vtable != 0 && vtable[2] != 0) {
                ReleaseMethod release =
                    reinterpret_cast<ReleaseMethod>(vtable[2]);
                release(interface_object);
            }
        }
    }
}
void scene_float_track_apply_hook_set(SceneFloatTrackApplyHook hook,
                                      void* context)
{
    s_float_track_apply_hook = hook;
    s_float_track_apply_context = context;
}
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
    if (segment >= count) segment = (uint32_t)count - 1u;
    value = values[segment];
    if (segment + 1u < count)
        value = (values[segment + 1u] - value) * fraction + value;
    s_float_track_apply_hook(owner, track, value,
                             s_float_track_apply_context);
}
void scene_animation_track_apply_hook_set(SceneAnimationTrackApplyHook hook,
                                          void* context)
{
    s_animation_track_apply_hook = hook;
    s_animation_track_apply_context = context;
}
void animation_track_keys_interpolate_apply(void* owner, const void* track,
                                            uint32_t segment, float fraction)
{
    const unsigned char* bytes =
        static_cast<const unsigned char*>(track);
    const float* keys;
    uint16_t count;
    float position[3];
    float rotation[3];
    float scale[3];
    unsigned int i;
    if (track == 0 || s_animation_track_apply_hook == 0) return;
    memcpy(&count, bytes + 0x26, sizeof(count));
    if (count < 2) return;
    memcpy(&keys, bytes + 0x18, sizeof(keys));
    if (keys == 0) return;
    segment &= 0xffffu;
    if (segment >= (uint32_t)count - 1u) segment = (uint32_t)count - 2u;
    keys = (const float*)((const unsigned char*)keys +
                          (size_t)segment * 0x28u);
    for (i = 0; i < 3; ++i) {
        position[i] = keys[i] + (keys[10 + i] - keys[i]) * fraction;
        rotation[i] = keys[3 + i] +
                      (keys[13 + i] - keys[3 + i]) * fraction;
        scale[i] = keys[6 + i] + (keys[16 + i] - keys[6 + i]) * fraction;
    }
    s_animation_track_apply_hook(owner, track, position, rotation, scale,
                                 s_animation_track_apply_context);
}
void scene_node_matrix_apply_hook_set(SceneNodeMatrixApplyHook hook,
                                      void* context)
{
    s_node_matrix_apply_hook = hook;
    s_node_matrix_apply_context = context;
}
void scene_node_transform_matrix_apply(void* owner, const void* track,
    const float position[3], const float rotation[3], const float scale[3])
{
    const unsigned char* bytes =
        static_cast<const unsigned char*>(track);
    unsigned char flags;
    float matrix[16];
    float offset_x = 0.0f;
    float offset_y = 0.0f;
    float offset_z = 0.0f;
    if (track == 0 || position == 0 || rotation == 0 || scale == 0 ||
        s_node_matrix_apply_hook == 0) return;
    flags = bytes[0x28];
    memcpy(&offset_x, bytes + 0x0c, sizeof(offset_x));
    memcpy(&offset_y, bytes + 0x10, sizeof(offset_y));
    memcpy(&offset_z, bytes + 0x14, sizeof(offset_z));
    if ((flags & 4u) != 0) {
        memset(matrix, 0, sizeof(matrix));
        matrix[0] = scale[0];
        matrix[5] = scale[1];
        matrix[10] = scale[2];
        matrix[12] = position[0];
        matrix[13] = position[1];
        matrix[14] = position[2];
        matrix[15] = 1.0f;
    }
    else {
        float sin_x = (float)sin((double)rotation[0]);
        float cos_x = (float)cos((double)rotation[0]);
        float sin_y = (float)sin((double)rotation[1]);
        float cos_y = (float)cos((double)rotation[1]);
        float sin_z = (float)sin((double)rotation[2]);
        float cos_z = (float)cos((double)rotation[2]);
        float m3c = scale[0] * sin_z * cos_y;
        float m40 = sin_x * (sin_y * m3c)
                  + cos_x * (scale[0] * cos_z);
        float m38 = cos_x * (sin_y * m3c)
                  - sin_x * (scale[0] * cos_z);
        float m44 = scale[1] * cos_z;
        float m48 = sin_y * m44;
        float m10 = -(scale[1] * sin_z);
        float m2c = cos_y * m44;
        float m30 = sin_x * m48 + cos_x * m10;
        float m28 = -(sin_x * m10) + cos_x * m48;
        float m1c = -(scale[2] * sin_y);
        float m20 = sin_x * (scale[2] * cos_y);
        float m18 = cos_x * (scale[2] * cos_y);
        memset(matrix, 0, sizeof(matrix));
        matrix[0] = m40; matrix[1] = m3c; matrix[2] = m38;
        matrix[4] = m30; matrix[5] = m2c; matrix[6] = m28;
        matrix[8] = m20; matrix[9] = m1c; matrix[10] = m18;
        matrix[12] = offset_x * m40 + offset_y * m30 +
                     offset_z * m20 + position[0];
        matrix[13] = offset_x * m3c + offset_y * m2c +
                     offset_z * m1c + position[1];
        matrix[14] = offset_x * m38 + offset_y * m28 +
                     offset_z * m18 + position[2];
        matrix[15] = 1.0f;
    }
    s_node_matrix_apply_hook(owner, track, matrix,
                             s_node_matrix_apply_context);
}
void scene_resource_string_tables_set(void** first, unsigned int first_count,
                                      void** second, unsigned int second_count)
{ s_string_table_a = first; s_string_count_a = first_count; s_string_table_b = second; s_string_count_b = second_count; }
// FUNCTION: LEGORR 0x00487760
void scene_resource_string_tables_free()
{
    unsigned int i;
    if (s_release_hook != 0) {
        for (i = 0; i < s_string_count_a; ++i)
            if (s_string_table_a != 0 && s_string_table_a[i] != 0) s_release_hook(s_string_table_a[i], s_release_context);
        for (i = 0; i < s_string_count_b; ++i)
            if (s_string_table_b != 0 && s_string_table_b[i] != 0) s_release_hook(s_string_table_b[i], s_release_context);
    }
    s_string_table_a = 0; s_string_table_b = 0; s_string_count_a = 0; s_string_count_b = 0;
}
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
// FUNCTION: LEGORR 0x00476f50
void resource_interface_call_slot48(void* wrapper, void* value)
{
    void* interface_object;
    void** vtable;
    typedef void (*Method)(void*, void*);
    if (wrapper == 0) return;
    memcpy(&interface_object, static_cast<unsigned char*>(wrapper) + 0x10, sizeof(void*));
    if (interface_object == 0) return;
    vtable = *static_cast<void***>(interface_object);
    if (vtable != 0 && vtable[0x48 / 4] != 0) {
        Method method = reinterpret_cast<Method>(vtable[0x48 / 4]);
        method(interface_object, value);
    }
}
static void resource_interface_call_noargs(void* wrapper, unsigned int byte_offset)
{
    void* interface_object;
    void** vtable;
    typedef void (*Method)(void*);
    if (wrapper == 0) return;
    memcpy(&interface_object, static_cast<unsigned char*>(wrapper) + 0x10, sizeof(void*));
    if (interface_object == 0) return;
    vtable = *static_cast<void***>(interface_object);
    if (vtable != 0 && vtable[byte_offset / 4] != 0) {
        Method method = reinterpret_cast<Method>(vtable[byte_offset / 4]);
        method(interface_object);
    }
}
// FUNCTION: LEGORR 0x00476fd0
void resource_interface_call_slot8c(void* wrapper) { resource_interface_call_noargs(wrapper, 0x8c); }
// FUNCTION: LEGORR 0x00476ff0
void resource_interface_call_slot90(void* wrapper) { resource_interface_call_noargs(wrapper, 0x90); }
}
