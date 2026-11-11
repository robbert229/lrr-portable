// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what upgrade_levels.cpp sees from
 * resource.cpp. A verbatim snapshot of resource_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GODS98_RESOURCE_FOR_UPGRADE_LEVELS_HPP
#define LRR_GODS98_RESOURCE_FOR_UPGRADE_LEVELS_HPP

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

extern SceneAnimationDurationHook s_animation_duration_hook;
extern SceneAnimationLastFrameHook s_animation_last_frame_hook;
extern void* s_animation_duration_context;
extern SceneResourceNamedResolveHook s_scene_resource_named_resolve_hook;
extern void* s_scene_resource_record_context;
extern SceneAnimationTrackApplyHook s_animation_track_apply_hook;
extern void* s_animation_track_context;
extern SceneResourceOwnerApplyHook s_scene_resource_owner_apply_hook;
extern void* s_scene_resource_owner_context;
extern SceneOwnedMemoryReleaseHook s_scene_owned_memory_release_hook;
extern void* s_scene_owned_memory_context;

#endif /* LRR_GODS98_RESOURCE_FOR_UPGRADE_LEVELS_HPP */
