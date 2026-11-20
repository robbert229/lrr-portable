// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_menu_frame_rate — FrontEnd.c's menu edge scroll
 * (front_menu_update_mouse_position, 00413970) and Reward.c's credits scroll
 * (reward_scroll_update, 00462ad0) on a simulated clock. The original stepped
 * both once per frame at its vblank-bound 60 Hz; the port scales the step by
 * elapsed time, so a simulated second moves the same distance at 30, 60, 240
 * and 1000 updates a second.
 *
 * The text cursor blink in front_menu_update (00412c00) is not driven here:
 * that function lays out and draws a whole menu.
 *
 * The test links with --wrap=main_time_get, so the units' calls to Main.c's
 * clock reach __wrap_main_time_get below instead of SDL_GetTicks. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "frontend_record.h"
#include "frontend_api.h"
#include "reward.h"
#include "gods98/images.h"
#include "gods98/input_globs.h"
#include "gods98/input_key_names.h"
#include "lego_globs.h"

extern "C" void front_menu_update_mouse_position(Front_Menu* menu, float elapsed);
extern "C" int reward_scroll_update(RewardScroll* scroll);
extern "C" uint8_t s_global_current_keys[256]; /* inputKeyMap */

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

static const double kFps[] = { 30.0, 60.0, 240.0, 1000.0 };

/* Pixels the menu scrolls in `seconds` with the mouse at msy, starting from
 * offset `start`, as front_run_screen_menu calls it: elapsed is the integer
 * milliseconds since the previous update times 0.025. */
static int menu_scroll_distance(double fps, int msy, int start, double seconds)
{
    Image image;
    Front_Menu menu;
    const uint64_t stepUs = (uint64_t)llround(1e6 / fps);
    uint64_t endUs;
    uint32_t lastMs;
    uint32_t nowMs;
    int settled;

    memset(&image, 0, sizeof(image));
    memset(&menu, 0, sizeof(menu));
    image.height = 4800;
    menu.menuImage = &image;
    menu.flags = 2;
    inputGlobs.mslb = 0;
    inputGlobs.msrb = 0;

    /* Settle the carry with the mouse in the dead band, then start. */
    inputGlobs.msy = 240;
    frontGlobs.scrollOffsetY = start;
    front_menu_update_mouse_position(&menu, 1.0f);
    inputGlobs.msy = (unsigned int)msy;
    lastMs = (uint32_t)(s_clockUs / 1000);
    endUs = s_clockUs + (uint64_t)(seconds * 1e6);
    settled = frontGlobs.scrollOffsetY;
    while (s_clockUs + stepUs / 2 < endUs) {
        s_clockUs += stepUs;
        nowMs = (uint32_t)(s_clockUs / 1000);
        front_menu_update_mouse_position(&menu, (float)(nowMs - lastMs) * 0.025f);
        lastMs = nowMs;
    }
    return frontGlobs.scrollOffsetY - settled;
}

/* Pixels the reward text scrolls up in `seconds` at `speed`. */
static double reward_scroll_distance(double fps, float speed, double seconds)
{
    RewardScroll scroll;
    const uint64_t stepUs = (uint64_t)llround(1e6 / fps);
    uint64_t endUs;
    float start;

    memset(&scroll, 0, sizeof(scroll));
    scroll.speed = speed;
    s_panel_highlight_flags = 0;
    /* The first call sees a gap and counts as one frame; settle it first. */
    s_clockUs += 1000000;
    reward_scroll_update(&scroll);
    start = scroll.area.y;
    endUs = s_clockUs + (uint64_t)(seconds * 1e6);
    while (s_clockUs + stepUs / 2 < endUs) {
        s_clockUs += stepUs;
        reward_scroll_update(&scroll);
    }
    return (double)(start - scroll.area.y);
}

/* The speed the reward debug key 0x0d (+) builds from 0 in `seconds`. */
static double reward_speed_after_key(double fps, double seconds)
{
    RewardScroll scroll;
    const uint64_t stepUs = (uint64_t)llround(1e6 / fps);
    uint64_t endUs;

    memset(&scroll, 0, sizeof(scroll));
    s_panel_highlight_flags = 0;
    s_clockUs += 1000000;
    reward_scroll_update(&scroll);
    s_panel_highlight_flags = 0x10;
    s_global_current_keys[0x0d] = 1;
    endUs = s_clockUs + (uint64_t)(seconds * 1e6);
    while (s_clockUs + stepUs / 2 < endUs) {
        s_clockUs += stepUs;
        reward_scroll_update(&scroll);
    }
    s_global_current_keys[0x0d] = 0;
    s_panel_highlight_flags = 0;
    return (double)scroll.speed;
}

int main(void)
{
    char what[200];
    int i;

    /* Menu edge scroll. msy 50: speed (50 * 0.2083 - 50)^2 * 0.02 = 31.3,
     * negated above 240, a (int)(31.3 * 0.25) = 7 px step: 420 px/s at 60 Hz.
     * msy 400: speed 22.2, a (int)(-5.6) = -5 px step: -300 px/s. */
    for (i = 0; i < 4; i++) {
        int up = menu_scroll_distance(kFps[i], 50, -4000, 5.0);
        int down = menu_scroll_distance(kFps[i], 400, -100, 5.0);
        printf("%6.0f fps: menu scroll %+d px up, %+d px down in 5 s (expected +2100, -1500)\n",
               kFps[i], up, down);
        snprintf(what, sizeof(what), "%.0f fps menu scroll up %d within 1%% of 2100", kFps[i], up);
        CHECK(fabs(up - 2100.0) <= 21.0, what);
        snprintf(what, sizeof(what), "%.0f fps menu scroll down %d within 1%% of -1500", kFps[i], down);
        CHECK(fabs(down + 1500.0) <= 15.0, what);
    }

    /* Reward scroll: speed 1.5 px per 60 Hz frame is 90 px/s. */
    for (i = 0; i < 4; i++) {
        double d = reward_scroll_distance(kFps[i], 1.5f, 5.0);
        double v = reward_speed_after_key(kFps[i], 1.0);
        printf("%6.0f fps: reward scroll %.1f px in 5 s (expected 450), "
               "+ key held 1 s gives speed %.3f (expected 0.600)\n", kFps[i], d, v);
        snprintf(what, sizeof(what), "%.0f fps reward scroll %.1f within 1%% of 450", kFps[i], d);
        CHECK(fabs(d - 450.0) <= 4.5, what);
        snprintf(what, sizeof(what), "%.0f fps reward speed %.3f within 3%% of 0.6", kFps[i], v);
        CHECK(fabs(v - 0.6) <= 0.018, what);
    }

    if (s_failures == 0) {
        printf("menu_frame_rate: all checks passed\n");
        return 0;
    }
    return 1;
}
