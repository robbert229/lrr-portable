// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_unaligned_readers — G98-026's BMP header reader and G98-016's FLIC
 * chunk decoders on synthetic data placed at odd addresses. A file inside
 * a WAD, and a FLIC chunk after an odd-sized one, can start anywhere, so
 * neither reader may assume its fields are aligned. Under
 * -fsanitize=undefined a misaligned load is reported here. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gods98/flic_unit.h"
#include "gods98/g98_026.h"
#include "gods98/g98_026_record.h"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static void put_u16(uint8_t* at, unsigned int value)
{
    at[0] = (uint8_t)value;
    at[1] = (uint8_t)(value >> 8);
}

static void put_u32(uint8_t* at, uint32_t value)
{
    put_u16(at, value & 0xffff);
    put_u16(at + 2, value >> 16);
}

/* An 8-bit, 2x2 BMP with a two-colour palette, one byte past an aligned
 * address. */
static void test_bmp_unaligned(void)
{
    static uint8_t storage[0x36 + 8 + 8 + 16];
    uint8_t* bmp = storage + 1;
    IndexedBitmapPaletteData out;

    memset(storage, 0, sizeof(storage));
    memset(&out, 0, sizeof(out));
    bmp[0] = 'B';
    bmp[1] = 'M';
    put_u32(bmp + 0x02, 0x36 + 8 + 8);  /* file size */
    put_u32(bmp + 0x0a, 0x36 + 8);      /* pixel data offset */
    put_u32(bmp + 0x0e, 40);            /* info header size */
    put_u32(bmp + 0x12, 2);             /* width */
    put_u32(bmp + 0x16, 2);             /* height */
    put_u16(bmp + 0x1a, 1);             /* planes */
    put_u16(bmp + 0x1c, 8);             /* bits per pixel */
    put_u32(bmp + 0x2e, 2);             /* colours used */
    /* Palette entries are B, G, R, 0 in the file. */
    bmp[0x36 + 0] = 0x11; bmp[0x36 + 1] = 0x22; bmp[0x36 + 2] = 0x33;
    bmp[0x36 + 4] = 0x44; bmp[0x36 + 5] = 0x55; bmp[0x36 + 6] = 0x66;

    indexed_bitmap_palette_data_prepare(bmp, 0x36 + 16, &out);

    CHECK(out.width == 2, "BMP width");
    CHECK(out.height == 2, "BMP height");
    CHECK(out.bits_per_pixel == 8, "BMP bits per pixel");
    CHECK(out.indexed_flag == 0, "8-bit BMP is paletted");
    CHECK(out.scanline_stride == 4, "BMP stride rounds up to 4");
    CHECK(out.pixels == bmp + 0x36 + 8, "BMP pixels at the header's offset");
    CHECK(out.palette_count == 2, "BMP palette count");
    CHECK(out.palette != NULL, "BMP palette allocated");
    if (out.palette) {
        const uint8_t* entry = (const uint8_t*)out.palette;
        CHECK(entry[0] == 0x33 && entry[1] == 0x22 && entry[2] == 0x11,
              "BMP palette entry 0 swapped to R, G, B");
        CHECK(entry[4] == 0x66 && entry[5] == 0x55 && entry[6] == 0x44,
              "BMP palette entry 1 swapped to R, G, B");
    }
    object_field_38_release_target(&out);
}

static FLICSTRUCT* new_flic(void)
{
    FLICSTRUCT* fsp = (FLICSTRUCT*)calloc(1, sizeof(FLICSTRUCT));
    return fsp;
}

/* A FLICMEMORY frame whose second subchunk starts at an odd offset: a
 * 7-byte label chunk, then a Colour256 chunk setting palette entry 1. */
static void test_flic_frame_odd_subchunk(void)
{
    static uint8_t storage[64];
    uint8_t* file = storage + 1;
    uint8_t* chunk;
    FLICSTRUCT* fsp = new_flic();

    memset(storage, 0, sizeof(storage));
    /* Frame header: size, type 0xf1fa, two subchunks. */
    put_u32(file, 16 + 7 + 13);
    put_u16(file + 4, 0xf1fa);
    file[6] = 2;
    /* Label (0x5344), 7 bytes. */
    chunk = file + 16;
    put_u32(chunk, 7);
    put_u16(chunk + 4, 0x5344);
    /* Colour256 (4): one packet, skip 1, count 1, RGB (6-bit << 2). */
    chunk += 7;
    put_u32(chunk, 13);
    put_u16(chunk + 4, 4);
    put_u16(chunk + 6, 1);
    chunk[8] = 1;
    chunk[9] = 1;
    chunk[10] = 0x10 << 2;
    chunk[11] = 0x20 << 2;
    chunk[12] = 0x30 << 2;

    fsp->userflags = FLICMEMORY;
    fsp->rambufferhandle = (char*)file;
    fsp->pointerposition = 0;
    fsp->fsHeader.size = 16 + 7 + 13;
    fsp->fsDisplayMode = FLICMODE_BYTEPERPIXEL;

    CHECK(Flic_FrameChunk(fsp) == 1, "FLIC frame decodes");
    CHECK(fsp->fsPalette256[3] == 0x10 && fsp->fsPalette256[4] == 0x20 &&
              fsp->fsPalette256[5] == 0x30,
          "FLIC Colour256 after an odd-sized chunk sets entry 1");
    CHECK(fsp->pointerposition == 16 + 7 + 13, "FLIC frame advances past it");
    free(fsp);
}

/* A 16-bit byte-run chunk at an odd address: one line of two pixels, one
 * run of colour 0x1234. */
static void test_flic_brun16_unaligned(void)
{
    static uint8_t storage[32];
    uint8_t* chunk = storage + 1;
    uint16_t pixels[2] = { 0, 0 };
    FLICSTRUCT* fsp = new_flic();
    const uint16_t c = 0x1234;
    const uint16_t expect = (uint16_t)(((c & 0xffe0) << 1) | (c & 0x1f));

    memset(storage, 0, sizeof(storage));
    put_u32(chunk, 12);
    put_u16(chunk + 4, 25);
    chunk[6] = 1;          /* packets (skipped) */
    chunk[7] = 2;          /* run of two */
    put_u16(chunk + 8, c); /* odd address */

    fsp->fsSource = chunk;
    fsp->fsXsize = 2;
    fsp->fsYsize = 1;
    fsp->fsPitch = sizeof(pixels);
    fsp->fsSPtr = pixels;
    CHECK(Flic_BrunDepack16(fsp) == 1, "FLIC BRUN16 decodes");
    CHECK(pixels[0] == expect && pixels[1] == expect,
          "FLIC BRUN16 reads the run's colour");
    free(fsp);
}

/* A 16-bit delta chunk at an even address whose words are odd: one line,
 * one packet copying one pixel after skipping one. */
static void test_flic_delta16_unaligned(void)
{
    static uint8_t storage[32];
    uint8_t* chunk = storage + 1;
    uint16_t pixels[3] = { 0, 0, 0 };
    FLICSTRUCT* fsp = new_flic();
    const uint16_t c = 0x0421;
    const uint16_t expect = (uint16_t)(((c & 0xffe0) << 1) | (c & 0x1f));

    memset(storage, 0, sizeof(storage));
    put_u32(chunk, 16);
    put_u16(chunk + 4, 27);
    put_u16(chunk + 6, 1);   /* lines */
    put_u16(chunk + 8, 1);   /* packets */
    chunk[10] = 1;           /* skip one pixel */
    chunk[11] = 1;           /* copy one */
    put_u16(chunk + 12, c);

    fsp->fsSource = chunk;
    fsp->fsPitch = sizeof(pixels);
    fsp->fsSPtr = pixels;
    Flic_DeltaWord16(fsp);
    CHECK(pixels[0] == 0 && pixels[1] == expect && pixels[2] == 0,
          "FLIC DELTA16 writes the packet's pixel");
    free(fsp);
}

int main(void)
{
    test_bmp_unaligned();
    test_flic_frame_odd_subchunk();
    test_flic_brun16_unaligned();
    test_flic_delta16_unaligned();
    if (s_failures == 0) printf("unaligned_readers: all checks passed\n");
    return s_failures == 0 ? 0 : 1;
}
