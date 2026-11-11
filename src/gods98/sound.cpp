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

/* The reference scene interfaces are 32-bit COM-style objects. This getter
 * reads the record that DDiSceneAnimationHookCreate owns at vtable +0x1c. */
static void* sound_instance_property_get(void* object)
{
#if defined(_MSC_VER) && defined(_M_IX86)
    void* result;
    __asm {
        mov ecx, object
        mov eax, [ecx]
        call dword ptr [eax + 1Ch]
        mov result, eax
    }
    return result;
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
    typedef void* (__attribute__((thiscall)) *GetProperty)(void*);
    void** vtable;
    GetProperty get_property;
    if (object == NULL) return NULL;
    vtable = *(void***)object;
    if (vtable == NULL || vtable[0x1c / sizeof(void*)] == NULL) return NULL;
    get_property = (GetProperty)vtable[0x1c / sizeof(void*)];
    return get_property(object);
#else
    typedef void* (*GetProperty)(void*);
    void** vtable;
    GetProperty get_property;
    if (object == NULL) return NULL;
    vtable = *(void***)object;
    if (vtable == NULL || vtable[0x1c / sizeof(void*)] == NULL) return NULL;
    get_property = (GetProperty)vtable[0x1c / sizeof(void*)];
    return get_property(object);
#endif
}

void sound_wave_data_reader_init(SoundWaveDataReader* reader,
                                 const uint8_t* data, uint32_t size)
{
    if (reader == NULL) return;
    reader->data = data;
    reader->data_size = size;
    reader->cursor = 0;
    reader->remaining = size;
}

/* Bank record mirrors the corpus stride and the offsets used by its sound
 * routines: name at +0, volume at +0x110, three buffers at +0x120, the
 * rotating buffer index at +0x12c, and flags at +0x130. The reserved words
 * retain the original spacing around the port-side sample and buffer data. */
typedef struct BankSlot {
    char     name[260];      /* stored "<base>.wav" resource name */
    Sound*   sound;          /* loaded payload, NULL until loaded */
    uint8_t  reserved_108[8];
    int      volume;         /* +0x110: validated millibels */
    uint8_t  reserved_114[12];
    AudioBuffer* buffers[3]; /* portable duplicate-buffer state */
    int      next_buffer;    /* +0x12c */
    uint32_t flags;          /* SOUND_SLOT_* + SOUND_LOOPING/STREAMED */
} BankSlot;

static BankSlot s_bank[SOUND_BANK_SLOTS];
/* Corpus-faithful default: the original's DAT_00533878 image bytes are
 * zero-fill (past the reference .data file span).  The corpus clears it
 * on sound-system init entry (0x47a649), raises it to 1 only when
 * DirectSound init succeeds (0x47a766), and clears it at teardown
 * (0x47a842); the frameloop's audio-open success branch carries that
 * 0x47a766 write.  Plain zero-init (.bss) keeps both gates green: the
 * // GLOBAL: annotation resolves (same shape as the 0x4aa450/0x4aa458
 * pair above) and datacmp sees (uninitialized) on both sides, like the
 * corpus-zero s_listener_view.  A file-backed 0 instead (explicit
 * .data segment) keeps datacmp flagging the pair anyway and perturbs
 * reccmp's neighbouring-global verification — measured, so avoided. */
// GLOBAL: LEGORR 0x00533878
int             s_bank_sound_on = 0; /* DAT_00533878 seam, see sound.hpp */

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

/* SoundSampleFileOpenByName (FUN_0047fb60) is a presence query: the retail
 * code opens and closes loose files, but returns the matching WAD entry
 * index for an archive-backed sample. Keep the decoded Sound loader below
 * this probe so the portable audio API can still consume sample data. */
// FUNCTION: LEGORR 0x0047fb60
int sound_sample_file_open_by_name(const char* name)
{
    const char* resolved;
    char* wad_name;
    int route;
    if (name == NULL || name[0] == '\0') return 0;
    resolved = resource_path_resolve_against_base(name);
    if (resolved == NULL) return 0;
    route = game_file_open_route_select(resolved, "r");
    if (route == 1) {
        GameFileHandle* handle = game_file_open_dispatch(name, "r");
        if (handle == NULL) return 0;
        game_file_close(handle);
        return 1;
    }
    if (route == 0) {
        wad_name = game_file_mounted_path_prefix_rewrite(resolved);
        return wad_entry_find_across_mounted_archives(wad_name, -1);
    }
    return 0;
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

static int s_has_wav_suffix(const char* name)
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

int sound_bank_sound_on(void)
{
    return s_bank_sound_on;
}

// FUNCTION: LEGORR 0x0047aad0
int sound_bank_free_slot_find(int* out_slot)
{
    /* SoundBankFreeSlotFind @ 0x47aad0. */
    if (out_slot == NULL) {
        return 0;
    }
    for (int i = 0; i < SOUND_BANK_SLOTS; ++i) {
        if ((s_bank[i].flags & SOUND_SLOT_BUSY) == 0) {
            *out_slot = i;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0047a850
#if defined(_MSC_VER) && defined(_M_IX86)
__declspec(naked) int sound_volume_value_valid(int millibels)
{
    __asm {
        mov eax, dword ptr [esp + 4]
        test eax, eax
        jg sound_volume_invalid
        cmp eax, -10000
        jl sound_volume_invalid
        mov eax, 1
        ret
    sound_volume_invalid:
        xor eax, eax
        ret
    }
}
#else
int sound_volume_value_valid(int millibels)
{
    /* SoundVolumeValueValid @ 0x47a850: (-10001 < v < 1). Deviation: the
     * corpus gates with test/jg then cmp -10000/jl; MSVC5 canonicalizes
     * the split-gate source shape into cmp/setge form, so keep the
     * two-sided expression which scores best. */
    return (millibels < 1 && -10001 < millibels) ? 1 : 0;
}
#endif

// FUNCTION: LEGORR 0x0047a870
int sound_bank_entry_load(const char* name, int streamed, int looping,
                          int volume)
{
    /* SoundBankEntryLoad @ 0x47a870. */
    if (!s_bank_sound_on) {
        return -1; /* corpus's DAT_00533878 gate */
    }
    int slot;
    if (name == NULL || !sound_bank_free_slot_find(&slot)) {
        return -1;
    }

    BankSlot* rec = &s_bank[slot];
    rec->flags = 0;      /* claimed record starts with cleared flags */
    rec->sound = NULL;
    rec->buffers[0] = NULL;
    rec->buffers[1] = NULL;
    rec->buffers[2] = NULL;
    rec->next_buffer = 0;
    rec->volume = 0;
    rec->name[0] = '\0';

    char full[1024];
    if (!s_has_wav_suffix(name)) {
        if (strlen(name) + 5 >= sizeof(full)) {
            return -1;
        }
        snprintf(full, sizeof(full), "%s.wav", name);
    } else {
        if (strlen(name) + 1 >= sizeof(full)) {
            return -1;
        }
        snprintf(full, sizeof(full), "%s", name);
    }

    /* Loader seam: the port's resource-layer loader stands in for the
     * corpus's platform mmio path (InMemoryWaveSampleLoad's role),
     * producing the same observable loaded record. */
    Sound* s = sound_load(full);
    if (s == NULL) {
        return -1; /* flags stay zeroed: the slot was never claimed */
    }
    if (streamed != 0) {
        rec->flags |= SOUND_STREAMED;
    } else {
        if (looping != 0) {
            rec->flags |= SOUND_LOOPING;
        }
    }

    size_t n = strlen(full);
    if (n >= sizeof(rec->name)) {
        n = sizeof(rec->name) - 1; /* truncate like the record copy loop */
    }
    memcpy(rec->name, full, n);
    rec->name[n] = '\0';
    s->flags = rec->flags & (SOUND_LOOPING | SOUND_STREAMED);
    rec->sound = s;
    rec->volume = sound_volume_value_valid(volume) ? volume : 0;
    rec->flags |= SOUND_SLOT_BUSY | SOUND_SLOT_LOADED;
    if ((rec->flags & SOUND_STREAMED) == 0u
        && sound_record_create_buffers(slot) == 0) {
        audio_buffer_destroy(rec->buffers[0]);
        audio_buffer_destroy(rec->buffers[1]);
        audio_buffer_destroy(rec->buffers[2]);
        rec->buffers[0] = NULL;
        rec->buffers[1] = NULL;
        rec->buffers[2] = NULL;
        sound_free(rec->sound);
        rec->sound = NULL;
        rec->flags = 0;
        return -1;
    }
    return slot;
}

// FUNCTION: LEGORR 0x0047ab00
int sound_bank_entry_release(int slot)
{
    AudioPlayer* player;
    int stream_index;
    /* SoundBankEntryRelease @ 0x47ab00. */
    if (!s_bank_sound_on || slot < 0 || slot >= SOUND_BANK_SLOTS) {
        return 0;
    }
    s_bank[slot].flags &= ~(uint32_t)SOUND_SLOT_BUSY;
    player = audio_system_player_get();
    if (player != NULL && s_bank[slot].sound != NULL) {
        if (player->loop_sound == s_bank[slot].sound)
            audio_stop(player);
        for (stream_index = 0; stream_index < 2; ++stream_index) {
            if (player->stream_sample[stream_index] == s_bank[slot].sound)
                audio_stream_stop(player, stream_index);
        }
    }
    audio_buffer_destroy(s_bank[slot].buffers[0]);
    audio_buffer_destroy(s_bank[slot].buffers[1]);
    audio_buffer_destroy(s_bank[slot].buffers[2]);
    s_bank[slot].buffers[0] = NULL;
    s_bank[slot].buffers[1] = NULL;
    s_bank[slot].buffers[2] = NULL;
    if (s_bank[slot].sound != NULL) {
        sound_free(s_bank[slot].sound);
        s_bank[slot].sound = NULL;
    }
    return 1;
}

uint32_t sound_bank_entry_flags(int slot)
{
    if (slot < 0 || slot >= SOUND_BANK_SLOTS) {
        return 0;
    }
    return s_bank[slot].flags;
}

// FUNCTION: LEGORR 0x0047b0d0
int sound_bank_entry_volume(int slot)
{
    /* IndexedRecordStride134WordGet @ 0x47b0d0 (record +0x110 word). */
    return s_bank[slot].volume;
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
static SoundPositionApplyHook s_sound_position_hook;
static void* s_sound_position_context;
static SoundInstanceRegisterHook s_instance_register_hook;
static SoundInstanceStopHook s_instance_stop_hook;
static void* s_instance_hook_context;
static void* s_sound_instances[20];
typedef struct SoundInstanceHostRecord {
    void* provider;
    void* instance;
    AudioBuffer* buffer;
    struct SoundInstanceHostRecord* next;
} SoundInstanceHostRecord;
static SoundInstanceHostRecord* s_sound_instance_host_records;
static SoundInstanceHostRecord* sound_instance_host_record_get(
    void* provider, void* instance, int create);
static void sound_instance_host_records_remove(void* provider,
                                               void* instance);
void sound_buffer_association_remove_instance(void* instance);
static SoundSpatialRefreshHook s_spatial_refresh_hook;
static void* s_spatial_refresh_context;

// FUNCTION: LEGORR 0x0047b790
int sound_wave_sample_duplicate_buffer_create(int slot, int buffer_index)
{
    Sound* sample = sound_bank_entry_sample_get(slot);
    AudioBuffer* buffer;
    if (sample == NULL || buffer_index < 0 || buffer_index >= 3) return 0;
    buffer = audio_buffer_create(sample);
    if (buffer == NULL) return 0;
    audio_buffer_destroy(s_bank[slot].buffers[buffer_index]);
    s_bank[slot].buffers[buffer_index] = buffer;
    audio_buffer_volume_set(buffer, s_bank[slot].volume);
    return 1;
}

// FUNCTION: LEGORR 0x0047b6c0
int sound_record_create_buffers(int slot)
{
    int i;
    int created = 0;
    int count;
    if (sound_bank_entry_sample_get(slot) == NULL) return 0;
    /* The reference's streamed-file branch initializes a stream state and
     * skips SoundRecordCreateBuffers; duplicate buffers belong to the
     * in-memory sample path only. */
    if ((s_bank[slot].flags & SOUND_STREAMED) != 0u) return 0;
    count = (s_bank[slot].flags & SOUND_LOOPING) != 0u ? 3 : 1;
    for (i = 0; i < count; ++i) {
        if (!sound_wave_sample_duplicate_buffer_create(slot, i)) break;
        ++created;
    }
    return created == count ? created : 0;
}

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

// FUNCTION: LEGORR 0x0047b0f0
AudioBuffer* sound_handle_buffer_get(int buffer_handle)
{
    uint32_t slot;
    int buffer_index;
    if (buffer_handle < 0 ||
        buffer_handle >= SOUND_BANK_SLOTS * 3) return NULL;
    buffer_index = buffer_handle % 3;
    slot = (uint32_t)(buffer_handle - buffer_index) / 3u;
    return s_bank[slot].buffers[buffer_index];
}

// FUNCTION: LEGORR 0x0047b130
void sound_buffer_stop_handle(int buffer_handle)
{
    AudioBuffer* buffer = sound_handle_buffer_get(buffer_handle);
    /* The retail body resolves the handle first, then checks the bank gate,
     * the -1 sentinel, and the returned interface before calling Stop. */
    if (s_bank_sound_on != 0 && buffer_handle != -1 && buffer != NULL)
        audio_buffer_stop(buffer);
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

// FUNCTION: LEGORR 0x0047ad10
void sound_position_set_from_vector(void* buffer_object,
                                    const float position[3])
{
    if (buffer_object == NULL || position == NULL
        || s_sound_position_hook == NULL) return;
    s_sound_position_hook(s_sound_position_context, buffer_object, position,
                          0u);
}

void sound_instance_hooks_set(SoundInstanceRegisterHook register_hook,
                              SoundInstanceStopHook stop_hook,
                              void* context)
{
    s_instance_register_hook = register_hook;
    s_instance_stop_hook = stop_hook;
    s_instance_hook_context = context;
}

static void sound_frame_release(void* frame);

static int sound_instance_callback_record_attach(void* instance)
{
    uint32_t* payload;
    void* frame_interface;
    if (sizeof(void*) != 4u || instance == NULL) return 0;

    frame_interface = sound_instance_property_get(instance);
    payload = (uint32_t*)engine_heap_alloc(0x14L);
    if (payload == NULL) return 0;
    memset(payload, 0, 0x14u);
    payload[0] = (uint32_t)(uintptr_t)instance;
    if (frame_interface != NULL)
        payload[4] = *(uint32_t*)((char*)frame_interface + 0x24);
    ddiscene_animation_hook_create(instance, 0, NULL, NULL, 0, NULL, NULL,
                                   0, 0, payload);
    /* The retail caller does not release its 0x14-byte payload after the
     * 0x475f70 call. That helper copies word 0, but no owner for this block is
     * visible in the checked-in path. Preserve the observed lifetime. */
    return sound_instance_property_get(instance) != NULL;
}

/* FUN_0047b1a0's 0x14-byte payload contains target pointers and a provider
 * list link. On 64-bit hosts keep the same ownership relationships in a
 * native-width sidecar instead of truncating those pointers into the retail
 * record layout. */
static SoundInstanceHostRecord* sound_instance_host_record_get(
    void* provider, void* instance, int create)
{
    SoundInstanceHostRecord* record = s_sound_instance_host_records;
    while (record != NULL) {
        if (record->provider == provider && record->instance == instance)
            return record;
        record = record->next;
    }
    if (!create || provider == NULL || instance == NULL) return NULL;
    record = (SoundInstanceHostRecord*)engine_heap_alloc(
        (long)sizeof(SoundInstanceHostRecord));
    if (record == NULL) return NULL;
    memset(record, 0, sizeof(*record));
    record->provider = provider;
    record->instance = instance;
    record->next = s_sound_instance_host_records;
    s_sound_instance_host_records = record;
    return record;
}

static void sound_instance_host_records_remove(void* provider,
                                               void* instance)
{
    SoundInstanceHostRecord** link = &s_sound_instance_host_records;
    while (*link != NULL) {
        SoundInstanceHostRecord* record = *link;
        if ((provider == NULL || record->provider == provider) &&
            (instance == NULL || record->instance == instance)) {
            if (record->buffer != NULL) audio_buffer_stop(record->buffer);
            *link = record->next;
            engine_heap_free(record);
        }
        else {
            link = &record->next;
        }
    }
}

static void sound_instance_host_buffer_set(void* provider, AudioBuffer* buffer)
{
    SoundInstanceHostRecord* record =
        sound_instance_host_record_get(provider, provider, 1);
    if (record != NULL) record->buffer = buffer;
}

// FUNCTION: LEGORR 0x0047b1a0
void sound_instance_track_register(void* instance)
{
    int index;
    if (instance == NULL) return;
    if (!sound_instance_callback_record_attach(instance) &&
        s_instance_register_hook != NULL)
        (void)s_instance_register_hook(s_instance_hook_context, instance);
    if (sizeof(void*) != 4u)
        (void)sound_instance_host_record_get(instance, instance, 1);
    for (index = 0; index < 20; index++) {
        if (s_sound_instances[index] == NULL) {
            s_sound_instances[index] = instance;
            return;
        }
    }
}

// FUNCTION: LEGORR 0x0047b220
void sound_instance_track_stop_and_remove(void* provider, void* instance)
{
    int index;
    int adapter = 0;

    /* The first argument is the provider frame (the reference obtains it
     * from the caller's this register). The second is the sound instance.
     * Release the instance before unlinking its source record, as the retail
     * vtable call does. */
#if (defined(_MSC_VER) && defined(_M_IX86)) || \
    (defined(__i386__) && (defined(__GNUC__) || defined(__clang__)))
    if (s_instance_stop_hook != NULL) {
        s_instance_stop_hook(s_instance_hook_context, provider, instance);
        adapter = 1;
    }
    else if (instance != NULL) {
        sound_frame_release(instance);
    }
#else
    if (s_instance_stop_hook != NULL)
        s_instance_stop_hook(s_instance_hook_context, provider, instance);
    adapter = 1;
#endif

    if (sizeof(void*) != 4u)
        sound_instance_host_records_remove(provider, instance);

    if (provider == NULL) {
        sound_buffer_association_remove_instance(instance);
        return;
    }

    if (!adapter) {
        void* property = sound_instance_property_get(provider);
        if (property != NULL) {
            void* record = *(void**)((char*)property + 0x24);
            if (nested_record_list_remove_matching_key(provider, instance,
                                                       record)) {
                void* next = NULL;
                sound_buffer_association_remove_instance(instance);
                memcpy(&next, (char*)record + 4 * sizeof(void*),
                       sizeof(next));
                memcpy((char*)property + 0x24, &next, sizeof(next));
                engine_heap_free(record);
            }
        }
    }
    else {
        sound_buffer_association_remove_instance(instance);
    }
    for (index = 0; index < 20; index++) {
        if (s_sound_instances[index] == provider) {
            s_sound_instances[index] = NULL;
        }
    }
}

void sound_spatial_refresh_hooks_set(SoundSpatialRefreshHook refresh,
                                     void* context)
{
    s_spatial_refresh_hook = refresh;
    s_spatial_refresh_context = context;
}

/* The retail provider is an IDirect3DRMFrame3. Its GetScene and GetPosition
 * slots are +0x68 and +0x60, while linked source records live in the private
 * scene-animation record returned by the provider's +0x1c accessor. Each
 * linked source record is five dwords: DirectSound buffer, cached xyz, next.
 * These COM layouts are only valid in the 32-bit reference backend; the SDL
 * backend's AudioBuffer is a port object and cannot be called as DirectSound. */
#if defined(_MSC_VER) && defined(_M_IX86)
static void sound_frame_scene_get(void* frame, void** scene_out)
{
    __asm {
        mov ecx, frame
        mov eax, [ecx]
        mov edx, scene_out
        push edx
        call dword ptr [eax + 68h]
    }
}

static void sound_frame_position_get(void* frame, void* scene,
                                     float position[3])
{
    __asm {
        mov ecx, frame
        mov eax, [ecx]
        mov edx, position
        push edx
        push scene
        call dword ptr [eax + 60h]
    }
}

static void sound_frame_release(void* frame)
{
    __asm {
        mov ecx, frame
        mov eax, [ecx]
        call dword ptr [eax + 8]
    }
}

static void sound_buffer_position_set(void* buffer,
                                      const float position[3])
{
    __asm {
        mov ecx, buffer
        mov eax, [ecx]
        push 0
        mov edx, position
        push dword ptr [edx + 8]
        push dword ptr [edx + 4]
        push dword ptr [edx]
        call dword ptr [eax + 4Ch]
    }
}
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
typedef long (__attribute__((thiscall)) *SoundFrameGetSceneFn)(
    void*, void**);
typedef long (__attribute__((thiscall)) *SoundFrameGetPositionFn)(
    void*, void*, float*);
typedef unsigned long (__attribute__((thiscall)) *SoundFrameReleaseFn)(void*);
typedef long (__attribute__((thiscall)) *SoundBufferSetPositionFn)(
    void*, float, float, float, unsigned long);

static void sound_frame_scene_get(void* frame, void** scene_out)
{
    void** vtable = *(void***)frame;
    ((SoundFrameGetSceneFn)vtable[0x68 / sizeof(void*)])(frame, scene_out);
}

static void sound_frame_position_get(void* frame, void* scene,
                                     float position[3])
{
    void** vtable = *(void***)frame;
    ((SoundFrameGetPositionFn)vtable[0x60 / sizeof(void*)])(
        frame, scene, position);
}

static void sound_frame_release(void* frame)
{
    void** vtable = *(void***)frame;
    ((SoundFrameReleaseFn)vtable[8])(frame);
}

static void sound_buffer_position_set(void* buffer,
                                      const float position[3])
{
    void** vtable = *(void***)buffer;
    ((SoundBufferSetPositionFn)vtable[0x4c / sizeof(void*)])(
        buffer, position[0], position[1], position[2], 0ul);
}
#endif

// FUNCTION: LEGORR 0x0047B410
void sound_source_spatial_refresh(void* sound_object)
{
#if (defined(_MSC_VER) && defined(_M_IX86)) || \
    (defined(__i386__) && (defined(__GNUC__) || defined(__clang__)))
    void* property;
    void* scene = NULL;
    void* record;
    float position[3] = { 0.0f, 0.0f, 0.0f };
    if (sound_object == NULL) return;
    property = sound_instance_property_get(sound_object);
    if (property == NULL) {
        if (s_spatial_refresh_hook != NULL)
            (void)s_spatial_refresh_hook(s_spatial_refresh_context,
                                         sound_object);
        return;
    }

    /* GetScene and GetPosition HRESULTs are deliberately ignored, matching
     * the retail routine. Its source cache advances even if SetPosition
     * reports failure. */
    sound_frame_scene_get(sound_object, &scene);
    record = *(void**)((char*)property + 0x24);
    while (record != NULL) {
        char* words = (char*)record;
        void* buffer = NULL;
        void* next = NULL;
        float cached[3];
        memcpy(&buffer, words, sizeof(buffer));
        memcpy(cached, words + sizeof(void*), sizeof(cached));
        memcpy(&next, words + 4 * sizeof(void*), sizeof(next));

        sound_frame_position_get(sound_object, scene, position);
        if (position[0] != cached[0] || position[1] != cached[1] ||
            position[2] != cached[2]) {
            if (buffer != NULL) sound_buffer_position_set(buffer, position);
            memcpy(words + sizeof(void*), position, sizeof(position));
        }
        record = next;
    }
    if (scene != NULL) sound_frame_release(scene);
#else
    if (sound_object != NULL && s_spatial_refresh_hook != NULL)
        (void)s_spatial_refresh_hook(s_spatial_refresh_context, sound_object);
#endif
}

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

/* ------------------------------------------------------------------ *
 *  Configured sound-variant chains (issues #318 / #324). See sound.hpp.
 * ------------------------------------------------------------------ */

int s_variant_slot[SOUND_ALIAS_MAX]; /* DAT_00501468 head slot words */
int s_variant_next[SOUND_ALIAS_MAX]; /* DAT_0050146c head next words */

VariantNode s_variant_pool[SOUND_VARIANT_POOL_NODES];

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

// FUNCTION: LEGORR 0x0047b050
void sound_bank_entry_volume_set(int slot, int millibels)
{
    int last;
    int i;
    /* SoundBankEntryVolumeSet @ 0x47b050. */
    if (slot < 0 || slot >= SOUND_BANK_SLOTS) {
        return; /* corpus writes without bounds */
    }
    /* Validate-and-store: invalid millibel values land as 0. */
    s_bank[slot].volume = sound_volume_value_valid(millibels) ? millibels : 0;
    last = (s_bank[slot].flags & SOUND_LOOPING) != 0u ? 3 : 1;
    for (i = 0; i < last; ++i)
        audio_buffer_volume_set(s_bank[slot].buffers[i], s_bank[slot].volume);
}

/* ------------------------------------------------------------------ *
 *  Sound submission route (issues #325 / #328 / #329 / #330 / #332).
 *  See sound.hpp.
 * ------------------------------------------------------------------ */

/* Instance-layer play seam. The portable audio API can represent an
 * explicit source position as stereo pan for one-shot samples, while
 * object-attached sources are tracked through the existing spatial hooks.
 * Stream selection and loop-buffer ownership remain with the bank path. */
// FUNCTION: LEGORR 0x0047ad70
int s_play_seam(int mode, int position, int slot, int loop,
                       const float* xyz)
{
    Sound* sample;
    void* attached_provider = NULL;
    if (!s_bank_sound_on || slot < 0 || slot >= SOUND_BANK_SLOTS) return -1;

    if (mode == 0 && position != 0 &&
        (s_bank[slot].flags & SOUND_STREAMED) == 0u) {
        attached_provider = (void*)(uintptr_t)(uint32_t)position;
        sound_instance_track_register(attached_provider);
        sound_source_spatial_refresh(attached_provider);
    }

    /* The current portable 3D API is a one-shot pan operation. Keep
     * streamed/loop-selected and looping bank playback on its established
     * path; do not turn a DirectSound selector into a portable loop mode. */
    if (mode == 1 && xyz != NULL && loop == 0
        && (sound_bank_entry_flags(slot) & SOUND_STREAMED) == 0u) {
        int frames;
        int previous_buffer_index = s_bank[slot].next_buffer;
        float distance = (float)sqrt(xyz[0] * xyz[0]
                                   + xyz[1] * xyz[1]
                                   + xyz[2] * xyz[2]);
        float pan = 0.0f;
        if (distance > 0.0f) pan = xyz[0] / distance;
        sample = sound_bank_entry_sample_get(slot);
        if (sample != NULL) {
            if ((s_bank[slot].flags & SOUND_LOOPING) != 0u) {
                s_bank[slot].next_buffer =
                    (s_bank[slot].next_buffer + 1) % 3;
            }
            frames = audio_active_play_sound_3d(sample, pan);
            if (frames >= 0)
                return slot * 3 + s_bank[slot].next_buffer;
            s_bank[slot].next_buffer = previous_buffer_index;
        }
    }

    if (sound_bank_entry_play_mode(slot, loop, mode) < 0) return -1;
    if ((s_bank[slot].flags & SOUND_STREAMED) != 0u) return slot * 3;
    {
        int handle = slot * 3 + s_bank[slot].next_buffer;
        if (attached_provider != NULL)
            sound_instance_host_buffer_set(attached_provider,
                sound_handle_buffer_get(handle));
        return handle;
    }
}

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

// FUNCTION: LEGORR 0x0047b8e0
double sound_streamed_wave_buffer_step_rate(int slot)
{
    /* StreamedWaveBufferStepRate @ 0x47b8e0. slot -1 is the variant
     * select's "no sound" result: zero, before any table read. */
    if (slot == -1) {
        return 0.0;
    }
    if (slot < 0 || slot >= SOUND_BANK_SLOTS) {
        return 0.0; /* deviation: the corpus reads past its record table */
    }
    const BankSlot* rec = &s_bank[slot];
    const Sound* snd = rec->sound;
    uint32_t bytes = (snd != NULL) ? snd->file_len : 0u;
    double rate = (snd != NULL) ? (double)snd->format.byte_rate : 0.0;
    return (double)bytes / rate;
}

// FUNCTION: LEGORR 0x00416150
int sound_stream_play_level(int level)
{
    /* StreamPlayLevel @ 0x416150: the Stream_LevelName_Level%d alias at
     * 0x4a25c8, submitted through the global cooldown when it resolves. */
    char name[128];
    snprintf(name, sizeof(name), "Stream_LevelName_Level%d", level);
    int alias = 0;
    if (sound_alias_lookup(name, &alias) != 0) {
        return sound_submit_with_global_cooldown(alias, NULL);
    }
    return 0;
}

// FUNCTION: LEGORR 0x004161a0
int sound_stream_play_tutorial(int level)
{
    /* StreamPlayTutorial @ 0x4161a0. */
    char name[128];
    snprintf(name, sizeof(name), "Stream_LevelName_TLevel%d", level);
    int alias = 0;
    if (sound_alias_lookup(name, &alias) != 0) {
        return sound_submit_with_global_cooldown(alias, NULL);
    }
    return 0;
}

/* SoundBufferAssociationAdd (FUN_0047b020) — see sound.hpp. */
// GLOBAL: LEGORR 0x00506684
static void* s_buffer_assoc_head; /* DAT_00506684 role: unowned */
// GLOBAL: LEGORR 0x0050668c
static uint32_t s_tracked_buffer_min_distance;
// GLOBAL: LEGORR 0x00506690
static uint32_t s_tracked_buffer_max_distance;

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

// FUNCTION: LEGORR 0x0047b020
void sound_buffer_association_add(uint32_t a, uint32_t b, uint32_t c)
{
    void* node = engine_heap_alloc(4 * sizeof(void*));
    if (node == NULL) {
        return; /* deviation: the corpus strides the record blindly */
    }
    char* w = (char*)node;
    void* wa = (void*)(uintptr_t)a;
    void* wb = (void*)(uintptr_t)b;
    void* wc = (void*)(uintptr_t)c;
    __builtin_memcpy(w + 0 * sizeof(void*), &wa, sizeof wa);
    __builtin_memcpy(w + 1 * sizeof(void*), &wb, sizeof wb);
    __builtin_memcpy(w + 2 * sizeof(void*), &wc, sizeof wc);
    __builtin_memcpy(w + 3 * sizeof(void*), &s_buffer_assoc_head,
                     sizeof s_buffer_assoc_head);
    s_buffer_assoc_head = node;
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

// FUNCTION: LEGORR 0x0047b390
void sound_source_spatial_refresh_all(void)
{
    int index;
    void* node;
    for (index = 0; index < 20; index++) {
        if (s_sound_instances[index] != NULL)
            sound_source_spatial_refresh(s_sound_instances[index]);
    }
    node = s_buffer_assoc_head;
    while (node != NULL) {
        char* words = (char*)node;
        void* provider = NULL;
        void* buffer_object = NULL;
        void* attached_entity = NULL;
        void* next = NULL;
        int active = 1;
        memcpy(&provider, words, sizeof(provider));
        memcpy(&buffer_object, words + sizeof(void*), sizeof(buffer_object));
        memcpy(&attached_entity, words + 2 * sizeof(void*),
               sizeof(attached_entity));
        memcpy(&next, words + 3 * sizeof(void*), sizeof(next));
        /* The retail path calls the stored buffer's GetStatus (+0x24) and
         * removes the owner record when playback is no longer active. The
         * portable AudioBuffer has no voice-status query, so its owner must
         * provide this test through the refresh hook. */
        if (buffer_object != NULL && s_spatial_refresh_hook != NULL)
            active = s_spatial_refresh_hook(s_spatial_refresh_context,
                                            buffer_object);
        if (!active)
            sound_instance_track_stop_and_remove(provider, attached_entity);
        node = next;
    }
    sound_stream_ring_buffer_tick(0);
    sound_stream_ring_buffer_tick(1);
}

// FUNCTION: LEGORR 0x0047ad30
int sound_attached_stop_by_entity(uint32_t entity)
{
    void* node = s_buffer_assoc_head;
    while (node != NULL) {
        char* words = (char*)node;
        void* provider = NULL;
        void* attached_entity = NULL;
        void* next = NULL;
        memcpy(&provider, words, sizeof(provider));
        memcpy(&attached_entity, words + 2 * sizeof(void*),
               sizeof(attached_entity));
        memcpy(&next, words + 3 * sizeof(void*), sizeof(next));
        if ((uint32_t)(uintptr_t)attached_entity == entity) {
            sound_instance_track_stop_and_remove(provider,
                                                 attached_entity);
            return 1;
        }
        node = next;
    }
    if (sizeof(void*) != 4u) {
        SoundInstanceHostRecord* record = s_sound_instance_host_records;
        while (record != NULL) {
            if ((uint32_t)(uintptr_t)record->instance == entity) {
                void* provider = record->provider;
                void* instance = record->instance;
                sound_instance_track_stop_and_remove(provider, instance);
                return 1;
            }
            record = record->next;
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x0047b160
void sound_subsystem_stop_all(void)
{
    sound_buffer_association_stop_all();
    sound_instance_host_records_remove(NULL, NULL);
    sound_stream_stop_and_release(0);
    sound_stream_stop_and_release(1);
    audio_stop(audio_system_player_get());
}

void  sound_buffer_association_head_set(void* node)
{
    s_buffer_assoc_head = node;
}
void* sound_buffer_association_head_peek(void) { return s_buffer_assoc_head; }

/* The association record stores its interface in word 2 and its next node
 * in word 3. These setters mirror the target's vtable dispatch while using
 * the host-widened record stride established by SoundBufferAssociationAdd. */
class SoundTrackedBufferInterface
{
public:
    virtual void slot_00(void) = 0;
    virtual void slot_04(void) = 0;
    virtual void slot_08(void) = 0;
    virtual void slot_0c(void) = 0;
    virtual void slot_10(void) = 0;
    virtual void slot_14(void) = 0;
    virtual void slot_18(void) = 0;
    virtual void slot_1c(void) = 0;
    virtual void slot_20(void) = 0;
    virtual void slot_24(void) = 0;
    virtual void slot_28(void) = 0;
    virtual void slot_2c(void) = 0;
    virtual void slot_30(void) = 0;
    virtual void slot_34(void) = 0;
    virtual void slot_38(void) = 0;
    virtual void slot_3c(void) = 0;
    virtual void max_distance_set(uint32_t value, uint32_t zero) = 0;
    virtual void min_distance_set(uint32_t value, uint32_t zero) = 0;
};

// FUNCTION: LEGORR 0x0047b4a0
void sound_tracked_buffers_set_min_distance(uint32_t value)
{
    const uint32_t distance = value;
    void** record = (void**)s_buffer_assoc_head;
    s_tracked_buffer_min_distance = distance;
    while (record != NULL) {
        ((SoundTrackedBufferInterface*)record[2])->min_distance_set(distance, 0);
        record = (void**)record[3];
    }
}

// FUNCTION: LEGORR 0x0047b4d0
void sound_tracked_buffers_set_max_distance(uint32_t value)
{
    const uint32_t distance = value;
    void** record = (void**)s_buffer_assoc_head;
    s_tracked_buffer_max_distance = distance;
    while (record != NULL) {
        ((SoundTrackedBufferInterface*)record[2])->max_distance_set(distance, 0);
        record = (void**)record[3];
    }
}

// FUNCTION: LEGORR 0x00434a50
void sound_samples_config_load(BlockFile* config, int allow_marked)
{
    BlockRec* group;
    BlockRec* entry;
    if (config == NULL) return;
    group = block_subgroup(config,
        string_path_join(util_root_name(config), "Samples", (const char*)NULL));
    if (group == NULL) return;
    sound_alias_registration_set(1);
    for (entry = group; entry != NULL;
         entry = block_next_sibling(entry)) {
        const char* value = block_value(entry);
        const char* alias_name = value;
        int alias;
        if (alias_name == NULL) continue;
        if (alias_name[0] == '!' && allow_marked) ++alias_name;
        else if (alias_name[0] == '!' && !allow_marked) continue;
        if (sound_alias_lookup(alias_name, &alias))
            (void)sound_sample_marker_parse(block_name(entry), alias);
    }
    sound_alias_registration_set(0);
}

namespace unported_ledger_stubs {

void ledger_stub_00434a50_SoundSamplesConfigLoad(BlockFile* config,
                                                  int allow_marked)
{
    sound_samples_config_load(config, allow_marked);
}

} // namespace unported_ledger_stubs

namespace unported_ledger_stubs {

// FUNCTION: LEGORR 0x0047c100
float ledger_stub_0047c100_DirectSoundListenerRolloffSet(float rolloff)
{
    if (s_bank_sound_on == 0) return rolloff;
    if (rolloff < 0.0f) rolloff = 0.0f;
    if (rolloff > 10.0f) rolloff = 10.0f;
    return audio_listener_rolloff_set(rolloff);
}


} // namespace unported_ledger_stubs
