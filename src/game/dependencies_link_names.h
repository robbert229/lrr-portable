// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-010 Dependencies.c (finding 612): the port's link names of the
 * unit's functions (as findings 597, 600 and 607).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments: an enable word, the config and the
 * game name, an object type, index and level (with the requirement list's
 * outputs, or the dependents record of four 32-word arrays and a count),
 * or a live object and a requirement. So the unit's definitions take the
 * port's names, and the C callers (lego_unit.c, aitask.c, helpwindow.c,
 * interface_unit.c) and the C++ ones (frameloop.cpp, aitask_port.cpp,
 * interface_port.cpp, dependency_grid.cpp) reach them with no caller edit.
 * What served these names is compiled out of the matching build:
 * - dependencies_port.cpp's two definitions (LRR_DEPENDENCIES_PORT_NAMES);
 * - screen_effect_quads.cpp's 0040ab10 and 0040ab30;
 * - the PORT-ONLY adapters in lego_port.cpp (0040ab70) and
 *   helpwindow_port.cpp (0040b000, 0040b1b0, 0040b250, 0040b2a0,
 *   0040b2e0), and interface_port.cpp's stub of 0040af90.
 * Every port name here has C linkage.
 *
 * Included only by dependencies.c, before any other header. A macro renames
 * a definition one for one, so MSVC's symbol numbering (finding 538) does
 * not change. */
#ifndef LEGORRR_GAME_DEPENDENCIES_LINK_NAMES_H
#define LEGORRR_GAME_DEPENDENCIES_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define dependencies_set_enabled global_mode_zero_input_set                       /* 0040ab10 */
#define dependencies_reset_clear_all_level_flags_10c grid_record_flag_mask_clear  /* 0040ab30 */
#define dependencies_initialise dependencies_initialise_adapter                   /* 0040ab70 */
#define dependencies_object_requirements_met ensure_cell_entries_loaded           /* 0040aea0 */
#define dependencies_live_object_callback terrain_cell_resource_entry_matches     /* 0040af40 */
#define dependencies_object_get_requirements dependencies_get_requirements        /* 0040af90 */
#define dependencies_object_find_dependents help_window_dependencies_matches_adapter /* 0040b000 */
#define dependencies_update help_window_dependencies_pass_adapter                 /* 0040b1b0 */
#define dependencies_object_is_unlocked help_window_dependencies_flag4_adapter    /* 0040b250 */
#define dependencies_object_add_level_flag_100 help_window_dependencies_flag100_set_adapter /* 0040b2a0 */
#define dependencies_object_get_level_flag_100 help_window_dependencies_flag100_adapter /* 0040b2e0 */
#endif

#endif /* LEGORRR_GAME_DEPENDENCIES_LINK_NAMES_H */
