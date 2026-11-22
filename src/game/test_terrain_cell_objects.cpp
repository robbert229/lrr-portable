// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_terrain_cell_objects — the fence grid and node pool, with a stud
 * allowed between opposing fences and blocked after either is removed. */
#include "game/terrain_cell_objects.h"
#include "test/lrr_test.h"

extern "C" void __wrap_lego_cell_init_pass(Lego_Level*, unsigned int, unsigned int, int)
{
}

extern "C" uint32_t __wrap_terrain_cell_occupied_or_flag_current(int, int, int)
{
    return 0;
}

static void count_fence(ElectricFenceNode*, void* data)
{
    ++*static_cast<unsigned int*>(data);
}

int main(void)
{
    Lego_Block blocks[25] = {};
    TerrainMap map = {};
    map.gridWidth = 5;
    map.gridHeight = 5;
    Lego_Level level = {};
    level.width = 5;
    level.height = 5;
    level.map = &map;
    level.blocks = blocks;
    electric_fence_initialise(&level);

    ElectricFenceObject west = {}, east = {}, center = {};
    CHECK_EQ(electric_fence_block_is_fence(1, 2), 0);
    ElectricFenceNode* westNode = electric_fence_create(&west, 1, 2);
    CHECK_EQ(electric_fence_block_is_fence(1, 2), 1);
    CHECK_EQ(westNode->object, &west);
    CHECK_EQ(electric_fence_stud_fits(&level, 2, 2), 0);
    ElectricFenceNode* eastNode = electric_fence_create(&east, 3, 2);
    CHECK_EQ(electric_fence_stud_fits(&level, 2, 2), 1);

    unsigned int count = 0;
    electric_fence_run_through_lists(count_fence, &count);
    CHECK_EQ(count, 2u);
    electric_fence_remove_node(eastNode, 3, 2);
    CHECK_EQ(electric_fence_block_is_fence(3, 2), 0);
    CHECK_EQ(electric_fence_stud_fits(&level, 2, 2), 0);
    count = 0;
    electric_fence_run_through_lists(count_fence, &count);
    CHECK_EQ(count, 1u);

    ElectricFenceNode* centerNode = electric_fence_create(&center, 2, 2);
    CHECK_EQ(centerNode, eastNode);
    CHECK_EQ(centerNode->blockX, 2);
    CHECK_EQ(centerNode->blockY, 2);
    CHECK_EQ(electric_fence_stud_fits(&level, 2, 2), 0);
    electric_fence_shutdown();
    return lrr_test_result("terrain_cell_objects");
}
