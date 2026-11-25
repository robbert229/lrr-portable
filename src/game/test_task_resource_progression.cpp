// SPDX-License-Identifier: LGPL-3.0-or-later
/* Collection jobs on a generated map with real scene frames, routing and
 * task queues. The synthetic units have no authored model or animations. */
#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "game/animation_object_record.h"
#include "game/aitask.h"
#include "game/aitask_globs.h"
#include "game/aitask_record.h"
#include "game/entity_stats.h"
#include "game/lego_unit.h"
#include "game/legoobject_unit.h"
#include "game/message_unit_link_names.h"
#include "game/message_unit.h"
#include "game/terrain_map.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"
#include "game/lego_globs.h"
#include "crt/vc5names.h"

namespace {

struct Unit {
    AnimationObject model = {};
    Lego_Object object = {};
};

void Place(Lego_Object* object, int x, int y, float offset = 0)
{
    float wx, wy;
    terrain_map_block_to_world(legoGlobs.currLevel->map, x, y, &wx, &wy);
    legoobject_world_position_set(object, wx + offset, wy, 0, 1);
}

bool InitUnit(Unit& unit, StatsData& stats, int x, int y)
{
    unit.model.container = Container_Create(static_cast<Container*>(legoGlobs_rootCont));
    if (!CHECK(unit.model.container != nullptr, "create generic scene frame")) return false;
    unit.object.miniFigure = &unit.model;
    unit.object.stats = &stats;
    unit.object.type = 2;
    unit.object.flags3 = 0x20400060; // Active, AI eligible, can collect.
    unit.object.health = unit.object.energy = 100.0f;
    unit.object.carryNullFrames = 1;
    Place(&unit.object, x, y);
    return true;
}

int FindOre(Lego_Object* object, void* data)
{
    if (object->type != 7) return 0;
    *static_cast<Lego_Object**>(data) = object;
    return 1;
}

unsigned PendingFor(Lego_Object* target)
{
    unsigned count = 0;
    for (AITask* task = aiTaskGlobs.globalHead; task != nullptr; task = task->next)
        if (task->type == 3 && task->object == target) count++;
    return count;
}

void Scenario()
{
    using namespace synthetic_fixture;
    if (!boot_game()) return;
    crt_srand(2468);
    if (!CHECK(lego_level_load("Level01"), "load synthetic level")) return;
    Lego_Object* ore = nullptr;
    legoobject_run_through_lists_skip_upgrade_parts(FindOre, &ore);
    if (!CHECK(ore != nullptr, "initial ore exists")) return;

    // Isolate the job list from the loader's initial terrain jobs.
    aitask_cmd_flush_all(0);
    aitask_cmd_flush_all(1);
    StatsData stats = {};
    stats.flags1 = 0x2000000; // Ordinary dry terrain.
    stats.RouteSpeed = 4.0f;
    stats.RubbleCoef = stats.PathCoef = 1.0f;
    stats.NumOfToolsCanCarry = 2;
    Unit first, second, unreachable;
    if (!InitUnit(first, stats, 1, 2) || !InitUnit(second, stats, 1, 3) ||
        !InitUnit(unreachable, stats, 5, 2)) return;
    auto* a = &first.object;
    auto* b = &second.object;

    // An ore demand is necessary before idle units consider collecting ore.
    // A wrong-id demand is a valid negative control.
    AITask* demand = aitask_cmd_alloc(5);
    demand->field_20 = 7;
    demand->field_24 = 1;
    aitask_cmd_create_task(ore, 0);
    AITask* job = aiTaskGlobs.globalHead;
    if (!CHECK(job && job->object == ore, "collection task queued")) return;
    CHECK_EQ(aitask_place_command_eligibility_check(job, a, nullptr, nullptr, nullptr, 1, 1), 0);
    demand->field_24 = 0;
    CHECK_EQ(aitask_place_command_eligibility_check(job, a, nullptr, nullptr, nullptr, 1, 1), 1);

    // A candidate across the wall cannot reserve the job. Its retry
    // timer expires without losing the original work.
    aiTaskGlobs.assignCandidates[0] = &unreachable.object;
    aiTaskGlobs.assignCandidateCount = 1;
    aitask_pending_command_entity_assignment_tick();
    CHECK_EQ(job->entityRef, nullptr);
    CHECK_EQ(job->timeIn, 75.0f);
    CHECK_EQ(PendingFor(ore), 1u);
    float retryElapsed = 75.0f;
    aitask_cmd_pool_timer_delta_apply(job, &retryElapsed);
    CHECK_EQ(job->timeIn, 0.0f);

    // The nearer candidate wins despite appearing second in the list.
    aiTaskGlobs.assignCandidates[0] = b;
    aiTaskGlobs.assignCandidates[1] = a;
    aiTaskGlobs.assignCandidateCount = 2;
    aitask_pending_command_entity_assignment_tick();
    CHECK_EQ(job->entityRef, a);
    CHECK_EQ(PendingFor(ore), 1u);
    CHECK_EQ(aiTaskGlobs.assignCandidateCount, 1u);
    message_update();
    if (!CHECK(a->aiTask != nullptr, "assignment event creates movement task")) return;
    CHECK_EQ(a->aiTask->type, 0);
    CHECK_EQ(a->aiTask->blockX, 2);
    CHECK_EQ(a->aiTask->blockY, 2);
    CHECK_EQ(b->aiTask, nullptr);

    // Start the approach and interrupt it before arrival: the pending
    // job survives, but the route and reservation must be released.
    float elapsed = 1.0f;
    aitask_cmd_entity_command_chain_tick(a, &elapsed);
    CHECK(a->routeBlocks != nullptr, "first candidate started moving");
    legoobject_path_step_advance_and_arrival_check(a, elapsed);
    legoobject_action_sequence_reset(a, 0);
    CHECK_EQ(a->routeBlocks, nullptr);
    CHECK_EQ(a->aiTask, nullptr);
    CHECK_EQ(job->entityRef, nullptr);
    CHECK_EQ(PendingFor(ore), 1u);
    aiTaskGlobs.assignCandidates[0] = b;
    aiTaskGlobs.assignCandidateCount = 1;
    aitask_pending_command_entity_assignment_tick();
    CHECK_EQ(job->entityRef, b);
    message_update();
    if (!CHECK(b->aiTask != nullptr, "replacement approach queued")) return;
    CHECK_EQ(b->aiTask->type, 0);

    aitask_cmd_entity_command_chain_tick(b, &elapsed);
    CHECK(b->routeBlocks != nullptr, "real approach route");
    for (unsigned i = 0; b->routeBlocks && i < 100; ++i)
        legoobject_path_step_advance_and_arrival_check(b, elapsed);
    CHECK_EQ(b->routeBlocks, nullptr);
    CHECK_EQ(b->aiTask, nullptr);
    int x = -1, y = -1;
    CHECK(legoobject_position_to_terrain_cell(b, &x, &y), "replacement on map");
    CHECK_EQ(x, 2);
    CHECK_EQ(y, 2);

    // The idle tick now claims the original job, not a duplicate.
    aitask_cmd_entity_command_chain_tick(b, &elapsed);
    CHECK_EQ(b->aiTask, job);
    CHECK_EQ(job->entityRef, nullptr);
    CHECK_EQ(PendingFor(ore), 0u);
    aitask_cmd_entity_command_chain_tick(b, &elapsed);
    CHECK_EQ(b->routeToObject, ore);
    CHECK(b->routeBlocks != nullptr, "collection route linked");
    for (unsigned i = 0; b->routeBlocks && i < 100; ++i)
        legoobject_update_callback(b, &elapsed);
    CHECK_EQ(b->routeBlocks, nullptr);
    CHECK(b->flags1 & 0x200, "arrived at collection activity");
    if (!CHECK(b->numCarriedObjects == 1 && b->carriedObjects[0] == ore,
               "replacement collected the ore")) return;
    CHECK_EQ(ore->carryingThisObject, b);
    CHECK_EQ(b->routeToObject, nullptr);
    CHECK(b->flags1 & 0x400, "carrying state");
    CHECK_EQ(PendingFor(ore), 0u);
    CHECK_EQ(ore->carryingIndex, 0u);

    // A full receiver must retain the resource at its current owner.
    b->routeToObject = a;
    a->carryNullFrames = 0;
    legoobject_simulation_queue_flush(b);
    CHECK_EQ(b->numCarriedObjects, 1u);
    CHECK_EQ(b->carriedObjects[0], ore);
    CHECK_EQ(ore->carryingThisObject, b);
    CHECK_EQ(a->numCarriedObjects, 0u);
    a->carryNullFrames = 1;
    legoobject_simulation_queue_flush(b);
    CHECK_EQ(b->numCarriedObjects, 0u);
    CHECK_EQ(b->carriedObjects[0], nullptr);
    CHECK_EQ(b->routeToObject, nullptr);
    CHECK_EQ(b->flags1 & 0x400u, 0u);
    CHECK_EQ(a->numCarriedObjects, 1u);
    CHECK_EQ(a->carriedObjects[0], ore);
    CHECK_EQ(ore->carryingThisObject, a);
    CHECK_EQ(ore->carryingIndex, 0u);

    // Raw ore uses this legacy balance field (lego_upgrade_resource_check(0)).
    const int balance = legoGlobs.currLevel->crystals;
    // A held resource can overlap a wall; a drop there must move it back
    // to its carrier's traversable location and clear both ownership links.
    Place(ore, 3, 2);
    legoobject_attached_object_list_detach(a, 0);
    CHECK_EQ(a->numCarriedObjects, 0u);
    CHECK_EQ(a->carriedObjects[0], nullptr);
    CHECK_EQ(ore->carryingThisObject, nullptr);
    CHECK_EQ(ore->interactObject, nullptr);
    CHECK_EQ(a->flags1 & 0x400u, 0u);
    CHECK(legoobject_position_to_terrain_cell(ore, &x, &y), "dropped resource on map");
    CHECK_EQ(x, 1);
    CHECK_EQ(y, 2);
    CHECK_EQ(legoGlobs.currLevel->crystals, balance);

    // The released resource is available for another collection job.
    Place(a, 1, 2, 4.0f);
    aitask_cmd_create_task(ore, 0.0f);
    CHECK_EQ(PendingFor(ore), 1u);
    aitask_cmd_entity_command_chain_tick(a, &elapsed);
    CHECK_EQ(PendingFor(ore), 0u);
    if (!CHECK(a->aiTask && a->aiTask->object == ore, "recollection attached")) return;
    aitask_cmd_entity_command_chain_tick(a, &elapsed);
    for (unsigned i = 0; a->routeBlocks && i < 100; ++i)
        legoobject_update_callback(a, &elapsed);
    if (!CHECK(a->numCarriedObjects == 1 && a->carriedObjects[0] == ore,
               "released ore collected again")) return;
    CHECK_EQ(ore->carryingThisObject, a);

    // A depot record with the resource-accepting stat represents the
    // receiving endpoint. Complete the existing deposit detach path.
    StatsData depotStats = {};
    depotStats.flags1 = 8;
    Lego_Object depot = {};
    depot.type = 4;
    depot.stats = &depotStats;
    a->routeToObject = &depot;
    aitask_cmd_create_gated(a, &depot);
    CHECK_EQ(aitask_entity_task_chain_contains_kind(a, 5), 1);
    legoobject_attached_object_list_detach(a, 1);
    CHECK_EQ(aitask_entity_task_chain_contains_kind(a, 5), 0);
    CHECK_EQ(a->numCarriedObjects, 0u);
    CHECK_EQ(a->carriedObjects[0], nullptr);
    CHECK_EQ(ore->carryingThisObject, nullptr);
    CHECK_EQ(legoGlobs.currLevel->crystals, balance + 1);
    CHECK_EQ(PendingFor(ore), 0u);
    CHECK(ore->nextFree != ore, "deposited resource is no longer live in the pool");
    legoobject_attached_object_list_detach(a, 1);
    CHECK_EQ(legoGlobs.currLevel->crystals, balance + 1);

    // Complete a synthetic training task at the same command boundary
    // used by the animation-completion branch; preserve prior abilities.
    aitask_entity_task_queue_flush(a);
    AITask* training = aitask_cmd_alloc(0x17);
    training->field_3c = 4;
    training->next = a->aiTask;
    a->aiTask = training;
    a->abilityFlags = 2;
    a->field_3d4 = 100.0f;
    aitask_entity_command_unlink_by_kind(a, 0x17, nullptr, 1);
    CHECK_EQ(a->abilityFlags, 6u);
    CHECK_EQ(a->field_3d4, 0.0f);
    CHECK_EQ(aitask_entity_task_chain_contains_kind(a, 0x17), 0);
    aitask_entity_command_unlink_by_kind(a, 0x17, nullptr, 1);
    CHECK_EQ(a->abilityFlags, 6u);
    aitask_entity_drain_command_chain(a);
    aitask_entity_drain_command_chain(b);
    aitask_cmd_free(demand, 1);
    for (Unit* unit : {&first, &second, &unreachable})
        Container_Remove(static_cast<Container*>(unit->model.container));
}

} // namespace

int main()
{
    using namespace synthetic_fixture;
    if (CHECK(make_fixture("task_resource_progression_fixture"), "write synthetic fixture") &&
        CHECK(write_text("Data/L/empty.ptl", "Lego* { CollectCrystal Goto }\n"), "write test event translation") &&
        initialise_headless("TaskResourceProgression"))
        Scenario();
    return lrr_test_result("task_resource_progression");
}
