// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what screen_effect_quads.cpp sees from
 * view.cpp. A verbatim snapshot of view_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GODS98_VIEW_FOR_SCREEN_EFFECT_QUADS_HPP
#define LRR_GODS98_VIEW_FOR_SCREEN_EFFECT_QUADS_HPP

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

int engine_context_grid_pick(EngineContext* ctx, const float* pos, int* gx, int* gy);
float terrain_height(EngineContext* ctx, float x, float y);

extern TerrainPointPickCellCheckHook s_terrain_point_pick_hook;
extern void* s_terrain_point_pick_user;
extern ViewportPointDispatchHooks s_viewport_point_dispatch_hooks;
extern TexturedSceneDrawHooks s_textured_scene_draw_hooks;

#define ROT_PAIR_SLOTS 256 /* deviation: bounded scratch (blind DAT stride) */

extern int s_rot_pairs[ROT_PAIR_SLOTS * 2];

extern RenderResourceBindingHooks s_render_resource_binding_hooks;

/* Row-span staging records from FUN_0046eb30. The corpus has a 0x320-byte
 * count slot and an adjacent x/y float-pair window; dimensions inferred
 * from that window bound the port's rough model to 96 points per row and
 * 64 staged rows. */
#define MAP_ROW_SPAN_RECORDS_MAX 64

#define MAP_ROW_SPAN_RECORD_SIZE 0x324

extern unsigned char s_map_row_span_records[MAP_ROW_SPAN_RECORDS_MAX] [MAP_ROW_SPAN_RECORD_SIZE];
extern uint32_t s_map_row_span_records_count;
extern GridSurfaceLookupFn s_grid_surface_lookup;
extern GridSurfaceHandleReleaseFn s_grid_surface_release;
extern void* s_grid_surface_user;

#endif /* LRR_GODS98_VIEW_FOR_SCREEN_EFFECT_QUADS_HPP */
