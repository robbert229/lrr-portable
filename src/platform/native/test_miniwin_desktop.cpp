// SPDX-License-Identifier: LGPL-3.0-or-later
// The desktop window's rectangle under miniwin: the primary display's
// bounds, as GetWindowRect(GetDesktopWindow()) gives on Windows. Main.c's
// fullscreen set-up moves its window onto that rectangle, and SDL then takes
// the window fullscreen on the display under it, so the rectangle has to lie
// on the primary display even where that display does not start at (0, 0).
// Synthetic on SDL's dummy driver, which has one display at (0, 0). Set
// MINIWIN_TEST_VIDEO_DRIVER to x11 or wayland to check a real desktop whose
// primary display is not the one at (0, 0).
#include "lrr_test.h"
#include "test_miniwin_device.h"

#include <miniwin/windows.h>

#include <SDL3/SDL.h>
#include <stdio.h>

int main()
{
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, MiniwinTestVideoDriver());
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_DisplayID primary = SDL_GetPrimaryDisplay();
    SDL_Rect bounds = {};
    CHECK(primary != 0, "SDL reports a primary display");
    CHECK(SDL_GetDisplayBounds(primary, &bounds), "the primary display has bounds");

    RECT desktop = {};
    CHECK(GetWindowRect(GetDesktopWindow(), &desktop), "the desktop window has a rectangle");
    CHECK_EQ(desktop.left, (LONG)bounds.x);
    CHECK_EQ(desktop.top, (LONG)bounds.y);
    CHECK_EQ(desktop.right, (LONG)(bounds.x + bounds.w));
    CHECK_EQ(desktop.bottom, (LONG)(bounds.y + bounds.h));

    // A window on the desktop rectangle, as Main.c places its fullscreen
    // window, goes fullscreen on the primary display.
    SDL_Rect placed = {desktop.left, desktop.top, desktop.right - desktop.left, desktop.bottom - desktop.top};
    CHECK_EQ(SDL_GetDisplayForRect(&placed), primary);

    SDL_Quit();
    return lrr_test_result("miniwin_desktop");
}
