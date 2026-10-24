// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — media-playback state machines (port issues #301 / #302).
 *
 * Two small state holders the shipped build drives from simulation call
 * paths, modelled with their observable state:
 *
 *  - CDAudioWatchdogModeUpdate @ 0x4297b0: the CD-audio watchdog state.
 *    Enable raises bit 8 of the audio/engine state word, draws the
 *    time-derived CD-track slot (random % window + start, per the
 *    CDTracks / CDStartTrack config), and schedules the watchdog re-arm
 *    callback on the port's one-shot timer; the callback re-arms with a
 *    fresh draw, exactly like the shipped re-arm path. Disable drops the
 *    bit, cancels the scheduled callback, and runs one watchdog tick
 *    with argument 0 — which touches nothing, as shipped. Real
 *    CD-track playback does not exist on native hardware under this
 *    architecture: the track-start/stop helpers are documented no-op
 *    seams and the "track died" check is an exposed flag (the port-side
 *    analog of the corpus's CD-media status query), so the state
 *    machine and scheduler are the real, testable observables. The winapi
 *    reference build keeps the real behaviour.
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
 *  The corpus routes both through unported platform/render helpers
 *  (the CD device layer, the renderer clip/listener setters, the
 *  audio min-distance push); those are inert seams here, and only the
 *  state values they would carry are tracked.
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

/* DAT_00556ecc bit 8 — the corpus keeps this in the engine state word;
 * the port models it module-locally (deviation: no engine context is
 * threaded through the call). */
#define MEDIA_CD_WATCHDOG_BIT 0x100u

/* Corpus config fields: CDStartTrack (DAT_00556f0c, default 2 when
 * unset) and CDTracks (DAT_00556f10, default 1 — disables when zero). */
void     media_cd_config_set(int start_track_base, int track_count);

/* The watchdog state machine. Enable draws the time-derived slot and
 * schedules the re-arm callback for that many seconds out; disable
 * clears the bit, cancels, and runs one watchdog tick with 0. */
void     media_cd_watchdog_update(int enable);
uint32_t media_cd_flags_get(void);   /* state word with bit 8 */
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

#endif /* LEGORRR_MEDIA_HPP */
