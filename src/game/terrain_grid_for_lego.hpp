// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what lego uses from terrain_grid.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_TERRAIN_GRID_FOR_LEGO_HPP
#define LRR_GAME_TERRAIN_GRID_FOR_LEGO_HPP

int terrain_texture_set_config_load_grid(TerrainTileMeshGrid* grid, const char* group_name);

#endif /* LRR_GAME_TERRAIN_GRID_FOR_LEGO_HPP */
