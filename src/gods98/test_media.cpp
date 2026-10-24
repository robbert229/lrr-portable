// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_media — validation for src/gods98/media.cpp.
 *
 * Covers the one-shot scheduler, the CD-audio watchdog state machine
 * (CDAudioWatchdogModeUpdate @ 0x4297b0, issue #301), and the
 * minimum-distance state machine (SoundMinimumDistanceUpdateMode @
 * 0x4295f0, issue #302). The watchdog draws are mirrored with the same
 * LCG the port models (the corpus's shared random word role), advanced
 * in lockstep with every module-side draw.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "media.hpp"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

static int float_eq(float a, float b)
{
    float d = a - b;
    if (d < 0) {
        d = -d;
    }
    return d < 1e-5f;
}

/* ------------------------------------------------------------------ *
 *  Scheduler
 * ------------------------------------------------------------------ */

static int s_fire_count;

static void fire_cb(void* user)
{
    ++s_fire_count;
    (void)user;
}

static void test_timer(void)
{
    s_fire_count = 0;
    media_timer_cancel();
    CHECK(media_timer_pending() == 0, "timer starts empty");

    CHECK(media_timer_schedule(2.0f, fire_cb, NULL) == 1, "schedule reports 1");
    CHECK(media_timer_pending() == 1, "pending after schedule");
    media_timer_tick(1.0f);
    CHECK(media_timer_pending() == 1 && s_fire_count == 0, "not due early");
    media_timer_tick(1.0f);
    CHECK(s_fire_count == 1, "fires once when due");
    CHECK(media_timer_pending() == 0, "one-shot clears after firing");
    media_timer_tick(1.0f);
    CHECK(s_fire_count == 1, "does not refire");

    /* The corpus's scheduled entry is overwritten by the next schedule. */
    media_timer_schedule(5.0f, fire_cb, NULL);
    media_timer_schedule(0.5f, fire_cb, NULL);
    int fired_before = s_fire_count;
    media_timer_tick(1.0f);
    CHECK(s_fire_count == fired_before + 1,
          "the later schedule replaces the earlier");
    media_timer_tick(1.0f);
    CHECK(s_fire_count == fired_before + 1, "the fired one-shot stays done");

    /* Cancel drops the pending callback (the corpus's cancel-everything). */
    media_timer_schedule(0.5f, fire_cb, NULL);
    media_timer_cancel();
    media_timer_tick(1.0f);
    CHECK(s_fire_count == fired_before + 1, "cancel drops the pending callback");
    CHECK(media_timer_pending() == 0, "cancel clears pending");
}

/* ------------------------------------------------------------------ *
 *  CD watchdog state machine (issue #301)
 * ------------------------------------------------------------------ */

/* Mirror of the port's module-local 15-bit draw; advance exactly once
 * per module-side draw. */
static uint32_t s_cd_mirror = 1;

static unsigned s_cd_mirror_draw(void)
{
    s_cd_mirror = s_cd_mirror * 214013u + 2531011u;
    return (s_cd_mirror >> 16) & 0x7fffu;
}

/* Slot arithmetic exactly as the port/corpus compute it:
 * draw % window - 1 + base, unsigned. */
static int s_cd_mirror_slot(int base, int window)
{
    unsigned d = s_cd_mirror_draw();
    return (int)(d % (unsigned)window - 1u + (unsigned)base);
}

static void test_cd_watchdog(void)
{
    s_cd_mirror = 1;
    media_cd_config_set(5, 4);          /* base 5, window 4 */
    media_cd_track_set_dead(0);

    /* Enable draws the time-derived slot and schedules the re-arm. */
    int slot1 = s_cd_mirror_slot(5, 4);
    media_cd_watchdog_update(1);
    CHECK((media_cd_flags_get() & MEDIA_CD_WATCHDOG_BIT) != 0,
          "enable raises the watchdog bit");
    CHECK(media_cd_slot() == slot1, "enable draws the time-derived slot");
    CHECK(media_timer_pending() == 1, "enable schedules the re-arm");

    /* Running the timer to its due fires the re-arm, which draws again
     * and re-schedules — the watchdog callback keeps the state armed. */
    media_timer_tick((float)slot1 + 1.0f);
    int slot2 = s_cd_mirror_slot(5, 4);
    CHECK(media_cd_slot() == slot2, "the due re-arm refreshes the slot");
    CHECK(media_timer_pending() == 1, "the re-arm keeps the timer pending");
    CHECK((media_cd_flags_get() & MEDIA_CD_WATCHDOG_BIT) != 0,
          "re-arming keeps the watchdog bit set");

    /* Disable clears the bit, cancels, and runs one no-op tick with 0. */
    media_cd_watchdog_update(0);
    CHECK((media_cd_flags_get() & MEDIA_CD_WATCHDOG_BIT) == 0,
          "disable clears the watchdog bit");
    CHECK(media_timer_pending() == 0, "disable cancels the scheduled tick");

    /* Loop flag is stale as shipped: only set, never cleared. */
    media_cd_loop_set(1);
    CHECK(media_cd_loop_get() == 1, "loop flag raised by loop schedules");
    media_cd_loop_set(0);
    CHECK(media_cd_loop_get() == 1, "the loop flag is never cleared");

    /* Stalled track: the tick's re-arm runs without waiting for the
     * timer, and clears the dead flag. */
    media_cd_config_set(5, 4);
    int slot3 = s_cd_mirror_slot(5, 4);
    media_cd_watchdog_update(1);
    media_cd_track_set_dead(1);
    media_cd_watchdog_tick(1);
    int slot4 = s_cd_mirror_slot(5, 4);
    CHECK(media_cd_slot() == slot4, "the stalled tick re-arms with a new draw");
    CHECK(media_timer_pending() == 1, "the stalled re-arm keeps the timer");
    media_cd_watchdog_tick(1);          /* alive again: nothing to do */
    CHECK(media_cd_slot() == slot4, "watching a live track draws nothing");

    /* Tick with 0 (the disable path) touches nothing. */
    media_cd_watchdog_tick(0);
    CHECK(media_cd_slot() == slot4, "the 0-argument tick is inert");
    media_cd_watchdog_update(0);

    /* Corpus config defaults: zero start lands as 2, zero window as 1 —
     * slot 1 with window 1. */
    media_cd_config_set(0, 0);
    int slot5 = s_cd_mirror_slot(2, 1);
    media_cd_watchdog_update(1);
    CHECK(media_cd_slot() == slot5, "config zero-to-default rules apply");
    media_cd_watchdog_update(0);
    CHECK(media_timer_pending() == 0 &&
              (media_cd_flags_get() & MEDIA_CD_WATCHDOG_BIT) == 0,
          "disable leaves nothing armed");

    (void)slot1;
    (void)slot3;
}

/* ------------------------------------------------------------------ *
 *  Minimum-distance state machine (issue #302)
 * ------------------------------------------------------------------ */

static void test_min_distance(void)
{
    int ctx_a = 1;
    int ctx_b = 2;
    media_min_distance_config_set(2.0f, 17.0f, 150.0f, 9.0f);

    /* Mode 0, selector 0: store the context, apply 0.9 / base*cell / 5. */
    media_min_distance_update_mode(0, &ctx_a, 0);
    CHECK(media_min_distance_mode() == 0, "mode 0 records mode 0");
    CHECK(media_min_distance_context() == &ctx_a, "mode 0 stores the context");
    CHECK(float_eq(media_min_distance_listener_scale(), 0.9f),
          "selector 0 applies the 0.9 scale");
    CHECK(float_eq(media_min_distance_clip_distance(), 18.0f),
          "mode 0 clip multiplies the min-distance base by the cell scale");
    CHECK(float_eq(media_min_distance_ref_distance(), 5.0f),
          "mode 0 applies the 5.0 reference distance");

    /* Repeated entry without an intervening mode 1: the newer context
     * and scalar simply replace what was stored. */
    media_min_distance_update_mode(0, &ctx_b, 1);
    CHECK(media_min_distance_context() == &ctx_b,
          "repeated mode 0 stores the newer context");
    CHECK(float_eq(media_min_distance_listener_scale(), 0.6f),
          "selector 1 applies the 0.6 scale");

    /* Mode 1: clears the context and restores the config previous values. */
    media_min_distance_update_mode(1, NULL, 0);
    CHECK(media_min_distance_mode() == 1, "mode 1 records mode 1");
    CHECK(media_min_distance_context() == NULL, "mode 1 clears the context");
    CHECK(float_eq(media_min_distance_listener_scale(), 0.5f),
          "mode 1 restores the 0.5 scale");
    CHECK(float_eq(media_min_distance_clip_distance(), 17.0f),
          "mode 1 restores the config clip distance");
    CHECK(float_eq(media_min_distance_ref_distance(), 150.0f),
          "mode 1 restores the config reference distance");

    /* Mode 1 with nothing stored: same visible result. */
    media_min_distance_update_mode(1, NULL, 0);
    CHECK(media_min_distance_context() == NULL &&
              media_min_distance_mode() == 1,
          "mode 1 without a stored context still clears");

    /* Unknown modes update only the mode global. */
    media_min_distance_update_mode(5, NULL, 1);
    CHECK(media_min_distance_mode() == 5, "unknown mode updates the global");
    CHECK(float_eq(media_min_distance_listener_scale(), 0.5f),
          "unknown mode leaves the applied values alone");

    /* NULL context in mode 0 is stored and applied (the corpus
     * dereferences unchecked — the port guards). */
    media_min_distance_update_mode(0, NULL, 0);
    CHECK(media_min_distance_context() == NULL &&
              float_eq(media_min_distance_listener_scale(), 0.9f),
          "mode 0 with a NULL context still applies the scalars");

    /* Mode 1 restores follow whatever the config holds. */
    media_min_distance_config_set(1.0f, 11.0f, 144.0f, 3.0f);
    media_min_distance_update_mode(1, NULL, 0);
    CHECK(float_eq(media_min_distance_clip_distance(), 11.0f) &&
              float_eq(media_min_distance_ref_distance(), 144.0f),
          "mode 1 restores the live config values");
}

int main(void)
{
    test_timer();
    test_cd_watchdog();
    test_min_distance();

    if (s_failures != 0) {
        printf("media_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("media_test: PASS\n");
    return 0;
}
