// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_terrain_map — direction codes, world/grid conversion and height
 * interpolation on a rectangular map, and symmetric texture fade links. */
#include "game/terrain_map.h"
#include "test/lrr_test.h"

int main(void)
{
    const int neighbours[][2] = {
        { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 },
        { 1, -1 }, { 1, 1 }, { -1, 1 }, { -1, -1 },
    };
    for (int i = 0; i < 8; ++i) {
        CHECK_EQ(cell_neighbor_direction_code(3, 5, 3 + neighbours[i][0],
                                              5 + neighbours[i][1]), i);
    }
    CHECK_EQ(cell_neighbor_direction_code(3, 5, 3, 5), -1);
    CHECK_EQ(cell_neighbor_direction_code(3, 5, 5, 5), -1);

    TerrainMapBlock blocks[12] = {};
    TerrainMap map = {};
    map.gridWidth = 3;
    map.gridHeight = 2;
    map.blockWidth = 4;
    map.blockHeight = 3;
    map.blockSize = 8.0f;
    map.roughLevel = 2.0f;
    map.worldX = -12.0f;
    map.worldY = 16.0f;
    map.blocks = blocks;

    float x = 0.0f;
    float y = 0.0f;
    CHECK_EQ(terrain_map_block_to_world(&map, 2, 1, &x, &y), 1);
    CHECK_EQ(x, 8.0f);
    CHECK_EQ(y, 4.0f);
    CHECK_EQ(terrain_map_block_to_world(&map, 1, 2, &x, &y), 0);

    int bx = -1;
    int by = -1;
    float centre = -1.0f;
    CHECK_EQ(terrain_map_world_to_block_centre(&map, x, y, &bx, &by, &centre), 1);
    CHECK_EQ(bx, 2);
    CHECK_EQ(by, 1);
    CHECK_EQ(centre, 1.0f);
    CHECK_EQ(terrain_map_world_to_block_centre(&map, x + 2.0f, y, &bx, &by, &centre), 1);
    CHECK_EQ(centre, 0.5f);
    CHECK_EQ(terrain_map_world_to_block(&map, 12.0f, y, &bx, &by), 0);
    terrain_map_world_to_block_clamped(&map, -28.0f, -8.0f, &bx, &by);
    CHECK_EQ(bx, 0);
    CHECK_EQ(by, 1);

    /* The first quad slopes east, independent of its north/south position. */
    blocks[0].height = blocks[4].height = 2;
    blocks[1].height = blocks[5].height = 6;
    CHECK_EQ(terrain_map_world_z_unmodified_get(&map, -10.0f, 14.0f), -6.0f);
    CHECK_EQ(terrain_map_world_z_unmodified_get(&map, -6.0f, 10.0f), -10.0f);

    blocks[0].texture = 2;
    blocks[1].texture = 7;
    CHECK_EQ(terrain_map_block_pair_fades(&map, 0, 0, 1, 0), 0);
    terrain_map_fade_pair_add(&map, 2, 7);
    terrain_map_fade_pair_add(&map, 7, 2);
    CHECK_EQ(map.fadePairCount[2], 1u);
    CHECK_EQ(map.fadePairCount[7], 1u);
    CHECK_EQ(terrain_map_block_pair_fades(&map, 0, 0, 1, 0), 1);
    CHECK_EQ(terrain_map_block_pair_fades(&map, 1, 0, 0, 0), 1);
    CHECK_EQ(terrain_map_block_pair_fades(&map, 0, 0, 0, 0), 0);
    CHECK_EQ(terrain_map_block_pair_fades(&map, 0, 0, 3, 0), 0);
    return lrr_test_result();
}
