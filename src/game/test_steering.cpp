// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_steering — the GAME-016 unit (game/steering.c, issue #491): item and
 * group creation, the follower stack, the pitch and turn steps, one physics
 * step, the wander, tuning and placement walks, following a target and the
 * arrival check.
 *
 * The inputs are small, exactly representable values, so most checks
 * compare exactly; the steps through sqrt and the matrix helpers use a
 * tolerance. */
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "game/steering.h"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static int near(float a, float b)
{
    return fabsf(a - b) < 1e-4f;
}

static SteeringVector3 vec(float x, float y, float z)
{
    SteeringVector3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

typedef struct Visit {
    SteeringItem* seen[4];
    int count;
} Visit;

static void visit(SteeringGroup* group, SteeringItem* item, void* data)
{
    Visit* v = (Visit*)data;
    (void)group;
    v->seen[v->count++] = item;
}

int main(void)
{
    /* The float part of the layout is the original's on every host. */
    CHECK(offsetof(SteeringItem, position) == 0x0c, "position at +0x0c");
    CHECK(offsetof(SteeringItem, heading) == 0x18, "heading at +0x18");
    CHECK(offsetof(SteeringItem, turnRate) == 0x30, "turnRate at +0x30");
    CHECK(offsetof(SteeringItem, baseWanderRadius) == 0x54,
          "baseWanderRadius at +0x54");
    CHECK(offsetof(SteeringItem, transform) == 0x58, "transform at +0x58");
    CHECK(offsetof(SteeringItem, container) == 0x98, "container at +0x98");
    if (sizeof(void*) == 4) {
        CHECK(sizeof(SteeringItem) == 0xa0, "an item is 0xa0 bytes on Win32");
        CHECK(sizeof(SteeringGroup) == 0x28, "a group is 0x28 bytes on Win32");
    }

    /* scene_node_record_create: the triples, the tuning pairs, the pins. */
    {
        SteeringVector3 position = vec(1.0f, 2.0f, 3.0f);
        SteeringVector3 target = vec(4.0f, 5.0f, 6.0f);
        int container = 0;
        SteeringItem* item = scene_node_record_create(&position, &target, 0.25f,
                                                      2.0f, 3.0f, 4.0f,
                                                      &container);
        CHECK(item != NULL, "the item allocates");
        CHECK(item->position.x == 1.0f && item->position.z == 3.0f,
              "the first argument is the position");
        CHECK(item->target.x == 4.0f && item->target.z == 6.0f,
              "the second argument is the target");
        CHECK(item->heading.x == 0.0f && item->heading.y == 0.0f
                  && item->heading.z == 1.0f,
              "the heading starts along +z");
        CHECK(item->yaw == 0.0f && item->pitch == 0.0f && item->roll == 0.0f
                  && item->turnRate == 0.0f && item->wanderTime == 0.0f,
              "the angles and timers start at zero");
        CHECK(item->turnStep == 0.25f && item->baseTurnStep == 0.25f,
              "turnStep lands in both copies");
        CHECK(item->speed == 2.0f && item->baseSpeed == 2.0f,
              "speed lands in both copies");
        CHECK(item->wanderRadius == 3.0f && item->baseWanderRadius == 3.0f,
              "wanderRadius lands in both copies");
        CHECK(item->wanderPeriod == 4.0f && item->baseWanderPeriod == 4.0f,
              "wanderPeriod lands in both copies");
        CHECK(item->container == &container, "the container is kept");
        optional_resource_destroy(item);
        optional_resource_destroy(NULL);

        item = scene_node_record_create(NULL, NULL, 0.0f, 0.0f, 0.0f, 0.0f,
                                        NULL);
        CHECK(item != NULL && item->position.x == 0.0f
                  && item->target.z == 0.0f,
              "NULL triples are the origin");
        optional_resource_destroy(item);
    }

    /* The group, its flags and the stack. */
    {
        SteeringVector3 position = vec(1.0f, 0.0f, 0.0f);
        SteeringGroup* group = callback_state_record_create(&position, NULL,
                                                            0.1f, 1.0f, 2.0f,
                                                            NULL);
        SteeringItem* a = scene_node_record_create(NULL, NULL, 0, 0, 0, 0, NULL);
        SteeringItem* b = scene_node_record_create(NULL, NULL, 0, 0, 0, 0, NULL);
        Visit v;
        CHECK(group != NULL && group->leader != NULL, "the group has a leader");
        CHECK(group->hasPosition == 1 && group->hasTarget == 0,
              "the flags record which triples were given");
        CHECK(group->head == NULL && group->count == 0, "the stack is empty");
        CHECK(group->leader->wanderPeriod == 0.0f,
              "the leader's wander period is 0");

        ptrstack_push(group, a);
        ptrstack_push(group, b);
        ptrstack_push(group, NULL);
        ptrstack_push(NULL, a);
        CHECK(group->count == 2 && group->head == b && b->next == a,
              "push links the item in front");
        memset(&v, 0, sizeof v);
        ptrstack_for_each(group, visit, &v);
        CHECK(v.count == 2 && v.seen[0] == b && v.seen[1] == a,
              "the walk runs from the head");
        CHECK(ptrstack_pop(group) == b && group->count == 1
                  && group->head == a,
              "pop answers the head");
        ptrstack_push(group, b);

        three_value_record_copy_and_mark_ready(group, &position);
        CHECK(group->hasTarget == 1 && group->leader->target.x == 1.0f,
              "the leader's target sets the flag");
        object_vector3_set(group, 0.5f, 6.0f, 7.0f);
        CHECK(group->leader->turnStep == 0.5f && group->leader->speed == 6.0f
                  && group->leader->wanderRadius == 7.0f,
              "the leader's tuning");

        global_vector_set_and_notify(group, 0.2f, 3.0f, 4.0f, 5.0f);
        CHECK(a->turnStep == 0.2f && a->baseTurnStep == 0.2f
                  && b->speed == 3.0f && b->baseSpeed == 3.0f
                  && a->wanderRadius == 4.0f && b->baseWanderPeriod == 5.0f,
              "every follower takes the tuning, current and base");
        CHECK(group->leader->turnStep == 0.5f, "the leader is not a follower");

        scene_object_list_apply_float_callback(group, 2.0f);
        CHECK(a->turnStep >= 0.1f - 1e-6f && a->turnStep <= 0.3f + 1e-6f
                  && a->wanderPeriod >= 2.5f - 1e-5f
                  && a->wanderPeriod <= 7.5f + 1e-5f,
              "the spread stays within half of the base either way");
        CHECK(a->baseTurnStep == 0.2f, "the spread keeps the base");

        {
            SteeringVector3 p = vec(7.0f, 8.0f, 9.0f);
            SteeringVector3 t = vec(-1.0f, -2.0f, -3.0f);
            scene_object_list_apply_position_callback(group, &p);
            scene_object_list_apply_second_position_callback(group, &t);
            CHECK(a->position.y == 8.0f && b->position.z == 9.0f
                      && a->target.x == -1.0f && b->target.z == -3.0f,
                  "the position and target walks reach every follower");
            scene_node_position_set(group, &p);
            scene_node_position_set(NULL, &p);
            scene_node_position_set(group, NULL);
            CHECK(group->leader->position.x == 7.0f && group->hasPosition == 1,
                  "the leader's position");

            CHECK(callback_operation_run_and_return_status(group) == 0,
                  "followers away from their targets have not arrived");
            a->position = a->target;
            b->position = b->target;
            CHECK(callback_operation_run_and_return_status(group) == 1,
                  "followers on their targets have arrived");
        }

        callback_state_list_dispose(group);
    }

    /* direction_driven_heading_angle_step: climb, dive and damping. */
    {
        SteeringItem item;
        SteeringVector3 up = vec(0.0f, 2.0f, 0.0f);
        SteeringVector3 down = vec(0.0f, -2.0f, 0.0f);
        SteeringVector3 level = vec(0.0f, 0.5f, 0.0f);
        memset(&item, 0, sizeof item);
        item.turnStep = 0.5f;
        direction_driven_heading_angle_step(&item, &up);
        CHECK(item.pitch == 0.5f, "a climb raises the pitch by the step");
        direction_driven_heading_angle_step(&item, &up);
        CHECK(near(item.pitch, 0.8f), "the pitch clamps at 0.8");
        direction_driven_heading_angle_step(&item, &level);
        CHECK(near(item.pitch, 0.76f), "level flight damps the pitch by 0.95");
        item.pitch = -0.6f;
        direction_driven_heading_angle_step(&item, &down);
        CHECK(near(item.pitch, -0.8f), "a dive clamps at -0.8");
    }

    /* kind_two_steering_torque_apply: a target to the side turns the item. */
    {
        SteeringItem item;
        SteeringVector3 dir = vec(1.0f, 3.0f, 0.0f);
        memset(&item, 0, sizeof item);
        item.heading = vec(0.0f, 0.0f, 2.0f);
        item.turnStep = 0.1f;
        kind_two_steering_torque_apply(&item, &dir);
        CHECK(item.heading.z == 1.0f && item.heading.y == 0.0f,
              "the heading is flattened and normalised");
        CHECK(dir.x == 0.0f && dir.y == -1.0f && dir.z == 0.0f,
              "the direction becomes cross(direction, heading)");
        /* dot = 0: turn = 1 * 0.1 * 10 * 0.5 = 0.5; y < 0.01:
         * turnRate = (9 * 0 - 0.5) * 0.1 = -0.05. */
        CHECK(near(item.turnRate, -0.05f) && near(item.yaw, -0.05f)
                  && near(item.roll, 0.45f),
              "the turn rate, yaw and roll follow the turn");
    }

    /* steering_physics_step and set_vector_and_notify_listeners: an item
     * that already faces its target moves along +z by speed * time. */
    {
        SteeringItem* item;
        SteeringVector3 position = vec(0.0f, 0.0f, 0.0f);
        SteeringVector3 target = vec(0.0f, 0.0f, 10.0f);
        item = scene_node_record_create(&position, &target, 0.1f, 2.0f, 0.0f,
                                        0.0f, NULL);
        set_vector_and_notify_listeners(item, 0.5f);
        CHECK(near(item->heading.z, 1.0f) && near(item->position.z, 1.0f)
                  && near(item->position.x, 0.0f),
              "one step moves speed * time along the heading");
        CHECK(near(item->transform[14], 1.0f) && near(item->transform[10], 1.0f),
              "the transform holds the heading and the new position");
        optional_resource_destroy(item);
    }

    /* steering_item_follow_target: straight at the old target, arriving
     * within one step, and the target is kept. */
    {
        SteeringItem* item;
        SteeringVector3 position = vec(0.0f, 0.0f, 0.0f);
        SteeringVector3 target = vec(3.0f, 0.0f, 4.0f);
        float elapsed = 1.0f;
        item = scene_node_record_create(&position, &target, 0.1f, 1.0f, 0.0f,
                                        0.0f, NULL);
        steering_item_follow_target(NULL, item, &elapsed);
        CHECK(near(item->position.x, 0.6f) && near(item->position.z, 0.8f),
              "the item moves one step straight at the target");
        CHECK(item->target.x == 3.0f && item->target.z == 4.0f,
              "the target is restored");
        CHECK(item->transform[12] == item->position.x
                  && item->transform[14] == item->position.z,
              "the transform's translation is the position");
        item->speed = 100.0f;
        steering_item_follow_target(NULL, item, &elapsed);
        CHECK(item->position.x == 3.0f && item->position.z == 4.0f,
              "within one step the item lands on its target");
        optional_resource_destroy(item);
    }

    /* scene_object_list_update_transforms: followers wander around the
     * leader once a wander period has passed. */
    {
        SteeringVector3 lead = vec(10.0f, 0.0f, 0.0f);
        SteeringGroup* group = callback_state_record_create(&lead, &lead, 0.1f,
                                                            0.0f, 1.0f, NULL);
        SteeringItem* follower = scene_node_record_create(NULL, NULL, 0.1f, 0.0f,
                                                          0.0f, 1.0f, NULL);
        float r = random_unit_float();
        CHECK(r >= 0.0f && r <= 1.0f, "random_unit_float is in [0, 1]");
        ptrstack_push(group, follower);
        scene_object_list_update_transforms(group, 0.5f);
        /* time += 1 / 0.5: two periods pass; radius 0 lands on the leader. */
        CHECK(near(follower->wanderTime, 2.0f), "the wander time advances");
        CHECK(near(follower->target.x, 10.0f) && near(follower->target.y, 0.0f),
              "the new target is around the leader's position");
        callback_state_list_dispose(group);
    }

    if (s_failures != 0) {
        printf("steering_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("steering_test: PASS\n");
    return 0;
}
