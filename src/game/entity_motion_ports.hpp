// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGO_GAME_ENTITY_MOTION_PORTS_HPP
#define LEGO_GAME_ENTITY_MOTION_PORTS_HPP

#include <math.h>
#include <stdint.h>

typedef struct EntityAdjacentRouteHooks {
    int (*candidate_reject)(void* context, void* entity,
                            const int cell[2], int* route_cookie);
    int (*route_find)(void* context, void* entity, const int start[2],
                      const int goal[2], int route_cookie,
                      int* x_path, int* y_path, unsigned int* count);
    void* context;
} EntityAdjacentRouteHooks;

static inline int entity_adjacent_position_candidate_find(
    void* entity, const int candidate[2], int output[2],
    const EntityAdjacentRouteHooks* hooks)
{
    int cookie = 0;
    int x_path[256];
    int y_path[256];
    unsigned int count = 0;
    int reject;
    if (entity == 0 || candidate == 0 || output == 0 || hooks == 0 ||
        hooks->candidate_reject == 0 || hooks->route_find == 0) return 0;
    reject = hooks->candidate_reject(hooks->context, entity, candidate,
                                     &cookie);
    if (reject != 0) return 0;
    if (!hooks->route_find(hooks->context, entity, candidate, 0, cookie,
                           x_path, y_path, &count) || count == 0 ||
        count > 256) {
        return 0;
    }
    output[0] = x_path[count - 1];
    output[1] = y_path[count - 1];
    return 1;
}

typedef struct EntityPoseOrientationHooks {
    int (*position_get)(void* context, void* entity, float* x, float* y);
    int (*route_position_resolve)(void* context, void* entity,
                                  float input_x, float input_y,
                                  float* output_x, float* output_y);
    void (*scene_position_set)(void* context, void* entity,
                               float x, float y, int callback_kind);
    void (*entity_prepare)(void* context, void* entity);
    void* context;
} EntityPoseOrientationHooks;

static inline int entity_pose_orientation_dispatch(
    int kind, void* entity, const EntityPoseOrientationHooks* hooks)
{
    float x = 0.0f;
    float y = 0.0f;
    if (entity == 0 || hooks == 0) return 0;
    if (hooks->entity_prepare != 0)
        hooks->entity_prepare(hooks->context, entity);
    if (kind == 2) {
        float resolved_x;
        float resolved_y;
        if (hooks->position_get == 0 || hooks->route_position_resolve == 0 ||
            !hooks->position_get(hooks->context, entity, &x, &y) ||
            !hooks->route_position_resolve(hooks->context, entity, x, y,
                                           &resolved_x, &resolved_y))
            return 0;
        if (hooks->scene_position_set != 0)
            hooks->scene_position_set(hooks->context, entity, resolved_x,
                                      resolved_y, 1);
        return 1;
    }
    if (kind == 1 || kind == 3 || kind == 4) {
        if (hooks->position_get == 0 ||
            !hooks->position_get(hooks->context, entity, &x, &y)) return 0;
        if (hooks->scene_position_set != 0)
            hooks->scene_position_set(hooks->context, entity, x, y,
                                      kind == 2 ? 1 : 0);
        return 1;
    }
    return 0;
}

typedef struct EntityTurnToVectorState {
    int kind;
    uint32_t flags_f8;
    float direction[3];
} EntityTurnToVectorState;

typedef struct EntityTurnToVectorHooks {
    float (*turn_angle_get)(void* context, void* entity);
    void (*activity_request)(void* context, void* entity,
                             unsigned int activity);
    void (*activity_refresh)(void* context, void* entity);
    void* context;
} EntityTurnToVectorHooks;

static inline int entity_turn_to_vector_activity_issue(
    EntityTurnToVectorState* state, void* entity, const float target[2],
    const EntityTurnToVectorHooks* hooks)
{
    float current_length;
    float target_length;
    float current_x;
    float current_y;
    float target_x;
    float target_y;
    float angle;
    float cross;
    unsigned int action;
    if (state == 0 || entity == 0 || target == 0 || hooks == 0 ||
        state->kind != 2) return 0;
    current_length = (float)sqrt(state->direction[0] * state->direction[0] +
                                 state->direction[1] * state->direction[1] +
                                 state->direction[2] * state->direction[2]);
    target_length = (float)sqrt(target[0] * target[0] + target[1] * target[1]);
    if (current_length == 0.0f || target_length == 0.0f) return 0;
    current_x = state->direction[0] / current_length;
    current_y = state->direction[1] / current_length;
    target_x = target[0] / target_length;
    target_y = target[1] / target_length;
    angle = hooks->turn_angle_get != 0
        ? (float)fabs(hooks->turn_angle_get(hooks->context, entity)) : 0.0f;
    if (angle <= 0.7853982f) return 0;
    cross = current_x * target_y - current_y * target_x;
    if (angle <= 2.3561945f) {
        if (cross == 0.0f) return 0;
        action = cross < 0.0f ? 9u : 10u;
    }
    else {
        state->direction[0] = -state->direction[0];
        state->direction[1] = -state->direction[1];
        state->flags_f8 |= 0x80u;
        action = 10u;
    }
    if ((state->flags_f8 & 0x400u) != 0u) action += 11u;
    state->flags_f8 |= 4u;
    if (hooks->activity_request != 0)
        hooks->activity_request(hooks->context, entity, action);
    if (hooks->activity_refresh != 0)
        hooks->activity_refresh(hooks->context, entity);
    return 1;
}

typedef struct FlaggedVectorStepState {
    uint32_t flags_f8;
    int movement_blocked;
    int route_present;
    int current_cell[2];
    float world_position[2];
    float movement_vector[2];
    float movement_range;
    int route_flags;
} FlaggedVectorStepState;

typedef struct FlaggedVectorStepHooks {
    int (*entity_eligible)(void* context, void* entity);
    int (*cell_valid)(void* context, int x, int y);
    int (*world_to_cell)(void* context, float x, float y, int* cell_x,
                         int* cell_y);
    int (*route_step)(void* context, void* entity, const int start[2],
                      const int goal[2], float target[2]);
    void (*movement_begin)(void* context, void* entity);
    void (*step_issue)(void* context, void* entity, float x, float y,
                       unsigned int opcode, unsigned int flags);
    void (*path_flag_set)(void* context, void* entity, unsigned int flag);
    void* context;
} FlaggedVectorStepHooks;

static inline int flagged_vector_step_command_issue(
    FlaggedVectorStepState* state, void* entity,
    const FlaggedVectorStepHooks* hooks)
{
    float magnitude;
    float candidate_x;
    float candidate_y;
    float target[2];
    int candidate_cell[2];
    if (state == 0 || entity == 0 || hooks == 0 ||
        (state->flags_f8 & 0x800000u) != 0u || state->movement_blocked ||
        state->route_present || hooks->entity_eligible == 0 ||
        !hooks->entity_eligible(hooks->context, entity) ||
        hooks->cell_valid == 0 ||
        !hooks->cell_valid(hooks->context, state->current_cell[0],
                           state->current_cell[1])) return 0;
    if (hooks->movement_begin != 0)
        hooks->movement_begin(hooks->context, entity);
    magnitude = (float)sqrt(state->movement_vector[0] *
                            state->movement_vector[0] +
                            state->movement_vector[1] *
                            state->movement_vector[1]);
    if (magnitude == 0.0f || hooks->world_to_cell == 0) return 0;
    candidate_x = state->world_position[0] +
        state->movement_vector[0] / magnitude * state->movement_range / 3.0f;
    candidate_y = state->world_position[1] +
        state->movement_vector[1] / magnitude * state->movement_range / 3.0f;
    if (!hooks->world_to_cell(hooks->context, candidate_x, candidate_y,
                              &candidate_cell[0], &candidate_cell[1])) return 0;
    if (candidate_cell[0] == state->current_cell[0] &&
        candidate_cell[1] == state->current_cell[1]) {
        target[0] = candidate_x;
        target[1] = candidate_y;
    }
    else if (hooks->route_step == 0 ||
             !hooks->route_step(hooks->context, entity, state->current_cell,
                                candidate_cell, target)) return 0;
    if (hooks->step_issue != 0)
        hooks->step_issue(hooks->context, entity, target[0], target[1],
                          9u, (unsigned int)state->route_flags);
    if (hooks->path_flag_set != 0)
        hooks->path_flag_set(hooks->context, entity, 0x40u);
    state->flags_f8 |= 0x800000u;
    return 1;
}

typedef struct EntityRandomCellStepHooks {
    int (*movement_blocked)(void* context, void* entity);
    int (*random_direction)(void* context);
    int (*candidate_valid)(void* context, void* entity, int x, int y);
    int (*route_step)(void* context, void* entity, const int start[2],
                      const int goal[2], float target[2]);
    void (*step_issue)(void* context, void* entity, float x, float y);
    void* context;
} EntityRandomCellStepHooks;

static inline int entity_random_cell_step_route_issue(
    void* entity, const int cell[2],
    const EntityRandomCellStepHooks* hooks)
{
    static const int offsets[8][2] = {
        { -1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 },
        { 1, 0 }, { 1, -1 }, { 0, -1 }, { -1, -1 }
    };
    unsigned int attempt;
    if (entity == 0 || cell == 0 || hooks == 0 ||
        hooks->candidate_valid == 0 || hooks->route_step == 0 ||
        (hooks->movement_blocked != 0 &&
         hooks->movement_blocked(hooks->context, entity))) return 0;
    for (attempt = 0; attempt < 16; ++attempt) {
        int direction = hooks->random_direction != 0
            ? hooks->random_direction(hooks->context) : (int)attempt;
        int goal[2];
        float target[2];
        direction %= 8;
        if (direction < 0) direction += 8;
        goal[0] = cell[0] + offsets[direction][0];
        goal[1] = cell[1] + offsets[direction][1];
        if (!hooks->candidate_valid(hooks->context, entity,
                                    goal[0], goal[1])) continue;
        if (!hooks->route_step(hooks->context, entity, cell, goal, target))
            continue;
        if (hooks->step_issue != 0)
            hooks->step_issue(hooks->context, entity, target[0], target[1]);
        return 1;
    }
    return 0;
}

typedef struct EntityMovementAccumulatorState {
    int kind;
    int turn_direction;
    float forward_distance;
    float strafe_distance;
    float vertical_distance;
    float yaw_accumulator;
    float move_x;
    float move_y;
    float move_z;
    int reverse;
    int reverse_allowed;
    int blocked;
} EntityMovementAccumulatorState;

typedef struct EntityMovementAccumulatorHooks {
    float (*movement_scale_get)(void* context, void* entity,
                                float delta, float* output_scale);
    int (*position_get)(void* context, void* entity, float position[3]);
    void (*rotation_apply)(void* context, void* entity, float radians);
    void (*translation_apply)(void* context, void* entity,
                              float x, float y, float z);
    int (*collision_resolve)(void* context, void* entity,
                             float current[3], float candidate[3],
                             int* reverse_blocked);
    void (*position_set)(void* context, void* entity, const float position[3]);
    int (*activity_reached)(void* context, void* entity);
    int (*secondary_collision)(void* context, void* entity);
    void* context;
} EntityMovementAccumulatorHooks;

static inline int entity_movement_accumulator_apply(
    EntityMovementAccumulatorState* state, void* entity, float delta,
    float* output_scale, const EntityMovementAccumulatorHooks* hooks)
{
    float scale = 0.0f;
    float current[3];
    float candidate[3];
    int reverse_blocked = 0;
    int result = 0;
    if (state == 0 || entity == 0 || hooks == 0) return 0;
    if (hooks->movement_scale_get != 0)
        hooks->movement_scale_get(hooks->context, entity, delta, &scale);
    if (state->yaw_accumulator != 0.0f && hooks->rotation_apply != 0) {
        float sign = state->kind == 1 && state->turn_direction < 0
            ? -1.0f : 1.0f;
        hooks->rotation_apply(hooks->context, entity,
                              sign * delta * state->yaw_accumulator);
        result = 1;
    }
    if (state->forward_distance != 0.0f ||
        state->strafe_distance != 0.0f) {
        float length = (float)sqrt(state->forward_distance *
                                   state->forward_distance +
                                   state->strafe_distance *
                                   state->strafe_distance);
        if (length != 0.0f && hooks->translation_apply != 0) {
            const float x = state->forward_distance / length * scale;
            const float z = state->strafe_distance / length * scale;
            hooks->translation_apply(hooks->context, entity, x, 0.0f, z);
            result = 1;
        }
    }
    if (result != 0 && hooks->position_get != 0 &&
        hooks->position_get(hooks->context, entity, current)) {
        candidate[0] = current[0] + state->move_x;
        candidate[1] = current[1] + state->move_y;
        candidate[2] = current[2] + state->move_z;
        if (hooks->collision_resolve == 0 ||
            hooks->collision_resolve(hooks->context, entity, current,
                                     candidate, &reverse_blocked)) {
            if (hooks->position_set != 0)
                hooks->position_set(hooks->context, entity, candidate);
        }
        if (state->reverse && reverse_blocked) result = 4;
        else if (hooks->activity_reached != 0 &&
                 hooks->activity_reached(hooks->context, entity)) result = 3;
        else if (hooks->secondary_collision != 0 &&
                 hooks->secondary_collision(hooks->context, entity)) result = 3;
    }
    state->yaw_accumulator = 0.0f;
    state->forward_distance = 0.0f;
    state->strafe_distance = 0.0f;
    state->vertical_distance = 0.0f;
    state->move_x = 0.0f;
    state->move_y = 0.0f;
    state->move_z = 0.0f;
    state->turn_direction = 0;
    state->reverse = 0;
    state->blocked = 0;
    if (result != 0 && output_scale != 0) *output_scale = scale;
    return result;
}

typedef struct EntityMovementTickScaleState {
    int kind;
    uint32_t flags_f8;
    uint32_t flags_fa;
    uint32_t flags_f9;
    uint32_t stats_flags;
    float surface_scale;
    float entity_scale;
    float animation_scale;
    int terrain_type;
    int terrain_cell_valid;
} EntityMovementTickScaleState;

typedef struct EntityMovementTickScaleHooks {
    float (*base_speed_get)(void* context, void* entity, int kind);
    int (*terrain_data_get)(void* context, void* entity, int* cell_cost,
                            int* terrain_type, int* occupiable,
                            int* entity_scale_gate);
    float (*surface_scale_get)(void* context, void* entity);
    float (*entity_scale_get)(void* context, void* entity);
    float (*animation_scale_get)(void* context, void* entity);
    void* context;
} EntityMovementTickScaleHooks;

static inline int entity_movement_tick_scale_and_delta_get(
    EntityMovementTickScaleState* state, void* entity, float delta,
    float* movement_delta, float* movement_scale,
    const EntityMovementTickScaleHooks* hooks)
{
    float base_speed = 0.0f;
    float speed;
    float scale_factor = 1.0f;
    int cell_cost = 0;
    int terrain_type = state != 0 ? state->terrain_type : 0;
    int occupiable = 0;
    int entity_scale_gate = 0;
    int terrain_available = 0;
    if (state == 0 || entity == 0 || movement_delta == 0 ||
        movement_scale == 0 || hooks == 0) return 0;
    if (hooks->base_speed_get != 0)
        base_speed = hooks->base_speed_get(hooks->context, entity,
                                           state->kind);
    if ((state->flags_fa & 0x20000u) != 0u || state->kind == 10) {
        *movement_scale = delta;
        return 1;
    }
    speed = base_speed;
    if ((state->flags_f8 & 8u) != 0u) speed *= 0.05f;
    else if ((state->flags_f8 & 0x400u) != 0u && state->kind != 1)
        speed *= 0.5f;
    else if ((state->flags_f8 & 2u) != 0u) speed *= 0.5f;
    if ((state->flags_f8 & 0x2000000u) != 0u) speed *= 0.05f;
    if (hooks->terrain_data_get != 0 &&
        hooks->terrain_data_get(hooks->context, entity, &cell_cost,
                                &terrain_type, &occupiable,
                                &entity_scale_gate)) {
        terrain_available = 1;
        float surface = hooks->surface_scale_get != 0
            ? hooks->surface_scale_get(hooks->context, entity) : 1.0f;
        scale_factor *= 1.0f - (1.0f - surface) *
                                  ((float)cell_cost * 0.25f);
        if (entity_scale_gate && hooks->entity_scale_get != 0)
            scale_factor *= hooks->entity_scale_get(hooks->context, entity);
        /* FUN_00446610 resets its local multiplier to 1.0 before the
         * terrain-kind-9 branch, so this branch is a fixed 0.2 factor;
         * it does not use the entity's surface-scale stat. */
        if (state->kind == 2 && terrain_type == 9)
            scale_factor *= 0.2f;
        if (occupiable)
            speed *= scale_factor;
    }
    *movement_delta = speed * delta;
    if (base_speed == 0.0f) *movement_scale = 1.0f;
    else *movement_scale = speed / base_speed;
    if (state->kind == 4 && (state->stats_flags & 2u) != 0u &&
        (state->flags_f9 & 0x8000u) != 0u &&
        hooks->animation_scale_get != 0)
        *movement_scale *= hooks->animation_scale_get(hooks->context, entity);
    *movement_scale *= delta;
    /* Legacy callers retain their numeric fallback. A checked owner must
     * distinguish a missing terrain query from a valid result. */
    return terrain_available;
}

typedef struct EntityPathStepState {
    uint32_t flags;
    int current_activity;
    float* points;
    uint32_t point_count;
} EntityPathStepState;

typedef struct EntityPathStepHooks {
    void (*special_callback)(void* context, void* entity);
    int (*world_to_cell)(void* context, float x, float y, int cell[2]);
    int (*cell_needs_centering)(void* context, int x, int y);
    int (*cell_center_get)(void* context, int x, int y, float center[2]);
    float (*cell_size_get)(void* context);
    void* context;
} EntityPathStepHooks;

static inline void entity_path_step_vector_normalize(
    EntityPathStepState* state, void* entity,
    const EntityPathStepHooks* hooks)
{
    uint32_t i;
    if (state == 0 || hooks == 0 || state->points == 0 ||
        (state->flags & 0x20000000u) == 0u ||
        state->current_activity == 7 || state->current_activity == 3 ||
        state->current_activity == 6 || state->current_activity == 5) return;
    if ((state->flags & 0x80u) != 0u && hooks->special_callback != 0)
        hooks->special_callback(hooks->context, entity);
    for (i = 0; i < state->point_count; ++i) {
        float* point = state->points + i * 2;
        float center[2];
        float dx;
        float dy;
        float length;
        float cell_size;
        int cell[2];
        if (hooks->world_to_cell == 0 || hooks->cell_needs_centering == 0 ||
            hooks->cell_center_get == 0 ||
            !hooks->world_to_cell(hooks->context, point[0], point[1], cell) ||
            !hooks->cell_needs_centering(hooks->context, cell[0], cell[1]) ||
            !hooks->cell_center_get(hooks->context, cell[0], cell[1], center))
            continue;
        dx = point[0] - center[0];
        dy = point[1] - center[1];
        length = (float)sqrt(dx * dx + dy * dy);
        if (length == 0.0f) continue;
        cell_size = hooks->cell_size_get != 0
            ? hooks->cell_size_get(hooks->context) : 1.0f;
        point[0] = center[0] + dx / length * cell_size * 0.71f;
        point[1] = center[1] + dy / length * cell_size * 0.71f;
    }
}

typedef struct EntityPathAdvanceState {
    int kind;
    float progress;
    float duration;
    float step_start;
    float step_end;
    uint32_t flags_fa;
    uint32_t flags_f8;
    int activity;
} EntityPathAdvanceState;

typedef struct EntityPathAdvanceHooks {
    void (*progress_set)(void* context, void* entity, float progress);
    int (*arrival_check)(void* context, void* entity);
    int (*next_step)(void* context, void* entity, float* duration,
                     float* start, float* end);
    void (*activity_request)(void* context, void* entity,
                             unsigned int activity);
    void (*activity_refresh)(void* context, void* entity);
    void* context;
} EntityPathAdvanceHooks;

static inline float entity_path_step_advance_and_arrival_check(
    EntityPathAdvanceState* state, void* entity, float delta,
    const EntityPathAdvanceHooks* hooks)
{
    float remaining;
    if (state == 0 || entity == 0 || hooks == 0) return 0.0f;
    if (state->duration <= 0.0f) {
        if (hooks->arrival_check != 0 &&
            hooks->arrival_check(hooks->context, entity))
            state->progress = state->duration;
        else if (hooks->next_step != 0 &&
                 hooks->next_step(hooks->context, entity, &state->duration,
                                  &state->step_start, &state->step_end))
            state->progress = 0.0f;
        else
            return 0.0f;
    }
    state->progress += delta;
    if (state->progress < state->duration) {
        remaining = state->progress / state->duration;
        if (hooks->progress_set != 0)
            hooks->progress_set(hooks->context, entity,
                state->step_start + (state->step_end - state->step_start) *
                                    remaining);
        return remaining;
    }
    state->progress = state->duration;
    if (hooks->progress_set != 0)
        hooks->progress_set(hooks->context, entity, state->step_end);
    if (hooks->arrival_check != 0 &&
        hooks->arrival_check(hooks->context, entity)) {
        state->flags_fa |= 0x400u;
        if (hooks->activity_request != 0)
            hooks->activity_request(hooks->context, entity,
                                    (unsigned int)state->activity);
        if (hooks->activity_refresh != 0)
            hooks->activity_refresh(hooks->context, entity);
        return 1.0f;
    }
    if (hooks->next_step != 0 &&
        hooks->next_step(hooks->context, entity, &state->duration,
                         &state->step_start, &state->step_end))
        state->progress = 0.0f;
    else
        state->flags_f8 |= 0x400u;
    return 1.0f;
}

typedef struct StepMovementVectorState {
    uint32_t flags_f8;
    uint32_t flags_fa;
    uint32_t path_index;
    int mode;
    float current_position[2];
    float direction[2];
} StepMovementVectorState;

typedef struct StepMovementVectorHooks {
    int (*waypoint_get)(void* context, void* entity, uint32_t index,
                        float position[2], uint32_t* waypoint_flags);
    int (*direction_project)(void* context, void* entity,
                             const float from[2], float to[2],
                             uint32_t waypoint_flags);
    float (*step_speed_get)(void* context, void* entity);
    void* context;
} StepMovementVectorHooks;

static inline int step_movement_vector_resolve(
    StepMovementVectorState* state, void* entity, int advance,
    float output_vector[2], float* output_speed,
    const StepMovementVectorHooks* hooks)
{
    float target[2];
    float dx;
    float dy;
    float length;
    uint32_t waypoint_flags = 0;
    if (state == 0 || entity == 0 || output_vector == 0 || hooks == 0 ||
        hooks->waypoint_get == 0) return 0;
    if (advance) ++state->path_index;
    if (!hooks->waypoint_get(hooks->context, entity, state->path_index,
                             target, &waypoint_flags)) return 0;
    dx = target[0] - state->current_position[0];
    dy = target[1] - state->current_position[1];
    if (hooks->direction_project != 0 &&
        (waypoint_flags & 0x40u) == 0u)
        hooks->direction_project(hooks->context, entity,
                                 state->current_position, target,
                                 waypoint_flags);
    dx = target[0] - state->current_position[0];
    dy = target[1] - state->current_position[1];
    length = (float)sqrt(dx * dx + dy * dy);
    if (length == 0.0f) {
        output_vector[0] = 0.0f;
        output_vector[1] = 0.0f;
    }
    else {
        output_vector[0] = dx / length;
        output_vector[1] = dy / length;
    }
    state->direction[0] = output_vector[0];
    state->direction[1] = output_vector[1];
    if (output_speed != 0)
        *output_speed = hooks->step_speed_get != 0
            ? hooks->step_speed_get(hooks->context, entity) : 0.0f;
    return length > 0.0f;
}

#endif
