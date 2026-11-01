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

/* Read the fixed WAVEFORMATEX prefix without copying the data chunk. The
 * shipped routines only consume the first 16 bytes of `fmt `; extra format
 * bytes are skipped by mmio, while a short fmt chunk is rejected. */
static uint32_t sound_wave_format_find(const uint8_t* data, uint32_t size,
                                      WavSample* format_out)
{
    uint32_t pos, riff_end;
    if (format_out == NULL) return 0xe101u;
    memset(format_out, 0, sizeof(*format_out));
    if (data == NULL || size < 12u) return 0xe101u;
    if (data[0] != 'R' || data[1] != 'I' || data[2] != 'F' ||
        data[3] != 'F' || data[8] != 'W' || data[9] != 'A' ||
        data[10] != 'V' || data[11] != 'E') return 0xe101u;

    riff_end = s_le32(data + 4u) + 8u;
    if (riff_end < 12u || riff_end > size) return 0xe102u;
    pos = 12u;
    while (pos <= riff_end && riff_end - pos >= 8u) {
        uint32_t chunk_size = s_le32(data + pos + 4u);
        uint32_t body = pos + 8u;
        uint32_t padded_size;
        if (chunk_size > riff_end - body) return 0xe102u;
        padded_size = chunk_size + (chunk_size & 1u);
        if (padded_size < chunk_size || padded_size > riff_end - body)
            return 0xe102u;
        if (data[pos] == 'f' && data[pos + 1] == 'm' &&
            data[pos + 2] == 't' && data[pos + 3] == ' ') {
            if (chunk_size < 16u) return 0xe101u;
            format_out->format_tag = s_le16(data + body);
            format_out->channels = s_le16(data + body + 2u);
            format_out->sample_rate = s_le32(data + body + 4u);
            format_out->byte_rate = s_le32(data + body + 8u);
            format_out->block_align = s_le16(data + body + 12u);
            format_out->bits_per_sample = s_le16(data + body + 14u);
            return 0u;
        }
        pos = body + padded_size;
    }
    return 0xe101u;
}

struct SoundWaveMemoryStream {
    const uint8_t* memory;
    uint32_t length;
    uint32_t cursor;
};

struct SoundWaveFileHandle {
    Resource* resource;
    SoundWaveMemoryStream* stream;
    uint32_t data_offset;
    uint32_t data_size;
};

// FUNCTION: LEGORR 0x00488c70
uint32_t sound_wave_memory_format_read(const uint8_t* data, int32_t length,
                                       SoundWaveMemoryStream** stream_out,
                                       uint8_t** format_out,
                                       SoundWaveChunk* parent_chunk_out)
{
    uint32_t size, riff_end, pos;
    SoundWaveMemoryStream* stream;
    uint8_t* format;
    if (stream_out == NULL || format_out == NULL || parent_chunk_out == NULL)
        return 0xe101u;
    *stream_out = NULL;
    *format_out = NULL;
    memset(parent_chunk_out, 0, sizeof(*parent_chunk_out));
    if (data == NULL || length < 0) return 0xe100u;
    size = (uint32_t)length;
    if (size < 12u) return 0xe101u;
    if (data[0] != 'R' || data[1] != 'I' || data[2] != 'F' ||
        data[3] != 'F' || data[8] != 'W' || data[9] != 'A' ||
        data[10] != 'V' || data[11] != 'E') return 0xe101u;
    riff_end = s_le32(data + 4u) + 8u;
    if (riff_end < 12u || riff_end > size) return 0xe102u;

    pos = 12u;
    while (pos <= riff_end && riff_end - pos >= 8u) {
        uint32_t chunk_size = s_le32(data + pos + 4u);
        uint32_t body = pos + 8u;
        uint32_t padded_size;
        if (chunk_size > riff_end - body) return 0xe102u;
        padded_size = chunk_size + (chunk_size & 1u);
        if (padded_size < chunk_size || padded_size > riff_end - body)
            return 0xe102u;
        if (data[pos] == 'f' && data[pos + 1] == 'm' &&
            data[pos + 2] == 't' && data[pos + 3] == ' ') {
            uint16_t extra_size = 0;
            uint32_t format_size;
            if (chunk_size < 16u) return 0xe101u;
            if (s_le16(data + body) != 1u) {
                if (chunk_size < 18u) return 0xe102u;
                extra_size = s_le16(data + body + 16u);
                if ((uint32_t)extra_size > chunk_size - 18u)
                    return 0xe102u;
            }
            format_size = 18u + (uint32_t)extra_size;
            format = (uint8_t*)calloc(format_size, 1u);
            if (format == NULL) return 0xe000u;
            memcpy(format, data + body, 16u);
            format[16] = (uint8_t)(extra_size & 0xffu);
            format[17] = (uint8_t)(extra_size >> 8);
            if (extra_size != 0u)
                memcpy(format + 18u, data + body + 18u, extra_size);
            stream = (SoundWaveMemoryStream*)malloc(sizeof(*stream));
            if (stream == NULL) {
                free(format);
                return 0xe000u;
            }
            stream->memory = data;
            stream->length = size;
            stream->cursor = body + padded_size;
            parent_chunk_out->chunk_id = 0x46464952u;
            parent_chunk_out->chunk_size = riff_end - 8u;
            parent_chunk_out->form_type = 0x45564157u;
            parent_chunk_out->data_offset = 8u;
            parent_chunk_out->flags = 0u;
            *format_out = format;
            *stream_out = stream;
            return 0u;
        }
        pos = body + padded_size;
    }
    return 0xe101u;
}

uint32_t sound_wave_memory_stream_read(SoundWaveMemoryStream* stream,
                                      void* destination, uint32_t requested,
                                      uint32_t* bytes_read)
{
    uint32_t amount;
    if (bytes_read != NULL) *bytes_read = 0u;
    if (stream == NULL || (destination == NULL && requested != 0u))
        return 0xe103u;
    if (stream->cursor > stream->length || stream->memory == NULL)
        return 0xe103u;
    amount = requested;
    if (amount > stream->length - stream->cursor)
        amount = stream->length - stream->cursor;
    if (amount == 0u && requested != 0u) return 0xe103u;
    if (amount != 0u) {
        memcpy(destination, stream->memory + stream->cursor, amount);
        stream->cursor += amount;
    }
    if (bytes_read != NULL) *bytes_read = amount;
    return 0u;
}

void sound_wave_memory_format_close(SoundWaveMemoryStream** stream,
                                    uint8_t** format)
{
    if (format != NULL && *format != NULL) {
        free(*format);
        *format = NULL;
    }
    if (stream != NULL && *stream != NULL) {
        free(*stream);
        *stream = NULL;
    }
}

// FUNCTION: LEGORR 0x00488e70
uint32_t sound_wave_file_average_bytes_per_second_read(const char* path)
{
    Resource* resource;
    WavSample format;
    uint32_t result = 0u;
    if (path == NULL) return 0u;
    resource = resource_open(path, RESOURCE_STD_ONLY);
    if (resource == NULL) return 0u;
    if (resource_read(resource) >= 0 &&
        sound_wave_format_find((const uint8_t*)resource->data,
                               (uint32_t)resource->size, &format) == 0u) {
        result = format.byte_rate;
    }
    resource_free(resource);
    return result;
}

// FUNCTION: LEGORR 0x00488f10
uint32_t sound_wave_file_open_read_chunks(const char* path,
                                          SoundWaveFileHandle** handle_out,
                                          uint8_t** format_out,
                                          SoundWaveChunk* parent_chunk_out)
{
    Resource* resource;
    SoundWaveMemoryStream* stream;
    SoundWaveFileHandle* handle;
    uint8_t* format;
    const uint8_t* data;
    uint32_t size, riff_end, pos, chunk_size, body, padded_size;
    uint32_t format_size;
    uint16_t extra_size;
    uint32_t result;
    if (handle_out == NULL || format_out == NULL || parent_chunk_out == NULL)
        return 0xe101u;
    *handle_out = NULL;
    *format_out = NULL;
    memset(parent_chunk_out, 0, sizeof(*parent_chunk_out));
    if (path == NULL) return 0xe100u;
    resource = resource_open(path, RESOURCE_STD_ONLY);
    if (resource == NULL) return 0xe100u;
    if (resource_read(resource) < 0 || resource->size < 0 ||
        (uint32_t)resource->size > 0x7fffffffu) {
        resource_free(resource);
        return 0xe102u;
    }
    data = (const uint8_t*)resource->data;
    size = (uint32_t)resource->size;
    if (data == NULL || size < 12u || data[0] != 'R' || data[1] != 'I' ||
        data[2] != 'F' || data[3] != 'F') {
        resource_free(resource);
        return 0xe101u;
    }
    riff_end = s_le32(data + 4u) + 8u;
    if (riff_end < 12u || riff_end > size || data[8] != 'W' ||
        data[9] != 'A' || data[10] != 'V' || data[11] != 'E') {
        resource_free(resource);
        return 0xe101u;
    }
    parent_chunk_out->chunk_id = 0x46464952u;
    parent_chunk_out->chunk_size = riff_end - 8u;
    parent_chunk_out->form_type = 0x45564157u;
    parent_chunk_out->data_offset = 8u;
    parent_chunk_out->flags = 0u;

    pos = 12u;
    while (riff_end - pos >= 8u) {
        chunk_size = s_le32(data + pos + 4u);
        body = pos + 8u;
        if (chunk_size > riff_end - body) {
            memset(parent_chunk_out, 0, sizeof(*parent_chunk_out));
            resource_free(resource);
            return 0xe102u;
        }
        padded_size = chunk_size + (chunk_size & 1u);
        if (padded_size < chunk_size || padded_size > riff_end - body) {
            memset(parent_chunk_out, 0, sizeof(*parent_chunk_out));
            resource_free(resource);
            return 0xe102u;
        }
        if (data[pos] == 'f' && data[pos + 1u] == 'm' &&
            data[pos + 2u] == 't' && data[pos + 3u] == ' ') {
            if (chunk_size <= 15u) {
                memset(parent_chunk_out, 0, sizeof(*parent_chunk_out));
                resource_free(resource);
                return 0xe101u;
            }
            extra_size = 0u;
            if (s_le16(data + body) != 1u) {
                if (chunk_size < 18u) {
                    memset(parent_chunk_out, 0, sizeof(*parent_chunk_out));
                    resource_free(resource);
                    return 0xe102u;
                }
                extra_size = s_le16(data + body + 16u);
                if ((uint32_t)extra_size > chunk_size - 18u) {
                    memset(parent_chunk_out, 0, sizeof(*parent_chunk_out));
                    resource_free(resource);
                    return 0xe102u;
                }
            }
            format_size = (uint32_t)extra_size + 18u;
            format = (uint8_t*)calloc(format_size, 1u);
            if (format == NULL) {
                memset(parent_chunk_out, 0, sizeof(*parent_chunk_out));
                resource_free(resource);
                return 0xe000u;
            }
            memcpy(format, data + body, 16u);
            format[16] = (uint8_t)(extra_size & 0xffu);
            format[17] = (uint8_t)(extra_size >> 8);
            if (extra_size != 0u)
                memcpy(format + 18u, data + body + 18u, extra_size);

            stream = (SoundWaveMemoryStream*)malloc(sizeof(*stream));
            if (stream == NULL) {
                free(format);
                memset(parent_chunk_out, 0, sizeof(*parent_chunk_out));
                resource_free(resource);
                return 0xe000u;
            }
            stream->memory = data;
            stream->length = size;
            stream->cursor = body + padded_size;

            handle = (SoundWaveFileHandle*)calloc(1, sizeof(*handle));
            if (handle == NULL) {
                sound_wave_memory_format_close(&stream, &format);
                memset(parent_chunk_out, 0, sizeof(*parent_chunk_out));
                resource_free(resource);
                return 0xe000u;
            }
            handle->resource = resource;
            handle->stream = stream;
            *handle_out = handle;
            *format_out = format;
            return 0u;
        }
        pos = body + padded_size;
    }
    memset(parent_chunk_out, 0, sizeof(*parent_chunk_out));
    resource_free(resource);
    result = 0xe101u;
    return result;
}

// FUNCTION: LEGORR 0x004890c0
void sound_wave_data_chunk_open(SoundWaveFileHandle** handle_inout,
                                SoundWaveChunk* data_chunk_out,
                                SoundWaveChunk* parent_chunk)
{
    SoundWaveFileHandle* handle;
    const uint8_t* data;
    uint32_t size, riff_end, pos;
    if (data_chunk_out == NULL) return;
    memset(data_chunk_out, 0, sizeof(*data_chunk_out));
    if (handle_inout == NULL || *handle_inout == NULL || parent_chunk == NULL)
        return;
    handle = *handle_inout;
    if (handle->resource == NULL || handle->resource->data == NULL ||
        handle->stream == NULL) return;
    data = (const uint8_t*)handle->resource->data;
    if (handle->resource->size < 12) return;
    size = (uint32_t)handle->resource->size;
    riff_end = s_le32(data + 4u) + 8u;
    if (riff_end < 12u || riff_end > size) return;
    if (parent_chunk->chunk_id != 0x46464952u ||
        parent_chunk->form_type != 0x45564157u ||
        parent_chunk->data_offset > 0xfffffffbu) return;
    pos = parent_chunk->data_offset + 4u;
    if (pos < 12u || pos > riff_end) return;
    while (riff_end - pos >= 8u) {
        uint32_t chunk_size = s_le32(data + pos + 4u);
        uint32_t body = pos + 8u;
        uint32_t padded_size;
        if (chunk_size > riff_end - body) return;
        padded_size = chunk_size + (chunk_size & 1u);
        if (padded_size < chunk_size || padded_size > riff_end - body)
            return;
        if (data[pos] == 'd' && data[pos + 1u] == 'a' &&
            data[pos + 2u] == 't' && data[pos + 3u] == 'a') {
            data_chunk_out->chunk_id = 0x61746164u;
            data_chunk_out->chunk_size = chunk_size;
            data_chunk_out->form_type = 0u;
            data_chunk_out->data_offset = body;
            data_chunk_out->flags = 0u;
            handle->data_offset = body;
            handle->data_size = chunk_size;
            handle->stream->cursor = body;
            handle->stream->length = body + chunk_size;
            return;
        }
        pos = body + padded_size;
    }
}

void sound_wave_file_handle_close(SoundWaveFileHandle** handle_inout,
                                  uint8_t** format_inout)
{
    SoundWaveFileHandle* handle;
    if (format_inout != NULL && *format_inout != NULL) {
        free(*format_inout);
        *format_inout = NULL;
    }
    if (handle_inout == NULL || *handle_inout == NULL) return;
    handle = *handle_inout;
    sound_wave_memory_format_close(&handle->stream, NULL);
    resource_free(handle->resource);
    handle->resource = NULL;
    free(handle);
    *handle_inout = NULL;
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

// FUNCTION: LEGORR 0x00489100
uint32_t sound_wave_data_read(SoundWaveFileHandle* handle,
                              uint32_t requested, void* destination,
                              SoundWaveChunk* data_chunk,
                              uint32_t* bytes_read)
{
    uint32_t amount;
    uint32_t remaining;
    if (bytes_read != NULL) *bytes_read = 0u;
    if (handle == NULL || handle->stream == NULL || data_chunk == NULL ||
        (destination == NULL && requested != 0u)) return 0xe103u;

    remaining = data_chunk->chunk_size;
    amount = requested;
    if (remaining < amount) amount = remaining;
    data_chunk->chunk_size = remaining - amount;

    if (handle->stream->memory == NULL ||
        handle->stream->cursor > handle->stream->length ||
        amount > handle->stream->length - handle->stream->cursor ||
        (amount != 0u && destination == NULL)) return 0xe103u;
    if (amount != 0u) {
        memcpy(destination, handle->stream->memory + handle->stream->cursor,
               amount);
        handle->stream->cursor += amount;
    }
    if (bytes_read != NULL) *bytes_read = amount;
    return 0u;
}

// FUNCTION: LEGORR 0x004891d0
uint32_t sound_wave_handle_and_buffer_release(SoundWaveFileHandle** handle,
                                              uint8_t** format)
{
    sound_wave_file_handle_close(handle, format);
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

// FUNCTION: LEGORR 0x0047fb60
Sound* sound_sample_file_open_by_name(const char* name)
{
    if (name == NULL) return NULL;
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
    return sound_sample_file_open_by_name(name);
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

/* SoundAliasLookup compares the GameStringHashCompute result, not the
 * original text. Retain the hashes in slot order so collisions resolve to
 * the first registered alias as they do in the reference. */
static uint32_t s_alias_hashes[SOUND_ALIAS_MAX];
static int s_alias_hashes_initialized;

/* Dynamic (config-registered) aliases appended by the lookup, mirroring
 * the corpus DAT_00502a24 slots behind the 45 built-ins. */
static char* s_alias_dynamic[SOUND_ALIAS_MAX - SOUND_ALIAS_BUILTINS];
// GLOBAL: LEGORR 0x00502a24
static int   s_alias_dynamic_count;

/* The corpus's DAT_00502a2c sound flag word: bit 0 subsystem-enabled
 * (enable-set @ 0x465260), bit 1 dynamic-alias registration, bit 3 batch
 * mode (batch-mode-get @ 0x4652b0). */
// GLOBAL: LEGORR 0x00502a2c
static uint32_t s_sound_flags;

static SoundBufferObjectStopHook s_buffer_object_stop_hook;
static void* s_buffer_object_stop_context;
static SoundPositionApplyHook s_sound_position_hook;
static void* s_sound_position_context;
static SoundInstanceRegisterHook s_instance_register_hook;
static SoundInstanceStopHook s_instance_stop_hook;
static void* s_instance_hook_context;
static void* s_sound_instances[20];
static void sound_buffer_association_remove_instance(void* instance);
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
    if (buffer_handle == -1) return NULL;
    buffer_index = buffer_handle % 3;
    slot = (uint32_t)(buffer_handle - buffer_index) / 3u;
    return s_bank[slot].buffers[buffer_index];
}

// FUNCTION: LEGORR 0x0047b130
void sound_buffer_stop_handle(int buffer_handle)
{
    if (!sound_bank_sound_on() || buffer_handle < 0
        || buffer_handle >= SOUND_BANK_SLOTS * 3) {
        return;
    }
    audio_buffer_stop(sound_handle_buffer_get(buffer_handle));
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

// FUNCTION: LEGORR 0x0047b1a0
void sound_instance_track_register(void* instance)
{
    int index;
    if (instance == NULL) return;
    if (!sound_instance_callback_record_attach(instance) &&
        s_instance_register_hook != NULL)
        (void)s_instance_register_hook(s_instance_hook_context, instance);
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

    if (provider == NULL) {
        sound_buffer_association_remove_instance(instance);
        return;
    }

#if (defined(_MSC_VER) && defined(_M_IX86)) || \
    (defined(__i386__) && (defined(__GNUC__) || defined(__clang__)))
    if (!adapter) {
        void* property = sound_instance_property_get(provider);
        if (property != NULL) {
            void* previous = NULL;
            void* record = *(void**)((char*)property + 0x24);
            while (record != NULL) {
                char* words = (char*)record;
                void* key = NULL;
                void* next = NULL;
                memcpy(&key, words, sizeof(key));
                memcpy(&next, words + 4 * sizeof(void*), sizeof(next));
                if (key == instance) {
                    sound_buffer_association_remove_instance(instance);
                    if (previous == NULL)
                        memcpy((char*)property + 0x24, &next,
                               sizeof(next));
                    else
                        memcpy((char*)previous + 4 * sizeof(void*), &next,
                               sizeof(next));
                    engine_heap_free(record);
                    break;
                }
                previous = record;
                record = next;
            }
        }
    }
    else {
        sound_buffer_association_remove_instance(instance);
    }
#else
    sound_buffer_association_remove_instance(instance);
#endif
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

/* _DAT_00502b2c — the global-cooldown word (the corpus types it as a
 * double; it holds the alias submitted under the cooldown, whose writer
 * lands with #296). */
// GLOBAL: LEGORR 0x00502b2c
static double s_cooldown_alias;

static char* s_dupstr(const char* s)
{
#if defined(_MSC_VER)
    /* Keep this allocation in the VC5 CRT heap; the alias table releases it
     * with free(). The runtime's _strdup performs the same NUL-inclusive
     * copy as the portable path below. */
    return _strdup(s);
#else
    size_t n = strlen(s) + 1;
    char* copy = (char*)calloc(n, 1);
    if (copy != NULL) {
        memcpy(copy, s, n);
    }
    return copy;
#endif
}

static uint32_t sound_alias_hash_compute(const char* name)
{
    const unsigned char* cursor = (const unsigned char*)name;
    uint32_t hash = 0u;
    uint32_t factor = 0x00600937u;
    uint32_t position = 1u;
    while (*cursor != 0u) {
        uint32_t value = (uint32_t)(unsigned char)toupper((int)*cursor);
        hash += position * factor * value;
        factor *= 0x00600937u;
        ++position;
        ++cursor;
    }
    return hash;
}

static void sound_alias_builtin_hashes_init(void)
{
    int index;
    if (s_alias_hashes_initialized != 0) return;
    for (index = 0; index < SOUND_ALIAS_BUILTINS; ++index)
        s_alias_hashes[index] = sound_alias_hash_compute(s_alias_builtins[index]);
    s_alias_hashes_initialized = 1;
}

static void sound_alias_scene_sample_callback(const char* sample_name,
                                              void* scene_object,
                                              void* context)
{
    int alias;
    (void)context;
    if (sound_alias_lookup(sample_name, &alias))
        sound_submission_route_wrapper(scene_object, alias, 0, 1, NULL);
}

// FUNCTION: LEGORR 0x00464740
void sound_alias_registry_init(void)
{
    int i;
    /* Corpus: the built-in hashes are written into a fresh allocation;
     * the port's built-ins are static. Deviation (documented): the
     * corpus's repeated call leaks the table yet keeps the stale dynamic
     * count; the port frees and zeroes the dynamic side and leaves the
     * registration flag as-is, exactly the DAT_00502a2c behaviour. */
    for (i = 0; i < s_alias_dynamic_count; ++i) {
        free(s_alias_dynamic[i]);
        s_alias_dynamic[i] = NULL;
    }
    s_alias_dynamic_count = 0;
    s_alias_hashes_initialized = 0;
    sound_alias_builtin_hashes_init();

    /* SoundAliasRegistryInit registers LAB_00464c20 through the scene sample
     * callback globals. Keep the existing owner callback and add the sound
     * resolver as an independent, idempotent subscriber. */
    (void)ddiscene_sample_hook_add(sound_alias_scene_sample_callback, NULL);
}

// FUNCTION: LEGORR 0x00464c50
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

// FUNCTION: LEGORR 0x00464c70
int sound_alias_lookup(const char* name, int* out_index)
{
    /* SoundAliasLookup @ 0x00464c70 compares GameStringHashCompute values
     * and returns the first matching slot. */
    if (name == NULL || out_index == NULL) {
        return 0; /* corpus's param_1 != 0 guard */
    }
    sound_alias_builtin_hashes_init();
    int i;
    uint32_t hash = sound_alias_hash_compute(name);
    for (i = 0; i < SOUND_ALIAS_BUILTINS + s_alias_dynamic_count; ++i) {
        if (s_alias_hashes[i] == hash) {
            *out_index = i;
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
        s_alias_hashes[index] = hash;
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

/* SoundSubsystemStopAll @ 0x47b160 — the corpus's disable seam. */
static void s_subsystem_stop_all(void)
{
    sound_subsystem_stop_all();
}

// FUNCTION: LEGORR 0x00465260
void sound_subsystem_enable_set(int enable, int stop_active)
{
    /* SoundSubsystemEnableSet @ 0x00465260. */
    if (enable != 0) {
        s_sound_flags |= SOUND_FLAG_ENABLED;
        return;
    }
    /* Corpus disable order: the flag word is read once, bit 0 cleared and
     * stored back before stop_active is tested — the store precedes the
     * stop call, and both sub-paths then zero the cooldown word. */
    uint32_t cleared = s_sound_flags & ~SOUND_FLAG_ENABLED;
    s_sound_flags = cleared;
    if (stop_active != 0) {
        s_subsystem_stop_all();
    }
    s_cooldown_alias = 0.0;
}

// FUNCTION: LEGORR 0x004652a0
void sound_subsystem_enable_set_and_stop(int enable)
{
    /* SoundSubsystemEnableSetAndStop @ 0x004652a0. */
    sound_subsystem_enable_set(enable, 1);
}

// FUNCTION: LEGORR 0x00465370
int sound_subsystem_enabled(void)
{
    return (s_sound_flags & SOUND_FLAG_ENABLED) != 0;
}

// FUNCTION: LEGORR 0x004652b0
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

// GLOBAL: LEGORR 0x00502b30
static int s_queue_alias[SOUND_REQUEST_QUEUE_CAPACITY];  /* DAT_00502b30 */
// GLOBAL: LEGORR 0x00502b58
static int s_queue_loop[SOUND_REQUEST_QUEUE_CAPACITY];   /* DAT_00502b58 */
// GLOBAL: LEGORR 0x00502bd0
static int s_queue_count;                                /* DAT_00502bd0 */

// FUNCTION: LEGORR 0x00464f60
void sound_request_queue_submit(int alias, int loop)
{
    /* SoundRequestQueueSubmit @ 0x464f60. */
    /* 0x464f68 uses the unsigned `jae` full gate. The private count starts at
     * zero and only this bounded submit path increments it. */
    if ((unsigned int)s_queue_count >=
        (unsigned int)SOUND_REQUEST_QUEUE_CAPACITY) {
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

// GLOBAL: LEGORR 0x00502b24
static float s_cooldown_timer;   /* _DAT_00502b24 (zero-initialised) */
// GLOBAL: LEGORR 0x00502b28
static int   s_cooldown_handle;  /* DAT_00502b28  (zero-initialised) */

/* The corpus compares against a file-backed .rdata dword (0x49e73c, bytes
 * 00 00 00 00). A const zero-initialized static lands with real file bytes
 * in .rdata — plain zero-initialized storage would sit in the image's zero
 * tail, which reccmp-datacmp reads as uninitialized. */
// GLOBAL: LEGORR 0x0049e73c
static const float s_deferred_zero = 0.0f; /* DAT_0049e73c role: 0.0 constant */

// FUNCTION: LEGORR 0x00464ec0
int sound_deferred_timer_set_if_idle(float seconds)
{
    /* DeferredSoundTimerSetIfIdle @ 0x464ec0. */
    if (s_cooldown_timer <= s_deferred_zero) {
        s_cooldown_handle = -1;
        s_cooldown_timer = seconds;
        return 1;
    }
    return 0;
}

// FUNCTION: LEGORR 0x00464e80
void sound_deferred_play_expire(void)
{
    /* DeferredSoundPlayExpire @ 0x464e80. */
    if (s_cooldown_handle != -1 && s_cooldown_timer > 0.0f) {
        /* SoundHandleStopForward -> SoundBufferStopHandle. The portable
         * bank validates the slot and forwards the buffer index to its
         * installed DirectSound-instance seam. */
        sound_buffer_stop_handle(s_cooldown_handle);
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

/* Variant draws use the shared VC5 rand.obj stream in the classic x86 build;
 * other builds keep a module-local LCG model. */
#if !defined(_MSC_VER) || (_MSC_VER >= 1200) || !defined(_M_IX86)
static uint32_t s_variant_rand_state = 1;
#endif

static int s_variant_rand(void)
{
#if defined(_MSC_VER) && (_MSC_VER < 1200) && defined(_M_IX86)
    return rand();
#else
    s_variant_rand_state = s_variant_rand_state * 214013u + 2531011u;
    return (int)((s_variant_rand_state >> 16) & 0x7fffu);
#endif
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

/* In-place split on any byte in the separator string. The reference leaves
 * a trailing separator attached to the final token by not testing the last
 * character as a separator start. */
// FUNCTION: LEGORR 0x00477440
int sound_variant_split(char* s, char** out, const char* separators)
{
    if (s[0] == '\0') {
        return 0;
    }
    out[0] = s;
    int n = 1;
    const size_t separator_len = strlen(separators);
    for (char* p = s; p[1] != '\0'; ++p) {
        if (strncmp(separators, p, separator_len) == 0) {
            *p = '\0';
            out[n++] = p + separator_len;
        }
    }
    return n;
}

// FUNCTION: LEGORR 0x00464d00
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
    int ntok = sound_variant_split(buf, tokens, ",");

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
            volume = atoi(digits); /* VC5 _atoi at FUN_0048db70 */
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

// FUNCTION: LEGORR 0x00464e20
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

// FUNCTION: LEGORR 0x00465010
void sound_variant_property_set(int alias_index, int millibels)
{
    /* ConfiguredVariantPropertySet @ 0x465010. Keep the selector nested in
     * the volume setter's first argument: MSVC 5 /Od evaluates the setter's
     * right argument first, staging millibels before calling the selector.
     * The corpus loads [esp+8] then [esp+4], pushes value then alias, calls
     * the selector, and reuses the staged value for the volume write. */
    sound_bank_entry_volume_set(sound_variant_select(alias_index), millibels);
}

// FUNCTION: LEGORR 0x00465030
void sound_variant_property_read(int alias_index)
{
    /* ConfiguredVariantPropertyRead @ 0x465030: only the variant draw
     * is observable — the corpus reads IndexedRecordStride134WordGet's
     * word (@ 0x47b0d0, via sound_bank_entry_volume) but discards it. */
    int slot = sound_variant_select(alias_index);
    (void)sound_bank_entry_volume(slot);
}

/* ------------------------------------------------------------------ *
 *  Sound submission route (issues #325 / #328 / #329 / #330 / #332).
 *  See sound.hpp.
 * ------------------------------------------------------------------ */

/* Corpus batch-deferred record: DAT_00502a30 +0 alias, +4 position
 * word, +8/+12/+16 coordinate words, +20 mode word (stride 6 words,
 * count DAT_00502b20). */
typedef struct BatchEntry {
    int      alias;      /* +0  DAT_00502a30 */
    int      position;   /* +4  DAT_00502a34 */
    float    xyz[3];     /* +8  DAT_00502a38/3c/40 */
    uint32_t mode;       /* +20 DAT_00502a44 */
} BatchEntry;

// GLOBAL: LEGORR 0x00502a44
static BatchEntry s_batch_entry[SOUND_BATCH_QUEUE_CAPACITY];
// GLOBAL: LEGORR 0x00502b20
static int        s_batch_count;   /* DAT_00502b20 */

/* The frame tick's active replay copies (DAT_00502b80 alias words,
 * DAT_00502ba8 loop words, count DAT_00502bd4). */
// GLOBAL: LEGORR 0x00502b80
static int s_active_alias[SOUND_REQUEST_QUEUE_CAPACITY];
// GLOBAL: LEGORR 0x00502ba8
static int s_active_loop[SOUND_REQUEST_QUEUE_CAPACITY];
// GLOBAL: LEGORR 0x00502bd4
static int s_active_count;

/* Instance-layer play seam. The portable audio API can represent an
 * explicit source position as stereo pan for one-shot samples, while
 * object-attached sources are tracked through the existing spatial hooks.
 * Stream selection and loop-buffer ownership remain with the bank path. */
// FUNCTION: LEGORR 0x0047ad70
static void s_play_seam(int mode, int position, int slot, int loop,
                        const float* xyz)
{
    Sound* sample;

    if (mode == 0 && position != 0) {
        void* instance = (void*)(uintptr_t)(uint32_t)position;
        sound_instance_track_register(instance);
        sound_source_spatial_refresh(instance);
    }

    /* The current portable 3D API is a one-shot pan operation. Keep
     * streamed/loop-selected and looping bank playback on its established
     * path; do not turn a DirectSound selector into a portable loop mode. */
    if (mode == 1 && xyz != NULL && loop == 0
        && (sound_bank_entry_flags(slot) & SOUND_STREAMED) == 0u) {
        int frames;
        float distance = (float)sqrt(xyz[0] * xyz[0]
                                   + xyz[1] * xyz[1]
                                   + xyz[2] * xyz[2]);
        float pan = 0.0f;
        if (distance > 0.0f) pan = xyz[0] / distance;
        sample = sound_bank_entry_sample_get(slot);
        if (sample != NULL) {
            frames = audio_active_play_sound_3d(sample, pan);
            if (frames >= 0) return;
        }
    }

    (void)sound_bank_entry_play_mode(slot, loop, mode);
}

// FUNCTION: LEGORR 0x00464fa0
void sound_alias_play(int alias, int loop)
{
    /* SoundAliasPlay @ 0x464fa0. */
    if ((s_sound_flags & SOUND_FLAG_BATCH) == 0) {
        if (alias != 0 && (s_sound_flags & SOUND_FLAG_ENABLED) != 0) {
            int slot = sound_variant_select(alias);
            /* Corpus quirk kept: a chain head reading 0 — a real bank
             * slot, or the unwritten head — counts as "no sound". */
            if (slot != 0) {
                s_play_seam(2, 0, slot, loop, NULL);
            }
        }
    } else if (s_batch_count < SOUND_BATCH_QUEUE_CAPACITY) {
        /* Deferred alias form writes only the alias and clears the mode
         * word's positional bit — the loop bit (and position words)
         * keep whatever an earlier batch record left in the slot
         * (corpus quirk: the deferred alias form never carries a loop
         * flag). */
        s_batch_entry[s_batch_count].alias = alias;
        s_batch_entry[s_batch_count].mode &= ~1u;
        ++s_batch_count;
    }
}

// FUNCTION: LEGORR 0x00465090
int sound_positional_sound_submit(int position, int alias, int loop,
                                  int positional, const float* xyz)
{
    /* PositionalSoundSubmit @ 0x465090. */
    if ((s_sound_flags & SOUND_FLAG_BATCH) == 0) {
        if (alias != 0 && (s_sound_flags & SOUND_FLAG_ENABLED) != 0) {
            int slot = sound_variant_select(alias);
            if (slot != 0) {
                if (positional != 0) {
                    s_play_seam(0, position, slot, loop, NULL);
                } else {
                    s_play_seam(1, 0, slot, loop, xyz);
                }
            }
        }
    /* ORIG 0x4650a4 also uses `jae`; the queue count is bounded by both
     * append gates and reset by replay drain @ 0x4652d0. */
    } else if ((unsigned int)s_batch_count <
               (unsigned int)SOUND_BATCH_QUEUE_CAPACITY) {
        BatchEntry* e = &s_batch_entry[s_batch_count];
        ++s_batch_count;
        e->alias = alias;
        e->position = position;
        e->mode = (uint32_t)((((positional & 1) << 1) | (loop & 1)) << 1)
                | (e->mode & 0xfffffff9u) | 1u;
        if (xyz != NULL) {
            e->xyz[0] = xyz[0];
            e->xyz[1] = xyz[1];
            e->xyz[2] = xyz[2];
        }
    }
    return 0;
}

// FUNCTION: LEGORR 0x00465050
void sound_submission_route_wrapper(const void* wrapped_position, int alias,
                                    int loop, int positional,
                                    const float* xyz)
{
    int position = 0;
    if (wrapped_position != NULL) {
        position = (int)scene_object_wrapped_pointer_get(wrapped_position);
    }
    sound_positional_sound_submit(position, alias, loop, positional, xyz);
}

// FUNCTION: LEGORR 0x004652d0
void sound_batch_queue_drain(int batch_set, int replay)
{
    /* SoundBatchQueueDrain @ 0x4652d0. */
    if (batch_set != 0) {
        s_sound_flags |= SOUND_FLAG_BATCH; /* raise the bit and leave */
        return;
    }
    s_sound_flags &= ~SOUND_FLAG_BATCH;
    if (replay == 0) {
        return;
    }
    /* The corpus uses an unsigned zero gate and an unsigned back edge
     * (0x465309 jbe / 0x46535a jb). Both append sites cap this private
     * count at SOUND_BATCH_QUEUE_CAPACITY, so it remains nonnegative. */
    for (unsigned int i = 0; i < (unsigned int)s_batch_count; ++i) {
        const BatchEntry* e = &s_batch_entry[i];
        const uint32_t m = e->mode;
        if ((m & 1u) == 0) {
            /* Deferred alias form replays with the stale loop bit. */
            sound_alias_play(e->alias, (int)((m << 30) >> 31));
        } else {
            sound_positional_sound_submit(e->position, e->alias,
                                          (int)((m << 30) >> 31),
                                          (int)((m << 29) >> 31),
                                          (const float*)e->xyz);
        }
    }
    s_batch_count = 0;
}

// FUNCTION: LEGORR 0x004652c0
void sound_deferred_queue_drain(int batch_set)
{
    /* SoundDeferredQueueDrain @ 0x4652c0. */
    sound_batch_queue_drain(batch_set, 1);
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

// FUNCTION: LEGORR 0x004651a0
void sound_subsystem_frame_tick(float dt)
{
    /* SoundSubsystemFrameTick @ 0x4651a0. */
    const int batch_saved = sound_batch_mode_get();
    int i;
    s_cooldown_timer -= dt;            /* _DAT_00502b24 step */
    sound_batch_queue_drain(0, 0);     /* batch off for the replay pass */
    /* Replay pass: the corpus re-reads the active count every iteration
     * (0x4651e2) and gates it unsigned, so neither is hoisted. */
    for (i = 0; i < (unsigned) s_active_count; ++i) {
        sound_alias_play(s_active_alias[i], s_active_loop[i]);
    }
    if (batch_saved != 0) {
        sound_deferred_queue_drain(1); /* restore batch mode */
    }
    /* Rotate the pending queue (#317) into the active replay copies:
     * two rep-movs passes over the pending count, then the count moves
     * and the pending count clears. */
    for (i = 0; i < (unsigned) s_queue_count; ++i) {
        s_active_alias[i] = s_queue_alias[i];
    }
    for (i = 0; i < (unsigned) s_queue_count; ++i) {
        s_active_loop[i] = s_queue_loop[i];
    }
    s_active_count = s_queue_count;
    s_queue_count = 0;
    /* SoundSourceSpatialRefreshAll @ 0x47b390: refresh attached sources,
     * remove stale associations, and advance both streamed-wave slots. */
    sound_source_spatial_refresh_all();
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

// FUNCTION: LEGORR 0x00465160
double sound_object_duration_get(int alias)
{
    /* SoundObjectDurationGet @ 0x465160: duration of whatever the alias's
     * chain selects — one PRNG draw per call (a global-cooldown submit
     * therefore draws twice: play's select and this one, as corpus). The
     * corpus tests the select (test eax,eax / je 0x46517b): a zero slot
     * never reaches the step-rate, it loads the 0x49e73c zero instead. */
    int slot = sound_variant_select(alias);
    if (slot != 0) {
        return sound_streamed_wave_buffer_step_rate(slot);
    }
    return s_deferred_zero; /* the corpus's test/je tail: a zero chain head
                             * (no sound) loads the 0x49e73c zero instead */
}

// FUNCTION: LEGORR 0x00464ef0
int sound_submit_with_global_cooldown(int alias, int* out_handle)
{
    /* SoundSubmitWithGlobalCooldown @ 0x464ef0. */
    if (s_cooldown_timer <= 0.0f) {
        const int batched = (s_sound_flags & SOUND_FLAG_BATCH) != 0;
        const int enabled = (s_sound_flags & SOUND_FLAG_ENABLED) != 0;
        sound_alias_play(alias, 0);       /* the submit's play (0x464fa0) */

        /* Corpus DAT_00502b28 = the play's instance-layer handle.
         * Deviation: the instance layer is TODO(corpus), so the port
         * records the routed bank slot — and -1 when nothing routed:
         * alias 0, a disabled subsystem, a chain head that reads 0 (the
         * "no sound" quirk), or a batch-mode deferral (no instance
         * handle until the replay). */
        int handle = -1;
        if (!batched && alias != 0 && enabled && alias > 0
            && alias < SOUND_ALIAS_MAX && s_variant_slot[alias] != 0) {
            handle = s_variant_slot[alias];
        }

        if (handle != -1) {
            s_cooldown_alias = (double)alias;   /* _DAT_00502b2c */
            s_cooldown_timer =
                (float)(sound_object_duration_get(alias) * 25.0);
            s_cooldown_handle = handle;         /* DAT_00502b28 */
            if (out_handle != NULL) {
                *out_handle = handle;
            }
            return 1;
        }
    }
    return 0;
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
static void sound_buffer_association_remove_instance(void* instance)
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
        void* buffer_object = NULL;
        void* attached_entity = NULL;
        void* next = NULL;
        int active = 1;
        memcpy(&buffer_object, words + sizeof(void*), sizeof(buffer_object));
        memcpy(&attached_entity, words + 2 * sizeof(void*),
               sizeof(attached_entity));
        memcpy(&next, words + 3 * sizeof(void*), sizeof(next));
        if (buffer_object != NULL && s_spatial_refresh_hook != NULL)
            active = s_spatial_refresh_hook(s_spatial_refresh_context,
                                            buffer_object);
        if (!active)
            (void)sound_attached_stop_by_entity(
                (uint32_t)(uintptr_t)attached_entity);
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
    return 0;
}

// FUNCTION: LEGORR 0x0047b160
void sound_subsystem_stop_all(void)
{
    sound_buffer_association_stop_all();
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
