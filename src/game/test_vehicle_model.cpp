// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_vehicle_model — upgrade eligibility masks, dual-activity
 * animation time, and attachment lookup priority, fallback and caching. */
#include "game/vehicle_model_link_names.h"
#include "game/vehicle_model.h"
#include "game/vehicle_model_record.h"
#include "test/lrr_test.h"

struct Activity {
    float animationTime;
    unsigned long frameCount;
    const char* attachmentName;
    void* attachment;
};

extern "C" float __wrap_ddiscene_advance_by_time_target(void* object, float delta)
{
    Activity* activity = static_cast<Activity*>(object);
    activity->animationTime += delta;
    return activity->animationTime - static_cast<float>(activity->frameCount);
}

extern "C" unsigned long __wrap_ddiscene_object_frame_position_get(void* object)
{
    return static_cast<Activity*>(object)->frameCount;
}

extern "C" const char* __wrap_xf_texture_name_format(void*, const char* name, const int* frame)
{
    static char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%s%d", name, *frame);
    return buffer;
}

extern "C" void* __wrap_resource_name_register_or_query_target(void** root, const char* name,
                                                               int, unsigned int*)
{
    Activity* activity = reinterpret_cast<Activity*>(root);
    return std::strcmp(name, activity->attachmentName) == 0 ? activity->attachment : nullptr;
}

int main(void)
{
    VehicleModel vehicle = {};
    vehicle.upgrades.levels = 5;
    vehicle.upgrades.currentLevel = 1;
    CHECK_EQ(vehicle_can_upgrade_type(&vehicle, 0, 0), 1u);
    CHECK_EQ(vehicle_can_upgrade_type(&vehicle, 0, 1), 0u);
    CHECK_EQ(vehicle_can_upgrade_type(&vehicle, 1, 0), 0u);
    CHECK_EQ(vehicle_can_upgrade_type(&vehicle, 2, 1), 4u);

    int primaryCamera = 0;
    int secondaryCamera = 0;
    Activity primary = { 7.0f, 8, "camera0", &primaryCamera };
    Activity secondary = { 4.0f, 4, "camera1", &secondaryCamera };
    vehicle.contAct1 = &primary;
    CHECK_EQ(vehicle_move_animation(&vehicle, 3.0f, 0.0f, 1), 2.0f);
    vehicle.contAct2 = &secondary;
    CHECK_EQ(vehicle_move_animation(&vehicle, 1.0f, 10.0f, 2), 2.0f);
    CHECK_EQ(primary.animationTime, 11.0f);
    CHECK_EQ(secondary.animationTime, 14.0f);
    vehicle.flags |= 0x4; // The primary activity is disabled.
    CHECK_EQ(vehicle_move_animation(&vehicle, 8.0f, 1.0f, 1), 11.0f);
    CHECK_EQ(primary.animationTime, 11.0f);
    CHECK_EQ(secondary.animationTime, 15.0f);

    CHECK_EQ(vehicle_get_camera_null(&vehicle, 0), nullptr);
    char cameraName[] = "camera";
    vehicle.cameraNullName = cameraName;
    CHECK_EQ(vehicle_get_camera_null(&vehicle, 0), static_cast<void*>(&primaryCamera));
    CHECK_EQ(vehicle_get_camera_null(&vehicle, 1), static_cast<void*>(&secondaryCamera));
    CHECK_EQ(vehicle_get_camera_null(&vehicle, 2), nullptr);
    secondary.attachment = nullptr;
    CHECK_EQ(vehicle_get_camera_null(&vehicle, 1), static_cast<void*>(&secondaryCamera));
    return lrr_test_result();
}
