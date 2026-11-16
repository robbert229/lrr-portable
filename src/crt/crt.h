// SPDX-License-Identifier: LGPL-3.0-or-later
/* Names other units give two gods98 globals: DirectDraw.c's back buffer and
 * Main.c's mainGlobs fields. */
#ifndef LEGORRR_CRT_CRT_H
#define LEGORRR_CRT_CRT_H

#include <stdint.h>

#include "gods98/directdraw.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DAT_0076ac8c, the DirectDraw back buffer (DirectDraw_bSurf()):
 * directDrawGlobs.bSurf (gods98/directdraw.h). */
#define s_render_surface_secondary (directDrawGlobs.bSurf)

/* Main.c's mainGlobs (finding 679), in place of the port's three names
 * below: the units that use them include gods98/main_globs.h again, after
 * their last declarations, for the names. */
#define LEGORRR_MAIN_GLOBS_NO_NAMES
#include "gods98/main_globs.h"
#undef LEGORRR_MAIN_GLOBS_NO_NAMES

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_CRT_CRT_H */
