// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 Images.c (original unit G98-011) and Font.c (G98-008) entry points
 * in their original ABI, for units rebuilt as C. The port's entries at these
 * addresses have other contracts (a TextureSurface rather than the Image
 * record; a software render target), so these are thin unannotated adapters
 * (ADR 0005 step 6) in gods98/texture.cpp and gods98/bitmap_font_port.cpp.
 * Font.c is converted (font.c, finding 605): in the matching build it
 * defines the two Font.c names, and the port tail defines them natively.
 * Converting Images.c retires the Images.c adapters.
 *
 * C boundary header (ADR 0005). Shared C++ headers do not include this file
 * (finding 506). It is separate from images.h and font.h because other
 * C units include those (finding 538, rule B). */
#ifndef LEGORRR_GODS98_IMAGE_ADAPTERS_H
#define LEGORRR_GODS98_IMAGE_ADAPTERS_H

#include <stdarg.h>
#include <stdint.h>

#include "gods98/images.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TextureSurface;

#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
/* Images.c is images_unit.c in the matching build (finding 632): the
 * C callers' calls go to the unit (images_link_names.h). */
#define image_setup_trans Image_SetupTrans /* 0047dbf0 */
#define image_get_pixel Image_GetPixel /* 0047dfa0 */
#define image_set_pen_zero_trans Image_SetPenZeroTrans /* 0047dbc0 */
#endif
/* Image_SetupTrans (FUN_0047dbf0): the colour-key range. */
void image_setup_trans(Image* image, float lowRed, float lowGreen,
                       float lowBlue, float highRed, float highGreen,
                       float highBlue);
/* Image_GetPixel (FUN_0047dfa0): nonzero when (x, y) is inside the image;
 * the pixel goes to *pixel. */
int image_get_pixel(Image* image, int x, int y, unsigned int* pixel);
/* Image_SetPenZeroTrans (FUN_0047dbc0): pen 0 is transparent. */
void image_set_pen_zero_trans(Image* image);

/* Font_VGetStringInfo (FUN_0047a150) and Font_VPrintF (FUN_0047a1b0). */
void font_v_get_string_info(const struct TextureSurface* font,
                            uint32_t* width, uint32_t* lineCount,
                            const char* msg, va_list args);
void font_v_print_f(const struct TextureSurface* font, int x, int y,
                    const char* msg, va_list args);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_IMAGE_ADAPTERS_H */
