// SPDX-License-Identifier: LGPL-3.0-or-later
#include <string.h>

#include "engine.hpp"
#include "input.hpp"
#include "message.hpp"
#include "crt/crt.hpp"
#include "frameloop.hpp"
#include "input_shared.hpp"

DirectInputHooks s_direct_input_hooks;
InputState s_direct_input_state;
InputState* s_active_input_state;
int s_direct_input_initialized;
int s_direct_input_keyboard_acquired;

void input_direct_hooks_set(const DirectInputHooks* hooks)
{
    if (hooks == NULL) memset(&s_direct_input_hooks, 0,
                              sizeof(s_direct_input_hooks));
    else s_direct_input_hooks = *hooks;
}

InputState* input_direct_state_get(void)
{
    return s_active_input_state != NULL ? s_active_input_state
                                       : &s_direct_input_state;
}

void input_direct_state_bind(InputState* state)
{
    s_active_input_state = state;
}
#include <ctype.h>
#include "input_for_interface.hpp"
#include "input_from_interface.hpp"

uint8_t s_global_current_keys[INPUT_KEY_COUNT];
// GLOBAL: LEGORR 0x0076AB34
int s_cursor_delta_x;
// GLOBAL: LEGORR 0x0076AB38
int s_cursor_delta_y;
InputClientToScreenHook s_input_client_to_screen_hook;
InputSetCursorPositionHook s_input_set_cursor_position_hook;
void* s_input_cursor_hook_context;

void input_global_current_keys_copy(const InputState* state)
{
    /* DirectInput polling supplies a full snapshot. Platform event capture
     * updates this table per key instead, so frame-edge clearing does not
     * make a still-held key appear released. */
    if (state != NULL) {
        memcpy(s_global_current_keys, state->cur, sizeof(s_global_current_keys));
    }
}

void input_key_mapping_reset(InputKeyMappingTable* table)
{
    if (table != NULL) memset(table, 0, sizeof(*table));
}

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
    s->mouse_press = 0;
    s->mouse_down = 0;
    s->pointer_count = 0;
    s->pointer_buttons = 0;
    s->pointer_suppressed = 0;
    s->pointer_overflow = 0;
    s->pointer_domain[0] = s->pointer_domain[1] = INPUT_POINTER_NONE;
    s->pointer_reset_serial = 0u;
    s->hkl_present = 0;
    memset(s->context_filter, 0, sizeof(s->context_filter));
    s->text_target = NULL;
    s->text_len = 0;
    s->text_cap = 0;
    memset(s_global_current_keys, 0, sizeof(s_global_current_keys));
}

/* FUN_00483810: toggle the state associated with one input action. The
 * DirectInput call boundary is supplied as a typed hook; the local slot table
 * preserves the original one-active-registration-per-action behavior. */
int input_action_state_toggle(InputActionToggleState* state,
                              const InputActionToggleHooks* hooks,
                              unsigned int action, const void* data)
{
    InputActionToggleSlot* slot;
    int mode = 0;
    int result;
    if (state == NULL || action >= 16u) return 0;
    if (hooks != NULL && hooks->query != NULL &&
        hooks->query(hooks->context, 0) != 0)
        mode = -1;
    if (hooks != NULL && hooks->apply != NULL) {
        result = hooks->apply(hooks->context, mode, action, data);
        if (result != 0) return 0;
    }
    slot = &state->slots[action];
    if (!slot->active) {
        slot->active = 1;
        slot->data = data;
    }
    else if (slot->data == data) {
        slot->active = 0;
        return 1;
    }
    return 1;
}

/* Port adapter for InputStatePoll (FUN_0047eef0). The portable caller supplies
 * its active state and snapshot; retail reads those through global owners. */
void input_state_poll(InputState* s, int poll_active)
{
    int i;
    int result = -1;
    if (s == NULL) return;
    if (!poll_active) {
        memset(s->cur, 0, sizeof(s->cur));
        memset(s->prev, 0, sizeof(s->prev));
        if (s_direct_input_initialized &&
            s_direct_input_hooks.keyboard_unacquire != NULL)
            s_direct_input_hooks.keyboard_unacquire(
                s_direct_input_hooks.context);
        s_direct_input_keyboard_acquired = 0;
        input_global_current_keys_copy(s);
        return;
    }
    if (!s_direct_input_initialized) return;
    if (s_direct_input_keyboard_acquired &&
        s_direct_input_hooks.keyboard_state_read != NULL) {
        for (i = 0; i < INPUT_KEY_COUNT; ++i) {
            s->prev[i] = s->cur[i];
            s->cur[i] = 0;
        }
        result = s_direct_input_hooks.keyboard_state_read(
            s_direct_input_hooks.context, INPUT_KEY_COUNT, s->cur);
    }
    if (result != 0) {
        s_direct_input_keyboard_acquired = 0;
        if (s_direct_input_hooks.keyboard_acquire != NULL &&
            s_direct_input_hooks.keyboard_acquire(
                s_direct_input_hooks.context) == 0)
            s_direct_input_keyboard_acquired = 1;
    }
    input_global_current_keys_copy(s);
}

/* Port frame boundary; InputStatePoll is a separate device operation. */
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
    s->pointer_count = 0;
    s->pointer_overflow = 0;
}

void input_pointer_rearm(InputState* s)
{
    if (s == NULL) return;
    s->pointer_count = 0;
    s->pointer_suppressed |= s->pointer_buttons;
    s->pointer_domain[0] = INPUT_POINTER_NONE;
    s->pointer_domain[1] = INPUT_POINTER_NONE;
    s->mouse_press = 0;
    ++s->pointer_reset_serial;
}

static void input_pointer_capture(InputState* s, const PlatformEvent* e)
{
    int button, held, transition, previous;
    InputPointerSample* sample;
    if ((e->code & PLATFORM_MOUSE_TAG) == 0) return;
    button = e->code & PLATFORM_MOUSE_BUTTONS;
    held = (e->code >> PLATFORM_MOUSE_HELD_SHIFT) & PLATFORM_MOUSE_BUTTONS;
    transition = e->code & (PLATFORM_MOUSE_DOWN | PLATFORM_MOUSE_UP |
                            PLATFORM_MOUSE_CANCEL);
    previous = s->pointer_buttons;
    if (transition == PLATFORM_MOUSE_CANCEL) {
        /* A focus/capture cancellation invalidates this whole removed batch.
         * No action may be reconstructed from partial history. */
        input_pointer_rearm(s);
        /* Rearm suppresses only buttons actually held before cancellation.
         * An empty held mask must not invent a release for the next click. */
        s->pointer_buttons = 0;
    }
    else {
        /* A button first observed already held has no admitted down edge. */
        s->pointer_suppressed |= held & ~previous &
            ~(transition == PLATFORM_MOUSE_DOWN ? button : 0);
        s->pointer_buttons = held;
        s->pointer_suppressed &= held;
        if (transition == PLATFORM_MOUSE_DOWN && (previous & button) != 0)
            transition = 0; /* duplicate down is not another gesture */
    }
    if (s->pointer_overflow) return;
    if (s->pointer_count >= INPUT_POINTER_CAPACITY) {
        input_pointer_rearm(s);
        s->pointer_overflow = 1;
        return;
    }
    sample = &s->pointer_samples[s->pointer_count++];
    memset(sample, 0, sizeof(*sample));
    sample->x = e->mx;
    sample->y = e->my;
    sample->button = button;
    sample->transition = transition;
    sample->held = held;
    if ((s->pointer_suppressed & button) != 0)
        sample->domain = INPUT_POINTER_BLOCKED;
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
                s_global_current_keys[e->code] = 1;
            }
            break;
        case PLATFORM_EVENT_KEYUP:
            if (e->code >= 0 && e->code < INPUT_KEY_COUNT) {
                s->cur[e->code] = 0;
                s_global_current_keys[e->code] = 0;
            }
            break;
        case PLATFORM_EVENT_MOUSE:
            s->mouse_x = e->mx;
            s->mouse_y = e->my;
            if (e->button) {
                s->mouse_press = 1;
            }
            s->mouse_down = e->held ? 1 : 0;
            input_pointer_capture(s, e);
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

int input_keyboard_key_held(int key)
{
    return key >= 0 && key < INPUT_KEY_COUNT
        ? s_global_current_keys[key] != 0u : 0;
}

void input_cursor_position_hooks_install(InputClientToScreenHook convert,
                                        InputSetCursorPositionHook set_cursor,
                                        void* context)
{
    s_input_client_to_screen_hook = convert;
    s_input_set_cursor_position_hook = set_cursor;
    s_input_cursor_hook_context = context;
}

void input_cursor_position_sync(InputState* state)
{
    if (state == NULL) return;
    state->mouse_x = (int32_t)s_renderer_word_a;
    state->mouse_y = (int32_t)s_renderer_word_b;
}

void input_cursor_client_position_get(int* x, int* y)
{
    if (x != NULL) *x = (int32_t)s_renderer_word_a;
    if (y != NULL) *y = (int32_t)s_renderer_word_b;
}

void input_cursor_delta_get(int* x, int* y)
{
    if (x != NULL) *x = s_cursor_delta_x;
    if (y != NULL) *y = s_cursor_delta_y;
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

/* Port-local key-to-ASCII helper. The reference entry at 0x48e380 is CRT
 * `_toupper`, used by text/string paths; it is not this InputState API.
 */
uint32_t input_key_to_ascii(const InputState* s, uint32_t scancode)
{
    if (s == NULL || s->hkl_present == 0) {
        if (scancode > 0x60u && scancode < 0x7bu) {
            return (uint32_t)toupper((int)scancode);
        }
        return scancode;
    }
    /* HKL mode: minimal US-layout translation for the letters only; other
     * keys pass through. (Win32 ToUnicodeEx translation deferred.) */
    if (scancode > 0x60u && scancode < 0x7bu) {
        return (uint32_t)toupper((int)scancode);
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

/* Port-local dwell utility. It is not the retail FUN_004589c0 routine, which
 * consumes global UI/cursor state and dispatches several unrelated paths.
 * This helper uses the finding 05 dwell-counter approximation:
 *   dwelled in the corner rectangle -> enter[slot]++
 *   left                               -> leave[slot]++
 *   gesture completes when the mouse is past the quit rectangle after a
 *   dwell; the corpus's quit path then probes KeepRunning and only quits
 *   when that probe agrees (quitFlag = KeepRunning()==0).
 */
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

static uint32_t s_player_kind1_flags[INPUT_FLAG_TABLE_SLOTS];
static uint32_t s_player_kind4_flags[INPUT_FLAG_TABLE_SLOTS];
uint32_t s_player_kind1_click_counts[INPUT_FLAG_TABLE_SLOTS];
uint32_t s_player_kind4_click_counts[INPUT_FLAG_TABLE_SLOTS];
uint32_t* player_flags_table(int kind);
InputActionStateQueryFn s_input_action_state_query;
void* s_input_action_state_query_context;

uint32_t input_action_state_word_get(int action)
{
    if (action < 0 || action >= INPUT_FLAG_TABLE_SLOTS) return 0u;
    return s_input_entry_flags[action];
}

void input_action_state_word_set(int action, uint32_t flags)
{
    if (action < 0 || action >= INPUT_FLAG_TABLE_SLOTS) return;
    s_input_entry_flags[action] = flags;
}

uint32_t input_binding_state_word_get(int kind, int entry)
{
    uint32_t* table = player_flags_table(kind);
    if (table == NULL || entry < 0 || entry >= INPUT_FLAG_TABLE_SLOTS)
        return 0u;
    return table[entry];
}

void input_binding_state_word_set(int kind, int entry, uint32_t flags)
{
    uint32_t* table = player_flags_table(kind);
    if (table == NULL || entry < 0 || entry >= INPUT_FLAG_TABLE_SLOTS)
        return;
    table[entry] = flags;
}

/* Called only after the real pointer latch admits a counted completion,
 * before the action effect is dispatched. A rejected effect still counts. */
void input_action_completion_record(int action, int current_mode, int entry)
{
    if (action != 73) {
        (void)input_action_click_count_swap(action,
            input_action_click_count_get(action) + 1u);
    }
    else if (current_mode == 0x13) {
        (void)input_binding_click_count_swap(4, entry,
            input_binding_click_count_get(4, entry) + 1u);
    }
    else if (current_mode == 0x14 || current_mode == 0x15) {
        (void)input_binding_click_count_swap(1, entry,
            input_binding_click_count_get(1, entry) + 1u);
    }
}

void input_action_level_state_clear(void)
{
    memset(s_input_action_click_counts, 0, sizeof(s_input_action_click_counts));
    memset(s_player_kind1_click_counts, 0, sizeof(s_player_kind1_click_counts));
    memset(s_player_kind4_click_counts, 0, sizeof(s_player_kind4_click_counts));
    memset(s_input_entry_flags, 0, sizeof(s_input_entry_flags));
    memset(s_player_kind1_flags, 0, sizeof(s_player_kind1_flags));
    memset(s_player_kind4_flags, 0, sizeof(s_player_kind4_flags));
}

void input_action_state_query_set(InputActionStateQueryFn query,
                                  void* context)
{
    s_input_action_state_query = query;
    s_input_action_state_query_context = context;
}

uint32_t* player_flags_table(int kind)
{
    if (kind == 1) {
        return s_player_kind1_flags;   /* DAT pointer global: table A */
    }
    if (kind == 4) {
        return s_player_kind4_flags;   /* DAT pointer global: table B */
    }
    return NULL;
}

/* InputActionPressedGet (FUN_0041c8f0) — see input.hpp. Action 0x49 ('I')
 * is hard-wired 0 before any table read; the gate is the engine-state word
 * read verbatim (bit 0x10000000 — NOT the 0x40000000 the
 * RuntimeFlag40000000Get port masks); alt == the corpus's param_2, and the
 * inverted reading is taken only when the system-state word agrees. The
 * table behind s_input_entry_flags is the corpus's DAT_004dd60c dwords. */
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
    /* Corpus 0x41c927 and 0x41c938 loads the table word separately in each
     * result arm, after the optional system-state getter call. */
    if (alt != 0 && (system_state_flags_get() & 1u) != 0u) {
        return (int)((~s_input_entry_flags[action] >> 2) & 1u);
    }
    return (int)(s_input_entry_flags[action] & 1u);
}

/* InputBindingPressedGet (FUN_0041c950) — see input.hpp. Same gate and
 * inverted-reading shape, with the kind 1 / kind 4 flag tables (the
 * corpus's DAT_004dd9a4 / DAT_004dd9a8 dwords) selected through
 * player_flags_table; any other kind falls through to not-pressed. */
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

namespace unported_ledger_stubs {

static InputActionToggleState s_input_action_toggle_state;

// FUNCTION: LEGORR 0x00483810
int ledger_stub_00483810_InputActionStateToggle(unsigned int action,
                                                 const void* data)
{
    return input_action_state_toggle(&s_input_action_toggle_state, NULL,
                                     action, data);
}





} // namespace unported_ledger_stubs
