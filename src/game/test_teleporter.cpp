// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_teleporter — service flag mapping, ring growth and the transition
 * to done for objects reached by a ring or teleported instantly. */
#include "game/legoobject_record.h"
#include "game/teleporter_link_names.h"
#include "game/teleporter.h"
#include "test/lrr_test.h"

static Lego_Object objects[2];

extern "C" int __wrap_legoobject_run_through_lists_skip_upgrade_parts(
    int (*callback)(Lego_Object*, void*), void* data)
{
    for (Lego_Object& object : objects) {
        if (callback(&object, data)) return 1;
    }
    return 0;
}

extern "C" void* __wrap_level_current_terrain_get(void) { return nullptr; }

extern "C" int __wrap_camera_get_topdown_world_pos(void*, void*, float out[3])
{
    out[0] = 2.0f;
    out[1] = 3.0f;
    out[2] = 0.0f;
    return 1;
}

extern "C" void __wrap_legoobject_transform_position_get(Lego_Object*, float* x, float* y)
{
    *x = 5.0f;
    *y = 7.0f;
}

int main(void)
{
    CHECK_EQ(teleporter_get_service_object_type(0), 0);
    CHECK_EQ(teleporter_get_service_object_type(TELEPORT_SERVICE_VEHICLE | TELEPORT_SERVICE_ORE), 1);
    CHECK_EQ(teleporter_get_service_object_type(TELEPORT_SERVICE_OOHSCARY), 13);

    Lego_Object& raider = objects[0];
    Lego_Object& vehicle = objects[1];
    raider.type = 2;
    vehicle.type = 1;
    raider.health = vehicle.health = 100.0f;
    teleporterGlobs.ringSpeed = 2.0f;
    teleporter_start(TELEPORT_SERVICE_MINIFIGURE, TELEPORT_MODE_SERVICE, TELEPORT_FLAG_RING);
    Teleporter_Service* ring = teleporterGlobs.serviceList;
    if (!CHECK(ring != nullptr, "a ring service was allocated")) return lrr_test_result();
    CHECK_EQ(raider.teleporterService, ring);
    CHECK_EQ(raider.teleporterMode, static_cast<unsigned int>(TELEPORT_MODE_SERVICE));
    CHECK_EQ(ring->teleportingCount, 1u);
    CHECK_EQ(vehicle.teleporterMode, 0u);
    CHECK_EQ(teleporter_service_all(TELEPORT_SERVICE_MINIFIGURE), 0);
    CHECK_EQ(teleporter_service_all(TELEPORT_SERVICE_VEHICLE), 1);

    teleporter_update(2.5f);
    CHECK_EQ(ring->radius, 5.0f);
    CHECK_EQ(raider.health, 100.0f);
    teleporter_update(0.5f);
    CHECK_EQ(ring->radius, 6.0f);
    CHECK_EQ(raider.health, -1.0f);
    CHECK_EQ(raider.teleporterMode, static_cast<unsigned int>(TELEPORT_MODE_DONE));
    CHECK_EQ(raider.teleporterService, nullptr);
    CHECK_EQ(raider.teleporterFlags, 0u);
    CHECK_EQ(ring->teleportingCount, 0u);
    CHECK_EQ(teleporterGlobs.teleportingCount, 0u);

    teleporter_start(TELEPORT_SERVICE_VEHICLE, TELEPORT_MODE_INSTANT, 0);
    teleporter_update(0.0f);
    CHECK_EQ(vehicle.health, -1.0f);
    CHECK_EQ(vehicle.teleporterMode, static_cast<unsigned int>(TELEPORT_MODE_DONE));
    CHECK_EQ(teleporterGlobs.teleportingCount, 0u);
    teleporter_restart();
    CHECK_EQ(teleporterGlobs.serviceList, nullptr);
    return lrr_test_result();
}
