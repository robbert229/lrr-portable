// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>

#include "input.hpp"

void input_reset(InputState* s)
{
    if (s == NULL) {
        return;
    }
    memset(s->cur, 0, sizeof(s->cur));
    memset(s->prev, 0, sizeof(s->prev));
    memset(s->mirror, 0, sizeof(s->mirror));
    memset(s->dev_enter, 0, sizeof(s->dev_enter));
    memset(s->dev_leave, 0, sizeof(s->dev_leave));
    s->mouse_x = 0;
    s->mouse_y = 0;
    s->device_mode = 0;
    s->hkl_present = 0;
    memset(s->context_filter, 0, sizeof(s->context_filter));
    s->text_target = NULL;
    s->text_len = 0;
    s->text_cap = 0;
}

void input_begin_frame(InputState* s)
{
    if (s == NULL) {
        return;
    }
    for (int i = 0; i < INPUT_KEY_COUNT; ++i) {
        s->prev[i] = s->cur[i];
        s->cur[i] = 0;
    }
    /* mouse slots are read by the quit machine; they stay until cursor
     * capture overwrites them (device layer, frame-loop milestone). */
}

void input_mirror_and_clear(InputState* s)
{
    if (s == NULL) {
        return;
    }
    for (int i = 0; i < INPUT_KEY_COUNT; ++i) {
        s->mirror[i] = s->cur[i];
        s->cur[i] = 0;
    }
}

int input_key_down(const InputState* s, int key)
{
    return (key >= 0 && key < INPUT_KEY_COUNT) ? s->cur[key] != 0 : 0;
}

int input_key_edge(const InputState* s, int key)
{
    if (key < 0 || key >= INPUT_KEY_COUNT) {
        return 0;
    }
    return s->cur[key] != 0 && s->prev[key] == 0;
}

int input_key_released(const InputState* s, int key)
{
    if (key < 0 || key >= INPUT_KEY_COUNT) {
        return 0;
    }
    return s->prev[key] != 0 && s->cur[key] == 0;
}

/* KeyToAscii — corpus FUN_0048e380.
 *
 * Without an HKL (DAT_00549180 == 0, the shipped default) this is exactly:
 *   0x61..0x7a ('a'-'z') -> 0x41..0x5a ('A'-'Z')
 *   everything else returns the scancode unchanged.
 * With an HKL the original reaches into Win32 ToUnicodeEx via a layout
 * table; that path is structural here (returns the scancode unchanged)
 * until a keyboard-layout provider exists (frame-loop/OS milestone).
 */
uint32_t input_key_to_ascii(const InputState* s, uint32_t scancode)
{
    if (s == NULL || s->hkl_present == 0) {
        if (scancode > 0x60u && scancode < 0x7bu) {
            return scancode - 0x20u;
        }
        return scancode;
    }
    /* HKL mode: minimal US-layout translation for the letters only; other
     * keys pass through. (Win32 ToUnicodeEx translation deferred.) */
    if (scancode > 0x60u && scancode < 0x7bu) {
        return scancode - 0x20u;
    }
    return scancode;
}

uint32_t input_key_context(const InputState* s, uint32_t key)
{
    if (s == NULL || key >= INPUT_KEY_COUNT) {
        return 0;
    }
    return s->context_filter[key];
}

void input_text_pump(InputState* s, char* buf, int cap, int* len_out)
{
    if (s == NULL) {
        return;
    }
    if (buf != NULL && len_out != NULL) {
        for (int k = 0; k < INPUT_KEY_COUNT && *len_out < cap; ++k) {
            if (!input_key_edge(s, k)) {
                continue;
            }
            if (input_key_context(s, (uint32_t)k) != 0) {
                continue; /* filtered by the text-input context */
            }
            uint32_t c = input_key_to_ascii(s, (uint32_t)k);
            if (c >= 0x20u && c < 0x7fu) {
                buf[(*len_out)++] = (char)c;
            }
        }
    }
    /* EnginePreTick tail: snapshot + clear. */
    input_mirror_and_clear(s);
}

/* QuitPoll machine — corpus FUN_004589c0. Modeled on finding 05:
 *   mouse inside the corner rectangle  -> enter[slot]++
 *   mouse outside                      -> leave[slot]++
 *   when the counters balance AND the mouse is beyond the quit rectangle,
 *   the game's quit path runs and closes with quitFlag = KeepRunning()==0.
 */
int input_quit_dwell(InputState* s, int mx, int my,
                     int rect_x, int rect_y, int rect_w, int rect_h,
                     int quit_w, int quit_h, int keep_running)
{
    if (s == NULL) {
        return 1;
    }
    int slot = (int)s->device_mode;
    if (slot < 0 || slot >= 5) {
        slot = 0;
    }
    int inside = mx >= rect_x && mx < rect_x + rect_w
              && my >= rect_y && my < rect_y + rect_h;
    if (inside) {
        ++s->dev_enter[slot];
        s->dev_leave[slot] = 0;
    } else {
        ++s->dev_leave[slot];
        /* past the quit rectangle? */
        int past = mx < rect_x || mx > rect_x + rect_w + quit_w
                || my < rect_y || my > rect_y + rect_h + quit_h;
        /* gesture = dwelled in the corner (enter > 0) then left past the
         * edge (leave > 0): the corpus's quit path then probes KeepRunning
         * and only quits when that probe agrees (quitFlag = KeepRunning==0). */
        if (past && s->dev_enter[slot] > 0 && s->dev_leave[slot] > 0) {
            return keep_running == 0 ? 1 : 0;
        }
    }
    return 0;
}