// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — media-playback state machines. See media.hpp. */

#include "media.hpp"

#include <stddef.h>
#include <stdlib.h>

#include "platform/platform.h"
#include "audio.hpp"
#include "frameloop.hpp"
#include "sound.hpp"
#include "engine.hpp"
#include "media_shared.hpp"
#include "media_for_lego.hpp"

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

int      s_cd_base  = 2;    /* DAT_00556f0c: CDStartTrack (2 default) */
int      s_cd_window = 1;   /* DAT_00556f10: CDTracks (1 default) */
unsigned s_cd_slot;         /* last selected track */

void media_cd_config_set(int start_track_base, int track_count)
{
    /* Corpus zero-to-default rules from config load: an unset/zero
     * CDStartTrack lands as 2, zero CDTracks lands as 1. */
    s_cd_base = start_track_base != 0 ? start_track_base : 2;
    s_cd_window = track_count != 0 ? track_count : 1;
}

uint32_t media_cd_flags_get(void)
{
    return *engine_shared_state_word();
}

int media_cd_slot(void)
{
    /* Signed view of the 32-bit track word. */
    return (int)s_cd_slot;
}

/* ------------------------------------------------------------------ *
 *  AudioPlaybackToggle @ 0x429810 (issue #298).
 * ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ *
 *  Cue dispatch + pointer-cursor state (issue #300). The corpus's
 *  DAT_00500a98 / _DAT_00500efc pair lives in the cursor/renderer
 *  context; the consumers (the cursor draw) land with that port, so
 *  the state is held module-locally as the observable half. The
 *  PointerCursorSelect gate is `test ah, 0x41` on an fcomp against
 *  0.0f, so it skips only strictly-positive overrides: zero, negative
 *  AND NaN all store the slot.
 * ------------------------------------------------------------------ */

int media_pointer_cursor_slot(void)
{
    return s_cursor_slot;
}

float media_pointer_cursor_override(void)
{
    return s_cursor_override;
}

/* ------------------------------------------------------------------ *
 *  SoundMinimumDistanceUpdateMode @ 0x4295f0 (issue #302).
 * ------------------------------------------------------------------ */

void*  s_mind_context;          /* DAT_00556160 role */
int    s_mind_mode;             /* DAT_00556ec8 role */
float  s_mind_listener;         /* applied listener/object scale */
float  s_mind_clip;             /* applied renderer clip */
float  s_mind_ref;              /* applied tracked-buffer minimum */
float  s_mind_cell_scale = 1.0f;   /* _DAT_00556110 analog */
float  s_mind_clip_base  = 0.0f;   /* DAT_00556114 analog */
float  s_mind_ref_base   = 150.0f; /* DAT_00556ec4 analog (150.0) */
float  s_mind_dist_base  = 0.0f;   /* DAT_005560d4 + 0x1c analog */

void media_min_distance_config_set(float cell_scale, float clip_base,
                                   float ref_base, float min_dist_base)
{
    s_mind_cell_scale = cell_scale;
    s_mind_clip_base = clip_base;
    s_mind_ref_base = ref_base;
    s_mind_dist_base = min_dist_base;
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

/* LoaderProgressCallbackSet (FUN_004802b0) — see media.hpp. */
// GLOBAL: LEGORR 0x005343ac
void* s_loader_progress_callback; /* DAT_005343ac role: unowned */
// GLOBAL: LEGORR 0x005343b0
void* s_loader_progress_context;  /* DAT_005343b0 role: unowned */

// FUNCTION: LEGORR 0x004802b0
void loader_progress_callback_set(void* callback, void* context)
{
    s_loader_progress_callback = callback;
    s_loader_progress_context = context;
}

void* loader_progress_callback_peek(void) { return s_loader_progress_callback; }
void* loader_progress_context_peek(void) { return s_loader_progress_context; }
