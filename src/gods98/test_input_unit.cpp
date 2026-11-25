// SPDX-License-Identifier: LGPL-3.0-or-later
/* input_unit.c — key snapshots across press, release, and focus loss.
 * SDL supplies the synthetic snapshot and runs without a window. */
#include <SDL3/SDL_keyboard.h>

#include "gods98/input_key_names.h"
#include "gods98/input_keys.h"
#include "gods98/input_unit.h"
#include "gods98/main_globs.h"
#include "test/lrr_test.h"

extern "C" {
#include "gods98/input_globs.h"
}

static bool keyboard[SDL_SCANCODE_COUNT];

extern "C" const bool* __wrap_SDL_GetKeyboardState(int* count)
{
    *count = SDL_SCANCODE_COUNT;
    return keyboard;
}

int main()
{
    if (!CHECK(input_direct_initialize(), "initialize SDL events")) return lrr_test_result();
    CHECK_EQ(s_input_keyboard_acquired, 1);
    s_window_application_active = 1;
    keyboard[SDL_SCANCODE_A] = true;
    keyboard[SDL_SCANCODE_SPACE] = true;
    /* A key DirectInput has no code for is never reported. */
    keyboard[SDL_SCANCODE_AC_HOME] = true;

    constexpr unsigned int dikA = 0x1e;
    constexpr unsigned int dikSpace = 0x39;
    input_state_poll_retail();
    CHECK_EQ(input_keyboard_down_key_count(), 2);
    CHECK_EQ(s_global_current_keys[dikA], 0x80u);
    CHECK_EQ(s_global_current_keys[dikSpace], 0x80u);
    CHECK_EQ(s_global_previous_keys[dikA], 0u);

    keyboard[SDL_SCANCODE_A] = false;
    input_state_poll_retail();
    CHECK_EQ(input_keyboard_down_key_count(), 1);
    CHECK_EQ(s_global_current_keys[dikA], 0u);
    CHECK_EQ(s_global_previous_keys[dikA], 0x80u);
    CHECK_EQ(s_global_previous_keys[dikSpace], 0x80u);

    s_window_application_active = 0;
    input_state_poll_retail();
    CHECK_EQ(input_keyboard_down_key_count(), 0);
    CHECK_EQ(s_global_previous_keys[dikSpace], 0u);
    CHECK_EQ(s_input_keyboard_acquired, 0);

    s_window_application_active = 1;
    input_state_poll_retail();
    CHECK_EQ(s_input_keyboard_acquired, 1);
    CHECK_EQ(s_global_current_keys[dikSpace], 0x80u);
    CHECK_EQ(s_global_previous_keys[dikSpace], 0u);
    /* Android's Back button is Escape (tweaks:back_button = escape, the
     * default; platform/native/input.h). */
    constexpr unsigned int dikEscape = 0x01;
    keyboard[SDL_SCANCODE_AC_BACK] = true;
    input_state_poll_retail();
    CHECK_EQ(s_global_current_keys[dikEscape], 0x80u);
    keyboard[SDL_SCANCODE_AC_BACK] = false;
    input_state_poll_retail();
    CHECK_EQ(s_global_current_keys[dikEscape], 0u);
    input_direct_shutdown();
    CHECK_EQ(s_input_keyboard_acquired, 0);
    return lrr_test_result();
}
