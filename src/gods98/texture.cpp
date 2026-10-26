// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resource.hpp"
#include "string.hpp"
#include "texture.hpp"

#define TEX_CACHE_MAX 4096

typedef struct CacheEntry {
    char*            name;
    TextureSurface*  surf;
} CacheEntry;

static CacheEntry s_cache[TEX_CACHE_MAX];
static int        s_cache_count = 0;

/* ------------------------------------------------------------------ *
 *  BMP decode (finding 13; the shipped corpus is BITMAPINFOHEADER 40,
 *  BI_RGB, 8-bit paletted + 24-bit).
 * ------------------------------------------------------------------ */

static uint32_t t_le32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint32_t t_le16(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8);
}

static int t_lower(int c) { return c >= 'a' && c <= 'z'; }

static int t_strcasecmp(const char* a, const char* b)
{
    for (;;) {
        int ca = (unsigned char)*a++;
        int cb = (unsigned char)*b++;
        if (t_lower(ca)) ca -= 0x20;
        if (t_lower(cb)) cb -= 0x20;
        if (ca != cb) {
            return ca - cb;
        }
        if (ca == 0) {
            return 0;
        }
    }
}

TextureSurface* texture_decode(const uint8_t* data, uint32_t size)
{
    if (size < 26 || data[0] != 'B' || data[1] != 'M') {
        return NULL;
    }

    uint32_t data_off = t_le32(data + 10);
    uint32_t hdr_size = t_le32(data + 14);

    int32_t  w, h;
    int      bpp;
    uint32_t num_colors;
    uint32_t pal_start;
    uint32_t pal_stride;
    int      top_down = 0;

    if (hdr_size == 12) {
        /* OS/2 BITMAPCOREHEADER (finding 01's "BM6" bmps). */
        w  = (int32_t)t_le16(data + 18);
        h  = (int32_t)t_le16(data + 22);
        bpp = (int)t_le16(data + 24);
        num_colors = (uint32_t)(1u << (bpp <= 24 ? (bpp == 24 ? 0 : bpp) : 0));
        pal_start  = 26;
        pal_stride = 3; /* BGR */
    } else if (hdr_size >= 40) {
        w  = (int32_t)t_le32(data + 18);
        h  = (int32_t)t_le32(data + 22);
        bpp = (int)t_le16(data + 28);
        uint32_t comp = t_le32(data + 30);
        if (comp != 0) {
            return NULL; /* BI_RGB only in the shipped corpus */
        }
        if (h < 0) {
            top_down = 1;
            h = -h;
        }
        num_colors = t_le32(data + 46);
        if (num_colors == 0) {
            num_colors = (bpp <= 8) ? (1u << bpp) : 0;
        }
        pal_start  = 54;
        pal_stride = 4; /* BGR(0) */
    } else {
        return NULL; /* unknown header size */
    }

    if (w <= 0 || h <= 0 || bpp < 1 || bpp > 32
        || num_colors > 256u || data_off > size) {
        return NULL;
    }

    /* Palette entries may be fewer than the implied count (files sometimes
     * store a truncated palette); clamp so only in-file entries are read. */
    uint64_t avail_entries = ((uint64_t)data_off - pal_start) / pal_stride;
    if (avail_entries < num_colors) {
        num_colors = (uint32_t)avail_entries;
    }

    /* Palette must sit inside the file, before the pixel data. */
    uint64_t pal_end = (uint64_t)pal_start + (uint64_t)num_colors * pal_stride;
    if (pal_end > data_off) {
        return NULL;
    }

    /* Row stride: each row padded to a multiple of 4 bytes. */
    uint32_t row_bits = (uint32_t)w * (uint32_t)bpp;
    uint32_t row_stride = ((row_bits + 31u) / 32u) * 4u;
    uint64_t pixel_bytes = (uint64_t)row_stride * (uint64_t)h;
    if (pixel_bytes > (uint64_t)size - data_off) {
        return NULL;
    }

    /* Palette table, converted to 24-bit RGB. Zero-filled: entries past the
     * table read as black, mirroring the zero-padded 0x400 palette copy in
     * the corpus loader (FUN_00489c30). */
    uint8_t r[256] = { 0 }, g[256] = { 0 }, b[256] = { 0 };
    for (uint32_t i = 0; i < num_colors; ++i) {
        const uint8_t* e = data + pal_start + i * pal_stride;
        if (e + pal_stride > data + size) {
            break; /* palette table ends early; rest reads as black */
        }
        b[i] = e[0];
        g[i] = e[1];
        r[i] = e[2];
    }

    TextureSurface* t = (TextureSurface*)calloc(1, sizeof(TextureSurface));
    if (t == NULL) {
        return NULL;
    }
    t->width  = w;
    t->height = h;
    t->bpp    = bpp;
    if (bpp == 8) {
        /* Corpus font keys: FUN_0047d9d0 hands FUN_0047e050 palette[0]
         * (bytes 0..2 of the copied palette) and palette[255] (bytes
         * 0x3fc..0x3fe) as the two colour keys of every 8bpp face. */
        for (int i = 0; i < 2; ++i) {
            uint32_t idx = (i == 0) ? 0u : 255u;
            t->keys[i] = 0xff000000u | ((uint32_t)r[idx] << 16)
                       | ((uint32_t)g[idx] << 8) | (uint32_t)b[idx];
        }
        t->key_count = 2;
    }
    t->pixels = (uint32_t*)malloc((size_t)w * (size_t)h * 4);
    if (t->pixels == NULL) {
        free(t);
        return NULL;
    }

    uint32_t* out = t->pixels;
    for (int ty = 0; ty < h; ++ty) {
        int fy = top_down ? ty : (h - 1 - ty); /* BMP rows are bottom-up */
        const uint8_t* row = data + data_off + (uint64_t)fy * row_stride;
        for (int x = 0; x < w; ++x) {
            int pr = 0, pg = 0, pb = 0, pa = 255;
            if (bpp == 8) {
                uint32_t idx = row[x];
                if (idx < num_colors) {
                    pr = r[idx]; pg = g[idx]; pb = b[idx];
                }
            } else if (bpp == 4) {
                uint32_t idx = (row[x / 2] >> (x & 1 ? 0 : 4)) & 0xf;
                if (idx < num_colors) {
                    pr = r[idx]; pg = g[idx]; pb = b[idx];
                }
            } else if (bpp == 1) {
                uint32_t idx = (row[x / 8] >> (7 - (x & 7))) & 1;
                if (idx < num_colors) {
                    pr = r[idx]; pg = g[idx]; pb = b[idx];
                }
            } else if (bpp == 16) {
                uint32_t v = t_le16(row + x * 2);
                if (v & 0x8000u) { /* 5-5-5 */
                    pr = (v >> 7 & 0x1f) * 255 / 31;
                    pg = (v >> 2 & 0x1f) * 255 / 31;
                    pb = (v & 0x1f) * 255 / 31;
                } else {           /* 5-6-5 */
                    pr = (v >> 11 & 0x1f) * 255 / 31;
                    pg = (v >> 5 & 0x3f) * 255 / 63;
                    pb = (v & 0x1f) * 255 / 31;
                }
            } else if (bpp == 24) {
                const uint8_t* p = row + x * 3;
                pb = p[0]; pg = p[1]; pr = p[2];
            } else if (bpp == 32) {
                const uint8_t* p = row + x * 4;
                pb = p[0]; pg = p[1]; pr = p[2]; pa = p[3];
            } else {
                free(t->pixels);
                free(t);
                return NULL;
            }
            *out++ = ((uint32_t)pa << 24) | ((uint32_t)pr << 16)
                   | ((uint32_t)pg << 8) | (uint32_t)pb;
        }
    }
    return t;
}

/* ------------------------------------------------------------------ *
 *  Load + get-or-load cache (finding 13 §2).
 * ------------------------------------------------------------------ */

TextureSurface* texture_load(const char* name)
{
    Resource* r = resource_open(name, RESOURCE_AUTO);
    if (r == NULL) {
        return NULL;
    }
    if (resource_read(r) < 0) {
        resource_free(r);
        return NULL;
    }
    TextureSurface* t = texture_decode((const uint8_t*)r->data,
                                       (uint32_t)r->size);
    resource_free(r);
    return t;
}

TextureSurface* texture_get_or_load(const char* base, const char* file)
{
    char joined[1024];
    const char* name;
    if (base != NULL && base[0] != '\0') {
        if (strlen(base) + strlen(file) + 1 >= sizeof(joined)) {
            return NULL;
        }
        snprintf(joined, sizeof(joined), "%s%s", base, file);
        name = joined;
    } else {
        name = file;
    }

    for (int i = 0; i < s_cache_count; ++i) {
        if (t_strcasecmp(s_cache[i].name, name) == 0) {
            TextureSurface* t = s_cache[i].surf;
            ++t->refcount;
            return t;
        }
    }

    TextureSurface* t = texture_load(name);
    if (t == NULL) {
        return NULL;
    }
    if (s_cache_count < TEX_CACHE_MAX) {
        t->name = (char*)malloc(strlen(name) + 1);
        if (t->name == NULL) {
            texture_free(t);
            return NULL;
        }
        strcpy(t->name, name);
        t->refcount = 1;
        s_cache[s_cache_count].name = t->name;
        s_cache[s_cache_count].surf = t;
        ++s_cache_count;
    } else {
        t->refcount = 1; /* uncached fallback: still usable */
    }
    return t;
}

void texture_release(TextureSurface* t)
{
    if (t == NULL) {
        return;
    }
    if (t->name == NULL) {
        texture_free(t);
        return;
    }
    if (--t->refcount > 0) {
        return;
    }
    for (int i = 0; i < s_cache_count; ++i) {
        if (s_cache[i].surf == t) {
            s_cache[i] = s_cache[s_cache_count - 1];
            --s_cache_count;
            break;
        }
    }
    texture_free(t);
}

void texture_free(TextureSurface* t)
{
    if (t == NULL) {
        return;
    }
    free(t->pixels);
    free(t->name);
    free(t);
}

void texture_cache_clear(void)
{
    for (int i = 0; i < s_cache_count; ++i) {
        texture_free(s_cache[i].surf);
    }
    s_cache_count = 0;
}

uint32_t texture_pixel_hash(TextureSurface* t)
{
    uint32_t h = 2166136261u;
    if (t == NULL || t->pixels == NULL) {
        return h;
    }
    int n = t->width * t->height;
    for (int i = 0; i < n; ++i) {
        uint32_t p = t->pixels[i];
        uint8_t bytes[4] = {
            (uint8_t)(p >> 16), /* R */
            (uint8_t)(p >> 8),  /* G */
            (uint8_t)(p),       /* B */
            (uint8_t)(p >> 24), /* A */
        };
        for (int k = 0; k < 4; ++k) {
            h ^= bytes[k];
            h *= 16777619u;
        }
    }
    return h;
}

/* --- the cache-window name pair (see texture.hpp) ------------------------- */
/* Both corpus halves work over a caller-supplied entry window; entries
 * ride as widened slots {word, name, word}. Deviation: the port guards
 * the table, the count, and the name pointer (the corpus strides them). */
void* texture_cache_name_slot_find(const void* table, uint32_t count,
                                   const char* name)
{
    if (table == NULL || count == 0u) {
        return NULL; /* deviation: the corpus answers 0 only for count 0 */
    }
    const long field = (long)sizeof(void*);
    for (uint32_t i = 0; i < count; ++i) {
        char* entry_name = NULL;
        __builtin_memcpy(&entry_name,
                         (const char*)table + (long)i * 3 * field + field,
                         sizeof entry_name);
        if (entry_name != NULL && t_strcasecmp(name, entry_name) == 0) {
            return (void*)((const char*)table + (long)i * 3 * field);
        }
    }
    return NULL;
}

void texture_cache_name_slot_append(void* table, int* count,
                                    const char* name, void* slot_a,
                                    void* slot_b)
{
    const long field = (long)sizeof(void*);
    char* copy = string_duplicate(name); /* FUN_00477550 */
    char* entry = (char*)table + (long)*count * 3 * field;
    __builtin_memcpy(entry, &slot_a, sizeof slot_a);
    __builtin_memcpy(entry + field, &copy, sizeof copy);
    __builtin_memcpy(entry + 2 * field, &slot_b, sizeof slot_b);
    *count += 1;
}