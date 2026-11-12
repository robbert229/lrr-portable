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
#endif

#endif /* LEGORRR_GAME_AITASK_PORT_ALIASES_H */
