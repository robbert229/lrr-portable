// SPDX-License-Identifier: LGPL-3.0-or-later
/* Transfer and attachment ownership on neutral cargo records and the
 * generated vehicle's single scene attachment. */
#include "gods98/container_link_names.h"
#include "gods98/container_unit.h"
#include "gods98/directdraw.h"
#include "game/entity_stats.h"
#include "game/interface_unit.h"
#include "game/lego_unit.h"
#include "game/legoobject_unit.h"
#include "game/message_unit_link_names.h"
#include "game/message_unit.h"
#include "game/terrain_map.h"
#include "game/vehicle_model_link_names.h"
#include "game/vehicle_model.h"
#include "test/lrr_test.h"
#include "test/synthetic_initializer_fixture.h"
#include "game/lego_globs.h"

namespace {
constexpr char Stats[] = " VehicleA { Levels 1 CollRadius 4 CrossLand TRUE RouteSpeed 5 MaxCarry 1 CarryStart 0 } ";
constexpr unsigned Carrying = 0x400;

Lego_Object* Create(void* model, int type, int x, int y)
{
    float wx, wy;
    terrain_map_block_to_world(legoGlobs.currLevel->map, x, y, &wx, &wy);
    auto* obj = legoobject_create_in_world(model, type, 0, 0, wx, wy, 0);
    CHECK(obj != nullptr, "create neutral scene object");
    return obj;
}

void CheckCargoAtAnchor(Lego_Object* cargo, Lego_Object* vehicle)
{
    auto* anchor = static_cast<Container*>(vehicle_find_null(vehicle->vehicle, "Anchor", 0));
    if (CHECK(anchor != nullptr, "resolve generated attachment frame")) {
        float expected[3] = {}, actual[3] = {};
        Container_GetPosition(anchor, nullptr, expected);
        Container_GetPosition(static_cast<Container*>(cargo->other), nullptr, actual);
        for (unsigned i = 0; i < 3; ++i) CHECK_NEAR(actual[i], expected[i], 1e-4);
    }
}

void Scenario()
{
    auto* vehicle = Create(legoGlobs.vehicleData, 1, 2, 3);
    auto* first = Create(legoGlobs.contCrystal, 6, 1, 3);
    auto* second = Create(legoGlobs.contOresTable, 7, 1, 4);
    if (!vehicle || !first || !second) return;
    CHECK_EQ(vehicle->carryNullFrames, 1u);
    CHECK_EQ(vehicle->stats->MaxCarry, 1u);
    CHECK_EQ(interface_callback_can_pick_up(vehicle, nullptr), 1);
    CHECK_EQ(interface_callback_can_unload(vehicle, nullptr), 0);

    // A neutral two-slot sender models the already-carried inventory at
    // the transfer boundary; the receiver and all payloads are real
    // scene-backed objects. Nothing is authored for a game level.
    StatsData senderStats = {};
    senderStats.MaxCarry = 2;
    Lego_Object source = {};
    source.type = 4;
    source.stats = &senderStats;
    source.carryNullFrames = 2;
    source.numCarriedObjects = 2;
    source.flags1 = Carrying;
    source.routeToObject = vehicle;
    source.carriedObjects[0] = first;
    source.carriedObjects[1] = second;
    first->carryingThisObject = second->carryingThisObject = &source;
    first->carryingIndex = 0;
    second->carryingIndex = 1;

    legoobject_simulation_queue_flush(&source);
    CHECK_EQ(vehicle->numCarriedObjects, 1u);
    CHECK_EQ(vehicle->carriedObjects[0], first);
    CHECK_EQ(first->carryingThisObject, vehicle);
    CHECK_EQ(first->carryingIndex, 0u);
    CHECK_EQ(source.numCarriedObjects, 1u);
    CHECK_EQ(source.carriedObjects[0], second);
    CHECK_EQ(source.carriedObjects[1], nullptr);
    CHECK_EQ(second->carryingThisObject, &source);
    CHECK_EQ(second->carryingIndex, 0u);
    CHECK_EQ(source.flags1 & Carrying, Carrying);
    CHECK_EQ(source.routeToObject, vehicle);
    CHECK_EQ(interface_callback_can_pick_up(vehicle, nullptr), 0);
    CHECK_EQ(interface_callback_can_unload(vehicle, nullptr), 1);

    // Another transfer into a full target is a no-op, without dropping
    // or duplicating the remaining object.
    legoobject_simulation_queue_flush(&source);
    CHECK_EQ(source.numCarriedObjects, 1u);
    CHECK_EQ(source.carriedObjects[0], second);
    CHECK_EQ(vehicle->carriedObjects[0], first);
    legoobject_attached_direction_vector_update(vehicle);
    CheckCargoAtAnchor(first, vehicle);
    float x, y;
    terrain_map_block_to_world(legoGlobs.currLevel->map, 2, 4, &x, &y);
    legoobject_world_position_set(vehicle, x, y, 0, 1);
    legoobject_attached_direction_vector_update(vehicle);
    CheckCargoAtAnchor(first, vehicle);

    // Real dispatch of the unload request clears both ownership links.
    message_select_object2(vehicle, 1, 0);
    CHECK_EQ(message_get_primary_selected_unit(), vehicle);
    interface_update_item_enabled(22);
    CHECK_EQ(interface_is_item_disabled(22, 0), 0);
    CHECK_EQ(interface_do_action(22), 1);
    CHECK_EQ(vehicle->numCarriedObjects, 0u);
    CHECK_EQ(vehicle->carriedObjects[0], nullptr);
    CHECK_EQ(first->carryingThisObject, nullptr);
    CHECK_EQ(vehicle->flags1 & Carrying, 0u);
    CHECK_EQ(interface_callback_can_pick_up(vehicle, nullptr), 1);
    CHECK_EQ(interface_callback_can_unload(vehicle, nullptr), 0);
    message_clear_selected_units();

    // The remaining record can now move to the same receiver. The
    // sender's route and carrying state are released when fully drained.
    legoobject_simulation_queue_flush(&source);
    CHECK_EQ(source.numCarriedObjects, 0u);
    CHECK_EQ(source.carriedObjects[0], nullptr);
    CHECK_EQ(source.carriedObjects[1], nullptr);
    CHECK_EQ(source.routeToObject, nullptr);
    CHECK_EQ(source.flags1 & Carrying, 0u);
    CHECK_EQ(vehicle->carriedObjects[0], second);
    CHECK_EQ(second->carryingThisObject, vehicle);
    legoobject_attached_direction_vector_update(vehicle);
    CheckCargoAtAnchor(second, vehicle);
    legoobject_attached_object_list_detach(vehicle, 0);
    CHECK_EQ(vehicle->numCarriedObjects, 0u);
    CHECK_EQ(second->carryingThisObject, nullptr);
}
}

int main()
{
    if (!synthetic_fixture::initialise_full_game("transport_capacity", "TransportCapacity", Stats))
        return lrr_test_result();
    Scenario();
    lego_engine_shutdown();
    CHECK_EQ(legoGlobs.currLevel, nullptr);
    directdraw_shutdown();
    return lrr_test_result("transport_capacity");
}
