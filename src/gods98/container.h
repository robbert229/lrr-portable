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

#endif /* LEGORRR_GODS98_CONTAINER_H */
