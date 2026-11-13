// SPDX-License-Identifier: LGPL-3.0-or-later
/* Font.c (G98-008) imports: C declarations of the Images.c callees the unit
 * uses (ADR 0005 step 6; finding 605). Included only by font.c and
 * src/game/test_c_boundaries.cpp, so the other C units' symbol numbering
 * does not change (finding 568, "Unit-private imports header"). Each
 * declaration names a definition with C linkage. */
#ifndef LEGORRR_GODS98_FONT_IMPORTS_H
#define LEGORRR_GODS98_FONT_IMPORTS_H

#include "gods98/images.h"
#include "gods98/view_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
/* Images.c is images_unit.c in the matching build (finding 632): the
 * C callers' calls go to the unit (images_link_names.h). */
#define image_set_pen_zero_trans Image_SetPenZeroTrans /* 0047dbc0 */
#define image_get_pen255 Image_GetPen255 /* 0047def0 */
#define image_get_pixel_mask Image_GetPixelMask /* 0047df50 */
#define image_lock_surface Image_LockSurface /* 0047de60 */
#define image_unlock_surface Image_UnlockSurface /* 0047ded0 */
#endif
/* Image_SetPenZeroTrans (0047dbc0), Image_GetPen255 (0047def0),
 * Image_GetPixelMask (0047df50), Image_LockSurface (0047de60) and
 * Image_UnlockSurface (0047ded0) in the original ABI. The port's entries at
 * those addresses take the TextureSurface the Image wraps, or are not
 * written, so these are the PORT-ONLY adapters in gods98/texture.cpp. */
void image_set_pen_zero_trans(Image* image);
unsigned int image_get_pen255(Image* image);
unsigned int image_get_pixel_mask(Image* image);
void* image_lock_surface(Image* image, unsigned int* pitch,
                         unsigned int* bpp);
void image_unlock_surface(Image* image);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_FONT_IMPORTS_H */
