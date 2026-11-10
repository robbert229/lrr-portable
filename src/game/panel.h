// SPDX-License-Identifier: LGPL-3.0-or-later
/* Panel.c functions (original unit GAME-032, finding 505) that other
 * units call. They are still defined in crt/crt.cpp and game/frontend.cpp.
 *
 * C boundary header (ADR 0005): C declarations, with C linkage, of the
 * panel functions that units rebuilt as C call. The C++ code keeps
 * using crt/crt.hpp and game/frontend.hpp, whose matching declarations carry extern "C";
 * src/game/test_c_boundaries.cpp includes both and fails to compile if they
 * disagree. Shared C++ headers do not include this file: every include and
 * declaration a header adds shifts MSVC's symbol numbering, and with it the
 * code generated for unrelated functions (finding 506). */
#ifndef LEGORRR_GAME_PANEL_H
#define LEGORRR_GAME_PANEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Panel record flag tests (FUN_0045ab30 / FUN_0045ab90): the retail
 * bodies, without the port's index guard. */
int request_record_ready_flag4_target(int index);
int request_record_flag8_is_set_target(int index);

/* Panel position (FUN_0045a700). The original stores floats; this
 * reconstruction stores the pair as integers (finding 506). */
void indexed_pair_get(int index, uint32_t* out_x, uint32_t* out_y);

/* Panel button area (FUN_0045bf70): x, y, width, height. */
void ui_entry_bounds_get(int panel, int item, float* out_bounds);

#ifdef __cplusplus
}
#endif

#endif /* LEGORRR_GAME_PANEL_H */
