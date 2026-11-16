// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-026's record, for g98_026.c and its test. images_unit.c declares the
 * same layout as BMP_Image, by the original's field names, so it does not
 * include this file. */
#ifndef LEGORRR_GODS98_G98_026_RECORD_H
#define LEGORRR_GODS98_G98_026_RECORD_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* What indexed_bitmap_palette_data_prepare fills in from a BMP file in
 * memory (images_unit.c's BMP_Image). */
typedef struct IndexedBitmapPaletteData {
    int32_t width;             /* +0x00 */
    int32_t height;            /* +0x04 */
    uint32_t flag_08;           /* +0x08 = 1 */
    uint32_t flag_0c;           /* +0x0c = 1 */
    uint32_t bits_per_pixel;    /* +0x10 */
    uint32_t indexed_flag;      /* +0x14: 0 for 8-bit, 1 otherwise */
    uint32_t scanline_stride;   /* +0x18 */
    const uint8_t* pixels;      /* +0x1c */
    void* reserved_20;          /* +0x20 = 0 (CntImage.buffer2) */
    uint32_t channel_fields[4]; /* +0x24..+0x30; 8-bit = 0xfc each */
    uint32_t palette_count;     /* +0x34 */
    uint32_t* palette;          /* +0x38 */
} IndexedBitmapPaletteData;
#ifdef __cplusplus
}
#endif
#endif
