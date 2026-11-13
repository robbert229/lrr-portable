// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-002 AITask.c (finding 570): matching-build-only aliases from the
 * port's AITask.c entry points (and three port-tail adapters) to the
 * original-shape functions in aitask.c, for the C units that call them.
 *
 * The port's implementations stay in the port tail aitask_port.cpp,
 * unannotated, and native callers keep calling them: they work on the port's
 * CmdRec, not the original's 0x68-byte AITask, so the unit's functions cannot
 * replace them natively. In the matching build (MSVC 5.0, _MSC_VER 1100)
 * these C callers call the unit's functions instead, so their call operands
 * name the original functions again.
 *
 * Include it first in a caller's file, before any declaration of these
 * names (as legoobject_port_aliases.h, #581). Each macro renames a
 * declaration or a call one for one, so MSVC's symbol numbering (finding
 * 538) does not change. Every aliased entry has C linkage. Only C units
 * include it; not for aitask_port.cpp, which defines the port models. */
#ifndef LEGORRR_GAME_AITASK_PORT_ALIASES_H
#define LEGORRR_GAME_AITASK_PORT_ALIASES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define cmd_kind_by_name aitask_cmd_kind_by_name /* 00401cb0 */
#define cmd_registry_init_target aitask_cmd_registry_init_target /* 00401cf0 */
#define cmd_mark_matching_adapter aitask_cmd_mark_matching /* 00402000 */
#define handle_array_release_and_reset aitask_handle_array_release_and_reset /* 004020c0 */
#define cmd_flush_all aitask_cmd_flush_all /* 00402100 */
#define level_no_gather_object_setup aitask_level_no_gather_object_setup /* 00402170 */
#define cmd_tick_global_delta aitask_cmd_tick_global_delta /* 00402220 */
#define terrain_cell_command_create_adapter aitask_terrain_cell_command_create /* 00402510 */
#define entity_command15_validated_record_queue aitask_entity_command15_validated_record_queue /* 00402670 */
#define entity_deferred_work_schedule aitask_entity_deferred_work_schedule /* 004026c0 */
#define cmd_create_task aitask_cmd_create_task /* 00402720 */
#define entity_command14_batch_issue aitask_entity_command14_batch_issue /* 00402800 */
#define entity_kind_command_issue aitask_entity_kind_command_issue /* 00402960 */
#define coordinate_pair_callback_dispatch aitask_coordinate_pair_callback_dispatch /* 00402a40 */
#define argument_callback_dispatch aitask_argument_callback_dispatch /* 00402aa0 */
#define terrain_cell_refresh_dispatch_current aitask_terrain_cell_refresh_dispatch_current /* 00402ae0 */
#define coordinate_callback_dispatch_current aitask_coordinate_callback_dispatch_current /* 00402b30 */
#define cell_command_type13_issue_original aitask_cell_command_type13_issue /* 00402e40 */
#define coordinate_pair_event_queue aitask_coordinate_pair_event_queue /* 004030b0 */
#define scalar_task_record_queue aitask_scalar_task_record_queue /* 004034a0 */
#define task_record_kind10_queue aitask_task_record_kind10_queue /* 00403520 */
#define timed_task_record_initialize_and_link aitask_timed_task_record_initialize_and_link /* 004038c0 */
#define terrain_cell_record_counts_prune aitask_terrain_cell_record_counts_prune /* 00403910 */
#define task_record_create_and_scan_grid aitask_task_record_create_and_scan_grid /* 004039a0 */
#define task_record_kind11_timed_queue aitask_task_record_kind11_timed_queue /* 004039d0 */
#define task_callback_dispatch aitask_task_callback_dispatch /* 00403a10 */
#define entity_drain_command_chain aitask_entity_drain_command_chain /* 00403a50 */
#define entity_activity_record_remove_by_type aitask_entity_activity_record_remove_by_type /* 00403c00 */
#define entity_command_unlink_by_kind aitask_entity_command_unlink_by_kind /* 00403d10 */
#define ai_idle_chain_record_attach aitask_ai_idle_chain_record_attach /* 00403f30 */
#define task_record_kind14_remove_if_head aitask_task_record_kind14_remove_if_head /* 00403f60 */
/* GAME-050 Weapons.c (finding 582), for lego_unit.c (config init and
 * 00433180): here rather than through weapons_port_aliases.h because a new
 * #include moves lego_unit.c's symbol counter (finding 538) and costs exact
 * matches (00431450); a #define in a header it already includes does not. */
#define weapon_type_config_load weapons_initialise /* 0046eb80 */
#define entity_indexed_float_lookup_with_minus_one_fallback weapons_get_damage_for_object /* 0046f1d0 */
/* GAME-041 SFX.c (finding 585), for lego_unit.c's engine tick (004232e0):
 * sfx.c defines 00465030, which returns the volume, under the port's name
 * (sfx_link_names.h); the port's adapter is native-only. */
#define sound_variant_property_read_original sound_variant_property_read /* 00465030 */
/* GAME-028 NERPs.c (finding 586), for lego_unit.c and legoobject_unit.c
 * (which include this header first): the port's tutorial-script entries
 * whose contract is the original's. Natively they stay the port's. */
#define tutorial_script_file_load_target nerps_file_load_script_file /* 00452df0 */
#define objective_advisor_token_file_load nerps_file_load_message_file /* 00452e70 */
#define tutorial_display_resources_release nerps_shutdown /* 00453220 */
#define tutorial_script_command_dispatch nerps_runtime_execute /* 00453320 */
#define deferred_callback_set nerps_set_has_next_button /* 00453900 */
#define objective_deferred_callback_invoke_if_set nerps_replay_message_sound /* 00453910 */
#define tutorial_advisor_scroll_step_advance nerps_message_previous /* 00453920 */
#define tutorial_frame_timer_array_advance nerps_runtime_update_timers /* 00453bb0 */
#define global_counter_004ff990_reset nerps_monsters_destroyed_reset /* 00454890 */
#define objective_voice_alias_raise_commit nerpfunc_set_level_completed /* 00454b70 */
#define objective_voice_alias_raise_gate_bit27 nerpfunc_set_level_fail /* 00454b80 */
#define entity_type_grid_record_count_get nerps_object_count /* 00455300 */
#define system_state_flags_get_original nerpfunc_get_tutorial_flags /* 00456240 */
/* legoobject_unit.c (#610) calls 00456240 with the original's argument
 * through LegoObjH_NERPsTutorialFlags, a cast of the port's declaration. */
#define system_state_flags_get nerpfunc_get_tutorial_flags /* 00456240 */
#define system_state_flags_set_target nerpfunc_set_tutorial_flags /* 00456550 */
#define engine_mode_set_target nerpfunc_set_message_permit /* 004565f0 */
#define tutorial_path_coordinate_index nerps_init_block_pointers_table /* 00456b80 */
#define lego_object_pool_lists_free_all_adapter nerps_free_block_pointers /* 00456c60 */
/* GAME-012, the effects unit (finding 589): for legoobject_unit.c the
 * boulder explosion (0040bde0) and the smash path (0040beb0), and for
 * lego_unit.c the update (0040c3a0). Their port names stay the port's in
 * the matching build too: a C++ port model passes the first two a scene
 * object, not a LegoObject, and simtick.cpp runs the port's sweep on the
 * port's state. rock_fall_styles_link_names.h gives the unit's other
 * entries the port's names instead. */
#define scene_marker_create_at_object_position effect_spawn_boulder_explode /* 0040bde0 */
#define scene_marker_create_at_position_or_object effect_spawn_smash_path /* 0040beb0 */
#define scene_resource_availability_sweep effect_update_all /* 0040c3a0 */
/* GAME-024 LightEffects.c (finding 594), for lego_unit.c, the unit's only
 * original caller: the port's light-effect entries keep the port's records
 * (effects.cpp, util.cpp) and stay the port's in the matching build too;
 * lego_unit.c passes the original's arguments, so in that build it calls
 * the unit instead. */
#define scene_transform_state_initialize light_effects_initialise /* 0044c710 */
#define scene_transform_enable_set light_effects_set_disabled /* 0044c790 */
#define light_effect_configuration_load light_effects_load /* 0044c7c0 */
#define simtick_scene_position_reset_and_effect_motion_cancel light_effects_invalidate_position /* 0044cf70 */
#define motion_gameplay_tick light_effects_update /* 0044cfa0 */
#define simtick_route_position_blocked_state_set light_effects_set_dimmer_mode /* 0044da20 */
#endif

#endif /* LEGORRR_GAME_AITASK_PORT_ALIASES_H */
