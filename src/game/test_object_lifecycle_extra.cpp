// SPDX-License-Identifier: LGPL-3.0-or-later
/* Real object motion, spawn/removal and web interactions on generated terrain. */
#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/config.h"
#include "game/animation_object_record.h"
#include "game/vehicle_model_record.h"
#include "game/legoobject_unit.h"
#include "game/lego_unit.h"
#include "game/spiderweb.h"
#include "game/terrain_map.h"
#include "game/entity_stats.h"
#include "game/panel_unit.h"
#include "game/lego_globs.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"

namespace {
constexpr unsigned Floor = 8, Wall = 16;
constexpr unsigned Stuck = 0x2000000, Removing = 0x800000;

void check_cell(Lego_Object* object, int x, int y)
{
    int actualX = -1, actualY = -1;
    CHECK_EQ(legoobject_position_to_terrain_cell(object, &actualX, &actualY), 1);
    CHECK_EQ(actualX, x);
    CHECK_EQ(actualY, y);
}

void place(Lego_Object* object, int x, int y)
{
    float wx, wy;
    terrain_map_block_to_world(legoGlobs.currLevel->map, x, y, &wx, &wy);
    legoobject_world_position_set(object, wx, wy, 0, 1);
    check_cell(object, x, y);
}

void move_north(Lego_Object* object)
{
    object->field_2c0 = 1;
    float animationDelta = 0;
    CHECK_EQ(legoobject_movement_accumulator_apply(object, 1, &animationDelta), 1);
    CHECK_EQ(object->field_2c0, 0);
    CHECK(animationDelta > 0, "movement consumes input and advances animation time");
}

int count_type(Lego_Object* object, void* data)
{
    if (object->type == 3) ++*static_cast<unsigned*>(data);
    return 0;
}

unsigned creatures()
{
    unsigned count = 0;
    legoobject_run_through_lists_skip_upgrade_parts(count_type, &count);
    return count;
}
} // namespace

// The existing weapon loader supplies a made-up web damage rate.
extern "C" int weapons_initialise(BlockRec*, const char*);

int main()
{
    using namespace synthetic_fixture;
    if (!CHECK(make_fixture("object_lifecycle_extra_fixture"), "generate artificial inputs") ||
        !initialise_headless("ObjectLifecycleExtra") || !boot_game() ||
        !CHECK(lego_level_load("Level01"), "load generated level")) return lrr_test_result();
    constexpr char weaponConfig[] =
        "Lego { WeaponTypes { SpiderWeb { DefaultDamage 25 } } }";
    BlockFile* weapons = block_parse(weaponConfig, sizeof(weaponConfig) - 1);
    if (!CHECK(weapons != nullptr, "parse synthetic web damage") ||
        !CHECK_EQ(weapons_initialise(weapons, "Lego"), 1)) return lrr_test_result();
    auto* level = legoGlobs.currLevel;
    auto* map = level->map;
    auto cell = [level](int x, int y) -> Lego_Block& { return level->blocks[y * level->width + x]; };
    auto* root = static_cast<Container*>(legoGlobs_rootCont);
    Container* prototype = Container_Create(root);
    Container* chassis = Container_Create(root);
    if (!CHECK(prototype && chassis, "generic scene frames")) return lrr_test_result();

    StatsData mobile = {};
    mobile.RouteSpeed = map->blockSize;
    mobile.RubbleCoef = mobile.PathCoef = 1;
    mobile.flags1 = 0x2000000; // Cross ordinary dry terrain.
    mobile.CollRadius = map->blockSize * 0.2f;

    // A vehicle really moves its scene frame and snaps to the terrain.
    // Failed entry into lava must restore its previous transform.
    VehicleModel vehicleModel = {};
    vehicleModel.contAct1 = chassis;
    Lego_Object vehicle = {};
    vehicle.type = 1;
    vehicle.vehicle = &vehicleModel;
    vehicle.stats = &mobile;
    vehicle.health = 100;
    for (int y = 1; y <= 4; ++y) {
        cell(1, y).flags1 = Floor;
        cell(1, y).flags2 = 0;
        cell(1, y).f_02 = 0;
    }
    place(&vehicle, 1, 4);
    float idleDelta = -1;
    CHECK_EQ(legoobject_movement_accumulator_apply(&vehicle, 1, &idleDelta), 0);
    CHECK_EQ(idleDelta, -1.0f);
    check_cell(&vehicle, 1, 4);
    move_north(&vehicle);
    check_cell(&vehicle, 1, 3);
    float stopped[3], pos[3];
    Container_GetPosition(chassis, root, stopped);
    cell(1, 2).f_02 = 6;
    move_north(&vehicle);
    check_cell(&vehicle, 1, 3);
    Container_GetPosition(chassis, root, pos);
    for (unsigned i = 0; i < 3; ++i) CHECK_NEAR(pos[i], stopped[i], 1e-4);
    cell(1, 2).f_02 = 9; // Water cannot be crossed by this dry-land vehicle.
    move_north(&vehicle);
    check_cell(&vehicle, 1, 3);
    cell(1, 2).f_02 = 0;
    move_north(&vehicle);
    check_cell(&vehicle, 1, 2);
    Container_GetPosition(chassis, root, pos);
    CHECK_NEAR(pos[2], terrain_map_world_z_get(map, pos[0], pos[1]), 1e-4);

    // Spawn a creature through the real object pool using an empty frame.
    Stats_Globs* stats = entity_stats_port_globs();
    StatsData* creatureStats[STATS_OBJECT_IDS] = {};
    StatsData creatureMobile = mobile;
    creatureStats[0] = &creatureMobile;
    auto* oldCreatureStats = stats->objectStats[3];
    const unsigned oldCreatureLevels = stats->objectLevels[3][0];
    stats->objectStats[3] = creatureStats;
    stats->objectLevels[3][0] = 1;
    AnimationObject creatureModel = {};
    creatureModel.container = prototype;
    unsigned before = creatures();
    float x, y;
    terrain_map_block_to_world(map, 1, 4, &x, &y);
    Lego_Object* creature = legoobject_create_in_world(&creatureModel, 3, 0, 0, x, y, 0);
    if (!CHECK(creature != nullptr, "create creature")) return lrr_test_result();
    CHECK_EQ(creatures(), before + 1);
    CHECK_EQ(creature->health, 100.0f);
    legoobject_objective_sidecar_append_gate(creature);
    CHECK_EQ(creatures(), before + 1); // A healthy creature is not removed.
    check_cell(creature, 1, 4);
    move_north(creature);
    check_cell(creature, 1, 3);

    // The same lava that blocked the vehicle is traversable only when
    // the creature is explicitly allowed to enter it.
    cell(1, 2).f_02 = 6;
    move_north(creature);
    check_cell(creature, 1, 3);
    creatureMobile.flags2 |= 0x200000;
    move_north(creature);
    check_cell(creature, 1, 2);
    creatureMobile.flags2 &= ~0x200000u;
    cell(1, 2).f_02 = 0;

    // A web needs walls on opposite sides. A real geometric collision
    // traps the creature, while removal of a supporting wall releases it.
    cell(1, 3).flags1 = Floor;
    cell(0, 3).flags1 = Wall;
    cell(2, 3).flags1 = Floor;
    auto* oldWebPrototype = legoGlobs_contSpiderWeb;
    legoGlobs_contSpiderWeb = prototype;
    auto& webCell = spiderWebGlobs.webBlocks[3 * level->width + 1];
    CHECK_EQ(spiderweb_spawn_at(1, 3), 0);
    CHECK_EQ(webCell.webObj, nullptr);
    cell(2, 3).flags1 = Wall;
    CHECK_EQ(spiderweb_spawn_at(1, 3), 1);
    Lego_Object* web = webCell.webObj;
    if (!CHECK(web != nullptr, "web allocated")) return lrr_test_result();
    CHECK_EQ(web->type, 12);
    CHECK_EQ(webCell.health, 100.0f);
    CHECK_EQ(spiderweb_spawn_at(1, 3), 0);
    CHECK_EQ(webCell.webObj, web);
    check_cell(web, 1, 3);
    CHECK_EQ(spiderweb_check_collision(creature), 0);
    move_north(creature); // Move away from the web's cell.
    check_cell(creature, 1, 1);
    place(creature, 1, 3);
    // A point on the same block but outside the narrow web collision band.
    terrain_map_block_to_world(map, 1, 3, &x, &y);
    legoobject_world_position_set(creature, x, y + map->blockSize * 0.4f, 0, 0);
    check_cell(creature, 1, 3);
    CHECK_EQ(spiderweb_check_collision(creature), 0);
    CHECK_EQ(creature->flags1 & Stuck, 0u);
    place(creature, 1, 3);
    CHECK_EQ(spiderweb_check_collision(creature), 1);
    CHECK(creature->flags1 & Stuck, "collision traps creature");
    float elapsed = 2;
    spiderweb_callback_update(creature, &elapsed);
    CHECK_EQ(webCell.health, 50.0f);
    cell(2, 3).flags1 = Floor;
    spiderweb_callback_update(web, &elapsed);
    CHECK(web->flags3 & Removing, "unsupported web is removed");
    spiderweb_callback_update(creature, &elapsed);
    CHECK_EQ(creature->flags1 & Stuck, 0u);

    CHECK_EQ(webCell.flags & SPIDERWEB_FLAG_WEB, 0u);
    legoobject_remove(web);
    // Re-spin in the perpendicular direction. At exactly zero health
    // the web is still present; only the next tick breaks it.
    cell(0, 3).flags1 = Floor;
    cell(1, 2).flags1 = cell(1, 4).flags1 = Wall;
    CHECK_EQ(spiderweb_spawn_at(1, 3), 1);
    web = webCell.webObj;
    if (!CHECK(web != nullptr, "replacement web allocated")) return lrr_test_result();
    CHECK_EQ(webCell.health, 100.0f);
    CHECK_EQ(spiderweb_check_collision(creature), 1);
    elapsed = 1;
    for (unsigned tick = 1; tick <= 4; ++tick) {
        spiderweb_callback_update(creature, &elapsed);
        CHECK_EQ(webCell.health, 100.0f - 25.0f * tick);
        CHECK(webCell.flags & SPIDERWEB_FLAG_WEB, "web survives through zero health");
        CHECK(creature->flags1 & Stuck, "creature remains trapped");
    }
    spiderweb_callback_update(creature, &elapsed);
    CHECK_EQ(creature->flags1 & Stuck, 0u);
    CHECK_EQ(webCell.flags & SPIDERWEB_FLAG_WEB, 0u);
    CHECK(web->flags3 & Removing, "torn web marked for removal");
    legoobject_remove(web);
    spiderweb_restart(level);
    legoGlobs_contSpiderWeb = oldWebPrototype;

    legoobject_damage_apply(creature, 101, 0, 1);
    CHECK_EQ(creature->health, -1.0f);
    CHECK_EQ(creatures(), before + 1); // Damage alone does not release the pool slot.
    legoobject_detach_carried_object_and_reset_state(creature);
    legoobject_objective_sidecar_append_gate(creature);
    CHECK_EQ(creatures(), before);
    // A subsequent spawn has fresh state rather than inheriting damage.
    creature = legoobject_create_in_world(&creatureModel, 3, 0, 0, x, y, 0);
    if (!CHECK(creature != nullptr, "spawn after removal")) return lrr_test_result();
    CHECK_EQ(creatures(), before + 1);
    CHECK_EQ(creature->health, 100.0f);
    CHECK_EQ(creature->flags3 & Removing, 0u);
    CHECK_EQ(creature->flags1 & Stuck, 0u);
    legoobject_remove(creature);
    CHECK_EQ(creatures(), before);
    stats->objectStats[3] = oldCreatureStats;
    stats->objectLevels[3][0] = oldCreatureLevels;
    Container_Remove(chassis);
    Container_Remove(prototype);
    // The level teardown needs the same empty top-panel controls as the
    // shared synthetic lifecycle test.
    const unsigned types[] = {0, 1, 2};
    const float areas[12] = {};
    const int tips[3] = {};
    panel_texture_slots_allocate(PANEL_TOP, 3, types, areas, nullptr, nullptr, nullptr, tips);
    lego_level_free();
    CHECK(legoGlobs.currLevel == nullptr, "level unloaded");
    block_close(weapons);
    return lrr_test_result();
}
