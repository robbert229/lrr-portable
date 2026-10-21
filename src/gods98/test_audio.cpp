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