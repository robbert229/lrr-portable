// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — texture loader scaffold (finding 13).
 *
 * Every pixel surface the game draws goes through one create path
 * (FUN_00474050, "BMP -> DDraw/D3D surface") and one name-keyed reuse path
 * (FUN_00482e70, "get-or-load"). The shipped assets are plain Windows .bmp
 * files (BITMAPINFOHEADER size 40, uncompressed, 8-bit paletted + 24-bit;
 * one 4-bit entry), indexed by the same disk-or-WAD layer the resource loader
 * serves (these functions consume resource_open on .bmp names).
 *
 * This scaffold covers the decode + cache halves of that pipeline, folded to
 * a platform-neutral ARGB8888 surface; the device upload (DDSURFACEDESC /
 * DAT_0076ac84 create path) is the renderer milestone and deliberately not
 * here (ADR 0004 seam).
 */
#ifndef LEGORRR_TEXTURE_HPP
#define LEGORRR_TEXTURE_HPP

#include <cstdint>
#include <cstddef>

/* A decoded texture. `pixels` holds width*height 0xAARRGGBB values, row 0 at
 * the top of the image (BMP rows are bottom-up in the file; flipped here). */
typedef struct TextureSurface {
    int32_t   width;
    int32_t   height;
    int32_t   bpp;         /* source BMP depth: 1/4/8/16/24/32 */
    uint32_t* pixels;      /* width*height, 0xAARRGGBB */
    char*     name;        /* registered cache name, else NULL */
    int32_t   refcount;
} TextureSurface;

/* Decode a full .bmp file buffer. Windows BITMAPINFOHEADER (size >= 40) and
 * OS/2 BITMAPCOREHEADER (size 12) headers; uncompressed rows only (BI_RGB);
 * depths 1/4/8 (paletted), 16 (555/565), 24, 32. Returns NULL for anything
 * else (including RLE). Caller owns the returned surface. */
TextureSurface* texture_decode(const uint8_t* data, uint32_t size);

/* Load + decode by name through the resource layer (RESOURCE_AUTO). */
TextureSurface* texture_load(const char* name);

/* Get-or-load with the engine's name-keyed cache (finding 13 §2):
 * joins base+file (base may be NULL), case-insensitive linear scan,
 * refcount++ on hit; miss loads via texture_load and registers a copy.
 * NULL on failure. */
TextureSurface* texture_get_or_load(const char* base, const char* file);

/* Cache-branch release: refcount--, frees + unregisters at 0. For a surface
 * that was never registered, frees immediately. */
void texture_release(TextureSurface* t);

/* Unconditional free (do not use on surfaces still reachable by name). */
void texture_free(TextureSurface* t);

/* Drop every cached surface. */
void texture_cache_clear(void);

/* FNV-1a over the canonical RGBA byte stream (R,G,B,A per pixel, top row
 * first) — deterministic across machines; the validation test compares this
 * against reference hashes produced from the shipped corpus via PIL. */
uint32_t texture_pixel_hash(TextureSurface* t);

#endif /* LEGORRR_TEXTURE_HPP */