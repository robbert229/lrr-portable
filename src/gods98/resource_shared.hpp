// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/gods98/resource.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GODS98_RESOURCE_SHARED_HPP
#define LRR_GODS98_RESOURCE_SHARED_HPP

extern void* s_numbered_bitmap_slots[11];
extern NumberedBitmapLoadHook s_numbered_bitmap_load_hook;
extern void* s_numbered_bitmap_context;

#define ROCK_FALL_STYLE_NAME_CAPACITY 64

#define ROCK_FALL_STYLE_NAME_LENGTH 128

typedef struct SceneAvailabilitySlot {
    void* handle;
    int configured;
    int enabled;
    int pending;
} SceneAvailabilitySlot;

#define SCENE_AVAILABILITY_STYLE_CAPACITY 64

typedef struct SceneResourceNameSlot {
    uint32_t handle_key;
    uint32_t secondary_key;
    void* handle;
    uint32_t name_base;
    uint32_t name_count;
    uint32_t registered;
} SceneResourceNameSlot;

extern char s_rock_fall_style_names[ROCK_FALL_STYLE_NAME_CAPACITY] [ROCK_FALL_STYLE_NAME_LENGTH];
extern uint32_t s_rock_fall_style_name_count;
extern SceneTimeoutEntrySet s_terrain_marker_sets[10];
extern void* s_scene_light_primary[4];
extern void* s_scene_light_secondary[4];
extern void* s_scene_marker_templates[3];
extern int s_scene_marker_scene_variant;
extern SceneObjectPositionHook s_scene_object_position_hook;
extern void* s_scene_object_position_context;
extern void* s_terrain_point_template;
extern TerrainHeightAtPointHook s_terrain_height_hook;
extern void* s_terrain_height_context;
extern SceneResourceAdvanceHook s_scene_advance_hook;
extern SceneResourceReleaseHook s_scene_release_hook;
extern SceneTimeoutEffectHook s_bird_scarer_effect_hook;
extern void* s_global_resource_handle;
extern void* s_scene_lifecycle_context;
extern void* s_terrain_point_objects[4];
extern uint32_t s_terrain_point_object_count;
extern SceneEffectPoseHook s_scene_effect_pose_hook;
extern void* s_scene_effect_pose_context;
extern SceneAvailabilitySlot s_scene_availability_styles[SCENE_AVAILABILITY_STYLE_CAPACITY][3][4];
extern uint32_t s_scene_cell_table_pairs[SCENE_AVAILABILITY_STYLE_CAPACITY] [3][4][2];
extern SceneResourceNameSlot s_scene_resource_name_slots[SCENE_AVAILABILITY_STYLE_CAPACITY][3][4];
extern SceneResourceNameFormatHook s_resource_name_format_hook;
extern SceneResourceNameApplyHook s_resource_name_apply_hook;
extern void* s_resource_name_context;
extern SceneResourceParentLoadHook s_resource_parent_load_hook;
extern SceneResourceChildCreateHook s_resource_child_create_hook;
extern SceneResourceLockHook s_resource_lock_hook;
extern void* s_resource_instance_context;
extern SceneAvailabilitySlot s_scene_availability_transients[2][30];
extern SceneActLoadHook s_scene_act_load_hook;
extern SceneActNameRegisterHook s_scene_act_name_register_hook;
extern void* s_scene_act_context;
extern SceneSlotPoseHook s_scene_slot_pose_hook;
extern void* s_scene_slot_pose_context;
extern TransientEffectConfigureHook s_transient_effect_configure_hook;
extern void* s_transient_effect_configure_context;
extern int s_scene_availability_selected_style;

extern SceneAnimationChannelUpdateHook s_animation_channel_update_hook;
extern SceneAnimationChannelQueryHook s_animation_channel_query_hook;
extern SceneAnimationVisibilityHook s_animation_visibility_hook;
extern void* s_animation_channel_context;
extern SceneIndexedRecordNodeReleaseHook s_scene_indexed_record_release_hook;
extern void* s_scene_indexed_record_context;
extern SceneChannelEntryProcessHook s_scene_channel_entry_process_hook;
extern void* s_scene_channel_entry_context;

#endif /* LRR_GODS98_RESOURCE_SHARED_HPP */
