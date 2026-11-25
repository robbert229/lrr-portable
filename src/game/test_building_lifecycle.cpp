// SPDX-License-Identifier: LGPL-3.0-or-later
#include "game/construction_link_names.h"
#include "game/construction.h"
#include "game/legoobject_unit.h"
#include "game/lego_globs.h"
#include "game/lego_level_record.h"
#include "game/building_model_record.h"
#include "game/terrain_map.h"
#include "test/lrr_test.h"

static Lego_Object ore, blocker, building;
static bool occupied = true;
static unsigned requests, refunds, created, teleported;
extern "C" void __wrap_terrain_map_shape_flatten(TerrainMap*, const TerrainMapPoint2I*, unsigned, float) {}
extern "C" void __wrap_terrain_map_block_texture_set(TerrainMap*, unsigned, unsigned, unsigned char, unsigned char) {}
extern "C" void __wrap_terrain_cell_mutation_progress(Lego_Level*, uint32_t, uint32_t) {}
extern "C" void __wrap_cancel_action_for_matching_coordinates(const uint32_t*) {}
extern "C" void __wrap_aitask_cmd_command_kind_record_issue(int, int, int, const int*, int, const float*, int) { ++requests; }
extern "C" void __wrap_aitask_cmd_create_task(Lego_Object* obj, float)
{
    CHECK_EQ(obj, &ore);
    ++refunds;
}
extern "C" void __wrap_aitask_cmd_cancel_by_coords(int, int, int, int) {}
extern "C" int __wrap_legoobject_run_through_lists_skip_upgrade_parts(int (*cb)(Lego_Object*, void*), void* data)
{
    if (cb(&ore, data)) return 1;
    return occupied && cb(&blocker, data);
}
extern "C" int __wrap_legoobject_position_to_terrain_cell(Lego_Object*, int* x, int* y)
{
    *x = *y = 2;
    return 1;
}
extern "C" Lego_Object* __wrap_legoobject_create_in_world(void*, int type, int id, int, float x, float y, float heading)
{
    CHECK_EQ(type, 4);
    CHECK_EQ(id, 0);
    CHECK_NEAR(x, 2.5f, 0.001f);
    CHECK_NEAR(y, -2.5f, 0.001f);
    CHECK_NEAR(heading, 1.5707964f, 0.001f);
    ++created;
    building.type = type;
    return &building;
}
extern "C" void __wrap_legoobject_dispatch_event5_and_set_flag(Lego_Object* obj)
{
    CHECK_EQ(obj, &building);
    ++teleported;
}
struct InfoPoint2I;
extern "C" void __wrap_info_send(int, char*, void*, InfoPoint2I*) {}
extern "C" unsigned __wrap_entity_stats_word_get(const Lego_Object*) { return 0; }
extern "C" unsigned __wrap_entity_state_flags_get(const Lego_Object*) { return 0x200; }
extern "C" void __wrap_aitask_terrain_cell_refresh_dispatch_current(const int*, int) {}
extern "C" void __wrap_electric_fence_block_surfaces_update(int, int) {}

int main()
{
    Lego_Block cells[36] = {};
    Lego_Level level = {};
    TerrainMap map = {};
    BuildingModel model = {};
    map.blockSize = 1;
    map.gridWidth = map.gridHeight = 5;
    level.width = level.height = 6;
    level.blocks = cells;
    level.map = &map;
    legoGlobs.currLevel = &level;
    legoGlobs.buildingData = &model;
    ore.type = 7;
    blocker.type = 2;
    int origin[] = {2, 2};
    int shape[] = {2, 2, 3, 2};
    auto handle = construction_zone_start_building(0, origin, 1, shape, 2);
    auto* zone = construction_zone_find_by_handle_or_at_block(origin, nullptr);
    CHECK_EQ(cells[14].construct, zone);
    CHECK_EQ(cells[15].construct, zone);
    construction_zone_request_resource(origin, 7, 0, 0, 2);
    construction_zone_place_resource(handle, &ore);
    construction_zone_cancel_building(origin);
    CHECK_EQ(refunds, 1u);
    CHECK_EQ(cells[14].construct, nullptr);
    CHECK_EQ(cells[15].construct, nullptr);
    CHECK_EQ(cells[14].flags1 & 0x100000, 0u);

    handle = construction_zone_start_building(0, origin, 1, shape, 2);
    construction_zone_request_resource(origin, 7, 0, 0, 1);
    construction_zone_place_resource(handle, &ore);
    construction_update_all(1);
    CHECK_EQ(created, 0u);
    CHECK_EQ(construction_zone_exists_at_block(origin), 1);
    occupied = false;
    construction_update_all(1);
    CHECK_EQ(created, 1u);
    CHECK_EQ(teleported, 1u);
    CHECK_EQ(construction_zone_exists_at_block(origin), 0);
    CHECK_EQ(cells[14].flags1 & 0x100400, 0x100400u);
    CHECK_EQ(cells[15].flags1 & 0x100400, 0x100400u);
    CHECK_EQ(cells[14].flags2 & 1, 1u);
    CHECK_EQ(ore.health, -1.0f);
    CHECK_EQ(requests, 3u);
    legoGlobs.currLevel = nullptr;
    legoGlobs.buildingData = nullptr;
    return lrr_test_result();
}
