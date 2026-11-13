// SPDX-License-Identifier: LGPL-3.0-or-later
/* Flic.c (G98-016) imports: C declarations of the callees and globals the
 * unit uses from other modules (ADR 0005 step 6; finding 632). Included only
 * by flic_unit.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 *
 * - Files.c (files.c, files.h) and DirectDraw.c (directdraw.c,
 *   directdraw.h) are converted units: the unit reads the DirectDraw object
 *   (0076ac84) and the back buffer (0076ac8c) from directDrawGlobs.
 * - 00456210, the chunk type 'UU' handler, is one of the original's shared
 *   do-nothing functions; tutorial_script.c defines it as
 *   flh_magic_uu_noop. */
#ifndef LEGORRR_GODS98_FLIC_IMPORTS_H
#define LEGORRR_GODS98_FLIC_IMPORTS_H

#include "files.h"
#include "directdraw.h"

#ifdef __cplusplus
extern "C" {
#endif

unsigned int flh_magic_uu_noop(void);                          /* 00456210 */
#define Flic_Sound(fsp) (((unsigned int (*)(LPFLICSTRUCT))flh_magic_uu_noop)(fsp))

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_FLIC_IMPORTS_H */
