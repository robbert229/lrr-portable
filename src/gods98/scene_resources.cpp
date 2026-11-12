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
#include "scene_resources_for_mesh_unit.hpp"
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
} // namespace
AnimatedTextureFrameApplyHook s_texture_frame_apply_hook = 0;
void* s_texture_frame_apply_context = 0;
char s_shared_texture_root[1024];
namespace {
SharedTextureRootInitializeHook s_shared_texture_initialize_hook = 0;
void* s_shared_texture_initialize_context = 0;
} // namespace
CachedRenderResourceRendererHook s_cached_resource_renderer_hook = 0;
void* s_cached_resource_renderer_context = 0;
namespace {
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
void shared_texture_root_initialize_hook_set(
    SharedTextureRootInitializeHook hook, void* context)
{
    s_shared_texture_initialize_hook = hook;
    s_shared_texture_initialize_context = context;
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
#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
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
#endif /* !LEGORRR_BACKEND_WINAPI */
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
