// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98's Input.c (G98-013): the keyboard through DirectInput and the
 * cursor through Win32.
 *
 * C boundary header (ADR 0005): C declarations, with C linkage, of the
 * unit's functions. The C++ code keeps using gods98/input.hpp, whose
 * matching declarations carry extern "C"; src/game/test_c_boundaries.cpp
 * includes both and fails to compile if they disagree. Only input_unit.c
 * and that test include it (finding 506). */
#ifndef LEGORRR_GODS98_INPUT_UNIT_H
#define LEGORRR_GODS98_INPUT_UNIT_H

#ifdef __cplusplus
extern "C" {
#endif

int input_direct_initialize(void);                    /* 0047ed90 */
void input_state_poll_retail(void);                   /* 0047eef0 */
int input_keyboard_down_key_count(void);              /* 0047efb0 */
void input_direct_shutdown(void);                     /* 0047efd0 */
void input_cursor_position_update(void);              /* 0047f010 */
void input_cursor_client_position_set(int x, int y);  /* 0047f0d0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_INPUT_UNIT_H */
