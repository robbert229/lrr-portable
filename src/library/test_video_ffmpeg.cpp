// SPDX-License-Identifier: LGPL-3.0-or-later
/* The video library (library/video_ffmpeg.h) on synthetic data only:
 * testdata/synthetic.avi (tools/make_synthetic_video.sh), 8 frames of
 * 32x24 at 15 fps, each one solid colour, with a mono 8 kHz track.
 *
 * Built three times. video_test runs against the host's FFmpeg and exits
 * 77, which ctest reports as skipped, when there is none. video_missing_test
 * is built with sonames that never load, and video_stub_test as the
 * LRR_MOVIES=OFF stub; both check that every entry point fails cleanly, as
 * the game does on a host without FFmpeg. */
#include "library/video_ffmpeg.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <string.h>

#include <vector>

namespace {

int failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__,        \
                    __LINE__, #cond);                                     \
            failures++;                                                   \
        }                                                                 \
    } while (0)

const char* ClipPath()
{
    return LRR_VIDEO_TEST_CLIP;
}

/* The clip's colours, as RGB565. */
const uint16_t kColours565[8] = {
    0xf800, 0x07e0, 0x001f, 0xffff, 0x0000, 0xffe0, 0x07ff, 0xf81f,
};

bool Near565(uint16_t got, uint16_t want)
{
    int dr = (int)(got >> 11) - (int)(want >> 11);
    int dg = (int)((got >> 5) & 0x3f) - (int)((want >> 5) & 0x3f);
    int db = (int)(got & 0x1f) - (int)(want & 0x1f);
    return dr >= -1 && dr <= 1 && dg >= -2 && dg <= 2 && db >= -1 && db <= 1;
}

/* The clip's frame a pixel shows, or -1. */
int FrameOf(uint16_t pixel)
{
    for (int i = 0; i < 8; i++) {
        if (Near565(pixel, kColours565[i])) return i;
    }
    return -1;
}

void TestMissing()
{
    CHECK(video_available() == 0);
    CHECK(video_open(ClipPath(), 1) == NULL);
    CHECK(video_open(ClipPath(), 0) == NULL);
    CHECK(video_open(NULL, 0) == NULL);
}

void TestOpen()
{
    CHECK(video_open("/nonexistent/clip.avi", 1) == NULL);
    CHECK(video_open("", 0) == NULL);

    VideoFile* video = video_open(ClipPath(), 1);
    CHECK(video != NULL);
    if (!video) return;
    unsigned int width = 0, height = 0, rate = 0, scale = 0;
    video_get_size(video, &width, &height);
    video_get_rate(video, &rate, &scale);
    CHECK(width == 32 && height == 24);
    CHECK(scale != 0 && rate / scale == 15);
    CHECK(video_get_frame_count(video) == 8);
    CHECK(video_get_duration(video) == 533);
    video_close(video);
}

/* The playback path: the frames in order, in RGB565 at a padded pitch,
 * paced to the clock, then the end. A busy host (CI's macOS runners)
 * oversleeps past a frame now and then, and video_update rightly drops
 * it, so the frames shown need only rise, each one whole, to the last. */
void TestUpdate()
{
    VideoFile* video = video_open(ClipPath(), 1);
    CHECK(video != NULL);
    if (!video) return;
    const long pitch = 80; /* wider than 32 * 2 */
    std::vector<uint16_t> pixels(pitch / 2 * 24, 0x1234);
    Uint64 start = SDL_GetTicksNS();
    video_start(video);
    int last = -1;
    for (int i = 0; i < 8; i++) {
        if (video_update(video, pixels.data(), pitch, 16, 0xf800, 0x07e0, 0x001f) != 1) break;
        int shown = FrameOf(pixels[0]);
        CHECK(shown > last);
        CHECK(FrameOf(pixels[23 * pitch / 2 + 31]) == shown);
        CHECK(pixels[32] == 0x1234); /* the padding is left alone */
        last = shown;
    }
    CHECK(last == 7);
    Uint64 elapsed = SDL_GetTicksNS() - start;
    /* Frame 7 is due 7/15 s after the start. */
    CHECK(elapsed >= 7 * SDL_NS_PER_SECOND / 15 - 20 * SDL_NS_PER_MS);
    CHECK(video_update(video, pixels.data(), pitch, 16, 0xf800, 0x07e0, 0x001f) == 0);
    video_close(video);
}

/* A late caller gets the frame that is due, not the next one: frame 5
 * after 5/15 s, or a later one if the host oversleeps, never an earlier. */
void TestDropsLateFrames()
{
    VideoFile* video = video_open(ClipPath(), 0);
    CHECK(video != NULL);
    if (!video) return;
    uint16_t pixels[32 * 24];
    video_start(video);
    SDL_DelayNS(SDL_NS_PER_SECOND * 5 / 15 + 20 * SDL_NS_PER_MS);
    CHECK(video_update(video, pixels, 64, 16, 0xf800, 0x07e0, 0x001f) == 1);
    CHECK(FrameOf(pixels[0]) >= 5);
    video_close(video);
}

/* An unsupported pixel format is no frame, not a crash. */
void TestUnsupportedFormat()
{
    VideoFile* video = video_open(ClipPath(), 0);
    CHECK(video != NULL);
    if (!video) return;
    uint8_t pixels[32 * 24];
    CHECK(video_update(video, pixels, 32, 8, 0, 0, 0) == 0);
    CHECK(video_get_frame_dib(video, 0, 12, 0xf00, 0xf0, 0xf) == NULL);
    video_close(video);
}

/* The frame-indexed path: a packed bottom-up DIB, forwards, backwards
 * and repeated. */
void TestFrameDib()
{
    VideoFile* video = video_open(ClipPath(), 0);
    CHECK(video != NULL);
    if (!video) return;
    const unsigned int order[] = { 0, 5, 5, 2, 7, 0 };
    for (unsigned int index : order) {
        const uint8_t* dib = (const uint8_t*)video_get_frame_dib(
            video, index, 16, 0xf800, 0x07e0, 0x001f);
        CHECK(dib != NULL);
        if (!dib) continue;
        uint32_t size, width, height;
        uint16_t bits;
        memcpy(&size, dib, 4);
        memcpy(&width, dib + 4, 4);
        memcpy(&height, dib + 8, 4);
        memcpy(&bits, dib + 14, 2);
        CHECK(size == 40 && width == 32 && height == 24 && bits == 16);
        uint16_t first, last;
        memcpy(&first, dib + 40, 2);
        memcpy(&last, dib + 40 + 23 * 64 + 31 * 2, 2);
        CHECK(Near565(first, kColours565[index]));
        CHECK(Near565(last, kColours565[index]));
    }
    CHECK(video_get_frame_dib(video, 8, 16, 0xf800, 0x07e0, 0x001f) == NULL);

    /* 32-bit XRGB, as a 32-bit display has. */
    const uint8_t* dib = (const uint8_t*)video_get_frame_dib(
        video, 1, 32, 0xff0000, 0xff00, 0xff);
    CHECK(dib != NULL);
    if (dib) {
        uint32_t pixel;
        memcpy(&pixel, dib + 40, 4);
        CHECK((pixel & 0xffffff) >= 0x00f000 && (pixel & 0xffffff) <= 0x00ff0f);
    }
    video_close(video);
}

} /* namespace */

int main()
{
#ifdef LRR_VIDEO_TEST_MISSING
    TestMissing();
#else
    if (!video_available()) {
        printf("video: no movies (FFmpeg not found, or built with "
               "LRR_MOVIES off); skipped\n");
        return 77;
    }
    TestOpen();
    TestUpdate();
    TestDropsLateFrames();
    TestUnsupportedFormat();
    TestFrameDib();
#endif
    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("video: all checks passed\n");
    return 0;
}
