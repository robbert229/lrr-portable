// SPDX-License-Identifier: LGPL-3.0-or-later
/* Private to src/gods98/input.cpp and the units relocated out of it (ADR 0006
 * decision 4; tools/relocate_unit.py). Statics both sides use have
 * external linkage through the declarations below. Include it only from
 * those files: it is not a shared header. */
#ifndef LRR_GODS98_INPUT_SHARED_HPP
#define LRR_GODS98_INPUT_SHARED_HPP

extern DirectInputHooks s_direct_input_hooks;
extern InputState s_direct_input_state;
extern InputState* s_active_input_state;
extern int s_direct_input_initialized;
extern int s_direct_input_keyboard_acquired;
extern uint8_t s_global_current_keys[INPUT_KEY_COUNT];
extern int s_cursor_delta_x;
extern int s_cursor_delta_y;
extern InputClientToScreenHook s_input_client_to_screen_hook;
extern InputSetCursorPositionHook s_input_set_cursor_position_hook;
extern void* s_input_cursor_hook_context;
void input_global_current_keys_copy(const InputState* state);

#endif /* LRR_GODS98_INPUT_SHARED_HPP */
