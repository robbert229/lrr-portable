// SPDX-License-Identifier: LGPL-3.0-or-later
/* G98-026: BMP palette and pixel-data preparation (ADR 0005). */
#include <string.h>
#include "g98_026.h"
#include "g98_026_record.h"
#include "crt/vc5names.h"

/* The BMP header's fields are little-endian and sit at offsets the buffer
 * need not align (a file inside a WAD can start anywhere): read them
 * bytewise. */
static inline uint16_t bmp_read_u16(const uint8_t* at)
{
    return (uint16_t)(at[0] | (at[1] << 8));
}

static inline uint32_t bmp_read_u32(const uint8_t* at)
{
    return (uint32_t)at[0] | ((uint32_t)at[1] << 8) |
           ((uint32_t)at[2] << 16) | ((uint32_t)at[3] << 24);
}

static inline int32_t bmp_read_i32(const uint8_t* at)
{
    return (int32_t)bmp_read_u32(at);
}

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
    if (bmp_read_u16(bitmap + 0x1c) == 8) {
        count = bmp_read_i32(bitmap + 0x2e);
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
    output->width = bmp_read_i32(bitmap + 0x12);
    output->height = bmp_read_i32(bitmap + 0x16);
    bpp = bmp_read_u16(bitmap + 0x1c);
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
    output->pixels = bitmap + bmp_read_u32(bitmap + 10);
    output->reserved_20 = NULL;
}

// FUNCTION: LEGORR 0x00489d70
void object_field_38_release_target(void* record)
{
    void* field = ((IndexedBitmapPaletteData*)record)->palette;
    if (field != NULL) free(field);
}
