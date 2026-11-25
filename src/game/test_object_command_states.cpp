// SPDX-License-Identifier: LGPL-3.0-or-later
/* Actual command and activity progression using generated null-only
 * animations and a small abstract terrain fixture. */
#include <string>
#include "gods98/config.h"
#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/lws.h"
#include "game/aitask.h"
#include "game/aitask_globs.h"
#include "game/aitask_record.h"
#include "game/animation_object_record.h"
#include "game/building_model_record.h"
#include "game/construction_link_names.h"
#include "game/construction.h"
#include "game/entity_stats_link_names.h"
#include "game/entity_stats.h"
#include "game/helpwindow_link_names.h"
#include "game/helpwindow.h"
#include "game/lego_unit.h"
#include "game/legoobject_unit.h"
#include "game/terrain_map.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"
#include "game/lego_globs.h"
#include "crt/vc5names.h"

extern "C" {
void legoobject_activity_grant(Lego_Object*, int);
int terrain_cell_speed_class_current(const int*);
int legoobject_pair_scalar_fields_set(Lego_Object*, float*, float);
void legoobject_heading_step_move(Lego_Object*, float);
}

namespace {
using namespace synthetic_fixture;
constexpr unsigned ClearActivity = 0x40000, RepairActivity = 0x1000000, EatActivity = 0x40000000;
constexpr unsigned TrainActivity = 0x400;

Lego_Level* Level() { return legoGlobs.currLevel; }
Lego_Block& Cell(int x, int y) { return Level()->blocks[y * Level()->width + x]; }

bool LoadCatalog()
{
    if (!CHECK(fixture_mkdir("Data/Primitive") &&
        write_null_scene("Data/Primitive/motion.lws") &&
        write_text("Data/Primitive/Primitive.ae",
            "Lego { Activities { Activity_Stand Motion Activity_Route Motion "
            "Activity_RouteRubble Motion Activity_TurnLeft Work Activity_TurnRight Work "
            "Activity_Reverse Motion Activity_Drill Motion Activity_Clear Work "
            "Activity_Train Work Activity_Repair Work Activity_Eat Work "
            "Activity_CantDo Work Activity_Rest Work Activity_Place Work "
            "Activity_Unpowered Motion } "
            "Motion { FILE motion LWSFILE TRUE LOOPING TRUE } "
            "Work { FILE motion LWSFILE TRUE LOOPING FALSE } "
            "DrillNullName Anchor CarryNullName Anchor CarryNullFrames 1 }"),
        "generate neutral moving-null catalog")) return false;
    constexpr char catalog[] =
        "Lego { MiniFigureTypes { Figure Primitive } BuildingTypes { Station Primitive } "
        "Stats { Figure { Levels 1 CollRadius 2 CrossLand TRUE RouteSpeed 6 "
        "RepairValue 30 NumOfToolsCanCarry 2 CanClearRubble TRUE } "
        "Station { Levels 1 CollRadius 4 SelfPowered TRUE TrainRepair TRUE } "
        "Ore { Levels 1 CollRadius 5 PickSphere 10 } "
        "PowerCrystal { Levels 1 CollRadius 5 PickSphere 10 } } "
        "HelpWindow { TextWindowArea 0,0,16,16 } }";
    auto* cfg = block_parse(catalog, sizeof(catalog) - 1);
    if (!CHECK(cfg != nullptr, "parse catalog")) return false;
    auto* old = legoGlobs.config;
    legoGlobs.config = cfg;
    lws_initialise(nullptr, nullptr, nullptr, nullptr);
    bool ok = CHECK(lego_entity_minifigure_type_table_load(), "load generic figure") &&
              CHECK(lego_entity_building_types_load(), "load generic station") &&
              CHECK(stats_initialise(cfg, "Lego"), "load stats");
    if (ok) help_window_initialise(cfg, "Lego");
    legoGlobs.config = old;
    block_close(cfg);
    return ok;
}

Lego_Object* Figure(int x, int y)
{
    float wx, wy;
    terrain_map_block_to_world(Level()->map, x, y, &wx, &wy);
    auto* obj = legoobject_create_in_world(legoGlobs.miniFigureData, 2, 0, 0, wx, wy, 0);
    if (!CHECK(obj != nullptr, "create figure")) return nullptr;
    // A neutral worker is ready for the AI tick, with no spawn sequence.
    obj->flags3 |= 0x20400060;
    return obj;
}

Lego_Object* Station(int x, int y)
{
    int pos[] = {x, y};
    auto* obj = static_cast<Lego_Object*>(construction_place_building(0, pos, 0, pos, 1, 0));
    if (CHECK(obj != nullptr, "create station")) {
        legoobject_kind_four_post_create_resolve(obj);
        CHECK(legoobject_shared_eligibility_check(obj, 0), "station active");
    }
    return obj;
}

void Place(Lego_Object* obj, int x, int y)
{
    float wx, wy;
    terrain_map_block_to_world(Level()->map, x, y, &wx, &wy);
    legoobject_world_position_set(obj, wx, wy, 0, 1);
}

void CheckCell(Lego_Object* obj, int x, int y)
{
    int bx = -1, by = -1;
    CHECK(legoobject_position_to_terrain_cell(obj, &bx, &by), "object on map");
    CHECK_EQ(bx, x);
    CHECK_EQ(by, y);
}

void Tick(Lego_Object* obj, float elapsed = 1.0f)
{
    legoobject_update_callback(obj, &elapsed);
}

void AI(Lego_Object* obj, float elapsed = 1.0f)
{
    aitask_cmd_entity_command_chain_tick(obj, &elapsed);
}

void ClearJobs()
{
    aitask_cmd_flush_all(0);
    aitask_cmd_flush_all(1);
}

AITask* Pending(int type, Lego_Object* target)
{
    for (AITask* task = aiTaskGlobs.globalHead; task; task = task->next)
        if (task->type == type && task->object == target) return task;
    return nullptr;
}

int Eligible(AITask* task, Lego_Object* obj, int checkTool = 1, int checkAbility = 1)
{
    return aitask_place_command_eligibility_check(task, obj, nullptr, nullptr, nullptr,
        checkTool, checkAbility);
}

void Training()
{
    auto* station = Station(2, 3);
    auto* figure = Figure(1, 3);
    if (!station || !figure) return;
    ClearJobs();
    Level()->trainTime = 10.0f;
    figure->field_3d4 = 0;
    figure->abilityFlags = 2; // Preserve an existing ability.
    aitask_task_type23_create_for_category_mask(station, 0x10, 0);
    auto* task = Pending(23, station);
    if (!CHECK(task != nullptr, "training request created")) return;
    CHECK_EQ(Eligible(task, figure), 0);
    for (unsigned i = 0; i < 5; ++i) Tick(figure, 2.0f);
    CHECK_EQ(figure->field_3d4, 10.0f);
    task->timeIn = 5;
    CHECK_EQ(Eligible(task, figure), 0);
    float wait = 5;
    aitask_cmd_pool_timer_delta_apply(task, &wait);
    figure->flags1 |= 0x800000;
    CHECK_EQ(Eligible(task, figure), 0);
    figure->flags1 &= ~0x800000u;
    figure->flags4 |= 0x400;
    CHECK_EQ(Eligible(task, figure), 0);
    figure->flags4 &= ~0x400u;
    CHECK_EQ(figure->aiTask, nullptr);
    CHECK_EQ(Pending(23, station), task);
    float wPos[2] = {};
    int bPos[2] = {-1, -1};
    CHECK_EQ(aitask_place_command_eligibility_check(task, figure, wPos, bPos, nullptr, 1, 1), 1);
    CHECK_EQ(bPos[0], 2);
    CHECK_EQ(bPos[1], 3);
    AI(figure);
    CHECK_EQ(figure->aiTask, task);
    CHECK_EQ(Pending(23, station), nullptr);
    AI(figure);
    CHECK(figure->routeBlocks != nullptr, "training approach route");
    CHECK_EQ(figure->routeToObject, station);
    for (unsigned i = 0; i < 100 && !(figure->flags2 & TrainActivity); ++i) Tick(figure);
    CHECK(figure->flags2 & TrainActivity, "arrived at training activity");
    CHECK_EQ(figure->abilityFlags, 2u);
    for (unsigned i = 0; i < 40 && (figure->flags2 & TrainActivity); ++i) Tick(figure);
    CHECK_EQ(figure->flags2 & TrainActivity, 0u);
    CHECK_EQ(figure->abilityFlags, 0x12u);
    CHECK_EQ(figure->field_3d4, 0.0f);
    CHECK_EQ(aitask_entity_task_chain_contains_kind(figure, 23), 0);
    AITask duplicate = {};
    duplicate.type = 23;
    duplicate.field_3c = 0x10;
    duplicate.object = station;
    figure->field_3d4 = 100;
    CHECK_EQ(Eligible(&duplicate, figure), 0);
}

void RepairAndEat()
{
    auto* station = Station(2, 3);
    auto* figure = Figure(1, 3);
    if (!station || !figure) return;
    ClearJobs();
    station->health = 25;
    figure->abilityFlags = 0;
    aitask_entity_threshold_progress_event_queue(station, 0);
    auto* task = Pending(10, station);
    if (!CHECK(task != nullptr, "repair request")) return;
    CHECK_EQ(Eligible(task, figure), 0);
    legoobject_activity_grant(figure, 3);
    CHECK_EQ(Eligible(task, figure), 0);
    CHECK_EQ(Eligible(task, figure, 1, 0), 1);
    figure->abilityFlags |= 0x10;
    CHECK_EQ(Eligible(task, figure), 1);
    AI(figure);
    CHECK_EQ(figure->aiTask, task);
    AI(figure);
    CHECK(figure->routeBlocks != nullptr, "repair route");
    for (unsigned i = 0; i < 100 && !(figure->flags1 & RepairActivity); ++i) Tick(figure);
    CHECK(figure->flags1 & RepairActivity, "repair activity reached");
    float previous = station->health;
    unsigned increases = 0;
    for (unsigned i = 0; i < 100 && (figure->flags1 & RepairActivity); ++i) {
        Tick(figure);
        CHECK(station->health >= previous && station->health <= 100, "repair monotonic and capped");
        if (station->health > previous) ++increases;
        previous = station->health;
    }
    CHECK_EQ(station->health, 100.0f);
    CHECK_EQ(increases, 3u);
    CHECK_EQ(figure->flags1 & RepairActivity, 0u);
    CHECK_EQ(aitask_entity_task_chain_contains_kind(figure, 10), 0);
    CHECK_EQ(figure->routeToObject, nullptr);
    aitask_cmd_tick_global_delta(1.0f);
    CHECK_EQ(Pending(10, station), nullptr);
    CHECK_EQ(aitask_entity_task_chain_contains_kind(figure, 10), 0);

    ClearJobs();
    figure->health = 80;
    figure->energy = 20;
    legoGlobs.MinEnergyForEat = 80;
    AITask eat = {};
    eat.type = 17;
    eat.object = station;
    CHECK_EQ(Eligible(&eat, figure), 1);
    legoobject_flag80000000_set_or_clear(station, 0);
    CHECK_EQ(Eligible(&eat, figure), 0);
    legoobject_flag80000000_set_or_clear(station, 1);
    figure->flags2 |= 0x800;
    CHECK_EQ(Eligible(&eat, figure), 0);
    figure->flags2 &= ~0x800u;
    figure->energy = 75;
    CHECK_EQ(Eligible(&eat, figure), 0);
    figure->energy = 20;
    figure->flags1 |= 0x400;
    CHECK_EQ(Eligible(&eat, figure), 0);
    figure->flags1 &= ~0x400;
    aitask_entity_target_command_queue(figure, station);
    AI(figure);
    CHECK(figure->routeBlocks != nullptr, "meal approach route");
    for (unsigned i = 0; i < 100 && !(figure->flags1 & EatActivity); ++i) Tick(figure);
    CHECK(figure->flags1 & EatActivity, "meal activity reached");
    previous = figure->energy;
    increases = 0;
    for (unsigned i = 0; i < 100 && (figure->flags1 & EatActivity); ++i) {
        Tick(figure);
        CHECK(figure->energy >= previous && figure->energy <= figure->health, "energy recovery capped by health");
        if (figure->energy > previous) ++increases;
        previous = figure->energy;
    }
    CHECK_EQ(figure->energy, 80.0f);
    CHECK_EQ(increases, 3u);
    CHECK_EQ(figure->flags1 & EatActivity, 0u);
    CHECK_EQ(aitask_entity_task_chain_contains_kind(figure, 17), 0);
}

void FollowAndClear()
{
    auto* leader = Figure(2, 4);
    auto* follower = Figure(1, 1);
    if (!leader || !follower) return;
    ClearJobs();
    AITask* follow = aitask_cmd_alloc(1);
    follow->object = leader;
    follower->aiTask = follow;
    AI(follower);
    CHECK_EQ(follow->time, 50.0f);
    CHECK(follower->routeBlocks != nullptr, "follow route issued");
    for (unsigned i = 0; i < 3; ++i) Tick(follower);
    // A turn can temporarily insert an idle animation marker before
    // the follow command. Let the actual turn finish before interrupting.
    for (unsigned i = 0; i < 60 && (follower->aiTask != follow || (follower->flags1 & 4)); ++i) Tick(follower);
    CHECK_EQ(follower->aiTask, follow);
    CHECK_EQ(follower->flags1 & 4u, 0u);
    CHECK(follower->routeBlocks != nullptr, "interrupted during approach");
    legoobject_action_sequence_reset(follower, 0);
    CHECK_EQ(follower->routeBlocks, nullptr);
    CHECK_EQ(follower->aiTask, follow);
    Place(leader, 2, 2);
    float elapsed = 25.0f;
    aitask_cmd_pool_timer_delta_apply(follow, &elapsed);
    AI(follower);
    CHECK_EQ(follow->time, 25.0f);
    CHECK_EQ(follower->routeBlocks, nullptr);
    aitask_cmd_pool_timer_delta_apply(follow, &elapsed);
    AI(follower);
    CHECK(follower->routeBlocks != nullptr, "follow resumes after timer");
    for (unsigned i = 0; i < 100 && follower->routeBlocks; ++i) Tick(follower);
    CHECK_EQ(follower->routeBlocks, nullptr);
    CheckCell(follower, 2, 2);
    follow->flags |= 0x20;
    AI(follower);
    CHECK_EQ(follower->aiTask, nullptr);

    int target[] = {2, 2};
    auto& cell = Cell(2, 2);
    cell.flags1 &= ~0x200000u;
    cell.flags1 = (cell.flags1 & ~3u) | 2u;
    CHECK_EQ(terrain_cell_speed_class_current(target), 3);
    aitask_coordinate_pair_event_queue(target, nullptr);
    auto* clear = Pending(12, nullptr);
    if (!CHECK(clear != nullptr, "clear job queued")) return;
    legoobject_activity_grant(follower, 1);
    follower->stats->flags1 &= ~0x10000u;
    CHECK_EQ(Eligible(clear, follower), 0);
    follower->stats->flags1 |= 0x10000u;
    CHECK_EQ(Eligible(clear, follower), 1);
    AI(follower);
    CHECK_EQ(follower->aiTask, clear);
    AI(follower);
    CHECK(follower->routeBlocks != nullptr, "clear route issued");
    for (unsigned i = 0; i < 100 && !(follower->flags1 & ClearActivity); ++i) Tick(follower);
    CHECK(follower->flags1 & ClearActivity, "clear activity reached");
    CHECK_EQ(terrain_cell_speed_class_current(target), 3);
    for (unsigned i = 0; i < 40 && (follower->flags1 & ClearActivity); ++i) Tick(follower);
    CHECK_EQ(follower->flags1 & ClearActivity, 0u);
    CHECK_EQ(terrain_cell_speed_class_current(target), 2);
    CHECK_EQ(aitask_entity_task_chain_contains_kind(follower, 12), 0);
}
void Drill()
{
    auto* figure = Figure(5, 3);
    if (!figure) return;
    ClearJobs();
    int target[] = {4, 3};
    auto& wall = Cell(4, 3);
    CHECK_EQ(wall.flags1 & 8u, 0u);
    figure->stats->DrillTimes[4] = 0;
    aitask_terrain_cell_command_create(target, 0, 0);
    auto* task = Pending(8, nullptr);
    if (!CHECK(task != nullptr, "drill request")) return;
    CHECK_EQ(Eligible(task, figure), 0);
    figure->stats->DrillTimes[4] = 8.0f;
    CHECK_EQ(Eligible(task, figure), 1);
    figure->flags1 |= 0x400;
    CHECK_EQ(Eligible(task, figure), 0);
    figure->flags1 &= ~0x400;
    AI(figure);
    CHECK_EQ(figure->aiTask, task);
    AI(figure);
    CHECK(figure->routeBlocks != nullptr, "drill approach issued");
    for (unsigned i = 0; i < 100 && !(figure->flags1 & 8); ++i) Tick(figure);
    CHECK(figure->flags1 & 8, "drill reached");
    float previous = wall.damage;
    unsigned advances = 0;
    for (unsigned i = 0; i < 100 && !(wall.flags1 & 8); ++i) {
        Tick(figure);
        if (wall.damage > previous) ++advances;
        previous = wall.damage;
    }
    CHECK(advances >= 2, "work accumulates over multiple updates");
    CHECK(wall.flags1 & 8, "wall opens");
    CHECK_EQ(figure->flags1 & 8u, 0u);
    CHECK_EQ(aitask_entity_task_chain_contains_kind(figure, 8), 0);
    CHECK_EQ(Pending(8, nullptr), nullptr);
    CHECK_EQ(wall.flags1 & 0x8000000u, 0u);
}
void TurningAndSlide()
{
    auto* figure = Figure(2, 3);
    if (!figure) return;
    ClearJobs();
    constexpr unsigned Turning = 4, HalfTurn = 0x80, Sliding = 0x4000;
    float x, y;
    terrain_map_block_to_world(Level()->map, 2, 3, &x, &y);
    legoobject_world_position_set(figure, x, y, 0, 1);
    CHECK_NEAR(figure->faceDirection[0], 0.0f, 1e-4);
    CHECK_NEAR(figure->faceDirection[1], 1.0f, 1e-4);

    float north[] = {0, 1};
    legoobject_turn_to_vector_activity_issue(figure, north);
    CHECK_EQ(figure->flags1 & Turning, 0u);
    float east[] = {1, 0};
    legoobject_turn_to_vector_activity_issue(figure, east);
    CHECK_EQ(figure->flags1 & Turning, Turning);
    CHECK_EQ(figure->flags1 & HalfTurn, 0u);
    CHECK_NEAR(figure->faceDirection[0], 1.0f, 1e-4);
    CHECK_NEAR(figure->faceDirection[1], 0.0f, 1e-4);
    for (unsigned i = 0; i < 40 && (figure->flags1 & Turning); ++i) Tick(figure);
    CHECK_EQ(figure->flags1 & Turning, 0u);
    CheckCell(figure, 2, 3);

    float west[] = {-1, 0};
    legoobject_turn_to_vector_activity_issue(figure, west);
    CHECK_EQ(figure->flags1 & (Turning | HalfTurn), Turning | HalfTurn);
    CHECK_NEAR(figure->faceDirection[0], -1.0f, 1e-4);
    for (unsigned i = 0; i < 80 && (figure->flags1 & Turning); ++i) Tick(figure);
    CHECK_EQ(figure->flags1 & (Turning | HalfTurn), 0u);
    CheckCell(figure, 2, 3);
    legoobject_turn_to_vector_activity_issue(figure, north);
    CHECK_EQ(figure->flags1 & Turning, Turning);
    CHECK_NEAR(figure->faceDirection[0], 0.0f, 1e-4);
    CHECK_NEAR(figure->faceDirection[1], 1.0f, 1e-4);
    for (unsigned i = 0; i < 40 && (figure->flags1 & Turning); ++i) Tick(figure);
    CHECK_EQ(figure->flags1 & Turning, 0u);

    // The existing finite displacement takes a clear step on the current
    // cell and reports remaining distance. Reinitialise on the cell
    // centre before trying a direction across the abstract wall.
    legoobject_world_position_set(figure, x, y, 0, 1);
    float move[] = {-5, 0};
    CHECK_EQ(legoobject_pair_scalar_fields_set(figure, move, 15), 1);
    CHECK_EQ(figure->flags2 & Sliding, Sliding);
    legoobject_heading_step_move(figure, 1);
    float actualX, actualY;
    legoobject_transform_position_get(figure, &actualX, &actualY);
    CHECK_NEAR(actualX, x - 5, 1e-4);
    CHECK_NEAR(actualY, y, 1e-4);
    CHECK_NEAR(figure->field_3bc, 10.0f, 1e-4);
    legoobject_world_position_set(figure, x, y, 0, 1);
    float blocked[] = {Level()->blockSize, 0};
    CHECK_EQ(legoobject_pair_scalar_fields_set(figure, blocked, Level()->blockSize), 1);
    legoobject_heading_step_move(figure, 1);
    CHECK_EQ(figure->flags2 & Sliding, 0u);
    legoobject_transform_position_get(figure, &actualX, &actualY);
    CHECK_NEAR(actualX, x, 1e-4);
    CHECK_NEAR(actualY, y, 1e-4);
    CheckCell(figure, 2, 3);
}
} // namespace

int main(int argc, char** argv)
{
    const std::string scenario = argc == 2 ? argv[1] : "training";
    if (!CHECK(scenario == "training" || scenario == "repair-eat" || scenario == "follow-clear" || scenario == "drill" || scenario == "turn-slide", "known scenario"))
        return lrr_test_result();
    const std::string directory = "object_command_states_" + scenario;
    if (!CHECK(make_fixture(directory.c_str()), "generate fixture") ||
        !initialise_headless("ObjectCommandStates") || !boot_game()) return lrr_test_result();
    crt_srand(631);
    if (!CHECK(lego_level_load("Level01"), "load generated terrain") || !LoadCatalog()) return lrr_test_result();
    if (scenario == "training") Training();
    else if (scenario == "repair-eat") RepairAndEat();
    else if (scenario == "follow-clear") FollowAndClear();
    else if (scenario == "drill") Drill();
    else TurningAndSlide();
    return lrr_test_result("object_command_states");
}
