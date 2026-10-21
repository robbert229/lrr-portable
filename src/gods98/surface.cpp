// SPDX-License-Identifier: LGPL-3.0-or-later
#include "platform/platform.h"
#include "surface.hpp"

/* ---- DirectDraw-backed implementation (miniwin) ---------------------- */

#if defined(LEGORRR_BACKEND_MINIWIN)

static LPDIRECTDRAW s_dd(void* p)      { return (LPDIRECTDRAW)p; }
static LPDIRECTDRAWSURFACE s_surf(void* p) { return (LPDIRECTDRAWSURFACE)p; }

int surface_create(DisplaySurface* out, void* hwnd, int w, int h)
{
    LPDIRECTDRAW dd = NULL;
    if (out == NULL || w <= 0 || h <= 0) {
        return -1;
    }
    if (DirectDrawCreate(NULL, &dd, NULL) != DD_OK || dd == NULL) {
        return -2;
    }
    if (dd->SetCooperativeLevel((HWND)hwnd, DDSCL_NORMAL) != DD_OK) {
        return -3;
    }
    DDSURFACEDESC d;
    memset(&d, 0, sizeof(d));
    d.dwSize = sizeof(d);
    d.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    d.dwWidth = (DWORD)w;
    d.dwHeight = (DWORD)h;
    d.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    LPDIRECTDRAWSURFACE surf = NULL;
    if (dd->CreateSurface(&d, &surf, NULL) != DD_OK || surf == NULL) {
        return -4;
    }
    out->dd = dd;
    out->surf = surf;
    out->hwnd = hwnd;
    out->w = w;
    out->h = h;
    return 0;
}

void surface_destroy(DisplaySurface* s)
{
    if (s == NULL) {
        return;
    }
    if (s->surf != NULL) {
        s_surf(s->surf)->Release();
    }
    if (s->dd != NULL) {
        s_dd(s->dd)->Release();
    }
    s->dd = NULL;
    s->surf = NULL;
}

int surface_width(DisplaySurface* s)  { return s != NULL ? s->w : 0; }
int surface_height(DisplaySurface* s) { return s != NULL ? s->h : 0; }

int surface_lock(DisplaySurface* s, void** pixels, int* pitch)
{
    if (s == NULL || s->surf == NULL || pixels == NULL || pitch == NULL) {
        return -1;
    }
    DDSURFACEDESC d;
    memset(&d, 0, sizeof(d));
    d.dwSize = sizeof(d);
    if (s_surf(s->surf)->Lock(NULL, &d, DDLOCK_WAIT | DDLOCK_WRITEONLY,
                              NULL) != DD_OK) {
        return -2;
    }
    *pixels = d.lpSurface;
    *pitch = d.lPitch;
    return 0;
}

void surface_unlock(DisplaySurface* s, void* pixels)
{
    if (s == NULL || s->surf == NULL) {
        return;
    }
    s_surf(s->surf)->Unlock(pixels);
}

#else  /* !LEGORRR_BACKEND_MINIWIN — winapi stub (TODO reference build) */ 

int surface_create(DisplaySurface* out, void* hwnd, int w, int h)
{
    (void)out; (void)hwnd; (void)w; (void)h;
    return -5;
}
void surface_destroy(DisplaySurface* s) { (void)s; }
int surface_width(DisplaySurface* s)  { (void)s; return 0; }
int surface_height(DisplaySurface* s) { (void)s; return 0; }
int surface_lock(DisplaySurface* s, void** pixels, int* pitch)
{
    (void)s; (void)pixels; (void)pitch;
    return -5;
}
void surface_unlock(DisplaySurface* s, void* pixels)
{
    (void)s; (void)pixels;
}

#endif /* LEGORRR_BACKEND_MINIWIN */

/* ---- backend-neutral pixel helpers ------------------------------------- */

void surface_fill(void* px, int pitch, int w, int h, int r, int g, int b)
{
    if (px == NULL) {
        return;
    }
    for (int y = 0; y < h; ++y) {
        unsigned char* row = (unsigned char*)px + (size_t)y * pitch;
        for (int x = 0; x < w; ++x) {
            row[x * 4 + 0] = (unsigned char)r;
            row[x * 4 + 1] = (unsigned char)g;
            row[x * 4 + 2] = (unsigned char)b;
            row[x * 4 + 3] = 0xff;
        }
    }
}

void surface_blit_texture(void* px, int pitch, int w, int h,
                          const TextureSurface* t, int x, int y)
{
    if (px == NULL || t == NULL || t->pixels == NULL) {
        return;
    }
    for (int yy = 0; yy < t->height; ++yy) {
        int dy = y + yy;
        if (dy < 0 || dy >= h) {
            continue;
        }
        unsigned char* row = (unsigned char*)px + (size_t)dy * pitch;
        for (int xx = 0; xx < t->width; ++xx) {
            int dx = x + xx;
            if (dx < 0 || dx >= w) {
                continue;
            }
            uint32_t p = t->pixels[yy * t->width + xx];
            row[dx * 4 + 0] = (unsigned char)(p >> 16); /* R */
            row[dx * 4 + 1] = (unsigned char)(p >> 8);  /* G */
            row[dx * 4 + 2] = (unsigned char)(p);       /* B */
            row[dx * 4 + 3] = (unsigned char)(p >> 24); /* A */
        }
    }
}

void surface_draw_sweep(void* px, int pitch, int w, int h, int frame)
{
    if (px == NULL) {
        return;
    }
    int bar_x = (frame * 4) % w;
    int bar_w = 40;
    for (int y = 0; y < h; ++y) {
        unsigned char* row = (unsigned char*)px + (size_t)y * pitch;
        for (int x = bar_x; x < bar_x + bar_w && x < w; ++x) {
            row[x * 4 + 0] = 0xff; /* R */
            row[x * 4 + 1] = 0x00; /* G */
            row[x * 4 + 2] = 0xff; /* B */
            row[x * 4 + 3] = 0xff; /* A */
        }
    }
}