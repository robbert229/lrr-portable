// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005) for the scene-container functions (G98-002,
 * gods98/view.cpp and resource.cpp) that game/animation_object.c calls,
 * beyond those in gods98/view.h. They are not added to view.h because other
 * C units include it, and every declaration shifts their symbol numbering
 * (finding 538). Shared C++ headers do not include this file;
 * src/game/test_c_boundaries.cpp checks it against view.hpp and
 * resource.hpp. */
#ifndef LEGORRR_GODS98_CONTAINER_H
#define LEGORRR_GODS98_CONTAINER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Container_SetActivity (FUN_00473370): nonzero when the activity exists. */
unsigned int scene_action_name_set(void* scene_object,
                                   const char* action_name);
/* Container_FormatPartName (FUN_00473c60) is in gods98/texture.h. */
/* Container_SearchTree (FUN_00473bc0). */
void* resource_name_register_or_query_target(
    void** root, const char* name, int mode,
    unsigned int* match_count_or_index);
/* Container_Clone (FUN_00473690). */
void* entity_visual_resource_load(void* scene_object);
/* FUN_004753f0: the activity's frame count. */
unsigned int ddiscene_object_frame_position_get(void* scene_object);
/* FUN_004757f0: the activity's transition time from the `.ae` data. */
float typed_child_resource_scalar_get(void* scene_object);

/* Ledger stubs (namespace unported_ledger_stubs in view.cpp), with C
 * linkage. FUN_00472a40 destroys a container; FUN_004756d0 reads a
 * frame's scale. */
void ledger_stub_00472a40_DDiSceneObjectDestroy(void* scene_object);
float ledger_stub_004756d0_VideoObjectAspectRatioGet(void* video_object);

#ifdef __cplusplus
}
#endif

/* Native builds route the Creature unit's Container calls through
 * forwarders that answer for native Containers (container_route.h,
 * finding 555), so the annotated gods98 functions stay free of backend
 * code. Every declaration after this point, in gods98/entity.h, record.h,
 * texture.h and view.h, names the forwarder, with the same signature. */
#if defined(LEGORRR_BACKEND_MINIWIN)
#include "container_route.h"
#define container_load container_route_load
#define scene_render_lock_toggle container_route_hide
#define entity_visual_resource_load container_route_clone
#define ddiscene_frame_update_and_callbacks container_route_animation_time_set
#define ddiscene_advance_by_time_target container_route_animation_forward
#define ddiscene_object_frame_position_get container_route_animation_frames
#define ddiscene_object_length_get container_route_animation_time
#define ddiscene_object_pose_method_dispatch container_route_orientation_set
#define ddiscene_object_position_set container_route_position_set
#define scene_action_name_set container_route_activity_set
#define ledger_stub_00472a40_DDiSceneObjectDestroy container_route_remove
#define xf_texture_name_format container_route_part_name_format
#define resource_name_register_or_query_target container_route_search_tree
#define typed_child_resource_scalar_get container_route_trans_coef
#define ledger_stub_004756d0_VideoObjectAspectRatioGet \
    container_route_frame_scale
#endif

#endif /* LEGORRR_GODS98_CONTAINER_H */
