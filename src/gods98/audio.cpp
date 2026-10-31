// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "audio.hpp"

static AudioPlayer* s_active_audio_player;
static AudioListenerRolloffHook s_listener_rolloff_hook;
static void* s_listener_rolloff_context;
static float s_listener_rolloff;
static AudioSystemInitHook s_audio_system_init_hook;
static void* s_audio_system_init_context;
static float s_audio_system_initial_volume = 1.0f;
// GLOBAL: LEGORR 0x00506694
static int s_audio_buffer_volume;

struct AudioBuffer {
    const Sound* sample;
    int16_t* pcm;
    int frames;
    int sample_rate;
    float gain;
    int millibels;
    int playing;
    int playing_loop;
};

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
    p->stream_dev[0] = NULL;
    p->stream_dev[1] = NULL;
    s_active_audio_player = p;
    p->volume = 1.0f;
    p->loop_sound = NULL;
    p->loop_pan = 0.0f;
    p->loop_render_dirty = 0;
    p->loop_buffer = NULL;
    p->stream_sample[0] = NULL;
    p->stream_sample[1] = NULL;
    memset(p->stream_format, 0, sizeof(p->stream_format));
    p->stream_buffer_bytes[0] = 0;
    p->stream_buffer_bytes[1] = 0;
    p->stream_cursor_bytes[0] = 0;
    p->stream_cursor_bytes[1] = 0;
    p->stream_active[0] = 0;
    p->stream_active[1] = 0;
    platform_audio_set_gain(p->dev, p->volume);
    return 0;
}

void audio_close(AudioPlayer* p)
{
    if (p == NULL) {
        return;
    }
    if (p->loop_buffer != NULL) {
        p->loop_buffer->playing = 0;
        p->loop_buffer->playing_loop = 0;
    }
    platform_audio_close(p->stream_dev[0]);
    platform_audio_close(p->stream_dev[1]);
    p->stream_dev[0] = NULL;
    p->stream_dev[1] = NULL;
    platform_audio_close(p->dev);
    p->dev = NULL;
    p->loop_sound = NULL;
    p->loop_pan = 0.0f;
    p->loop_render_dirty = 0;
    p->loop_buffer = NULL;
    p->stream_sample[0] = NULL;
    p->stream_sample[1] = NULL;
    memset(p->stream_format, 0, sizeof(p->stream_format));
    p->stream_buffer_bytes[0] = 0;
    p->stream_buffer_bytes[1] = 0;
    p->stream_cursor_bytes[0] = 0;
    p->stream_cursor_bytes[1] = 0;
    p->stream_active[0] = 0;
    p->stream_active[1] = 0;
    if (s_active_audio_player == p) {
        s_active_audio_player = NULL;
    }
}

void audio_system_player_set(AudioPlayer* p)
{
    s_active_audio_player = p;
}

AudioPlayer* audio_system_player_get(void)
{
    return s_active_audio_player;
}

// FUNCTION: LEGORR 0x0047a800
void audio_active_shutdown(void)
{
    if (!sound_bank_sound_on()) return;
    if (s_active_audio_player != NULL) {
        audio_stop(s_active_audio_player);
        audio_set_volume(s_active_audio_player,
                         s_audio_system_initial_volume);
        audio_close(s_active_audio_player);
    }
    sound_bank_sound_on_set(0);
}

void audio_system_shutdown(void)
{
    sound_subsystem_stop_all();
    audio_active_shutdown();
}

void audio_system_init_hook_set(AudioSystemInitHook hook, void* context)
{
    s_audio_system_init_hook = hook;
    s_audio_system_init_context = context;
}

// FUNCTION: LEGORR 0x0047a640
int audio_system_init(void* window)
{
    int initialized = 0;
    sound_bank_sound_on_set(0);
    if (s_audio_system_init_hook != NULL)
        initialized = s_audio_system_init_hook(s_audio_system_init_context,
                                               window);
    else if (s_active_audio_player != NULL)
        initialized = audio_open(s_active_audio_player) == 0;
    if (initialized != 0) {
        sound_bank_sound_on_set(1);
        s_listener_rolloff = 1.0f;
        if (s_active_audio_player != NULL) {
            s_audio_system_initial_volume = s_active_audio_player->volume;
            audio_set_volume(s_active_audio_player, 1.0f);
        }
    }
    /* The reference's completion result is one after either a successful
     * device setup or its fallback/skip path; only its retry-dialog code
     * loops. */
    return 1;
}

void audio_set_volume(AudioPlayer* p, float volume)
{
    int stream_index;
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
    for (stream_index = 0; stream_index < 2; ++stream_index) {
        if (p->stream_dev[stream_index] != NULL)
            platform_audio_set_gain(p->stream_dev[stream_index], volume);
    }
}

// GLOBAL: LEGORR 0x004aae30
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
void audio_sound_volume_set(int millibels)
{
    if (s_bank_sound_on != 0) {
        if (sound_volume_value_valid(millibels) != 0) {
            s_audio_buffer_volume = millibels;
            if (s_active_audio_player == NULL) return;
            const float gain = millibels <= -10000
                ? 0.0f : powf(10.0f, (float)millibels / 2000.0f);
            audio_set_volume(s_active_audio_player, gain);
        }
    }
}

void audio_active_sound_volume_set(int millibels)
{
    audio_sound_volume_set(millibels);
}

// FUNCTION: LEGORR 0x0047b550
void audio_sound_volume_minus_300(void)
{
    if (s_bank_sound_on != 0)
        audio_sound_volume_set(-300);
}

/* SoundVolumeIndexSet (FUN_0047b530): the table lookup is bounded to the
 * eleven menu slider entries, then the selected attenuation is applied. */
// FUNCTION: LEGORR 0x0047b530
void audio_sound_volume_index_set(int index)
{
    if (index >= 0 && index < 11) {
        audio_sound_volume_set(s_sfx_attenuation[index]);
    }
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

void audio_music_mixer_volume_get(float* music, float* sfx)
{
    float volume = s_active_audio_player != NULL
        ? s_active_audio_player->volume : 1.0f;
    if (music != NULL) *music = volume;
    if (sfx != NULL) *sfx = volume;
}

// FUNCTION: LEGORR 0x004789c0
int audio_mixer_control_init(float* music, float* sfx, int apply_volume)
{
    if (music == NULL || sfx == NULL) return 0;
    if (apply_volume == 0) {
        audio_music_mixer_volume_get(music, sfx);
        return s_active_audio_player != NULL && s_active_audio_player->dev != NULL;
    }
    if (*music > 1.0f) *music = 1.0f;
    if (*music < 0.0f) *music = 0.0f;
    if (*sfx > 1.0f) *sfx = 1.0f;
    if (*sfx < 0.0f) *sfx = 0.0f;
    music_mixer_volume_set(*music, *sfx);
    return s_active_audio_player != NULL && s_active_audio_player->dev != NULL;
}

int audio_cd_mixer_volume_get(float* music, float* sfx)
{
    if (music == NULL || sfx == NULL) return 0;
    *music = 0.0f;
    *sfx = 0.0f;
#if defined(LEGORRR_BACKEND_WINAPI)
    return platform_cd_mixer_volume_get(music, sfx);
#else
    return 0;
#endif
}

void audio_listener_rolloff_hook_set(AudioListenerRolloffHook hook,
                                     void* context)
{
    s_listener_rolloff_hook = hook;
    s_listener_rolloff_context = context;
}

float audio_listener_rolloff_set(float rolloff)
{
    if (rolloff < 0.0f) rolloff = 0.0f;
    if (rolloff > 10.0f) rolloff = 10.0f;
    if (sound_bank_sound_on()) {
        s_listener_rolloff = rolloff;
        if (s_listener_rolloff_hook != NULL)
            s_listener_rolloff_hook(s_listener_rolloff_context, rolloff);
    }
    return rolloff;
}

float audio_listener_rolloff_get(void)
{
    return s_listener_rolloff;
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

int audio_active_play_sound_3d(const Sound* s, float pan)
{
    if (!sound_bank_sound_on() || s_active_audio_player == NULL) return -1;
    return audio_play_sound_3d(s_active_audio_player, s, pan);
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
    return audio_loop_sample(p, s, pan);
}

int audio_loop_sample(AudioPlayer* p, const Sound* s, float pan)
{
    int16_t* lr = NULL;
    int frames;
    if (p == NULL || s == NULL || p->dev == NULL) return -1;
    if (pan < -1.0f) pan = -1.0f;
    if (pan > 1.0f) pan = 1.0f;
    frames = audio_to_stereo_pan(s, pan, &lr);
    if (frames <= 0) return -1;
    if (platform_audio_loop(p->dev, lr, frames,
                            (int)s->format.sample_rate) != 0) {
        audio_free_pcm(lr);
        return -1;
    }
    audio_free_pcm(lr);
    if (p->loop_buffer != NULL)
        p->loop_buffer->playing_loop = 0;
    p->loop_buffer = NULL;
    p->loop_sound = (Sound*)s;
    p->loop_pan = pan;
    p->loop_render_dirty = 0;
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
    int stream_index;
    if (p == NULL) {
        return;
    }
    platform_audio_stop(p->dev);
    for (stream_index = 0; stream_index < 2; ++stream_index)
        audio_stream_stop(p, stream_index);
    if (p->loop_buffer != NULL) {
        p->loop_buffer->playing = 0;
        p->loop_buffer->playing_loop = 0;
    }
    p->loop_sound = NULL;
    p->loop_buffer = NULL;
    p->loop_render_dirty = 0;
}

AudioBuffer* audio_buffer_create(const Sound* sample)
{
    AudioBuffer* buffer;
    int16_t* pcm = NULL;
    int frames;
    if (sample == NULL) return NULL;
    frames = audio_to_16mono(sample, &pcm);
    if (frames <= 0) return NULL;
    buffer = (AudioBuffer*)calloc(1, sizeof(AudioBuffer));
    if (buffer == NULL) {
        audio_free_pcm(pcm);
        return NULL;
    }
    buffer->pcm = pcm;
    buffer->sample = sample;
    buffer->frames = frames;
    buffer->sample_rate = (int)sample->format.sample_rate;
    buffer->gain = 1.0f;
    buffer->millibels = 0;
    return buffer;
}

void audio_buffer_destroy(AudioBuffer* buffer)
{
    if (buffer == NULL) return;
    audio_buffer_stop(buffer);
    free(buffer->pcm);
    free(buffer);
}

void audio_buffer_volume_set(AudioBuffer* buffer, int millibels)
{
    if (buffer == NULL) return;
    if (sound_volume_value_valid(millibels) == 0) millibels = 0;
    buffer->millibels = millibels;
    buffer->gain = millibels <= -10000 ? 0.0f
        : powf(10.0f, (float)millibels / 2000.0f);
}

int audio_buffer_play(AudioBuffer* buffer)
{
    return audio_buffer_play_attenuated(buffer, 0);
}

int audio_buffer_play_attenuated(AudioBuffer* buffer, int extra_millibels)
{
    int rc;
    int i;
    int16_t* scaled;
    float gain;
    AudioPlayer* player = s_active_audio_player;
    if (buffer == NULL || player == NULL || player->dev == NULL
        || buffer->pcm == NULL || buffer->frames <= 0) return -1;
    gain = buffer->gain;
    /* DirectSound rejects SetVolume requests below -10000. The reference
     * ignores that HRESULT, so retain the configured gain in that edge. */
    if (extra_millibels != 0 && buffer->millibels + extra_millibels >= -10000)
        gain = powf(10.0f,
                    (float)(buffer->millibels + extra_millibels) / 2000.0f);
    if (gain == 1.0f) {
        rc = platform_audio_play(player->dev, buffer->pcm, buffer->frames,
                                 buffer->sample_rate);
    } else {
        scaled = (int16_t*)calloc((size_t)buffer->frames, sizeof(int16_t));
        if (scaled == NULL) return -1;
        for (i = 0; i < buffer->frames; ++i)
            scaled[i] = (int16_t)(buffer->pcm[i] * gain);
        rc = platform_audio_play(player->dev, scaled, buffer->frames,
                                 buffer->sample_rate);
        free(scaled);
    }
    buffer->playing = rc == 0;
    buffer->playing_loop = 0;
    return rc == 0 ? buffer->frames : -1;
}

void audio_buffer_stop(AudioBuffer* buffer)
{
    AudioPlayer* player = s_active_audio_player;
    if (buffer == NULL) return;
    if (buffer->playing_loop && player != NULL && player->dev != NULL
        && player->loop_buffer == buffer) {
        (void)platform_audio_loop(player->dev, NULL, 0, 0);
        player->loop_sound = NULL;
        player->loop_buffer = NULL;
        player->loop_render_dirty = 0;
    }
    /* The SDL queue has no voice identifiers for one-shot samples. */
    buffer->playing = 0;
    buffer->playing_loop = 0;
}

int audio_buffer_loop(AudioBuffer* buffer, const Sound* sample)
{
    return audio_buffer_loop_attenuated(buffer, sample, 0);
}

int audio_buffer_loop_attenuated(AudioBuffer* buffer, const Sound* sample,
                                 int extra_millibels)
{
    int i;
    int16_t* lr;
    float gain;
    AudioPlayer* player = s_active_audio_player;
    if (buffer == NULL || sample == NULL || player == NULL
        || player->dev == NULL || buffer->pcm == NULL || buffer->frames <= 0)
        return -1;
    lr = (int16_t*)calloc((size_t)buffer->frames * 2u, sizeof(int16_t));
    if (lr == NULL) return -1;
    gain = buffer->gain;
    if (extra_millibels != 0 && buffer->millibels + extra_millibels >= -10000)
        gain = powf(10.0f,
                    (float)(buffer->millibels + extra_millibels) / 2000.0f);
    for (i = 0; i < buffer->frames; ++i) {
        int16_t value = (int16_t)(buffer->pcm[i] * gain);
        lr[i * 2] = value;
        lr[i * 2 + 1] = value;
    }
    i = platform_audio_loop(player->dev, lr, buffer->frames,
                            buffer->sample_rate);
    free(lr);
    if (i != 0) return -1;
    if (player->loop_buffer != NULL)
        player->loop_buffer->playing_loop = 0;
    player->loop_buffer = buffer;
    player->loop_sound = (Sound*)sample;
    player->loop_pan = 0.0f;
    player->loop_render_dirty = 0;
    buffer->playing = 1;
    buffer->playing_loop = 1;
    return 0;
}

int audio_stream_setup(AudioPlayer* p, int stream_index,
                       const WavSample* format, uint32_t buffer_bytes)
{
    uint32_t frame_bytes;
    if (p == NULL || p->dev == NULL || stream_index < 0 || stream_index > 1
        || format == NULL || format->format_tag != 1
        || (format->channels != 1 && format->channels != 2)
        || (format->bits_per_sample != 8 && format->bits_per_sample != 16))
        return 0;
    frame_bytes = (uint32_t)format->channels
                * ((uint32_t)format->bits_per_sample / 8u);
    if (frame_bytes == 0u || format->sample_rate == 0u
        || format->block_align != frame_bytes || buffer_bytes < frame_bytes)
        return 0;
    audio_stream_stop(p, stream_index);
    p->stream_buffer_bytes[stream_index] =
        buffer_bytes - buffer_bytes % frame_bytes;
    p->stream_format[stream_index] = *format;
    p->stream_sample[stream_index] = NULL;
    p->stream_cursor_bytes[stream_index] = 0;
    p->stream_active[stream_index] = 0;
    return 1;
}

/* Feed one aligned slice from a loaded WAVE sample to this stream's owned
 * output queue. */
static int audio_stream_submit_chunk(AudioPlayer* p, int stream_index)
{
    const Sound* sample;
    Sound slice;
    int16_t* pcm;
    int frames;
    uint32_t frame_bytes;
    uint32_t amount;
    if (p == NULL || stream_index < 0 || stream_index > 1
        || p->stream_dev[stream_index] == NULL) return 0;
    sample = p->stream_sample[stream_index];
    if (sample == NULL || sample->data == NULL
        || p->stream_cursor_bytes[stream_index] >= sample->data_len)
        return 0;
    frame_bytes = (uint32_t)sample->format.channels
                * ((uint32_t)sample->format.bits_per_sample / 8u);
    if (frame_bytes == 0u) return 0;
    amount = p->stream_buffer_bytes[stream_index];
    if (amount == 0u) amount = frame_bytes * 4096u;
    amount -= amount % frame_bytes;
    if (amount == 0u) return 0;
    amount = sample->data_len - p->stream_cursor_bytes[stream_index] < amount
        ? sample->data_len - p->stream_cursor_bytes[stream_index] : amount;
    amount -= amount % frame_bytes;
    if (amount == 0u || amount > 0x7fffffffu) return 0;
    memset(&slice, 0, sizeof(slice));
    slice.format = sample->format;
    slice.data = sample->data + p->stream_cursor_bytes[stream_index];
    slice.data_len = amount;
    frames = audio_to_16mono(&slice, &pcm);
    if (frames <= 0) return 0;
    if (platform_audio_play(p->stream_dev[stream_index], pcm, frames,
                            (int)sample->format.sample_rate) != 0) {
        audio_free_pcm(pcm);
        return 0;
    }
    audio_free_pcm(pcm);
    p->stream_cursor_bytes[stream_index] += amount;
    return frames;
}

int audio_stream_start(AudioPlayer* p, int stream_index, const Sound* sample)
{
    uint32_t frame_bytes;
    uint32_t buffer_bytes;
    if (p == NULL || stream_index < 0 || stream_index > 1 || sample == NULL
        || p->dev == NULL || sample->data == NULL || sample->data_len == 0u)
        return 0;
    if (sample->format.format_tag != 1
        || (sample->format.channels != 1 && sample->format.channels != 2)
        || (sample->format.bits_per_sample != 8 &&
            sample->format.bits_per_sample != 16)) return 0;
    frame_bytes = (uint32_t)sample->format.channels
                * ((uint32_t)sample->format.bits_per_sample / 8u);
    if (frame_bytes == 0u || sample->format.sample_rate == 0u
        || sample->format.block_align != frame_bytes) return 0;
    buffer_bytes = p->stream_buffer_bytes[stream_index];
    audio_stream_stop(p, stream_index);
    p->stream_buffer_bytes[stream_index] = buffer_bytes;
    p->stream_dev[stream_index] = platform_audio_open();
    if (p->stream_dev[stream_index] == NULL) return 0;
    (void)platform_audio_set_gain(p->stream_dev[stream_index], p->volume);
    if (p->stream_buffer_bytes[stream_index] == 0u)
        p->stream_buffer_bytes[stream_index] = frame_bytes * 4096u;
    p->stream_format[stream_index] = sample->format;
    p->stream_sample[stream_index] = sample;
    p->stream_cursor_bytes[stream_index] = 0;
    p->stream_active[stream_index] = 1;
    if (audio_stream_submit_chunk(p, stream_index) <= 0) {
        audio_stream_stop(p, stream_index);
        return 0;
    }
    return 1;
}

void audio_stream_stop(AudioPlayer* p, int stream_index)
{
    if (p == NULL || stream_index < 0 || stream_index > 1) return;
    if (p == s_active_audio_player) {
        (void)sound_stream_stop_and_release(stream_index);
        return;
    }
    platform_audio_close(p->stream_dev[stream_index]);
    p->stream_dev[stream_index] = NULL;
    p->stream_active[stream_index] = 0;
    p->stream_sample[stream_index] = NULL;
    p->stream_buffer_bytes[stream_index] = 0;
    p->stream_cursor_bytes[stream_index] = 0;
    memset(&p->stream_format[stream_index], 0, sizeof(WavSample));
}

// FUNCTION: LEGORR 0x0047ba20
int sound_stream_stop_and_release(int stream_index)
{
    AudioPlayer* p = s_active_audio_player;
    int slot = stream_index == 0 ? 0 : 1;
    if (p != NULL) {
        platform_audio_close(p->stream_dev[slot]);
        p->stream_dev[slot] = NULL;
        p->stream_active[slot] = 0;
        p->stream_sample[slot] = NULL;
        p->stream_buffer_bytes[slot] = 0;
        p->stream_cursor_bytes[slot] = 0;
        memset(&p->stream_format[slot], 0, sizeof(WavSample));
    }
    return 1;
}

// FUNCTION: LEGORR 0x0047bc30
int sound_stream_drain_check(int stream_index)
{
    AudioPlayer* p = s_active_audio_player;
    int slot = stream_index == 0 ? 0 : 1;
    const Sound* sample;
    uint32_t before;
    uint32_t frame_bytes;
    int queued_ms;
    int buffer_ms;
    int submitted;
    if (p == NULL || !p->stream_active[slot]
        || p->stream_dev[slot] == NULL) return 0;
    sample = p->stream_sample[slot];
    if (sample == NULL) return 0;
    queued_ms = platform_audio_queued_milliseconds(
        p->stream_dev[slot]);
    if (p->stream_cursor_bytes[slot] >= sample->data_len) {
        if (queued_ms == 0) (void)sound_stream_stop_and_release(slot);
        return 0;
    }
    frame_bytes = p->stream_format[slot].block_align;
    if (frame_bytes == 0u || p->stream_format[slot].byte_rate == 0u)
        return 0;
    buffer_ms = (int)(((double)p->stream_buffer_bytes[slot] * 1000.0)
                      / p->stream_format[slot].byte_rate);
    if (buffer_ms < 2) buffer_ms = 2;
    if (queued_ms >= buffer_ms / 2) return 0;
    before = p->stream_cursor_bytes[slot];
    submitted = audio_stream_submit_chunk(p, slot);
    if (submitted <= 0) return 0;
    return (int)(p->stream_cursor_bytes[slot] - before);
}

// FUNCTION: LEGORR 0x0047bdb0
void sound_stream_ring_buffer_tick(int stream_index)
{
    AudioPlayer* p = s_active_audio_player;
    int slot = stream_index == 0 ? 0 : 1;
    if (p == NULL || !p->stream_active[slot]
        || p->stream_sample[slot] == NULL
        || p->stream_dev[slot] == NULL) return;
    if (p->stream_cursor_bytes[slot] >= p->stream_sample[slot]->data_len
        && platform_audio_queued_milliseconds(p->stream_dev[slot]) == 0) {
        (void)sound_stream_stop_and_release(slot);
        return;
    }
    (void)sound_stream_drain_check(slot);
}
