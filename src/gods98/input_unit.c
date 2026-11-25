// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit G98-013 (text 0047ed90-0047f130; the __FILE__
 * string 004aaed0; its state in the commons, 0076aa00-0076ac80; finding
 * 683): gods98's Input.c. It opens DirectInput and its keyboard, polls the
 * keyboard into the two key maps, counts the keys held, closes DirectInput,
 * samples the cursor into the client-relative mouse words and moves it.
 * The binary attests none of the names; the functions keep the names the
 * port gave them.
 *
 * The keyboard is a library substitution (ADR 0007, `// [library:input]`),
 * as in isle-portable's input manager: SDL3's keyboard state fills the
 * DirectInput key map through an SDL scancode to DIK_* table
 * (input_sdl_keys.h). SDL's keyboard state comes through the port's input
 * layer (platform/native/input.h), which adds the keys pressed and released
 * since the last poll and those the touch gestures hold. The mouse still
 * comes from miniwin's window messages and cursor calls.
 *
 * The state is the original's, in the commons: the previous and current
 * key maps, the DirectInput object, the keyboard device, its acquired flag,
 * and the cursor position and movement. The DirectInput object and the
 * keyboard device stay null under the substitution. The other units read
 * the key maps and the mouse words by their port names, so the state is a
 * set of separate commons rather than the original's one Input_Globs
 * (finding 683, section 2). */
#include "input_unit.h"


#include <string.h>
#include <miniwin/windows.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_log.h>

#include "input_sdl_keys.h"
#include "platform/native/input.h" // [library:input] LRR_InputKeyboardState

#ifdef __cplusplus
extern "C" {
#endif

/* DirectInput's value for a key held down in the key map. */
#define INPUT_KEY_DOWN 0x80

/* Main.c's state: mainGlobs' hWnd, hInst, active, appWidth, appHeight and
 * flags. In the matching build the port names are its fields
 * (main_globs.h, finding 679); the MinGW reference build keeps the port's
 * variables. */
#include "main_globs.h"

/* The unit's state, in the commons (tentative definitions). */
#include "input_globs.h"
#include "input_key_names.h"

/* Tentative definitions: commons, as in the original. */
// GLOBAL: LEGORR 0x0076aa00
unsigned char inputPrevKeyMap[256];
// GLOBAL: LEGORR 0x0076ab00
struct Input_Globs inputGlobs;
// GLOBAL: LEGORR 0x0076ab80
unsigned char inputKeyMap[256];
/* [divergence:modern-controls] Port-only (input_globs.h). */
int inputMiddleButton;
int inputWheelDelta;

// FUNCTION: LEGORR 0x0047ed90
int input_direct_initialize(void)
{
    /* DirectInputCreateA, CreateDevice(GUID_SysKeyboard), SetDataFormat
     * and SetCooperativeLevel: SDL's keyboard needs only its events
     * subsystem, and there is no device object. */
    s_input_lpdi = NULL;
    s_input_lpdi_keyboard = NULL;
    if (!SDL_InitSubSystem(SDL_INIT_EVENTS)) { // [library:input]
        SDL_Log("Input.c: SDL_InitSubSystem: %s", SDL_GetError());
        return 0;
    }
    /* Acquire: SDL's keyboard state is always readable. */
    s_input_keyboard_acquired = 1; // [library:input]
    return 1;
}

// FUNCTION: LEGORR 0x0047eef0
void input_state_poll_retail(void)
{
    int active;
    const bool* state;
    int count;
    int i;
    unsigned char dik;

    if (s_engine_feature_word & 2) active = 1;
    else active = s_window_application_active;

    if (!active) {
        memset(s_global_current_keys, 0, sizeof(s_global_current_keys));
        memset(s_global_previous_keys, 0, sizeof(s_global_previous_keys));
        s_input_keyboard_acquired = 0; // [library:input] Unacquire
        return;
    }

    /* Acquire, then GetDeviceState: the key map from SDL's keyboard state.
     * SDL_PumpEvents keeps the state current for loops that poll the
     * keyboard without pumping window messages. */
    s_input_keyboard_acquired = 1; // [library:input] Acquire
    memcpy(s_global_previous_keys, s_global_current_keys,
           sizeof(s_global_previous_keys));
    memset(s_global_current_keys, 0, sizeof(s_global_current_keys));
    SDL_PumpEvents(); // [library:input]
    state = LRR_InputKeyboardState(&count); // [library:input]
    for (i = 0; i < count; i++) {
        if (state[i] && (dik = input_sdl_scancode_dik((SDL_Scancode)i)) != 0)
            s_global_current_keys[dik] = INPUT_KEY_DOWN;
    }
}

// FUNCTION: LEGORR 0x0047efb0
int input_keyboard_down_key_count(void)
{
    int count = 0;
    unsigned char* key = s_global_current_keys;
    int i;

    for (i = 256; i != 0; i--) {
        if (*key) count++;
        key++;
    }
    return count;
}

// FUNCTION: LEGORR 0x0047efd0
void input_direct_shutdown(void)
{
    /* Unacquire and Release the keyboard and DirectInput. */
    s_input_keyboard_acquired = 0;
    SDL_QuitSubSystem(SDL_INIT_EVENTS); // [library:input]
}

// FUNCTION: LEGORR 0x0047f010
void input_cursor_position_update(void)
{
    RECT rect;
    POINT offset;
    POINT pos;
    int x;
    int y;

    if (GetClientRect((HWND)g_platform_cursor_window_handle, &rect)) {
        offset.y = 0;
        offset.x = 0;
        ClientToScreen((HWND)g_platform_cursor_window_handle, &offset);
        GetCursorPos(&pos);
        x = pos.x - offset.x;
        y = pos.y - offset.y;
        if (x >= s_display_w) x = s_display_w - 1;
        if (y >= s_display_h) y = s_display_h - 1;
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        s_cursor_delta_x = x - s_renderer_word_a;
        s_cursor_delta_y = y - s_renderer_word_b;
        s_renderer_word_a = x;
        s_renderer_word_b = y;
    }
}

// FUNCTION: LEGORR 0x0047f0d0
void input_cursor_client_position_set(int x, int y)
{
    POINT offset;

    offset.y = 0;
    offset.x = 0;
    ClientToScreen((HWND)g_platform_cursor_window_handle, &offset);
    s_renderer_word_a = x;
    s_renderer_word_b = y;
    s_cursor_delta_x = 0;
    s_cursor_delta_y = 0;
    SetCursorPos(offset.x + x, offset.y + y);
}

#ifdef __cplusplus
}
#endif

