// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_movie — validation for src/gods98/movie.cpp (findings 14 §4, 23).
 *
 * The config surface (AvisEnabled / MoviePlayNamed / MoviePlayLevelVideo
 * keys, '\'->'/' normalization) and the pixel scaler run always, no decoder
 * needed.
 *
 * When the FFmpeg runtime libs are present (movie_available), a synthetic
 * DIB/rawvideo AVI built by this test (three 8x6 solid-color frames, no
 * audio) is decoded end-to-end — open, info, per-frame RGBA pixel, EOF — and
 * the pure pixel scaler is exercised. With `--real <dir>` the shipped
 * `Data/AVI/intro.avi` is the probe: dims/fps/audio rate pinned, plus a
 * frame count bounded by the ffprobe duration.
 *
 * Wire-up: meson test 'movie' (suite 'movie'); the decode checks skip
 * (reported) when the host has no FFmpeg runtime.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "block.hpp"
#include "movie.hpp"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static BlockFile* parse(const char* text)
{
    return block_parse((char*)text, strlen(text));
}

/* ------------------------------------------------------------------ *
 *  Config surface (always)
 * ------------------------------------------------------------------ */

static void test_config(void)
{
    BlockFile* bf = parse(
        "Lego* {\n"
        "\tMain {\n"
        "\t\tDontPlayAvis\tFALSE\n"
        "\t\tRRAvi\t\tavi\\intro.avi\n"
        "\t\tLegoAvi\t\tavi\\lmi640.avi\n"
        "\t}\n"
        "\tLevels {\n"
        "\t\tTran01 { Video Avi\\mbats.avi Nested {\n"
        "\t\t\tVideo Avi\\rock1.avi } }\n"
        "\t}\n"
        "}\n");

    CHECK(movie_avis_enabled(bf) == 1, "DontPlayAvis FALSE -> enabled");
    CHECK(movie_avis_enabled(NULL) == 1, "no config -> enabled (corpus default)");

    BlockFile* off = parse("Lego* { Main { DontPlayAvis TRUE } }");
    CHECK(movie_avis_enabled(off) == 0, "DontPlayAvis TRUE -> disabled");
    block_close(off);

    char buf[256];
    CHECK(movie_key_path(bf, "RRAvi", buf, sizeof(buf)) == 1
          && strcmp(buf, "avi/intro.avi") == 0,
          "RRAvi key -> normalized path");
    CHECK(movie_key_path(bf, "LegoAvi", buf, sizeof(buf)) == 1
          && strcmp(buf, "avi/lmi640.avi") == 0,
          "LegoAvi key -> normalized path");
    CHECK(movie_key_path(bf, "NoSuchKey", buf, sizeof(buf)) == 0,
          "missing key -> 0");

    CHECK(movie_level_video_path(bf, "Tran01", buf, sizeof(buf)) == 1
          && strcmp(buf, "Avi/mbats.avi") == 0,
          "level Video key through Levels container");
    /* The first match in document order wins ("Video" under Nested); the
     * flat lookup must not shadow the Levels container. */
    CHECK(movie_level_video_path(bf, "Nope", buf, sizeof(buf)) == 0,
          "missing level -> 0");

    BlockFile* flat = parse(
        "Lego* { Tran01 { Video Avi\\rock2.avi } Levels { Tran01 { } } }");
    CHECK(movie_level_video_path(flat, "Tran01", buf, sizeof(buf)) == 1
          && strcmp(buf, "Avi/rock2.avi") == 0,
          "flat shape fallback");

    block_close(bf);
    block_close(flat);
}

/* ------------------------------------------------------------------ *
 *  Pure pixel scaler (always)
 * ------------------------------------------------------------------ */

static void test_scaler(void)
{
    /* 2x2 RGBA frame: four distinct corner colours. */
    const uint8_t src[] = {
        255, 0,   0,   255,  0,   255, 0,   255,
        0,   0,   255, 255,  255, 255, 0,   255,
    };
    uint8_t dst[8 * 8 * 4];
    memset(dst, 0, sizeof(dst));
    movie_scale_rgba(src, 2, 2, 8, dst, 4, 4, 4 * 4);

    CHECK(dst[0] == 255 && dst[1] == 0 && dst[2] == 0,
          "scaled top-left is red");
    const uint8_t* tl = dst + (4 * 4 * 0);
    CHECK(tl[0] == 255 && tl[1] == 0 && tl[2] == 0, "row0 col0 red");
    const uint8_t* tr = dst + (4 * 4 * 0 + (4 - 1) * 4);
    CHECK(tr[0] == 0 && tr[1] == 255 && tr[2] == 0, "row0 col3 green");
    const uint8_t* bl = dst + (4 * 4 * (4 - 1));
    CHECK(bl[0] == 0 && bl[1] == 0 && bl[2] == 255, "row3 col0 blue");
    const uint8_t* br = dst + (4 * 4 * (4 - 1) + (4 - 1) * 4);
    CHECK(br[0] == 255 && br[1] == 255 && br[2] == 0, "row3 col3 white");
}

/* ------------------------------------------------------------------ *
 *  Synthetic DIB/rawvideo AVI (decoder present)
 * ------------------------------------------------------------------ */

#define TEST_W 8
#define TEST_H 6
#define TEST_FRAMES 3

static void w32(void* dst, uint32_t v)
{
    uint8_t* b = (uint8_t*)dst;
    b[0] = (uint8_t)(v & 0xff);
    b[1] = (uint8_t)((v >> 8) & 0xff);
    b[2] = (uint8_t)((v >> 16) & 0xff);
    b[3] = (uint8_t)((v >> 24) & 0xff);
}

static void w16(void* dst, uint16_t v)
{
    uint8_t* b = (uint8_t*)dst;
    b[0] = (uint8_t)(v & 0xff);
    b[1] = (uint8_t)((v >> 8) & 0xff);
}

/* Write a minimal AVI (LIST hdrl/strl + LIST movi, no idx1) whose single
 * stream is 24-bit DIB ("DIB " handler -> rawvideo bgr24). Three frames of
 * TEST_W x TEST_H: red, green, blue. Returns the number of bytes written. */
static size_t avi_tiny(uint8_t* buf, size_t cap)
{
    (void)cap; /* the caller bounds buf; the writer cannot overflow because
                * the AVI is fixed-size and buf is sized accordingly. */
    const int frame_bytes = TEST_W * TEST_H * 3; /* 8*6*3 = 144 */
    size_t p = 0;
    auto putc32 = [&](const char fourcc[4]) {
        memcpy(buf + p, fourcc, 4);
        p += 4;
    };
    auto putc64c = [&](const char fourcc[4], uint32_t size) {
        putc32(fourcc);
        w32(buf + p, size);
        p += 4;
    };

    size_t riff_size_pos;
    memcpy(buf + p, "RIFF", 4);
    p += 4;
    riff_size_pos = p;
    w32(buf + p, 0); /* patched below */
    p += 4;
    putc32("AVI ");
    /* LIST hdrl */
    putc64c("LIST", 0); /* size patched */
    size_t hdrl_size_pos = p - 4;
    putc32("hdrl");
    /* avih */
    putc64c("avih", 56);
    w32(buf + p, 250000);        /* dwMicroSecPerFrame (fps 4) */
    w32(buf + p + 4, 0);          /* dwMaxBytesPerSec */
    w32(buf + p + 8, 0);          /* dwPaddingGranularity */
    w32(buf + p + 12, 0);         /* dwFlags */
    w32(buf + p + 16, TEST_FRAMES);/* dwTotalFrames */
    w32(buf + p + 20, 0);         /* dwInitialFrames */
    w32(buf + p + 24, 1);         /* dwStreams */
    w32(buf + p + 28, frame_bytes);/* dwSuggestedBufferSize */
    w32(buf + p + 32, TEST_W);    /* dwWidth */
    w32(buf + p + 36, TEST_H);    /* dwHeight */
    memset(buf + p + 40, 0, 16);  /* 4 reserved dwords */
    p += 56;
    /* LIST strl */
    putc64c("LIST", 0);
    size_t strl_size_pos = p - 4;
    putc32("strl");
    /* strh */
    putc64c("strh", 56);
    putc32("vids");
    putc32("DIB ");
    w32(buf + p, 0);              /* dwFlags */
    w32(buf + p + 4, 0);          /* wPriority:wLanguage */
    w32(buf + p + 8, 0);          /* dwInitialFrames */
    w32(buf + p + 12, 1);         /* dwScale */
    w32(buf + p + 16, 4);         /* dwRate (fps) */
    w32(buf + p + 20, 0);         /* dwStart */
    w32(buf + p + 24, TEST_FRAMES);/* dwLength */
    w32(buf + p + 28, frame_bytes);
    w32(buf + p + 32, 0xFFFFFFFFu);/* dwQuality */
    w32(buf + p + 36, 0);         /* dwSampleSize */
    memset(buf + p + 40, 0, 16);  /* rcFrame */
    p += 56;
    /* strf (BITMAPINFOHEADER, top-down; biPlanes/biBitCount are 2-byte
     * packed fields, shifting biCompression down to 0x10) */
    putc64c("strf", 40);
    w32(buf + p, 40);             /* biSize */
    w32(buf + p + 4, TEST_W);     /* biWidth */
    w32(buf + p + 8, (uint32_t)(int32_t)-TEST_H); /* biHeight (top-down) */
    w16(buf + p + 12, 1);         /* biPlanes */
    w16(buf + p + 14, 24);        /* biBitCount */
    w32(buf + p + 16, 0);         /* biCompression BI_RGB */
    w32(buf + p + 20, frame_bytes);
    w32(buf + p + 24, 0);         /* biXPelsPerMeter */
    w32(buf + p + 28, 0);         /* biYPelsPerMeter */
    w32(buf + p + 32, 0);         /* biClrUsed */
    w32(buf + p + 36, 0);         /* biClrImportant */
    p += 40;
    /* patch the two list sizes */
    w32(buf + strl_size_pos, (uint32_t)(p - (strl_size_pos + 4)));
    /* strl content is 4 + 64 + 48 = 116 */
    w32(buf + hdrl_size_pos, (uint32_t)(p - (hdrl_size_pos + 4)));
    /* LIST movi */
    putc64c("LIST", 0);
    size_t movi_size_pos = p - 4;
    putc32("movi");
    static const uint8_t red[3]   = { 0x00, 0x00, 0xff }; /* B G R = red */
    static const uint8_t green[3] = { 0x00, 0xff, 0x00 };
    static const uint8_t blue[3]  = { 0xff, 0x00, 0x00 };
    const uint8_t* palette[TEST_FRAMES] = { red, green, blue };
    for (int f = 0; f < TEST_FRAMES; ++f) {
        putc64c("00dc", frame_bytes);
        for (int y = 0; y < TEST_H; ++y) {
            for (int x = 0; x < TEST_W; ++x) {
                buf[p++] = palette[f][0];
                buf[p++] = palette[f][1];
                buf[p++] = palette[f][2];
            }
        }
    }
    w32(buf + movi_size_pos, (uint32_t)(p - (movi_size_pos + 4)));
    w32(buf + riff_size_pos, (uint32_t)(p - (riff_size_pos + 4)));
    return p;
}

static void test_decode_synthetic(void)
{
    uint8_t avi[4096];
    size_t n = avi_tiny(avi, sizeof(avi));
    CHECK(n > 0 && n < sizeof(avi), "synthetic AVI built");

    char path[256];
    snprintf(path, sizeof(path), "/tmp/opencode/lrr_movie_test_%ld.avi",
             (long)getpid());
    FILE* f = fopen(path, "wb");
    if (f == NULL) {
        printf("SKIP: cannot write %s\n", path);
        return;
    }
    fwrite(avi, 1, n, f);
    fclose(f);

    Movie* mv = movie_open_path(path);
    CHECK(mv != NULL, "synthetic AVI opens");
    if (mv == NULL) {
        return;
    }
    MovieInfo info;
    CHECK(movie_info(mv, &info) == 0, "movie_info ok");
    CHECK(info.width == TEST_W && info.height == TEST_H,
          "synthetic dims 8x6");
    CHECK(info.fps_num == 4 && info.fps_den == 1,
          "synthetic fps 4/1 (strh rate 4 scale 1)");
    CHECK(info.audio_rate == 0 && info.audio_channels == 0,
          "synthetic has no audio");

    const uint8_t expect[TEST_FRAMES][3] = {
        { 0xff, 0x00, 0x00 }, /* red   */
        { 0x00, 0xff, 0x00 }, /* green */
        { 0x00, 0x00, 0xff }, /* blue  */
    };
    int got = 0;
    MovieOut out;
    for (int i = 0; i < TEST_FRAMES + 2; ++i) {
        int r = movie_next(mv, &out);
        if (r == 1) {
            break;
        }
        CHECK(r == 0, "movie_next ok");
        if (r != 0) {
            break;
        }
        CHECK(out.rgba != NULL && out.rgba_pitch == TEST_W * 4,
              "frame present with 8x6 pitch");
        if (out.rgba != NULL && got < TEST_FRAMES) {
            const uint8_t* px = out.rgba;
            CHECK(px[0] == expect[got][0] && px[1] == expect[got][1]
                  && px[2] == expect[got][2] && px[3] == 0xff,
                  "frame pixel colour matches DIB frame");
        }
        if (got < TEST_FRAMES) {
            CHECK(out.audio == NULL && out.audio_frames == 0,
                  "no audio track -> empty FIFO");
        }
        ++got;
    }
    CHECK(got == TEST_FRAMES, "three frames decoded, then EOF");

    movie_close(mv);
    if (getenv("LEGORRR_KEEP_AVI") == NULL) {
        remove(path);
    } else {
        printf("movie: kept %s for inspection\n", path);
    }
}

/* ------------------------------------------------------------------ *
 *  Shipped-data probe: Data/AVI/intro.avi (--real <game dir>)
 * ------------------------------------------------------------------ */

static void test_real(const char* dir)
{
    if (!movie_available()) {
        printf("SKIP: no FFmpeg runtime — shipped probe skipped\n");
        return;
    }
    char os[512];
    if (!movie_path_resolve(dir, "avi/intro.avi", os, sizeof(os))) {
        printf("SKIP: no Data/AVI/intro.avi under %s\n", dir);
        return;
    }
    Movie* mv = movie_open_path(os);
    CHECK(mv != NULL, "intro.avi opens");
    if (mv == NULL) {
        return;
    }
    MovieInfo info;
    CHECK(movie_info(mv, &info) == 0, "intro.avi info ok");
    CHECK(info.width == 320 && info.height == 200,
          "intro.avi is 320x200 (indeo5)");
    CHECK(info.fps_num == 15 && info.fps_den == 1,
          "intro.avi is 15 fps");
    CHECK(info.audio_rate == 22050,
          "intro.avi audio is 22050 Hz (adpcm_ms)");

    long frames = 0;
    int rc = 0;
    MovieOut out;
    while ((rc = movie_next(mv, &out)) == 0) {
        ++frames;
    }
    CHECK(rc == 1, "intro.avi ends cleanly");
    /* ffprobe: 254.733 s @ 15 fps = 3821 frames. */
    CHECK(frames >= 3700 && frames <= 3950,
          "intro.avi frame count sane (~3821)");
    printf("real: intro.avi %dx%d %d/%d fps, %d Hz, %ld frames\n",
           info.width, info.height, info.fps_num, info.fps_den,
           info.audio_rate, frames);
    movie_close(mv);
}

/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ *
 *  Rect-aware pump (DirectShowVideoPlaybackLoop @ 0x42efc0, issue
 *  #303). Driven by a fake decoder (frames 0..n-1, then EOF) and a fake
 *  present sink, so both branches are checked without any real media.
 * ------------------------------------------------------------------ */

typedef struct FakePump {
    int       next_count;
    int       frame_count;
    int       quit_polls;
    int       quit_after;    /* return quit truthy after this many polls */
    int       present_count;
    int       restore_count;
    int       restore_seen_before_first_present;
    int       stop_at_frame; /* present reports failure at this frame */
    const MovieRect* last_rect;
    int       rect_was_null;
} FakePump;

static FakePump s_fake;

static int fake_next(MovieOut* out, void* user)
{
    (void)user;
    if (++s_fake.next_count > 3) {
        return 1;
    }
    static const uint8_t rgba[16] = { 1, 0, 0, 0, 0, 1, 0, 0,
                                      0, 0, 1, 0, 0, 0, 0, 1 };
    memset(out, 0, sizeof(*out));
    out->rgba = rgba;
    out->rgba_pitch = 16;
    return 0;
}

static void fake_restore(void* user)
{
    (void)user;
    if (s_fake.present_count == 0) {
        s_fake.restore_seen_before_first_present = 1;
    }
    ++s_fake.restore_count;
}

static int fake_present(const MovieOut* out, const MovieInfo* info,
                        const MovieRect* rect, void* user)
{
    (void)out;
    (void)info;
    (void)user;
    if (rect == NULL) {
        s_fake.rect_was_null = 1;
    } else {
        s_fake.last_rect = rect;
    }
    ++s_fake.present_count;
    if (s_fake.stop_at_frame > 0 && s_fake.present_count == s_fake.stop_at_frame) {
        return 1;
    }
    return 0;
}

static int fake_quit(void* user)
{
    (void)user;
    ++s_fake.quit_polls;
    if (s_fake.quit_after > 0 && s_fake.quit_polls >= s_fake.quit_after) {
        return 1;
    }
    return 0;
}

static void fake_pump_reset(void)
{
    memset(&s_fake, 0, sizeof(s_fake));
    s_fake.rect_was_null = 0;
}

static void test_rect_pump(void)
{
    MovieInfo info;
    memset(&info, 0, sizeof(info));
    info.width = 2;
    info.height = 2;

    MoviePump pump;
    memset(&pump, 0, sizeof(pump));
    pump.next = fake_next;
    pump.info = &info;
    pump.present = fake_present;
    pump.quit = fake_quit;
    pump.restore = fake_restore;

    /* --- full-window branch --------------------------------------- */
    fake_pump_reset();
    pump.rect = NULL;
    int frames = movie_video_playback_loop(&pump);
    CHECK(frames == 3, "null-rect pump plays the decoder's frames (3)");
    CHECK(s_fake.present_count == 3 && s_fake.rect_was_null == 1,
          "null-rect present always sees a null rectangle");
    CHECK(s_fake.restore_count == 0,
          "null-rect branch never runs the surface-restore seam");

    /* --- rect branch ---------------------------------------------- */
    fake_pump_reset();
    MovieRect r = { 8, 4, 6, 12 };
    pump.rect = &r;
    frames = movie_video_playback_loop(&pump);
    CHECK(frames == 3, "rect pump plays the same frames");
    CHECK(s_fake.restore_count == 1 &&
              s_fake.restore_seen_before_first_present == 1,
          "the rect branch restores the surface once, before the first frame");
    CHECK(s_fake.last_rect != NULL &&
              s_fake.last_rect->x == 8 && s_fake.last_rect->y == 4 &&
              s_fake.last_rect->w == 6 && s_fake.last_rect->h == 12,
          "the rect reaches the present hook unchanged");
    CHECK(s_fake.rect_was_null == 0, "the rect branch never reports null");
    CHECK(s_fake.quit_polls == 3, "the pump polls quit once per played frame");

    /* --- quit ends the branch mid-stream -------------------------- */
    fake_pump_reset();
    pump.rect = NULL;
    s_fake.quit_after = 2;
    frames = movie_video_playback_loop(&pump);
    CHECK(frames == 2 && s_fake.present_count == 2,
          "a quit between frames leaves the branch");
    CHECK(s_fake.next_count == 2, "the pump stops pulling frames on quit");
    s_fake.quit_after = 0;

    /* --- present failure leaves too (blit-report role) ------------- */
    fake_pump_reset();
    s_fake.stop_at_frame = 2;
    frames = movie_video_playback_loop(&pump);
    CHECK(frames == 1 && s_fake.next_count == 2,
          "a failed present ends the pump after the failing frame was pulled");

    CHECK(movie_video_playback_loop(NULL) == 0, "null pump reports zero frames");

    /* --- the rect scaler itself ------------------------------------ */
    uint8_t src[16] = { 1, 1, 1, 0,  2, 2, 2, 0,  3, 3, 3, 0,  4, 4, 4, 0 };
    uint8_t dst[64];   /* 4x4 destination */
    memset(dst, 0, sizeof(dst));
    MovieRect fill = { 1, 1, 2, 2 };
    movie_scale_rgba_rect(src, 2, 2, 8, dst, 4, 4, 16, &fill);
    CHECK(dst[(1 * 16) + 1 * 4] == 1 && dst[(1 * 16) + 2 * 4] == 2 &&
              dst[(2 * 16) + 1 * 4] == 3 && dst[(2 * 16) + 2 * 4] == 4,
          "the rect blit maps the four source pixels into the destination rect");
    CHECK(dst[0] == 0 && dst[3 * 16 + 3 * 4] == 0,
          "pixels outside the rect are untouched");

    /* clipped rect: only the intersection lands */
    memset(dst, 0, sizeof(dst));
    MovieRect over = { 3, 3, 4, 4 };
    movie_scale_rgba_rect(src, 2, 2, 8, dst, 4, 4, 16, &over);
    CHECK(dst[3 * 16 + 3 * 4] != 0, "clipped rect paints the in-bounds corner");
    CHECK(dst[0] == 0 && dst[1] == 0, "the clip spares the out-of-rect pixels");
}


int main(int argc, char** argv)
{
    printf("== movie ==\n");

    test_config();
    test_scaler();
    test_rect_pump();

    if (movie_available()) {
        printf("movie: FFmpeg runtime present (libavcodec major id in use)\n");
        test_decode_synthetic();
    } else {
        printf("SKIP: FFmpeg runtime absent — decode checks skipped\n");
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--real") == 0 && i + 1 < argc) {
            test_real(argv[i + 1]);
            ++i;
        }
    }

    movie_unload();

    if (s_failures != 0) {
        printf("movie: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("movie: all pass\n");
    return 0;
}
