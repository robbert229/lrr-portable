// SPDX-License-Identifier: LGPL-3.0-or-later
/* winapi platform glue — reference-build stubs.
 *
 * The winapi adapter (ADR 0004) is the A/B reference build: it must compile
 * the same source against real DirectX headers. Real window creation
 * (RegisterClassEx/CreateWindowEx) and real DDraw Flip are deferred until
 * the reference build is wired into the A.2 harness — these stubs keep the
 * seam compiling on that backend.
 */
#include "platform/platform.h"

// STUB: LEGORR 0x004783f0
PlatformWindow* platform_create_window(const char* title, int width, int height)
{
    (void)title; (void)width; (void)height;
    return NULL;
}

void platform_destroy_window(PlatformWindow* win)
{
    (void)win;
}

void* platform_window_hwnd(PlatformWindow* win)
{
    (void)win;
    return NULL;
}

int platform_window_width(PlatformWindow* win)
{
    (void)win;
    return 0;
}

int platform_window_height(PlatformWindow* win)
{
    (void)win;
    return 0;
}

int platform_poll_events(PlatformWindow* win)
{
    (void)win;
    return 1;
}

// STUB: LEGORR 0x00478040
int platform_pump_events(PlatformWindow* win, PlatformEvent* out, int cap)
{
    (void)win; (void)out; (void)cap;
    return 0;
}

// STUB: LEGORR 0x0047c8d0
void platform_present_pixels(PlatformWindow* win, const void* pixels, int pitch,
                             int width, int height)
{
    (void)win; (void)pixels; (void)pitch; (void)width; (void)height;
}

void platform_screenshot(PlatformWindow* win, const void* pixels, int pitch,
                         int width, int height, int frame)
{
    (void)win; (void)pixels; (void)pitch; (void)width; (void)height;
    (void)frame;
}

PlatformAudio* platform_audio_open(void)
{
    return NULL;
}

void platform_audio_close(PlatformAudio* a)
{
    (void)a;
}

int platform_audio_play(PlatformAudio* a, const int16_t* pcm, int frames,
                        int sample_rate)
{
    (void)a; (void)pcm; (void)frames; (void)sample_rate;
    return -1;
}

int platform_audio_play_stereo(PlatformAudio* a, const int16_t* lr,
                               int frames, int sample_rate)
{
    (void)a; (void)lr; (void)frames; (void)sample_rate;
    return -1;
}

int platform_audio_loop(PlatformAudio* a, const int16_t* lr,
                        int frames, int sample_rate)
{
    (void)a; (void)lr; (void)frames; (void)sample_rate;
    return -1;
}

void platform_audio_refill(PlatformAudio* a)
{
    (void)a;
}

int platform_audio_set_gain(PlatformAudio* a, float gain)
{
    (void)a; (void)gain;
    return -1;
}

float platform_audio_get_gain(PlatformAudio* a)
{
    (void)a;
    return 0.0f;
}

int platform_audio_queued(PlatformAudio* a)
{
    (void)a;
    return 0;
}

void platform_audio_stop(PlatformAudio* a)
{
    (void)a;
}
