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
void animated_texture_frame_bind(void* texture, void* frame, int mode); /* 0x00474a60 */
int animated_texture_frame_resolve(void* interface_object, int frame,
                                   void** resolved_out); /* 0x00474c00 */
typedef void (*AnimatedTextureFrameApplyHook)(void* interface_object,
                                               void* surface_record,
                                               void* context);
void animated_texture_frame_apply_hook_set(AnimatedTextureFrameApplyHook hook,
                                           void* context);
void animated_texture_frame_surface_set(void* texture, int frame_index,
                                        const void* frame_state);
typedef int (*SharedTextureRootLoadHook)(const char* root, unsigned int index,
                                         void** resource, void* context);
void shared_texture_root_load_hook_set(SharedTextureRootLoadHook hook,
                                       void* context);
int shared_texture_root_register(const char* root); /* 0x004805b0 */
int shared_texture_root_initialize(); /* 0x00480650 */
const char* shared_texture_root_get();
void* shared_texture_root_resource_get(unsigned int index);
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
void resource_interface_call_slot8c(void* wrapper); /* 0x00476fd0 */
void resource_interface_call_slot90(void* wrapper); /* 0x00476ff0 */
}
#endif
