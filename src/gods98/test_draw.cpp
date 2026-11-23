// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_draw — the G98-019 unit (gods98/draw.c, issue #412): the clip
 * window, the surface lock and colour packer, the pixel writers for each
 * depth and effect, clipped lines, rectangle fills and dotted circles.
 *
 * draw.c draws on DirectDraw.c's back buffer (directDrawGlobs.bSurf); the
 * test installs a surface whose methods fail, then a fake one. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <miniwin/ddraw.h>

#include "gods98/directdraw.h"
#include "gods98/draw.h"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

/* A DirectDraw surface whose every method fails with DDERR_SURFACELOST. */
struct LostSurface : public IDirectDrawSurface4 {
    // IDirectDrawSurface
    HRESULT AddAttachedSurface(LPDIRECTDRAWSURFACE) override { return DDERR_SURFACELOST; }
    HRESULT Blt(LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DDBltFlags, LPDDBLTFX) override
    {
        return DDERR_SURFACELOST;
    }
    HRESULT BltFast(DWORD, DWORD, LPDIRECTDRAWSURFACE, LPRECT, DDBltFastFlags) override
    {
        return DDERR_SURFACELOST;
    }
    HRESULT Flip(LPDIRECTDRAWSURFACE, DDFlipFlags) override { return DDERR_SURFACELOST; }
    HRESULT GetAttachedSurface(LPDDSCAPS, LPDIRECTDRAWSURFACE*) override { return DDERR_SURFACELOST; }
    HRESULT GetDC(HDC*) override { return DDERR_SURFACELOST; }
    HRESULT GetPalette(LPDIRECTDRAWPALETTE*) override { return DDERR_SURFACELOST; }
    HRESULT GetPixelFormat(LPDDPIXELFORMAT) override { return DDERR_SURFACELOST; }
    HRESULT GetSurfaceDesc(LPDDSURFACEDESC) override { return DDERR_SURFACELOST; }
    HRESULT IsLost() override { return DDERR_SURFACELOST; }
    HRESULT Lock(LPRECT, LPDDSURFACEDESC, DDLockFlags, HANDLE) override { return DDERR_SURFACELOST; }
    HRESULT ReleaseDC(HDC) override { return DDERR_SURFACELOST; }
    HRESULT Restore() override { return DDERR_SURFACELOST; }
    HRESULT SetClipper(LPDIRECTDRAWCLIPPER) override { return DDERR_SURFACELOST; }
    HRESULT SetColorKey(DDColorKeyFlags, LPDDCOLORKEY) override { return DDERR_SURFACELOST; }
    HRESULT SetPalette(LPDIRECTDRAWPALETTE) override { return DDERR_SURFACELOST; }
    HRESULT Unlock(LPVOID) override { return DDERR_SURFACELOST; }
    // IDirectDrawSurface4
    HRESULT Blt(LPRECT, LPDIRECTDRAWSURFACE4, LPRECT, DDBltFlags, LPDDBLTFX) override
    {
        return DDERR_SURFACELOST;
    }
    HRESULT Flip(LPDIRECTDRAWSURFACE4, DDFlipFlags) override { return DDERR_SURFACELOST; }
    HRESULT GetAttachedSurface(LPDDSCAPS2, LPDIRECTDRAWSURFACE4*) override { return DDERR_SURFACELOST; }
    HRESULT GetSurfaceDesc(LPDDSURFACEDESC2) override { return DDERR_SURFACELOST; }
    HRESULT Lock(LPRECT, LPDDSURFACEDESC2, DDLockFlags, HANDLE) override { return DDERR_SURFACELOST; }
    HRESULT Unlock(LPRECT) override { return DDERR_SURFACELOST; }
};


enum { FAKE_WIDTH = 10, FAKE_HEIGHT = 8, FAKE_PITCH = 48 };

/* An IDirectDrawSurface4 with the three methods draw.c calls. */
struct FakeSurface : public LostSurface {
    uint32_t bpp;
    uint32_t masks[3];
    int locks;
    int unlocks;
    int locked;
    uint8_t pixels[FAKE_PITCH * FAKE_HEIGHT + 4];

    HRESULT GetSurfaceDesc(LPDDSURFACEDESC2 desc) override
    {
        if (desc->dwSize != sizeof(DDSURFACEDESC2)) return DDERR_INVALIDPARAMS;
        desc->dwWidth = FAKE_WIDTH;
        desc->dwHeight = FAKE_HEIGHT;
        return DD_OK;
    }

    HRESULT Lock(LPRECT rect, LPDDSURFACEDESC2 desc, DDLockFlags flags,
                 HANDLE event) override
    {
        if (rect != NULL || event != NULL || flags != DDLOCK_WAIT)
            return DDERR_INVALIDPARAMS;
        if (desc->dwSize != sizeof(DDSURFACEDESC2)) return DDERR_INVALIDPARAMS;
        locks++;
        locked = 1;
        desc->lpSurface = pixels;
        desc->lPitch = FAKE_PITCH;
        desc->ddpfPixelFormat.dwRGBBitCount = bpp;
        desc->ddpfPixelFormat.dwRBitMask = masks[0];
        desc->ddpfPixelFormat.dwGBitMask = masks[1];
        desc->ddpfPixelFormat.dwBBitMask = masks[2];
        return DD_OK;
    }

    HRESULT Unlock(LPRECT) override
    {
        unlocks++;
        locked = 0;
        return DD_OK;
    }
};

static void fake_init(FakeSurface* fake, uint32_t bpp, uint32_t red,
                      uint32_t green, uint32_t blue)
{
    fake->bpp = bpp;
    fake->masks[0] = red;
    fake->masks[1] = green;
    fake->masks[2] = blue;
    fake->locks = 0;
    fake->unlocks = 0;
    fake->locked = 0;
    memset(fake->pixels, 0, sizeof(fake->pixels));
}

static uint16_t pixel16(const FakeSurface* fake, int x, int y)
{
    uint16_t value;
    memcpy(&value, fake->pixels + y * FAKE_PITCH + x * 2, sizeof(value));
    return value;
}

static int count16(const FakeSurface* fake)
{
    int x, y, count = 0;
    for (y = 0; y < FAKE_HEIGHT; y++)
        for (x = 0; x < FAKE_WIDTH; x++)
            if (pixel16(fake, x, y) != 0) count++;
    return count;
}

static void set_area(Area2F* area, float x, float y, float w, float h)
{
    area->x = x;
    area->y = y;
    area->width = w;
    area->height = h;
}

int main(void)
{
    LostSurface lost;
    LPDIRECTDRAWSURFACE4 const native_surface = &lost;
    FakeSurface fake;
    Area2F area;
    Area2F clip;

    CHECK(sizeof(DrawRect) == 0x1c, "a coloured rectangle is 0x1c bytes");

    /* A lost back buffer fails, so nothing is locked or drawn. */
    directDrawGlobs.bSurf = native_surface;
    surface_render_setup(NULL);
    viewport_rectangle_get(&clip);
    CHECK(clip.x == 0.0f && clip.y == 0.0f, "no surface: clip starts at 0");
    CHECK(!surface_active_pixel_writer_configure(native_surface,
                                                 DRAW_EFFECT_NONE),
          "no surface: the lock fails");

    fake_init(&fake, 16, 0xf800, 0x07e0, 0x001f);
    directDrawGlobs.bSurf = &fake;

    /* Draw_SetClipWindow / Draw_GetClipWindow. */
    viewport_subrectangle_set(NULL);
    viewport_rectangle_get(&clip);
    CHECK(clip.x == 0.0f && clip.y == 0.0f && clip.width == 10.0f &&
          clip.height == 8.0f, "NULL clips to the whole surface");
    set_area(&area, 2.0f, 1.0f, 5.0f, 4.0f);
    viewport_subrectangle_set(&area);
    viewport_rectangle_get(&clip);
    CHECK(clip.x == 2.0f && clip.y == 1.0f && clip.width == 5.0f &&
          clip.height == 4.0f, "a window inside the surface");
    set_area(&area, -3.0f, 6.0f, 20.0f, 20.0f);
    viewport_subrectangle_set(&area);
    viewport_rectangle_get(&clip);
    CHECK(clip.x == 0.0f && clip.y == 6.0f && clip.width == 10.0f &&
          clip.height == 2.0f, "a window past the surface is cut to it");
    /* A negative origin is not added back: the end is x + width. */
    set_area(&area, -3.0f, 0.0f, 8.0f, 8.0f);
    viewport_subrectangle_set(&area);
    viewport_rectangle_get(&clip);
    CHECK(clip.x == 0.0f && clip.width == 5.0f,
          "a negative origin ends at x + width");
    surface_render_setup(NULL);

    /* Draw_LockSurface, Draw_GetColour and Draw_UnlockSurface (5:6:5). */
    CHECK(surface_active_pixel_writer_configure(&fake, DRAW_EFFECT_NONE),
          "a 16-bit surface locks");
    CHECK(fake.locked == 1, "the surface stays locked");
    CHECK(surface_color_pack_to_pixel(1.0f, 0.0f, 0.0f) == 0xf800, "red");
    CHECK(surface_color_pack_to_pixel(0.0f, 1.0f, 0.0f) == 0x07e0, "green");
    CHECK(surface_color_pack_to_pixel(0.0f, 0.0f, 1.0f) == 0x001f, "blue");
    CHECK(surface_color_pack_to_pixel(1.0f, 1.0f, 1.0f) == 0xffff, "white");
    surface_unlock_reset_pixel_writer(&fake);
    CHECK(fake.locked == 0 && fake.unlocks == 1, "the surface is unlocked");

    /* Draw_LineListEx and Draw_LineActual: both ends drawn, clipped. */
    {
        Point2F from[2], to[2];
        from[0].x = 0.0f; from[0].y = 0.0f;
        to[0].x = 3.0f;   to[0].y = 3.0f;
        from[1].x = -2.0f; from[1].y = 7.0f;
        to[1].x = 12.0f;   to[1].y = 7.0f;
        surface_draw_polyline(from, to, 2, 0.0f, 1.0f, 0.0f,
                              DRAW_EFFECT_NONE);
        CHECK(pixel16(&fake, 0, 0) == 0x07e0 && pixel16(&fake, 3, 3) == 0x07e0,
              "a diagonal includes both ends");
        CHECK(pixel16(&fake, 1, 1) == 0x07e0 && pixel16(&fake, 2, 2) == 0x07e0,
              "a diagonal steps both axes");
        CHECK(pixel16(&fake, 0, 7) == 0x07e0 && pixel16(&fake, 9, 7) == 0x07e0,
              "a clipped line reaches both edges");
        CHECK(count16(&fake) == 4 + 10, "nothing outside the clip window");
        CHECK(fake.locked == 0 && fake.locks == 2, "lines unlock the surface");
    }

    /* A steep reverse line enters and exits the clip midway through its
     * Bresenham steps. Clipping must preserve the original pixel positions. */
    {
        Point2F from = {5.0f, 7.0f}, to = {2.0f, 0.0f};
        memset(fake.pixels, 0, sizeof(fake.pixels));
        set_area(&area, 3.0f, 2.0f, 3.0f, 4.0f);
        viewport_subrectangle_set(&area);
        surface_draw_polyline(&from, &to, 1, 1.0f, 0.0f, 0.0f, DRAW_EFFECT_NONE);
        CHECK(pixel16(&fake, 4, 5) == 0xf800 && pixel16(&fake, 4, 4) == 0xf800 &&
                  pixel16(&fake, 3, 3) == 0xf800 && pixel16(&fake, 3, 2) == 0xf800,
              "reverse steep line follows the uncut raster inside the clip");
        CHECK(count16(&fake) == 4, "reverse steep line leaves the outside untouched");
        viewport_subrectangle_set(NULL);
    }

    /* Draw_RectListEx: the end column and row are exclusive, and a
     * rectangle that reaches the clip edge stops one pixel short of it. */
    {
        Area2F rects[2];
        memset(fake.pixels, 0, sizeof(fake.pixels));
        set_area(&rects[0], 1.0f, 1.0f, 3.0f, 2.0f);
        set_area(&rects[1], -5.0f, 6.0f, 30.0f, 30.0f);
        surface_fill_pixel_spans(rects, 2, 0.0f, 0.0f, 1.0f, DRAW_EFFECT_NONE);
        CHECK(pixel16(&fake, 1, 1) == 0x001f && pixel16(&fake, 3, 2) == 0x001f,
              "the rectangle is filled");
        CHECK(pixel16(&fake, 4, 1) == 0 && pixel16(&fake, 1, 3) == 0,
              "the far edges are exclusive");
        CHECK(pixel16(&fake, 0, 6) == 0x001f && pixel16(&fake, 8, 6) == 0x001f,
              "a clipped rectangle starts at the clip window");
        CHECK(pixel16(&fake, 9, 6) == 0 && pixel16(&fake, 0, 7) == 0,
              "a clipped rectangle stops short of the clip edge");
        CHECK(count16(&fake) == 6 + 9, "fill count");
    }

    /* Draw_RectList2Ex: each rectangle has its own colour. */
    {
        DrawRect rects[2];
        memset(fake.pixels, 0, sizeof(fake.pixels));
        memset(rects, 0, sizeof(rects));
        set_area(&rects[0].rect, 0.0f, 0.0f, 1.0f, 1.0f);
        rects[0].r = 1.0f;
        set_area(&rects[1].rect, 2.0f, 0.0f, 1.0f, 1.0f);
        rects[1].b = 1.0f;
        surface_fill_pixel_span_records(rects, 2, DRAW_EFFECT_NONE);
        CHECK(pixel16(&fake, 0, 0) == 0xf800 && pixel16(&fake, 2, 0) == 0x001f,
              "per-rectangle colours");
        CHECK(count16(&fake) == 2, "one pixel each");
    }

    /* Draw_DotCircle: dot i at (x + r sin(i step), y + r cos(i step)). */
    {
        Point2F centre;
        memset(fake.pixels, 0, sizeof(fake.pixels));
        centre.x = 5.5f;
        centre.y = 4.5f;
        surface_draw_circle_pixels(&centre, 2, 4, 1.0f, 1.0f, 1.0f,
                                   DRAW_EFFECT_NONE);
        CHECK(pixel16(&fake, 5, 6) == 0xffff && pixel16(&fake, 7, 4) == 0xffff,
              "the first two dots");
        CHECK(pixel16(&fake, 5, 2) == 0xffff && pixel16(&fake, 3, 4) == 0xffff,
              "the last two dots");
        CHECK(count16(&fake) == 4, "four dots");
    }

    /* The 16-bit effects. */
    {
        Point2F from, to;
        memset(fake.pixels, 0, sizeof(fake.pixels));
        from.x = 1.0f; from.y = 1.0f;
        to = from;
        surface_draw_polyline(&from, &to, 1, 1.0f, 1.0f, 1.0f,
                              DRAW_EFFECT_HALF_TRANS);
        CHECK(pixel16(&fake, 1, 1) == 0x7bef, "half of white on black");
        surface_draw_polyline(&from, &to, 1, 1.0f, 1.0f, 1.0f,
                              DRAW_EFFECT_XOR);
        CHECK(pixel16(&fake, 1, 1) == (0x7bef ^ 0xffff), "XOR");
        surface_draw_polyline(&from, &to, 1, 1.0f, 1.0f, 1.0f,
                              DRAW_EFFECT_XOR);
        CHECK(pixel16(&fake, 1, 1) == 0x7bef, "XOR twice restores");
    }

    /* 32, 24 and 8 bits. */
    {
        Point2F at;
        uint32_t value;
        fake_init(&fake, 32, 0xff0000, 0x00ff00, 0x0000ff);
        at.x = 2.0f; at.y = 1.0f;
        surface_draw_polyline(&at, &at, 1, 1.0f, 0.0f, 1.0f, DRAW_EFFECT_NONE);
        memcpy(&value, fake.pixels + FAKE_PITCH + 2 * 4, sizeof(value));
        CHECK(value == 0xff00ff, "a 32-bit pixel");

        /* The 24-bit writer stores the colour in bytes 1-3 of the dword at
         * x * 3 and keeps byte 0. */
        fake_init(&fake, 24, 0xff0000, 0x00ff00, 0x0000ff);
        fake.pixels[FAKE_PITCH + 6] = 0x5a;
        surface_draw_polyline(&at, &at, 1, 0.0f, 1.0f, 0.0f, DRAW_EFFECT_NONE);
        memcpy(&value, fake.pixels + FAKE_PITCH + 2 * 3, sizeof(value));
        CHECK(value == 0x00ff005a, "a 24-bit pixel");

        fake_init(&fake, 8, 0, 0, 0);
        CHECK(surface_active_pixel_writer_configure(&fake,
                                                    DRAW_EFFECT_NONE),
              "an 8-bit surface locks");
        CHECK(surface_color_pack_to_pixel(1.0f, 1.0f, 1.0f) == 0,
              "8-bit colours are 0");
        surface_unlock_reset_pixel_writer(&fake);
        fake.pixels[FAKE_PITCH + 2] = 0x33;
        surface_draw_polyline(&at, &at, 1, 1.0f, 1.0f, 1.0f, DRAW_EFFECT_NONE);
        CHECK(fake.pixels[FAKE_PITCH + 2] == 0, "an 8-bit pixel");

        fake_init(&fake, 15, 0x7c00, 0x03e0, 0x001f);
        CHECK(!surface_active_pixel_writer_configure(&fake,
                                                     DRAW_EFFECT_NONE),
              "an unsupported depth fails");
        CHECK(fake.locks == 1 && fake.unlocks == 1 && fake.locked == 0,
              "and unlocks the surface");
    }

    directDrawGlobs.bSurf = NULL;
    if (s_failures != 0) {
        printf("%d failure(s)\n", s_failures);
        return 1;
    }
    printf("draw: all checks passed\n");
    return 0;
}
