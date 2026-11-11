// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-016 — original translation unit (text 0040f6c0-004102b0; finding 505).
 * Steered groups: items that turn towards and move at their targets, and
 * a leader whose position the followers wander around.
 *
 * Rebuilt in the original's shape (issue #491, ADR 0005): one file for the
 * unit, its 30 functions in original order and the C-compatible subset, so
 * the matching build compiles it as C. The native port compiles it as C++.
 * The exported functions keep the port's names, apart from
 * random_unit_float (the port called it set_global_scalar_to_one). The seven
 * ptrstack_for_each callbacks are new: six are reached only through the
 * unit's walks, and GAME-023 passes steering_item_follow_target. They are
 * external, not static: MSVC emits a static function after its first
 * caller, and the original has them in source order. The Maths.h inline
 * helpers are in steering_maths.h (finding 544). */
#include <math.h>
#include <stdlib.h>

#include "steering.h"
#include "gods98/maths.h"
#include "gods98/record.h"
#include "crt/vc5names.h"

/* The unit's .rdata, in original order (0049e148-0049e16c). MSVC emits
 * each float literal once per object, in order of first use, including
 * those no instruction reads: 0049e154 is the double's alignment and the
 * 2.0 at 0049e16c is folded into fadd st, st. */
// GLOBAL: LEGORR 0x0049e148
static const float s_steering_zero = 0.0f;
// GLOBAL: LEGORR 0x0049e14c
static const float s_steering_one = 1.0f;
// GLOBAL: LEGORR 0x0049e150
static const float s_steering_half = 0.5f;
// GLOBAL: LEGORR 0x0049e158
static const double s_steering_level = 0.01;
// GLOBAL: LEGORR 0x0049e160
static const float s_steering_tenth = 0.1f;
// GLOBAL: LEGORR 0x0049e164
static const float s_steering_epsilon = 0.00001f;
// GLOBAL: LEGORR 0x0049e168
static const float s_steering_min_elapsed = 0.001f;

/* The unit's initialised .data. */
// GLOBAL: LEGORR 0x004a1ee8
static float s_steering_max_pitch = 0.8f;
// GLOBAL: LEGORR 0x004a1eec
static float s_steering_pitch_damping = 0.95f;
// GLOBAL: LEGORR 0x004a1ef0
static float s_steering_roll_gain = 9.0f;
// GLOBAL: LEGORR 0x004a1ef4
static float s_steering_turn_gain = 10.0f;
// GLOBAL: LEGORR 0x004a1ef8
static float s_steering_climb_threshold = 1.0f;

/* global_vector_set_and_notify's arguments, for its walk (common). */
typedef struct SteeringTuning {
    float turnStep;
    float speed;
    float wanderRadius;
    float wanderPeriod;
} SteeringTuning;

// GLOBAL: LEGORR 0x00557850
SteeringTuning steeringTuning;

#include "steering_maths.h"

// FUNCTION: LEGORR 0x0040f6c0
SteeringItem* scene_node_record_create(const SteeringVector3* position,
                                       const SteeringVector3* target,
                                       float turnStep, float speed,
                                       float wanderRadius, float wanderPeriod,
                                       void* container)
{
    SteeringVector3 origin = { 0.0f, 0.0f, 0.0f };
    SteeringItem* item = (SteeringItem*)malloc(sizeof(SteeringItem));
    if (item == NULL) return NULL;

    item->turnRate = 0.0f;
    item->roll = 0.0f;
    item->pitch = 0.0f;
    item->yaw = 0.0f;
    item->heading.y = 0.0f;
    item->heading.x = 0.0f;
    item->heading.z = 1.0f;

    if (target != NULL) item->target = *target;
    else item->target = origin;
    if (position != NULL) item->position = *position;
    else item->position = origin;

    item->wanderTime = 0.0f;
    item->turnStep = item->baseTurnStep = turnStep;
    item->speed = item->baseSpeed = speed;
    item->wanderRadius = item->baseWanderRadius = wanderRadius;
    item->wanderPeriod = item->baseWanderPeriod = wanderPeriod;
    item->container = container;
    return item;
}

// FUNCTION: LEGORR 0x0040f7b0
void optional_resource_destroy(void* item)
{
    if (item != NULL) free(item);
}

// FUNCTION: LEGORR 0x0040f7d0
SteeringGroup* callback_state_record_create(const SteeringVector3* position,
                                            const SteeringVector3* target,
                                            float turnStep, float speed,
                                            float wanderRadius,
                                            void* container)
{
    SteeringGroup* group = (SteeringGroup*)malloc(sizeof(SteeringGroup));
    if (group == NULL) return NULL;

    group->leader = scene_node_record_create(position, target, turnStep, speed,
                                             wanderRadius, 0.0f, container);
    if (position != NULL) group->hasPosition = 1;
    else group->hasPosition = 0;
    if (target != NULL) group->hasTarget = 1;
    else group->hasTarget = 0;
    group->head = NULL;
    group->count = 0;
    return group;
}

// FUNCTION: LEGORR 0x0040f850
void callback_state_list_dispose(SteeringGroup* group)
{
    SteeringItem* item;

    item = ptrstack_pop(group);
    while (item != NULL) {
        optional_resource_destroy(item);
        item = ptrstack_pop(group);
    }
    optional_resource_destroy(group->leader);
    free(group);
}

// FUNCTION: LEGORR 0x0040f890
void direction_driven_heading_angle_step(SteeringItem* item,
                                         const SteeringVector3* direction)
{
    if (direction->y > s_steering_climb_threshold) {
        item->pitch += item->turnStep;
        if (item->pitch > s_steering_max_pitch) item->pitch = s_steering_max_pitch;
    } else if (direction->y < -s_steering_climb_threshold) {
        item->pitch -= item->turnStep;
        if (item->pitch < -s_steering_max_pitch) item->pitch = -s_steering_max_pitch;
    } else {
        item->pitch *= s_steering_pitch_damping;
    }
}

// FUNCTION: LEGORR 0x0040f920
void kind_two_steering_torque_apply(SteeringItem* item,
                                    SteeringVector3* direction)
{
    float turn;

    direction->y = 0.0f;
    item->heading.y = 0.0f;
    steering_vector3d_normalise(&item->heading);
    steering_vector3d_normalise(direction);
    turn = (s_steering_one - steering_vector3d_dot(&item->heading, direction)) * item->turnStep * s_steering_turn_gain * s_steering_half;
    steering_vector3d_cross(direction, direction, &item->heading);
    if (direction->y > s_steering_level) {
        item->turnRate = (s_steering_roll_gain * item->turnRate + turn) * s_steering_tenth;
    } else if (direction->y < s_steering_level) {
        item->turnRate = (s_steering_roll_gain * item->turnRate - turn) * s_steering_tenth;
    }
    item->yaw += item->turnRate;
    item->roll = -(s_steering_roll_gain * item->turnRate);
}

// FUNCTION: LEGORR 0x0040fb00
void matrix_rotation_compose_xyz(SteeringItem* item)
{
    float rotY[16], rotZ[16], rotX[16], rotXY[16];

    matrix4_set_rotation_x(rotX, item->pitch);
    matrix4_set_rotation_y(rotY, item->yaw);
    matrix4_set_rotation_z(rotZ, item->roll);
    matrix4_multiply(rotXY, rotY, rotX);
    matrix4_multiply(item->transform, rotXY, rotZ);
}

// FUNCTION: LEGORR 0x0040fb90
void steering_physics_step(SteeringGroup* group, SteeringItem* item,
                           void* data)
{
    SteeringVector3 direction, move;
    float elapsed = *(float*)data;
    float translation[16], rotation[16];

    steering_vector3d_subtract(&direction, &item->target, &item->position);
    if (steering_vector3d_modulus(&direction) < s_steering_epsilon) {
        d3drm_vector_random(&direction.x);
    }

    direction_driven_heading_angle_step(item, &direction);
    kind_two_steering_torque_apply(item, &direction);
    matrix_rotation_compose_xyz(item);

    item->heading.x = item->transform[8];
    item->heading.y = item->transform[9];
    item->heading.z = item->transform[10];
    steering_vector3d_scale(&move, &item->heading, item->speed * elapsed);
    steering_vector3d_add(&item->position, &item->position, &move);

    matrix4_identity_translation_set(translation, &item->position.x);
    matrix4_copy(rotation, item->transform);
    matrix4_multiply(item->transform, translation, rotation);
}

// FUNCTION: LEGORR 0x0040fcc0
void vector_scale_components_in_place(SteeringVector3* v, float sx, float sy,
                                      float sz)
{
    if (v != NULL) {
        d3drm_vector_random(&v->x);
        v->x = sx * v->x;
        v->y = sy * v->y;
        v->z = sz * v->z;
    }
}

/* Every wanderPeriod time units, a new target around the leader. */
// FUNCTION: LEGORR 0x0040fd00
void steering_item_wander(SteeringGroup* group, SteeringItem* item,
                                 void* data)
{
    SteeringVector3 offset;
    float elapsed = *(float*)data;
    float time = item->wanderTime;
    int before = (int)(item->wanderTime / item->wanderPeriod);

    if (elapsed == s_steering_zero) elapsed = s_steering_min_elapsed;
    time += s_steering_one / elapsed;
    if ((int)(time / item->wanderPeriod) - before > 0) {
        vector_scale_components_in_place(&offset, item->wanderRadius,
                                         item->wanderRadius, item->wanderRadius);
        steering_vector3d_add(&item->target, &group->leader->position, &offset);
    }
    item->wanderTime = time;
}

// FUNCTION: LEGORR 0x0040fdb0
void three_value_record_copy_and_mark_ready(SteeringGroup* group,
                                            const SteeringVector3* target)
{
    group->leader->target = *target;
    group->hasTarget = 1;
}

// FUNCTION: LEGORR 0x0040fde0
void set_vector_and_notify_listeners(SteeringItem* item, float elapsed)
{
    steering_physics_step(NULL, item, &elapsed);
}

// FUNCTION: LEGORR 0x0040fe00
void scene_object_list_update_transforms(SteeringGroup* group, float elapsed)
{
    set_vector_and_notify_listeners(group->leader, elapsed);
    ptrstack_for_each(group, steering_physics_step, &elapsed);
    ptrstack_for_each(group, steering_item_wander, &elapsed);
}

// FUNCTION: LEGORR 0x0040fe40
void ptrstack_push(SteeringGroup* group, SteeringItem* item)
{
    if (group != NULL && item != NULL) {
        item->next = group->head;
        group->head = item;
        group->count++;
    }
}

// FUNCTION: LEGORR 0x0040fe60
SteeringItem* ptrstack_pop(SteeringGroup* group)
{
    SteeringItem* item;

    if (group != NULL && group->count != 0) {
        item = group->head;
        group->head = item->next;
        group->count--;
        return item;
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0040fe90
void ptrstack_for_each(SteeringGroup* group, SteeringItemCallback callback,
                       void* data)
{
    SteeringItem* item;

    for (item = group->head; item != NULL; item = item->next) {
        callback(group, item, data);
    }
}

// FUNCTION: LEGORR 0x0040fec0
float random_unit_float(void)
{
    return random_float_range(0.0f, 1.0f);
}

/* Each current tuning value becomes its base, spread by up to 1 / spread
 * of itself either way. */
// FUNCTION: LEGORR 0x0040fed0
void steering_item_spread_tuning(SteeringGroup* group,
                                        SteeringItem* item, void* data)
{
    float spread = *(float*)data;
    float r = random_unit_float();
    float scale = s_steering_one / spread;
    float factor = (2.0f * r) - s_steering_one;

    item->turnStep = scale * factor * item->baseTurnStep + item->baseTurnStep;
    item->wanderRadius = scale * factor * item->baseWanderRadius + item->baseWanderRadius;
    item->wanderPeriod = scale * factor * item->baseWanderPeriod + item->baseWanderPeriod;
}

// FUNCTION: LEGORR 0x0040ff50
void scene_object_list_apply_float_callback(SteeringGroup* group,
                                            float spread)
{
    ptrstack_for_each(group, steering_item_spread_tuning, &spread);
}

// FUNCTION: LEGORR 0x0040ff70
void object_vector3_set(SteeringGroup* group, float turnStep, float speed,
                        float wanderRadius)
{
    group->leader->turnStep = turnStep;
    group->leader->speed = speed;
    group->leader->wanderRadius = wanderRadius;
}

// FUNCTION: LEGORR 0x0040ff90
void steering_item_set_tuning(SteeringGroup* group, SteeringItem* item,
                                     void* data)
{
    item->baseTurnStep = steeringTuning.turnStep;
    item->turnStep = steeringTuning.turnStep;
    item->baseSpeed = steeringTuning.speed;
    item->speed = steeringTuning.speed;
    item->baseWanderRadius = steeringTuning.wanderRadius;
    item->wanderRadius = steeringTuning.wanderRadius;
    item->baseWanderPeriod = steeringTuning.wanderPeriod;
    item->wanderPeriod = steeringTuning.wanderPeriod;
}

// FUNCTION: LEGORR 0x0040ffe0
void global_vector_set_and_notify(SteeringGroup* group, float turnStep,
                                  float speed, float wanderRadius,
                                  float wanderPeriod)
{
    steeringTuning.turnStep = turnStep;
    steeringTuning.speed = speed;
    steeringTuning.wanderRadius = wanderRadius;
    steeringTuning.wanderPeriod = wanderPeriod;
    ptrstack_for_each(group, steering_item_set_tuning, NULL);
}

// FUNCTION: LEGORR 0x00410020
void scene_node_position_set(SteeringGroup* group,
                             const SteeringVector3* position)
{
    if (group != NULL && position != NULL) {
        group->hasPosition = 1;
        group->leader->position = *position;
    }
}

// FUNCTION: LEGORR 0x00410050
void steering_item_set_position(SteeringGroup* group,
                                       SteeringItem* item, void* data)
{
    item->position = *(SteeringVector3*)data;
}

// FUNCTION: LEGORR 0x00410070
void scene_object_list_apply_position_callback(SteeringGroup* group,
                                              const SteeringVector3* position)
{
    ptrstack_for_each(group, steering_item_set_position, (void*)position);
}

// FUNCTION: LEGORR 0x00410090
void steering_item_set_target(SteeringGroup* group, SteeringItem* item,
                                     void* data)
{
    item->target = *(SteeringVector3*)data;
}

// FUNCTION: LEGORR 0x004100b0
void scene_object_list_apply_second_position_callback(
    SteeringGroup* group, const SteeringVector3* target)
{
    ptrstack_for_each(group, steering_item_set_target, (void*)target);
}

// FUNCTION: LEGORR 0x004100d0
void steering_item_follow_target(SteeringGroup* group, SteeringItem* item,
                                 void* data)
{
    SteeringVector3 dir, position, target;
    float elapsed = *(float*)data;
    float len, step;

    target = item->target;
    position = item->position;
    steering_physics_step(group, item, data);

    steering_vector3d_subtract(&dir, &target, &position);
    step = item->speed * elapsed;
    len = steering_vector3d_modulus(&dir);
    if (len < step) {
        item->position = item->target;
    } else {
        steering_vector3d_scale(&dir, &dir, s_steering_one / len);
        steering_vector3d_scale(&dir, &dir, step);
        steering_vector3d_add(&item->position, &position, &dir);
    }
    item->target = target;
    item->transform[12] = item->position.x;
    item->transform[13] = item->position.y;
    item->transform[14] = item->position.z;
}

/* Clears *data unless the item is at its target; hides an arrived item. */
// FUNCTION: LEGORR 0x00410230
void steering_item_arrived(SteeringGroup* group, SteeringItem* item,
                                  void* data)
{
    if (item->position.x == item->target.x &&
        item->position.y == item->target.y &&
        item->position.z == item->target.z) {
        scene_render_lock_toggle(item->container, 1);
        return;
    }
    *(int*)data = 0;
}

// FUNCTION: LEGORR 0x00410280
int callback_operation_run_and_return_status(SteeringGroup* group)
{
    int arrived = 1;

    ptrstack_for_each(group, steering_item_arrived, &arrived);
    return arrived;
}
