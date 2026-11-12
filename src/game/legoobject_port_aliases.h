// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-023 LegoObject.c part 1 (issue #575): matching-build-only aliases
 * from the port's LegoObject.c entry points to the original-shape functions
 * in legoobject_unit.c.
 *
 * The port's implementations stay in legoobject.cpp, unannotated, and native
 * callers keep calling them: they read the port's EntitySlot, not the
 * original's 0x40c-byte record, so the unit's functions cannot replace them
 * natively. In the matching build (MSVC 5.0, _MSC_VER 1100) the callers
 * whose contract is already the original's call the unit's functions
 * instead, so their call operands name the original functions again.
 *
 * Include it first in a caller's file, before any declaration of these
 * names. Each macro renames a declaration or a call one for one, so MSVC's
 * symbol numbering (finding 538) does not change. Every aliased entry has
 * C linkage. Not for legoobject.cpp, which defines the port models. */
#ifndef LEGORRR_GAME_LEGOOBJECT_PORT_ALIASES_H
#define LEGORRR_GAME_LEGOOBJECT_PORT_ALIASES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define ai_activity_name_table_initialize legoobject_initialise /* 00436c20 */
#define pooled_record_pool_resources_release legoobject_shutdown /* 00437050 */
#define render_config_six_field_copy_out legoobject_render_config_copy_out /* 004370b0 */
#define render_config_six_field_copy_in_or_clear legoobject_render_config_copy_in_or_clear /* 004370d0 */
#define grid_record_count_difference_sum legoobject_grid_record_count_difference_sum /* 00437100 */
#define tooltip_icons_config_load legoobject_tool_tip_icons_load /* 00437150 */
#define spawn_coordinate_pool_clear legoobject_spawn_coordinate_pool_clear /* 004372a0 */
#define process_queued_grid_animations legoobject_process_queued_grid_animations /* 00437300 */
#define entity_activity_toggle legoobject_activity_toggle /* 004373d0 */
#define grid_record_count_tables_clear legoobject_grid_record_count_tables_clear /* 00437440 */
#define grid_record_count_difference_get legoobject_grid_record_count_difference_get /* 00437460 */
#define grid_record_count_get legoobject_grid_record_count_get /* 004374a0 */
#define entity_task_counter_increment_by_branch legoobject_task_counter_increment_by_branch /* 004374d0 */
#define entity_callback_dispatch_unless_type_five legoobject_callback_dispatch_unless_type_five /* 004374f0 */
#define entity_linked_route_clear_callback legoobject_linked_route_clear_callback /* 00437510 */
#define entity_teardown legoobject_remove /* 00437540 */
#define entity_pool_find_by_callback_filtered_current legoobject_run_through_lists_skip_upgrade_parts /* 004377b0 */
#define entity_pool_find_by_callback_current legoobject_run_through_lists /* 004377d0 */
#define grid_object_records_release legoobject_grid_object_records_release /* 004378e0 */
#define deferred_olist_entity_create_and_link legoobject_deferred_olist_create_and_link /* 00437940 */
#define deferred_olist_entity_queue_add legoobject_deferred_olist_queue_add /* 00437c20 */
#define entity_flag40000_threshold_compare legoobject_flag40000_threshold_compare /* 00437cc0 */
#define pooled_record_allocate_and_reset legoobject_pool_allocate /* 004382c0 */
#define pooled_record_chunk_allocate_and_link legoobject_pool_chunk_allocate /* 00438310 */
#define entity_global_animation_state_set legoobject_global_animation_state_set /* 004383a0 */
#define rover_work_queue_update legoobject_rover_work_queue_update /* 004383b0 */
#define entity_kind_record_update_dispatch legoobject_kind_record_update_dispatch /* 00438460 */
#define entity_flag80000000_set_or_clear legoobject_flag80000000_set_or_clear /* 00438580 */
#define entity_shared_eligibility_check legoobject_shared_eligibility_check /* 004385b0 */
#define entity_create_with_position legoobject_create_in_world /* 00438610 */
#define entity_at_grid_position_find legoobject_at_grid_position_find /* 00438670 */
#define entity_available_resource_spend legoobject_available_resource_spend /* 00438720 */
#define building_upgrade_resource_admission_check legoobject_building_upgrade_resource_admission_check /* 00438770 */
#define building_upgrade_commit legoobject_building_upgrade_commit /* 004387f0 */
#define entity_action_kind_prerequisite_check legoobject_action_kind_prerequisite_check /* 004388b0 */
#define entity_route_marker_candidate_find legoobject_route_marker_candidate_find /* 004389e0 */
#define entity_lookup_at_world_position legoobject_lookup_at_world_position /* 00438df0 */
#define entity_flags_contain_requested_single_bit legoobject_flags_contain_requested_single_bit /* 00438ed0 */
#define dock_definition_matches_cell legoobject_dock_definition_matches_cell /* 00438f60 */
#define entity_route_marker_candidate_visit legoobject_route_marker_candidate_visit /* 00438fb0 */
#define task_kind_filtered_search legoobject_task_kind_filtered_search /* 00439200 */
#define task_entity_flags_and_cell_task_match legoobject_task_flags_and_cell_task_match /* 00439240 */
#define entity_build_placement_command_batch_expand legoobject_build_placement_command_batch_expand /* 00439280 */
#define entity_build_placement_command_pair_create legoobject_build_placement_command_pair_create /* 00439340 */
#define entity_display_name_lookup legoobject_display_name_lookup /* 004395e0 */
#define item_name_by_kind_lookup legoobject_item_name_by_kind /* 004396c0 */
#define localized_object_the_name_get legoobject_the_name_get /* 00439790 */
#define localized_object_name_get legoobject_name_get /* 00439860 */
#define copy_two_dword_pair_to_outputs legoobject_copy_two_dword_pair /* 00439930 */
#define entity_tool_name_strings_load legoobject_tool_name_strings_load /* 00439950 */
#define entity_activity_scene_state_update_retail legoobject_activity_scene_state_update /* 00439990 */
#define entity_flag400_and_health_threshold_check legoobject_flag400_and_health_threshold_check /* 004399c0 */
#define entity_gated_step_command_issue legoobject_gated_step_command_issue /* 00439a20 */
#define entity_task_target_cell_eligibility_check legoobject_task_target_cell_eligibility_check /* 00439b80 */
#define entity_accepts_target_for_task_check legoobject_accepts_target_for_task_check /* 00439bd0 */
#define entity_indexed_flag_set_and_refresh legoobject_indexed_flag_set_and_refresh /* 00439c80 */
#define entity_animation_state_set legoobject_animation_state_set /* 00439cd0 */
#define entity_stats_flag10_test legoobject_stats_flag10_test /* 00439cf0 */
#define entity_stats_flag20_test legoobject_stats_flag20_test /* 00439d10 */
#define entity_stats_flag40_test legoobject_stats_flag40_test /* 00439d30 */
#define entity_vehicle_unload_adjacent_cell_find legoobject_vehicle_unload_adjacent_cell_find /* 00439d50 */
#define entity_vehicle_unload_cell_find_if_eligible legoobject_vehicle_unload_cell_find_if_eligible /* 00439e10 */
#define entity_record_flag20_query legoobject_record_flag20_query /* 00439e40 */
#define entity_attached_object_list_detach legoobject_attached_object_list_detach /* 00439e70 */
#define entity_cell_step_command_issue_with_jitter legoobject_cell_step_command_issue_with_jitter /* 0043a120 */
#define entity_deposit_activity_gate legoobject_deposit_activity_gate /* 0043a300 */
#define entity_visual_resource_get_or_create legoobject_visual_resource_get_or_create /* 0043a5f0 */
#define entity_action_kind_entity_create_and_command_issue legoobject_action_kind_create_and_command_issue /* 0043a650 */
#define entity_action_kind_supported legoobject_action_kind_supported /* 0043a7c0 */
#define entity_action_record_consume_and_cleanup legoobject_action_record_consume_and_cleanup /* 0043a850 */
#define simulation_update_counter_add legoobject_simulation_update_counter_add /* 0043a8f0 */
#define simulation_mode_counter_add legoobject_simulation_mode_counter_add /* 0043a910 */
#define ai_idle_table_pointer_remove legoobject_tracked_pointer_remove /* 0043a930 */
#define ai_lava_crust_activity_register legoobject_lava_crust_activity_register /* 0043a960 */
#define task_type_and_condition_check legoobject_task_type_and_condition_check /* 0043c490 */
/* Original-ABI adapters other port tails give converted C units. A file
 * that defines them sets LEGORRR_LEGOOBJECT_ALIASES_NO_ADAPTERS first. */
#if !defined(LEGORRR_LEGOOBJECT_ALIASES_NO_ADAPTERS)
#define lego_object_can_eject legoobject_action_kind_supported /* 0043a7c0 */
#define lego_object_find_trainer legoobject_query_center_mask100 /* 00438e50 */
#define lego_object_set_primary_teleporter legoobject_flag80000000_set_or_clear /* 00438580 */
#define entity_custom_name_set_adapter legoobject_rename /* 00437880 */
#define lego_object_create_adapter legoobject_create /* 00437d00 */
#define entity_pool_find_by_callback_filtered_current_original legoobject_run_through_lists_skip_upgrade_parts /* 004377b0 */
#endif
#endif

#endif /* LEGORRR_GAME_LEGOOBJECT_PORT_ALIASES_H */
