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
 * (bit 2 lensed = looping, bit 3 = streamed). The portable streamed voice
 * path submits aligned chunks through the active audio player; DirectSound
 * ring-buffer cursor semantics remain a backend seam.
 */
#ifndef LEGORRR_SOUND_HPP
#define LEGORRR_SOUND_HPP

#include <cstdint>
#include <cstddef>

typedef struct BlockRec BlockFile;

#define SOUND_LOOPING  0x4u
#define SOUND_STREAMED 0x8u
#define SOUND_BANK_SLOTS 600 /* finding 14 §1: (0x5339a8 - 0x506798)/0x134 */

/* In-place token split (FUN_00477440, gods98/utils.c) used by config
 * paths, menu image records and sound variants. */
#ifdef __cplusplus
extern "C"
#endif
int sound_variant_split(char* text, char** out, const char* separators);

/* fmt chunk (16 bytes, PCM). */
typedef struct WavSample {
    uint16_t format_tag;    /* 1 = PCM */
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
} WavSample;

typedef struct SoundWaveDataReader {
    const uint8_t* data;
    uint32_t data_size;
    uint32_t cursor;
    uint32_t remaining;
} SoundWaveDataReader;
void sound_wave_data_reader_init(SoundWaveDataReader* reader,
                                 const uint8_t* data, uint32_t size);
uint32_t sound_wave_data_reader_read(SoundWaveDataReader* reader,
                                     uint32_t requested, void* destination,
                                     uint32_t capacity, uint32_t* bytes_read);

/* A loaded sound. `data` is the raw PCM payload of the data chunk. */
typedef struct Sound {
    char*     name;        /* resource name (or NULL for parsed-from-buffer) */
    WavSample format;
    uint8_t*  data;        /* PCM bytes (8-bit unsigned, 16-bit signed LE) */
    uint32_t  data_len;
    uint32_t  flags;       /* SOUND_LOOPING | SOUND_STREAMED */
    uint32_t  file_len;    /* full buffer the chunks were parsed from (the
                            * corpus's stored stream size role) */
} Sound;

/* Load a loose wave file through the original's mmio readers (soundsys.c,
 * WaveOpenFile/WaveStartDataRead/WaveReadFile). PCM only; NULL if the file
 * is missing or not a PCM wave (sound_wave.cpp). */
Sound* sound_wave_file_load(const char* name);

/* Parse a RIFF/WAVE buffer. Chunk order-independent; PCM (tag 1) only; NULL
 * for anything else (missing fmt/data, non-WAVE, truncated, RLE'd etc.). */
Sound* sound_parse(const uint8_t* data, uint32_t size);

void sound_free(Sound* s);

/* Load by resource name (disk or WAD) through the resource layer. */
Sound* sound_load(const char* name);
#ifdef __cplusplus
extern "C"
#endif
int sound_sample_file_open_by_name(const char* name); /* 0x0047fb60 */
int sound_sample_load_into(Sound* output, const char* name, int mode_flags);
void sound_sample_clear(Sound* sound);

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
extern "C" int s_bank_sound_on; /* DAT_00533878, read by soundsys.c */

/* SoundBankFreeSlotFind @ 0x47aad0: first slot whose busy bit is clear.
 * 1 with *out_slot set, or 0 when all 600 slots are claimed. */
int sound_bank_free_slot_find(int* out_slot);

/* SoundBankEntryLoad @ 0x47a870: claim a free slot, load
 * "<name>.wav", set the loop/stream bits per mode, validate `volume`
 * (millibels, see the validity helper) storing 0 when invalid, and
 * raise busy+loaded. Returns the slot index, or -1 when sound is off,
 * no slot is free, or the file fails to load (the claimed slot's flags
 * stay zeroed, exactly as shipped). No dedupe — the corpus has none. */
#ifdef __cplusplus
extern "C"
#endif
int sound_bank_entry_load(const char* name, int streamed, int looping,
                          int volume);

/* SoundBankEntryRelease @ 0x47ab00: clear the busy bit and free the
 * record's sample memory and its portable PCM duplicate buffers. Returns 1
 * when the sound system is enabled and the slot is valid, otherwise 0. */
#ifdef __cplusplus
extern "C"
#endif
int sound_bank_entry_release(int slot);

/* SoundVolumeValueValid @ 0x47a850: the millibel sanity gate — valid
 * while (-10001 < v < 1). */
#ifdef __cplusplus
extern "C"
#endif
int sound_volume_value_valid(int millibels);

/* Record inspection: flag word / stored volume for a slot, or
 * 0/0-invalid when the index is out of range. The volume read is the
 * corpus's IndexedRecordStride134WordGet. */
uint32_t sound_bank_entry_flags(int slot);
#ifdef __cplusplus
extern "C"
#endif
int sound_bank_entry_volume(int slot);
Sound* sound_bank_entry_sample_get(int slot);
/* Create up to three portable duplicate-buffer records for the loaded slot. */
int sound_record_create_buffers(int slot);
int sound_wave_sample_duplicate_buffer_create(int slot, int buffer_index);
int sound_bank_entry_play(int slot, int loop);
int sound_bank_entry_play_mode(int slot, int loop, int mode);

/* SoundBankEntryVolumeSet @ 0x47b050: validate `millibels` (storing 0
 * when invalid) into the record's volume word and apply the equivalent
 * gain to buffer 0 or all three duplicate buffers when the loop bit is set.
 * The port guards the corpus's unbounded slot write. */
#ifdef __cplusplus
extern "C"
#endif
void sound_bank_entry_volume_set(int slot, int millibels);

/* ------------------------------------------------------------------ *
 *  Configured variant properties (issues #319 / #320).
 *
 *  The corpus pair selects a variant and then applies/reads the volume
 *  word of the picked record:
 *
 *    ConfiguredVariantPropertySet @ 0x465010: variant-select, then
 *    volume-set. Deviation: the decompiled wrapper only passes the
 *    slot on to the volume setter — the corpus's second argument is
 *    register-passed and invisible — so the port takes the millibel
 *    value explicitly, as issue #319's "stores the supplied variant
 *    properties" describes.
 *
 *    ConfiguredVariantPropertyRead @ 0x465030: variant-select only.
 *    The corpus reads the volume word but discards it, so the only
 *    observable effect is the draw-and-swap.
 * ------------------------------------------------------------------ */

/* Set the head variant's record volume (see above). */
#ifdef __cplusplus
extern "C"
#endif
void sound_variant_property_set(int alias_index, int millibels);

/* Rotate the variant chain, discarding the read (as shipped). */
extern "C" void sound_variant_property_read(int alias_index);

/* FNV-1a over the PCM payload (tests compare against wave-module-derived
 * reference hashes from the shipped corpus). */
uint32_t sound_pcm_hash(Sound* s);

/* ------------------------------------------------------------------ *
 *  Sound alias registry (finding 137; issues #313 / #321 / #314).
 *
 *  The corpus builds the index-to-alias-name table at startup in
 *  SoundAliasRegistryInit @ 0x00464740: allocate a 0x7bc-byte (495-slot)
 *  word table, write the GameStringHashCompute value for each of 45
 *  hard-coded "SFX_*" names to slots 0..44, and register the scene sample
 *  callback that resolves frame sample names and submits positional sound.
 *  SoundAliasLookup @ 0x464c70 hashes each query and returns the first
 *  matching slot, including hash collisions, and — only while
 *  SoundDynamicAliasRegistrationSet @ 0x00464c50 has raised DAT_00502a2c
 *  bit 2 — appends unknown names at the end of the table (dynamic
 *  aliases, DAT_00502a24 count) so `Sounds.*`-style names resolve to
 *  fresh indices without a config re-read.
 *
 *  The port keeps hashes in slot order and retains names separately for
 *  sound_alias_name(). This preserves the corpus's first-match behavior
 *  when distinct names produce the same hash.
 * ------------------------------------------------------------------ */

#define SOUND_ALIAS_BUILTINS 45  /* DAT_00502a24 base 0x2d in the lookup */
#define SOUND_ALIAS_MAX 495      /* 0x7bc-byte table, one alias per word */

/* SoundAliasRegistryInit @ 0x00464740: rebuild the built-in and retained
 * dynamic hashes in reference order and idempotently register the scene
 * sample callback. Reinitialization preserves dynamic names and indices. */
#ifdef __cplusplus
extern "C"
#endif
void sound_alias_registry_init(void);

/* SoundDynamicAliasRegistrationSet @ 0x00464c50: enable (nonzero) or
 * disable (zero) appending unknown names during lookup (DAT_00502a2c
 * bit 2). */
#ifdef __cplusplus
extern "C"
#endif
void sound_alias_registration_set(int enable);

/* SoundAliasLookup @ 0x00464c70: resolve `name` (case-insensitive) to an
 * alias index, writing it to *out_index. NULL `name` finds nothing (the
 * corpus's param_1 guard). A miss appends at the first free slot only
 * while registration is enabled — and the port returns 0 instead of the
 * corpus's out-of-bounds write once SOUND_ALIAS_MAX slots are full. */
#ifdef __cplusplus
extern "C"
#endif
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
 * seam is SoundSubsystemStopAll — it walks the tracked buffer-association
 * chain and sends both stream indices through their stop hooks. Disabling
 * also zeroes the cooldown word _DAT_00502b2c (the last
 * alias submitted under the global cooldown — written by the submission
 * queue port #296). */
#ifdef __cplusplus
extern "C"
#endif
void sound_subsystem_enable_set(int enable, int stop_active);

/* SoundSubsystemEnableSetAndStop @ 0x4652a0: enable_set with the stop
 * seam armed. */
#ifdef __cplusplus
extern "C"
#endif
void sound_subsystem_enable_set_and_stop(int enable);

/* SoundBufferStopHandle @ 0x47b130: stop the selected duplicate buffer's
 * active voice when the backend supports per-submission cancellation. */
#ifdef __cplusplus
extern "C"
#endif
void sound_buffer_stop_handle(int buffer_handle);
#ifdef __cplusplus
extern "C"
#endif
void sound_handle_stop_forward(int buffer_handle); /* 0x465190 */
struct AudioBuffer;
struct AudioBuffer* sound_handle_buffer_get(int buffer_handle); /* 0x47b0f0 */
typedef void (*SoundBufferObjectStopHook)(void* context, void* buffer_object);
/* Stream operations submit whole in-memory samples to the active player and
 * track the two logical reference stream slots. */
int sound_stream_playback_start(int stream_index, const Sound* sample);
int sound_stream_format_setup(int stream_index, const WavSample* format,
                              uint32_t buffer_bytes);
int sound_stream_drain_check(int stream_index);
void sound_buffer_object_stop_hook_set(SoundBufferObjectStopHook hook,
                                      void* context);
void sound_buffer_association_stop_all(void);
#ifdef __cplusplus
extern "C"
#endif
int sound_stream_stop_and_release(int stream_index);
#ifdef __cplusplus
extern "C"
#endif
void sound_subsystem_stop_all(void);
typedef void (*SoundPositionApplyHook)(void* context, void* buffer_object,
                                       const float position[3],
                                       uint32_t flags);
void sound_position_apply_hook_set(SoundPositionApplyHook hook,
                                   void* context);
void sound_position_set_from_vector(void* buffer_object,
                                    const float position[3]);
int sound_attached_stop_by_entity(uint32_t entity);
typedef void* (*SoundInstanceRegisterHook)(void* context, void* instance);
typedef void (*SoundInstanceStopHook)(void* context, void* provider,
                                      void* instance);
void sound_instance_hooks_set(SoundInstanceRegisterHook register_hook,
                              SoundInstanceStopHook stop_hook,
                              void* context);
void sound_instance_track_register(void* instance);
void sound_instance_track_stop_and_remove(void* provider, void* instance);
void sound_buffer_association_remove_instance(void* instance);
typedef int (*SoundSpatialRefreshHook)(void* context, void* sound_object);
/* SoundSourceSpatialRefresh uses the retail Direct3DRM/DirectSound vtables
 * in the 32-bit reference backend. This hook is only the fallback adapter
 * for non-COM providers; the SDL queue cannot reposition a submitted voice. */
void sound_spatial_refresh_hooks_set(SoundSpatialRefreshHook refresh,
                                     void* context);
/* SoundSourceSpatialRefresh @ 0x47b410; called by the production sound tick. */
void sound_source_spatial_refresh(void* sound_object);
#ifdef __cplusplus
extern "C"
#endif
void sound_source_spatial_refresh_all(void);
void sound_stream_ring_buffer_tick(int stream_index);

/* SoundSubsystemEnabledGet @ 0x465370: bit 0. */
#ifdef __cplusplus
extern "C"
#endif
int sound_subsystem_enabled(void);

/* SoundBatchModeGet @ 0x4652b0: bit 3. */
#ifdef __cplusplus
extern "C"
#endif
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
#ifdef __cplusplus
extern "C"
#endif
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
 *  recorded handle through the shared SoundHandleStopForward path and
 *  resets both words.
 * ------------------------------------------------------------------ */

/* DeferredSoundTimerSetIfIdle @ 0x464ec0: arm the cooldown to
 * `seconds` while idle (timer <= 0), forgetting any recorded handle
 * (the corpus writes -1 so the arming submit cannot be expired); 1 if
 * armed, 0 if a cooldown was already running. */
extern "C" int sound_deferred_timer_set_if_idle(float seconds);

/* DeferredSoundPlayExpire @ 0x464e80: when a handle is recorded and
 * the timer still runs, submit it to the buffer-stop hook, record -1, and zero
 * the timer. Otherwise a no-op. */
extern "C" void sound_deferred_play_expire(void);

/* The cooldown timer word (_DAT_00502b24). */
float sound_deferred_cooldown(void);

/* Port seam recording the submitted-handle word DAT_00502b28 exactly
 * as the submission route's writer (SoundSubmitWithGlobalCooldown's
 * body, whose alias-play and duration callees are unported) would:
 * pass -1 to forget it. Expire only stops sounds this recorded. */
void sound_deferred_handle_record(int handle);

/* The recorded handle word (DAT_00502b28). */
int sound_deferred_handle(void);

/* ------------------------------------------------------------------ *
 *  Configured sound-variant chains (issues #318 / #324; findings
 *  107/134/135/142).
 *
 *  A `Samples` config value — e.g. "*Sounds\drtdrillc,#-400#B,@stream" —
 *  is split on commas and loaded path-by-path into bank records by
 *  SoundSampleMarkerParse @ 0x464d00, linking every success into the
 *  alias's variant chain: the head record holds the current pick and
 *  the node list holds the alternates. Markers per finding 134/135/142:
 *  a leading '*' raises the record's duplicate-buffer flag (stored in
 *  the loop bit), '#N#' (signed decimal) sets the per-sample volume,
 *  and '@' switches every later path in the value to streamed mode.
 *
 *  SoundSampleVariantSelect @ 0x464e20 then re-draws the chain: it
 *  picks a chain member with random_value % variant_count and swaps it
 *  to the head, returning the new head slot.
 *
 *  Port deviations: the corpus writes the chain pool (DAT_005023e0,
 *  counter DAT_00502a28) with no bound — the port caps it and drops
 *  further links when full (the corpus would scribble over adjacent
 *  data); '#...#' buffers stay unterminated in the corpus (finding
 *  145) — the port terminates; and the corpus's faithful quirk that
 *  the second load overwrites the head slot (the first value lands in
 *  the head only until overwritten) is reproduced deliberately.
 * ------------------------------------------------------------------ */

#define SOUND_VARIANT_TOKENS 100   /* corpus token buffer, local_190[100] */
#define SOUND_VARIANT_POOL_NODES 16384 /* port bound, see above */

/* SoundSampleMarkerParse @ 0x464d00: parse one Samples value into the
 * alias's variant chain. 1 if every path loaded, 0 if any failed (the
 * chain still keeps the successes, like the corpus). */
#ifdef __cplusplus
extern "C"
#endif
int sound_sample_marker_parse(const char* spec, int alias_index);
/* Load the Samples section after the audio bank has been enabled.
 * allow_marked controls the reference's leading-`!` reduce-samples gate. */
#ifdef __cplusplus
extern "C"
#endif
void sound_samples_config_load(BlockFile* config, int allow_marked);

/* SoundSampleVariantSelect @ 0x464e20: rotate the chain with the
 * corpus's 15-bit draw and return the head's bank slot (-1 for the
 * null alias 0, or a head that holds none). */
#ifdef __cplusplus
extern "C"
#endif
int sound_variant_select(int alias_index);

/* Chain inspection for callers/tests: write the chain's bank slots in
 * head order into `out_slots` (up to max) and return the count. */
int sound_variant_chain(int alias_index, int* out_slots, int max);


/* ------------------------------------------------------------------ *
 *  Sound submission route (issues #325 / #328 / #329 / #330 / #332;
 *  findings 137–149).
 *
 *  With batch mode off, a cue plays through the sound subsystem: the
 *  alias's variant chain is drawn (the port's select) and the picked
 *  record goes to the instance-layer play seam. With batch mode on
 *  (bit 3, batch-mode-get @ 0x4652b0), submissions defer into the
 *  corpus's batch-deferred table — 10 records of six words: alias,
 *  position word, position-vector words, and a mode word (bit 0 marks
 *  the positional form; bit 1 the loop; bit 2 the position-selector
 *  passed back at replay). Submissions past the tenth are dropped.
 *  SoundBatchQueueDrain switches the mode; with replay it re-runs the
 *  deferred entries through the route (batch mode off first) and clears
 *  the table.
 *
 *  The frame tick owns the deferred-request queue: it steps the global
 *  cooldown timer, plays last frame's rotated queue through the
 *  alias-play route, restores batch mode if it was armed, rotates the
 *  pending queue (#317) into the active replay copies, and pumps the
 *  instance chain.
 *
 *  Deviation: the play/pump calls land at instance-layer seams that
 *  wait on TODO(corpus); the observable state (chains, queues, records,
 *  PRNG draws) matches the corpus.
 * ------------------------------------------------------------------ */

#define SOUND_BATCH_QUEUE_CAPACITY 10   /* DAT_00502b20 bound */

/* SoundAliasPlay @ 0x464fa0: play `alias` (looping when `loop`) —
 * immediately when not batching (variant draw + play seam when the
 * chain's head holds a record), or deferred to the batch table. Returns
 * the encoded bank/buffer handle on successful immediate playback, else -1. */
#ifdef __cplusplus
extern "C"
#endif
int sound_alias_play(int alias, int loop);

/* PositionalSoundSubmit @ 0x465090: like sound_alias_play but carries a
 * position: `positional` selects the position-word form (`position`) or
 * the coordinate-vector form (`xyz`, three floats); the deferred form
 * keeps both. Immediate playback returns its encoded bank/buffer handle;
 * deferred or unsuccessful playback returns 0. */
#ifdef __cplusplus
extern "C"
#endif
int sound_positional_sound_submit(int position, int alias, int loop,
                                  int positional, const float* xyz);
/* SoundSubmissionRouteWrapper @ 0x465050: unwraps the optional position
 * handle before forwarding the request to PositionalSoundSubmit. */
#ifdef __cplusplus
extern "C"
#endif
int sound_submission_route_wrapper(const void* wrapped_position, int alias,
                                   int loop, int positional,
                                   const float* xyz);

/* SoundBatchQueueDrain @ 0x4652d0: with `batch_set` raise batch mode and
 * return (the corpus stops there); otherwise clear batch mode and, when
 * `replay`, re-run every deferred entry through the route before
 * clearing the queue count. */
#ifdef __cplusplus
extern "C"
#endif
void sound_batch_queue_drain(int batch_set, int replay);

/* SoundDeferredQueueDrain @ 0x4652c0: batch_queue_drain(batch_set, 1). */
extern "C" void sound_deferred_queue_drain(int batch_set);

/* Inspection of the batch-deferred table (mode word bits: bit 0
 * positional, bit 1 loop, bit 2 position selector). `xyz`, when given,
 * takes the record's three coordinate words (stale where the alias
 * form never wrote them). Out-of-range reads report 0. */
int sound_batch_queue_count(void);
int sound_batch_queue_get(int index, int* alias, int* position,
                          uint32_t* mode, float* xyz);

/* SoundSubsystemFrameTick @ 0x4651a0. */
#ifdef __cplusplus
extern "C"
#endif
void sound_subsystem_frame_tick(float dt);

/* The rotated queue the frame tick replays each frame (the corpus's
 * active copies DAT_00502b80/DAT_00502ba8/DAT_00502bd4). */
int sound_active_request_count(void);
int sound_active_request_get(int index, int* alias, int* loop);


/* ------------------------------------------------------------------ *
 *  Streamed-voice duration + stream triggers (issues #297 / #299;
 *  findings 406 / 217).
 * ------------------------------------------------------------------ */

/* StreamedWaveBufferStepRate @ 0x47b8e0: the bank record slot's file
 * size over the format's average bytes per second. No zero guard as
 * shipped (finding 217): unloaded slots read as 0/0 -> NaN, zero-rate
 * formats -> infinity. Slot -1 (the select's no-sound result) reads 0.
 * Deviation: the corpus keeps stream records in a side table with a
 * cached-vs-dereferenced rate word selected by the stream flag; the
 * port reads its bank record (the same value for both branches). */
#ifdef __cplusplus
extern "C"
#endif
float sound_streamed_wave_buffer_step_rate(int slot);

/* SoundObjectDurationGet @ 0x465160: duration of whatever `alias`'s
 * chain selects — consumes one variant draw per call. Alias 0
 * (nothing selects) reads 0. */
#ifdef __cplusplus
extern "C"
#endif
double sound_object_duration_get(int alias);

/* SoundSubmitWithGlobalCooldown @ 0x464ef0: with the cooldown timer
 * down to <= 0, plays `alias` through the submission route and records
 * the last-alias word, the handle word, and a timer of
 * duration * 25 (corpus units). Returns 1 (and writes *out_handle when
 * given) on submission, 0 when the timer is still running or nothing
 * routed. Deviation: the corpus handle is an instance-layer handle;
 * the port records the routed bank slot (see the .cpp note). */
extern "C" int sound_submit_with_global_cooldown(int alias, int* out_handle);

/* StreamPlayLevel @ 0x416150 / StreamPlayTutorial @ 0x4161a0: format
 * the `Stream_LevelName_Level%d` / `Stream_LevelName_TLevel%d`
 * stream name, resolve it through the alias lookup, and submit it
 * through the global cooldown when it resolves. Returns the submit
 * result (0 when the name resolves nowhere). */
int sound_stream_play_level(int level);
int sound_stream_play_tutorial(int level);

/* SoundBufferAssociationAdd (FUN_0047b020): allocates a four-word record
 * — the association's value words in order, the prior head last — and
 * makes it the chain head. Deviation: the corpus's dword record is laid
 * out host-widened (word n at record + n*sizeof(void*)); the chain head
 * (DAT_00506684) has no ported owner, so it lands here as a zero-init
 * static with hooks (NULL record guarded — corpus strides blindly). */
void sound_buffer_association_add(uint32_t a, uint32_t b, uint32_t c);
                                                /* FUN_0047b020 */
void  sound_buffer_association_head_set(void* node); /* port hook */
void* sound_buffer_association_head_peek(void);      /* port hook */

/* SoundMinimumDistanceUpdateMode (FUN_004295f0) applies this raw float
 * bit pattern to every tracked buffer through its min-distance method. */
#ifdef __cplusplus
extern "C"
#endif
void sound_tracked_buffers_set_min_distance(float value);

#endif /* LEGORRR_SOUND_HPP */
