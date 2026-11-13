// SPDX-License-Identifier: LGPL-3.0-or-later
/* Images.c (original translation unit G98-011, finding 632): gods98's 2D
 * images. An Image is a DirectDraw surface with its size and pens, kept in
 * a pool of blocks of 1, 2, 4, ... records. Image_LoadBMPScaled reads a
 * bitmap into a system-memory surface, Image_DisplayScaled blits one to the
 * back buffer, and Image_SetupTrans and Image_SetPenZeroTrans set its
 * source colour key.
 *
 * C-compatible header (ADR 0005). images_unit.c is compiled as C in the
 * matching build. The native port has no DirectDraw, so the unit compiles
 * to nothing there and the port's surfaces (texture.hpp, surface.hpp) and
 * adapters (images.h, image_adapters.h) serve the callers. Shared C++
 * headers do not include this file (finding 506);
 * src/game/test_c_boundaries.cpp includes it.
 *
 * The names are the unit's own, after gods98's Image_* convention. In the
 * matching build two of them take the port's names (images_link_names.h). */
#ifndef LEGORRR_GODS98_IMAGES_UNIT_H
#define LEGORRR_GODS98_IMAGES_UNIT_H

#include "images.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Image_SurfaceIface;
struct Image_SurfaceDesc;
struct IndexedBitmapPaletteData;

#define IMAGE_MAXLISTS 32

/* Image flags (+0x18). */
#define IMAGE_FLAG_TRANS 0x00000002

void Image_Initialise(void);                                     /* 0047d410 */
void Image_Shutdown(void);                                       /* 0047d430 */
void Image_Remove(Image* dead);                                  /* 0047d470 */
int Image_CopyToDataToSurface(struct Image_SurfaceIface* surface,
                              struct IndexedBitmapPaletteData* image); /* 0047d490 */
int Image_8BitSourceCopy(struct Image_SurfaceDesc* desc,
                         struct IndexedBitmapPaletteData* image); /* 0047d520 */
int Image_CountMaskBits(unsigned int mask);                      /* 0047d700 */
int Image_CountMaskBitShift(unsigned int mask);                  /* 0047d720 */
void Image_FlipSurface(struct Image_SurfaceDesc* desc);          /* 0047d740 */
int Image_24BitSourceCopy(struct Image_SurfaceDesc* desc,
                          struct IndexedBitmapPaletteData* image); /* 0047d800 */
Image* Image_LoadBMPScaled(const char* fileName, unsigned int width,
                           unsigned int height);                /* 0047d9d0 */
unsigned int Image_RGB2CR(unsigned char r, unsigned char g,
                          unsigned char b);                     /* 0047db90 */
void Image_SetPenZeroTrans(Image* image);                        /* 0047dbc0 */
void Image_SetupTrans(Image* image, float lowr, float lowg, float lowb,
                      float highr, float highg, float highb);    /* 0047dbf0 */
void Image_DisplayScaled(Image* image, const Area2F* src,
                         const Point2F* destPos,
                         const Point2F* destSize);               /* 0047dcb0 */
void* Image_LockSurface(Image* image, unsigned int* pitch,
                        unsigned int* bpp);                      /* 0047de60 */
void Image_UnlockSurface(Image* image);                          /* 0047ded0 */
unsigned int Image_GetPen255(Image* image);                      /* 0047def0 */
unsigned int Image_GetPixelMask(Image* image);                   /* 0047df50 */
int Image_GetPixel(Image* image, unsigned int x, unsigned int y,
                   unsigned int* colour);                       /* 0047dfa0 */
Image* Image_Create(struct Image_SurfaceIface* surface, unsigned int width,
                    unsigned int height, unsigned int penZero,
                    unsigned int pen255);                        /* 0047e050 */
void Image_AddList(void);                                        /* 0047e0c0 */
void Image_RemoveAll(void);                                      /* 0047e130 */
unsigned int Image_DDColorMatch(struct Image_SurfaceIface* surface,
                                unsigned int rgb);              /* 0047e190 */
void Image_CR2RGB(unsigned int rgb, unsigned char* r, unsigned char* g,
                  unsigned char* b);                            /* 0047e2d0 */
void Image_GetScreenshot(Image* image, unsigned int xsize,
                         unsigned int ysize);                   /* 0047e300 */
void Image_InitFromSurface(Image* image, struct Image_SurfaceIface* surface,
                           unsigned int width, unsigned int height,
                           unsigned int penZero,
                           unsigned int pen255);                /* 0047e3e0 */
int Image_SaveBMP(Image* image, const char* fname);              /* 0047e440 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_IMAGES_UNIT_H */
