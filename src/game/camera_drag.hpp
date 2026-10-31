// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef LEGORRR_GAME_CAMERA_DRAG_HPP
#define LEGORRR_GAME_CAMERA_DRAG_HPP

#include <stdint.h>

typedef struct CameraDragState {
    uint32_t flags;
    int anchor_x;
    int anchor_y;
    int current_x;
    int current_y;
    int drag_mode;
} CameraDragState;

typedef struct CameraDragHooks {
    void* context;
    int (*cursor_rect_hit)(void* context, int x, int y, int anchor_x,
                           int anchor_y, int mode);
    void (*cursor_rebase)(void* context, int* x, int* y, int anchor_x,
                          int anchor_y, int drag_mode);
    void (*cursor_position_set)(void* context, int x, int y);
    void (*screen_delta_to_camera)(void* context, float dx, float dy,
                                   int drag_mode, float* horizontal,
                                   float* vertical);
    void (*camera_horizontal_apply)(void* context, float value);
    void (*camera_vertical_apply)(void* context, float value);
} CameraDragHooks;

/* CameraDragFromCursorApply (FUN_0045bcd0): maintain the captured drag state,
 * rebase the cursor when it leaves the initial drag region, map screen deltas
 * to camera offsets, and preserve the reference's directional edge flags. */
static inline int camera_drag_from_cursor_apply(CameraDragState* state,
                                                const CameraDragHooks* hooks,
                                                int x, int y, float delta)
{
    float horizontal = 0.0f;
    float vertical = 0.0f;
    int inside;
    if (state == NULL || hooks == NULL || hooks->cursor_rect_hit == NULL ||
        hooks->screen_delta_to_camera == NULL) return 0;
    inside = hooks->cursor_rect_hit(hooks->context, x, y, state->anchor_x,
                                    state->anchor_y, state->drag_mode);
    if (!inside) {
        if ((state->flags & 1u) == 0u || hooks->cursor_rebase == NULL) return 0;
        hooks->cursor_rebase(hooks->context, &x, &y, state->anchor_x,
                             state->anchor_y, state->drag_mode);
        if (hooks->cursor_position_set != NULL)
            hooks->cursor_position_set(hooks->context, x, y);
    }
    else if (hooks->cursor_rect_hit(hooks->context, x, state->current_y,
                                    state->current_x, state->current_y, 4)) {
        return 1;
    }
    state->flags |= 1u;
    hooks->screen_delta_to_camera(hooks->context,
        (float)(state->current_x - x), (float)(y - state->current_y),
        state->drag_mode, &horizontal, &vertical);
    if (!hooks->cursor_rect_hit(hooks->context, x, state->current_y,
                                state->current_x, state->current_y, 3)) {
        if (!hooks->cursor_rect_hit(hooks->context, state->current_x, y,
                                    state->current_x, state->current_y, 3)) {
            if (hooks->camera_horizontal_apply != NULL)
                hooks->camera_horizontal_apply(hooks->context,
                                               horizontal * delta * 0.04f);
            if (hooks->camera_vertical_apply != NULL)
                hooks->camera_vertical_apply(hooks->context,
                                             vertical * delta * 0.04f);
            state->flags |= (vertical >= 0.0f) ? 0x20u : 0x10u;
            if (horizontal < 0.0f) {
                state->flags |= 0x80u;
                state->current_x = x;
                state->current_y = y;
                return 1;
            }
        }
        else if (hooks->camera_horizontal_apply != NULL) {
            hooks->camera_horizontal_apply(hooks->context,
                                           horizontal * delta * 0.04f);
            if (horizontal < 0.0f) {
                state->flags |= 0x80u;
                state->current_x = x;
                state->current_y = y;
                return 1;
            }
        }
        state->flags |= 0x40u;
    }
    else {
        if (hooks->camera_vertical_apply != NULL)
            hooks->camera_vertical_apply(hooks->context,
                                         vertical * delta * 0.04f);
        state->flags |= (vertical < 0.0f) ? 0x10u : 0x20u;
    }
    state->current_x = x;
    state->current_y = y;
    return 1;
}

#endif
