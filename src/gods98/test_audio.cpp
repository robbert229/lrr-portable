// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_audio — validation for src/gods98/audio.cpp.
 *
 * Runs against the SDL "dummy" audio driver (set in meson test env): opens
 * the audio surface, plays a synthetic sine, and verifies the bank-PCM ->
 * mono-16-bit conversion (8-bit unsigned and stereo-16 -> mono average).
 * With `--real <dir>` it plays an actual shipped .wav through the bank.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "audio.hpp"
#include "resource.hpp"
#include "sound.hpp"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static void test_synthetic(void)
{
    /* A 0.1 s 440 Hz sine at 8 kHz. */
    enum { N = 8000 };
    int16_t sine[N];
    for (int i = 0; i < N; ++i) {
        double t = (double)i / 8000.0;
        sine[i] = (int16_t)(15000 * sin(2 * 3.14159265358979 * 440.0 * t));
    }

    AudioPlayer p = {};
    CHECK(audio_open(&p) == 0, "syn: audio_open");
    CHECK(p.dev != NULL, "syn: device returned");
    CHECK(platform_audio_play(p.dev, sine, N, 8000) == 0, "syn: play sine");

    /* Volume (stream gain) set + readback. */
    CHECK(platform_audio_set_gain(p.dev, 0.35f) == 0, "syn: gain set");
    float g = platform_audio_get_gain(p.dev);
    CHECK(g > 0.30f && g < 0.40f, "syn: gain readback");

    /* Continuous loop: seed + refill keeps the queue fed (dummy driver). */
    static int16_t loopbuf[1600]; /* 0.1 s @ 8 kHz -> stereo */
    for (int i = 0; i < 800; ++i) {
        loopbuf[i * 2]     = (int16_t)(12000 * sin(2 * 3.14159265 * 330.0
                                                  * (double)i / 8000.0));
        loopbuf[i * 2 + 1] = loopbuf[i * 2];
    }
    CHECK(platform_audio_loop(p.dev, loopbuf, 800, 8000) == 0,
          "syn: loop seed");
    platform_audio_refill(p.dev);
    CHECK(platform_audio_queued(p.dev) > 0, "syn: loop queued after refill");
    platform_audio_stop(p.dev);
    CHECK(platform_audio_queued(p.dev) == 0, "syn: loop stopped");

    /* Constant-power stereo pan weights. */
    uint8_t pd[4] = { 0xe8, 0x03, 0xd0, 0x07 }; /* s16 LE: 1000, 2000 */
    Sound ps = {};
    ps.format.channels = 1;
    ps.format.bits_per_sample = 16;
    ps.format.sample_rate = 44100;
    ps.data = pd;
    ps.data_len = 4;
    int16_t* lr = NULL;
    int pf = audio_to_stereo_pan(&ps, -1.0f, &lr);
    CHECK(pf == 2 && lr[0] == 1000 && lr[1] == 0, "syn: pan far-left");
    audio_free_pcm(lr);
    pf = audio_to_stereo_pan(&ps, 1.0f, &lr);
    CHECK(pf == 2 && lr[0] == 0 && lr[1] == 1000, "syn: pan far-right");
    audio_free_pcm(lr);
    pf = audio_to_stereo_pan(&ps, 0.0f, &lr);
    CHECK(pf == 2 && lr[0] == 1000 && lr[1] == 1000, "syn: pan center");
    audio_free_pcm(lr);

    audio_stop(&p);
    audio_close(&p);

    /* 8-bit unsigned mono -> signed 16 mono. */
    uint8_t d8[4] = { 128, 129, 0, 255 };
    Sound s = {};
    s.format.channels = 1;
    s.format.bits_per_sample = 8;
    s.format.sample_rate = 11025;
    s.data = d8;
    s.data_len = 4;
    int16_t* out = NULL;
    int f = audio_to_16mono(&s, &out);
    CHECK(f == 4, "conv: 8-bit frames");
    CHECK(out != NULL && out[0] == 0 && out[1] == 256
          && out[2] == -32768 && out[3] == 32512, "conv: 8-bit values");
    audio_free_pcm(out);

    /* 16-bit stereo -> mono average. */
    uint8_t d16[8] = { 0, 0, 0, 0,  0x80, 0x7f, 0, 0 }; /* (0,0) (32640,0) */
    Sound st = {};
    st.format.channels = 2;
    st.format.bits_per_sample = 16;
    st.format.sample_rate = 44100;
    st.data = d16;
    st.data_len = 8;
    f = audio_to_16mono(&st, &out);
    CHECK(f == 2, "conv: stereo frames");
    CHECK(out != NULL && out[0] == 0 && out[1] == 16320, "conv: stereo avg");
    audio_free_pcm(out);
}

static void test_real(const char* dir)
{
    resource_unmount_all();
    resource_mount(dir, "LegoRR");

    AudioPlayer p = {};
    CHECK(audio_open(&p) == 0, "real: audio_open");
    if (p.dev != NULL) {
        int frames = audio_play_named(&p, "Sounds\\Buildings\\BTHUD.wav");
        CHECK(frames > 0, "real: play shipped wav");
        audio_stop(&p);
    }
    audio_close(&p);
    resource_unmount_all();
}

int main(int argc, char** argv)
{
    test_synthetic();
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--real") == 0 && i + 1 < argc) {
            test_real(argv[i + 1]);
            break;
        }
    }
    if (s_failures == 0) {
        printf("audio_test: PASS%s\n", argc > 2 ? " (real wads)" : "");
        return 0;
    }
    printf("audio_test: %d failure(s)\n", s_failures);
    return 1;
}