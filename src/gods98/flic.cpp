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
 * stream is preloaded at open and frames are decoded on tick. RAM loops keep
 * the decoded keyframe and restart at frame 1; streamed loops replay it.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flic.hpp"
#include "record.hpp"
#include "movie.hpp"
#include "resource.hpp"
#include "crt/crt.hpp"
#include "flic_shared.hpp"

#define FLIC_MAGIC_AF43 0xaf43
#define FLIC_STREAM_INITIAL_BUFFER 20000u

uint16_t le16(const uint8_t* p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

uint32_t le32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16)
           | ((uint32_t)p[3] << 24);
}

int flic_resource_route_resolve(const char* name,
                                       const char** open_path,
                                       int* resource_mode,
                                       int* route);

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

/* --- open / close --------------------------------------------------------- */

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
    if (f->pix == NULL) {
        flic_close(f);
        return NULL;
    }
    /* Retail opens the stream with a 20,000-byte scratch allocation; its
     * logical chunk capacity remains zero until a chunk handler grows it. */
    f->stream_buffer = (uint8_t*)malloc(FLIC_STREAM_INITIAL_BUFFER);
    if (f->stream_buffer == NULL) {
        flic_close(f);
        return NULL;
    }
    f->stream_buffer_allocated = FLIC_STREAM_INITIAL_BUFFER;
    flic_chunk_walk(f); /* target-compatible walker return is always zero */
    if (!f->last_walk_frame_valid) {
        flic_close(f);
        return NULL;
    }
    f->idx = 0;
    f->eof = 0;
    /* FUN_00484260 captures its loop mark before reading the first streamed
     * frame chunk. Stream loops therefore replay the keyframe at 0x80; only
     * RAM mode retains its decoded keyframe and restarts at frame 1. */
    f->stream_loop_cursor = FLIC_CHUNK_START;
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

/* FlicFileOpenAndStateCreate uses GameFileOpenDispatch, which selects a WAD
 * member before a same-named loose file. ResourceStream's AUTO mode prefers
 * loose files, so reproduce the dispatch decision before choosing its stream
 * mode and WAD-relative path. */
int flic_resource_route_resolve(const char* name,
                                       const char** open_path,
                                       int* resource_mode,
                                       int* route)
{
    const char* resolved;
    int selected;
    if (name == NULL || open_path == NULL || resource_mode == NULL ||
        route == NULL) return 0;
    resolved = resource_path_resolve_against_base(name);
    if (resolved == NULL) return 0;
    selected = game_file_open_route_select(resolved, "rb");
    if (selected == 0) {
        *open_path = game_file_mounted_path_prefix_rewrite(resolved);
        *resource_mode = RESOURCE_WAD_ONLY;
    }
    else if (selected == 1) {
        *open_path = resolved;
        /* AUTO preserves the loose-file/CD fallback after the WAD query has
         * already established that no mounted entry shadows this path. */
        *resource_mode = RESOURCE_AUTO;
    }
    else {
        return 0;
    }
    *route = selected;
    return *open_path != NULL;
}

Flic* flic_open_resource_stream(const char* name)
{
    ResourceStream* stream;
    FlicStreamOps ops;
    Flic* f;
    const char* open_path;
    int resource_mode;
    int route;
    uint32_t file_size = 0u;
    if (name == NULL || name[0] == '\0' ||
        !flic_resource_route_resolve(name, &open_path, &resource_mode,
                                     &route)) return NULL;
    stream = resource_stream_open(open_path, resource_mode, &file_size);
    if (stream == NULL && route == 1) {
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

/* Public adapter retains the null guard used by existing portable callers. */
void flic_packed_triplet_table_to_word_build(void* record)
{
    if (record != NULL) flic_packed_triplet_table_to_word_build_target(record);
}

/* --- the corpus frame-record window band (2026-10-25) ----------------------
 * These decoders run on the corpus's record buffer (the opcode-16 dispatch
 * and triplet-word precedent): triplets +0x11c, words +0x41c (the ported
 * indexed-word-table lookup window), stream link +0x6d0, pixel buffer +0x6cc,
 * pitch +0x638, row count +0x10, row length +0xc, op +0x6d4, format flag
 * +0x6e4. The stream chunks carry their count at stream+6 and bytes at
 * stream+8. The corpus strides all of it blindly; the port guards only the
 * record pointer (deviation, the dispatch precedent). */

flic_window_slot g_flic_windows[16];
static unsigned g_flic_window_next = 0u;

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

namespace unported_ledger_stubs {

int ledger_stub_00483f00_FlicPlayerClose(Flic* player)
{
    if (player == NULL) return 0;
    flic_close(player);
    return 1;
}

int ledger_stub_00483f60_FlicFileHeaderValidate(const char* path,
                                                int* context)
{
    return flic_file_header_validate_path(path, context);
}

} // namespace unported_ledger_stubs

/* --- Flic.c entry points in the original ABI (gods98/flic.h) --------------
 * Thin unannotated adapters (ADR 0005 step 6) for Panel.c (GAME-032,
 * finding 554) over the port's player. Drawing to the back buffer is the
 * renderer's, so Flic_Animate only advances the frame. */
#include "flic.h"

int flic_setup(const char* name, void** out, int flags)
{
    Flic* flic;
    (void)flags;
    flic = flic_open(name);
    *out = flic;
    return flic != NULL;
}

int flic_animate(void* flic, const struct Area2F* area, int advance,
                 int trans)
{
    (void)area;
    (void)trans;
    if (flic == NULL) return 0;
    if (advance) flic_tick_loop((Flic*)flic, 1);
    return 1;
}
