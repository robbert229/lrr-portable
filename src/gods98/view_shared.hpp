// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/gods98/view.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GODS98_VIEW_SHARED_HPP
#define LRR_GODS98_VIEW_SHARED_HPP

extern AnimationObjectTransformApplyHook s_animation_orientation_hook;
extern RenderNodePositionApplyHook s_render_node_position_hook;
extern void* s_animation_transform_context;
extern RendererRecordCloneHooks s_renderer_record_clone_hooks;
extern SceneTimingValueGetHook s_scene_timing_value_get_hook;
extern SceneTimingCountGetHook s_scene_timing_count_get_hook;
extern void* s_scene_timing_hooks_context;
extern AnimationObjectTimeUpdateHook s_animation_object_time_update_hook;
extern AnimationObjectActionHook s_animation_object_time_get_hook;
extern AnimationObjectActionHook s_animation_object_value_get_hook;
extern void* s_animation_object_hooks_context;
extern RendererHandleScalarGetHook s_renderer_handle_scalar_get_hook;
extern void* s_renderer_handle_scalar_get_context;
extern RendererSubobjectResolveFn s_renderer_subobject_resolver;

extern SceneObjectRecordRetainHook s_scene_object_record_retain_hook;
extern void* s_scene_object_record_retain_context;
void* scene_object_subobject_resolve(void* owner, void* source, int index);

extern QuadMeshRenderHooks s_quad_mesh_render_hooks;

extern SceneObjectListApplyHook s_scene_object_list_apply_hook;
extern void* s_scene_object_list_apply_context;

#endif /* LRR_GODS98_VIEW_SHARED_HPP */
