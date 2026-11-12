// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-043 Stats.c (finding 581): the port's link names of the unit's
 * functions, for the matching build only.
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * functions passes the port's EntitySlot, whose fields Stats.c reads sit at
 * the original's offsets (type +0x000, id +0x004, objLevel +0x330, stats
 * +0x334), and the stats table is the unit's statsGlobs, which the port's
 * loader fills (entity.cpp). So the contract is the original's, and the
 * unit's definitions take the port's names: C and C++ callers then reach
 * them without an alias header or a caller edit, and the port tail
 * entity_stats_port.cpp compiles its own definitions of these names out of
 * the matching build. Natively the port tail keeps serving them: native
 * objects are EntitySlot, whose layout differs.
 *
 * Included only by entity_stats.c, before any other header. A macro renames
 * a declaration or a definition one for one, so MSVC's symbol numbering
 * (finding 538) does not change. Stats_Initialise (004667e0) is not here:
 * the port's loader has another contract. */
#ifndef LEGORRR_GAME_ENTITY_STATS_LINK_NAMES_H
#define LEGORRR_GAME_ENTITY_STATS_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define stats_add_tool_task_type entity_indexed_value_append /* 00469a90 */
#define stats_get_cost_ore entity_indexed_record_word_at_b0_get /* 00469ac0 */
#define stats_get_cost_crystal entity_indexed_record_word_at_b4_get /* 00469af0 */
#define stats_get_cost_refined_ore stats_table_word_at_b8 /* 00469b20 */
#define stats_object_get_crystal_drain stats_instance_word_at_bc /* 00469b50 */
#define stats_object_get_capacity stats_instance_word_at_e0 /* 00469b80 */
#define stats_object_get_max_carry stats_instance_word_at_ec /* 00469bb0 */
#define stats_object_get_carry_start stats_instance_word_at_f0 /* 00469be0 */
#define stats_object_set_object_level entity_building_index_set /* 00469c10 */
#define stats_object_get_route_speed entity_stats_float_at20 /* 00469cb0 */
#define stats_object_get_drill_time_type entity_stats_float_at24_indexed /* 00469cc0 */
#define stats_object_get_rubble_coef entity_stats_float_atd4 /* 00469ce0 */
#define stats_object_get_wake_radius entity_stats_float_at9c /* 00469d00 */
#define stats_object_get_path_coef entity_stats_float_atd8 /* 00469d20 */
#define stats_object_get_coll_radius entity_stats_float_at6c /* 00469d40 */
#define stats_object_get_coll_height entity_stats_float_at74 /* 00469d50 */
#define stats_object_get_pick_sphere entity_stats_float_at78 /* 00469d60 */
#define stats_object_get_pain_threshold entity_stats_float_ate8 /* 00469d70 */
#define stats_object_get_alert_radius entity_stats_float_at70 /* 00469d90 */
#define stats_object_get_coll_box entity_stats_field_at_a0_corpus /* 00469da0 */
#define stats_object_get_track_dist entity_stats_float_at7c /* 00469db0 */
#define stats_object_get_health_decay_rate entity_stats_float_at80 /* 00469dc0 */
#define stats_object_get_energy_decay_rate entity_energy_decay_rate_get /* 00469de0 */
#define stats_get_oxygen_coef stats_table_float_at_dc /* 00469e00 */
#define stats_object_get_oxygen_coef entity_stats_float_atdc /* 00469e20 */
#define stats_object_get_survey_radius entity_stats_word_atac /* 00469e40 */
#define stats_object_get_stats_flags1 entity_stats_word_get /* 00469e60 */
#define stats_object_get_stats_flags2 entity_state_flags_get /* 00469e80 */
#define stats_object_get_stats_flags3 entity_capability_flags_get /* 00469ea0 */
#define stats_get_stats_flags1 stats_table_word_at_144 /* 00469ec0 */
#define stats_get_stats_flags2 stats_table_word_at_148 /* 00469ee0 */
#define stats_get_stats_flags3 stats_table_word_at_14c /* 00469f00 */
#define stats_object_get_repair_value entity_stats_float_at88 /* 00469f20 */
#define stats_get_levels building_upgrade_admission_check /* 00469f40 */
#define stats_get_water_entrances stats_table_word_at_d0 /* 00469f60 */
#define stats_object_get_drill_sound_type entity_stats_word_at_c0_or_c4 /* 00469f90 */
#define stats_object_get_engine_sound entity_stats_word_atc8 /* 00469fc0 */
#define stats_object_get_rest_percent entity_stats_float_at8c /* 00469fe0 */
#define stats_object_get_carry_min_health entity_stats_float_at90 /* 0046a000 */
#define stats_object_get_attack_radius entity_stats_float_at98 /* 0046a020 */
#define stats_object_get_stamp_radius entity_stats_float_at94 /* 0046a040 */
#define stats_object_get_num_of_tools_can_carry ai_activity_cap /* 0046a060 */
#define stats_object_get_upgrade_time entity_stats_float_atf4 /* 0046a080 */
#define stats_object_get_function_coef entity_speed_component_get /* 0046a0a0 */
#define stats_get_upgrade_cost_ore entity_mapped_indexed_record_word_at_fc_get /* 0046a0c0 */
#define stats_get_upgrade_cost_studs stats_table_word_at_100 /* 0046a0f0 */
#define stats_find_tool_from_task_type entity_grouped_code_table_find_value_index /* 0046a120 */
#define stats_object_get_flocks_height entity_stats_float_at130 /* 0046a170 */
#define stats_object_get_flocks_randomness entity_stats_float_at134 /* 0046a190 */
#define stats_object_get_flocks_turn entity_stats_float_at120 /* 0046a1b0 */
#define stats_object_get_flocks_tightness entity_stats_float_at128 /* 0046a1d0 */
#define stats_object_get_flocks_speed entity_stats_float_at124 /* 0046a1f0 */
#define stats_object_get_flocks_size entity_stats_word_at13c /* 0046a210 */
#define stats_object_get_flocks_goal_update entity_stats_float_at12c /* 0046a230 */
#define stats_object_get_flocks_attack_time entity_stats_float_at138 /* 0046a250 */
#define stats_object_get_awareness_range entity_collision_radius_get /* 0046a270 */
#define stats_object_get_pusher_dist entity_stats_float_at114 /* 0046a290 */
#define stats_object_get_pusher_damage entity_stats_float_at118 /* 0046a2b0 */
#define stats_object_get_laser_damage entity_stats_float_at11c /* 0046a2d0 */
#define stats_object_get_freezer_damage entity_stats_float_at110 /* 0046a2f0 */
#define stats_object_get_freezer_time entity_stats_float_at10c /* 0046a310 */
#define stats_object_debug_toggle_self_powered entity_record_flag400_toggle_and_refresh /* 0046a330 */
#endif

#endif /* LEGORRR_GAME_ENTITY_STATS_LINK_NAMES_H */
