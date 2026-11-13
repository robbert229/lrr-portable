// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 Images.c entry points (original unit G98-011, finding 505) in their
 * original ABI, for converted C units. The reconstruction's software
 * surfaces (texture.hpp) hold the pixels; texture.cpp wraps them in the
 * original Image record.
 *
 * C-compatible boundary header (ADR 0005): the declarations a C translation
 * unit may call, with C linkage. */
#ifndef LEGORRR_GODS98_IMAGES_H
#define LEGORRR_GODS98_IMAGES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Point2F {
    float x;
    float y;
} Point2F;

typedef struct Area2F {
    float x;
    float y;
    float width;
    float height;
} Area2F;

/* The original Image record (0x20 bytes; Images.c, finding 632). Callers
 * read the size directly at +4/+8. The port's adapters (texture.cpp) fill
 * the first three fields. */
typedef struct Image {
    void* surface;          /* +0x00: TextureSurface* (a DirectDraw surface originally) */
    unsigned int width;     /* +0x04 */
    unsigned int height;    /* +0x08 */
    unsigned int penZero;   /* +0x0c pen 0 in the surface's pixel format */
    unsigned int pen255;    /* +0x10 pen 255 in the surface's pixel format */
    unsigned int penZeroRGB; /* +0x14 pen 0 as 0x00BBGGRR */
    unsigned int flags;     /* +0x18 IMAGE_FLAG_TRANS */
    struct Image* nextFree; /* +0x1c the free list; itself when in use */
} Image;

#if defined(_MSC_VER) && _MSC_VER < 1200 && !defined(__cplusplus)
/* Images.c is images_unit.c in the matching build (finding 632): the
 * C callers' calls go to the unit (images_link_names.h). */
#define image_load_bmp_scaled Image_LoadBMPScaled /* 0047d9d0 */
#define image_display_scaled Image_DisplayScaled /* 0047dcb0 */
#endif
/* Image_LoadBMPScaled (FUN_0047d9d0): load a bitmap resource. Port adapter:
 * a non-zero width or height asks the original to scale; the port ignores
 * it (every converted caller passes 0, 0). NULL when the load fails. */
Image* image_load_bmp_scaled(const char* name, unsigned int width,
                             unsigned int height);

/* Image_DisplayScaled (FUN_0047dcb0): draw `src` (NULL for the whole image)
 * at `dest_pos`, scaled to `dest_size` (NULL for no scaling). Port adapter:
 * drawing to the back buffer is renderer-owned and has no portable owner
 * yet, so this does nothing. */
void image_display_scaled(Image* image, const Area2F* src,
                          const Point2F* dest_pos, const Point2F* dest_size);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_IMAGES_H */
