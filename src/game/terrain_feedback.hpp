// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGO_GAME_TERRAIN_FEEDBACK_HPP
#define LEGO_GAME_TERRAIN_FEEDBACK_HPP

#include <stdint.h>

typedef struct TerrainCommandFeedbackState {
    int cell_x;
    int cell_y;
    int last_cell_x;
    int last_cell_y;
    uint32_t cell_flags;
    uint32_t cell_state;
    uint32_t auxiliary_flags;
    unsigned char cell_kind;
    int primary_value;
    int secondary_value;
} TerrainCommandFeedbackState;

typedef struct TerrainCommandFeedbackHooks {
    int (*detail_text_build)(void* context,
                             const TerrainCommandFeedbackState* state,
                             char* output, unsigned int capacity);
    void (*message_show)(void* context, int message_group,
                         const char* text, int message_id);
    void (*message_clear)(void* context, int message_group);
    int (*message_enabled)(void* context, int message_id);
    void* context;
} TerrainCommandFeedbackHooks;

static inline int terrain_command_feedback_dispatch(
    TerrainCommandFeedbackState* state, int details_requested,
    int parameter_four, const TerrainCommandFeedbackHooks* hooks)
{
    int message_id = -1;
    char text[256];
    if (state == 0 || hooks == 0) return 0;
    if (state->last_cell_x != state->cell_x ||
        state->last_cell_y != state->cell_y) {
        state->last_cell_x = state->cell_x;
        state->last_cell_y = state->cell_y;
        return 0;
    }
    if ((state->cell_flags & 0x100000u) != 0u &&
        (state->cell_flags & 0x400u) == 0u &&
        (state->cell_state & 4u) == 0u &&
        (state->cell_flags & 0x8000u) == 0u && details_requested) {
        text[0] = '\0';
        if (hooks->detail_text_build != 0)
            hooks->detail_text_build(hooks->context, state, text,
                                     (unsigned int)sizeof(text));
        if (hooks->message_show != 0)
            hooks->message_show(hooks->context, 3, text, -1);
        if (hooks->message_clear != 0) {
            hooks->message_clear(hooks->context, 3);
            hooks->message_clear(hooks->context, 0);
        }
        return 1;
    }
    if ((state->cell_flags & 8u) == 0u) {
        if ((state->cell_flags & 0x10u) == 0u &&
            (state->cell_flags & 0x20000u) != 0u) message_id = 16;
        else if ((state->cell_flags & 0x20u) != 0u) message_id = 13;
        else message_id = state->cell_kind;
    }
    else if ((state->cell_flags & 0x80000000u) != 0u) message_id = 0;
    else if ((state->cell_flags & 0x200000u) != 0u)
        message_id = (state->cell_state & 0x20u) != 0u ? 15 : 14;
    else if (parameter_four != 0 &&
             (state->cell_flags & 0x400u) == 0u &&
             (state->cell_state & 4u) == 0u) message_id = 17;
    else message_id = state->cell_kind;
    if (hooks->message_enabled != 0 &&
        !hooks->message_enabled(hooks->context, message_id)) return 0;
    if (hooks->message_show != 0)
        hooks->message_show(hooks->context, 6, 0, message_id);
    if (hooks->message_clear != 0) hooks->message_clear(hooks->context, 3);
    return 1;
}

#endif
