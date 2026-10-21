// SPDX-License-Identifier: LGPL-3.0-or-later
/* winapi platform glue — reference-build stubs.
 *
 * The winapi adapter (ADR 0004) is the A/B reference build: it must compile
 * the same source against real DirectX headers. Real window creation
 * (RegisterClassEx/CreateWindowEx) and real DDraw Flip are deferred until
 * the reference build is wired into the A.2 harness — these stubs keep the
 * seam compiling on that backend.
 */
#include "platform/platform.h"

PlatformWindow* platform_create_window(const char* title, int width, int height)
{
    (void)title; (void)width; (void)height;
    return NULL;
}

void platform_destroy_window(PlatformWindow* win)
{
    (void)win;
}

void* platform_window_hwnd(PlatformWindow* win)
{
    (void)win;
    return NULL;
}

int platform_window_width(PlatformWindow* win)
{
    (void)win;
    return 0;
}

int platform_window_height(PlatformWindow* win)
{
    (void)win;
    return 0;
}

int platform_poll_events(PlatformWindow* win)
{
    (void)win;
    return 1;
}

void platform_present_pixels(PlatformWindow* win, const void* pixels, int pitch,
                             int width, int height)
{
    (void)win; (void)pixels; (void)pitch; (void)width; (void)height;
}

void platform_screenshot(PlatformWindow* win, const void* pixels, int pitch,
                         int width, int height, int frame)
{
    (void)win; (void)pixels; (void)pitch; (void)width; (void)height;
    (void)frame;
}