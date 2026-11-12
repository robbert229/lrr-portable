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
#include "scene_resources_for_viewports.hpp"
#include "scene_resources_for_lws.hpp"
#include "scene_resources_for_container_unit.hpp"
namespace lego {
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
namespace {
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
InterfaceArrayForwardSyncHooks s_interface_array_forward_sync_hooks;
void** s_interface_forward_sync_items;
uint32_t s_interface_forward_sync_items_capacity;
void scene_resource_release_hook_set(SceneResourceReleaseHook hook,
                                     void* context)
{ s_release_hook = hook; s_release_context = context; }
void* scene_resource_slot_get(unsigned int index)
{ return s_slots_live && index < SCENE_RESOURCE_SLOT_COUNT ? s_render_surface_pool_blocks[index] : 0; }
int scene_resource_slot_set(unsigned int index, void* resource)
{
    if (!s_slots_live || index >= SCENE_RESOURCE_SLOT_COUNT) return 0;
    s_render_surface_pool_blocks[index] = resource;
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

void scene_float_track_apply_hook_set(SceneFloatTrackApplyHook hook,
                                      void* context)
{
    s_float_track_apply_hook = hook;
    s_float_track_apply_context = context;
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

void interface_array_forward_sync_hooks_set(
    const InterfaceArrayForwardSyncHooks* hooks)
{
    if (hooks == NULL)
        memset(&s_interface_array_forward_sync_hooks, 0,
               sizeof(s_interface_array_forward_sync_hooks));
    else
        s_interface_array_forward_sync_hooks = *hooks;
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
