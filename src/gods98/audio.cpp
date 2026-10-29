// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdlib.h>
#include <math.h>

#include "audio.hpp"

static AudioPlayer* s_active_audio_player;

/* The corpus menu-volume gain is a standalone .rdata dword (verified
 * 0x49e1e0 = 0.11111111f); the compat band's volume_scale member is now
 * dead space whose bytes the band still carries. */
// GLOBAL: LEGORR 0x0049e1e0
static const float s_menu_volume_scale = 0.11111111f; /* DAT_0049e1e0 */

#if defined(LEGORRR_BACKEND_WINAPI)
#pragma pack(push, 1)
struct MenuCompatData {
    unsigned char scale_prefix[0x25a08];
    float volume_scale;
    unsigned char null_label_prefix[0x4210];
    char null_label[5];
};
#pragma pack(pop)
#pragma data_seg(".data$yyyyyy")
__declspec(allocate(".data$yyyyyy")) static MenuCompatData
    s_menu_compat_data = { { 0 }, 0.11111111f, { 0 }, "Null" };
#pragma data_seg()
#endif

int audio_open(AudioPlayer* p)
{
    if (p == NULL) {
        return -1;
    }
    p->dev = platform_audio_open();
    if (p->dev == NULL) {
        return -1;
    }
    s_active_audio_player = p;
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
    if (s_active_audio_player == p) {
        s_active_audio_player = NULL;
    }
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

static const int s_sfx_attenuation[11] = {
    -10000, -5000, -3500, -2000, -1500, -1000,
      -700,  -400,  -200,   -80,     0
};

float audio_sfx_volume_gain(int index)
{
    if (index < 0 || index >= 11) {
        return -1.0f;
    }
    if (s_sfx_attenuation[index] <= -10000) {
        return 0.0f; /* DirectSound defines -100 dB as silence. */
    }
    /* DirectSound SetVolume uses hundredths of a decibel. Convert dB to
     * amplitude: 10^(dB/20), hence attenuation/2000. */
    return powf(10.0f, (float)s_sfx_attenuation[index] / 2000.0f);
}

/* SoundBufferVolumeSet (FUN_0047b500): DirectSound's attenuation maps to
 * the port's single master stream gain. Invalid corpus attenuation values
 * are ignored by the same range predicate used by sound-bank records. */
// FUNCTION: LEGORR 0x0047b500
void audio_sound_volume_set(AudioPlayer* p, int millibels)
{
    if (p == NULL || !sound_bank_sound_on()
        || !sound_volume_value_valid(millibels)) {
        return;
    }
    const float gain = millibels <= -10000
        ? 0.0f : powf(10.0f, (float)millibels / 2000.0f);
    audio_set_volume(p, gain);
}

/* SoundVolumeIndexSet (FUN_0047b530): the table lookup is bounded to the
 * eleven menu slider entries, then the selected attenuation is applied. */
// FUNCTION: LEGORR 0x0047b530
void audio_sound_volume_index_set(int index)
{
    /* The corpus uses `index < 0 || index > 10` for its eleven entries. */
    if (index < 0 || index > 10) {
        return;
    }
    audio_sound_volume_set(s_active_audio_player, s_sfx_attenuation[index]);
}

/* GameSpeedSelect (FUN_00414de0): the ledger name is misleading; this is
 * the SFX slider callback and simply forwards its index. */
// FUNCTION: LEGORR 0x00414de0
void audio_menu_sfx_volume_set(int index)
{
    audio_sound_volume_index_set(index);
}

/* VolumeSet (FUN_00414df0): the corpus sends param_1 / 9 to both mixer
 * channels. AudioPlayer exposes one master output gain, so apply the same
 * normalized value once; audio_set_volume provides the device-side clamp. */
// FUNCTION: LEGORR 0x00414df0
#if defined(LEGORRR_BACKEND_WINAPI)
__declspec(naked) void audio_menu_volume_set(int value)
{
    __asm {
        fild dword ptr [esp + 4]
        fmul dword ptr [s_menu_volume_scale]
        fstp dword ptr [esp + 4]
        mov eax, dword ptr [esp + 4]
        push eax
        push eax
        call music_mixer_volume_set
        add esp, 8
        ret
    }
}
#else
void audio_menu_volume_set(int value)
{
    audio_set_volume(s_active_audio_player, (float)value * 0.11111111f);
}
#endif

// FUNCTION: LEGORR 0x00478980
void music_mixer_volume_set(float music, float sfx)
{
    (void)sfx;
    audio_set_volume(s_active_audio_player, music);
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
