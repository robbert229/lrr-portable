// SPDX-License-Identifier: LGPL-3.0-or-later
/* images_unit.c — indexed and BGR bitmap rows become top-down RGB565
 * pixels, honoring both source and destination stride. */
#include <cstdint>

#include <miniwin/ddraw.h>

#include "gods98/g98_026_record.h"
#include "gods98/images_unit.h"
#include "test/lrr_test.h"

static void check_rows(const uint16_t* pixels)
{
    CHECK_EQ(pixels[0], 0xf800u);
    CHECK_EQ(pixels[1], 0x07e0u);
    CHECK_EQ(pixels[2], 0xcafeu);
    CHECK_EQ(pixels[3], 0x001fu);
    CHECK_EQ(pixels[4], 0xffffu);
    CHECK_EQ(pixels[5], 0xcafeu);
}

int main()
{
    uint16_t pixels[6] = {0, 0, 0xcafe, 0, 0, 0xcafe};
    DDSURFACEDESC2 desc = {};
    desc.dwWidth = desc.dwHeight = 2;
    desc.lpSurface = pixels;
    desc.lPitch = 3 * sizeof(uint16_t);
    desc.ddpfPixelFormat.dwRGBBitCount = 16;
    desc.ddpfPixelFormat.dwRBitMask = 0xf800;
    desc.ddpfPixelFormat.dwGBitMask = 0x07e0;
    desc.ddpfPixelFormat.dwBBitMask = 0x001f;

    const uint8_t bgr[] = {255, 0, 0, 255, 255, 255, 0xee, 0xee,
                           0, 0, 255, 0, 255, 0, 0xee, 0xee};
    IndexedBitmapPaletteData bmp = {};
    bmp.width = bmp.height = 2;
    bmp.scanline_stride = 8;
    bmp.pixels = bgr;
    CHECK_EQ(Image_24BitSourceCopy(&desc, &bmp), 1);
    check_rows(pixels);

    pixels[0] = pixels[1] = pixels[3] = pixels[4] = 0;
    uint32_t palette[] = {Image_RGB2CR(255, 0, 0), Image_RGB2CR(0, 255, 0),
                          Image_RGB2CR(0, 0, 255), Image_RGB2CR(255, 255, 255)};
    const uint8_t indices[] = {2, 3, 0xee, 0xee, 0, 1, 0xee, 0xee};
    bmp.scanline_stride = 4;
    bmp.pixels = indices;
    bmp.palette = palette;
    CHECK_EQ(Image_8BitSourceCopy(&desc, &bmp), 1);
    check_rows(pixels);

    const unsigned int colour = Image_RGB2CR(0x12, 0x34, 0x56);
    CHECK_EQ(colour, 0x00563412u);
    unsigned char red = 0, blue = 0;
    Image_CR2RGB(colour, &red, nullptr, &blue);
    CHECK_EQ(red, 0x12u);
    CHECK_EQ(blue, 0x56u);
    return lrr_test_result();
}
