// SPDX-License-Identifier: LGPL-3.0-or-later
/* platform.h — the portability seam.
 *
 * Game-code below this line is platform-neutral. The only thing that varies
 * by backend is which header set satisfies the DX6-era API surface recorded
 * in docs/findings/15-compat-audit.md §1:
 *
 *   native : miniwin (https://github.com/isledecomp/isle-portable miniwin/)
 *            — source-compatible shim over SDL3, primary end-goal build
 *   winapi : the real DirectX headers from MinGW-w64 — the reference build,
 *            A/B-compared against the shipped binary under the A.2 harness
 *
 * Keep this file shallow: re-export the platform's windows/directdraw headers
 * under one roof, plus the platform-window + presentation glue that miniwin
 * deliberately leaves to the app (miniwin has no window-creation or blanket
 * present step: it is a framebuffer seam — window/events/present are app's
 * job, findings 15/16). Do not put game logic here (docs/adr/0003).
 */
#ifndef LEGORRR_PLATFORM_H
#define LEGORRR_PLATFORM_H

#include <cstddef>

#if defined(LEGORRR_BACKEND_MINIWIN)
#  include <miniwin/windows.h>
#  include <miniwin/ddraw.h>
#elif defined(LEGORRR_BACKEND_WINAPI)
#  include <windows.h>
/* TODO(winapi): vendored DirectDraw headers for the reference build
 * (MinGW-w64 ships no DirectDraw headers). The winapi adapter is A/B-only
 * (ADR 0004) and is not wired here yet. */
#else
#  error "no platform defined — set the meson option platform=native|winapi"
#endif

/* --- Platform window + presentation (backend-specific glue) --------------- */
/* miniwin exposes DDraw surfaces as framebuffers but never creates the
 * window nor presents; those are the platform glue's job. The winapi backend
 * would implement these with RegisterClassEx/CreateWindowEx + real Flip. */

typedef struct PlatformWindow PlatformWindow;

PlatformWindow* platform_create_window(const char* title, int width, int height);
void            platform_destroy_window(PlatformWindow* win);

/* The DX-era HWND to hand to SetCooperativeLevel (miniwin reinterprets it
 * as an SDL_Window*). */
void* platform_window_hwnd(PlatformWindow* win);

int  platform_window_width(PlatformWindow* win);
int  platform_window_height(PlatformWindow* win);

/* Pump platform events (SDL); returns 1 when the app should quit
 * (window close / ESC). */
int  platform_poll_events(PlatformWindow* win);

/* Commit a framebuffer (RGBA32 byte order R,G,B,A on little-endian) to the
 * window. `pixels`/`pitch` are what the DDraw surface Lock handed back. */
void platform_present_pixels(PlatformWindow* win,
                             const void* pixels, int pitch,
                             int width, int height);

/* Optional: dump the last presented framebuffer to a PPM for headless/CI
 * verification; no-op unless LEGORRR_SCREENSHOT is set. */
void platform_screenshot(PlatformWindow* win, const void* pixels, int pitch,
                         int width, int height, int frame);

#endif /* LEGORRR_PLATFORM_H */