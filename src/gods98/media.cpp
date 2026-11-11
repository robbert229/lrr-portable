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

static int      s_cd_base  = 2;    /* DAT_00556f0c: CDStartTrack (2 default) */
static int      s_cd_window = 1;   /* DAT_00556f10: CDTracks (1 default) */
static unsigned s_cd_slot;         /* last selected track */

/* VC5 rand.obj supplies the shared stream in the classic x86 build; other
 * builds keep this subsystem's local LCG model. */
#if !defined(_MSC_VER) || (_MSC_VER >= 1200) || !defined(_M_IX86)
static uint32_t s_cd_rand_state = 1;
#endif

static int s_cd_rand15(void)
{
#if defined(_MSC_VER) && (_MSC_VER < 1200) && defined(_M_IX86)
    return rand();
#else
    s_cd_rand_state = s_cd_rand_state * 214013u + 2531011u;
    return (int)((s_cd_rand_state >> 16) & 0x7fffu);
#endif
}

/* Retail track selection uses unsigned remainder + base - 1. */
static unsigned s_cd_draw_slot(void)
{
    unsigned draw = (unsigned)s_cd_rand15();
    return draw % (unsigned)s_cd_window - 1u + (unsigned)s_cd_base;
}

static void s_cd_rearm(void)
{
    media_cd_audio_stop_and_close_if_active();
    media_cd_watchdog_update(1);
}

void media_cd_config_set(int start_track_base, int track_count)
{
    /* Corpus zero-to-default rules from config load: an unset/zero
     * CDStartTrack lands as 2, zero CDTracks lands as 1. */
    s_cd_base = start_track_base != 0 ? start_track_base : 2;
    s_cd_window = track_count != 0 ? track_count : 1;
}

// FUNCTION: LEGORR 0x004297b0
void media_cd_watchdog_update(int enable)
{
    uint32_t* flags = engine_shared_state_word();
    if (enable != 0) {
        s_cd_slot = s_cd_draw_slot();
        *flags |= MEDIA_CD_WATCHDOG_BIT;
        media_cd_audio_track_start((int)s_cd_slot, 0, s_cd_rearm);
    } else {
        *flags &= ~MEDIA_CD_WATCHDOG_BIT;
        media_cd_audio_stop_and_close_if_active();
        media_cd_audio_watchdog_tick(0);
    }
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

// FUNCTION: LEGORR 0x00429810
void media_audio_playback_toggle(int enable)
{
    if (enable != 0) {
        *engine_shared_state_word() |= MEDIA_FLAG_SOUND_ON;
        sound_subsystem_enable_set_and_stop(enable);
        return;
    }
    *engine_shared_state_word() &= ~MEDIA_FLAG_SOUND_ON;
    sound_subsystem_enable_set_and_stop(0);
}

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

// FUNCTION: LEGORR 0x00428800
void media_action_feedback_sound_dispatch(int event)
{
    int saved = sound_batch_mode_get();
    sound_batch_queue_drain(0, 0);      /* force batch mode off, no replay */
    switch (event) {
    case 0:
        media_pointer_cursor_override_set(0x14, 12.5f);
        sound_alias_play(0x13, 0);
        break;
    case 1:
        media_pointer_cursor_override_set(0x15, 12.5f);
        sound_alias_play(0x14, 0);
        break;
    case 2:
        media_pointer_cursor_override_set(0x15, 12.5f);
        sound_alias_play(0x1f, 0);
        break;
    case 3:
        media_pointer_cursor_override_set(0x14, 12.5f);
        sound_alias_play(0x1e, 0);
        break;
    case 4:
        media_pointer_cursor_override_set(0x14, 12.5f);
        sound_alias_play(0x20, 0);
        break;
    case 5:
        media_pointer_cursor_override_set(0x14, 12.5f);
        sound_alias_play(0x21, 0);
        break;
    default:
        /* Unknown event: submit nothing, but still restore the mode. */
        sound_batch_queue_drain(saved, 0);
        return;
    }
    sound_batch_queue_drain(saved, 0);  /* restore the saved mode */
}

// FUNCTION: LEGORR 0x004351a0
void media_action_code_to_sound_alias(int code)
{
    /* Corpus at 0x4351a0 uses an ordered compare/call/return ladder:
     * each match pushes its literal cue to PointerCursorSelect, while a
     * miss falls through to the next comparison. Keep the arguments
     * literal and the returns local to preserve that /Od shape. */
    if (code == 1) {
        media_pointer_cursor_select(0x2b);
        return;
    }
    if (code == 2) {
        media_pointer_cursor_select(0x2c);
        return;
    }
    if (code == 3) {
        media_pointer_cursor_select(0x2d);
        return;
    }
    if (code == 4) {
        media_pointer_cursor_select(0x2e);
        return;
    }
    if (code == 5) {
        media_pointer_cursor_select(0x2f);
        return;
    }
    if (code == 8) {
        media_pointer_cursor_select(0x32);
        return;
    }
    if (code == 10) {
        media_pointer_cursor_select(0x34);
        return;
    }
    if (code == 11) {
        media_pointer_cursor_select(0x35);
        return;
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

static uint32_t media_float_bits(float value)
{
    union FloatBits {
        float value;
        uint32_t bits;
    } converted;
    converted.value = value;
    return converted.bits;
}

void media_min_distance_config_set(float cell_scale, float clip_base,
                                   float ref_base, float min_dist_base)
{
    s_mind_cell_scale = cell_scale;
    s_mind_clip_base = clip_base;
    s_mind_ref_base = ref_base;
    s_mind_dist_base = min_dist_base;
}

// FUNCTION: LEGORR 0x004295f0
void media_min_distance_update_mode(int mode, void* context,
                                    int scalar_selector)
{
    if (mode == 0) {
        /* Scalar selector: 0 -> 0.9, nonzero -> 0.6 (corpus float
         * constants 0x3f666666 / 0x3f19999a). */
        s_mind_listener = scalar_selector != 0 ? 0.6f : 0.9f;
        s_mind_clip = s_mind_dist_base * s_mind_cell_scale;
        s_mind_ref = 50.0f; /* 0x42480000 */
        sound_tracked_buffers_set_min_distance(
            media_float_bits(s_mind_ref));
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
        sound_tracked_buffers_set_min_distance(
            media_float_bits(s_mind_ref));
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
