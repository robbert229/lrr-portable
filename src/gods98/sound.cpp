// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resource.hpp"
#include "sound.hpp"

/* Bank record (corpus record of stride 0x134; port keeps the fields the
 * shipped record carries where observable: name at record +0, volume at
 * +0x110, the flag word at +0x130). `sound` is the port-side payload
 * handle standing in for the record's data/buffer fields. */
typedef struct BankSlot {
    char     name[260];      /* stored "<base>.wav" resource name */
    Sound*   sound;          /* loaded payload, NULL until loaded */
    uint32_t flags;          /* SOUND_SLOT_* + SOUND_LOOPING/STREAMED */
    int      volume;         /* +0x110: validated millibels */
} BankSlot;

static BankSlot s_bank[SOUND_BANK_SLOTS];
static int      s_bank_sound_on = 1; /* DAT_00533878 seam, see sound.hpp */

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

static int s_lower(int c) { return c >= 'a' && c <= 'z'; }

static int s_strcasecmp(const char* a, const char* b)
{
    for (;;) {
        int ca = (unsigned char)*a++;
        int cb = (unsigned char)*b++;
        if (s_lower(ca)) ca -= 0x20;
        if (s_lower(cb)) cb -= 0x20;
        if (ca != cb) {
            return ca - cb;
        }
        if (ca == 0) {
            return 0;
        }
    }
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

Sound* sound_load(const char* name)
{
    Resource* r = resource_open(name, RESOURCE_AUTO);
    if (r == NULL) {
        return NULL;
    }
    if (resource_read(r) < 0) {
        resource_free(r);
        return NULL;
    }
    Sound* s = sound_parse((const uint8_t*)r->data, (uint32_t)r->size);
    resource_free(r);
    if (s != NULL) {
        size_t n = strlen(name) + 1;
        s->name = (char*)malloc(n);
        if (s->name == NULL) {
            sound_free(s);
            return NULL;
        }
        memcpy(s->name, name, n);
    }
    return s;
}

/* ------------------------------------------------------------------ *
 *  Sound bank (finding 14 §1: 600-slot table, flag bits 2/3).
 * ------------------------------------------------------------------ */

static int s_has_wav_suffix(const char* name)
{
    size_t n = strlen(name);
    return n > 4 && name[n - 4] == '.'
        && (name[n - 3] == 'w' || name[n - 3] == 'W')
        && (name[n - 2] == 'a' || name[n - 2] == 'A')
        && (name[n - 1] == 'v' || name[n - 1] == 'V');
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
            && s_strcasecmp(s_bank[i].name, fullname) == 0) {
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

int sound_volume_value_valid(int millibels)
{
    /* SoundVolumeValueValid @ 0x47a850: (-10001 < v < 1). */
    return (millibels < 1 && -10001 < millibels) ? 1 : 0;
}

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
    return slot;
}

void sound_bank_entry_release(int slot)
{
    /* SoundBankEntryRelease @ 0x47ab00. */
    if (!s_bank_sound_on || slot < 0 || slot >= SOUND_BANK_SLOTS) {
        return;
    }
    s_bank[slot].flags &= ~(uint32_t)SOUND_SLOT_BUSY;
    if (s_bank[slot].sound != NULL) {
        sound_free(s_bank[slot].sound);
        s_bank[slot].sound = NULL;
    }
    /* Corpus: releases the three buffer interfaces through their
     * vtables — TODO(corpus), no instance layer to hand them back to. */
}

uint32_t sound_bank_entry_flags(int slot)
{
    if (slot < 0 || slot >= SOUND_BANK_SLOTS) {
        return 0;
    }
    return s_bank[slot].flags;
}

int sound_bank_entry_volume(int slot)
{
    if (slot < 0 || slot >= SOUND_BANK_SLOTS) {
        return 0;
    }
    return s_bank[slot].volume;
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
static const char* const s_alias_builtins[SOUND_ALIAS_BUILTINS] = {
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
static char* s_alias_dynamic[SOUND_ALIAS_MAX - SOUND_ALIAS_BUILTINS];
static int   s_alias_dynamic_count;

/* The corpus's DAT_00502a2c sound flag word: bit 0 subsystem-enabled
 * (enable-set @ 0x465260), bit 1 dynamic-alias registration, bit 3 batch
 * mode (batch-mode-get @ 0x4652b0). */
static uint32_t s_sound_flags;

/* _DAT_00502b2c — the global-cooldown word (the corpus types it as a
 * double; it holds the alias submitted under the cooldown, whose writer
 * lands with #296). */
static double s_cooldown_alias;

static char* s_dupstr(const char* s)
{
    size_t n = strlen(s) + 1;
    char* copy = (char*)calloc(n, 1);
    if (copy != NULL) {
        memcpy(copy, s, n);
    }
    return copy;
}

void sound_alias_registry_init(void)
{
    /* Corpus: the built-in hashes are written into a fresh allocation;
     * the port's built-ins are static. Deviation (documented): the
     * corpus's repeated call leaks the table yet keeps the stale dynamic
     * count; the port frees and zeroes the dynamic side and leaves the
     * registration flag as-is, exactly the DAT_00502a2c behaviour. */
    for (int i = 0; i < s_alias_dynamic_count; ++i) {
        free(s_alias_dynamic[i]);
        s_alias_dynamic[i] = NULL;
    }
    s_alias_dynamic_count = 0;
}

void sound_alias_registration_set(int enable)
{
    /* SoundDynamicAliasRegistrationSet @ 0x00464c50: raises/clears the
     * value-2 bit of DAT_00502a2c (`|= 2` / `&= 0xfffffffd`). */
    if (enable != 0) {
        s_sound_flags |= SOUND_FLAG_REG_UNKNOWN;
    } else {
        s_sound_flags &= ~SOUND_FLAG_REG_UNKNOWN;
    }
}

int sound_alias_lookup(const char* name, int* out_index)
{
    /* SoundAliasLookup @ 0x00464c70. The corpus compares hash keys under
     * the KeyToAscii uppercase fold; s_strcasecmp applies the same fold
     * to the names. */
    if (name == NULL || out_index == NULL) {
        return 0; /* corpus's param_1 != 0 guard */
    }
    for (int i = 0; i < SOUND_ALIAS_BUILTINS; ++i) {
        if (s_strcasecmp(name, s_alias_builtins[i]) == 0) {
            *out_index = i;
            return 1;
        }
    }
    for (int i = 0; i < s_alias_dynamic_count; ++i) {
        if (s_strcasecmp(name, s_alias_dynamic[i]) == 0) {
            *out_index = SOUND_ALIAS_BUILTINS + i;
            return 1;
        }
    }
    if ((s_sound_flags & SOUND_FLAG_REG_UNKNOWN) != 0) {
        if (s_alias_dynamic_count >= SOUND_ALIAS_MAX - SOUND_ALIAS_BUILTINS) {
            return 0; /* port bound: the corpus writes past its 0x7bc
                       * table instead of failing */
        }
        int index = SOUND_ALIAS_BUILTINS + s_alias_dynamic_count;
        char* copy = s_dupstr(name);
        if (copy == NULL) {
            return 0;
        }
        s_alias_dynamic[s_alias_dynamic_count] = copy;
        ++s_alias_dynamic_count;
        *out_index = index;
        return 1;
    }
    return 0;
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

void sound_subsystem_enable_set(int enable, int stop_active)
{
    /* SoundSubsystemEnableSet @ 0x00465260. */
    if (enable != 0) {
        s_sound_flags |= SOUND_FLAG_ENABLED;
        return;
    }
    s_sound_flags &= ~SOUND_FLAG_ENABLED;
    if (stop_active != 0) {
        /* Corpus stop seam (instance-chain walk, TODO(corpus) — see
         * sound.hpp). Deviation: the port's disable leaves live playback
         * running until the instance layer lands. */
    }
    s_cooldown_alias = 0.0;
}

void sound_subsystem_enable_set_and_stop(int enable)
{
    /* SoundSubsystemEnableSetAndStop @ 0x004652a0. */
    sound_subsystem_enable_set(enable, 1);
}

int sound_subsystem_enabled(void)
{
    return (s_sound_flags & SOUND_FLAG_ENABLED) != 0;
}

int sound_batch_mode_get(void)
{
    /* SoundBatchModeGet @ 0x004652b0: `return DAT_00502a2c & 8` — the
     * corpus returns the masked word, so 8 (not 1) when set. */
    return (int)(s_sound_flags & SOUND_FLAG_BATCH);
}

double sound_deferred_last_alias(void)
{
    return s_cooldown_alias;
}

/* ------------------------------------------------------------------ *
 *  Deferred sound request queue (issue #317). See sound.hpp.
 * ------------------------------------------------------------------ */

static int s_queue_alias[SOUND_REQUEST_QUEUE_CAPACITY];  /* DAT_00502b30 */
static int s_queue_loop[SOUND_REQUEST_QUEUE_CAPACITY];   /* DAT_00502b58 */
static int s_queue_count;                                /* DAT_00502bd0 */

void sound_request_queue_submit(int alias, int loop)
{
    /* SoundRequestQueueSubmit @ 0x464f60. */
    if (s_queue_count >= SOUND_REQUEST_QUEUE_CAPACITY) {
        return; /* corpus drop-on-full: no counter bump, no error */
    }
    s_queue_alias[s_queue_count] = alias;
    s_queue_loop[s_queue_count] = loop;
    ++s_queue_count;
}

int sound_request_queue_count(void)
{
    return s_queue_count;
}

int sound_request_queue_get(int index, int* alias, int* loop)
{
    if (index < 0 || index >= s_queue_count) {
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
    s_queue_count = 0;
}

/* ------------------------------------------------------------------ *
 *  Deferred-play global cooldown (issues #326 / #327). See sound.hpp.
 * ------------------------------------------------------------------ */

static float s_cooldown_timer;   /* _DAT_00502b24 (zero-initialised) */
static int   s_cooldown_handle;  /* DAT_00502b28  (zero-initialised) */

int sound_deferred_timer_set_if_idle(float seconds)
{
    /* DeferredSoundTimerSetIfIdle @ 0x464ec0. */
    if (s_cooldown_timer <= 0.0f) {
        s_cooldown_handle = -1;
        s_cooldown_timer = seconds;
        return 1;
    }
    return 0;
}

void sound_deferred_play_expire(void)
{
    /* DeferredSoundPlayExpire @ 0x464e80. */
    if (s_cooldown_handle != -1 && s_cooldown_timer > 0.0f) {
        /* Corpus stop seam (SoundHandleStopForward -> the shared stop
         * path). Deviation: the instance layer is TODO(corpus), so the
         * port's seam is a no-op and only the bookkeeping runs. */
        s_cooldown_handle = -1;
        s_cooldown_timer = 0.0f;
    }
}

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

static int s_variant_slot[SOUND_ALIAS_MAX]; /* DAT_00501468 head slot words */
static int s_variant_next[SOUND_ALIAS_MAX]; /* DAT_0050146c head next words */

/* Corpus chain pool DAT_005023e0 (stride 8: slot word + next word),
 * allocation counter DAT_00502a28. The corpus identifies nodes by their
 * address and treats 0 as null, so the port reserves node id 0 for "no
 * link" and allocates every node exactly once (monotone counter). */
typedef struct VariantNode {
    int slot;   /* bank slot: -1 failed link, stale until first written */
    int next;   /* node id, 0 = end of chain */
} VariantNode;

static VariantNode s_variant_pool[SOUND_VARIANT_POOL_NODES];
static int         s_variant_pool_next;   /* DAT_00502a28 */

/* Variant draws: the corpus's thunk_FUN_0048e170 (shared RNG word
 * DAT_004afcc8), modelled module-locally the way level.cpp and
 * frontend.cpp do — same MSVCRT-style LCG, same 15-bit result. */
static uint32_t s_variant_rand_state = 1;

static int s_variant_rand(void)
{
    s_variant_rand_state = s_variant_rand_state * 214013u + 2531011u;
    return (int)((s_variant_rand_state >> 16) & 0x7fffu);
}

static int s_variant_pool_alloc(void)
{
    if (s_variant_pool_next >= SOUND_VARIANT_POOL_NODES) {
        return 0; /* port bound; the corpus scribbles past the pool */
    }
    ++s_variant_pool_next;
    s_variant_pool[s_variant_pool_next].next = 0; /* DAT_005023e4 write */
    /* The slot field keeps its previous content: unwritten links read as
     * stale chain data, exactly like the corpus's global region. */
    return s_variant_pool_next;
}

/* Comma split as FUN_00477440 (BlockTextTokenSplit) applies it here:
 * splits in place, keeps empty tokens between consecutive commas, and
 * never tests the final character as a separator start — the corpus
 * quirk that leaves a trailing comma in the last path. Tokens beyond
 * the port's copy of the 100-entry stack buffer split but go untracked
 * (the corpus writes past its own frame); returns the tracked count. */
static int s_variant_split(char* s, char** out, int max)
{
    if (s[0] == '\0') {
        return 0;
    }
    out[0] = s;
    int n = 1;
    for (char* p = s; p[1] != '\0'; ++p) {
        if (*p == ',') {
            *p = '\0';
            if (n < max) {
                out[n++] = p + 1;
            }
        }
    }
    return n;
}

int sound_sample_marker_parse(const char* spec, int alias_index)
{
    if (spec == NULL || alias_index < 0 || alias_index >= SOUND_ALIAS_MAX) {
        return 0; /* corpus: NULL deref / out-of-bounds scribble */
    }

    /* Chain reset: the head's next word (DAT_0050146c) zeroes — the head
     * slot itself keeps its value until the first write. */
    s_variant_next[alias_index] = 0;

    char buf[1024]; /* corpus splits destructively in the config string;
                     * the port splits a copy (documented deviation) */
    snprintf(buf, sizeof(buf), "%s", spec);
    char* tokens[SOUND_VARIANT_TOKENS];
    int ntok = s_variant_split(buf, tokens, SOUND_VARIANT_TOKENS);

    int stream_mode = 0;            /* '@' marker, sticky (finding 135) */
    int ok = 1;

    /* Corpus cursors: W (piVar9) is the link written next, C (local_1d4)
     * the previous link. W starts at the chain head, C starts null. */
    int w_head = 1, w_id = alias_index;
    int c_head = 0, c_id = 0;
    int c_valid = 0;

    for (int t = 0; t < ntok; ++t) {
        char* p = tokens[t];
        int looping = (p[0] == '*'); /* '*' = duplicate-buffer request */
        if (looping) {
            ++p;
        }
        int volume = 0;
        if (p[0] == '#') {           /* '#N#' millibel marker, finding 142 */
            ++p;
            char digits[64];         /* corpus local_1d0[64] */
            size_t d = 0;
            if (*p != '#') {         /* empty '#...#' parses as 0 (the
                                     * corpus reads its stack garbage) */
                for (; *p != '\0' && *p != '#'; ++p) {
                    if (d < sizeof(digits) - 1) {
                        digits[d++] = *p;
                    }
                }
                if (*p == '\0') {
                    /* Unterminated '#': the corpus walks past the NUL and
                     * parses its stack; the port parses what it captured
                     * and loads the (empty) tail as the path. */
                }
            }
            digits[d] = '\0';
            if (*p == '#') {
                ++p;                 /* step over the closing '#' */
            }
            volume = (int)strtol(digits, NULL, 10); /* FUN_0048db70 role */
        }
        if (p[0] == '@') {
            stream_mode = 1;
            ++p;
        }

        int slot = sound_bank_entry_load(p, stream_mode, looping, volume);
        if (w_head) {
            s_variant_slot[w_id] = slot;   /* *piVar9 = iVar5 */
        } else {
            s_variant_pool[w_id].slot = slot;
        }

        if (slot == -1) {
            ok = 0;  /* failed link stays linked with its -1 (as shipped);
                      * both cursors stay where they were */
            continue;
        }
        if (c_valid) {
            int node = s_variant_pool_alloc();
            if (node != 0) {
                if (c_head) {
                    s_variant_next[c_id] = node;
                } else {
                    s_variant_pool[c_id].next = node;
                }
                w_head = 0;
                w_id   = node;
            }
            /* Pool full: the link is dropped (the corpus scribbles past
             * the pool region instead). */
        }
        c_valid = 1;
        c_head  = w_head;
        c_id    = w_id;
    }
    return ok;
}

int sound_variant_select(int alias_index)
{
    if (alias_index <= 0 || alias_index >= SOUND_ALIAS_MAX) {
        return -1; /* corpus: alias 0 (the silent alias) returns -1 and
                    * negatives scribble out of bounds */
    }
    if (s_variant_next[alias_index] != 0) {
        int count = 1;
        for (int node = s_variant_next[alias_index]; node != 0;
             node = s_variant_pool[node].next) {
            ++count;
        }
        int pick = s_variant_rand() % count;
        int head_slot = s_variant_slot[alias_index];
        if (pick != 0) {
            int node = s_variant_next[alias_index];
            for (int i = 1; i < pick; ++i) {
                node = s_variant_pool[node].next;
            }
            int tmp = s_variant_pool[node].slot;
            s_variant_pool[node].slot = head_slot;
            head_slot = tmp; /* corpus swaps picked slot with head */
        }
        s_variant_slot[alias_index] = head_slot;
    }
    return s_variant_slot[alias_index];
}

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
