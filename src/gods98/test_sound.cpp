// SPDX-License-Identifier: LGPL-3.0-or-later
/* test_sound — validation for src/gods98/sound.cpp.
 *
 * Synthetic RIFF/WAVE builds (PCM 8/16-bit, odd-size chunk padding, chunk
 * order variants, junk rejection) always run. With `--real <dir>` the shipped
 * wads are mounted and ALL 387 .wav entries are parsed (each must succeed) and
 * five reference entries are compared against hashes/format produced from the
 * corpus via the python `wave` module — the C parser is byte-identical to it
 * for those files.
 *
 * Wire-up: meson test 'sound' (suite 'sound'); real-wad checks via the
 * `test_wads_dir` meson option.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resource.hpp"
#include "gods98/heap.hpp"
#include "sound.hpp"

static int s_failures = 0;

static void CHECK(int cond, const char* what)
{
    if (!cond) {
        printf("FAIL: %s\n", what);
        ++s_failures;
    }
}

/* ------------------------------------------------------------------ *
 *  Synthetic RIFF/WAVE builder.
 * ------------------------------------------------------------------ */

typedef struct WavSeg {
    const char* id;          /* 4-char chunk id; NULL = fmt, "DATA!" = data */
    const uint8_t* bytes;    /* payload (NULL for fmt/data generated) */
    uint32_t len;
} WavSeg;

static size_t seg_payload_len(const WavSeg* seg, uint32_t pcm_len)
{
    if (seg->id == NULL) {
        return 16; /* fmt */
    }
    if (strcmp(seg->id, "DATA!") == 0) {
        return pcm_len;
    }
    return seg->len;
}

static void put32(uint8_t* p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xff);
    p[1] = (uint8_t)((v >> 8) & 0xff);
    p[2] = (uint8_t)((v >> 16) & 0xff);
    p[3] = (uint8_t)((v >> 24) & 0xff);
}

static uint8_t* build_wav(int ch, int bits, int rate,
                          const uint8_t* pcm, uint32_t pcm_len,
                          const WavSeg* segs, int nsegs, const uint8_t* acid,
                          uint32_t acid_len, size_t* out_size)
{
    size_t total = 12;
    for (int i = 0; i < nsegs; ++i) {
        total += 8 + seg_payload_len(&segs[i], pcm_len);
    }
    /* odd-size payload padding */
    for (int i = 0; i < nsegs; ++i) {
        size_t l = seg_payload_len(&segs[i], pcm_len);
        if (l & 1) {
            total += 1;
        }
    }
    uint8_t* b = (uint8_t*)calloc(1, total);
    if (b == NULL) {
        return NULL;
    }
    memcpy(b, "RIFF", 4);
    put32(b + 4, (uint32_t)(total - 8));
    memcpy(b + 8, "WAVE", 4);
    size_t pos = 12;
    for (int i = 0; i < nsegs; ++i) {
        const WavSeg* s = &segs[i];
        size_t l = seg_payload_len(s, pcm_len);
        if (s->id == NULL) {
            memcpy(b + pos, "fmt ", 4);
            put32(b + pos + 4, 16);
            uint8_t* f = b + pos + 8;
            put32(f, 1);
            put32(f + 2, (uint32_t)ch);
            put32(f + 4, (uint32_t)rate);
            put32(f + 8, rate * ch * bits / 8);
            put32(f + 12, (uint32_t)(ch * bits / 8));
            f[14] = (uint8_t)bits; f[15] = 0;
        } else if (strcmp(s->id, "DATA!") == 0) {
            memcpy(b + pos, "data", 4);
            put32(b + pos + 4, pcm_len);
            if (pcm_len > 0) {
                memcpy(b + pos + 8, pcm, pcm_len);
            }
        } else {
            memcpy(b + pos, s->id, 4);
            put32(b + pos + 4, (uint32_t)s->len);
            if (s->len > 0) {
                memcpy(b + pos + 8, s->bytes, s->len);
            }
        }
        pos += 8 + l + (l & 1);
    }
    (void)acid; (void)acid_len;
    *out_size = total;
    return b;
}

static void check_fmt(const Sound* s, int ch, int bits, int rate, const char* what)
{
    CHECK(s->format.format_tag == 1, what);
    CHECK(s->format.channels == ch, what);
    CHECK(s->format.bits_per_sample == bits, what);
    CHECK(s->format.sample_rate == (uint32_t)rate, what);
}

static void test_synthetic(void)
{
    uint8_t pcm16[80];
    for (int i = 0; i < 80; ++i) {
        pcm16[i] = (uint8_t)(i * 7 + 3);
    }
    /* canonical: fmt, data (16-bit mono 22050, even size). */
    {
        WavSeg segs[] = { { NULL, 0, 0 }, { "DATA!", 0, 0 } };
        size_t sz = 0;
        uint8_t* b = build_wav(1, 16, 22050, pcm16, (uint32_t)sizeof(pcm16),
                               segs, 2, NULL, 0, &sz);
        CHECK(b != NULL, "syn: build 16-bit");
        Sound* s = sound_parse(b, (uint32_t)sz);
        CHECK(s != NULL, "syn: 16-bit parse");
        if (s != NULL) {
            check_fmt(s, 1, 16, 22050, "syn: 16-bit fmt");
            CHECK(s->data_len == 80
                  && memcmp(s->data, pcm16, sizeof(pcm16)) == 0,
                  "syn: 16-bit data");
            sound_free(s);
        }
        free(b);
    }
    /* odd-size extra chunk (LIST 5 bytes) + data, order: LIST, fmt, data. */
    {
        uint8_t oddbuf[5] = { 1, 2, 3, 4, 5 };
        WavSeg segs[] = { { "LIST", oddbuf, 5 }, { NULL, 0, 0 },
                          { "DATA!", 0, 0 } };
        size_t sz = 0;
        uint8_t b2[8]; (void)b2;
        uint8_t* b = build_wav(2, 8, 11025, pcm16, 17, segs, 3, NULL, 0, &sz);
        Sound* s = sound_parse(b, (uint32_t)sz);
        CHECK(s != NULL, "syn: order+padding parse");
        if (s != NULL) {
            check_fmt(s, 2, 8, 11025, "syn: stereo 8-bit fmt");
            CHECK(s->data_len == 17, "syn: odd data len preserved");
            sound_free(s);
        }
        free(b);
    }
    /* data before fmt (order independence). */
    {
        WavSeg segs[] = { { "DATA!", 0, 0 }, { NULL, 0, 0 } };
        size_t sz = 0;
        uint8_t* b = build_wav(1, 16, 44100, pcm16, 20, segs, 2, NULL, 0, &sz);
        Sound* s = sound_parse(b, (uint32_t)sz);
        CHECK(s != NULL, "syn: data-before-fmt");
        if (s != NULL) {
            check_fmt(s, 1, 16, 44100, "syn: data-before-fmt fmt");
            sound_free(s);
        }
        free(b);
    }
    /* rejections. */
    {
        WavSeg segs[] = { { NULL, 0, 0 }, { "DATA!", 0, 0 } };
        size_t sz = 0;
        uint8_t* b = build_wav(1, 16, 22050, pcm16, 80, segs, 2, NULL, 0, &sz);
        CHECK(sound_parse(b, 8) == NULL, "syn: truncated rejected");
        free(b);
        uint8_t* bad = (uint8_t*)malloc(32);
        memcpy(bad, "NOPE", 4);
        CHECK(sound_parse(bad, 32) == NULL, "syn: non-RIFF rejected");
        free(bad);
    }
    /* non-PCM format tag rejected. */
    {
        WavSeg segs[] = { { NULL, 0, 0 }, { "DATA!", 0, 0 } };
        size_t sz = 0;
        uint8_t* b = build_wav(1, 16, 22050, pcm16, 80, segs, 2, NULL, 0, &sz);
        put32(b + 12 + 8, 3); /* fmt tag -> float */
        CHECK(sound_parse(b, (uint32_t)sz) == NULL, "syn: non-PCM rejected");
        free(b);
    }
}

/* ------------------------------------------------------------------ *
 *  Real-wad validation (--real <dir>).
 * ------------------------------------------------------------------ */

typedef struct RefSound {
    const char* name;  /* full archive-relative name */
    int  ch, bits, rate;
    uint32_t data_len;
    uint32_t hash;
} RefSound;

static const RefSound REFS[] = {
    { "Sounds\\Voices\\BuildingNames\\docks2.wav", 1, 8, 11025,  8112, 0xe7fc3c9e },
    { "Sounds\\Buildings\\BTHUD.wav",              1, 16, 22050, 73982, 0xc2bac4fc },
    { "Sounds\\Buildings\\barrierplace2.WAV",      1, 16, 16000, 11402, 0xed113e81 },
    { "Sounds\\Buildings\\elec5.wav",              1, 16, 44100, 58880, 0xc7135d8b },
    { "Sounds\\Buildings\\barrierwood.wav",        1, 16, 11025, 12184, 0x5638be36 },
};

static int endswith_wav(const char* n)
{
    size_t l = strlen(n);
    if (l < 5) {
        return 0;
    }
    return (n[l - 4] == '.' && n[l - 3] == 'w' && n[l - 2] == 'a'
            && n[l - 1] == 'v')
        || (n[l - 4] == '.' && n[l - 3] == 'W' && n[l - 2] == 'A'
            && n[l - 1] == 'V');
}

static void test_real(const char* dir)
{
    resource_mount(dir, "LegoRR");

    /* Every shipped .wav must parse (finding 14 §2: corpus is all PCM). */
    int total = 0, failed = 0;
    for (int w = 0; w < 2; ++w) {
        uint32_t n = resource_wad_entry_count(w);
        for (uint32_t i = 0; i < n; ++i) {
            const char* name = resource_wad_entry_name(w, (int)i);
            if (name == NULL || !endswith_wav(name)) {
                continue;
            }
            ++total;
            Resource* r = resource_open(name, RESOURCE_WAD_ONLY);
            if (r == NULL || resource_read(r) < 0) {
                ++failed;
                resource_free(r);
                continue;
            }
            Sound* s = sound_parse((const uint8_t*)r->data,
                                   (uint32_t)r->size);
            if (s == NULL) {
                printf("  FAIL parse: %s\n", name);
                ++failed;
            } else {
                sound_free(s);
            }
            resource_free(r);
        }
    }
    CHECK(total == 387, "real: 387 .wav entries found");
    CHECK(failed == 0, "real: every .wav parsed");

    /* Reference entries: format + PCM data hash (wave-module derived). */
    for (int i = 0; i < (int)(sizeof(REFS) / sizeof(REFS[0])); ++i) {
        Sound* s = sound_load(REFS[i].name);
        CHECK(s != NULL, "real: open");
        if (s == NULL) {
            continue;
        }
        check_fmt(s, REFS[i].ch, REFS[i].bits, REFS[i].rate, "real: fmt");
        CHECK(s->data_len == REFS[i].data_len, "real: data_len");
        CHECK(sound_pcm_hash(s) == REFS[i].hash, "real: pcm hash");
        sound_free(s);
    }

    /* Bank behavior (finding 14 §1). */
    CHECK(sound_bank_load("Sounds\\Buildings\\BTHUD", 1, 0) == 0,
          "real: bank load streamed");
    CHECK(sound_bank_count() == 1, "real: bank count == 1");
    Sound* f = sound_bank_find("sounds\\buildings\\bthud.wav");
    CHECK(f != NULL && (f->flags & SOUND_STREAMED) != 0,
          "real: bank find + streamed flag");
    CHECK(sound_bank_find("SOUNDS\\BUILDINGS\\BTHUD.WAV") == f,
          "real: bank find case-insensitive");
    CHECK(sound_bank_load("Sounds\\Buildings\\BTHUD", 1, 0) == 0
          && sound_bank_count() == 1, "real: bank load idempotent");
    CHECK(sound_bank_load("Sounds\\Voices\\BuildingNames\\docks2", 0, 1) == 0,
          "real: bank load looping");
    Sound* g = sound_bank_find("Sounds\\Voices\\BuildingNames\\docks2.wav");
    CHECK(g != NULL && (g->flags & SOUND_LOOPING) != 0,
          "real: bank loop flag");
    sound_bank_clear();
    CHECK(sound_bank_count() == 0, "real: bank clear");

    resource_unmount_all();
}

/* ------------------------------------------------------------------ *
 *  Alias registry (SoundAliasRegistryInit @ 0x00464740, SoundAliasLookup
 *  @ 0x00464c70, SoundDynamicAliasRegistrationSet @ 0x00464c50 — issues
 *  #313 / #321 / #314, finding 137).
 * ------------------------------------------------------------------ */

static void test_alias_registry(void)
{
    static const char* const expected[45] = {
        "SFX_NULL", "SFX_Stamp", "SFX_Drill", "SFX_DrillFade",
        "SFX_RockBreak", "SFX_Drip", "SFX_Ambient", "SFX_AmbientLoop",
        "SFX_Step", "SFX_RockMonster", "SFX_RockMonster2",
        "SFX_RockMonsterStep", "SFX_MFDeposit", "SFX_ButtonPressed",
        "SFX_MFLift", "SFX_MFThrow", "SFX_Walker", "SFX_YesSir",
        "SFX_Build", "SFX_Okay", "SFX_NotOkay",
        "SFX_InterfaceSlideOnScreen", "SFX_InterfaceSlideOffScreen",
        "SFX_PanelSlideOnScreen", "SFX_PanelSlideOffScreen", "SFX_Siren",
        "SFX_CrystalRecharge", "SFX_Laser", "SFX_LaserHit",
        "SFX_LazerRecharge", "SFX_TopPriority", "SFX_ImmovableRock",
        "SFX_Wall", "SFX_Floor", "SFX_BoulderHit", "SFX_Place",
        "SFX_PlaceOre", "SFX_PlaceCrystal", "SFX_Lava", "SFX_RockWipe",
        "SFX_FallIn", "SFX_MusicLoop", "SFX_CaptainSlide", "SFX_Dynamite",
        "SFX_AmbientMusicLoop",
    };

    sound_alias_registry_init();
    CHECK(sound_alias_count() == 45, "registry init builds 45 built-ins");
    for (int i = 0; i < 45; ++i) {
        const char* n = sound_alias_name(i);
        CHECK(n != NULL && strcmp(n, expected[i]) == 0, "built-in name/order");
    }
    /* Indices pinned by the findings, not just the literal table. */
    CHECK(sound_alias_name(7) != NULL
              && strcmp(sound_alias_name(7), "SFX_AmbientLoop") == 0,
          "index 7 is SFX_AmbientLoop (finding 137)");
    CHECK(sound_alias_name(0x26) != NULL
              && strcmp(sound_alias_name(0x26), "SFX_Lava") == 0,
          "index 0x26 is SFX_Lava (finding 137)");
    CHECK(sound_alias_name(0x29) != NULL
              && strcmp(sound_alias_name(0x29), "SFX_MusicLoop") == 0,
          "index 0x29 is SFX_MusicLoop (finding 144)");
    CHECK(sound_alias_name(0x2c) != NULL
              && strcmp(sound_alias_name(0x2c), "SFX_AmbientMusicLoop") == 0,
          "index 0x2c is SFX_AmbientMusicLoop (finding 144)");
    CHECK(sound_alias_name(-1) == NULL && sound_alias_name(45) == NULL,
          "name access is bounded to the built-ins");

    /* Lookup: exact and folded (the corpus folds both sides through
     * KeyToAscii; the fold is ASCII-only). */
    int idx = -1;
    CHECK(sound_alias_lookup("SFX_Lava", &idx) == 1 && idx == 0x26,
          "lookup finds a built-in");
    CHECK(sound_alias_lookup("sfx_lava", &idx) == 1 && idx == 0x26,
          "lookup is case-insensitive");
    CHECK(sound_alias_lookup("SFX_MUSICLOOP", &idx) == 1 && idx == 0x29,
          "lookup finds a built-in upper-cased");

    /* Not-found path: no registration by default, nothing appended. */
    idx = 7;
    CHECK(sound_alias_lookup("SFX_Nope", &idx) == 0,
          "unknown alias finds nothing (registration off)");
    CHECK(idx == 7, "failed lookup leaves the index untouched");
    CHECK(sound_alias_lookup(NULL, &idx) == 0, "NULL name finds nothing");
    CHECK(sound_alias_count() == 45, "unknown lookup appends nothing");

    /* Registration set @ 0x00464c50: unknown names append at the tail. */
    sound_alias_registration_set(1);
    CHECK(sound_alias_lookup("SFX_Nope", &idx) == 1 && idx == 45,
          "registration appends the first unknown alias");
    CHECK(sound_alias_lookup("sfx_nope", &idx) == 1 && idx == 45,
          "appended alias re-finds case-insensitively");
    CHECK(sound_alias_count() == 46 && sound_alias_name(45) != NULL
              && strcmp(sound_alias_name(45), "SFX_Nope") == 0,
          "appended alias is the first dynamic slot");

    sound_alias_registration_set(0);
    CHECK(sound_alias_lookup("SFX_Other", &idx) == 0,
          "registration off blocks appending");
    CHECK(sound_alias_count() == 46 && sound_alias_name(46) == NULL,
          "blocked lookup leaves no gap");
    CHECK(sound_alias_lookup("SFX_Nope", &idx) == 1 && idx == 45,
          "already-registered alias survives registration being off");

    /* Re-init drops the dynamic side (documented deviation: the corpus
     * keeps the stale count and reads uninitialised slots) but leaves the
     * registration flag exactly as it is (the corpus never touches
     * DAT_00502a2c here — the disabled flag below would keep lookup from
     * appending until re-enabled). */
    sound_alias_registration_set(1);
    sound_alias_registry_init();
    CHECK(sound_alias_count() == 45 && sound_alias_name(45) == NULL,
          "re-init clears the dynamic aliases");
    CHECK(sound_alias_lookup("SFX_Nope", &idx) == 1 && idx == 45,
          "registration flag survives re-init (corpus behaviour)");

    /* Port bound (deviation: the corpus writes OOB past its 0x7bc-byte
     * table): filling the table makes further appends fail cleanly. */
    char fill[64];
    for (int i = 1; i < 450; ++i) {
        sprintf(fill, "SFX_Fill%d", i);
        CHECK(sound_alias_lookup(fill, &idx) == 1 && idx == 45 + i,
              "dynamic fill reaches the table bound");
    }
    CHECK(sound_alias_count() == SOUND_ALIAS_MAX, "count hits the bound");
    CHECK(sound_alias_lookup("SFX_Fill999", &idx) == 0,
          "appending at the bound returns 0 (port guard)");
    sound_alias_registry_init();
}

/* ------------------------------------------------------------------ *
 *  Subsystem enable state (SoundSubsystemEnableSet @ 0x465260,
 *  EnableSetAndStop @ 0x4652a0, EnabledGet @ 0x465370, BatchModeGet @
 *  0x4652b0 — issues #315/#322/#323/#316).
 * ------------------------------------------------------------------ */

static void test_sound_enable(void)
{
    CHECK(sound_subsystem_enabled() == 0, "subsystem starts disabled (zero word)");
    CHECK(sound_batch_mode_get() == 0, "batch mode starts off");

    sound_subsystem_enable_set(1, 1);
    CHECK(sound_subsystem_enabled() != 0, "enable raises the enabled bit");

    sound_subsystem_enable_set(0, 1);
    CHECK(sound_subsystem_enabled() == 0, "disable clears the enabled bit");

    /* The stop-and-stop variant is enable-set with stop active. */
    sound_subsystem_enable_set_and_stop(1);
    CHECK(sound_subsystem_enabled() != 0, "enable_and_stop enables");
    sound_subsystem_enable_set_and_stop(0);
    CHECK(sound_subsystem_enabled() == 0, "enable_and_stop disables");

    /* Disable zeroes the global-cooldown word (the corpus double at
     * _DAT_00502b2c; its writer lands with the submission queue #296). */
    CHECK(sound_deferred_last_alias() == 0.0, "cooldown word zeroed on disable");

    /* Batch-mode get returns the masked word. */
    CHECK(sound_batch_mode_get() == 0, "batch mode stays off with no queue");

    /* The registration bit and the enable bit are independent. */
    sound_subsystem_enable_set(1, 0);
    sound_alias_registration_set(1);
    CHECK(sound_subsystem_enabled() != 0, "registration flag does not disable");
    sound_subsystem_enable_set(0, 0);
    /* Re-enable and confirm registration survived (shared word, distinct
     * bits) — a live alias lookup still appends while disabled. */
    int idx = -1;
    sound_subsystem_enable_set(1, 0);
    CHECK(sound_alias_lookup("SFX_Lava", &idx) == 1 && idx == 0x26,
          "lookup unaffected by enable state");
    sound_subsystem_enable_set(0, 0);
}

/* ------------------------------------------------------------------ *
 *  Deferred request queue (SoundRequestQueueSubmit @ 0x464f60 — issue
 *  #317).
 * ------------------------------------------------------------------ */

static void test_request_queue(void)
{
    sound_request_queue_clear();
    CHECK(sound_request_queue_count() == 0, "queue starts empty");
    CHECK(sound_request_queue_get(0, NULL, NULL) == 0,
          "empty queue reads nothing");

    int a = -1, l = -1;
    sound_request_queue_submit(0x2c, 1);
    CHECK(sound_request_queue_count() == 1 &&
              sound_request_queue_get(0, &a, &l) == 1 && a == 0x2c && l == 1,
          "submit appends alias + loop flag in order");

    sound_request_queue_submit(0x29, 0);
    sound_request_queue_submit(0x27, 0);
    CHECK(sound_request_queue_count() == 3, "appends grow the count");
    CHECK(sound_request_queue_get(1, &a, &l) == 1 && a == 0x29 && l == 0,
          "second entry keeps its values");
    CHECK(sound_request_queue_get(-1, &a, &l) == 0 &&
              sound_request_queue_get(3, &a, &l) == 0,
          "get is bounded to the pending range");

    /* Drop-on-full: fill to capacity, then one more. The corpus drops
     * silently — count and stored entries stay. */
    for (int i = 3; i < SOUND_REQUEST_QUEUE_CAPACITY; ++i) {
        sound_request_queue_submit(0x10 + i, 0);
    }
    CHECK(sound_request_queue_count() == SOUND_REQUEST_QUEUE_CAPACITY,
          "queue fills to capacity");
    sound_request_queue_submit(0x3f, 1);
    CHECK(sound_request_queue_count() == SOUND_REQUEST_QUEUE_CAPACITY,
          "submission past capacity is dropped");
    CHECK(sound_request_queue_get(SOUND_REQUEST_QUEUE_CAPACITY - 1, &a, &l)
                  == 1 && a == 0x19 && l == 0,
          "last stored entry is the capacity fill, not the dropped submit");

    /* Clear empties the pending set but leaves stored words (the corpus
     * only zeroes the count). */
    sound_request_queue_clear();
    CHECK(sound_request_queue_count() == 0,
          "clear zeroes the count (corpus tick behaviour)");
    CHECK(sound_request_queue_get(0, NULL, NULL) == 0,
          "cleared queue reads nothing");
}

/* ------------------------------------------------------------------ *
 *  Deferred-play cooldown (DeferredSoundTimerSetIfIdle @ 0x464ec0,
 *  DeferredSoundPlayExpire @ 0x464e80 — issues #326 / #327).
 * ------------------------------------------------------------------ */

static void test_deferred_cooldown(void)
{
    /* Words zero-initialise like the corpus; expire is inert until an
     * arming submit records a handle. */
    CHECK(sound_deferred_cooldown() == 0.0f, "cooldown starts idle");
    sound_deferred_play_expire();
    CHECK(sound_deferred_cooldown() == 0.0f, "expire on idle is a no-op");

    /* Idle set arms and clears the recorded handle (corpus writes -1). */
    CHECK(sound_deferred_timer_set_if_idle(10.0f) == 1, "idle set arms");
    CHECK(sound_deferred_cooldown() == 10.0f, "timer takes the value");
    CHECK(sound_deferred_handle() == -1, "arming forgets the handle");
    CHECK(sound_deferred_timer_set_if_idle(5.0f) == 0,
          "running cooldown refuses a re-arm");
    CHECK(sound_deferred_cooldown() == 10.0f, "refused set keeps old timer");

    /* Expire skips while no handle is recorded, even mid-timer. */
    sound_deferred_play_expire();
    CHECK(sound_deferred_cooldown() == 10.0f && sound_deferred_handle() == -1,
          "expire skips with the handle at -1");

    /* A recorded handle with the timer running expires: stop seam,
     * handle back to -1, timer zeroed. */
    sound_deferred_handle_record(7);
    sound_deferred_play_expire();
    CHECK(sound_deferred_handle() == -1, "expire forgets the handle");
    CHECK(sound_deferred_cooldown() == 0.0f, "expire zeroes the timer");

    /* Recorded but drained (timer not > 0): no expiry. */
    sound_deferred_handle_record(9);
    sound_deferred_play_expire();
    CHECK(sound_deferred_handle() == 9, "expire skips on a zero timer");

    /* Idle again, so the next set arms. */
    sound_deferred_handle_record(-1);
    CHECK(sound_deferred_timer_set_if_idle(2.0f) == 1, "re-arms after drain");
    CHECK(sound_deferred_timer_set_if_idle(0.0f) == 0,
          "zero timer counts as running once armed");
}

/* ------------------------------------------------------------------ *
 *  Bank record lifecycle (SoundBankEntryLoad @ 0x47a870,
 *  SoundBankFreeSlotFind @ 0x47aad0, SoundBankEntryRelease @ 0x47ab00,
 *  SoundVolumeValueValid @ 0x47a850; finding 14 §1).
 * ------------------------------------------------------------------ */

static void test_bank_entries(void)
{
    sound_bank_clear();
    sound_bank_sound_on_set(1);

    /* Millibel validator boundaries (valid while -10001 < v < 1). */
    CHECK(sound_volume_value_valid(0) == 1, "volume 0 valid");
    CHECK(sound_volume_value_valid(-1) == 1, "volume -1 valid");
    CHECK(sound_volume_value_valid(-10000) == 1, "volume -10000 valid");
    CHECK(sound_volume_value_valid(-10001) == 0, "volume -10001 invalid");
    CHECK(sound_volume_value_valid(1) == 0, "volume 1 invalid");

    int slot = -1;
    CHECK(sound_bank_free_slot_find(&slot) == 1 && slot == 0,
          "free-slot scan starts at 0");
    CHECK(sound_bank_free_slot_find(NULL) == 0, "null out fails the scan");
    CHECK(sound_bank_entry_flags(-1) == 0 && sound_bank_entry_volume(-1) == 0,
          "record accessors are bounded");

    /* Entry load needs a file the resource layer can open: write a
     * minimal WAV where the loader path reaches it. */
    static const char* base = "/tmp/opencode/sndbe_entry";
    uint8_t pcm[8] = {1, 0, 2, 0, 3, 0, 4, 0};
    WavSeg segs[2];
    segs[0].id = NULL; segs[0].bytes = NULL; segs[0].len = 0;
    segs[1].id = "DATA!"; segs[1].bytes = NULL; segs[1].len = 0;
    size_t wsz = 0;
    uint8_t* w = build_wav(1, 16, 11025, pcm, sizeof(pcm), segs, 2, NULL, 0,
                           &wsz);
    CHECK(w != NULL, "entry test wav built");
    char path[512];
    snprintf(path, sizeof(path), "%s.wav", base);
    FILE* fp = fopen(path, "wb");
    if (w == NULL || fp == NULL) {
        printf("SKIP bank-entry: wav write failed\n");
        free(w);
        return;
    }
    fwrite(w, 1, wsz, fp);
    fclose(fp);
    free(w);

    int s0 = sound_bank_entry_load(base, 0 /* memory */, 1 /* loop */, -500);
    CHECK(s0 == 0, "entry load claims the free slot");
    CHECK(sound_bank_entry_flags(s0)
              == (SOUND_SLOT_BUSY | SOUND_SLOT_LOADED | SOUND_LOOPING),
          "memory+loop flags: busy, loaded, loop bit");
    CHECK(sound_bank_entry_volume(s0) == -500, "valid volume stored");
    CHECK(sound_bank_count() == 1, "busy slot counted");
    Sound* f = sound_bank_find(base);
    CHECK(f != NULL && (f->flags & SOUND_LOOPING) != 0,
          "find sees the loaded looping record");

    /* No dedupe in the corpus loader: a second load claims slot 1. */
    int s1 = sound_bank_entry_load(base, 1 /* streamed */, 0, 5);
    CHECK(s1 == 1, "second load claims the next slot");
    CHECK(sound_bank_entry_flags(s1)
              == (SOUND_SLOT_BUSY | SOUND_SLOT_LOADED | SOUND_STREAMED),
          "streamed flags carry the stream bit");
    CHECK(sound_bank_entry_volume(s1) == 0, "invalid volume stored as 0");
    CHECK(sound_bank_count() == 2, "two records counted");

    /* Release frees the slot for reuse and drops the payload. */
    sound_bank_entry_release(s0);
    CHECK(sound_bank_count() == 1 && sound_bank_free_slot_find(&slot) == 1
              && slot == s0,
          "release clears busy and the slot comes back");
    sound_bank_entry_release(-1); /* no-op by the corpus's param guard */

    /* Failure path: missing file claims and releases nothing. */
    CHECK(sound_bank_entry_load("/tmp/opencode/sndbe_missing", 0, 0, 0) == -1,
          "missing file fails the entry load");
    CHECK(sound_bank_count() == 1, "failed load registers no record");

    /* Sound-off gate: loads refuse while the subsystem switch is off. */
    sound_bank_sound_on_set(0);
    CHECK(sound_bank_entry_load(base, 0, 0, 0) == -1, "sound-off blocks load");
    sound_bank_sound_on_set(1);

    /* Port convenience keeps bank_load deduped. */
    CHECK(sound_bank_load(base, 0, 0) == 0 && sound_bank_count() == 1,
          "bank_load dedupes before claiming (port layer)");
    CHECK(sound_bank_load("/tmp/opencode/sndbe_missing", 0, 0) == -1,
          "bank_load fails on a missing file");

    sound_bank_clear();
    remove(path);
    CHECK(sound_bank_count() == 0, "clear empties the bank");
}

/* ------------------------------------------------------------------ *
 *  Configured variant chains (SoundSampleMarkerParse @ 0x464d00,
 *  SoundSampleVariantSelect @ 0x464e20 — issues #318 / #324;
 *  findings 107/134/135/142).
 * ------------------------------------------------------------------ */

/* Mirror of the port's module-local 15-bit draw (corpus
 * thunk_FUN_0048e170, modelled as in level.cpp). Advance only before a
 * select that runs on a multi-link chain: single-link selects skip the
 * draw entirely, like the corpus. */
static uint32_t s_variant_rand_mirror = 1;

static int test_variant_draw(void)
{
    s_variant_rand_mirror = s_variant_rand_mirror * 214013u + 2531011u;
    return (int)((s_variant_rand_mirror >> 16) & 0x7fffu);
}

static void test_variant_chains(void)
{
    sound_bank_clear();
    sound_bank_sound_on_set(1);

    uint8_t pcm[8] = { 1, 0, 2, 0, 3, 0, 4, 0 };
    WavSeg segs[2];
    segs[0].id = NULL;    segs[0].bytes = NULL; segs[0].len = 0;
    segs[1].id = "DATA!"; segs[1].bytes = NULL; segs[1].len = 0;
    size_t wsz = 0;
    uint8_t* w = build_wav(1, 16, 11025, pcm, (uint32_t)sizeof(pcm),
                           segs, 2, NULL, 0, &wsz);
    if (w == NULL) {
        printf("SKIP variant-chains: wav build failed\n");
        return;
    }
    const char* files[3] = { "/tmp/opencode/svar_a.wav",
                             "/tmp/opencode/svar_b.wav",
                             "/tmp/opencode/svar_c.wav" };
    char pathA[256], pathB[256], pathC[256];
    snprintf(pathA, sizeof pathA, "%s", "/tmp/opencode/svar_a");
    snprintf(pathB, sizeof pathB, "%s", "/tmp/opencode/svar_b");
    snprintf(pathC, sizeof pathC, "%s", "/tmp/opencode/svar_c");
    int ok_files = 1;
    for (int i = 0; i < 3; ++i) {
        FILE* fp = fopen(files[i], "wb");
        if (fp == NULL) {
            ok_files = 0;
            break;
        }
        fwrite(w, 1, wsz, fp);
        fclose(fp);
    }
    free(w);
    if (!ok_files) {
        printf("SKIP variant-chains: wav write failed\n");
        return;
    }

    char spec[1600];
    int out[16];
    const int alias1 = 1; /* chain keys are alias indices; 1 is the first
                           * real alias (0 is the corpus's silent alias) */

    CHECK(sound_sample_marker_parse(NULL, alias1) == 0,
          "null spec parses nothing (the corpus deref-faults)");
    CHECK(sound_variant_select(0) == -1, "the silent alias selects -1");
    CHECK(sound_variant_select(-1) == -1 &&
              sound_variant_chain(-1, out, 16) == 0,
          "chain accessors are bounded");

    /* Two-path build: the second load overwrites the head slot (the
     * corpus's quirk) and links one fresh node holding stale data. */
    snprintf(spec, sizeof spec, "%s,%s", pathA, pathB);
    CHECK(sound_sample_marker_parse(spec, alias1) == 1,
          "two-path parse succeeds");
    CHECK(sound_bank_count() == 2, "both paths claimed bank slots");
    int n = sound_variant_chain(alias1, out, 16);
    CHECK(n == 2 && out[0] == 1 && out[1] == 0,
          "head keeps the second load; node holds stale 0");

    /* First draw: 41 -> pick 1 of 2 -> swap node to the head. */
    int h = 1, node = 0;
    int pick = test_variant_draw() % 2;
    if (pick != 0) {
        int t = node; node = h; h = t;
    }
    CHECK(sound_variant_select(alias1) == h, "select returns the new head");
    n = sound_variant_chain(alias1, out, 16);
    CHECK(n == 2 && out[0] == h && out[1] == node,
          "the pick swapped into the head slot");

    /* Second draw: swaps only when it picks the node. */
    h = out[0];
    node = out[1];
    pick = test_variant_draw() % 2;
    if (pick != 0) {
        int t = node; node = h; h = t;
    }
    CHECK(sound_variant_select(alias1) == h, "second draw keeps-or-swaps");
    n = sound_variant_chain(alias1, out, 16);
    CHECK(n == 2 && out[0] == h && out[1] == node,
          "second draw leaves the chain consistent");

    /* Markers: '*' duplicate-buffer flag, '#N#' volume, '@' stream mode
     * (and the stream marker persists across later paths, finding 135). */
    sound_bank_clear();
    const int alias2 = 2;
    snprintf(spec, sizeof spec, "*%s,#-500#%s,@%s,%s", pathA, pathB,
             pathC, pathA);
    CHECK(sound_sample_marker_parse(spec, alias2) == 1,
          "marker spec parses");
    CHECK(sound_bank_count() == 4, "four paths claimed slots");
    CHECK((sound_bank_entry_flags(0) & SOUND_LOOPING) != 0 &&
              (sound_bank_entry_flags(0) & SOUND_STREAMED) == 0,
          "* raises only the duplicate-buffer (loop) bit");
    CHECK(sound_bank_entry_volume(1) == -500, "#N# stores the millibel volume");
    CHECK((sound_bank_entry_flags(2) & SOUND_STREAMED) != 0,
          "@ loads its path streamed");
    CHECK((sound_bank_entry_flags(3) & SOUND_STREAMED) != 0,
          "the stream marker persists across later paths (finding 135)");
    n = sound_variant_chain(alias2, out, 16);
    CHECK(n == 4 && out[0] == 1 && out[1] == 2 && out[2] == 3,
          "chain: head is the second load, nodes follow in parse order");
    CHECK(out[3] == 0, "the last link stays unwritten (stale pool data)");

    /* Out-of-range volume validates to 0, like the record layer. */
    sound_bank_clear();
    const int alias3 = 3;
    snprintf(spec, sizeof spec, "#-10001#%s", pathA);
    CHECK(sound_sample_marker_parse(spec, alias3) == 1,
          "invalid volume still loads the path");
    CHECK(sound_bank_entry_volume(0) == 0, "invalid volume stores 0");

    /* Failed load: the parse reports 0 and the failure link stays
     * linked as -1 (its slot word gets overwritten, the node does not). */
    sound_bank_clear();
    const int alias4 = 4;
    snprintf(spec, sizeof spec, "%s,%s,/tmp/opencode/svar_missing", pathA,
             pathB);
    CHECK(sound_sample_marker_parse(spec, alias4) == 0,
          "failed load drops the parse result");
    n = sound_variant_chain(alias4, out, 16);
    CHECK(n == 2 && out[0] == 1 && out[1] == -1,
          "failed link stays linked as -1 (corpus-faithful)");
    h = 1;
    node = -1;
    pick = test_variant_draw() % 2;
    if (pick != 0) {
        int t = node; node = h; h = t;
    }
    CHECK(sound_variant_select(alias4) == h,
          "select can surface the failed link's -1");
    if (h != -1) { /* swap put -1 into the node; keep the chain readable */
        n = sound_variant_chain(alias4, out, 16);
        CHECK(n == 2 && out[1] == -1 && out[0] == 1,
              "the -1 link survives swaps");
    }

    /* Single-link chain: select returns the head and draws nothing. */
    sound_bank_clear();
    const int alias5 = 5;
    snprintf(spec, sizeof spec, "%s", pathC);
    CHECK(sound_sample_marker_parse(spec, alias5) == 1, "single-path parse");
    CHECK(sound_variant_chain(alias5, NULL, 16) == 1,
          "null slots reports the count");
    CHECK(sound_variant_select(alias5) == 0,
          "single-link select returns the head without drawing");

    /* Split quirks: a trailing comma stays in the path (the corpus never
     * tests the final character as a separator), and an empty spec
     * succeeds while leaving the chain unlinked. */
    const int alias6 = 6;
    snprintf(spec, sizeof spec, "%s,", pathA);
    CHECK(sound_sample_marker_parse(spec, alias6) == 0,
          "trailing comma stays part of the path (split quirk)");
    CHECK(sound_variant_chain(alias6, NULL, 16) == 1,
          "the comma-path load failed with nothing linked");
    snprintf(spec, sizeof spec, "%s,%s", pathA, pathB);
    CHECK(sound_sample_marker_parse(spec, alias6) == 1, "reparse rebuilds");
    CHECK(sound_variant_chain(alias6, NULL, 16) == 2,
          "rebuild links fresh nodes (stale old nodes orphaned)");
    CHECK(sound_sample_marker_parse("", alias6) == 1,
          "empty spec succeeds with the chain unlinked");

    sound_bank_clear();
    for (int i = 0; i < 3; ++i) {
        remove(files[i]);
    }
}

/* ------------------------------------------------------------------ *
 *  Configured variant property pair (ConfiguredVariantPropertySet @
 *  0x465010 / ConfiguredVariantPropertyRead @ 0x465030 — issues
 *  #319 / #320) plus the bank volume setter (0x47b050).
 * ------------------------------------------------------------------ */

static void test_variant_properties(void)
{
    sound_bank_clear();

    uint8_t pcm[8] = { 1, 0, 2, 0, 3, 0, 4, 0 };
    WavSeg segs[2];
    segs[0].id = NULL;    segs[0].bytes = NULL; segs[0].len = 0;
    segs[1].id = "DATA!"; segs[1].bytes = NULL; segs[1].len = 0;
    size_t wsz = 0;
    uint8_t* w = build_wav(1, 16, 11025, pcm, (uint32_t)sizeof(pcm),
                           segs, 2, NULL, 0, &wsz);
    if (w == NULL) {
        printf("SKIP variant-properties: wav build failed\n");
        return;
    }
    const char* files[3] = { "/tmp/opencode/svp_a.wav",
                             "/tmp/opencode/svp_b.wav" };
    int ok_files = 1;
    for (int i = 0; i < 2; ++i) {
        FILE* fp = fopen(files[i], "wb");
        if (fp == NULL) {
            ok_files = 0;
            break;
        }
        fwrite(w, 1, wsz, fp);
        fclose(fp);
    }
    free(w);
    if (!ok_files) {
        printf("SKIP variant-properties: wav write failed\n");
        return;
    }

    const int alias = 7;
    char spec[1600];
    snprintf(spec, sizeof spec, "%s,%s", "/tmp/opencode/svp_a",
             "/tmp/opencode/svp_b");
    CHECK(sound_sample_marker_parse(spec, alias) == 1,
          "property pair chain parses");
    int out[16];
    CHECK(sound_variant_chain(alias, out, 16) == 2 && out[0] == 1,
          "head starts at the second load");

    /* Property read: draws and swaps like the selector, changes nothing
     * else (the corpus discards the volume word). */
    int h = out[0], nd = out[1];
    int pick = test_variant_draw() % 2;
    if (pick != 0) {
        int t = nd; nd = h; h = t;
    }
    sound_variant_property_read(alias);
    CHECK(sound_variant_chain(alias, out, 16) == 2 && out[0] == h
              && out[1] == nd,
          "property read rotates the chain only");

    /* Property set: volume lands on the record after the draw. */
    h = out[0];
    nd = out[1];
    pick = test_variant_draw() % 2;
    if (pick != 0) {
        int t = nd; nd = h; h = t;
    }
    sound_variant_property_set(alias, -777);
    CHECK(sound_bank_entry_volume(h) == -777,
          "property set stores the millibels in the head record");
    CHECK(sound_bank_entry_volume(nd) == 0,
          "the alternate record keeps its volume");

    /* Invalid value stores 0 through the same route. */
    h = out[0];
    nd = out[1];
    pick = test_variant_draw() % 2;
    if (pick != 0) {
        int t = nd; nd = h; h = t;
    }
    sound_variant_property_set(alias, 99999);
    CHECK(sound_bank_entry_volume(h) == 0,
          "invalid property volume stores 0 on the drawn head");

    /* Direct setter: validator boundaries and slot bounds. */
    sound_bank_entry_volume_set(0, -10001);
    CHECK(sound_bank_entry_volume(0) == 0, "volume set -10001 stores 0");
    sound_bank_entry_volume_set(0, -10000);
    CHECK(sound_bank_entry_volume(0) == -10000, "volume set -10000 valid");
    sound_bank_entry_volume_set(1, 1);
    CHECK(sound_bank_entry_volume(1) == 0, "volume set 1 stores 0");
    sound_bank_entry_volume_set(1, -7);
    sound_bank_entry_volume_set(-1, 123);          /* guarded */
    sound_bank_entry_volume_set(SOUND_BANK_SLOTS, 123);
    CHECK(sound_bank_entry_volume(1) == -7, "out-of-range writes are guarded");

    /* Silent alias: select answers -1 and no record is touched. */
    sound_bank_entry_volume_set(0, -42);
    sound_variant_property_set(0, -5);
    sound_variant_property_read(0);
    CHECK(sound_bank_entry_volume(0) == -42,
          "silent alias property calls change nothing");

    sound_bank_clear();
    for (int i = 0; i < 2; ++i) {
        remove(files[i]);
    }
}

int main(int argc, char** argv)
{
    test_synthetic();
    test_alias_registry();
    test_sound_enable();
    test_request_queue();
    test_deferred_cooldown();
    test_bank_entries();
    test_variant_chains();
    test_variant_properties();
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--real") == 0 && i + 1 < argc) {
            test_real(argv[i + 1]);
            break;
        }
    }
    /* SoundBufferAssociationAdd (2026-10-24): the association prepend. */
    {
        void* saved = sound_buffer_association_head_peek();
        sound_buffer_association_head_set(NULL);
        sound_buffer_association_add(1u, 2u, 3u);
        void* h1 = sound_buffer_association_head_peek();
        CHECK(h1 != NULL, "the first association is the head");
        char* w = (char*)h1;
        void* w0 = NULL; void* w1 = NULL; void* w2 = NULL; void* link = (void*)1;
        __builtin_memcpy(&w0, w + 0 * sizeof(void*), sizeof w0);
        __builtin_memcpy(&w1, w + 1 * sizeof(void*), sizeof w1);
        __builtin_memcpy(&w2, w + 2 * sizeof(void*), sizeof w2);
        __builtin_memcpy(&link, w + 3 * sizeof(void*), sizeof link);
        CHECK(w0 == (void*)(uintptr_t)1 && w1 == (void*)(uintptr_t)2
                  && w2 == (void*)(uintptr_t)3,
              "the value words store in order");
        CHECK(link == NULL, "the empty head links NULL");
        sound_buffer_association_add(4u, 5u, 6u);
        void* h2 = sound_buffer_association_head_peek();
        __builtin_memcpy(&link, (char*)h2 + 3 * sizeof(void*), sizeof link);
        CHECK(link == h1, "the second node prepends through the prior head");
        engine_heap_free(h2);
        engine_heap_free(h1);
        sound_buffer_association_head_set(saved);
    }

    if (s_failures == 0) {
        printf("sound_test: PASS%s\n", argc > 2 ? " (real wads)" : "");
        return 0;
    }
    printf("sound_test: %d failure(s)\n", s_failures);
    return 1;
}