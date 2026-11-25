// SPDX-License-Identifier: LGPL-3.0-or-later
/* Synthetic catalog objects: upgrade completion, driving and recall. */
#include <cmath>
#include <cstdio>
#include <string>

#include "gods98/config.h"
#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/files_native.h"
#include "gods98/lws.h"
#include "game/animation_object_record.h"
#include "game/building_model_record.h"
#include "game/entity_stats_link_names.h"
#include "game/entity_stats.h"
#include "game/lego_unit.h"
#include "game/interface_unit.h"
#include "game/helpwindow_link_names.h"
#include "game/helpwindow.h"
#include "game/legoobject_unit.h"
#include "game/objectrecall.h"
#include "game/panel_unit.h"
#include "game/teleporter_link_names.h"
#include "game/teleporter.h"
#include "game/terrain_map.h"
#include "game/vehicle_model_link_names.h"
#include "game/vehicle_model.h"
#include "game/vehicle_model_record.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"
#include "test/synthetic_level_fixture.h"
#include "game/lego_globs.h"

namespace {
using namespace synthetic_fixture;
constexpr unsigned Upgrading = 0x8000, Driving = 8;
constexpr unsigned Removing = 0x800000;

bool load_catalog()
{
    // Use the positive-startup fixture's generic moving null and triangle.
    // The null is shared by the attachment slots, not a game model.
    if (!CHECK(fixture_mkdir("Data/Primitive") &&
        write_null_scene("Data/Primitive/motion.lws") &&
        write_triangle("Data/Primitive/triangle.lwo") &&
        write_text("Data/Primitive/Primitive.ae",
            "Lego { Activities { Activity_Stand Motion Activity_TeleportIn Arrival } "
            "Motion { FILE motion LWSFILE TRUE LOOPING TRUE } "
            "Arrival { FILE motion LWSFILE TRUE LOOPING FALSE } "
            "CarryNullName Anchor CarryNullFrames 1 DriverNullName Anchor DepositNullName Anchor "
            "WheelMesh triangle WheelNullName Anchor WheelRadius 2.5 "
            "Upgrades { Level0001 { Part Anchor,1 } } }"),
        "generate catalog primitives")) return false;
    // Barracks is the spawn unit's required lookup key; its model is
    // the same generic null as the other entries.
    constexpr char catalog[] =
        "Lego { UpgradeTypes { Part Primitive\\triangle } "
        "VehicleTypes { VehicleA Primitive } MiniFigureTypes { Figure Primitive } "
        "BuildingTypes { Barracks Primitive } "
        "Stats { VehicleA { Levels 4 CollRadius 8 CrossLand TRUE CanBeDriven TRUE GetOutAtLand TRUE "
        "RouteSpeed 5:7:7:7 UpgradeCostOre 9:9:9:6 UpgradeCostStuds 2:2:2:2 } "
        "Figure { Levels 2 CollRadius 4 CrossLand TRUE RouteSpeed 5:7 "
        "UseLegoManTeleporter TRUE CostCrystal 3 } "
        "Barracks { Levels 1 CollRadius 5 SelfPowered TRUE ManTeleporter TRUE } } "
        "HelpWindow { TextWindowArea 0,0,16,16 } }";
    BlockFile* config = block_parse(catalog, sizeof(catalog) - 1);
    if (!CHECK(config != nullptr, "parse synthetic catalog")) return false;
    auto* oldConfig = legoGlobs.config;
    legoGlobs.config = config;
    lws_initialise(nullptr, nullptr, nullptr, nullptr);
    bool ok = CHECK(lego_upgrade_types_load(), "load triangle upgrade") &&
              CHECK(lego_entity_vehicle_type_table_load(), "load synthetic vehicle") &&
              CHECK(lego_entity_minifigure_type_table_load(), "load synthetic figure") &&
              CHECK(lego_entity_building_types_load(), "load synthetic building") &&
              CHECK(stats_initialise(config, "Lego"), "load synthetic stats");
    if (ok) help_window_initialise(config, "Lego");
    legoGlobs.config = oldConfig;
    block_close(config);
    return ok;
}

Container* root() { return static_cast<Container*>(legoGlobs.rootCont); }
Lego_Level* level() { return legoGlobs.currLevel; }
Lego_Block& cell(int x, int y) { return level()->blocks[y * level()->width + x]; }

void place_frame(Container* frame, int x, int y, float offset = 0)
{
    float wx, wy;
    terrain_map_block_to_world(level()->map, x, y, &wx, &wy);
    Container_SetPosition(frame, root(), wx + offset, wy, 0);
    Container_SetOrientation(frame, root(), 0, 1, 0, 0, 0, -1);
}

void check_cell(Lego_Object* object, int x, int y)
{
    int bx = -1, by = -1;
    CHECK_EQ(legoobject_position_to_terrain_cell(object, &bx, &by), 1);
    CHECK_EQ(bx, x);
    CHECK_EQ(by, y);
}

Lego_Object* create_vehicle()
{
    auto* vehicle = legoobject_create(legoGlobs.vehicleData, 1, 0);
    if (!CHECK(vehicle != nullptr, "create loaded vehicle")) return nullptr;
    // The primitive catalog has only a single wheel null. Use the chassis
    // frame directly; the driving scenario tests boarding, not suspension.
    place_frame(static_cast<Container*>(vehicle->vehicle->contAct1), 1, 3);
    return vehicle;
}

Lego_Object* create_figure()
{
    float wx, wy;
    terrain_map_block_to_world(level()->map, 1, 3, &wx, &wy);
    auto* figure = legoobject_create_in_world(legoGlobs.miniFigureData, 2, 0, 0, wx, wy, 0);
    if (!CHECK(figure != nullptr, "create loaded figure")) return nullptr;
    return figure;
}

int count_kind(Lego_Object* object, void* data)
{
    auto* counts = static_cast<unsigned*>(data);
    if (object->type == 2) ++counts[0];
    if (object->type == 10) ++counts[1];
    return 0;
}

unsigned count_objects(int index, bool skipUpgrades = false)
{
    unsigned counts[2] = {};
    legoobject_run_through_lists(count_kind, counts, skipUpgrades);
    return counts[index];
}

void begin_upgrade(Lego_Object* vehicle, Lego_Object* workshop)
{
    // The arrival action has selected a workshop and started the upgrade.
    vehicle->field_404 = 0;
    vehicle->routeToObject = workshop;
    vehicle->flags2 |= Upgrading;
    workshop->interactObject = vehicle;
}

void finish_upgrade_tick(Lego_Object* vehicle)
{
    float elapsed = 20;
    for (unsigned frame = 0; frame < 4 && (vehicle->flags2 & Upgrading); ++frame)
        legoobject_update_callback(vehicle, &elapsed);
}

void check_upgrade()
{
    auto* first = create_vehicle();
    auto* second = create_vehicle();
    if (!first || !second) return;
    auto* model = first->vehicle;
    auto* templateModel = static_cast<VehicleModel*>(legoGlobs.vehicleData);
    CHECK_EQ(Container_GetUserData(static_cast<Container*>(model->contAct1)), first);
    CHECK(model->contAct1 != templateModel->contAct1, "live chassis cloned from template");
    StatsData workshopStats = {};
    workshopStats.flags2 = 2; // Workshop accepts upgrades.
    Lego_Object workshop = {};
    workshop.type = 4;
    workshop.stats = &workshopStats;

    level()->crystals = 5;
    level()->ore = 1;
    CHECK_EQ(legoobject_action_kind_prerequisite_check(first, 0), 0);
    CHECK_EQ(legoobject_action_kind_prerequisite_check(first, 1), 0);
    begin_upgrade(first, &workshop);
    finish_upgrade_tick(first);
    CHECK_EQ(first->flags2 & Upgrading, 0u);
    CHECK_EQ(first->routeToObject, nullptr);
    CHECK_EQ(workshop.interactObject, nullptr);
    CHECK_EQ(first->objLevel, 0u);
    CHECK_EQ(model->upgrades.objectList, nullptr);
    CHECK_EQ(level()->crystals, 5);
    CHECK_EQ(level()->ore, 1);

    level()->crystals = 6;
    CHECK_EQ(legoobject_action_kind_prerequisite_check(first, 0), 1);
    begin_upgrade(first, &workshop);
    workshop.flags2 = Upgrading;
    finish_upgrade_tick(first);
    CHECK(first->flags2 & Upgrading, "wait while workshop is busy");
    CHECK_EQ(first->objLevel, 0u);
    CHECK_EQ(level()->crystals, 6);
    workshop.flags2 = 0;
    finish_upgrade_tick(first);
    CHECK_EQ(first->flags2 & Upgrading, 0u);
    CHECK_EQ(first->objLevel, 1u);
    CHECK_EQ(model->upgrades.currentLevel, 1u);
    CHECK_EQ(first->stats->RouteSpeed, 7.0f);
    CHECK_EQ(level()->crystals, 0);
    CHECK_EQ(level()->ore, 1);
    CHECK_EQ(legoobject_action_kind_prerequisite_check(first, 0), 0);
    CHECK_EQ(count_objects(1), 1u);
    CHECK_EQ(count_objects(1, true), 0u);
    auto* part = model->upgrades.objectList;
    if (!CHECK(part && part->upgradePart, "part created in object pool")) return;
    CHECK_EQ(part->upgradePart->partInfo, &model->upgrades.parts[0]);
    auto* anchor = static_cast<Container*>(vehicle_find_null(model, "Anchor", 0));
    if (CHECK(anchor != nullptr, "attachment null found")) {
        CHECK_EQ(Container_GetParent(static_cast<Container*>(part->upgradePart->cont)), anchor);
    }
    CHECK_EQ(templateModel->upgrades.objectList, nullptr);
    CHECK_EQ(second->vehicle->upgrades.objectList, nullptr);

    level()->ore = 2;
    CHECK_EQ(legoobject_action_kind_prerequisite_check(second, 0), 1);
    begin_upgrade(second, &workshop);
    finish_upgrade_tick(second);
    CHECK_EQ(second->objLevel, 1u);
    CHECK_EQ(level()->ore, 0);
    CHECK_EQ(level()->crystals, 0);
    CHECK_EQ(count_objects(1), 2u);
    CHECK(second->vehicle->upgrades.objectList != part, "each vehicle owns its own upgrade");
    legoobject_remove(second);
    CHECK_EQ(count_objects(1), 1u);
    CHECK_EQ(model->upgrades.objectList, part);
    legoobject_remove(first);
    CHECK_EQ(count_objects(1), 0u);
}

bool board_vehicle(Lego_Object* driver, Lego_Object* vehicle)
{
    place_frame(static_cast<Container*>(driver->miniFigure->container), 1, 3, -10);
    if (!CHECK_EQ(legoobject_level_debug_cell_retarget_dispatch(driver, vehicle), 1) ||
        !CHECK(driver->routeBlocks != nullptr && driver->routeBlocksTotal != 0,
               "boarding route allocated")) return false;
    const auto* route = static_cast<LegoObjG_RouteBlock*>(driver->routeBlocks);
    CHECK_EQ(route[driver->routeBlocksTotal - 1].actionFlags, 12u);
    CHECK_EQ(driver->routeToObject, vehicle);
    float elapsed = 1;
    for (unsigned frame = 0; frame < 30 && !driver->driveObject; ++frame)
        legoobject_update_callback(driver, &elapsed);
    if (!CHECK_EQ(driver->driveObject, vehicle)) return false;
    CHECK_EQ(vehicle->driveObject, driver);
    CHECK_EQ(driver->flags2 & Driving, Driving);
    CHECK_EQ(driver->routeBlocks, nullptr);
    CHECK_EQ(driver->routeToObject, nullptr);
    return true;
}

void check_driver_exit(Lego_Object* driver, Lego_Object* vehicle, int x, int y,
                       const float expectedDir[3])
{
    CHECK_EQ(vehicle->driveObject, nullptr);
    CHECK_EQ(driver->driveObject, nullptr);
    CHECK_EQ(driver->flags2 & Driving, 0u);
    check_cell(driver, x, y);
    float dir[3], up[3];
    Container_GetOrientation(static_cast<Container*>(driver->miniFigure->container),
                             root(), dir, up);
    const float horizontal = std::hypot(expectedDir[0], expectedDir[1]);
    if (!CHECK(horizontal > 0, "fixture has a horizontal facing")) return;
    CHECK_NEAR(dir[0], expectedDir[0] / horizontal, 1e-4);
    CHECK_NEAR(dir[1], expectedDir[1] / horizontal, 1e-4);
    CHECK_NEAR(dir[2], 0.0f, 1e-4);
    CHECK_NEAR(up[0], 0.0f, 1e-4);
    CHECK_NEAR(up[1], 0.0f, 1e-4);
    CHECK_NEAR(up[2], -1.0f, 1e-4);
}

void check_drive()
{
    auto* vehicle = create_vehicle();
    auto* driver = create_figure();
    if (!vehicle || !driver) return;
    CHECK_EQ(legoobject_terrain_cell_to_world_position(vehicle, 1), 0);
    driver->abilityFlags = 0;
    CHECK_EQ(legoobject_level_debug_cell_retarget_dispatch(driver, vehicle), 0);
    CHECK_EQ(driver->routeBlocks, nullptr);
    driver->abilityFlags = 1;
    if (!board_vehicle(driver, vehicle)) return;
    // The actual pose-sync routine follows the driver's attachment null.
    auto* chassis = static_cast<Container*>(vehicle->vehicle->contAct1);
    auto* driverFrame = static_cast<Container*>(driver->miniFigure->container);
    place_frame(chassis, 1, 2);
    legoobject_scene_object_pose_position_sync(vehicle);
    check_cell(driver, 1, 2);
    auto* seat = static_cast<Container*>(vehicle_get_driver_null(vehicle->vehicle));
    if (CHECK(seat != nullptr, "driver attachment found")) {
        float expected[3], actual[3];
        Container_GetPosition(seat, root(), expected);
        Container_GetPosition(driverFrame, root(), actual);
        for (unsigned i = 0; i < 3; ++i) CHECK_NEAR(actual[i], expected[i], 1e-4);
    }
    place_frame(chassis, 1, 3);
    legoobject_scene_object_pose_position_sync(vehicle);
    check_cell(driver, 1, 3);

    // Already occupied vehicles cannot accept a second driver.
    auto* other = create_figure();
    if (!other) return;
    other->abilityFlags = 1;
    CHECK_EQ(legoobject_level_debug_cell_retarget_dispatch(other, vehicle), 0);
    CHECK_EQ(other->routeBlocks, nullptr);

    driver->flags1 |= 1;
    CHECK_EQ(legoobject_terrain_cell_to_world_position(vehicle, 1), 0);
    CHECK_EQ(driver->driveObject, vehicle);
    driver->flags1 &= ~1u;
    cell(1, 3).f_02 = 6; // No safe exit on this tile.
    CHECK_EQ(legoobject_terrain_cell_to_world_position(vehicle, 1), 0);
    CHECK_EQ(vehicle->driveObject, driver);

    // Ordinary dry-land exit: preserve the driver's current facing, not
    // an arbitrary default direction. Disabled adjacent fallback is fine.
    cell(1, 3).f_02 = 0;
    Container_SetOrientation(chassis, root(), 0.6f, 0.8f, 0, 0, 0, -1);
    legoobject_scene_object_pose_position_sync(vehicle);
    float facing[3], up[3];
    Container_GetOrientation(driverFrame, root(), facing, up);
    CHECK_NEAR(facing[0], 0.6f, 1e-4);
    CHECK_NEAR(facing[1], 0.8f, 1e-4);
    auto* exitFrame = static_cast<Container*>(legoobject_visual_resource_get_or_create(vehicle));
    if (!CHECK(exitFrame != nullptr, "vehicle exit frame")) return;
    float pos[3], exitPos[3];
    Container_GetPosition(exitFrame, root(), exitPos);
    if (!CHECK_EQ(legoobject_terrain_cell_to_world_position(vehicle, 1), 1)) return;
    check_driver_exit(driver, vehicle, 1, 3, facing);
    Container_GetPosition(driverFrame, root(), pos);
    CHECK_NEAR(pos[0], exitPos[0], 1e-4);
    CHECK_NEAR(pos[1], exitPos[1], 1e-4);
    CHECK_NEAR(pos[2], terrain_map_world_z_get(level()->map, exitPos[0], exitPos[1]), 1e-4);

    // Reboard and try the adjacent fallback. Lava blocks the own tile
    // and the first candidate (north), so east is selected.
    place_frame(chassis, 1, 3);
    if (!board_vehicle(driver, vehicle)) return;
    Container_SetOrientation(chassis, root(), -0.8f, 0.6f, 0, 0, 0, -1);
    legoobject_scene_object_pose_position_sync(vehicle);
    Container_GetOrientation(driverFrame, root(), facing, up);
    cell(1, 3).f_02 = 6;
    cell(1, 2).f_02 = 6;
    cell(2, 3).flags1 |= 8;
    cell(2, 3).flags2 = 0;
    cell(2, 3).f_02 = 0;
    CHECK_EQ(legoobject_terrain_cell_to_world_position(vehicle, 1), 0);
    CHECK_EQ(driver->driveObject, vehicle);
    if (!CHECK_EQ(legoobject_terrain_cell_to_world_position(vehicle, 0), 1)) return;
    check_driver_exit(driver, vehicle, 2, 3, facing);
    float wx, wy;
    terrain_map_block_to_world(level()->map, 2, 3, &wx, &wy);
    Container_GetPosition(driverFrame, root(), pos);
    CHECK_NEAR(pos[0], wx, 1e-4);
    CHECK_NEAR(pos[1], wy, 1e-4);
    CHECK_NEAR(pos[2], terrain_map_world_z_get(level()->map, wx, wy), 1e-4);

    // Dock exit still uses the dock's facing rather than the driver's.
    cell(1, 3).f_02 = cell(1, 2).f_02 = 0;
    place_frame(chassis, 1, 3);
    if (!board_vehicle(driver, vehicle)) return;
    cell(1, 3).f_02 = 6;
    Container* dock = Container_Create(root());
    if (!CHECK(dock != nullptr, "generic dock frame")) return;
    place_frame(dock, 1, 4);
    Container_SetOrientation(dock, root(), -1, 0, 0, 0, 0, -1);
    Container_GetOrientation(dock, root(), facing, up);
    BuildingModel buildingModel = {};
    char dockName[] = "Dock";
    buildingModel.entranceNullName = dockName;
    Lego_Object building = {};
    building.type = 4;
    building.building = &buildingModel;
    vehicle->flags4 |= 0x40;
    CHECK_EQ(legoobject_terrain_cell_to_world_position(vehicle, 1), 0);
    vehicle->routeToObject = &building;
    buildingModel.entranceNull = dock;
    if (!CHECK_EQ(legoobject_terrain_cell_to_world_position(vehicle, 1), 1)) return;
    check_driver_exit(driver, vehicle, 1, 4, facing);
    vehicle->flags4 &= ~0x40u;
    vehicle->routeToObject = nullptr;
    cell(1, 3).f_02 = 0;
    legoobject_remove(other);
    legoobject_remove(driver);
    legoobject_remove(vehicle);
    Container_Remove(dock);
}

void check_recall()
{
    auto* first = create_figure();
    auto* second = create_figure();
    auto* third = create_figure();
    if (!first || !second || !third) return;
    constexpr char MaxName[] = "Sample-Long";
    static_assert(sizeof(MaxName) == 12, "eleven visible characters");
    first->abilityFlags = 8; // Unnamed entry to exercise clearing on recall.
    CHECK_EQ(stats_object_set_object_level(second, 1), 1);
    second->abilityFlags = 5;
    CHECK_EQ(second->customName, nullptr);
    legoobject_rename(second, "Sample");
    CHECK_STREQ(second->customName, "Sample");
    if (!CHECK(second->customName != nullptr, "name allocated")) return;
    auto* name = second->customName;
    legoGlobs.renameInput = name; // The name editor borrows this buffer.
    legoobject_rename(second, MaxName);
    CHECK_STREQ(second->customName, MaxName);
    CHECK_EQ(second->customName, name);
    CHECK_EQ(legoGlobs.renameInput, name);
    legoobject_rename(first, "Other");
    legoobject_rename(first, nullptr);
    CHECK_EQ(first->customName, nullptr);
    CHECK_EQ(legoGlobs.renameInput, name);
    legoobject_rename(second, "Hi");
    CHECK_STREQ(second->customName, "Hi");
    legoobject_rename(second, "");
    CHECK_EQ(second->customName, nullptr);
    CHECK_EQ(legoGlobs.renameInput, nullptr);
    legoobject_rename(second, "Again");
    CHECK_STREQ(second->customName, "Again");
    legoobject_rename(second, nullptr);
    CHECK_EQ(second->customName, nullptr);
    legoobject_rename(second, nullptr);
    CHECK_EQ(second->customName, nullptr);
    legoobject_rename(second, MaxName);
    CHECK_STREQ(second->customName, MaxName);
    third->abilityFlags = 2;
    legoobject_rename(third, "Sample-B");
    CHECK_STREQ(third->customName, "Sample-B");
    CHECK_EQ(count_objects(0), 3u);
    CHECK_EQ(object_recall_restore(), 0);
    CHECK_EQ(teleporter_service_all(TELEPORT_SERVICE_MINIFIGURE), 1);
    teleporter_start(TELEPORT_SERVICE_MINIFIGURE, TELEPORT_MODE_SERVICE, TELEPORT_FLAG_REMOVE);
    CHECK_EQ(teleporterGlobs.teleportingCount, 3u);
    CHECK_EQ(first->teleporterMode, static_cast<unsigned>(TELEPORT_MODE_SERVICE));
    CHECK_EQ(first->teleporterService, teleporterGlobs.serviceList);
    CHECK_EQ(second->teleporterService, teleporterGlobs.serviceList);
    CHECK_EQ(third->teleporterService, teleporterGlobs.serviceList);
    CHECK_EQ(teleporter_service_all(TELEPORT_SERVICE_MINIFIGURE), 0);
    CHECK_EQ(teleporter_service_all(TELEPORT_SERVICE_ORE | TELEPORT_SERVICE_POWERCRYSTAL), 1);
    CHECK_EQ(first->health, 100.0f);
    teleporter_update(1);
    CHECK_EQ(teleporterGlobs.teleportingCount, 0u);
    CHECK_EQ(first->health, -1.0f);
    CHECK_EQ(second->health, -1.0f);
    CHECK_EQ(third->health, -1.0f);
    CHECK(first->flags3 & Removing, "first scheduled for removal");
    CHECK(second->flags3 & Removing, "second scheduled for removal");
    CHECK(third->flags3 & Removing, "third scheduled for removal");
    // The real removal gate stores the outgoing figures before recycling them.
    legoobject_objective_sidecar_append_gate(first);
    legoobject_objective_sidecar_append_gate(second);
    CHECK_EQ(second->customName, nullptr);
    legoobject_objective_sidecar_append_gate(third);
    CHECK_EQ(third->customName, nullptr);
    CHECK_EQ(count_objects(0), 0u);
    CHECK_EQ(teleporter_service_all(TELEPORT_SERVICE_MINIFIGURE), 1);
    teleporter_restart();
    CHECK_EQ(teleporterGlobs.serviceList, nullptr);
    object_recall_snapshot();
    CHECK_EQ(object_recall_is_loaded(), 1);

    files_native_set_save_path(nullptr);
    if (!CHECK(fixture_mkdir("Data/Saves"), "scratch save directory")) return;
    CHECK_EQ(object_recall_save("Saves\\progress.osf"), 1);
    CHECK_EQ(object_recall_is_loaded(), 0);
    // The existing spawn queue must restore the named record into a
    // newly created figure, with its own name storage.
    auto* source = legoobject_create(legoGlobs.buildingData, 4, 0);
    if (!CHECK(source != nullptr, "create teleport source")) return;
    place_frame(static_cast<Container*>(source->building->contAct), 1, 4);
    CHECK_EQ(legoobject_spawn_request_queue_add(source, 2, 0), 1);
    CHECK_EQ(legoGlobs.objectRequestCount, 1u);
    level()->f_08c = 2;
    level()->f_098 = 0;
    legoobject_spawn_request_queue_process(source);
    CHECK_EQ(legoGlobs.objectRequestCount, 1u);
    CHECK_EQ(source->field_3cc, nullptr);
    CHECK_EQ(level()->f_08c, 2);
    CHECK_EQ(count_objects(0), 0u);
    level()->f_08c = 3;
    level()->f_098 = 1; // One crystal is committed elsewhere.
    legoobject_spawn_request_queue_process(source);
    CHECK_EQ(source->field_3cc, nullptr);
    CHECK_EQ(legoGlobs.objectRequestCount, 1u);
    CHECK_EQ(level()->f_08c, 3);
    level()->f_098 = 0;
    legoobject_spawn_request_queue_process(source);
    auto* recalled = source->field_3cc;
    if (!CHECK(recalled != nullptr, "affordable queued figure spawned")) return;
    CHECK_EQ(level()->f_08c, 0);
    CHECK_EQ(legoGlobs.objectRequestCount, 0u);
    CHECK_EQ(count_objects(0), 1u);
    CHECK(recalled->flags1 & 0x200000, "teleport-in animation pending");
    check_cell(recalled, 1, 4);
    CHECK_EQ(recalled->abilityFlags, 2u);
    CHECK_EQ(recalled->objLevel, 0u);
    CHECK_STREQ(recalled->customName, "Sample-B");
    float elapsed = 20;
    for (unsigned frame = 0; frame < 4 && (recalled->flags1 & 0x200000); ++frame)
        legoobject_update_callback(recalled, &elapsed);
    CHECK_EQ(recalled->flags1 & 0x200000, 0u);
    CHECK_EQ(source->field_3cc, nullptr);
    CHECK_EQ(object_recall_recall_minifigure(recalled), 1);
    CHECK_EQ(recalled->abilityFlags, 5u);
    CHECK_EQ(recalled->objLevel, 1u);
    CHECK_EQ(recalled->stats->RouteSpeed, 7.0f);
    CHECK_STREQ(recalled->customName, MaxName);
    // The unnamed record clears an already-owned name.
    CHECK_EQ(object_recall_recall_minifigure(recalled), 1);
    CHECK_EQ(recalled->abilityFlags, 8u);
    CHECK_EQ(recalled->customName, nullptr);
    CHECK_EQ(recalled->objLevel, 0u);
    CHECK_EQ(object_recall_recall_minifigure(recalled), 0);
    CHECK_EQ(object_recall_restore(), 1);
    CHECK_EQ(object_recall_recall_minifigure(recalled), 1);
    CHECK_EQ(recalled->abilityFlags, 2u);
    CHECK_STREQ(recalled->customName, "Sample-B");

    // A missing file clears the live list, and a subsequent load recovers
    // the entries from the saved sidecar, including their names and order.
    std::remove("Data/Saves/absent.osf");
    CHECK_EQ(object_recall_load_rrosfile("Saves\\absent.osf"), 0);
    CHECK_EQ(object_recall_recall_minifigure(recalled), 0);
    CHECK_EQ(object_recall_load_rrosfile("Saves\\progress.osf"), 1);
    CHECK_EQ(object_recall_is_loaded(), 0);
    CHECK_EQ(object_recall_recall_minifigure(recalled), 1);
    CHECK_EQ(recalled->abilityFlags, 2u);
    CHECK_STREQ(recalled->customName, "Sample-B");
    CHECK_EQ(object_recall_recall_minifigure(recalled), 1);
    CHECK_EQ(recalled->abilityFlags, 5u);
    CHECK_EQ(recalled->objLevel, 1u);
    CHECK_STREQ(recalled->customName, MaxName);
    CHECK_EQ(object_recall_recall_minifigure(recalled), 1);
    CHECK_EQ(recalled->abilityFlags, 8u);
    CHECK_EQ(recalled->customName, nullptr);
    CHECK_EQ(object_recall_recall_minifigure(recalled), 0);

    // Removing a named figure releases the owned buffer before the pool
    // slot is recycled, including any name-editor borrow of that buffer.
    legoobject_rename(recalled, "Transient");
    CHECK_STREQ(recalled->customName, "Transient");
    legoGlobs.renameInput = recalled->customName;
    legoobject_remove(recalled);
    CHECK_EQ(recalled->customName, nullptr);
    CHECK_EQ(legoGlobs.renameInput, nullptr);
    auto* fresh = create_figure();
    if (!fresh) return;
    CHECK_EQ(fresh->customName, nullptr);
    legoobject_remove(fresh);
    legoobject_remove(source);
    object_recall_reset();
    std::remove("Data/Saves/progress.osf");
}
// A narrower case for sanitizer runs: the same unit functions on real
// retained-mode frames and a loaded synthetic grid, without an LWS catalog.
void check_ownership()
{
    auto* chassis = Container_Create(root());
    auto* prototype = Container_Create(root());
    if (!CHECK(chassis && prototype, "generic frames")) return;
    StatsData figureStats[2] = {};
    for (auto& stats : figureStats) {
        stats.flags1 = 0x2000000;
        stats.RouteSpeed = 5;
    }
    StatsData* byId[STATS_OBJECT_IDS] = {};
    byId[0] = figureStats;
    auto* stats = entity_stats_port_globs();
    auto* oldStats = stats->objectStats[2];
    const auto oldLevels = stats->objectLevels[2][0];
    stats->objectStats[2] = byId;
    stats->objectLevels[2][0] = 2;
    AnimationObject model = {};
    model.container = prototype;
    auto create = [&]() {
        float x, y;
        terrain_map_block_to_world(level()->map, 1, 3, &x, &y);
        return legoobject_create_in_world(&model, 2, 0, 0, x, y, 0);
    };
    auto* driver = create();
    if (!CHECK(driver != nullptr, "real pooled figure")) return;
    StatsData vehicleStats = {};
    vehicleStats.flags1 = 0x2020000;
    vehicleStats.flags3 = 0x10;
    VehicleModel vehicleModel = {};
    vehicleModel.contAct1 = chassis;
    Lego_Object vehicle = {};
    vehicle.type = 1;
    vehicle.vehicle = &vehicleModel;
    vehicle.stats = &vehicleStats;
    place_frame(chassis, 1, 3);
    auto seat_driver = [&](float x, float y) {
        Container_SetOrientation(chassis, root(), x, y, 0, 0, 0, -1);
        vehicle.driveObject = driver;
        driver->driveObject = &vehicle;
        driver->flags2 |= Driving;
        legoobject_scene_object_pose_position_sync(&vehicle);
    };
    seat_driver(0.6f, 0.8f);
    const float directDir[] = {0.6f, 0.8f, 0.0f};
    if (!CHECK_EQ(legoobject_terrain_cell_to_world_position(&vehicle, 1), 1)) return;
    check_driver_exit(driver, &vehicle, 1, 3, directDir);
    seat_driver(-0.8f, 0.6f);
    cell(1, 3).f_02 = 6;
    cell(1, 2).f_02 = 6;
    CHECK_EQ(legoobject_terrain_cell_to_world_position(&vehicle, 1), 0);
    CHECK_EQ(vehicle.driveObject, driver);
    const float adjacentDir[] = {-0.8f, 0.6f, 0.0f};
    if (!CHECK_EQ(legoobject_terrain_cell_to_world_position(&vehicle, 0), 1)) return;
    check_driver_exit(driver, &vehicle, 2, 3, adjacentDir);
    cell(1, 3).f_02 = cell(1, 2).f_02 = 0;

    constexpr char MaxName[] = "Sample-Long";
    legoobject_rename(driver, "Short");
    CHECK_STREQ(driver->customName, "Short");
    legoobject_rename(driver, MaxName);
    CHECK_STREQ(driver->customName, MaxName);
    legoobject_rename(driver, nullptr);
    CHECK_EQ(driver->customName, nullptr);
    legoobject_rename(driver, MaxName);
    CHECK_STREQ(driver->customName, MaxName);
    driver->abilityFlags = 5;
    CHECK_EQ(stats_object_set_object_level(driver, 1), 1);
    object_recall_store_minifigure(driver);
    legoGlobs.renameInput = driver->customName;
    legoobject_remove(driver);
    CHECK_EQ(driver->customName, nullptr);
    CHECK_EQ(legoGlobs.renameInput, nullptr);

    driver = create();
    if (!CHECK(driver != nullptr, "fresh pooled figure")) return;
    CHECK_EQ(driver->customName, nullptr);
    CHECK_EQ(object_recall_recall_minifigure(driver), 1);
    CHECK_STREQ(driver->customName, MaxName);
    CHECK_EQ(driver->abilityFlags, 5u);
    CHECK_EQ(driver->objLevel, 1u);
    legoobject_rename(driver, "Short");
    CHECK_STREQ(driver->customName, "Short");
    legoobject_rename(driver, "");
    CHECK_EQ(driver->customName, nullptr);
    legoobject_rename(driver, "Final");
    CHECK_STREQ(driver->customName, "Final");
    legoobject_remove(driver);
    CHECK_EQ(driver->customName, nullptr);
    object_recall_reset();
    stats->objectStats[2] = oldStats;
    stats->objectLevels[2][0] = oldLevels;
    Container_Remove(prototype);
    Container_Remove(chassis);
}
} // namespace

int main(int argc, char** argv)
{
    const std::string scenario = argc == 2 ? argv[1] : "upgrade";
    if (!CHECK(scenario == "upgrade" || scenario == "drive" ||
               scenario == "recall" || scenario == "ownership", "known scenario")) return lrr_test_result();
    const std::string directory = "upgrade_recall_progression_" + scenario;
    if (!CHECK(make_fixture(directory.c_str()), "generate synthetic level") ||
        !initialise_headless("UpgradeRecallProgression") || !boot_game() ||
        !CHECK(lego_level_load("Level01"), "load synthetic level")) return lrr_test_result();
    if (scenario != "ownership" && !load_catalog()) return lrr_test_result();
    if (scenario == "ownership") check_ownership();
    else if (scenario == "upgrade") check_upgrade();
    else if (scenario == "drive") check_drive();
    else check_recall();
    // The shared boot fixture has no dynamic catalog buttons. Supply
    // the one-vehicle and one-building tables cleared by interface_reset.
    if (lrr_test::failures != 0) return lrr_test_result();
    unsigned vehicleClicks[1] = {1}, vehicleFlags[1] = {1};
    unsigned buildingClicks[1] = {1}, buildingFlags[1] = {1};
    const unsigned catalogCount = scenario == "ownership" ? 0u : 1u;
    if (!CHECK_EQ(s_catalog_counts[0], catalogCount) ||
        !CHECK_EQ(s_catalog_counts[3], catalogCount)) return lrr_test_result();
    interfaceGlobs.vehicleItemClicks = vehicleClicks;
    interfaceGlobs.vehicleItemFlags = vehicleFlags;
    interfaceGlobs.buildingItemClicks = buildingClicks;
    interfaceGlobs.buildingItemFlags = buildingFlags;
    const unsigned types[] = {0, 1, 2};
    const float areas[12] = {};
    const int tips[3] = {};
    panel_texture_slots_allocate(PANEL_TOP, 3, types, areas, nullptr, nullptr, nullptr, tips);
    lego_level_free();
    CHECK(legoGlobs.currLevel == nullptr, "level unloaded");
    CHECK_EQ(vehicleClicks[0], 1u - catalogCount);
    CHECK_EQ(vehicleFlags[0], 1u - catalogCount);
    interfaceGlobs.vehicleItemClicks = nullptr;
    interfaceGlobs.vehicleItemFlags = nullptr;
    CHECK_EQ(buildingClicks[0], 1u - catalogCount);
    CHECK_EQ(buildingFlags[0], 1u - catalogCount);
    interfaceGlobs.buildingItemClicks = nullptr;
    interfaceGlobs.buildingItemFlags = nullptr;
    return lrr_test_result();
}
