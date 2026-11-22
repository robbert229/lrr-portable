// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_entity_stats — per-level record selection and upgrade counters,
 * plus tool lookup across multiple task types. */
#include "game/entity_stats_link_names.h"
#include "game/entity_stats.h"
#include "game/legoobject_unit.h"
#include "test/lrr_test.h"

int main(void)
{
    Stats_Globs* stats = entity_stats_port_globs();
    constexpr int type = 1;
    constexpr int id = 3;
    StatsData levels[2] = {};
    StatsData* byId[STATS_OBJECT_IDS] = {};
    byId[id] = levels;
    stats->objectStats[type] = byId;
    stats->objectLevels[type][id] = 2;
    levels[0].RouteSpeed = 2.0f;
    levels[0].CostOre = 4;
    levels[1].RouteSpeed = 3.5f;
    levels[1].CostOre = 8;
    levels[1].DrillTimes[4] = 12.0f;
    levels[1].flags2 = 0x400;

    StatsObject vehicle = {};
    vehicle.type = type;
    vehicle.id = id;
    CHECK_EQ(stats_get_levels(type, id), 2u);
    CHECK_EQ(stats_get_cost_ore(type, id, 0), 4);
    CHECK_EQ(stats_get_cost_ore(type, id, 1), 8);
    CHECK_EQ(stats_object_set_object_level(&vehicle, 0), 1);
    CHECK_EQ(vehicle.stats, &levels[0]);
    CHECK_EQ(stats_object_get_route_speed(&vehicle), 2.0f);

    CHECK_EQ(stats_object_set_object_level(&vehicle, 1), 1);
    CHECK_EQ(vehicle.objLevel, 1u);
    CHECK_EQ(vehicle.stats, &levels[1]);
    CHECK_EQ(stats_object_get_route_speed(&vehicle), 3.5f);
    CHECK_EQ(stats_object_get_drill_time_type(&vehicle, 4), 12.0f);
    CHECK_EQ(stats_object_get_stats_flags2(&vehicle), 0x400u);
    CHECK_EQ(stats_object_set_object_level(&vehicle, 1), 1);
    CHECK_EQ(stats_object_set_object_level(&vehicle, 2), 0);
    CHECK_EQ(vehicle.objLevel, 1u);
    CHECK_EQ(vehicle.stats, &levels[1]);
    CHECK_EQ(legoobject_grid_record_count_get(type, id, 0), 1u);
    CHECK_EQ(legoobject_grid_record_count_get(type, id, 1), 0u);
    CHECK_EQ(legoobject_grid_record_count_difference_get(type, id, 0, 1), -1);
    CHECK_EQ(legoobject_grid_record_count_difference_get(type, id, 1, 1), 1);

    stats_add_tool_task_type(4, 19);
    stats_add_tool_task_type(4, 23);
    stats_add_tool_task_type(6, 25);
    int tool = -1;
    CHECK_EQ(stats_find_tool_from_task_type(23, &tool), 1);
    CHECK_EQ(tool, 4);
    CHECK_EQ(stats_find_tool_from_task_type(25, &tool), 1);
    CHECK_EQ(tool, 6);
    CHECK_EQ(stats_find_tool_from_task_type(26, &tool), 0);
    CHECK_EQ(tool, 6);
    stats->objectStats[type] = nullptr;
    return lrr_test_result();
}
