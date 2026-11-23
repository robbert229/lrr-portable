// SPDX-License-Identifier: LGPL-3.0-or-later
/* g98_026.c — parse packed true-colour BMP metadata and the default
 * 256-entry palette when the colour-count field is zero. */
#include <array>

#include "gods98/g98_026.h"
#include "gods98/g98_026_record.h"
#include "test/lrr_test.h"

int main()
{
    constexpr unsigned int headerSize = 54;
    std::array<uint8_t, headerSize + 256 * 4 + 32> bmp = {};
    bmp[0] = 'B';
    bmp[1] = 'M';
    bmp[10] = headerSize;
    bmp[14] = 40;
    bmp[18] = 3;
    bmp[22] = 2;
    bmp[26] = 1;

    const unsigned int bitCounts[] = {15, 16, 24, 32};
    const unsigned int strides[] = {8, 8, 12, 12};
    for (unsigned int i = 0; i < 4; ++i) {
        bmp[28] = bitCounts[i];
        IndexedBitmapPaletteData image = {};
        indexed_bitmap_palette_data_prepare(bmp.data(), bmp.size(), &image);
        CHECK_EQ(image.width, 3);
        CHECK_EQ(image.height, 2);
        CHECK_EQ(image.bits_per_pixel, bitCounts[i]);
        CHECK_EQ(image.indexed_flag, 1u);
        CHECK_EQ(image.scanline_stride, strides[i]);
        CHECK_EQ(image.pixels, bmp.data() + headerSize);
        CHECK_EQ(image.palette_count, 0u);
        CHECK_EQ(image.palette, nullptr);
        object_field_38_release_target(&image);
    }

    constexpr unsigned int paletteEnd = headerSize + 256 * 4;
    bmp[10] = paletteEnd & 0xff;
    bmp[11] = paletteEnd >> 8;
    bmp[28] = 8;
    bmp[paletteEnd - 4] = 0x12; // Blue.
    bmp[paletteEnd - 3] = 0x34; // Green.
    bmp[paletteEnd - 2] = 0x56; // Red.
    IndexedBitmapPaletteData indexed = {};
    indexed_bitmap_palette_data_prepare(bmp.data(), bmp.size(), &indexed);
    CHECK_EQ(indexed.indexed_flag, 0u);
    CHECK_EQ(indexed.palette_count, 256u);
    CHECK_EQ(indexed.scanline_stride, 4u);
    CHECK_EQ(indexed.pixels, bmp.data() + paletteEnd);
    if (CHECK(indexed.palette != nullptr, "default palette is allocated")) {
        const auto* last = reinterpret_cast<const uint8_t*>(&indexed.palette[255]);
        CHECK_EQ(last[0], 0x56u);
        CHECK_EQ(last[1], 0x34u);
        CHECK_EQ(last[2], 0x12u);
        CHECK_EQ(last[3], 0u);
    }
    object_field_38_release_target(&indexed);
    return lrr_test_result();
}
