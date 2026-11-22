// SPDX-License-Identifier: LGPL-3.0-or-later
/* avi_player.cpp — open a synthetic video stream, expose its length in
 * frames, flip the decoded frame's rows when blitting, and seek. Use
 * miniwin's offscreen surfaces. */
#include <miniwin/ddraw.h>

#include "gods98/avi_player.h"
#include "gods98/directdraw.h"
#include "library/video_ffmpeg.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"

struct VideoFile {
    unsigned int lastFrame;
};
static VideoFile stream;

extern "C" VideoFile* __wrap_video_open(const char* path, int withAudio)
{
    if (withAudio || std::filesystem::path(path).filename() != "clip.avi") return nullptr;
    stream = {};
    return &stream;
}

extern "C" void __wrap_video_get_rate(VideoFile*, unsigned int* rate, unsigned int* scale)
{
    *rate = 24;
    *scale = 1;
}

extern "C" void __wrap_video_get_size(VideoFile*, unsigned int* width, unsigned int* height)
{
    *width = 4;
    *height = 2;
}

extern "C" unsigned int __wrap_video_get_frame_count(VideoFile*)
{
    return 48;
}

extern "C" const void* __wrap_video_get_frame_dib(VideoFile* video, unsigned int index,
                                                 unsigned int bpp, uint32_t, uint32_t, uint32_t)
{
    static struct {
        BITMAPINFOHEADER header;
        uint16_t pixels[2][4];
    } dib = {};
    dib.header.biSize = sizeof(dib.header);
    dib.header.biWidth = 4;
    dib.header.biHeight = 2;
    dib.header.biPlanes = 1;
    dib.header.biBitCount = static_cast<unsigned short>(bpp);
    for (int x = 0; x < 4; ++x) {
        dib.pixels[0][x] = 0x001f; // Blue bottom row in a packed DIB.
        dib.pixels[1][x] = 0xf800; // Red top row.
    }
    video->lastFrame = index;
    return index < 48 && bpp == 16 ? &dib : nullptr;
}

extern "C" void __wrap_video_close(VideoFile*)
{
}

static void check_frame_pixels(AviMovie* clip, IDirectDraw4* draw)
{
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.dwWidth = 4;
    desc.dwHeight = 2;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    auto*& surface = directDrawGlobs.bSurf;
    if (!CHECK_EQ(draw->CreateSurface(&desc, &surface, nullptr), DD_OK)) return;
    avi_blit_to_back_buffer(clip, nullptr);

    desc = {};
    desc.dwSize = sizeof(desc);
    if (CHECK_EQ(surface->Lock(nullptr, &desc, DDLOCK_READONLY | DDLOCK_WAIT, nullptr), DD_OK)) {
        CHECK_EQ(desc.ddpfPixelFormat.dwRGBBitCount, 16u);
        const auto* pixels = static_cast<const unsigned char*>(desc.lpSurface);
        uint16_t top = 0, bottom = 0;
        std::memcpy(&top, pixels, sizeof(top));
        std::memcpy(&bottom, pixels + desc.lPitch, sizeof(bottom));
        CHECK_EQ(top, 0xf800u);
        CHECK_EQ(bottom, 0x001fu);
        surface->Unlock(nullptr);
    }
    surface->Release();
    surface = nullptr;
}

int main()
{
    IDirectDraw* draw = nullptr;
    IDirectDraw4* draw4 = nullptr;
    if (!CHECK_EQ(DirectDrawCreate(nullptr, &draw, nullptr), DD_OK)) return lrr_test_result();
    CHECK_EQ(draw->QueryInterface(IID_IDirectDraw4, reinterpret_cast<void**>(&draw4)), DD_OK);
    draw->Release();
    if (!draw4) return lrr_test_result();
    avi_initialise(draw4);

    char filename[] = "clip.avi";
    AviMovie* clip = avi_load(filename);
    if (CHECK(avi_is_ok(clip), "open synthetic AVI")) {
        CHECK_EQ(avi_get_length(clip), 48ul);
        CHECK_EQ(avi_get_time(clip), 0ul);
        CHECK_EQ(avi_update(clip), 1);
        check_frame_pixels(clip, draw4);
        avi_set_time(clip, 17);
        CHECK_EQ(avi_get_time(clip), 17ul);
        CHECK_EQ(avi_update(clip), 1);
        CHECK_EQ(stream.lastFrame, 17u);
    }
    avi_remove(clip);

    char invalid[] = "invalid.avi";
    AviMovie* rejected = avi_load(invalid);
    CHECK_EQ(avi_is_ok(rejected), 0);
    CHECK_EQ(avi_get_length(rejected), 0ul);
    CHECK_EQ(avi_update(rejected), 0);
    avi_remove(rejected);
    draw4->Release();
    return lrr_test_result();
}
