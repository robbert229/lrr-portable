// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — media-playback state machines (port issues #301 / #302).
 *
 * Two small state holders the shipped build drives from simulation call
 * paths, modelled with their observable state:
 *
 *  - CDAudioWatchdogModeUpdate @ 0x4297b0 selects a configured random
 *    track and installs a completion callback. The callback closes the
 *    device and selects another track. Disable closes the active device.
 *
 *  - SoundMinimumDistanceUpdateMode @ 0x4295f0: the minimum-distance
 *    mode. Mode 0 stores the supplied entity/context and applies the
 *    distance scalars for the caller-selected value (0.9 / 0.6 listener
 *    scale, clip = min-distance base * cell distance scale, 50.0 tracked
 *    reference distance); mode 1 clears the stored context and restores
 *    the config-derived previous values (listener scale 0.5, the config
 *    clip distance, the config reference distance). Other mode values
 *    only update the mode global. The higher-level purpose of the state
 *    is flagged uncertain by finding 418, so this port models exactly
 *    the observable state machine.
 *
 *  The corpus routes these through platform/render helpers. The CD path
 *  reaches MCI on winapi and reports its native limitation; renderer
 *  clip/listener setters remain injectable seams, with values tracked.
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

/* Bits in the shared engine state word DAT_00556ecc. */
#define MEDIA_CD_WATCHDOG_BIT 0x8u
#define MEDIA_FLAG_SOUND_ON   0x10u

/* Corpus config fields: CDStartTrack (DAT_00556f0c, default 2 when
 * unset) and CDTracks (DAT_00556f10, default 1 — disables when zero). */
void     media_cd_config_set(int start_track_base, int track_count);

/* Enable selects and starts a track; disable clears the bit and closes. */
void     media_cd_watchdog_update(int enable);
uint32_t media_cd_flags_get(void);   /* state word with the 0x8/0x10 bits */
int      media_cd_slot(void);        /* last selected track */
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
#ifdef __cplusplus
extern "C"
#endif
void media_pointer_cursor_select(int slot);
/* PointerCursorOverrideSet @ 0x45cc40. */
#ifdef __cplusplus
extern "C"
#endif
void media_pointer_cursor_override_set(int slot, float value);
void media_pointer_cursor_override_tick(float delta);
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
#ifdef __cplusplus
extern "C"
#endif
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

/* --- the CD track player (gods98 sound layer, soundsys.c) ------------------
 * Defined in the original-shape unit soundsys.c and declared there in
 * gods98/soundsys.h; test_c_boundaries checks these against it. The track
 * argument is 0-based (MCI plays track + 1). */
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*MediaCdAudioCompletionHook)(void);
int media_cd_audio_device_state_update(int nosound); /* 0x00488b50 */
int media_cd_audio_playback_available(void); /* 0x00488b90 */
void media_cd_audio_track_start(int track, int mode,
                                MediaCdAudioCompletionHook callback);
                                             /* 0x00488bb0 */
void media_cd_audio_stop_and_close_if_active(void); /* 0x00488bf0 */
void media_cd_audio_watchdog_tick(int cdtrack); /* 0x00488c00 */
#ifdef __cplusplus
}
#endif

/* LoaderProgressCallbackSet (FUN_004802b0): stores the optional loader
 * progress callback pair (DAT_005343ac/DAT_005343b0, finding 118). The
 * corpus's read tap lives with the unported whole-file read loop, so the
 * pair lands as zero-init module storage with setter/peek hooks. */
void  loader_progress_callback_set(void* callback, void* context);
                                 /* FUN_004802b0 */
extern void* s_loader_progress_callback;
extern void* s_loader_progress_context;
void* loader_progress_callback_peek(void);
void* loader_progress_context_peek(void);
#endif /* LEGORRR_MEDIA_HPP */
