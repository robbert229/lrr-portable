// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-051 Fallin.c (finding 599): the port's link names of the unit's
 * functions, for the matching build only (as finding 597's
 * tooltip_link_names.h).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments: a block position (two ints), a
 * direction mask, the cave-in flag, the elapsed time, the cave-in limit. So
 * the unit's definitions take the port's names (level.hpp, cmd.hpp and
 * crt.hpp, C linkage): lego_unit.c and the port's C++ callers reach them
 * without an alias or a caller edit, and the port tail
 * (lava_erosion_port.cpp) compiles its own definitions of these names out of
 * the matching build. Natively the port tail keeps serving them.
 *
 * Included only by fallin.c, before any other header. A macro renames a
 * definition one for one, so MSVC's symbol numbering (finding 538) does not
 * change. */
#ifndef LEGORRR_GAME_FALLIN_LINK_NAMES_H
#define LEGORRR_GAME_FALLIN_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define fallin_update kind_six_random_sweep_dispatch /* 0040f0e0 */
#define fallin_try_generate_land_slide lava_erosion_cell_eligible_and_mutate /* 0040f190 */
#define fallin_can_land_slide_at_block lava_erosion_neighbor_eligible_current /* 0040f2b0 */
#define fallin_generate_land_slide lava_erosion_neighbor_mutate /* 0040f330 */
#define fallin_initialise global_config_value_set /* 0040f5e0 */
#define fallin_generate_cave_in lava_erosion_eight_neighbor_propagate /* 0040f5f0 */
#endif

#endif /* LEGORRR_GAME_FALLIN_LINK_NAMES_H */
