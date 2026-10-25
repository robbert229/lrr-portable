// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — FLIC `.flh` overlay animation player (finding 21).
 *
 * The `.flh` files (18 in the shipped wads — `Interface\Pointers\...`,
 * `Interface\InfoPanel\HOLO.FLH`, `Interface\Reward\CAPTAIN/CAPTY.FLH` — and
 * 11 loose `Data/AVI` copies used by the front-end `!Overlay` entries)
 * are FLIC-family containers decoded by the corpus's private player
 * (MoviePlayWait @ 0x415700, the RAM/stream players FUN_00483c80 /
 * FUN_00484070): a 128-byte FLIC header, then a stream of 16-byte chunks
 * from 0x80, each a keyframe (opcode 25, FLI16 byte-RLE) plus nFrames delta
 * frames (opcode 27, word-based) with zero-sized hold chunks marking
 * repeated frames. Pixels are RGB555. Finding 21 closed the format
 * byte-exact (18/18 files, 469/469 chunks) and validated the coder model;
 * this module is the port's runtime player over that model.
 *
 * API shape mirrors the corpus: flic_open (= FUN_00483c80), flic_tick
 * (advance one frame, = the frame steppers 0x4841d0/0x484260),
 * flic_close. The decoded frame keeps the corpus's RGB555 storage and is
 * presented to the renderer as RGBA32.
 */
#ifndef LEGORRR_FLIC_HPP
#define LEGORRR_FLIC_HPP

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Flic Flic;

/* Open a `.flh` by resource name (wad entry like `Interface\Pointers\
 * HAMMER.FLH`, or a loose `Data/AVI/...` path like `avi\PB1.flh`, resolved
 * case-insensitively through the movie path helper). Returns NULL on open/
 * validation failure (magic not in the accepted set, depth != 16, or the
 * chunk stream does not parse). The keyframe is decoded at open, so
 * flic_tick starts at frame 1. */
Flic* flic_open(const char* name);

void flic_close(Flic* f);

/* --- geometry / inventory ------------------------------------------------- */

int flic_width(const Flic* f);
int flic_height(const Flic* f);
/* Total frames = header nFrames + 1 (the keyframe). */
int flic_frames(const Flic* f);
/* Current frame index (0 = keyframe), advanced by flic_tick. */
int flic_frame(const Flic* f);
/* 1 after flic_tick stepped past the last frame (loop-able). */
int flic_eof(const Flic* f);

/* --- stepping -------------------------------------------------------------- */

/* Advance one frame: applies the next delta record, or repeats the current
 * frame on a hold chunk. If `loop` is set, wrapping past the last frame
 * restarts at index 1 (the keyframe is already decoded — the corpus's RAM
 * mode rule). Returns the new frame index. */
int flic_tick_loop(Flic* f, int loop);

/* --- pixels ----------------------------------------------------------------- */

/* The current frame in the corpus's RGB555 storage: width*height u16, row
 * pitch = width*2. */
const uint16_t* flic_frame555(const Flic* f);

/* The current frame converted to RGBA32 (byte order R,G,B,A on LE, A = 0xff),
 * row pitch = width*4. Owned by the module; valid until the next tick. */
const uint8_t* flic_frame_rgba(Flic* f);

/* RGB555 -> RGBA32 expansion of one pixel (the renderer's conversion). */
uint32_t flic_rgb555_to_rgba(uint16_t c);

/* FlicOpcode25SuccessStub (FUN_00484b90, finding 21): the 8-bit
 * opcode-25 handler — a six-byte success-returning stub, no pixel
 * writes (unused in the corpus; the 16-bit coder above owns all shipped
 * files). Corpus arguments are ignored; definition stays void. */
uint32_t flic_opcode25_success_stub(void);

#ifdef __cplusplus
}
#endif

/* FlicOpcode27FallbackNoop (FUN_00484c90, finding 155): the corpus's
 * fallback for unknown opcode 27 records is a pure return — faithful. */
void flic_opcode27_fallback_noop(void);

/* FlicOpcode16ModeDispatch (FUN_00484880): mode-word dispatch over the
 * frame-record window (+0x6d4 opcode, +0x6d8 mode word). Mode 0x10 takes
 * the fallback and returns; opcodes 0 and 4 run the fallback in place;
 * all routes answer 1. Deviation: the corpus reads its own frame buffer
 * blindly; the port guards the NULL window. */
int flic_opcode16_mode_dispatch(const uint8_t* rec);

/* FlicDecode16BitRowRecords (FUN_00484c00): the chunked triplet writer on
 * the corpus frame-record window — stream link +0x6d0, count stream+6,
 * bytes stream+8, triplets +0x11c (skip byte*3 per chunk; a zero count byte
 * restarts the table for one 0x100 pass and ends the walk), and op +0x6d4
 * 4 hands off to the triplet fold.
 * FlicDecode8BitOpcode25Mode4 (FUN_004848d0): the byte stream at stream+7
 * over rows +0x10 of length +0xc, pixel stores at +0x6cc plus pitch/2
 * * row * 2, palette stores through the +0x41c word window. Each row pass
 * ends one byte short of the stream (the corpus steps +1 between rows).
 * Deviation: the corpus strides the record, stream, and pixels blindly. */
int flic_decode16_bit_row_records(void* record);
int flic_decode8_bit_opcode25_mode4(void* record);
void flic_frame_window_layout_install(void* rec, int row_len, int row_count,
                                     int pitch, void* stream,
                                     void* pixel_buffer, int flag);

/* PackedTripletTableToWordBuild (FUN_00484ba0): fold the record's 256
 * packed RGB triplets at +0x11c into 16-bit words at +0x41c, then clear
 * the first word. The record rides the frame-record byte window. */
void flic_packed_triplet_table_to_word_build(void* record);

#endif /* LEGORRR_FLIC_HPP */