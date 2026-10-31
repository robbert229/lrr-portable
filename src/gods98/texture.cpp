// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resource.hpp"
#include "scene_resources.hpp"
#include "string.hpp"
#include "texture.hpp"

#define TEX_CACHE_MAX 4096
#define TEXTURE_RESOURCE_NAME_MAX 260

typedef struct CacheEntry {
    char*            name;
    TextureSurface*  surf;
} CacheEntry;

static CacheEntry s_cache[TEX_CACHE_MAX];
static int        s_cache_count = 0;
static CacheEntry s_shared_cache[TEX_CACHE_MAX];
static int        s_shared_cache_count = 0;

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

typedef struct TextureBitmapLayout {
    int32_t width, height;
    int bpp;
    uint32_t data_off, row_stride, num_colors;
    int top_down;
    uint8_t r[256], g[256], b[256];
} TextureBitmapLayout;

/* IndexedBitmapPaletteDataPrepare: read bitmap dimensions/format/scanline
 * layout and normalize the optional indexed palette for the software path.
 * The port validates file bounds and supports OS/2 headers as deviations. */
// FUNCTION: LEGORR 0x00489c30
static int texture_bitmap_layout_prepare(const uint8_t* data, uint32_t size,
                                         TextureBitmapLayout* layout)
{
    if (layout == NULL || size < 26 || data[0] != 'B' || data[1] != 'M') {
        return 0;
    }

    memset(layout, 0, sizeof(*layout));

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
        return 0; /* unknown header size */
    }

    if (w <= 0 || h <= 0 || bpp < 1 || bpp > 32
        || num_colors > 256u || data_off > size) {
        return 0;
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
        return 0;
    }

    /* Row stride: each row padded to a multiple of 4 bytes. */
    uint32_t row_bits = (uint32_t)w * (uint32_t)bpp;
    uint32_t row_stride = ((row_bits + 31u) / 32u) * 4u;
    uint64_t pixel_bytes = (uint64_t)row_stride * (uint64_t)h;
    if (pixel_bytes > (uint64_t)size - data_off) {
        return 0;
    }

    /* Palette table, converted to 24-bit RGB. Zero-filled: entries past the
     * table read as black, mirroring the zero-padded 0x400 palette copy in
     * the corpus loader (FUN_00489c30). */
    for (uint32_t i = 0; i < num_colors; ++i) {
        const uint8_t* e = data + pal_start + i * pal_stride;
        if (e + pal_stride > data + size) {
            break; /* palette table ends early; rest reads as black */
        }
        layout->b[i] = e[0];
        layout->g[i] = e[1];
        layout->r[i] = e[2];
    }

    layout->width = w;
    layout->height = h;
    layout->bpp = bpp;
    layout->data_off = data_off;
    layout->row_stride = row_stride;
    layout->num_colors = num_colors;
    layout->top_down = top_down;
    return 1;
}

/* Decode a full BMP buffer into the port's software surface. The reference
 * BitmapSurfaceLoad entry at 0x0047d9d0 also owns resource lookup. */
TextureSurface* texture_decode(const uint8_t* data, uint32_t size)
{
    TextureBitmapLayout layout;
    if (!texture_bitmap_layout_prepare(data, size, &layout)) {
        return NULL;
    }
    const int32_t w = layout.width;
    const int32_t h = layout.height;
    const int bpp = layout.bpp;
    const uint32_t data_off = layout.data_off;
    const uint32_t row_stride = layout.row_stride;
    const uint32_t num_colors = layout.num_colors;
    const int top_down = layout.top_down;

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
            t->keys[i] = 0xff000000u | ((uint32_t)layout.r[idx] << 16)
                       | ((uint32_t)layout.g[idx] << 8)
                       | (uint32_t)layout.b[idx];
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
                    pr = layout.r[idx]; pg = layout.g[idx]; pb = layout.b[idx];
                }
            } else if (bpp == 4) {
                uint32_t idx = (row[x / 2] >> (x & 1 ? 0 : 4)) & 0xf;
                if (idx < num_colors) {
                    pr = layout.r[idx]; pg = layout.g[idx]; pb = layout.b[idx];
                }
            } else if (bpp == 1) {
                uint32_t idx = (row[x / 8] >> (7 - (x & 7))) & 1;
                if (idx < num_colors) {
                    pr = layout.r[idx]; pg = layout.g[idx]; pb = layout.b[idx];
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

/* Portable BitmapSurfaceLoad adapter. The retail 0x0047d9d0 entry takes one
 * resource name and uses the engine's Direct3D texture interface and surface
 * globals; this software-surface helper has extra flags and returns a
 * TextureSurface pointer, so it is not the retail ABI implementation. */
TextureSurface* image_surface_load(const char* name, uint32_t flags,
                                   uint32_t color_key)
{
    Resource* resource;
    TextureSurface* surface;
    /* The retail entry receives only the resource name; these two values are
       retained by the portable adapter for callers with a DDraw-style API. */
    (void)flags;
    (void)color_key;
    if (name == NULL) return NULL;
    resource = resource_open(name, RESOURCE_AUTO);
    if (resource == NULL) return NULL;
    if (resource_read(resource) < 0) {
        resource_free(resource);
        return NULL;
    }
    surface = texture_decode((const uint8_t*)resource->data,
                             (uint32_t)resource->size);
    resource_free(resource);
    return surface;
}

/* ------------------------------------------------------------------ *
 *  Load + get-or-load cache (finding 13 §2).
 * ------------------------------------------------------------------ */

/* Port adapter for CreateBmpSurface. Retail uses the Direct3D device and
 * global surfaces to create/upload a COM texture; this helper decodes BMP
 * bytes into a software TextureSurface. */
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

/* TextureGetOrCreate: name-keyed hit/refcount, then secondary shared-root
 * lookup and load on a primary miss. */
/* Port adapter for TextureGetOrCreate. Retail takes four arguments and
 * returns a three-dword texture record; this software helper takes a base
 * and file name and returns a TextureSurface. */
TextureSurface* texture_get_or_load(const char* base, const char* file)
{
    char joined[TEXTURE_RESOURCE_NAME_MAX];
    char shared_name[TEXTURE_RESOURCE_NAME_MAX];
    const char* name;
    const char* shared_root;
    int shared_load = 0;
    int i;
    TextureSurface* t;
    size_t name_length;
    if (base != NULL && base[0] != '\0') {
        if (file == NULL) return NULL;
        if (strlen(base) + strlen(file) + 1 > sizeof(joined)) {
            return NULL;
        }
        memcpy(joined, base, strlen(base));
        memcpy(joined + strlen(base), file, strlen(file) + 1);
        name = joined;
    } else {
        name = file;
    }

    if (name == NULL) return NULL;
    for (i = 0; i < s_cache_count; ++i) {
        if (strcasecmp_lite(s_cache[i].name, name) == 0) {
            t = s_cache[i].surf;
            ++t->refcount;
            return t;
        }
    }

    t = texture_load(name);
    shared_root = lego::shared_texture_root_get();
    if (t == NULL && shared_root != NULL && shared_root[0] != '\0') {
        name_length = strlen(name);
        if (strlen(shared_root) + name_length < sizeof(shared_name)) {
            memcpy(shared_name, shared_root, strlen(shared_root));
            memcpy(shared_name + strlen(shared_root), name, name_length + 1);
            name = shared_name;
            for (i = 0; i < s_shared_cache_count; ++i) {
                if (strcasecmp_lite(s_shared_cache[i].name, name) == 0) {
                    t = s_shared_cache[i].surf;
                    ++t->refcount;
                    return t;
                }
            }
            t = texture_load(name);
            shared_load = 1;
        }
    }
    if (t == NULL) {
        return NULL;
    }
    if ((!shared_load && s_cache_count < TEX_CACHE_MAX) ||
        (shared_load && s_shared_cache_count < TEX_CACHE_MAX)) {
#if defined(_MSC_VER)
        t->name = _strdup(name);
#else
        t->name = (char*)malloc(strlen(name) + 1);
#endif
        if (t->name == NULL) {
            texture_free(t);
            return NULL;
        }
#if !defined(_MSC_VER)
        strcpy(t->name, name);
#endif
        t->refcount = 1;
        if (shared_load) {
            s_shared_cache[s_shared_cache_count].name = t->name;
            s_shared_cache[s_shared_cache_count].surf = t;
            ++s_shared_cache_count;
        } else {
            s_cache[s_cache_count].name = t->name;
            s_cache[s_cache_count].surf = t;
            ++s_cache_count;
        }
    } else {
        t->refcount = 1; /* uncached fallback: still usable */
    }
    return t;
}

/* Port release for TextureSurface. Retail releases the two COM interfaces
 * held in its resource record; this frees/refcounts the software surface. */
void texture_release(TextureSurface* t)
{
    int i;
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
    for (i = 0; i < s_cache_count; ++i) {
        if (s_cache[i].surf == t) {
            s_cache[i] = s_cache[s_cache_count - 1];
            --s_cache_count;
            break;
        }
    }
    for (i = 0; i < s_shared_cache_count; ++i) {
        if (s_shared_cache[i].surf == t) {
            s_shared_cache[i] = s_shared_cache[s_shared_cache_count - 1];
            --s_shared_cache_count;
            break;
        }
    }
    texture_free(t);
}

void texture_source_color_key_set(TextureSurface* t, uint32_t low,
                                  uint32_t high)
{
    if (t == NULL) return;
    t->source_key_low = low;
    t->source_key_high = high;
    t->source_key_set = 1;
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
    int i;
    for (i = 0; i < s_cache_count; ++i) {
        texture_free(s_cache[i].surf);
    }
    s_cache_count = 0;
    for (i = 0; i < s_shared_cache_count; ++i) {
        texture_free(s_shared_cache[i].surf);
    }
    s_shared_cache_count = 0;
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
 * ride as widened slots {word, name, word}. */
// FUNCTION: LEGORR 0x00483030
void* texture_cache_name_slot_find(const void* table, uint32_t count,
                                   const char* name)
{
    /* Corpus 0x483030 gates the unsigned count once, then walks name fields
     * at 0x0c-byte strides; its bottom comparison is unsigned (jb). */
    const long field = (long)sizeof(void*);
    for (uint32_t i = 0u; i < count; ++i) {
        const char* entry_name = *(const char* const*)
            ((const char*)table + (long)i * 3 * field + field);
        if (strcasecmp_lite(name, entry_name) == 0) {
            return (void*)((const char*)table + (long)i * 3 * field);
        }
    }
    return NULL;
}

// FUNCTION: LEGORR 0x00483080
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
