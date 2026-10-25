// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — display surface scaffold (finding 13 create path).
 *
 * A thin wrapper over a DirectDraw offscreen surface (the corpus's
 * DDSURFACEDESC / create pipeline, FUN_00474050) reached through the
 * platform seam (platform.h re-exports the backend's DirectDraw). On miniwin
 * the surface is an SDL framebuffer in SDL_PIXELFORMAT_RGBA32 (little-endian
 * byte order R,G,B,A); Lock hands back pixels+pitch, Unlock releases, and the
 * platform glue presents it (miniwin's surface Flip is a stub by design).
 *
 * The draw helpers convert the texture loader's ARGB (0xAARRGGBB) surfaces
 * into that byte order. TODO(winapi): the reference build needs vendored
 * DirectDraw headers; until then the DDraw half compiles to no-ops there.
 */
#ifndef LEGORRR_SURFACE_HPP
#define LEGORRR_SURFACE_HPP

#include <cstdint>
#include <cstddef>

#include "texture.hpp"

typedef struct DisplaySurface {
    void*  dd;    /* LPDIRECTDRAW  (opaque out of the seam) */
    void*  surf;  /* LPDIRECTDRAWSURFACE */
    void*  hwnd;
    int    w;
    int    h;
} DisplaySurface;

int  surface_create(DisplaySurface* out, void* hwnd, int w, int h);
void surface_destroy(DisplaySurface* s);
int  surface_width(DisplaySurface* s);
int  surface_height(DisplaySurface* s);

/* Lock returns the framebuffer pointer + row pitch (RGBA32 R,G,B,A on LE). */
int  surface_lock(DisplaySurface* s, void** pixels, int* pitch);
void surface_unlock(DisplaySurface* s, void* pixels);

/* Fill the locked framebuffer with a solid color. */
void surface_fill(void* px, int pitch, int w, int h, int r, int g, int b);

/* Blit a texture (top-left at x,y), converting ARGB -> RGBA32. */
void surface_blit_texture(void* px, int pitch, int w, int h,
                          const TextureSurface* t, int x, int y);

/* Moving bar (animation proof; magenta). */
void surface_draw_sweep(void* px, int pitch, int w, int h, int frame);

/* RGBChannelsPack (FUN_0047db90, findings 23/431): pack three channel
 * bytes into the corpus's 0x00BBGGRR word — first argument lands in the
 * low byte, matching the corpus's byte-array caller (palette entries
 * pack exactly as read: bytes 0..2 become bits 0..24). */
uint32_t rgb_channels_pack(uint8_t red, uint8_t green, uint8_t blue);

/* The pixel-conversion surface record (corpus 0x47d520/0x47d740/0x47d800):
 * the corpus's surface object, modeled LP64-safe as fields (the byte
 * offsets ride in comments). Rows +0x8, row bytes +0x10, pixel buffer
 * +0x24, depth +0x54, the three channel masks +0x58/+0x5c/+0x60. The
 * conversion jobs ride a parallel descriptor (the corpus's int array;
 * width [0], height [1], source stride [6], source pointer [7], and —
 * for the palette walk — the palette pointer [14]). */
typedef struct SurfaceRecord {
    int row_count;      /* +0x8  */
    int row_bytes;      /* +0x10 */
    void* pixels;       /* +0x24 */
    int depth;          /* +0x54 */
    uint32_t mask_low;  /* +0x58 */
    uint32_t mask_mid;  /* +0x5c */
    uint32_t mask_hi;   /* +0x60 */
} SurfaceRecord;

typedef struct SurfaceConvertJob {
    int width;          /* param_2[0]  */
    int height;         /* param_2[1]  */
    int src_stride;     /* param_2[6]  */
    const void* src;    /* param_2[7]  */
    const void* palette;/* param_2[14] (palette walk only) */
} SurfaceConvertJob;

/* SurfacePixelRowsFlipWithScratch (FUN_0047D740): pair-flip the rows
 * through an engine-heap scratch, rounding each row to the even half. */
void surface_pixel_rows_flip_with_scratch(SurfaceRecord* rec);

/* Palette8To16BitSurfaceExpand (FUN_0047D520): the indexed walk — each
 * palette entry's bytes land through their mask's field ((byte >>
 * (8 - popcount)) << first-set), then the rows flip. 16-bit targets only;
 * the other depths return the corpus's verdict without work. */
int surface_palette8_to_16bit_surface_expand(SurfaceRecord* rec,
                                             const SurfaceConvertJob* job);

/* Bitmap24BitSurfaceConvert (FUN_0047D800): the 24-bit sibling — the three
 * source bytes per pixel land through the masks REVERSED (byte 0 takes
 * mask_hi's field, byte 2 mask_low's), then the rows flip. */
int surface_bitmap24_bit_surface_convert(SurfaceRecord* rec,
                                         const SurfaceConvertJob* job);

#endif /* LEGORRR_SURFACE_HPP */