// SPDX-License-Identifier: LGPL-3.0-only
// Surface layer leaves: the RGB channel packer (finding 23/431) and the
// surface-record pixel band (flip + the 8-bit palette expand + the 24-bit
// convert). Draw helpers need a real framebuffer and stay under the
// platform suites.
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gods98/surface.hpp"

static int s_failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            ++s_failures;                                                      \
        }                                                                      \
    } while (0)

int main(void)
{
    /* RGBChannelsPack (FUN_0047db90): byte triples pack little-endian —
     * first argument in the low byte, matching the palette scan. */
    CHECK(rgb_channels_pack(0x11, 0x22, 0x33) == 0x00332211u,
          "channels land low-to-high");
    CHECK(rgb_channels_pack(0, 0, 0) == 0u, "black packs zero");
    CHECK(rgb_channels_pack(255, 255, 255) == 0x00FFFFFFu,
          "white leaves the top byte clear");
    CHECK(rgb_channels_pack(0xFF, 0x00, 0xFF) == 0x00FF00FFu,
          "magenta keeps byte one empty");

    /* SurfacePixelRowsFlipWithScratch (2026-10-25): row pairs swap through
     * the engine-heap scratch, three stage copies per pair. */
    {
        /* Four rows of eight bytes as words. */
        uint16_t px[16] = {1, 2, 3, 4,     /* row 0 */
                           5, 6, 7, 8,     /* row 1 */
                           9, 10, 11, 12,  /* row 2 */
                           13, 14, 15, 16}; /* row 3 */
        SurfaceRecord rec;
        memset(&rec, 0, sizeof rec);
        rec.row_count = 4;
        rec.row_bytes = 8;
        rec.pixels = px;
        surface_pixel_rows_flip_with_scratch(&rec);
        const uint16_t want[16] = {13, 14, 15, 16, 9, 10, 11, 12,
                                   5, 6, 7, 8, 1, 2, 3, 4};
        CHECK(memcmp(px, want, sizeof px) == 0,
              "four rows reverse through the scratch");

        /* The odd row count keeps the middle row. */
        uint16_t q[6] = {1, 2, 3, 4, 5, 6};
        rec.row_count = 3;
        rec.row_bytes = 4;
        rec.pixels = q;
        surface_pixel_rows_flip_with_scratch(&rec);
        CHECK(q[0] == 5 && q[1] == 6 && q[2] == 3 && q[3] == 4 && q[4] == 1 &&
                  q[5] == 2,
              "the middle row rides the pair swap");

        /* The even rounding: the corpus strides the rows at (row_bytes/2)*2
         * bytes — five-byte rows address two-byte spans apart, so the swap
         * moves four-byte spans (the odd byte is the corpus's rounding). */
        unsigned char r[8] = {1, 2, 3, 4, 5, 6, 7, 8};
        rec.row_count = 2;
        rec.row_bytes = 5;
        rec.pixels = r;
        surface_pixel_rows_flip_with_scratch(&rec);
        const unsigned char want_r[8] = {5, 6, 7, 8, 1, 2, 3, 4};
        CHECK(memcmp(r, want_r, sizeof r) == 0,
              "five-byte rows swap the four-byte span");

        /* Single-row records: the pair loop skips (the deviation guard). */
        uint16_t s[2] = {0x1111u, 0x2222u};
        rec.row_count = 1;
        rec.row_bytes = 4;
        rec.pixels = s;
        surface_pixel_rows_flip_with_scratch(&rec);
        CHECK(s[0] == 0x1111u && s[1] == 0x2222u,
              "the single-row record rides (deviation-guarded)");
        surface_pixel_rows_flip_with_scratch(NULL); /* NULL is guarded */
    }

    /* Palette8To16BitSurfaceExpand (2026-10-25): the indexed walk lands the
     * entry bytes through their mask fields ((byte >> (8 - popcount)) <<
     * first-set), then flips. Masks are the 5/5/5 split: byte 0 rides the
     * +0x58 field, byte 1 the +0x5c field, byte 2 the +0x60 field. */
    {
        static uint16_t pixels[32];
        /* Palette entries are four bytes: [b0, b1, b2, x]. */
        static unsigned char palette[12] = {0x00, 0x00, 0x00, 0x00, /* idx 0 */
                                            0xF8, 0xF8, 0xF8, 0x00, /* idx 1 */
                                            0x00, 0xF8, 0x00, 0x00}; /* idx 2 */
        unsigned char src[16];
        SurfaceRecord rec;
        SurfaceConvertJob job;
        memset(&rec, 0, sizeof rec);
        memset(&job, 0, sizeof job);
        memset(pixels, 0, sizeof pixels);
        memset(src, 0, sizeof src);
        rec.depth = 16;
        rec.mask_low = 0x7C00u; /* pop 5, ffs 10: (b >> 3) << 10 */
        rec.mask_mid = 0x03E0u; /* pop 5, ffs 5: (b >> 3) << 5 */
        rec.mask_hi = 0x001Fu;  /* pop 5, ffs 0: (b >> 3) << 0 */
        rec.row_bytes = 8;
        rec.row_count = 1;
        rec.pixels = pixels;
        job.width = 4;
        job.height = 1;
        job.src_stride = 4;
        job.src = src;
        job.palette = palette;
        src[0] = 1;
        src[1] = 2;
        src[3] = 1;
        CHECK(surface_palette8_to_16bit_surface_expand(&rec, &job) == 1,
              "the indexed walk answers 1");
        CHECK(pixels[0] == 0x7FFFu, "the full-entry byte triple lands 0x7FFF");
        CHECK(pixels[1] == 0x03E0u, "the mid-byte-only entry lands 0x03E0");
        CHECK(pixels[2] == 0x0000u, "the zero entry is zero");
        CHECK(pixels[3] == 0x7FFFu, "the index repeat rides");

        /* Height two pins the flip tail: the rows reverse. */
        rec.row_count = 2;
        job.height = 2;
        job.width = 4;
        memset(pixels, 0, sizeof pixels);
        memset(src, 0, sizeof src);
        for (int i = 0; i != 4; ++i) {
            src[i] = 1;
            src[4 + i] = 2;
        }
        surface_palette8_to_16bit_surface_expand(&rec, &job);
        int ok = 1;
        for (int i = 0; i != 4; ++i) {
            if (pixels[i] != 0x03E0u) ok = 0; /* row 1 flipped to the top */
            if (pixels[4 + i] != 0x7FFFu) ok = 0;
        }
        CHECK(ok, "the flip tail reverses the two rows");

        /* Stride pin: width two, stride four, one pad byte per row — the
         * row 1 pixels land row_bytes clear of the origin, past the pad. */
        rec.row_count = 2;
        job.height = 2;
        job.width = 2;
        job.src_stride = 4;
        memset(pixels, 0, sizeof pixels);
        memset(src, 0, sizeof src);
        src[0] = 1; src[1] = 1;               /* row 0 pixels */
        src[2] = 0; src[3] = 0;               /* pad bytes */
        src[4] = 1; src[5] = 2;               /* row 1, pad skipped */
        surface_palette8_to_16bit_surface_expand(&rec, &job);
        /* Rows before the flip: [7FFF, 7FFF, 0, 0 | 7FFF, 03E0, 0, 0].
         * After the flip: the stride-tested row 1 rides the top half. */
        CHECK(pixels[0] == 0x7FFFu && pixels[1] == 0x03E0u,
              "the stride skips the pad (row 1 lands top half after flip)");
        CHECK(pixels[2] == 0u && pixels[3] == 0u, "the pad ride is clean");
        CHECK(pixels[4] == 0x7FFFu && pixels[5] == 0x7FFFu,
              "row 0 rides the bottom half");
        CHECK(pixels[6] == 0u && pixels[7] == 0u, "the tail words ride");

        /* Depth verdicts: 8/24/32 answer 0 untouched, other depths 1. */
        rec.depth = 8;
        memset(pixels, 0xAB, sizeof pixels);
        CHECK(surface_palette8_to_16bit_surface_expand(&rec, &job) == 0,
              "depth 8 answers 0 untouched");
        int touched = 0;
        for (size_t i = 0; i != 8; ++i) {
            if (((unsigned char*)pixels)[i] != 0xABu) {
                touched++;
            }
        }
        CHECK(touched == 0, "the depth-8 verdict writes nothing");
        rec.depth = 24;
        CHECK(surface_palette8_to_16bit_surface_expand(&rec, &job) == 0,
              "depth 24 answers 0");
        rec.depth = 32;
        CHECK(surface_palette8_to_16bit_surface_expand(&rec, &job) == 0,
              "depth 32 answers 0");
        rec.depth = 2;
        CHECK(surface_palette8_to_16bit_surface_expand(&rec, &job) == 1,
              "the odd depth answers 1 untouched");
        rec.depth = 16;
        CHECK(surface_palette8_to_16bit_surface_expand(NULL, &job) == 1,
              "the null record answers 1 (deviation)");
    }

    /* Bitmap24BitSurfaceConvert (2026-10-25): three source bytes per pixel
     * ride the masks REVERSED — byte 0 lands in the +0x60 high field, byte
     * 1 in +0x5c, byte 2 in +0x58 — and rows advance by the stride. */
    {
        static uint16_t pixels[16];
        static unsigned char src[16];
        SurfaceRecord rec;
        SurfaceConvertJob job;
        memset(&rec, 0, sizeof rec);
        memset(&job, 0, sizeof job);
        rec.depth = 16;
        rec.mask_low = 0x001Fu; /* pop 5, ffs 0: (b >> 3) << 0 */
        rec.mask_mid = 0x07E0u; /* pop 6, ffs 5: (b >> 2) << 5 */
        rec.mask_hi = 0xF800u;  /* pop 5, ffs 11: (b >> 3) << 11 */
        rec.row_bytes = 8;
        rec.row_count = 1;
        rec.pixels = pixels;
        job.width = 1;
        job.height = 1;
        job.src_stride = 3;
        job.src = src;
        memset(src, 0, sizeof src);
        memset(pixels, 0, sizeof pixels);

        /* 565 white: byte 0 0xF8 high, byte 1 0xFC mid, byte 2 0xF8 low. */
        src[0] = 0xF8;
        src[1] = 0xFC;
        src[2] = 0xF8;
        CHECK(surface_bitmap24_bit_surface_convert(&rec, &job) == 1,
              "the 24-bit walk answers 1");
        CHECK(pixels[0] == 0xFFFFu, "565 white lands full-field");

        /* The reversed mapping alone. */
        memset(src, 0, 3);
        src[0] = 0x10;
        surface_bitmap24_bit_surface_convert(&rec, &job);
        CHECK(pixels[0] == 0x1000u, "byte 0 lands in the HIGH field");
        memset(src, 0, 3);
        src[2] = 0x10;
        surface_bitmap24_bit_surface_convert(&rec, &job);
        CHECK(pixels[0] == 0x0002u, "byte 2 lands in the LOW field");

        /* Stride pin: width two, stride seven (three bytes per pixel, one
         * pad byte), two rows; then the flip reverses them. */
        job.width = 2;
        job.height = 2;
        job.src_stride = 7;
        rec.row_count = 2;
        memset(pixels, 0, sizeof pixels);
        memset(src, 0, sizeof src);
        src[3] = 0x00; src[4] = 0x00; src[5] = 0xF8; /* row 0 px1 -> 0x1F */
        src[6] = 0xFF;                               /* pad */
        src[7] = 0xF8; src[8] = 0xFC; src[9] = 0xF8; /* row 1 px0 -> white */
        surface_bitmap24_bit_surface_convert(&rec, &job);
        /* Before the flip: [0, 0x1F, 0, 0 | 0xFFFF, 0, 0, 0]; the flip
         * swaps the row halves. If the stride were ignored (6), row 1's
         * first pixel would ride the pad byte instead. */
        uint16_t w[8];
        memcpy(w, pixels, sizeof w);
        CHECK(w[0] == 0xFFFFu && w[1] == 0u && w[2] == 0u && w[3] == 0u,
              "the stride skips the pad (row 1 rides the top after flip)");
        CHECK(w[4] == 0u && w[5] == 0x1Fu && w[6] == 0u && w[7] == 0u,
              "row 0 rides the bottom half with its stride-placed pixels");

        /* Depth verdicts match the palette walk. */
        rec.depth = 8;
        memset(pixels, 0xAB, sizeof pixels);
        CHECK(surface_bitmap24_bit_surface_convert(&rec, &job) == 0,
              "depth 8 answers 0 untouched");
        rec.depth = 32;
        CHECK(surface_bitmap24_bit_surface_convert(&rec, &job) == 0,
              "depth 32 answers 0");
        rec.depth = 7;
        CHECK(surface_bitmap24_bit_surface_convert(&rec, &job) == 1,
              "the odd depth answers 1 untouched");
        rec.depth = 16;
        CHECK(surface_bitmap24_bit_surface_convert(NULL, &job) == 1,
              "the null record answers 1 (deviation)");
    }

    if (s_failures != 0) {
        fprintf(stderr, "surface_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("surface_test: PASS\n");
    return 0;
}
