// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-049 Water.c (finding 614): the port's link names of the unit's
 * functions that Lego.c calls (as findings 590, 600, 607 and 611).
 *
 * In the matching build (MSVC 5.0, _MSC_VER 1100) every caller of these
 * names passes the original's arguments, so the unit's definitions take
 * the port's names: Lego.c (lego_unit.c, lego_unit_imports.h) reaches them
 * with no caller edit. The port tail cell_flood_passes_port.cpp compiles
 * its definitions of these names out of the matching build
 * (LRR_WATER_PORT_NAMES). Both port names have C linkage.
 *
 * Lego.c's four other callees in this unit keep the unit's names: their
 * port definitions are in gods98/view.cpp, game/terrain_grid.cpp and
 * game/lego_port.cpp, so lego_unit_imports.h points Lego.c at the unit's
 * names in the matching build instead.
 *
 * Included only by water.c, before any other header. A macro renames a
 * definition one for one, so MSVC's symbol numbering (finding 538) does
 * not change. */
#ifndef LEGORRR_GAME_WATER_LINK_NAMES_H
#define LEGORRR_GAME_WATER_LINK_NAMES_H

#if defined(_MSC_VER) && _MSC_VER < 1200
#define water_initialise terrain_cell_quad_run_stage_level               /* 0046db90 */
#define water_destroy_wall_complete terrain_cell_surface_quad_height_fill_current /* 0046e220 */
#endif

#endif /* LEGORRR_GAME_WATER_LINK_NAMES_H */
