// SPDX-License-Identifier: LGPL-3.0-or-later
/* Original translation unit G98-019 (text 00485e80-004869f0, .rdata
 * 0049ea10-0049ea38, .bss 005407e8-00540834; findings 505 and 519).
 *
 * 2D drawing on the back buffer: the clip window, clipped Bresenham lines,
 * filled rectangles and dotted circles, the colour packer, the surface
 * lock, and the six pixel writers it chooses between. Rebuilt in the
 * original's shape (issue #412, ADR 0005): one file for the unit, its
 * functions in original order and the C-compatible subset, so the matching
 * build compiles it as C. The native port compiles it as C++.
 *
 * The binary does not name the file. The exported functions keep the
 * port's names; the comments and the static pixel writers use descriptive
 * Draw_* names, which are not attested either.
 *
 * The unit calls no other unit. It reads the back buffer from the common
 * word at 0076ac8c (crt.h); the native port reads draw_port_back_surface
 * instead (draw.h). */
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "draw.h"
#include "crt/crt.h"

#if defined(LEGORRR_BACKEND_MINIWIN)
#define DRAW_BACK_SURFACE() (draw_port_back_surface)
#else
#define DRAW_BACK_SURFACE() ((DrawSurface*)(uintptr_t)s_render_surface_secondary)
#endif

#define DRAW_FLAG_INITIALISED 0x1

#define DRAW_DD_OK 0
#define DRAW_DDLOCK_WAIT 0x1

typedef void (*DrawPixelFunc)(int32_t x, int32_t y, uint32_t value);

/* The unit's .bss (005407e8, 0x4c bytes on Win32). */
typedef struct DrawGlobs {
    DrawPixelFunc drawPixelFunc;  /* +0x00 005407e8 */
    Point2F clipStart;            /* +0x04 005407ec */
    Point2F clipEnd;              /* +0x0c 005407f4 (exclusive) */
    uint32_t reserved_14[4];      /* +0x14 005407fc: never referenced */
    void* buffer;                 /* +0x24 0054080c */
    uint32_t pitch;               /* +0x28 00540810 */
    uint32_t bpp;                 /* +0x2c 00540814 */
    uint32_t redMask;             /* +0x30 00540818 */
    uint32_t greenMask;           /* +0x34 0054081c */
    uint32_t blueMask;            /* +0x38 00540820 */
    uint32_t redBits;             /* +0x3c 00540824 */
    uint32_t greenBits;           /* +0x40 00540828 */
    uint32_t blueBits;            /* +0x44 0054082c */
    uint32_t flags;               /* +0x48 00540830 */
} DrawGlobs;

/* The .rdata block opens with the {1, 2, 4, -1} header table at 0049ea10
 * (d3drmdef.h, finding 524), which is not reproduced: the toolchain's
 * DirectX 3 d3drm.h moves the symbol counter so that Draw_Pixel24 and
 * viewport_subrectangle_set fall (finding 646). 0049ea2c is the double's
 * alignment. */
// GLOBAL: LEGORR 0x0049ea20
static const float s_draw_zero = 0.0f;
// GLOBAL: LEGORR 0x0049ea24
static const float s_draw_one = 1.0f;
// GLOBAL: LEGORR 0x0049ea28
static const float s_draw_two_pi = 6.28318548f;
// GLOBAL: LEGORR 0x0049ea30
static const double s_draw_255 = 255.0;

// GLOBAL: LEGORR 0x005407e8
static DrawGlobs drawGlobs;

static void Draw_Pixel8(int32_t x, int32_t y, uint32_t value);
static void Draw_Pixel16(int32_t x, int32_t y, uint32_t value);
static void Draw_Pixel16XOR(int32_t x, int32_t y, uint32_t value);
static void Draw_Pixel16HalfTrans(int32_t x, int32_t y, uint32_t value);
static void Draw_Pixel24(int32_t x, int32_t y, uint32_t value);
static void Draw_Pixel32(int32_t x, int32_t y, uint32_t value);

/* Write one pixel if it lies inside the clip window. */
#define Draw_PixelClip(x, y, c)                                            \
    if ((float)(x) >= drawGlobs.clipStart.x &&                             \
        (float)(y) >= drawGlobs.clipStart.y &&                             \
        (float)(x) < drawGlobs.clipEnd.x &&                                \
        (float)(y) < drawGlobs.clipEnd.y)                                  \
        drawGlobs.drawPixelFunc((x), (y), (c))

/* Draw_Initialise. */
// FUNCTION: LEGORR 0x00485e80
void surface_render_setup(const Area2F* window)
{
    drawGlobs.flags |= DRAW_FLAG_INITIALISED;
    viewport_subrectangle_set(window);
}

/* Draw_SetClipWindow. */
// FUNCTION: LEGORR 0x00485ea0
void viewport_subrectangle_set(const Area2F* window)
{
    DrawSurface* surf = DRAW_BACK_SURFACE();
    DrawSurfaceDesc desc;
    float width, height;

    drawGlobs.clipStart.x = 0.0f;
    drawGlobs.clipStart.y = 0.0f;
    if (window) {
        if (window->x > s_draw_zero) drawGlobs.clipStart.x = window->x;
        if (window->y > s_draw_zero) drawGlobs.clipStart.y = window->y;
    }

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    if (surf->lpVtbl->GetSurfaceDesc(surf, &desc) == DRAW_DD_OK) {
        width = (float)desc.dwWidth;
        height = (float)desc.dwHeight;
        drawGlobs.clipEnd.x = width;
        drawGlobs.clipEnd.y = height;
        if (window) {
            if (window->width + window->x < width)
                drawGlobs.clipEnd.x = window->width + window->x;
            if (window->height + window->y < height)
                drawGlobs.clipEnd.y = window->height + window->y;
        }
    }
}

/* Draw_GetClipWindow. */
// FUNCTION: LEGORR 0x00485fb0
void viewport_rectangle_get(Area2F* window)
{
    window->x = drawGlobs.clipStart.x;
    window->y = drawGlobs.clipStart.y;
    window->width = drawGlobs.clipEnd.x - drawGlobs.clipStart.x;
    window->height = drawGlobs.clipEnd.y - drawGlobs.clipStart.y;
}

/* Draw_LineListEx. */
// FUNCTION: LEGORR 0x00485ff0
void surface_draw_polyline(Point2F* fromList, Point2F* toList,
                           uint32_t count, float r, float g, float b,
                           int effect)
{
    DrawSurface* surf = DRAW_BACK_SURFACE();
    uint32_t colour;
    uint32_t loop;
    Point2F* from;
    Point2F* to;

    if (surface_active_pixel_writer_configure(surf, effect)) {
        colour = surface_color_pack_to_pixel(r, g, b);
        for (loop = 0; loop < count; loop++) {
            from = &fromList[loop];
            to = &toList[loop];
            surface_bresenham_line_walk_clipped((int32_t)from->x, (int32_t)from->y,
                                                (int32_t)to->x, (int32_t)to->y,
                                                colour);
        }
        surface_unlock_reset_pixel_writer(surf);
    }
}

/* Draw_RectListEx. */
// FUNCTION: LEGORR 0x00486090
void surface_fill_pixel_spans(Area2F* rectList, uint32_t count, float r,
                              float g, float b, int effect)
{
    DrawSurface* surf = DRAW_BACK_SURFACE();
    uint32_t colour;
    uint32_t loop;
    Area2F rect;
    Point2F end;
    int32_t x, y, endX, endY;

    if (surface_active_pixel_writer_configure(surf, effect)) {
        colour = surface_color_pack_to_pixel(r, g, b);
        for (loop = 0; loop < count; loop++) {
            rect = rectList[loop];
            end.x = rect.x + rect.width;
            end.y = rect.y + rect.height;
            if (rect.x < drawGlobs.clipStart.x) rect.x = drawGlobs.clipStart.x;
            if (rect.y < drawGlobs.clipStart.y) rect.y = drawGlobs.clipStart.y;
            if (end.x >= drawGlobs.clipEnd.x) end.x = drawGlobs.clipEnd.x - s_draw_one;
            if (end.y >= drawGlobs.clipEnd.y) end.y = drawGlobs.clipEnd.y - s_draw_one;
            endX = (int32_t)end.x;
            endY = (int32_t)end.y;
            for (y = (int32_t)rect.y; y < endY; y++) {
                for (x = (int32_t)rect.x; x < endX; x++) {
                    drawGlobs.drawPixelFunc(x, y, colour);
                }
            }
        }
        surface_unlock_reset_pixel_writer(surf);
    }
}

/* Draw_RectList2Ex. */
// FUNCTION: LEGORR 0x00486210
void surface_fill_pixel_span_records(DrawRect* rectList, uint32_t count,
                                     int effect)
{
    DrawSurface* surf = DRAW_BACK_SURFACE();
    uint32_t colour;
    uint32_t loop;
    Area2F rect;
    Point2F end;
    int32_t x, y, endX, endY;

    if (surface_active_pixel_writer_configure(surf, effect)) {
        for (loop = 0; loop < count; loop++) {
            rect = rectList[loop].rect;
            colour = surface_color_pack_to_pixel(rectList[loop].r,
                                                 rectList[loop].g,
                                                 rectList[loop].b);
            end.x = rect.x + rect.width;
            end.y = rect.y + rect.height;
            if (rect.x < drawGlobs.clipStart.x) rect.x = drawGlobs.clipStart.x;
            if (rect.y < drawGlobs.clipStart.y) rect.y = drawGlobs.clipStart.y;
            if (end.x >= drawGlobs.clipEnd.x) end.x = drawGlobs.clipEnd.x - s_draw_one;
            if (end.y >= drawGlobs.clipEnd.y) end.y = drawGlobs.clipEnd.y - s_draw_one;
            endX = (int32_t)end.x;
            endY = (int32_t)end.y;
            for (y = (int32_t)rect.y; y < endY; y++) {
                for (x = (int32_t)rect.x; x < endX; x++) {
                    drawGlobs.drawPixelFunc(x, y, colour);
                }
            }
        }
        surface_unlock_reset_pixel_writer(surf);
    }
}

/* Draw_DotCircle. */
// FUNCTION: LEGORR 0x00486390
void surface_draw_circle_pixels(Point2F* pos, uint32_t radius,
                                uint32_t dotCount, float r, float g, float b,
                                int effect)
{
    DrawSurface* surf = DRAW_BACK_SURFACE();
    float step = s_draw_two_pi / (float)dotCount;
    uint32_t colour;
    uint32_t loop;
    uint32_t x, y;

    if (surface_active_pixel_writer_configure(surf, effect)) {
        colour = surface_color_pack_to_pixel(r, g, b);
        for (loop = 0; loop < dotCount; loop++) {
            x = (uint32_t)((float)(sin(step * (float)loop) * (float)radius) + pos->x);
            y = (uint32_t)((float)(cos(step * (float)loop) * (float)radius) + pos->y);
            Draw_PixelClip(x, y, colour);
        }
        surface_unlock_reset_pixel_writer(surf);
    }
}

/* Draw_GetColour. */
// FUNCTION: LEGORR 0x004864d0
uint32_t surface_color_pack_to_pixel(float r, float g, float b)
{
    uint32_t colour = 0;

    if (drawGlobs.bpp != 8) {
        colour = ((uint32_t)(r * s_draw_255) >> (8 - drawGlobs.redBits))
                 << (drawGlobs.greenBits + drawGlobs.blueBits);
        colour |= ((uint32_t)(g * s_draw_255) >> (8 - drawGlobs.greenBits))
                  << drawGlobs.blueBits;
        colour |= (uint32_t)(b * s_draw_255) >> (8 - drawGlobs.blueBits);
    }
    return colour;
}

/* Draw_LockSurface. */
// FUNCTION: LEGORR 0x00486550
int surface_active_pixel_writer_configure(void* surf, int effect)
{
    DrawSurface* surface = (DrawSurface*)surf;
    DrawSurfaceDesc desc;
    uint32_t loop;

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    if (surface->lpVtbl->Lock(surface, NULL, &desc, DRAW_DDLOCK_WAIT, NULL) == DRAW_DD_OK) {
        drawGlobs.buffer = desc.lpSurface;
        drawGlobs.pitch = desc.lPitch;
        drawGlobs.redMask = desc.pfRBitMask;
        drawGlobs.greenMask = desc.pfGBitMask;
        drawGlobs.blueMask = desc.pfBBitMask;
        drawGlobs.bpp = desc.pfRGBBitCount;
        drawGlobs.redBits = drawGlobs.greenBits = drawGlobs.blueBits = 0;
        for (loop = 0; loop < drawGlobs.bpp; loop++) {
            if (drawGlobs.redMask & (1 << loop)) drawGlobs.redBits++;
            if (drawGlobs.greenMask & (1 << loop)) drawGlobs.greenBits++;
            if (drawGlobs.blueMask & (1 << loop)) drawGlobs.blueBits++;
        }
        if (surface_pixel_writer_select_by_format(effect)) return 1;
        surface_unlock_reset_pixel_writer(surf);
    }
    return 0;
}

/* Draw_UnlockSurface. */
// FUNCTION: LEGORR 0x00486650
void surface_unlock_reset_pixel_writer(void* surf)
{
    DrawSurface* surface = (DrawSurface*)surf;

    surface->lpVtbl->Unlock(surface, NULL);
    drawGlobs.buffer = NULL;
    drawGlobs.pitch = 0;
    drawGlobs.redMask = 0;
    drawGlobs.greenMask = 0;
    drawGlobs.blueMask = 0;
    drawGlobs.drawPixelFunc = NULL;
    drawGlobs.bpp = 0;
}

/* Draw_SetDrawPixelFunc. */
// FUNCTION: LEGORR 0x00486690
int surface_pixel_writer_select_by_format(int effect)
{
    if (drawGlobs.bpp == 8) {
        drawGlobs.drawPixelFunc = Draw_Pixel8;
    } else if (drawGlobs.bpp == 16) {
        if (effect == DRAW_EFFECT_XOR) drawGlobs.drawPixelFunc = Draw_Pixel16XOR;
        else if (effect == DRAW_EFFECT_HALF_TRANS) drawGlobs.drawPixelFunc = Draw_Pixel16HalfTrans;
        else drawGlobs.drawPixelFunc = Draw_Pixel16;
    } else if (drawGlobs.bpp == 24) {
        drawGlobs.drawPixelFunc = Draw_Pixel24;
    } else if (drawGlobs.bpp == 32) {
        drawGlobs.drawPixelFunc = Draw_Pixel32;
    } else {
        drawGlobs.drawPixelFunc = NULL;
        return 0;
    }
    return 1;
}

/* Draw_LineActual. */
// FUNCTION: LEGORR 0x00486720
void surface_bresenham_line_walk_clipped(int32_t x1, int32_t y1, int32_t x2,
                                         int32_t y2, uint32_t colour)
{
    int32_t i, deltax, deltay, numpixels;
    int32_t d, dinc1, dinc2;
    int32_t x, xinc1, xinc2;
    int32_t y, yinc1, yinc2;

    deltax = abs(x2 - x1);
    deltay = abs(y2 - y1);

    if (deltax >= deltay) {
        numpixels = deltax + 1;
        d = (2 * deltay) - deltax;
        dinc1 = deltay << 1;
        dinc2 = (deltay - deltax) << 1;
        xinc1 = 1;
        xinc2 = 1;
        yinc1 = 0;
        yinc2 = 1;
    } else {
        numpixels = deltay + 1;
        d = (2 * deltax) - deltay;
        dinc1 = deltax << 1;
        dinc2 = (deltax - deltay) << 1;
        xinc1 = 0;
        xinc2 = 1;
        yinc1 = 1;
        yinc2 = 1;
    }

    if (x1 > x2) {
        xinc1 = -xinc1;
        xinc2 = -xinc2;
    }
    if (y1 > y2) {
        yinc1 = -yinc1;
        yinc2 = -yinc2;
    }

    x = x1;
    y = y1;
    for (i = 1; i <= numpixels; i++) {
        Draw_PixelClip(x, y, colour);
        if (d < 0) {
            d = d + dinc1;
            x = x + xinc1;
            y = y + yinc1;
        } else {
            d = d + dinc2;
            x = x + xinc2;
            y = y + yinc2;
        }
    }
}

// FUNCTION: LEGORR 0x00486880
static void Draw_Pixel8(int32_t x, int32_t y, uint32_t value)
{
    ((uint8_t*)drawGlobs.buffer)[(y * drawGlobs.pitch) + x] = (uint8_t)value;
}

// FUNCTION: LEGORR 0x004868a0
static void Draw_Pixel16(int32_t x, int32_t y, uint32_t value)
{
    ((uint16_t*)((uint8_t*)drawGlobs.buffer + (y * drawGlobs.pitch)))[x] = (uint16_t)value;
}

// FUNCTION: LEGORR 0x004868d0
static void Draw_Pixel16XOR(int32_t x, int32_t y, uint32_t value)
{
    uint16_t* addr = &((uint16_t*)((uint8_t*)drawGlobs.buffer + (y * drawGlobs.pitch)))[x];
    *addr ^= (uint16_t)value;
}

/* Average the new colour with the pixel, channel by channel. Red is
 * masked with 0xf000 >> 1, as for a 5:6:5 surface. */
// FUNCTION: LEGORR 0x00486900
static void Draw_Pixel16HalfTrans(int32_t x, int32_t y, uint32_t value)
{
    uint16_t* addr = &((uint16_t*)((uint8_t*)drawGlobs.buffer + (y * drawGlobs.pitch)))[x];
    uint16_t pixel = *addr;
    uint16_t colour = (uint16_t)value;
    uint16_t greenHalf = (uint16_t)((uint16_t)drawGlobs.greenMask >> 1);
    uint16_t blueHalf = (uint16_t)((uint16_t)drawGlobs.blueMask >> 1);

    *addr = (uint16_t)(
        (((((colour >> 1) & greenHalf) & 0x7fe0) + (((pixel >> 1) & greenHalf) & 0x7fe0)) & drawGlobs.greenMask) |
        ((((colour >> 1) & blueHalf) + ((pixel >> 1) & blueHalf)) & drawGlobs.blueMask) |
        ((((colour >> 1) & 0x7800) + ((pixel >> 1) & 0x7800)) & drawGlobs.redMask));
}

/* The original writes a whole dword: the colour goes in bytes 1-3 and
 * byte 0 keeps the previous value. */
// FUNCTION: LEGORR 0x004869a0
static void Draw_Pixel24(int32_t x, int32_t y, uint32_t value)
{
    uint32_t* addr = (uint32_t*)((uint8_t*)drawGlobs.buffer + (y * drawGlobs.pitch) + (x * 3));
    *addr = (value << 8) | (*addr & 0xff);
}

// FUNCTION: LEGORR 0x004869d0
static void Draw_Pixel32(int32_t x, int32_t y, uint32_t value)
{
    ((uint32_t*)((uint8_t*)drawGlobs.buffer + (y * drawGlobs.pitch)))[x] = value;
}
