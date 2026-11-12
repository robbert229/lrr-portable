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
/* GAME-023 part 1 (issue #575) ends; 0043c490 is part 2's (pulled forward). */
#define task_type_and_condition_check legoobject_task_type_and_condition_check /* 0043c490 */
/* GAME-023 part 2 (issue #588): 0043a9e0-00445830. */
#define entity_queue_for_rockfall_processing_if_eligible legoobject_queue_for_rockfall_processing_if_eligible /* 0043a9e0 */
#define entity_pair_interaction_gate legoobject_pair_interaction_gate /* 0043a9f0 */
#define entity_proximity_activity_raise legoobject_proximity_activity_raise /* 0043aab0 */
#define terrain_cell_variant_update_queue legoobject_terrain_cell_variant_update_queue /* 0043abf0 */
#define predug_spawn_coordinate_select_and_create legoobject_predug_spawn_coordinate_select_and_create /* 0043ad50 */
#define creature_spawn_from_emergence legoobject_creature_spawn_from_emergence /* 0043af30 */
#define entity_iteration_callback_guard legoobject_remove_all /* 0043b320 */
#define entity_task_at_grid_coordinate_find legoobject_pick_at_screen_position /* 0043b360 */
#define entity_clipped_rectangle_callback_dispatch legoobject_select_in_screen_rect /* 0043b6c0 */
#define entity_type_two_event_dispatch legoobject_first_person_meshes_swap /* 0043b820 */
#define entity_type_two_positioned_callback_dispatch legoobject_lod_meshes_update /* 0043b850 */
#define entity_flags_mask107c33de_or_1c00203_any_test legoobject_flags_mask107c33de_or_1c00203_any_test /* 0043baf0 */
#define entity_building_deposit_state_monitor legoobject_building_deposit_state_monitor /* 0043bb20 */
#define entity_detach_carried_object_and_reset_state legoobject_detach_carried_object_and_reset_state /* 0043bbc0 */
#define objective_sidecar_append_gate legoobject_objective_sidecar_append_gate /* 0043bc40 */
#define entity_state_pair_predicate legoobject_state_pair_predicate /* 0043c200 */
#define entity_nested_float_add_unless_blocked legoobject_nested_float_add_unless_blocked /* 0043c280 */
#define entity_type26_command_completion_gate legoobject_type26_command_completion_gate /* 0043c3e0 */
#define ai_activity_probe legoobject_can_do_activity_classify /* 0043c440 */
#define entity_finish_state_1a legoobject_finish_state_1a /* 0043c4c0 */
#define entity_mark_and_finish_state_1a legoobject_mark_and_finish_state_1a /* 0043c530 */
#define entity_kind_four_post_create_resolve legoobject_kind_four_post_create_resolve /* 0043c570 */
#define entity_capacity_or_state_predicate legoobject_capacity_or_state_predicate /* 0043c650 */
#define entity_activity_counters_advance legoobject_activity_counters_advance /* 0043c6b0 */
#define entity_linked_route_flag_clear legoobject_linked_route_flag_clear /* 0043f100 */
#define entity_spawn_request_queue_add legoobject_spawn_request_queue_add /* 0043f150 */
#define entity_spawn_request_queue_process legoobject_spawn_request_queue_process /* 0043f190 */
#define entity_pool_find_with_local_predicate legoobject_pool_find_with_local_predicate /* 0043f560 */
#define entity_damage_apply legoobject_damage_apply /* 0043f6a0 */
#define entity_update_and_unregister_at_tile legoobject_update_and_unregister_at_tile /* 00440130 */
#define spatial_entity_damage_radius_callback legoobject_spatial_callback_query_visit /* 00440800 */
#define spatial_entity_vector_step_callback legoobject_spatial_callback_query_at_optional_coordinate_visit /* 00440920 */
#define ai_set_activity legoobject_set_activity /* 004409e0 */
#define object_list_field_assign_all legoobject_object_list_field_assign_all /* 00440bf0 */
#define terrain_grid_path_search legoobject_terrain_grid_path_search /* 00440c70 */
#define terrain_neighbor_path_check_current legoobject_terrain_neighbor_path_check /* 004410f0 */
#define entity_route_setup_and_marker_associate legoobject_route_setup_and_marker_associate /* 00441700 */
#define entity_action_sequence_reset legoobject_action_sequence_reset /* 00441940 */
#define entity_state_reset legoobject_state_reset /* 00441b30 */
#define entity_type5_finalize_if_allowed legoobject_type5_finalize_if_allowed /* 00441ea0 */
#define entity_face_target_action_start legoobject_face_target_action_start /* 00441ed0 */
#define kind_effect_slot_init_dispatch legoobject_kind_effect_slot_init_dispatch /* 004420d0 */
#define entity_action_route_state_initialize legoobject_action_route_state_initialize /* 00442210 */
#define entity_transform_position_get_outputs legoobject_transform_position_get /* 00442260 */
#define entity_surface_vector_normalize legoobject_surface_vector_normalize /* 004422a0 */
#define entity_velocity_vector_dispatch legoobject_velocity_vector_dispatch /* 00442300 */
#define entity_vector_direction_angle_radians_get legoobject_vector_direction_angle_radians_get /* 00442480 */
#define entity_position_to_terrain_cell legoobject_position_to_terrain_cell /* 004424f0 */
#define grid_operation_argument_adapter legoobject_world_z_callback /* 00442540 */
#define entity_world_position_set legoobject_world_position_set /* 004428a0 */
#define scene_object_pose_position_sync legoobject_scene_object_pose_position_sync /* 00443510 */
#define entity_random_activity_attempt legoobject_random_activity_attempt /* 00443670 */
#define entity_type3_flag04000000_test legoobject_type3_flag04000000_test /* 004436f0 */
#define entity_dispatch_event31_for_pending_flag legoobject_dispatch_event31_for_pending_flag /* 004437f0 */
#define entity_route_cell_candidate_accept legoobject_route_cell_candidate_accept /* 00443840 */
#define entity_path_step_advance_and_arrival_check legoobject_path_step_advance_and_arrival_check /* 004438b0 */
#define linked_entity_flag_and_command_update legoobject_linked_flag_and_command_update /* 004440a0 */
#define entity_enter_event21_state_once legoobject_enter_event21_state_once /* 00444620 */
#define entity_step_movement_vector_resolve legoobject_step_movement_vector_resolve /* 00444680 */
/* Port entries in entity.cpp (no FUNCTION annotation there). */
#define entity_action_target_cell_is_valid legoobject_action_target_cell_is_valid /* 0043fdc0 */
#define entity_route_to_explicit_target legoobject_route_to_explicit_target /* 0043fe70 */
#define entity_route_marker_setup legoobject_route_marker_setup /* 0043fff0 */
/* Original-ABI adapters other port tails give converted C units. A file
 * that defines them sets LEGORRR_LEGOOBJECT_ALIASES_NO_ADAPTERS first. */
#if !defined(LEGORRR_LEGOOBJECT_ALIASES_NO_ADAPTERS)
#define lego_object_can_eject legoobject_action_kind_supported /* 0043a7c0 */
#define lego_object_find_trainer legoobject_query_center_mask100 /* 00438e50 */
#define lego_object_set_primary_teleporter legoobject_flag80000000_set_or_clear /* 00438580 */
#define entity_custom_name_set_adapter legoobject_rename /* 00437880 */
#define lego_object_create_adapter legoobject_create /* 00437d00 */
#define entity_pool_find_by_callback_filtered_current_original legoobject_run_through_lists_skip_upgrade_parts /* 004377b0 */
/* Part 2 (#588): lego_port.cpp's, aitask_port.cpp's and interface_port.cpp's
 * original-ABI adapters, whose contract is the original's. */
#define simtick_scene_pass_original legoobject_update_all /* 0043b270 */
#define lego_object_pair_requirement_met legoobject_state_pair_predicate /* 0043c200 */
#define periodic_timer25_advance_original legoobject_periodic_timer25_advance /* 0043c2b0 */
#define lego_object_drop_carried legoobject_record_pair_pose_copy_and_activity_raise /* 0043fc20 */
#define lego_object_go_to_block legoobject_terrain_cell_to_world_position /* 004401b0 */
#define level_debug_cell_retarget_adapter legoobject_level_debug_cell_retarget_dispatch /* 004403d0 */
#define aitask_grid_route_search_adapter legoobject_grid_route_search_adapter /* 00440c30 */
#define entity_position_to_terrain_cell_adapter legoobject_position_to_terrain_cell /* 004424f0 */
#define aitask_cell_step_command_issue_adapter legoobject_cell_step_command_issue_with_jitter /* 0043a120 */
#endif
#endif

#endif /* LEGORRR_GAME_LEGOOBJECT_PORT_ALIASES_H */
