// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GAME_SCREEN_PICK_HPP
#define LEGORRR_GAME_SCREEN_PICK_HPP

#include "terrain_grid.hpp"
#include "../gods98/view.hpp"
#include <math.h>

/* ScreenRayTerrainCellPick (FUN_004502e0): turn a screen coordinate into a
 * world ray, test the nearby terrain surface cells, and return the nearest
 * hit. The original narrows the scan to a 5x5 neighborhood around its
 * iterative ray-to-grid estimate; the portable owner scans the supplied
 * grid so callers can use the same operation with sparse or synthetic maps. */
static inline int screen_ray_terrain_cell_pick(
    TerrainSurfaceGrid* grid, const CameraPointerRayHooks* ray_hooks,
    float screen_x, float screen_y, uint32_t depth_iterations,
    int* out_cell_x, int* out_cell_y, float out_hit_xyz[3])
{
    float near_point[3];
    float far_point[3];
    float direction[3];
    float candidate[3];
    float best_distance = 0.0f;
    uint32_t x, y;
    int found = 0;
    (void)depth_iterations;
    if (grid == NULL || ray_hooks == NULL ||
        ray_hooks->screen_point_to_ray == NULL || out_cell_x == NULL ||
        out_cell_y == NULL || out_hit_xyz == NULL ||
        grid->surface_vectors == NULL || grid->width == 0 ||
        grid->height == 0 ||
        !ray_hooks->screen_point_to_ray(ray_hooks->context, screen_x,
                                        screen_y, 0.0f, near_point) ||
        !ray_hooks->screen_point_to_ray(ray_hooks->context, screen_x,
                                        screen_y, 1.0f, far_point)) {
        return 0;
    }
    direction[0] = far_point[0] - near_point[0];
    direction[1] = far_point[1] - near_point[1];
    direction[2] = far_point[2] - near_point[2];
    {
        float length = (float)sqrt(direction[0] * direction[0] +
                                   direction[1] * direction[1] +
                                   direction[2] * direction[2]);
        if (length == 0.0f) return 0;
        direction[0] /= length;
        direction[1] /= length;
        direction[2] /= length;
    }
    for (y = 0; y < grid->height; ++y) {
        for (x = 0; x < grid->width; ++x) {
            float dx, dy, dz, distance;
            if (!terrain_cell_ray_hit_test(grid, x, y, near_point,
                                           direction, candidate)) continue;
            dx = candidate[0] - near_point[0];
            dy = candidate[1] - near_point[1];
            dz = candidate[2] - near_point[2];
            distance = dx * direction[0] + dy * direction[1] +
                       dz * direction[2];
            if (distance < 0.0f) continue;
            if (!found || distance < best_distance) {
                best_distance = distance;
                *out_cell_x = (int)x;
                *out_cell_y = (int)y;
                out_hit_xyz[0] = candidate[0];
                out_hit_xyz[1] = candidate[1];
                out_hit_xyz[2] = candidate[2];
                found = 1;
            }
        }
    }
    return found;
}

#endif
