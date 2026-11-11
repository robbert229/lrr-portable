// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what camera_control uses from view.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_VIEW_FOR_CAMERA_CONTROL_HPP
#define LRR_GODS98_VIEW_FOR_CAMERA_CONTROL_HPP

/* --- the camera-controller bounds band (see view.hpp) -----------------------
 * Fixed-offset record operations; the corpus's controller record is not
 * modeled as a struct (its owners are the per-view camera objects, and
 * the pose-dispatch half of the keys' logic stays at the unported scene
 * seam). Only the record-field shape these five leaves need is written. */
#define CAM_FLAGS_OFF 0xb8  /* gate-bit byte */

extern ViewObjectLookupHook s_view_object_lookup_hook;
extern ViewTargetBlendHook s_view_target_blend_hook;
extern void* s_view_follow_target_context;
extern CameraViewContextReleaseHook s_view_context_release_hook;
extern CameraViewContextFreeHook s_view_context_free_hook;
extern void* s_view_context_close_context;
extern SceneObjectChainHooks s_scene_object_chain_hooks;
extern CameraPoseDispatchHook s_camera_pose_dispatch_hook;
extern void* s_camera_pose_dispatch_context;

#endif /* LRR_GODS98_VIEW_FOR_CAMERA_CONTROL_HPP */
