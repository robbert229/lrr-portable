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

typedef struct AudioBuffer AudioBuffer;

typedef struct AudioPlayer {
    PlatformAudio* dev;   /* owned platform audio device */
    PlatformAudio* stream_dev[2]; /* independently stoppable streamed voices */
    float          volume; /* master gain 0..1 */
    /* looping 3D track: the source sound + current pan; re-rendered and
     * re-seeded lazily when the pan changes. */
    Sound*   loop_sound;
    float    loop_pan;
    int      loop_render_dirty;
    AudioBuffer* loop_buffer;
    /* Streamed WAV slots own separate platform queues so stop/refill stays
     * local to one voice. */
    const Sound* stream_sample[2];
    WavSample stream_format[2];
    uint32_t stream_buffer_bytes[2];
    uint32_t stream_cursor_bytes[2];
    int      stream_active[2];
} AudioPlayer;

int  audio_open(AudioPlayer* p);     /* 0 on success, non-zero otherwise */
void audio_close(AudioPlayer* p);
void audio_system_player_set(AudioPlayer* p);
AudioPlayer* audio_system_player_get(void);
extern "C" void audio_active_shutdown(void); /* 0x0047a800 */
void audio_system_shutdown(void);
typedef int (*AudioSystemInitHook)(void* context, void* window);
void audio_system_init_hook_set(AudioSystemInitHook hook, void* context);
extern "C" int audio_system_init(void* window); /* 0x0047a640 */

/* Master volume 0..1 (FUN_00464ef0 analog); applied to the audio stream. */
void audio_set_volume(AudioPlayer* p, float volume);
/* Convert the SFX option's DirectSound attenuation step (0..10) to a
 * linear gain; invalid indices return -1. */
float audio_sfx_volume_gain(int index);
/* SoundBufferVolumeSet (FUN_0047b500): apply DirectSound hundredth-dB
 * attenuation as the port's single output gain. */
extern "C" void audio_sound_volume_set(int millibels);
/* Apply attenuation to the currently installed AudioPlayer. */
void audio_active_sound_volume_set(int millibels);
void audio_sound_volume_minus_300(void); /* 0x0047b550 */
/* SoundVolumeIndexSet (FUN_0047b530): map the options index 0..10 through
 * the corpus attenuation table and apply it. */
extern "C" void audio_sound_volume_index_set(int index);
/* GameSpeedSelect (FUN_00414de0; actually the SFX volume callback): delegate
 * the selected slider index to SoundVolumeIndexSet. */
void audio_menu_sfx_volume_set(int index);
/* VolumeSet (FUN_00414df0): map the menu's 0..9 volume value to the
 * normalized master gain. The port has one output gain in place of the
 * corpus's paired music-mixer channels. */
extern "C" void audio_menu_volume_set(int value);
extern "C" void music_mixer_volume_set(float music, float sfx);
/* Legacy mixer helper: query/apply the compact-disc source through WinMM.
 * The native backend has no such control and reports unavailable. */
int audio_mixer_control_init(float* music, float* sfx, int apply_volume);
void audio_music_mixer_volume_get(float* music, float* sfx);
/* Read the legacy compact-disc mixer source. Returns 1 when WinMM finds a
 * control matching the reference's type masks on a
 * MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC source.
 * `music` receives its first channel; `sfx` receives its second channel for
 * stereo, or mirrors the first for mono. Both are normalized 0..1 values
 * from the same source control, not separate music/SFX devices. Portable
 * backends return 0 and clear both outputs. */
int audio_cd_mixer_volume_get(float* music, float* sfx);
int audio_cd_mixer_volume_set(float music, float sfx);
#if defined(LEGORRR_BACKEND_WINAPI)
int platform_cd_mixer_volume_get(float* music, float* sfx);
int platform_cd_mixer_volume_set(float music, float sfx);
#endif
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
int  audio_loop_sample(AudioPlayer* p, const Sound* sample, float pan);
void audio_set_loop_pan(AudioPlayer* p, float pan);

/* Per-frame: refill the looping stream + re-seed when the pan changed. */
void audio_update(AudioPlayer* p);

void audio_stop(AudioPlayer* p);

/* Portable counterparts for bank-owned duplicate buffers and the two
 * reference streamed-wave slots. Each stream slot owns its stoppable queue. */
AudioBuffer* audio_buffer_create(const Sound* sample);
void audio_buffer_destroy(AudioBuffer* buffer);
void audio_buffer_volume_set(AudioBuffer* buffer, int millibels);
void audio_buffer_stop(AudioBuffer* buffer);
int audio_buffer_play(AudioBuffer* buffer);
int audio_buffer_play_attenuated(AudioBuffer* buffer, int extra_millibels);
int audio_buffer_loop(AudioBuffer* buffer, const Sound* sample);
int audio_buffer_loop_attenuated(AudioBuffer* buffer, const Sound* sample,
                                 int extra_millibels);
int audio_stream_setup(AudioPlayer* p, int stream_index,
                       const WavSample* format, uint32_t buffer_bytes);
int audio_stream_start(AudioPlayer* p, int stream_index, const Sound* sample);
void audio_stream_stop(AudioPlayer* p, int stream_index);

/* Convert a bank Sound to mono 16-bit signed PCM (the seam's transport).
 * Returns frame count; *out is allocated (free with audio_free_pcm). */
int  audio_to_16mono(const Sound* s, int16_t** out);

/* Constant-power stereo pan of a bank sound: interleaved L,R 16-bit with
 * pan -1 (left) .. +1 (right), 0 = center. Frames count returned. */
int  audio_to_stereo_pan(const Sound* s, float pan, int16_t** out_lr);

void audio_free_pcm(int16_t* pcm);

#endif /* LEGORRR_AUDIO_HPP */
