// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — FLIC `.flh` overlay animation player (finding 21).
 *
 * Implements the finding's coder model exactly: 136-byte header + a chunk
 * stream from 0x80; frame chunk = one opcode-25 keyframe (FLI16 byte-RLE)
 * plus nFrames opcode-27 delta records (word-based row jumps + skip/control
 * commands), with size-16 / count-0 chunks acting as frame holds. Pixels are
 * stored RGB555 and expanded to RGBA32 for the renderer. Only the shipped
 * surface — magic AF43, depth 16, opcodes {25, 27} — is handled; the other
 * corpus-accepted magics/8-bit coders remain unsupported (none ship).
 *
 * The corpus players are FUN_00483c80 (open) / FUN_00484070 (tick) /
 * FUN_004841d0+260 (frame steppers); MoviePlayWait @ 0x415700 drives them
 * modal. This module keeps the same RAM-mode semantics: the whole chunk
 * stream is preloaded at open and frames are decoded on tick, with loop
 * restarting at index 1 (the keyframe is already in the buffer).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flic.hpp"
#include "record.hpp"
#include "movie.hpp"
#include "resource.hpp"

#define FLIC_MAGIC_AF43 0xaf43
#define FLIC_HEADER 136      /* the engine's read size (context +0x640) */
#define FLIC_CHUNK_START 0x80 /* frame data starts at file offset 0x80 (§2) */
#define FLIC_CHUNK_HDR 16

struct FlicChunk {
    const uint8_t* data; /* chunk start (16-byte header included) */
    uint32_t       size; /* chunk size, incl. header */
    uint16_t       count; /* subrecord count; 0 = hold chunk */
    uint16_t       type;
};

struct Flic {
    int width, height;
    int frames;       /* header nFrames + 1 */
    int idx;          /* current frame (0 = keyframe) */
    int eof;
    FlicChunk* chunks; /* chunks[0] = keyframe, [1..] = deltas/holds */
    int chunk_n;
    int chunk_cap;
    int frame_n;
    int last_walk_frame_valid;
    int first_frame_chunk;
    int next_chunk;
    size_t cursor;   /* byte offset of the next chunk in the RAM image */
    uint8_t* raw;     /* owned copy of the whole file */
    size_t   raw_len;
    uint16_t* pix;    /* RGB555 frame buffer */
    uint8_t* rgba;    /* lazy RGBA32 conversion */
    int current_chunk;
    int stream_mode;
    FlicStreamOps stream_ops;
    uint32_t stream_length;
    uint8_t* stream_buffer;
    uint32_t stream_buffer_capacity;
    FlicChunk stream_chunk;
    size_t stream_loop_cursor;
    void (*stream_context_close)(void* context);
};

static uint16_t le16(const uint8_t* p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t le32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16)
           | ((uint32_t)p[3] << 24);
}

/* FlicFileHeaderValidate (FUN_00483f60): the original reads 0x88 bytes,
 * compares the declared file length with the stream length, and accepts the
 * five FLIC-family markers. Format-specific surface constraints are checked
 * by flic_open after this shared container validation. */
int flic_file_header_validate(const void* bytes, size_t length,
                              FlicHeaderInfo* header)
{
    const uint8_t* data = (const uint8_t*)bytes;
    uint16_t magic;
    if (data == NULL || header == NULL || length < FLIC_HEADER
        || length > 0xffffffffu || le32(data) != (uint32_t)length) {
        return 0;
    }
    magic = le16(data + 4);
    if (magic != 0x1234u && magic != 0x9119u
        && magic != 0xaf11u && magic != 0xaf12u && magic != 0xaf43u) {
        return 0;
    }
    header->magic = magic;
    header->frames = le16(data + 6);
    header->width = le16(data + 8);
    header->height = le16(data + 10);
    header->depth = le16(data + 12);
    return 1;
}

int flic_width(const Flic* f)  { return f != NULL ? f->width : 0; }
int flic_height(const Flic* f) { return f != NULL ? f->height : 0; }
int flic_frames(const Flic* f) { return f != NULL ? f->frames : 0; }
int flic_frame(const Flic* f)  { return f != NULL ? f->idx : 0; }
int flic_eof(const Flic* f)    { return f != NULL ? f->eof : 0; }

uint32_t flic_rgb555_to_rgba(uint16_t c)
{
    uint32_t r = (uint32_t)((c >> 10) & 0x1f);
    uint32_t g = (uint32_t)((c >> 5) & 0x1f);
    uint32_t b = (uint32_t)(c & 0x1f);
    r = (r << 3) | (r >> 2);
    g = (g << 3) | (g >> 2);
    b = (b << 3) | (b >> 2);
    return (0xffu << 24) | (b << 16) | (g << 8) | r;
}

/* --- decoders (finding 21 §4/§5) ---------------------------------------- */

/* Keyframe, opcode 25: FLI16 byte-RLE, body at record+7, one terminator
 * byte after each row (the engine over-reads the final one by up to a byte;
 * the bounded reader tolerates that). */
static void fli_keyframe(uint16_t* pix, int w, int h, const uint8_t* body,
                         size_t len)
{
    size_t p = 0;
    for (int row = 0; row < h; ++row) {
        int x = 0;
        while (x < w) {
            if (p + 1 > len) {
                return; /* bounded read (final terminators may overrun) */
            }
            int c = body[p++];
            if (c < 0x80) {
                if (p + 2 > len) {
                    return;
                }
                uint16_t color = le16(body + p);
                p += 2;
                for (int k = 0; k < c && x + k < w; ++k) {
                    pix[(size_t)row * w + x + k] = color;
                }
                x += c;
            } else {
                int n = 256 - c;
                for (int k = 0; k < n && x + k < w; ++k) {
                    if (p + 2 > len) {
                        return;
                    }
                    pix[(size_t)row * w + x + k] = le16(body + p);
                    p += 2;
                }
                x += n;
            }
        }
        if (p + 1 > len) {
            return;
        }
        ++p; /* row terminator (value don't-care) */
    }
}

/* Delta frame, opcode 27: word stream — 0xC…/row jumps (signed), then per
 * line a command-count word and that many [skip:u8|control:u8] commands
 * with literal (control < 0x80) or repeat (control >= 0x80) pixel runs.
 * Body at record+8; decode until the record body is consumed. */
static void fli_delta(uint16_t* pix, int w, int h, const uint8_t* body,
                      size_t len)
{
    size_t p = 0;
    int row = 0;
    while (p + 2 <= len) {
        uint16_t wd = le16(body + p);
        p += 2;
        int top = (wd & 0xC000) >> 12;
        if (top == 0xC) { /* row jump: row -= (short)wd (0xFFFF -> +1) */
            row -= (int16_t)wd;
            continue;
        }
        if (top == 0x4 || top == 0x8) {
            break; /* end-of-record / 0x8 color-embedded (not shipped) */
        }
        int n = wd; /* command count for the next changed line */
        int x = 0;
        for (int i = 0; i < n; ++i) {
            if (p + 2 > len) {
                return;
            }
            uint16_t sc = le16(body + p);
            p += 2;
            /* The packet word is stored little-endian as [skip, control],
             * so skip is its low byte and the signed run control its high
             * byte. Reversing these fields makes changed rows wander into
             * adjacent pixels after the first delta frame. */
            int skip = sc & 0xff;
            int ctrl = (sc >> 8) & 0xff;
            x += skip;
            if (ctrl < 0x80) {
                for (int k = 0; k < ctrl; ++k) {
                    if (p + 2 > len) {
                        return;
                    }
                    uint16_t c = le16(body + p);
                    p += 2;
                    if (row >= 0 && row < h && x + k >= 0 && x + k < w) {
                        pix[(size_t)row * w + x + k] = c;
                    }
                }
                x += ctrl;
            } else {
                if (p + 2 > len) {
                    return;
                }
                uint16_t c = le16(body + p);
                p += 2;
                int r = 256 - ctrl;
                for (int k = 0; k < r; ++k) {
                    if (row >= 0 && row < h && x + k >= 0 && x + k < w) {
                        pix[(size_t)row * w + x + k] = c;
                    }
                }
                x += r;
            }
        }
        row += 1;
    }
}

/* Decode every subrecord of one chunk into the frame buffer (a keyframe in
 * chunk 0, deltas otherwise; holds have none). */
static int fli_decode_chunk(Flic* f, const FlicChunk* ch)
{
    if (ch->count == 0) {
        return 1; /* hold: pixels unchanged */
    }
    size_t pos = FLIC_CHUNK_HDR; /* subrecords start at chunk + 0x10 */
    for (uint16_t r = 0; r < ch->count; ++r) {
        if (pos + 6 > ch->size) {
            return 0; /* truncated size/opcode header */
        }
        const uint8_t* rec = ch->data + pos;
        uint32_t rec_size = le32(rec);
        uint16_t opcode = le16(rec + 4);
        uint32_t header_size = opcode == 25 ? 7u : 8u;
        if (rec_size < header_size || rec_size > ch->size - pos) {
            return 0; /* record overruns its chunk — bounded */
        }
        const uint8_t* body = rec + header_size;
        size_t blen = rec_size - header_size;
        if (opcode == 25) {
            fli_keyframe(f->pix, f->width, f->height, body, blen);
        } else if (opcode == 27) {
            fli_delta(f->pix, f->width, f->height, body, blen);
        }
        pos += rec_size;
    }
    if (f->rgba != NULL) {
        f->rgba = NULL; /* invalidate the conversion */
    }
    return 1;
}

static int flic_stream_buffer_reserve(Flic* f, uint32_t capacity)
{
    uint8_t* buffer;
    if (f == NULL) return 0;
    if (capacity <= f->stream_buffer_capacity) return 1;
    buffer = (uint8_t*)realloc(f->stream_buffer, (size_t)capacity);
    if (buffer == NULL) return 0;
    f->stream_buffer = buffer;
    f->stream_buffer_capacity = capacity;
    return 1;
}

static int flic_stream_chunk_header_read(Flic* f)
{
    uint32_t size;
    if (f == NULL || f->stream_mode == 0 ||
        f->cursor > f->stream_length ||
        f->stream_length - f->cursor < FLIC_CHUNK_HDR ||
        !flic_stream_buffer_reserve(f, FLIC_CHUNK_HDR)) return 0;
    if (!f->stream_ops.read_at(f->stream_ops.context, (uint32_t)f->cursor,
                               f->stream_buffer, FLIC_CHUNK_HDR)) return 0;
    size = le32(f->stream_buffer);
    if (size < FLIC_CHUNK_HDR || size > f->stream_length - f->cursor)
        return 0;
    f->stream_chunk.data = f->stream_buffer;
    f->stream_chunk.size = size;
    f->stream_chunk.type = le16(f->stream_buffer + 4);
    f->stream_chunk.count = le16(f->stream_buffer + 6);
    return 1;
}

static int flic_stream_chunk_load(Flic* f)
{
    if (f == NULL || f->stream_mode == 0 ||
        !flic_stream_buffer_reserve(f, f->stream_chunk.size)) return 0;
    if (!f->stream_ops.read_at(f->stream_ops.context, (uint32_t)f->cursor,
                               f->stream_buffer, f->stream_chunk.size))
        return 0;
    f->stream_chunk.data = f->stream_buffer;
    return 1;
}

// FUNCTION: LEGORR 0x004844b0
/* FlicFrameRecordWalk: walk the declared subrecords from the
 * 16-byte chunk header, advancing the byte cursor by each record's size. RAM
 * mode has already buffered the chunk; stream-mode refill/capacity work is
 * not representable by this resource-independent in-memory player. */
int flic_frame_record_walk(Flic* f)
{
    const FlicChunk* ch;
    if (f == NULL) {
        return 0;
    }
    if (f->stream_mode != 0) {
        if (!flic_stream_chunk_load(f)) return 0;
        ch = &f->stream_chunk;
    }
    else {
        if (f->chunks == NULL || f->current_chunk < 0 ||
            f->current_chunk >= f->chunk_n) return 0;
        ch = &f->chunks[f->current_chunk];
    }
    if (ch->type != 0xf1fau) {
        return 0;
    }
    if (f->stream_mode == 0)
        f->cursor = (size_t)(ch->data - f->raw) + FLIC_CHUNK_HDR;
    if (!fli_decode_chunk(f, ch)) {
        return 0;
    }
    {
        size_t pos = FLIC_CHUNK_HDR;
        uint16_t r;
        for (r = 0; r < ch->count; ++r) {
            uint32_t rec_size = le32(ch->data + pos);
            pos += rec_size;
        }
        if (f->stream_mode == 0)
            f->cursor = (size_t)(ch->data - f->raw) + pos;
    }
    return 1;
}

// FUNCTION: LEGORR 0x00484720
/* FlicChunkBufferAndContinueLK (FUN_00484720): the in-memory file-image
 * path is a no-op. In external-stream mode, grow the chunk buffer to the
 * current chunk's declared size, seek to the chunk start through read_at,
 * and refill the whole chunk. read_at is the port's equivalent of the
 * reference seek/read pair; the reference routine always returns zero. */
int flic_chunk_buffer_and_continue_lk(Flic* f)
{
    if (f == NULL || f->stream_mode == 0) return 0;
    f->stream_chunk.data = NULL;
    if (!flic_stream_buffer_reserve(f, f->stream_chunk.size)) return 0;
    if (f->stream_ops.read_at == NULL ||
        !f->stream_ops.read_at(f->stream_ops.context, (uint32_t)f->cursor,
                               f->stream_buffer, f->stream_chunk.size)) {
        return 0;
    }
    f->stream_chunk.data = f->stream_buffer;
    return 0;
}

// FUNCTION: LEGORR 0x004847d0
/* FlicChunkBufferAndContinueType0B has the same successful stream path. The
 * original uniquely returns 1 when scratch allocation fails. */
int flic_chunk_buffer_and_continue_type0b(Flic* f)
{
    if (f != NULL && f->stream_mode != 0 &&
        !flic_stream_buffer_reserve(f, f->stream_chunk.size)) return 1;
    if (f != NULL && f->stream_mode != 0) flic_stream_chunk_load(f);
    return 0;
}

// FUNCTION: LEGORR 0x00484320
/* FlicChunkWalk: advance through non-frame chunks until the next F1FA frame
 * is found. The RAM branch uses the indexed image; stream mode reads a header
 * then dispatches LK/type-0B through their seek-and-buffer handlers. */
int flic_chunk_walk(Flic* f)
{
    if (f == NULL) {
        return 0;
    }
    f->last_walk_frame_valid = 0;
    if (f->stream_mode != 0) {
        while (f->cursor < f->stream_length) {
            uint32_t chunk_size;
            uint16_t chunk_type;
            if (!flic_stream_chunk_header_read(f)) return 0;
            chunk_size = f->stream_chunk.size;
            chunk_type = f->stream_chunk.type;
            if (chunk_type == 0x000bu) {
                flic_chunk_buffer_and_continue_type0b(f);
            }
            else if (chunk_type == 0x4b4cu) {
                flic_chunk_buffer_and_continue_lk(f);
            }
            else if (chunk_type == 0xf1fau) {
                f->last_walk_frame_valid = flic_frame_record_walk(f);
                f->cursor += chunk_size;
                return 0;
            }
            f->cursor += chunk_size;
        }
        return 0;
    }
    if (f->chunks == NULL || f->chunk_n == 0) {
        return 0;
    }
    while (f->next_chunk < f->chunk_n) {
        int chunk_index = f->next_chunk++;
        const FlicChunk* ch = &f->chunks[chunk_index];
        f->current_chunk = chunk_index;
        f->cursor = (size_t)(ch->data - f->raw);
        if (ch->type == 0x000bu) {
            flic_chunk_buffer_and_continue_type0b(f);
        } else if (ch->type == 0x4b4cu) {
            flic_chunk_buffer_and_continue_lk(f);
        } else if (ch->type == 0xf1fau) {
            f->last_walk_frame_valid = flic_frame_record_walk(f);
            f->cursor = (size_t)(ch->data - f->raw) + ch->size;
            return 0;
        }
        f->cursor = (size_t)(ch->data - f->raw) + ch->size;
        /* All other chunks are skipped using the checked size recorded by
         * the open-time chunk scan. */
    }
    return 0;
}

/* --- open / close --------------------------------------------------------- */

// FUNCTION: LEGORR 0x00483c80
Flic* flic_open(const char* name)
{
    Flic* stream_flic;
    if (name == NULL || name[0] == '\0') {
        return NULL;
    }
    stream_flic = flic_open_resource_stream(name);
    if (stream_flic != NULL) return stream_flic;
    Resource* r = resource_open(name, RESOURCE_AUTO);
    if (r == NULL) {
        /* Loose `Data/AVI/...` overlay copies (avi\PB1.flh etc.) resolve
         * through the movie path helper (case-insensitive, base = game dir). */
        char norm[256];
        strncpy(norm, name, sizeof(norm) - 1);
        norm[sizeof(norm) - 1] = '\0';
        char* separator = strchr(norm, '\\');
        while (separator != NULL) {
            *separator++ = '/';
            separator = strchr(separator, '\\');
        }
        const char* wd = getenv("LEGORRR_WAD_DIR");
        char os[512];
        if (movie_path_resolve(wd != NULL && wd[0] != '\0' ? wd : ".",
                               norm, os, sizeof(os))) {
            r = resource_open(os, RESOURCE_STD_ONLY);
        }
    }
    if (r == NULL || resource_read(r) <= FLIC_HEADER) {
        if (r != NULL) {
            resource_free(r);
        }
        return NULL;
    }
    const uint8_t* d = r->data;
    int n = r->size;

    /* Header validator mirrors FUN_00483f60; retain the port's explicit
     * 16-bit surface and allocation bounds after container validation. */
    FlicHeaderInfo header;
    header.depth = 0;
    if (!flic_file_header_validate(d, (size_t)n, &header)
        || header.depth != 16 || header.width == 0 || header.height == 0
        || header.width > 4096 || header.height > 4096) {
        fprintf(stderr, "flic: %s: bad header (size %d vs %d, depth %d)\n",
                name, (int)le32(d), n, header.depth);
        resource_free(r);
        return NULL;
    }

    Flic* f = (Flic*)calloc(1, sizeof(*f));
    if (f == NULL) {
        resource_free(r);
        return NULL;
    }
    f->width = header.width;
    f->height = header.height;
    f->frames = (int)header.frames + 1;
    f->first_frame_chunk = -1;
    f->current_chunk = -1;
    f->raw = (uint8_t*)malloc((size_t)n);
    f->pix = (uint16_t*)malloc((size_t)header.width * header.height * 2);
    f->chunk_cap = (int)(((size_t)n - FLIC_CHUNK_START) / FLIC_CHUNK_HDR) + 1;
    f->chunks = (FlicChunk*)calloc((size_t)f->chunk_cap, sizeof(FlicChunk));
    if (f->raw == NULL || f->pix == NULL || f->chunks == NULL) {
        flic_close(f);
        resource_free(r);
        return NULL;
    }
    memcpy(f->raw, d, (size_t)n);
    f->raw_len = (size_t)n;
    resource_free(r);

    /* Bounded chunk inventory from 0x80. Keep all chunks because the
     * original walker skips ancillary and unknown types between frames. */
    size_t pos = FLIC_CHUNK_START;
    int cn = 0;
    while (pos + FLIC_CHUNK_HDR <= f->raw_len && cn < f->chunk_cap) {
        const uint8_t* ch = f->raw + pos;
        uint32_t csize = le32(ch);
        if (csize < FLIC_CHUNK_HDR || pos + csize > f->raw_len) {
            break; /* truncated tail — the header nFrames is authoritative */
        }
        f->chunks[cn].data = ch;
        f->chunks[cn].size = csize;
        f->chunks[cn].count = le16(ch + 6);
        f->chunks[cn].type = le16(ch + 4);
        if (f->chunks[cn].type == 0xf1fau) {
            if (f->first_frame_chunk < 0) {
                f->first_frame_chunk = cn;
            }
            ++f->frame_n;
        }
        ++cn;
        pos += csize;
    }
    f->chunk_n = cn;

    /* Frame 0 is the first F1FA chunk; prefix metadata chunks are skipped. */
    if (f->first_frame_chunk < 0) {
        flic_close(f);
        return NULL;
    }
    f->current_chunk = f->first_frame_chunk;
    f->cursor = (size_t)(f->chunks[f->first_frame_chunk].data - f->raw);
    if (!flic_frame_record_walk(f)) {
        flic_close(f);
        return NULL;
    }
    f->cursor = (size_t)(f->chunks[f->first_frame_chunk].data - f->raw)
                + f->chunks[f->first_frame_chunk].size;
    f->next_chunk = f->first_frame_chunk + 1;
    f->idx = 0;
    return f;
}

Flic* flic_open_stream(const FlicStreamOps* ops, uint32_t file_size)
{
    uint8_t header_bytes[FLIC_HEADER];
    FlicHeaderInfo header;
    Flic* f;
    if (ops == NULL || ops->read_at == NULL || file_size < FLIC_HEADER ||
        !ops->read_at(ops->context, 0u, header_bytes, FLIC_HEADER) ||
        !flic_file_header_validate(header_bytes, file_size, &header) ||
        header.magic != FLIC_MAGIC_AF43 || header.depth != 16 ||
        header.width == 0 || header.height == 0 || header.width > 4096 ||
        header.height > 4096) return NULL;
    f = (Flic*)calloc(1, sizeof(*f));
    if (f == NULL) return NULL;
    f->width = header.width;
    f->height = header.height;
    f->frames = (int)header.frames + 1;
    f->first_frame_chunk = -1;
    f->current_chunk = -1;
    f->stream_mode = 1;
    f->stream_ops = *ops;
    f->stream_length = file_size;
    f->cursor = FLIC_CHUNK_START;
    f->pix = (uint16_t*)malloc((size_t)header.width * header.height * 2u);
    if (f->pix == NULL || !flic_stream_buffer_reserve(f, FLIC_CHUNK_HDR)) {
        flic_close(f);
        return NULL;
    }
    flic_chunk_walk(f); /* target-compatible walker return is always zero */
    if (!f->last_walk_frame_valid) {
        flic_close(f);
        return NULL;
    }
    f->idx = 0;
    f->eof = 0;
    f->stream_loop_cursor = f->cursor;
    return f;
}

static int flic_resource_stream_read_at(void* context, uint32_t offset,
                                        void* bytes, uint32_t length)
{
    return resource_stream_read_at((ResourceStream*)context, offset, bytes,
                                   length);
}

static void flic_resource_stream_close(void* context)
{
    resource_stream_close((ResourceStream*)context);
}

Flic* flic_open_resource_stream(const char* name)
{
    ResourceStream* stream;
    FlicStreamOps ops;
    Flic* f;
    uint32_t file_size = 0u;
    if (name == NULL || name[0] == '\0') return NULL;
    stream = resource_stream_open(name, RESOURCE_AUTO, &file_size);
    if (stream == NULL) {
        char normalized[256];
        const char* wad_dir = getenv("LEGORRR_WAD_DIR");
        char os_path[512];
        const char* slash;
        strncpy(normalized, name, sizeof(normalized) - 1u);
        normalized[sizeof(normalized) - 1u] = '\0';
        slash = strchr(normalized, '\\');
        while (slash != NULL) {
            normalized[slash - normalized] = '/';
            slash = strchr(slash + 1, '\\');
        }
        if (!movie_path_resolve(wad_dir != NULL && wad_dir[0] != '\0'
                                    ? wad_dir : ".",
                                normalized, os_path, sizeof(os_path)))
            return NULL;
        stream = resource_stream_open(os_path, RESOURCE_STD_ONLY,
                                      &file_size);
        if (stream == NULL) return NULL;
    }
    ops.context = stream;
    ops.read_at = flic_resource_stream_read_at;
    f = flic_open_stream(&ops, file_size);
    if (f == NULL) {
        resource_stream_close(stream);
        return NULL;
    }
    f->stream_context_close = flic_resource_stream_close;
    return f;
}

void flic_close(Flic* f)
{
    if (f == NULL) {
        return;
    }
    free(f->chunks);
    free(f->raw);
    free(f->pix);
    free(f->rgba);
    free(f->stream_buffer);
    if (f->stream_context_close != NULL)
        f->stream_context_close(f->stream_ops.context);
    free(f);
}

// FUNCTION: LEGORR 0x004841d0
int flic_tick_loop(Flic* f, int loop)
{
    if (f == NULL) {
        return 0;
    }
    if (f->idx + 1 >= f->frames) {
        if (!loop) {
            f->eof = 1;
            return f->idx; /* stay on the last frame */
        }
        /* RAM mode keeps the decoded keyframe and restarts at frame 1. */
        f->idx = 0;
        if (f->stream_mode != 0) {
            f->cursor = f->stream_loop_cursor;
        }
        else {
            f->next_chunk = f->first_frame_chunk + 1;
        }
    }
    flic_chunk_walk(f);
    if (!f->last_walk_frame_valid) {
        f->eof = 1;
        return f->idx;
    }
    ++f->idx;
    f->eof = 0;
    return f->idx;
}

const uint16_t* flic_frame555(const Flic* f)
{
    return f != NULL ? f->pix : NULL;
}

const uint8_t* flic_frame_rgba(Flic* f)
{
    if (f == NULL || f->pix == NULL) {
        return NULL;
    }
    if (f->rgba == NULL) {
        size_t np = (size_t)f->width * f->height;
        f->rgba = (uint8_t*)malloc(np * 4);
        if (f->rgba == NULL) {
            return NULL;
        }
        for (size_t i = 0; i < np; ++i) {
            uint32_t px = flic_rgb555_to_rgba(f->pix[i]);
            f->rgba[i * 4 + 0] = (uint8_t)(px & 0xff);
            f->rgba[i * 4 + 1] = (uint8_t)((px >> 8) & 0xff);
            f->rgba[i * 4 + 2] = (uint8_t)((px >> 16) & 0xff);
            f->rgba[i * 4 + 3] = 0xff;
        }
    }
    return f->rgba;
}

/* FlicOpcode25SuccessStub (FUN_00484b90, finding 21): success, no work. */
// FUNCTION: LEGORR 0x00484b90
uint32_t flic_opcode25_success_stub(void)
{
    return 1u;
}

// FUNCTION: LEGORR 0x00484c90
void flic_opcode27_fallback_noop(void)
{
    /* FUN_00484c90: pure return — faithful. */
}

/* FlicOpcode16ModeDispatch (FUN_00484880) — see flic.hpp. The corpus reads
 * the dispatch words from its frame record; the port takes the record as a
 * byte window (the corpus buffer shape). The fallback callee is the pure
 * no-op the port already models, so every branch observes as 1. */
// FUNCTION: LEGORR 0x00484880
int flic_opcode16_mode_dispatch(const uint8_t* rec)
{
    uint32_t mode = 0u, op = 0u;
    if (rec != NULL) {
        __builtin_memcpy(&mode, rec + 0x6d8, sizeof mode);
        __builtin_memcpy(&op, rec + 0x6d4, sizeof op);
    }
    if (mode == 0x10u) {
        flic_opcode27_fallback_noop();
        return 1;
    }
    if (op == 0u) {
        flic_opcode27_fallback_noop();
    }
    if (op == 4u) {
        flic_opcode27_fallback_noop();
    }
    return 1;
}

/* PackedTripletTableToWordBuild (FUN_00484ba0) — see flic.hpp. The record
 * shape rides the frame-record byte window (the opcode-16 dispatch
 * precedent): 256 packed RGB triplets at +0x11c become 16-bit words at
 * +0x41c, and the first word is then cleared. Deviation: the corpus strides
 * the record blindly. */
// FUNCTION: LEGORR 0x00484ba0
void flic_packed_triplet_table_to_word_build(void* record)
{
    if (record == NULL) {
        return; /* deviation: pinned by test_flic.cpp:418 — the corpus strides blindly */
    }
    uint8_t* words = (uint8_t*)record + 0x41c;
    const uint8_t* src = (const uint8_t*)record + 0x11c;
    uint8_t* dst = words;
    for (int count = 0x100; count != 0; --count) {
        const uint16_t w = (uint16_t)(
            (uint16_t)((uint16_t)(src[1] | (uint16_t)((src[0] & 0x3e) << 5))
                       << 5)
            | (uint16_t)(src[2] >> 1));
        __builtin_memcpy(dst, &w, sizeof w);
        dst += 2;
        src += 3;
    }
    const uint16_t zero = 0u;
    __builtin_memcpy(words, &zero, sizeof zero);
}

/* --- the corpus frame-record window band (2026-10-25) ----------------------
 * These decoders run on the corpus's record buffer (the opcode-16 dispatch
 * and triplet-word precedent): triplets +0x11c, words +0x41c (the ported
 * indexed-word-table lookup window), stream link +0x6d0, pixel buffer +0x6cc,
 * pitch +0x638, row count +0x10, row length +0xc, op +0x6d4, format flag
 * +0x6e4. The stream chunks carry their count at stream+6 and bytes at
 * stream+8. The corpus strides all of it blindly; the port guards only the
 * record pointer (deviation, the dispatch precedent). */

/* The corpus palette store: rec + 0x41c + index*2, folded as the ported
 * indexed-word-table lookup (the byte window stands in for the table). */
static uint16_t rec_palette_word(const char* rec, unsigned index)
{
    uint16_t w = 0u;
    __builtin_memcpy(&w, rec + 0x41c + (size_t)(index & 0xffu) * 2u, sizeof w);
    return w;
}

/* The corpus record is 32-bit: the stream link +0x6d0 and the pixel buffer
 * +0x6cc sit four bytes apart, so no LP64 pointer fits in the byte window
 * (the pixel store overwrites the stream's low half). Install-hook side
 * slots keep both (the hook-boundary static precedent); the window keeps
 * only the corpus-shaped decorations. Deviation: a missing install falls
 * back to the raw window read, guarded NULL instead of a stride. */
struct flic_window_slot {
    const void* rec;
    void* stream;
    void* pixels;
};
static flic_window_slot g_flic_windows[16];
static unsigned g_flic_window_next = 0u;

static int flic_window_find(const void* rec, void** stream, void** pixels)
{
    for (int i = 0; i != 16; ++i) {
        if (rec != NULL && g_flic_windows[i].rec == rec) {
            *stream = g_flic_windows[i].stream;
            if (pixels != NULL) {
                *pixels = g_flic_windows[i].pixels;
            }
            return 1;
        }
    }
    return 0;
}

/* FlicDecode16BitRowRecords (FUN_00484c00) — see flic.hpp. The triplet copy
 * lands each source byte >>2; the zero-flag chunk restarts the table for a
 * full 0x100-triplet pass on the loop's last lap; op 4 hands off to the
 * triplet fold. */
// FUNCTION: LEGORR 0x00484c00
int flic_decode16_bit_row_records(void* record)
{
    if (record == NULL) {
        return 1; /* deviation: the corpus strides the record blindly */
    }
    char* rec = (char*)record;
    void* stream = NULL;
    if (flic_window_find(rec, &stream, NULL) == 0) {
        __builtin_memcpy(&stream, rec + 0x6d0, sizeof stream);
    }
    if (stream == NULL) {
        return 1; /* deviation: the corpus strides the window blindly */
    }
    char* dst = rec + 0x11c;
    const char* chunk = (const char*)stream + 8;
    int16_t left = 0;
    __builtin_memcpy(&left, (const char*)stream + 6, sizeof left);
    while (left != 0) {
        const char* data = chunk + 2;
        uint32_t count = (uint8_t)chunk[1];
        dst += (size_t)(uint8_t)chunk[0] * 3u;
        if (count == 0u) {
            left = 1;
            count = 0x100u;
            dst = rec + 0x11c;
        }
        int32_t n = (int32_t)(count * 3u);
        if ((short)n != 0) {
            for (; n != 0; n = n - 1) {
                *dst = (char)((uint8_t)*data >> 2);
                dst = dst + 1;
                data = data + 1;
            }
        }
        chunk = data;
        left = (short)(left - 1);
    }
    int op = 0;
    __builtin_memcpy(&op, rec + 0x6d4, sizeof op);
    if (op == 4) {
        flic_packed_triplet_table_to_word_build(rec);
    }
    return 1;
}

/* FlicDecode8BitOpcode25Mode4 (FUN_004848d0) — see flic.hpp. The byte
 * stream rides stream+7; the row walk ends each pass one byte short and
 * the palette store lands through the indexed word window. */
// FUNCTION: LEGORR 0x004848d0
int flic_decode8_bit_opcode25_mode4(void* record)
{
    if (record == NULL) {
        return 1; /* deviation: the corpus strides the record blindly */
    }
    char* rec = (char*)record;
    void* stream = NULL;
    void* pixels = NULL;
    if (flic_window_find(rec, &stream, &pixels) == 0) {
        __builtin_memcpy(&stream, rec + 0x6d0, sizeof stream);
        __builtin_memcpy(&pixels, rec + 0x6cc, sizeof pixels);
    }
    if (stream == NULL || pixels == NULL) {
        return 1; /* deviation: the corpus strides the window blindly */
    }
    const char* src = (const char*)stream + 7;
    uint32_t rows = 0u;
    uint16_t rows16 = 0u;
    __builtin_memcpy(&rows16, rec + 0x10, sizeof rows16);
    rows = rows16;
    if (rows != 0u) {
        int16_t row_len = 0;
        __builtin_memcpy(&row_len, rec + 0x0c, sizeof row_len);
        uint32_t row_i = 0u;
        do {
            uint32_t pitch = 0u;
            __builtin_memcpy(&pitch, rec + 0x638, sizeof pitch);
            char* pixel = (char*)pixels
                          + (size_t)(pitch >> 1) * (size_t)row_i * 2u;
            int16_t left = row_len;
            while (left != 0) {
                const uint32_t b0 = (uint8_t)*src;
                const char* next = src + 1;
                if (b0 < 0x80u) {
                    const uint8_t v = (uint8_t)*next;
                    left = (short)(left - (short)b0);
                    src = next + 1;
                    if (b0 != 0u) {
                        for (uint32_t k = b0; k != 0u; k = k - 1u) {
                            const uint16_t w = rec_palette_word(rec, v);
                            __builtin_memcpy(pixel, &w, sizeof w);
                            pixel += 2;
                        }
                    }
                } else {
                    const uint32_t run = 0x100u - b0;
                    left = (short)(left - (short)(uint16_t)run);
                    src = next;
                    for (uint32_t k = run; k != 0u; k = k - 1u) {
                        const uint16_t w = rec_palette_word(rec, (uint8_t)*src);
                        src = src + 1;
                        __builtin_memcpy(pixel, &w, sizeof w);
                        pixel += 2;
                    }
                }
            }
            row_i = row_i + 1u;
            src = src + 1;
            rows = rows - 1u;
        } while (rows != 0u);
    }
    return 1;
}

/* The corpus 16-bit pixel expansion: the green field shifts one lane up
 * (flag +0x6e4 keeps the field where it is), and the low five ride along. */
static uint16_t expand16_word(const char* rec, uint16_t w)
{
    int flag = 0;
    __builtin_memcpy(&flag, rec + 0x6e4, sizeof flag);
    const uint16_t g = (flag != 0) ? (uint16_t)(w & 0x7fe0u)
                                   : (uint16_t)(((unsigned)(w & 0xffe0u)) << 1);
    return (uint16_t)(g | (w & 0x1fu));
}

/* FlicDecode16BitKeyframe (FUN_004849d0) — see flic.hpp. The word stream
 * rides stream+7; each row re-reads the row length +0xc (the corpus walks
 * one straddle byte past the row) and targets pixel_base +0x6cc with the
 * pitch/2 stride of the row index; the row count +0x10 is read once. */
// FUNCTION: LEGORR 0x004849d0
int flic_decode16_bit_keyframe(void* record)
{
    if (record == NULL) {
        return 1; /* deviation: the corpus strides the record blindly */
    }
    char* rec = (char*)record;
    void* stream = NULL;
    void* pixels = NULL;
    if (flic_window_find(rec, &stream, &pixels) == 0) {
        __builtin_memcpy(&stream, rec + 0x6d0, sizeof stream);
        __builtin_memcpy(&pixels, rec + 0x6cc, sizeof pixels);
    }
    if (stream == NULL || pixels == NULL) {
        return 1; /* deviation: the corpus strides the window blindly */
    }
    const char* cursor = (const char*)stream + 7;
    int16_t rows = 0;
    __builtin_memcpy(&rows, rec + 0x10, sizeof rows);
    if (rows > 0) {
        int pitch = 0;
        __builtin_memcpy(&pitch, rec + 0x638, sizeof pitch);
        uint32_t row_i = 0u;
        for (;;) {
            int16_t left = 0; /* the corpus re-reads the length each row */
            __builtin_memcpy(&left, rec + 0x0c, sizeof left);
            char* pixel = (char*)pixels
                          + (size_t)(((uint32_t)pitch >> 1) * (uint32_t)row_i
                                     * 2u);
            while (left > 0) {
                uint16_t w = 0u;
                __builtin_memcpy(&w, cursor, sizeof w);
                const uint32_t count = (uint8_t)w;
                if (count < 0x80u) {
                    uint16_t v = 0u;
                    __builtin_memcpy(&v, cursor + 1, sizeof v);
                    left = (short)(left - (short)count);
                    cursor += 3;
                    if ((uint8_t)w != 0u) {
                        for (uint32_t k = count; k != 0u; k = k - 1u) {
                            const uint16_t px = expand16_word(rec, v);
                            __builtin_memcpy(pixel, &px, sizeof px);
                            pixel += 2;
                        }
                    }
                } else {
                    const uint32_t run = 0x100u - count;
                    left = (short)(left - (short)(uint16_t)run);
                    cursor += 1;
                    const char* read = cursor;
                    for (uint32_t k = run; k != 0u; k = k - 1u) {
                        uint16_t v = 0u;
                        __builtin_memcpy(&v, read, sizeof v);
                        read += 2;
                        const uint16_t px = expand16_word(rec, v);
                        __builtin_memcpy(pixel, &px, sizeof px);
                        pixel += 2;
                    }
                    cursor = read;
                }
            }
            cursor += 1; /* the row straddle */
            row_i = row_i + 1u;
            if (!((int)row_i < (int)rows)) {
                break;
            }
        }
    }
    return 1;
}

/* FlicDecode16BitDelta (FUN_00484e50) — see flic.hpp. The chunk walk over
 * the word stream at stream+8: 0xC-type words retire rows by subtracting
 * the whole word, 8-type chunks remember one expanded pixel and land it as
 * a lone BYTE store at the walk's end, and 4-type ends the record. The
 * pair words advance the target by the pair's low byte and expand their
 * own words (or a repeated one on the back-count quirk). */
// FUNCTION: LEGORR 0x00484e50
int flic_decode16_bit_delta(void* record)
{
    if (record == NULL) {
        return 1; /* deviation: the corpus strides the record blindly */
    }
    char* rec = (char*)record;
    void* stream = NULL;
    void* pixels = NULL;
    if (flic_window_find(rec, &stream, &pixels) == 0) {
        __builtin_memcpy(&stream, rec + 0x6d0, sizeof stream);
        __builtin_memcpy(&pixels, rec + 0x6cc, sizeof pixels);
    }
    if (stream == NULL || pixels == NULL) {
        return 1; /* deviation: the corpus strides the window blindly */
    }
    int pitch = 0;
    __builtin_memcpy(&pitch, rec + 0x638, sizeof pitch);
    const char* cursor = (const char*)stream + 8;
    int16_t left = 0;
    __builtin_memcpy(&left, (const char*)stream + 6, sizeof left);
    uint16_t row = 0u; /* the corpus counter is a ushort (quirk-pinned) */
    for (;;) {
        if (left == 0) {
            return 1;
        }
        uint16_t w = 0u;
        __builtin_memcpy(&w, cursor, sizeof w);
        char* pixel = (char*)pixels
                      + (size_t)((uint32_t)row * (uint32_t)pitch);
        const int16_t kind = (int16_t)((w & 0xc000u) >> 12);
        if (kind == 0xc) {
            cursor += 2;
            row = (uint16_t)(row - w); /* whole-word subtract (corpus quirk) */
            continue;
        }
        if (kind == 4) {
            return 1;
        }
        bool remembered = false;
        uint16_t remember = 0u;
        if (kind == 8) {
            int flag = 0;
            __builtin_memcpy(&flag, rec + 0x6e4, sizeof flag);
            if (flag != 0) {
                w = (uint16_t)(((unsigned)w >> 1) & 0x7ff0u) | (uint16_t)(w & 0x1fu);
            }
            cursor += 2;
            remembered = true;
            remember = w;
        }
        uint16_t pairs = 0u;
        __builtin_memcpy(&pairs, cursor, sizeof pairs);
        cursor += 2;
        if (pairs != 0u) {
            uint32_t keep = pairs;
            const char* pair = cursor;
            do {
                uint8_t b1 = 0;
                uint16_t w1 = 0u;
                __builtin_memcpy(&w1, pair, sizeof w1);
                b1 = (uint8_t)(((uint32_t)w1) >> 8);
                const uint8_t skip = (uint8_t)w1;
                pixel += (size_t)skip * 2u;
                const char* next = pair + 2;
                if ((b1 & 0x80u) == 0u) {
                    if (b1 != 0u) {
                        for (uint32_t k = (uint32_t)b1; k != 0u; k = k - 1u) {
                            uint16_t v = 0u;
                            __builtin_memcpy(&v, next, sizeof v);
                            next += 2;
                            const uint16_t px = expand16_word(rec, v);
                            __builtin_memcpy(pixel, &px, sizeof px);
                            pixel += 2;
                        }
                    }
                } else {
                    uint16_t v = 0u;
                    __builtin_memcpy(&v, next, sizeof v);
                    next = pair + 4; /* (the corpus's 0x100 - uVar7 quirk) */
                    const uint32_t run = 0x100u - (uint32_t)b1;
                    if ((short)run != 0) {
                        for (uint32_t k = run; k != 0u; k = k - 1u) {
                            const uint16_t px = expand16_word(rec, v);
                            __builtin_memcpy(pixel, &px, sizeof px);
                            pixel += 2;
                        }
                    }
                }
                pair = next;
                keep = keep - 1u;
            } while (keep != 0u);
            cursor = pair;
        }
        if (remembered) {
            const uint8_t low = (uint8_t)remember;
            *pixel = (char)low;
        }
        row = (uint16_t)(row + 1u);
        left = (short)(left - 1);
    }
}

void flic_frame_window_layout_install(void* rec, int row_len, int row_count,
                                      int pitch, void* stream,
                                      void* pixel_buffer, int flag)
{
    if (rec == NULL) {
        return;
    }
    int16_t v = (int16_t)row_len;
    __builtin_memcpy((char*)rec + 0x0c, &v, sizeof v);
    v = (int16_t)row_count;
    __builtin_memcpy((char*)rec + 0x10, &v, sizeof v);
    __builtin_memcpy((char*)rec + 0x638, &pitch, sizeof pitch);
    const uint32_t lo = (uint32_t)(uintptr_t)stream;
    const uint32_t lp = (uint32_t)(uintptr_t)pixel_buffer;
    __builtin_memcpy((char*)rec + 0x6d0, &lo, sizeof lo);
    __builtin_memcpy((char*)rec + 0x6cc, &lp, sizeof lp);
    __builtin_memcpy((char*)rec + 0x6e4, &flag, sizeof flag);

    int slot = -1;
    for (int i = 0; i != 16; ++i) {
        if (g_flic_windows[i].rec == rec) {
            slot = i;
            break;
        }
        if (slot < 0 && g_flic_windows[i].rec == NULL) {
            slot = i;
        }
    }
    if (slot < 0) {
        slot = (int)(g_flic_window_next++ % 16u);
    }
    g_flic_windows[slot].rec = rec;
    g_flic_windows[slot].stream = stream;
    g_flic_windows[slot].pixels = pixel_buffer;
}

/* FlicDecode8BitAlternateDelta (FUN_00484ca0) — see flic.hpp. The opcode-27
 * 8-bit sibling of flic_decode16_bit_delta: the chunk stream rides stream+8
 * and the header re-read quirk repeats (kind 0 doubles the header as the
 * pair count, kind 8 advances the cursor), but the pixels land through the
 * +0x41c indexed-word palette — each pixel is a BYTE PAIR expanded to two
 * palette words, and the 0x100 - n back-count repeats the one pair that
 * trails the pair word. Deviations: the corpus strides the record blindly
 * (guarded here, like the rest of the band), and the corpus's zero-argument
 * callee calls leave the record pointer in a register — the port threads
 * it through, the way the window-slot fix must for the whole band. */
// FUNCTION: LEGORR 0x00484ca0
int flic_decode8_bit_alternate_delta(void* record)
{
    if (record == NULL) {
        return 1; /* deviation: the corpus strides the record blindly */
    }
    char* rec = (char*)record;
    void* stream = NULL;
    void* pixels = NULL;
    if (flic_window_find(rec, &stream, &pixels) == 0) {
        __builtin_memcpy(&stream, rec + 0x6d0, sizeof stream);
        __builtin_memcpy(&pixels, rec + 0x6cc, sizeof pixels);
    }
    if (stream == NULL || pixels == NULL) {
        return 1; /* deviation: the corpus strides the window blindly */
    }
    const char* cursor = (const char*)stream + 8;
    int16_t left = 0;
    __builtin_memcpy(&left, (const char*)stream + 6, sizeof left);
    int16_t lap = left; /* the pair walk restores this, the tail walks it */
    uint32_t row = 0u; /* the row counter: masked ushort at the multiply */
    int pitch = 0;
    __builtin_memcpy(&pitch, rec + 0x638, sizeof pitch);
    for (;;) {
        int16_t kind = 0;
        char* pixel = NULL;
        uint16_t header = 0u;
        for (;;) {
            if (lap == 0) {
                return 1;
            }
            __builtin_memcpy(&header, cursor, sizeof header);
            pixel = (char*)pixels
                    + (size_t)((row & 0xffffu) * (uint32_t)pitch);
            kind = (int16_t)((header & 0xc000u) >> 12);
            if (kind != 0xc) {
                break;
            }
            cursor += 2;
            row = (uint32_t)(row - header); /* the whole-word subtract */
        }
        if (kind == 4) {
            return 1;
        }
        uint8_t remembered = 0u;
        if (kind == 8) {
            remembered = (uint8_t)header; /* NO flag fold here */
            cursor += 2;
        }
        uint16_t pairs = 0u;
        __builtin_memcpy(&pairs, cursor, sizeof pairs);
        cursor += 2;
        if (pairs != 0u) {
            uint32_t count = pairs;
            const char* pair = cursor;
            do {
                uint16_t pw = 0u;
                __builtin_memcpy(&pw, pair, sizeof pw);
                const uint8_t skip = (uint8_t)pw;
                const uint8_t run = (uint8_t)(pw >> 8);
                cursor = pair + 2;
                pixel += (size_t)skip * 2u; /* WORD units */
                if ((run & 0x80u) == 0u) {
                    if (run != 0u) {
                        for (uint32_t k = (uint32_t)run; k != 0u; k = k - 1u) {
                            uint16_t v = 0u;
                            __builtin_memcpy(&v, cursor, sizeof v);
                            cursor += 2;
                            const uint16_t lo = indexed_word_table_lookup(
                                (uint8_t)v, rec);
                            const uint16_t hi = indexed_word_table_lookup(
                                (uint8_t)(v >> 8), rec);
                            __builtin_memcpy(pixel, &lo, sizeof lo);
                            __builtin_memcpy(pixel + 2, &hi, sizeof hi);
                            pixel += 4;
                        }
                    }
                } else {
                    uint16_t v = 0u;
                    __builtin_memcpy(&v, pair + 2, sizeof v);
                    cursor = pair + 4; /* the corpus resumes four bytes on */
                    const uint32_t back = 0x100u - (uint32_t)run;
                    if ((short)(0x100u - (uint32_t)run) != 0) {
                        for (uint32_t k = back; k != 0u; k = k - 1u) {
                            const uint16_t lo = indexed_word_table_lookup(
                                (uint8_t)v, rec);
                            const uint16_t hi = indexed_word_table_lookup(
                                (uint8_t)(v >> 8), rec);
                            __builtin_memcpy(pixel, &lo, sizeof lo);
                            __builtin_memcpy(pixel + 2, &hi, sizeof hi);
                            pixel += 4;
                        }
                    }
                }
                count = count - 1u;
                pair = cursor;
                lap = left;
            } while (count != 0u);
        }
        if (kind == 8) {
            const uint16_t w = indexed_word_table_lookup(remembered, rec);
            __builtin_memcpy(pixel, &w, sizeof w); /* ONE word, not a byte */
        }
        left = (int16_t)(lap - 1);
        row = (uint32_t)(row + 1u);
        lap = left;
    }
}

/* FlicOpcode25DecodeDispatch (FUN_00484b20) — see flic.hpp. The dispatch
 * words ride the record window: bit depth +0x64c, mode op +0x6d4, marker
 * word +0x644. The corpus loads the depth once into ax and gates both the
 * 8 and the 16 routes from it, reloads the op dword raw for the mode-4
 * gate, and compares the op and the marker straight against memory (cmp
 * DWORD PTR,4 / cmp WORD PTR,0xaf43) — plain typed loads, no staging.
 * Deviation: the corpus's keyframe call passes no argument (the record
 * pointer rides the register) — the port threads the record through, as
 * the window band's precedent requires. */
// FUNCTION: LEGORR 0x00484b20
int flic_opcode25_decode_dispatch(void* record)
{
    if (record == NULL) {
        return 1; /* pinned deviation: the corpus strides the record blindly */
    }
    char* rec = (char*)record;
    const uint16_t depth = *(const uint16_t*)(rec + 0x64c); /* ax, once */
    if (depth == 8u) {
        if (*(const uint32_t*)(rec + 0x6d4) == 0u) {
            flic_opcode25_success_stub();
        }
        if (*(const uint32_t*)(rec + 0x6d4) == 4u) { /* corpus: cmp mem,4 */
            flic_decode8_bit_opcode25_mode4(rec);
            return 1;
        }
    } else if (depth == 0x10u) {
        if (*(const uint16_t*)(rec + 0x644) == 0xaf43u) { /* cmp mem,0xaf43 */
            flic_decode16_bit_keyframe(rec);
            return 1;
        }
        flic_opcode25_success_stub();
    }
    return 1;
}

/* FlicOpcode27DecodeDispatch (FUN_00485030) — see flic.hpp. The sibling's
 * dispatch: the corpus loads the depth once into ax and gates both the 8
 * and the 16 routes from it, compares the op dword straight against memory
 * (cmp DWORD PTR,4), and loads the marker WORD only on the 16-bit route and
 * reuses ax for both gates (cmp ax,0x1234 / cmp ax,0xaf43) — plain typed
 * loads, no staging. The literal 0x1234 goes to the fallback, and so does a
 * mismatch; the 8-bit op-4 route lands in the alternate delta. Deviations
 * ride the same two notes as flic_opcode25_decode_dispatch. */
// FUNCTION: LEGORR 0x00485030
int flic_opcode27_decode_dispatch(void* record)
{
    if (record == NULL) {
        return 1; /* pinned deviation: the corpus strides the record blindly */
    }
    char* rec = (char*)record;
    const uint16_t depth = *(const uint16_t*)(rec + 0x64c); /* ax, once */
    if (depth == 8u) {
        if (*(const uint32_t*)(rec + 0x6d4) == 0u) {
            flic_opcode27_fallback_noop();
        }
        if (*(const uint32_t*)(rec + 0x6d4) == 4u) { /* corpus: cmp mem,4 */
            flic_decode8_bit_alternate_delta(rec);
            return 1;
        }
    } else if (depth == 0x10u) {
        const uint16_t marker = *(const uint16_t*)(rec + 0x644); /* ax here */
        if (marker == 0x1234u) {
            flic_opcode27_fallback_noop();
            return 1;
        }
        if (marker == 0xaf43u) { /* cmp mem,0xaf43 */
            flic_decode16_bit_delta(rec);
            return 1;
        }
        flic_opcode27_fallback_noop();
    }
    return 1;
}
