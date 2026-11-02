// SPDX-License-Identifier: LGPL-3.0-or-later
#include "scene_resources.hpp"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "record.hpp"
#include "model.hpp"
#include "heap.hpp"
#include "view.hpp"
#include "crt/crt.hpp"
#if defined(_MSC_VER)
#define SCENE_RESOURCE_COM_CALL __stdcall
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#define SCENE_RESOURCE_COM_CALL __attribute__((stdcall))
#else
#define SCENE_RESOURCE_COM_CALL
#endif
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
SharedTextureRootInitializeHook s_shared_texture_initialize_hook = 0;
void* s_shared_texture_initialize_context = 0;
CachedRenderResourceRendererHook s_cached_resource_renderer_hook = 0;
void* s_cached_resource_renderer_context = 0;
CachedRenderResourceRendererReleaseHook s_cached_resource_renderer_release_hook = 0;
void* s_cached_resource_renderer_release_context = 0;
}
static InterfaceArrayForwardSyncHooks s_interface_array_forward_sync_hooks;
static void** s_interface_forward_sync_items;
static uint32_t s_interface_forward_sync_items_capacity;
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
    free(s_interface_forward_sync_items);
    s_interface_forward_sync_items = NULL;
    s_interface_forward_sync_items_capacity = 0u;
}
void* scene_resource_slot_get(unsigned int index)
{ return s_slots_live && index < SCENE_RESOURCE_SLOT_COUNT ? s_render_surface_pool_blocks[index] : 0; }
int scene_resource_slot_set(unsigned int index, void* resource)
{
    if (!s_slots_live || index >= SCENE_RESOURCE_SLOT_COUNT) return 0;
    s_render_surface_pool_blocks[index] = resource;
    return 1;
}
// FUNCTION: LEGORR 0x00474a20
int animated_texture_frame_count(const void* texture)
{
    const unsigned char* record = static_cast<const unsigned char*>(texture);
    void* owner;
    void* direct_count;
    void* interface_object;
    void** vtable;
    void* frame_container;
    typedef void* (SCENE_RESOURCE_COM_CALL *FrameContainerGet)(void*);
    typedef int (SCENE_RESOURCE_COM_CALL *FrameCountGet)(void*);
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

// FUNCTION: LEGORR 0x00474a60
void animated_texture_frame_bind(void* texture, uint32_t frame_index, int mode)
{
    unsigned char* record = static_cast<unsigned char*>(texture);
    void* owner;
    void* direct_count;
    void* interface_object;
    void* resolved_interface;
    int resolved_index;
    void** vtable;
    unsigned int selector = 0x40;
    typedef void (SCENE_RESOURCE_COM_CALL *FrameBind)(
        void*, void*, unsigned int);
    if (texture == 0) return;
    memcpy(&owner, record + 0x0c, sizeof(owner));
    if (owner == 0) return;
    memcpy(&direct_count, static_cast<unsigned char*>(owner) + 0x0c,
           sizeof(direct_count));
    if (direct_count != 0) return;
    memcpy(&interface_object, static_cast<unsigned char*>(owner) + 8,
           sizeof(interface_object));
    if (interface_object == 0) return;
    if (mode != 0) selector = frame_index;
    if (mode == 1) selector = 0x80;
    if (mode == 2) selector = 0x88;
    if (mode == 3) selector = 0x89;
    resolved_interface = interface_object;
    resolved_index = (int)frame_index;
    (void)animated_texture_frame_resolve(&resolved_interface,
                                         &resolved_index);
    vtable = *static_cast<void***>(interface_object);
    if (vtable == 0 || vtable[0x4c / 4] == 0) return;
    {
        FrameBind bind = reinterpret_cast<FrameBind>(vtable[0x4c / 4]);
        bind(interface_object, resolved_interface, selector);
    }
}

// FUNCTION: LEGORR 0x00474c00
int animated_texture_frame_resolve(void** interface_io,
                                   int* frame_index_io)
{
    void* interface_object;
    void** vtable;
    void* frame_container;
    void* frame_array;
    int frame_index;
    int frame_count;
    typedef void* (SCENE_RESOURCE_COM_CALL *FrameContainerGet)(void*);
    if (interface_io == 0 || frame_index_io == 0) return 0;
    interface_object = *interface_io;
    if (interface_object == 0) return 0;
    vtable = *static_cast<void***>(interface_object);
    if (vtable == 0 || vtable[0x1c / 4] == 0) return 0;
    {
        FrameContainerGet get_container =
            reinterpret_cast<FrameContainerGet>(vtable[0x1c / 4]);
        frame_container = get_container(interface_object);
    }
    if (frame_container == 0) return 0;
    frame_index = *frame_index_io;
    if (frame_index != 0) {
        if (frame_index < 1) return 0;
        memcpy(&frame_count, static_cast<unsigned char*>(frame_container) + 4,
               sizeof(frame_count));
        if (frame_index > frame_count) return 0;
        memcpy(&frame_array, frame_container, sizeof(frame_array));
        if (frame_array == 0) return 0;
        memcpy(interface_io, static_cast<unsigned char*>(frame_array) +
               (size_t)(frame_index - 1) * sizeof(void*),
               sizeof(*interface_io));
    }
    *frame_index_io = 0;
    return 1;
}

// FUNCTION: LEGORR 0x00474da0
void animated_texture_frame_set(void* texture, uint32_t frame_index,
                                const void* frame_state)
{
    unsigned char* texture_record = (unsigned char*)texture;
    void* owner;
    void* direct_pool;
    void* interface_object;
    void* frame_interface = NULL;
    void** vtable;
    int resolved_index = (int)frame_index;
    typedef int32_t (SCENE_RESOURCE_COM_CALL *QueryInterfaceFn)(
        void*, const void*, void**);
    typedef uint32_t (SCENE_RESOURCE_COM_CALL *ReleaseFn)(void*);
    typedef void (SCENE_RESOURCE_COM_CALL *FrameSurfaceSetFn)(
        void*, uint32_t, void*);
    static const unsigned char frame_surface_iid[16] = {
        0x09, 0xcb, 0x16, 0xeb, 0x71, 0xd2, 0xce, 0x11,
        0xac, 0x48, 0x00, 0x00, 0xc0, 0x38, 0x25, 0xa1
    };

    if (texture_record == NULL) return;
    memcpy(&owner, texture_record + 0x0c, sizeof(owner));
    if (owner == NULL) return;
    memcpy(&direct_pool, static_cast<unsigned char*>(owner) + 0x0c,
           sizeof(direct_pool));
    if (direct_pool != NULL) {
        animated_texture_frame_surface_set(direct_pool, (int)frame_index,
                                           frame_state);
        return;
    }
    memcpy(&interface_object, static_cast<unsigned char*>(owner) + 8,
           sizeof(interface_object));
    if (interface_object == NULL) return;
    (void)animated_texture_frame_resolve(&interface_object,
                                         &resolved_index);
    if (frame_state != NULL) {
        void* source_interface = NULL;
        memcpy(&source_interface,
               static_cast<const unsigned char*>(frame_state) + 4,
               sizeof(source_interface));
        if (source_interface != NULL) {
            void** source_vtable = *static_cast<void***>(source_interface);
            if (source_vtable != NULL && source_vtable[0] != NULL) {
                QueryInterfaceFn query =
                    reinterpret_cast<QueryInterfaceFn>(source_vtable[0]);
                (void)query(source_interface, frame_surface_iid,
                            &frame_interface);
            }
        }
    }
    vtable = *static_cast<void***>(interface_object);
    if (vtable != NULL && vtable[0x54 / 4] != NULL) {
        FrameSurfaceSetFn set_surface =
            reinterpret_cast<FrameSurfaceSetFn>(vtable[0x54 / 4]);
        set_surface(interface_object, (uint32_t)resolved_index,
                    frame_interface);
    }
    if (frame_interface != NULL) {
        void** frame_vtable = *static_cast<void***>(frame_interface);
        if (frame_vtable != NULL && frame_vtable[2] != NULL) {
            ReleaseFn release = reinterpret_cast<ReleaseFn>(frame_vtable[2]);
            (void)release(frame_interface);
        }
    }
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
void shared_texture_root_initialize_hook_set(
    SharedTextureRootInitializeHook hook, void* context)
{
    s_shared_texture_initialize_hook = hook;
    s_shared_texture_initialize_context = context;
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
        /* The reference checks the final input character: if the root
         * lacks a trailing backslash, it appends one.  The reference also
         * repoints the model-render list
         * head (DAT_00534558) at this freshly allocated string; this port
         * keeps the root in the module static instead. */
        if (length == 0u || root[length - 1u] != '\\') {
            if (length >= sizeof(s_shared_texture_root) - 1u)
                length = sizeof(s_shared_texture_root) - 2u;
            written = length + 1u;
        }
        if (length != 0u) memcpy(s_shared_texture_root, root, length);
        s_shared_texture_root[written - 1u] = '\\';
        s_shared_texture_root[written] = '\0';
    }
    /* DAT_00540584/D4/D8 are the render-resource instance pool owned by
     * record.cpp. TextureGetOrCreate keys cached shared resources by their
     * complete path, so changing this root does not require cache eviction. */
    render_resource_instance_pool_reset();
    (void)shared_texture_root_initialize();
    return 1;
}
/* SharedTextureRootInitialize uses the Direct3D interface globals and COM
 * methods documented in LEGORR.full.c:86278. There is no portable texture
 * resource loop here; the renderer owner may install that exact operation. */
int shared_texture_root_initialize()
{
    if (s_shared_texture_initialize_hook == 0) return 0;
    return s_shared_texture_initialize_hook(s_shared_texture_initialize_context);
}
const char* shared_texture_root_get()
{
    return s_shared_texture_root;
}

// FUNCTION: LEGORR 0x004773e0
void scene_resource_tracked_references_release_all()
{
    unsigned int pool;
    typedef uint32_t (SCENE_RESOURCE_COM_CALL *ReleaseMethod)(void*);
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
    float position[3];
    float rotation[3];
    float scale[3];
    unsigned int i;
    if (track == 0) return;
    memcpy(&keys, bytes + 0x18, sizeof(keys));
    if (keys == 0) return;
    segment &= 0xffffu;
    keys = (const float*)((const unsigned char*)keys +
                          (size_t)segment * 0x28u);
    for (i = 0; i < 3; ++i) {
        position[i] = keys[i] + (keys[10 + i] - keys[i]) * fraction;
        rotation[i] = keys[3 + i] +
                      (keys[13 + i] - keys[3 + i]) * fraction;
        scale[i] = keys[6 + i] + (keys[16 + i] - keys[6 + i]) * fraction;
    }
    if (s_animation_track_apply_hook != 0) {
        s_animation_track_apply_hook(owner, track, position, rotation, scale,
                                     s_animation_track_apply_context);
    }
    else {
        /* Use the mapped scene-node implementation directly. Its owner node
         * table and COM transform methods are the data consumed by the retail
         * target; the optional adapter hook is only for host overrides. */
        ::scene_node_transform_matrix_apply(owner, track, position, rotation,
                                            scale);
    }
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
    typedef void (SCENE_RESOURCE_COM_CALL *Method)(void*, void*);
    if (wrapper == 0) return;
    memcpy(&interface_object, static_cast<unsigned char*>(wrapper) + 0x10, sizeof(void*));
    if (interface_object == 0) return;
    vtable = *static_cast<void***>(interface_object);
    if (vtable != 0 && vtable[0x48 / 4] != 0) {
        Method method = reinterpret_cast<Method>(vtable[0x48 / 4]);
        method(interface_object, value);
    }
}
#if !defined(_MSC_VER) || !defined(_M_IX86)
static void resource_interface_call_noargs(void* wrapper, unsigned int byte_offset)
{
    void* interface_object;
    void** vtable;
    typedef void (SCENE_RESOURCE_COM_CALL *Method)(void*);
    if (wrapper == 0) return;
    memcpy(&interface_object, static_cast<unsigned char*>(wrapper) + 0x10, sizeof(void*));
    if (interface_object == 0) return;
    vtable = *static_cast<void***>(interface_object);
    if (vtable != 0 && vtable[byte_offset / 4] != 0) {
        Method method = reinterpret_cast<Method>(vtable[byte_offset / 4]);
        method(interface_object);
    }
}
#endif
// FUNCTION: LEGORR 0x00476fd0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void resource_interface_call_slot8c(void* wrapper)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov eax, dword ptr [eax + 10h]
        push eax
        mov ecx, dword ptr [eax]
        call dword ptr [ecx + 8ch]
        ret
    }
}
#else
void resource_interface_call_slot8c(void* wrapper)
{
    resource_interface_call_noargs(wrapper, 0x8c);
}
#endif
// FUNCTION: LEGORR 0x00476ff0
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void resource_interface_call_slot90(void* wrapper)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        mov eax, dword ptr [eax + 10h]
        push eax
        mov ecx, dword ptr [eax]
        call dword ptr [ecx + 90h]
        ret
    }
}
#else
void resource_interface_call_slot90(void* wrapper)
{
    resource_interface_call_noargs(wrapper, 0x90);
}
#endif

void cached_render_resource_renderer_hook_set(
    CachedRenderResourceRendererHook hook, void* context)
{
    s_cached_resource_renderer_hook = hook;
    s_cached_resource_renderer_context = context;
}

void cached_render_resource_renderer_release_hook_set(
    CachedRenderResourceRendererReleaseHook hook, void* context)
{
    s_cached_resource_renderer_release_hook = hook;
    s_cached_resource_renderer_release_context = context;
}

// FUNCTION: LEGORR 0x00480900
CachedRenderResourceInstanceRecord* cached_render_resource_instance_create(
    CachedRenderResourceTemplateRecord* source)
{
    CachedRenderResourceTemplateRecord* template_record;
    CachedRenderResourceInstanceRecord* instance;
    uint32_t count;
    uint32_t index;
    size_t bytes;
    void* pool_record;
    if (source == NULL) return NULL;

    template_record = source->shared_template != NULL
        ? source->shared_template : source;
    count = template_record->words[0];
    if (count > 0x7fffffffu / 0x70u) return NULL;
    bytes = (size_t)count * 0x70u;

    pool_record = render_resource_instance_record_acquire();
    if (pool_record == NULL) return NULL;
    instance = (CachedRenderResourceInstanceRecord*)engine_heap_alloc(
        (long)sizeof(*instance));
    if (instance == NULL) {
        render_resource_instance_record_release(pool_record);
        return NULL;
    }
    memset(instance, 0, sizeof(*instance));
    memcpy(instance->words, template_record->words,
           sizeof(instance->words));
    instance->pool_record = pool_record;
    instance->words[1] = count;
    instance->words[2] = 0u;
    instance->words[3] = 0u;
    instance->words[8] = 0u;
    instance->words[9] = 0u;
    instance->words[10] = 0u;
    memcpy(pool_record, instance->words, sizeof(instance->words));
    instance->source_template = template_record;

    if (bytes != 0u) {
        if (template_record->entry_block == NULL) {
            render_resource_instance_record_release(pool_record);
            engine_heap_free(instance);
            return NULL;
        }
        instance->entry_block = (unsigned char*)engine_heap_alloc(
            (long)bytes);
        if (instance->entry_block == NULL) {
            render_resource_instance_record_release(pool_record);
            engine_heap_free(instance);
            return NULL;
        }
        /* The source pointer includes a 0x10-byte header. Retail copies
         * count * 0x70 bytes from it, then initializes and copies each
         * per-entry 0x50-byte transform window at +0x10. */
        memcpy(instance->entry_block, template_record->entry_block, bytes);
        for (index = 0u; index < count; ++index) {
            unsigned char* destination = instance->entry_block + 0x10u
                + (size_t)index * 0x70u;
            const unsigned char* entry_source =
                template_record->entry_block + 0x10u
                + (size_t)index * 0x70u;
            uint32_t transform[20];
            unsigned int word;
            transform[0] = 0x50u;
            transform[1] = 0x3f800000u;
            transform[2] = 0x3f800000u;
            transform[3] = 0x3f800000u;
            transform[4] = 0x3f800000u;
            for (word = 5u; word < 20u; ++word) transform[word] = 0u;
            memcpy(destination, transform, sizeof(transform));
            memcpy(destination, entry_source, sizeof(transform));
        }
    }

    /* These renderer calls create and attach the wrapped interface. Their
     * COM vtables belong to the platform renderer; the hook updates the
     * host-side handles without guessing those ABI signatures here. */
    if (s_cached_resource_renderer_hook != NULL)
        s_cached_resource_renderer_hook(instance,
                                        s_cached_resource_renderer_context);

    ++template_record->words[8];
    template_record->words[11] |= 0x00200000u;
    return instance;
}

void cached_render_resource_instance_dispose(
    CachedRenderResourceInstanceRecord* instance)
{
    if (instance == NULL) return;
    if (s_cached_resource_renderer_release_hook != NULL)
        s_cached_resource_renderer_release_hook(
            instance, s_cached_resource_renderer_release_context);
    engine_heap_free(instance->entry_block);
    instance->entry_block = NULL;
    render_resource_instance_record_release(instance->pool_record);
    instance->pool_record = NULL;
    engine_heap_free(instance);
}
}

namespace unported_ledger_stubs {
// SharedTextureRootRegister
int ledger_stub_004805b0_SharedTextureRootRegister(const char* root)
{
    return lego::shared_texture_root_register(root);
}

// SharedTextureRootInitialize
int ledger_stub_00480650_SharedTextureRootInitialize()
{
    return lego::shared_texture_root_initialize();
}

// CachedRenderResourceInstanceCreate
lego::CachedRenderResourceInstanceRecord*
ledger_stub_00480900_CachedRenderResourceInstanceCreate(
    lego::CachedRenderResourceTemplateRecord* source)
{
    return lego::cached_render_resource_instance_create(source);
}
}

namespace lego {
static int interface_forward_sync_items_reserve(uint32_t count)
{
    uint32_t capacity;
    void** items;
    size_t bytes;
    if (count <= s_interface_forward_sync_items_capacity) return 1;
    capacity = s_interface_forward_sync_items_capacity != 0u
        ? s_interface_forward_sync_items_capacity : 4u;
    while (capacity < count) {
        if (capacity > 0x7fffffffu / 2u) {
            capacity = count;
            break;
        }
        capacity *= 2u;
    }
    if ((size_t)capacity > (size_t)-1 / sizeof(void*)) return 0;
    bytes = (size_t)capacity * sizeof(void*);
    items = (void**)malloc(bytes);
    if (items == NULL) return 0;
    if (s_interface_forward_sync_items != NULL) {
        memcpy(items, s_interface_forward_sync_items,
               (size_t)s_interface_forward_sync_items_capacity *
                   sizeof(void*));
        free(s_interface_forward_sync_items);
    }
    s_interface_forward_sync_items = items;
    s_interface_forward_sync_items_capacity = capacity;
    return 1;
}

static void interface_array_forward_sync_slot80(void* object,
                                                 uint32_t* count,
                                                 void** items)
{
    void** vtable;
    typedef void (SCENE_RESOURCE_COM_CALL *Method)(void*, void*, void*);
    if (object == NULL || count == NULL) return;
    if (s_interface_array_forward_sync_hooks.enumerate != NULL) {
        s_interface_array_forward_sync_hooks.enumerate(
            s_interface_array_forward_sync_hooks.context, object, count,
            items);
        return;
    }
    memcpy(&vtable, object, sizeof(vtable));
    if (vtable == NULL || vtable[0x80 / 4] == NULL) return;
    ((Method)vtable[0x80 / 4])(object, count, items);
}

static void interface_array_forward_sync_slot(void* object,
                                               unsigned int byte_offset,
                                               void* value)
{
    void** vtable;
    typedef void (SCENE_RESOURCE_COM_CALL *Method)(void*, void*);
    if (object == NULL) return;
    if (s_interface_array_forward_sync_hooks.set_value != NULL) {
        s_interface_array_forward_sync_hooks.set_value(
            s_interface_array_forward_sync_hooks.context, object,
            byte_offset, value);
        return;
    }
    memcpy(&vtable, object, sizeof(vtable));
    if (vtable == NULL || vtable[byte_offset / 4] == NULL) return;
    ((Method)vtable[byte_offset / 4])(object, value);
}

void interface_array_forward_sync_hooks_set(
    const InterfaceArrayForwardSyncHooks* hooks)
{
    if (hooks == NULL)
        memset(&s_interface_array_forward_sync_hooks, 0,
               sizeof(s_interface_array_forward_sync_hooks));
    else
        s_interface_array_forward_sync_hooks = *hooks;
}

// FUNCTION: LEGORR 0x00474760
void interface_array_forward_sync(void* owner_link, void* source_record,
                                  void* state_value)
{
    unsigned char* record = (unsigned char*)owner_link;
    unsigned char* source = (unsigned char*)source_record;
    uint32_t kind;
    uint32_t count;
    uint32_t i;
    void* primary;
    void* primary_a;
    void* primary_b;
    void* secondary;
    void* secondary_child;
    uint32_t flags;
    void* selector;

    if (record == NULL) return;
    if (s_interface_array_forward_sync_hooks.record_read != NULL) {
        if (!s_interface_array_forward_sync_hooks.record_read(
                s_interface_array_forward_sync_hooks.context, owner_link,
                &primary_a, &primary_b, &secondary, &secondary_child,
                &kind, &flags)) return;
        primary = kind == 6u ? primary_a : primary_b;
    }
    else {
        /* The raw owner-link fields are four-byte pointers in the reference
         * ABI. Wider hosts must bind the typed record accessor above. */
        if (sizeof(void*) != 4u) return;
        memcpy(&kind, record + 0x10, sizeof(kind));
        memcpy(&primary_a, record + 0x00, sizeof(primary_a));
        memcpy(&primary_b, record + 0x04, sizeof(primary_b));
        primary = kind == 6u ? primary_a : primary_b;
        memcpy(&secondary, record + 0x08, sizeof(secondary));
        memcpy(&flags, record + 0x14, sizeof(flags));
        secondary_child = NULL;
        if (secondary != NULL)
            memcpy(&secondary_child, (unsigned char*)secondary + 0x08,
                   sizeof(secondary_child));
    }
    if (primary == NULL) return;

    if (state_value != NULL) {
        count = (uint32_t)(uintptr_t)state_value;
        interface_array_forward_sync_slot80(primary, &count, NULL);
        if (count != 0u) {
            if (!interface_forward_sync_items_reserve(count)) return;
            interface_array_forward_sync_slot80(primary, &count,
                                                 s_interface_forward_sync_items);
            for (i = 0u; i < count; ++i) {
                interface_array_forward_sync_slot(primary, 0xa0,
                    s_interface_forward_sync_items[i]);
            }
        }

        if (secondary != NULL) {
            uint32_t secondary_count = 0u;
            interface_array_forward_sync_slot80(secondary,
                                                 &secondary_count, NULL);
            if (secondary_count != 0u) {
                if (!interface_forward_sync_items_reserve(secondary_count))
                    return;
                interface_array_forward_sync_slot80(secondary,
                    &secondary_count, s_interface_forward_sync_items);
                for (i = 0u; i < secondary_count; ++i) {
                    void* item = s_interface_forward_sync_items[i];
                    interface_array_forward_sync_slot(primary, 0x48, item);
                    interface_array_forward_sync_slot(secondary_child, 0xa0,
                                                       item);
                }
            }
        }
        flags &= ~4u;
        if (s_interface_array_forward_sync_hooks.record_flags_set != NULL)
            s_interface_array_forward_sync_hooks.record_flags_set(
                s_interface_array_forward_sync_hooks.context, owner_link,
                flags);
        else if (sizeof(void*) == 4u)
            memcpy(record + 0x14, &flags, sizeof(flags));
    }
    else {
        count = 0u;
        interface_array_forward_sync_slot80(primary, &count,
                                             NULL);
        if (count != 0u) {
            if (!interface_forward_sync_items_reserve(count)) return;
            interface_array_forward_sync_slot80(primary, &count,
                                                 s_interface_forward_sync_items);
            for (i = 0u; i < count; ++i) {
                void* item = s_interface_forward_sync_items[i];
                interface_array_forward_sync_slot(secondary, 0x48, item);
                interface_array_forward_sync_slot(primary, 0xa0, item);
            }
        }
        selector = NULL;
        if (source != NULL &&
            s_interface_array_forward_sync_hooks.selector_get != NULL) {
            (void)s_interface_array_forward_sync_hooks.selector_get(
                s_interface_array_forward_sync_hooks.context,
                source_record, &selector);
        }
        else if (source != NULL && sizeof(void*) == 4u) {
            void* selector_owner = NULL;
            void* selector_data = NULL;
            memcpy(&selector_owner, source + 0x0c, sizeof(selector_owner));
            if (selector_owner != NULL) {
                memcpy(&selector, (unsigned char*)selector_owner + 0x08,
                       sizeof(selector));
                if (selector == NULL) {
                    memcpy(&selector_data,
                           (unsigned char*)selector_owner + 0x0c,
                           sizeof(selector_data));
                    if (selector_data != NULL)
                        memcpy(&selector,
                               (unsigned char*)selector_data + 0x0c,
                               sizeof(selector));
                }
            }
        }
        interface_array_forward_sync_slot(primary, 0x48, selector);
        flags |= 4u;
        if (s_interface_array_forward_sync_hooks.record_flags_set != NULL)
            s_interface_array_forward_sync_hooks.record_flags_set(
                s_interface_array_forward_sync_hooks.context, owner_link,
                flags);
        else if (sizeof(void*) == 4u)
            memcpy(record + 0x14, &flags, sizeof(flags));
    }
}
}

namespace unported_ledger_stubs {
void ledger_stub_00474760_InterfaceArrayForwardSync(
    void* owner_link, void* source_record, void* state_value)
{
    lego::interface_array_forward_sync(owner_link, source_record, state_value);
}


// AnimatedTextureFrameCount
int ledger_stub_00474a20_AnimatedTextureFrameCount(const void* texture)
{ return lego::animated_texture_frame_count(texture); }

// SceneSurfaceTextureBindByMode
void ledger_stub_00474a60_SceneSurfaceTextureBindByMode(
    void* texture, uint32_t frame_index, int mode)
{ lego::animated_texture_frame_bind(texture, frame_index, mode); }

// AnimatedTextureFrameResolve
int ledger_stub_00474c00_AnimatedTextureFrameResolve(void** interface_io,
                                                       int* frame_index_io)
{ return lego::animated_texture_frame_resolve(interface_io, frame_index_io); }

// AnimatedTextureFrameSet compatibility entrypoint.
void ledger_stub_00474da0_AnimatedTextureFrameSet(
    void* texture, uint32_t frame_index, const void* frame_state)
{
    lego::animated_texture_frame_set(texture, frame_index, frame_state);
}


// RenderResourceTrackedReferencesRelease
void ledger_stub_004773e0_RenderResourceTrackedReferencesRelease()
{ lego::scene_resource_tracked_references_release_all(); }



} // namespace unported_ledger_stubs

#undef SCENE_RESOURCE_COM_CALL
