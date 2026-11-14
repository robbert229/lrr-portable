// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-023 LegoObject.c part 1 (issue #575): matching-build-only aliases
 * from the port's LegoObject.c entry points to the original-shape functions
 * in legoobject_unit.c.
 *
 * The port's implementations stay in legoobject_port.cpp, unannotated, and native
 * callers keep calling them: they read the port's EntitySlot, not the
 * original's 0x40c-byte record, so the unit's functions cannot replace them
 * natively. In the matching build (MSVC 5.0, _MSC_VER 1100) the callers
 * whose contract is already the original's call the unit's functions
 * instead, so their call operands name the original functions again.
 *
 * Include it first in a caller's file, before any declaration of these
 * names. Each macro renames a declaration or a call one for one, so MSVC's
 * symbol numbering (finding 538) does not change. Every aliased entry has
 * C linkage. Not for legoobject_port.cpp, which defines the port models. */
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
/* GAME-023 part 3 (issue #615): 00445d70-0044c710. The port's entries in
 * namespaces (lego::, unported_ledger_stubs::), the overloaded
 * selection_clear_marker_and_reset_hud and the entries with other
 * contracts are not aliased. */
#define entity_route_adjacent_step_append legoobject_route_adjacent_step_append /* 004468c0 */
#define entity_progress_advance_by_25 legoobject_progress_advance_by_25 /* 00447730 */
#define entity_clear_linked_object_state legoobject_clear_linked_object_state /* 00447780 */
#define entity_type_four_callback_dispatch legoobject_type_four_callback_dispatch /* 00447900 */
#define entity_directional_activity_start legoobject_directional_activity_start /* 00447950 */
#define entity_dispatch_event5_and_set_flag legoobject_dispatch_event5_and_set_flag /* 00447b00 */
#define activity_register_bookkeeping legoobject_activity_register_bookkeeping /* 00447c40 */
#define entity_clear_nested_state_8_fields legoobject_clear_nested_state_8_fields /* 004487c0 */
#define entity_set_activity_place legoobject_set_activity_place /* 00448c50 */
#define scene_object_has_task_type_4_5_7 legoobject_has_task_type_4_5_7 /* 004493e0 */
#define entity_activity23_state_transition legoobject_activity23_state_transition /* 00449a70 */
#define entity_type_operation_release legoobject_type_operation_release /* 00449ff0 */
#define entity_follow_target_pose_get legoobject_follow_target_pose_get /* 0044a070 */
#define entity_type_component_get legoobject_type_component_get /* 0044a200 */
#define type11_cell_coordinate_append legoobject_type11_cell_coordinate_append /* 0044a390 */
#define predug_coordinate_pool_append legoobject_predug_coordinate_pool_append /* 0044a4b0 */
#define terrain_object_sfx_grid_config_load legoobject_sfx_grid_config_load /* 0044acc0 */
#define terrain_object_type_sfx_at_cell_get legoobject_sfx_at_cell_get /* 0044ad80 */
#define scatter_spawn_gate_set legoobject_scatter_spawn_gate_set /* 0044adc0 */
#define entity_kind_one_random_spawn_trigger legoobject_kind_one_random_spawn_trigger /* 0044ade0 */
#define ai_activity_scan legoobject_activity_scan /* 0044b480 */
#define ai_can_do legoobject_ai_can_do /* 0044c530 */
#define global_scatter_spawn_enable_and_drain legoobject_scatter_spawn_enable_and_drain /* 0044c550 */
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
/* Part 3 (#615): aitask_port.cpp's and lego_port.cpp's original-ABI
 * adapters, whose contract is the original's. */
#define aitask_nearest_cell_route_commit_adapter legoobject_nearest_cell_route_commit /* 00446e40 */
#define aitask_type_five_spawn_adapter legoobject_conditional_type_five_spawn /* 004470d0 */
#define aitask_move_to_cell_adapter legoobject_move_to_cell_coordinates /* 00448800 */
#define aitask_gated_step_record_adapter legoobject_record_gated_step_command_issue /* 00448880 */
#define aitask_neighbor_cell_move_adapter legoobject_neighbor_cell_move_command_handle /* 00448a60 */
#define aitask_cardinal_step_link_adapter legoobject_cardinal_step_command_issue_and_link /* 00448c90 */
#define aitask_proximity_step_jitter_adapter legoobject_proximity_step_command_issue_with_jitter /* 00448eb0 */
#define aitask_kind6_approach_route_adapter legoobject_kind6_record_approach_route_issue /* 00449010 */
#define aitask_proximity_step_link_adapter legoobject_proximity_step_command_issue_with_link /* 004490a0 */
#define aitask_walkable_cell_route_adapter legoobject_walkable_target_cell_route_issue /* 00449240 */
#define aitask_cell_step_mark_adapter legoobject_cell_step_command_issue_with_target_mark /* 004492b0 */
#define aitask_target_cell_route_associate_adapter legoobject_target_cell_route_and_associate /* 00449430 */
#define aitask_target_cell_route_marker14_adapter legoobject_target_cell_route_marker14 /* 00449520 */
#define aitask_coordinate_route_mark_adapter legoobject_coordinate_route_and_mark /* 00449610 */
#define aitask_route_step_adapter legoobject_route_step_command_issue /* 00449700 */
#define aitask_proximity_step_clearing_adapter legoobject_proximity_step_command_issue_clearing_gate /* 00449ac0 */
#define aitask_type11_nearest_resolve_adapter legoobject_type11_cell_coordinate_nearest_resolve /* 0044a3d0 */
#define aitask_position_step_link_adapter legoobject_position_step_command_issue_and_link /* 0044b500 */
#define simtick_render_step_original legoobject_simtick_render_step /* 00449c00 */
/* #699 (finding 626): lego_port.cpp's remaining original-ABI adapters for
 * lego_unit.c, whose contract is the original's. */
#define spawn_registry_probe_adapter legoobject_unknown_00438960 /* 00438960 */
#define nearest_open_cell_search_adapter legoobject_nearest_open_cell_coord_search /* 0044a5d0 */
#define entity_try_create_linked_at_position_original legoobject_try_create_linked_at_position /* 004471b0 */
#define entity_pair_scalar_fields_set_original legoobject_pair_scalar_fields_set /* 0044c150 */
#define entity_damage_apply_original legoobject_damage_apply /* 0043f6a0 */
#define simulation_update_counter_add_original legoobject_simulation_update_counter_add /* 0043a8f0 */
#define simulation_mode_counter_add_original legoobject_simulation_mode_counter_add /* 0043a910 */
#define entity_type_operation_dispatch_adapter legoobject_type_operation_dispatch /* 00449f50 */
#define ledger_stub_0044b140_SmallSpiderCellSwarmSpawn legoobject_small_spider_cell_swarm_spawn /* 0044b140 */
/* #730 (finding 638): aitask_port.cpp's last two LegoObject.c adapters.
 * aitask.c passes 00443710 a fourth argument, which the original pushes
 * and the unit's function does not read. */
#define aitask_adjacent_position_find_adapter legoobject_adjacent_position_candidate_find /* 00443710 */
#define aitask_object_command_gate_adapter legoobject_type_two_target_resolve_and_apply /* 0044b4c0 */
#endif
#endif

#endif /* LEGORRR_GAME_LEGOOBJECT_PORT_ALIASES_H */
