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
    /* Font colour keys (FUN_0047d9d0 -> FUN_0047e050): an 8bpp face keys
     * BOTH palette entries 0 and 255 as transparent. key_count is 2 for
     * surfaces decoded from an 8bpp palettised BMP, else 0 — the caller
     * falls back to its own heuristic for those. */
    uint32_t  keys[2];
    int32_t   key_count;
    /* The legacy resource-record path applies DDCOLORKEY_SRCBLT to the
     * wrapped surface. Preserve that range in the software surface too. */
    uint32_t  source_key_low;
    uint32_t  source_key_high;
    int32_t   source_key_set;
    uint32_t  resource_flags;
} TextureSurface;

/* Decode a full .bmp file buffer. Windows BITMAPINFOHEADER (size >= 40) and
 * OS/2 BITMAPCOREHEADER (size 12) headers; uncompressed rows only (BI_RGB);
 * depths 1/4/8 (paletted), 16 (555/565), 24, 32. Returns NULL for anything
 * else (including RLE). Caller owns the returned surface. */
TextureSurface* texture_decode(const uint8_t* data, uint32_t size);

/* Portable adapter that opens and decodes a named image into TextureSurface;
 * retail BitmapSurfaceLoad uses renderer-owned Direct3D surface state. */
TextureSurface* image_surface_load(const char* name, uint32_t flags,
                                   uint32_t color_key);

/* Portable resource-backed BMP decoder. Retail CreateBmpSurface takes two
 * arguments and uploads through the Direct3D device. */
TextureSurface* texture_load(const char* name);

/* Portable get-or-load cache adapter. Retail TextureGetOrCreate takes four
 * arguments and returns a three-word texture record. This helper:
 * joins base+file (base may be NULL), case-insensitive linear scan,
 * refcount++ on hit; miss loads via texture_load and registers a copy.
 * NULL on failure. */
TextureSurface* texture_get_or_load(const char* base, const char* file);

/* Portable TextureSurface reference release; retail TextureRelease drops
 * COM interfaces from its resource record. */
void texture_release(TextureSurface* t);

/* Retail TextureRelease record: two independently reference-counted COM
 * surfaces at offsets 0 and 4 on the 32-bit target. This is intentionally
 * distinct from the port's CPU-backed TextureSurface. */
typedef struct TextureReleaseRecord {
    void* surface;
    void* texture;
} TextureReleaseRecord;
void texture_release_record(TextureReleaseRecord* record); /* 0x00474710 */

/* ResourceRecordAddRef's software equivalent of IDirectDrawSurface
 * SetColorKey(DDCKEY_SRCBLT, range). */
void texture_source_color_key_set(TextureSurface* t, uint32_t low,
                                  uint32_t high);

/* Unconditional free (do not use on surfaces still reachable by name). */
void texture_free(TextureSurface* t);

/* Drop every cached surface. */
void texture_cache_clear(void);

/* FNV-1a over the canonical RGBA byte stream (R,G,B,A per pixel, top row
 * first) — deterministic across machines; the validation test compares this
 * against reference hashes produced from the shipped corpus via PIL. */
uint32_t texture_pixel_hash(TextureSurface* t);

/* --- cache-window name pair (2026-10-25) ---------------------------------
 * TextureCacheLookup (FUN_00483030) / TextureCacheInsert (FUN_00483080):
 * the corpus pair works over a caller-supplied cache-entry window, so the
 * port models it as pure window operations (the corpus's twelve-byte
 * entries land as widened slots: {word slot, name pointer, word slot}).
 * Deviations: the port guards the table/count; the corpus strides the
 * window blindly. */

/* Fold-find the entry whose name slot matches; answers the entry address
 * or NULL. */
void* texture_cache_name_slot_find(const void* table, uint32_t count,
                                   const char* name); /* FUN_00483030 */

/* Append {slot_a, key copy through the string duplicate layer
 * (FUN_00477550), slot_b} past the count word and bump it. */
void texture_cache_name_slot_append(void* table, int* count,
                                    const char* name, void* slot_a,
                                    void* slot_b);    /* FUN_00483080 */

#endif /* LEGORRR_TEXTURE_HPP */
