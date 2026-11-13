// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-015 Erode.c (finding 599): the port's link names of the unit's
 * functions, for the matching build only (as finding 597's
 * tooltip_link_names.h).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments: a block position (two ints), the
 * erode speed, the elapsed time, the three Lego.cfg times. So the unit's
 * definitions take the port's names (level.hpp, C linkage): lego_unit.c and
 * the port's C++ callers reach them without an alias or a caller edit, and
 * the port tail (lava_erosion_port.cpp, and level.cpp's
 * lava_erosion_stage_register) compiles its own definitions of these names
 * out of the matching build. Natively the port tail keeps serving them.
 *
 * Included only by erode.c, before any other header. A macro renames a
 * definition one for one, so MSVC's symbol numbering (finding 538) does not
 * change. */
#ifndef LEGORRR_GAME_ERODE_LINK_NAMES_H
#define LEGORRR_GAME_ERODE_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define erode_initialise erosion_timing_state_init /* 0040e930 */
#define erode_get_free_activity lava_erosion_stage_slot_find_free /* 0040e990 */
#define erode_get_block_erode_rate lava_erosion_delay_from_cell /* 0040e9c0 */
#define erode_set_block_erode_rate lava_erosion_stage_register /* 0040ea10 */
#define erode_update lava_erosion_pending_cell_tick /* 0040eab0 */
#define erode_add_locked_block cell_coordinate_timestamp_remember /* 0040ee00 */
#define erode_set_active_block lava_erosion_pending_cell_add /* 0040ee50 */
#define erode_is_block_locked cell_coordinate_timestamp_exists /* 0040efb0 */
#define erode_find_adjacent_block lava_erosion_neighbor_check /* 0040f000 */
#endif

#endif /* LEGORRR_GAME_ERODE_LINK_NAMES_H */
