// SPDX-License-Identifier: LGPL-3.0-or-later
/* Generated grid, with blocked perimeter and an interior wall with a gap. */
#include <cstdlib>
#include "game/terrain_map.h"
#include "game/lego_unit.h"
#include "game/legoobject_unit.h"
#include "game/lego_globs.h"
#include "test/lrr_test.h"

namespace {
constexpr unsigned int Side = 7;
Lego_Block blocks[Side * Side] = {};
Lego_Level level = {};
TerrainMap map = {};

Lego_Block& Cell(unsigned int x, unsigned int y)
{
    return blocks[y * Side + x];
}

void CheckOccupancy()
{
    Lego_Block& destination = Cell(2, 2);
    struct Case { unsigned int flags1, flags2, terrain; int expected; };
    const Case cases[] = {
        {8, 0, 0, 1},      // Floor.
        {0x10, 0, 0, 0},   // Wall.
        {8 | 0x400, 0, 0, 0}, // Blocked.
        {8, 4, 0, 0},      // Rubble obstruction.
        {8, 0, 9, 0},      // Water.
        {8, 0, 6, 0},      // Lava.
        {8 | 0x200, 0, 6, 1}, // Protected lava.
    };
    for (const Case& entry : cases) {
        destination.flags1 = entry.flags1;
        destination.flags2 = entry.flags2;
        destination.f_02 = entry.terrain;
        CHECK_EQ(lego_terrain_cell_route_and_occupancy_check(
            nullptr, 2, 2, 2, 3, 0), entry.expected);
    }
    destination = {};
    destination.flags1 = 8;
}

void CheckRoute(unsigned int expectedCount)
{
    unsigned int *xs = nullptr, *ys = nullptr, count = 0;
    const int result = legoobject_grid_route_search_adapter(
        nullptr, 1, 3, 5, 3, &xs, &ys, &count);
    CHECK_EQ(result, expectedCount != 0);
    if (expectedCount && CHECK(xs && ys && count, "route allocated")) {
        CHECK_EQ(count, expectedCount);
        CHECK_EQ(xs[0], 1u);
        CHECK_EQ(ys[0], 3u);
        CHECK_EQ(xs[count - 1], 5u);
        CHECK_EQ(ys[count - 1], 3u);
        for (unsigned int i = 0; i < count; ++i) {
            if (CHECK(xs[i] < Side && ys[i] < Side, "route stays in bounds"))
                CHECK_EQ(Cell(xs[i], ys[i]).flags1 & 8u, 8u);
            if (i)
                CHECK_EQ(std::abs(int(xs[i]) - int(xs[i - 1])) +
                         std::abs(int(ys[i]) - int(ys[i - 1])), 1);
        }
    } else if (!expectedCount) {
        CHECK_EQ(xs, nullptr);
        CHECK_EQ(ys, nullptr);
        CHECK_EQ(count, 0u);
    }
    free(xs);
    free(ys);
}
} // namespace

int main()
{
    level.width = level.height = Side;
    level.blocks = blocks;
    level.map = &map;
    map.gridWidth = map.gridHeight = Side;
    legoGlobs.currLevel = &level;
    for (unsigned int y = 1; y < Side - 1; ++y)
        for (unsigned int x = 1; x < Side - 1; ++x)
            Cell(x, y).flags1 = 8; // Floor.

    CheckOccupancy();
    CheckRoute(5);
    for (unsigned int y = 1; y < Side - 1; ++y)
        Cell(3, y).flags1 = 0x10; // Wall.
    Cell(3, 1).flags1 = 8;
    CheckRoute(9);
    Cell(3, 1).flags2 = 4; // Obstruct the only gap.
    CheckRoute(0);
    Cell(3, 1).flags2 = 0;
    Cell(3, 1).flags1 = 0x10;
    Cell(3, 5).flags1 = 8; // Reopen on the opposite side.
    CheckRoute(9);

    unsigned int *xs = nullptr, *ys = nullptr, count = 0;
    CHECK_EQ(legoobject_grid_route_search_adapter(nullptr, Side, 3, 5, 3,
        &xs, &ys, &count), 0);
    CHECK_EQ(legoobject_grid_route_search_adapter(nullptr, 1, 3, 5, Side,
        &xs, &ys, &count), 0);
    CHECK_EQ(xs, nullptr);
    CHECK_EQ(ys, nullptr);
    CHECK_EQ(count, 0u);
    legoGlobs.currLevel = nullptr;
    return lrr_test_result("navigation_synthetic");
}
