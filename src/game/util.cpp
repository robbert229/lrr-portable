// SPDX-License-Identifier: LGPL-3.0-or-later
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <math.h>
#include <locale.h>

#include "crt/crt.hpp"
#include "gods98/resource.hpp"
#include "gods98/heap.hpp"
#include "gods98/view.hpp"
#include "effects.hpp"
#include "port_loop_ports.hpp"
#include "util.hpp"

extern "C" float random_float_range(float minimum, float maximum);
extern "C" short rng_thunk_draw15(void);

static ParsedEntityHealthValue s_parsed_entity_health_value;

#if defined(_MSC_VER) && defined(_M_IX86)
extern "C" uint32_t __cdecl __strgtold12(uint16_t*, char**,
                                           const char*, uint32_t,
                                           int, int, int);
extern "C" uint32_t __cdecl _ld12tod(const uint16_t*, uint32_t*);
#endif

/* FUNCTION: LEGORR 0x00481AC0 */
int* string_trailing_digits_split(const char* text, char* prefix,
                                  int* value, int* digit_count)
{
    size_t length;
    size_t split;
    size_t digits;
    uint32_t number;
    uint32_t place;
    if (text == NULL || prefix == NULL || value == NULL
        || digit_count == NULL) {
        return NULL;
    }
    length = strlen(text);
    if (length < 2u || text[length - 1u] < '0' || text[length - 1u] > '9') {
        return NULL;
    }
    split = length - 1u;
    number = 0u;
    place = 1u;
    digits = 0u;
    while (split != 0u) {
        unsigned char ch = (unsigned char)text[split];
        uint32_t digit;
        if (ch < (unsigned char)'0' || ch > (unsigned char)'9') {
            break;
        }
        digit = (uint32_t)(ch - (unsigned char)'0');
        number += digit * place;
        place *= 10u;
        ++digits;
        --split;
    }
    if (digits == 0u || split == 0u) {
        return NULL;
    }
    strcpy(prefix, text);
    prefix[split + 1u] = '\0';
    *value = (int)number;
    *digit_count = (int)digits;
    return digit_count;
}

/* FUNCTION: LEGORR 0x00481B80 */
void counter_format_string_build(const char* prefix, char* out,
                                 int* counter, int width)
{
    char digits[16];
    int padding;
    size_t prefix_length;
    size_t digit_length;
    if (prefix == NULL || out == NULL || counter == NULL) {
        return;
    }
    strcpy(out, prefix);
    prefix_length = strlen(out);
    sprintf(digits, "%d", *counter);
    digit_length = strlen(digits);
    padding = width - (int)digit_length;
    while (padding > 0) {
        out[prefix_length] = '0';
        ++prefix_length;
        --padding;
    }
    out[prefix_length] = '\0';
    ++*counter;
    strcat(out, digits);
}

/* FUNCTION: LEGORR 0x004907f0 */
const ParsedEntityHealthValue* parse_entity_health_value(
    const char* text, int length, int flags, int mode)
{
    uint32_t scan_status = 0u;
    uint32_t conversion_status = 0u;
    char* end = NULL;
    char* conversion_text = NULL;
    size_t consumed = 0u;
    double value = 0.0;
    (void)length;
    (void)flags;
    (void)mode;
    memset(&s_parsed_entity_health_value, 0,
           sizeof(s_parsed_entity_health_value));
    if (text == NULL) {
        s_parsed_entity_health_value.flags = 0x200u;
        return &s_parsed_entity_health_value;
    }

#if defined(_MSC_VER) && defined(_M_IX86)
    {
        uint16_t extended[6];
        char* cursor = (char*)text;
        memset(extended, 0, sizeof(extended));
        /* Target FUN_004907f0 calls ___strgtold12 with all four optional
         * controls zero, then converts its 12-byte extended result via the
         * VC5 binary64 format wrapper. */
        scan_status = __strgtold12(extended, &cursor, text, 0u, 0, 0, 0);
        end = cursor;
        if ((scan_status & 4u) == 0u)
            conversion_status = _ld12tod(
                extended, (uint32_t*)&s_parsed_entity_health_value.value);
        else
            conversion_status = 0u;
        consumed = (size_t)(cursor - text);
    }
#else
    {
        const char* cursor = text;
        const char* token_end;
        const char* decimal_point = ".";
        struct lconv* locale = localeconv();
        size_t digits_before = 0u;
        size_t digits_after = 0u;
        size_t token_length;

        if (locale != NULL && locale->decimal_point != NULL &&
            locale->decimal_point[0] != '\0')
            decimal_point = locale->decimal_point;
        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n' ||
               *cursor == '\r') ++cursor;
        if (*cursor == '+' || *cursor == '-') ++cursor;
        while (*cursor >= '0' && *cursor <= '9') {
            ++digits_before;
            ++cursor;
        }
        if ((unsigned char)*cursor == (unsigned char)decimal_point[0]) {
            ++cursor;
            while (*cursor >= '0' && *cursor <= '9') {
                ++digits_after;
                ++cursor;
            }
        }
        if (digits_before + digits_after == 0u) {
            end = (char*)cursor;
            scan_status = 4u;
            consumed = (size_t)(cursor - text);
        }
        else {
            if (*cursor == 'e' || *cursor == 'E') {
                const char* exponent = cursor;
                const char* exponent_digits;
                ++cursor;
                if (*cursor == '+' || *cursor == '-') ++cursor;
                exponent_digits = cursor;
                while (*cursor >= '0' && *cursor <= '9') ++cursor;
                if (cursor == exponent_digits) cursor = exponent;
            }
            token_end = cursor;
            end = (char*)token_end;
            consumed = (size_t)(token_end - text);
            token_length = (size_t)(token_end - text);
            if (token_length > 0x7ffffffeu) {
                scan_status = 4u;
            }
            else {
                conversion_text = (char*)engine_heap_alloc(
                    (long)(token_length + 1u));
                if (conversion_text != NULL) {
                    memcpy(conversion_text, text, token_length);
                    conversion_text[token_length] = '\0';
                    errno = 0;
                    conversion_status = crt_number_string_to_binary64(
                        (uint32_t*)&value, conversion_text);
                    if (errno == ERANGE) {
                        if (isinf(value)) conversion_status = 1u;
                        else conversion_status = 2u;
                    }
                    s_parsed_entity_health_value.value = value;
                    engine_heap_free(conversion_text);
                }
                else {
                    scan_status = 4u;
                }
            }
        }
    }
#endif

    if ((scan_status & 4u) != 0u) {
        s_parsed_entity_health_value.flags = 0x200u;
        s_parsed_entity_health_value.value = 0.0;
    }
    else {
        if ((scan_status & 2u) != 0u || conversion_status == 1u)
            s_parsed_entity_health_value.flags |= 0x80u;
        if ((scan_status & 1u) != 0u || conversion_status == 2u)
            s_parsed_entity_health_value.flags |= 0x100u;
    }
    s_parsed_entity_health_value.consumed =
        consumed > 0xffffffffu ? 0xffffffffu : (uint32_t)consumed;
    (void)end;
    return &s_parsed_entity_health_value;
}

static MotionDriftState* s_motion_drift_state;
static MotionRandomRangeFn s_motion_drift_random_range;
static void* s_motion_drift_user;
static MotionWaypointTimerState s_motion_waypoint_storage;
static MotionWaypointTimerState* s_motion_waypoint_state =
    &s_motion_waypoint_storage;
static PortLoopRandomRangeHook s_motion_waypoint_random_range;
static void* s_motion_waypoint_user;
static void* s_motion_gameplay_scene_object;
static void* s_motion_gameplay_scene_owner;
static uint32_t s_motion_gameplay_scene_flags;
static MotionJitterState s_motion_jitter_state;
// GLOBAL: LEGORR 0x004EAE18
static float s_motion_waypoint_target[3];
// GLOBAL: LEGORR 0x004EAE24
static float s_motion_waypoint_bounds_min[3];
// GLOBAL: LEGORR 0x004EAE30
static float s_motion_waypoint_bounds_max[3];
// GLOBAL: LEGORR 0x004EAE3C
static float s_motion_waypoint_delay;
// GLOBAL: LEGORR 0x004EAE40
static float s_motion_waypoint_delay_range[2];
// GLOBAL: LEGORR 0x004EAE48
static float s_motion_waypoint_velocity[3];
// GLOBAL: LEGORR 0x004EAE54
static float s_motion_waypoint_scale_range[2];
// GLOBAL: LEGORR 0x004EAE5C
static float s_motion_waypoint_duration;
// GLOBAL: LEGORR 0x004EAE60
static float s_motion_waypoint_duration_range[2];
// GLOBAL: LEGORR 0x004EAE68
static float s_motion_waypoint_progress[3];
// GLOBAL: LEGORR 0x004EADEC
static float s_motion_waypoint_position[3];
// GLOBAL: LEGORR 0x004EADD8
void* s_motion_jitter_retail_scene_object;
// GLOBAL: LEGORR 0x004EADDC
void* s_motion_jitter_retail_scene_owner;
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
static void motion_jitter_center_refresh(void);

static float motion_waypoint_default_random(void* context, float minimum,
                                            float maximum)
{
    (void)context;
    return random_float_range(minimum, maximum);
}

void motion_drift_state_install(MotionDriftState* state,
                                MotionRandomRangeFn random_range,
                                void* user)
{
    s_motion_drift_state = state;
    s_motion_drift_random_range = random_range;
    s_motion_drift_user = user;
}

void motion_waypoint_timer_state_install(MotionWaypointTimerState* state,
                                         PortLoopRandomRangeHook random_range,
                                         void* context)
{
    s_motion_waypoint_state = state != NULL ? state :
        &s_motion_waypoint_storage;
    s_motion_waypoint_random_range = random_range;
    s_motion_waypoint_user = context;
#if defined(_WIN32) && defined(_M_IX86)
    s_motion_jitter_flags = (s_motion_jitter_flags & ~0x3cu) |
        (s_motion_waypoint_state->flags & 0x3cu);
#endif
}

void motion_gameplay_scene_bind(void* target, uint32_t flags)
{
    s_motion_gameplay_scene_object = target;
    s_motion_gameplay_scene_flags = flags;
    s_motion_jitter_retail_scene_object = target;
}

void motion_gameplay_scene_owner_bind(void* target, void* owner)
{
    if (target == s_motion_gameplay_scene_object) {
        s_motion_gameplay_scene_owner = owner;
        s_motion_jitter_retail_scene_owner = owner;
        motion_jitter_center_refresh();
    }
}

static void motion_jitter_center_refresh(void)
{
    float position[3];
    if (s_motion_gameplay_scene_object != NULL &&
        ddiscene_object_position_get(s_motion_gameplay_scene_object,
            s_motion_gameplay_scene_owner, position)) {
        memcpy(s_motion_jitter_state.center, position, sizeof(position));
        memcpy(s_motion_jitter_state.position, position, sizeof(position));
        memcpy(s_motion_jitter_center, position, sizeof(position));
    }
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

static void motion_jitter_position_apply(void*, const float position[3])
{
    if (s_motion_gameplay_scene_object == NULL || position == NULL) return;
    (void)ddiscene_object_position_set(s_motion_gameplay_scene_object,
        s_motion_gameplay_scene_owner, position[0], position[1], position[2]);
}

void motion_jitter_state_step(MotionJitterState* state, float delta,
                              PortLoopRandomRangeHook random_range,
                              const MotionJitterHooks* hooks)
{
    float length;
    float candidate[3];
    int i;
    int valid;
    if (state == NULL || hooks == NULL ||
        hooks->candidate_valid == NULL || (state->flags & 0x40u) == 0u)
        return;
    if ((state->flags & 0x80u) == 0u) {
        state->delay -= delta;
        if (state->delay > 0.0f) return;
        state->delay = random_range != NULL
            ? random_range(hooks->context, state->delay_min, state->delay_max)
            : motion_waypoint_default_random(NULL, state->delay_min,
                                              state->delay_max);
        state->lifetime = random_range != NULL
            ? random_range(hooks->context, state->lifetime_min,
                           state->lifetime_max)
            : motion_waypoint_default_random(NULL, state->lifetime_min,
                                              state->lifetime_max);
        state->remaining = random_range != NULL
            ? random_range(hooks->context, state->magnitude_min,
                           state->magnitude_max)
            : motion_waypoint_default_random(NULL, state->magnitude_min,
                                              state->magnitude_max);
        if (hooks->random_vector != NULL) {
            if (!hooks->random_vector(hooks->context, state->direction))
                return;
        }
        else {
            d3drm_vector_random(state->direction);
        }
        length = (float)sqrt((double)(state->direction[0] * state->direction[0] +
                                      state->direction[1] * state->direction[1] +
                                      state->direction[2] * state->direction[2]));
        if (length == 0.0f) return;
        for (i = 0; i < 3; ++i)
            state->direction[i] = state->direction[i] / length * state->remaining;
        state->flags |= 0x80u;
        return;
    }
    for (i = 0; i < 3; ++i)
        candidate[i] = state->position[i] + state->direction[i];
    valid = hooks->candidate_valid(hooks->context, candidate);
    if (!valid) {
        for (i = 0; i < 3; ++i)
            candidate[i] = state->position[i] - state->direction[i];
        valid = hooks->candidate_valid(hooks->context, candidate);
    }
    if (!valid) {
        state->flags &= ~0x80u;
        return;
    }
    for (i = 0; i < 3; ++i) state->position[i] = candidate[i];
    if (hooks->position_apply != NULL)
        hooks->position_apply(hooks->context, state->position);
    state->remaining -= state->lifetime;
    if (state->remaining <= 0.0f) state->flags &= ~0x80u;
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

void motion_waypoint_position_bind(float x, float y, float z)
{
    MotionWaypointTimerState* state = s_motion_waypoint_state;
    state->configured_position[0] = x;
    state->configured_position[1] = y;
    state->configured_position[2] = z;
    state->position[0] = x;
    state->position[1] = y;
    state->position[2] = z;
#if defined(_WIN32) && defined(_M_IX86)
    s_motion_waypoint_position[0] = x;
    s_motion_waypoint_position[1] = y;
    s_motion_waypoint_position[2] = z;
#endif
}

void motion_waypoint_parameters_store(float min_x, float min_y, float min_z,
                                      float max_x, float max_y, float max_z,
                                      float hold_min, float hold_max,
                                      float scale_min, float scale_max,
                                      float duration_min, float duration_max)
{
    MotionWaypointTimerState* state = s_motion_waypoint_state;
    state->bounds_min[0] = min_x;
    state->bounds_min[1] = min_y;
    state->bounds_min[2] = min_z;
    state->bounds_max[0] = max_x;
    state->bounds_max[1] = max_y;
    state->bounds_max[2] = max_z;
    state->delay_min = hold_min;
    state->delay_max = hold_max;
    state->scale_min = scale_min;
    state->scale_max = scale_max;
    state->duration_min = duration_min;
    state->duration_max = duration_max;
    state->flags |= 4u;
#if defined(_WIN32) && defined(_M_IX86)
    s_motion_waypoint_bounds_min[0] = min_x;
    s_motion_waypoint_bounds_min[1] = min_y;
    s_motion_waypoint_bounds_min[2] = min_z;
    s_motion_waypoint_bounds_max[0] = max_x;
    s_motion_waypoint_bounds_max[1] = max_y;
    s_motion_waypoint_bounds_max[2] = max_z;
    s_motion_waypoint_delay_range[0] = hold_min;
    s_motion_waypoint_delay_range[1] = hold_max;
    s_motion_waypoint_scale_range[0] = scale_min;
    s_motion_waypoint_scale_range[1] = scale_max;
    s_motion_waypoint_duration_range[0] = duration_min;
    s_motion_waypoint_duration_range[1] = duration_max;
    s_motion_jitter_flags |= 4u;
#endif
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

/* FUNCTION: LEGORR 0x0044da50 */
void motion_three_channel_tween_step(float delta)
{
    MotionWaypointTimerState* state = s_motion_waypoint_state;
    int i;
    if (state == NULL) return;
    if ((state->flags & 0x200u) == 0u) {
        if ((state->flags & 0x400u) == 0u) {
            const float step = delta * 0.1f;
            for (i = 0; i < 3; ++i) state->position[i] += step;
            if (state->configured_position[0] <= state->position[0]) {
                state->flags |= 0x400u;
                state->position[0] = state->configured_position[2];
            }
        }
    } else {
        if ((state->flags & 0x1000u) != 0u) {
            state->position[0] = 0.0f;
            state->position[1] = 0.0f;
            state->position[2] = 0.0f;
            return;
        }
        const float step = delta * 0.1f;
        for (i = 0; i < 3; ++i) state->position[i] -= step;
        if (state->position[0] <= 0.0f) {
            state->position[0] = 0.0f;
            state->position[1] = 0.0f;
            state->position[2] = 0.0f;
            state->flags |= 0x1000u;
        }
    }
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

/* FUNCTION: LEGORR 0x0044CFA0 */
/* The corpus caller steps drift, waypoint, jitter, tween and then publishes
 * the position-derived color. Only the owned drift/waypoint/tween state has
 * a source-level owner in this build; the engine ticker still drives those
 * live records with the caller's delta. */
void motion_gameplay_tick(float delta)
{
    float drift_before[3];
    int i;
    if (s_motion_gameplay_scene_object == NULL ||
        (s_motion_gameplay_scene_flags & 0x100u) != 0u ||
        (s_motion_waypoint_state != NULL &&
         (s_motion_waypoint_state->flags & 0x100u) != 0u)) return;
    if (s_motion_waypoint_state == NULL) return;
    for (i = 0; i < 3; ++i) {
        drift_before[i] = s_motion_drift_state != NULL
            ? s_motion_drift_state->offset[i] : 0.0f;
    }
    motion_drift_offset_step(delta);
    if (s_motion_drift_state != NULL) {
        for (i = 0; i < 3; ++i)
            s_motion_waypoint_state->position[i] +=
                s_motion_drift_state->offset[i] - drift_before[i];
    }
#if defined(_WIN32) && defined(_M_IX86)
    s_motion_waypoint_position[0] = s_motion_waypoint_state->position[0];
    s_motion_waypoint_position[1] = s_motion_waypoint_state->position[1];
    s_motion_waypoint_position[2] = s_motion_waypoint_state->position[2];
#endif
    motion_waypoint_timer_tick(delta);
#if defined(_WIN32) && defined(_M_IX86)
    s_motion_waypoint_state->position[0] = s_motion_waypoint_position[0];
    s_motion_waypoint_state->position[1] = s_motion_waypoint_position[1];
    s_motion_waypoint_state->position[2] = s_motion_waypoint_position[2];
    s_motion_waypoint_state->target[0] = s_motion_waypoint_target[0];
    s_motion_waypoint_state->target[1] = s_motion_waypoint_target[1];
    s_motion_waypoint_state->target[2] = s_motion_waypoint_target[2];
    s_motion_waypoint_state->velocity[0] = s_motion_waypoint_velocity[0];
    s_motion_waypoint_state->velocity[1] = s_motion_waypoint_velocity[1];
    s_motion_waypoint_state->velocity[2] = s_motion_waypoint_velocity[2];
    s_motion_waypoint_state->progress[0] = s_motion_waypoint_progress[0];
    s_motion_waypoint_state->progress[1] = s_motion_waypoint_progress[1];
    s_motion_waypoint_state->progress[2] = s_motion_waypoint_progress[2];
    s_motion_waypoint_state->delay = s_motion_waypoint_delay;
    s_motion_waypoint_state->duration = s_motion_waypoint_duration;
    s_motion_waypoint_state->flags = (s_motion_waypoint_state->flags & ~0x3cu) |
        (s_motion_jitter_flags & 0x3cu);
#endif
    motion_jitter_step(delta);
    motion_three_channel_tween_step(delta);
    scene_transform_current_set(s_motion_waypoint_state->position[0],
                                s_motion_waypoint_state->position[1],
                                s_motion_waypoint_state->position[2]);
    scene_transform_rgb_clamp_apply();
}

#define STRING_TABLE_MAX_ENTRIES 256
static const char* s_string_table_entries[STRING_TABLE_MAX_ENTRIES];
// GLOBAL: LEGORR 0x004b8a50
static unsigned int s_string_table_count;

/* The initialized bubble-name pointers reference distinct original .rdata
 * strings; keep same-spelling labels distinct where retail did. */
// GLOBAL: LEGORR 0x004a1ca4
static const char s_bubble_cant_do[] = "Bubble_CantDo";
// GLOBAL: LEGORR 0x004a1c98
static const char s_bubble_idle[] = "Bubble_Idle";
// GLOBAL: LEGORR 0x004a1c80
static const char s_bubble_collect_crystal[] = "Bubble_CollectCrystal";
// GLOBAL: LEGORR 0x004a1c6c
static const char s_bubble_collect_ore[] = "Bubble_CollectOre";
// GLOBAL: LEGORR 0x004a1c58
static const char s_bubble_collect_stud[] = "Bubble_CollectStud";
// GLOBAL: LEGORR 0x004a1c40
static const char s_bubble_collect_dynamite[] = "Bubble_CollectDynamite";
// GLOBAL: LEGORR 0x004a1c28
static const char s_bubble_collect_elec_fence[] = "Bubble_CollectElecFence";
// GLOBAL: LEGORR 0x004a1c10
static const char s_bubble_collect_barrier[] = "Bubble_CollectBarrier";
// GLOBAL: LEGORR 0x004a1bfc
static const char s_bubble_collect_drill[] = "Bubble_CollectDrill";
// GLOBAL: LEGORR 0x004a1be8
static const char s_bubble_collect_spade[] = "Bubble_CollectSpade";
// GLOBAL: LEGORR 0x004a1bd0
static const char s_bubble_collect_hammer[] = "Bubble_CollectHammer";
// GLOBAL: LEGORR 0x004a1bb8
static const char s_bubble_collect_spanner[] = "Bubble_CollectSpanner";
// GLOBAL: LEGORR 0x004a1ba4
static const char s_bubble_collect_laser[] = "Bubble_CollectLaser";
// GLOBAL: LEGORR 0x004a1b8c
static const char s_bubble_collect_pusher[] = "Bubble_CollectPusher";
// GLOBAL: LEGORR 0x004a1b74
static const char s_bubble_collect_freezer[] = "Bubble_CollectFreezer";
// GLOBAL: LEGORR 0x004a1b58
static const char s_bubble_collect_bird_scarer[] = "Bubble_CollectBirdScarer";
// GLOBAL: LEGORR 0x004a1b44
static const char s_bubble_carry_crystal[] = "Bubble_CarryCrystal";
// GLOBAL: LEGORR 0x004a1b34
static const char s_bubble_carry_ore[] = "Bubble_CarryOre";
// GLOBAL: LEGORR 0x004a1b20
static const char s_bubble_carry_stud[] = "Bubble_CarryStud";
// GLOBAL: LEGORR 0x004a1b08
static const char s_bubble_carry_dynamite[] = "Bubble_CarryDynamite";
// GLOBAL: LEGORR 0x004a1af4
static const char s_bubble_carry_barrier[] = "Bubble_CarryBarrier";
// GLOBAL: LEGORR 0x004a1adc
static const char s_bubble_carry_elec_fence[] = "Bubble_CarryElecFence";
// GLOBAL: LEGORR 0x004a1ad0
static const char s_bubble_goto[] = "Bubble_Goto";
// GLOBAL: LEGORR 0x004a1ac0
static const char s_bubble_dynamite[] = "Bubble_Dynamite";
// GLOBAL: LEGORR 0x004a1aac
static const char s_bubble_reinforce[] = "Bubble_Reinforce";
// GLOBAL: LEGORR 0x004a1a9c
static const char s_bubble_drill[] = "Bubble_Drill";
// GLOBAL: LEGORR 0x004a1a8c
static const char s_bubble_repair[] = "Bubble_Repair";
// GLOBAL: LEGORR 0x004a1a80
static const char s_bubble_dig[] = "Bubble_Dig";
// GLOBAL: LEGORR 0x004a1a74
static const char s_bubble_flee[] = "Bubble_Flee";
// GLOBAL: LEGORR 0x004a1a64
static const char s_bubble_power_off[] = "Bubble_PowerOff";
// GLOBAL: LEGORR 0x004a1a50
static const char s_bubble_call_to_arms[] = "Bubble_CallToArms";
// GLOBAL: LEGORR 0x004a1a3c
static const char s_bubble_elec_fence[] = "Bubble_ElecFence";
// GLOBAL: LEGORR 0x004a1a30
static const char s_bubble_eat[] = "Bubble_Eat";
// GLOBAL: LEGORR 0x004a1a20
static const char s_bubble_drive[] = "Bubble_Drive";
// GLOBAL: LEGORR 0x004a1a10
static const char s_bubble_upgrade[] = "Bubble_Upgrade";
// GLOBAL: LEGORR 0x004a19fc
static const char s_bubble_build_path[] = "Bubble_BuildPath";
// GLOBAL: LEGORR 0x004a19ec
static const char s_bubble_train[] = "Bubble_Train";
// GLOBAL: LEGORR 0x004a19dc
static const char s_bubble_recharge[] = "Bubble_Recharge";
// GLOBAL: LEGORR 0x004a19cc
static const char s_bubble_request[] = "Bubble_Request";
// GLOBAL: LEGORR 0x00557900
static const char* s_bubble_action_strings[BUBBLE_ACTION_STRING_COUNT];

#define PENDING_RECORD_CAPACITY 256
static uint32_t s_pending_record_rows[PENDING_RECORD_CAPACITY][5];
static unsigned int s_pending_record_count;
static uint32_t s_pending_record_flags;
static void (*s_pending_record_release_hook)(uint32_t* row);

#define LEVEL_REMAP_CAPACITY 128
static uint32_t s_level_remap_pairs[LEVEL_REMAP_CAPACITY][2];
static unsigned int s_level_remap_count;

static EntityLinkList** s_entity_link_lists;
static void (*s_entity_link_node_release_hook)(EntityLinkNode*);
static void (*s_entity_link_list_release_hook)(EntityLinkList*);
static void (*s_resource_entry_release_hook)(void*);
static void (*s_resource_array_release_hook)(void*);

#define STRING_QUEUE_CAPACITY 15
#define STRING_QUEUE_TEXT_CAPACITY 1024
struct StringQueueEntry {
    uint32_t first;
    uint32_t second;
    uint32_t third;
    char text[STRING_QUEUE_TEXT_CAPACITY];
};
static StringQueueEntry s_string_queue[STRING_QUEUE_CAPACITY];
static unsigned int s_string_queue_count;
static StringQueueFlushHook s_string_queue_flush_hook;

void string_table_install(const char* const* entries, unsigned int count)
{
    unsigned int i;
    if (count > STRING_TABLE_MAX_ENTRIES) count = STRING_TABLE_MAX_ENTRIES;
    s_string_table_count = count;
    for (i = 0; i < count; ++i) {
        s_string_table_entries[i] = entries != NULL ? entries[i] : NULL;
    }
    for (; i < STRING_TABLE_MAX_ENTRIES; ++i) {
        s_string_table_entries[i] = NULL;
    }
}

/* StringTableIndexLookup (FUN_00408c80): scan the configured name-pointer
 * table in order and return the first exact byte-string match.  The retail
 * comparator loads two bytes per iteration, but still checks both bytes and
 * stops at the same NUL terminator as an ordinary byte walk.  A miss returns
 * the table count. */
// FUNCTION: LEGORR 0x00408c80
unsigned int string_table_index_lookup(const char* value)
{
    unsigned int i;
    if (s_string_table_count == 0 || value == NULL) return 0;
    for (i = 0; i < s_string_table_count; ++i) {
        const unsigned char* left = (const unsigned char*)
            s_string_table_entries[i];
        const unsigned char* right = (const unsigned char*)value;
        int equal = 1;
        if (left == NULL) continue;
        for (;;) {
            unsigned char first = *left;
            unsigned char second = *right;
            if (first != second) {
                equal = 0;
                break;
            }
            if (first == 0) break;
            first = left[1];
            second = right[1];
            if (first != second) {
                equal = 0;
                break;
            }
            left += 2;
            right += 2;
            if (second == 0) break;
        }
        if (equal) return i;
    }
    return s_string_table_count;
}

// FUNCTION: LEGORR 0x004070b0
void bubble_action_strings_initialize(void)
{
    s_bubble_action_strings[0] = s_bubble_cant_do;
    s_bubble_action_strings[1] = s_bubble_idle;
    s_bubble_action_strings[2] = s_bubble_collect_crystal;
    s_bubble_action_strings[3] = s_bubble_collect_ore;
    s_bubble_action_strings[4] = s_bubble_collect_stud;
    s_bubble_action_strings[5] = s_bubble_collect_dynamite;
    s_bubble_action_strings[7] = s_bubble_collect_elec_fence;
    s_bubble_action_strings[6] = s_bubble_collect_barrier;
    s_bubble_action_strings[8] = s_bubble_collect_drill;
    s_bubble_action_strings[9] = s_bubble_collect_spade;
    s_bubble_action_strings[10] = s_bubble_collect_hammer;
    s_bubble_action_strings[11] = s_bubble_collect_spanner;
    s_bubble_action_strings[12] = s_bubble_collect_laser;
    s_bubble_action_strings[13] = s_bubble_collect_pusher;
    s_bubble_action_strings[14] = s_bubble_collect_freezer;
    s_bubble_action_strings[15] = s_bubble_collect_bird_scarer;
    s_bubble_action_strings[16] = s_bubble_carry_crystal;
    s_bubble_action_strings[17] = s_bubble_carry_ore;
    s_bubble_action_strings[18] = s_bubble_carry_stud;
    s_bubble_action_strings[19] = s_bubble_carry_dynamite;
    s_bubble_action_strings[20] = s_bubble_carry_barrier;
    s_bubble_action_strings[21] = s_bubble_carry_elec_fence;
    s_bubble_action_strings[22] = s_bubble_goto;
    s_bubble_action_strings[23] = s_bubble_dynamite;
    s_bubble_action_strings[24] = s_bubble_reinforce;
    s_bubble_action_strings[25] = s_bubble_drill;
    s_bubble_action_strings[26] = s_bubble_repair;
    s_bubble_action_strings[27] = s_bubble_dig;
    s_bubble_action_strings[28] = s_bubble_flee;
    s_bubble_action_strings[29] = s_bubble_power_off;
    s_bubble_action_strings[30] = s_bubble_call_to_arms;
    s_bubble_action_strings[31] = s_bubble_elec_fence;
    s_bubble_action_strings[32] = s_bubble_eat;
    s_bubble_action_strings[33] = s_bubble_drive;
    s_bubble_action_strings[34] = s_bubble_upgrade;
    s_bubble_action_strings[35] = s_bubble_build_path;
    s_bubble_action_strings[36] = s_bubble_train;
    s_bubble_action_strings[37] = s_bubble_recharge;
    s_bubble_action_strings[38] = s_bubble_request;
}

const char* bubble_action_string_get(unsigned int index)
{
    if (index >= BUBBLE_ACTION_STRING_COUNT) return NULL;
    /* GameConfigInit's portable boot counterpart initializes this table. Keep
     * direct utility callers safe when they run before that boot path. */
    if (s_bubble_action_strings[0] == NULL) {
        bubble_action_strings_initialize();
    }
    return s_bubble_action_strings[index];
}

/* FUNCTION: LEGORR 0x00451fd0
 * The reference scans 65 resource records and compares each name at +8
 * case-insensitively. This seam accepts the table explicitly; a miss
 * returns the final comparison result just as the reference does. */
int case_insensitive_name_table_lookup_at_offset8(const void* const* entries,
                                                   unsigned int count,
                                                   const char* name)
{
    unsigned int i;
    int comparison = -1;
    if (entries == NULL || name == NULL) return comparison;
    for (i = 0; i < count; ++i) {
        const unsigned char* left;
        const unsigned char* right;
        const char* entry = (const char*)entries[i];
        if (entry == NULL) {
            comparison = 1;
        } else {
            left = (const unsigned char*)(entry + 8);
            right = (const unsigned char*)name;
            do {
                const int a = tolower(*left++);
                const int b = tolower(*right++);
                comparison = a - b;
            } while (comparison == 0 && left[-1] != 0);
        }
        if (comparison == 0) return (int)i;
    }
    return comparison;
}

void pending_record_table_install(const uint32_t* rows, unsigned int count)
{
    unsigned int i;
    unsigned int j;
    if (count > PENDING_RECORD_CAPACITY) count = PENDING_RECORD_CAPACITY;
    s_pending_record_count = count;
    for (i = 0; i < count; ++i) {
        for (j = 0; j < 5; ++j)
            s_pending_record_rows[i][j] = rows != NULL ? rows[i * 5 + j] : 0;
    }
    for (; i < PENDING_RECORD_CAPACITY; ++i) {
        for (j = 0; j < 5; ++j) s_pending_record_rows[i][j] = 0;
    }
}

void pending_record_release_hook_set(void (*hook)(uint32_t* row))
{
    s_pending_record_release_hook = hook;
}

unsigned int pending_record_count_get(void)
{
    return s_pending_record_count;
}

// FUNCTION: LEGORR 0x00419de0
void pending_record_list_entry_move_to_front(unsigned int index)
{
    uint32_t row[5];
    unsigned int i;
    unsigned int j;
    if (index < s_pending_record_count) {
        if (index != 0) {
            for (j = 0; j < 5; ++j) row[j] = s_pending_record_rows[index][j];
            for (i = index; i > 0; --i) {
                for (j = 0; j < 5; ++j)
                    s_pending_record_rows[i][j] = s_pending_record_rows[i - 1][j];
            }
            for (j = 0; j < 5; ++j) s_pending_record_rows[0][j] = row[j];
        }
        s_pending_record_flags |= 3u;
    }
}

// FUNCTION: LEGORR 0x00419e80
void pending_record_list_entry_remove_and_release(unsigned int index)
{
    unsigned int i;
    unsigned int j;
    if (index >= s_pending_record_count) return;
    if (s_pending_record_release_hook != NULL)
        s_pending_record_release_hook(s_pending_record_rows[index]);
    if (s_pending_record_rows[index][1] == 0) {
        for (i = index; i + 1 < s_pending_record_count; ++i) {
            for (j = 0; j < 5; ++j)
                s_pending_record_rows[i][j] = s_pending_record_rows[i + 1][j];
        }
        --s_pending_record_count;
        for (j = 0; j < 5; ++j)
            s_pending_record_rows[s_pending_record_count][j] = 0;
    }
    if (index == 0) s_pending_record_flags &= ~1u;
}

/* PendingRecordReleaseIfPresent (FUN_00419e60): release the head row only
 * when the table is nonempty. */
// FUNCTION: LEGORR 0x00419E60
void pending_record_release_if_present(void)
{
    if (s_pending_record_count != 0u) {
        pending_record_list_entry_remove_and_release(0u);
    }
}

/* PendingRecordReleaseAll (FUN_00419f10): repeatedly release the head until
 * the table empties. The guard prevents a malformed retained row from
 * trapping the portable loop when its release hook does not clear +4. */
// FUNCTION: LEGORR 0x00419F10
void pending_record_release_all(void)
{
    unsigned int attempts = 0u;
    while (s_pending_record_count != 0u &&
           attempts < PENDING_RECORD_CAPACITY) {
        const unsigned int before = s_pending_record_count;
        pending_record_list_entry_remove_and_release(0u);
        ++attempts;
        if (s_pending_record_count == before) {
            break;
        }
    }
}

void level_identifier_remap_table_install(const uint32_t* pairs,
                                          unsigned int count)
{
    unsigned int i;
    if (count > LEVEL_REMAP_CAPACITY) count = LEVEL_REMAP_CAPACITY;
    s_level_remap_count = count;
    for (i = 0; i < count; ++i) {
        s_level_remap_pairs[i][0] = pairs != NULL ? pairs[i * 2] : 0;
        s_level_remap_pairs[i][1] = pairs != NULL ? pairs[i * 2 + 1] : 0;
    }
}

// FUNCTION: LEGORR 0x0045d870
void command_entity_reference_remap(uint32_t* entity_reference)
{
    unsigned int i;
    if (entity_reference == NULL) return;
    for (i = 0; i < s_level_remap_count; ++i) {
        if (*entity_reference == s_level_remap_pairs[i][0]) {
            *entity_reference = s_level_remap_pairs[i][1];
            return;
        }
    }
}

void entity_link_lists_install(EntityLinkList** head)
{
    s_entity_link_lists = head;
}

void entity_link_release_hooks_set(void (*node_hook)(EntityLinkNode*),
                                  void (*list_hook)(EntityLinkList*))
{
    s_entity_link_node_release_hook = node_hook;
    s_entity_link_list_release_hook = list_hook;
}

// FUNCTION: LEGORR 0x00419830
void entity_linked_list_entries_remove_by_key(uintptr_t key)
{
    EntityLinkList** list_link;
    if (s_entity_link_lists == NULL) return;
    list_link = s_entity_link_lists;
    while (*list_link != NULL) {
        EntityLinkList* list = *list_link;
        EntityLinkNode** node_link = &list->head;
        while (*node_link != NULL) {
            EntityLinkNode* node = *node_link;
            if (node->key == key) {
                /* FUN_004197b0 returns the removed entry, but 0x419830
                 * discards that pointer; this path does not release it. */
                *node_link = node->next;
                --list->count;
            }
            else {
                node_link = &node->next;
            }
        }
        if (list->count == 0) {
            /* FUN_00419e80 compacts the empty indexed registry record; it
             * does not free the host mirror record itself. */
            *list_link = list->next;
        }
        else {
            list_link = &list->next;
        }
    }
}

// FUNCTION: LEGORR 0x004198f0
void terrain_cell_linked_entries_remove(uint32_t key)
{
    EntityLinkList** list_link;
    if (s_entity_link_lists == NULL) return;
    list_link = s_entity_link_lists;
    while (*list_link != NULL) {
        EntityLinkList* list = *list_link;
        if (list->kind != 6 && list->kind != 7) {
            list_link = &list->next;
            continue;
        }
        EntityLinkNode** node_link = &list->head;
        while (*node_link != NULL) {
            EntityLinkNode* node = *node_link;
            if (node->key == key) {
                *node_link = node->next;
                --list->count;
                if (s_entity_link_node_release_hook != NULL)
                    s_entity_link_node_release_hook(node);
            }
            else {
                node_link = &node->next;
            }
        }
        if (list->count == 0) {
            *list_link = list->next;
            if (s_entity_link_list_release_hook != NULL)
                s_entity_link_list_release_hook(list);
        }
        else {
            list_link = &list->next;
        }
    }
}

void rectangular_resource_table_release_hooks_set(void (*resource_hook)(void*),
                                                   void (*array_hook)(void*))
{
    s_resource_entry_release_hook = resource_hook;
    s_resource_array_release_hook = array_hook;
}

// FUNCTION: LEGORR 0x0042b480
void rectangular_resource_table_destroy(RectangularResourceTable* table)
{
    unsigned long i;
    unsigned long total;
    if (table == NULL || table->resources == NULL) return;
    total = (unsigned long)table->width * (unsigned long)table->height;
    for (i = 0; i < total; ++i) {
        if (s_resource_entry_release_hook != NULL)
            s_resource_entry_release_hook(table->resources[i]);
        else
            unported_ledger_stubs::cell_visual_channel_table_dispose(
                (unported_ledger_stubs::CellVisualChannelTable*)
                    table->resources[i]);
    }
    if (s_resource_array_release_hook != NULL)
        s_resource_array_release_hook(table->resources);
    else
        engine_heap_free(table->resources);
    table->resources = NULL;
}

void string_queue_flush_hook_set(StringQueueFlushHook hook)
{
    s_string_queue_flush_hook = hook;
}

unsigned int string_queue_count_get(void)
{
    return s_string_queue_count;
}

// FUNCTION: LEGORR 0x00410320
void string_queue_record_or_flush(uint32_t first, uint32_t second,
                                  uint32_t third, const char* text,
                                  int record)
{
    unsigned int i;
    if (!record) {
        if (s_string_queue_flush_hook != NULL) {
            for (i = 0; i < s_string_queue_count; ++i) {
                s_string_queue_flush_hook(s_string_queue[i].first,
                                          s_string_queue[i].second,
                                          s_string_queue[i].third,
                                          s_string_queue[i].text);
            }
        }
        s_string_queue_count = 0;
        return;
    }
    if (s_string_queue_count >= STRING_QUEUE_CAPACITY || text == NULL) return;
    s_string_queue[s_string_queue_count].first = first;
    s_string_queue[s_string_queue_count].second = second;
    s_string_queue[s_string_queue_count].third = third;
    strncpy(s_string_queue[s_string_queue_count].text, text,
            STRING_QUEUE_TEXT_CAPACITY - 1);
    s_string_queue[s_string_queue_count].text[STRING_QUEUE_TEXT_CAPACITY - 1] = '\0';
    ++s_string_queue_count;
}

// FUNCTION: LEGORR 0x00410280
int callback_operation_run_and_return_status(void* stack_owner,
                                             CallbackOperationNode* first,
                                             CallbackOperationVisit visit)
{
    int status = 1;
    CallbackOperationNode* node;
    if (visit == NULL) return status;
    for (node = first; node != NULL; node = node->next)
        visit(stack_owner, node, &status);
    return status;
}

const char* util_root_name(BlockFile* bf)
{
    static char root_buf[64];
    if (bf == NULL || block_record_count(bf) == 0) {
        return "Lego*";
    }
    BlockRec* r0 = block_record_at(bf, 0);
    const char* name = r0 ? block_name(r0) : NULL;
    snprintf(root_buf, sizeof(root_buf), "%s", name != NULL ? name : "Lego*");
    return root_buf;
}

const char* util_level_cfg_path(BlockFile* cfg, const char* level,
                                const char* suffix, char* out, size_t out_sz)
{
    if (cfg == NULL || level == NULL) {
        return NULL;
    }
    const char* root = util_root_name(cfg);
    snprintf(out, out_sz, "%s::Levels::%s::%s", root, level,
             suffix != NULL ? suffix : "");
    if (block_get_strptr(cfg, out) != NULL) {
        return out;
    }
    snprintf(out, out_sz, "%s::%s::%s", root, level,
             suffix != NULL ? suffix : "");
    return block_get_strptr(cfg, out) != NULL ? out : NULL;
}

/* Split a NUL-terminated token stream produced by `src` into fields.
 * `|` is a field separator too (the config's pipe-delimited
 * `file.bmp|x|y` triples, finding 09 §"Config format"). Returns the number
 * of fields written (0..6). */
static int util_tokenize_fields(const char* src,
                                char out[UTIL_QUERY_MAX_FIELDS][256], size_t out_sz)
{
    static const char delimiters[] = " \t\r\n|";
    int n = 0;
    const char* p = src;
    while (*p != '\0' && n < UTIL_QUERY_MAX_FIELDS) {
        p += strspn(p, delimiters);
        if (*p == '\0') {
            break;
        }
        size_t token_length = strcspn(p, delimiters);
        size_t copy_length = token_length;
        if (copy_length >= out_sz) copy_length = out_sz - 1;
        memcpy(out[n], p, copy_length);
        out[n][copy_length] = '\0';
        p += copy_length;
        ++n;
    }
    return n;
}

static int util_status_from_count(int n)
{
    if (n <= 0) {
        return 0;
    }
    if (n == 3) {
        return 3;
    }
    if (n >= 6) {
        return 6;
    }
    return 2;
}

int util_query_fields(const char* path_or_value,
                      char out[UTIL_QUERY_MAX_FIELDS][256], size_t out_sz)
{
    for (int i = 0; i < UTIL_QUERY_MAX_FIELDS; ++i) {
        out[i][0] = '\0';
    }
    if (path_or_value == NULL || path_or_value[0] == '\0') {
        return 0;
    }

    /* Case (b): the value names a resource. Parse its content when it
     * opens; otherwise fall through and parse the value inline (case a). */
    Resource* r = resource_open(path_or_value, RESOURCE_AUTO);
    if (r != NULL) {
        int32_t sz = resource_read(r);
        if (sz > 0) {
            char* buf = (char*)malloc((size_t)sz + 1);
            if (buf != NULL) {
                memcpy(buf, r->data, (size_t)sz);
                buf[sz] = '\0';
                int n = util_tokenize_fields(buf, out, out_sz);
                free(buf);
                resource_free(r);
                return util_status_from_count(n);
            }
        }
        resource_free(r);
    }

    /* Case (a): inline token list (also the path-with-pipes shape some
     * config keys use). */
    int n = util_tokenize_fields(path_or_value, out, out_sz);
    return util_status_from_count(n);
}

namespace unported_ledger_stubs {

static CellVisualChannelReleaseHook s_cell_visual_release_hook;
static void* s_cell_visual_release_context;

void cell_visual_channel_release_hook_set(CellVisualChannelReleaseHook hook,
                                           void* context)
{
    s_cell_visual_release_hook = hook;
    s_cell_visual_release_context = context;
}

CellVisualChannelTable* cell_visual_channel_table_create(void)
{
    CellVisualChannelTable* table = (CellVisualChannelTable*)
        engine_heap_alloc((long)sizeof(CellVisualChannelTable));
    if (table != NULL) memset(table, 0, sizeof(*table));
    return table;
}

int cell_visual_channel_table_resource_set(CellVisualChannelTable* table,
                                            uint32_t channel,
                                            uint32_t index, void* resource)
{
    if (table == NULL || channel >= 2u || index >= 100u) return 0;
    table->resources[channel][index] = resource;
    if (resource != NULL && table->resource_count <= index)
        table->resource_count = index + 1u;
    return 1;
}

// FUNCTION: LEGORR 0x0040b810
void cell_visual_channel_table_resources_clear(CellVisualChannelTable* table)
{
    uint32_t channel;
    uint32_t index;
    if (table == NULL) return;
    table->resource_count = 0;
    for (channel = 0; channel < 2u; channel++) {
        for (index = 0; index < 100u; index++) {
            void* resource = table->resources[channel][index];
            if (resource != NULL) {
                if (s_cell_visual_release_hook != NULL)
                    s_cell_visual_release_hook(s_cell_visual_release_context,
                                               resource);
                else
                    ddiscene_object_destroy(resource);
            }
            table->resources[channel][index] = NULL;
        }
    }
}

// FUNCTION: LEGORR 0x0040b7d0
void cell_visual_channel_table_dispose(CellVisualChannelTable* table)
{
    int index;
    if (table == NULL) return;
    cell_visual_channel_table_resources_clear(table);
    for (index = 0; index < 2; index++) {
        if (table->roots[index] != NULL) {
            if (s_cell_visual_release_hook != NULL)
                s_cell_visual_release_hook(s_cell_visual_release_context,
                                           table->roots[index]);
            else
                ddiscene_object_destroy(table->roots[index]);
        }
    }
    engine_heap_free(table);
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// LevelIdentifierRemapTableLoad
// FUNCTION: LEGORR 0x0045d7e0
void ledger_stub_0045d7e0_LevelIdentifierRemapTableLoad(
    const uint32_t* identifier_pairs, uint32_t pair_count)
{
    level_identifier_remap_table_install(identifier_pairs, pair_count);
}





void ledger_stub_0040b880_CellVisualChannelRelay(
    SceneCellVisualRelayRecord* record, const float corner_points[12],
    uint32_t lookup_selector, uint32_t corner_index, float scale,
    float uniform_scale)
{
    scene_cell_visual_channels_apply(record, corner_points, lookup_selector,
                                     corner_index, scale, uniform_scale);
}


} // namespace unported_ledger_stubs
