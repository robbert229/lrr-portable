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

#endif /* LEGORRR_SURFACE_HPP */