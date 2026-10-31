// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — media-playback state machines. See media.hpp. */

#include "media.hpp"

#include <stddef.h>
#include <stdlib.h>

#include "platform/platform.h"
#include "audio.hpp"
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

// GLOBAL: LEGORR 0x00556ecc
static uint32_t s_cd_flags;        /* DAT_00556ecc role, module-local */
static int      s_cd_base  = 2;    /* DAT_00556f0c: CDStartTrack (2 default) */
static int      s_cd_window = 1;   /* DAT_00556f10: CDTracks (1 default) */
static int      s_cd_loop;         /* DAT_00544a04 role (stale: set only by
                                    * the "loop start" schedule argument the
                                    * watchdog never passes) */
static unsigned s_cd_slot;         /* last derived slot */
static int      s_cd_track_dead;   /* port analog of the track-dead check */

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

// FUNCTION: LEGORR 0x004297b0
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

// FUNCTION: LEGORR 0x00429810
void media_audio_playback_toggle(int enable)
{
    if (enable != 0) {
        s_cd_flags |= MEDIA_FLAG_SOUND_ON;
        sound_subsystem_enable_set_and_stop(enable);
        return;
    }
    s_cd_flags &= ~MEDIA_FLAG_SOUND_ON;
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

// GLOBAL: LEGORR 0x00500a98
static int   s_cursor_slot;       /* DAT_00500a98 role */
// GLOBAL: LEGORR 0x00500efc
static float s_cursor_override;   /* _DAT_00500efc role (statics: 0.0) */

/* The corpus's override gate compares against this .rdata zero dword
 * (objdump-verified fcomp operand of PointerCursorSelect); a named const
 * static keeps the fcomp operand resolvable by Reccmp. */
// GLOBAL: LEGORR 0x0049e64c
static const float s_cursor_gate = 0.0f;   /* DAT_0049e64c, verified */

// FUNCTION: LEGORR 0x0045cc20
void media_pointer_cursor_select(int slot)
{
    if (!(s_cursor_override > s_cursor_gate)) {
        s_cursor_slot = slot;
    }
}

// FUNCTION: LEGORR 0x0045cc40
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

/* --- the CD-audio availability seam (see media.hpp) -------------------------- */
// GLOBAL: LEGORR 0x0054486c
static int s_cd_audio_open; /* DAT_0054486c role: unowned, zero-init */
// GLOBAL: LEGORR 0x00544a04
static int s_cd_audio_current_track; /* DAT_00544a04 */
static MediaCdAudioInitHook s_cd_audio_init_hook;
static void* s_cd_audio_init_context;
static MediaCdAudioErrorTextHook s_cd_audio_error_hook;
static void* s_cd_audio_error_context;
static MediaCdAudioStatusHook s_cd_audio_status_hook;
static void* s_cd_audio_status_context;
static MediaCdAudioCommandHook s_cd_audio_command_hook;
static void* s_cd_audio_command_context;
static MediaCdAudioTimeHook s_cd_audio_time_hook;
static void* s_cd_audio_time_context;
static uint32_t s_cd_audio_watchdog_time;
static int s_cd_audio_target_track;
static int s_cd_audio_restart_when_done;
static MediaCdAudioCompletionHook s_cd_audio_completion_hook;
// GLOBAL: LEGORR 0x00544a1c
static int s_cd_audio_last_error;
// GLOBAL: LEGORR 0x00576500
static char s_cd_audio_error_text[200];

// FUNCTION: LEGORR 0x00488b90
int media_cd_audio_playback_available(void)
{
    return (s_cd_audio_open != 0 && s_bank_sound_on != 0) ? 1 : 0;
}

void media_cd_audio_init_hook_set(MediaCdAudioInitHook hook, void* context)
{
    s_cd_audio_init_hook = hook;
    s_cd_audio_init_context = context;
}

/* CDAudioDeviceStateUpdate (FUN_00488b50): a nonzero request clears the
 * open flag and succeeds. A zero request marks it open and delegates the
 * remaining audio-system initialization through the installed seam. */
// FUNCTION: LEGORR 0x00488B50
int media_cd_audio_device_state_update(int request)
{
    s_cd_audio_open = request == 0;
    s_cd_audio_current_track = -1;
    if (request != 0) return 1;
    if (s_cd_audio_init_hook != NULL)
        return s_cd_audio_init_hook(s_cd_audio_init_context) != 0;
    return audio_system_init(NULL) != 0;
}

void media_cd_audio_open_set(int open)
{
    s_cd_audio_open = open;
}

void media_cd_audio_error_hook_set(MediaCdAudioErrorTextHook hook,
                                   void* context)
{
    s_cd_audio_error_hook = hook;
    s_cd_audio_error_context = context;
}

void media_cd_audio_error_code_set(int error_code)
{
    s_cd_audio_last_error = error_code;
}

const char* media_cd_audio_error_text_get(void)
{
    s_cd_audio_error_text[0] = '\0';
    if (s_cd_audio_error_hook != NULL) {
        s_cd_audio_error_hook(s_cd_audio_error_context, s_cd_audio_last_error,
                              s_cd_audio_error_text,
                              (int)sizeof(s_cd_audio_error_text));
        s_cd_audio_error_text[sizeof(s_cd_audio_error_text) - 1] = '\0';
    }
    else {
        (void)platform_cd_audio_error_text(s_cd_audio_last_error,
                                           s_cd_audio_error_text,
                                           (int)sizeof(s_cd_audio_error_text));
        s_cd_audio_error_text[sizeof(s_cd_audio_error_text) - 1] = '\0';
    }
    return s_cd_audio_error_text;
}

// FUNCTION: LEGORR 0x00489260
void media_cd_audio_error_text_refresh(void)
{
    (void)media_cd_audio_error_text_get();
}

void media_cd_audio_status_hook_set(MediaCdAudioStatusHook hook,
                                    void* context)
{
    s_cd_audio_status_hook = hook;
    s_cd_audio_status_context = context;
}

static int media_cd_audio_text_equal(const char* left, const char* right)
{
    unsigned char a;
    unsigned char b;
    if (left == NULL || right == NULL) return 0;
    while (*left != '\0' && *right != '\0') {
        a = (unsigned char)*left++;
        b = (unsigned char)*right++;
        if (a >= 'A' && a <= 'Z') a = (unsigned char)(a + ('a' - 'A'));
        if (b >= 'A' && b <= 'Z') b = (unsigned char)(b + ('a' - 'A'));
        if (a != b) return 0;
    }
    return *left == *right;
}

// FUNCTION: LEGORR 0x00489280
int media_cd_audio_track_completion_poll(int track)
{
    char mode[32];
    int current_track = 0;
    int error_code = 0;
    mode[0] = '\0';
    if (s_cd_audio_open == 0) return 0;
    if (s_cd_audio_status_hook != NULL) {
        if (!s_cd_audio_status_hook(s_cd_audio_status_context, mode,
                                    (int)sizeof(mode), &current_track,
                                    &error_code)) {
            s_cd_audio_last_error = error_code;
            (void)media_cd_audio_error_text_get();
            return 0;
        }
    }
    else if (!platform_cd_audio_status(mode, (int)sizeof(mode),
                                       &current_track, &error_code)) {
        s_cd_audio_last_error = error_code;
        (void)media_cd_audio_error_text_get();
        return 0;
    }
    mode[sizeof(mode) - 1] = '\0';
    s_cd_audio_current_track = current_track;
    if (!media_cd_audio_text_equal(mode, "stopped")
        && current_track <= track + 1) {
        return 1;
    }
    s_cd_audio_last_error = error_code;
    (void)media_cd_audio_error_text_get();
    return 0;
}

void media_cd_audio_command_hook_set(MediaCdAudioCommandHook hook,
                                     void* context)
{
    s_cd_audio_command_hook = hook;
    s_cd_audio_command_context = context;
}

// FUNCTION: LEGORR 0x00489210
int media_cd_audio_track_play_from(int track)
{
    int error_code = 0;
    int played;
    if (s_cd_audio_command_hook != NULL) {
        played = s_cd_audio_command_hook(s_cd_audio_command_context, "play",
                                         track + 1, &error_code);
    }
    else {
        played = platform_cd_audio_play(track + 1, &error_code);
    }
    if (!played) {
        s_cd_audio_last_error = error_code;
        (void)media_cd_audio_error_text_get();
        return 0;
    }
    s_cd_audio_current_track = track;
    return 1;
}

// FUNCTION: LEGORR 0x00489330
int media_cd_audio_device_open(int initial_track)
{
    int error_code = 0;
    int opened;
    if (s_cd_audio_command_hook != NULL) {
        opened = s_cd_audio_command_hook(s_cd_audio_command_context, "open",
                                         0, &error_code);
    }
    else {
        opened = platform_cd_audio_open(&error_code);
    }
    if (!opened) {
        s_cd_audio_open = 0;
        s_cd_audio_last_error = error_code;
        (void)media_cd_audio_error_text_get();
        return 0;
    }
    s_cd_audio_open = 1;
    if (s_cd_audio_command_hook != NULL) {
        opened = s_cd_audio_command_hook(s_cd_audio_command_context,
                                         "set-tmsf", 0, &error_code);
    }
    else {
        opened = platform_cd_audio_set_tmsf(&error_code);
    }
    if (!opened) {
        s_cd_audio_open = 0;
        s_cd_audio_last_error = error_code;
        (void)media_cd_audio_error_text_get();
        if (s_cd_audio_command_hook != NULL) {
            int ignored_error = 0;
            (void)s_cd_audio_command_hook(s_cd_audio_command_context,
                                          "close", 0, &ignored_error);
        }
        else {
            (void)platform_cd_audio_close(NULL);
        }
        return 0;
    }
    return media_cd_audio_track_play_from(initial_track);
}

void media_cd_audio_track_start(int track, int restart_when_done,
                                MediaCdAudioCompletionHook callback)
{
    if (s_cd_audio_open == 0) return;
    if (restart_when_done == 1) s_cd_audio_restart_when_done = 1;
    s_cd_audio_target_track = track;
    s_cd_audio_completion_hook = callback;
    (void)media_cd_audio_device_open(track);
}

// FUNCTION: LEGORR 0x004893a0
int media_cd_audio_stop_and_close(void)
{
    int error_code = 0;
    int stopped;
    if (s_cd_audio_command_hook != NULL) {
        stopped = s_cd_audio_command_hook(s_cd_audio_command_context,
                                          "stop", 0, &error_code);
    }
    else {
        stopped = platform_cd_audio_stop(&error_code);
    }
    if (!stopped) {
        s_cd_audio_last_error = error_code;
        (void)media_cd_audio_error_text_get();
        return 0;
    }
    if (s_cd_audio_command_hook != NULL) {
        stopped = s_cd_audio_command_hook(s_cd_audio_command_context,
                                          "close", 0, &error_code);
    }
    else {
        stopped = platform_cd_audio_close(&error_code);
    }
    if (!stopped) {
        s_cd_audio_last_error = error_code;
        (void)media_cd_audio_error_text_get();
        return 0;
    }
    s_cd_audio_open = 0;
    s_cd_audio_current_track = -1;
    return 1;
}

// FUNCTION: LEGORR 0x00488bf0
void media_cd_audio_stop_and_close_if_active(void)
{
    if (s_cd_audio_open != 0) (void)media_cd_audio_stop_and_close();
}

void media_cd_audio_time_hook_set(MediaCdAudioTimeHook hook, void* context)
{
    s_cd_audio_time_hook = hook;
    s_cd_audio_time_context = context;
}

// FUNCTION: LEGORR 0x00488c00
void media_cd_audio_watchdog_tick(int enabled)
{
    uint32_t now;
    if (enabled == 0 || s_cd_audio_open == 0)
        return;
    now = s_cd_audio_time_hook != NULL
        ? s_cd_audio_time_hook(s_cd_audio_time_context)
        : platform_time_milliseconds();
    if (now <= s_cd_audio_watchdog_time + 4000u) return;
    if (media_cd_audio_track_completion_poll(s_cd_audio_target_track) == 0) {
        if (s_cd_audio_restart_when_done != 0)
            (void)media_cd_audio_track_play_from(s_cd_audio_target_track);
        if (s_cd_audio_completion_hook != NULL)
            s_cd_audio_completion_hook();
    }
    s_cd_audio_watchdog_time = now;
}

/* LoaderProgressCallbackSet (FUN_004802b0) — see media.hpp. */
// GLOBAL: LEGORR 0x005343ac
static void* s_loader_progress_callback; /* DAT_005343ac role: unowned */
// GLOBAL: LEGORR 0x005343b0
static void* s_loader_progress_context;  /* DAT_005343b0 role: unowned */

// FUNCTION: LEGORR 0x004802b0
void loader_progress_callback_set(void* callback, void* context)
{
    s_loader_progress_callback = callback;
    s_loader_progress_context = context;
}

void* loader_progress_callback_peek(void) { return s_loader_progress_callback; }
void* loader_progress_context_peek(void) { return s_loader_progress_context; }
