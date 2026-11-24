// SPDX-License-Identifier: LGPL-3.0-or-later
/* The host's application lifecycle, as the game's main loop sees it. A
 * handheld or phone suspends the application (the 3DS's HOME menu or
 * sleep mode; Android and iOS going to the background) and later restores
 * it, and can ask it to exit. The platform code reports those events here;
 * the game reacts through the paths it already has:
 *
 * - Suspend and resume are a Windows application's loss and return of
 *   focus. The game's main loop (gods98/main.c) reads LRR_ApplicationActive
 *   where it would otherwise keep a full-screen application active for
 *   good, and Lego's main loop pauses the level on an inactive frame when
 *   the configuration's Main:LoseFocusAndPause is on (lego_unit.c), which
 *   also stops the level's sounds. The first frame after a resume is
 *   inactive, so the level is paused when the player returns to it.
 * - Exit is a close request: SDL_EVENT_QUIT, which miniwin turns into the
 *   WM_CLOSE that WinMain's message loop handles for the window's close
 *   button, so the game shuts down, and writes its saves and settings, as
 *   it does on the desktop.
 *
 * The 3DS reports them from its APT hooks (3ds/apthooks.cpp). Android's and
 * iOS's SDL_EVENT_WILL_ENTER_BACKGROUND and SDL_EVENT_DID_ENTER_FOREGROUND
 * could report them the same way. */
#ifndef LEGORRR_PLATFORM_NATIVE_LIFECYCLE_H
#define LEGORRR_PLATFORM_NATIVE_LIFECYCLE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum LRR_LifecycleEvent {
    /* The application is about to be suspended or put to sleep. */
    LRR_LIFECYCLE_SUSPEND,
    /* The application is running again. */
    LRR_LIFECYCLE_RESUME,
    /* The system asks the application to exit. */
    LRR_LIFECYCLE_EXIT,
} LRR_LifecycleEvent;

/* Takes one event. Safe to call from a platform callback: it only sets
 * state and posts an SDL event. */
void LRR_ReportLifecycle(LRR_LifecycleEvent event);

/* Whether the application counts as active, for the main loop's
 * s_window_application_active. Without any event it is always true, so the
 * desktop is unchanged. After a suspend it is false; after the resume it is
 * false for one more call, so that a frame sees the application come back
 * inactive, and then true. The main loop calls it once a frame. */
int LRR_ApplicationActive(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_PLATFORM_NATIVE_LIFECYCLE_H */
