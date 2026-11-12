// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-021 Lego.c part 3 (issue #587): matching-build-only aliases from the
 * port's Lego.c entry points to the original-shape functions in
 * lego_unit.c, in the pattern of legoobject_port_aliases.h (#581).
 *
 * The port's implementations stay in lego_port.cpp, and native callers keep
 * calling them: they read the port's LevelState, EntitySlot and config
 * models, and the unit's functions read the original's records and the
 * legoGlobs stand-ins. Each entry below has the original's contract (same
 * arguments and meaning; LevelState starts with the word image of the
 * original Lego_Level). In the matching build (MSVC 5.0, _MSC_VER 1100) the
 * callers that include this header call the unit's function instead, so the
 * original's call graph reaches it and its call operands name it.
 *
 * Include it first in a caller's file, before any declaration of these
 * names. Each macro renames a declaration or a call one for one, so MSVC's
 * symbol numbering (finding 538) does not change. Every aliased declaration
 * has C linkage. Not for lego_port.cpp, which defines the port models, and
 * not for files that also declare the unit's name (a renamed declaration
 * would become a redeclaration). Macros only: this header must not declare
 * anything. */
#ifndef LEGORRR_GAME_LEGO_PORT_ALIASES_H
#define LEGORRR_GAME_LEGO_PORT_ALIASES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define engine_game_config_callback_registry_init lego_game_config_callback_registry_init /* 0041fa20 */
#define ledger_stub_004263c0_GlobalSubsystemStateReset lego_global_subsystem_state_reset /* 004263c0 */
#define level_upgrade_name_strings_load lego_level_upgrade_name_strings_load /* 00427f80 */
#define entity_command_status_feedback_dispatch lego_entity_command_status_feedback_dispatch /* 00428020 */
#define media_min_distance_update_mode lego_sound_minimum_distance_update_mode /* 004295f0 */
#define media_cd_watchdog_update lego_cd_audio_watchdog_mode_update /* 004297b0 */
#define media_audio_playback_toggle lego_audio_playback_toggle /* 00429810 */
#define terrain_tile_mesh_grid_load_level lego_load_tile_meshes /* 0042b330 */
#define terrain_texture_set_config_load_level lego_texture_set_config_load /* 0042bb60 */
#define upgrade_name_entry_find lego_upgrade_name_entry_find /* 0042cd50 */
#define entity_vehicle_type_table_load lego_entity_vehicle_type_table_load /* 0042cda0 */
#define entity_minifigure_type_table_load lego_entity_minifigure_type_table_load /* 0042cf50 */
#define entity_rockmonster_type_table_load lego_entity_rockmonster_type_table_load /* 0042d100 */
#define entity_building_types_load lego_entity_building_types_load /* 0042d2b0 */
#define terrain_cell_edge_flag_set_and_update lego_terrain_cell_edge_flag_set_and_update /* 0042f2e0 */
#define terrain_neighbor_flag_set lego_terrain_neighbor_flag_set /* 00430470 */
#define terrain_cell_mutate lego_terrain_cell_mutate /* 00430530 */
#define terrain_cell_initialize lego_terrain_cell_initialize /* 00430ee0 */
#define terrain_stamp_impact_handle_current lego_terrain_stamp_impact_handle /* 00431780 */
#define terrain_cell_material_get_current lego_terrain_cell_material_get /* 004319b0 */
#define terrain_placement_blocked_adjacent_cell_scan lego_terrain_placement_blocked_adjacent_cell_scan /* 00431b20 */
#define terrain_cell_route_and_occupancy_check_current lego_terrain_cell_route_and_occupancy_check /* 00431da0 */
#endif

#endif /* LEGORRR_GAME_LEGO_PORT_ALIASES_H */
