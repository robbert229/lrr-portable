// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_advisor — vector operations, the configured position and animation
 * names, and admission of a request without a loaded animation asset. */
#include "game/advisor.h"
#include "game/camera_control.h"
#include "game/lego_globs.h"
#include "test/lrr_test.h"

extern "C" void* __wrap_ddiscene_light_create(void*, unsigned long, float, float, float)
{
    return nullptr;
}

extern "C" void __wrap_ddiscene_object_pose_method_dispatch(void*, void*, float, float, float,
                                                            float, float, float)
{
}

extern "C" void __wrap_scene_render_lock_toggle(void*, int)
{
}

int main(void)
{
    float a[] = { 3.0f, 4.0f, 0.0f };
    const float b[] = { 0.0f, 0.0f, 2.0f };
    float out[3] = {};
    CHECK_NEAR(vec2_dist(a, b), 5.0f, 1e-6);
    CHECK_NEAR(vec_len(a), 5.0f, 1e-6);
    CHECK_EQ(vec_cross(out, a, b), &out[0]);
    CHECK_EQ(out[0], 8.0f);
    CHECK_EQ(out[1], -6.0f);
    CHECK_EQ(out[2], 0.0f);
    CHECK_EQ(vec_dot(out, a), 0.0f);
    vec_normalize(a);
    CHECK_NEAR(a[0], 0.6f, 1e-6);
    CHECK_NEAR(a[1], 0.8f, 1e-6);

    LegoCamera camera = {};
    advisor_scene_controller_initialize("Game", &camera, nullptr);
    int index = -1;
    CHECK_EQ(advisor_position_name_to_index("advisor_talkingame", &index), 1);
    CHECK_EQ(index, 20);
    CHECK_EQ(advisor_animation_name_to_index("ADVISOR_ANIM_TALK_TOP", &index), 1);
    CHECK_EQ(index, 10);
    CHECK_EQ(advisor_animation_name_to_index("missing", &index), 0);
    CHECK_EQ(index, 10);

    constexpr int objective = 0;
    advisor_position_record_initialize(objective, 0, 0x1a, 0, 0xc, 20.0f, 32.0f);
    advisor_action_position_and_mode_set(objective, 2, 10.0f, 16.0f);
    float x = 0.0f;
    float y = 0.0f;
    advisor_action_position_get(objective, &x, &y);
    CHECK_EQ(x, 20.0f);
    CHECK_EQ(y, 32.0f);

    s_context_mode = 0;
    CHECK_EQ(advisor_request_admit(objective, 0), 0);
    CHECK_EQ(global_flag_bit0_is_set(), 0u);
    s_context_mode = 1;
    CHECK_EQ(advisor_request_admit(1, 0), 0);
    CHECK_EQ(advisor_request_admit(objective, 1), 1);
    CHECK_EQ(global_flag_bit0_is_set(), 1u);
    CHECK_EQ(advisor_request_admit(objective, 0), 0);
    advisor_animation_end_cleanup();
    CHECK_EQ(global_flag_bit0_is_set(), 0u);
    return lrr_test_result("advisor");
}
