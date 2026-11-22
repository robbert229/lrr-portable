// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_radarmap — half-open screen bounds, projection of world rectangles
 * and conversion from a radar click to world coordinates and a map block. */
#include "game/radarmap_link_names.h"
#include "game/radarmap.h"
#include "game/terrain_map.h"
#include "test/lrr_test.h"

int main(void)
{
    TerrainMap map = {};
    map.gridWidth = 3;
    map.gridHeight = 3;
    map.blockSize = 8.0f;
    map.worldX = -8.0f;
    map.worldY = 16.0f;
    const Area2F bounds = { 10.0f, 20.0f, 32.0f, 32.0f };
    RadarMap* radar = radarmap_create(&map, &bounds, 8.0f);
    if (!CHECK(radar != nullptr, "the radar map is allocated")) return lrr_test_result();
    radar->centrePos = { 4.0f, 4.0f };

    CHECK_EQ(radarmap_inside_radar(radar, 10, 20), 1);
    CHECK_EQ(radarmap_inside_radar(radar, 41, 51), 1);
    CHECK_EQ(radarmap_inside_radar(radar, 42, 51), 0);
    CHECK_EQ(radarmap_inside_radar(radar, 41, 52), 0);

    Point2F world = {};
    int bx = -1, by = -1;
    CHECK_EQ(radarmap_screen_to_block_pos(radar, 30, 32, &world, &bx, &by), 1);
    CHECK_EQ(world.x, 8.0f);
    CHECK_EQ(world.y, 8.0f);
    CHECK_EQ(bx, 2);
    CHECK_EQ(by, 1);
    CHECK_EQ(radarmap_screen_to_block_pos(radar, 42, 32, &world, &bx, &by), 0);
    CHECK_EQ(radarmap_screen_to_block_pos(radar, 40, 36, &world, &bx, &by), 0);

    radar->zoom = 4.0f;
    Area2F rect = { 12.0f, -4.0f, 8.0f, 4.0f };
    radarmap_transform_rect(radar, &rect);
    CHECK_EQ(rect.x, 30.0f);
    CHECK_EQ(rect.y, 40.0f);
    CHECK_EQ(rect.width, 4.0f);
    CHECK_EQ(rect.height, 2.0f);
    radarmap_free(radar);
    return lrr_test_result();
}
