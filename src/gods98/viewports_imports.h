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

/* FUN_00482d60 (Mesh_PostRenderAll), mesh_unit.cpp; surface.hpp declares
 * it with C linkage. */
void model_render_pass_dispatch(void* device);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_VIEWPORTS_IMPORTS_H */
