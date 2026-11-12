// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 container and viewport functions that units rebuilt as C call.
 *
 * C boundary header (ADR 0005): C declarations, with C linkage, of
 * functions the port keeps in gods98/view.cpp. The C++ code keeps using
 * gods98/view.hpp, whose matching declarations carry extern "C";
 * src/game/test_c_boundaries.cpp includes both and fails to compile if they
 * disagree. Shared C++ headers do not include this file: every include and
 * declaration a header adds shifts MSVC's symbol numbering (finding 506). */
#ifndef LEGORRR_GODS98_VIEW_H
#define LEGORRR_GODS98_VIEW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ViewProjectionAdapter ViewProjectionAdapter;

/* Container_MakeLight (FUN_004734f0): the colour components are passed as
 * their float bits. */
void* ddiscene_light_create(void* owner, uint32_t value_a,
                            uint32_t value_b, uint32_t value_c,
                            uint32_t value_d);
/* FUN_004734c0: the second argument is the container. */
void advisor_companion_transform_apply(void* unused, void** object_reference);
/* Container_MoveAnimation (FUN_00475120). */
float ddiscene_advance_by_time_target(void* scene_object, float delta);
/* Container_SetAnimationTime (FUN_00475140). */
float ddiscene_frame_update_and_callbacks(void* scene_object, float time);
/* Container_GetAnimationTime (FUN_00475390). */
float ddiscene_object_length_get(void* scene_object);
/* Container_SetPosition (FUN_00475430). */
int ddiscene_object_position_set(void* object, void* alternate_frame,
                                 float x, float y, float z);
/* Container_SetOrientation (FUN_00475470). */
int ddiscene_object_pose_method_dispatch(void* object,
                                         void* alternate_frame,
                                         float parameter_3,
                                         float parameter_4,
                                         float parameter_5,
                                         float parameter_6,
                                         float parameter_7,
                                         float parameter_8);
/* Viewport_InverseTransform (FUN_00477290). */
void camera_screen_coordinate_project(const ViewProjectionAdapter* adapter,
                                      float* output, const float* input);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_VIEW_H */
