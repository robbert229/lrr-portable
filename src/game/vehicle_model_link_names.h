// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-048 Vehicle.c (finding 590): the port's link names of the unit's
 * functions, for the matching build only (as findings 581 and 585).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes a vehicle model of the original's layout: lego_unit.c's
 * vehicle table (Lego.c loads it through vehicle_load) and LegoObject's
 * +0x00c, which legoobject_unit.c fills with vehicle_clone. So the unit's
 * definitions take the port's names: the C callers (lego_unit.c,
 * legoobject_unit.c) and the C++ ones (legoobject.cpp, message_unit.cpp,
 * record.cpp, upgrade_levels.cpp) reach them without an alias header or a
 * caller edit, and the port tail vehicle_model_port.cpp compiles its own
 * definitions of these names out of the matching build. Natively the port
 * tail keeps serving them, over the port's own vehicle records.
 *
 * Two of the names are original-ABI adapters the unit replaces:
 * lego_vehicle_load_adapter (lego_port.cpp, for 0046c6f0) and
 * scene_record_indexed_entries_rebuild_original (vehicle_model_port.cpp,
 * for 0046cf90). Both did nothing.
 *
 * Included only by vehicle_model.c, before any other header. A macro
 * renames a declaration or a definition one for one, so MSVC's symbol
 * numbering (finding 538) does not change. The five functions whose only
 * callers are port code with another contract (0046d1c0, 0046d380,
 * 0046d9f0, 0046da50, 0046db70) keep the unit's names. */
#ifndef LEGORRR_GAME_VEHICLE_MODEL_LINK_NAMES_H
#define LEGORRR_GAME_VEHICLE_MODEL_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define vehicle_load lego_vehicle_load_adapter /* 0046c6f0 */
#define vehicle_remove entity_payload_type1_release /* 0046ce10 */
#define vehicle_swap_poly_medium paired_resource_state_set /* 0046ced0 */
#define vehicle_swap_poly_high scene_entity_animation_tracks_set_enabled /* 0046cf20 */
#define vehicle_can_upgrade_type record_flag_bit_test /* 0046cf40 */
#define vehicle_get_upgrade_level entity_type_mask_get /* 0046cf80 */
#define vehicle_set_upgrade_level scene_record_indexed_entries_rebuild_original /* 0046cf90 */
#define vehicle_hide_wheels six_resource_slots_set_state /* 0046cfc0 */
#define vehicle_clone scene_record_template_instantiate /* 0046cff0 */
#define vehicle_set_owner_object scene_assign_owner_from_model /* 0046d140 */
#define vehicle_get_animation_time scene_object_attached_resource_release /* 0046d1a0 */
#define vehicle_populate_wheel_nulls scene_resource_slots_rebuild /* 0046d260 */
#define vehicle_hide scene_resource_visibility_set /* 0046d2c0 */
#define vehicle_is_hidden entity_type_flag_bit1_get_target /* 0046d330 */
#define vehicle_set_orientation scene_resource_draw_at_origin /* 0046d350 */
#define vehicle_get_activity_container scene_object_resource_get /* 0046d9e0 */
#define vehicle_get_drill_null scene_resource_cached_slot_b4_get /* 0046da90 */
#define vehicle_get_deposit_null scene_resource_cached_slot_b8_get /* 0046dac0 */
#define vehicle_get_driver_null scene_resource_cached_slot_c0_get /* 0046daf0 */
#define vehicle_get_carry_null attached_entity_slot_object_get /* 0046db20 */
#define vehicle_get_carry_null_frames object_record_field11c_get /* 0046db60 */
#endif

#endif /* LEGORRR_GAME_VEHICLE_MODEL_LINK_NAMES_H */
