// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005 step 6) for DirectDraw.c (G98-010,
 * finding 596): the Main.c (G98-005) functions and state the unit calls.
 * Only directdraw.c and src/game/test_c_boundaries.cpp include it; the C++
 * declarations it pairs with have C linkage in place. */
#ifndef LEGORRR_GODS98_DIRECTDRAW_IMPORTS_H
#define LEGORRR_GODS98_DIRECTDRAW_IMPORTS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Main_GetWindowsBitDepth (FUN_00477f80): the desktop's bits per pixel
 * (frameloop.hpp). */
int window_runtime_display_caps_query(void);                         /* 00477f80 */

/* Main_AdjustWindowRect (FUN_004780b0, surface.hpp): the same contract as
 * the original's (full screen, x, y, width, height). */
void renderer_window_mode_apply(int fullscreen, int x, int y,
                                int width, int height);              /* 004780b0 */

/* Main_SetupDirect3D (FUN_004781d0, surface.hpp). The original takes the
 * selected device, the IDirectDraw object, the back buffer and the full
 * screen flag. The port's model takes an adapter record, its flags, a
 * surface and a single-mode flag; directdraw_setup passes the original's
 * arguments in those slots (ADR 0005 step 6: the callee's own conversion
 * recovers the contract). The port renderer never runs directdraw_setup. */
int direct3drm_device_initialize(void* adapter, unsigned int adapter_flags,
                                 void* surface, int force_single_mode); /* 004781d0 */

/* DAT_00505918: Main.c's IDirect3DRMDevice3 (lpDevice()). The stand-in that
 * Viewports.c also reads (viewports_imports.h); viewports_port.cpp defines
 * it, and the Windows renderer (directdraw_port.cpp) publishes its device
 * there. */
extern void* s_main_device;

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_DIRECTDRAW_IMPORTS_H */
