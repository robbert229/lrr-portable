// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_fps_counter -- the frame rate the FPS overlay shows
 * (platform/native/fps_counter.h): frames over windows of half a second. */
#include "lrr_test.h"
#include "platform/native/fps_counter.h"

int main()
{
    LRR_FpsReset();

    /* Nothing to show until a window is done. */
    CHECK_NEAR(LRR_FpsFrame(1000), 0.0f, 1e-6);
    CHECK_NEAR(LRR_FpsFrame(1100), 0.0f, 1e-6);
    CHECK_NEAR(LRR_FpsFrame(1200), 0.0f, 1e-6);
    CHECK_NEAR(LRR_FpsFrame(1400), 0.0f, 1e-6);

    /* The window closes at 500 ms: the first frame only starts it, so 4
     * frames (1100, 1200, 1400, 1500) in 500 ms. */
    CHECK_NEAR(LRR_FpsFrame(1500), 8.0f, 1e-4);

    /* The rate holds until the next window is done. */
    CHECK_NEAR(LRR_FpsFrame(1600), 8.0f, 1e-4);
    CHECK_NEAR(LRR_FpsFrame(1900), 8.0f, 1e-4);

    /* 30 frames a second: a frame every 33.3 ms, 15 of them in 500 ms. */
    LRR_FpsReset();
    float fps = LRR_FpsFrame(0);
    for (int i = 1; i <= 15; i++) fps = LRR_FpsFrame((unsigned long)(i * 1000 / 30));
    CHECK_NEAR(fps, 30.0f, 0.5);

    /* A slow frame longer than the window still gives a rate. */
    fps = LRR_FpsFrame(500 + 2000);
    CHECK(fps > 0.0f && fps < 3.0f, "a slow frame still gives a rate");

    /* A reset forgets it. */
    LRR_FpsReset();
    CHECK_NEAR(LRR_FpsFrame(5000), 0.0f, 1e-6);

    /* The clock wrapping gives the elapsed time, not a huge number. */
    LRR_FpsReset();
    LRR_FpsFrame(0xfffffff0ul);
    for (int i = 1; i < 10; i++) LRR_FpsFrame(0xfffffff0ul + (unsigned long)(i * 60));
    fps = LRR_FpsFrame(0xfffffff0ul + 600);
    CHECK(fps > 15.0f && fps < 20.0f, "the clock wrapping gives the elapsed time");

    return lrr_test_result("fps_counter");
}
