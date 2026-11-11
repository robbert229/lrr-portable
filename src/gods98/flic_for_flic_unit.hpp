// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private (ADR 0006 decision 4; #496): what flic_unit.cpp sees from
 * flic.cpp. A verbatim snapshot of flic_shared.hpp as it was when the
 * #496 migration froze it, so the unit's declarations no longer change
 * with later relocations. Never edited by tools/relocate_unit.py. */
#ifndef LRR_GODS98_FLIC_FOR_FLIC_UNIT_HPP
#define LRR_GODS98_FLIC_FOR_FLIC_UNIT_HPP

#define FLIC_HEADER 136      /* the engine's read size (context +0x640) */

#define FLIC_CHUNK_START 0x80 /* frame data starts at file offset 0x80 (§2) */

#define FLIC_CHUNK_HDR 16

struct FlicChunk {
    const uint8_t* data; /* full RAM chunk, or stream body when body_only */
    uint32_t       size; /* chunk size, incl. header */
    uint16_t       count; /* subrecord count; 0 = hold chunk */
    uint16_t       type;
    int            body_only; /* streamed frame body omits its 16-byte header */
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
    uint32_t stream_buffer_allocated;
    uint32_t stream_buffer_capacity;
    uint32_t stream_frame_count;
    int stream_loop_enabled;
    FlicChunk stream_chunk;
    uint8_t stream_header[FLIC_CHUNK_HDR];
    size_t stream_loop_cursor;
    void (*stream_context_close)(void* context);
};

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

void flic_packed_triplet_table_to_word_build_target(void* record);
uint16_t le16(const uint8_t* p);
uint32_t le32(const uint8_t* p);
int flic_resource_route_resolve(const char* name, const char** open_path, int* resource_mode, int* route);
extern flic_window_slot g_flic_windows[16];

#endif /* LRR_GODS98_FLIC_FOR_FLIC_UNIT_HPP */
