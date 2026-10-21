// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — one-shot audio playback glue (finding 14 §1).
 *
 * Rides the platform's audio surface (platform.h, a DirectSound analog over
 * SDL). Converts any bank Sound — 8-bit unsigned or 16-bit signed, mono or
 * stereo, arbitrary sample rate — to mono 16-bit signed and submits it as a
 * one-shot. Playback of the loop/streamed voices and volume/3D positioning
 * (FUN_00464ef0, RollOffFor3DSounds…) are later milestones.
 */
#ifndef LEGORRR_AUDIO_HPP
#define LEGORRR_AUDIO_HPP

#include <cstdint>
#include <cstddef>

#include "platform/platform.h"
#include "sound.hpp"

typedef struct AudioPlayer {
    PlatformAudio* dev;   /* owned platform audio device */
} AudioPlayer;

int  audio_open(AudioPlayer* p);     /* 0 on success, non-zero otherwise */
void audio_close(AudioPlayer* p);

/* Submit one bank sound as a one-shot. Returns the number of frames queued,
 * or -1 on failure. */
int  audio_play_sound(AudioPlayer* p, const Sound* s);

/* Load + play by resource name through the sound bank. -1 on failure. */
int  audio_play_named(AudioPlayer* p, const char* name);

void audio_stop(AudioPlayer* p);

/* Convert a bank Sound to mono 16-bit signed PCM (the seam's transport).
 * Returns frame count; *out is allocated (free with audio_free_pcm). */
int  audio_to_16mono(const Sound* s, int16_t** out);

void audio_free_pcm(int16_t* pcm);

#endif /* LEGORRR_AUDIO_HPP */