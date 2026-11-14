// SPDX-License-Identifier: LGPL-3.0-or-later
/* C boundary header (ADR 0005) for Main.c (G98-005) state that units
 * rebuilt as C read, beyond the flags word in crt/crt.h. Shared C++ headers
 * do not include this file; src/game/test_c_boundaries.cpp checks it against
 * crt/crt.hpp. */
#ifndef LEGORRR_GODS98_MAIN_H
#define LEGORRR_GODS98_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* DAT_00505914: Main.c's IDirect3DRM3 (lpD3DRM()), defined in
 * lws_port.cpp. On Windows, main.c's Main_SetupDirect3D sets it during
 * start-up (finding 641); natively it stays NULL, and only lws.c's
 * original code, which the native port never calls, reads it. */
extern void* s_main_d3drm;

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_MAIN_H */
