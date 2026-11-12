// SPDX-License-Identifier: LGPL-3.0-or-later
/* Native-only forwarders for the Container calls of the Creature unit
 * (game/animation_object.c, GAME-004). Each takes the same arguments as the
 * gods98 function it stands for, answers for a native Container
 * (container_native.hpp) and otherwise calls that function unchanged.
 *
 * gods98/container.h routes GAME-004's calls here in MINIWIN builds, so the
 * annotated Container functions (view.cpp, texture.cpp, resource.cpp,
 * record.cpp) and container_load (entity.cpp) carry no native code
 * (finding 555). The matching build never sees this file. */
#ifndef LEGORRR_GODS98_CONTAINER_ROUTE_H
#define LEGORRR_GODS98_CONTAINER_ROUTE_H

#if defined(LEGORRR_BACKEND_MINIWIN)

#ifdef __cplusplus
extern "C" {
#endif

/* container_load (gods98/entity.h): "ACT" objects load natively. */
void* container_route_load(void* parent, const char* filename,
                           const char* typeString, int looseLoad);
/* scene_render_lock_toggle (gods98/record.h): Container_Hide. */
void container_route_hide(void* scene_object, int enabled);
/* entity_visual_resource_load: Container_Clone. */
void* container_route_clone(void* scene_object);
/* ddiscene_frame_update_and_callbacks: Container_SetAnimationTime. */
float container_route_animation_time_set(void* scene_object, float time);
/* ddiscene_advance_by_time_target: Container_ForwardAnimation. */
float container_route_animation_forward(void* scene_object, float delta);
/* ddiscene_object_frame_position_get: the activity's frame count. */
unsigned int container_route_animation_frames(void* scene_object);
/* ddiscene_object_length_get: Container_GetAnimationTime. */
float container_route_animation_time(void* scene_object);
/* ddiscene_object_pose_method_dispatch: Container_SetOrientation. */
int container_route_orientation_set(void* object, void* alternate_frame,
                                    float parameter_3, float parameter_4,
                                    float parameter_5, float parameter_6,
                                    float parameter_7, float parameter_8);
/* ddiscene_object_position_set: Container_SetPosition. */
int container_route_position_set(void* object, void* alternate_frame,
                                 float x, float y, float z);
/* scene_action_name_set: Container_SetActivity. */
unsigned int container_route_activity_set(void* scene_object,
                                          const char* action_name);
/* ledger_stub_00472a40_DDiSceneObjectDestroy: Container_Remove. */
void container_route_remove(void* scene_object);
/* xf_texture_name_format (gods98/texture.h): Container_FormatPartName. */
const char* container_route_part_name_format(void* texture_record,
                                             const char* name_base,
                                             const int* frame_index);
/* resource_name_register_or_query_target: Container_SearchTree. */
void* container_route_search_tree(void** root, const char* name, int mode,
                                  unsigned int* match_count_or_index);
/* typed_child_resource_scalar_get: Container_GetTransCoef. */
float container_route_trans_coef(void* scene_object);
/* ledger_stub_004756d0_VideoObjectAspectRatioGet: a frame's scale. */
float container_route_frame_scale(void* video_object);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_BACKEND_MINIWIN */
#endif /* LEGORRR_GODS98_CONTAINER_ROUTE_H */
