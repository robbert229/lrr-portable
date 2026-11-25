// SPDX-License-Identifier: LGPL-3.0-or-later
/* Raider inventory and health transitions using independent stats records. */
#include <miniwin/d3drm.h>
#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/main_globs.h"
#include "game/animation_object_record.h"
#include "game/terrain_map.h"
#include "game/entity_stats_link_names.h"
#include "game/entity_stats.h"
#include "game/legoobject_unit.h"
#include "game/lego_globs.h"
#include "test/lrr_test.h"

extern "C" {
void legoobject_activity_grant(Lego_Object*, int);
int legoobject_activity_scan(Lego_Object*, int);
int legoobject_type_two_target_resolve_and_apply(Lego_Object*, int);
}

namespace {
void CheckDecay()
{
    IDirect3DRM* rm = nullptr;
    IDirect3DRM3* rm3 = nullptr;
    if (!CHECK_EQ(Direct3DRMCreate(&rm), DD_OK)) return;
    CHECK_EQ(rm->QueryInterface(IID_IDirect3DRM3, reinterpret_cast<void**>(&rm3)), DD_OK);
    rm->Release();
    if (!rm3) return;
    s_main_d3drm = rm3;
    Container* root = Container_Initialise("SyntheticRaider");
    Container* child = Container_Create(root);
    if (CHECK(child != nullptr, "create raider frame")) {
        Lego_Level level = {};
        Lego_Block blocks[16] = {};
        TerrainMap map = {};
        level.width = level.height = 4;
        level.blocks = blocks;
        level.map = &map;
        map.gridWidth = map.gridHeight = 3;
        map.blockWidth = map.blockHeight = 4;
        map.blockSize = 1.0f;
        legoGlobs.currLevel = &level;
        Container_SetPosition(child, root, 1.5f, -1.5f, 0.0f);
        AnimationObject model = {};
        model.container = child;
        StatsData stats = {};
        stats.HealthDecayRate = 5.0f;
        stats.EnergyDecayRate = 10.0f;
        Lego_Object raider = {};
        raider.type = 2;
        raider.stats = &stats;
        raider.miniFigure = &model;
        raider.health = 100.0f;
        raider.energy = 80.0f;
        int x = -1, y = -1;
        CHECK_EQ(legoobject_position_to_terrain_cell(&raider, &x, &y), 1);
        CHECK_EQ(x, 1);
        CHECK_EQ(y, 1);
        legoobject_energy_low_target_route_command_tick(&raider, 25.0f);
        CHECK_NEAR(raider.health, 95.0f, 1e-5);
        CHECK_NEAR(raider.energy, 70.0f, 1e-5);
        raider.flags3 |= 0x80000000u;
        legoobject_energy_low_target_route_command_tick(&raider, 25.0f);
        CHECK_NEAR(raider.health, 95.0f, 1e-5);
        CHECK_NEAR(raider.energy, 70.0f, 1e-5);
        raider.energy = 100.0f;
        legoobject_energy_low_target_route_command_tick(&raider, 1.0f);
        CHECK_EQ(raider.energy, raider.health);
        unsigned int xs[] = {1, 2, 3};
        unsigned int ys[] = {1, 1, 1};
        raider.flags3 = 0;
        CHECK_EQ(legoobject_route_setup_and_marker_associate(&raider, 3, xs, ys, nullptr), 1);
        CHECK_EQ(raider.routeBlocksTotal, 3u);
        CHECK_EQ(raider.routeBlocksCurrent, 0u);
        CHECK_EQ(raider.flags1 & 1u, 1u);
        if (CHECK(raider.routeBlocks != nullptr, "route allocated")) {
            const auto* route = static_cast<const LegoObjG_RouteBlock*>(raider.routeBlocks);
            for (unsigned int i = 0; i < 3; ++i) {
                CHECK_EQ(route[i].blockPos[0], xs[i]);
                CHECK_EQ(route[i].blockPos[1], ys[i]);
                CHECK_EQ(route[i].blockOffset[0], 0.5f);
                CHECK_EQ(route[i].blockOffset[1], 0.5f);
            }
        }
        legoobject_action_sequence_reset(&raider, 0);
        CHECK_EQ(raider.routeBlocks, nullptr);
        CHECK_EQ(raider.routeBlocksTotal, 0u);
        CHECK_EQ(raider.flags1 & 1u, 0u);
        xs[1] = 3; // Non-adjacent step: a rejected route must leave no route.
        CHECK_EQ(legoobject_route_setup_and_marker_associate(&raider, 3, xs, ys, nullptr), 0);
        CHECK_EQ(raider.routeBlocks, nullptr);
        CHECK_EQ(raider.routeBlocksTotal, 0u);
        legoGlobs.currLevel = nullptr;
        Container_Remove(child);
    }
    Container_Shutdown();
    s_main_d3drm = nullptr;
    rm3->Release();
}
} // namespace

int main()
{
    StatsData stats = {};
    Lego_Object raider = {}, carrier = {};
    raider.stats = &stats;
    raider.type = 2;
    raider.health = 80.0f;

    legoobject_damage_apply(&raider, 15.0f, 0, 1.0f);
    CHECK_EQ(raider.health, 65.0f);
    raider.carryingThisObject = &carrier;
    legoobject_damage_apply(&raider, 10.0f, 0, 1.0f);
    CHECK_EQ(raider.health, 65.0f);
    raider.carryingThisObject = nullptr;
    legoobject_damage_apply(&raider, -5.0f, 0, 1.0f);
    CHECK_EQ(raider.health, 70.0f);

    legoobject_activity_grant(&raider, 0);
    CHECK_EQ(raider.numCarriedTools, 0u);
    stats.NumOfToolsCanCarry = 3;
    stats_add_tool_task_type(0, 3);
    CHECK_EQ(legoobject_type_two_target_resolve_and_apply(&raider, 3), 0);
    legoobject_activity_grant(&raider, 0);
    CHECK_EQ(legoobject_type_two_target_resolve_and_apply(&raider, 3), 1);
    legoobject_activity_grant(&raider, 1);
    legoobject_activity_grant(&raider, 4);
    CHECK_EQ(raider.numCarriedTools, 3u);
    legoobject_activity_grant(&raider, 5);
    CHECK_EQ(raider.numCarriedTools, 3u);
    CHECK_EQ(legoobject_activity_scan(&raider, 4), 0);
    CHECK_EQ(legoobject_activity_scan(&raider, 5), 1);
    legoobject_activity_grant(&raider, 2);
    CHECK_EQ(raider.numCarriedTools, 3u);
    CHECK_EQ(raider.carriedTools[0], 1);
    CHECK_EQ(raider.carriedTools[1], 5);
    CHECK_EQ(raider.carriedTools[2], 2);
    CHECK_EQ(legoobject_type_two_target_resolve_and_apply(&raider, 3), 0);
    CheckDecay();
    return lrr_test_result("raider_synthetic");
}
