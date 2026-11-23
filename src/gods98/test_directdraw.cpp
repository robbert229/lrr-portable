// SPDX-License-Identifier: LGPL-3.0-or-later
/* directdraw.c — pack RGB into a 565 surface and save a 32-bit offscreen
 * surface as a 24-bit BMP with bottom-up rows and four-byte padding. */
#include <array>
#include <fstream>

#include <miniwin/ddraw.h>

#include "gods98/directdraw.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"

#include "gods98/files.h"

static IDirectDrawSurface4* make_surface(IDirectDraw4* draw, unsigned int bpp,
                                        unsigned int red, unsigned int green, unsigned int blue)
{
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    desc.dwWidth = 2;
    desc.dwHeight = 2;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    desc.ddpfPixelFormat.dwSize = sizeof(desc.ddpfPixelFormat);
    desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
    desc.ddpfPixelFormat.dwRGBBitCount = bpp;
    desc.ddpfPixelFormat.dwRBitMask = red;
    desc.ddpfPixelFormat.dwGBitMask = green;
    desc.ddpfPixelFormat.dwBBitMask = blue;
    IDirectDrawSurface4* surface = nullptr;
    CHECK_EQ(draw->CreateSurface(&desc, &surface, nullptr), DD_OK);
    return surface;
}

static void check_bmp(IDirectDrawSurface4* surface)
{
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    if (!CHECK_EQ(surface->Lock(nullptr, &desc, DDLOCK_WAIT, nullptr), DD_OK)) return;
    const uint32_t top[] = {0x00123456, 0x00ff0000};
    const uint32_t bottom[] = {0x0000ff00, 0x000000ff};
    std::memcpy(desc.lpSurface, top, sizeof(top));
    std::memcpy(static_cast<unsigned char*>(desc.lpSurface) + desc.lPitch, bottom, sizeof(bottom));
    surface->Unlock(nullptr);

    if (!CHECK(directdraw_save_bmp(surface, "pixels.bmp"), "write BMP")) return;
    std::ifstream bmp("Data/pixels.bmp", std::ios::binary);
    std::array<unsigned char, 70> bytes = {};
    bmp.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    CHECK_EQ(bmp.gcount(), static_cast<std::streamsize>(bytes.size()));
    CHECK_EQ(bmp.peek(), std::char_traits<char>::eof());

    const unsigned char header[] = {
        'B', 'M', 70, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0,
        40, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 1, 0, 24, 0,
    };
    const unsigned char rows[] = {0, 255, 0, 255, 0, 0, 0, 0,
                                  0x56, 0x34, 0x12, 0, 0, 255, 0, 0};
    for (unsigned int i = 0; i < sizeof(header); ++i) CHECK_EQ(bytes[i], header[i]);
    for (unsigned int i = 0; i < sizeof(rows); ++i) CHECK_EQ(bytes[54 + i], rows[i]);
}

int main()
{
    if (!CHECK(fixture_enter("directdraw_bmp_fixture") && fixture_mkdir("Data"), "create BMP fixture"))
        return lrr_test_result();
    file_initialise("directdraw_bmp_fixture", 0, "");

    IDirectDraw* draw = nullptr;
    IDirectDraw4* draw4 = nullptr;
    if (!CHECK_EQ(DirectDrawCreate(nullptr, &draw, nullptr), DD_OK)) return lrr_test_result();
    CHECK_EQ(draw->QueryInterface(IID_IDirectDraw4, reinterpret_cast<void**>(&draw4)), DD_OK);
    draw->Release();
    if (!draw4) return lrr_test_result();

    IDirectDrawSurface4* surface = make_surface(draw4, 16, 0xf800, 0x07e0, 0x001f);
    if (surface) {
        CHECK_EQ(directdraw_get_colour(surface, 0xffffff), 0xffffu);
        CHECK_EQ(directdraw_get_colour(surface, 0xff8040), 0xfc08u);
        CHECK_EQ(directdraw_get_colour(surface, 0x000000), 0u);
        surface->Release();
    }
    surface = make_surface(draw4, 32, 0xff0000, 0x00ff00, 0x0000ff);
    if (surface) {
        check_bmp(surface);
        surface->Release();
    }
    draw4->Release();
    return lrr_test_result();
}
