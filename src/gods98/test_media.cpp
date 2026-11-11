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
#include "audio.hpp"
#include "winmm.h"

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

/* The CD player is the original-shape soundsys.c (Sound_PlayCDTrack,
 * Sound_Update, Play/Stop/Status_CDTrack). Its MCI commands go to
 * winmm_port.cpp's mciSendStringA, whose test hook stands in for the
 * "cdaudio" device; the hook also answers the status queries. */
static int cd_open_calls, cd_play_calls, cd_stop_calls, cd_close_calls;
static int cd_status_calls, cd_last_track, cd_fail_stop, cd_fail_open;
static int cd_playing_track; /* 1-based track the disc reports; 0 = stopped */
static uint32_t cd_time;
static MCIERROR cd_mci(LPCSTR command, LPSTR result, UINT capacity)
{
    (void)capacity;
    if (!strcmp(command, "open cdaudio")) {
        ++cd_open_calls;
        return cd_fail_open ? MCIERR_INVALID_DEVICE_NAME : 0;
    }
    if (!strncmp(command, "play cdaudio from ", 18)) {
        ++cd_play_calls;
        cd_last_track = atoi(command + 18);
        return 0;
    }
    if (!strcmp(command, "stop cdaudio")) {
        ++cd_stop_calls;
        return cd_fail_stop ? 1 : 0;
    }
    if (!strcmp(command, "close cdaudio")) { ++cd_close_calls; return 0; }
    if (!strcmp(command, "status cdaudio mode")) {
        ++cd_status_calls;
        strcpy(result, cd_playing_track != 0 ? "playing" : "stopped");
        return 0;
    }
    if (!strcmp(command, "status cdaudio current track")) {
        sprintf(result, "%d", cd_playing_track);
        return 0;
    }
    return 0; /* "set cdaudio time format tmsf" */
}
static DWORD cd_clock(void) { return cd_time; }
static int cd_audio_init(void* context, void* window)
{
    (void)context; (void)window;
    return 1;
}

static void test_cd_watchdog(void)
{
    winmm_port_mci_hook_set(cd_mci);
    winmm_port_time_hook_set(cd_clock);
    audio_system_init_hook_set(cd_audio_init, NULL);
    media_cd_config_set(2, 3);

    CHECK(media_cd_audio_device_state_update(1) == 1, "nosound initialises");
    media_cd_watchdog_update(1);
    CHECK(cd_open_calls == 0 && cd_play_calls == 0, "sound off does not play");
    CHECK((media_cd_flags_get() & MEDIA_CD_WATCHDOG_BIT) != 0, "enable sets shared bit");
    CHECK(media_timer_pending() == 0, "track index is not a timer");

    CHECK(media_cd_audio_device_state_update(0) == 1,
          "sound on starts 3D sound on the main window");
    media_cd_watchdog_update(1);
    CHECK(cd_open_calls == 1 && cd_play_calls == 1, "enable opens and plays");
    CHECK(cd_last_track >= 2 && cd_last_track <= 4, "configured track range reaches device");

    cd_time = 5000;
    media_cd_audio_watchdog_tick(0);
    media_cd_audio_watchdog_tick(1);
    CHECK(cd_status_calls == 0 && cd_play_calls == 1,
          "disabled tick advances the poll timestamp without polling");

    cd_playing_track = cd_last_track;
    cd_time = 9001;
    media_cd_audio_watchdog_tick(1);
    CHECK(cd_status_calls == 1 && cd_play_calls == 1 && cd_stop_calls == 0,
          "a track still playing is left alone");

    cd_playing_track = 0;
    cd_time = 13002;
    media_cd_audio_watchdog_tick(1);
    /* Sound_Initialise leaves the loop flag at -1, so the ended track is
     * replayed before the callback stops the disc and starts a new one. */
    CHECK(cd_status_calls == 2 && cd_stop_calls == 1 && cd_close_calls == 1 &&
              cd_play_calls == 3 && cd_open_calls == 2,
          "completion replays, then stops and starts a newly selected track");

    media_cd_watchdog_update(0);
    CHECK((media_cd_flags_get() & MEDIA_CD_WATCHDOG_BIT) == 0 && cd_stop_calls == 2 && cd_close_calls == 2,
          "disable clears bit and stops and closes");
    cd_fail_stop = 1;
    media_cd_watchdog_update(0);
    CHECK(cd_stop_calls == 3 && cd_close_calls == 2, "failed stop does not close");
    cd_fail_stop = 0;
    cd_fail_open = 1;
    media_cd_watchdog_update(1);
    CHECK(cd_open_calls == 3 && cd_play_calls == 3, "failed open does not play");
    cd_fail_open = 0;

    (void)media_cd_audio_device_state_update(1);
    winmm_port_mci_hook_set(NULL);
    winmm_port_time_hook_set(NULL);
    audio_system_init_hook_set(NULL, NULL);
}

/* ------------------------------------------------------------------ *
 *  Minimum-distance state machine (issue #302)
 * ------------------------------------------------------------------ */

static void test_min_distance(void)
{
    int ctx_a = 1;
    int ctx_b = 2;
    media_min_distance_config_set(2.0f, 17.0f, 150.0f, 9.0f);

    /* Mode 0, selector 0: store the context, apply 0.9 / base*cell / 50. */
    media_min_distance_update_mode(0, &ctx_a, 0);
    CHECK(media_min_distance_mode() == 0, "mode 0 records mode 0");
    CHECK(media_min_distance_context() == &ctx_a, "mode 0 stores the context");
    CHECK(float_eq(media_min_distance_listener_scale(), 0.9f),
          "selector 0 applies the 0.9 scale");
    CHECK(float_eq(media_min_distance_clip_distance(), 18.0f),
          "mode 0 clip multiplies the min-distance base by the cell scale");
    CHECK(float_eq(media_min_distance_ref_distance(), 50.0f),
          "mode 0 applies the 50.0 reference distance");

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
