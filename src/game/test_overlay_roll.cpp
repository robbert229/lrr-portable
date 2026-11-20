// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_overlay_roll — FrontEnd.c's random menu overlay roll
 * (front_menu_should_random_play, 00412190) on a simulated clock. The port
 * rolls on elapsed time at a 60 Hz reference rate, so overlays start about
 * 60 / 400 = 0.15 times a second at any frame rate.
 *
 * The test links with --wrap=main_time_get, so the unit's calls to Main.c's
 * clock reach __wrap_main_time_get below instead of SDL_GetTicks. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "crt/vc5names.h"

extern "C" int front_menu_should_random_play(void);

static uint64_t s_clockUs = 0;

extern "C" unsigned long __wrap_main_time_get(void)
{
    return (unsigned long)(uint32_t)(s_clockUs / 1000);
}

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

/* Starts per simulated second over `seconds` at `fps` updates a second,
 * rolling every update, as the menu does while no overlay plays. */
static double starts_per_second(double fps, double seconds)
{
    const uint64_t stepUs = (uint64_t)llround(1e6 / fps);
    const uint64_t endUs = s_clockUs + (uint64_t)(seconds * 1e6);
    uint64_t starts = 0;
    uint64_t beginUs;

    crt_srand(12345);
    /* The first roll sees a gap and counts as one frame; settle it first. */
    s_clockUs += 1000000;
    front_menu_should_random_play();
    beginUs = s_clockUs;
    while (s_clockUs < endUs + 1000000) {
        s_clockUs += stepUs;
        starts += front_menu_should_random_play() ? 1 : 0;
    }
    return (double)starts / ((double)(s_clockUs - beginUs) / 1e6);
}

int main(void)
{
    /* 0.15 starts/s; 40000 s gives about 6000 starts, a 1.3% standard error. */
    const double expected = 60.0 / 400.0;
    const double seconds = 40000.0;
    const double fpsList[] = { 30.0, 60.0, 240.0, 1000.0 };
    double rates[4];
    char what[160];
    int i;

    for (i = 0; i < 4; i++) {
        rates[i] = starts_per_second(fpsList[i], seconds);
        printf("%6.0f fps: %.4f starts/s (expected %.4f)\n", fpsList[i], rates[i], expected);
        /* 6% allows about 4 standard errors plus the 1 ms rounding bias. */
        snprintf(what, sizeof(what), "%.0f fps rate %.4f within 6%% of %.4f",
                 fpsList[i], rates[i], expected);
        CHECK(fabs(rates[i] - expected) < 0.06 * expected, what);
    }
    /* At the reference rate the roll is the original's 82-in-32768 roll. */
    snprintf(what, sizeof(what), "60 fps rate %.4f within 4%% of 60/400", rates[1]);
    CHECK(fabs(rates[1] - expected) < 0.04 * expected, what);

    /* A long gap (an overlay playing, another screen) counts as one frame:
     * after 10 minutes away, the next roll starts an overlay about 1 time in
     * 400, not almost always. */
    {
        int trial;
        int starts = 0;
        crt_srand(777);
        for (trial = 0; trial < 20000; trial++) {
            s_clockUs += 600ull * 1000000ull;
            starts += front_menu_should_random_play() ? 1 : 0;
        }
        snprintf(what, sizeof(what), "gap rolls start %d of 20000 (about 50)", starts);
        CHECK(starts > 20 && starts < 90, what);
    }

    /* Rolls within the same millisecond never start one. */
    {
        int trial;
        int starts = 0;
        s_clockUs += 1000000;
        front_menu_should_random_play();
        for (trial = 0; trial < 100000; trial++) {
            starts += front_menu_should_random_play() ? 1 : 0;
        }
        CHECK(starts == 0, "same-millisecond rolls start nothing");
    }

    if (s_failures == 0) {
        printf("overlay_roll: all checks passed\n");
        return 0;
    }
    return 1;
}
