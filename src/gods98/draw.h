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

/* The back buffer is miniwin's IDirectDrawSurface4. Declared, not included:
 * shared headers do not pull in miniwin's DirectX headers. */
struct IDirectDrawSurface4;
typedef struct IDirectDrawSurface4* LPDIRECTDRAWSURFACE4;


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
int surface_active_pixel_writer_configure(LPDIRECTDRAWSURFACE4 surf,
                                          int effect);

/* Draw_UnlockSurface (FUN_00486650). */
void surface_unlock_reset_pixel_writer(LPDIRECTDRAWSURFACE4 surf);

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
