// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-006 Building.c (finding 592): the port's link names of the unit's
 * functions, for the matching build only (as findings 581, 585 and 590).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) the C callers of these
 * names pass a building model of the original's layout: lego_unit.c's
 * building table (legoGlobs_buildingData, 0x14c bytes a type), which Lego.c
 * loads through building_load, and LegoObject's +0x018, which
 * legoobject_unit.c fills with building_clone. So the unit's definitions
 * take the port's names: the C callers (aitask.c, lego_unit.c,
 * legoobject_unit.c, terrain_cell_objects.c) and the C++ ones reach them
 * without an alias header or a caller edit, and the port tail
 * building_model_port.cpp compiles its own definitions of these names out
 * of the matching build. Natively the port tail keeps serving them.
 *
 * Included only by building_model.c, before any other header. A macro
 * renames a definition one for one, so MSVC's symbol numbering (finding
 * 538) does not change. Two functions keep the unit's names:
 * 004083a0, whose port name only port code with another contract calls,
 * and 00408570, which has no port name (legoobject_unit.c calls
 * building_set_activity directly). */
#ifndef LEGORRR_GAME_BUILDING_MODEL_LINK_NAMES_H
#define LEGORRR_GAME_BUILDING_MODEL_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define building_load entity_building_type_config_record_parse /* 00407d60 */
#define building_change_power_level scene_object_scalar_adjust_by_direction /* 004082e0 */
#define building_get_shape_points task_field84_get_and_field88_out /* 00408360 */
#define building_set_owner_object scene_assign_entity_owner /* 00408380 */
#define building_get_camera_null scene_activity_change_secondary /* 004085f0 */
#define building_clone scene_object_record_clone_retain_handles /* 00408620 */
#define building_hide scene_object_pair_apply_mode /* 00408670 */
#define building_is_hidden animation_object_visibility_get /* 004086a0 */
#define building_set_orientation scene_object_pair_set_render_parameters /* 004086c0 */
#define building_set_position scene_object_pair_set_callback_scalar /* 00408710 */
#define building_get_carry_null scene_object_indexed_handle_get_or_create /* 00408770 */
#define building_get_deposit_null scene_object_primary_handle_get_or_create /* 004087b0 */
#define building_get_entrance_null scene_object_secondary_handle_get_or_create /* 004087e0 */
#define building_get_tool_null scene_object_indexed_secondary_handle_get_or_create /* 00408810 */
#define building_get_carry_null_frames scene_object_flags_get /* 00408850 */
#define building_move_animation animation_loop_time_adjust /* 00408860 */
#define building_get_trans_coef constant_zero_float_get /* 00408930 */
#define building_remove entity_payload_teardown /* 00408940 */
#define building_can_upgrade_type task_flag_bit_query /* 00408970 */
#define building_set_upgrade_level entity_payload_state_set /* 004089a0 */
#endif

#endif /* LEGORRR_GAME_BUILDING_MODEL_LINK_NAMES_H */
