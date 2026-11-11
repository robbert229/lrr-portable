// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what terrain_map.cpp sees from
 * screen_pick.cpp. A verbatim snapshot of screen_pick_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GAME_SCREEN_PICK_FOR_TERRAIN_MAP_HPP
#define LRR_GAME_SCREEN_PICK_FOR_TERRAIN_MAP_HPP

int screen_ray_terrain_cell_pick_target( TerrainSurfaceGrid* grid, const CameraPointerRayHooks* ray_hooks, float screen_x, float screen_y, int* out_cell_x, int* out_cell_y, float out_hit_xyz[3]);

#endif /* LRR_GAME_SCREEN_PICK_FOR_TERRAIN_MAP_HPP */
