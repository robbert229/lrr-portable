// SPDX-License-Identifier: LGPL-3.0-or-later
/* Synthetic resource deposits and admission rules, with no scene assets. */
#include "game/camera_control.h"
#include "game/entity_stats.h"
#include "game/lego_unit.h"
#include "game/legoobject_unit.h"
#include "game/lego_globs.h"
#include "test/lrr_test.h"

namespace {

void CheckDiscovery()
{
    Lego_Level level = {};
    Lego_Block blocks[9] = {};
    level.width = level.height = 3;
    level.blocks = blocks;
    legoGlobs.currLevel = &level;
    const int cell[] = {1, 1};
    const unsigned int quantities[] = {1, 3, 5, 10, 25};
    for (unsigned int code = 0; code <= 20; ++code) {
        blocks[4].f_05 = code;
        unsigned int crystal = 99, upgradedCrystal = 99, ore = 99, upgradedOre = 99;
        CHECK_EQ(lego_terrain_cell_byte_state_classify_current(
            cell, &crystal, &upgradedCrystal, &ore, &upgradedOre), code != 0);
        const unsigned int expected = code ? quantities[(code - 1) / 4] : 0;
        CHECK_EQ(crystal, code && (code - 1) % 4 == 0 ? expected : 0u);
        CHECK_EQ(ore, code && (code - 1) % 4 == 1 ? expected : 0u);
        CHECK_EQ(upgradedCrystal, code && (code - 1) % 4 == 2 ? expected : 0u);
        CHECK_EQ(upgradedOre, code && (code - 1) % 4 == 3 ? expected : 0u);
        CHECK_EQ(blocks[4].f_05, code);
    }
    legoGlobs.currLevel = nullptr;
}

void CheckAdmission()
{
    StatsData stats = {};
    Lego_Object carrier = {}, crystal = {}, ore = {};
    carrier.type = 2;
    carrier.stats = &stats;
    crystal.type = 6;
    ore.type = 7;
    CHECK_EQ(legoobject_accepts_target_for_task_check(&carrier, &crystal, 0), 0);
    carrier.flags3 = 0x20;
    for (unsigned int mask : {0u, 2u, 4u}) {
        stats.flags1 = mask;
        CHECK_EQ(legoobject_accepts_target_for_task_check(&carrier, &crystal, 0), mask != 2);
        CHECK_EQ(legoobject_accepts_target_for_task_check(&carrier, &ore, 0), mask != 4);
    }
    stats.flags1 = 0;
    carrier.carriedObjects[0] = &crystal;
    carrier.numCarriedObjects = 1;
    CHECK_EQ(legoobject_accepts_target_for_task_check(&carrier, &crystal, 0), 1);
    CHECK_EQ(legoobject_accepts_target_for_task_check(&carrier, &ore, 0), 0);
    carrier.flags1 = 1;
    CHECK_EQ(legoobject_accepts_target_for_task_check(&carrier, &crystal, 0), 0);
    carrier.flags1 = 0;
    CHECK_EQ(legoobject_deposit_activity_gate(&carrier, nullptr), 0);
    CHECK_EQ(carrier.carriedObjects[0], &crystal);
    CHECK_EQ(carrier.numCarriedObjects, 1u);
}

void CheckDelivery()
{
    Lego_Level level = {};
    legoGlobs.currLevel = &level;
    StatsData stats = {};
    LegoCamera camera = {};
    legoGlobs.cameraTrack = &camera;
    Lego_Object depot = {};
    Lego_Object& resource = *legoobject_pool_allocate();
    depot.type = 4;
    depot.stats = &stats;
    resource.stats = &stats;
    resource.type = 6;
    resource.carryingThisObject = &depot;
    legoobject_action_record_consume_and_cleanup(&depot, &resource);
    CHECK_EQ(level.f_08c, 1);
    CHECK_EQ(resource.carryingThisObject, nullptr);
    CHECK_EQ(legoobject_pool_allocate(), &resource);
    resource.stats = &stats;
    resource.type = 7;
    resource.id = 1;
    resource.carryingThisObject = &depot;
    legoobject_action_record_consume_and_cleanup(&depot, &resource);
    CHECK_EQ(level.ore, 1);
    CHECK_EQ(level.crystals, 0);
    CHECK_EQ(level.f_08c, 1);
    CHECK_EQ(resource.carryingThisObject, nullptr);
    legoobject_shutdown();
    legoGlobs.cameraTrack = nullptr;
    legoGlobs.currLevel = nullptr;
}

} // namespace

int main()
{
    CheckDiscovery();
    CheckAdmission();
    CheckDelivery();
    return lrr_test_result("mining_logistics");
}
