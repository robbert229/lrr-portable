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
 * Device-mode word (_DAT_004ffbc0): 1/2/4 = alternative device modes, with
 * per-mode enter/leave counters used by the corner-dwell quit machine
 * (QuitPoll FUN_004589c0). The OS-level *writer* of that word (device
 * capture, finding 05's still-open writer region 0x457d7e-0x459157) lands
 * with the frame-loop milestone; this module models the state + consumers.
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

    uint32_t device_mode;             /* _DAT_004ffbc0 (0..4) */
    uint32_t dev_enter[5];            /* DAT_004ffdd8 per-mode enter counts */
    uint32_t dev_leave[5];            /* DAT_004ffdf8 per-mode leave counts */

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
 * current device-mode slot; returns 1 when the gesture completes (balanced
 * counters and the mouse has moved past the quit rectangle) AND keep_running
 * is 0 — mirroring quitFlag = (KeepRunning() == 0). Returns 0 otherwise. */
int input_quit_dwell(InputState* s, int mx, int my,
                     int rect_x, int rect_y, int rect_w, int rect_h,
                     int quit_w, int quit_h, int keep_running);

#endif /* LEGORRR_INPUT_HPP */