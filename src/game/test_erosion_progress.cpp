// SPDX-License-Identifier: LGPL-3.0-or-later
#include "game/erode_link_names.h"
#include "game/erode.h"
#include "game/lego_globs.h"
#include "game/lego_level_record.h"
#include "test/lrr_test.h"

static unsigned effects[8];
extern "C" void __wrap_map_cell_tracking_flags_clear_and_refresh_current(const int*) {}
extern "C" void __wrap_lego_cell_init_pass(Lego_Level*, unsigned, unsigned, int) {}
struct TerrainMap;
extern "C" void __wrap_terrain_map_block_to_world(TerrainMap*, int x, int y, float* wx, float* wy)
{
    *wx = static_cast<float>(x);
    *wy = static_cast<float>(y);
}
extern "C" float __wrap_terrain_map_world_z_get(TerrainMap*, float, float) { return 0; }
extern "C" int __wrap_lava_effect_select_and_place(int slot, const float*, const float*)
{
    if (CHECK(slot >= 4 && slot <= 7, "erosion effect stage")) ++effects[slot];
    return 1;
}

int main()
{
    Lego_Block cells[25] = {};
    Lego_Level level = {};
    level.width = level.height = 5;
    level.blocks = cells;
    legoGlobs.currLevel = &level;
    erode_initialise(100, 1, 1);
    ErodePoint2I tile = {2, 2};
    auto& cell = cells[12];
    cell.flags1 = 8;
    cell.f_06 = 4;
    erode_set_block_erode_rate(&tile, 0);
    CHECK_EQ(erode_get_block_erode_rate(&tile), 250.0f);
    for (unsigned stage = 1; stage <= 4; ++stage) {
        erode_update(stage == 1 ? 1.0f : 62.5f);
        CHECK_EQ(cell.f_07, stage);
        CHECK_EQ(effects[8-stage], 1u);
        CHECK_EQ(cell.f_02, stage == 4 ? 6 : 0);
    }
    unsigned slot = 99;
    CHECK_EQ(erode_get_free_activity(&slot), 1);
    CHECK_EQ(slot, 0u);
    // Extinguishing the source locks it temporarily, then allows reuse.
    erode_set_active_block(&tile, 0);
    CHECK_EQ(erode_is_block_locked(&tile), 1);
    erode_update(26);
    CHECK_EQ(erode_is_block_locked(&tile), 0);
    legoGlobs.currLevel = nullptr;
    return lrr_test_result();
}
