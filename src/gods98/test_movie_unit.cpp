// SPDX-License-Identifier: LGPL-3.0-or-later
/* movie_unit.cpp — a synthetic stream yields one frame, then EOF. Exercise
 * the movie's public API and inspect the pixels it blits to a miniwin surface. */
#include <fstream>

#include <miniwin/ddraw.h>

#include "gods98/directdraw.h"
#include "gods98/movie_unit.h"
#include "library/video_ffmpeg.h"
#include "platform/native/test_fixture.h"
#include "test/lrr_test.h"

#include "gods98/files.h"

struct VideoFile {
    bool started;
    bool hasFrame;
};
static VideoFile stream;

extern "C" VideoFile* __wrap_video_open(const char* path, int withAudio)
{
    if (!withAudio || std::filesystem::path(path).filename() != "movie.avi") return nullptr;
    stream = {false, true};
    return &stream;
}

extern "C" void __wrap_video_get_size(VideoFile*, unsigned int* width, unsigned int* height)
{
    *width = 4;
    *height = 2;
}

extern "C" unsigned int __wrap_video_get_duration(VideoFile*)
{
    return 375;
}

extern "C" void __wrap_video_start(VideoFile* video)
{
    video->started = true;
}

extern "C" int __wrap_video_update(VideoFile* video, void* pixels, long pitch,
                                  unsigned int bpp, uint32_t red, uint32_t green, uint32_t blue)
{
    if (!video->started || !video->hasFrame) return 0;
    video->hasFrame = false;
    const uint32_t white = red | green | blue;
    for (int y = 0; y < 2; ++y) {
        auto* row = static_cast<unsigned char*>(pixels) + y * pitch;
        for (int x = 0; x < 4; ++x) std::memcpy(row + x * (bpp / 8), &white, bpp / 8);
    }
    return 1;
}

extern "C" void __wrap_video_close(VideoFile* video)
{
    video->started = false;
}

static void check_movie()
{
    G98CMovie* movie = movie_load("movie.avi");
    if (!CHECK(movie != nullptr, "movie opens")) return;

    unsigned int width = 0, height = 0;
    movie_get_size(movie, &width, &height);
    CHECK_EQ(width, 4u);
    CHECK_EQ(height, 2u);
    CHECK_EQ(movie_get_duration(movie), 375u);

    RECT rect = {2, 1, 6, 3};
    CHECK_EQ(movie_update(movie, 1.0f, &rect), 1);
    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    auto* target = directDrawGlobs.bSurf;
    if (CHECK_EQ(target->Lock(nullptr, &desc, DDLOCK_READONLY | DDLOCK_WAIT, nullptr), DD_OK)) {
        const auto& format = desc.ddpfPixelFormat;
        const unsigned int bytes = format.dwRGBBitCount / 8;
        const auto* pixels = static_cast<const unsigned char*>(desc.lpSurface);
        uint32_t white = 0, outside = 0;
        std::memcpy(&white, pixels + desc.lPitch + 2 * bytes, bytes);
        std::memcpy(&outside, pixels, bytes);
        const uint32_t rgb = format.dwRBitMask | format.dwGBitMask | format.dwBBitMask;
        CHECK_EQ(white & rgb, rgb);
        CHECK_EQ(outside & rgb, 0u);
        target->Unlock(nullptr);
    }

    CHECK_EQ(movie_update(movie, 1.0f, &rect), 0);
    CHECK_EQ(movie_update(movie, 1.0f, &rect), 0);
    movie_get_size(movie, &width, &height);
    CHECK_EQ(width, 0u);
    CHECK_EQ(height, 0u);
    CHECK_EQ(movie_get_duration(movie), 0u);
    movie_free(movie);
}

int main()
{
    if (!CHECK(fixture_enter("movie_unit_fixture") && fixture_mkdir("Data"), "create movie fixture"))
        return lrr_test_result();
    std::ofstream("Data/movie.avi", std::ios::binary).put('x');
    std::ofstream("Data/rejected.avi", std::ios::binary).put('x');
    file_initialise("movie_unit_fixture", 0, "");

    IDirectDraw* draw = nullptr;
    if (!CHECK_EQ(DirectDrawCreate(nullptr, &draw, nullptr), DD_OK)) return lrr_test_result();
    auto*& dd4 = directDrawGlobs.lpDirectDraw;
    CHECK_EQ(draw->QueryInterface(IID_IDirectDraw4, reinterpret_cast<void**>(&dd4)), DD_OK);
    draw->Release();
    if (!dd4) return lrr_test_result();

    DDSURFACEDESC2 desc = {};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    desc.dwWidth = 8;
    desc.dwHeight = 4;
    auto*& target = directDrawGlobs.bSurf;
    if (CHECK_EQ(dd4->CreateSurface(&desc, &target, nullptr), DD_OK)) {
        DDBLTFX fill = {};
        fill.dwSize = sizeof(fill);
        CHECK_EQ(target->Blt(nullptr, nullptr, nullptr, DDBLT_COLORFILL, &fill), DD_OK);
        check_movie();

        G98CMovie* rejected = movie_load("rejected.avi");
        if (CHECK(rejected != nullptr, "existing file gives a movie object even if decoding fails")) {
            unsigned int width = 1, height = 1;
            movie_get_size(rejected, &width, &height);
            CHECK_EQ(width, 0u);
            CHECK_EQ(height, 0u);
            CHECK_EQ(movie_get_duration(rejected), 0u);
            CHECK_EQ(movie_update(rejected, 1.0f, nullptr), 0);
            movie_free(rejected);
        }
        target->Release();
        target = nullptr;
    }
    dd4->Release();
    dd4 = nullptr;
    return lrr_test_result();
}
