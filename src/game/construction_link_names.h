// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-052 Construction.c (finding 603): the port's link names of the
 * unit's functions, for the matching build only (as finding 599's
 * fallin_link_names.h).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments: a block position (a Point2I), a
 * zone handle, an object type and index, a zone. So the unit's definitions
 * take the port's names (C linkage: util.hpp, cmd_shared.hpp, level.hpp,
 * record.hpp, simtick.hpp, interface_callees.h): lego_unit.c,
 * legoobject_unit.c, interface_unit.c, aitask.c and entity_stats.c reach
 * them without a caller edit, and the port tail (placement_port.cpp,
 * level.cpp's 00409300 model, interface_port.cpp's 00409600 adapter)
 * compiles its own definitions of these names out of the matching build.
 * Natively the port keeps serving them.
 *
 * Included only by construction.c, before any other header. A macro renames
 * a definition one for one, so MSVC's symbol numbering (finding 538) does
 * not change. */
#ifndef LEGORRR_GAME_CONSTRUCTION_LINK_NAMES_H
#define LEGORRR_GAME_CONSTRUCTION_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define construction_get_building_base string_table_index_lookup /* 00408c80 */
#define construction_zone_place_resource cmd_operation_group_append_object /* 00408ce0 */
#define construction_zone_needs_more_of_resource terrain_descriptor_coordinate_count_limit_check /* 00408d70 */
#define construction_zone_count_of_resource_placed count_matching_coordinates_in_list /* 00408e10 */
#define construction_zone_exists_at_block terrain_cell_predicate_check_current /* 004090a0 */
#define construction_zone_find_by_handle_or_at_block terrain_descriptor_find_exact_current /* 004090c0 */
#define construction_zone_complete_path simtick_selection_group_release /* 00409110 */
#define construction_zone_cancel_path simtick_selection_group_clear /* 00409150 */
#define construction_zone_start_path tutorial_path_descriptor_create /* 00409300 */
#define construction_zone_cancel_building lego_path_remove /* 00409600 */
#define construction_update_surrounding_blocks terrain_cell_refresh_eight_neighbors_current /* 00409940 */
#define construction_remove_all path_record_release_all /* 004099d0 */
#endif

#endif /* LEGORRR_GAME_CONSTRUCTION_LINK_NAMES_H */
