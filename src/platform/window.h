// SPDX-License-Identifier: LGPL-3.0-or-later
/* The platform glue's window globals that units rebuilt as C read.
 *
 * C boundary header (ADR 0005) for platform/platform.h, which is C++ and
 * declares the same globals with extern "C". src/game/test_c_boundaries.cpp
 * includes both and fails to compile if they disagree. Shared C++ headers do
 * not include this file (finding 506). */
#ifndef LEGORRR_PLATFORM_WINDOW_H
#define LEGORRR_PLATFORM_WINDOW_H

#ifdef __cplusplus
extern "C" {
#endif

/* mainGlobs.hWnd (DAT_00505800): the game window. */
extern void* g_platform_cursor_window_handle;

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_PLATFORM_WINDOW_H */
