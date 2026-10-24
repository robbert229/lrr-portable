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

#endif /* LEGORRR_FLIC_HPP */