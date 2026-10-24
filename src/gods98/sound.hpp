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

/* ------------------------------------------------------------------ *
 *  Sound alias registry (finding 137; issues #313 / #321 / #314).
 *
 *  The corpus builds the index-to-alias-name table at startup in
 *  SoundAliasRegistryInit @ 0x00464740: allocate a 0x7bc-byte (495-slot)
 *  word table, write the uppercase string hash (GameStringHashCompute,
 *  unported here — its only observable effect is the uppercase fold) of
 *  each of 45 hard-coded "SFX_*"
 *  names to slots 0..44, and register the scene sample hook
 *  (DDiSceneSampleHookRegister, engine-side, TODO(corpus) here).
 *  SoundAliasLookup @ 0x464c70 resolves a name through
 *  that table (case-insensitive: both sides pass the KeyToAscii uppercase
 *  fold, which the port
 *  applies directly with s_strcasecmp), and — only while
 *  SoundDynamicAliasRegistrationSet @ 0x00464c50 has raised DAT_00502a2c
 *  bit 2 — appends unknown names at the end of the table (dynamic
 *  aliases, DAT_00502a24 count) so `Sounds.*`-style names resolve to
 *  fresh indices without a config re-read.
 *
 *  Port modelling: the corpus slots hold only the 32-bit hash key; the
 *  port keeps the alias *names* (built-ins as static literals, dynamic
 *  copies allocated), so equality is decided with the same fold the
 *  corpus's hash keys collapse to. Hash-collision resolution is the one
 *  corpus behaviour not reproduced (collisions are unobservable for
 *  distinct shipped names).
 * ------------------------------------------------------------------ */

#define SOUND_ALIAS_BUILTINS 45  /* DAT_00502a24 base 0x2d in the lookup */
#define SOUND_ALIAS_MAX 495      /* 0x7bc-byte table, one alias per word */

/* SoundAliasRegistryInit @ 0x00464740: (re)build the registry. Deviations:
 * the corpus's second call leaks the old allocation while keeping the
 * stale dynamic count (uninitialised reads); the port frees the dynamic
 * copies and zeroes the count, keeping the registration flag (the corpus
 * leaves DAT_00502a2c untouched here). */
void sound_alias_registry_init(void);

/* SoundDynamicAliasRegistrationSet @ 0x00464c50: enable (nonzero) or
 * disable (zero) appending unknown names during lookup (DAT_00502a2c
 * bit 2). */
void sound_alias_registration_set(int enable);

/* SoundAliasLookup @ 0x00464c70: resolve `name` (case-insensitive) to an
 * alias index, writing it to *out_index. NULL `name` finds nothing (the
 * corpus's param_1 guard). A miss appends at the first free slot only
 * while registration is enabled — and the port returns 0 instead of the
 * corpus's out-of-bounds write once SOUND_ALIAS_MAX slots are full. */
int sound_alias_lookup(const char* name, int* out_index);

/* Current table size (built-ins + dynamic), i.e. DAT_00502a24 + 45. */
int sound_alias_count(void);

/* Alias name stored at `index`, or NULL out of range. The corpus table
 * only keeps hash keys; this exposes the port-side name mapping. */
const char* sound_alias_name(int index);

#endif /* LEGORRR_SOUND_HPP */
