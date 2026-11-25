// SPDX-License-Identifier: LGPL-3.0-or-later
/* Capability-dependent admission and staged terrain changes on generated
 * terrain, using a generic scene frame and real route/task records. */
#include <array>
#include <cstdlib>
#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "game/animation_object_record.h"
#include "game/aitask.h"
#include "game/aitask_globs.h"
#include "game/aitask_record.h"
#include "game/entity_stats.h"
#include "game/lego_unit.h"
#include "game/legoobject_unit.h"
#include "game/terrain_map.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"
#include "game/lego_globs.h"
#include "crt/vc5names.h"

extern "C" {
int terrain_cell_walkability_query_current(const Lego_Object*, uint32_t, uint32_t, int, int);
int lego_entity_move_destination_cell_usable(Lego_Object*, unsigned, unsigned);
int lego_terrain_cell_allows_placement(Lego_Object*, unsigned, unsigned);
int terrain_cell_drill_work_add_current(int, int, float, float);
int terrain_cell_speed_class_current(const int*);
uint32_t terrain_cell_flag_reserved_current(const int*);
void level_tile_mark_current_target(const int*, int);
int terrain_cell_variant_flag_propagate_current(const int*);
int legoobject_nearest_cell_route_commit(Lego_Object*, unsigned, unsigned, int);
}

namespace {
using namespace synthetic_fixture;
constexpr unsigned Floor = 8, Wall = 0x10, Clear = 0x200000;
constexpr unsigned Dry = 0x2000000, Water = 0x1000000, Floating = 1;
constexpr unsigned Driven = 0x20000, ClearRubble = 0x10000;

Lego_Block& Cell(unsigned x, unsigned y)
{
    auto* level = legoGlobs.currLevel;
    return level->blocks[y * level->width + x];
}

void Place(Lego_Object* object, int x, int y)
{
    float wx, wy;
    terrain_map_block_to_world(legoGlobs.currLevel->map, x, y, &wx, &wy);
    legoobject_world_position_set(object, wx, wy, 0, 1);
}

void CheckAdmission(Lego_Object& object, StatsData& stats)
{
    // Keep the generated level geometry, but temporarily use neutral
    // block records for this decision matrix. Restore before mutations.
    std::array<Lego_Block, W * H> saved;
    auto* level = legoGlobs.currLevel;
    for (unsigned i = 0; i < saved.size(); ++i) saved[i] = level->blocks[i];
    for (unsigned y = 1; y < 5; ++y)
        for (unsigned x = 1; x < 5; ++x) {
            Cell(x, y) = {};
            Cell(x, y).flags1 = Floor | Clear;
            Cell(x, y).f_00 = 1;
        }
    auto& cell = Cell(2, 2);
    const Lego_Block floor = cell;
    struct Case {
        unsigned flags1, flags2, terrain, dug, capabilities, state, extra;
        int expected;
    };
    const Case cases[] = {
        {Floor, 0, 0, 1, Dry, 0, 0, 1},
        {Floor, 0, 0, 1, Water, 0, 0, 0}, // water-only object on dry ground
        {Floor, 0, 9, 1, Dry, 0, 0, 0},
        {Floor, 0, 9, 1, Water, 0, 0, 1},
        {Wall, 0, 9, 1, Water | Floating, 0, 0, 3},
        {Wall, 0, 9, 0, Water | Floating, 0, 0, 0},
        {Floor | 0x2000, 0, 9, 1, Water | Floating, 0, 0, 2},
        {Floor, 0, 6, 1, Dry, 0, 0, 0},
        {Floor, 0, 6, 1, Dry, 0x200000, 0, 1},
        {Floor | 0x200, 0, 6, 1, Dry, 0, 0, 1},
        {Floor, 4, 0, 1, Dry, 0, 0, 0},
        {Floor, 4, 0, 1, Dry, 0, 0x200, 1},
        {Floor | 0x40, 0, 0, 1, Dry, 0, 0, 0},
        {Floor, 0, 0, 1, Dry | Driven, 0, 0, 0},
    };
    unsigned caseIndex = 0;
    for (const Case& entry : cases) {
        cell.flags1 = entry.flags1;
        cell.flags2 = entry.flags2;
        cell.f_02 = entry.terrain;
        cell.f_00 = entry.dug;
        stats.flags1 = entry.capabilities;
        stats.flags2 = entry.state;
        stats.flags3 = entry.extra;
        CHECK(lego_terrain_cell_route_and_occupancy_check(&object, 2, 2, 2, 3, 1)
              == entry.expected, ("route admission case " + std::to_string(caseIndex)).c_str());
        ++caseIndex;
    }
    cell = floor;
    stats.flags1 = Dry | Driven;
    Lego_Object driver = {};
    object.driveObject = &driver;
    CHECK_EQ(lego_terrain_cell_route_and_occupancy_check(&object, 2, 2, 2, 3, 1), 1);
    object.driveObject = nullptr;
    stats.flags1 = Dry;
    object.type = 11;
    CHECK_EQ(lego_terrain_cell_route_and_occupancy_check(&object, 2, 2, 2, 3, 1), 0);
    object.type = 2;

    CHECK_EQ(lego_terrain_cell_route_and_occupancy_check(&object, 2, 2, 3, 3, 1), 2);
    Cell(2, 3).flags1 = Wall;
    CHECK_EQ(lego_terrain_cell_route_and_occupancy_check(&object, 2, 2, 3, 3, 1), 1);
    Cell(2, 3) = floor;
    cell.flags1 = Wall | 0x400;
    Place(&object, 2, 2);
    CHECK_EQ(lego_terrain_cell_route_and_occupancy_check(&object, 2, 2, 2, 3, 0), 1);
    CHECK_EQ(lego_terrain_cell_route_and_occupancy_check(&object, 2, 2, 2, 3, 1), 0);

    // A floating object cannot enter a water edge beside lava or rock.
    cell = floor;
    cell.flags1 = Wall;
    cell.f_02 = 9;
    stats.flags1 = Water | Floating;
    for (unsigned material : {6u, 7u}) {
        Cell(1, 2).f_02 = material;
        CHECK_EQ(lego_terrain_cell_route_and_occupancy_check(&object, 2, 2, 2, 3, 1), 0);
    }
    Cell(1, 2) = floor;
    Cell(2, 1).f_02 = 6;
    CHECK_EQ(lego_terrain_cell_route_and_occupancy_check(&object, 2, 2, 2, 3, 1), 0);
    Cell(2, 1) = floor;
    CHECK_EQ(lego_terrain_cell_route_and_occupancy_check(&object, 2, 2, 2, 3, 1), 3);
    Cell(2, 3).flags1 |= 0x2000;
    CHECK_EQ(lego_terrain_cell_route_and_occupancy_check(&object, 2, 2, 2, 3, 1), 2);
    Cell(2, 3) = floor;

    // Drill admission and placement are different policies.
    stats.flags1 = Dry;
    stats.flags2 = stats.flags3 = 0;
    object.flags3 = 2 | 4 | 0x8000;
    for (unsigned material : {1u, 4u, 7u, 8u, 10u, 11u}) {
        stats.DrillTimes[material] = 10.0f;
        cell.f_02 = material;
        cell.flags1 = Wall;
        const bool drillable = material != 1 && material != 7 && material != 11;
        CHECK_EQ(terrain_cell_walkability_query_current(&object, 2, 2, 0, 1), drillable);
        CHECK_EQ(lego_entity_move_destination_cell_usable(&object, 2, 2), drillable);
        CHECK_EQ(lego_terrain_cell_allows_placement(&object, 2, 2),
                 material != 8 && material != 10 && material != 11);
    }
    cell.f_02 = 4;
    cell.flags1 = Wall;
    stats.DrillTimes[4] = 0.0f;
    CHECK_EQ(terrain_cell_walkability_query_current(&object, 2, 2, 0, 1), 0);
    stats.DrillTimes[4] = 10.0f;
    stats.flags1 = Dry | Driven;
    CHECK_EQ(terrain_cell_walkability_query_current(&object, 2, 2, 0, 1), 0);
    object.driveObject = &driver;
    CHECK_EQ(terrain_cell_walkability_query_current(&object, 2, 2, 0, 1), 1);
    object.driveObject = nullptr;
    stats.flags1 = Dry;
    object.flags3 = 0;
    CHECK_EQ(terrain_cell_walkability_query_current(&object, 2, 2, 0, 1), 0);
    CHECK_EQ(lego_terrain_cell_allows_placement(&object, 2, 2), 0);
    CHECK_EQ(lego_entity_move_destination_cell_usable(&object, 2, 2), 0);
    object.flags3 = 2 | 4 | 0x8000;
    cell.flags1 = Wall | 0x40;
    CHECK_EQ(terrain_cell_walkability_query_current(&object, 2, 2, 0, 1), 0);
    cell.flags1 = Wall | 0x20;
    CHECK_EQ(lego_terrain_cell_allows_placement(&object, 2, 2), 0);
    cell.flags1 = Wall;
    object.flags1 = 0x400;
    CHECK_EQ(terrain_cell_walkability_query_current(&object, 2, 2, 0, 1), 0);
    CHECK_EQ(terrain_cell_walkability_query_current(&object, 2, 2, 0, 0), 1);
    object.flags1 = 0;
    CHECK_EQ(terrain_cell_walkability_query_current(&object, 2, 2, 1, 1), 0);
    stats.flags1 = Dry | Floating;
    cell.f_00 = 0;
    for (auto pos : {std::array<unsigned, 2>{1,2}, {2,1}, {3,2}, {2,3}})
        Cell(pos[0], pos[1]).f_00 = 0;
    CHECK_EQ(terrain_cell_walkability_query_current(&object, 2, 2, 1, 1), 0);
    Cell(1, 2).f_00 = 1;
    for (auto pos : {std::array<unsigned, 2>{1,1}, {2,1}, {1,2}, {2,2}})
        Cell(pos[0], pos[1]).f_02 = 1;
    CHECK_EQ(terrain_cell_walkability_query_current(&object, 2, 2, 1, 1), 0);
    cell.f_02 = 4;
    CHECK_EQ(terrain_cell_walkability_query_current(&object, 2, 2, 1, 1), 1);
    CHECK_EQ(terrain_cell_walkability_query_current(&object, W - 2, 2, 0, 1), 0);
    CHECK_EQ(lego_terrain_cell_allows_placement(&object, 0, 2), 0);
    CHECK_EQ(lego_entity_move_destination_cell_usable(&object, 2, H - 2), 0);

    cell = floor;
    Cell(1, 2) = floor;
    Cell(2, 1) = floor;
    CHECK_EQ(lego_terrain_placement_blocked_adjacent_cell_scan(2, 2, 1, 0), 1);
    Cell(3, 2).f_02 = 11;
    CHECK_EQ(lego_terrain_placement_blocked_adjacent_cell_scan(2, 2, 1, 0), 0);
    Cell(3, 2).f_02 = 0;
    cell.flags2 = 0x20;
    CHECK_EQ(lego_terrain_placement_blocked_adjacent_cell_scan(2, 2, 1, 0), 0);
    cell.flags2 = 0;
    cell.f_02 = 9;
    CHECK_EQ(lego_terrain_placement_blocked_adjacent_cell_scan(2, 2, 1, 0), 0);
    CHECK_EQ(lego_terrain_placement_blocked_adjacent_cell_scan(2, 2, 1, 1), 1);

    // Only one neighbour is viable, independently of rotation order.
    stats.flags1 = Dry;
    int centre[] = {3, 3}, chosen[] = {-1, -1};
    Cell(2, 3).flags1 = Wall;
    Cell(3, 2).flags1 = Wall;
    Cell(4, 3).flags1 = Wall;
    Cell(3, 4) = floor;
    CHECK_EQ(lego_first_free_neighbor_cell_pick(&object, centre, chosen, 1), 1);
    CHECK_EQ(chosen[0], 3);
    CHECK_EQ(chosen[1], 4);
    Cell(3, 4).flags1 = Wall;
    CHECK_EQ(lego_first_free_neighbor_cell_pick(&object, centre, chosen, 1), 0);

    for (unsigned i = 0; i < saved.size(); ++i) level->blocks[i] = saved[i];
    object.flags1 = 0;
    object.flags3 = 0x20408006;
    stats.flags1 = Dry | ClearRubble;
}

unsigned Terrain(unsigned x, unsigned y)
{
    if (x == 0 || y == 0 || x == W - 1 || y == H - 1) return 1;
    if (x == 4 && y == 3) return 8;
    return x == 3 || x == 4 ? 4 : 0;
}

unsigned Resources(unsigned x, unsigned y)
{
    return x == 4 && y == 3 ? 2 : 0;
}

int CountOre(Lego_Object* obj, void* data)
{
    if (obj->type == 7) ++*static_cast<unsigned*>(data);
    return 0;
}

unsigned OreCount()
{
    unsigned count = 0;
    legoobject_run_through_lists_skip_upgrade_parts(CountOre, &count);
    return count;
}

void CheckRoute(bool expected)
{
    unsigned *xs = nullptr, *ys = nullptr, count = 0;
    CHECK_EQ(legoobject_grid_route_search_adapter(nullptr, 2, 3, 5, 3, &xs, &ys, &count), expected);
    if (expected) CHECK_EQ(count, 4u);
    free(xs);
    free(ys);
}

void CheckExcavation(Lego_Object& object, StatsData& stats)
{
    auto* level = legoGlobs.currLevel;
    int target[] = {4, 3};
    auto& wall = Cell(4, 3);
    CHECK_EQ(wall.f_02, 8);
    // A reserved face cannot admit another placement job. Release the
    // reservation, then let the real command reserve it exactly once.
    level_tile_mark_current_target(target, 1);
    CHECK_EQ(aitask_kind_nine_cell_command_issue(target, 1), 0);
    level_tile_mark_current_target(target, 0);
    CHECK_EQ(aitask_kind_nine_cell_command_issue(target, 1), 1);
    AITask* placement = aiTaskGlobs.globalHead;
    CHECK(placement && placement->type == 9 && placement->blockX == 4 &&
          placement->blockY == 3, "placement job queued");
    CHECK(terrain_cell_flag_reserved_current(target), "placement reserves face");
    CHECK_EQ(aitask_kind_nine_cell_command_issue(target, 1), 0);
    CHECK_EQ(aiTaskGlobs.globalHead, placement);
    // The remaining scenario isolates excavation from this unrelated job.
    aitask_cmd_flush_all(0);
    aitask_cmd_flush_all(1);
    level_tile_mark_current_target(target, 0);
    if (!CHECK(lego_load_path_ai(level, "L\\resources.map", 0), "load resource layer")) return;
    const unsigned initialOre = OreCount();
    CheckRoute(false);
    stats.DrillTimes[8] = 4.0f;
    Place(&object, 1, 3);
    CHECK_EQ(legoobject_nearest_cell_route_commit(&object, 4, 3, 0), 0);
    CHECK_EQ(object.routeBlocks, nullptr);
    Place(&object, 5, 1);
    CHECK_EQ(legoobject_nearest_cell_route_commit(&object, 4, 3, 0), 1);
    if (CHECK(object.routeBlocks && object.routeBlocksTotal > 2, "route to remote face")) {
        const auto* route = static_cast<const LegoObjG_RouteBlock*>(object.routeBlocks);
        const auto& last = route[object.routeBlocksTotal - 1];
        CHECK_EQ(last.blockPos[0], 4);
        CHECK_EQ(last.blockPos[1], 3);
        CHECK_EQ(last.actionFlags, 1);
    }
    legoobject_action_sequence_reset(&object, 0);
    Place(&object, 5, 3);
    CHECK_EQ(legoobject_nearest_cell_route_commit(&object, 4, 3, 0), 1);
    if (CHECK(object.routeBlocks && object.routeBlocksTotal == 2, "route to drillable face")) {
        auto* route = static_cast<LegoObjG_RouteBlock*>(object.routeBlocks);
        CHECK_EQ(route[1].blockPos[0], 4);
        CHECK_EQ(route[1].blockPos[1], 3);
        CHECK_EQ(route[1].actionFlags, 1);
    }
    legoobject_action_sequence_reset(&object, 0);
    CHECK_EQ(object.routeBlocks, nullptr);

    // The actual terrain command remains pending until the dig completes.
    aitask_terrain_cell_command_create(target, 0, 0);
    AITask* drill = aiTaskGlobs.globalHead;
    if (!CHECK(drill && drill->type == 8 && drill->blockX == 4 &&
               drill->blockY == 3, "drilling command queued")) return;
    CHECK(wall.flags1 & 0x800000, "drill target highlighted");

    // The material needs four completed drilling cycles, not four calls
    // with insufficient work. Exactly 1.0 is still below completion.
    for (unsigned stage = 1; stage <= 4; ++stage) {
        CHECK_EQ(terrain_cell_drill_work_add_current(4, 3, 0, 1), 0);
        CHECK_EQ(terrain_cell_drill_work_add_current(4, 3, 4, 2), 0);
        CHECK_EQ(terrain_cell_drill_work_add_current(4, 3, 4, 2), 0);
        CHECK_NEAR(wall.damage, 1.0f, 1e-6);
        CHECK_EQ(terrain_cell_drill_work_add_current(4, 3, 4, 1), 1);
        CHECK_EQ(lego_terrain_cell_mutate(level, 4, 3, 0), stage == 4);
        CHECK_EQ(wall.f_22, stage);
        CHECK_NEAR(wall.damage, 0.0f, 1e-6);
        for (unsigned frame = 0; frame < 3; ++frame) lego_engine_tick(1.0f);
        CheckRoute(stage == 4);
        // Each seam strike produces one ore in an adjacent open cell.
        // The final dig-complete event additionally opens the resource layer.
        CHECK_EQ(OreCount(), initialOre + stage + (stage == 4 ? 1u : 0u));
        CHECK_EQ(Cell(5, 3).f_30, stage);
        CHECK_EQ(wall.f_30, stage == 4 ? 1 : 0);
        unsigned pendingDrills = 0;
        for (AITask* task = aiTaskGlobs.globalHead; task; task = task->next)
            if (task->type == 8 && task->blockX == 4 && task->blockY == 3)
                ++pendingDrills;
        CHECK_EQ(pendingDrills, stage < 4 ? 1u : 0u);
    }
    CHECK_EQ(wall.f_30, 1);
    CHECK_EQ(wall.flags1 & Floor, Floor);
    CHECK_EQ(wall.flags1 & 0x800000u, 0u);
    CHECK_EQ(terrain_cell_speed_class_current(target), 4);
    CHECK_EQ(lego_terrain_cell_mutate(level, 4, 3, 0), 0);

    // Excavation creates real clear jobs. Complete the four stage
    // commands in order at their existing completion boundary.
    for (unsigned remaining = 4; remaining > 0; --remaining) {
        AITask* prev = nullptr;
        AITask* task = aiTaskGlobs.globalHead;
        while (task && !(task->type == 12 && task->blockX == 4 && task->blockY == 3)) {
            prev = task;
            task = task->next;
        }
        if (!CHECK(task != nullptr, "pending clear task")) return;
        aitask_entity_record_attach_and_relink(task, prev, &object);
        float elapsed = 1.0f;
        aitask_cmd_entity_command_chain_tick(&object, &elapsed);
        CHECK(object.routeBlocks != nullptr, "clear task has route");
        for (unsigned i = 0; object.routeBlocks && i < 100; ++i)
            legoobject_path_step_advance_and_arrival_check(&object, elapsed);
        CHECK_EQ(object.routeBlocks, nullptr);
        CHECK(object.flags1 & 0x40000, "clear activity reached");
        aitask_entity_command_unlink_by_kind(&object, 12, nullptr, 1);
        object.flags1 &= ~0x40000; // No authored clear animation in the fixture.
        CHECK_EQ(terrain_cell_speed_class_current(target), remaining - 1);
        CHECK_EQ(wall.flags1 & Clear, remaining == 1 ? Clear : 0u);
    }
    CHECK_EQ(terrain_cell_variant_flag_propagate_current(target), 1);
    CHECK(wall.flags1 & 0x20000000, "cleared terrain becomes a path");
    CHECK_EQ(terrain_cell_flag_reserved_current(target), 0u);
    CheckRoute(true);
    CHECK_EQ(OreCount(), initialOre + 5);
}
} // namespace

int main()
{
    if (!CHECK(make_fixture("terrain_transitions_fixture"), "generate fixture") ||
        !CHECK(write_map("Data/L/terr.map", W, H, Terrain), "generate staged wall") ||
        !CHECK(write_map("Data/L/resources.map", W, H, Resources), "generate resource layer") ||
        !CHECK(write_text("Data/L/empty.ptl", "Lego* { DigComplete GenerateFromCryOre }\n"), "event translations") ||
        !initialise_headless("TerrainTransitions") || !boot_game()) return lrr_test_result();
    crt_srand(753);
    if (!CHECK(lego_level_load("Level01"), "load terrain")) return lrr_test_result();
    AnimationObject model = {};
    model.container = Container_Create(static_cast<Container*>(legoGlobs_rootCont));
    if (!CHECK(model.container != nullptr, "generic scene frame")) return lrr_test_result();
    StatsData stats = {};
    Lego_Object object = {};
    object.type = 2;
    object.stats = &stats;
    object.miniFigure = &model;
    object.health = object.energy = 100.0f;
    stats.RouteSpeed = 8.0f;
    stats.RubbleCoef = stats.PathCoef = 1.0f;
    CheckAdmission(object, stats);
    CheckExcavation(object, stats);
    Container_Remove(static_cast<Container*>(model.container));
    return lrr_test_result("terrain_transitions");
}
