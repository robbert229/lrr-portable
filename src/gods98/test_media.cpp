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
#include "sound.hpp"

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

/* ------------------------------------------------------------------ *
 *  AudioPlaybackToggle (issue #298)
 * ------------------------------------------------------------------ */

static void test_playback_toggle(void)
{
    media_audio_playback_toggle(1);
    CHECK((media_cd_flags_get() & MEDIA_FLAG_SOUND_ON) != 0,
          "enable raises the sound-on bit");
    CHECK(sound_subsystem_enabled() == 1, "enable re-enables the subsystem");

    media_audio_playback_toggle(1);
    CHECK((media_cd_flags_get() & MEDIA_FLAG_SOUND_ON) != 0 &&
              sound_subsystem_enabled() == 1,
          "repeated enable is idempotent");

    media_audio_playback_toggle(0);
    CHECK((media_cd_flags_get() & MEDIA_FLAG_SOUND_ON) == 0,
          "clearing drops the sound-on bit");
    CHECK(sound_subsystem_enabled() == 0, "clearing mutes the subsystem");
    media_audio_playback_toggle(0);
    CHECK((media_cd_flags_get() & MEDIA_FLAG_SOUND_ON) == 0 &&
              sound_subsystem_enabled() == 0,
          "repeated disable is idempotent");

    media_audio_playback_toggle(1);   /* leave enabled for later tests */
    CHECK(sound_subsystem_enabled() == 1, "enable leaves the subsystem on");
}

/* ------------------------------------------------------------------ *
 *  Cue dispatch + cursor override (issue #300)
 * ------------------------------------------------------------------ */

static void test_cue_dispatch(void)
{
    sound_batch_queue_drain(0, 0);                 /* start unbatched */
    media_pointer_cursor_override_set(0, 0.0f);    /* clear override */

    /* Known event, batch off: override lands on the mapped slot, mode
     * stays off, and the unset cue alias plays through with no draw. */
    media_action_feedback_sound_dispatch(2);
    CHECK(media_pointer_cursor_slot() == 0x15, "event 2 maps slot 0x15");
    CHECK(float_eq(media_pointer_cursor_override(), 12.5f),
          "the dispatch sets the 12.5 override value");
    CHECK(sound_batch_mode_get() == 0, "the dispatch restores batch off");

    /* The active override blocks plain selects until cleared. */
    media_pointer_cursor_select(1);
    CHECK(media_pointer_cursor_slot() == 0x15,
          "an active override blocks the cursor select");
    media_pointer_cursor_override_set(0x21, 0.0f);
    media_pointer_cursor_select(7);
    CHECK(media_pointer_cursor_slot() == 7, "a zero override reopens selects");

    /* Batch armed with one deferred record: the dispatch forces the mode
     * off WITHOUT replaying the deferred table, then restores it. */
    sound_batch_queue_drain(1, 0);                 /* raise mode */
    sound_alias_play(13, 0);                       /* unset alias defers */
    CHECK(sound_batch_queue_count() == 1, "the unset alias deferred");
    media_action_feedback_sound_dispatch(6);       /* unknown event */
    CHECK(sound_batch_queue_count() == 1,
          "the unknown event neither replays nor clears deferred records");
    CHECK(sound_batch_mode_get() != 0,
          "the unknown event still restores the saved mode");
    media_action_feedback_sound_dispatch(0);       /* known event */
    CHECK(sound_batch_queue_count() == 1 &&
              sound_batch_mode_get() != 0 &&
              media_pointer_cursor_slot() == 0x14,
          "the known event plays without touching the deferred table");

    /* Drain the deferred table through the replay so nothing leaks. */
    sound_batch_queue_drain(0, 1);
    CHECK(sound_batch_queue_count() == 0 && sound_batch_mode_get() == 0,
          "cleanup replay clears the batch table");

    /* Action codes route through the select, not the override. */
    media_pointer_cursor_override_set(0, 0.0f);
    media_action_code_to_sound_alias(3);
    CHECK(media_pointer_cursor_slot() == 0x2d, "code 3 selects cue 0x2d");
    media_action_code_to_sound_alias(11);
    CHECK(media_pointer_cursor_slot() == 0x35, "code 11 selects cue 0x35");
    media_action_code_to_sound_alias(7);
    CHECK(media_pointer_cursor_slot() == 0x35, "unmapped codes do nothing");

    media_pointer_cursor_override_set(99, 12.5f);
    media_action_code_to_sound_alias(1);
    CHECK(media_pointer_cursor_slot() == 99,
          "mapped codes cannot overrule an active override");
    media_pointer_cursor_override_set(0, 0.0f);
}

int main(void)
{
    test_timer();
    test_playback_toggle();
    test_cue_dispatch();
    test_cd_watchdog();
    test_min_distance();

    /* The loader-progress setter pair (2026-10-24): stores, peeks, clears. */
    {
        loader_progress_callback_set(NULL, NULL);
        CHECK(loader_progress_callback_peek() == NULL
                  && loader_progress_context_peek() == NULL,
              "a clear store answers clear");
        loader_progress_callback_set((void*)1, (void*)2);
        CHECK(loader_progress_callback_peek() == (void*)1
                  && loader_progress_context_peek() == (void*)2,
              "both words store and read back");
        loader_progress_callback_set(NULL, NULL);
    }

    if (s_failures != 0) {
        printf("media_test: %d failure(s)\n", s_failures);
        return 1;
    }
    printf("media_test: PASS\n");
    return 0;
}
