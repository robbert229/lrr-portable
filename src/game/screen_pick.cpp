#include "screen_pick.hpp"
#include "terrain_grid.hpp"
#include "level.hpp"
#include <math.h>
#include "screen_pick_shared.hpp"

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
