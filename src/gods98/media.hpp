// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — media-playback state machines (port issues #301 / #302).
 *
 * Two small state holders the shipped build drives from simulation call
 * paths, modelled with their observable state:
 *
 *  - CDAudioWatchdogModeUpdate @ 0x4297b0: the CD-audio watchdog state.
 *    Enable raises bit value 8 of the audio/engine state word, draws the
 *    time-derived CD-track slot (random % window + start, per the
 *    CDTracks / CDStartTrack config), and schedules the watchdog re-arm
 *    callback on the port's one-shot timer; the callback re-arms with a
 *    fresh draw, exactly like the shipped re-arm path. Disable drops the
 *    bit, cancels the scheduled callback, and runs one watchdog tick
 *    with argument 0 — which touches nothing, as shipped. Track start,
 *    stop, status, error text, and watchdog polling use installable device
 *    hooks, so native builds can supply an MCI-like CD backend without
 *    coupling this state machine to WinMM.
 *
 *  - SoundMinimumDistanceUpdateMode @ 0x4295f0: the minimum-distance
 *    mode. Mode 0 stores the supplied entity/context and applies the
 *    distance scalars for the caller-selected value (0.9 / 0.6 listener
 *    scale, clip = min-distance base * cell distance scale, 5.0 tracked
 *    reference distance); mode 1 clears the stored context and restores
 *    the config-derived previous values (listener scale 0.5, the config
 *    clip distance, the config reference distance). Other mode values
 *    only update the mode global. The higher-level purpose of the state
 *    is flagged uncertain by finding 418, so this port models exactly
 *    the observable state machine.
 *
 *  The corpus routes these through platform/render helpers. The CD path
 *  now exposes command/status hooks; the renderer clip/listener setters
 *  remain injectable seams, with the values they would carry tracked.
 */
#ifndef LEGORRR_MEDIA_HPP
#define LEGORRR_MEDIA_HPP

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ *
 *  One-shot timer (the port's analog of the corpus's scheduled-
 *  callback table, driven by the engine clock). One pending slot —
 *  like the corpus's single scheduled CD track, scheduling overwrites
 *  it.
 * ------------------------------------------------------------------ */

typedef void (*MediaTimerCallback)(void* user);

/* Schedule `cb(user)` after `delay` seconds. Returns 1. */
int  media_timer_schedule(float delay, MediaTimerCallback cb, void* user);
/* Drop the pending timer (the corpus's cancel-everything role). */
void media_timer_cancel(void);
/* Advance by `dt` seconds; fires due callbacks exactly once. */
void media_timer_tick(float dt);
/* Inspection for tests / diagnostics. */
int   media_timer_pending(void);
float media_timer_remaining(void);

/* ------------------------------------------------------------------ *
 *  CDAudioWatchdogModeUpdate @ 0x4297b0. See header block.
 * ------------------------------------------------------------------ */

/* DAT_00556ecc bit value 8 — the corpus keeps this in the engine state
 * word; the port models it module-locally (deviation: no engine context
 * is threaded through the call). Bit value 0x10 is AudioPlaybackToggle's
 * sound-on flag, modelled in the same word (#298, below). */
#define MEDIA_CD_WATCHDOG_BIT 0x8u
#define MEDIA_FLAG_SOUND_ON   0x10u

/* Corpus config fields: CDStartTrack (DAT_00556f0c, default 2 when
 * unset) and CDTracks (DAT_00556f10, default 1 — disables when zero). */
void     media_cd_config_set(int start_track_base, int track_count);

/* The watchdog state machine. Enable draws the time-derived slot and
 * schedules the re-arm callback for that many seconds out; disable
 * clears the bit, cancels, and runs one watchdog tick with 0. */
void     media_cd_watchdog_update(int enable);
uint32_t media_cd_flags_get(void);   /* state word with the 0x8/0x10 bits */
int      media_cd_slot(void);        /* last derived slot (scheduled delay) */
void     media_cd_loop_set(int loop);/* DAT_00544a04 analog (never cleared
                                      * as shipped: stale across calls) */
int      media_cd_loop_get(void);

/* The "track died" flag — the port's analog of the corpus CD-media
 * status query. 0 = alive (native builds have no CD media), 1 = died. */
void     media_cd_track_set_dead(int dead);

/* The watchdog tick (corpus scheduled-check role): with `arg` nonzero,
 * a due timer (>= 4 s elapsed), the loop flag, and the track dead, the
 * re-arm callback runs and the slot refreshes with a fresh draw.
 * `arg` 0 — the disable call's — does nothing, as shipped. */
void     media_cd_watchdog_tick(int arg);

/* ------------------------------------------------------------------ *
 *  AudioPlaybackToggle @ 0x429810 (issue #298). See header block.
 * ------------------------------------------------------------------ */

/* Set (enable != 0) raises the sound-on bit and re-enables the sound
 * subsystem; clearing it mutes through the same gate. Repeats are
 * idempotent, like the corpus's flag write. */
void media_audio_playback_toggle(int enable);

/* ------------------------------------------------------------------ *
 *  Cue dispatch tables (issue #300; ActionFeedbackSoundDispatch @
 *  0x428800, ActionCodeToSoundAlias @ 0x4351a0) over the ported
 *  submission queue, plus the pointer-cursor override state they write
 *  (PointerCursorSelect @ 0x45cc20, PointerCursorOverrideSet @
 *  0x45cc40 — DAT_00500a98/DAT_00500efc; consumers land with the cursor
 *  draw, so the port holds the state as the observable half).
 * ------------------------------------------------------------------ */

/* PointerCursorSelect @ 0x45cc20: writes the cursor slot only while no
 * override value is active. */
void media_pointer_cursor_select(int slot);
/* PointerCursorOverrideSet @ 0x45cc40. */
void media_pointer_cursor_override_set(int slot, float value);
int   media_pointer_cursor_slot(void);
float media_pointer_cursor_override(void);

/* ActionFeedbackSoundDispatch @ 0x428800: saves and forces batch mode
 * off, maps events 0-5 to the cursor override + cue pair, plays it, and
 * restores the saved mode (unknown events submit nothing but still
 * restore). */
void media_action_feedback_sound_dispatch(int event);

/* ActionCodeToSoundAlias @ 0x4351a0: action-record codes 1-5, 8, 10, 11
 * map to cues 0x2b-0x2f, 0x32, 0x34, 0x35 through the cursor select;
 * other codes do nothing. */
void media_action_code_to_sound_alias(int code);

/* ------------------------------------------------------------------ *
 *  SoundMinimumDistanceUpdateMode @ 0x4295f0. See header block.
 * ------------------------------------------------------------------ */

/* Run the state machine: `mode` 0/1 (other values only update the mode),
 * `context` the entity/context pointer stored in mode 0. Mode 0 picks
 * the listener scale from `scalar_selector` (0 -> 0.9, nonzero -> 0.6). */
void  media_min_distance_update_mode(int mode, void* context,
                                     int scalar_selector);

/* State inspection: mode global, stored context, applied scalars. */
int   media_min_distance_mode(void);
void* media_min_distance_context(void);
float media_min_distance_listener_scale(void);  /* DAT_00556160-era value */
float media_min_distance_clip_distance(void);   /* renderer clip value */
float media_min_distance_ref_distance(void);    /* tracked-buffer value */

/* The config-derived "previous values" the mode updates apply or
 * restore: cell distance scale (the render-quality scalar), the config
 * clip distance (tvclipdist), the config reference distance
 * (MinDistFor3DSoundsOnTopView, corpus default 150.0), and the level
 * min-distance base the mode-0 clip multiplies with. Defaults mirror
 * the corpus defaults. */
void media_min_distance_config_set(float cell_scale, float clip_base,
                                   float ref_base, float min_dist_base);

#ifdef __cplusplus
}
#endif

/* --- the CD-audio availability seam (2026-10-24; finding 14) ---------------
 * CDAudioPlaybackAvailable (FUN_00488b90): the corpus answers yes only
 * while the CD-audio device word (DAT_0054486c, raised by the MCI device
 * opener — unported) and the sound-system word (DAT_00533878, the port's
 * sound-on seam) are both set. The device word has no ported owner, so it
 * lands zero-initialized with a set hook standing in (documented
 * deviation). */
int  media_cd_audio_playback_available(void); /* FUN_00488b90 */
void media_cd_audio_open_set(int open);       /* port hook */
typedef int (*MediaCdAudioInitHook)(void* context);
void media_cd_audio_init_hook_set(MediaCdAudioInitHook hook, void* context);
int media_cd_audio_device_state_update(int request);
typedef int (*MediaCdAudioErrorTextHook)(void* context, int error_code,
                                         char* output, int capacity);
typedef int (*MediaCdAudioStatusHook)(void* context, char* mode, int capacity,
                                      int* current_track, int* error_code);
void media_cd_audio_error_hook_set(MediaCdAudioErrorTextHook hook,
                                   void* context);
void media_cd_audio_error_code_set(int error_code);
const char* media_cd_audio_error_text_get(void); /* 0x00489260 */
void media_cd_audio_status_hook_set(MediaCdAudioStatusHook hook,
                                    void* context);
int media_cd_audio_track_completion_poll(int track); /* 0x00489280 */
typedef int (*MediaCdAudioCommandHook)(void* context, const char* command,
                                       int argument, int* error_code);
void media_cd_audio_command_hook_set(MediaCdAudioCommandHook hook,
                                     void* context);
int media_cd_audio_track_play_from(int track); /* 0x00489210 */
int media_cd_audio_device_open(int initial_track); /* 0x00489330 */
int media_cd_audio_stop_and_close(void); /* 0x004893a0 */
void media_cd_audio_stop_and_close_if_active(void); /* 0x00488bf0 */
typedef uint32_t (*MediaCdAudioTimeHook)(void* context);
void media_cd_audio_time_hook_set(MediaCdAudioTimeHook hook, void* context);
void media_cd_audio_watchdog_tick(int enabled); /* 0x00488c00 */
typedef void (*MediaCdAudioCompletionHook)(void);
void media_cd_audio_track_start(int track, int restart_when_done,
                                MediaCdAudioCompletionHook callback);

/* LoaderProgressCallbackSet (FUN_004802b0): stores the optional loader
 * progress callback pair (DAT_005343ac/DAT_005343b0, finding 118). The
 * corpus's read tap lives with the unported whole-file read loop, so the
 * pair lands as zero-init module storage with setter/peek hooks. */
void  loader_progress_callback_set(void* callback, void* context);
                                 /* FUN_004802b0 */
void* loader_progress_callback_peek(void);
void* loader_progress_context_peek(void);
#endif /* LEGORRR_MEDIA_HPP */
