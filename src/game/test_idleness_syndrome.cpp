// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_idleness_syndrome — AITask.c's job assignment (00405c10) and idle
 * unit update (00404280) with tweaks:fix_idleness_syndrome off (the
 * original) and on (docs/divergences.md, "Idle raiders take the jobs
 * waiting for them").
 *
 * The jobs are Clear jobs (type 0xc): a unit must stand on the job's cell,
 * so a unit elsewhere is sent there (Message_Clear, which the level's .ptl
 * turns into a Goto) and becomes the job's unit on its way (entityRef).
 *
 * The test links with --wrap for the calls AITask.c makes into the units
 * that hold the map, the objects and the messages: units stand at fixed
 * cells, a route exists unless the unit is marked unreachable, and the
 * messages are recorded. */
#include <stdint.h>
#include <stdlib.h>

#include "game/aitask.h"
#include "game/aitask_globs.h"
#include "game/aitask_record.h"
#include "game/legoobject_record.h"
#include "test/lrr_test.h"

namespace {

bool s_fix = false;

struct Unit {
    Lego_Object obj;
    int cellX;
    int cellY;
    bool unreachable;
};

Unit s_units[4];

Unit* UnitOf(const void* obj)
{
    for (Unit& unit : s_units) {
        if (&unit.obj == obj) return &unit;
    }
    return nullptr;
}

struct Produced {
    int op;
    const void* unit;
    uintptr_t task;
    int cell[2];
};

Produced s_produced[8];
unsigned int s_producedCount = 0;

int s_map; // The map handle AITask.c passes back to the Map3D.c stubs.

} // namespace

extern "C" {

bool __wrap__Z26LRR_GetFixIdlenessSyndromev(void)
{
    return s_fix;
}

void* __wrap_level_current_terrain_get(void)
{
    return &s_map;
}

unsigned int __wrap_selection_contains_entity_active_target(Lego_Object*, unsigned int*)
{
    return 0;
}

int __wrap_lego_entity_matches_current_selection_or_owner(const void*)
{
    return 0;
}

/* Stats.c: every unit can clear rubble (0x10000). */
unsigned int __wrap_entity_stats_word_get(const Lego_Object*)
{
    return 0x10000;
}

/* LegoObject.c's command gate: every unit has the tool. */
int __wrap_legoobject_type_two_target_resolve_and_apply(Lego_Object*, int)
{
    return 1;
}

void __wrap_legoobject_transform_position_get(Lego_Object* obj, float* outX, float* outY)
{
    Unit* unit = UnitOf(obj);
    *outX = unit->cellX + 0.5f;
    *outY = unit->cellY + 0.5f;
}

int __wrap_legoobject_position_to_terrain_cell(Lego_Object* obj, int* outX, int* outY)
{
    Unit* unit = UnitOf(obj);
    *outX = unit->cellX;
    *outY = unit->cellY;
    return 1;
}

void __wrap_legoobject_surface_vector_normalize(Lego_Object*, float* outDir)
{
    outDir[0] = 1.0f;
    outDir[1] = 0.0f;
}

int __wrap_legoobject_adjacent_position_candidate_find(Lego_Object*, int*, int*)
{
    return 0;
}

/* One map cell is one world unit. */
int __wrap_terrain_map_block_to_world(void*, unsigned int bx, unsigned int by, float* x, float* y)
{
    *x = bx + 0.5f;
    *y = by + 0.5f;
    return 1;
}

int __wrap_terrain_map_world_to_block(void*, float x, float y, int* bx, int* by)
{
    *bx = (int)x;
    *by = (int)y;
    return 1;
}

int __wrap_legoobject_grid_route_search_adapter(Lego_Object* obj, unsigned int, unsigned int,
                                                unsigned int, unsigned int, unsigned int** outXs,
                                                unsigned int** outYs, unsigned int* outCount)
{
    if (UnitOf(obj)->unreachable) return 0;
    *outXs = (unsigned int*)malloc(sizeof(unsigned int));
    *outYs = (unsigned int*)malloc(sizeof(unsigned int));
    *outCount = 1;
    return 1;
}

void __wrap_cmd_produce(int op, const void* a1, uintptr_t a2, const int* pair)
{
    if (s_producedCount < sizeof(s_produced) / sizeof(s_produced[0])) {
        Produced& p = s_produced[s_producedCount++];
        p.op = op;
        p.unit = a1;
        p.task = a2;
        p.cell[0] = pair[0];
        p.cell[1] = pair[1];
    }
}

void __wrap_idle_register(Lego_Object*) {}

} // extern "C"

namespace {

constexpr int kMessageClear = 0x27;

/* A Clear job on (x, y). */
AITask ClearJob(int x, int y)
{
    AITask task = {};
    task.type = 0xc;
    task.blockX = x;
    task.blockY = y;
    return task;
}

void Reset()
{
    for (Unit& unit : s_units) unit = Unit{};
    for (Unit& unit : s_units) {
        unit.obj.type = 2;
        unit.obj.health = 100.0f;
        unit.obj.flags3 = 0x400000;
    }
    s_producedCount = 0;
    aiTaskGlobs.globalHead = nullptr;
    aiTaskGlobs.assignCandidateCount = 0;
}

void SetCandidates(std::initializer_list<Unit*> units)
{
    aiTaskGlobs.assignCandidateCount = 0;
    for (Unit* unit : units)
        aiTaskGlobs.assignCandidates[aiTaskGlobs.assignCandidateCount++] = &unit->obj;
}

/* A job whose unit is on its way still takes the nearest idle unit. The
 * original gives that unit nothing and keeps it from the next job. */
void JobWithUnitOnItsWay(bool fix)
{
    Reset();
    s_fix = fix;
    Unit& walker = s_units[0];
    Unit& idle = s_units[1];
    idle.cellX = 3;
    idle.cellY = 3;

    AITask busy = ClearJob(8, 8);
    busy.entityRef = &walker.obj;
    AITask waiting = ClearJob(2, 2);
    aiTaskGlobs.globalHead = &busy;
    busy.next = &waiting;
    SetCandidates({ &idle });

    aitask_pending_command_entity_assignment_tick();

    CHECK_EQ(busy.entityRef, (void*)&walker.obj);
    if (fix) {
        CHECK_EQ(s_producedCount, 1u);
        CHECK_EQ(s_produced[0].op, kMessageClear);
        CHECK_EQ(s_produced[0].unit, (const void*)&idle.obj);
        CHECK_EQ(s_produced[0].task, (uintptr_t)&waiting);
        CHECK_EQ(s_produced[0].cell[0], 2);
        CHECK_EQ(s_produced[0].cell[1], 2);
        CHECK_EQ(waiting.entityRef, (void*)&idle.obj);
        CHECK_EQ(aiTaskGlobs.assignCandidateCount, 0u);
    } else {
        CHECK_EQ(s_producedCount, 0u);
        CHECK_EQ(waiting.entityRef, (void*)nullptr);
        CHECK_EQ(aiTaskGlobs.assignCandidateCount, 0u);
    }
}

/* A unit that cannot reach a job, ahead of one that can. The original
 * pauses the job at the first, which turns the second away too. */
void UnreachableUnitFirst(bool fix)
{
    Reset();
    s_fix = fix;
    Unit& trapped = s_units[0];
    Unit& idle = s_units[1];
    trapped.cellX = 9;
    trapped.cellY = 9;
    trapped.unreachable = true;
    idle.cellX = 3;
    idle.cellY = 3;

    AITask job = ClearJob(2, 2);
    aiTaskGlobs.globalHead = &job;
    SetCandidates({ &trapped, &idle });

    aitask_pending_command_entity_assignment_tick();

    if (fix) {
        CHECK_EQ(s_producedCount, 1u);
        CHECK_EQ(s_produced[0].unit, (const void*)&idle.obj);
        CHECK_EQ(job.entityRef, (void*)&idle.obj);
        CHECK_EQ(job.timeIn, 0.0f);
    } else {
        CHECK_EQ(s_producedCount, 0u);
        CHECK_EQ(job.entityRef, (void*)nullptr);
        CHECK_EQ(job.timeIn, 75.0f);
    }
}

/* No unit can reach the job: both pause it for 75 ticks. */
void NoUnitCanReach(bool fix)
{
    Reset();
    s_fix = fix;
    Unit& trapped = s_units[0];
    trapped.cellX = 9;
    trapped.cellY = 9;
    trapped.unreachable = true;

    AITask job = ClearJob(2, 2);
    aiTaskGlobs.globalHead = &job;
    SetCandidates({ &trapped });

    aitask_pending_command_entity_assignment_tick();

    CHECK_EQ(s_producedCount, 0u);
    CHECK_EQ(job.timeIn, 75.0f);
}

/* A unit on its way that is idle again (its Goto failed): the original
 * keeps it as the job's unit on its way, so the job waits for it. */
void IdleUnitOnItsWay(bool fix)
{
    Reset();
    s_fix = fix;
    Unit& walker = s_units[0];
    walker.cellX = 5;
    walker.cellY = 5;

    AITask job = ClearJob(2, 2);
    job.entityRef = &walker.obj;
    job.timeIn = 10.0f; // Paused, so the unit does not take it here.
    aiTaskGlobs.globalHead = &job;

    float elapsed = 1.0f;
    aitask_cmd_entity_command_chain_tick(&walker.obj, &elapsed);

    CHECK_EQ(job.entityRef, fix ? (void*)nullptr : (void*)&walker.obj);
    CHECK_EQ(walker.obj.aiTask, (AITask*)nullptr);
    CHECK_EQ(aiTaskGlobs.assignCandidateCount, 1u);
}

constexpr unsigned int kPlayerRequested = 0x10000;

/* An idle unit and a paused Clear job (paused by a route failure). With
 * the unit at (unitX, unitY), returns whether the unit took the job. */
bool IdleUnitTakesPausedJob(bool fix, unsigned int jobFlags, int unitX, int unitY)
{
    Reset();
    s_fix = fix;
    Unit& raider = s_units[0];
    raider.cellX = unitX;
    raider.cellY = unitY;

    static AITask job;
    job = ClearJob(2, 2);
    job.flags = jobFlags;
    job.timeIn = 75.0f;
    aiTaskGlobs.globalHead = &job;

    float elapsed = 1.0f;
    aitask_cmd_entity_command_chain_tick(&raider.obj, &elapsed);

    CHECK_EQ(job.timeIn, 75.0f);
    bool took = raider.obj.aiTask == &job;
    if (took) {
        CHECK_EQ(aiTaskGlobs.globalHead, (AITask*)nullptr);
        CHECK_EQ(aiTaskGlobs.assignCandidateCount, 0u);
    } else {
        CHECK_EQ(raider.obj.aiTask, (AITask*)nullptr);
        CHECK_EQ(aiTaskGlobs.globalHead, &job);
        CHECK_EQ(aiTaskGlobs.assignCandidateCount, 1u);
    }
    raider.obj.aiTask = nullptr;
    return took;
}

/* The player sent the unit to a job that is paused. The original turns the
 * unit away when it gets there, so it takes other work. */
void PlayerJobPaused(bool fix)
{
    CHECK_EQ(IdleUnitTakesPausedJob(fix, kPlayerRequested, 2, 2), fix);
    // Not at the job: the pause still keeps the unit from it.
    CHECK(!IdleUnitTakesPausedJob(fix, kPlayerRequested, 5, 5), "player job, unit elsewhere");
    // A job the player did not ask for keeps its pause.
    CHECK(!IdleUnitTakesPausedJob(fix, 0, 2, 2), "unrequested job, unit at it");
}

} // namespace

int main(void)
{
    for (bool fix : { false, true }) {
        JobWithUnitOnItsWay(fix);
        UnreachableUnitFirst(fix);
        NoUnitCanReach(fix);
        IdleUnitOnItsWay(fix);
        PlayerJobPaused(fix);
    }
    Reset();
    return lrr_test_result("idleness_syndrome");
}
