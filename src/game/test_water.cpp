// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_water — joining contiguous runs on adjacent rows, matching an
 * outlet to its pool, and resetting its flow on opening and sealing. */
#include "game/water_link_names.h"
#include "game/water.h"
#include "test/lrr_test.h"

extern "C" void __wrap_scene_render_lock_toggle(void*, int) {}

extern "C" int __wrap_terrain_map_block_vertices_get(TerrainMap*, unsigned int x,
                                                     unsigned int y, TerrainMapVector3* verts)
{
    CHECK_EQ(x, 2u);
    CHECK_EQ(y, 4u);
    verts[0] = { 0.0f, 0.0f, -8.0f };
    verts[1] = { 1.0f, 0.0f, -6.0f };
    verts[2] = { 1.0f, 1.0f, -2.0f };
    verts[3] = { 0.0f, 1.0f, -4.0f };
    return 0;
}

int main(void)
{
    water_create_pool(4, 1, 3);
    water_create_pool(4, 4, 5);
    WaterPool& left = waterGlobs.poolList[0];
    WaterPool& right = waterGlobs.poolList[1];
    CHECK_EQ(waterGlobs.poolCount, 2u);
    CHECK_EQ(left.pointCount, 2u);
    CHECK_EQ(right.pointCount, 1u);
    CHECK_EQ(water_compare_pools(&left, &right), -1);
    CHECK_EQ(water_compare_pools(&right, &left), 1);
    CHECK_EQ(water_find_joining_pool(5, 3, 4), nullptr);
    CHECK_EQ(water_find_joining_pool(5, 2, 5), &left);
    if (CHECK_EQ(waterGlobs.joinCount, 1u)) {
        CHECK_EQ(waterGlobs.joinList[0].first, &left);
        CHECK_EQ(waterGlobs.joinList[0].second, &right);
    }
    water_add_pool_points(&left, 5, 2, 5);
    CHECK_EQ(left.pointCount, 5u);
    CHECK_EQ(left.points[4].x, 4.0f);
    CHECK_EQ(left.points[4].y, 5.0f);

    left.entryCount = 2;
    left.entries[0] = { 0, 0, 0.0f, 0.0f, 0.0f, 0 }; // North of (1, 4)
    left.entries[1] = { 1, 1, 0.0f, 10.0f, 4.0f, 0 }; // East of (2, 4)
    unsigned int entry = 99;
    CHECK_EQ(water_find_pool(1, 3, &entry), &left);
    CHECK_EQ(entry, 0u);
    CHECK_EQ(water_find_pool(3, 4, &entry), &left);
    CHECK_EQ(entry, 1u);
    CHECK_EQ(water_find_pool(2, 4, &entry), nullptr);

    Lego_Level level = {};
    left.flags = WATER_FLAG_FILLED;
    water_destroy_wall_complete(&level, 3, 4);
    CHECK_EQ(left.flags, static_cast<unsigned int>(WATER_FLAG_VISIBLE));
    CHECK_EQ(left.entries[0].active, 0);
    CHECK_EQ(left.entries[1].active, 1);
    CHECK_EQ(left.entries[1].level, -2.0f);
    CHECK_EQ(left.entries[1].spread, 0.0f);
    CHECK_EQ(left.entries[1].recede, 0.0f);

    left.flags |= WATER_FLAG_DRAINED;
    water_seal_block(3, 4);
    CHECK_EQ(left.flags, static_cast<unsigned int>(WATER_FLAG_VISIBLE));
    CHECK_EQ(left.entries[1].active, 0);
    return lrr_test_result();
}
