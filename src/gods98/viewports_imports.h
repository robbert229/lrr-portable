// SPDX-License-Identifier: LGPL-3.0-or-later
/* What Viewports.c (G98-003, finding 593) uses from units not yet rebuilt
 * as C. Included only by viewports.c; src/game/test_c_boundaries.cpp checks
 * it against the C++ declarations. */
#ifndef LEGORRR_GODS98_VIEWPORTS_IMPORTS_H
#define LEGORRR_GODS98_VIEWPORTS_IMPORTS_H

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DAT_00505918: Main.c's IDirect3DRMDevice3 (lpDevice()). Like
 * s_main_d3drm, its stand-in (viewports_port.cpp) stays NULL: the port's
 * renderer keeps its own device (surface.cpp). */
extern void* s_main_device;

/* FUN_00482d60 (Mesh_PostRenderAll). Natively the port tail
 * gods98/mesh_port.cpp's model, which surface.hpp declares with C linkage;
 * in the matching build the original-shape unit gods98/mesh_unit.c's
 * (finding 625), which takes the same argument. */
#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
#define model_render_pass_dispatch mesh_post_render_all
#endif
void model_render_pass_dispatch(void* device);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_VIEWPORTS_IMPORTS_H */
