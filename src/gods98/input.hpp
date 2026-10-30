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
 * and a port-local ASCII helper, then clears the tables with the mirror trick.
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
 *  - input_key_to_ascii is a port-local ASCII helper. The reference entry at
 *    0x48e380 is CRT `_toupper`; DAT_00549180 is a CRT locale handle, not a
 *    keyboard-layout handle.
 *  - The KeyContextCheck filter table is reconstruction-owned (the shipped
 *    256x4 table in .rdata is not embedded); default = all entries zero
 *    ("no restriction").
 */
#ifndef LEGORRR_INPUT_HPP
#define LEGORRR_INPUT_HPP

#include <cstdint>
#include <cstddef>

#include "platform/platform.h"  /* PlatformEvent + canonical key codes */

#define INPUT_KEY_COUNT 256

typedef struct InputState {
    uint8_t  cur[INPUT_KEY_COUNT];    /* DAT_0076ab80 */
    uint8_t  prev[INPUT_KEY_COUNT];   /* DAT_0076aa00 */
    uint8_t  mirror[INPUT_KEY_COUNT]; /* DAT_004de430 */

    int32_t  mouse_x;                 /* DAT_0076ab24 */
    int32_t  mouse_y;                 /* DAT_0076ab28 */
    int      mouse_press;             /* latched left-press edge (cleared by
                                       * input_begin_frame) */
    int      mouse_down;              /* level state: 1 while a button is held
                                       * (corpus mouse-button-held semantics;
                                       * not cleared per frame) */

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

/* Device capture — the "writer" half (finding 05's device layer, deferred
 * from PR #8 to the frame loop). Consumes a batch of platform events into
 * the current-frame tables: KEYDOWN sets cur[code], KEYUP clears,
 * MOUSE updates mouse_x/y. Canonical codes come from platform.h. */
void input_capture_events(InputState* s, const PlatformEvent* evs, int count);

/* EnginePreTick tail: snapshot cur into the text mirror (cur kept for the
 * tick's MainSimStep edges; the frame boundary clears it via
 * input_begin_frame at the end of the loop). */
void input_mirror(InputState* s);

/* Snapshot cur into the text mirror and clear cur. */
void input_mirror_and_clear(InputState* s);

int  input_key_down(const InputState* s, int key);
int  input_key_edge(const InputState* s, int key);
int  input_key_released(const InputState* s, int key);

/* Left-button press edge latched this frame (consumed by input_begin_frame).
 * Returns 1 for one frame after the platform reports MOUSE + button=1. */
int  input_mouse_pressed(const InputState* s);

/* Port-local key-to-ASCII helper: uppercase ASCII lowercase letters and
 * return other values unchanged. The reference 0x48e380 entry is CRT
 * `_toupper`, represented separately in src/crt. */
uint32_t input_key_to_ascii(const InputState* s, uint32_t scancode);

/* KeyContextCheck (FUN_00411f00): filter[key]; 0 = unrestricted. */
uint32_t input_key_context(const InputState* s, uint32_t key);

/* EnginePreTick text pump: for each key edge allowed by the context filter,
 * append its ASCII character to buf (capacity cap); when `clear` is set the
 * mirror/+clear tail runs (corpus EnginePreTick), otherwise only the mirror
 * (frame-loop model: the frame boundary clears). Advances *len_out. */
void input_text_pump(InputState* s, char* buf, int cap, int* len_out,
                     int clear);

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

/* --- input-entry / player-input flag-8 leaves (2026-10-24) ----------------
 * InputEntryFlag8SetClear / InputEntryFlag8Get (FUN_0041c9f0 / FUN_0041ca60,
 * findings 376/444): raise/clear/read bit 0x8 of the corpus's indexed
 * input-entry flag word. PlayerInputFlag8SetClear / PlayerInputFlag8Get
 * (FUN_0041ca20 / FUN_0041ca70): the same bit over the two player-input
 * tables selected by kind 1 / kind 4. None of the corpus's table owners
 * (the pointer-table allocator and the action-state leaves, still
 * unported) lands, so all three tables land zero-initialized with a
 * documented 256-entry window (the corpus's own sizes come from heap
 * allocations; the port bounds-checks, answering zero outside the window
 * and skipping writes — deviation). */
void input_entry_flag8_set_clear(int entry, int set);
uint32_t input_entry_flag8_get(int entry);
void player_input_flag8_set_clear(int kind, int action, int set);
uint32_t player_input_flag8_get(int kind, int action);

struct EngineContext;  /* engine.hpp: the DAT_00556ecc state word lives here */

/* InputActionPressedGet / InputBindingPressedGet (FUN_0041c8f0 /
 * FUN_0041c950): "is this action/binding pressed right now?" Both read the
 * engine-state word (DAT_00556ecc) bit 0x10000000 as a "pressed-yes-every-
 * thing" gate — NOT the 0x40000000 RuntimeFlag40000000Get mask — and both
 * consult the system-state word (DAT_004ff980) bit 0 for the inverted
 * alternate reading. Action 0x49 is hard-wired not-pressed. The corpus
 * indexes its table dwords blindly (the port bounds-checks, answering
 * zero outside the window — deviation). */
int input_action_pressed_get(const struct EngineContext* ctx, int action,
                             int alt);
int input_binding_pressed_get(const struct EngineContext* ctx, int kind,
                              int entry, int alt);

/* IndexedEntityValueGet (FUN_0041cab0): the indexed binding value — the
 * pressed gate (FUN_0041c8f0) picks the DAT_004dd120 window, the flag-8
 * entry reader (FUN_0041ca60) the DAT_004dd248 window, and both answers
 * clear the DAT_004dcff8 window. All three windows are own storage (their
 * corpus writer is the recorded interface-images config seam); the hooks
 * install/peek them. */
uint32_t indexed_entity_value_get(const struct EngineContext* ctx, int index);
void     input_indexed_window_set(int which, int index, uint32_t value);
uint32_t input_indexed_window_peek(int which, int index);

#endif /* LEGORRR_INPUT_HPP */
