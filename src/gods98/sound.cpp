// SPDX-License-Identifier: LGPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resource.hpp"
#include "sound.hpp"

static const char* s_bank_names[SOUND_BANK_SLOTS];
static Sound*      s_bank_sounds[SOUND_BANK_SLOTS];
static int         s_bank_count = 0;

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

    if (sound_bank_find(full) != NULL) {
        return 0; /* already loaded */
    }
    if (s_bank_count >= SOUND_BANK_SLOTS) {
        return -1;
    }

    Sound* s = sound_load(full);
    if (s == NULL) {
        return -1;
    }
    s->flags = (streamed ? SOUND_STREAMED : 0)
             | (looping ? SOUND_LOOPING : 0);

    s_bank_names[s_bank_count] = s->name;
    s_bank_sounds[s_bank_count] = s;
    ++s_bank_count;
    return 0;
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
    for (int i = 0; i < s_bank_count; ++i) {
        if (s_strcasecmp(s_bank_names[i], fullname) == 0) {
            return s_bank_sounds[i];
        }
    }
    return NULL;
}

int sound_bank_count(void)
{
    return s_bank_count;
}

void sound_bank_clear(void)
{
    for (int i = 0; i < s_bank_count; ++i) {
        sound_free(s_bank_sounds[i]);
    }
    s_bank_count = 0;
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
