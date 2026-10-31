// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGO_GAME_ACTIVITY_TIMEOUT_HPP
#define LEGO_GAME_ACTIVITY_TIMEOUT_HPP

#include <stdint.h>

typedef struct ActivityTimeoutState {
    uint32_t kind;
    uint32_t flags_f8;
    uint32_t flags_fb;
    uint32_t flags_fa;
    float timeout_threshold;
    float timeout_rate;
    float timeout_accumulator;
    int activity_slot;
    int activity_slot_value;
    int terrain_type;
    int terrain_blocked;
} ActivityTimeoutState;

typedef struct ActivityTimeoutHooks {
    float (*activity_limit_get)(void* context, void* entity);
    float (*timeout_scale_get)(void* context, void* entity);
    int (*entity_cell_get)(void* context, void* entity, int* x, int* y);
    int (*terrain_state_get)(void* context, int x, int y,
                             int* terrain_type, int* blocked);
    void (*activity_request)(void* context, void* entity,
                             unsigned int activity, int flags);
    void (*activity_refresh)(void* context, void* entity);
    void (*timeout_reset)(void* context, void* entity);
    void* context;
} ActivityTimeoutHooks;

static inline void activity_timeout_accumulate_and_raise(
    void* entity, ActivityTimeoutState* state,
    const ActivityTimeoutHooks* hooks, float delta)
{
    float rate;
    int x = 0;
    int y = 0;
    int terrain_type = 0;
    int blocked = 0;
    if (entity == 0 || state == 0 || hooks == 0) return;
    if ((state->flags_f8 & 0x400u) == 0 ||
        (state->kind == 3u && state->activity_slot_value != 5)) return;
    if (hooks->activity_limit_get != 0 &&
        hooks->activity_limit_get(hooks->context, entity) <
            state->timeout_threshold && (state->flags_fb & 0x20u) == 0) {
        rate = hooks->timeout_scale_get != 0
            ? hooks->timeout_scale_get(hooks->context, entity) * 0.01f : 0.0f;
        if (rate != 0.0f && (state->flags_f8 & 4u) == 0 &&
            rate * state->timeout_threshold <
                state->timeout_accumulator * 0.04f) {
            int valid_cell = hooks->entity_cell_get != 0 &&
                hooks->entity_cell_get(hooks->context, entity, &x, &y);
            int valid_terrain = valid_cell && hooks->terrain_state_get != 0 &&
                hooks->terrain_state_get(hooks->context, x, y,
                                         &terrain_type, &blocked);
            if (valid_terrain && !blocked && terrain_type != 9 &&
                state->activity_slot != state->activity_slot_value - 1) {
                state->timeout_accumulator = 0.0f;
                state->flags_fa |= 0x4000u;
                state->flags_f8 |= 0x20000000u;
                if (hooks->activity_request != 0)
                    hooks->activity_request(hooks->context, entity, 0x22u, 0);
                if (hooks->activity_refresh != 0)
                    hooks->activity_refresh(hooks->context, entity);
            }
        }
        state->timeout_accumulator += delta;
        return;
    }
    if (hooks->timeout_reset != 0)
        hooks->timeout_reset(hooks->context, entity);
}

#endif
