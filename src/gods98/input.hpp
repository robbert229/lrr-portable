// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — input core scaffold (findings 04/05).
 *
 * The corpus's input model is three 256-byte key tables, mouse x/y, and a
 * per-key "context filter" (findings 04/05):
 *
 *   cur    DAT_0076ab80  current frame snapshot (1 byte per key)
 *   prev   DAT_0076aa00  previous frame snapshot
 *   mirror DAT_004de430  copy made by EnginePreTick's text pump
 *   mouse  DAT_0076ab24/ab28
 *
 * Edge detection everywhere in the corpus is (cur != 0 && prev != cur), i.e.
 * a key that became non-zero this frame — nothing fancier. EnginePreTick
 * (findings 04/05) turns edges into text via KeyContextCheck (context filter)
 * + KeyToAscii and clears the tables with the mirror trick.
 *
 * QuitPoll's dwell machine (FUN_004589c0): corner-dwell quit gesture with
 * per-slot enter/leave counters. The original indexes those counters off a
 * word that finding 05 mislabeled an "input-device mode word"
 * (_DAT_004ffbc0, "1/2/4 = alternative device modes") — finding 22 corrects
 * it: that word is the **objective/advisor flag word** (0x1 pending / 0x2
 * achieved / 0x4 failed + config bits), touched only by the objective/
 * announcer functions. So the dwell slot is supplied by the caller (the
 * objective layer, finding 09/22), NOT picked from an input device here.
 *
 * Reconstruction notes:
 *  - KeyToAscii ports the no-HKL path exactly (DAT_00549180 == 0: a-z ->
 *    A-Z); the Win32 HKL/ToUnicodeEx path is structural only until a layout
 *    provider exists. Default key_to_ascii covers the common path.
 *  - The KeyContextCheck filter table is reconstruction-owned (the shipped
 *    256x4 table in .rdata is not embedded); default = all entries zero
 *    ("no restriction").
 */
#ifndef LEGORRR_INPUT_HPP
#define LEGORRR_INPUT_HPP

#include <cstdint>
#include <cstddef>

#define INPUT_KEY_COUNT 256

typedef struct InputState {
    uint8_t  cur[INPUT_KEY_COUNT];    /* DAT_0076ab80 */
    uint8_t  prev[INPUT_KEY_COUNT];   /* DAT_0076aa00 */
    uint8_t  mirror[INPUT_KEY_COUNT]; /* DAT_004de430 */

    int32_t  mouse_x;                 /* DAT_0076ab24 */
    int32_t  mouse_y;                 /* DAT_0076ab28 */

    /* Corner-dwell quit counters (QuitPoll FUN_004589c0). Slot 0..4 is
     * supplied by the caller — in the original it indexes off the
     * objective/advisor flag word, not an input device (finding 22). */
    uint32_t dwell_enter[5];          /* enter counts per slot */
    uint32_t dwell_leave[5];          /* leave counts per slot */

    uint8_t  hkl_present;             /* DAT_00549180 analog */
    uint32_t context_filter[INPUT_KEY_COUNT]; /* DAT_004a1f60 analog */

    char*    text_target;             /* active text-entry buffer, else NULL */
    int32_t  text_len;
    int32_t  text_cap;
} InputState;

void input_reset(InputState* s);

/* Device pre-tick (the corpus's pre-ticks FUN_0047eef0 / FUN_0047f010):
 * roll the current frame's keys into prev and clear cur for the device to
 * refill. Edges are then cur != 0 && prev == 0. */
void input_begin_frame(InputState* s);

/* EnginePreTick tail: snapshot cur into the text mirror and clear cur. */
void input_mirror_and_clear(InputState* s);

int  input_key_down(const InputState* s, int key);
int  input_key_edge(const InputState* s, int key);
int  input_key_released(const InputState* s, int key);

/* KeyToAscii (FUN_0048e380): without an HKL this is exactly "uppercase
 * a-z"; level-1 ASCII for keys 0x20..0x7e passes through unchanged. */
uint32_t input_key_to_ascii(const InputState* s, uint32_t scancode);

/* KeyContextCheck (FUN_00411f00): filter[key]; 0 = unrestricted. */
uint32_t input_key_context(const InputState* s, uint32_t key);

/* EnginePreTick text pump: for each key edge allowed by the context filter,
 * append its ASCII character to buf (capacity cap); then the mirror+clear
 * tail runs. Advances *len_out. */
void input_text_pump(InputState* s, char* buf, int cap, int* len_out);

/* QuitPoll machine (FUN_004589c0): corner-dwell quit gesture. Feeds the
 * mouse position against the corner rectangle; enter/leave counters on the
 * caller-supplied dwell `slot` (0..4; in the original the slot indexes off
 * the objective/advisor flag word `_DAT_004ffbc0` — finding 22, NOT input
 * devices); returns 1 when the gesture completes (dwelled in the corner,
 * then left past the quit rectangle) AND keep_running is 0 — mirroring
 * quitFlag = (KeepRunning() == 0). Returns 0 otherwise. */
int input_quit_dwell(InputState* s, int slot, int mx, int my,
                     int rect_x, int rect_y, int rect_w, int rect_h,
                     int quit_w, int quit_h, int keep_running);

#endif /* LEGORRR_INPUT_HPP */