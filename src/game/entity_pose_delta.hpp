// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGO_GAME_ENTITY_POSE_DELTA_HPP
#define LEGO_GAME_ENTITY_POSE_DELTA_HPP

#include <math.h>
#include <stdint.h>

typedef struct EntityPoseDeltaState {
    int kind;
    uint32_t flags_f8;
    float target_x;
    float target_y;
    float origin_x;
    float origin_y;
    float origin_z;
    float direction_x;
    float direction_y;
    float direction_z;
    float distance;
} EntityPoseDeltaState;

typedef struct EntityPoseDeltaHooks {
    int (*origin_get)(void* context, int kind, float* x, float* y, float* z);
    void (*direction_apply)(void* context, int kind, float x, float y,
                            float z);
    void (*position_apply)(void* context, int kind, float x, float y,
                           int completion_kind, void* callback_data);
    void* context;
    void* callback_data;
} EntityPoseDeltaHooks;

static inline int entity_subobject_pose_delta_normalize_and_dispatch(
    EntityPoseDeltaState* state, const EntityPoseDeltaHooks* hooks,
    float target_x, float target_y)
{
    float dx;
    float dy;
    float length;
    float current_x;
    float current_y;
    float current_z;
    if (state == 0 || hooks == 0) return 0;
    if (hooks->origin_get == 0 ||
        !hooks->origin_get(hooks->context, state->kind,
                           &current_x, &current_y, &current_z)) return 0;

    /* The retail record caches its source position at +0x2a0 and its
     * normalized planar direction/length at +0x2ac/+0x2b8. It refreshes
     * those fields only when the requested pair differs from the current
     * pose; an unchanged pair still reaches the dispatch below with the
     * previously cached direction. */
    if (target_x != current_x || target_y != current_y) {
        state->origin_x = current_x;
        state->origin_y = current_y;
        state->origin_z = current_z;
        dx = target_x - current_x;
        dy = target_y - current_y;
        /* The decompile computes -origin_z and then explicitly overwrites
         * the z component with zero before measuring the vector. */
        state->direction_z = 0.0f - current_z;
        state->direction_z = 0.0f;
        length = (float)sqrt(dx * dx + dy * dy +
                             state->direction_z * state->direction_z);
        state->distance = length;
        state->direction_x = dx / length;
        state->direction_y = dy / length;
        state->direction_z /= length;
    }

    /* The three record families apply the pose flag differently. Kind 1
     * reverses the full vector, kind 2 reverses its planar pair, and kind 3
     * consumes the cached pair unchanged. */
    dx = state->direction_x;
    dy = state->direction_y;
    length = state->direction_z;
    if (state->kind == 1 && (state->flags_f8 & 2u) != 0u) {
        dx = -dx;
        dy = -dy;
        length = -length;
    }
    else if (state->kind == 2 && (state->flags_f8 & 2u) != 0u) {
        dx = -state->direction_x;
        dy = -state->direction_y;
    }
    if (hooks->direction_apply != 0 &&
        (state->kind == 1 || state->kind == 2 || state->kind == 3))
        hooks->direction_apply(hooks->context, state->kind,
            dx, dy, length);
    if (hooks->position_apply != 0 &&
        (state->kind == 1 || state->kind == 2 || state->kind == 3))
        hooks->position_apply(hooks->context, state->kind, target_x, target_y,
                              state->kind == 2 ? 1 : 0,
                              hooks->callback_data);
    return 1;
}

typedef struct EntityBehaviorCallbackState {
    int kind;
    uint32_t flags_fa;
    void* visual;
} EntityBehaviorCallbackState;

typedef struct EntityBehaviorCallbackHooks {
    void (*callback_schedule)(void* context, int callback_kind,
                              void* entity);
    uint32_t (*visual_duration_get)(void* context, void* visual);
    float (*visual_time_get)(void* context, void* visual);
    void* context;
} EntityBehaviorCallbackHooks;

static inline void entity_behavior_callback_tick(
    EntityBehaviorCallbackState* state, const EntityBehaviorCallbackHooks* hooks,
    void* entity)
{
    if (state == 0 || hooks == 0) return;
    if (state->kind == 3 && hooks->callback_schedule != 0)
        hooks->callback_schedule(hooks->context, 0, entity);
    if ((state->kind == 8 || state->kind == 13) &&
        (state->flags_fa & 0x10000u) != 0u && state->visual != 0 &&
        hooks->visual_duration_get != 0 && hooks->visual_time_get != 0 &&
        hooks->callback_schedule != 0) {
        const float duration = (float)hooks->visual_duration_get(
            hooks->context, state->visual);
        if (duration - 75.0f < hooks->visual_time_get(
                hooks->context, state->visual))
            hooks->callback_schedule(hooks->context, 1, entity);
    }
}

#endif
