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
 * two flag bits. Returns 0 on success (or already loaded), -1 on failure.
 * The dedupe check is a port-layer convenience: the corpus has none, its
 * level-init caller loads each file exactly once. */
int sound_bank_load(const char* name, int streamed, int looping);

/* Case-insensitive lookup by the stored "<name>.wav" resource name. */
Sound* sound_bank_find(const char* fullname);

int  sound_bank_count(void);
void sound_bank_clear(void);

/* ------------------------------------------------------------------ *
 *  Bank record lifecycle (the chain blocked.md roots at the bank —
 *  finding 14 §1; records of stride 0x134, busy-flag base
 *  DAT_005067c8, 600 slots).
 *
 *  Slot flag word (record +0x130): bit 0 = busy, bit 1 = loaded
 *  (set on success alongside bit 0, no port-side consumer yet),
 *  bit 2 = looping, bit 3 = streamed/memory mode. The corpus loads
 *  the WAV through the platform mmio path (InMemoryWaveSampleLoad's
 *  role); the port reuses its own resource-layer loader for the same
 *  observable result (documented seam).
 * ------------------------------------------------------------------ */

#define SOUND_SLOT_BUSY   1u
#define SOUND_SLOT_LOADED 2u
/* + SOUND_LOOPING / SOUND_STREAMED for bits 2/3. */

/* Port seam for the corpus's sound-system switch (DAT_00533878, raised
 * when DirectSound init succeeds and dropped by the
 * LoaderProfileNoSound profile). The port defaults it to on; the
 * engine's config read drives it when that milestone lands. */
void sound_bank_sound_on_set(int on);
int  sound_bank_sound_on(void);

/* SoundBankFreeSlotFind @ 0x47aad0: first slot whose busy bit is clear.
 * 1 with *out_slot set, or 0 when all 600 slots are claimed. */
int sound_bank_free_slot_find(int* out_slot);

/* SoundBankEntryLoad @ 0x47a870: claim a free slot, load
 * "<name>.wav", set the loop/stream bits per mode, validate `volume`
 * (millibels, see the validity helper) storing 0 when invalid, and
 * raise busy+loaded. Returns the slot index, or -1 when sound is off,
 * no slot is free, or the file fails to load (the claimed slot's flags
 * stay zeroed, exactly as shipped). No dedupe — the corpus has none. */
int sound_bank_entry_load(const char* name, int streamed, int looping,
                          int volume);

/* SoundBankEntryRelease @ 0x47ab00: clear the busy bit and free the
 * record's sample memory. Deviation: the corpus additionally releases
 * three DirectSound buffer interfaces through their vtables — the
 * instance layer is TODO(corpus), so that part of the seam is a no-op. */
void sound_bank_entry_release(int slot);

/* SoundVolumeValueValid @ 0x47a850: the millibel sanity gate — valid
 * while (-10001 < v < 1). */
int sound_volume_value_valid(int millibels);

/* Record inspection: flag word / stored volume for a slot, or
 * 0/0-invalid when the index is out of range. */
uint32_t sound_bank_entry_flags(int slot);
int sound_bank_entry_volume(int slot);

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

/* ------------------------------------------------------------------ *
 *  Sound subsystem enable state (finding 137; issues #315 / #322 / #323
 *  / #316) — the corpus flag word DAT_00502a2c: bit 0 subsystem-enabled,
 *  bit 1 dynamic-alias registration (see above), bit 3 batch mode. The
 *  enable/disable setter suppresses future submissions through this word
 *  once the submission queue lands (#296/#317); the port exposes the
 *  state and consumers gate on it there.
 * ------------------------------------------------------------------ */

#define SOUND_FLAG_ENABLED       1u /* DAT_00502a2c bit 0 @ 0x00465260   */
#define SOUND_FLAG_REG_UNKNOWN   2u /* bit 1 — registration_set          */
#define SOUND_FLAG_BATCH         8u /* bit 3 — batch-mode-get            */

/* SoundSubsystemEnableSet @ 0x465260: raise bit
 * 0 on enable (and return — nothing else runs); on disable clear bit 0
 * and, when stop_active is nonzero, run the stop seam. The corpus's stop
 * seam is SoundSubsystemStopAll (unported here) — it walks the live
 * chain (DAT_00506684) and stops both stream states; that instance layer
 * is TODO(corpus), so the port's seam is a no-op today (documented
 * deviation: only the state transition is observable until instances
 * land). Disabling also zeroes the cooldown word _DAT_00502b2c (the last
 * alias submitted under the global cooldown — written by the submission
 * queue port #296). */
void sound_subsystem_enable_set(int enable, int stop_active);

/* SoundSubsystemEnableSetAndStop @ 0x4652a0: enable_set with the stop
 * seam armed. */
void sound_subsystem_enable_set_and_stop(int enable);

/* SoundSubsystemEnabledGet @ 0x465370: bit 0. */
int sound_subsystem_enabled(void);

/* SoundBatchModeGet @ 0x4652b0: bit 3. */
int sound_batch_mode_get(void);

/* The corpus's cooldown word _DAT_00502b2c: holds the alias submitted
 * under the global cooldown (its writer lands with submission queue
 * #296); disable zeroes the word. */
double sound_deferred_last_alias(void);

/* ------------------------------------------------------------------ *
 *  Deferred sound request queue (issue #317; aggregate #296 write
 *  side).
 *
 *  The corpus's queue writer appends `alias` + `loop` to two parallel
 *  10-entry tables (the DAT_00502b30 alias words and the DAT_00502b58
 *  loop-flag words, count in DAT_00502bd0). A submission is dropped
 *  silently once the count reaches ten; nothing else is observable in
 *  the writer. The per-frame owner (SoundSubsystemFrameTick, unported
 *  here) rotates the pending slots into the active playback copies and
 *  zeroes the count; that consumer lands with its own milestone.
 * ------------------------------------------------------------------ */

#define SOUND_REQUEST_QUEUE_CAPACITY 10 /* DAT_00502bd0 bound */

/* SoundRequestQueueSubmit @ 0x464f60: append an alias request
 * (`loop` nonzero marks the queued submission as looping — the corpus's
 * menu/reward callers pass 1 only for the two music loops). Drops when
 * full, like the corpus. */
void sound_request_queue_submit(int alias, int loop);

/* Pending count (DAT_00502bd0). */
int sound_request_queue_count(void);

/* Reads pending entry `index` into *alias/*loop; 1 on success, 0 out of
 * range. */
int sound_request_queue_get(int index, int* alias, int* loop);

/* Port-side reset of the pending count (the corpus clears the queue
 * only inside its frame tick; exposed for tests and pre-level reuse). */
void sound_request_queue_clear(void);

/* ------------------------------------------------------------------ *
 *  Deferred-play global cooldown (issues #326 / #327; the corpus's
 *  float timer _DAT_00502b24 counts down in the frame tick and its
 *  handle word DAT_00502b28 names the sound whose stop the expiry
 *  runs).
 *
 *  The idle setter gates deferred plays: nothing arms while a
 *  cooldown is already running. On expiry the corpus stops the
 *  recorded handle through the shared stop path
 *  (SoundHandleStopForward, unported here — instance layer TODO) and
 *  resets both words.
 * ------------------------------------------------------------------ */

/* DeferredSoundTimerSetIfIdle @ 0x464ec0: arm the cooldown to
 * `seconds` while idle (timer <= 0), forgetting any recorded handle
 * (the corpus writes -1 so the arming submit cannot be expired); 1 if
 * armed, 0 if a cooldown was already running. */
int sound_deferred_timer_set_if_idle(float seconds);

/* DeferredSoundPlayExpire @ 0x464e80: when a handle is recorded and
 * the timer still runs, stop it (port stop seam: no-op until the
 * instance layer lands, documented deviation), record -1, and zero
 * the timer. Otherwise a no-op. */
void sound_deferred_play_expire(void);

/* The cooldown timer word (_DAT_00502b24). */
float sound_deferred_cooldown(void);

/* Port seam recording the submitted-handle word DAT_00502b28 exactly
 * as the submission route's writer (SoundSubmitWithGlobalCooldown's
 * body, whose alias-play and duration callees are unported) would:
 * pass -1 to forget it. Expire only stops sounds this recorded. */
void sound_deferred_handle_record(int handle);

/* The recorded handle word (DAT_00502b28). */
int sound_deferred_handle(void);

#endif /* LEGORRR_SOUND_HPP */
