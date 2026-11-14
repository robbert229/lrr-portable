// SPDX-License-Identifier: LGPL-3.0-or-later
/* SelectPlace.c (GAME-040) imports: C declarations of the callees and
 * globals the unit uses from other modules (ADR 0005 step 6). Included only
 * by selectplace.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header"). */
#ifndef LEGORRR_GAME_SELECTPLACE_IMPORTS_H
#define LEGORRR_GAME_SELECTPLACE_IMPORTS_H

#include <stdint.h>

#include "gods98/container_unit.h"
#include "lego_unit.h"
#include "terrain_map.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GAME-021 Lego.c (lego_unit.h): legoGlobs.currLevel (terrain_grid.cpp's
 * g_active_level_retail, legoGlobs_currLevel), Lego_Level, and the two
 * placement checks. */
struct LevelState;
extern struct LevelState* g_active_level_retail;            /* 005560d4 */
uint32_t terrain_cell_flag_8000_current(const int cell_xy[2]);   /* 00433030 */
uint32_t level_cell_occupied_at_current(const int cell_xy[2]);   /* 00433090 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_SELECTPLACE_IMPORTS_H */
