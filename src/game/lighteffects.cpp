// SPDX-License-Identifier: LGPL-3.0-or-later
/* GAME-024 LightEffects.c: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/GAME-024.csv. */
#include "effects.hpp"
#include "combat_ray_ports.hpp"
#include "entity.hpp"
#include "frontend.hpp"
#include "crt/crt.hpp"
#include "engine.hpp"
#include "ai.hpp"
#include "simtick.hpp"
#include "sound.hpp"
#include "gods98/heap.hpp"
#include "gods98/record.hpp"
#include "gods98/scene_pool.hpp"
#include "gods98/scene_resources.hpp"
#include "gods98/surface.hpp"
#include "gods98/texture.hpp"
#include "gods98/view.hpp"
#include "gods98/resource.hpp"
#include "gods98/block.hpp"
#include "level.hpp"
#include "weapon.hpp"
#include "util.hpp"
#include "bezier.h"
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "effects_for_lighteffects.hpp"
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <locale.h>
#include "port_loop_ports.hpp"
#include "util_for_lighteffects.hpp"
#include "message.hpp"
#include "game/cmd.hpp"
#include "game/select.hpp"
#include "game/render.hpp"
#include "game/terrain_grid.hpp"
#include "simtick_for_lighteffects.hpp"
#include <stdarg.h>
#include "string.hpp"
#include "model.hpp"
#include "view_for_lighteffects.hpp"

// GLOBAL: LEGORR 0x004EADD8
void* s_motion_jitter_retail_scene_object;
// GLOBAL: LEGORR 0x004EADDC
void* s_motion_jitter_retail_scene_owner;
// GLOBAL: LEGORR 0x004EADEC
float s_motion_waypoint_position[3];
// GLOBAL: LEGORR 0x004EAE18
float s_motion_waypoint_target[3];
// GLOBAL: LEGORR 0x004EAE24
float s_motion_waypoint_bounds_min[3];
// GLOBAL: LEGORR 0x004EAE30
float s_motion_waypoint_bounds_max[3];
// GLOBAL: LEGORR 0x004EAE3C
float s_motion_waypoint_delay;
// GLOBAL: LEGORR 0x004EAE40
float s_motion_waypoint_delay_range[2];
// GLOBAL: LEGORR 0x004EAE48
float s_motion_waypoint_velocity[3];
// GLOBAL: LEGORR 0x004EAE54
float s_motion_waypoint_scale_range[2];
// GLOBAL: LEGORR 0x004EAE5C
float s_motion_waypoint_duration;
// GLOBAL: LEGORR 0x004EAE60
float s_motion_waypoint_duration_range[2];
// GLOBAL: LEGORR 0x004EAE68
float s_motion_waypoint_progress[3];

// GLOBAL: LEGORR 0x004EAE74
float s_motion_jitter_center[3];
// GLOBAL: LEGORR 0x004EAE80
float s_motion_jitter_tolerance[3];
// GLOBAL: LEGORR 0x004EAE8C
float s_motion_jitter_delay;
// GLOBAL: LEGORR 0x004EAE90
float s_motion_jitter_delay_min;
// GLOBAL: LEGORR 0x004EAE94
float s_motion_jitter_delay_max;
// GLOBAL: LEGORR 0x004EAE98
float s_motion_jitter_duration;
// GLOBAL: LEGORR 0x004EAE9C
float s_motion_jitter_duration_min;
// GLOBAL: LEGORR 0x004EAEA0
float s_motion_jitter_duration_max;
// GLOBAL: LEGORR 0x004EAEA4
float s_motion_jitter_direction[3];
// GLOBAL: LEGORR 0x004EAEB0
float s_motion_jitter_remaining;
// GLOBAL: LEGORR 0x004EAEB4
float s_motion_jitter_magnitude_min;
// GLOBAL: LEGORR 0x004EAEB8
float s_motion_jitter_magnitude_max;
// GLOBAL: LEGORR 0x004EAEC8
uint32_t s_motion_jitter_flags;
extern "C" short rng_thunk_draw15(void);

/* Defined below (forward declarations added by tools/relocate_unit.py). */
static int light_effect_config_float_triplet_read(BlockFile* config, const char* path, float values[3]);
static int light_effect_config_path_build(char* path, size_t path_capacity, const char* config_root, const char* key);
static int motion_jitter_candidate_valid(void*, const float position[3]);
int motion_jitter_candidate_valid_retail(float position[3]);
static void motion_jitter_position_apply(void*, const float position[3]);
static int motion_jitter_vector_random(void*, float out[3]);

// FUNCTION: LEGORR 0x0044c710
void scene_transform_state_initialize(void* target, void* owner,
                                      float x, float y, float z)
{
    s_scene_transform_state.target = target;
    s_scene_transform_state.owner = owner;
    s_scene_transform_state.base[0] = x;
    s_scene_transform_state.base[1] = y;
    s_scene_transform_state.base[2] = z;
    s_scene_transform_state.current[0] = x;
    s_scene_transform_state.current[1] = y;
    s_scene_transform_state.current[2] = z;
    motion_gameplay_scene_bind(target, s_scene_transform_state.flags);
    motion_gameplay_scene_owner_bind(target, owner);
    motion_waypoint_position_bind(x, y, z);
    render_object_set_packed_rgba_from_float_channels(target, x, y, z,
                                                       1.0f);
}

// FUNCTION: LEGORR 0x0044c790
void scene_transform_enable_set(int enabled)
{
    if (enabled != 0) {
        s_scene_transform_state.flags |= 0x100u;
        render_object_set_packed_rgba_from_float_channels(
            s_scene_transform_state.target,
            s_scene_transform_state.base[0],
            s_scene_transform_state.base[1],
            s_scene_transform_state.base[2], 1.0f);
    }
    else {
        s_scene_transform_state.flags &= ~0x100u;
    }
    motion_gameplay_scene_bind(s_scene_transform_state.target,
                               s_scene_transform_state.flags);
}
// FUNCTION: LEGORR 0x0044c7c0
void light_effect_configuration_load(void* config,
                                    const char* config_root)
{
    (void)light_effect_blink_config_load(config, (void*)config_root);
    (void)light_effect_fade_config_load(config, (void*)config_root);
    (void)light_effect_move_config_load(config, config_root);
}
// FUNCTION: LEGORR 0x0044c7f0
uint32_t light_effect_blink_config_load(void* config, void* section)
{
    return s_light_effect_config_load_hook != NULL
        ? s_light_effect_config_load_hook(s_light_effect_config_load_context,
                                          0, config, section) : 0u;
}

// FUNCTION: LEGORR 0x0044c970
void light_effect_blink_parameters_store(uint32_t a, uint32_t b,
                                         uint32_t c, uint32_t d,
                                         uint32_t e, uint32_t f)
{
    s_light_effect_parameters.blink[0] = a;
    s_light_effect_parameters.blink[1] = b;
    s_light_effect_parameters.blink[2] = c;
    s_light_effect_parameters.blink[3] = d;
    s_light_effect_parameters.blink[4] = e;
    s_light_effect_parameters.blink[5] = f;
    s_light_effect_parameters.flags |= 1u;
}
// FUNCTION: LEGORR 0x0044c9c0
uint32_t light_effect_fade_config_load(void* config, void* section)
{
    return s_light_effect_config_load_hook != NULL
        ? s_light_effect_config_load_hook(s_light_effect_config_load_context,
                                          1, config, section) : 0u;
}

// FUNCTION: LEGORR 0x0044cc10
void light_effect_fade_parameters_store(uint32_t a, uint32_t b, uint32_t c,
                                        uint32_t d, uint32_t e, uint32_t f,
                                        uint32_t g, uint32_t h, uint32_t i,
                                        uint32_t j, uint32_t k, uint32_t l)
{
    s_light_effect_parameters.fade[0] = a;
    s_light_effect_parameters.fade[1] = b;
    s_light_effect_parameters.fade[2] = c;
    s_light_effect_parameters.fade[3] = d;
    s_light_effect_parameters.fade[4] = e;
    s_light_effect_parameters.fade[5] = f;
    s_light_effect_parameters.fade[6] = g;
    s_light_effect_parameters.fade[7] = h;
    s_light_effect_parameters.fade[8] = i;
    s_light_effect_parameters.fade[9] = j;
    s_light_effect_parameters.fade[10] = k;
    s_light_effect_parameters.fade[11] = l;
    s_light_effect_parameters.flags |= 4u;
}
// FUNCTION: LEGORR 0x0044cca0
uint32_t light_effect_move_config_load(void* config,
                                      const char* config_root)
{
    BlockFile* file = (BlockFile*)config;
    const char* root = config_root != NULL ? config_root : "Lego*";
    static const char* const keys[4] = {
        "RandomRangeForTimeBetweenMoves",
        "RandomRangeForSpeedOfMove",
        "RandomRangeForDistOfMove",
        "MoveLimit"
    };
    char path[256];
    float time_min, time_max;
    float speed_min, speed_max;
    float distance_min, distance_max;
    float move_limit[3];
    if (file == NULL ||
        !light_effect_config_path_build(path, sizeof(path), root, keys[0]) ||
        !light_effect_config_range_pair_read(file, path, &time_min,
                                             &time_max) ||
        !light_effect_config_path_build(path, sizeof(path), root, keys[1]) ||
        !light_effect_config_range_pair_read(file, path, &speed_min,
                                             &speed_max) ||
        !light_effect_config_path_build(path, sizeof(path), root, keys[2]) ||
        !light_effect_config_range_pair_read(file, path, &distance_min,
                                             &distance_max) ||
        !light_effect_config_path_build(path, sizeof(path), root, keys[3]) ||
        !light_effect_config_float_triplet_read(file, path, move_limit)) {
        return 0u;
    }
    light_effect_move_parameters_store(
        light_effect_float_word(time_min * 25.0f),
        light_effect_float_word(time_max * 25.0f),
        light_effect_float_word(speed_min * 25.0f),
        light_effect_float_word(speed_max * 25.0f),
        light_effect_float_word(distance_min),
        light_effect_float_word(distance_max),
        light_effect_float_word(move_limit[0]),
        light_effect_float_word(move_limit[1]),
        light_effect_float_word(move_limit[2]));
    return 1u;
}

// FUNCTION: LEGORR 0x0044cef0
void motion_jitter_parameters_store(float delay_min, float delay_max,
                                    float speed_min, float speed_max,
                                    float magnitude_min, float magnitude_max,
                                    float tolerance_x, float tolerance_y,
                                    float tolerance_z)
{
#if defined(_WIN32)
    s_motion_jitter_delay_min = delay_min;
    s_motion_jitter_delay_max = delay_max;
    s_motion_jitter_duration_max = speed_max;
    s_motion_jitter_magnitude_min = magnitude_min;
    s_motion_jitter_duration_min = speed_min;
    s_motion_jitter_magnitude_max = magnitude_max;
    (void)ddiscene_object_position_get(s_motion_jitter_retail_scene_object,
        s_motion_jitter_retail_scene_owner, s_motion_jitter_center);
    s_motion_jitter_tolerance[1] = tolerance_y;
    s_motion_jitter_tolerance[0] = tolerance_x;
    s_motion_jitter_tolerance[2] = tolerance_z;
    s_motion_jitter_flags |= 0x40u;
#else
    s_motion_jitter_state.delay_min = delay_min;
    s_motion_jitter_state.delay_max = delay_max;
    s_motion_jitter_state.lifetime_min = speed_min;
    s_motion_jitter_state.lifetime_max = speed_max;
    s_motion_jitter_state.magnitude_min = magnitude_min;
    s_motion_jitter_state.magnitude_max = magnitude_max;
    s_motion_jitter_delay_min = delay_min;
    s_motion_jitter_delay_max = delay_max;
    s_motion_jitter_duration_min = speed_min;
    s_motion_jitter_duration_max = speed_max;
    s_motion_jitter_magnitude_min = magnitude_min;
    s_motion_jitter_magnitude_max = magnitude_max;
    s_motion_jitter_tolerance[0] = tolerance_x;
    s_motion_jitter_tolerance[1] = tolerance_y;
    s_motion_jitter_tolerance[2] = tolerance_z;
    memcpy(s_motion_jitter_state.tolerance, s_motion_jitter_tolerance,
           sizeof(s_motion_jitter_tolerance));
    s_motion_jitter_state.flags |= 0x40u;
    s_motion_jitter_flags |= 0x40u;
    motion_jitter_center_refresh();
#endif
}

// FUNCTION: LEGORR 0x0044cf70
void simtick_scene_position_reset_and_effect_motion_cancel(void)
{
    __builtin_memcpy(s_xform_config, s_xform_current, sizeof s_xform_config);
    s_motion_jitter_flags &= 0xffffff7fu;
}

// FUNCTION: LEGORR 0x0044cff0
void scene_transform_rgb_clamp_apply(void)
{
    float color[3];
    unsigned int i;
    for (i = 0; i < 3; ++i) {
        color[i] = s_scene_transform_state.current[i];
        if (color[i] < 0.0f) color[i] = 0.0f;
        if (color[i] > 1.0f) color[i] = 1.0f;
    }
    render_object_set_packed_rgba_from_float_channels(
        s_scene_transform_state.target, color[0], color[1], color[2], 1.0f);
}

/* DriftOffsetStepApply (FUN_0044d0d0): periodically choose a three-axis
 * offset, reflect it at the horizontal bound, and accumulate it into the
 * module-owned drift position. */
// FUNCTION: LEGORR 0x0044D0D0
void motion_drift_offset_step(float delta)
{
    MotionDriftState* state = s_motion_drift_state;
    float random_value;
    float step[3];
    float shifted_x;
    int i;

    if (state == NULL || (state->flags & 1u) == 0u ||
        s_motion_drift_random_range == NULL) return;
    state->timer -= delta;
    if (state->timer > 0.0f) return;

    state->timer = s_motion_drift_random_range(state->timer_min,
                                                state->timer_max,
                                                s_motion_drift_user);
    random_value = s_motion_drift_random_range(0.0f, 1.0f,
                                               s_motion_drift_user);
    for (i = 0; i != 3; ++i)
        step[i] = ((random_value * 2.0f) - 1.0f) * state->amplitude[i];
    shifted_x = step[0] + state->center_x;
    if (state->max_abs_x <= (shifted_x < 0.0f ? -shifted_x : shifted_x)) {
        for (i = 0; i != 3; ++i)
            step[i] = step[i] < 0.0f ? step[i] : -step[i];
        shifted_x = step[0] + state->center_x;
        if (state->max_abs_x <=
            (shifted_x < 0.0f ? -shifted_x : shifted_x)) return;
    }
    state->offset[0] += step[0];
    state->offset[1] += step[1];
    state->offset[2] += step[2];
    state->center_x = shifted_x;
}

/* FloatNegateAbsoluteValue (FUN_0044d250) — see view.hpp. */
// FUNCTION: LEGORR 0x0044d250
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) void float_negate_absolute_value(float* value)
{
    __asm {
        mov ecx, dword ptr [esp + 4]
        test ecx, ecx
        je float_negate_null
        fld dword ptr [ecx]
        fcomp dword ptr ds:[0049e4b0h]
        fld dword ptr [ecx]
        fnstsw ax
        test ah, 1
        je float_negate_nonnegative
        fmul dword ptr ds:[0049e4c8h]
        fstp dword ptr [ecx]
        ret

    float_negate_nonnegative:
        fchs
        fstp dword ptr [ecx]
    float_negate_null:
        ret
    }
}
#else
void float_negate_absolute_value(float* value)
{
    if (value == NULL) {
        return; /* deviation: the corpus dereferences blindly */
    }
    *value = -*value; /* both corpus branches negate */
}
#endif

// FUNCTION: LEGORR 0x0044d280
void motion_waypoint_timer_tick(float delta)
{
#if defined(_WIN32) && defined(_M_IX86)
    float random_fraction;
    float first_step;
    if ((s_motion_jitter_flags & 4u) == 0u) return;
    if ((s_motion_jitter_flags & 8u) == 0u) {
        s_motion_waypoint_delay = s_motion_waypoint_delay - delta;
        if (s_motion_waypoint_delay > 0.0f) return;
        s_motion_waypoint_delay = random_float_range(
            s_motion_waypoint_delay_range[0],
            s_motion_waypoint_delay_range[1]);
        s_motion_waypoint_duration = random_float_range(
            s_motion_waypoint_duration_range[0],
            s_motion_waypoint_duration_range[1]);
        s_motion_waypoint_progress[0] = 0.0f;
        s_motion_waypoint_progress[1] = 0.0f;
        s_motion_waypoint_progress[2] = 0.0f;
        random_fraction = (float)((int16_t)rng_thunk_draw15() % 1000) *
                          0.001f;
        s_motion_waypoint_target[0] =
            (float)fabs((double)(s_motion_waypoint_bounds_max[0] -
                                 s_motion_waypoint_bounds_min[0])) *
            random_fraction + s_motion_waypoint_bounds_min[0];
        s_motion_waypoint_target[1] =
            (float)fabs((double)(s_motion_waypoint_bounds_max[1] -
                                 s_motion_waypoint_bounds_min[1])) *
            random_fraction + s_motion_waypoint_bounds_min[1];
        s_motion_waypoint_target[2] =
            (float)fabs((double)(s_motion_waypoint_bounds_max[2] -
                                 s_motion_waypoint_bounds_min[2])) *
            random_fraction + s_motion_waypoint_bounds_min[2];
        motion_waypoint_vector_random_scale_apply();
        s_motion_jitter_flags |= 0x18u;
    }
    else if ((s_motion_jitter_flags & 0x10u) == 0u) {
        if ((s_motion_jitter_flags & 0x20u) == 0u) {
            s_motion_waypoint_duration = s_motion_waypoint_duration - delta;
            if (s_motion_waypoint_duration > 0.0f) return;
            motion_waypoint_vector_random_scale_apply();
            s_motion_waypoint_progress[0] = s_motion_waypoint_target[0];
            s_motion_waypoint_progress[1] = s_motion_waypoint_target[1];
            s_motion_waypoint_progress[2] = s_motion_waypoint_target[2];
            s_motion_jitter_flags |= 0x20u;
            return;
        }
        else {
            first_step = delta * s_motion_waypoint_velocity[0];
            s_motion_waypoint_position[0] -= first_step;
            s_motion_waypoint_progress[0] -= first_step;
            s_motion_waypoint_position[1] -=
                delta * s_motion_waypoint_velocity[1];
            s_motion_waypoint_progress[1] -=
                delta * s_motion_waypoint_velocity[1];
            s_motion_waypoint_position[2] -=
                delta * s_motion_waypoint_velocity[2];
            s_motion_waypoint_progress[2] -=
                delta * s_motion_waypoint_velocity[2];
            if (first_step < 0.0f) {
                if (s_motion_waypoint_progress[0] >= 0.0f) {
                    s_motion_waypoint_position[0] -=
                        s_motion_waypoint_progress[0];
                    s_motion_waypoint_position[1] -=
                        s_motion_waypoint_progress[1];
                    s_motion_waypoint_position[2] -=
                        s_motion_waypoint_progress[2];
                    s_motion_jitter_flags &= 0xffffffd7u;
                    return;
                }
            }
            else if (s_motion_waypoint_progress[0] <= 0.0f) {
                s_motion_waypoint_position[0] -=
                    s_motion_waypoint_progress[0];
                s_motion_waypoint_position[1] -=
                    s_motion_waypoint_progress[1];
                s_motion_waypoint_position[2] -=
                    s_motion_waypoint_progress[2];
                s_motion_jitter_flags &= 0xffffffd7u;
                return;
            }
        }
    }
    else {
        first_step = delta * s_motion_waypoint_velocity[0];
        s_motion_waypoint_position[0] += first_step;
        s_motion_waypoint_position[1] +=
            delta * s_motion_waypoint_velocity[1];
        s_motion_waypoint_position[2] +=
            delta * s_motion_waypoint_velocity[2];
        s_motion_waypoint_progress[0] += first_step;
        s_motion_waypoint_progress[1] +=
            delta * s_motion_waypoint_velocity[1];
        s_motion_waypoint_progress[2] +=
            delta * s_motion_waypoint_velocity[2];
        if (first_step < 0.0f) {
            if (s_motion_waypoint_progress[0] <= s_motion_waypoint_target[0]) {
                s_motion_waypoint_position[0] += s_motion_waypoint_target[0] -
                                                  s_motion_waypoint_progress[0];
                s_motion_waypoint_position[1] += s_motion_waypoint_target[1] -
                                                  s_motion_waypoint_progress[1];
                s_motion_jitter_flags &= 0xffffffefu;
                s_motion_waypoint_position[2] += s_motion_waypoint_target[2] -
                                                  s_motion_waypoint_progress[2];
                return;
            }
        }
        else if (s_motion_waypoint_target[0] <= s_motion_waypoint_progress[0]) {
            s_motion_waypoint_position[0] -= s_motion_waypoint_progress[0] -
                                              s_motion_waypoint_target[0];
            s_motion_waypoint_position[1] -= s_motion_waypoint_progress[1] -
                                              s_motion_waypoint_target[1];
            s_motion_jitter_flags &= 0xffffffefu;
            s_motion_waypoint_position[2] -= s_motion_waypoint_progress[2] -
                                              s_motion_waypoint_target[2];
            return;
        }
    }
#else
    MotionWaypointTimerState* state = s_motion_waypoint_state;
    int i;
    float first_step;
    PortLoopRandomRangeHook random_range = s_motion_waypoint_random_range;
    void* context = s_motion_waypoint_user;

    if (state == NULL || (state->flags & 4u) == 0u) return;
    if (random_range == NULL) {
        random_range = motion_waypoint_default_random;
        context = NULL;
    }
    if ((state->flags & 8u) == 0u) {
        state->delay -= delta;
        if (state->delay > 0.0f) return;
        state->delay = random_range(context, state->delay_min,
                                    state->delay_max);
        state->duration = random_range(context, state->duration_min,
                                       state->duration_max);
        for (i = 0; i < 3; ++i) state->progress[i] = 0.0f;
        {
            const float fraction = (float)((int)(int16_t)rng_thunk_draw15() %
                                             1000) * 0.001f;
            for (i = 0; i < 3; ++i) {
                const float span = state->bounds_max[i] - state->bounds_min[i];
                state->target[i] = (float)fabs((double)span) * fraction +
                                   state->bounds_min[i];
            }
        }
        motion_waypoint_vector_random_scale_apply();
        state->flags |= 0x18u;
        return;
    }
    if ((state->flags & 0x10u) == 0u) {
        if ((state->flags & 0x20u) == 0u) {
            state->duration -= delta;
            if (state->duration > 0.0f) return;
            motion_waypoint_vector_random_scale_apply();
            for (i = 0; i < 3; ++i) state->progress[i] = state->target[i];
            state->flags |= 0x20u;
            return;
        }
        first_step = delta * state->velocity[0];
        for (i = 0; i < 3; ++i) {
            const float step = delta * state->velocity[i];
            state->position[i] -= step;
            state->progress[i] -= step;
        }
        if ((first_step < 0.0f && state->progress[0] >= 0.0f) ||
            (first_step >= 0.0f && state->progress[0] <= 0.0f)) {
            for (i = 0; i < 3; ++i)
                state->position[i] -= state->progress[i];
            /* The decompile clears mask 0x28 here. This branch already has
             * bit 0x10 clear, so preserve its exact write mask rather than
             * clearing that unrelated phase bit as well. */
            state->flags &= ~0x28u;
        }
        return;
    }
    first_step = delta * state->velocity[0];
    for (i = 0; i < 3; ++i) {
        const float step = delta * state->velocity[i];
        state->position[i] += step;
        state->progress[i] += step;
    }
    if ((first_step < 0.0f && state->progress[0] <= state->target[0]) ||
        (first_step >= 0.0f && state->progress[0] >= state->target[0])) {
        for (i = 0; i < 3; ++i)
            state->position[i] += state->target[i] - state->progress[i];
        state->flags &= ~0x10u;
    }
#endif
}
// FUNCTION: LEGORR 0x0044d710
void motion_waypoint_vector_random_scale_apply(void)
{
#if defined(_WIN32) && defined(_M_IX86)
    float scale = random_float_range(s_motion_waypoint_scale_range[0],
                                     s_motion_waypoint_scale_range[1]);
    s_motion_waypoint_velocity[0] = s_motion_waypoint_target[0] / scale;
    s_motion_waypoint_velocity[1] = s_motion_waypoint_target[1] / scale;
    s_motion_waypoint_velocity[2] = s_motion_waypoint_target[2] / scale;
#else
    MotionWaypointTimerState* state = s_motion_waypoint_state;
    float scale;
    PortLoopRandomRangeHook random_range = s_motion_waypoint_random_range;
    void* context = s_motion_waypoint_user;
    if (state == NULL) return;
    if (random_range == NULL) {
        random_range = motion_waypoint_default_random;
        context = NULL;
    }
    scale = random_range(context, state->scale_min, state->scale_max);
    state->velocity[0] = state->target[0] / scale;
    state->velocity[1] = state->target[1] / scale;
    state->velocity[2] = state->target[2] / scale;
#endif
}
// FUNCTION: LEGORR 0x0044D760
void motion_jitter_step(float delta)
{
#if defined(_WIN32) && defined(_M_IX86)
    float position[3];
    float candidate[3];
    float inverse_length;
    int valid;
    if ((s_motion_jitter_flags & 0x40u) == 0u) return;
    if ((s_motion_jitter_flags & 0x80u) == 0u) {
        s_motion_jitter_delay = s_motion_jitter_delay - delta;
        if (0.0f < s_motion_jitter_delay) return;
        s_motion_jitter_delay = random_float_range(
            s_motion_jitter_delay_min, s_motion_jitter_delay_max);
        s_motion_jitter_duration = random_float_range(
            s_motion_jitter_duration_min, s_motion_jitter_duration_max);
        s_motion_jitter_remaining = random_float_range(
            s_motion_jitter_magnitude_min, s_motion_jitter_magnitude_max);
        d3drm_vector_random(s_motion_jitter_direction);
        inverse_length = 1.0f / (float)sqrt((double)(
            s_motion_jitter_direction[2] * s_motion_jitter_direction[2] +
            s_motion_jitter_direction[1] * s_motion_jitter_direction[1] +
            s_motion_jitter_direction[0] * s_motion_jitter_direction[0]));
        s_motion_jitter_direction[0] = inverse_length *
            s_motion_jitter_direction[0] * s_motion_jitter_duration;
        s_motion_jitter_direction[1] = inverse_length *
            s_motion_jitter_direction[1] * s_motion_jitter_duration;
        s_motion_jitter_direction[2] = inverse_length *
            s_motion_jitter_direction[2] * s_motion_jitter_duration;
        s_motion_jitter_flags |= 0x80u;
        return;
    }
    if (!ddiscene_object_position_get(s_motion_jitter_retail_scene_object,
            s_motion_jitter_retail_scene_owner, position)) return;
    candidate[0] = position[0] + s_motion_jitter_direction[0];
    candidate[1] = position[1] + s_motion_jitter_direction[1];
    candidate[2] = position[2] + s_motion_jitter_direction[2];
    valid = motion_jitter_candidate_valid_retail(candidate);
    if (valid == 0) {
        candidate[0] = position[0] - s_motion_jitter_direction[0];
        candidate[1] = position[1] - s_motion_jitter_direction[1];
        candidate[2] = position[2] - s_motion_jitter_direction[2];
        valid = motion_jitter_candidate_valid_retail(candidate);
        if (valid == 0) {
            s_motion_jitter_flags &= 0xffffff7fu;
            return;
        }
    }
    (void)ddiscene_object_position_set(s_motion_jitter_retail_scene_object,
        s_motion_jitter_retail_scene_owner, candidate[0], candidate[1],
        candidate[2]);
    s_motion_jitter_remaining = s_motion_jitter_remaining -
        s_motion_jitter_duration;
    if (0.0f < s_motion_jitter_remaining) return;
    s_motion_jitter_flags &= 0xffffff7fu;
#else
    MotionJitterHooks hooks;
    float position[3];
    if (s_motion_gameplay_scene_object == NULL ||
        (s_motion_jitter_state.flags & 0x40u) == 0u ||
        !ddiscene_object_position_get(s_motion_gameplay_scene_object,
            s_motion_gameplay_scene_owner, position)) return;
    memcpy(s_motion_jitter_state.position, position, sizeof(position));
    memset(&hooks, 0, sizeof(hooks));
    hooks.random_vector = motion_jitter_vector_random;
    hooks.candidate_valid = motion_jitter_candidate_valid;
    hooks.position_apply = motion_jitter_position_apply;
    hooks.context = s_motion_waypoint_user;
    motion_jitter_state_step(&s_motion_jitter_state, delta,
        s_motion_waypoint_random_range, &hooks);
#endif
}
// FUNCTION: LEGORR 0x0044D9A0
int motion_jitter_candidate_valid_retail(float position[3])
{
    if (fabs((double)(position[0] - s_motion_jitter_center[0])) >
        (double)s_motion_jitter_tolerance[0])
        return 0;
    if (fabs((double)(position[1] - s_motion_jitter_center[1])) >
        (double)s_motion_jitter_tolerance[1])
        return 0;
    if (fabs((double)(position[2] - s_motion_jitter_center[2])) >
        (double)s_motion_jitter_tolerance[2])
        return 0;
    return 1;
}

/* RoutePositionBlockedStateSet (FUN_0044da20) — see simtick.hpp. */
// FUNCTION: LEGORR 0x0044da20
void simtick_route_position_blocked_state_set(int blocked)
{
    if (blocked != 0) {
        s_motion_jitter_flags = (s_motion_jitter_flags & ~0x400u) | 0x200u;
        return;
    }
    s_motion_jitter_flags &= ~0x1200u; /* bits 9 and 12; bit 10 survives */
}

static int light_effect_config_path_build(char* path, size_t path_capacity,
                                          const char* config_root,
                                          const char* key)
{
    static const char middle[] = "::LightEffects::";
    size_t root_length;
    size_t middle_length = sizeof(middle) - 1u;
    size_t key_length;
    if (path == NULL || config_root == NULL || key == NULL ||
        path_capacity == 0u) return 0;
    root_length = strlen(config_root);
    key_length = strlen(key);
    if (root_length + middle_length + key_length + 1u > path_capacity)
        return 0;
    memcpy(path, config_root, root_length);
    memcpy(path + root_length, middle, middle_length);
    memcpy(path + root_length + middle_length, key, key_length + 1u);
    return 1;
}

static int light_effect_config_float_triplet_read(BlockFile* config,
                                                  const char* path,
                                                  float values[3])
{
    const char* value = block_get_strptr(config, path);
    char copy[256];
    char* cursor;
    unsigned int i;
    size_t length = 0u;
    if (value == NULL || values == NULL) return 0;
    while (length + 1u < sizeof(copy) && value[length] != '\0') {
        copy[length] = value[length];
        ++length;
    }
    if (value[length] != '\0') return 0;
    copy[length] = '\0';
    cursor = copy;
    for (i = 0u; i < 3u; ++i) {
        char* end;
        double parsed = strtod(cursor, &end);
        if (end == cursor) return 0;
        values[i] = (float)parsed;
        while (*end == ' ' || *end == '\t') ++end;
        if (i != 2u) {
            if (*end != ':') return 0;
            cursor = end + 1;
        }
        else if (*end != '\0') {
            return 0;
        }
    }
    return 1;
}

static int motion_jitter_vector_random(void*, float out[3])
{
    if (out == NULL) return 0;
    d3drm_vector_random(out);
    return 1;
}

static int motion_jitter_candidate_valid(void*, const float position[3])
{
    int i;
    if (position == NULL) return 0;
    for (i = 0; i < 3; ++i) {
        const float difference = (float)fabs(
            (double)(position[i] - s_motion_jitter_state.center[i]));
        if (s_motion_jitter_tolerance[i] < difference) return 0;
    }
    return 1;
}

static void motion_jitter_position_apply(void*, const float position[3])
{
    if (s_motion_gameplay_scene_object == NULL || position == NULL) return;
    (void)ddiscene_object_position_set(s_motion_gameplay_scene_object,
        s_motion_gameplay_scene_owner, position[0], position[1], position[2]);
}
