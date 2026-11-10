// SPDX-License-Identifier: LGPL-3.0-or-later
/* gods98 simulation tick: globals that game units read and write.
 *
 * C boundary header (ADR 0005): C declarations, with C linkage, of the
 * simtick functions and globals that units rebuilt as C call. The C++ code keeps
 * using gods98/simtick.hpp, whose matching declarations carry extern "C";
 * src/game/test_c_boundaries.cpp includes both and fails to compile if they
 * disagree. Shared C++ headers do not include this file: every include and
 * declaration a header adds shifts MSVC's symbol numbering, and with it the
 * code generated for unrelated functions (finding 506). */
#ifndef LEGORRR_GODS98_SIMTICK_H
#define LEGORRR_GODS98_SIMTICK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void simulation_global_word_set(uint32_t value);       /* FUN_0041ba90 */

/* DAT_00500860, defined in simtick.cpp: the shared flags word. Bits
 * 0x10..0x80 arm the four index-9 timers; bits 2 and 4 select how
 * Priorities.c reorders the priority list (move to top, or move up one). */
extern uint32_t simtick_timer_word;

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GODS98_SIMTICK_H */
