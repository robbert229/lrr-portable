// SPDX-License-Identifier: LGPL-3.0-or-later
/* Game command layer: priority-type names and per-priority state.
 *
 * C boundary header (ADR 0005): C declarations, with C linkage, of the
 * command functions that units rebuilt as C call. The C++ code keeps
 * using game/cmd.hpp, whose matching declarations carry extern "C";
 * src/game/test_c_boundaries.cpp includes both and fails to compile if they
 * disagree. Shared C++ headers do not include this file: every include and
 * declaration a header adds shifts MSVC's symbol numbering, and with it the
 * code generated for unrelated functions (finding 506). */
#ifndef LEGORRR_GAME_CMD_H
#define LEGORRR_GAME_CMD_H

#ifdef __cplusplus
extern "C" {
#endif

int cmd_kind_by_name(const char* name, int* out);       /* FUN_00401cb0 */
void priority_entry_order_value_set(int kind, int value); /* FUN_00402090 */
void priority_entry_state_set(int kind, int value);    /* FUN_004020a0 */
int priority_entry_state_get(int kind);                /* FUN_004020b0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_CMD_H */
