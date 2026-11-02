// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 engine — input state and translation tables (findings 04/05).
 *
 * The corpus's input model is three 256-byte key tables and mouse x/y. The
 * separate DAT_004a1f60 table maps DirectInput scan-code indices to text
 * bytes; its exact retail initializer is in input.cpp.
 *
 *   cur    DAT_0076ab80  current frame snapshot (1 byte per key)
 *   prev   DAT_0076aa00  previous frame snapshot
 *   mirror DAT_004de430  copy made by EnginePreTick's text pump
 *   mouse  DAT_0076ab24/ab28
 *
 * Edge detection everywhere in the corpus is (cur != 0 && prev != cur), i.e.
 * a key that became non-zero this frame — nothing fancier. EnginePreTick
 * (findings 04/05) turns edges into text via KeyContextCheck (context filter)
 * and CRT character conversion, then clears the tables with the mirror trick.
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
 *  - InputState::context_filter is port-local text-pump state. It is not the
 *    retail KeyContextCheck table, which is process-global and initialized
 *    from the shipped 256-entry scan-code-to-character data.
 */
#ifndef LEGORRR_INPUT_HPP
#define LEGORRR_INPUT_HPP

#include <cstdint>
#include <cstddef>

#include "platform/platform.h"  /* PlatformEvent + canonical key codes */

#define INPUT_KEY_COUNT 256
#define INPUT_KEY_MAPPING_KEY_COUNT 128
#define INPUT_KEY_MAPPING_CAPACITY 128

typedef struct InputKeyMappingTable {
    /* The retail record reserves 0x34 bytes before the mapping matrix. */
    uint8_t reserved[0x34];
    uint8_t values[INPUT_KEY_MAPPING_KEY_COUNT][INPUT_KEY_MAPPING_CAPACITY];
    uint8_t count[INPUT_KEY_MAPPING_KEY_COUNT];
} InputKeyMappingTable;

void input_key_mapping_reset(InputKeyMappingTable* table);
void input_key_mapping_add_bidirectional(InputKeyMappingTable* table,
                                         uint8_t key_a, uint8_t key_b);


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
    uint32_t context_filter[INPUT_KEY_COUNT]; /* port-local text filter */

    char*    text_target;             /* active text-entry buffer, else NULL */
    int32_t  text_len;
    int32_t  text_cap;
} InputState;

typedef struct InputActionToggleSlot {
    const void* data;
    int active;
} InputActionToggleSlot;
typedef struct InputActionToggleState {
    InputActionToggleSlot slots[16];
} InputActionToggleState;
typedef int (*InputActionToggleQueryFn)(void* context, int mode);
typedef int (*InputActionToggleApplyFn)(void* context, int mode,
                                        unsigned int action,
                                        const void* data);
typedef struct InputActionToggleHooks {
    void* context;
    InputActionToggleQueryFn query;
    InputActionToggleApplyFn apply;
} InputActionToggleHooks;
int input_action_state_toggle(InputActionToggleState* state,
                              const InputActionToggleHooks* hooks,
                              unsigned int action, const void* data);
typedef struct DirectInputHooks {
    void* context;
    /* Initialize requires interface/device creation, configuration, acquire,
     * and release callbacks. The mouse callback is an optional port extension. */
    int (*interface_create)(void* context);
    int (*keyboard_create)(void* context);
    int (*mouse_create)(void* context);
    int (*devices_configure)(void* context);
    int (*devices_acquire)(void* context);
    void (*devices_release)(void* context);
    /* DirectInput state-read/acquire/unacquire hooks for InputStatePoll.
     * The two status hooks return zero on success. */
    int (*keyboard_state_read)(void* context, uint32_t bytes, uint8_t* keys);
    int (*keyboard_acquire)(void* context);
    void (*keyboard_unacquire)(void* context);
} DirectInputHooks;
void input_direct_hooks_set(const DirectInputHooks* hooks);
int input_direct_initialize(void); /* FUN_0047ed90 */
void input_direct_shutdown(void);
InputState* input_direct_state_get(void);
void input_direct_state_bind(InputState* state);
/* Port adapter for InputStatePoll. Retail takes no arguments and reads
 * DirectInput plus two global polling gates; this helper receives those
 * state inputs explicitly. */
void input_state_poll(InputState* state, int poll_active);

void input_reset(InputState* s);

/* Port frame boundary: roll current keys into prev and clear cur for the
 * next event batch. Edges are then cur != 0 && prev == 0. */
void input_begin_frame(InputState* s);

/* Device capture — the "writer" half (finding 05's device layer, deferred
 * from PR #8 to the frame loop). Consumes platform events into the current
 * frame edge table and the held-key snapshot used by KeyboardDownKeyCount:
 * KEYDOWN sets, KEYUP clears, MOUSE updates mouse_x/y. */
void input_capture_events(InputState* s, const PlatformEvent* evs, int count);

/* EnginePreTick tail: snapshot cur into the text mirror (cur kept for the
 * tick's MainSimStep edges; the frame boundary clears it via
 * input_begin_frame at the end of the loop). */
void input_mirror(InputState* s);

/* Snapshot cur into the text mirror and clear cur. */
void input_mirror_and_clear(InputState* s);

int  input_key_down(const InputState* s, int key);
int  input_keyboard_key_held(int key); /* current 256-byte backend snapshot */
int  input_keyboard_down_key_count(void); /* FUN_0047efb0 */
typedef int (*InputClientToScreenHook)(int* x, int* y, void* context);
typedef void (*InputSetCursorPositionHook)(int x, int y, void* context);
void input_cursor_position_hooks_install(InputClientToScreenHook convert,
                                        InputSetCursorPositionHook set_cursor,
                                        void* context);
void input_cursor_position_update(void); /* FUN_0047f010 */
void input_cursor_position_sync(InputState* state);
void input_cursor_client_position_set(int x, int y); /* FUN_0047f0d0 */
void input_cursor_client_position_get(int* x, int* y);
void input_cursor_delta_get(int* x, int* y);
int  input_key_edge(const InputState* s, int key);
int  input_key_released(const InputState* s, int key);

/* Left-button press edge latched this frame (consumed by input_begin_frame).
 * Returns 1 for one frame after the platform reports MOUSE + button=1. */
int  input_mouse_pressed(const InputState* s);

/* Port-local key-to-ASCII helper: uppercase ASCII lowercase letters and
 * return other values unchanged. The reference 0x48e380 entry is CRT
 * `_toupper`, represented separately in src/crt. */
uint32_t input_key_to_ascii(const InputState* s, uint32_t scancode);

/* Port-local text filter stored per InputState. The retail KeyContextCheck
 * table is a separate process-global scan-code-to-character map. */
uint32_t input_key_context(const InputState* s, uint32_t key);
uint32_t input_key_context_check(int key); /* FUN_00411f00 */

/* EnginePreTick text pump: for each key edge allowed by the context filter,
 * append its ASCII character to buf (capacity cap); when `clear` is set the
 * mirror/+clear tail runs (corpus EnginePreTick), otherwise only the mirror
 * (frame-loop model: the frame boundary clears). Advances *len_out. */
void input_text_pump(InputState* s, char* buf, int cap, int* len_out,
                     int clear);

/* Port-local dwell utility. This is not the retail QuitPoll ABI: the target
 * takes two mode gates and an output pointer, reads global cursor/window and
 * objective state, and may dispatch input/audio/UI operations. That owner
 * state is not represented by InputState. */
int input_quit_dwell(InputState* s, int slot, int mx, int my,
                     int rect_x, int rect_y, int rect_w, int rect_h,
                     int quit_w, int quit_h, int keep_running);

/* --- input-entry / player-input flag-8 leaves (2026-10-24) ----------------
 * InputEntryFlag8SetClear / InputEntryFlag8Get (FUN_0041c9f0 / FUN_0041ca60,
 * findings 376/444): raise/clear/read bit 0x8 of the corpus's indexed
 * input-entry flag word. PlayerInputFlag8SetClear / PlayerInputFlag8Get
 * (FUN_0041ca20 / FUN_0041ca70): the same bit over the two player-input
 * tables selected by kind 1 / kind 4. The entry table is the zero-initialized
 * 74-word DAT_004dd60c array; its indexed leaves follow the corpus's unchecked
 * access. The player tables remain portable zero-initialized stand-ins. */
void input_entry_flag8_set_clear(int entry, int set);
uint32_t input_entry_flag8_get(int entry);
/* Full action/binding words used by InputActionStateUpdate and the catalog
 * slot helpers. Categories 1 and 4 share these same live input tables. */
uint32_t input_action_state_word_get(int action);
void input_action_state_word_set(int action, uint32_t flags);
uint32_t input_binding_state_word_get(int kind, int entry);
void input_binding_state_word_set(int kind, int entry, uint32_t flags);
typedef int (*InputActionStateQueryFn)(void* context, int action);
void input_action_state_query_set(InputActionStateQueryFn query,
                                  void* context);
void input_action_state_update(int action); /* FUN_0041c7b0 */
void player_input_flag8_set_clear(int kind, int action, int set);
uint32_t player_input_flag8_get(int kind, int action);

struct EngineContext;  /* engine.hpp: the DAT_00556ecc state word lives here */

/* Port adapters for InputActionPressedGet / InputBindingPressedGet
 * (FUN_0041c8f0 / FUN_0041c950): "is this action/binding pressed right
 * now?" Retail signatures omit the EngineContext argument and read its
 * global state word. Both read the engine-state word (DAT_00556ecc) bit
 * 0x10000000 as a "pressed-yes-every-
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
