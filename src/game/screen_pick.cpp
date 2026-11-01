#include "screen_pick.hpp"
#include "terrain_grid.hpp"
#include "level.hpp"
#include <math.h>

/* The seven argument slots match FUN_004502e0. The grid and ray-hook pointees
 * are portable owners rather than the retail terrain/view records, so this is
 * an ABI-shaped implementation over semantic adapters, not a raw-record
 * replacement. */
// FUNCTION: LEGORR 0x004502e0
int screen_ray_terrain_cell_pick_target(
    TerrainSurfaceGrid* grid, const CameraPointerRayHooks* ray_hooks,
    float screen_x, float screen_y, int* out_cell_x, int* out_cell_y,
    float out_hit_xyz[3])
{
    float near_point[3];
    float far_point[3];
    float direction[3];
    float plane_point[3] = { 0.0f, 0.0f, -50.0f };
    const float plane_normal[3] = { 0.0f, 0.0f, -1.0f };
    float intersection[3];
    float terrain_height;
    int center_x, center_y;
    uint32_t iteration;
    uint32_t x, y;
    float best_distance = 0.0f;
    int found = 0;
    /* FUN_004502e0 iterates twenty times in FUN_00450560, starting from
     * z=-50, then checks the estimated cell and its two-cell border.
     * The portable view interface exposes the ray and plane intersection but
     * not the reference's world-to-screen vertex transform, so choose the
     * nearest exact terrain-ray hit within that same local neighborhood. */
    if (grid == NULL || ray_hooks == NULL ||
        ray_hooks->screen_point_to_ray == NULL || out_cell_x == NULL ||
        out_cell_y == NULL || out_hit_xyz == NULL ||
        grid->owner_level == NULL || grid->owner_level->ptr_terrain == NULL ||
        grid->width == 0u || grid->height == 0u ||
        !ray_hooks->screen_point_to_ray(ray_hooks->context, screen_x,
                                        screen_y, 0.0f, near_point) ||
        !ray_hooks->screen_point_to_ray(ray_hooks->context, screen_x,
                                        screen_y, 1.0f, far_point)) return 0;
    direction[0] = far_point[0] - near_point[0];
    direction[1] = far_point[1] - near_point[1];
    direction[2] = far_point[2] - near_point[2];
    {
        const float length = (float)sqrt(direction[0] * direction[0] +
            direction[1] * direction[1] + direction[2] * direction[2]);
        if (length == 0.0f) return 0;
        direction[0] /= length;
        direction[1] /= length;
        direction[2] /= length;
    }
    terrain_height = plane_point[2];
    for (iteration = 0u; iteration < 20u; ++iteration) {
        if (ray_hooks->plane_intersect != NULL) {
            plane_point[2] = terrain_height;
            if (!ray_hooks->plane_intersect(ray_hooks->context, near_point,
                    direction, plane_point, plane_normal, intersection))
                return 0;
        }
        else {
            float distance;
            if (direction[2] == 0.0f) return 0;
            distance = (terrain_height - near_point[2]) / direction[2];
            if (distance < 0.0f) return 0;
            intersection[0] = near_point[0] + direction[0] * distance;
            intersection[1] = near_point[1] + direction[1] * distance;
            intersection[2] = near_point[2] + direction[2] * distance;
        }
        terrain_height = terrain_surface_height_interpolate(
            grid->owner_level->ptr_terrain,
            intersection[0], intersection[1]);
    }
    if (!terrain_world_xy_to_cell(grid->owner_level->ptr_terrain,
            intersection[0], intersection[1], &center_x, &center_y)) return 0;
    for (y = 0u; y < 5u; ++y) {
        int candidate_y = center_y + (int)y - 2;
        if (candidate_y < 0 || (uint32_t)candidate_y >= grid->height) continue;
        for (x = 0u; x < 5u; ++x) {
            int candidate_x = center_x + (int)x - 2;
            float candidate[3];
            float dx, dy, dz, distance;
            if (candidate_x < 0 || (uint32_t)candidate_x >= grid->width)
                continue;
            if (!terrain_cell_ray_hit_test(grid, candidate_x, candidate_y,
                                           near_point, direction, candidate))
                continue;
            dx = candidate[0] - near_point[0];
            dy = candidate[1] - near_point[1];
            dz = candidate[2] - near_point[2];
            distance = dx * direction[0] + dy * direction[1] +
                       dz * direction[2];
            if (distance < 0.0f || (found && distance >= best_distance))
                continue;
            best_distance = distance;
            *out_cell_x = candidate_x;
            *out_cell_y = candidate_y;
            out_hit_xyz[0] = candidate[0];
            out_hit_xyz[1] = candidate[1];
            out_hit_xyz[2] = candidate[2];
            found = 1;
        }
    }
    return found;
}

int screen_ray_terrain_cell_pick(
    TerrainSurfaceGrid* grid, const CameraPointerRayHooks* ray_hooks,
    float screen_x, float screen_y, uint32_t depth_iterations,
    int* out_cell_x, int* out_cell_y, float out_hit_xyz[3])
{
    (void)depth_iterations;
    return screen_ray_terrain_cell_pick_target(
        grid, ray_hooks, screen_x, screen_y, out_cell_x, out_cell_y,
        out_hit_xyz);
}

namespace unported_ledger_stubs {

// ScreenRayTerrainCellPick
int ledger_stub_004502e0_ScreenRayTerrainCellPick(
    TerrainSurfaceGrid* grid, const CameraPointerRayHooks* ray_hooks,
    float screen_x, float screen_y, uint32_t depth_iterations,
    int* out_cell_x, int* out_cell_y, float out_hit_xyz[3])
{
    return screen_ray_terrain_cell_pick(grid, ray_hooks, screen_x, screen_y,
                                        depth_iterations, out_cell_x,
                                        out_cell_y, out_hit_xyz);
}

// TerrainCellRayHitTest
int ledger_stub_004515e0_TerrainCellRayHitTest(
    TerrainSurfaceGrid* grid, int cell_x, int cell_y, const float origin[3],
    const float direction[3], float hit[3])
{
    return terrain_cell_ray_hit_test(grid, cell_x, cell_y,
                                     origin, direction, hit);
}




} // namespace unported_ledger_stubs
