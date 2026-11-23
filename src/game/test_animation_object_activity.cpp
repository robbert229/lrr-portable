// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_animation_object_activity — cache invalidation on activity change
 * and adjustment of animation time remaining across multiple repeats. */
#include "game/animation_object.h"
#include "game/terrain_map.h"
#include "test/lrr_test.h"

struct Scene {
    const char* activity;
    float time;
    float overrun;
    unsigned long frames;
};

extern "C" unsigned int __wrap_scene_action_name_set(void* object, const char* name)
{
    static_cast<Scene*>(object)->activity = name;
    return 1u;
}

extern "C" float __wrap_ddiscene_frame_update_and_callbacks(void* object, float time)
{
    static_cast<Scene*>(object)->time = time;
    return 0.0f;
}

extern "C" float __wrap_ddiscene_advance_by_time_target(void* object, float delta)
{
    auto& scene = *static_cast<Scene*>(object);
    scene.time += delta;
    return scene.overrun;
}

extern "C" unsigned long __wrap_ddiscene_object_frame_position_get(void* object)
{
    return static_cast<Scene*>(object)->frames;
}

int main(void)
{
    Scene scene = { "Idle", 2.0f, 0.0f, 8u };
    int attachment = 0;
    ModelEntryRecord mesh = {};
    mesh.instance = &attachment;
    AnimationObject object = {};
    object.container = &scene;
    object.polyMedium = &mesh;
    object.drillNull = &attachment;
    object.footStepNull = &attachment;
    object.carryNull = &attachment;
    object.throwNull = &attachment;
    object.depositNull = &attachment;
    object.cameraNullFrames = 2;
    object.cameraNulls[0] = &attachment;
    object.cameraNulls[1] = &attachment;

    CHECK_EQ(activity_runner_update(&object, "Drill", 4.0f), 1u);
    CHECK_STREQ(scene.activity, "Drill");
    CHECK_EQ(scene.time, 4.0f);
    CHECK_EQ(object.drillNull, nullptr);
    CHECK_EQ(object.footStepNull, nullptr);
    CHECK_EQ(object.carryNull, nullptr);
    CHECK_EQ(object.throwNull, nullptr);
    CHECK_EQ(object.depositNull, nullptr);
    CHECK_EQ(object.cameraNulls[0], nullptr);
    CHECK_EQ(object.cameraNulls[1], nullptr);
    CHECK_EQ(mesh.instance, nullptr);

    CHECK_EQ(renderer_object_timing_value_adjust_by_count(&object, 2.0f, 3u), 0.0f);
    CHECK_EQ(scene.time, 6.0f);
    scene.overrun = 20.0f;
    CHECK_EQ(renderer_object_timing_value_adjust_by_count(&object, 1.0f, 1u), 20.0f);
    CHECK_EQ(renderer_object_timing_value_adjust_by_count(&object, 1.0f, 3u), 4.0f);
    return lrr_test_result("animation_object_activity");
}
