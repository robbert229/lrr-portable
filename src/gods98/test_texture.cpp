// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_texture — validation for src/gods98/texture.cpp.
 *
 * Synthetic BMPs (info + OS/2 core headers, 1/4/8/24/32-bit, RLE reject,
 * truncation reject) always run. With `--real <dir>` the shipped wads are
 * mounted and five entries are decoded and compared against reference hashes
 * produced pixel-exact from the corpus via Pillow (finding 13) — the C
 * decoder is byte-for-byte equivalent to a PIL decode for those images.
 *
 * Wire-up: meson test 'texture' (suite 'texture'); real-wad checks via the
 * `test_wads_dir` meson option.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resource.hpp"
#include "texture.hpp"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static uint32_t px(int r, int g, int b, int a)
{
    return ((uint32_t)a << 24) | ((uint32_t)r << 16)
         | ((uint32_t)g << 8) | (uint32_t)b;
}

/* ------------------------------------------------------------------ *
 *  Minimal BMP builder (finding 01/13 byte layout).
 * ------------------------------------------------------------------ */

static size_t bmp_row_stride(uint32_t w, int bpp)
{
    return ((w * (uint32_t)bpp + 31u) / 32u) * 4u;
}

/* core=1 -> OS/2 BITMAPCOREHEADER (12 B), else BITMAPINFOHEADER (40 B).
 * palette: nc entries x (core?3:4) bytes, BGR order. rows: h x row_stride. */
static uint8_t* build_bmp(uint32_t w, uint32_t h, int bpp, int core,
                          const uint8_t* palette, uint32_t nc,
                          const uint8_t* rows, size_t* out_size)
{
    (void)h;
    size_t row_stride = bmp_row_stride(w, bpp);
    size_t pal_stride = core ? 3 : 4;
    size_t hdr_size   = core ? 12 : 40;
    size_t data_off   = 14 + hdr_size + (size_t)nc * pal_stride;
    size_t total      = data_off + ((size_t)h) * row_stride;
    uint8_t* b = (uint8_t*)calloc(1, total);
    if (b == NULL) {
        return NULL;
    }
    b[0] = 'B'; b[1] = 'M';
    b[2] = (uint8_t)(total & 0xff);
    b[3] = (uint8_t)((total >> 8) & 0xff);
    b[4] = (uint8_t)((total >> 16) & 0xff);
    b[5] = (uint8_t)((total >> 24) & 0xff);
    b[10] = (uint8_t)(data_off & 0xff);
    b[11] = (uint8_t)((data_off >> 8) & 0xff);
    b[12] = (uint8_t)((data_off >> 16) & 0xff);
    b[13] = (uint8_t)((data_off >> 24) & 0xff);
    if (core) {
        b[14] = 12; b[15] = 0; b[16] = 0; b[17] = 0;
        b[18] = (uint8_t)(w & 0xff); b[19] = (uint8_t)((w >> 8) & 0xff);
        b[20] = (uint8_t)(h & 0xff); b[21] = (uint8_t)((h >> 8) & 0xff);
        b[22] = 1; b[23] = 0;           /* planes */
        b[24] = (uint8_t)bpp; b[25] = 0;
    } else {
        uint32_t hs = 40;
        memcpy(b + 14, &hs, 4);
        memcpy(b + 18, &w, 4);
        memcpy(b + 22, &h, 4);
        b[26] = 1; b[27] = 0;           /* planes */
        b[28] = (uint8_t)bpp; b[29] = 0;
        /* comp = 0, image size = 0: fine for BI_RGB */
        memcpy(b + 46, &nc, 4);         /* colors used */
    }
    size_t palette_at = 14 + hdr_size;
    for (uint32_t i = 0; i < nc; ++i) {
        memcpy(b + palette_at + i * pal_stride, palette + i * 3, 3);
        /* 4th byte (info) stays zero */
    }
    memcpy(b + data_off, rows, ((size_t)h) * row_stride);
    *out_size = total;
    return b;
}

static void test_synthetic(void)
{
    /* 8-bit 2x2, info header, 4-color palette. file row0=bottom [0,1],
     * row1=top [2,3]; decode flips to top-first. */
    uint8_t pal8[4 * 3] = {
        30, 20, 10,   /* idx 0 -> r10 g20 b30 */
        60, 50, 40,   /* idx 1 -> r40 g50 b60 */
        90, 80, 70,   /* idx 2 -> r70 g80 b90 */
        120, 110, 100 /* idx 3 -> r100 g110 b120 */
    };
    uint8_t rows8[2 * 4] = {
        0, 1, 0, 0,   /* bottom */
        2, 3, 0, 0    /* top */
    };
    size_t sz = 0;
    uint8_t* b = build_bmp(2, 2, 8, 0, pal8, 4, rows8, &sz);
    CHECK(b != NULL, "syn: build 8-bit");
    TextureSurface* t = texture_decode(b, (uint32_t)sz);
    CHECK(t != NULL && t->width == 2 && t->height == 2 && t->bpp == 8,
          "syn: 8-bit dims");
    if (t != NULL) {
        CHECK(t->pixels[0] == px(70, 80, 90, 255), "syn: 8-bit px[0] (top-left)");
        CHECK(t->pixels[1] == px(100, 110, 120, 255), "syn: 8-bit px[1]");
        CHECK(t->pixels[2] == px(10, 20, 30, 255), "syn: 8-bit px[2] (bottom-left)");
        CHECK(t->pixels[3] == px(40, 50, 60, 255), "syn: 8-bit px[3]");
        /* corpus font keys ride on the surface (FUN_0047d9d0 -> e050):
         * palette entries 0 and 255; a short table zero-pads to black */
        CHECK(t->key_count == 2, "syn: 8-bit key_count");
        CHECK(t->keys[0] == px(10, 20, 30, 255), "syn: key[0] = pal[0]");
        CHECK(t->keys[1] == px(0, 0, 0, 255), "syn: key[1] zero-padded");
        texture_free(t);
    }
    free(b);

    /* 24-bit 3x2 with padding (stride 12). Row 0 = bottom (30+x), row 1 =
     * top (40+x); decode flips so pixels[0..2] = top row. */
    uint8_t rows24[2 * 12];
    memset(rows24, 0, sizeof(rows24));
    for (int r = 0; r < 2; ++r) {
        for (int i = 0; i < 3; ++i) {
            uint8_t* p = rows24 + r * 12 + i * 3;
            p[0] = (uint8_t)(10 + i + 10 * r); /* B */
            p[1] = (uint8_t)(20 + i + 10 * r); /* G */
            p[2] = (uint8_t)(30 + i + 10 * r); /* R */
        }
    }
    b = build_bmp(3, 2, 24, 0, NULL, 0, rows24, &sz);
    t = texture_decode(b, (uint32_t)sz);
    CHECK(t != NULL && t->width == 3 && t->height == 2, "syn: 24-bit dims");
    if (t != NULL) {
        CHECK(t->pixels[0] == px(40, 30, 20, 255), "syn: 24-bit px[0] (top)");
        CHECK(t->pixels[1] == px(41, 31, 21, 255), "syn: 24-bit px[1]");
        CHECK(t->pixels[3] == px(30, 20, 10, 255), "syn: 24-bit px[3] (bottom)");
        CHECK(t->key_count == 0, "syn: 24-bit no keys");
        texture_free(t);
    }
    free(b);

    /* 8-bit 1x1 with a full 256-entry palette: pal[255] is the second key
     * (the shipped Menu_Font* convention — palette-255 pure red). */
    uint8_t pal256[256 * 3];
    memset(pal256, 0, sizeof(pal256));
    pal256[0] = 5; pal256[1] = 6; pal256[2] = 7;           /* pal[0] */
    pal256[255 * 3 + 2] = 255;                             /* pal[255] */
    uint8_t rowsk[4] = { 255, 0, 0, 0 };
    b = build_bmp(1, 1, 8, 0, pal256, 256, rowsk, &sz);
    t = texture_decode(b, (uint32_t)sz);
    CHECK(t != NULL, "syn: keyed bmp");
    if (t != NULL) {
        CHECK(t->key_count == 2, "syn: full-pal key_count");
        CHECK(t->keys[0] == px(7, 6, 5, 255), "syn: key[0] full-pal");
        CHECK(t->keys[1] == px(255, 0, 0, 255), "syn: key[1] = pal[255]");
        CHECK(t->pixels[0] == px(255, 0, 0, 255), "syn: keyed px");
        texture_free(t);
    }
    free(b);

    /* 32-bit 2x1 with alpha. */
    uint8_t rows32[8] = { 5, 10, 15, 200,   6, 11, 16, 100 };
    b = build_bmp(2, 1, 32, 0, NULL, 0, rows32, &sz);
    t = texture_decode(b, (uint32_t)sz);
    CHECK(t != NULL && t->bpp == 32 && t->width == 2, "syn: 32-bit dims");
    if (t != NULL) {
        CHECK(t->pixels[0] == px(15, 10, 5, 200), "syn: 32-bit px[0] alpha");
        CHECK(t->pixels[1] == px(16, 11, 6, 100), "syn: 32-bit px[1] alpha");
        texture_free(t);
    }
    free(b);

    /* OS/2 core header 8-bit 2x1. */
    uint8_t palcore[2 * 3] = { 7, 8, 9, 17, 18, 19 }; /* bgr */
    uint8_t rowscore[4] = { 0, 1, 0, 0 };
    b = build_bmp(2, 1, 8, 1, palcore, 2, rowscore, &sz);
    t = texture_decode(b, (uint32_t)sz);
    CHECK(t != NULL && t->width == 2 && t->bpp == 8, "syn: core dims");
    if (t != NULL) {
        CHECK(t->pixels[0] == px(9, 8, 7, 255), "syn: core px[0]");
        CHECK(t->pixels[1] == px(19, 18, 17, 255), "syn: core px[1]");
        texture_free(t);
    }
    free(b);

    /* RLE compression must be rejected. */
    b = build_bmp(2, 2, 8, 0, pal8, 4, rows8, &sz);
    b[30] = 1; /* BI_RLE8 */
    CHECK(texture_decode(b, (uint32_t)sz) == NULL, "syn: RLE rejected");
    free(b);

    /* Truncated buffer must be rejected. */
    b = build_bmp(2, 2, 8, 0, pal8, 4, rows8, &sz);
    CHECK(texture_decode(b, 14) == NULL, "syn: truncated rejected");
    free(b);
}

/* ------------------------------------------------------------------ *
 *  Real-wad validation (--real <dir>).
 * ------------------------------------------------------------------ */

typedef struct RefEntry {
    const char* name;
    int32_t     w;
    int32_t     h;
    int32_t     bpp;
    uint32_t    hash;
} RefEntry;

/* Reference values produced from the shipped corpus via Pillow
 * (finding 13). The python pipeline was verified pixel-exact against PIL
 * itself, so matching these hashes == matching a PIL decode. */
static const RefEntry REFS[] = {
    { "Interface\\Buttons\\info_path.bmp",      32,   32,   8, 0x76366f0d },
    { "World\\WorldTextures\\ROCKROOF.BMP",     64,   64,   8, 0x01c2f02a },
    { "Interface\\Icons\\barracks.bmp",         40,   40,   8, 0x783225c1 },
    { "Interface\\BriefingPanel\\BriefingPanel.bmp", 475, 281, 24, 0x04a7ed73 },
    { "Interface\\LEVELPICKER\\Null.bmp",        4,    4,   4, 0x35598445 },
};

static void test_real(const char* dir)
{
    (void)resource_mount(dir, "LegoRR"); /* unused directly: texture_load
                                            drives the loader itself */
    for (int i = 0; i < (int)(sizeof(REFS) / sizeof(REFS[0])); ++i) {
        TextureSurface* t = texture_load(REFS[i].name);
        CHECK(t != NULL, "real: open");
        if (t == NULL) {
            continue;
        }
        CHECK(t->width == REFS[i].w && t->height == REFS[i].h
              && t->bpp == REFS[i].bpp, "real: dims");
        CHECK(t->width * t->height > 0 && t->pixels != NULL, "real: pixels");
        CHECK(texture_pixel_hash(t) == REFS[i].hash, "real: pixel hash");
        texture_free(t);
    }

    /* Case-insensitive + cache behavior (finding 13 §2). */
    TextureSurface* a = texture_get_or_load(NULL,
                                            "world\\worldtextures\\rockroof.bmp");
    CHECK(a != NULL, "real: get_or_load lowercase");
    if (a != NULL) {
        CHECK(texture_pixel_hash(a) == 0x01c2f02a, "real: cache hash");
        TextureSurface* b2 = texture_get_or_load(NULL,
                                                 "WORLD\\WORLDTEXTURES\\ROCKROOF.BMP");
        CHECK(b2 == a, "real: cache hit returns same object");
        CHECK(a->refcount == 2, "real: refcount incremented");
        texture_release(a); /* 1 */
        texture_release(a); /* 0 -> freed */
    }

    CHECK(texture_load("no.such.texture") == NULL, "real: missing texture NULL");
    texture_cache_clear();
}

int main(int argc, char** argv)
{
    test_synthetic();
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--real") == 0 && i + 1 < argc) {
            test_real(argv[i + 1]);
            break;
        }
    }
    /* The cache-window name pair (2026-10-25). */
    {
        static void* window[12];
        memset(window, 0, sizeof window);
        int count = 0;
        char key1[] = "MENUFACE";
        char key2[] = "OTHERFACE";
        CHECK(texture_cache_name_slot_find(window, 0u, key1) == NULL,
              "the empty window answers null");
        texture_cache_name_slot_append(window, &count, key1, (void*)1,
                                       (void*)2);
        CHECK(count == 1, "the append bumps the count");
        void* entry = texture_cache_name_slot_find(window, 1u, key1);
        CHECK(entry == (void*)&window[0], "the fold-find lands the entry");
        if (entry != NULL) {
            char* name = NULL;
            __builtin_memcpy(&name, (char*)entry + sizeof(void*),
                             sizeof name);
            CHECK(name != NULL && name != key1
                      && strcmp(name, "MENUFACE") == 0,
                  "the key rides a duplicate");
            void* slot_b = NULL;
            __builtin_memcpy(&slot_b, (char*)entry + 2 * sizeof(void*),
                             sizeof slot_b);
            CHECK(slot_b == (void*)2, "the slot pair rides with the key");
        }
        texture_cache_name_slot_append(window, &count, key2, (void*)3,
                                       (void*)4);
        CHECK(texture_cache_name_slot_find(window, 2u, "menuface")
                  == (void*)&window[0],
              "the find folds case on the first entry");
        CHECK(texture_cache_name_slot_find(window, 2u, key2)
                  == (void*)&window[3],
              "the second append lands past the first");
        CHECK(texture_cache_name_slot_find(window, 2u, "missing") == NULL,
              "the miss answers null");
    }

    if (s_failures == 0) {
        printf("texture_test: PASS%s\n",
               argc > 2 ? " (real wads)" : "");
        return 0;
    }
    printf("texture_test: %d failure(s)\n", s_failures);
    return 1;
}