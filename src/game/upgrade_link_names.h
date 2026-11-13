// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-047 Upgrade.c (finding 617): the port's link names of the unit's
 * functions, for the matching build only (as findings 590 and 597).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's records: an upgrade part (0x10 bytes, from
 * lego_unit.c's upgrade table or a LegoObject's +0x20) or an upgrades model
 * (a vehicle's +0x124 or a building's +0x8c). So the
 * unit's definitions take the port's names, and the C callers (lego_unit.c,
 * legoobject_unit.c, building_model.c, vehicle_model.c) reach them with no
 * caller edit. The port tails tooltip_port.cpp and upgrade_levels_port.cpp
 * compile their own definitions of these names out of the matching build.
 * Natively the port tails keep serving them, over the port's own records.
 *
 * Included by upgrade.c and vehicle_model.c, before any other header. A macro renames a
 * declaration or a definition one for one, so MSVC's symbol numbering
 * (finding 538) does not change. Upgrade_Part_MoveAnimation and
 * Upgrade_Load keep the unit's names: their callers (legoobject_unit.c,
 * vehicle_model.c, building_model.c) call those names. */
#ifndef LEGORRR_GAME_UPGRADE_LINK_NAMES_H
#define LEGORRR_GAME_UPGRADE_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define upgrade_part_load render_resource_binding_initialize /* 0046c030 */
#define upgrade_part_clone render_resource_binding_copy_and_convert /* 0046c0b0 */
#define upgrade_part_remove render_resource_binding_release /* 0046c0f0 */
#define upgrade_part_set_activity scene_object_set_animation_with_fallback /* 0046c130 */
#define upgrade_set_upgrade_level scene_indexed_record_list_rebuild_for_value /* 0046c340 */
#endif

#endif /* LEGORRR_GAME_UPGRADE_LINK_NAMES_H */
