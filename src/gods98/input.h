// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 input layer: cursor placement.
 *
 * C boundary header (ADR 0005): C declarations, with C linkage, of the
 * input functions that units rebuilt as C call. The C++ code keeps
 * using gods98/input.hpp, whose matching declarations carry extern "C";
 * src/game/test_c_boundaries.cpp includes both and fails to compile if they
 * disagree. Shared C++ headers do not include this file: every include and
 * declaration a header adds shifts MSVC's symbol numbering, and with it the
 * code generated for unrelated functions (finding 506). */
#ifndef LEGORRR_GODS98_INPUT_H
#define LEGORRR_GODS98_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

void input_cursor_client_position_set(int x, int y);   /* FUN_0047f0d0 */

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_INPUT_H */
