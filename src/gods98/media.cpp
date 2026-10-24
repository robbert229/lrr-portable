// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — media-playback state machines. See media.hpp. */

#include "media.hpp"

#include <stddef.h>

#include "sound.hpp"

/* ------------------------------------------------------------------ *
 *  One-shot timer. The corpus schedules exactly one pending callback
 *  (the CD watchdog's re-arm), so the port's table is a single slot;
 *  a second schedule overwrites the first, like the corpus's overwrite
 *  of its scheduled entry.
 * ------------------------------------------------------------------ */

typedef struct MediaTimer {
    int               active;
    float             remaining;
    MediaTimerCallback cb;
    void*             user;
} MediaTimer;

static MediaTimer s_timer;

int media_timer_schedule(float delay, MediaTimerCallback cb, void* user)
{
    s_timer.active = 1;
    s_timer.remaining = delay;
    s_timer.cb = cb;
    s_timer.user = user;
    return 1;
}

void media_timer_cancel(void)
{
    s_timer.active = 0;
    s_timer.cb = NULL;
    s_timer.user = NULL;
}

void media_timer_tick(float dt)
{
    if (s_timer.active == 0) {
        return;
    }
    s_timer.remaining -= dt;
    while (s_timer.remaining <= 0.0f && s_timer.cb != NULL) {
        /* One-shot: fire exactly once; re-scheduling from the callback
         * (the re-arm path) re-arms through media_timer_schedule. */
        MediaTimerCallback cb = s_timer.cb;
        void* user = s_timer.user;
        s_timer.cb = NULL;
        s_timer.user = NULL;
        s_timer.remaining = 0.0f;
        cb(user);
        if (s_timer.cb != NULL) {
            break; /* re-armed: stop the catch-up walk */
        }
    }
    if (s_timer.cb == NULL) {
        s_timer.active = 0;
    }
}

int media_timer_pending(void)
{
    return s_timer.active;
}

float media_timer_remaining(void)
{
    return s_timer.active ? s_timer.remaining : 0.0f;
}

/* ------------------------------------------------------------------ *
 *  CDAudioWatchdogModeUpdate @ 0x4297b0 (issue #301).
 * ------------------------------------------------------------------ */

static uint32_t s_cd_flags;        /* DAT_00556ecc role, module-local */
static int      s_cd_base  = 2;    /* DAT_00556f0c: CDStartTrack (2 default) */
static int      s_cd_window = 1;   /* DAT_00556f10: CDTracks (1 default) */
static int      s_cd_loop;         /* DAT_00544a04 role (stale: set only by
                                    * the "loop start" schedule argument the
                                    * watchdog never passes) */
static unsigned s_cd_slot;         /* last derived slot */
static int      s_cd_track_dead;   /* port analog of the track-dead check */

/* PRNG: the corpus's shared random word draw (thunk_FUN_0048e170),
 * modelled module-locally the way level.cpp and frontend.cpp do. */
static uint32_t s_cd_rand_state = 1;

static int s_cd_rand15(void)
{
    s_cd_rand_state = s_cd_rand_state * 214013u + 2531011u;
    return (int)((s_cd_rand_state >> 16) & 0x7fffu);
}

/* Time-derived slot: (uint)draw % window - 1 + base. The corpus's
 * unsigned wrap is kept: when draw % window == 0 the slot lands huge
 * and the timer effectively never fires. */
static unsigned s_cd_draw_slot(void)
{
    unsigned draw = (unsigned)s_cd_rand15();
    return draw % (unsigned)s_cd_window - 1u + (unsigned)s_cd_base;
}

static void s_cd_rearm(void* user)
{
    (void)user;
    s_cd_slot = s_cd_draw_slot();
    media_timer_schedule((float)s_cd_slot, s_cd_rearm, NULL);
}

void media_cd_config_set(int start_track_base, int track_count)
{
    /* Corpus zero-to-default rules from config load: an unset/zero
     * CDStartTrack lands as 2, zero CDTracks lands as 1. */
    s_cd_base = start_track_base != 0 ? start_track_base : 2;
    s_cd_window = track_count != 0 ? track_count : 1;
}

void media_cd_watchdog_update(int enable)
{
    if (enable != 0) {
        s_cd_flags |= MEDIA_CD_WATCHDOG_BIT;
        s_cd_slot = s_cd_draw_slot();
        media_timer_schedule((float)s_cd_slot, s_cd_rearm, NULL);
        return;
    }
    s_cd_flags &= ~MEDIA_CD_WATCHDOG_BIT;
    media_timer_cancel();   /* the corpus's cancel-everything role */
    media_cd_watchdog_tick(0); /* disable runs one tick with 0 — which
                                * touches nothing, as shipped */
}

uint32_t media_cd_flags_get(void)
{
    return s_cd_flags;
}

int media_cd_slot(void)
{
    /* Signed view of the unsigned slot: the wrapped draws (draw % window
     * == 0) read negative here, exactly as the corpus's word does. */
    return (int)s_cd_slot;
}

void media_cd_loop_set(int loop)
{
    /* DAT_00544a04 is raised by schedules that pass the loop argument
     * and never cleared anywhere in the corpus — stale across calls. */
    if (loop != 0) {
        s_cd_loop = 1;
    }
}

int media_cd_loop_get(void)
{
    return s_cd_loop;
}

void media_cd_track_set_dead(int dead)
{
    s_cd_track_dead = dead;
}

void media_cd_watchdog_tick(int arg)
{
    if (arg == 0) {
        return; /* shipped tick with 0: only the timestamp touch */
    }
    if (media_timer_pending() == 0 || s_cd_track_dead == 0) {
        return; /* corpus: nothing to restart while the track lives */
    }
    /* Stalled track: the corpus restarts it (loop flag — a CD-device
     * seam here) and runs the re-arm callback. */
    s_cd_track_dead = 0;
    s_cd_rearm(NULL);
}

/* ------------------------------------------------------------------ *
 *  AudioPlaybackToggle @ 0x429810 (issue #298).
 * ------------------------------------------------------------------ */

void media_audio_playback_toggle(int enable)
{
    if (enable != 0) {
        s_cd_flags |= MEDIA_FLAG_SOUND_ON;
        sound_subsystem_enable_set_and_stop(1);
        return;
    }
    s_cd_flags &= ~MEDIA_FLAG_SOUND_ON;
    sound_subsystem_enable_set_and_stop(0);
}

/* ------------------------------------------------------------------ *
 *  Cue dispatch + pointer-cursor state (issue #300). The corpus's
 *  DAT_00500a98 / _DAT_00500efc pair lives in the cursor/renderer
 *  context; the consumers (the cursor draw) land with that port, so
 *  the state is held module-locally as the observable half.
 * ------------------------------------------------------------------ */

static int   s_cursor_slot;       /* DAT_00500a98 role */
static float s_cursor_override;   /* _DAT_00500efc role (statics: 0.0) */

void media_pointer_cursor_select(int slot)
{
    if (s_cursor_override <= 0.0f) {
        s_cursor_slot = slot;
    }
}

void media_pointer_cursor_override_set(int slot, float value)
{
    s_cursor_slot = slot;
    s_cursor_override = value;
}

int media_pointer_cursor_slot(void)
{
    return s_cursor_slot;
}

float media_pointer_cursor_override(void)
{
    return s_cursor_override;
}

void media_action_feedback_sound_dispatch(int event)
{
    int saved = sound_batch_mode_get();
    sound_batch_queue_drain(0, 0);      /* force batch mode off, no replay */
    int slot = 0;
    int cue = 0;
    switch (event) {
    case 0:  slot = 0x14; cue = 0x13; break;
    case 1:  slot = 0x15; cue = 0x14; break;
    case 2:  slot = 0x15; cue = 0x1f; break;
    case 3:  slot = 0x14; cue = 0x1e; break;
    case 4:  slot = 0x14; cue = 0x20; break;
    case 5:  slot = 0x14; cue = 0x21; break;
    default:
        /* Unknown event: submit nothing, but still restore the mode. */
        sound_batch_queue_drain(saved, 0);
        return;
    }
    media_pointer_cursor_override_set(slot, 12.5f);  /* 0x41480000 */
    sound_alias_play(cue, 0);
    sound_batch_queue_drain(saved, 0);  /* restore the saved mode */
}

void media_action_code_to_sound_alias(int code)
{
    int cue = 0;
    int hit = 0;
    switch (code) {
    case 1:  cue = 0x2b; hit = 1; break;
    case 2:  cue = 0x2c; hit = 1; break;
    case 3:  cue = 0x2d; hit = 1; break;
    case 4:  cue = 0x2e; hit = 1; break;
    case 5:  cue = 0x2f; hit = 1; break;
    case 8:  cue = 0x32; hit = 1; break;
    case 10: cue = 0x34; hit = 1; break;
    case 11: cue = 0x35; hit = 1; break;
    default: break;
    }
    if (hit) {
        media_pointer_cursor_select(cue);
    }
}

/* ------------------------------------------------------------------ *
 *  SoundMinimumDistanceUpdateMode @ 0x4295f0 (issue #302).
 * ------------------------------------------------------------------ */

static void*  s_mind_context;          /* DAT_00556160 role */
static int    s_mind_mode;             /* DAT_00556ec8 role */
static float  s_mind_listener;         /* applied listener/object scale */
static float  s_mind_clip;             /* applied renderer clip */
static float  s_mind_ref;              /* applied tracked-buffer minimum */
static float  s_mind_cell_scale = 1.0f;   /* _DAT_00556110 analog */
static float  s_mind_clip_base  = 0.0f;   /* DAT_00556114 analog */
static float  s_mind_ref_base   = 150.0f; /* DAT_00556ec4 analog (150.0) */
static float  s_mind_dist_base  = 0.0f;   /* DAT_005560d4 + 0x1c analog */

void media_min_distance_config_set(float cell_scale, float clip_base,
                                   float ref_base, float min_dist_base)
{
    s_mind_cell_scale = cell_scale;
    s_mind_clip_base = clip_base;
    s_mind_ref_base = ref_base;
    s_mind_dist_base = min_dist_base;
}

void media_min_distance_update_mode(int mode, void* context,
                                    int scalar_selector)
{
    if (mode == 0) {
        /* Scalar selector: 0 -> 0.9, nonzero -> 0.6 (corpus float
         * constants 0x3f666666 / 0x3f19999a). */
        s_mind_listener = scalar_selector != 0 ? 0.6f : 0.9f;
        s_mind_clip = s_mind_dist_base * s_mind_cell_scale;
        s_mind_ref = 5.0f; /* 0x42480000 */
        /* Context release / entity-kind hooks land at the entity layer
         * (unported seams); storing is the observable half. The corpus
         * dereferences the context unchecked; the port guards NULL. */
        s_mind_context = context;
    } else if (mode == 1) {
        if (s_mind_context != NULL) {
            /* Corpus: context release (entity layer seam). */
            s_mind_context = NULL;
        } else {
            s_mind_context = NULL;
        }
        s_mind_listener = 0.5f; /* 0x3f000000 */
        s_mind_clip = s_mind_clip_base;
        s_mind_ref = s_mind_ref_base;
    }
    /* Other modes update only the mode global, like the corpus. */
    s_mind_mode = mode;
}

int media_min_distance_mode(void)
{
    return s_mind_mode;
}

void* media_min_distance_context(void)
{
    return s_mind_context;
}

float media_min_distance_listener_scale(void)
{
    return s_mind_listener;
}

float media_min_distance_clip_distance(void)
{
    return s_mind_clip;
}

float media_min_distance_ref_distance(void)
{
    return s_mind_ref;
}
