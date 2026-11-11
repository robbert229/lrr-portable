// SPDX-License-Identifier: LGPL-3.0-or-later
/* draw.c (original translation unit G98-019, finding 519): 2D drawing on
 * the locked back buffer. Clipped lines, filled rectangles and dotted
 * circles go through a per-format pixel writer.
 *
 * C-compatible header (ADR 0005). draw.c is compiled as C in the matching
 * build and as C++ in the native port. Shared C++ headers do not include this
 * file: every include and declaration a header adds shifts MSVC's symbol
 * numbering (finding 506). The C++ declarations of the same functions are in
 * surface.hpp and view.hpp, and src/game/test_c_boundaries.cpp checks them
 * against this header.
 *
 * The functions keep the port's names. The comments give descriptive
 * Draw_* names; the binary attests neither. */
#ifndef LEGORRR_GODS98_DRAW_H
#define LEGORRR_GODS98_DRAW_H

#include <stdint.h>

#include "gods98/images.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Effects for the pixel writer (DrawEffect). XOR and half-transparency
 * exist only for 16-bit surfaces. */
#define DRAW_EFFECT_NONE 0
#define DRAW_EFFECT_XOR 1
#define DRAW_EFFECT_HALF_TRANS 2

/* One coloured rectangle for surface_fill_pixel_span_records (0x1c bytes).
 * The channels run 0..1, as for surface_color_pack_to_pixel. */
typedef struct DrawRect {
    Area2F rect;  /* +0x00 */
    float r;      /* +0x10 */
    float g;      /* +0x14 */
    float b;      /* +0x18 */
} DrawRect;

/* The DirectDraw surface interface as the unit uses it. The functions take
 * the surface as void*; it is an IDirectDrawSurface4 in the original. */
#if defined(_WIN32) && !defined(_WIN64)
#define DRAW_STDCALL __stdcall
#else
#define DRAW_STDCALL
#endif

/* DDSURFACEDESC2 (0x7c bytes on Win32), with the fields the unit reads. */
typedef struct DrawSurfaceDesc {
    uint32_t dwSize;            /* +0x00 */
    uint32_t dwFlags;           /* +0x04 */
    uint32_t dwHeight;          /* +0x08 */
    uint32_t dwWidth;           /* +0x0c */
    int32_t lPitch;             /* +0x10 */
    uint32_t reserved_14[4];    /* +0x14 */
    void* lpSurface;            /* +0x24 */
    uint32_t colorKeys[8];      /* +0x28 */
    uint32_t pfSize;            /* +0x48 DDPIXELFORMAT */
    uint32_t pfFlags;           /* +0x4c */
    uint32_t pfFourCC;          /* +0x50 */
    uint32_t pfRGBBitCount;     /* +0x54 */
    uint32_t pfRBitMask;        /* +0x58 */
    uint32_t pfGBitMask;        /* +0x5c */
    uint32_t pfBBitMask;        /* +0x60 */
    uint32_t pfAlphaBitMask;    /* +0x64 */
    uint32_t caps[4];           /* +0x68 DDSCAPS2 */
    uint32_t dwTextureStage;    /* +0x78 */
} DrawSurfaceDesc;

typedef struct DrawSurface DrawSurface;

/* IDirectDrawSurface4's method table, with the three slots the unit calls. */
typedef struct DrawSurfaceVtbl {
    void* slots_00[22];
    long (DRAW_STDCALL* GetSurfaceDesc)(DrawSurface* surf,
                                        DrawSurfaceDesc* desc); /* +0x58 */
    void* slots_5c[2];
    long (DRAW_STDCALL* Lock)(DrawSurface* surf, void* rect,
                              DrawSurfaceDesc* desc, uint32_t flags,
                              void* event);                     /* +0x64 */
    void* slots_68[6];
    long (DRAW_STDCALL* Unlock)(DrawSurface* surf, void* rect); /* +0x80 */
} DrawSurfaceVtbl;

struct DrawSurface {
    const DrawSurfaceVtbl* lpVtbl;
};

#if defined(LEGORRR_BACKEND_MINIWIN)
/* Native port only (draw_port.cpp). The original draws on the back buffer
 * in the common word at 0076ac8c. The port keeps that word as a uint32_t
 * (crt.hpp), which cannot hold a 64-bit pointer, and the native port has
 * no DirectDraw back buffer, so draw.c reads this one instead. By default
 * it is a surface whose methods fail, so nothing is drawn. */
extern DrawSurface* draw_port_back_surface;
#endif

/* Draw_Initialise (FUN_00485e80): set the initialised flag and the clip
 * window (NULL: the whole back buffer). */
void surface_render_setup(const Area2F* window);

/* Draw_SetClipWindow (FUN_00485ea0): clip to window, within the back
 * buffer's size; NULL clips to the back buffer. */
void viewport_subrectangle_set(const Area2F* window);

/* Draw_GetClipWindow (FUN_00485fb0). */
void viewport_rectangle_get(Area2F* window);

/* Draw_LineListEx (FUN_00485ff0): count lines, from fromList[i] to
 * toList[i]. Channels run 0..1. */
void surface_draw_polyline(Point2F* fromList, Point2F* toList,
                           uint32_t count, float r, float g, float b,
                           int effect);

/* Draw_RectListEx (FUN_00486090): fill count rectangles in one colour. */
void surface_fill_pixel_spans(Area2F* rectList, uint32_t count, float r,
                              float g, float b, int effect);

/* Draw_RectList2Ex (FUN_00486210): fill count rectangles, each in its own
 * colour. */
void surface_fill_pixel_span_records(DrawRect* rectList, uint32_t count,
                                     int effect);

/* Draw_DotCircle (FUN_00486390): dotCount dots on a circle. */
void surface_draw_circle_pixels(Point2F* pos, uint32_t radius,
                                uint32_t dotCount, float r, float g, float b,
                                int effect);

/* Draw_GetColour (FUN_004864d0): the pixel value of a colour on the locked
 * surface; 0 on an 8-bit surface. */
uint32_t surface_color_pack_to_pixel(float r, float g, float b);

/* Draw_LockSurface (FUN_00486550): lock surf, record its format and choose
 * the pixel writer. Returns 1 with the surface locked, or 0. */
int surface_active_pixel_writer_configure(void* surf, int effect);

/* Draw_UnlockSurface (FUN_00486650). */
void surface_unlock_reset_pixel_writer(void* surf);

/* Draw_SetDrawPixelFunc (FUN_00486690): choose the pixel writer for the
 * locked surface's depth. Returns 0 for an unsupported depth. */
int surface_pixel_writer_select_by_format(int effect);

/* Draw_LineActual (FUN_00486720): a clipped Bresenham line, both ends
 * included. */
void surface_bresenham_line_walk_clipped(int32_t x1, int32_t y1, int32_t x2,
                                         int32_t y2, uint32_t colour);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_DRAW_H */
