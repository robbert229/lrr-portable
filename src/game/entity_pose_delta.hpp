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
    float length;
    if (state == 0 || hooks == 0) return 0;
    state->target_x = target_x;
    state->target_y = target_y;
    if (hooks->origin_get == 0 ||
        !hooks->origin_get(hooks->context, state->kind,
                           &state->origin_x, &state->origin_y,
                           &state->origin_z)) return 0;
    state->direction_x = target_x - state->origin_x;
    state->direction_y = target_y - state->origin_y;
    state->direction_z = -state->origin_z;
    state->direction_z = 0.0f;
    length = (float)sqrt(state->direction_x * state->direction_x +
                         state->direction_y * state->direction_y);
    if (length > 0.0f) {
        state->distance = length;
        state->direction_x /= length;
        state->direction_y /= length;
        state->direction_z = 0.0f;
    }
    else {
        state->distance = 0.0f;
        state->direction_x = 0.0f;
        state->direction_y = 0.0f;
    }
    if ((state->flags_f8 & 2u) != 0u) {
        state->direction_x = -state->direction_x;
        state->direction_y = -state->direction_y;
        state->direction_z = -state->direction_z;
    }
    if (hooks->direction_apply != 0 &&
        (state->kind == 1 || state->kind == 2 || state->kind == 3))
        hooks->direction_apply(hooks->context, state->kind,
            state->direction_x, state->direction_y, state->direction_z);
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
