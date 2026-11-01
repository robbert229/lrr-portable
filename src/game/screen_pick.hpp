// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GAME_SCREEN_PICK_HPP
#define LEGORRR_GAME_SCREEN_PICK_HPP

#include "terrain_grid.hpp"
#include "../gods98/view.hpp"

/* ScreenRayTerrainCellPick (FUN_004502e0): refine the camera ray against the
 * terrain heightfield, then test the estimated cell's 5x5 neighborhood. */
int screen_ray_terrain_cell_pick(
    TerrainSurfaceGrid* grid, const CameraPointerRayHooks* ray_hooks,
    float screen_x, float screen_y, uint32_t depth_iterations,
    int* out_cell_x, int* out_cell_y, float out_hit_xyz[3]);

#endif
