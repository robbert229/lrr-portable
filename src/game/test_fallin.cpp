// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_fallin — eligibility depends on floor and terrain, reservation,
 * and whether safe caverns protect a tile marked as an outer edge. */
#include "game/fallin_link_names.h"
#include "game/fallin.h"
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
    FallinPoint2I center = { 1, 1 }, outside = { -1, 1 };
    Lego_Block& cell = blocks[4];

    CHECK_EQ(fallin_can_land_slide_at_block(&outside), 0);
    CHECK_EQ(fallin_can_land_slide_at_block(&center), 0);
    cell.flags1 = 0x8u; // Floor.
    CHECK_EQ(fallin_can_land_slide_at_block(&center), 1);
    for (const unsigned char excluded : { 6, 7, 9 }) {
        cell.f_02 = excluded;
        CHECK_EQ(fallin_can_land_slide_at_block(&center), 0);
    }
    cell.f_02 = 0;
    cell.flags1 |= 0x8000000u; // Reserved.
    CHECK_EQ(fallin_can_land_slide_at_block(&center), 0);
    cell.flags1 = 0x8u | 0x4000000u; // Floor on the outer edge.
    CHECK_EQ(fallin_can_land_slide_at_block(&center), 1);
    level.safeCaverns = 1;
    CHECK_EQ(fallin_can_land_slide_at_block(&center), 0);
    cell.flags1 = 0x8u;
    CHECK_EQ(fallin_can_land_slide_at_block(&center), 1);
    legoGlobs.currLevel = nullptr;
    return lrr_test_result("fallin");
}
