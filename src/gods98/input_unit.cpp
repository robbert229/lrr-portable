// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-013 Input.c: the unit's functions in original address order and the
 * state they own, relocated verbatim by tools/relocate_unit.py (ADR 0006).
 * Body hashes: docs/coverage/relocations/G98-013.csv. */
#include <string.h>
#include "engine.hpp"
#include "input.hpp"
#include "message.hpp"
#include "crt/crt.hpp"
#include "frameloop.hpp"
#include <ctype.h>
#include "input_shared.hpp"

// FUNCTION: LEGORR 0x0047ed90
int input_direct_initialize(void)
{
    input_reset(&s_direct_input_state);
    s_direct_input_initialized = 0;
    s_direct_input_keyboard_acquired = 0;
    /* Retail only reports success after DirectInputCreate, keyboard-device
     * creation, data-format/cooperative configuration, and Acquire succeed.
     * Missing adapters cannot stand in for those operations. */
    if (s_direct_input_hooks.interface_create == NULL ||
        s_direct_input_hooks.keyboard_create == NULL ||
        s_direct_input_hooks.devices_configure == NULL ||
        s_direct_input_hooks.devices_acquire == NULL ||
        s_direct_input_hooks.devices_release == NULL) {
        input_reset(&s_direct_input_state);
        return 0;
    }
    if (!s_direct_input_hooks.interface_create(s_direct_input_hooks.context)) {
        input_reset(&s_direct_input_state);
        return 0;
    }
    if (!s_direct_input_hooks.keyboard_create(s_direct_input_hooks.context))
        goto failed;
    if (s_direct_input_hooks.mouse_create != NULL &&
        !s_direct_input_hooks.mouse_create(s_direct_input_hooks.context))
        goto failed;
    if (!s_direct_input_hooks.devices_configure(s_direct_input_hooks.context))
        goto failed;
    if (!s_direct_input_hooks.devices_acquire(s_direct_input_hooks.context))
        goto failed;
    s_direct_input_initialized = 1;
    s_direct_input_keyboard_acquired = 1;
    return 1;
failed:
    if (s_direct_input_hooks.devices_release != NULL)
        s_direct_input_hooks.devices_release(s_direct_input_hooks.context);
    input_reset(&s_direct_input_state);
    s_direct_input_keyboard_acquired = 0;
    return 0;
}

/* InputStatePoll (FUN_0047eef0): update the live keyboard snapshot using the
 * engine's fullscreen and application-active gates. DirectInput ownership is
 * represented by the existing callbacks because this backend has no native
 * DirectInput device object. */
// FUNCTION: LEGORR 0x0047eef0
void input_state_poll_retail(void)
{
    int poll_active = 1;

    if ((s_engine_feature_word & 2u) == 0u)
        poll_active = s_window_application_active != 0u;

    if (!poll_active) {
        InputState* s = s_active_input_state != NULL
            ? s_active_input_state : &s_direct_input_state;
        memset(s->cur, 0, sizeof(s->cur));
        memset(s->prev, 0, sizeof(s->prev));
        if (s_direct_input_initialized &&
            s_direct_input_hooks.keyboard_unacquire != NULL) {
            s_direct_input_hooks.keyboard_unacquire(
                s_direct_input_hooks.context);
        }
        s_direct_input_keyboard_acquired = 0;
        input_global_current_keys_copy(s);
        return;
    }
    if (!s_direct_input_initialized) return;

    InputState* s = s_active_input_state != NULL
        ? s_active_input_state : &s_direct_input_state;
    int result;
    if (s_direct_input_keyboard_acquired) {
        result = -0x7ff8ffe2;
        if (s_direct_input_hooks.keyboard_state_read != NULL) {
            memcpy(s->prev, s->cur, sizeof(s->cur));
            memset(s->cur, 0, sizeof(s->cur));
            result = s_direct_input_hooks.keyboard_state_read(
                s_direct_input_hooks.context, INPUT_KEY_COUNT, s->cur);
        }
    }
    else result = -0x7ff8ffe2;
    if (result != 0) {
        s_direct_input_keyboard_acquired = 0;
        if (s_direct_input_hooks.keyboard_acquire != NULL &&
            s_direct_input_hooks.keyboard_acquire(
                s_direct_input_hooks.context) == 0) {
            s_direct_input_keyboard_acquired = 1;
        }
    }
    input_global_current_keys_copy(s);
}

/* KeyboardDownKeyCount (FUN_0047efb0): count nonzero entries in the
 * module's current 256-byte key snapshot. DirectInput writes a full sample;
 * the event backend maintains held state until it receives key-up. */
// FUNCTION: LEGORR 0x0047EFB0
int input_keyboard_down_key_count(void)
{
    int count = 0;
    int i;
    for (i = 0; i < INPUT_KEY_COUNT; ++i) {
        if (s_global_current_keys[i] != 0u) {
            ++count;
        }
    }
    return count;
}
// FUNCTION: LEGORR 0x0047efd0
void input_direct_shutdown(void)
{
    if (s_direct_input_initialized &&
        s_direct_input_hooks.devices_release != NULL)
        s_direct_input_hooks.devices_release(s_direct_input_hooks.context);
    s_direct_input_initialized = 0;
    s_direct_input_keyboard_acquired = 0;
}

/* CursorPositionUpdate (FUN_0047f010): sample the active cursor, clamp it to
 * the configured dimensions, then publish the pair and per-sample delta.
 * WinAPI's platform getter converts the stretched client to framebuffer
 * coordinates; reading GetCursorPos here would overwrite normalized events. */
// FUNCTION: LEGORR 0x0047F010
void input_cursor_position_update(void)
{
    int x;
    int y;
#if defined(LEGORRR_BACKEND_WINAPI)
    if (!platform_cursor_position_get(&x, &y)) return;
    if (x >= s_display_w) x = s_display_w - 1;
    if (y >= s_display_h) y = s_display_h - 1;
#else
    int width;
    int height;
    if (!platform_cursor_position_get(&x, &y)) return;
    width = g_platform_cursor_width;
    height = g_platform_cursor_height;
    if (x >= width) x = width - 1;
    if (y >= height) y = height - 1;
#endif
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    s_cursor_delta_x = x - (int32_t)s_renderer_word_a;
    s_cursor_delta_y = y - (int32_t)s_renderer_word_b;
    s_renderer_word_a = (uint32_t)x;
    s_renderer_word_b = (uint32_t)y;
}

/* CursorSetClientPosition (FUN_0047f0d0): update the client-relative mouse
 * globals, clear the accumulated movement pair, convert against the active
 * window when the platform hook exists, and move the host cursor. */
// FUNCTION: LEGORR 0x0047F0D0
void input_cursor_client_position_set(int x, int y)
{
#if defined(LEGORRR_BACKEND_WINAPI)
    /* These limits belong to the retained framebuffer, not WM_SIZE. */
    if (g_platform_cursor_width <= 0 || g_platform_cursor_height <= 0) return;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= g_platform_cursor_width) x = g_platform_cursor_width - 1;
    if (y >= g_platform_cursor_height) y = g_platform_cursor_height - 1;
#endif
    int screen_x = x;
    int screen_y = y;
    s_renderer_word_a = (uint32_t)x;
    s_renderer_word_b = (uint32_t)y;
    s_cursor_delta_x = 0;
    s_cursor_delta_y = 0;
    if (s_input_client_to_screen_hook != NULL) {
        if (s_input_client_to_screen_hook(&screen_x, &screen_y,
                                          s_input_cursor_hook_context) == 0) {
            screen_x = x;
            screen_y = y;
        }
    }
    if (s_input_set_cursor_position_hook != NULL) {
        s_input_set_cursor_position_hook(screen_x, screen_y,
                                         s_input_cursor_hook_context);
    }
    else {
        platform_cursor_position_set(x, y);
#if defined(LEGORRR_BACKEND_WINAPI)
        /* Publish the actual logical sample after the inverse warp, including
         * quantization in a smaller client or an OS cursor restriction. */
        if (platform_cursor_position_get(&screen_x, &screen_y)) {
            s_renderer_word_a = (uint32_t)screen_x;
            s_renderer_word_b = (uint32_t)screen_y;
        }
#endif
    }
}
