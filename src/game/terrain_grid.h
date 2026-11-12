// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Reward.c (GAME-037, finding 560):
 * Lego.c's current level (terrain_grid.hpp).
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_TERRAIN_GRID_H
#define LEGORRR_GAME_TERRAIN_GRID_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct LevelState;
extern struct LevelState* g_active_level_retail;            /* DAT_005560d4 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_TERRAIN_GRID_H */
