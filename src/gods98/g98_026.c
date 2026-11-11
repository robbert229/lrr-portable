// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-026: BMP palette and pixel-data preparation (ADR 0005). */
#include <string.h>
#include "g98_026.h"
#include "crt/vc5names.h"

typedef struct IndexedBitmapPaletteData {
    int32_t width;             /* +0x00 */
    int32_t height;            /* +0x04 */
    uint32_t flag_08;           /* +0x08 = 1 */
    uint32_t flag_0c;           /* +0x0c = 1 */
    uint32_t bits_per_pixel;    /* +0x10 */
    uint32_t indexed_flag;      /* +0x14: 0 for 8-bit, 1 otherwise */
    uint32_t scanline_stride;   /* +0x18 */
    const uint8_t* pixels;      /* +0x1c */
    uint32_t reserved_20;       /* +0x20 = 0 */
    uint32_t channel_fields[4]; /* +0x24..+0x30; 8-bit = 0xfc each */
    uint32_t palette_count;     /* +0x34 */
    uint32_t* palette;          /* +0x38 */
} IndexedBitmapPaletteData;

// FUNCTION: LEGORR 0x00489c30
void indexed_bitmap_palette_data_prepare(const uint8_t* bitmap, uint32_t size,
                                         IndexedBitmapPaletteData* output)
{
    int count;
    int i;
    unsigned int bpp;
    unsigned int bytes_per_pixel;
    uint32_t* palette;
    uint8_t* entry;
    uint8_t red;
    (void)size;
    if (*(const uint16_t*)(bitmap + 0x1c) == 8) {
        count = *(const int32_t*)(bitmap + 0x2e);
        if (count == 0) count = 256;
        palette = (uint32_t*)malloc(0x400);
        memset(palette, 0, 0x400);
        memcpy(palette, bitmap + 0x36, count * 4);
        output->indexed_flag = 0;
        output->channel_fields[0] = 0xfc;
        output->channel_fields[1] = 0xfc;
        output->channel_fields[2] = 0xfc;
        output->channel_fields[3] = 0xfc;
        output->palette_count = count;
        entry = (uint8_t*)palette;
        for (i = 0; i < count; i++, entry += 4) {
            red = entry[0];
            entry[0] = entry[2];
            entry[2] = red;
        }
        output->palette = palette;
    } else {
        output->indexed_flag = 1;
        output->palette_count = 0;
        output->palette = NULL;
    }
    output->width = *(const int32_t*)(bitmap + 0x12);
    output->height = *(const int32_t*)(bitmap + 0x16);
    bpp = *(const uint16_t*)(bitmap + 0x1c);
    output->bits_per_pixel = bpp;
    switch (bpp) {
    case 8: bytes_per_pixel = 1; break;
    case 15: case 16: bytes_per_pixel = 2; break;
    case 24: bytes_per_pixel = 3; break;
    case 32: bytes_per_pixel = 4; break;
    default: bytes_per_pixel = 0; break;
    }
    output->scanline_stride = (bytes_per_pixel * output->width + 3) & ~3u;
    output->flag_0c = 1;
    output->flag_08 = 1;
    output->pixels = bitmap + *(const uint32_t*)(bitmap + 10);
    output->reserved_20 = 0;
}

// FUNCTION: LEGORR 0x00489d70
void object_field_38_release_target(void* record)
{
    void* field = *(void**)((char*)record + 0x38);
    if (field != NULL) free(field);
}
