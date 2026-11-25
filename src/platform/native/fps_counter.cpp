// SPDX-License-Identifier: LGPL-3.0-or-later
/* See fps_counter.h. */
#include "platform/native/fps_counter.h"

namespace {

bool s_started = false;
unsigned long s_windowStart = 0;
unsigned long s_frames = 0;
float s_fps = 0.0f;

} // namespace

extern "C" {

float LRR_FpsFrame(unsigned long nowMs)
{
    if (!s_started) {
        s_started = true;
        s_windowStart = nowMs;
        s_frames = 0;
        return s_fps;
    }
    s_frames++;
    /* Unsigned, so a wrapped clock still gives the elapsed time. */
    unsigned long elapsed = (nowMs - s_windowStart) & 0xffffffffu;
    if (elapsed >= LRR_FPS_WINDOW_MS) {
        s_fps = (float)s_frames * 1000.0f / (float)elapsed;
        s_frames = 0;
        s_windowStart = nowMs;
    }
    return s_fps;
}

void LRR_FpsReset(void)
{
    s_started = false;
    s_frames = 0;
    s_fps = 0.0f;
}

} /* extern "C" */
