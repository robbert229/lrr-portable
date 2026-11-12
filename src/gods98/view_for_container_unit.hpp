// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what container_unit uses from view.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GODS98_VIEW_FOR_CONTAINER_UNIT_HPP
#define LRR_GODS98_VIEW_FOR_CONTAINER_UNIT_HPP

#define DDI_COORD_SLOT(n) virtual void slot_##n(void) = 0

/* DDiSceneObject pose interface. Keep the vtable ordinals explicit: world
 * position get/set and pose dispatch live at byte offsets 0x60, 0xf0, and
 * 0xec in the reference interface. */
#define DDI_POSE_SLOT(n) virtual void slot_##n(void) = 0

extern SceneObjectBoundsHooks s_scene_object_bounds_hooks;
extern RenderPackedRgbaHooks s_render_packed_rgba_hooks;
extern SceneContextHooks s_scene_context_hooks;
extern SceneActFrameScanHooks s_scene_act_frame_scan_hooks;
extern EntityVisualResourceLoadHooks s_entity_visual_resource_load_hooks;
extern DDiSceneAnimationHookRecordHooks s_ddiscene_animation_hook_record_hooks;
extern DDiSceneFrameRecordFindHooks s_ddiscene_frame_record_find_hooks;
extern int s_ddiscene_sample_enabled;
extern DDiSceneEndHook s_ddiscene_end_hook;
extern void* s_ddiscene_end_context;
void ddiscene_sample_hook_dispatch(const char* sample_name, void* scene_object);
extern DDiSceneObjectDestroyHook s_ddiscene_object_destroy_hook;
extern void* s_ddiscene_object_destroy_context;
extern SceneChildSearchHooks s_scene_child_search_hooks;
extern SceneFormattedStringHooks s_scene_formatted_string_hooks;
extern RenderInterfacePairHooks s_render_interface_pair_hooks;
extern DDiScenePayloadReleaseHooks s_ddiscene_payload_release_hooks;
void* ddiscene_action_name_record_find(void* scene_object, const char* action_name, unsigned int selector);
int ddiscene_animation_hook_record_set( void* scene_interface, DDiSceneAnimationHookRecord* record);
extern void* s_scene_root;
extern uint32_t s_scene_fog_color;
extern uint32_t s_scene_fog_mode;
extern const void* s_advisor_companion_transform;
extern AdvisorCompanionApplyFn s_advisor_object_apply;
extern AdvisorCompanionApplyFn s_advisor_scene_apply;
namespace unported_ledger_stubs {
extern DdiSceneResourceExistsHook s_ddiscene_resource_exists_hook;
extern DdiSceneResourceOpenHook s_ddiscene_resource_open_hook;
extern void* s_ddiscene_resource_hook_context;
} // namespace unported_ledger_stubs

#endif /* LRR_GODS98_VIEW_FOR_CONTAINER_UNIT_HPP */
