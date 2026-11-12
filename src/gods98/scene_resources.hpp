// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GODS98_SCENE_RESOURCES_HPP
#define LEGORRR_GODS98_SCENE_RESOURCES_HPP
#include <stdint.h>
namespace lego {
enum { SCENE_RESOURCE_SLOT_COUNT = 32 };
typedef void (*SceneResourceReleaseHook)(void* resource, void* context);
void scene_resource_release_hook_set(SceneResourceReleaseHook hook,
                                     void* context);
void scene_resource_slots_initialize(); /* 0x00476d50 */
void scene_resource_slots_release_all(); /* 0x00476d80 */
void* scene_resource_slot_get(unsigned int index);
int scene_resource_slot_set(unsigned int index, void* resource);
void scene_resource_string_tables_set(void** first, unsigned int first_count,
                                      void** second, unsigned int second_count);
void scene_resource_string_tables_free(); /* 0x00487760 */
int shared_object_sample_time_matches(const void* record, uint32_t time); /* 0x00487ed0 */
float keyframe_time_segment_fraction(const void* track, float time, uint16_t* segment_out); /* 0x00487fc0 */
int animated_texture_frame_count(const void* texture); /* 0x00474a20 */
void animated_texture_frame_bind(void* texture, uint32_t frame_index,
                                 int mode); /* 0x00474a60 */
int animated_texture_frame_resolve(void** interface_io,
                                   int* frame_index_io); /* 0x00474c00 */
void animated_texture_frame_set(void* texture, uint32_t frame_index,
                                const void* frame_state); /* 0x00474da0 */
typedef void (*AnimatedTextureFrameApplyHook)(void* interface_object,
                                               void* surface_record,
                                               void* context);
void animated_texture_frame_apply_hook_set(AnimatedTextureFrameApplyHook hook,
                                           void* context);
void animated_texture_frame_surface_set(void* texture, int frame_index,
                                        const void* frame_state);
/* InterfaceArrayForwardSync @ 0x474760 uses owner-link fields plus renderer
 * slots +0x80/+0x48/+0xa0. The field reader keeps host records separate from
 * the 32-bit target layout; the method hooks mirror those exact calls. */
typedef struct InterfaceArrayForwardSyncHooks {
    void* context;
    int (*record_read)(void* context, void* owner_link,
                       void** primary_a, void** primary_b,
                       void** secondary, void** secondary_child,
                       uint32_t* kind, uint32_t* flags);
    void (*record_flags_set)(void* context, void* owner_link,
                             uint32_t flags);
    int (*selector_get)(void* context, void* source_record,
                        void** selector);
    void (*enumerate)(void* context, void* interface_object,
                      uint32_t* count, void** values);
    void (*set_value)(void* context, void* interface_object,
                      uint32_t method_offset, void* value);
} InterfaceArrayForwardSyncHooks;
void interface_array_forward_sync_hooks_set(
    const InterfaceArrayForwardSyncHooks* hooks);
void interface_array_forward_sync(void* owner_link, void* source_record,
                                  void* state_value); /* 0x00474760 */
typedef int (*SharedTextureRootInitializeHook)(void* context);
void shared_texture_root_initialize_hook_set(
    SharedTextureRootInitializeHook hook, void* context);
int shared_texture_root_register(const char* root); /* 0x004805b0 */
int shared_texture_root_initialize(); /* 0x00480650 */
const char* shared_texture_root_get();
/* Host records for CachedRenderResourceInstanceCreate @ 0x00480900. `words`
 * retains the source's thirteen dwords; pointer fields at words[2]/[9] and
 * mutable counters are represented by widened host fields. entry_block
 * points at the source's 0x10-byte header followed by `words[0]` records of
 * 0x70 bytes each. */
typedef struct CachedRenderResourceTemplateRecord {
    uint32_t words[13];
    const unsigned char* entry_block;
    struct CachedRenderResourceTemplateRecord* shared_template;
} CachedRenderResourceTemplateRecord;
typedef struct CachedRenderResourceInstanceRecord {
    /* Dword copy; pointer words [2], [3], [9], and [10] are represented by
     * the widened fields below. */
    uint32_t words[13];
    void* pool_record;
    unsigned char* entry_block;
    CachedRenderResourceTemplateRecord* source_template;
    void* renderer_interface;
    void* renderer_owner;
} CachedRenderResourceInstanceRecord;
typedef void (*CachedRenderResourceRendererHook)(
    CachedRenderResourceInstanceRecord* instance, void* context);
typedef void (*CachedRenderResourceRendererReleaseHook)(
    CachedRenderResourceInstanceRecord* instance, void* context);
void cached_render_resource_renderer_hook_set(
    CachedRenderResourceRendererHook hook, void* context);
void cached_render_resource_renderer_release_hook_set(
    CachedRenderResourceRendererReleaseHook hook, void* context);
CachedRenderResourceInstanceRecord* cached_render_resource_instance_create(
    CachedRenderResourceTemplateRecord* source); /* 0x00480900 */
void cached_render_resource_instance_dispose(
    CachedRenderResourceInstanceRecord* instance);
void scene_resource_tracked_references_release_all(); /* 0x004773e0 */
typedef void (*SceneFloatTrackApplyHook)(void* owner, const void* track,
                                         float value, void* context);
void scene_float_track_apply_hook_set(SceneFloatTrackApplyHook hook,
                                      void* context);
void shared_object_float_track_interpolate_apply(
    void* owner, const void* track, uint32_t segment, float fraction);
typedef void (*SceneAnimationTrackApplyHook)(void* owner, const void* track,
    const float position[3], const float rotation[3], const float scale[3],
    void* context);
void scene_animation_track_apply_hook_set(SceneAnimationTrackApplyHook hook,
                                          void* context);
void animation_track_keys_interpolate_apply(void* owner, const void* track,
                                            uint32_t segment, float fraction);
typedef void (*SceneNodeMatrixApplyHook)(void* owner, const void* track,
                                         const float matrix[16],
                                         void* context);
void scene_node_matrix_apply_hook_set(SceneNodeMatrixApplyHook hook,
                                      void* context);
void scene_node_transform_matrix_apply(void* owner, const void* track,
    const float position[3], const float rotation[3], const float scale[3]);
void resource_interface_call_slot48(void* wrapper, void* value); /* 0x00476f50 */
extern "C" float resource_interface_call_slot8c(void* wrapper); /* 0x00476fd0 */
extern "C" float resource_interface_call_slot90(void* wrapper); /* 0x00476ff0 */
}
#endif
