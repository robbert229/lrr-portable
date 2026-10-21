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

int main(int argc, char** argv)
{
    test_synthetic();
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--real") == 0 && i + 1 < argc) {
            test_real(argv[i + 1]);
            break;
        }
    }
    if (s_failures == 0) {
        printf("sound_test: PASS%s\n", argc > 2 ? " (real wads)" : "");
        return 0;
    }
    printf("sound_test: %d failure(s)\n", s_failures);
    return 1;
}