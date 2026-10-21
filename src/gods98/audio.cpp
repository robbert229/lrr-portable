// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdlib.h>

#include "audio.hpp"

int audio_open(AudioPlayer* p)
{
    if (p == NULL) {
        return -1;
    }
    p->dev = platform_audio_open();
    return p->dev != NULL ? 0 : -1;
}

void audio_close(AudioPlayer* p)
{
    if (p == NULL) {
        return;
    }
    platform_audio_close(p->dev);
    p->dev = NULL;
}

/* Corpus sounds are all PCM: 8-bit unsigned or 16-bit signed LE, mostly
 * mono (one stereo file). Convert stereo -> mono by averaging channels. */
int audio_to_16mono(const Sound* s, int16_t** out)
{
    if (s == NULL || out == NULL || s->data == NULL || s->data_len == 0) {
        return -1;
    }
    int ch   = s->format.channels;
    int bits = s->format.bits_per_sample;
    if (ch != 1 && ch != 2) {
        return -1;
    }
    int bytes_per_frame = ch * (bits / 8);
    if (bytes_per_frame <= 0) {
        return -1;
    }
    int frames = (int)(s->data_len / (uint32_t)bytes_per_frame);
    int16_t* pcm = (int16_t*)calloc(frames > 0 ? (size_t)frames : 1u, 2);
    if (pcm == NULL) {
        return -1;
    }
    if (bits == 8) {
        for (int i = 0; i < frames; ++i) {
            int sum = 0;
            for (int c = 0; c < ch; ++c) {
                sum += (int)s->data[i * ch + c];
            }
            int v = (sum / ch) - 128;
            pcm[i] = (int16_t)(v << 8);
        }
    } else { /* 16-bit */
        for (int i = 0; i < frames; ++i) {
            int off = i * ch;
            if (ch == 1) {
                pcm[i] = (int16_t)(s->data[off * 2]
                                   | (s->data[off * 2 + 1] << 8));
            } else {
                int l = (int16_t)(s->data[off * 2] | (s->data[off * 2 + 1] << 8));
                int r = (int16_t)(s->data[(off + 1) * 2]
                                  | (s->data[(off + 1) * 2 + 1] << 8));
                pcm[i] = (int16_t)((l + r) / 2);
            }
        }
    }
    *out = pcm;
    return frames;
}

void audio_free_pcm(int16_t* pcm)
{
    free(pcm);
}

int audio_play_sound(AudioPlayer* p, const Sound* s)
{
    if (p == NULL || s == NULL) {
        return -1;
    }
    int16_t* pcm = NULL;
    int frames = audio_to_16mono(s, &pcm);
    if (frames <= 0) {
        return -1;
    }
    int rc = platform_audio_play(p->dev, pcm, frames, (int)s->format.sample_rate);
    audio_free_pcm(pcm);
    return rc == 0 ? frames : -1;
}

int audio_play_named(AudioPlayer* p, const char* name)
{
    if (p == NULL || name == NULL) {
        return -1;
    }
    Sound* s = sound_bank_find(name);
    if (s == NULL) {
        if (sound_bank_load(name, 0 /* memory */, 0) != 0) {
            return -1;
        }
        s = sound_bank_find(name);
        if (s == NULL) {
            return -1;
        }
    }
    return audio_play_sound(p, s);
}

void audio_stop(AudioPlayer* p)
{
    if (p == NULL) {
        return;
    }
    platform_audio_stop(p->dev);
}