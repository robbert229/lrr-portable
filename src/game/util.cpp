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
#include "util_shared.hpp"
#include "util_for_tile_variants.hpp"
#include "util_for_lego.hpp"
#include "util_for_frontend_unit.hpp"

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

MotionDriftState* s_motion_drift_state;
MotionRandomRangeFn s_motion_drift_random_range;
void* s_motion_drift_user;
static MotionWaypointTimerState s_motion_waypoint_storage;
MotionWaypointTimerState* s_motion_waypoint_state =
    &s_motion_waypoint_storage;
PortLoopRandomRangeHook s_motion_waypoint_random_range;
void* s_motion_waypoint_user;
void* s_motion_gameplay_scene_object;
void* s_motion_gameplay_scene_owner;
static uint32_t s_motion_gameplay_scene_flags;
MotionJitterState s_motion_jitter_state;
void motion_jitter_center_refresh(void);

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
float motion_waypoint_default_random(void* context, float minimum,
                                            float maximum)
{
    (void)context;
    return random_float_range(minimum, maximum);
}
#endif /* !LEGORRR_BACKEND_WINAPI */

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

void motion_jitter_center_refresh(void)
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

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
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
#endif /* !LEGORRR_BACKEND_WINAPI */

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

/* Port model of 0x0044da50 (lighteffects.c). */
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

/* Port model of 0x0044cfa0 (lighteffects.c). */
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

const char* s_string_table_entries[STRING_TABLE_MAX_ENTRIES];

// GLOBAL: LEGORR 0x00557900
const char* s_bubble_action_strings[BUBBLE_ACTION_STRING_COUNT];

uint32_t s_pending_record_rows[PENDING_RECORD_CAPACITY][5];
unsigned int s_pending_record_count;
void (*s_pending_record_release_hook)(uint32_t* row);

uint32_t s_level_remap_pairs[LEVEL_REMAP_CAPACITY][2];
unsigned int s_level_remap_count;

EntityLinkList** s_entity_link_lists;
void (*s_entity_link_node_release_hook)(EntityLinkNode*);
void (*s_entity_link_list_release_hook)(EntityLinkList*);
void (*s_resource_entry_release_hook)(void*);
void (*s_resource_array_release_hook)(void*);

unsigned int s_string_queue_count;
StringQueueFlushHook s_string_queue_flush_hook;

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

void rectangular_resource_table_release_hooks_set(void (*resource_hook)(void*),
                                                   void (*array_hook)(void*))
{
    s_resource_entry_release_hook = resource_hook;
    s_resource_array_release_hook = array_hook;
}

void string_queue_flush_hook_set(StringQueueFlushHook hook)
{
    s_string_queue_flush_hook = hook;
}

unsigned int string_queue_count_get(void)
{
    return s_string_queue_count;
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

CellVisualChannelReleaseHook s_cell_visual_release_hook;
void* s_cell_visual_release_context;

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

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

void ledger_stub_0040b880_CellVisualChannelRelay(
    SceneCellVisualRelayRecord* record, const float corner_points[12],
    uint32_t lookup_selector, uint32_t corner_index, float scale,
    float uniform_scale)
{
    scene_cell_visual_channels_apply(record, corner_points, lookup_selector,
                                     corner_index, scale, uniform_scale);
}


} // namespace unported_ledger_stubs
