// SPDX-License-Identifier: LGPL-3.0-or-later
#include "game/legoobject_unit.h"
#include "game/lego_globs.h"
#include "game/lego_level_record.h"
#include "game/terrain_map.h"
#include "test/lrr_test.h"

extern "C" void* __wrap_scene_object_resource_get(const void*) { return nullptr; }
extern "C" void __wrap_ddiscene_object_position_get(void*, void*, float* pos)
{
    pos[0] = 2.5f;
    pos[1] = -2.5f;
    pos[2] = 0;
}
extern "C" unsigned __wrap_entity_capability_flags_get(const Lego_Object*) { return 0x10; }
extern "C" unsigned __wrap_entity_state_flags_get(const Lego_Object*) { return 0; }
static unsigned hits;
extern "C" void __wrap_flagged_object_timer_registry_insert(Lego_Object*) { ++hits; }

int main()
{
    Lego_Block cells[25] = {};
    Lego_Level level = {};
    level.width = level.height = 5;
    level.blocks = cells;
    TerrainMap map = {};
    map.gridWidth = map.gridHeight = 4;
    map.blockSize = 1;
    level.map = &map;
    legoGlobs.currLevel = &level;
    Lego_Object vehicle = {};
    vehicle.type = 1;
    int out[] = {-1, -1};
    cells[7].flags1 = cells[13].flags1 = 8;
    cells[7].f_02 = 6; // Lava blocks the preferred north exit.
    CHECK_EQ(legoobject_vehicle_unload_cell_find_if_eligible(&vehicle, out), 1);
    CHECK_EQ(out[0], 3);
    CHECK_EQ(out[1], 2);
    cells[13].flags2 = 4; // Rubble closes the east exit too.
    CHECK_EQ(legoobject_vehicle_unload_cell_find_if_eligible(&vehicle, out), 0);
    cells[7].f_02 = 0;
    CHECK_EQ(legoobject_vehicle_unload_cell_find_if_eligible(&vehicle, out), 1);
    CHECK_EQ(out[0], 2);
    CHECK_EQ(out[1], 1);

    Lego_Object monster = {}, crystal = {};
    monster.type = 3;
    monster.health = 40;
    monster.flags1 = 0x400;
    monster.carriedObjects[0] = &crystal;
    monster.numCarriedObjects = 1;
    crystal.type = 6;
    crystal.health = 100;
    crystal.carryingThisObject = &monster;
    legoobject_damage_apply(&crystal, 20, 1, 1);
    CHECK_EQ(crystal.health, 100.0f);
    CHECK_EQ(hits, 0u);
    legoobject_damage_apply(&monster, 15, 1, 1);
    CHECK_EQ(monster.health, 25.0f);
    CHECK_EQ(hits, 1u);
    CHECK_EQ(monster.carriedObjects[0], &crystal);
    legoobject_damage_apply(&monster, 26, 1, 1);
    CHECK_EQ(monster.health, -1.0f);
    CHECK_EQ(hits, 2u);
    legoobject_damage_apply(&monster, 10, 1, 1);
    CHECK_EQ(hits, 2u);
    legoobject_detach_carried_object_and_reset_state(&monster);
    CHECK_EQ(monster.numCarriedObjects, 0u);
    CHECK_EQ(monster.carriedObjects[0], nullptr);
    CHECK_EQ(crystal.carryingThisObject, nullptr);
    CHECK_EQ(crystal.health, -1.0f);
    CHECK(monster.flags3 & 0x800000, "monster scheduled for removal");
    legoGlobs.currLevel = nullptr;
    return lrr_test_result();
}
