// SPDX-License-Identifier: LGPL-3.0-or-later
/* The port's input layer below Input.c's keyboard substitution
 * (`// [library:input]`, ADR 0007; issue #986): SDL's keyboard state with
 * the input latch (input_latch.h), the touch gestures (touch_gestures.h),
 * Android's Back button as Escape, and the on-screen keyboard. Shared by
 * every host; the gestures are on where tweaks:touch is (config.h).
 *
 * It watches SDL's events as SDL queues them (SDL_AddEventWatch; on Android
 * the finger and key events come from the UI thread) and acts on them when
 * the game polls the keyboard, once a frame, on the game's thread: the
 * pointer moves (SDL_WarpMouseInWindow), and the mouse-button changes the
 * latch lets through this poll are queued as SDL mouse events, which
 * miniwin's message pump turns into WM_*BUTTON* messages in the next frame.
 * A mouse's own events pass through untouched. */
#ifndef LEGORRR_PLATFORM_NATIVE_INPUT_H
#define LEGORRR_PLATFORM_NATIVE_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

/* In place of SDL_GetKeyboardState, once per poll (input_state_poll_retail):
 * the keys SDL holds down, with the latch's (so a key pressed and released
 * since the last poll is down in this one), the keys the gestures hold, and
 * Escape while Back is down. Also runs the gestures for this poll, and
 * starts the input layer on its first call. The array is indexed by SDL
 * scancode and holds *numkeys entries. */
const bool* LRR_InputKeyboardState(int* numkeys);

/* The game takes typed text in this frame (a unit's name, a menu's text
 * field): the on-screen keyboard opens, where the host has one and
 * tweaks:touch_keyboard is "auto", and closes after the first poll without
 * such a call. */
void LRR_InputTextWanted(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_PLATFORM_NATIVE_INPUT_H */
