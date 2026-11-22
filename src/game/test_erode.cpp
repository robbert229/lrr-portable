// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_erode — erosion time from a cell's resistance, cardinal selection
 * around a lava source, and suppression until a neighbor's lock expires. */
#include "game/erode_link_names.h"
#include "game/erode.h"
#include "game/lego_globs.h"
#include "game/lego_level_record.h"
#include "test/lrr_test.h"

int main(void)
{
    Lego_Block blocks[9] = {};
    Lego_Level level = {};
    level.width = 3;
    level.height = 3;
    level.blocks = blocks;
    legoGlobs.currLevel = &level;
    erode_initialise(10.0f, 1.0f, 1.0f);
    unsigned int slot = 99;
    CHECK_EQ(erode_get_free_activity(&slot), 1);
    CHECK_EQ(slot, 0u);

    ErodePoint2I center = { 1, 1 }, north = { 1, 0 }, adjacent = {};
    blocks[1].flags1 = 0x8u; // Floor.
    blocks[1].f_06 = 4;
    blocks[5].flags1 = 0x8u;
    blocks[5].f_06 = 2;
    CHECK_EQ(erode_get_block_erode_rate(&north), 250.0f);
    CHECK_EQ(erode_find_adjacent_block(&center, &adjacent), 0);
    blocks[4].f_02 = 6; // Lava.
    CHECK_EQ(erode_find_adjacent_block(&center, &adjacent), 1);
    CHECK_EQ(adjacent.x, 1);
    CHECK_EQ(adjacent.y, 0);

    erode_add_locked_block(&north);
    CHECK_EQ(erode_is_block_locked(&north), 1);
    CHECK_EQ(erode_find_adjacent_block(&center, &adjacent), 1);
    CHECK_EQ(adjacent.x, 2);
    CHECK_EQ(adjacent.y, 1);
    erode_update(25.0f);
    CHECK_EQ(erode_is_block_locked(&north), 1);
    erode_update(1.0f);
    CHECK_EQ(erode_is_block_locked(&north), 0);
    CHECK_EQ(erode_find_adjacent_block(&center, &adjacent), 1);
    CHECK_EQ(adjacent.x, 1);
    CHECK_EQ(adjacent.y, 0);
    legoGlobs.currLevel = nullptr;
    return lrr_test_result("erode");
}
