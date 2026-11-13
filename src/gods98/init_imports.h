// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for Init.c (LATE-002, finding 613):
 * the Main.c (G98-005) function and state the unit uses. Only init.c and
 * src/game/test_c_boundaries.cpp include it; the C++ declarations it pairs
 * with have C linkage in place. */
#ifndef LEGORRR_GODS98_INIT_IMPORTS_H
#define LEGORRR_GODS98_INIT_IMPORTS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Main_GetWindowsBitDepth (FUN_00477f80): the desktop's bits per pixel
 * (frameloop.hpp). */
int window_runtime_display_caps_query(void);                         /* 00477f80 */

/* DAT_00505800: Main.c's window (platform/window.h). */
extern void* g_platform_cursor_window_handle;

/* DAT_00505804: Main.c's module instance, the dialog template's module
 * (platform/winapi/frameloop.cpp). The port never sets it: NULL makes
 * DialogBoxParamA load the template from the executable, the same
 * module. */
extern void* g_main_instance_handle;

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_INIT_IMPORTS_H */
