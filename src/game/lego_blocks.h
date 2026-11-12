// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for the electric fences (GAME-013,
 * finding 566): Lego.c (GAME-021) block queries declared in level.hpp, and the
 * port-tail adapter for Level_BlockUpdateSurface.
 * Only C units and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GAME_LEGO_BLOCKS_H
#define LEGORRR_GAME_LEGO_BLOCKS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct LevelState;

uint32_t terrain_cell_secondary_flag_100_current(const int cell_xy[2]); /* 00432170 */
uint32_t terrain_cell_occupied_or_flag_current(int cx, int cy, int mode); /* 00432aa0 */
/* 0042f6f0 Level_BlockUpdateSurface. The port entry takes the column as a
 * float; this adapter, in the GAME-013 port tail, converts it. */
void level_block_surface_update(struct LevelState* level, int bx, int by,
                                int reserved);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_LEGO_BLOCKS_H */
