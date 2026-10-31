// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — one-shot audio playback glue (finding 14 §1).
 *
 * Rides the platform's audio surface (platform.h, a DirectSound analog over
 * SDL). Converts any bank Sound — 8-bit unsigned or 16-bit signed, mono or
 * stereo, arbitrary sample rate — to mono 16-bit signed and submits it as a
 * one-shot. Loop/streamed voices and spatial positioning remain later
 * milestones; the menu master-gain path models DirectSound attenuation.
 */
#ifndef LEGORRR_AUDIO_HPP
#define LEGORRR_AUDIO_HPP

#include <cstdint>
#include <cstddef>

#include "platform/platform.h"
#include "sound.hpp"

typedef struct AudioPlayer {
    PlatformAudio* dev;   /* owned platform audio device */
    float          volume; /* master gain 0..1 */
    /* looping 3D track: the source sound + current pan; re-rendered and
     * re-seeded lazily when the pan changes. */
    Sound*   loop_sound;
    float    loop_pan;
    int      loop_render_dirty;
} AudioPlayer;

int  audio_open(AudioPlayer* p);     /* 0 on success, non-zero otherwise */
void audio_close(AudioPlayer* p);
void audio_active_shutdown(void); /* 0x0047a800 */
typedef int (*AudioSystemInitHook)(void* context, void* window);
void audio_system_init_hook_set(AudioSystemInitHook hook, void* context);
int audio_system_init(void* window); /* 0x0047a640 */

/* Master volume 0..1 (FUN_00464ef0 analog); applied to the audio stream. */
void audio_set_volume(AudioPlayer* p, float volume);
/* Convert the SFX option's DirectSound attenuation step (0..10) to a
 * linear gain; invalid indices return -1. */
float audio_sfx_volume_gain(int index);
/* SoundBufferVolumeSet (FUN_0047b500): apply DirectSound hundredth-dB
 * attenuation as the port's single output gain. */
void audio_sound_volume_set(int millibels);
/* Apply attenuation to the currently installed AudioPlayer. */
void audio_active_sound_volume_set(int millibels);
/* SoundVolumeIndexSet (FUN_0047b530): map the options index 0..10 through
 * the corpus attenuation table and apply it. */
void audio_sound_volume_index_set(int index);
/* GameSpeedSelect (FUN_00414de0; actually the SFX volume callback): delegate
 * the selected slider index to SoundVolumeIndexSet. */
void audio_menu_sfx_volume_set(int index);
/* VolumeSet (FUN_00414df0): map the menu's 0..9 volume value to the
 * normalized master gain. The port has one output gain in place of the
 * corpus's paired music-mixer channels. */
void audio_menu_volume_set(int value);
void music_mixer_volume_set(float music, float sfx);
int audio_mixer_control_init(float* music, float* sfx, int apply_volume);
void audio_music_mixer_volume_get(float* music, float* sfx);
typedef void (*AudioListenerRolloffHook)(void* context, float rolloff);
void audio_listener_rolloff_hook_set(AudioListenerRolloffHook hook,
                                     void* context);
float audio_listener_rolloff_set(float rolloff); /* 0x0047c100 */
float audio_listener_rolloff_get(void);

/* Submit one bank sound as a one-shot. Returns the number of frames queued,
 * or -1 on failure. */
int  audio_play_sound(AudioPlayer* p, const Sound* s);

/* Load + play by resource name through the sound bank. -1 on failure. */
int  audio_play_named(AudioPlayer* p, const char* name);

/* 3D-ish one-shot: mono sound spatialized with a -1..1 pan (constant-power
 * balance; distance falloff via audio_set_volume at the call site). */
int  audio_play_sound_3d(AudioPlayer* p, const Sound* s, float pan);
int  audio_active_play_sound_3d(const Sound* s, float pan);

/* Start/replace the continuous looping track (finding 14 §1 flag bit 2) for
 * a sound at pan -1..1. Loads through the bank when given a resource name
 * (name may be NULL to change only the pan of the current loop). */
int  audio_loop_named(AudioPlayer* p, const char* name, float pan);
void audio_set_loop_pan(AudioPlayer* p, float pan);

/* Per-frame: refill the looping stream + re-seed when the pan changed. */
void audio_update(AudioPlayer* p);

void audio_stop(AudioPlayer* p);

/* Convert a bank Sound to mono 16-bit signed PCM (the seam's transport).
 * Returns frame count; *out is allocated (free with audio_free_pcm). */
int  audio_to_16mono(const Sound* s, int16_t** out);

/* Constant-power stereo pan of a bank sound: interleaved L,R 16-bit with
 * pan -1 (left) .. +1 (right), 0 = center. Frames count returned. */
int  audio_to_stereo_pan(const Sound* s, float pan, int16_t** out_lr);

void audio_free_pcm(int16_t* pcm);

#endif /* LEGORRR_AUDIO_HPP */
