// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what level.cpp uses from legoobject.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by level.cpp only. */
#ifndef LRR_GAME_LEVEL_FROM_LEGOOBJECT_HPP
#define LRR_GAME_LEVEL_FROM_LEGOOBJECT_HPP

#include "level_for_legoobject.hpp"

extern uint32_t s_grid_count_first[GRID_COUNT_TABLE_DWORDS];
extern uint32_t s_grid_count_second[GRID_COUNT_TABLE_DWORDS];
extern int s_terrain_sfx_grid[TERRAIN_SFX_GRID_CELLS];

#endif /* LRR_GAME_LEVEL_FROM_LEGOOBJECT_HPP */
