// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what placement.cpp sees from
 * util.cpp. A verbatim snapshot of util_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GAME_UTIL_FOR_PLACEMENT_HPP
#define LRR_GAME_UTIL_FOR_PLACEMENT_HPP

#define STRING_TABLE_MAX_ENTRIES 256

extern const char* s_string_table_entries[STRING_TABLE_MAX_ENTRIES];
extern unsigned int s_string_table_count;

#define PENDING_RECORD_CAPACITY 256

extern uint32_t s_pending_record_rows[PENDING_RECORD_CAPACITY][5];
extern unsigned int s_pending_record_count;
extern void (*s_pending_record_release_hook)(uint32_t* row);
extern EntityLinkList** s_entity_link_lists;
extern void (*s_entity_link_node_release_hook)(EntityLinkNode*);
extern void (*s_entity_link_list_release_hook)(EntityLinkList*);

extern const char* s_bubble_action_strings[BUBBLE_ACTION_STRING_COUNT];

extern "C" float random_float_range(float minimum, float maximum);

extern void* s_motion_jitter_retail_scene_object;
extern void* s_motion_jitter_retail_scene_owner;
extern float s_motion_jitter_center[3];
extern uint32_t s_motion_jitter_flags;
extern MotionDriftState* s_motion_drift_state;
extern MotionRandomRangeFn s_motion_drift_random_range;
extern void* s_motion_drift_user;
extern MotionWaypointTimerState* s_motion_waypoint_state;
extern PortLoopRandomRangeHook s_motion_waypoint_random_range;
extern void* s_motion_waypoint_user;
extern void* s_motion_gameplay_scene_object;
extern void* s_motion_gameplay_scene_owner;
extern MotionJitterState s_motion_jitter_state;
extern float s_motion_waypoint_target[3];
extern float s_motion_waypoint_bounds_min[3];
extern float s_motion_waypoint_bounds_max[3];
extern float s_motion_waypoint_delay;
extern float s_motion_waypoint_delay_range[2];
extern float s_motion_waypoint_velocity[3];
extern float s_motion_waypoint_scale_range[2];
extern float s_motion_waypoint_duration;
extern float s_motion_waypoint_duration_range[2];
extern float s_motion_waypoint_progress[3];
extern float s_motion_waypoint_position[3];
void motion_jitter_center_refresh(void);
float motion_waypoint_default_random(void* context, float minimum, float maximum);

#define LEVEL_REMAP_CAPACITY 128

extern uint32_t s_level_remap_pairs[LEVEL_REMAP_CAPACITY][2];
extern unsigned int s_level_remap_count;

#endif /* LRR_GAME_UTIL_FOR_PLACEMENT_HPP */
