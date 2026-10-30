// SPDX-License-Identifier: LGPL-3.0-or-later
#include "platform/platform.h"
#include "surface.hpp"

#include "crt/crt.hpp"
#include "heap.hpp"

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
/* RGBChannelsPack (FUN_0047db90, findings 23/431): the corpus's CONCAT
 * chain drops the three bytes little-endian — argument one in bits 0-7,
 * two in 8-15, three in 16-23, top byte zero. Callers (the palette scan
 * of finding 23) feed the bytes in memory order, so the word is the
 * 0x00BBGGRR triple as stored. */
// FUNCTION: LEGORR 0x0047db90
uint32_t rgb_channels_pack(uint8_t red, uint8_t green, uint8_t blue)
{
    return (uint32_t)red | ((uint32_t)green << 8) | ((uint32_t)blue << 16);
}

/* SurfacePixelRowsFlipWithScratch (FUN_0047D740) — see surface.hpp. The
 * corpus walks the row pairs from the outside in, three stage copies per
 * pair (top row to the scratch, bottom row onto the top, scratch onto the
 * bottom), each stage (row_bytes/2) DWORDs plus (even-rounded row_bytes) & 3
 * trailing bytes, advancing both cursors by the even-rounded row_bytes.
 * The dword/tail split sums to exactly that, so the port copies the whole
 * even span per stage. Deviations: NULL record/pixels return (the corpus
 * strides its surface blindly) and a non-positive row count skips the walk. */
// FUNCTION: LEGORR 0x0047d740
void surface_pixel_rows_flip_with_scratch(SurfaceRecord* rec)
{
    if (rec == NULL || rec->pixels == NULL) {
        return; /* deviation: the corpus strides the surface blindly */
    }
    const int half = rec->row_bytes / 2;          /* corpus's uVar2 */
    const long span = (long)half * 2;             /* even-rounded row_bytes */
    char* const first = (char*)rec->pixels;
    char* const last = first + (ptrdiff_t)(rec->row_count - 1) * span;
    char* scratch = (char*)engine_heap_alloc(span);
    uint32_t lap = 0u;
    if (rec->row_count > 1) { /* deviation: guarded; corpus tests (rows & ~1) */
        char* top = first;
        char* bottom = last;
        for (;;) {
            memcpy(scratch, top, (size_t)span);              /* A -> scratch */
            memcpy(top, bottom, (size_t)span);               /* B -> A */
            memcpy(bottom, scratch, (size_t)span);           /* scratch -> B */
            lap = lap + 1u;
            top = top + span;
            bottom = bottom - span;
            if (lap >= (uint32_t)((uint32_t)rec->row_count >> 1)) {
                break;
            }
        }
    }
    engine_heap_free(scratch);
}

/* Palette8To16BitSurfaceExpand (FUN_0047D520) — see surface.hpp. The corpus
 * reads the depth at +0x54: 8/24/32 do no work and answer 0, 16-bit runs,
 * any other depth answers 1 untouched. The 16-bit walk: for every source
 * byte (a palette index) the entry's three bytes land in their mask's field
 * as ((byte >> (8 - popcount(mask))) << first_set(mask)), OR'd into the
 * stored word — byte 0 rides the +0x58 mask, byte 1 the +0x5c mask, byte 2
 * the +0x60 mask, the entries four bytes apart. Rows then flip. Deviations:
 * the blind record/job/source strides are guarded. */
// FUNCTION: LEGORR 0x0047d520
int surface_palette8_to_16bit_surface_expand(SurfaceRecord* rec,
                                             const SurfaceConvertJob* job)
{
    if (rec == NULL || job == NULL || rec->pixels == NULL) {
        return 1; /* deviation: the corpus strides the record blindly */
    }
    if (rec->depth == 8 || rec->depth == 24 || rec->depth == 32) {
        return 0;
    }
    if (rec->depth != 16) {
        return 1;
    }
    if (job->src == NULL || job->palette == NULL) {
        return 1; /* deviation: the corpus strides the job blindly */
    }
    const uint32_t mask_low = rec->mask_low;
    const uint32_t mask_mid = rec->mask_mid;
    const uint32_t mask_hi = rec->mask_hi;
    const uint32_t ffs_low = find_first_set_bit32(mask_low);
    const uint32_t ffs_mid = find_first_set_bit32(mask_mid);
    const uint32_t ffs_hi = find_first_set_bit32(mask_hi);
    const uint32_t shr_low = (uint32_t)(8 - pop_count32(mask_low)) & 0x1fu;
    const uint32_t shr_mid = (uint32_t)(8 - pop_count32(mask_mid)) & 0x1fu;
    const uint32_t shr_hi = (uint32_t)(8 - pop_count32(mask_hi)) & 0x1fu;
    const char* src = (const char*)job->src;
    const char* palette = (const char*)job->palette;
    uint16_t* dst = (uint16_t*)rec->pixels;
    for (int y = 0; y < job->height; ++y) {
        for (int x = 0; x < job->width; ++x) {
            const uint32_t index = (uint8_t)*src++;
            const char* entry = palette + index * 4u;
            const uint16_t word = (uint16_t)(
                ((uint32_t)((uint8_t)entry[0] >> shr_low) << (ffs_low & 0x1fu))
                | ((uint32_t)((uint8_t)entry[1] >> shr_mid)
                   << (ffs_mid & 0x1fu))
                | ((uint32_t)((uint8_t)entry[2] >> shr_hi) << (ffs_hi & 0x1fu)));
            *dst++ = word;
        }
        src += job->src_stride - job->width;
        dst += (size_t)((rec->row_bytes >> 1) - job->width);
    }
    surface_pixel_rows_flip_with_scratch(rec);
    return 1;
}

/* Bitmap24BitSurfaceConvert (FUN_0047D800) — see surface.hpp. The 24-bit
 * sibling: same depth verdict, same field placement per mask, but the three
 * source bytes per pixel ride the masks REVERSED — byte 0 lands in the
 * +0x60 (high) field, byte 1 in +0x5c, byte 2 in +0x58 — and rows advance
 * by the source stride over three-byte pixels. Deviations and the flip tail
 * ride the palette walk's notes. */
// FUNCTION: LEGORR 0x0047d800
int surface_bitmap24_bit_surface_convert(SurfaceRecord* rec,
                                         const SurfaceConvertJob* job)
{
    if (rec == NULL || job == NULL || rec->pixels == NULL) {
        return 1; /* deviation: the corpus strides the record blindly */
    }
    if (rec->depth == 8 || rec->depth == 24 || rec->depth == 32) {
        return 0;
    }
    if (rec->depth != 16) {
        return 1;
    }
    if (job->src == NULL) {
        return 1; /* deviation: the corpus strides the job blindly */
    }
    const uint32_t mask_low = rec->mask_low;
    const uint32_t mask_mid = rec->mask_mid;
    const uint32_t mask_hi = rec->mask_hi;
    const uint32_t ffs_low = find_first_set_bit32(mask_low);
    const uint32_t ffs_mid = find_first_set_bit32(mask_mid);
    const uint32_t ffs_hi = find_first_set_bit32(mask_hi);
    const uint32_t shr_low = (uint32_t)(8 - pop_count32(mask_low)) & 0x1fu;
    const uint32_t shr_mid = (uint32_t)(8 - pop_count32(mask_mid)) & 0x1fu;
    const uint32_t shr_hi = (uint32_t)(8 - pop_count32(mask_hi)) & 0x1fu;
    const char* src = (const char*)job->src;
    uint16_t* dst = (uint16_t*)rec->pixels;
    for (int y = 0; y < job->height; ++y) {
        for (int x = 0; x < job->width; ++x) {
            const uint32_t b0 = (uint8_t)src[0];
            const uint32_t b1 = (uint8_t)src[1];
            const uint32_t b2 = (uint8_t)src[2];
            src += 3;
            const uint16_t word = (uint16_t)(
                ((b0 >> shr_hi) << (ffs_hi & 0x1fu))
                | ((b1 >> shr_mid) << (ffs_mid & 0x1fu))
                | ((b2 >> shr_low) << (ffs_low & 0x1fu)));
            *dst++ = word;
        }
        src += job->src_stride - 3 * job->width;
        dst += (size_t)((rec->row_bytes >> 1) - job->width);
    }
    surface_pixel_rows_flip_with_scratch(rec);
    return 1;
}
