// SPDX-License-Identifier: LGPL-3.0-or-later
/* Search and route ownership contracts on an invented rectangular grid.
 * Only the bookkeeping variant needs a generic in-memory scene frame. */
#include <array>
#include <cstdlib>
#include <miniwin/d3drm.h>
#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/main_globs.h"
#include "game/animation_object_record.h"
#include "game/entity_stats.h"
#include "game/legoobject_unit.h"
#include "game/lego_unit.h"
#include "game/terrain_map.h"
#include "game/lego_globs.h"
#include "test/lrr_test.h"

extern "C" void level_tile_mark_current_target(const int*, int);

namespace {
constexpr unsigned W = 8, H = 7;
constexpr unsigned Floor = 8, Wall = 0x10, Prefer = 0x20000000, Reserved = 0x8000000;
struct Grid {
    Lego_Level level = {};
    TerrainMap map = {};
    Lego_Block blocks[W * H] = {};
    Grid()
    {
        level.width = map.gridWidth = W;
        level.height = map.gridHeight = H;
        map.blockWidth = W + 1;
        map.blockHeight = H + 1;
        map.blockSize = 10;
        level.blocks = blocks;
        level.map = &map;
        legoGlobs.currLevel = &level;
        for (unsigned y = 1; y < H - 1; ++y)
            for (unsigned x = 1; x < W - 1; ++x)
                Open(x, y);
    }
    ~Grid() { legoGlobs.currLevel = nullptr; }
    Lego_Block& At(unsigned x, unsigned y) { return blocks[y * W + x]; }
    void Open(unsigned x, unsigned y) { At(x, y).flags1 = Floor; }
    void Close(unsigned x, unsigned y) { At(x, y).flags1 = Wall; }
};

struct Route {
    unsigned *xs = nullptr, *ys = nullptr, count = 0;
    ~Route() { free(xs); free(ys); }
    Route() = default;
    Route(const Route&) = delete;
    Route& operator=(const Route&) = delete;
    void Check(Grid& grid, unsigned sx, unsigned sy, unsigned ex, unsigned ey, unsigned length)
    {
        if (!CHECK(xs && ys, "successful search owns coordinate lists") ||
            !CHECK_EQ(count, length)) return;
        CHECK_EQ(xs[0], sx);
        CHECK_EQ(ys[0], sy);
        CHECK_EQ(xs[count - 1], ex);
        CHECK_EQ(ys[count - 1], ey);
        for (unsigned i = 0; i < count; ++i) {
            if (CHECK(xs[i] < W && ys[i] < H, "route within rectangular grid"))
                CHECK(grid.At(xs[i], ys[i]).flags1 & Floor, "route traverses admitted cell");
            if (i)
                CHECK_EQ(std::abs(int(xs[i]) - int(xs[i - 1])) +
                         std::abs(int(ys[i]) - int(ys[i - 1])), 1);
        }
    }
    void Rejected() { CHECK_EQ(xs, nullptr); CHECK_EQ(ys, nullptr); CHECK_EQ(count, 0u); }
};

struct Candidates {
    // A callback that accepts two distinct candidates in the same layer.
    unsigned visits[W * H] = {};
    unsigned accepted = 0;
    bool enabled = true;
};
int Accept(Lego_Object* object, int* cell, void* data)
{
    CHECK_EQ(object, nullptr);
    auto& c = *static_cast<Candidates*>(data);
    if (!CHECK(cell[0] >= 0 && cell[0] < int(W) && cell[1] >= 0 && cell[1] < int(H),
               "callback receives a bounded coordinate")) return 0;
    ++c.visits[cell[1] * W + cell[0]];
    bool match = c.enabled && ((cell[0] == 2 && cell[1] == 1) ||
                              (cell[0] == 3 && cell[1] == 2));
    if (match) ++c.accepted;
    return match;
}

void Search()
{
    Grid grid;
    // Both implementations scan the rest of the accepting layer. The later
    // equally near candidate is selected; neither visits the origin.
    for (bool weighted : {false, true}) {
        auto search = weighted ? legoobject_terrain_neighbor_path_check :
                                 legoobject_terrain_grid_path_search;
        Candidates c;
        Route route;
        CHECK_EQ(search(nullptr, 2, 2, W, H, &route.xs, &route.ys, &route.count, Accept, &c), 1);
        route.Check(grid, 2, 2, 3, 2, 2);
        CHECK_EQ(c.accepted, 2u);
        CHECK_EQ(c.visits[2 * W + 2], 0u);
        CHECK_EQ(c.visits[2 * W + 4], 0u);

        Candidates none;
        none.enabled = false;
        Route absent;
        CHECK_EQ(search(nullptr, 2, 2, W, H, &absent.xs, &absent.ys, &absent.count, Accept, &none), 0);
        absent.Rejected();
        CHECK_EQ(none.accepted, 0u);
        for (unsigned y = 1; y < H - 1; ++y)
            for (unsigned x = 1; x < W - 1; ++x)
                CHECK_EQ(none.visits[y * W + x], x == 2 && y == 2 ? 0u : 1u);

        Route bad;
        CHECK_EQ(search(nullptr, W, 2, 3, 2, &bad.xs, &bad.ys, &bad.count, nullptr, nullptr), 0);
        CHECK_EQ(search(nullptr, 2, 2, 3, H, &bad.xs, &bad.ys, &bad.count, nullptr, nullptr), 0);
        grid.Close(3, 2);
        CHECK_EQ(search(nullptr, 2, 2, 3, 2, &bad.xs, &bad.ys, &bad.count, nullptr, nullptr), 0);
        bad.Rejected();
        grid.Open(3, 2);
    }

    // An equal-length diagonal destination has multiple cardinal routes.
    // The depth search chooses the north predecessor first at the end.
    Route depth;
    CHECK_EQ(legoobject_terrain_grid_path_search(nullptr, 2, 2, 4, 4,
        &depth.xs, &depth.ys, &depth.count, nullptr, nullptr), 1);
    depth.Check(grid, 2, 2, 4, 4, 5);
    if (depth.count == 5) {
        CHECK_EQ(depth.xs[1], 3u);
        CHECK_EQ(depth.ys[1], 2u);
    }
    Route ordinary;
    CHECK_EQ(legoobject_grid_route_search_adapter(nullptr, 2, 2, 4, 4,
        &ordinary.xs, &ordinary.ys, &ordinary.count), 1);
    ordinary.Check(grid, 2, 2, 4, 4, 5);
    if (ordinary.count == 5) {
        CHECK_EQ(ordinary.xs[1], 3u);
        CHECK_EQ(ordinary.ys[1], 2u);
    }

    // A preferred (not blocking) cell changes the cost tie-break. The
    // caller's already returned route remains independently owned.
    grid.At(3, 4).flags1 |= Prefer;
    Route preferred;
    CHECK_EQ(legoobject_grid_route_search_adapter(nullptr, 2, 2, 4, 4,
        &preferred.xs, &preferred.ys, &preferred.count), 1);
    preferred.Check(grid, 2, 2, 4, 4, 5);
    if (preferred.count == 5) {
        CHECK_EQ(preferred.xs[3], 3u);
        CHECK_EQ(preferred.ys[3], 4u);
    }
    if (ordinary.count == 5) {
        CHECK_EQ(ordinary.xs[3], 4u);
        CHECK_EQ(ordinary.ys[3], 3u);
    }

    // Exhaust the depth search behind a barrier, reopen a single aperture,
    // and verify that a subsequent search does not reuse stale depth values.
    for (unsigned y = 1; y < H - 1; ++y) grid.Close(3, y);
    Route blocked;
    CHECK_EQ(legoobject_terrain_grid_path_search(nullptr, 2, 2, 4, 4,
        &blocked.xs, &blocked.ys, &blocked.count, nullptr, nullptr), 0);
    blocked.Rejected();
    grid.Open(3, 5);
    Route reopened;
    CHECK_EQ(legoobject_terrain_grid_path_search(nullptr, 2, 2, 4, 4,
        &reopened.xs, &reopened.ys, &reopened.count, nullptr, nullptr), 1);
    reopened.Check(grid, 2, 2, 4, 4, 7);
    if (reopened.count == 7) {
        CHECK_EQ(reopened.xs[4], 3u);
        CHECK_EQ(reopened.ys[4], 5u);
    }
}

void Bookkeeping()
{
    Grid grid;
    // Rotate the first-free-neighbor preference through all four candidates.
    int centre[] = {3, 3}, result[] = {-1, -1};
    unsigned directions = 0;
    for (unsigned i = 0; i < 4; ++i) {
        CHECK_EQ(lego_first_free_neighbor_cell_pick(nullptr, centre, result, 0), 1);
        const int dx = result[0] - centre[0], dy = result[1] - centre[1];
        if (CHECK(std::abs(dx) + std::abs(dy) == 1, "cardinal result")) {
            unsigned bit = dx > 0 ? 1 : dy > 0 ? 2 : dx < 0 ? 4 : 8;
            CHECK_EQ(directions & bit, 0u);
            directions |= bit;
        }
    }
    CHECK_EQ(directions, 15u);
    grid.Close(3, 2);
    grid.Close(4, 3);
    grid.At(3, 4).flags2 = 4;
    CHECK_EQ(lego_first_free_neighbor_cell_pick(nullptr, centre, result, 1), 1);
    CHECK_EQ(result[0], 2);
    CHECK_EQ(result[1], 3);
    grid.Close(2, 3);
    result[0] = result[1] = -1;
    CHECK_EQ(lego_first_free_neighbor_cell_pick(nullptr, centre, result, 1), 0);
    CHECK_EQ(result[0], -1);
    CHECK_EQ(result[1], -1);

    // Reopen the grid for a route with a turn and an explicit endpoint.
    grid.Open(3, 2);
    grid.Open(4, 3);
    grid.Open(2, 3);
    grid.At(3, 4).flags2 = 0;
    IDirect3DRM* rm = nullptr;
    IDirect3DRM3* rm3 = nullptr;
    if (!CHECK_EQ(Direct3DRMCreate(&rm), DD_OK)) return;
    CHECK_EQ(rm->QueryInterface(IID_IDirect3DRM3, reinterpret_cast<void**>(&rm3)), DD_OK);
    rm->Release();
    if (!rm3) return;
    s_main_d3drm = rm3;
    Container* root = Container_Initialise("SyntheticGrid");
    Container* frame = Container_Create(root);
    if (CHECK(frame != nullptr, "generic scene frame")) {
        Container_SetPosition(frame, root, 25, -25, 0);
        AnimationObject model = {};
        model.container = frame;
        StatsData stats = {};
        stats.flags1 = 0x2000000;
        Lego_Object object = {}, target = {};
        object.type = 2;
        object.stats = &stats;
        object.miniFigure = &model;
        target.type = 6;
        unsigned xs[] = {2, 3, 3}, ys[] = {2, 2, 3};
        float endpoint[] = {37.5f, -32.5f};
        CHECK_EQ(legoobject_route_setup_and_marker_associate(&object, 3, xs, ys, endpoint), 1);
        auto* route = static_cast<LegoObjG_RouteBlock*>(object.routeBlocks);
        if (CHECK(route && object.routeBlocksTotal == 3, "route installed")) {
            CHECK_EQ(route[1].flags & 0x10, 0x10);
            CHECK_NEAR(route[2].blockOffset[0], 0.75f, 1e-5);
            CHECK_NEAR(route[2].blockOffset[1], 0.75f, 1e-5);
            // Invalid replacement must retain the original route.
            unsigned invalid[] = {2, 5, 3};
            CHECK_EQ(legoobject_route_setup_and_marker_associate(&object, 3, invalid, ys, nullptr), 0);
            CHECK_EQ(object.routeBlocks, route);
            CHECK_EQ(object.routeBlocksTotal, 3u);
            // Interrupting a route releases its resource interaction and
            // reservation, but does not alter an unrelated cell reservation.
            route[2].actionFlags = 3;
            object.routeToObject = &target;
            target.interactObject = &object;
            object.flags1 |= 8;
            object.targetBlockPos[0] = 3;
            object.targetBlockPos[1] = 3;
            int other[] = {4, 4};
            level_tile_mark_current_target(centre, 1);
            level_tile_mark_current_target(other, 1);
            legoobject_action_sequence_reset(&object, 0);
            CHECK_EQ(object.routeBlocks, nullptr);
            CHECK_EQ(object.routeBlocksTotal, 0u);
            CHECK_EQ(object.routeToObject, nullptr);
            CHECK_EQ(target.interactObject, nullptr);
            CHECK_EQ(grid.At(3, 3).flags1 & Reserved, 0u);
            CHECK_EQ(grid.At(4, 4).flags1 & Reserved, Reserved);
        }
        Container_Remove(frame);
    }
    Container_Shutdown();
    s_main_d3drm = nullptr;
    rm3->Release();
}
}

int main()
{
    Search();
    Bookkeeping();
    return lrr_test_result("grid_search_contracts");
}
