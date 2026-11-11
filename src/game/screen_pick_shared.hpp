// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/game/screen_pick.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GAME_SCREEN_PICK_SHARED_HPP
#define LRR_GAME_SCREEN_PICK_SHARED_HPP

int screen_ray_terrain_cell_pick_target( TerrainSurfaceGrid* grid, const CameraPointerRayHooks* ray_hooks, float screen_x, float screen_y, int* out_cell_x, int* out_cell_y, float out_hit_xyz[3]);

#endif /* LRR_GAME_SCREEN_PICK_SHARED_HPP */
