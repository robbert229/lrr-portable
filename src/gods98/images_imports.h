// SPDX-License-Identifier: LGPL-3.0-or-later
/* Images.c (G98-011) imports: C declarations of the callees and globals the
 * unit uses from other modules (ADR 0005 step 6; finding 632). Included only
 * by images_unit.c and test_c_boundaries.cpp, so the other C units' symbol
 * numbering does not change (finding 568, "Unit-private imports header").
 *
 * Files.c (files.c), DirectDraw.c (directdraw.c), BMP.c (g98_026.c) and
 * DXError.c (dxerror.c) are converted units. The unit reads the DirectDraw
 * object (0076ac84) and the back buffer (0076ac8c) from directDrawGlobs. */
#ifndef LEGORRR_GODS98_IMAGES_IMPORTS_H
#define LEGORRR_GODS98_IMAGES_IMPORTS_H

#include "files.h"
#include "directdraw.h"
#include "dxerror.h"
#include "g98_026.h"

/* BMP.c: BMP_Parse (00489c30) and BMP_Cleanup (00489d70). */
#define BMP_Parse(data, size, image) \
    indexed_bitmap_palette_data_prepare((const uint8_t*)(data), (size), (image))
#define BMP_Cleanup object_field_38_release_target

#endif /* LEGORRR_GODS98_IMAGES_IMPORTS_H */
