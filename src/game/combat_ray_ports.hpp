// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGO_GAME_COMBAT_RAY_PORTS_HPP
#define LEGO_GAME_COMBAT_RAY_PORTS_HPP

#include <math.h>

typedef struct CombatRayState {
    void* tracked_entity;
    float origin[3];
    float target[3];
    float range;
    int type_kind;
    int effect_mode;
    int effect_kind;
} CombatRayState;

typedef struct CombatRayHooks {
    float (*configured_range_get)(void* context, int kind);
    int (*raycast)(void* context, const float origin[3],
                   const float direction[3], float* hit_fraction,
                   float hit_point[3], void** hit_object_out);
    void (*entity_release)(void* context, void* entity);
    void (*impact_apply)(void* context, void* entity, int mode,
                         const float hit_point[3], int kind, int effect_kind);
    void* context;
} CombatRayHooks;

static inline void combat_ray_hit_scan_and_effect_apply(
    CombatRayState* state, const CombatRayHooks* hooks)
{
    float delta[3];
    float hit_point[3];
    float fraction = 1.0f;
    float distance;
    void* hit_object = 0;
    int collided;
    int i;
    if (state == 0 || hooks == 0 || state->tracked_entity == 0 ||
        hooks->configured_range_get == 0 || hooks->raycast == 0) return;
    state->range = hooks->configured_range_get(hooks->context,
                                               state->type_kind);
    delta[0] = state->target[0] - state->origin[0];
    delta[1] = state->target[1] - state->origin[1];
    delta[2] = state->target[2] - state->origin[2];
    distance = (float)sqrt(delta[0] * delta[0] + delta[1] * delta[1] +
                           delta[2] * delta[2]);
    if (state->range != 0.0f && distance >= state->range) {
        if (hooks->entity_release != 0)
            hooks->entity_release(hooks->context, state->tracked_entity);
        state->tracked_entity = 0;
        return;
    }
    for (i = 0; i < 3; ++i) hit_point[i] = 0.0f;
    collided = hooks->raycast(hooks->context, state->origin, delta,
                              &fraction, hit_point, &hit_object);
    if (!collided) return;
    if (hooks->entity_release != 0)
        hooks->entity_release(hooks->context, state->tracked_entity);
    state->tracked_entity = 0;
    if (hit_object != 0 && hooks->impact_apply != 0)
        hooks->impact_apply(hooks->context, hit_object, state->effect_mode,
                            hit_point, state->type_kind, state->effect_kind);
}

typedef struct LaserImpactHooks {
    void (*spatial_sound)(void* context, unsigned int selector,
                         const float position[3]);
    void* (*segment_trace)(void* context, const float origin[3],
                           const float end[3], float* hit_fraction,
                           float hit[3]);
    int (*terrain_surface_get)(void* context, const float point[3],
                               int* surface);
    void (*entity_impact)(void* context, void* entity, const float point[3],
                          int kind, float scale);
    void (*terrain_impact)(void* context, int surface,
                           const float point[3], int kind);
    void (*fallback_impact)(void* context, const float point[3], int kind);
    void (*effect_segment_create)(void* context, const float origin[3],
                                  const float end[3]);
    void (*progress_queue)(void* context, void* request, int kind,
                           float scale);
    void* context;
} LaserImpactHooks;

static inline void laser_spatial_sound_and_impact(
    void* request, int kind, float scale, const float origin[3],
    const float direction[3], const LaserImpactHooks* hooks)
{
    float end[3];
    float impact_point[3];
    float hit_point[3];
    float hit_fraction = 1.0f;
    int surface = 0;
    void* hit;
    if (hooks == 0 || origin == 0 || direction == 0) return;
    end[0] = origin[0] + direction[0] * scale;
    end[1] = origin[1] + direction[1] * scale;
    end[2] = origin[2] + direction[2] * scale;
    if (hooks->spatial_sound != 0)
        hooks->spatial_sound(hooks->context, 0x1bu, origin);
    hit = hooks->segment_trace != 0
        ? hooks->segment_trace(hooks->context, origin, end, &hit_fraction,
                               hit_point) : 0;
    impact_point[0] = hit != 0 ? hit_point[0]
        : origin[0] + (end[0] - origin[0]) * hit_fraction;
    impact_point[1] = hit != 0 ? hit_point[1]
        : origin[1] + (end[1] - origin[1]) * hit_fraction;
    impact_point[2] = hit != 0 ? hit_point[2]
        : origin[2] + (end[2] - origin[2]) * hit_fraction;
    if (hooks->effect_segment_create != 0)
        hooks->effect_segment_create(hooks->context, origin, impact_point);
    if (hit != 0) {
        if (hooks->entity_impact != 0)
            hooks->entity_impact(hooks->context, hit, hit_point, kind, scale);
        if (hooks->spatial_sound != 0)
            hooks->spatial_sound(hooks->context, 0x1cu, hit_point);
    }
    else if (hit_fraction < 1.0f) {
        if (hooks->terrain_surface_get != 0 &&
            hooks->terrain_surface_get(hooks->context, impact_point, &surface)) {
            if (hooks->terrain_impact != 0)
                hooks->terrain_impact(hooks->context, surface, impact_point,
                                     kind);
        }
        else if (hooks->fallback_impact != 0)
            hooks->fallback_impact(hooks->context, impact_point, kind);
    }
    if (hooks->progress_queue != 0)
        hooks->progress_queue(hooks->context, request, kind, scale);
}

#endif
