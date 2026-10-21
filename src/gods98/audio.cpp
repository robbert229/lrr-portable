// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdlib.h>

#include "audio.hpp"

int audio_open(AudioPlayer* p)
{
    if (p == NULL) {
        return -1;
    }
    p->dev = platform_audio_open();
    if (p->dev == NULL) {
        return -1;
    }
    p->volume = 1.0f;
    platform_audio_set_gain(p->dev, p->volume);
    return 0;
}

void audio_close(AudioPlayer* p)
{
    if (p == NULL) {
        return;
    }
    platform_audio_close(p->dev);
    p->dev = NULL;
    p->loop_sound = NULL;
}

void audio_set_volume(AudioPlayer* p, float volume)
{
    if (p == NULL) {
        return;
    }
    if (volume < 0.0f) {
        volume = 0.0f;
    }
    if (volume > 1.0f) {
        volume = 1.0f;
    }
    p->volume = volume;
    if (p->dev != NULL) {
        platform_audio_set_gain(p->dev, volume);
    }
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

/* Constant-power balance: pan 0 keeps both channels full; -1/+1 pan fully
 * to one side. */
int audio_to_stereo_pan(const Sound* s, float pan, int16_t** out_lr)
{
    if (s == NULL || out_lr == NULL) {
        return -1;
    }
    int16_t* mono = NULL;
    int frames = audio_to_16mono(s, &mono);
    if (frames <= 0) {
        return -1;
    }
    if (pan < -1.0f) {
        pan = -1.0f;
    }
    if (pan > 1.0f) {
        pan = 1.0f;
    }
    float lw = pan <= 0.0f ? 1.0f : 1.0f - pan;
    float rw = pan >= 0.0f ? 1.0f : 1.0f + pan;
    int16_t* lr = (int16_t*)calloc((size_t)frames * 2, 2);
    if (lr != NULL) {
        for (int i = 0; i < frames; ++i) {
            lr[i * 2]     = (int16_t)(mono[i] * lw);
            lr[i * 2 + 1] = (int16_t)(mono[i] * rw);
        }
    }
    audio_free_pcm(mono);
    if (lr == NULL) {
        return -1;
    }
    *out_lr = lr;
    return frames;
}

int audio_play_sound_3d(AudioPlayer* p, const Sound* s, float pan)
{
    if (p == NULL || s == NULL) {
        return -1;
    }
    int16_t* lr = NULL;
    int frames = audio_to_stereo_pan(s, pan, &lr);
    if (frames <= 0) {
        return -1;
    }
    int rc = platform_audio_play_stereo(p->dev, lr, frames,
                                        (int)s->format.sample_rate);
    audio_free_pcm(lr);
    return rc == 0 ? frames : -1;
}

int audio_loop_named(AudioPlayer* p, const char* name, float pan)
{
    if (p == NULL || name == NULL) {
        return -1;
    }
    Sound* s = sound_bank_find(name);
    if (s == NULL) {
        if (sound_bank_load(name, 0 /* memory */, 1 /* looping */) != 0) {
            return -1;
        }
        s = sound_bank_find(name);
        if (s == NULL) {
            return -1;
        }
    }
    p->loop_sound = s;
    p->loop_pan = pan;
    p->loop_render_dirty = 1;
    return 0;
}

void audio_set_loop_pan(AudioPlayer* p, float pan)
{
    if (p == NULL || p->loop_sound == NULL) {
        return;
    }
    if (pan < -1.0f) {
        pan = -1.0f;
    }
    if (pan > 1.0f) {
        pan = 1.0f;
    }
    if (pan != p->loop_pan) {
        p->loop_pan = pan;
        p->loop_render_dirty = 1;
    }
}

void audio_update(AudioPlayer* p)
{
    if (p == NULL) {
        return;
    }
    if (p->loop_sound != NULL && p->loop_render_dirty) {
        int16_t* lr = NULL;
        int frames = audio_to_stereo_pan(p->loop_sound, p->loop_pan, &lr);
        if (frames > 0) {
            platform_audio_loop(p->dev, lr, frames,
                                (int)p->loop_sound->format.sample_rate);
            audio_free_pcm(lr);
        }
        p->loop_render_dirty = 0;
    }
    platform_audio_refill(p->dev);
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
    p->loop_sound = NULL;
    p->loop_render_dirty = 0;
}