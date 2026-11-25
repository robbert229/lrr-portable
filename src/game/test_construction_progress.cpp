// SPDX-License-Identifier: LGPL-3.0-or-later
#include "game/construction_link_names.h"
#include "game/construction.h"
#include "game/lego_unit.h"
#include "game/lego_globs.h"
#include "game/lego_level_record.h"
#include "game/legoobject_record.h"
#include "test/lrr_test.h"

static unsigned requests, refunds, cancellations, ready;
static Lego_Object* refunded;
struct TerrainMap;
extern "C" void __wrap_terrain_map_block_texture_set(TerrainMap*, unsigned, unsigned, unsigned char, unsigned char) {}
extern "C" void __wrap_cancel_action_for_matching_coordinates(const uint32_t*) {}
extern "C" void __wrap_aitask_cmd_command_kind_record_issue(int type, int id, int level,
    const int* pos, int, const float*, int)
{
    CHECK_EQ(type, 7);
    CHECK_EQ(id, 0);
    CHECK_EQ(level, 0);
    CHECK_EQ(pos[0], 2);
    CHECK_EQ(pos[1], 2);
    ++requests;
}
extern "C" uint32_t __wrap_entity_indexed_record_word_at_b0_get(int, int, int) { return 2; }
extern "C" uint32_t __wrap_entity_indexed_record_word_at_b4_get(int, int, int) { return 0; }
extern "C" uint32_t __wrap_stats_table_word_at_b8(int, int, int) { return 0; }
extern "C" Lego_Object* __wrap_legoobject_query_center_mask02(Lego_Object*, int*, int, int) { return nullptr; }
extern "C" void __wrap_aitask_bobbin_clear_delay(const int*) { ++ready; }
extern "C" void __wrap_aitask_cmd_create_task(Lego_Object* object, float)
{
    refunded = object;
    ++refunds;
}
extern "C" void __wrap_aitask_cmd_cancel_by_coords(int, int, int, int) { ++cancellations; }

int main()
{
    Lego_Block blocks[25] = {};
    Lego_Level level = {};
    level.width = level.height = 5;
    level.blocks = blocks;
    legoGlobs.currLevel = &level;
    int pos[] = {2, 2};
    auto& tile = blocks[12];
    tile.flags1 = 0x20000000;
    CHECK_EQ(construction_zone_start_path(pos), 0);
    CHECK_EQ(construction_zone_exists_at_block(pos), 0);
    tile.flags1 = 8;
    CHECK_EQ(construction_zone_start_path(pos), 1);
    CHECK(tile.flags1 & 0x40000000, "path reserved");
    auto* zone = construction_zone_find_by_handle_or_at_block(pos, nullptr);
    if (!CHECK(zone != nullptr, "zone allocated")) return lrr_test_result();
    construction_update_all(1);
    construction_update_all(1);
    CHECK_EQ(requests, 2u);
    CHECK_EQ(zone->requestCount, 2u);
    Lego_Object ore = {}, second = {};
    ore.type = second.type = 7;
    construction_zone_place_resource(zone->handle, &ore);
    CHECK_EQ(construction_zone_needs_more_of_resource(zone->handle, 7, 0), 1);
    construction_update_all(1);
    CHECK_EQ(ready, 0u);
    construction_zone_cancel_path(pos);
    CHECK_EQ(refunds, 1u);
    CHECK_EQ(refunded, &ore);
    CHECK_EQ(ore.flags4 & 0x100, 0u);
    CHECK_EQ(cancellations, 1u);
    CHECK_EQ(tile.flags1 & 0x40000000, 0u);
    CHECK_EQ(construction_zone_exists_at_block(pos), 0);

    CHECK_EQ(construction_zone_start_path(pos), 1);
    zone = construction_zone_find_by_handle_or_at_block(pos, nullptr);
    if (!CHECK(zone != nullptr, "replacement zone allocated")) return lrr_test_result();
    construction_update_all(1);
    construction_zone_place_resource(zone->handle, &ore);
    construction_zone_place_resource(zone->handle, &second);
    CHECK_EQ(construction_zone_needs_more_of_resource(zone->handle, 7, 0), 0);
    construction_update_all(1);
    construction_update_all(1);
    CHECK_EQ(ready, 1u);
    construction_zone_complete_path(pos);
    CHECK_EQ(construction_zone_exists_at_block(pos), 0);
    CHECK_EQ(refunds, 1u);
    // A connected branch of power paths is refreshed; an isolated tile
    // and then the branch beyond a removed connector stay unpowered.
    for (auto& block : blocks) block = {};
    blocks[12].flags1 = blocks[13].flags1 = blocks[8].flags1 = 0x400;
    // Move the isolated tile away from cardinal contact with the network.
    blocks[16].flags1 = 0x400;
    construction_flood_path_blocks(pos);
    CHECK(blocks[12].flags2 & 0x100, "source revisited through connected path");
    CHECK(blocks[13].flags2 & 0x100, "connector powered");
    CHECK(blocks[8].flags2 & 0x100, "branch powered");
    CHECK_EQ(blocks[16].flags2 & 0x100, 0u);
    lego_terrain_cell_previous_state_track();
    blocks[13].flags1 &= ~0x400u;
    construction_flood_path_blocks(pos);
    CHECK_EQ(blocks[8].flags2 & 0x100, 0u);
    lego_terrain_cell_update_queue_flush();
    lego_terrain_cell_previous_state_track();
    lego_terrain_cell_update_queue_flush();
    legoGlobs.currLevel = nullptr;
    return lrr_test_result();
}
