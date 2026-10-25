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
};

struct Flic {
    int width, height;
    int frames;       /* header nFrames + 1 */
    int idx;          /* current frame (0 = keyframe) */
    int eof;
    FlicChunk* chunks; /* chunks[0] = keyframe, [1..] = deltas/holds */
    int chunk_n;
    uint8_t* raw;     /* owned copy of the whole file */
    size_t   raw_len;
    uint16_t* pix;    /* RGB555 frame buffer */
    uint8_t* rgba;    /* lazy RGBA32 conversion */
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
static void fli_decode_chunk(Flic* f, const FlicChunk* ch)
{
    if (ch->count == 0) {
        return; /* hold: pixels unchanged */
    }
    size_t pos = FLIC_CHUNK_HDR; /* subrecords start at chunk + 0x10 */
    for (uint16_t r = 0; r < ch->count; ++r) {
        if (pos + 10 > ch->size) {
            break; /* truncated record header */
        }
        const uint8_t* rec = ch->data + pos;
        uint32_t rec_size = le32(rec);
        uint16_t opcode = le16(rec + 4);
        if (rec_size < 8 || pos + rec_size > ch->size) {
            break; /* record overruns its chunk — bounded */
        }
        const uint8_t* body = rec + ((opcode == 25) ? 7 : 8);
        size_t blen = rec_size - ((opcode == 25) ? 7u : 8u);
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
}

/* --- open / close --------------------------------------------------------- */

Flic* flic_open(const char* name)
{
    if (name == NULL || name[0] == '\0') {
        return NULL;
    }
    Resource* r = resource_open(name, RESOURCE_AUTO);
    if (r == NULL) {
        /* Loose `Data/AVI/...` overlay copies (avi\PB1.flh etc.) resolve
         * through the movie path helper (case-insensitive, base = game dir). */
        char norm[256];
        size_t i = 0;
        for (; i < sizeof(norm) - 1 && name[i] != '\0'; ++i) {
            norm[i] = (name[i] == '\\') ? '/' : name[i];
        }
        norm[i] = '\0';
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

    /* Header (finding 21 §2). */
    uint32_t filesize = le32(d + 0x00);
    uint16_t magic = le16(d + 0x04);
    uint16_t nframes = le16(d + 0x06);
    uint16_t width = le16(d + 0x08);
    uint16_t height = le16(d + 0x0a);
    uint16_t depth = le16(d + 0x0c);
    if ((uint32_t)n != filesize || depth != 16 || width == 0 || height == 0
        || width > 4096 || height > 4096) {
        fprintf(stderr, "flic: %s: bad header (size %d vs %d, depth %d)\n",
                name, (int)filesize, n, depth);
        resource_free(r);
        return NULL;
    }
    if (magic != FLIC_MAGIC_AF43 && magic != 0x1234 && magic != 0x9119
        && magic != 0x911a && magic != 0xaf11 && magic != 0xaf12) {
        fprintf(stderr, "flic: %s: unrecognized magic 0x%04x\n", name, magic);
        resource_free(r);
        return NULL;
    }

    Flic* f = (Flic*)calloc(1, sizeof(*f));
    if (f == NULL) {
        resource_free(r);
        return NULL;
    }
    f->width = width;
    f->height = height;
    f->frames = nframes + 1; /* chunk count = nFrames + 1, finding 21 §2 */
    f->raw = (uint8_t*)malloc((size_t)n);
    f->pix = (uint16_t*)malloc((size_t)width * height * 2);
    f->chunks = (FlicChunk*)calloc((size_t)f->frames, sizeof(FlicChunk));
    if (f->raw == NULL || f->pix == NULL || f->chunks == NULL) {
        flic_close(f);
        resource_free(r);
        return NULL;
    }
    memcpy(f->raw, d, (size_t)n);
    f->raw_len = (size_t)n;
    resource_free(r);

    /* Chunk walk from 0x80 (finding 21 §3). */
    size_t pos = FLIC_CHUNK_START;
    int cn = 0;
    while (pos + FLIC_CHUNK_HDR <= f->raw_len && cn < f->frames) {
        const uint8_t* ch = f->raw + pos;
        uint32_t csize = le32(ch);
        if (csize < FLIC_CHUNK_HDR || pos + csize > f->raw_len) {
            break; /* truncated tail — the header nFrames is authoritative */
        }
        f->chunks[cn].data = ch;
        f->chunks[cn].size = csize;
        f->chunks[cn].count = le16(ch + 6);
        ++cn;
        pos += csize;
    }
    f->chunk_n = cn;

    /* Frame 0 = keyframe, decoded at open. */
    fli_decode_chunk(f, &f->chunks[0]);
    f->idx = 0;
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
    free(f);
}

int flic_tick_loop(Flic* f, int loop)
{
    if (f == NULL) {
        return 0;
    }
    int next = f->idx + 1;
    if (next >= f->chunk_n) {
        if (!loop) {
            f->eof = 1;
            return f->idx; /* stay on the last frame */
        }
        next = 1; /* corpus RAM loop: keyframe already decoded */
    }
    f->idx = next;
    f->eof = 0;
    fli_decode_chunk(f, &f->chunks[next]);
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
uint32_t flic_opcode25_success_stub(void)
{
    return 1u;
}

void flic_opcode27_fallback_noop(void)
{
    /* FUN_00484c90: pure return — faithful. */
}

/* FlicOpcode16ModeDispatch (FUN_00484880) — see flic.hpp. The corpus reads
 * the dispatch words from its frame record; the port takes the record as a
 * byte window (the corpus buffer shape). The fallback callee is the pure
 * no-op the port already models, so every branch observes as 1. */
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
