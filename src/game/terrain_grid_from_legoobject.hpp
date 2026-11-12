// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what terrain_grid.cpp uses from legoobject.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by terrain_grid.cpp only. */
#ifndef LRR_GAME_TERRAIN_GRID_FROM_LEGOOBJECT_HPP
#define LRR_GAME_TERRAIN_GRID_FROM_LEGOOBJECT_HPP

extern float* s_terrain_neighbor_path_cost;

#endif /* LRR_GAME_TERRAIN_GRID_FROM_LEGOOBJECT_HPP */
