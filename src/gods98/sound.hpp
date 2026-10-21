// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — sound loader scaffold (finding 14 §1–2).
 *
 * The game adds no custom audio container: SFX/voices are plain RIFF/WAVE
 * files fetched through the same disk-or-WAD layer as everything else, with
 * the classic `RIFF -> fmt -> (fact) -> data` chunk walk (WINMM!mmio* in the
 * original; a portable parser here). The shipped corpus is all PCM (format
 * tag 1): 8/16-bit, mono (one stereo file), rates 11025/16000/22050/44100,
 * with extra `LIST`/`cue`/`smpl`/`acid`/`fact` chunks in varying order.
 *
 * The sound bank mirrors finding 14 §1: a fixed 600-slot table
 * (corpus DAT_00506698, stride 0x134) whose slots carry loop/stream flags
 * (bit 2 lensed = looping, bit 3 = streamed). Actual playback (DirectSound;
 * streamed voices) is a later milestone — this is the loader half.
 */
#ifndef LEGORRR_SOUND_HPP
#define LEGORRR_SOUND_HPP

#include <cstdint>
#include <cstddef>

#define SOUND_LOOPING  0x4u
#define SOUND_STREAMED 0x8u
#define SOUND_BANK_SLOTS 600 /* finding 14 §1: (0x5339a8 - 0x506798)/0x134 */

/* fmt chunk (16 bytes, PCM). */
typedef struct WavSample {
    uint16_t format_tag;    /* 1 = PCM */
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
} WavSample;

/* A loaded sound. `data` is the raw PCM payload of the data chunk. */
typedef struct Sound {
    char*     name;        /* resource name (or NULL for parsed-from-buffer) */
    WavSample format;
    uint8_t*  data;        /* PCM bytes (8-bit unsigned, 16-bit signed LE) */
    uint32_t  data_len;
    uint32_t  flags;       /* SOUND_LOOPING | SOUND_STREAMED */
} Sound;

/* Parse a RIFF/WAVE buffer. Chunk order-independent; PCM (tag 1) only; NULL
 * for anything else (missing fmt/data, non-WAVE, truncated, RLE'd etc.). */
Sound* sound_parse(const uint8_t* data, uint32_t size);

void sound_free(Sound* s);

/* Load by resource name (disk or WAD) through the resource layer. */
Sound* sound_load(const char* name);

/* Sound bank (finding 14 §1). `name` is the base name — "<name>.wav" is
 * built internally (corpus "%s.wav" 0x4aae5c). `streamed`/`looping` set the
 * two flag bits. Returns 0 on success (or already loaded), -1 on failure. */
int sound_bank_load(const char* name, int streamed, int looping);

/* Case-insensitive lookup by the stored "<name>.wav" resource name. */
Sound* sound_bank_find(const char* fullname);

int  sound_bank_count(void);
void sound_bank_clear(void);

/* FNV-1a over the PCM payload (tests compare against wave-module-derived
 * reference hashes from the shipped corpus). */
uint32_t sound_pcm_hash(Sound* s);

#endif /* LEGORRR_SOUND_HPP */