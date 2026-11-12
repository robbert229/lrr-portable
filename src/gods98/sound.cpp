// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "resource.hpp"
#include "heap.hpp"
#include "sound.hpp"
#include "audio.hpp"
#include "string.hpp"
#include "record.hpp"
#include "view.hpp"
#include "block.hpp"
#include "game/util.hpp"
#include "sound_shared.hpp"
#include "sound_for_sound3d.hpp"
#include "sound_from_sound3d.hpp"

void sound_wave_data_reader_init(SoundWaveDataReader* reader,
                                 const uint8_t* data, uint32_t size)
{
    if (reader == NULL) return;
    reader->data = data;
    reader->data_size = size;
    reader->cursor = 0;
    reader->remaining = size;
}

BankSlot s_bank[SOUND_BANK_SLOTS];

/* ------------------------------------------------------------------ *
 *  RIFF/WAVE parse (finding 14 §2).
 * ------------------------------------------------------------------ */

static uint32_t s_le32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t s_le16(const uint8_t* p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

uint32_t sound_wave_data_reader_read(SoundWaveDataReader* reader,
                                     uint32_t requested, void* destination,
                                     uint32_t capacity, uint32_t* bytes_read)
{
    uint32_t amount;
    if (bytes_read != NULL) *bytes_read = 0;
    if (reader == NULL || (destination == NULL && requested != 0u))
        return 0xe103u;
    amount = requested;
    if (amount > reader->remaining) amount = reader->remaining;
    if (amount > capacity) amount = capacity;
    if (reader->cursor > reader->data_size ||
        amount > reader->data_size - reader->cursor) return 0xe103u;
    if (reader->data == NULL && amount != 0u) return 0xe103u;
    if (amount != 0u) {
        memcpy(destination, reader->data + reader->cursor, amount);
        reader->cursor += amount;
        reader->remaining -= amount;
    }
    if (bytes_read != NULL) *bytes_read = amount;
    return 0u;
}

Sound* sound_parse(const uint8_t* data, uint32_t size)
{
    if (size < 12 + 8 || data[0] != 'R' || data[1] != 'I'
        || data[2] != 'F' || data[3] != 'F'
        || data[8] != 'W' || data[9] != 'A' || data[10] != 'V'
        || data[11] != 'E') {
        return NULL;
    }

    int      have_fmt = 0, have_data = 0;
    WavSample fmt;
    const uint8_t* pcm = NULL;
    uint32_t pcm_len = 0;

    uint32_t pos = 12;
    while (pos + 8 <= size) {
        uint32_t csz = s_le32(data + pos + 4);
        if (csz > (uint64_t)size - pos - 8) {
            break; /* truncated chunk */
        }
        const uint8_t* body = data + pos + 8;
        if (data[pos] == 'f' && data[pos + 1] == 'm' && data[pos + 2] == 't'
            && data[pos + 3] == ' ') {
            if (csz >= 16) {
                fmt.format_tag     = s_le16(body + 0);
                fmt.channels       = s_le16(body + 2);
                fmt.sample_rate    = s_le32(body + 4);
                fmt.byte_rate      = s_le32(body + 8);
                fmt.block_align    = s_le16(body + 12);
                fmt.bits_per_sample = s_le16(body + 14);
                have_fmt = 1;
            }
        } else if (data[pos] == 'd' && data[pos + 1] == 'a'
                   && data[pos + 2] == 't' && data[pos + 3] == 'a') {
            pcm     = body;
            pcm_len = csz;
            have_data = 1;
        }
        /* every chunk, incl. odd sizes, padded to even */
        pos += 8 + csz + (csz & 1);
    }

    if (!have_fmt || !have_data) {
        return NULL;
    }
    /* The corpus is all PCM; others (extensible, float, ADPCM…) are rejected
     * rather than silently misdecoded. */
    if (fmt.format_tag != 1) {
        return NULL;
    }

    Sound* s = (Sound*)calloc(1, sizeof(Sound));
    if (s == NULL) {
        return NULL;
    }
    s->format = fmt;
    s->data_len = pcm_len;
    s->file_len = size;   /* the corpus stores the streamed file size */
    s->data = (uint8_t*)malloc(pcm_len > 0 ? pcm_len : 1u);
    if (s->data == NULL) {
        free(s);
        return NULL;
    }
    if (pcm_len > 0) {
        memcpy(s->data, pcm, pcm_len);
    }
    return s;
}

void sound_free(Sound* s)
{
    if (s == NULL) {
        return;
    }
    free(s->data);
    free(s->name);
    free(s);
}

static Sound* sound_sample_file_load_by_name(const char* name)
{
    Resource* r;
    Sound* s;
    if (name == NULL) return NULL;
    s = sound_wave_file_load(name);
    if (s == NULL) {
        r = resource_open(name, RESOURCE_AUTO);
        if (r == NULL) return NULL;
        if (resource_read(r) < 0) {
            resource_free(r);
            return NULL;
        }
        s = sound_parse((const uint8_t*)r->data, (uint32_t)r->size);
        resource_free(r);
    }
    if (s != NULL) {
#if defined(_MSC_VER)
        s->name = _strdup(name);
#else
        size_t n = strlen(name) + 1;
        s->name = (char*)malloc(n);
        if (s->name == NULL) {
            sound_free(s);
            return NULL;
        }
        memcpy(s->name, name, n);
#endif
        if (s->name == NULL) {
            sound_free(s);
            return NULL;
        }
    }
    return s;
}

/* Public resource-layer adapter used by the bank loader and other callers. */
Sound* sound_load(const char* name)
{
    return sound_sample_file_load_by_name(name);
}

/* Portable WAVE sample-load adapter. Retail 0x0047b580 writes the parsed
 * format/data fields into a 0x134-byte bank slot record; this API fills the
 * separate Sound value used by the portable bank sidecar. */
int sound_sample_load_into(Sound* output, const char* name, int mode_flags)
{
    Sound* loaded;
    if (output == NULL || name == NULL) return 0;
    memset(output, 0, sizeof(*output));
    loaded = sound_load(name);
    if (loaded == NULL) return 0;
    *output = *loaded;
    output->flags &= ~(SOUND_LOOPING | SOUND_STREAMED);
    if ((mode_flags & 1) != 0) output->flags |= SOUND_LOOPING;
    if ((mode_flags & 2) != 0) output->flags |= SOUND_STREAMED;
    free(loaded);
    return 1;
}

void sound_sample_clear(Sound* sound)
{
    if (sound == NULL) return;
    free(sound->data);
    free(sound->name);
    memset(sound, 0, sizeof(*sound));
}

/* ------------------------------------------------------------------ *
 *  Sound bank (finding 14 §1: 600-slot table, flag bits 2/3).
 * ------------------------------------------------------------------ */

int s_has_wav_suffix(const char* name)
{
    size_t n = strlen(name);
    return n > 4 && strcasecmp_lite(name + n - 4, ".wav") == 0;
}

int sound_bank_load(const char* name, int streamed, int looping)
{
    /* Port convenience over the corpus entry load: skip when already
     * present, like the shipped loader's own dedupe callers. */
    if (sound_bank_find(name) != NULL) {
        return 0;
    }
    return sound_bank_entry_load(name, streamed, looping, 0) >= 0 ? 0 : -1;
}

Sound* sound_bank_find(const char* fullname)
{
    if (fullname == NULL) {
        return NULL;
    }
    char full[1024];
    if (!s_has_wav_suffix(fullname)) {
        size_t n = strlen(fullname);
        if (n + 5 >= sizeof(full)) {
            return NULL;
        }
        snprintf(full, sizeof(full), "%s.wav", fullname);
        fullname = full;
    }
    for (int i = 0; i < SOUND_BANK_SLOTS; ++i) {
        if ((s_bank[i].flags & SOUND_SLOT_BUSY) != 0
            && strcasecmp_lite(s_bank[i].name, fullname) == 0) {
            return s_bank[i].sound;
        }
    }
    return NULL;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
int sound_bank_count(void)
{
    int count = 0;
    for (int i = 0; i < SOUND_BANK_SLOTS; ++i) {
        if ((s_bank[i].flags & SOUND_SLOT_BUSY) != 0) {
            ++count;
        }
    }
    return count;
}

void sound_bank_clear(void)
{
    for (int i = 0; i < SOUND_BANK_SLOTS; ++i) {
        AudioPlayer* player = audio_system_player_get();
        int stream_index;
        if (player != NULL && s_bank[i].sound != NULL) {
            if (player->loop_sound == s_bank[i].sound)
                audio_stop(player);
            for (stream_index = 0; stream_index < 2; ++stream_index) {
                if (player->stream_sample[stream_index] == s_bank[i].sound)
                    audio_stream_stop(player, stream_index);
            }
        }
        audio_buffer_destroy(s_bank[i].buffers[0]);
        audio_buffer_destroy(s_bank[i].buffers[1]);
        audio_buffer_destroy(s_bank[i].buffers[2]);
        s_bank[i].buffers[0] = NULL;
        s_bank[i].buffers[1] = NULL;
        s_bank[i].buffers[2] = NULL;
        if (s_bank[i].sound != NULL) {
            sound_free(s_bank[i].sound);
        }
        s_bank[i].sound = NULL;
        s_bank[i].flags = 0;
        s_bank[i].volume = 0;
        s_bank[i].name[0] = '\0';
    }
}

void sound_bank_sound_on_set(int on)
{
    s_bank_sound_on = (on != 0);
}
#endif /* !LEGORRR_BACKEND_WINAPI */

int sound_bank_sound_on(void)
{
    return s_bank_sound_on;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
uint32_t sound_bank_entry_flags(int slot)
{
    if (slot < 0 || slot >= SOUND_BANK_SLOTS) {
        return 0;
    }
    return s_bank[slot].flags;
}

Sound* sound_bank_entry_sample_get(int slot)
{
    if (slot < 0 || slot >= SOUND_BANK_SLOTS
        || (s_bank[slot].flags & SOUND_SLOT_BUSY) == 0u) return NULL;
    return s_bank[slot].sound;
}

uint32_t sound_pcm_hash(Sound* s)
{
    uint32_t h = 2166136261u;
    if (s == NULL || s->data == NULL) {
        return h;
    }
    for (uint32_t i = 0; i < s->data_len; ++i) {
        h ^= s->data[i];
        h *= 16777619u;
    }
    return h;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* ------------------------------------------------------------------ *
 *  Sound alias registry (finding 137; issues #313 / #321 / #314).
 *  See sound.hpp for the corpus model and documented deviations.
 * ------------------------------------------------------------------ */

/* SoundAliasRegistryInit @ 0x00464740: the 45 hard-coded SFX_* names in
 * slot order (literal strings s_SFX_NULL..s_SFX_AmbientMusicLoop at
 * 0x4a9794..0x4a94d0; indices 7, 0x26, 0x29 and 0x2c are pinned by
 * findings 137/144). */
const char* const s_alias_builtins[SOUND_ALIAS_BUILTINS] = {
    "SFX_NULL",                  /* 0  */
    "SFX_Stamp",                 /* 1  */
    "SFX_Drill",                 /* 2  */
    "SFX_DrillFade",             /* 3  */
    "SFX_RockBreak",             /* 4  */
    "SFX_Drip",                  /* 5  */
    "SFX_Ambient",               /* 6  */
    "SFX_AmbientLoop",           /* 7  */
    "SFX_Step",                  /* 8  */
    "SFX_RockMonster",           /* 9  */
    "SFX_RockMonster2",          /* 10 */
    "SFX_RockMonsterStep",       /* 11 */
    "SFX_MFDeposit",             /* 12 */
    "SFX_ButtonPressed",         /* 13 */
    "SFX_MFLift",                /* 14 */
    "SFX_MFThrow",               /* 15 */
    "SFX_Walker",                /* 16 */
    "SFX_YesSir",                /* 17 */
    "SFX_Build",                 /* 18 */
    "SFX_Okay",                  /* 19 */
    "SFX_NotOkay",               /* 20 */
    "SFX_InterfaceSlideOnScreen",  /* 21 */
    "SFX_InterfaceSlideOffScreen", /* 22 */
    "SFX_PanelSlideOnScreen",    /* 23 */
    "SFX_PanelSlideOffScreen",   /* 24 */
    "SFX_Siren",                 /* 25 */
    "SFX_CrystalRecharge",       /* 26 */
    "SFX_Laser",                 /* 27 */
    "SFX_LaserHit",              /* 28 */
    "SFX_LazerRecharge",         /* 29 */
    "SFX_TopPriority",           /* 30 */
    "SFX_ImmovableRock",         /* 31 */
    "SFX_Wall",                  /* 32 */
    "SFX_Floor",                 /* 33 */
    "SFX_BoulderHit",            /* 34 */
    "SFX_Place",                 /* 35 */
    "SFX_PlaceOre",              /* 36 */
    "SFX_PlaceCrystal",          /* 37 */
    "SFX_Lava",                  /* 38 (0x26) */
    "SFX_RockWipe",              /* 39 */
    "SFX_FallIn",                /* 40 */
    "SFX_MusicLoop",             /* 41 (0x29) */
    "SFX_CaptainSlide",          /* 42 */
    "SFX_Dynamite",              /* 43 */
    "SFX_AmbientMusicLoop",      /* 44 (0x2c) */
};

/* Dynamic (config-registered) aliases appended by the lookup, mirroring
 * the corpus DAT_00502a24 slots behind the 45 built-ins. */
char* s_alias_dynamic[SOUND_ALIAS_MAX - SOUND_ALIAS_BUILTINS];

static SoundBufferObjectStopHook s_buffer_object_stop_hook;
static void* s_buffer_object_stop_context;
SoundPositionApplyHook s_sound_position_hook;
void* s_sound_position_context;
SoundInstanceRegisterHook s_instance_register_hook;
SoundInstanceStopHook s_instance_stop_hook;
void* s_instance_hook_context;
void sound_buffer_association_remove_instance(void* instance);
SoundSpatialRefreshHook s_spatial_refresh_hook;
void* s_spatial_refresh_context;

int sound_bank_entry_play(int slot, int loop)
{
    return sound_bank_entry_play_mode(slot, loop, 0);
}

int sound_bank_entry_play_mode(int slot, int loop, int mode)
{
    AudioPlayer* player;
    int index = 0;
    int count;
    int frames;
    if (slot < 0 || slot >= SOUND_BANK_SLOTS
        || (s_bank[slot].flags & SOUND_SLOT_BUSY) == 0u) return -1;
    if ((s_bank[slot].flags & SOUND_STREAMED) != 0u) {
        Sound* sample = s_bank[slot].sound;
        if (loop < 0 || loop > 1 || sample == NULL) return -1;
        player = audio_system_player_get();
        if (player == NULL) return -1;
        /* Streamed aliases use the selector as one of the two stream slots.
         * The original stops/releases that slot before starting its
         * replacement; the portable backend retains the same slot lifecycle
         * while submitting the in-memory sample through its shared queue. */
        audio_stream_stop(player, loop);
        if (!audio_stream_start(player, loop, sample)
            || sample->format.block_align == 0) return -1;
        return (int)(sample->data_len / sample->format.block_align);
    }
    count = (s_bank[slot].flags & SOUND_LOOPING) != 0u ? 3 : 1;
    if (count == 3) {
        index = s_bank[slot].next_buffer;
        s_bank[slot].next_buffer = (index + 1) % count;
    }
    /* Global dispatcher mode 2 subtracts 0x320 millibels from the
     * stored buffer volume before playback; the streamed branch bypasses
     * that buffer-volume adjustment. */
    if (loop != 0) {
        Sound* sample = s_bank[slot].sound;
        frames = audio_buffer_loop_attenuated(s_bank[slot].buffers[index],
                sample, mode == 2 ? -800 : 0) == 0
            && sample->format.block_align != 0
            ? (int)(sample->data_len / sample->format.block_align) : -1;
    } else {
        frames = audio_buffer_play_attenuated(s_bank[slot].buffers[index],
                                               mode == 2 ? -800 : 0);
    }
    return frames;
}

void sound_buffer_object_stop_hook_set(SoundBufferObjectStopHook hook,
                                      void* context)
{
    s_buffer_object_stop_hook = hook;
    s_buffer_object_stop_context = context;
}

/* Portable playback adapter. Retail 0x0047b970 receives a wave/MMIO owner,
 * stream index, and mode; this helper receives a decoded Sound and index. */
int sound_stream_playback_start(int stream_index, const Sound* sample)
{
    if (stream_index < 0 || stream_index > 1 || sample == NULL) return 0;
    return audio_stream_start(audio_system_player_get(), stream_index, sample);
}

/* Portable stream setup adapter. Retail 0x0047baa0 parses the WAVE format
 * directly from a stream handle and owns the ring-buffer setup. */
int sound_stream_format_setup(int stream_index, const WavSample* format,
                              uint32_t buffer_bytes)
{
    if (stream_index < 0 || stream_index > 1 || format == NULL) return 0;
    return audio_stream_setup(audio_system_player_get(), stream_index, format,
                              buffer_bytes);
}

void sound_position_apply_hook_set(SoundPositionApplyHook hook,
                                   void* context)
{
    s_sound_position_hook = hook;
    s_sound_position_context = context;
}

void sound_instance_hooks_set(SoundInstanceRegisterHook register_hook,
                              SoundInstanceStopHook stop_hook,
                              void* context)
{
    s_instance_register_hook = register_hook;
    s_instance_stop_hook = stop_hook;
    s_instance_hook_context = context;
}

void sound_spatial_refresh_hooks_set(SoundSpatialRefreshHook refresh,
                                     void* context)
{
    s_spatial_refresh_hook = refresh;
    s_spatial_refresh_context = context;
}

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
int sound_alias_count(void)
{
    return SOUND_ALIAS_BUILTINS + s_alias_dynamic_count;
}

const char* sound_alias_name(int index)
{
    if (index < 0) {
        return NULL;
    }
    if (index < SOUND_ALIAS_BUILTINS) {
        return s_alias_builtins[index];
    }
    if (index < SOUND_ALIAS_BUILTINS + s_alias_dynamic_count) {
        return s_alias_dynamic[index - SOUND_ALIAS_BUILTINS];
    }
    return NULL;
}

/* ------------------------------------------------------------------ *
 *  Sound subsystem enable state (issues #315 / #322 / #323 / #316;
 *  finding 137 flag word DAT_00502a2c).
 * ------------------------------------------------------------------ */

double sound_deferred_last_alias(void)
{
    return s_cooldown_alias;
}

/* ------------------------------------------------------------------ *
 *  Deferred sound request queue (issue #317). See sound.hpp.
 * ------------------------------------------------------------------ */

int sound_request_queue_count(void)
{
    return s_sound_queue_count;
}

int sound_request_queue_get(int index, int* alias, int* loop)
{
    if (index < 0 || index >= s_sound_queue_count) {
        return 0;
    }
    if (alias != NULL) {
        *alias = s_queue_alias[index];
    }
    if (loop != NULL) {
        *loop = s_queue_loop[index];
    }
    return 1;
}

void sound_request_queue_clear(void)
{
    /* Port-side helper: the corpus zeroes DAT_00502bd0 only in its
     * frame tick (that consumer is unported here). */
    s_sound_queue_count = 0;
}

/* ------------------------------------------------------------------ *
 *  Deferred-play global cooldown (issues #326 / #327). See sound.hpp.
 * ------------------------------------------------------------------ */

float sound_deferred_cooldown(void)
{
    return s_cooldown_timer;
}
void sound_deferred_handle_record(int handle)
{
    s_cooldown_handle = handle;
}

int sound_deferred_handle(void)
{
    return s_cooldown_handle;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* ------------------------------------------------------------------ *
 *  Configured sound-variant chains (issues #318 / #324). See sound.hpp.
 * ------------------------------------------------------------------ */

int s_variant_slot[SOUND_ALIAS_MAX]; /* DAT_00501468 head slot words */
int s_variant_next[SOUND_ALIAS_MAX]; /* DAT_0050146c head next words */

VariantNode s_variant_pool[SOUND_VARIANT_POOL_NODES];

#if !defined(LEGORRR_BACKEND_WINAPI) /* native backend and tests only (#576) */
int sound_variant_chain(int alias_index, int* out_slots, int max)
{
    if (alias_index < 0 || alias_index >= SOUND_ALIAS_MAX || max <= 0) {
        return 0;
    }
    if (out_slots != NULL) {
        out_slots[0] = s_variant_slot[alias_index];
    }
    int n = 1;
    for (int node = s_variant_next[alias_index];
         node != 0 && n < max; node = s_variant_pool[node].next) {
        if (out_slots != NULL) {
            out_slots[n] = s_variant_pool[node].slot;
        }
        ++n;
    }
    return n;
}

/* ------------------------------------------------------------------ *
 *  Bank-record volume write + configured variant properties (issues
 *  #319 / #320). See sound.hpp.
 * ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ *
 *  Sound submission route (issues #325 / #328 / #329 / #330 / #332).
 *  See sound.hpp.
 * ------------------------------------------------------------------ */

int sound_batch_queue_count(void)
{
    return s_batch_count;
}

int sound_batch_queue_get(int index, int* alias, int* position,
                          uint32_t* mode, float* xyz)
{
    if (index < 0 || index >= s_batch_count) {
        return 0;
    }
    if (alias != NULL) {
        *alias = s_batch_entry[index].alias;
    }
    if (position != NULL) {
        *position = s_batch_entry[index].position;
    }
    if (mode != NULL) {
        *mode = s_batch_entry[index].mode;
    }
    if (xyz != NULL) {
        xyz[0] = s_batch_entry[index].xyz[0];
        xyz[1] = s_batch_entry[index].xyz[1];
        xyz[2] = s_batch_entry[index].xyz[2];
    }
    return 1;
}

int sound_active_request_count(void)
{
    return s_active_count;
}

int sound_active_request_get(int index, int* alias, int* loop)
{
    if (index < 0 || index >= s_active_count) {
        return 0;
    }
    if (alias != NULL) {
        *alias = s_active_alias[index];
    }
    if (loop != NULL) {
        *loop = s_active_loop[index];
    }
    return 1;
}
#endif /* !LEGORRR_BACKEND_WINAPI */

/* ------------------------------------------------------------------ *
 *  Streamed-voice duration + configured stream triggers (issues
 *  #297 / #299; findings 406 / 217).
 *
 *  A stream's duration is its stored file size divided by the format's
 *  average bytes per second. The corpus keeps that in a side record
 *  table and picks between a cached copy and a dereferenced format
 *  word by the record's stream flag — both resolve to the same value
 *  in the port, whose bank records carry the loaded WAV. The
 *  division has no zero guard as shipped: unloaded or zero-rate
 *  records yield infinity/NaN, and a NaN cooldown would then never
 *  expire (finding 217), kept as-is.
 * ------------------------------------------------------------------ */

#if defined(LEGORRR_BACKEND_MINIWIN)
/* The native port's model of the unit's association list (sound3d.c);
 * the DirectSound builds keep the list in sound3d.c's globals. */
/* KeyedResourceEntryRemove (FUN_0047b2a0) removes the first global
 * association whose third word is the stopped instance. The provider is
 * passed through the original recursion but does not participate in its key
 * comparison. */
void sound_buffer_association_remove_instance(void* instance)
{
    void* previous = NULL;
    void* node = s_buffer_assoc_head;
    while (node != NULL) {
        char* words = (char*)node;
        void* key = NULL;
        void* next = NULL;
        memcpy(&key, words + 2 * sizeof(void*), sizeof(key));
        memcpy(&next, words + 3 * sizeof(void*), sizeof(next));
        if (key == instance) {
            if (previous == NULL) s_buffer_assoc_head = next;
            else memcpy((char*)previous + 3 * sizeof(void*), &next,
                        sizeof(next));
            engine_heap_free(node);
            return;
        }
        previous = node;
        node = next;
    }
}

void sound_buffer_association_stop_all(void)
{
    void* node = s_buffer_assoc_head;
    while (node != NULL) {
        char* words = (char*)node;
        void* buffer_object = NULL;
        void* next = NULL;
        memcpy(&buffer_object, words + sizeof(void*), sizeof(buffer_object));
        memcpy(&next, words + 3 * sizeof(void*), sizeof(next));
        if (buffer_object != NULL && s_buffer_object_stop_hook != NULL)
            s_buffer_object_stop_hook(s_buffer_object_stop_context,
                                      buffer_object);
        node = next;
    }
}

void  sound_buffer_association_head_set(void* node)
{
    s_buffer_assoc_head = node;
}
void* sound_buffer_association_head_peek(void) { return s_buffer_assoc_head; }
#endif /* LEGORRR_BACKEND_MINIWIN */

namespace unported_ledger_stubs {

void ledger_stub_00434a50_SoundSamplesConfigLoad(BlockFile* config,
                                                  int allow_marked)
{
    sound_samples_config_load(config, allow_marked);
}

} // namespace unported_ledger_stubs

