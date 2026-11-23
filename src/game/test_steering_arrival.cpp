// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_steering_arrival — a group reports completion after every follower
 * reaches its target. Each arrived follower becomes hidden. */
#include "game/steering.h"
#include "test/lrr_test.h"

extern "C" void __wrap_scene_render_lock_toggle(void* container, int hide)
{
    *static_cast<bool*>(container) = hide != 0;
}

int main(void)
{
    SteeringGroup group = {};
    CHECK_EQ(callback_operation_run_and_return_status(&group), 1);

    bool nearHidden = false;
    bool farHidden = false;
    SteeringItem nearFollower = {}, farFollower = {};
    nearFollower.container = &nearHidden;
    farFollower.container = &farHidden;
    const SteeringVector3 target = { 3.0f, 4.0f, 5.0f };
    ptrstack_push(&group, &nearFollower);
    ptrstack_push(&group, &farFollower);
    scene_object_list_apply_second_position_callback(&group, &target);
    nearFollower.position = target;
    farFollower.position = { 3.0f, 4.0f, 6.0f };

    CHECK_EQ(callback_operation_run_and_return_status(&group), 0);
    CHECK_EQ(nearHidden, true);
    CHECK_EQ(farHidden, false);
    scene_object_list_apply_position_callback(&group, &target);
    CHECK_EQ(callback_operation_run_and_return_status(&group), 1);
    CHECK_EQ(farHidden, true);
    return lrr_test_result("steering_arrival");
}
