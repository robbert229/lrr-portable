// SPDX-License-Identifier: LGPL-3.0-or-later
#include "game/fallin_link_names.h"
#include "game/fallin.h"
#include "game/lego_globs.h"
#include "game/lego_level_record.h"
#include "game/terrain_map.h"
#include "test/lrr_test.h"

static unsigned effects, cleared, sounds;
extern "C" int __wrap_terrain_cell_state_update_current(const int*) { return 1; }
extern "C" void __wrap_terrain_cell_work_flag_clear_current(const int*, uint32_t) { ++cleared; }
extern "C" void __wrap_lego_cell_init_pass(Lego_Level*, unsigned, unsigned, int) {}
extern "C" float __wrap_terrain_map_world_z_get(TerrainMap*, float, float) { return 0; }
extern "C" int __wrap_scene_slot_pose_scale_register(int group, uint32_t x, uint32_t y,
    float, float, float, float dx, float dy)
{
    CHECK_EQ(group, 0);
    CHECK_EQ(x, 2u);
    CHECK_EQ(y, 2u);
    CHECK_EQ(dx, 0.0f);
    CHECK_EQ(dy, 1.0f);
    ++effects;
    return 1;
}
extern "C" int __wrap_sound_submission_route_wrapper(const void*, int alias, int, int, const float*)
{
    CHECK_EQ(alias, 0x28);
    ++sounds;
    return 1;
}

int main()
{
    Lego_Block cells[25] = {};
    TerrainMap map = {};
    map.gridWidth = map.gridHeight = 4;
    map.blockSize = 1;
    Lego_Level level = {};
    level.width = level.height = 5;
    level.blocks = cells;
    level.map = &map;
    legoGlobs.currLevel = &level;
    fallin_initialise(3);
    FallinPoint2I pos = {2, 2};
    cells[12].flags1 = 8;
    CHECK_EQ(fallin_try_generate_land_slide(&pos, 1), 0);
    cells[7].flags1 = 0x10;
    cells[7].f_02 = 1;
    CHECK_EQ(fallin_try_generate_land_slide(&pos, 1), 0);
    cells[7].f_02 = 2;
    CHECK_EQ(fallin_try_generate_land_slide(&pos, 1), 1);
    CHECK_EQ(effects, 1u);
    CHECK_EQ(cleared, 1u);
    CHECK_EQ(sounds, 1u);
    CHECK_EQ(cells[12].f_24, 1u);
    CHECK_EQ(cells[12].flags1 & 0x8001800, 0x8001800u);
    CHECK_EQ(fallin_try_generate_land_slide(&pos, 1), 0);
    CHECK_EQ(effects, 1u);
    legoGlobs.currLevel = nullptr;
    return lrr_test_result();
}
