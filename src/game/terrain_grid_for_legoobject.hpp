// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; tools/relocate_unit.py, #496): what legoobject uses from terrain_grid.cpp.
 * Written once by that relocation and never edited again, so later
 * relocations do not change what earlier unit files see. Included by both files. */
#ifndef LRR_GAME_TERRAIN_GRID_FOR_LEGOOBJECT_HPP
#define LRR_GAME_TERRAIN_GRID_FOR_LEGOOBJECT_HPP

extern TerrainVariantSceneMarkerHook s_terrain_variant_marker_hook;
extern TerrainVariantEntityVisitHook s_terrain_variant_visit_hook;
extern void* s_terrain_variant_scene_context;
extern TerrainPathGridSizeHook s_terrain_path_grid_size_hook;
extern TerrainPathRouteHook s_terrain_path_route_hook;
extern TerrainPathHalfCostHook s_terrain_path_half_cost_hook;
extern void* s_terrain_path_context;
extern uint32_t s_terrain_neighbor_path_cost_count;

#endif /* LRR_GAME_TERRAIN_GRID_FOR_LEGOOBJECT_HPP */
