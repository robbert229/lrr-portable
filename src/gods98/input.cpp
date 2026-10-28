// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>

#include "engine.hpp"
#include "input.hpp"
#include "message.hpp"

void input_reset(InputState* s)
{
    if (s == NULL) {
        return;
    }
    memset(s->cur, 0, sizeof(s->cur));
    memset(s->prev, 0, sizeof(s->prev));
    memset(s->mirror, 0, sizeof(s->mirror));
    memset(s->dwell_enter, 0, sizeof(s->dwell_enter));
    memset(s->dwell_leave, 0, sizeof(s->dwell_leave));
    s->mouse_x = 0;
    s->mouse_y = 0;
    s->hkl_present = 0;
    memset(s->context_filter, 0, sizeof(s->context_filter));
    s->text_target = NULL;
    s->text_len = 0;
    s->text_cap = 0;
}

/* InputStatePoll's successful-device state rollover (the previous/current
 * 256-byte tables) is represented here; the DirectInput acquisition and
 * retry path remain platform-owned. */
// FUNCTION: LEGORR 0x0047eef0
void input_begin_frame(InputState* s)
{
    if (s == NULL) {
        return;
    }
    for (int i = 0; i < INPUT_KEY_COUNT; ++i) {
        s->prev[i] = s->cur[i];
        s->cur[i] = 0;
    }
    s->mouse_press = 0;
}

void input_capture_events(InputState* s, const PlatformEvent* evs, int count)
{
    if (s == NULL || evs == NULL) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        const PlatformEvent* e = &evs[i];
        switch (e->kind) {
        case PLATFORM_EVENT_KEYDOWN:
            if (e->code >= 0 && e->code < INPUT_KEY_COUNT) {
                s->cur[e->code] = 1;
            }
            break;
        case PLATFORM_EVENT_KEYUP:
            if (e->code >= 0 && e->code < INPUT_KEY_COUNT) {
                s->cur[e->code] = 0;
            }
            break;
        case PLATFORM_EVENT_MOUSE:
            s->mouse_x = e->mx;
            s->mouse_y = e->my;
            if (e->button) {
                s->mouse_press = 1;
            }
            s->mouse_down = e->held ? 1 : 0;
            break;
        default:
            break;
        }
    }
}

void input_mirror(InputState* s)
{
    if (s == NULL) {
        return;
    }
    for (int i = 0; i < INPUT_KEY_COUNT; ++i) {
        s->mirror[i] = s->cur[i];
    }
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

int input_mouse_pressed(const InputState* s)
{
    return s != NULL ? s->mouse_press : 0;
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
// FUNCTION: LEGORR 0x0048e380
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

// FUNCTION: LEGORR 0x00411f00
uint32_t input_key_context(const InputState* s, uint32_t key)
{
    if (s == NULL || key >= INPUT_KEY_COUNT) {
        return 0;
    }
    return s->context_filter[key];
}

void input_text_pump(InputState* s, char* buf, int cap, int* len_out, int clear)
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
    if (clear) {
        input_mirror_and_clear(s);
    } else {
        input_mirror(s);
    }
}

/* QuitPoll machine — corpus FUN_004589c0. Modeled on finding 05 (dwell
 * counters) corrected by finding 22 (the word the original indexes off,
 * _DAT_004ffbc0, is the objective/advisor flag word, not an input device):
 *   dwelled in the corner rectangle -> enter[slot]++
 *   left                               -> leave[slot]++
 *   gesture completes when the mouse is past the quit rectangle after a
 *   dwell; the corpus's quit path then probes KeepRunning and only quits
 *   when that probe agrees (quitFlag = KeepRunning()==0).
 */
// FUNCTION: LEGORR 0x004589c0
int input_quit_dwell(InputState* s, int slot, int mx, int my,
                     int rect_x, int rect_y, int rect_w, int rect_h,
                     int quit_w, int quit_h, int keep_running)
{
    if (s == NULL) {
        return 1;
    }
    if (slot < 0 || slot >= 5) {
        slot = 0;
    }
    int inside = mx >= rect_x && mx < rect_x + rect_w
              && my >= rect_y && my < rect_y + rect_h;
    if (inside) {
        ++s->dwell_enter[slot];
        s->dwell_leave[slot] = 0;
    } else {
        ++s->dwell_leave[slot];
        /* past the quit rectangle? */
        int past = mx < rect_x || mx > rect_x + rect_w + quit_w
                || my < rect_y || my > rect_y + rect_h + quit_h;
        if (past && s->dwell_enter[slot] > 0 && s->dwell_leave[slot] > 0) {
            return keep_running == 0 ? 1 : 0;
        }
    }
    return 0;
}
/* --- input-entry / player-input flag-8 leaves (see input.hpp) --------------
 * Zero-initialized module statics with the documented window; the corpus
 * indexes through (possibly heap) pointers blindly. */
#define INPUT_FLAG_TABLE_SLOTS 256

static uint32_t s_input_entry_flags[INPUT_FLAG_TABLE_SLOTS];
static uint32_t s_player_kind1_flags[INPUT_FLAG_TABLE_SLOTS];
static uint32_t s_player_kind4_flags[INPUT_FLAG_TABLE_SLOTS];

// FUNCTION: LEGORR 0x0041c9f0
void input_entry_flag8_set_clear(int entry, int set)
{
    if (entry < 0 || entry >= INPUT_FLAG_TABLE_SLOTS) {
        return; /* deviation: the corpus indexes blindly */
    }
    if (set != 0) {
        s_input_entry_flags[entry] = s_input_entry_flags[entry] | 8u;
    } else {
        s_input_entry_flags[entry] = s_input_entry_flags[entry] & ~8u;
    }
}

// FUNCTION: LEGORR 0x0041ca60
uint32_t input_entry_flag8_get(int entry)
{
    if (entry < 0 || entry >= INPUT_FLAG_TABLE_SLOTS) {
        return 0u;
    }
    return s_input_entry_flags[entry] & 8u;
}

static uint32_t* player_flags_table(int kind)
{
    if (kind == 1) {
        return s_player_kind1_flags;   /* DAT pointer global: table A */
    }
    if (kind == 4) {
        return s_player_kind4_flags;   /* DAT pointer global: table B */
    }
    return NULL;
}

// FUNCTION: LEGORR 0x0041ca20
void player_input_flag8_set_clear(int kind, int action, int set)
{
    /* MEASURED: the inlined kind 1/kind 4 ladder scored 24.0% vs the
     * player_flags_table helper CALL at 45.0% — MSVC5 re-canonicalizes
     * the inline form (b27 helper-route class). Keep the helper. */
    uint32_t* table = player_flags_table(kind);
    if (table == NULL || action < 0 || action >= INPUT_FLAG_TABLE_SLOTS) {
        return; /* deviation: the corpus reads any kind through the table */
    }
    if (set != 0) {
        table[action] = table[action] | 8u;
    } else {
        table[action] = table[action] & ~8u;
    }
}

// FUNCTION: LEGORR 0x0041ca70
uint32_t player_input_flag8_get(int kind, int action)
{
    const uint32_t* table = player_flags_table(kind);
    if (table == NULL || action < 0 || action >= INPUT_FLAG_TABLE_SLOTS) {
        return 0u;
    }
    return table[action] & 8u;
}

/* InputActionPressedGet (FUN_0041c8f0) — see input.hpp. Action 0x49 ('I')
 * is hard-wired 0 before any table read; the gate is the engine-state word
 * read verbatim (bit 0x10000000 — NOT the 0x40000000 the
 * RuntimeFlag40000000Get port masks); alt == the corpus's param_2, and the
 * inverted reading is taken only when the system-state word agrees. The
 * table behind s_input_entry_flags is the corpus's DAT_004dd60c dwords. */
// FUNCTION: LEGORR 0x0041c8f0
int input_action_pressed_get(const EngineContext* ctx, int action, int alt)
{
    if (action == 0x49) {
        return 0;
    }
    if (ctx != NULL && (ctx->state & 0x10000000u) != 0u) {
        return 1; /* deviation: a NULL context reads the gate clear */
    }
    if (action < 0 || action >= INPUT_FLAG_TABLE_SLOTS) {
        return 0; /* deviation: the corpus indexes blindly */
    }
    const uint32_t word = s_input_entry_flags[action];
    if (alt != 0 && (system_state_flags_get() & 1u) != 0u) {
        return (int)((~word >> 2) & 1u);
    }
    return (int)(word & 1u);
}

/* InputBindingPressedGet (FUN_0041c950) — see input.hpp. Same gate and
 * inverted-reading shape, with the kind 1 / kind 4 flag tables (the
 * corpus's DAT_004dd9a4 / DAT_004dd9a8 dwords) selected through
 * player_flags_table; any other kind falls through to not-pressed. */
// FUNCTION: LEGORR 0x0041c950
int input_binding_pressed_get(const EngineContext* ctx, int kind, int entry,
                              int alt)
{
    if (ctx != NULL && (ctx->state & 0x10000000u) != 0u) {
        return 1; /* deviation: a NULL context reads the gate clear */
    }
    const uint32_t* table = player_flags_table(kind);
    if (table == NULL) {
        return 0;
    }
    if (entry < 0 || entry >= INPUT_FLAG_TABLE_SLOTS) {
        return 0; /* deviation: the corpus indexes blindly */
    }
    const uint32_t word = table[entry];
    if (alt != 0 && (system_state_flags_get() & 1u) != 0u) {
        return (int)((~word >> 2) & 1u);
    }
    return (int)(word & 1u);
}

/* IndexedEntityValueGet (FUN_0041cab0) — see input.hpp. The three binding
 * windows stride in the corpus's data section as adjacent 74-dword blocks
 * (DAT_004dcff8 / DAT_004dd120 / DAT_004dd248); their only writer, the
 * interface-images config load, is a recorded file-seam boundary, so the
 * port models the windows as own storage installed through the hook below.
 * Deviations: the corpus indexes blindly (the port answers zero outside
 * its 74-slot window), and the NULL-context gate deviation inherited
 * from InputActionPressedGet (FUN_0041c8f0) applies. */
#define INPUT_INDEXED_WINDOW_SLOTS 74

/* The corpus keeps the three 74-slot windows as individually-addressed
 * tables (DAT_004dcff8 / DAT_004dd120 / DAT_004dd248, contiguous zero-tail
 * band 0x4dcff8..0x4dd370); Reccmp only resolves whole-object operands, so
 * each window gets its own annotated array (the s_fe_flow_* carve-out
 * precedent). Own storage, zero-init like the corpus's tail. */
// GLOBAL: LEGORR 0x004dcff8
static uint32_t s_indexed_window_c[INPUT_INDEXED_WINDOW_SLOTS]; /* default window */
// GLOBAL: LEGORR 0x004dd120
static uint32_t s_indexed_window_a[INPUT_INDEXED_WINDOW_SLOTS]; /* pressed window */
// GLOBAL: LEGORR 0x004dd248
static uint32_t s_indexed_window_b[INPUT_INDEXED_WINDOW_SLOTS]; /* flag8 window */

// FUNCTION: LEGORR 0x0041cab0
uint32_t indexed_entity_value_get(const EngineContext* ctx, int index)
{
    if (index < 0 || index >= INPUT_INDEXED_WINDOW_SLOTS) {
        return 0u; /* deviation: the corpus strides its window blindly */
    }
    if (input_action_pressed_get(ctx, index, 0) != 0) {
        return s_indexed_window_a[index]; /* DAT_004dd120 window */
    }
    if (input_entry_flag8_get(index) != 0u) {
        return s_indexed_window_b[index]; /* DAT_004dd248 window */
    }
    return s_indexed_window_c[index];     /* DAT_004dcff8 window */
}

void input_indexed_window_set(int which, int index, uint32_t value)
{
    uint32_t* window;
    if (which >= 0 && which < 3
        && index >= 0 && index < INPUT_INDEXED_WINDOW_SLOTS) {
        window = NULL;
        switch (which) {
        case 0:  window = s_indexed_window_a; break; /* pressed */
        case 1:  window = s_indexed_window_b; break; /* flag8 */
        default: window = s_indexed_window_c; break; /* default */
        }
        window[index] = value;
    }
}

uint32_t input_indexed_window_peek(int which, int index)
{
    uint32_t* window;
    if (which >= 0 && which < 3
        && index >= 0 && index < INPUT_INDEXED_WINDOW_SLOTS) {
        window = NULL;
        switch (which) {
        case 0:  window = s_indexed_window_a; break; /* pressed */
        case 1:  window = s_indexed_window_b; break; /* flag8 */
        default: window = s_indexed_window_c; break; /* default */
        }
        return window[index];
    }
    return 0u;
}
