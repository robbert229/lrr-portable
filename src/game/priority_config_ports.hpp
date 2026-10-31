// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGO_GAME_PRIORITY_CONFIG_PORTS_HPP
#define LEGO_GAME_PRIORITY_CONFIG_PORTS_HPP

#include <stdint.h>

/* Resolved inputs for the three priority panel config sections.  Config
 * parsing and resource creation stay behind hooks, while the port owns the
 * table layout and ordering used by simulation controls. */
typedef struct PriorityImageConfigRow {
    const char* label;
    const char* sound_alias;
    void* images[3];
} PriorityImageConfigRow;

typedef struct PriorityConfigRow {
    uint32_t kind;
    int enabled;
} PriorityConfigRow;

typedef struct PriorityImagePositionRow {
    int x;
    int y;
    int visible;
} PriorityImagePositionRow;

typedef struct PriorityConfigState {
    const char* labels[64];
    const char* sound_aliases[64];
    void* images[64][3];
    uint32_t kinds[64];
    int weights[64];
    int enabled[64];
    PriorityImagePositionRow positions[64];
    uint32_t image_count;
    uint32_t priority_count;
    uint32_t position_count;
} PriorityConfigState;

typedef struct PriorityConfigHooks {
    void (*weight_set)(void* context, uint32_t kind, int weight);
    void (*enabled_set)(void* context, uint32_t kind, int enabled);
    void* context;
} PriorityConfigHooks;

static inline void priority_config_state_init(PriorityConfigState* state)
{
    uint32_t i;
    if (state == 0) return;
    state->image_count = 0;
    state->priority_count = 0;
    state->position_count = 0;
    for (i = 0; i < 64; ++i) {
        state->labels[i] = 0;
        state->sound_aliases[i] = 0;
        state->kinds[i] = 0xffffffffu;
        state->weights[i] = 0;
        state->enabled[i] = 0;
        state->positions[i].x = 0;
        state->positions[i].y = 0;
        state->positions[i].visible = 0;
        state->images[i][0] = 0;
        state->images[i][1] = 0;
        state->images[i][2] = 0;
    }
}

static inline uint32_t priority_images_config_load(
    PriorityConfigState* state, const PriorityImageConfigRow* rows,
    uint32_t count)
{
    uint32_t i;
    if (state == 0) return 0;
    if (count > 64) count = 64;
    state->image_count = count;
    for (i = 0; i < count; ++i) {
        state->labels[i] = rows != 0 ? rows[i].label : 0;
        state->sound_aliases[i] = rows != 0 ? rows[i].sound_alias : 0;
        state->images[i][0] = rows != 0 ? rows[i].images[0] : 0;
        state->images[i][1] = rows != 0 ? rows[i].images[1] : 0;
        state->images[i][2] = rows != 0 ? rows[i].images[2] : 0;
    }
    return count;
}

static inline uint32_t priorities_config_load(
    PriorityConfigState* state, const PriorityConfigRow* rows,
    uint32_t count, const PriorityConfigHooks* hooks)
{
    uint32_t i;
    if (state == 0) return 0;
    if (count > 64) count = 64;
    state->priority_count = count;
    for (i = 0; i < count; ++i) {
        const uint32_t kind = rows != 0 ? rows[i].kind : 0xffffffffu;
        const int enabled = rows != 0 && rows[i].enabled != 0;
        state->kinds[i] = kind;
        state->weights[i] = 0x50 - (int)i * 5;
        state->enabled[i] = enabled;
        if (hooks != 0 && kind != 0xffffffffu) {
            if (hooks->weight_set != 0)
                hooks->weight_set(hooks->context, kind, state->weights[i]);
            if (hooks->enabled_set != 0)
                hooks->enabled_set(hooks->context, kind, enabled);
        }
    }
    return count;
}

static inline uint32_t priorities_image_positions_config_load(
    PriorityConfigState* state, const PriorityImagePositionRow* rows,
    uint32_t count)
{
    uint32_t i;
    if (state == 0) return 0;
    if (count > 64) count = 64;
    state->position_count = count;
    for (i = 0; i < count; ++i) {
        if (rows != 0) {
            state->positions[i].x = rows[i].x;
            state->positions[i].y = rows[i].y;
            state->positions[i].visible = rows[i].visible;
        }
        else {
            state->positions[i].x = 0;
            state->positions[i].y = 0;
            state->positions[i].visible = 0;
        }
    }
    return count;
}

#endif
